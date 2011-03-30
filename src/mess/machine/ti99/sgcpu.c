/***************************************************************************

    SGCPU board
    including decoder/mapper

    This component implements the address decoder and mapper logic from the
    SGCPU expansion card (inofficially called TI-99/4P).

    AMA/B/C: According to the manual, the 19-bit address extension is not
    supported by the SGCPU, due to some cards not checking these lines (like
    the Horizon Ramdisk)

    Michael Zapf, October 2010

    TODO: Check for wait states
***************************************************************************/

#include "emu.h"
#include "peribox.h"
#include "sgcpu.h"
#include "videowrp.h"
#include "sound/sn76496.h"
#include "imagedev/cassette.h"

typedef struct _sgcpu_state
{
	/* Pointer to ROM0 */
	UINT16 *rom0;

	/* Pointer to DSR ROM */
	UINT16 *dsr;

	/* Pointer to ROM6, first bank */
	UINT16 *rom6a;

	/* Pointer to ROM6, second bank */
	UINT16 *rom6b;

	/* AMS RAM (1 Mib) */
	UINT16 *ram;

	/* Scratch pad ram (1 KiB) */
	UINT16 *scratchpad;

	/* True if SGCPU DSR is enabled */
	int internal_dsr;

	/* True if SGCPU rom6 is enabled */
	int internal_rom6;

	/* Offset to the ROM6 bank. */
	int rom6_bank;

	/* TRUE when mapper is active */
	int map_mode;

	/* TRUE when mapper registers are accessible */
	int access_mapper;

	UINT8	lowbyte;
	UINT8	highbyte;
	UINT8	latch;

	/* Mapper registers */
	UINT8 mapper[16];

	device_t *cpu;
	device_t *soundchip;
	device_t *video;
	device_t *cassette;
	device_t *peribox;

	int keyCol;
	int alphaLockLine;

} sgcpu_state;


INLINE sgcpu_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == SGCPU);

	return (sgcpu_state *)downcast<legacy_device_base *>(device)->token();
}


/***************************************************************************
   AMS Memory implementation
***************************************************************************/

#define MAP_CRU_BASE 0x0f00
#define SAMS_CRU_BASE 0x1e00

/*
    CRU write (there is no read here)
*/
WRITE8_DEVICE_HANDLER( sgcpu_cru_w )
{
	sgcpu_state *sgcpu = get_safe_token(device);
	int addroff = (offset + 0x0400)<<1;

	if ((addroff & 0xff00)==MAP_CRU_BASE)
	{
		if ((addroff & 0x000e)==0) sgcpu->internal_dsr = data;
		if ((addroff & 0x000e)==2) sgcpu->internal_rom6 = data;
		if ((addroff & 0x000e)==4)
		{
			ti99_peb_senila(sgcpu->peribox, data);
		}
		if ((addroff & 0x000e)==6)
		{
			ti99_peb_senilb(sgcpu->peribox, data);
		}
		// TODO: more CRU bits? 8=Fast timing / a=KBENA
		return;
	}
	if ((addroff & 0xff00)==SAMS_CRU_BASE)
	{
		if ((addroff & 0x000e)==0) sgcpu->access_mapper = data;
		if ((addroff & 0x000e)==2) sgcpu->map_mode = data;
		return;
	}

	// No match - pass to peribox
	ti99_peb_cru_w(sgcpu->peribox, addroff, data);
}

READ8_DEVICE_HANDLER( sgcpu_cru_r )
{
	sgcpu_state *sgcpu = get_safe_token(device);
	UINT8 value = 0;
	int addroff = (offset + 0x0080)<<4;

	ti99_peb_cru_rz(sgcpu->peribox, addroff, &value);
	return value;
}

/*
    Memory read. The SAMS card has two address areas: The memory is at locations
    0x2000-0x3fff and 0xa000-0xffff, and the mapper area is at 0x4000-0x401e
    (only even addresses).
*/
static READ16_DEVICE_HANDLER( sgcpu_samsmem_r )
{
	sgcpu_state *sgcpu = get_safe_token(device);
	UINT32 address = 0;
	int addroff = offset << 1;

	// select memory expansion
	if (sgcpu->map_mode)
		address = (sgcpu->mapper[(addroff>>12) & 0x000f] << 12) + (addroff & 0x0fff);
	else // transparent mode
		address = addroff;

	return sgcpu->ram[address>>1];
}

/*
    Memory write
*/
static WRITE16_DEVICE_HANDLER( sgcpu_samsmem_w )
{
	sgcpu_state *sgcpu = get_safe_token(device);
	UINT32 address = 0;
	int addroff = offset << 1;

	// select memory expansion
	if (sgcpu->map_mode)
		address = (sgcpu->mapper[(addroff>>12) & 0x000f] << 12) + (addroff & 0x0fff);
	else // transparent mode
		address = addroff;

	COMBINE_DATA(&sgcpu->ram[address>>1]);
}

