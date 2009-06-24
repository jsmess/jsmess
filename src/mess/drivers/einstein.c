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
#include "einstei2.lh"


/***************************************************************************
    CONSTANTS
***************************************************************************/

/* xtals */
#define XTAL_X001  XTAL_10_738635MHz
#define XTAL_X002  XTAL_8MHz

/* integrated circuits */
#define IC_I001  "i001"  /* Z8400A */
#define IC_I030  "i030"  /* AY-3-8910 */
#define IC_I038  "i038"  /* TMM9129 */
#define IC_I042  "i042"  /* WD1770-PH */
#define IC_I050  "i050"  /* ADC0844CCN */
#define IC_I058  "i058"  /* Z8430A */
#define IC_I060  "i060"  /* uPD8251A */
#define IC_I063  "i063"  /* Z8420A */

/* interrupt sources */
#define EINSTEIN_KEY_INT   (1<<0)
#define EINSTEIN_ADC_INT   (1<<1)
#define EINSTEIN_FIRE_INT  (1<<2)


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _einstein_state einstein_state;
struct _einstein_state
{
	const device_config *color_screen;
	const device_config *ctc;

	int rom_enabled;
	int interrupt;
	int interrupt_mask;
	int ctc_trigger;

	/* keyboard */
	UINT8 keyboard_line;
	UINT8 keyboard_data;

	/* 80 column device */
	const device_config *mc6845;
	const device_config *crtc_screen;
	UINT8 *crtc_ram;
};


/***************************************************************************
    80 COLUMN DEVICE
***************************************************************************/

/* lower 3 bits of address define a 256-byte "row".
 * upper 8 bits define the offset in the row,
 * data bits define data to write
 */
static WRITE8_HANDLER( einstein_80col_ram_w )
{
	einstein_state *einstein = space->machine->driver_data;
	einstein->crtc_ram[((offset & 0x07) << 8) | ((offset >> 8) & 0xff)] = data;
}

static READ8_HANDLER( einstein_80col_ram_r )
{
	einstein_state *einstein = space->machine->driver_data;
	return einstein->crtc_ram[((offset & 0x07) << 8) | ((offset >> 8) & 0xff)];
}

/* TODO: Verify implementation */
static MC6845_UPDATE_ROW( einstein_6845_update_row )
{
	einstein_state *einstein = device->machine->driver_data;
	UINT8 *data = memory_region(device->machine, "gfx1");
	UINT8 char_code, data_byte;
	int i, x;

	for (i = 0, x = 0; i < x_count; i++, x += 8)
	{
		char_code = einstein->crtc_ram[(ma + i) & 0x07ff];
		data_byte = data[(char_code << 3) + ra];

		*BITMAP_ADDR16(bitmap, y, x + 0) = TMS9928A_PALETTE_SIZE + BIT(data_byte, 7);
		*BITMAP_ADDR16(bitmap, y, x + 1) = TMS9928A_PALETTE_SIZE + BIT(data_byte, 6);
		*BITMAP_ADDR16(bitmap, y, x + 2) = TMS9928A_PALETTE_SIZE + BIT(data_byte, 5);
		*BITMAP_ADDR16(bitmap, y, x + 3) = TMS9928A_PALETTE_SIZE + BIT(data_byte, 4);
		*BITMAP_ADDR16(bitmap, y, x + 4) = TMS9928A_PALETTE_SIZE + BIT(data_byte, 3);
		*BITMAP_ADDR16(bitmap, y, x + 5) = TMS9928A_PALETTE_SIZE + BIT(data_byte, 2);
		*BITMAP_ADDR16(bitmap, y, x + 6) = TMS9928A_PALETTE_SIZE + BIT(data_byte, 1);
		*BITMAP_ADDR16(bitmap, y, x + 7) = TMS9928A_PALETTE_SIZE + BIT(data_byte, 0);
	}
}

/* bit 0 - vsync state?
 * bit 1 - 0 = 40 column mode (color monitor), 1 = 80 column mode (b/w monitor)
 * bit 2 - 0 = 50Hz, 1 = 60Hz
 */
static READ8_HANDLER( einstein_80col_state_r )
{
	einstein_state *einstein = space->machine->driver_data;
	UINT8 result = 0;

	result |= video_screen_get_vblank(einstein->crtc_screen);
	result |= input_port_read(space->machine, "80column_dips") & 0x06;

	logerror("%s: einstein_80col_state_r %02x\n", cpuexec_describe_context(space->machine), result);

	return result;
}


