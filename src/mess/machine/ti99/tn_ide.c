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

    Rewritten as device
    Michael Zapf, September 2010

    FIXME: idecont assumes that there is only one drive that is called
    "harddisk" at the root level
*/
#include "emu.h"
#include "peribox.h"
#include "machine/idectrl.h"
#include "machine/rtc65271.h"
#include "machine/idectrl.h"
#include "cpu/tms9900/tms9900.h"
#include "tn_ide.h"

#define CRU_BASE 0x1000

typedef ti99_pebcard_config tn_ide_config;

enum
{	/* 0xff for 2 mbytes, 0x3f for 512kbytes, 0x03 for 32 kbytes */
	page_mask = /*0xff*/0x3f
};

enum
{
	cru_reg_page_switching = 0x04,
	cru_reg_page_0 = 0x08,
	/*cru_reg_rambo = 0x10,*/	/* not emulated */
	cru_reg_wp = 0x20,
	cru_reg_int_en = 0x40,
	cru_reg_reset = 0x80
};

typedef struct _tn_ide_state
{
	int 	selected;

	/* Used for GenMod */
	int		select_mask;
	int		select_value;

	device_t		*rtc;
	device_t		*ide;

	int		ide_irq;
	int		clk_irq;
	int 	sram_enable;
	int 	sram_enable_dip;
	int 	cru_register;
	int		cur_page;

	int		tms9995_mode;

	UINT16	input_latch;
	UINT16	output_latch;

	UINT8	*ram;

	/* Callback lines to the main system. */
	ti99_peb_connect	lines;

} tn_ide_state;

INLINE tn_ide_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TNIDE);

	return (tn_ide_state *)downcast<legacy_device_base *>(device)->token();
}


/*
    CRU read
*/
static READ8Z_DEVICE_HANDLER( cru_rz )
{
	tn_ide_state *card = get_safe_token(device);
	UINT8 reply = 0;
	if ((offset & 0xff00)==CRU_BASE)
	{
		int bit = (offset >> 4) & 7;

		if (bit==0)
		{
			reply = card->cru_register & 0x30;
			reply |= 8;	/* IDE bus IORDY always set */
			if (!card->clk_irq)
				reply |= 4;
			if (card->sram_enable_dip)
				reply |= 2;
			if (!card->ide_irq)
				reply |= 1;
		}
		*value = reply;
	}
}

/*
    CRU write
*/
static WRITE8_DEVICE_HANDLER( cru_w )
{
	tn_ide_state *card = get_safe_token(device);

	if ((offset & 0xff00)==CRU_BASE)
	{
		int bit = (offset >>1) & 7;
		switch (bit)
		{
		case 0:			/* turn card on: handled by core */
			card->selected = data;

		case 1:			/* enable SRAM or registers in 0x4000-0x40ff */
			card->sram_enable = data;
			break;

		case 2:			/* enable SRAM page switching */
		case 3:			/* force SRAM page 0 */
		case 4:			/* enable SRAM in 0x6000-0x7000 ("RAMBO" mode) */
		case 5:			/* write-protect RAM */
		case 6:			/* irq and reset enable */
		case 7:			/* reset drive */
			if (data)
				card->cru_register |= 1 << bit;
			else
				card->cru_register &= ~(1 << bit);

			if (bit == 6)
				devcb_call_write_line(&card->lines.inta, (card->cru_register & cru_reg_int_en) && card->ide_irq);

			if ((bit == 6) || (bit == 7))
				if ((card->cru_register & cru_reg_int_en) && !(card->cru_register & cru_reg_reset))
					card->ide->reset();
			break;
		}
	}
}