/***************************************************************************
    Internal datamux; similar to TI-99/4A
***************************************************************************/

static READ16_DEVICE_HANDLER( sgcpu_datamux_r )
{
	sgcpu_state *sgcpu = get_safe_token(device);

	UINT8 hbyte = 0;
	UINT16 addroff = (offset << 1);

	ti99_peb_data_rz(sgcpu->peribox, addroff+1, &sgcpu->latch);
	sgcpu->lowbyte = sgcpu->latch;

	// Takes three cycles
	device_adjust_icount(sgcpu->cpu, -3);

	ti99_peb_data_rz(sgcpu->peribox, addroff, &hbyte);
	sgcpu->highbyte = hbyte;

	// Takes three cycles
	device_adjust_icount(sgcpu->cpu, -3);

	// use the latch and the currently read byte and put it on the 16bit bus
//  printf("read  address = %04x, value = %04x, memmask = %4x\n", addroff,  (hbyte<<8) | sgcpu->latch, mem_mask);
	return (hbyte<<8) | sgcpu->latch ;
}

/*
    Write access.
    TODO: use the 16-bit expansion in the box for suitable cards
*/
static WRITE16_DEVICE_HANDLER( sgcpu_datamux_w )
{
	sgcpu_state *sgcpu = get_safe_token(device);
	UINT16 addroff = (offset << 1);
//  printf("write address = %04x, value = %04x, memmask = %4x\n", addroff, data, mem_mask);

	// read more about the datamux in datamux.c

	// byte-only transfer, high byte
	// we use the previously read low byte to complete
	if (mem_mask == 0xff00)
		data = data | sgcpu->lowbyte;

	// byte-only transfer, low byte
	// we use the previously read high byte to complete
	if (mem_mask == 0x00ff)
		data = data | (sgcpu->highbyte << 8);

	// Write to the PEB
	ti99_peb_data_w(sgcpu->peribox, addroff+1, data & 0xff);

	// Takes three cycles
	device_adjust_icount(sgcpu->cpu,-3);

	// Write to the PEB
	ti99_peb_data_w(sgcpu->peribox, addroff, (data>>8) & 0xff);

	// Takes three cycles
	device_adjust_icount(sgcpu->cpu,-3);
}

/*
    Read access.
*/
READ16_DEVICE_HANDLER( sgcpu_r )
{
	sgcpu_state *sgcpu = get_safe_token(device);
	if (sgcpu->cpu == NULL) return 0;

	int addroff = offset << 1;
	UINT16 zone = addroff & 0xe000;
	UINT16 value = 0;

	if (zone==0x0000)
	{
		// ROM0
		value = sgcpu->rom0[(addroff & 0x1fff)>>1];
		return value;
	}
	if (zone==0x2000 || zone==0xa000 || zone==0xc000 || zone==0xe000)
	{
		value = sgcpu_samsmem_r(device, offset, mem_mask);
		return value;
	}

	if (zone==0x4000)
	{
		if (sgcpu->internal_dsr)
		{
			value = sgcpu->dsr[(addroff & 0x1fff)>>1];
			return value;
		}
		else
		{
			if (sgcpu->access_mapper && ((addroff & 0xffe0)==0x4000))
			{
				value = sgcpu->mapper[offset & 0x000f]<<8;
				return value;
			}
		}
	}

	if (zone==0x6000 && sgcpu->internal_rom6)
	{
		if (sgcpu->rom6_bank==0)
			value = sgcpu->rom6a[(addroff & 0x1fff)>>1];
		else
			value = sgcpu->rom6b[(addroff & 0x1fff)>>1];

		return value;
	}

	// Scratch pad RAM and sound
	// speech is in peribox
	// groms are in hsgpl in peribox
	if (zone==0x8000)
	{
		if ((addroff & 0xfff0)==0x8400)
		{
			value = 0;
			return value;
		}
		if ((addroff & 0xfc00)==0x8000)
		{
			value = sgcpu->scratchpad[(addroff & 0x03ff)>>1];
			return value;
		}
		// Video: 8800, 8802
		if ((addroff & 0xfffd)==0x8800)
		{
			value = ti_v9938_r16(sgcpu->video, offset, mem_mask);
			return value;
		}
	}

	// If we are here, check the peribox via the datamux
	// catch-all for unmapped zones
	value = sgcpu_datamux_r(device, offset, mem_mask);
	return value;
}

