/*
    Thierry Nouspikel's USB-SmartMedia card emulation

    This card features three USB ports (two host and one device) and a
    SmartMedia interface.  The original prototype was designed by Thierry
    Nouspikel, and its description was published in 2003; a small series of
    approximately 100 printed-circuit boards was ordered shortly afterwards
    by various TI users.

    The specs have been published in <http://www.nouspikel.com/ti99/usb.html>.

    The USB interface uses a Philips ISP1161A USB controller that supports
    USB 2.0 full-speed (12 Mbit/s) and low-speed (1.5 Mbit/s) I/O (high speed
    (480 Mbits/sec) is not supported, but it is hardly a problem since TI99 is
    too slow to take advantage from high speed).  The SmartMedia interface uses
    a few TTL buffers.  The card also includes an 8MByte StrataFlash FEEPROM
    and 1MByte of SRAM for DSR use.

    The card has a 8-bit->16-bit demultiplexer that can be set-up to assume
    either that the LSByte of a word is accessed first or that the MSByte of a word
    is accessed first.  The former is true with ti-99/4(a), the latter with the
    tms9995 CPU used by Geneve and ti-99/8.

    TODO:
    * Test SmartMedia support
    * Implement USB controller and assorted USB devices
    * Save DSR FEEPROM to disk

    Raphael Nabet, 2004.

    Rewritten as device
    Michael Zapf, September 2010
*/
#include "emu.h"
#include "peribox.h"
#include "tn_usbsm.h"
#include "cpu/tms9900/tms9900.h"
#include "machine/strata.h"
#include "machine/smartmed.h"

#define CRU_BASE 0x1600

typedef ti99_pebcard_config tn_usbsm_config;

typedef struct _tn_usbsm_state
{
	device_t *smartmedia;
	device_t *strata;

	int 	selected;

	/* Used for GenMod */
	int 	select_mask;
	int		select_value;

	int		feeprom_page;
	int		sram_page;
	int		cru_register;
	int		tms9995_mode;

	UINT16	input_latch;

	UINT16	output_latch;
	UINT16	*ram;

	/* Callback lines to the main system. */
	ti99_peb_connect	lines;

} tn_usbsm_state;

enum
{
	cru_reg_io_regs_enable = 0x02,
	cru_reg_int_enable = 0x04,
	cru_reg_sm_enable = 0x08,
	cru_reg_feeprom_write_enable = 0x10
};

INLINE tn_usbsm_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == USBSMART);

	return (tn_usbsm_state *)downcast<legacy_device_base *>(device)->token();
}

/*
    demultiplexed read in USB-SmartMedia DSR space
*/
static UINT16 usbsm_mem_16_r(device_t *device, offs_t offset)
{
	UINT16 reply = 0;
	tn_usbsm_state *card = get_safe_token(device);

	if (offset < 0x2800)
	{	/* 0x4000-0x4fff range */
		if ((card->cru_register & cru_reg_io_regs_enable) && (offset >= 0x27f8))
		{	/* SmartMedia interface */
			if (offset == 0)
				reply = smartmedia_data_r(card->smartmedia) << 8;
		}
		else
		{	/* FEEPROM */
			if (!(card->cru_register & cru_reg_feeprom_write_enable))
				reply = strataflash_16_r(card->strata, offset);
		}
	}
	else
	{	/* 0x5000-0x5fff range */
		if ((card->cru_register & cru_reg_io_regs_enable) && (offset >= 0x2ff8))
		{	/* USB controller */
		}
		else
		{	/* SRAM */
			reply = card->ram[card->sram_page*0x800+(offset-0x2800)];
		}
	}
	return reply;
}

/*
    demultiplexed write in USB-SmartMedia DSR space
*/
static void usbsm_mem_16_w(device_t *device, offs_t offset, UINT16 data)
{
	tn_usbsm_state *card = get_safe_token(device);

	if (offset < 0x2800)
	{	/* 0x4000-0x4fff range */
		if ((card->cru_register & cru_reg_io_regs_enable) && (offset >= 0x27f8))
		{	/* SmartMedia interface */
			switch (offset & 3)
			{
			case 0:
				smartmedia_data_w(card->smartmedia, data >> 8);
				break;
			case 1:
				smartmedia_address_w(card->smartmedia, data >> 8);
				break;
			case 2:
				smartmedia_command_w(card->smartmedia, data >> 8);
				break;
			case 3:
				/* bogus, don't use(?) */
				break;
			}
		}
		else
		{	/* FEEPROM */
			if (card->cru_register & cru_reg_feeprom_write_enable)
				strataflash_16_w(card->strata, offset, data);
		}
	}
	else
	{	/* 0x5000-0x5fff range */
		if ((card->cru_register & cru_reg_io_regs_enable) && (offset >= 0x2ff8))
		{	/* USB controller */
		}
		else
		{	/* SRAM */
			card->ram[card->sram_page*0x800+(offset-0x2800)] = data;
		}
	}
}

