/*
    Peripheral expansion bus support.

    The ti-99/4, ti-99/4a, ti computer 99/8, myarc geneve, and snug sgcpu
    99/4p systems all feature a bus connector that enables the connection of
    extension cards.  (Although the hexbus is the preferred bus to add
    additional peripherals to a ti-99/8, ti-99/8 is believed to be compatible
    with the older PEB system.)  In the case of the TI consoles, this bus
    connector is located on the right side of the console.

    While a few extension cards connect to the side bus connector of the
    ti-99/4(a) console directly, most extension cards were designed to be
    inserted in a PEB instead.  The PEB (Peripheral Expansion Box) is a big box
    with an alimentation, a few bus drivers, and several card slots, that
    connects to the ti-99/4(a) side port.  The reason for using a PEB is that
    daisy-chaining many modules caused the system to be unreliable due to the
    noise produced by the successive contacts.  (As a matter of fact, TI
    initially released most of its extension cards as side bus units, but when
    the design proved to be unreliable, the PEB was introduced.  The TI speech
    synthesizer was the only TI extension that remained on the side bus after
    the introduction of the PEB, probably because TI wanted the speech
    synthesizer to be a cheap extension, and the PEB was not cheap.)

    The third-party myarc geneve and snug sgcpu computers are actually cards
    to be inserted into a PEB: they have a PC keyboard connector, and replace
    the original ti-99 console completely.

    Each expansion card is assigned to one of 16 CRU address ranges.
    I appended the names of known TI peripherals that use each port (I appended
    '???' when I don't know what the peripheral is :-) ).
    * 0x1000-0x10FE "For test equipment use on production line"
    * 0x1100-0x11FE disk controller
    * 0x1200-0x12FE modem???
    * 0x1300-0x13FE RS232 1&2, PIO 1
    * 0x1400-0x14FE unassigned
    * 0x1500-0x15FE RS232 3&4, PIO 2
    * 0x1600-0x16FE unassigned
    * 0x1700-0x17FE hex-bus (prototypes)
    * 0x1800-0x18FE thermal printer (early peripheral)
    * 0x1900-0x19FE EPROM programmer??? (Mezzanine board in July 1981's TI99/7
        prototype)
    * 0x1A00-0x1AFE unassigned
    * 0x1B00-0x1BFE TI GPL debugger card
    * 0x1C00-0x1CFE Video Controller Card (Possibly the weirdest device.  This
        card is connected to the video output of the computer, to a VCR, and a
        video monitor.  It can control the VCR, connect the display to either
        VCR output or computer video output, and it can read or save binary
        data to video tape.  I think it can act as a genlock interface (i.e.
        TMS9918 transparent background shows the video signal), too, but I am
        not sure about this.)
    * 0x1D00-0x1DFE IEEE 488 Controller Card ('intelligent' parallel bus,
        schematics on ftp.whtech.com)
    * 0x1E00-0x1EFE unassigned
    * 0x1F00-0x1FFE P-code card (part of a complete development system)

    The ti-99/8 implements 16 extra ports:
    * 0x2000-0x26ff and 0x2800-0x2fff: reserved for future expansion?
    * 0x2700-0x27ff: internal DSR

    The snug sgcpu 99/4p implements 12 extra ports:
    * 0x0400-0x0eff: free
    * 0x0f00-0x0fff: internal DSR

    Known mappings for 3rd party cards:
    * Horizon RAMdisk: any ports from 0 to 7 (port 0 is most common).
    * Myarc 128k/512k (RAM and DSR ROM): port 0 (0x1000-0x11FE)
    * Corcomp 512k (RAM and DSR ROM): port unknown
    * Super AMS 128k/512k: port 14 (0x1E00-0x1EFE)
    * Foundation 128k/512k: port 14 (0x1E00-0x1EFE)
    * Gram Karte: any port (0-15)
    * EVPC (video card): 0x1400-0x14FE
    * HSGPL (GROM replacement): 0x1B00-0x1BFE

    Of course, these devices additionally need some support routines, and
    possibly memory-mapped I/O registers.  To do so, memory range 0x4000-5FFF
    is shared by all cards.  The system enables each card as needed by writing
    a 1 to the first CRU bit: when this happens the card can safely enable its
    ROM, RAM, memory-mapped registers, etc.  Provided the ROM uses the proper
    ROM header, the system will recognize and call peripheral I/O functions,
    interrupt service routines, and startup routines.  (All these routines are
    generally called DSR = Device Service Routine.)

    Also, the cards can trigger level-1 ("INTA") interrupts.  A LOAD interrupt
    is present on the side port of the ti-99/4(a), too, but it was never used
    by TI, AFAIK.  Also, an interrupt line called "INTB" is found on several
    cards built by TI, but it is not connected: I am quite unsure whether it
    was intended to be connected to the LOAD line, to the INT2 line of TMS9985,
    or it was only to be used with the ti-99/4(a) development system running on
    a TI990/10.  This is not the only unused feature: cards built by TI support
    sensing of an interrupt status register and 19-bit addressing, even though
    ti-99/4(A) does not take advantage of these features.  TI documentation
    tells that some of these features are "defined by the Personal Computer PCC
    at Texas Instrument", which is not very explicit (What on earth does "PCC"
    stand for?).  Maybe it refers to the ti-99/4(a) development system running
    on a TI990/10, or to the ti-99/7 and ti-99/8 prototypes.

    Note that I use 8-bit RAM handlers.  Obviously, using 16-bit handlers would
    be equivalent and faster on a ti-99/4(a), but we need to interface the
    extension cards not only to the ti-99/4(a), but to the geneve and ti-99/8
    emulators as well.

    June 2010: Reimplemented using device structure (MZ) (obsoletes 99_peb.c)
*/

