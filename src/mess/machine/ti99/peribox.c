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

    June 2010: Reimplemented using device structure (MZ) (obsoletes 99_peb.c)

Slots:

          REAR
     +8V  1||2   +8V
     GND  3||4   READY
     GND  5||6   RESET*
     GND  7||8   SCLK
 BOOTPG*  9||10  AUDIO
 RDBENA* 11||12  PCBEN
   HOLD* 13||14  IAQHDA
 SENILA* 15||16  SENILB*
  INTA*  17||18  INTB*
     D7  19||20  GND
     D5  21||22  D6
     D3  23||24  D4
     D1  25||26  D2
    GND  27||28  D0
    A14  29||30  A15/CRUOUT
    A12  31||32  A13
    A10  33||34  A11
     A8  35||36  A9
     A6  37||38  A7
     A4  39||40  A5
     A2  41||42  A3
     A0  43||44  A1
    AMB  45||46  AMA
    GND  47||48  AMC
    GND  49||50  CLKOUT*
CRUCLK*  51||52  DBIN
    GND  53||54  WE*
  CRUIN  55||56  MEMEN*
   -16V  57||58  -16V
   +16V  59||60  +16V
         FRONT

        < from box to console
        > from console into box

READYA  <    System ready (goes to READY, 10K pull-up to +5V) A low level puts the cpu on hold.
RESET*  >    System reset (active low)
SCLK    nc   System clock (not connected in interface card)
LCP*    nc   CPU indicator 1=TI99 0=2nd generation (not connected in interface card)
BOOTPG* nc   ?
AUDIO   <    Input audio (to AUDIOIN in console)
RDBENA* <    Active low: enable flex cable data bus drivers (1K pull-up)
PCBEN   H    PCB enable for burn-in (always High)
HOLD*   H    Active low CPU hold request (always High)
IAQHDA  nc   IAQ [or] HOLDA (logical or)
SENILA* H(>) Interrupt level A sense enable (always High)
SENILB* H(>) Interrupt level B sense enable (always High)
INTA*   <    Interrupt level A (active low, goes to EXTINT*)
INTB*   nc   Interrupt level B (not used)
LOAD*   nc   Unmaskable interrupt (not carried by interface cable/card)
D0-D7   <>   Data bus (D0 most significant)
A0-A15  >    Address bus (A0 most sig; A15 also used as CRUOUT)
AMA/B/C H    Extra address bits (always high for TI-99/4x)
CLKOUT* >    Inverted PHI3 clock, from TIM9904 clock generator
CRUCLK* >    Inverted CRU clock, from TMS9900 CRUCLK pin
DBIN    >    Active high = read memory. Drives the data bus buffers.
WE*     >    Write Enable pulse (derived from TMS9900 WE* pin)
CRUIN   <    CRU input bit to TMS9900
MEMEN*  >    Memory access enable (active low)


The obscure SENILx lines:

With SENILA* going low, a value shall be put on the data bus,
representing the interrupt status bits. It can also be used to determine
the source of the interrupt: The RS232 card (in its standard configuration)
uses the data bus bits 0 and 1 for its two UARTs, while in the second
configuration, it uses bits 4 and 5. Still note that the TI-99/4x does not make
use of SENILA*.

SENILB* / INTB* was planned to be used with disk controllers. The PHP1240 disk
controller puts the value of INTB* on D0 when SENILB* gets active (low) which
reflects the INTRQ output pin of the WD1771. This signal is not used, however.
Instead, the disk controller combines DRQ and IRQ and makes use of a READY/HOLD
control of the CPU.

Obviously, SENILA* and SENILB* should never be active at the same time, and
neither should any memory access to a card be active at the same time, for in
both cases, data bus lines may be set to different levels simultaneously. One
possible application case is to turn off all cards in the box, lower SENILA*,
and then do a read access in the memory area of any card in the P-Box (e.g.
0x4000-0x5fff). Another possiblity is that the currently active card simply
does not respond to a certain memory access, and in this case the status bits
can be read.

