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
*/

#include "driver.h"
#include "99_peb.h"

/* TRUE if we are using the snug sgcpu 99/4p 16-bit extensions */
static int has_16bit_peb;

/* handlers for each of 16 slots + 16 extra slots for ti-99/8 */
static ti99_peb_card_handlers_t expansion_ports[16+16];

/* expansion card structure for the snug sgcpu 99/4p system, which supports
dynamical 16-bit accesses (TI did not design this!) */
typedef struct ti99_4p_peb_card_handlers_t
{
	cru_read_handler cru_read;		/* card CRU read handler */
	cru_write_handler cru_write;	/* card CRU handler */

	enum {width_8bit = 0, width_16bit} width;
	union
	{
		struct
		{
			read8_handler mem_read;		/* card mem read handler (8 bits) */
			write8_handler mem_write;	/* card mem write handler (8 bits) */
		} width_8bit;
		struct
		{
			read16_handler mem_read;	/* card mem read handler (16 bits) */
			write16_handler mem_write;	/* card mem write handler (16 bits) */
		} width_16bit;
	} w;
} ti99_4p_peb_card_handlers_t;

/* handlers for each of 28 slots */
static ti99_4p_peb_card_handlers_t ti99_4p_expansion_ports[28];

/* index of the currently active card (-1 if none) */
static int active_card;

/* when 1, enable a workaround required by snug sgcpu 99/4p, which mistakenly
enables 2 cards simultaneously */
#define ACTIVATE_BIT_EMULATE 1

#if ACTIVATE_BIT_EMULATE
/* activate mask: 1 bit set for each card enable CRU bit set */
static int active_card_mask;
#endif

/* ila: inta status register (not actually used on ti-99/4(a), but nevertheless
present) */
static int ila;
/* ilb: intb status register (completely pointless on ti-99/4(a), as neither
INTB nor SENILB are connected; OTOH, INTB can trigger interrupts on a Geneve,
and snug sgcpu 99/4p can sense ILB) */
static int ilb;

/* only used by the snug sgcpu 99/4p */
static int senila, senilb;

/* hack to simulate TMS9900 byte write */
static int tmp_buffer;

/* inta/intb handlers */
void (*inta_callback)(int state);
void (*intb_callback)(int state);


/*
	Resets the expansion card handlers

	in_has_16bit_peb: TRUE if we are using the snug sgcpu 99/4p 16-bit
		extensions
	in_inta_callback: callback called when the state of INTA changes (may be
		NULL)
	in_intb_callback: callback called when the state of INTB changes (may be
		NULL)
*/
void ti99_peb_init(int in_has_16bit_peb, void (*in_inta_callback)(int state), void (*in_intb_callback)(int state))
{
	memset(expansion_ports, 0, sizeof(expansion_ports));
	memset(ti99_4p_expansion_ports, 0, sizeof(ti99_4p_expansion_ports));

	has_16bit_peb = in_has_16bit_peb;
	inta_callback = in_inta_callback;
	intb_callback = in_intb_callback;

	active_card = -1;
#if ACTIVATE_BIT_EMULATE
	active_card_mask = 0;
#endif
	ila = 0;
	ilb = 0;
}