/*
    Write access.
*/
WRITE16_DEVICE_HANDLER( sgcpu_w )
{
	sgcpu_state *sgcpu = get_safe_token(device);
	if (sgcpu->cpu == NULL) return;

//  device_adjust_icount(sgcpu->cpu, -4);

	int addroff = offset << 1;
	UINT16 zone = addroff & 0xe000;

	if (zone==0x0000)
	{
		// ROM0
		logerror("SGCPU: ignoring ROM write access at %04x\n", addroff);
		return;
	}

	if (zone==0x2000 || zone==0xa000 || zone==0xc000 || zone==0xe000)
	{
		sgcpu_samsmem_w(device, offset, data, mem_mask);
		return;
	}

	if (zone==0x4000)
	{
		if (sgcpu->internal_dsr)
		{
			logerror("SGCPU: ignoring DSR write access at %04x\n", addroff);
			return;
		}
		else
		{
			if (sgcpu->access_mapper && ((addroff & 0xffe0)==0x4000))
			{
				COMBINE_DATA(&sgcpu->mapper[offset & 0x000f]);
				return;
			}
		}
	}

	if (zone==0x6000 && sgcpu->internal_rom6)
	{
		sgcpu->rom6_bank = offset & 0x0001;
		return;
	}

	// Scratch pad RAM and sound
	// speech is in peribox
	// groms are in hsgpl in peribox
	if (zone==0x8000)
	{
		if ((addroff & 0xfff0)==0x8400)
		{
			sn76496_w(sgcpu->soundchip, 0, (data >> 8) & 0xff);
			return;
		}
		if ((addroff & 0xfc00)==0x8000)
		{
			COMBINE_DATA(&sgcpu->scratchpad[(addroff & 0x03ff)>>1]);
			return;
		}
		// Video: 8C00, 8C02
		if ((addroff & 0xfffd)==0x8c00)
		{
			ti_v9938_w16(sgcpu->video, offset, data, mem_mask);
			return;
		}
	}

	// If we are here, check the peribox via the datamux
	// catch-all for unmapped zones
	sgcpu_datamux_w(device, offset, data, mem_mask);
}

/***************************************************************************
    Control lines
****************************************************************************/
/*
    Handles the EXTINT line. This one is directly connected to the 9901.
*/
WRITE_LINE_DEVICE_HANDLER( card_extint )
{
	tms9901_set_single_int(device->machine().device("tms9901"), 1, state);
}

WRITE_LINE_DEVICE_HANDLER( card_notconnected )
{
	logerror("Not connected line set ... sorry\n");
}

WRITE_LINE_DEVICE_HANDLER( card_ready )
{
	logerror("READY line set ... not yet connected, level=%02x\n", state);
}

/*
    set the state of int2 (called by the v9938)
*/
void tms9901_sg_set_int2(running_machine &machine, int state)
{
	tms9901_set_single_int(machine.device("tms9901"), 2, state);
}

/***************************************************************************
    Keyboard/tape control
****************************************************************************/
static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3" };

/*
    Called by the 9901 core whenever the state of INTREQ and IC0-3 changes
*/
static TMS9901_INT_CALLBACK( tms9901_interrupt_callback )
{
	if (intreq)
	{
		// On TI99, TMS9900 IC0-3 lines are not connected to TMS9901,
		// but hard-wired to force level 1 interrupts
		cputag_set_input_line_and_vector(device->machine(), "maincpu", 0, ASSERT_LINE, 1);
	}
	else
	{
		cputag_set_input_line(device->machine(), "maincpu", 0, CLEAR_LINE);
	}
}


/*
    Read pins INT3*-INT7* of TI99's 9901.

    signification:
     (bit 1: INT1 status)
     (bit 2: INT2 status)
     bit 3-7: keyboard status bits 0 to 4
*/
static READ8_DEVICE_HANDLER( sgcpu_R9901_0 )
{
	device_t *dev = device->machine().device("sgcpu_board");
	sgcpu_state *sgcpu = get_safe_token(dev);
	int answer;

	answer = ((input_port_read(device->machine(), keynames[sgcpu->keyCol >> 1]) >> ((sgcpu->keyCol & 1) * 8)) << 3) & 0xF8;

	if (sgcpu->alphaLockLine == FALSE)
		answer &= ~(input_port_read(device->machine(), "ALPHA") << 3);

	return answer;
}

/*
    Read pins INT8*-INT15* of TI99's 9901.
     bit 0-2: keyboard status bits 5 to 7
     bit 3: tape input mirror
     bit 5-7: weird, not emulated
*/
static READ8_DEVICE_HANDLER( sgcpu_R9901_1 )
{
	device_t *dev = device->machine().device("sgcpu_board");
	sgcpu_state *sgcpu = get_safe_token(dev);
	int answer;

	if (sgcpu->keyCol == 7)
		answer = 0x07;
	else
	{
		answer = ((input_port_read(device->machine(), keynames[sgcpu->keyCol >> 1]) >> ((sgcpu->keyCol & 1) * 8)) >> 5) & 0x07;
	}
	return answer;
}