#include "emu.h"
#include "ti99_peb.h"

enum width_t {
	width_8bit = 0,
	width_16bit
};
/* expansion card structure for the snug sgcpu 99/4p system, which supports
dynamical 16-bit accesses (TI did not design this!) */
typedef struct ti99_4p_peb_card_handlers_t
{
	cru_read_handler cru_read;		/* card CRU read handler */
	cru_write_handler cru_write;	/* card CRU handler */

	width_t width;
	union
	{
		struct
		{
			read8_space_func mem_read;		/* card mem read handler (8 bits) */
			write8_space_func mem_write;	/* card mem write handler (8 bits) */
		} width_8bit;
		struct
		{
			read16_space_func mem_read;	/* card mem read handler (16 bits) */
			write16_space_func mem_write;	/* card mem write handler (16 bits) */
		} width_16bit;
	} w;
} ti99_4p_peb_card_handlers_t;

typedef struct _ti99_peb_state
{
	/* TRUE if we are using the snug sgcpu 99/4p 16-bit extensions */
	int has_16bit_peb;

	/* handlers for each of 28 slots */
	ti99_4p_peb_card_handlers_t ti99_4p_expansion_ports[28];

	/* handlers for each of 16 slots + 16 extra slots for ti-99/8 */
	ti99_peb_card_handlers_t expansion_ports[16+16];

	/* index of the currently active card (-1 if none) */
	int active_card;

	/* index of the currently active card, overridden by the mapper */
	int active_card_tmp;

	/* Mapper override. */
	int mapper_override;

	// ila: inta status register (not actually used on ti-99/4(a), but nevertheless
	// present)
	int ila;

	// ilb: intb status register (completely pointless on ti-99/4(a), as neither
	// INTB nor SENILB are connected; OTOH, INTB can trigger interrupts on a Geneve,
	// and snug sgcpu 99/4p can sense ILB)
	int ilb;

	/* only used by the snug sgcpu 99/4p */
	int senila, senilb;

	/* hack to simulate TMS9900 byte write */
	int tmp_buffer;

	/* inta/intb handlers */
	void (*inta_callback)(running_machine *machine, int state);
	void (*intb_callback)(running_machine *machine, int state);

	address_space *space;

} ti99_peb_state;

INLINE ti99_peb_state *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(downcast<legacy_device_base *>(device)->token() != NULL);
	return (ti99_peb_state *)downcast<legacy_device_base *>(device)->token();
}

INLINE const ti99_peb_config *get_config(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == PBOX);
	return (const ti99_peb_config *) downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