Also note that the SENILx lines access all cards in parallel, meaning that there
must be an agreement which cards may use which bits on the data bus. The lines
do not depend on the card being active at that time.
*/

#include "emu.h"
#include "peribox.h"

/* Expansion cards */
#include "p_code.h"
#include "ti_fdc.h"
#include "hfdc.h"
#include "bwg.h"
#include "ti_rs232.h"
#include "tn_usbsm.h"
#include "tn_ide.h"
#include "hsgpl.h"
#include "spchsyn.h"
#include "evpc.h"
#include "memex.h"

#include "imagedev/flopdrv.h"
#include "imagedev/harddriv.h"
#include "formats/ti99_dsk.h"
#include "ti99_hd.h"

static const floppy_config ti99_4_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(ti99),
	NULL
};

typedef struct _ti99_peb_slot
{
	device_t		*card;
	const ti99_peb_card	*intf;
} ti99_peb_slot;

typedef struct _ti99_peb_state
{
	ti99_peb_slot			slot[MAXSLOTS];

	// The next three members are used to emulate the collective lines
	// If any card goes L, L is propagated. We are using flags.
	UINT32					inta_state;
	UINT32					intb_state;
	UINT32					ready_state;

	int						highest;

	/* Address bits that are preset to 1 (AMA/AMB/AMC) */
	int						address_prefix;

	// Callback to the main system
	ti99_peb_connect		lines;

} ti99_peb_state;

INLINE ti99_peb_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == PBOX4 || device->type() == PBOX4A || device->type() == PBOX8 || device->type() == PBOXEV || device->type() == PBOXSG || device->type() == PBOXGEN);

	return (ti99_peb_state *)downcast<legacy_device_base *>(device)->token();
}

INLINE const ti99_peb_config *get_config(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == PBOX4 || device->type() == PBOX4A || device->type() == PBOX8 || device->type() == PBOXEV || device->type() == PBOXSG || device->type() == PBOXGEN);

	return (const ti99_peb_config *) downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

/* Callbacks */
static WRITE_LINE_DEVICE_HANDLER( inta )
{
	int slot = get_pebcard_config(device)->slot;
	ti99_peb_state *peb = get_safe_token(device->owner());
	if (state==TRUE)
	{
		// The flags are stored as inverted (inta_state=0 if all lines are H)
		peb->inta_state &= ~(1 << slot);
	}
	else
	{
		// 1 means: The card set it to L
		peb->inta_state |= (1 << slot);
	}
	// Call back the main system, setting the line to L if any PEB card set it to L
	// This is the case when inta_state is not 0
	devcb_call_write_line( &peb->lines.inta, (peb->inta_state == 0) );
}

static WRITE_LINE_DEVICE_HANDLER( intb )
{
	int slot = get_pebcard_config(device)->slot;
	ti99_peb_state *peb = get_safe_token(device->owner());
	if (state==TRUE)
		peb->intb_state &= ~(1 << slot);
	else
		peb->intb_state |= (1 << slot);
	devcb_call_write_line( &peb->lines.intb, (peb->intb_state == 0) );
}

static WRITE_LINE_DEVICE_HANDLER( ready )
{
	int slot = get_pebcard_config(device)->slot;
	ti99_peb_state *peb = get_safe_token(device->owner());
	if (state==TRUE)
		peb->ready_state &= ~(1 << slot);
	else
		peb->ready_state |= (1 << slot);
	devcb_call_write_line( &peb->lines.ready, (peb->ready_state == 0) );
}

/*
    Declares the callbacks above that are called from the cards. This instance
    is passed to the cards with MCFG_PBOXCARD_ADD and must be handled as a
    static_config.
*/
const peb_callback_if peb_callback =
{
	DEVCB_LINE( inta ),
	DEVCB_LINE( intb ),
	DEVCB_LINE( ready )
};