/*
    Memory read
*/
static READ8Z_DEVICE_HANDLER( data_rz )
{
	tn_ide_state *card = get_safe_token(device);
	UINT8 reply = 0;

	if (((offset & card->select_mask)==card->select_value) && card->selected)
	{
		int addr = offset & 0x1fff;

		if ((addr <= 0xff) && (card->sram_enable == card->sram_enable_dip))
		{	/* registers */
			switch ((addr >> 5) & 0x3)
			{
			case 0:		/* RTC RAM */
				if (addr & 0x80)
					/* RTC RAM page register */
					reply = rtc65271_xram_r(card->rtc, (addr & 0x1f) | 0x20);
				else
					/* RTC RAM read */
					reply = rtc65271_xram_r(card->rtc, addr);
				break;
			case 1:		/* RTC registers */
				if (addr & 0x10)
					/* register data */
					reply = rtc65271_rtc_r(card->rtc, 1);
				else
					/* register select */
					reply = rtc65271_rtc_r(card->rtc, 0);
				break;
			case 2:		/* IDE registers set 1 (CS1Fx) */
				if (card->tms9995_mode ? (!(addr & 1)) : (addr & 1))
				{	/* first read triggers 16-bit read cycle */
					card->input_latch = (! (addr & 0x10)) ? ide_bus_r(card->ide, 0, (addr >> 1) & 0x7) : 0;
				}

				/* return latched input */
				/*reply = (addr & 1) ? input_latch : (input_latch >> 8);*/
				/* return latched input - bytes are swapped in 2004 IDE card */
				reply = ((addr & 1) ? (card->input_latch >> 8) : card->input_latch) & 0xff;
				break;
			case 3:		/* IDE registers set 2 (CS3Fx) */
				if (card->tms9995_mode ? (!(addr & 1)) : (addr & 1))
				{	/* first read triggers 16-bit read cycle */
					card->input_latch = (! (addr & 0x10)) ? ide_bus_r(card->ide, 1, (addr >> 1) & 0x7) : 0;
				}

				/* return latched input */
				/*reply = (addr & 1) ? input_latch : (input_latch >> 8);*/
				/* return latched input - bytes are swapped in 2004 IDE card */
				reply = ((addr & 1) ? (card->input_latch >> 8) : card->input_latch) & 0xff;
				break;
			}
		}
		else
		{	/* sram */
			if ((card->cru_register & cru_reg_page_0) || (addr >= 0x1000))
				reply = card->ram[addr+0x2000 * card->cur_page];
			else
				reply = card->ram[addr];
		}
		*value = reply;
	}
}

/*
    Memory write. The controller is 16 bit, so we need to demultiplex again.
*/
static WRITE8_DEVICE_HANDLER( data_w )
{
	tn_ide_state *card = get_safe_token(device);

	if (((offset & card->select_mask)==card->select_value) && card->selected)
	{
		if (card->cru_register & cru_reg_page_switching)
		{
			card->cur_page = (offset >> 1) & page_mask;
		}

		int addr = offset & 0x1fff;

		if ((addr <= 0xff) && (card->sram_enable == card->sram_enable_dip))
		{	/* registers */
			switch ((addr >> 5) & 0x3)
			{
			case 0:		/* RTC RAM */
				if (addr & 0x80)
					/* RTC RAM page register */
					rtc65271_xram_w(card->rtc, (addr & 0x1f) | 0x20, data);
				else
					/* RTC RAM write */
					rtc65271_xram_w(card->rtc, addr, data);
				break;
			case 1:		/* RTC registers */
				if (addr & 0x10)
					/* register data */
					rtc65271_rtc_w(card->rtc, 1, data);
				else
					/* register select */
					rtc65271_rtc_w(card->rtc, 0, data);
				break;
			case 2:		/* IDE registers set 1 (CS1Fx) */
/*
                if (addr & 1)
                    card->output_latch = (card->output_latch & 0xff00) | data;
                else
                    card->output_latch = (card->output_latch & 0x00ff) | (data << 8);
*/
				/* latch write - bytes are swapped in 2004 IDE card */
				if (addr & 1)
					card->output_latch = (card->output_latch & 0x00ff) | (data << 8);
				else
					card->output_latch = (card->output_latch & 0xff00) | data;

				if (card->tms9995_mode ? (addr & 1) : (!(addr & 1)))
				{	/* second write triggers 16-bit write cycle */
					ide_bus_w(card->ide, 0, (addr >> 1) & 0x7, card->output_latch);
				}
				break;
			case 3:		/* IDE registers set 2 (CS3Fx) */
/*
                if (addr & 1)
                    card->output_latch = (card->output_latch & 0xff00) | data;
                else
                    card->output_latch = (card->output_latch & 0x00ff) | (data << 8);
*/
				/* latch write - bytes are swapped in 2004 IDE card */
				if (addr & 1)
					card->output_latch = (card->output_latch & 0x00ff) | (data << 8);
				else
					card->output_latch = (card->output_latch & 0xff00) | data;

				if (card->tms9995_mode ? (addr & 1) : (!(addr & 1)))
				{	/* second write triggers 16-bit write cycle */
					ide_bus_w(card->ide, 1, (addr >> 1) & 0x7, card->output_latch);
				}
				break;
			}
		}
		else
		{	/* sram */
			if (! (card->cru_register & cru_reg_wp))
			{
				if ((card->cru_register & cru_reg_page_0) || (addr >= 0x1000))
					card->ram[addr+0x2000 * card->cur_page] = data;
				else
					card->ram[addr] = data;
			}
		}
	}
}

