
#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "cpu/z80/z80daisy.h"
#include "machine/ctronics.h"
#include "machine/kay_kbd.h"
#include "machine/wd17xx.h"
#include "devices/basicdsk.h"
#include "includes/kaypro.h"


static const device_config *kaypro2_z80pio_g;
static const device_config *kaypro2_z80pio_s;
static const device_config *kaypro2_z80sio;
static const device_config *kaypro2_printer;
static const device_config *kaypro2_fdc;



/***********************************************************

	PIO

	It seems there must have been a bulk-purchase
	special on pios - port B is unused on both of them

************************************************************/

static WRITE8_DEVICE_HANDLER( kaypro2_interrupt )
{
	cpu_set_input_line(device->machine->cpu[0], 0, data);
}

static READ8_DEVICE_HANDLER( pio_system_r )
{
/*	d3 Centronics ready flag */

	return ~centronics_busy_r(kaypro2_printer) << 3;
}

static WRITE8_DEVICE_HANDLER( pio_system_w )
{
/*	d7 bank select
	d6 disk drive motors - not emulated
	d5 double-density enable
	d4 Centronics strobe
	d1 drive B
	d0 drive A */

	/* get address space */
	const address_space *mem = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	if (data & 0x80)
	{
		memory_install_readwrite8_handler (mem, 0x0000, 0x3fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		memory_install_read8_handler (mem, 0x0000, 0x0fff, 0, 0, SMH_BANK1);
		memory_set_bankptr(mem->machine, 1, memory_region(mem->machine, "maincpu"));
		memory_install_readwrite8_handler (mem, 0x3000, 0x3fff, 0, 0, kaypro_videoram_r, kaypro_videoram_w);
	}
	else
	{
		memory_install_readwrite8_handler (mem, 0x0000, 0x3fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
 		memory_install_readwrite8_handler (mem, 0x0000, 0x3fff, 0, 0, SMH_BANK2, SMH_BANK3);
		memory_set_bankptr(mem->machine, 2, memory_region(mem->machine, "rambank"));
		memory_set_bankptr(mem->machine, 3, memory_region(mem->machine, "rambank"));
	}

	wd17xx_set_density(kaypro2_fdc, (data & 0x20) ? 1 : 0);

	centronics_strobe_w(kaypro2_printer, BIT(data, 4));

	if (data & 1)
		wd17xx_set_drive(kaypro2_fdc, 0);
	else
	if (data & 2)
		wd17xx_set_drive(kaypro2_fdc, 1);

	if (data & 3)
		wd17xx_set_side(kaypro2_fdc, 0);	/* only has 1 side */
}

const z80pio_interface kaypro2_pio_g_intf =
{
	DEVCB_HANDLER(kaypro2_interrupt),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_HANDLER("centronics", centronics_data_w),
	DEVCB_NULL,
	DEVCB_NULL,			/* portA ready active callback */
	DEVCB_NULL			/* portB ready active callback */
};

const z80pio_interface kaypro2_pio_s_intf =
{
	DEVCB_HANDLER(kaypro2_interrupt),
	DEVCB_HANDLER(pio_system_r),	/* read printer status */
	DEVCB_NULL,
	DEVCB_HANDLER(pio_system_w),	/* activate various internal devices */
	DEVCB_NULL,
	DEVCB_NULL,			/* portA ready active callback */
	DEVCB_NULL			/* portB ready active callback */
};

READ8_DEVICE_HANDLER( kaypro2_pio_r )
{
	if (!offset)
		return z80pio_d_r(device, 0);
	else
	if (offset == 1)
		return z80pio_c_r(device, 0);
	else
	if (offset == 2)
		return z80pio_d_r(device, 1);
	else
		return z80pio_c_r(device, 1);
}

WRITE8_DEVICE_HANDLER( kaypro2_pio_w )
{
	if (!offset)
		z80pio_d_w(device, 0, data);
	else
	if (offset == 1)
		z80pio_c_w(device, 0, data);
	else
	if (offset == 2)
		z80pio_d_w(device, 1, data);
	else
		z80pio_c_w(device, 1, data);
}


/***********************************************************

	SIO
	Baud rate setup commented out until MAME includes
	the feature.

************************************************************/

/* Set baud rate. bits 0..3 Rx and Tx are tied together. Baud Rate Generator is a AY-5-8116, SMC8116, etc.
	00h    50  
	11h    75  
	22h    110  
	33h    134.5  
	44h    150  
	55h    300  
	66h    600  
	77h    1200  
	88h    1800  
	99h    2000  
	AAh    2400  
	BBh    3600  
	CCh    4800  
	DDh    7200  
	EEh    9600  
	FFh    19200 */

static const int baud_clock[]={ 800, 1200, 1760, 2152, 2400, 4800, 9600, 19200, 28800, 32000, 38400, 57600, 76800, 115200, 153600, 307200 };

WRITE8_HANDLER( kaypro2_baud_a_w )	/* channel A - RS232C */
{
	data &= 0x0f;

//	z80sio_set_rx_clock( kaypro2_z80sio, baud_clock[data], 0);
//	z80sio_set_tx_clock( kaypro2_z80sio, baud_clock[data], 0);
}

WRITE8_HANDLER( kaypro2_baud_b_w )	/* Channel B - Keyboard */
{
/* Only speed used is 300 baud */

	data &= 0x0f;

//	z80sio_set_rx_clock( kaypro2_z80sio, baud_clock[data], 1);
//	z80sio_set_tx_clock( kaypro2_z80sio, baud_clock[data], 1);
}

/* when sio devcb'ed like pio is, change to use pio's int handler */
static void kaypro2_int_sio(const device_config *device, int state)
{
	cpu_set_input_line(device->machine->cpu[0], 0, state);
}

const z80sio_interface kaypro2_sio_intf =
{
	kaypro2_int_sio,	/* interrupt handler */
	0,			/* DTR changed handler */
	0,			/* RTS changed handler */
	0,			/* BREAK changed handler */
	0,			/* transmit handler - which channel is this for? */
	0			/* receive handler - which channel is this for? */
};

static const z80_daisy_chain kaypro2_daisy_chain[] =
{
/* Which order are these supposed to be in? assumed highest priority at the top */
	{ "z80sio" },		/* sio */
	{ "z80pio_s" },		/* System pio */
	{ "z80pio_g" },		/* General purpose pio */
	{ NULL }
};

READ8_DEVICE_HANDLER( kaypro2_sio_r )
{
	if (!offset)
		return z80sio_d_r(device, 0);
	else
	if (offset == 1)
//		return z80sio_d_r(device, 1);
		return kay_kbd_d_r();
	else
	if (offset == 2)
		return z80sio_c_r(device, 0);
	else
//		return z80sio_c_r(device, 1);
		return kay_kbd_c_r();
}

WRITE8_DEVICE_HANDLER( kaypro2_sio_w )
{
	if (!offset)
		z80sio_d_w(device, 0, data);
	else
	if (offset == 1)
//		z80sio_d_w(device, 1, data);
		kay_kbd_d_w(device->machine, data);
	else
	if (offset == 2)
		z80sio_c_w(device, 0, data);
	else
		z80sio_c_w(device, 1, data);
}


/***********************************************************

	Floppy DIsk

	If there is no diskette in the drive, the cpu will
	HALT, until the fdc does a NMI. NMI is deactivated
	otherwise.

************************************************************/

static WD17XX_CALLBACK( kaypro2_fdc_callback )
{
	switch (state)
	{
		case WD17XX_DRQ_CLR:
		case WD17XX_IRQ_CLR:
			cpu_set_input_line(device->machine->cpu[0], INPUT_LINE_NMI, 0);
			break;
		case WD17XX_DRQ_SET:
		case WD17XX_IRQ_SET:
			if (cpu_get_reg(device->machine->cpu[0], Z80_HALT))
				cpu_set_input_line(device->machine->cpu[0], INPUT_LINE_NMI, PULSE_LINE);
			break;
	}
}

const wd17xx_interface kaypro2_wd1793_interface = { kaypro2_fdc_callback, NULL };

/* I have no idea how to set up floppies - these ones have 40 tracks, 40 sectors, 128 bytes per track, single sided */

void kaypro2_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:			info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
//		case MESS_DEVINFO_PTR_LOAD:			info->load = DEVICE_IMAGE_LOAD_NAME(kaypro2_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "dsk"); break;

		default:					legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}


/***********************************************************

	Machine

************************************************************/

MACHINE_RESET( kayproii )
{
	kaypro2_z80pio_g = devtag_get_device(machine, "z80pio_g");
	kaypro2_z80pio_s = devtag_get_device(machine, "z80pio_s");
	kaypro2_z80sio = devtag_get_device(machine, "z80sio");
	kaypro2_printer = devtag_get_device(machine, "centronics");
	kaypro2_fdc = devtag_get_device(machine, "wd1793");
	pio_system_w(kaypro2_z80pio_s, 0, 0x80);
	MACHINE_RESET_CALL(kay_kbd);
}