/****************************************************************
	EINSTEIN NON-Z80 DEVICES DAISY CHAIN SUPPORT
****************************************************************/

static DEVICE_START( einstein_daisy ) { }

static int einstein_keyboard_daisy_irq_state(const device_config *device)
{
	einstein_state *einstein = device->machine->driver_data;

	if (einstein->interrupt & einstein->interrupt_mask & EINSTEIN_KEY_INT)
		return Z80_DAISY_INT;

	return 0;
}

static int einstein_keyboard_daisy_irq_ack(const device_config *device)
{
	return 0xf7;
}

static DEVICE_GET_INFO( einstein_keyboard_daisy )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = 4;											break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;											break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;						break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(einstein_daisy);		break;
		case DEVINFO_FCT_IRQ_STATE:						info->f = (genf *)einstein_keyboard_daisy_irq_state;	break;
		case DEVINFO_FCT_IRQ_ACK:						info->f = (genf *)einstein_keyboard_daisy_irq_ack;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Einstein keyboard daisy chain");		break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Einstein daisy chain");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");									break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);								break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");				break;
	}
}

static int einstein_adc_daisy_irq_state(const device_config *device)
{
	einstein_state *einstein = device->machine->driver_data;

	if (einstein->interrupt & einstein->interrupt_mask & EINSTEIN_ADC_INT)
		return Z80_DAISY_INT;

	return 0;
}

static int einstein_adc_daisy_irq_ack(const device_config *device)
{
	return 0xfb;
}

static DEVICE_GET_INFO( einstein_adc_daisy )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = 4;											break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;											break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;						break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(einstein_daisy);		break;
		case DEVINFO_FCT_IRQ_STATE:						info->f = (genf *)einstein_adc_daisy_irq_state;	break;
		case DEVINFO_FCT_IRQ_ACK:						info->f = (genf *)einstein_adc_daisy_irq_ack;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Einstein ADC daisy chain");			break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Einstein daisy chain");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");									break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);								break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");				break;
	}
}

static int einstein_fire_daisy_irq_state(const device_config *device)
{
	einstein_state *einstein = device->machine->driver_data;

	if (einstein->interrupt & einstein->interrupt_mask & EINSTEIN_FIRE_INT)
		return Z80_DAISY_INT;

	return 0;
}

static int einstein_fire_daisy_irq_ack(const device_config *device)
{
	return 0xfd;
}

static DEVICE_GET_INFO( einstein_fire_daisy )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = 4;											break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;											break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;						break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(einstein_daisy);		break;
		case DEVINFO_FCT_IRQ_STATE:						info->f = (genf *)einstein_fire_daisy_irq_state;		break;
		case DEVINFO_FCT_IRQ_ACK:						info->f = (genf *)einstein_fire_daisy_irq_ack;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Einstein fire button daisy chain");		break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Einstein daisy chain");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");									break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);								break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");				break;
	}
}

/* int priority */
/* keyboard int->ctc/adc->pio */
static const z80_daisy_chain einstein_daisy_chain[] =
{
	{ "keyboard_daisy" },
	{ IC_I058 },
	{ "adc_daisy" },
	{ IC_I063 },
	{ "fire_daisy" },
	{ NULL }
};


/***************************************************************************
    KEYBOARD
***************************************************************************/

/* refresh keyboard data. It is refreshed when the keyboard line is written */
static void einstein_scan_keyboard(running_machine *machine)
{
	einstein_state *einstein = machine->driver_data;
	UINT8 data = 0xff;

	if (!BIT(einstein->keyboard_line, 0)) data &= input_port_read(machine, "LINE0");
	if (!BIT(einstein->keyboard_line, 1)) data &= input_port_read(machine, "LINE1");
	if (!BIT(einstein->keyboard_line, 2)) data &= input_port_read(machine, "LINE2");
	if (!BIT(einstein->keyboard_line, 3)) data &= input_port_read(machine, "LINE3");
	if (!BIT(einstein->keyboard_line, 4)) data &= input_port_read(machine, "LINE4");
	if (!BIT(einstein->keyboard_line, 5)) data &= input_port_read(machine, "LINE5");
	if (!BIT(einstein->keyboard_line, 6)) data &= input_port_read(machine, "LINE6");
	if (!BIT(einstein->keyboard_line, 7)) data &= input_port_read(machine, "LINE7");

	einstein->keyboard_data = data;
}