/*
	Sets the handlers for one expansion port (normal 8-bit card)

	cru_base: CRU base address (any of 0x1000, 0x1100, 0x1200, ..., 0x1F00)
		(snug sgcpu 99/4p accepts 0x0400, ..., 0x1F00, ti-99/8 accepts 0x1000,
		..., 0x2F00)
	handler: handler structure for the given card
*/
void ti99_peb_set_card_handlers(int cru_base, const ti99_peb_card_handlers_t *handler)
{
	int port;

	if (cru_base & 0xff)
		return;

	if (has_16bit_peb)
	{
		port = (cru_base - 0x0400) >> 8;

		if ((port>=0) && (port<28))
		{
			ti99_4p_expansion_ports[port].cru_read = handler->cru_read;
			ti99_4p_expansion_ports[port].cru_write = handler->cru_write;
			ti99_4p_expansion_ports[port].width = width_8bit;
			ti99_4p_expansion_ports[port].w.width_8bit.mem_read = handler->mem_read;
			ti99_4p_expansion_ports[port].w.width_8bit.mem_write = handler->mem_write;
		}
	}
	else
	{
		port = (cru_base - 0x1000) >> 8;

		if ((port>=0) && (port</*16*/32))
		{
			expansion_ports[port] = *handler;
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
void ti99_peb_set_16bit_card_handlers(int cru_base, const ti99_peb_16bit_card_handlers_t *handler)
{
	int port;

	if (cru_base & 0xff)
		return;

	if (has_16bit_peb)
	{
		port = (cru_base - 0x0400) >> 8;

		if ((port>=0) && (port<28))
		{
			ti99_4p_expansion_ports[port].cru_read = handler->cru_read;
			ti99_4p_expansion_ports[port].cru_write = handler->cru_write;
			ti99_4p_expansion_ports[port].width = width_16bit;
			ti99_4p_expansion_ports[port].w.width_16bit.mem_read = handler->mem_read;
			ti99_4p_expansion_ports[port].w.width_16bit.mem_write = handler->mem_write;
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
void ti99_peb_set_ila_bit(int bit, int state)
{
	if (state)
	{
		ila |= 1 << bit;
		if (inta_callback)
			(*inta_callback)(1);
	}
	else
	{
		ila &= ~(1 << bit);
		if ((! ila) && inta_callback)
			(*inta_callback)(0);
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
void ti99_peb_set_ilb_bit(int bit, int state)
{
	if (state)
	{
		ilb |= 1 << bit;
		if (intb_callback)
			(*intb_callback)(1);
	}
	else
	{
		ilb &= ~(1 << bit);
		if ((! ilb) && intb_callback)
			(*intb_callback)(0);
	}
}


/*
	Read CRU in range >1000->1ffe (>800->fff) (ti-99/4(a))
*/
 READ8_HANDLER ( ti99_4x_peb_cru_r )
{
	int port;
	cru_read_handler handler;
	int reply;

	port = (offset & 0x00f0) >> 4;
	handler = expansion_ports[port].cru_read;

	reply = (handler) ? (*handler)(offset & 0xf) : 0;

	return reply;
}

/*
	Write CRU in range >1000->1ffe (>800->fff) (ti-99/4(a))
*/
WRITE8_HANDLER ( ti99_4x_peb_cru_w )
{
	int port;
	cru_write_handler handler;

	port = (offset & 0x0780) >> 7;
	handler = expansion_ports[port].cru_write;

	if (handler)
		(*handler)(offset & 0x7f, data);

	/* expansion card enable? */
	if ((offset & 0x7f) == 0)
	{
		if (data & 1)
		{
			/* enable */
			active_card = port;
		}
		else
		{
			if (port == active_card)	/* geez... who cares? */
			{
				active_card = -1;			/* no port selected */
			}
		}
	}
}

/*
	Read mem in range >4000->5ffe (ti-99/4(a))
*/
READ16_HANDLER ( ti99_4x_peb_r )
{
	int reply = 0;
	read8_handler handler;

	activecpu_adjust_icount(-4);

	if (active_card != -1)
	{
		handler = expansion_ports[active_card].mem_read;
		if (handler)
		{
			reply = (*handler)((offset << 1) + 1);
			reply |= ((unsigned) (*handler)(offset << 1)) << 8;
		}
	}

	return tmp_buffer = reply;
}

/*
	Write mem in range >4000->5ffe (ti-99/4(a))
*/
WRITE16_HANDLER ( ti99_4x_peb_w )
{
	write8_handler handler;

	activecpu_adjust_icount(-4);

	/* simulate byte write */
	data = (tmp_buffer & mem_mask) | (data & ~mem_mask);

	if (active_card != -1)
	{
		handler = expansion_ports[active_card].mem_write;
		if (handler)
		{
			(*handler)((offset << 1) + 1, data & 0xff);
			(*handler)(offset << 1, (data >> 8) & 0xff);
		}
	}
}


/*
	Read CRU in range >1000->1ffe (>800->fff) (Geneve)
*/
 READ8_HANDLER ( geneve_peb_cru_r )
{
	int port;
	cru_read_handler handler;
	int reply;

	port = (offset & 0x00f0) >> 4;
	handler = expansion_ports[port].cru_read;

	reply = (handler) ? (*handler)(offset & 0xf) : 0;

	return reply;
}

/*
	Write CRU in range >1000->1ffe (>800->fff) (Geneve)
*/
WRITE8_HANDLER ( geneve_peb_cru_w )
{
	int port;
	cru_write_handler handler;

	port = (offset & 0x0780) >> 7;
	handler = expansion_ports[port].cru_write;

	if (handler)
		(*handler)(offset & 0x7f, data);

	/* expansion card enable? */
	if ((offset & 0x7f) == 0)
	{
		if (data & 1)
		{
			/* enable */
			active_card = port;
		}
		else
		{
			if (port == active_card)	/* geez... who cares? */
			{
				active_card = -1;			/* no port selected */
			}
		}
	}
}

/*
	Read mem in range >4000->5fff (Geneve)
*/
 READ8_HANDLER ( geneve_peb_r )
{
	int reply = 0;
	read8_handler handler;

	activecpu_adjust_icount(-8);

	if (active_card != -1)
	{
		handler = expansion_ports[active_card].mem_read;
		if (handler)
			reply = (*handler)(offset);
	}

	return reply;
}

/*
	Write mem in range >4000->5fff (Geneve)
*/
WRITE8_HANDLER ( geneve_peb_w )
{
	write8_handler handler;

	activecpu_adjust_icount(-8);

	if (active_card != -1)
	{
		handler = expansion_ports[active_card].mem_write;
		if (handler)
			(*handler)(offset, data);
	}
}

/*
	Read CRU in range >1000->2ffe (>0800->17ff) (ti-99/8)
*/
 READ8_HANDLER ( ti99_8_peb_cru_r )
{
	int port;
	cru_read_handler handler;
	int reply;

	port = (offset & 0x01f0) >> 4;
	handler = expansion_ports[port].cru_read;

	reply = (handler) ? (*handler)(offset & 0xf) : 0;

	return reply;
}

/*
	Write CRU in range >1000->2ffe (>0800->17ff) (ti-99/8)
*/
WRITE8_HANDLER ( ti99_8_peb_cru_w )
{
	int port;
	cru_write_handler handler;

	port = (offset & 0x0f80) >> 7;
	handler = expansion_ports[port].cru_write;

	if (handler)
		(*handler)(offset & 0x7f, data);

	/* expansion card enable? */
	if ((offset & 0x7f) == 0)
	{
		if (data & 1)
		{
			/* enable */
			active_card = port;
		}
		else
		{
			if (port == active_card)	/* geez... who cares? */
			{
				active_card = -1;			/* no port selected */
			}
		}
	}
}

/*
	Read mem in range >4000->5fff (ti-99/8)
*/
 READ8_HANDLER ( ti99_8_peb_r )
{
	int reply = 0;
	read8_handler handler;

	activecpu_adjust_icount(-4);

	if (active_card != -1)
	{
		handler = expansion_ports[active_card].mem_read;
		if (handler)
			reply = (*handler)(offset);
	}

	return reply;
}

/*
	Write mem in range >4000->5fff (ti-99/8)
*/
WRITE8_HANDLER ( ti99_8_peb_w )
{
	write8_handler handler;

	activecpu_adjust_icount(-4);

	if (active_card != -1)
	{
		handler = expansion_ports[active_card].mem_write;
		if (handler)
			(*handler)(offset, data);
	}
}

/*
	Read CRU in range >0400->1ffe (>200->fff) (snug sgcpu 99/4p)
*/
 READ8_HANDLER ( ti99_4p_peb_cru_r )
{
	int port;
	cru_read_handler handler;
	int reply;

	port = offset >> 4;
	handler = ti99_4p_expansion_ports[port].cru_read;

	reply = (handler) ? (*handler)(offset & 0xf) : 0;

	return reply;
}

/*
	Write CRU in range >0400->1ffe (>200->fff) (snug sgcpu 99/4p)
*/
WRITE8_HANDLER ( ti99_4p_peb_cru_w )
{
	int port;
	cru_write_handler handler;

	port = offset >> 7;
	handler = ti99_4p_expansion_ports[port].cru_write;

	if (handler)
		(*handler)(offset & 0x7f, data);

	/* expansion card enable? */
	if ((offset & 0x7f) == 0)
	{
		if (data & 1)
		{
			/* enable */
			active_card = port;
#if ACTIVATE_BIT_EMULATE
			active_card_mask |= (1 << port);
#endif
		}
		else
		{
#if ACTIVATE_BIT_EMULATE
			active_card_mask &= ~(1 << port);
#endif
			if (port == active_card)	/* geez... who cares? */
			{
				active_card = -1;			/* no port selected */
#if ACTIVATE_BIT_EMULATE
				active_card_mask &= ~(1 << port);

				if (active_card_mask)
				{
					int i;
					logerror("Extension card error, trying to recover\n");
					for (i = 0; i<28; i++)
						if (active_card_mask & (1 << i))
							active_card = i;
				}
#endif
			}
		}
	}
}

/*
	Read mem in range >4000->5ffe (snug sgcpu 99/4p)
*/
READ16_HANDLER ( ti99_4p_peb_r )
{
	int reply = 0;
	read8_handler handler;
	read16_handler handler16;


	if (active_card == -1)
		activecpu_adjust_icount(-4);	/* ??? */
	else
	{

		if (ti99_4p_expansion_ports[active_card].width == width_8bit)
		{
			activecpu_adjust_icount(-4);

			handler = ti99_4p_expansion_ports[active_card].w.width_8bit.mem_read;
			if (handler)
			{
				reply = (*handler)((offset << 1) + 1);
				reply |= ((unsigned) (*handler)(offset << 1)) << 8;
			}
		}
		else
		{
			activecpu_adjust_icount(-1);	/* ??? */

			handler16 = ti99_4p_expansion_ports[active_card].w.width_16bit.mem_read;
			if (handler16)
				reply = (*handler16)(offset, /*mem_mask*/0);
		}
	}

	if (senila || senilb)
	{
		if ((active_card != -1) || (senila && senilb))
			/* ah, the smell of burnt silicon... */
			logerror("<Scrrrrr>: your computer has just burnt (maybe).\n");

		if (senila)
			reply = (ila & 0xff) | (ila << 8);
		else if (senilb)
			reply = (ilb & 0xff) | (ilb << 8);
	}

	return tmp_buffer = reply;
}

/*
	Write mem in range >4000->5ffe (snug sgcpu 99/4p)
*/
WRITE16_HANDLER ( ti99_4p_peb_w )
{
	write8_handler handler;
	write16_handler handler16;

	if (active_card == -1)
		activecpu_adjust_icount(-4);	/* ??? */
	else
	{
		/* simulate byte write */
		data = (tmp_buffer & mem_mask) | (data & ~mem_mask);

		if (ti99_4p_expansion_ports[active_card].width == width_8bit)
		{
			activecpu_adjust_icount(-4);

			handler = ti99_4p_expansion_ports[active_card].w.width_8bit.mem_write;
			if (handler)
			{
				(*handler)((offset << 1) + 1, data & 0xff);
				(*handler)(offset << 1, (data >> 8) & 0xff);
			}
		}
		else
		{
			activecpu_adjust_icount(-1);	/* ??? */

			handler16 = ti99_4p_expansion_ports[active_card].w.width_16bit.mem_write;
			if (handler16)
				(*handler16)(offset, data, /*mem_mask*/0);
		}
	}
}

/*
	Set the state of the SENILA line (snug sgcpu 99/4p)
*/
void ti99_4p_peb_set_senila(int state)
{
	senila = (state != 0);
}

/*
	Set the state of the SENILB line (snug sgcpu 99/4p)
*/
void ti99_4p_peb_set_senilb(int state)
{
	senilb = (state != 0);
}
