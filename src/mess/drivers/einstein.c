/******************************************************************************

    Tatung Einstein
    system driver


    TMS9129 VDP Graphics
        16k ram

    Z80 CPU (4 MHz)

    Z80 CTC (4 MHz)
        channel 0 is serial transmit clock
        channel 1 is serial receive clock
        trigger for channel 0,1 and 2 is a 2 MHz clock
        trigger for channel 3 is the terminal count of channel 2

    Intel 8251 Serial (2 MHz clock?)

    WD1770 Floppy Disc controller
        density is fixed, 4 drives and double sided supported

    AY-3-8910 PSG (2 MHz)
        port A and port B are connected to the keyboard. Port A is keyboard
        line select, Port B is data.

    printer connected to port A of PIO. /ACK from printer is connected to /ASTB.
    D7-D0 of PIO port A is printer data lines.
    ARDY of PIO is connected to /STROBE on printer.

    user port is port B of PIO
    keyboard connected to port A and port B of PSG

    TODO:
    - The ADC is not emulated!
    - printer emulation needs checking!

    Many thanks to Chris Coxall for the schematics of the TC-01, the dump of the
    system rom and a dump of a Xtal boot disc.

    Many thanks to Andrew Dunipace for his help with the 80-column card
    and Speculator hardware (Spectrum emulator).

    Kevin Thacker [MESS driver]



 ******************************************************************************/


/* I/O ports */
/* 0x00-0x07 PSG */
/* 0x08-0x0f VDP */
/* 0x10-0x17 SERIAL */
/* 0x18-0x1f FDC */
/* 0x20-0x27 MISC (see below) */
/* 0x28-0x2f CTC */
/* 0x30-0x37 PIO */
/* 0x38-0x3f ADC */

/* MISC */
/* 0x20 - bit 0 is keyboard int mask; read to get state of some keys and /fire button states;
        set to 0 to ENABLE interrupt; 1 to DISABLE interrupt; write to set mask. */
/* 0x21 - bit 0 is adc int mask; set to 0 to ENABLE interrupt; 1 to DISABLE interrupt;
        write to set mask; read has no effect */
/* 0x22 - alph */
/* 0x23 - drive select and side select */
/* 0x24 - rom */
/* 0x25 - bit 0 is fire int mask; set to 0 to ENABLE interrupt; 1 to DISABLE interrupt;
        write to set mask; read has no effect */
/* 0x25 - */
/* 0x26 - */
/* 0x27 - */

/* 0x40-0xff for expansion */


/* speculator */
/* 7e = 0, read from fe, 7e = 1, read from fe */

#include "driver.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "video/tms9928a.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/wd17xx.h"
#include "machine/ctronics.h"
#include "machine/msm8251.h"
#include "devices/dsk.h"
#include "devices/basicdsk.h"
#include "sound/ay8910.h"
#include "video/mc6845.h"

#define EINSTEIN_SYSTEM_CLOCK 4000000

#define EINSTEIN_KEY_INT	(1<<0)
#define EINSTEIN_ADC_INT		(1<<1)
#define EINSTEIN_FIRE_INT		(1<<2)

static int einstein_rom_enabled = 1;

static int einstein_int = 0;
static int einstein_int_mask = 0;

static int einstein_ctc_trigger = 0;

static const device_config *einstein_z80pio;

/* KEYBOARD */
static int einstein_keyboard_line = 0;
static int einstein_keyboard_data = 0x0ff;

#define EINSTEIN_DUMP_RAM

#ifdef EINSTEIN_DUMP_RAM
static void einstein_dump_ram(void)
{
	ram_dump("einstein.bin");
}
#endif

/**********************************************************/
/*
    80 column board has a UM6845,2K ram and a char rom:

    0x040-0x047 used to access 2K ram. (bits 3-0 define a 256 byte row, bits 15-8 define
                the offset in the row)
    0x048 = crtc register index (w),
    0x049 = crtc data register (w)

    0x04c
        bit 2 = 50/60Hz mode?
        bit 1 = 1
        bit 0 = vsync state?

    0: 126 (127) horizontal total
    1: 80 horizontal displayed
    2: 97 horizontal sync position
    3: &38
    4: 26 vertical total
    5: 19 vertical adjust
    6: 25 vertical displayed
    7: 26 vertical sync pos
    8: 0 no interlace
    9: 8 (9 scanlines tall)
    10: 32

  total scanlines: ((reg 9+1) * (reg 4+1))+reg 5 = 262
  127 cycles per line
  127*262 = 33274 cycles per frame, vsync len 375 cycles


*/
static int einstein_80col_state;
static char *einstein_80col_ram = NULL;

static WRITE8_HANDLER(einstein_80col_ram_w)
{
	/* lower 3 bits of address define a 256-byte "row".
        upper 8 bits define the offset in the row,
        data bits define data to write */
	einstein_80col_ram[((offset & 0x07)<<8)|((offset>>8) & 0x0ff)] = data;
}

static  READ8_HANDLER(einstein_80col_ram_r)
{
	return einstein_80col_ram[((offset & 0x07)<<8)|((offset>>8) & 0x0ff)];
}

static  READ8_HANDLER(einstein_80col_state_r)
{
	/* fake vsync for now */
	einstein_80col_state^=0x01;
	return einstein_80col_state;
}