static TIMER_DEVICE_CALLBACK( einstein_keyboard_timer_callback )
{
	einstein_state *einstein = timer->machine->driver_data;

	/* re-scan keyboard */
	einstein_scan_keyboard(timer->machine);

	/* if /fire1 or /fire2 is 0, signal a fire interrupt */
	if ((input_port_read(timer->machine, "BUTTONS") & 0x03) != 0)
	{
		einstein->interrupt |= EINSTEIN_FIRE_INT;
	}

	/* keyboard data changed? */
	if (einstein->keyboard_data != 0xff)
	{
		/* generate interrupt */
		einstein->interrupt |= EINSTEIN_KEY_INT;
	}
}

static WRITE8_HANDLER( einstein_keyboard_line_write )
{
	einstein_state *einstein = space->machine->driver_data;

	logerror("einstein_keyboard_line_write: %02x\n", data);

	einstein->keyboard_line = data;

	/* re-scan the keyboard */
	einstein_scan_keyboard(space->machine);
}

static READ8_HANDLER( einstein_keyboard_data_read )
{
	einstein_state *einstein = space->machine->driver_data;

	/* re-scan the keyboard */
	einstein_scan_keyboard(space->machine);

	logerror("einstein_keyboard_data_read: %02x\n", einstein->keyboard_data);

	return einstein->keyboard_data;
}


/***************************************************************************
    FLOPPY DRIVES
***************************************************************************/

static WRITE8_DEVICE_HANDLER( einstein_drsel_w )
{
	logerror("%s: einstein_drsel_w %02x\n", cpuexec_describe_context(device->machine), data);

	/* bit 0 to 3 select the drive */
	if (BIT(data, 0)) wd17xx_set_drive(device, 0);
	if (BIT(data, 1)) wd17xx_set_drive(device, 1);
	if (BIT(data, 2)) wd17xx_set_drive(device, 2);
	if (BIT(data, 3)) wd17xx_set_drive(device, 3);

	/* bit 4 selects the side */
	wd17xx_set_side(device, BIT(data, 4));
}


/***************************************************************************
    CTC
***************************************************************************/

/* channel 0 and 1 have a 2 MHz input clock for triggering */
static TIMER_DEVICE_CALLBACK( einstein_ctc_trigger_callback )
{
	einstein_state *einstein = timer->machine->driver_data;

	/* toggle line status */
	einstein->ctc_trigger ^= 1;

	z80ctc_trg0_w(einstein->ctc, 0, einstein->ctc_trigger);
	z80ctc_trg1_w(einstein->ctc, 0, einstein->ctc_trigger);
}


/***************************************************************************
    UART
***************************************************************************/

static WRITE8_DEVICE_HANDLER( einstein_serial_transmit_clock )
{
	const device_config *uart = devtag_get_device(device->machine, IC_I060);
	msm8251_transmit_clock(uart);
}

static WRITE8_DEVICE_HANDLER( einstein_serial_receive_clock )
{
	const device_config *uart = devtag_get_device(device->machine, IC_I060);
	msm8251_receive_clock(uart);
}


/***************************************************************************
    MEMORY BANKING
***************************************************************************/

static void einstein_page_rom(running_machine *machine)
{
	einstein_state *einstein = machine->driver_data;
	memory_set_bankptr(machine, 1, einstein->rom_enabled ? memory_region(machine, "bios") : mess_ram);
}

/* writing to this port is a simple trigger, and switches between RAM and ROM */
static WRITE8_HANDLER( einstein_rom_w )
{
	einstein_state *einstein = space->machine->driver_data;
	einstein->rom_enabled ^= 1;
	einstein_page_rom(space->machine);
}


/***************************************************************************
    INTERRUPTS
***************************************************************************/

static READ8_HANDLER( einstein_kybintmsk_r )
{
	const device_config *printer = devtag_get_device(space->machine, "centronics");
	einstein_state *einstein = space->machine->driver_data;
	UINT8 data = 0;

	/* clear key int. a read of this I/O port will do this or a reset */
	einstein->interrupt &= ~EINSTEIN_KEY_INT;

	/* bit 0 and 1: fire buttons on the joysticks */
	data |= input_port_read(space->machine, "BUTTONS");

	/* bit 2 to 4: printer status */
	data |= centronics_busy_r(printer) << 2;
	data |= centronics_pe_r(printer) << 3;
	data |= centronics_fault_r(printer) << 4;

	/* bit 5 to 7: graph, control and shift key */
	data |= input_port_read(space->machine, "EXTRA");

	logerror("%s: einstein_kybintmsk_r %02x\n", cpuexec_describe_context(space->machine), data);

	return data;
}