/*
    Sets the handlers for one expansion port (normal 8-bit card)

    cru_base: CRU base address (any of 0x1000, 0x1100, 0x1200, ..., 0x1F00)
        (snug sgcpu 99/4p accepts 0x0400, ..., 0x1F00, ti-99/8 accepts 0x1000,
        ..., 0x2F00)
    handler: handler structure for the given card
*/
void ti99_peb_set_card_handlers(running_device *box, int cru_base, const ti99_peb_card_handlers_t *handler)
{
	int port;
	ti99_peb_state *peb = get_safe_token(box);

	if (cru_base & 0xff)
		return;

	if (peb->has_16bit_peb)
	{
		port = (cru_base - 0x0400) >> 8;

		if ((port>=0) && (port<28))
		{
			peb->ti99_4p_expansion_ports[port].cru_read = handler->cru_read;
			peb->ti99_4p_expansion_ports[port].cru_write = handler->cru_write;
			peb->ti99_4p_expansion_ports[port].width = width_8bit;
			peb->ti99_4p_expansion_ports[port].w.width_8bit.mem_read = handler->mem_read;
			peb->ti99_4p_expansion_ports[port].w.width_8bit.mem_write = handler->mem_write;
		}
	}
	else
	{
		port = (cru_base - 0x1000) >> 8;

		if ((port>=0) && (port</*16*/32))
		{
			peb->expansion_ports[port] = *handler;
		}
	}
}

/*
    Sets the handlers for one expansion port (special 16-bit card for snug
    sgcpu 99/4p)

    cru_base: CRU base address (any of 0x1000, 0x1100, 0x1200, ..., 0x1F00)
        (snug sgcpu 99/4p accepts 0x0400, ..., 0x1F00, ti-99/8 accepts 0x1000,
        ..., 0x2F00)
    handler: handler structure for the given card
*/
void ti99_peb_set_16bit_card_handlers(running_device *box, int cru_base, const ti99_peb_16bit_card_handlers_t *handler)
{
	int port;
	ti99_peb_state *peb = get_safe_token(box);

	if (cru_base & 0xff)
		return;

	if (peb->has_16bit_peb)
	{
		port = (cru_base - 0x0400) >> 8;

		if ((port>=0) && (port<28))
		{
			peb->ti99_4p_expansion_ports[port].cru_read = handler->cru_read;
			peb->ti99_4p_expansion_ports[port].cru_write = handler->cru_write;
			peb->ti99_4p_expansion_ports[port].width = width_16bit;
			peb->ti99_4p_expansion_ports[port].w.width_16bit.mem_read = handler->mem_read;
			peb->ti99_4p_expansion_ports[port].w.width_16bit.mem_write = handler->mem_write;
		}
	}
}

/*
    Update ila status register and assert or clear INTA interrupt line
    accordingly.

    bit: bit number ([0,7] for original INTA register, [8,15] are extra
        "virtual" bits for devices that assert the INTA line without setting a
        bit of the ILA register)
    state: 1 to assert bit, 0 to clear
*/
void ti99_peb_set_ila_bit(running_device *box, int bit, int state)
{
	ti99_peb_state *peb = get_safe_token(box);
	if (state)
	{
		peb->ila |= 1 << bit;
		if (peb->inta_callback)
			(*peb->inta_callback)(box->machine, 1);
	}
	else
	{
		peb->ila &= ~(1 << bit);
		if ((!peb->ila) && peb->inta_callback)
			(*peb->inta_callback)(box->machine, 0);
	}
}

/*
    Update ilb status register and assert or clear INTB interrupt line
    accordingly.

    bit: bit number ([0,7] for original INTAB register, [8,15] are extra
        "virtual" bits for devices that assert the INTB line without setting a
        bit of the ILB register)
    state: 1 to assert bit, 0 to clear
*/
void ti99_peb_set_ilb_bit(running_device *box, int bit, int state)
{
	ti99_peb_state *peb = get_safe_token(box);

	if (state)
	{
		peb->ilb |= 1 << bit;
		if (peb->intb_callback)
			(*peb->intb_callback)(box->machine, 1);
	}
	else
	{
		peb->ilb &= ~(1 << bit);
		if ((!peb->ilb) && peb->intb_callback)
			(*peb->intb_callback)(box->machine, 0);
	}
}


/*
    Read CRU in range >1000->1ffe (>800->fff) (ti-99/4(a))
*/
READ8_DEVICE_HANDLER( ti99_4x_peb_cru_r )
{
	int port;
	cru_read_handler handler;
	int reply;
	ti99_peb_state *peb = get_safe_token(device);

	port = (offset & 0x00f0) >> 4;
	handler = peb->expansion_ports[port].cru_read;

	reply = (handler) ? (*handler)(device->machine, offset & 0xf) : 0;

	return reply;
}