//static int Einstein_6845_RA = 0;
//static int Einstein_scr_x = 0;
static int Einstein_scr_y = 0;
//static int Einstein_HSync = 0;
//static int Einstein_VSync = 0;
//static int Einstein_DE = 0;

//// called when the 6845 changes the character row
//static void Einstein_Set_RA(int offset, int data)
//{
//  Einstein_6845_RA=data;
//}
//
//
//// called when the 6845 changes the HSync
//static void Einstein_Set_HSync(running_machine *machine, int offset, int data)
//{
//  Einstein_HSync=data;
//  if(!Einstein_HSync)
//  {
//      Einstein_scr_y++;
//      Einstein_scr_x = -40;
//  }
//}
//
//// called when the 6845 changes the VSync
//static void Einstein_Set_VSync(running_machine *machine, int offset, int data)
//{
//  Einstein_VSync=data;
//  if (!Einstein_VSync)
//  {
//      Einstein_scr_y = 0;
//  }
//}
//
//static void Einstein_Set_DE(int offset, int data)
//{
//  Einstein_DE = data;
//}


//static const struct m6845_interface einstein_m6845_interface = {
//  0,// Memory Address register
//  Einstein_Set_RA,// Row Address register
//  Einstein_Set_HSync,// Horizontal status
//  Einstein_Set_VSync,// Vertical status
//  Einstein_Set_DE,// Display Enabled status
//  0,// Cursor status
//};

static MC6845_UPDATE_ROW( einstein_6845_update_row )
{
	/* TODO: Verify implementation */
	unsigned char *data = memory_region(device->machine, "maincpu") + 0x012000;
	unsigned char data_byte;
	int char_code;
	int i, x;

	for ( i = 0, x = 0; i < x_count; i++, x+=8 )
	{
		int w;
		char_code = einstein_80col_ram[(ma + i)&0x07ff];
		data_byte = data[(char_code<<3) + ra];
		for (w=0; w<8;w++)
		{
			*BITMAP_ADDR16(bitmap, y, x+w) = (data_byte & 0x080) ? 1 : 0;
			data_byte = data_byte<<1;
		}
	}
}

static WRITE_LINE_DEVICE_HANDLER( einstein_6845_display_enable_changed )
{
	/* TODO: Implement me properly */
	if ( state ) {
		Einstein_scr_y = ( Einstein_scr_y + 1 ) % 240 /*?*/;
	}
}

static const mc6845_interface einstein_crtc6845_interface = {
	"screen",
	8 /*?*/,
	NULL,
	einstein_6845_update_row,
	NULL,
	DEVCB_LINE(einstein_6845_display_enable_changed),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};

/* 80 column card init */
static void	einstein_80col_init(running_machine *machine)
{
	/* 2K RAM */
	einstein_80col_ram = auto_alloc_array(machine, char, 2048);

	einstein_80col_state=(1<<2)|(1<<1);
}

static  READ8_HANDLER(einstein_80col_r)
{
	switch (offset & 0x0f)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			return einstein_80col_ram_r(space, offset);
		case 0x0c:
			return einstein_80col_state_r(space, offset);
		default:
			break;
	}
	return 0x0ff;
}


static WRITE8_HANDLER(einstein_80col_w)
{
	const device_config *mc6845 = devtag_get_device(space->machine, "crtc");

	switch (offset & 0x0f)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			einstein_80col_ram_w(space, offset,data);
			break;
		case 8:
			mc6845_address_w( mc6845, offset, data );
			break;
		case 9:
			mc6845_register_w( mc6845, offset, data );
			break;
		default:
			break;
	}
}

static TIMER_CALLBACK(einstein_ctc_trigger_callback)
{
	const device_config *device = ptr;

	einstein_ctc_trigger^=1;

	/* channel 0 and 1 have a 2 MHz input clock for triggering */
	z80ctc_trg0_w(device, 0, einstein_ctc_trigger);
	z80ctc_trg1_w(device, 0, einstein_ctc_trigger);
}

/* refresh keyboard data. It is refreshed when the keyboard line is written */
static void einstein_scan_keyboard(running_machine *machine)
{
	unsigned char data = 0x0ff;
	int i;
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7" };

	for (i=0; i<8; i++)
	{
		if ((einstein_keyboard_line & (1<<i)) == 0)
		{
			data &= input_port_read(machine, keynames[i]);
		}
	}

	einstein_keyboard_data = data;
}

static void einstein_update_interrupts(running_machine *machine)
{
	/* NPW 21-Jul-2005 - Not sure how to update this for MAME 0.98u2 */
/*
    if (einstein_int & einstein_int_mask & EINSTEIN_KEY_INT)
        cputag_set_input_line(machine, "maincpu", Z80_INT_REQ, PULSE_LINE);
    else
        cputag_set_input_line(machine, "maincpu", Z80_INT_IEO, PULSE_LINE);

    if (einstein_int & einstein_int_mask & EINSTEIN_ADC_INT)
        cputag_set_input_line(machine, "maincpu", Z80_INT_REQ, PULSE_LINE);
    else
        cputag_set_input_line(machine, "maincpu", Z80_INT_IEO, PULSE_LINE);

    if (einstein_int & einstein_int_mask & EINSTEIN_FIRE_INT)
        cputag_set_input_line(machine, "maincpu", Z80_INT_REQ, PULSE_LINE);
    else
        cputag_set_input_line(machine, "maincpu", Z80_INT_IEO, PULSE_LINE);
*/
}