static WRITE8_HANDLER( einstein_kybintmsk_w )
{
	einstein_state *einstein = space->machine->driver_data;

	logerror("%s: einstein_kybintmsk_w %02x\n", cpuexec_describe_context(space->machine), data);

	/* set mask from bit 0 */
	if (data & 0x01)
	{
		logerror("key int is disabled\n");
		einstein->interrupt_mask &= ~EINSTEIN_KEY_INT;
	}
	else
	{
		logerror("key int is enabled\n");
		einstein->interrupt_mask |= EINSTEIN_KEY_INT;
	}
}

/* writing to this I/O port sets the state of the mask; D0 is used */
/* writing 0 enables the /ADC interrupt */
static WRITE8_HANDLER( einstein_adcintmsk_w )
{
	einstein_state *einstein = space->machine->driver_data;

	logerror("%s: einstein_adcintmsk_w %02x\n", cpuexec_describe_context(space->machine), data);

	if (data & 0x01)
	{
		logerror("adc int is disabled\n");
		einstein->interrupt_mask &= ~EINSTEIN_ADC_INT;
	}
	else
	{
		logerror("adc int is enabled\n");
		einstein->interrupt_mask |= EINSTEIN_ADC_INT;
	}
}

/* writing to this I/O port sets the state of the mask; D0 is used */
/* writing 0 enables the /FIRE interrupt */
static WRITE8_HANDLER( einstein_fire_int_w )
{
	einstein_state *einstein = space->machine->driver_data;

	logerror("%s: einstein_fire_int_w %02x\n", cpuexec_describe_context(space->machine), data);

	if (data & 0x01)
	{
		logerror("fire int is disabled\n");
		einstein->interrupt_mask &= ~EINSTEIN_FIRE_INT;
	}
	else
	{
		logerror("fire int is enabled\n");
		einstein->interrupt_mask |= EINSTEIN_FIRE_INT;
	}
}


/***************************************************************************
    MACHINE EMULATION
***************************************************************************/

static const TMS9928a_interface tms9928a_interface =
{
	TMS9929A,
	0x4000, /* 16k RAM, provided by IC i040 and i041 */
	0, 0,
	NULL
};

static MACHINE_START( einstein )
{
	TMS9928A_configure(&tms9928a_interface);
}

static MACHINE_RESET( einstein )
{
	einstein_state *einstein = machine->driver_data;

	/* save pointers to our devices */
	einstein->color_screen = devtag_get_device(machine, "screen");
	einstein->ctc = devtag_get_device(machine, IC_I058);

	/* initialize memory mapping */
	memory_set_bankptr(machine, 2, mess_ram);
	memory_set_bankptr(machine, 3, mess_ram + 0x8000);
	einstein->rom_enabled = 1;
	einstein_page_rom(machine);

	TMS9928A_reset();

	/* a reset causes the fire int, adc int, keyboard int mask
    to be set to 1, which causes all these to be DISABLED */
	einstein->interrupt = 0;
	einstein->interrupt_mask = 0;

	floppy_drive_set_geometry(image_from_devtype_and_index(machine, IO_FLOPPY, 0), FLOPPY_DRIVE_SS_40);

	einstein->ctc_trigger = 0;
}

static MACHINE_RESET( einstein2 )
{
	einstein_state *einstein = machine->driver_data;

	/* call standard initialization first */
	MACHINE_RESET_CALL(einstein);

	/* get 80 column specific devices */
	einstein->mc6845 = devtag_get_device(machine, "crtc");
	einstein->crtc_screen = devtag_get_device(machine, "80column");

	/* 80 column card palette */
	palette_set_color(machine, TMS9928A_PALETTE_SIZE, RGB_BLACK);
	palette_set_color(machine, TMS9928A_PALETTE_SIZE + 1, MAKE_RGB(0, 224, 0));

	einstein->crtc_ram = auto_alloc_array(machine, UINT8, 2048);
}


/***************************************************************************
    VIDEO EMULATION
***************************************************************************/