/*
    Read pins P8-P15 of TI99's 9901.
     bit 26: high
     bit 27: tape input
*/
static READ8_DEVICE_HANDLER( sgcpu_R9901_3 )
{
	device_t *dev = device->machine().device("sgcpu_board");
	sgcpu_state *sgcpu = get_safe_token(dev);
	int answer = 4;	/* on systems without handset, the pin is pulled up to avoid spurious interrupts */
	if (cassette_input(sgcpu->cassette) > 0)
		answer |= 8;
	return answer;
}

/*
    WRITE key column select (P2-P4)
*/
static WRITE8_DEVICE_HANDLER( sgcpu_KeyC )
{
	device_t *dev = device->machine().device("sgcpu_board");
	sgcpu_state *sgcpu = get_safe_token(dev);
	if (data)
		sgcpu->keyCol |= 1 << (offset-2);
	else
		sgcpu->keyCol &= ~(1 << (offset-2));
}

/*
    WRITE alpha lock line - TI99/4a only (P5)
*/
static WRITE8_DEVICE_HANDLER( sgcpu_AlphaW )
{
	device_t *dev = device->machine().device("sgcpu_board");
	sgcpu_state *sgcpu = get_safe_token(dev);
	sgcpu->alphaLockLine = data;
}

/*
    command CS1 (only) tape unit motor (P6)
*/
static WRITE8_DEVICE_HANDLER( sgcpu_CS_motor )
{
	device_t *dev = device->machine().device("sgcpu_board");
	sgcpu_state *sgcpu = get_safe_token(dev);
	cassette_change_state(sgcpu->cassette, data ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
}

/*
    audio gate (P8)

    Set to 1 before using tape: this enables the mixing of tape input sound
    with computer sound.

    We do not really need to emulate this as the tape recorder generates sound
    on its own.
*/
static WRITE8_DEVICE_HANDLER( sgcpu_audio_gate )
{
}

/*
    tape output (P9)
    I think polarity is correct, but don't take my word for it.
*/
static WRITE8_DEVICE_HANDLER( sgcpu_CS_output )
{
	device_t *dev = device->machine().device("sgcpu_board");
	sgcpu_state *sgcpu = get_safe_token(dev);
	cassette_output(sgcpu->cassette, data ? +1 : -1);
}

/* TMS9901 setup. The callback functions pass a reference to the TMS9901 as device. */
const tms9901_interface tms9901_wiring_ti99_4p =
{
	TMS9901_INT1 | TMS9901_INT2 | TMS9901_INTC,	/* only input pins whose state is always known */

	{	/* read handlers */
		sgcpu_R9901_0,
		sgcpu_R9901_1,
		NULL,
		sgcpu_R9901_3
	},

	{	/* write handlers */
		NULL,
		NULL,
		sgcpu_KeyC,
		sgcpu_KeyC,
		sgcpu_KeyC,
		sgcpu_AlphaW,
		sgcpu_CS_motor,
		NULL,
		sgcpu_audio_gate,
		sgcpu_CS_output,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	},

	/* interrupt handler */
	tms9901_interrupt_callback,

	/* clock rate = 3MHz */
	3000000.
};


/***************************************************************************
    DEVICE LIFECYCLE FUNCTIONS
***************************************************************************/

static DEVICE_START( sgcpu )
{
//  // printf("Starting mapper\n");
	sgcpu_state *sgcpu = get_safe_token(device);
	sgcpu->cpu = device->machine().device("maincpu");
	sgcpu->peribox = device->machine().device("peribox");
	sgcpu->soundchip = device->machine().device("soundgen");
	sgcpu->video = device->machine().device("video");
	sgcpu->cassette = device->machine().device("cassette");

	assert(sgcpu->cpu && sgcpu->peribox && sgcpu->soundchip && sgcpu->video && sgcpu->cassette);

	sgcpu->ram = (UINT16*)malloc(0x080000);
	sgcpu->scratchpad = (UINT16*)malloc(0x0400);

	UINT16 *rom = (UINT16*)device->machine().region("maincpu")->base();
	sgcpu->rom0  = rom + 0x2000;
	sgcpu->dsr   = rom + 0x6000;
	sgcpu->rom6a = rom + 0x3000;
	sgcpu->rom6b = rom + 0x7000;
}

static DEVICE_STOP( sgcpu )
{
	// printf("Stopping SGCPU\n");
	sgcpu_state *sgcpu = get_safe_token(device);
	free(sgcpu->ram);
	free(sgcpu->scratchpad);
}

static DEVICE_RESET( sgcpu )
{
	logerror("Resetting SGCPU\n");
}

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##sgcpu##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET
#define DEVTEMPLATE_NAME                "SGCPU Board"
#define DEVTEMPLATE_FAMILY              "Peripheral Expansion"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( SGCPU, sgcpu );