/*
    CRU read
*/
static READ8Z_DEVICE_HANDLER( cru_rz )
{
	tn_usbsm_state *card = get_safe_token(device);

	if ((offset & 0xff00)==CRU_BASE)
	{
		UINT8 reply = 0;
		device_t *smartmedia = card->smartmedia;
		offset &= 3;

		if (offset == 0)
		{
			// bit
			// 0   >1x00   0: USB Host controller requests interrupt.
			// 1   >1x02   0: USB Device controller requests interrupt.
			// 2   >1x04   1: USB Host controller suspended.
			// 3   >1x06   1: USB Device controller suspended.
			// 4   >1x08   0: Strata FEEPROM is busy.
			//             1: Strata FEEPROM is ready.
			// 5   >1x0A   0: SmartMedia card is busy.
			//             1: SmartMedia card absent or ready.
			// 6   >1x0C   0: No SmartMedia card present.
			//             1: A card is in the connector.
			// 7   >1x0E   0: SmartMedia card is protected.
			//             1: Card absent or not protected.

			reply = 0x33;
			if (!smartmedia_present(smartmedia))
				reply |= 0xc0;
			else if (!smartmedia_protected(smartmedia))
				reply |= 0x80;
		}
		*value = reply;
	}
}

/*
    CRU write
*/
static WRITE8_DEVICE_HANDLER( cru_w )
{
	tn_usbsm_state *card = get_safe_token(device);

	if ((offset & 0xff00)==CRU_BASE)
	{
		int bit = (offset >> 1) & 0x1f;

		switch (bit)
		{
		case 0:
			card->selected = data;
			break;

		case 1:			/* enable I/O registers */
		case 2:			/* enable interrupts */
		case 3:			/* enable SmartMedia card */
		case 4:			/* enable FEEPROM writes (and disable reads) */
			if (data)
				card->cru_register |= 1 << bit;
			else
				card->cru_register &= ~ (1 << bit);
			break;
		case 5:			/* FEEPROM page */
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			if (data)
				card->feeprom_page |= 1 << (bit-5);
			else
				card->feeprom_page &= ~ (1 << (bit-5));
			break;

		case 16:		/* SRAM page */
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
			if (data)
				card->sram_page |= 1 << (bit-16);
			else
				card->sram_page &= ~ (1 << (bit-16));
			break;
		}
	}
}

/*
    Memory read
    TODO: Check whether AMA/B/C is actually checked
*/
static READ8Z_DEVICE_HANDLER( data_rz )
{
	tn_usbsm_state *card = get_safe_token(device);

	if (((offset & card->select_mask)==card->select_value) && card->selected)
	{
		if (card->tms9995_mode ? (!(offset & 1)) : (offset & 1))
		{	/* first read triggers 16-bit read cycle */
			card->input_latch = usbsm_mem_16_r(device, (offset >> 1)&0xffff);
		}

		/* return latched input */
		*value = ((offset & 1) ? (card->input_latch) : (card->input_latch >> 8)) & 0xff;
	}
}

/*
    Memory write. The controller is 16 bit, so we need to demultiplex again.
*/
static WRITE8_DEVICE_HANDLER( data_w )
{
	tn_usbsm_state *card = get_safe_token(device);

	if (((offset & card->select_mask)==card->select_value) && card->selected)
	{
		/* latch write */
		if (offset & 1)
			card->output_latch = (card->output_latch & 0xff00) | data;
		else
			card->output_latch = (card->output_latch & 0x00ff) | (data << 8);

		if ((card->tms9995_mode)? (offset & 1) : (!(offset & 1)))
		{	/* second write triggers 16-bit write cycle */
			usbsm_mem_16_w(device, (offset >> 1)&0xffff, card->output_latch);
		}
	}
}

/**************************************************************************/

static const ti99_peb_card tn_usbsm_card =
{
	data_rz, data_w,				// memory access read/write
	cru_rz, cru_w,					// CRU access
	NULL, NULL,						// SENILA/B access
	NULL, NULL						// 16 bit access (none here)
};

static DEVICE_START( tn_usbsm )
{
	tn_usbsm_state *card = get_safe_token(device);
	card->ram = auto_alloc_array(device->machine(), UINT16, 0x100000/2);
	card->smartmedia = device->subdevice("smartmedia");
	card->strata = device->subdevice("strata");
}

static DEVICE_STOP( tn_usbsm )
{
}

static DEVICE_RESET( tn_usbsm )
{
	tn_usbsm_state *card = get_safe_token(device);
	/* Register the card */
	device_t *peb = device->owner();

	if (input_port_read(device->machine(), "HDCTRL") & HD_USB)
	{
		int success = mount_card(peb, device, &tn_usbsm_card, get_pebcard_config(device)->slot);
		if (!success) return;

		card->feeprom_page = 0;
		card->sram_page = 0;
		card->cru_register = 0;
		card->tms9995_mode = (device->type()==TMS9995);

		card->select_mask = 0x7e000;
		card->select_value = 0x74000;

		if (input_port_read(device->machine(), "MODE")==GENMOD)
		{
			// GenMod card modification
			card->select_mask = 0x1fe000;
			card->select_value = 0x174000;
		}
	}
}

MACHINE_CONFIG_FRAGMENT( tn_usbsm )
	MCFG_SMARTMEDIA_ADD( "smartmedia" )
	MCFG_STRATAFLASH_ADD( "strata" )
MACHINE_CONFIG_END

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##tn_usbsm##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_INLINE_CONFIG | DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_NAME                "Nouspikel USB/Smartmedia card"
#define DEVTEMPLATE_FAMILY              "Peripheral expansion"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( USBSMART, tn_usbsm );