static VIDEO_UPDATE( einstein2 )
{
	einstein_state *einstein = screen->machine->driver_data;

	if (screen == einstein->color_screen)
		VIDEO_UPDATE_CALL(tms9928a);
	else if (screen == einstein->crtc_screen)
		mc6845_update(einstein->mc6845, bitmap, cliprect);
	else
		fatalerror("Unknown screen '%s'\n", screen->tag);

	return 0;
}


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START( einstein_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07fff) AM_READWRITE(SMH_BANK(1), SMH_BANK(2))
	AM_RANGE(0x8000, 0x0ffff) AM_RAMBANK(3)
ADDRESS_MAP_END

/* The I/O ports are decoded into 8 blocks using address lines A3 to A7 */
static ADDRESS_MAP_START( einstein_io, ADDRESS_SPACE_IO, 8 )
	/* block 0, ay8910 psg */
	AM_RANGE(0x02, 0x02) AM_MIRROR(0xff04) AM_DEVREADWRITE(IC_I030, ay8910_r, ay8910_address_w)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0xff04) AM_DEVWRITE(IC_I030, ay8910_data_w)
	/* block 1, tms9928a vdp */
	AM_RANGE(0x08, 0x08) AM_MIRROR(0xff06) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0x09, 0x09) AM_MIRROR(0xff06) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
	/* block 2, msm8251 uart */
	AM_RANGE(0x10, 0x10) AM_MIRROR(0xff06) AM_DEVREADWRITE(IC_I060, msm8251_data_r, msm8251_data_w)
	AM_RANGE(0x11, 0x11) AM_MIRROR(0xff06) AM_DEVREADWRITE(IC_I060, msm8251_status_r, msm8251_control_w)
	/* block 3, wd1770 floppy controller */
	AM_RANGE(0x18, 0x1b) AM_MIRROR(0xff04) AM_DEVREADWRITE(IC_I042, wd17xx_r, wd17xx_w)
	/* block 4, internal controls */
	AM_RANGE(0x20, 0x20) AM_MIRROR(0xff00) AM_READWRITE(einstein_kybintmsk_r, einstein_kybintmsk_w)
	AM_RANGE(0x21, 0x21) AM_MIRROR(0xff00) AM_WRITE(einstein_adcintmsk_w)
	AM_RANGE(0x23, 0x23) AM_MIRROR(0xff00) AM_DEVWRITE(IC_I042, einstein_drsel_w)
	AM_RANGE(0x24, 0x24) AM_MIRROR(0xff00) AM_WRITE(einstein_rom_w)
	AM_RANGE(0x25, 0x25) AM_MIRROR(0xff00) AM_WRITE(einstein_fire_int_w)
	/* block 5, z80ctc */
	AM_RANGE(0x28, 0x2b) AM_MIRROR(0xff04) AM_DEVREADWRITE(IC_I058, z80ctc_r, z80ctc_w)
	/* block 6, z80pio */
	AM_RANGE(0x30, 0x33) AM_MIRROR(0xff04) AM_DEVREADWRITE(IC_I063, z80pio_r, z80pio_w)
#if 0
	/* block 7, adc */
	AM_RANGE(0x38, 0x38) AM_MIRROR(0xff07) AM_DEVREADWRITE(IC_I050, adc0844_r, adc0844_w)
#endif
ADDRESS_MAP_END

static ADDRESS_MAP_START( einstein2_io, ADDRESS_SPACE_IO, 8 )
	AM_IMPORT_FROM(einstein_io)
	AM_RANGE(0x40, 0x47) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READWRITE(einstein_80col_ram_r, einstein_80col_ram_w)
	AM_RANGE(0x48, 0x48) AM_MIRROR(0xff00) AM_DEVWRITE("crtc", mc6845_address_w)
	AM_RANGE(0x49, 0x49) AM_MIRROR(0xff00) AM_DEVWRITE("crtc", mc6845_register_w)
	AM_RANGE(0x4c, 0x4c) AM_MIRROR(0xff00) AM_READ(einstein_80col_state_r)
ADDRESS_MAP_END


/***************************************************************************
    INPUT PORTS
***************************************************************************/