/*
    Mount the PEB card. Although possible, this is not done by the box itself;
    instead, the card must call this function on the box. The advantage is
    that we can assign the same slot to different cards which are selected
    by dip switches. Only the card which is selected will register itself; the
    other cards are stillborn.
    Returns TRUE when the card could be mounted.
*/
int mount_card(device_t *device, device_t *cardptr, const ti99_peb_card *cardintf, int slotindex)
{
	ti99_peb_state *peb = get_safe_token(device);
	if ((slotindex > 0) &&  (slotindex < MAXSLOTS))
	{
		if (peb->slot[slotindex].card == NULL)
		{
			peb->slot[slotindex].card = cardptr;
			peb->slot[slotindex].intf = cardintf;
			if (slotindex > peb->highest) peb->highest = slotindex;
			logerror("ti99_peb: registering the card at slot index %d\n", slotindex);
			return TRUE;
		}
		else
		{
			logerror("ti99_peb: Tried to mount a second card in slot %d\n", slotindex);
		}
	}
	else
		logerror("ti99_peb: Invalid slot number %d\n", slotindex);
	return FALSE;
}

void unmount_card(device_t *device, int slotindex)
{
	ti99_peb_state *peb = get_safe_token(device);
	peb->slot[slotindex].card = NULL;
}

/***************************************************************************
    Memory and CRU access
***************************************************************************/

READ8Z_DEVICE_HANDLER( ti99_peb_data_rz )
{
	ti99_peb_state *peb = get_safe_token(device);
	int i;
	for (i=1; i <= peb->highest; i++)
	{
		if ((peb->slot[i].card != NULL) && (*peb->slot[i].intf->card_read_data))
			(*peb->slot[i].intf->card_read_data)(peb->slot[i].card, peb->address_prefix | offset, value);
	}
}

WRITE8_DEVICE_HANDLER( ti99_peb_data_w )
{
	ti99_peb_state *peb = get_safe_token(device);
	int i;
	for (i=1; i <= peb->highest; i++)
	{
		if ((peb->slot[i].card != NULL) && (*peb->slot[i].intf->card_write_data))
			(*peb->slot[i].intf->card_write_data)(peb->slot[i].card, peb->address_prefix | offset, data);
	}
}

READ8Z_DEVICE_HANDLER( ti99_peb_cru_rz )
{
	ti99_peb_state *peb = get_safe_token(device);
	int i;
	for (i=1; i <= peb->highest; i++)
	{
		if ((peb->slot[i].card != NULL) && (*peb->slot[i].intf->card_read_cru))
			(*peb->slot[i].intf->card_read_cru)(peb->slot[i].card, offset, value);
	}
}

WRITE8_DEVICE_HANDLER( ti99_peb_cru_w )
{
	ti99_peb_state *peb = get_safe_token(device);
	int i;
	for (i=1; i <= peb->highest; i++)
	{
		if ((peb->slot[i].card != NULL) && (*peb->slot[i].intf->card_write_cru))
			(*peb->slot[i].intf->card_write_cru)(peb->slot[i].card, offset, data);
	}
}

/*
    For SGCPU
*/
READ16Z_DEVICE_HANDLER( ti99_peb_data16_rz )
{
	ti99_peb_state *peb = get_safe_token(device);
	int i;
	for (i=1; i <= peb->highest; i++)
	{
		if ((peb->slot[i].card != NULL) && (*peb->slot[i].intf->card_read_data16))
			(*peb->slot[i].intf->card_read_data16)(peb->slot[i].card, peb->address_prefix | offset, value);
	}
}

/*
    For SGCPU
*/
WRITE16_DEVICE_HANDLER( ti99_peb_data16_w )
{
	ti99_peb_state *peb = get_safe_token(device);
	int i;
	for (i=1; i <= peb->highest; i++)
	{
		if ((peb->slot[i].card != NULL) && (*peb->slot[i].intf->card_write_data16))
			(*peb->slot[i].intf->card_write_data16)(peb->slot[i].card, peb->address_prefix | offset, data);
	}
}

/*
    SENILx
    TODO: Check whether device is the correct pointer (seems to be wrong here,
    the device is usually the caller)
*/
WRITE_LINE_DEVICE_HANDLER( ti99_peb_senila )
{
	ti99_peb_state *peb = get_safe_token(device);
	int i;
	for (i=1; i <= peb->highest; i++)
	{
		if ((peb->slot[i].card != NULL) && (*peb->slot[i].intf->senila))
			(*peb->slot[i].intf->senila)(peb->slot[i].card, state);
	}
}