/*
    Write CRU in range >1000->1ffe (>800->fff) (ti-99/4(a))
*/
WRITE8_DEVICE_HANDLER( ti99_4x_peb_cru_w )
{
	int port;
	cru_write_handler handler;
	ti99_peb_state *peb = get_safe_token(device);

	port = (offset & 0x0780) >> 7;
	handler = peb->expansion_ports[port].cru_write;

	if (handler)
		(*handler)(device->machine, offset & 0x7f, data);

	/* expansion card enable? */
	if ((offset & 0x7f) == 0)
	{
		if (data & 1)
		{
			/* enable */
			peb->active_card = port;
		}
		else
		{
			if (port == peb->active_card)	/* geez... who cares? */
			{
				peb->active_card = -1;			/* no port selected */
			}
		}
	}
}

/*
    Read mem in range >4000->5ffe (ti-99/4(a))
*/
READ16_DEVICE_HANDLER( ti99_4x_peb_r )
{
	int reply = 0;
	read8_space_func handler;
	ti99_peb_state *peb = get_safe_token(device);

	cpu_adjust_icount(device->machine->firstcpu,-4);

	if (peb->active_card != -1)
	{
		handler = peb->expansion_ports[peb->active_card].mem_read;
		if (handler)
		{
			reply = (*handler)(peb->space, (offset << 1) + 1);
			reply |= ((unsigned) (*handler)(peb->space, offset << 1)) << 8;
		}
	}
//  printf("[%04x:%04x] = %04x\n", (active_card<<8)+0x1000, (offset<<1)+0x4000, reply);

	return peb->tmp_buffer = reply;
}

/*
    Write mem in range >4000->5ffe (ti-99/4(a))
*/
WRITE16_DEVICE_HANDLER( ti99_4x_peb_w )
{
	write8_space_func handler;
	ti99_peb_state *peb = get_safe_token(device);

	cpu_adjust_icount(device->machine->firstcpu,-4);

	/* simulate byte write */
	data = (peb->tmp_buffer & ~mem_mask) | (data & mem_mask);

	if (peb->active_card != -1)
	{
		handler = peb->expansion_ports[peb->active_card].mem_write;
		if (handler)
		{
			(*handler)(peb->space, (offset << 1) + 1, data & 0xff);
			(*handler)(peb->space, offset << 1, (data >> 8) & 0xff);
		}
	}
}


/*
    Read CRU in range >1000->1ffe (>800->fff) (Geneve)
*/
READ8_DEVICE_HANDLER( geneve_peb_cru_r )
{
	int port;
	cru_read_handler handler;
	int reply;
	ti99_peb_state *peb = get_safe_token(device);

	port = (offset & 0x00f0) >> 4;
	handler = peb->expansion_ports[port].cru_read;

	reply = (handler) ? (*handler)(device->machine, offset & 0xf) : 0;

	return reply;
}

/*
    Write CRU in range >1000->1ffe (>800->fff) (Geneve)
*/
WRITE8_DEVICE_HANDLER( geneve_peb_cru_w )
{
	int port;
	cru_write_handler handler;
	ti99_peb_state *peb = get_safe_token(device);

	port = (offset & 0x0780) >> 7;
	handler = peb->expansion_ports[port].cru_write;

	if (handler)
		(*handler)(device->machine, offset & 0x7f, data);

	/* expansion card enable? */
	if ((offset & 0x7f) == 0)
	{
		if (data & 1)
		{
			/* enable */
			peb->active_card = port;
		}
		else
		{
			if (port == peb->active_card)	/* geez... who cares? */
			{
				peb->active_card = -1;			/* no port selected */
			}
		}
	}
}

/*
    Read mem in range >4000->5fff (Geneve)
*/
READ8_DEVICE_HANDLER( geneve_peb_r )
{
	int reply = 0;
	read8_space_func handler;
	ti99_peb_state *peb = get_safe_token(device);

	cpu_adjust_icount(device->machine->firstcpu,-8);

	if (peb->active_card != -1)
	{
		handler = peb->expansion_ports[peb->active_card].mem_read;
		if (handler)
			reply = (*handler)(peb->space, offset);
	}

	return reply;
}

/*
    Write mem in range >4000->5fff (Geneve)
*/
WRITE8_DEVICE_HANDLER( geneve_peb_w )
{
	write8_space_func handler;
	ti99_peb_state *peb = get_safe_token(device);

	cpu_adjust_icount(device->machine->firstcpu,-8);

	if (peb->active_card != -1)
	{
		handler = peb->expansion_ports[peb->active_card].mem_write;
		if (handler)
			(*handler)(peb->space, offset, data);
	}
}