static INPUT_PORTS_START( einstein )
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
	PORT_BIT(0x1f, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GRPH")    PORT_CODE(KEYCODE_F8)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CONTROL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT")   PORT_CODE(KEYCODE_LSHIFT)   PORT_CODE(KEYCODE_RSHIFT)   PORT_CHAR(UCHAR_SHIFT_1)

	/* fire buttons for analogue joysticks */
	PORT_START("BUTTONS")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("Joystick 1 Button 1") PORT_PLAYER(1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("Joystick 2 Button 1") PORT_PLAYER(2)
	PORT_BIT(0xfc, IP_ACTIVE_HIGH, IPT_UNUSED)

	/* analog joystick 1 x axis */
	PORT_START("JOY1_X")
	PORT_BIT(0xff, 0x80, IPT_AD_STICK_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(1) PORT_REVERSE

	/* analog joystick 1 y axis */
	PORT_START("JOY1_Y")
	PORT_BIT(0xff, 0x80, IPT_AD_STICK_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(1) PORT_REVERSE

	/* analog joystick 2 x axis */
	PORT_START("JOY2_X")
	PORT_BIT(0xff, 0x80, IPT_AD_STICK_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(2) PORT_REVERSE

	/* analog joystick 2 Y axis */
	PORT_START("JOY2_Y")
	PORT_BIT(0xff, 0x80, IPT_AD_STICK_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(2) PORT_REVERSE
INPUT_PORTS_END

INPUT_PORTS_START( einstein_80col )
	PORT_INCLUDE(einstein)

	/* dip switches on the 80 column card */
	PORT_START("80column_dips")
	PORT_DIPNAME(0x02, 0x00, "Startup mode")
	PORT_DIPSETTING(0x00, "40 column")
	PORT_DIPSETTING(0x02, "80 column")
	PORT_DIPNAME(0x04, 0x00, "50/60Hz")
	PORT_DIPSETTING(0x00, "50Hz")
	PORT_DIPSETTING(0x04, "60Hz")
INPUT_PORTS_END


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static const z80ctc_interface einstein_ctc_intf =
{
	0,
	0,
	einstein_serial_transmit_clock,
	einstein_serial_receive_clock,
	z80ctc_trg3_w
};


static const z80pio_interface einstein_pio_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_HANDLER("centronics", centronics_data_w),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE("centronics", centronics_strobe_w),
	DEVCB_NULL
};

static const ay8910_interface einstein_ay_interface =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_NULL,
	DEVCB_MEMORY_HANDLER(IC_I001, PROGRAM, einstein_keyboard_data_read),
	DEVCB_MEMORY_HANDLER(IC_I001, PROGRAM, einstein_keyboard_line_write),
	DEVCB_NULL
};

static const centronics_interface einstein_centronics_config =
{
	FALSE,
	DEVCB_DEVICE_LINE(IC_I063, z80pio_astb_w),
	DEVCB_NULL,
	DEVCB_NULL
};

static const mc6845_interface einstein_crtc6845_interface =
{
	"80column",
	8,
	NULL,
	einstein_6845_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};


static MACHINE_DRIVER_START( einstein )
	/* basic machine hardware */
	MDRV_CPU_ADD(IC_I001, Z80, XTAL_X002 / 2)
	MDRV_CPU_PROGRAM_MAP(einstein_mem)
	MDRV_CPU_IO_MAP(einstein_io)
	MDRV_CPU_CONFIG(einstein_daisy_chain)

	MDRV_DRIVER_DATA(einstein_state)

	MDRV_MACHINE_START(einstein)
	MDRV_MACHINE_RESET(einstein)

	/* this is actually clocked at the system clock 4 MHz, but this would be too fast for our
	driver. So we update at 50Hz and hope this is good enough. */
	MDRV_TIMER_ADD_PERIODIC("keyboard", einstein_keyboard_timer_callback, HZ(50))

	MDRV_Z80PIO_ADD(IC_I063, einstein_pio_intf)

	MDRV_Z80CTC_ADD(IC_I058, XTAL_X002 / 2, einstein_ctc_intf)
	/* the input to channel 0 and 1 of the ctc is a 2 MHz clock */
	MDRV_TIMER_ADD_PERIODIC("ctc", einstein_ctc_trigger_callback, HZ(XTAL_X002 /4))

	/* Einstein daisy chain support for non-Z80 devices */
	MDRV_DEVICE_ADD("keyboard_daisy", DEVICE_GET_INFO_NAME(einstein_keyboard_daisy), 0)
	MDRV_DEVICE_ADD("adc_daisy", DEVICE_GET_INFO_NAME(einstein_adc_daisy), 0)
	MDRV_DEVICE_ADD("fire_daisy", DEVICE_GET_INFO_NAME(einstein_fire_daisy), 0)

    /* video hardware */
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(IC_I030, AY8910, XTAL_X002 / 4)
	MDRV_SOUND_CONFIG(einstein_ay_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	/* printer */
	MDRV_CENTRONICS_ADD("centronics", einstein_centronics_config)

	/* uart */
	MDRV_MSM8251_ADD(IC_I060, default_msm8251_interface)

	MDRV_WD1770_ADD(IC_I042, default_wd17xx_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( einstei2 )
	MDRV_IMPORT_FROM( einstein )

	MDRV_CPU_MODIFY(IC_I001)
	MDRV_CPU_IO_MAP(einstein2_io)
	MDRV_MACHINE_RESET(einstein2)

    /* video hardware */
	MDRV_DEFAULT_LAYOUT(layout_einstei2)

	MDRV_SCREEN_ADD("80column", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 400-1)
	MDRV_SCREEN_REFRESH_RATE(50)

	/* 2 additional colors for the 80 column screen */
	MDRV_PALETTE_LENGTH(TMS9928A_PALETTE_SIZE + 2)

	MDRV_MC6845_ADD("crtc", MC6845, XTAL_X002 / 4, einstein_crtc6845_interface)

	MDRV_VIDEO_UPDATE(einstein2)
MACHINE_DRIVER_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

/* There are two sockets, i023 and i024, each either a 2764 or 27128
 * only i023 is used by default and fitted with the 8k bios (called MOS).
 *
 * We are missing dumps of version MOS 1.1, possibly of 1.0 if it exists.
 */
ROM_START( einstein )
	ROM_REGION(0x8000, "bios", 0)
	/* i023 */
	ROM_SYSTEM_BIOS(0,  "mos12",  "MOS 1.2")
	ROMX_LOAD("mos12.i023", 0, 0x2000, CRC(ec134953) SHA1(a02125d8ebcda48aa784adbb42a8b2d7ef3a4b77), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1,  "mos121",  "MOS 1.21")
	ROMX_LOAD("mos121.i023", 0, 0x2000, CRC(a746eeb6) SHA1(f75aaaa777d0fd92225acba291f6bf428b341d3e), ROM_BIOS(2))
	ROM_RELOAD(0x2000, 0x2000)
	/* i024 */
	ROM_FILL(0x4000, 0x4000, 0xff)
ROM_END

ROM_START( einstei2 )
	ROM_REGION(0x8000, "bios", 0)
	/* i023 */
	ROM_SYSTEM_BIOS(0,  "mos12",  "MOS 1.2")
	ROMX_LOAD("mos12.i023", 0, 0x2000, CRC(ec134953) SHA1(a02125d8ebcda48aa784adbb42a8b2d7ef3a4b77), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1,  "mos121",  "MOS 1.21")
	ROMX_LOAD("mos121.i023", 0, 0x2000, CRC(a746eeb6) SHA1(f75aaaa777d0fd92225acba291f6bf428b341d3e), ROM_BIOS(2))
	ROM_RELOAD(0x2000, 0x2000)
	/* i024 */
	ROM_FILL(0x4000, 0x4000, 0xff)

	/* character rom from 80 column card */
	ROM_REGION(0x800, "gfx1", 0)
	ROM_LOAD("charrom.rom", 0, 0x800, NO_DUMP)
ROM_END


/***************************************************************************
    SYSTEM CONFIG
***************************************************************************/

static void einstein_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:	info->i = 4; break;

		default:						legacydsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START( einstein )
	/* RAM is provided by 8k DRAM ICs i009, i010, i011, i012, i013, i014, i015 and i016 */
	CONFIG_RAM_DEFAULT(8 * 8 * 1024)
	CONFIG_DEVICE(einstein_floppy_getinfo)
SYSTEM_CONFIG_END


/***************************************************************************
    GAME DRIVERS
***************************************************************************/

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT           INIT  CONFIG,   COMPANY   FULLNAME                             FLAGS */
COMP( 1984, einstein, 0,        0,		einstein, einstein,       0,    einstein, "Tatung", "Einstein TC-01",                    0 )
COMP( 1984, einstei2, einstein, 0,		einstei2, einstein_80col, 0,    einstein, "Tatung", "Einstein TC-01 + 80 column device", 0 )