WRITE_LINE_DEVICE_HANDLER( ti99_peb_senilb )
{
	ti99_peb_state *peb = get_safe_token(device);
	int i;
	for (i=1; i <= peb->highest; i++)
	{
		if ((peb->slot[i].card != NULL) && (*peb->slot[i].intf->senilb))
			(*peb->slot[i].intf->senilb)(peb->slot[i].card, state);
	}
}

/***************************************************************************
    DEVICE FUNCTIONS
***************************************************************************/

static DEVICE_START( ti99_peb )
{
	ti99_peb_state *peb = get_safe_token(device);
	const ti99_peb_config* pebconf = (const ti99_peb_config*)get_config(device);

	for (int slotindex=0; slotindex < MAXSLOTS; slotindex++)
		unmount_card(device, slotindex);

	/* Resolve the callbacks to the console */
	devcb_write_line inta = DEVCB_LINE(pebconf->inta);
	devcb_write_line intb = DEVCB_LINE(pebconf->intb);
	devcb_write_line ready = DEVCB_LINE(pebconf->ready);

	devcb_resolve_write_line(&peb->lines.ready, &ready, device);
	devcb_resolve_write_line(&peb->lines.inta, &inta, device);
	devcb_resolve_write_line(&peb->lines.intb, &intb, device);

	peb->address_prefix = (pebconf->amx << 16);
}

static DEVICE_STOP( ti99_peb )
{
	logerror("ti99_peb: stop\n");
}

static DEVICE_RESET( ti99_peb )
{
	ti99_peb_state *peb = get_safe_token(device);
	logerror("ti99_peb: reset\n");
	peb->inta_state = 0;
	peb->intb_state = 0;
	peb->ready_state = 0;
	peb->highest = 0;
	for (int i=0; i < MAXSLOTS; i++) unmount_card(device, i);
}

#define MCFG_PBOXCARD_ADD( _tag, _type, _slot ) \
	MCFG_DEVICE_ADD(_tag, _type, 0) \
	MCFG_DEVICE_CONFIG_DATA32( ti99_pebcard_config, slot, _slot ) \
	MCFG_DEVICE_CONFIG( peb_callback )

/*
    We're emulating a "super box" with more than 8 slots. The original
    box had 8 slots, but that's a bit too limiting.
    This may become organized differently in future versions; for now
    we'll go this way.

    Slot 0 is the "Flex cable interface" / SGCPU / Geneve
    We assign the same slot number for some cards that cannot be used
    in parallel anyway. The config switch will determine which one to use.
    Future works:
    // WHTech SCSI controller
    // MCFG_PBOXCARD_ADD("SCSI", 4, scsi_intf)

    // Horizon Ramdisk
    // MCFG_PBOXCARD_ADD("HRD", 10, ramdisk_intf)
*/

static MACHINE_CONFIG_FRAGMENT( ti99_peb )
	MCFG_PBOXCARD_ADD( "mem_ti32k", 	TI32KMEM, 1 )
	MCFG_PBOXCARD_ADD( "mem_sams1m",	SAMSMEM,  1 )
	MCFG_PBOXCARD_ADD( "mem_myarc512",	MYARCMEM, 1 )
	MCFG_PBOXCARD_ADD( "speech",		TISPEECH, 2 )
	MCFG_PBOXCARD_ADD( "usbsmart",		USBSMART, 3 )
	MCFG_PBOXCARD_ADD( "ide",			TNIDE,	  4 )
	MCFG_PBOXCARD_ADD( "hsgpl", 		HSGPL,    5 )
	MCFG_PBOXCARD_ADD( "rs232_card",	TIRS232,  7 )
	MCFG_PBOXCARD_ADD( "ti_fdc",		TIFDC,	  8 )
	MCFG_PBOXCARD_ADD( "hfdc",			HFDC,	  8 )
	MCFG_PBOXCARD_ADD( "bwg",			BWG,	  8 )

	MCFG_FLOPPY_4_DRIVES_ADD(ti99_4_floppy_config)
	MCFG_MFMHD_3_DRIVES_ADD()