/*
    Read CRU in range >1000->2ffe (>0800->17ff) (ti-99/8)
*/
READ8_DEVICE_HANDLER( ti99_8_peb_cru_r )
{
	int port;
	cru_read_handler handler;
	int reply;
	ti99_peb_state *peb = get_safe_token(device);

	port = (offset & 0x01f0) >> 4;
	handler = peb->expansion_ports[port].cru_read;

	reply = (handler) ? (*handler)(device->machine, offset & 0xf) : 0;

	return reply;
}

/*
    Write CRU in range >1000->2ffe (>0800->17ff) (ti-99/8)
*/
WRITE8_DEVICE_HANDLER( ti99_8_peb_cru_w )
{
	int port;
	cru_write_handler handler;
	ti99_peb_state *peb = get_safe_token(device);

	port = (offset & 0x0f80) >> 7;
	handler = peb->expansion_ports[port].cru_write;

	if (handler)
		(*handler)(device->machine, offset & 0x7f, data);

	/* expansion card enable? */
	if ((offset & 0x7f) == 0)
	{
		if (data & 1)
		{
			/* enable */
			peb->active_card = port;
		}
		else
		{
			if (port == peb->active_card)	/* geez... who cares? */
			{
				peb->active_card = -1;			/* no port selected */
			}
		}
	}
}

/*
    Read mem in range >4000->5fff (ti-99/8)
*/
READ8_DEVICE_HANDLER( ti99_8_peb_r )
{
	int reply = 0;
	read8_space_func handler;
	ti99_peb_state *peb = get_safe_token(device);

	cpu_adjust_icount(device->machine->firstcpu,-4);

	if (peb->active_card != -1)
	{
		handler = peb->expansion_ports[peb->active_card].mem_read;
		if (handler)
			reply = (*handler)(peb->space, offset);
	}

	return reply;
}

/*
    Write mem in range >4000->5fff (ti-99/8)
*/
WRITE8_DEVICE_HANDLER( ti99_8_peb_w )
{
	write8_space_func handler;
	ti99_peb_state *peb = get_safe_token(device);

	cpu_adjust_icount(device->machine->firstcpu,-4);

	if (peb->active_card != -1)
	{
		handler = peb->expansion_ports[peb->active_card].mem_write;
		if (handler)
			(*handler)(peb->space, offset, data);
	}
}

/*
    Read CRU in range >0400->1ffe (>200->fff) (snug sgcpu 99/4p)
*/
READ8_DEVICE_HANDLER( ti99_4p_peb_cru_r )
{
	int port;
	cru_read_handler handler;
	int reply;
	ti99_peb_state *peb = get_safe_token(device);

	port = offset >> 4;
	handler = peb->ti99_4p_expansion_ports[port].cru_read;

	reply = (handler) ? (*handler)(device->machine, offset & 0xf) : 0;

	return reply;
}

/*
    Write CRU in range >0400->1ffe (>200->fff) (snug sgcpu 99/4p)
*/
WRITE8_DEVICE_HANDLER( ti99_4p_peb_cru_w )
{
	int port;
	cru_write_handler handler;
	ti99_peb_state *peb = get_safe_token(device);

	int cru_address = (offset + 0x200)<<1;

	port = offset >> 7;
	handler = peb->ti99_4p_expansion_ports[port].cru_write;

	if (handler)
		(*handler)(device->machine, offset & 0x7f, data);

	/* expansion card enable? */
	if ((cru_address & 0xfe) == 0)
	{
		if (data & 1)
		{
			/* enable */
			if (cru_address == 0x1e00)
			{
				// The internal mapper has precedence over the external bus.
				// It may be turned on although there is another device currently turned on
				peb->active_card_tmp = peb->active_card;
				peb->mapper_override = TRUE;
			}
			peb->active_card = port;
		}
		else
		{
			if ((cru_address == 0x1e00) && peb->mapper_override)
			{
				peb->active_card = peb->active_card_tmp;
				peb->mapper_override = FALSE;
			}
			else
			{
				if (port == peb->active_card)	/* geez... who cares? */
				{
					peb->active_card = -1;			/* no port selected */
				}
			}
		}
	}
}

