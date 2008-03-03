/*
	Thierry Nouspikel's IDE card emulation

	This card is just a prototype.  It has been designed by Thierry Nouspikel,
	and its description was published in 2001.  The card have been revised in
	2004.

	The specs have been published in <http://www.nouspikel.com/ti99/ide.html>.

	The IDE interface is quite simple, since it only implements PIO transfer.
	The card includes a clock chip to timestamp files, and an SRAM for the DSR.
	It should be possible to use a battery backed DSR SRAM, but since the clock
	chip includes 4kb of battery-backed RAM, a bootstrap loader can be saved in
	the clock SRAM in order to load the DSR from the HD when the computer
	starts.

	Raphael Nabet, 2002-2004.
*/

#include "driver.h"
#include "machine/idectrl.h"
#include "devices/idedrive.h"
#include "machine/rtc65271.h"
#include "ti99_4x.h"
#include "99_peb.h"
#include "99_ide.h"


/* prototypes */
static void ide_interrupt_callback(int state);

static int ide_cru_r(int offset);
static void ide_cru_w(int offset, int data);
static  READ8_HANDLER(ide_mem_r);
static WRITE8_HANDLER(ide_mem_w);

/* pointer to the IDE RAM area */
static UINT8 *ti99_ide_RAM;
static int cur_page;
enum
{	/* 0xff for 2 mbytes, 0x3f for 512kbytes, 0x03 for 32 kbytes */
	page_mask = /*0xff*/0x3f
};

static const ti99_peb_card_handlers_t ide_handlers =
{
	ide_cru_r,
	ide_cru_w,
	ide_mem_r,
	ide_mem_w
};

static int sram_enable;
static int sram_enable_dip = 0;
static int cru_register;
enum
{
	cru_reg_page_switching = 0x04,
	cru_reg_page_0 = 0x08,
	/*cru_reg_rambo = 0x10,*/	/* not emulated */
	cru_reg_wp = 0x20,
	cru_reg_int_en = 0x40,
	cru_reg_reset = 0x80
};
static int input_latch, output_latch;

static int ide_irq, clk_irq;

static const struct ide_interface ti99_ide_interface =
{
	ide_interrupt_callback
};

/* The card has a 8-bit->16-bit demultiplexer that can be set-up to assume
either that the LSByte of a word is accessed first or that the MSByte of a word
is accessed first.  The former is true with ti-99/4(a), the latter with the
tms9995 CPU used by Geneve and ti-99/8.  Note that the first revision of IDE
only supported ti-99/4(a) (LSByte first) mode. */
static int tms9995_mode;

/*
	ide_interrupt_callback()

	IDE interrupt callback
*/
static void ide_interrupt_callback(int state)
{
	ide_irq = state;
	if (cru_register & cru_reg_int_en)
		ti99_peb_set_ila_bit(inta_ide_bit, state);
}

/*
	clk_interrupt_callback()

	clock interrupt callback
*/
static void clk_interrupt_callback(int state)
{
	clk_irq = state;
	ti99_peb_set_ila_bit(inta_ide_clk_bit, state);
}

/*
	Initializes the ide card, set up handlers
*/
void ti99_ide_init()
{
	rtc65271_init(memory_region(region_dsr) + offset_ide_ram2, clk_interrupt_callback);
	ti99_ide_RAM = memory_region(region_dsr) + offset_ide_ram;
}

/*
	Reset ide card, set up handlers
*/
void ti99_ide_reset(int in_tms9995_mode)
{
	ti99_peb_set_card_handlers(0x1000, & ide_handlers);

	cur_page = 0;
	sram_enable = 0;
	cru_register = 0;

	tms9995_mode = in_tms9995_mode;
}

int ti99_ide_load_memcard(void)
{
	file_error filerr;
	mame_file *file;

	filerr = mame_fopen(SEARCHPATH_MEMCARD, "ide.nv", OPEN_FLAG_READ, &file);
	if (filerr != FILERR_NONE)
		return /*1*/0;
	if (rtc65271_file_load(file))
	{
		mame_fclose(file);
		return 1;
	}

	mame_fclose(file);
	return 0;
}

int ti99_ide_save_memcard(void)
{
	file_error filerr;
	mame_file *file;

	/*if (ti99_ide_get_dirty_flag())*/
	{
		filerr = mame_fopen(SEARCHPATH_MEMCARD, "ide.nv", OPEN_FLAG_WRITE, &file);
		if (filerr != FILERR_NONE)
			return 1;
		if (rtc65271_file_save(file))
		{
			mame_fclose(file);
			return 1;
		}

		mame_fclose(file);
	}

	return 0;
}

/*
	Read ide CRU interface
*/
static int ide_cru_r(int offset)
{
	int reply;

	switch (offset)
	{
	default:
		reply = cru_register & 0x30;
		reply |= 8;	/* IDE bus IORDY always set */
		if (! clk_irq)
			reply |= 4;
		if (sram_enable_dip)
			reply |= 2;
		if (! ide_irq)
			reply |= 1;
		break;
	}

	return reply;
}

/*
	Write ide CRU interface
*/
static void ide_cru_w(int offset, int data)
{
	offset &= 7;


	switch (offset)
	{
	case 0:			/* turn card on: handled by core */
		break;

	case 1:			/* enable SRAM or registers in 0x4000-0x40ff */
		sram_enable = data;
		break;

	case 2:			/* enable SRAM page switching */
	case 3:			/* force SRAM page 0 */
	case 4:			/* enable SRAM in 0x6000-0x7000 ("RAMBO" mode) */
	case 5:			/* write-protect RAM */
	case 6:			/* irq and reset enable */
	case 7:			/* reset drive */
		if (data)
			cru_register |= 1 << offset;
		else
			cru_register &= ~ (1 << offset);

		if (offset == 6)
			ti99_peb_set_ila_bit(inta_ide_bit, (cru_register & cru_reg_int_en) && ide_irq);

		if ((offset == 6) || (offset == 7))
			if ((cru_register & cru_reg_int_en) && !(cru_register & cru_reg_reset))
				ide_controller_reset(0);
		break;
	}
}