MACHINE_CONFIG_END

static MACHINE_CONFIG_FRAGMENT( ti994a_peb )
	MCFG_PBOXCARD_ADD( "mem_ti32k", 	TI32KMEM, 1 )
	MCFG_PBOXCARD_ADD( "mem_sams1m",	SAMSMEM,  1 )
	MCFG_PBOXCARD_ADD( "mem_myarc512",	MYARCMEM, 1 )
	MCFG_PBOXCARD_ADD( "speech",		TISPEECH, 2 )
	MCFG_PBOXCARD_ADD( "usbsmart",		USBSMART, 3 )
	MCFG_PBOXCARD_ADD( "ide",			TNIDE,	  4 )
	MCFG_PBOXCARD_ADD( "hsgpl", 		HSGPL,    5 )
	MCFG_PBOXCARD_ADD( "p_code_card",	PCODEN,   6 )
	MCFG_PBOXCARD_ADD( "rs232_card",	TIRS232,  7 )
	MCFG_PBOXCARD_ADD( "ti_fdc",		TIFDC,	  8 )
	MCFG_PBOXCARD_ADD( "hfdc",			HFDC,	  8 )
	MCFG_PBOXCARD_ADD( "bwg",			BWG,	  8 )

	MCFG_FLOPPY_4_DRIVES_ADD(ti99_4_floppy_config)
	MCFG_MFMHD_3_DRIVES_ADD()
MACHINE_CONFIG_END

static MACHINE_CONFIG_FRAGMENT( ti99ev_peb )
	MCFG_PBOXCARD_ADD( "mem_ti32k", 	TI32KMEM, 1 )
	MCFG_PBOXCARD_ADD( "mem_sams1m",	SAMSMEM,  1 )
	MCFG_PBOXCARD_ADD( "mem_myarc512",	MYARCMEM, 1 )
	MCFG_PBOXCARD_ADD( "speech",		TISPEECH, 2 )
	MCFG_PBOXCARD_ADD( "usbsmart",		USBSMART, 3 )
	MCFG_PBOXCARD_ADD( "ide",			TNIDE,	  4 )
	MCFG_PBOXCARD_ADD( "hsgpl", 		HSGPL,    5 )
	MCFG_PBOXCARD_ADD( "p_code_card",	PCODEN,   6 )
	MCFG_PBOXCARD_ADD( "rs232_card",	TIRS232,  7 )
	MCFG_PBOXCARD_ADD( "ti_fdc",		TIFDC,	  8 )
	MCFG_PBOXCARD_ADD( "hfdc",			HFDC,	  8 )
	MCFG_PBOXCARD_ADD( "bwg",			BWG,	  8 )
	MCFG_PBOXCARD_ADD( "evpc",			EVPC,	  9 )

	MCFG_FLOPPY_4_DRIVES_ADD(ti99_4_floppy_config)
	MCFG_MFMHD_3_DRIVES_ADD()
MACHINE_CONFIG_END


static MACHINE_CONFIG_FRAGMENT( ti998_peb )
	MCFG_PBOXCARD_ADD( "usbsmart",		USBSMART, 1 )
	MCFG_PBOXCARD_ADD( "ide",			TNIDE,	  2 )
	MCFG_PBOXCARD_ADD( "rs232_card",	TIRS232,  3 )
	MCFG_PBOXCARD_ADD( "ti_fdc",		TIFDC,	  8 )
	MCFG_PBOXCARD_ADD( "hfdc",			HFDC,	  8 )
	MCFG_PBOXCARD_ADD( "bwg",			BWG,	  8 )

	MCFG_FLOPPY_4_DRIVES_ADD(ti99_4_floppy_config)
	MCFG_MFMHD_3_DRIVES_ADD()
MACHINE_CONFIG_END