static TIMER_CALLBACK(einstein_keyboard_timer_callback)
{
	einstein_scan_keyboard(machine);

	/* if /fire1 or /fire2 is 0, then trigger a fire interrupt if the interrupt is enabled */
	if ((input_port_read(machine, "BUTTONS") & 0x03)!=0)
	{
		einstein_int |= EINSTEIN_FIRE_INT;
	}
	else
	{
		einstein_int &= ~EINSTEIN_FIRE_INT;
	}

	/* keyboard data changed? */
	if (einstein_keyboard_data!=0x0ff)
	{
		/* generate interrupt */
		einstein_int |= EINSTEIN_KEY_INT;
	}
	else
	{
		einstein_int &= ~EINSTEIN_KEY_INT;
	}

	einstein_update_interrupts(machine);
}


/* interrupt state callback for ctc */
static void einstein_ctc_interrupt(const device_config *device, int state)
{
	logerror("ctc irq state: %02x\n", state);
	cputag_set_input_line(device->machine, "maincpu", 1, state);
}

static void einstein_pio_interrupt(const device_config *device, int state)
{
	logerror("pio irq state: %02x\n", state);
	cputag_set_input_line(device->machine, "maincpu", 3, state);
}

static WRITE8_DEVICE_HANDLER(einstein_serial_transmit_clock)
{
	const device_config *uart = devtag_get_device(device->machine, "uart");
	msm8251_transmit_clock(uart);
}

static WRITE8_DEVICE_HANDLER(einstein_serial_receive_clock)
{
	const device_config *uart = devtag_get_device(device->machine, "uart");
	msm8251_receive_clock(uart);
}

static const z80ctc_interface einstein_ctc_intf =
{
	0,
	einstein_ctc_interrupt,
	einstein_serial_transmit_clock,
	einstein_serial_receive_clock,
	z80ctc_trg3_w
};


static void einstein_pio_ardy(const device_config *device, int state)
{
	const device_config *printer = devtag_get_device(device->machine, "centronics");
	centronics_strobe_w(printer, state);
}

static WRITE8_DEVICE_HANDLER( einstein_pio_port_a_w )
{
	const device_config *printer = devtag_get_device(device->machine, "centronics");
	centronics_data_w(printer, 0, data);
}


static const z80pio_interface einstein_pio_intf =
{
	DEVCB_LINE(einstein_pio_interrupt),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(einstein_pio_port_a_w),
	DEVCB_NULL,
	DEVCB_LINE(einstein_pio_ardy),
	DEVCB_NULL
};


/****************************************************************
	Einstein specific keyboard daisy chain code
****************************************************************/

static int einstein_daisy_irq_state(const device_config *device)
{
	return 0xFF;
}


static void einstein_daisy_irq_reti(const device_config *device)
{
}


static DEVICE_START( einstein_daisy )
{
}


static DEVICE_RESET( einstein_daisy )
{
}


static DEVICE_GET_INFO( einstein_daisy )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = 4;											break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;											break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;						break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(einstein_daisy);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */											break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(einstein_daisy);		break;
		case DEVINFO_FCT_IRQ_STATE:						info->f = (genf *)einstein_daisy_irq_state;				break;
		case DEVINFO_FCT_IRQ_RETI:						info->f = (genf *)einstein_daisy_irq_reti;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Einstein daisy");								break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Z80");										break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);										break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");					break;
	}
}


#ifdef UNUSED_FUNCTION
/* not required for this interrupt source */
static void einstein_fire_int_reset(int which)
{
	einstein_int_mask &= ~EINSTEIN_FIRE_INT;

	einstein_update_interrupts(Machine);
}


static int einstein_fire_interrupt(int which)
{
	logerror("fire int routine in daisy chain\n");
	/* return vector */
	return 0x0ff;
}


/* reti has no effect on this interrupt */
static void einstein_fire_reti(int which)
{
}
#endif

static const z80_daisy_chain einstein_daisy_chain[] =
{
	{ "keyboard_daisy" },
//	{einstein_keyboard_int_reset, einstein_keyboard_interrupt, 0, einstein_keyboard_reti, 0},
	{ "z80ctc" },
	{ "adc_daisy" },
//	{einstein_adc_int_reset,einstein_adc_interrupt, 0, einstein_adc_reti, 0},
	{ "z80pio" },
//  {einstein_fire_int_reset,einstein_fire_interrupt, einstein_fire_reti, 0},
	{ NULL }
};

static  READ8_HANDLER(einstein_vdp_r)
{
	/*logerror("vdp r: %04x\n",offset); */

	if (offset & 0x01)
	{
		return TMS9928A_register_r(space, offset & 0x01);
	}

	return TMS9928A_vram_r(space, offset & 0x01);
}

static WRITE8_HANDLER(einstein_vdp_w)
{
	/* logerror("vdp w: %04x %02x\n",offset,data); */

	if (offset & 0x01)
	{
		TMS9928A_register_w(space, offset & 0x01,data);
		return;
	}

	TMS9928A_vram_w(space, offset & 0x01,data);
}