/**************************************************************************/

static const ti99_peb_card tn_ide_card =
{
	data_rz, data_w,				// memory access read/write
	cru_rz, cru_w,					// CRU access
	NULL, NULL,						// SENILA/B access
	NULL, NULL						// 16 bit access (none here)
};

/*
    ti99_ide_interrupt()
    IDE interrupt callback
*/
static void ide_interrupt_callback(device_t *device, int state)
{
	tn_ide_state *card = get_safe_token(device->owner());
	card->ide_irq = state;
	if (card->cru_register & cru_reg_int_en)
		devcb_call_write_line(&card->lines.inta, state);
}

/*
    clk_interrupt_callback()
    clock interrupt callback
*/
static void clock_interrupt_callback(device_t *device, int state)
{
	tn_ide_state *card = get_safe_token(device->owner());
	card->clk_irq = state;
	devcb_call_write_line(&card->lines.inta, state);
}


static DEVICE_START( tn_ide )
{
	tn_ide_state *card = get_safe_token(device);
	card->rtc = device->subdevice("ide_rtc");
	card->ide = device->subdevice("ide");

	peb_callback_if *topeb = (peb_callback_if *)device->baseconfig().static_config();
	devcb_resolve_write_line(&card->lines.inta, &topeb->inta, device);

	card->ram = auto_alloc_array(device->machine(), UINT8, 0x080000);
	card->sram_enable_dip = 0;
}

static DEVICE_STOP( tn_ide )
{
}

static DEVICE_RESET( tn_ide )
{
	tn_ide_state *card = get_safe_token(device);
	device_t *peb = device->owner();

	if (input_port_read(device->machine(), "HDCTRL") & HD_IDE)
	{
		int success = mount_card(peb, device, &tn_ide_card, get_pebcard_config(device)->slot);
		if (!success) return;

		card->cur_page = 0;
		card->sram_enable = 0;
		card->cru_register = 0;

		card->select_mask = 0x7e000;
		card->select_value = 0x74000;

		if (input_port_read(device->machine(), "MODE")==GENMOD)
		{
			// GenMod card modification
			card->select_mask = 0x1fe000;
			card->select_value = 0x174000;
		}

		card->tms9995_mode =  (device->type()==TMS9995);
	}
}

MACHINE_CONFIG_FRAGMENT( tn_ide )
	MCFG_RTC65271_ADD( "ide_rtc", clock_interrupt_callback )
	MCFG_IDE_CONTROLLER_ADD( "ide", ide_interrupt_callback )
MACHINE_CONFIG_END

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##tn_ide##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_INLINE_CONFIG | DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_NAME                "Nouspikel IDE controller card"
#define DEVTEMPLATE_FAMILY              "Peripheral expansion"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( TNIDE, tn_ide );

