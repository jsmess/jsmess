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

	TODO:
	* Test SmartMedia support
	* Implement USB controller and assorted USB devices
	* Save DSR FEEPROM to disk

	Raphael Nabet, 2004.
*/

#include "driver.h"
#include "ti99_4x.h"
#include "99_peb.h"
#include "99_usbsm.h"
#include "strata.h"
#include "smartmed.h"


/* prototypes */
static int usbsm_cru_r(int offset);
static void usbsm_cru_w(int offset, int data);
static  READ8_HANDLER(usbsm_mem_r);
static WRITE8_HANDLER(usbsm_mem_w);
static UINT16 usbsm_mem_16_r(offs_t offset);
static void usbsm_mem_16_w(offs_t offset, UINT16 data);

/* pointer to the SRAM area */
static UINT16 *ti99_usbsm_RAM;
static int feeprom_page;
static int sram_page;

static const ti99_peb_card_handlers_t usbsm_handlers =
{
	usbsm_cru_r,
	usbsm_cru_w,
	usbsm_mem_r,
	usbsm_mem_w
};

static UINT8 cru_register;
enum
{
	cru_reg_io_regs_enable = 0x02,
	cru_reg_int_enable = 0x04,
	cru_reg_sm_enable = 0x08,
	cru_reg_feeprom_write_enable = 0x10
};
static int input_latch, output_latch;

/* The card has a 8-bit->16-bit demultiplexer that can be set-up to assume
either that the LSByte of a word is accessed first or that the MSByte of a word
is accessed first.  The former is true with ti-99/4(a), the latter with the
tms9995 CPU used by Geneve and ti-99/8. */
static int tms9995_mode;

/*
	Reset USB-SmartMedia card, set up handlers
*/
int ti99_usbsm_init(int in_tms9995_mode)
{
	ti99_usbsm_RAM = auto_malloc(0x100000);
	if (strataflash_init(0))
		return 1;
	if (smartmedia_machine_init(0))
		return 1;

	ti99_peb_set_card_handlers(0x1600, & usbsm_handlers);

	feeprom_page = 0;
	sram_page = 0;
	cru_register = 0;

	tms9995_mode = in_tms9995_mode;

	return 0;
}

/*
	Read USB-SmartMedia CRU interface
*/
static int usbsm_cru_r(int offset)
{
	int reply = 0;

	offset &= 3;

	switch (offset)
	{
	case 0:
		/*
			bit
			0	>1x00	0: USB Host controller requests interrupt.
			1	>1x02	0: USB Device controller requests interrupt. 
			2	>1x04	1: USB Host controller suspended.
			3	>1x06	1: USB Device controller suspended.
			4	>1x08	0: Strata FEEPROM is busy.
						1:ÊStrata FEEPROM is ready.
			5	>1x0A	0: SmartMedia card is busy.
						1: SmartMedia card absent or ready.
			6	>1x0C	0: No SmartMedia card present.
						1: A card is in the connector.
			7	>1x0E	0: SmartMedia card is protected.
						1: Card absent or not protected.
		 */
		reply = 0x33;
		if (! smartmedia_present(0))
			reply |= 0xc0;
		else if (! smartmedia_protected(0))
			reply |= 0x80;
		break;
	}

	return reply;
}

/*
	Write USB-SmartMedia CRU interface
*/
static void usbsm_cru_w(int offset, int data)
{
	offset &= 31;


	switch (offset)
	{
	case 0:			/* turn card on: handled by core */
		break;

	case 1:			/* enable I/O registers */
	case 2:			/* enable interrupts */
	case 3:			/* enable SmartMedia card */
	case 4:			/* enable FEEPROM writes (and disable reads) */
		if (data)
			cru_register |= 1 << offset;
		else
			cru_register &= ~ (1 << offset);
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
			feeprom_page |= 1 << (offset-5);
		else
			feeprom_page &= ~ (1 << (offset-5));
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
			sram_page |= 1 << (offset-16);
		else
			sram_page &= ~ (1 << (offset-16));
		break;
	}
}

/*
	read a byte in USB-SmartMedia DSR space
*/
static READ8_HANDLER(usbsm_mem_r)
{
	if (tms9995_mode ? (!(offset & 1)) : (offset & 1))
	{	/* first read triggers 16-bit read cycle */
		input_latch = usbsm_mem_16_r(offset >> 1);
	}

	/* return latched input */
	return (offset & 1) ? input_latch : (input_latch >> 8);
}

/*
	write a byte in USB-SmartMedia DSR space
*/
static WRITE8_HANDLER(usbsm_mem_w)
{
	/* latch write */
	if (offset & 1)
		output_latch = (output_latch & 0xff00) | data;
	else
		output_latch = (output_latch & 0x00ff) | (data << 8);

	if (tms9995_mode ? (offset & 1) : (!(offset & 1)))
	{	/* second write triggers 16-bit write cycle */
		usbsm_mem_16_w(offset >> 1, output_latch);
	}
}

/*
	demultiplexed read in USB-SmartMedia DSR space
*/
static UINT16 usbsm_mem_16_r(offs_t offset)
{
	UINT16 reply = 0;

	if (offset < 0x800)
	{	/* 0x4000-0x4fff range */
		if ((cru_register & cru_reg_io_regs_enable) && (offset >= 0x7f8))
		{	/* SmartMedia interface */
			if (offset == 0)
				reply = smartmedia_data_r(0) << 8;
		}
		else
		{	/* FEEPROM */
			if (! (cru_register & cru_reg_feeprom_write_enable))
				reply = strataflash_16_r(0, offset);
		}
	}
	else
	{	/* 0x5000-0x5fff range */
		if ((cru_register & cru_reg_io_regs_enable) && (offset >= 0xff8))
		{	/* USB controller */
		}
		else
		{	/* SRAM */
			reply = ti99_usbsm_RAM[sram_page*0x800+(offset-0x800)];
		}
	}

	return reply;
}

/*
	demultiplexed write in USB-SmartMedia DSR space
*/
static void usbsm_mem_16_w(offs_t offset, UINT16 data)
{
	if (offset < 0x800)
	{	/* 0x4000-0x4fff range */
		if ((cru_register & cru_reg_io_regs_enable) && (offset >= 0x7f8))
		{	/* SmartMedia interface */
			switch (offset & 3)
			{
			case 0:
				smartmedia_data_w(0, data >> 8);
				break;
			case 1:
				smartmedia_address_w(0, data >> 8);
				break;
			case 2:
				smartmedia_command_w(0, data >> 8);
				break;
			case 3:
				/* bogus, don't use(?) */
				break;
			}
		}
		else
		{	/* FEEPROM */
			if (cru_register & cru_reg_feeprom_write_enable)
				strataflash_16_w(0, offset, data);
		}
	}
	else
	{	/* 0x5000-0x5fff range */
		if ((cru_register & cru_reg_io_regs_enable) && (offset >= 0xff8))
		{	/* USB controller */
		}
		else
		{	/* SRAM */
			ti99_usbsm_RAM[sram_page*0x800+(offset-0x800)] = data;
		}
	}
}