/*
    Read mem in range >4000->5ffe (snug sgcpu 99/4p)
*/
READ16_DEVICE_HANDLER( ti99_4p_peb_r )
{
	int reply = 0;
	read8_space_func handler;
	read16_space_func handler16;
	ti99_peb_state *peb = get_safe_token(device);

	if (peb->active_card == -1)
		cpu_adjust_icount(device->machine->firstcpu,-4);	/* ??? */
	else
	{
		if (peb->ti99_4p_expansion_ports[peb->active_card].width == width_8bit)
		{
			cpu_adjust_icount(device->machine->firstcpu,-4);

			handler = peb->ti99_4p_expansion_ports[peb->active_card].w.width_8bit.mem_read;
			if (handler)
			{
				reply = (*handler)(peb->space, (offset << 1) + 1);
				reply |= ((unsigned) (*handler)(peb->space, offset << 1)) << 8;
			}
		}
		else
		{
			cpu_adjust_icount(device->machine->firstcpu,-1);	/* ??? */

			handler16 = peb->ti99_4p_expansion_ports[peb->active_card].w.width_16bit.mem_read;
			if (handler16)
				reply = (*handler16)(peb->space, offset, /*mem_mask*/0xffff);
		}
	}

	if (peb->senila || peb->senilb)
	{
		if ((peb->active_card != -1) || (peb->senila && peb->senilb))
			/* ah, the smell of burnt silicon... */
			logerror("<Scrrrrr>: your computer has just burnt (maybe).\n");

		if (peb->senila)
			reply = (peb->ila & 0xff) | (peb->ila << 8);
		else if (peb->senilb)
			reply = (peb->ilb & 0xff) | (peb->ilb << 8);
	}

//  printf("[%04x:%04x] = %04x\n", (active_card<<8)+0x400, (offset<<1)+0x4000, reply);
	return peb->tmp_buffer = reply;
}

/*
    Write mem in range >4000->5ffe (snug sgcpu 99/4p)
*/
WRITE16_DEVICE_HANDLER( ti99_4p_peb_w )
{
	write8_space_func handler;
	write16_space_func handler16;
	ti99_peb_state *peb = get_safe_token(device);

	if (peb->active_card == -1)
		cpu_adjust_icount(device->machine->firstcpu,-4);	/* ??? */
	else
	{
		/* simulate byte write */
		data = (peb->tmp_buffer & ~mem_mask) | (data & mem_mask);

		if (peb->ti99_4p_expansion_ports[peb->active_card].width == width_8bit)
		{
			cpu_adjust_icount(device->machine->firstcpu,-4);

			handler = peb->ti99_4p_expansion_ports[peb->active_card].w.width_8bit.mem_write;
			if (handler)
			{
				(*handler)(peb->space, (offset << 1) + 1, data & 0xff);
				(*handler)(peb->space, offset << 1, (data >> 8) & 0xff);
			}
		}
		else
		{
			cpu_adjust_icount(device->machine->firstcpu,-1);	/* ??? */

			handler16 = peb->ti99_4p_expansion_ports[peb->active_card].w.width_16bit.mem_write;
			if (handler16)
				(*handler16)(peb->space, offset, data, /*mem_mask*/0xffff);
		}
	}
}

/*
    Set the state of the SENILA line (snug sgcpu 99/4p)
*/
void ti99_4p_peb_set_senila(running_device *box, int state)
{
	ti99_peb_state *peb = get_safe_token(box);
	peb->senila = (state != 0);
}

/*
    Set the state of the SENILB line (snug sgcpu 99/4p)
*/
void ti99_4p_peb_set_senilb(running_device *box, int state)
{
	ti99_peb_state *peb = get_safe_token(box);
	peb->senilb = (state != 0);
}


/***************************************************************************
    DEVICE FUNCTIONS
***************************************************************************/

static DEVICE_START( ti99_peb )
{
}

static DEVICE_STOP( ti99_peb )
{
}

static DEVICE_RESET( ti99_peb )
{
	ti99_peb_state *peb = get_safe_token(device);

	peb->has_16bit_peb = get_config(device)->mode16;
	peb->inta_callback = get_config(device)->inta_callback;
	peb->intb_callback = get_config(device)->intb_callback;

	peb->active_card = -1;
	peb->active_card_tmp = -1;

	peb->mapper_override = FALSE; // for ti99_4p

	peb->ila = 0;
	peb->ilb = 0;

	peb->space = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	// Remove all card handlers; must be reinstalled
	memset(peb->expansion_ports, 0, sizeof(peb->expansion_ports));
	memset(peb->ti99_4p_expansion_ports, 0, sizeof(peb->ti99_4p_expansion_ports));
}

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti99_peb##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_INLINE_CONFIG
#define DEVTEMPLATE_NAME                "TI99 Peripheral Expansion System"
#define DEVTEMPLATE_FAMILY              "External devices"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( PBOX, ti99_peb );