static WRITE8_HANDLER(einstein_fdc_w)
{
	int reg = offset & 0x03;
	const device_config *fdc = devtag_get_device(space->machine, "wd177x");

	logerror("fdc w: PC: %04x %04x %02x\n", cpu_get_pc(cputag_get_cpu(space->machine,"maincpu")), offset, data);

	switch (reg)
	{
		case 0:
		{
			wd17xx_command_w(fdc, reg, data);
		}
		break;

		case 1:
		{
			wd17xx_track_w(fdc, reg, data);
		}
		break;

		case 2:
		{
			wd17xx_sector_w(fdc, reg, data);
		}
		break;

		case 3:
		{
			wd17xx_data_w(fdc, reg, data);
		}
		break;
	}
}


static  READ8_HANDLER(einstein_fdc_r)
{
	int reg = offset & 0x03;
	const device_config *fdc = devtag_get_device(space->machine, "wd177x");

	logerror("fdc r: PC: %04x %04x\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset);

	switch (reg)
	{
		case 0:
		{
			return wd17xx_status_r(fdc, reg);
		}
		break;

		case 1:
		{
			return wd17xx_track_r(fdc, reg);
		}
		break;

		case 2:
		{
			return wd17xx_sector_r(fdc, reg);
		}
		break;

		case 3:
		{
			return wd17xx_data_r(fdc, reg);
		}
		break;
	}

	return 0x0ff;
}

static WRITE8_HANDLER(einstein_pio_w)
{
	logerror("pio w: %04x %02x\n",offset,data);

	z80pio_w( einstein_z80pio, offset, data );
}

static  READ8_HANDLER(einstein_pio_r)
{
	logerror("pio r: %04x\n",offset);

	return z80pio_r( einstein_z80pio, offset );
}

static  READ8_DEVICE_HANDLER(einstein_ctc_r)
{
	logerror("ctc r: %04x\n",offset);

	return z80ctc_r(device, offset & 0x03);
}

static WRITE8_DEVICE_HANDLER(einstein_ctc_w)
{
	logerror("ctc w: %04x %02x\n",offset,data);

	z80ctc_w(device, offset & 0x03,data);
}

static WRITE8_HANDLER(einstein_serial_w)
{
	const device_config *uart = devtag_get_device(space->machine, "uart");
	int reg = offset & 0x01;

	/* logerror("serial w: %04x %02x\n",offset,data); */

	if ((reg)==0)
	{
		msm8251_data_w(uart, reg,data);
		return;
	}

	msm8251_control_w(uart, reg,data);
}


static READ8_HANDLER(einstein_serial_r)
{
	const device_config *uart = devtag_get_device(space->machine, "uart");
	int reg = offset & 0x01;

	/* logerror("serial r: %04x\n",offset); */

	if ((reg)==0)
	{
		return msm8251_data_r(uart, reg);
	}

	return msm8251_status_r(uart, reg);
}

/*

    AY-3-8912 PSG:

    NOTE: BC2 is connected to +5v

    BDIR    BC1     FUNCTION
    0       0       INACTIVE
    0       1       READ FROM PSG
    1       0       WRITE TO PSG
    1       1       LATCH ADDRESS

    /PSG select, /WR connected to NOR -> BDIR

    when /PSG=0 and /WR=0, BDIR is 1. (write to psg or latch address)

    /PSG select, A0 connected to NOR -> BC1
    when A0 = 0, BC1 is 1. when A0 = 1, BC1 is 0.

    when /PSG = 1, BDIR is 0 and BC1 is 0.


*/


static WRITE8_HANDLER(einstein_psg_w)
{
	const device_config *ay8910 = devtag_get_device(space->machine, "ay8910");
	int reg = offset & 0x03;

	/*logerror("psg w: %04x %02x\n",offset,data); */

	switch (reg)
	{
		/* case 0 and 1 are not handled */
		case 2:
		{
			ay8910_address_w(ay8910, 0, data);
		}
		break;

		case 3:
		{
			ay8910_data_w(ay8910, 0, data);
		}
		break;

		default:
			break;
	}
}

static READ8_HANDLER(einstein_psg_r)
{
	const device_config *ay8910 = devtag_get_device(space->machine, "ay8910");
	int reg = offset & 0x03;

	switch (reg)
	{
		/* case 0 and 1 are not handled */
		case 2:
			return ay8910_r(ay8910, 0);

		default:
			break;
	}

	return 0x0ff;
}


/* int priority */
/* keyboard int->ctc/adc->pio */


static ADDRESS_MAP_START( einstein_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x01fff) AM_READWRITE(SMH_BANK(1), SMH_BANK(3))
	AM_RANGE(0x2000, 0x0ffff) AM_READWRITE(SMH_BANK(2), SMH_BANK(4))
ADDRESS_MAP_END



static void einstein_page_rom(running_machine *machine)
{
	if (einstein_rom_enabled)
	{
		memory_set_bankptr(machine, 1, memory_region(machine, "maincpu")+0x010000);
	}
	else
	{
#ifdef EINSTEIN_DUMP_RAM
		einstein_dump_ram();
#endif
		memory_set_bankptr(machine, 1, mess_ram);
	}
}

static WRITE8_HANDLER(einstein_drive_w)
{
	const device_config *fdc = devtag_get_device(space->machine, "wd177x");

	/* bit 4: side select */
	/* bit 3: select drive 3 */
	/* bit 2: select drive 2 */
	/* bit 1: select drive 1 */
	/* bit 0: select drive 0 */

	logerror("drive w: PC: %04x %04x %02x\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset, data);

	wd17xx_set_side(fdc, (data >> 4) & 0x01);

	if (data & 0x01)
	{
		wd17xx_set_drive(fdc, 0);
	}
	else
	if (data & 0x02)
	{
		wd17xx_set_drive(fdc, 1);
	}
	else
	if (data & 0x04)
	{
		wd17xx_set_drive(fdc, 2);
	}
	else
	if (data & 0x08)
	{
		wd17xx_set_drive(fdc, 3);
	}
}

static WRITE8_HANDLER(einstein_rom_w)
{
	einstein_rom_enabled ^= 1;
	einstein_page_rom(space->machine);
}

static READ8_HANDLER(einstein_key_int_r)
{
	const device_config *printer = devtag_get_device(space->machine, "centronics");
	int data;

	/* clear key int. a read of this I/O port will do this or a reset */
	einstein_int &= ~EINSTEIN_KEY_INT;

	einstein_update_interrupts(space->machine);

	/* bit 7: 0=shift pressed */
	/* bit 6: 0=control pressed */
	/* bit 5: 0=graph pressed */
	/* bit 4: 0=error from printer? */
	/* bit 3: 0=paper empty */
	/* bit 2: 1=printer busy */
	/* bit 1: fire 1 */
	/* bit 0: fire 0 */
	data = ((input_port_read(space->machine, "EXTRA") & 0x07)<<5) | (input_port_read(space->machine, "BUTTONS") & 0x03) | 0x01c;

	data |= centronics_fault_r(printer) << 4;
	data |= centronics_pe_r(printer) << 3;
	data |= centronics_busy_r(printer) << 2;

	logerror("key int r: %02x\n",data);

	return data;
}


static WRITE8_HANDLER(einstein_key_int_w)
{
	logerror("key int w: %02x\n",data);

	/* set mask from bit 0 */
	if (data & 0x01)
	{
		logerror("key int is disabled\n");
		einstein_int_mask &= ~EINSTEIN_KEY_INT;
	}
	else
	{
		logerror("key int is enabled\n");
		einstein_int_mask |= EINSTEIN_KEY_INT;
	}

	einstein_update_interrupts(space->machine);
}

static WRITE8_HANDLER(einstein_adc_int_w)
{
	logerror("adc int w: %02x\n",data);
	/* writing to this I/O port sets the state of the mask; D0 is used */
	/* writing 0 enables the /ADC interrupt */

	if (data & 0x01)
	{
		logerror("adc int is disabled\n");
		einstein_int_mask &= ~EINSTEIN_ADC_INT;
	}
	else
	{
		logerror("adc int is enabled\n");
		einstein_int_mask |= EINSTEIN_ADC_INT;
	}

	einstein_update_interrupts(space->machine);
}

static WRITE8_HANDLER(einstein_fire_int_w)
{
	logerror("fire int w: %02x\n",data);

	/* writing to this I/O port sets the state of the mask; D0 is used */
	/* writing 0 enables the /FIRE interrupt */

	if (data & 0x01)
	{
		logerror("fire int is disabled\n");
		einstein_int_mask &= ~EINSTEIN_FIRE_INT;
	}
	else
	{
		logerror("fire int is enabled\n");
		einstein_int_mask |= EINSTEIN_FIRE_INT;
	}

	einstein_update_interrupts(space->machine);
}


static READ8_HANDLER(einstein2_port_r)
{
	switch (offset & 0x0ff)
	{
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			return einstein_psg_r(space, offset);
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
			return einstein_vdp_r(space, offset);
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			return einstein_serial_r(space, offset);
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
			return einstein_fdc_r(space, offset);
		case 0x20:
			return einstein_key_int_r(space, offset);
		case 0x28:
		case 0x29:
		case 0x2a:
		case 0x2b:
		case 0x2c:
		case 0x2d:
		case 0x2e:
		case 0x2f:
			return einstein_ctc_r( devtag_get_device(space->machine, "z80ctc"), offset);
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
			return einstein_pio_r(space, offset);
		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
		case 0x47:
		case 0x48:
		case 0x49:
		case 0x4a:
		case 0x4b:
		case 0x4c:
		case 0x4d:
		case 0x4e:
		case 0x4f:
			return einstein_80col_r(space, offset);

		default:
			break;
	}

	logerror("unhandled port r: %04x\n",offset);

	return 0xff;
}

static WRITE8_HANDLER(einstein2_port_w)
{
	switch (offset & 0x0ff)
	{
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			einstein_psg_w(space, offset,data);
			return;
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
			einstein_vdp_w(space, offset,data);
			return;
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			einstein_serial_w(space, offset,data);
			return;
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
			einstein_fdc_w(space, offset,data);
			return;
		case 0x20:
			einstein_key_int_w(space, offset,data);
			return;
		case 0x21:
			einstein_adc_int_w(space, offset,data);
			return;
		case 0x23:
			einstein_drive_w(space, offset,data);
			return;
		case 0x24:
			einstein_rom_w(space, offset,data);
			return;
		case 0x25:
			einstein_fire_int_w(space, offset,data);
			return;
		case 0x28:
		case 0x29:
		case 0x2a:
		case 0x2b:
		case 0x2c:
		case 0x2d:
		case 0x2e:
		case 0x2f:
			einstein_ctc_w( devtag_get_device(space->machine, "z80ctc"), offset,data);
			return;
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
			einstein_pio_w(space, offset,data);
			return;
		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
		case 0x47:
		case 0x48:
		case 0x49:
		case 0x4a:
		case 0x4b:
		case 0x4c:
		case 0x4d:
		case 0x4e:
		case 0x4f:
			einstein_80col_w(space, offset,data);
			return;

		default:
			break;
	}

	logerror("unhandled port w: %04x %02x\n",offset,data);
}


static  READ8_HANDLER(einstein_port_r)
{
	switch (offset & 0x0ff)
	{
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			return einstein_psg_r(space, offset);
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
			return einstein_vdp_r(space, offset);
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			return einstein_serial_r(space, offset);
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
			return einstein_fdc_r(space, offset);
		case 0x20:
			return einstein_key_int_r(space, offset);
		case 0x28:
		case 0x29:
		case 0x2a:
		case 0x2b:
		case 0x2c:
		case 0x2d:
		case 0x2e:
		case 0x2f:
			return einstein_ctc_r( devtag_get_device(space->machine, "z80ctc"), offset);
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
			return einstein_pio_r(space, offset);

		default:
			break;
	}

	logerror("unhandled port r: %04x\n",offset);

	return 0xff;
}

static WRITE8_HANDLER(einstein_port_w)
{
	switch (offset & 0x0ff)
	{
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			einstein_psg_w(space, offset,data);
			return;
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
			einstein_vdp_w(space, offset,data);
			return;
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			einstein_serial_w(space, offset,data);
			return;
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
			einstein_fdc_w(space, offset,data);
			return;
		case 0x20:
			einstein_key_int_w(space, offset,data);
			return;
		case 0x21:
			einstein_adc_int_w(space, offset,data);
			return;
		case 0x23:
			einstein_drive_w(space, offset,data);
			return;
		case 0x24:
			einstein_rom_w(space, offset,data);
			return;
		case 0x25:
			einstein_fire_int_w(space, offset,data);
			return;
		case 0x28:
		case 0x29:
		case 0x2a:
		case 0x2b:
		case 0x2c:
		case 0x2d:
		case 0x2e:
		case 0x2f:
			einstein_ctc_w( devtag_get_device(space->machine, "z80ctc"), offset,data);
			return;
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
			einstein_pio_w(space, offset,data);
			return;

		default:
			break;
	}
	logerror("unhandled port w: %04x %02x\n",offset,data);
}


static ADDRESS_MAP_START( einstein2_io , ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000,0x0ffff) AM_READWRITE(einstein2_port_r, einstein2_port_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( einstein_io , ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000,0x0ffff) AM_READWRITE(einstein_port_r, einstein_port_w)
ADDRESS_MAP_END

#if 0
ADDRESS_MAP_START( readport_einstein , ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x000, 0x007) AM_READ( einstein_psg_r)
	AM_RANGE(0x008, 0x00f) AM_READ( einstein_vdp_r)
	AM_RANGE(0x010, 0x017) AM_READ( einstein_serial_r)
	AM_RANGE(0x018, 0x01f) AM_READ( einstein_fdc_r)
	AM_RANGE(0x020, 0x020) AM_READ( einstein_key_int_r)
	AM_RANGE(0x028, 0x02f) AM_READ( einstein_ctc_r)
	AM_RANGE(0x030, 0x037) AM_READ( einstein_pio_r)
//  {0x040, 0x0ff, einstein_unmapped_r},
ADDRESS_MAP_END

ADDRESS_MAP_START( writeport_einstein , ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x000, 0x007) AM_WRITE( einstein_psg_w)
	AM_RANGE(0x008, 0x00f) AM_WRITE( einstein_vdp_w)
	AM_RANGE(0x010, 0x017) AM_WRITE( einstein_serial_w)
	AM_RANGE(0x018, 0x01f) AM_WRITE( einstein_fdc_w)
	AM_RANGE(0x020, 0x020) AM_WRITE( einstein_key_int_w)
	AM_RANGE(0x023, 0x023) AM_WRITE( einstein_drive_w)
	AM_RANGE(0x024, 0x024) AM_WRITE( einstein_rom_w)
	AM_RANGE(0x028, 0x02f) AM_WRITE( einstein_ctc_w)
	AM_RANGE(0x030, 0x037) AM_WRITE( einstein_pio_w)
//  {0x040, 0x0ff, einstein_unmapped_w},
ADDRESS_MAP_END
#endif



/* when Z80 acknowledges int, /IORQ and /M1 will be low */
/* this allows I057 octal latch to output data onto the bus */
static IRQ_CALLBACK(einstein_cpu_acknowledge_int)
{
	int vector = 0;

	/* 8 different values for vector */

	/* vector */
	/* bits 7,6,5,4 and 0 are forced to "0" */

	/* if /adc int OR /ctc int, then pio is not allowed to interrupt */

	/* 4d set to 1 */
	/* 1d is set to 0 if key int has occured */
	/* 2d is set to 0 if adc int has occured and CTC int hasn't occured */
	/* 3d is set to 0 if fire int has occured but not if adc or CTC or PIO int has occured */

	/* priority (highest to lowest): */
	/* keyboard */
	/* ctc */
	/* adc */
	/* pio */
	/* fire */

	return (vector<<1);
}

static const TMS9928a_interface tms9928a_interface =
{
	TMS9929A,
	0x4000,
	0, 0,
	NULL
};

static MACHINE_START( einstein )
{
	TMS9928A_configure(&tms9928a_interface);

	einstein_z80pio = devtag_get_device(machine, "z80pio");
}

static MACHINE_RESET( einstein )
{
	memory_set_bankptr(machine, 2, mess_ram+0x02000);
	memory_set_bankptr(machine, 3, mess_ram);
	memory_set_bankptr(machine, 4, mess_ram+0x02000);

	TMS9928A_reset ();

	einstein_rom_enabled = 1;
	einstein_page_rom(machine);

	einstein_ctc_trigger = 0;

	einstein_int=0;
	/* a reset causes the fire int, adc int, keyboard int mask
    to be set to 1, which causes all these to be DISABLED */
	einstein_int_mask = 0;
	floppy_drive_set_geometry(image_from_devtype_and_index(machine, IO_FLOPPY, 0), FLOPPY_DRIVE_SS_40);

	cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"), einstein_cpu_acknowledge_int);

	/* the einstein keyboard can generate a interrupt */
	/* the int is actually clocked at the system clock 4 MHz, but this would be too fast for our
    driver. So we update at 50Hz and hope this is good enough. */
	timer_pulse(machine, ATTOTIME_IN_HZ(50), NULL, 0, einstein_keyboard_timer_callback);

	/* the input to channel 0 and 1 of the ctc is a 2 MHz clock */
	einstein_ctc_trigger = 0;
	timer_pulse(machine, ATTOTIME_IN_HZ(2000000), (void *)devtag_get_device(machine, "z80ctc"), 0, einstein_ctc_trigger_callback);
}

static MACHINE_RESET( einstein2 )
{
	MACHINE_RESET_CALL(einstein);
	einstein_80col_init(machine);
}

static INPUT_PORTS_START(einstein)
	PORT_START("LINE0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BREAK") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("?") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F0") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("?") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))

	PORT_START("LINE1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEFT") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('\xA3')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DOWN") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\xBA') PORT_CHAR('\xBD')	// is \xBA correct for double vertical bar || ?
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('@')

	PORT_START("LINE2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RIGHT") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F5))

	PORT_START("LINE3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DELETE") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('-')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("UP") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F4))

	PORT_START("LINE4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F3))

	PORT_START("LINE5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F2))

	PORT_START("LINE6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F1))

	PORT_START("LINE7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F6))

	PORT_START("EXTRA")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GRPH") PORT_CODE(KEYCODE_F8)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CONTROL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	/* fire buttons for analogue joysticks */
	PORT_START("BUTTONS")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("Joystick 1 Button 1")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("Joystick 2 Button 1") PORT_PLAYER(2)

	/* analog joystick 1 x axis */
	PORT_START("JOY1_X")
	PORT_BIT(0xff, 0x80, IPT_AD_STICK_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_REVERSE

	/* analog joystick 1 y axis */
	PORT_START("JOY1_Y")
	PORT_BIT(0xff, 0x80, IPT_AD_STICK_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_REVERSE

	/* analog joystick 2 x axis */
	PORT_START("JOY2_X")
	PORT_BIT(0xff, 0x80, IPT_AD_STICK_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(2) PORT_REVERSE

	/* analog joystick 2 Y axis */
	PORT_START("JOY2_Y")
	PORT_BIT(0xff, 0x80, IPT_AD_STICK_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(2) PORT_REVERSE

INPUT_PORTS_END

static WRITE8_HANDLER(einstein_port_a_write)
{
	einstein_keyboard_line = data;

//  logerror("line: %02x\n",einstein_keyboard_line);

	/* re-scan the keyboard */
	einstein_scan_keyboard(space->machine);
}

static READ8_HANDLER(einstein_port_b_read)
{
	einstein_scan_keyboard(space->machine);

//  logerror("key: %02x\n",einstein_keyboard_data);

	return einstein_keyboard_data;
}

static const ay8910_interface einstein_ay_interface =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_NULL,
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, einstein_port_b_read),
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, einstein_port_a_write),
	DEVCB_NULL
};

static const centronics_interface einstein_centronics_config =
{
	FALSE,
	DEVCB_DEVICE_LINE("z80pio", z80pio_astb_w),
	DEVCB_NULL,
	DEVCB_NULL
};

/*
    0: 126 (127) horizontal total
    1: 80 horizontal displayed
    2: 97 horizontal sync position
    3: &38
    4: 26 vertical total
    5: 19 vertical adjust
    6: 25 vertical displayed
    7: 26 vertical sync pos
    8: 0 no interlace
    9: 8 (9 scanlines tall)
    10: 32

  total scanlines: ((reg 9+1) * (reg 4+1))+reg 5 = 262
  127 cycles per line
  127*262 = 33274 cycles per frame, vsync len 375 cycles
*/

//static void einstein_80col_plot_char_line(int x,int y, bitmap_t *bitmap)
//{
//  int w;
//  if (Einstein_DE)
//  {
//
//      unsigned char *data = memory_region(machine, "maincpu")+0x012000;
//      unsigned char data_byte;
//      int char_code;
//
//      char_code = einstein_80col_ram[m6845_memory_address_r(0)&0x07ff];
//
//      data_byte = data[(char_code<<3) + Einstein_6845_RA];
//
//      for (w=0; w<8;w++)
//      {
//          *BITMAP_ADDR16(bitmap, y, x+w) = (data_byte & 0x080) ? 1 : 0;
//
//          data_byte = data_byte<<1;
//
//      }
//  }
//  else
//  {
//      for (w=0; w<8;w++)
//          *BITMAP_ADDR16(bitmap, y, x+w) = 0;
//  }
//
//}
//
//static VIDEO_UPDATE( einstein_80col )
//{
//  long c=0; // this is used to time out the screen redraw, in the case that the 6845 is in some way out state.
//
//  c=0;
//
//  // loop until the end of the Vertical Sync pulse
//  while((Einstein_VSync)&&(c<33274))
//  {
//      // Clock the 6845
//      m6845_clock(screen->machine);
//      c++;
//  }
//
//  // loop until the Vertical Sync pulse goes high
//  // or until a timeout (this catches the 6845 with silly register values that would not give a VSYNC signal)
//  while((!Einstein_VSync)&&(c<33274))
//  {
//      while ((Einstein_HSync)&&(c<33274))
//      {
//          m6845_clock(screen->machine);
//          c++;
//      }
//      // Do all the clever split mode changes in here before the next while loop
//
//      while ((!Einstein_HSync)&&(c<33274))
//      {
//          // check that we are on the emulated screen area.
//          if ((Einstein_scr_x>=0) && (Einstein_scr_x<640) && (Einstein_scr_y>=0) && (Einstein_scr_y<400))
//          {
//              einstein_80col_plot_char_line(Einstein_scr_x, Einstein_scr_y, bitmap);
//          }
//
//          Einstein_scr_x+=8;
//
//          // Clock the 6845
//          m6845_clock(screen->machine);
//          c++;
//      }
//  }
//  return 0;
//}

static VIDEO_UPDATE( einstein2 )
{
	const device_config *mc6845 = devtag_get_device(screen->machine, "crtc");

	VIDEO_UPDATE_CALL(tms9928a);
	mc6845_update(mc6845, bitmap, cliprect);
//  VIDEO_UPDATE_CALL(einstein_80col);
	return 0;
}

static MACHINE_DRIVER_START( einstein )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, EINSTEIN_SYSTEM_CLOCK)
	MDRV_CPU_PROGRAM_MAP(einstein_mem)
	MDRV_CPU_IO_MAP(einstein_io)
	MDRV_CPU_CONFIG(einstein_daisy_chain)
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_START( einstein )
	MDRV_MACHINE_RESET( einstein )

	MDRV_Z80PIO_ADD( "z80pio", einstein_pio_intf )
	MDRV_Z80CTC_ADD( "z80ctc", EINSTEIN_SYSTEM_CLOCK, einstein_ctc_intf )
	MDRV_DEVICE_ADD( "keyboard_daisy", DEVICE_GET_INFO_NAME( einstein_daisy ), 0 )
	MDRV_DEVICE_ADD( "adc_daisy", DEVICE_GET_INFO_NAME( einstein_daisy ), 0 )

    /* video hardware */
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("ay8910", AY8910, 2000000)
	MDRV_SOUND_CONFIG(einstein_ay_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	/* printer */
	MDRV_CENTRONICS_ADD("centronics", einstein_centronics_config)

	/* uart */
	MDRV_MSM8251_ADD("uart", default_msm8251_interface)

	MDRV_WD177X_ADD("wd177x", default_wd17xx_interface )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( einstei2 )
	MDRV_IMPORT_FROM( einstein )

	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_IO_MAP(einstein2_io)
	MDRV_MACHINE_RESET( einstein2 )

    /* video hardware */
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MDRV_MC6845_ADD("crtc", MC6845, EINSTEIN_SYSTEM_CLOCK /*?*/, einstein_crtc6845_interface)

	MDRV_VIDEO_UPDATE( einstein2 )
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(einstein)
	ROM_REGION(0x010000+0x02000, "maincpu",0)
	ROM_LOAD("einstein.rom",0x10000, 0x02000, CRC(ec134953) SHA1(a02125d8ebcda48aa784adbb42a8b2d7ef3a4b77))
ROM_END

ROM_START(einstei2)
	ROM_REGION(0x010000+0x02000+0x0800, "maincpu",0)
	ROM_LOAD("einstein.rom",0x10000, 0x02000, CRC(ec134953) SHA1(a02125d8ebcda48aa784adbb42a8b2d7ef3a4b77))
	ROM_LOAD("charrom.rom",0x012000, 0x0800, NO_DUMP)
ROM_END

static void einstein_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 4; break;

		default:										legacydsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(einstein)
	CONFIG_RAM_DEFAULT(65536)
	CONFIG_DEVICE(einstein_floppy_getinfo)
SYSTEM_CONFIG_END

/*     YEAR  NAME       PARENT  COMPAT  MACHINE    INPUT     INIT  CONFIG,   COMPANY   FULLNAME */
COMP( 1984, einstein,	0,      0,		einstein,  einstein, 0,    einstein, "Tatung", "Tatung Einstein TC-01", 0)
COMP( 1984, einstei2,	0,      0,		einstei2,  einstein, 0,    einstein, "Tatung", "Tatung Einstein TC-01 + 80 column device", GAME_NOT_WORKING)