static MACHINE_CONFIG_FRAGMENT( ti99sg_peb )
	MCFG_PBOXCARD_ADD( "speech",		TISPEECH, 1 )
	MCFG_PBOXCARD_ADD( "usbsmart",		USBSMART, 2 )
	MCFG_PBOXCARD_ADD( "ide",			TNIDE,	  3 )
	MCFG_PBOXCARD_ADD( "hsgpl", 		HSGPL,    4 )
	MCFG_PBOXCARD_ADD( "rs232_card",	TIRS232,  6 )
	MCFG_PBOXCARD_ADD( "ti_fdc",		TIFDC,	  7 )
	MCFG_PBOXCARD_ADD( "hfdc",			HFDC,	  7 )
	MCFG_PBOXCARD_ADD( "bwg",			BWG,	  7 )
	MCFG_PBOXCARD_ADD( "evpc",			EVPC,	  8 )

	MCFG_FLOPPY_4_DRIVES_ADD(ti99_4_floppy_config)
	MCFG_MFMHD_3_DRIVES_ADD()
MACHINE_CONFIG_END

static MACHINE_CONFIG_FRAGMENT( geneve_peb )
	MCFG_PBOXCARD_ADD( "memex",			GENMEMEX, 1 )
	MCFG_PBOXCARD_ADD( "speech",		TISPEECH, 2 )
	MCFG_PBOXCARD_ADD( "usbsmart",		USBSMART, 3 )
	MCFG_PBOXCARD_ADD( "ide",			TNIDE,	  4 )
	MCFG_PBOXCARD_ADD( "rs232_card",	TIRS232,  6 )
	MCFG_PBOXCARD_ADD( "ti_fdc",		TIFDC,	  7 )
	MCFG_PBOXCARD_ADD( "hfdc",			HFDC,	  7 )
	MCFG_PBOXCARD_ADD( "bwg",			BWG,	  7 )

	MCFG_FLOPPY_4_DRIVES_ADD(ti99_4_floppy_config)
	MCFG_MFMHD_3_DRIVES_ADD()
MACHINE_CONFIG_END

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti99_peb##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_MACHINE_CONFIG | DT_HAS_INLINE_CONFIG
#define DEVTEMPLATE_NAME                "TI-99 Peripheral Expansion System (99/4)"
#define DEVTEMPLATE_FAMILY              "External devices"
#include "devtempl.h"

#define DEVTEMPLATE_DERIVED_ID(p,s)		p##ti994a_peb##s
#define DEVTEMPLATE_DERIVED_FEATURES	DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_DERIVED_NAME		"TI-99 Peripheral Expansion System (99/4A)"
#include "devtempl.h"

#define DEVTEMPLATE_DERIVED_ID(p,s)		p##ti998_peb##s
#define DEVTEMPLATE_DERIVED_FEATURES	DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_DERIVED_NAME		"TI-99 Peripheral Expansion System (99/8)"
#include "devtempl.h"

#define DEVTEMPLATE_DERIVED_ID(p,s)		p##ti99ev_peb##s
#define DEVTEMPLATE_DERIVED_FEATURES	DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_DERIVED_NAME		"TI-99 Peripheral Expansion System (99/4a with EVPC)"
#include "devtempl.h"

#define DEVTEMPLATE_DERIVED_ID(p,s)		p##ti99sg_peb##s
#define DEVTEMPLATE_DERIVED_FEATURES	DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_DERIVED_NAME		"TI99 Peripheral Expansion System (SGCPU)"
#include "devtempl.h"

#define DEVTEMPLATE_DERIVED_ID(p,s)		p##geneve_peb##s
#define DEVTEMPLATE_DERIVED_FEATURES	DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_DERIVED_NAME		"TI99 Peripheral Expansion System (Geneve)"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( PBOX4, ti99_peb );
DEFINE_LEGACY_DEVICE( PBOX4A, ti994a_peb );
DEFINE_LEGACY_DEVICE( PBOXEV, ti99ev_peb );
DEFINE_LEGACY_DEVICE( PBOX8, ti998_peb );
DEFINE_LEGACY_DEVICE( PBOXSG, ti99sg_peb );
DEFINE_LEGACY_DEVICE( PBOXGEN, geneve_peb );