/*
	read a byte in ide DSR space
*/
static READ8_HANDLER(ide_mem_r)
{
	int reply = 0;


	if ((offset <= 0xff) && (sram_enable == sram_enable_dip))
	{	/* registers */
		switch ((offset >> 5) & 0x3)
		{
		case 0:		/* RTC RAM */
			if (offset & 0x80)
				/* RTC RAM page register */
				reply = rtc65271_r(1, (offset & 0x1f) | 0x20);
			else
				/* RTC RAM read */
				reply = rtc65271_r(1, offset);
			break;
		case 1:		/* RTC registers */
			if (offset & 0x10)
				/* register data */
				reply = rtc65271_r(0, 1);
			else
				/* register select */
				reply = rtc65271_r(0, 0);
			break;
		case 2:		/* IDE registers set 1 (CS1Fx) */
			if (tms9995_mode ? (!(offset & 1)) : (offset & 1))
			{	/* first read triggers 16-bit read cycle */
				input_latch = (! (offset & 0x10)) ? ide_bus_0_r(0, (offset >> 1) & 0x7) : 0;
			}

			/* return latched input */
			/*reply = (offset & 1) ? input_latch : (input_latch >> 8);*/
			/* return latched input - bytes are swapped in 2004 IDE card */
			reply = (offset & 1) ? (input_latch >> 8) : input_latch;
			break;
		case 3:		/* IDE registers set 2 (CS3Fx) */
			if (tms9995_mode ? (!(offset & 1)) : (offset & 1))
			{	/* first read triggers 16-bit read cycle */
				input_latch = (! (offset & 0x10)) ? ide_bus_0_r(1, (offset >> 1) & 0x7) : 0;
			}

			/* return latched input */
			/*reply = (offset & 1) ? input_latch : (input_latch >> 8);*/
			/* return latched input - bytes are swapped in 2004 IDE card */
			reply = (offset & 1) ? (input_latch >> 8) : input_latch;
			break;
		}
	}
	else
	{	/* sram */
		if ((cru_register & cru_reg_page_0) || (offset >= 0x1000))
			reply = ti99_ide_RAM[offset+0x2000*cur_page];
		else
			reply = ti99_ide_RAM[offset];
	}

	return reply;
}

/*
	write a byte in ide DSR space
*/
static WRITE8_HANDLER(ide_mem_w)
{
	if (cru_register & cru_reg_page_switching)
	{
		cur_page = (offset >> 1) & page_mask;
	}

	if ((offset <= 0xff) && (sram_enable == sram_enable_dip))
	{	/* registers */
		switch ((offset >> 5) & 0x3)
		{
		case 0:		/* RTC RAM */
			if (offset & 0x80)
				/* RTC RAM page register */
				rtc65271_w(1, (offset & 0x1f) | 0x20, data);
			else
				/* RTC RAM write */
				rtc65271_w(1, offset, data);
			break;
		case 1:		/* RTC registers */
			if (offset & 0x10)
				/* register data */
				rtc65271_w(0, 1, data);
			else
				/* register select */
				rtc65271_w(0, 0, data);
			break;
		case 2:		/* IDE registers set 1 (CS1Fx) */
			/* latch write */
			/*if (offset & 1)
				output_latch = (output_latch & 0xff00) | data;
			else
				output_latch = (output_latch & 0x00ff) | (data << 8);*/
			/* latch write - bytes are swapped in 2004 IDE card */
			if (offset & 1)
				output_latch = (output_latch & 0x00ff) | (data << 8);
			else
				output_latch = (output_latch & 0xff00) | data;

			if (tms9995_mode ? (offset & 1) : (!(offset & 1)))
			{	/* second write triggers 16-bit write cycle */
				ide_bus_0_w(0, (offset >> 1) & 0x7, output_latch);
			}
			break;
		case 3:		/* IDE registers set 2 (CS3Fx) */
			/* latch write */
			/*if (offset & 1)
				output_latch = (output_latch & 0xff00) | data;
			else
				output_latch = (output_latch & 0x00ff) | (data << 8);*/
			/* latch write - bytes are swapped in 2004 IDE card */
			if (offset & 1)
				output_latch = (output_latch & 0x00ff) | (data << 8);
			else
				output_latch = (output_latch & 0xff00) | data;

			if (tms9995_mode ? (offset & 1) : (!(offset & 1)))
			{	/* second write triggers 16-bit write cycle */
				ide_bus_0_w(1, (offset >> 1) & 0x7, output_latch);
			}
			break;
		}
	}
	else
	{	/* sram */
		if (! (cru_register & cru_reg_wp))
		{
			if ((cru_register & cru_reg_page_0) || (offset >= 0x1000))
				ti99_ide_RAM[offset+0x2000*cur_page] = data;
			else
				ti99_ide_RAM[offset] = data;
		}
	}
}



void ti99_ide_harddisk_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:						info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_IDEDRIVE_INTERFACE: info->p = (void *) &ti99_ide_interface; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_DEV_TAG:					strcpy(info->s = device_temp_str(), "99_ide"); break;

		default: ide_harddisk_device_getinfo(devclass, state, info); break;
	}
}
