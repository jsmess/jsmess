/***************************************************************************

	drivers/bw2.c

	Bondwell BW 2

	- Z80L CPU 4MHz
	- 64KB RAM, expandable to 224KB RAM.
	- 4KB System ROM
	- MSM6255 LCD controller, 640x200 pixels
	- 16KB Video RAM
	- TMS2797 FDC controller
	- 8251 USART serial interface
	- 8253 PIT timer
	- 8255 PPI

  http://www.thebattles.net/bondwell/

  http://www.vintage-computer.com/bondwell2.shtml

  http://www2.okisemi.com/site/productscatalog/displaydrivers/availabledocuments/Intro-7090.html


	TODO:

	- modem card

***************************************************************************/

#include "driver.h"
#include "includes/bw2.h"
#include "cpu/z80/z80.h"
#include "includes/serial.h"
#include "machine/8255ppi.h"
#include "machine/ctronics.h"
#include "machine/msm8251.h"
#include "machine/wd17xx.h"
#include "machine/pit8253.h"
#include "devices/basicdsk.h"
#include "video/msm6255.h"


static const device_config *get_floppy_image(running_machine *machine, int drive)
{
	return image_from_devtype_and_index(machine, IO_FLOPPY, drive);
}

static int get_ramdisk_size(running_machine *machine)
{
	return input_port_read(machine, "RAMCARD") * 256;
}

/* Memory */

static void bw2_set_banks(running_machine *machine, UINT8 data)
{
	/*

		Y0  /RAM1  	Memory bank 1
		Y1  /VRAM  	Video memory
		Y2  /RAM2  	Memory bank 2
		Y3  /RAM3  	Memory bank 3
		Y4  /RAM4  	Memory bank 4
		Y5  /RAM5  	Memory bank 5
		Y6  /RAM6  	Memory bank 6
		Y7  /ROM 	ROM

	*/

	bw2_state *state = machine->driver_data;

	int max_ram_bank = 0;

	state->bank = data & 0x07;

	switch (mess_ram_size)
	{
	case 64 * 1024:
		max_ram_bank = BANK_RAM1;
		break;

	case 96 * 1024:
		max_ram_bank = BANK_RAM2;
		break;

	case 128 * 1024:
		max_ram_bank = BANK_RAM3;
		break;

	case 160 * 1024:
		max_ram_bank = BANK_RAM4;
		break;

	case 192 * 1024:
		max_ram_bank = BANK_RAM5;
		break;

	case 224 * 1024:
		max_ram_bank = BANK_RAM6;
		break;
	}

	switch (state->bank)
	{
	case BANK_RAM1:
		memory_install_readwrite8_handler(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		break;

	case BANK_VRAM:
		memory_install_readwrite8_handler(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x3fff, 0, 0x4000, SMH_BANK(1), SMH_BANK(1));
		break;

	case BANK_RAM2:
	case BANK_RAM3:
	case BANK_RAM4:
	case BANK_RAM5:
	case BANK_RAM6:
		if (state->bank > max_ram_bank)
		{
			memory_install_readwrite8_handler(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		}
		else
		{
			memory_install_readwrite8_handler(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		}
		break;

	case BANK_ROM:
		memory_install_readwrite8_handler(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_UNMAP);
		break;
	}

	memory_set_bank(machine, 1, state->bank);
}

static void ramcard_set_banks(running_machine *machine, UINT8 data)
{
	/*

		Y0  /RAM1  	Memory bank 1
		Y1  /VRAM  	Video memory
		Y2  /RAM2  	RAMCARD ROM
		Y3  /RAM3  	Memory bank 3
		Y4  /RAM4  	Memory bank 4
		Y5  /RAM5  	RAMCARD RAM
		Y6  /RAM6  	Memory bank 6
		Y7  /ROM 	ROM

	*/

	bw2_state *state = machine->driver_data;

	int max_ram_bank = BANK_RAM1;

	state->bank = data & 0x07;

	switch (mess_ram_size)
	{
	case 64 * 1024:
	case 96 * 1024:
		max_ram_bank = BANK_RAM1;
		break;

	case 128 * 1024:
		max_ram_bank = BANK_RAM3;
		break;

	case 160 * 1024:
		max_ram_bank = BANK_RAM4;
		break;

	case 192 * 1024:
	case 224 * 1024:
		max_ram_bank = BANK_RAM6;
		break;
	}

	switch (state->bank)
	{
	case BANK_RAM1:
	case BANK_RAMCARD_RAM:
		memory_install_readwrite8_handler(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		break;

	case BANK_VRAM:
		memory_install_readwrite8_handler(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x3fff, 0, 0x4000, SMH_BANK(1), SMH_BANK(1));
		break;

	case BANK_RAM3:
	case BANK_RAM4:
	case BANK_RAM6:
		if (state->bank > max_ram_bank)
		{
			memory_install_readwrite8_handler(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		}
		else
		{
			memory_install_readwrite8_handler(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		}
		break;

	case BANK_RAMCARD_ROM:
		memory_install_readwrite8_handler(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x3fff, 0, 0x4000, SMH_BANK(1), SMH_UNMAP);
		break;

	case BANK_ROM:
		memory_install_readwrite8_handler(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_UNMAP);
		break;
	}

	memory_set_bank(machine, 1, state->bank);
}

static WRITE8_HANDLER( ramcard_bank_w )
{
	bw2_state *state = space->machine->driver_data;

	UINT8 ramcard_bank = data & 0x0f;
	UINT32 bank_offset = ramcard_bank * 0x8000;

	if ((get_ramdisk_size(space->machine) == 256) && (ramcard_bank > 7))
	{
		memory_install_readwrite8_handler(cputag_get_address_space(space->machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
	}
	else
	{
		memory_install_readwrite8_handler(cputag_get_address_space(space->machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
	}

	memory_configure_bank(space->machine, 1, BANK_RAMCARD_RAM, 1, state->ramcard_ram + bank_offset, 0);
	memory_set_bank(space->machine, 1, state->bank);
}

/* Serial */

static DEVICE_IMAGE_LOAD( bw2_serial )
{
	bw2_state *state = image->machine->driver_data;

	/* filename specified */
	if (device_load_serial_device(image) == INIT_PASS)
	{
		/* setup transmit parameters */
		serial_device_setup(image, 9600 >> input_port_read(image->machine, "BAUD"), 8, 1, SERIAL_PARITY_NONE);

		/* connect serial chip to serial device */
		msm8251_connect_to_serial_device(state->msm8251, image);

		serial_device_set_protocol(image, SERIAL_PROTOCOL_NONE);

		/* and start transmit */
		serial_device_set_transmit_state(image, 1);

		return INIT_PASS;
	}

	return INIT_FAIL;
}


/* Floppy */

static DEVICE_IMAGE_LOAD( bw2_floppy )
{
	if (image_has_been_created(image)) {
		return INIT_FAIL;
	}

	if (device_load_basicdsk_floppy(image) == INIT_PASS)
	{
		switch (image_length(image))
		{
		case 80*1*17*256: // 340K
			basicdsk_set_geometry(image, 80, 1, 17, 256, 0, 0, FALSE);
			return INIT_PASS;

		case 80*1*18*256: // 360K
			basicdsk_set_geometry(image, 80, 1, 18, 256, 0, 0, FALSE);
			return INIT_PASS;
		}
	}

	return INIT_FAIL;
}

static WD17XX_CALLBACK( bw2_wd17xx_callback )
{
	switch(state)
	{
		case WD17XX_IRQ_CLR:
			cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_IRQ0, CLEAR_LINE);
			break;

		case WD17XX_IRQ_SET:
			cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_IRQ0, HOLD_LINE);
			break;

		case WD17XX_DRQ_CLR:
			cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_NMI, CLEAR_LINE);
			break;

		case WD17XX_DRQ_SET:
			if (cpu_get_reg(cputag_get_cpu(device->machine, Z80_TAG), Z80_HALT))
			{
				cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_NMI, HOLD_LINE);
			}
			break;
	}
}

static READ8_HANDLER( bw2_wd2797_r )
{
	UINT8 result = 0xff;
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");

	switch (offset & 0x03)
	{
		case 0:
			result = wd17xx_status_r(fdc, 0);
			break;
		case 1:
			result = wd17xx_track_r(fdc, 0);
			break;
		case 2:
			result = wd17xx_sector_r(fdc, 0);
			break;
		case 3:
			result = wd17xx_data_r(fdc, 0);
			break;
	}

	return result;
}

static WRITE8_HANDLER( bw2_wd2797_w )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");
	switch (offset & 0x3)
	{
		case 0:
			/* disk side is encoded in bit 1 of Type II/III commands */

			if (BIT(data, 7))
			{
				if ((data & 0xf0) != 0xd0)
				{
					int side = BIT(data, 1);

					wd17xx_set_side(fdc,side);

					data &= ~0x02;
				}
			}

			wd17xx_command_w(fdc, 0, data);

			break;
		case 1:
			wd17xx_track_w(fdc, 0, data);
			break;
		case 2:
			wd17xx_sector_w(fdc, 0, data);
			break;
		case 3:
			wd17xx_data_w(fdc, 0, data);
			break;
	};
}


/* PPI */

static WRITE8_DEVICE_HANDLER( bw2_ppi8255_a_w )
{
	const device_config *fdc = devtag_get_device(device->machine, "wd179x");
	/*

		PA0     KB0 Keyboard line select 0
		PA1     KB1 Keyboard line select 1
		PA2     KB2 Keyboard line select 2
		PA3     KB3 Keyboard line select 3
		PA4     /DS0 Drive select 0
		PA5     /DS1 Drive select 1
		PA6     Select RS232 connector
		PA7     /STROBE to centronics printer

	*/

	bw2_state *state = device->machine->driver_data;

	state->keyboard_row = data & 0x0f;

	if (BIT(data, 4))
	{
		state->selected_drive = 0;
		wd17xx_set_drive(fdc,state->selected_drive);
	}

	if (BIT(data, 5))
	{
		state->selected_drive = 1;
		wd17xx_set_drive(fdc,state->selected_drive);
	}

	centronics_strobe_w(state->centronics, BIT(data, 7));
}

static READ8_DEVICE_HANDLER( bw2_ppi8255_b_r )
{
	/*

		PB0     Keyboard column status of selected line
		PB1     Keyboard column status of selected line
		PB2     Keyboard column status of selected line
		PB3     Keyboard column status of selected line
		PB4     Keyboard column status of selected line
		PB5     Keyboard column status of selected line
		PB6     Keyboard column status of selected line
		PB7     Keyboard column status of selected line

	*/

	bw2_state *state = device->machine->driver_data;

	UINT8 row;
	static const char *const rownames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7", "ROW8", "ROW9" };

	row = state->keyboard_row;

	if (row <= 9)
		return input_port_read(device->machine, rownames[row]);

	return 0xff;
}

static WRITE8_DEVICE_HANDLER( bw2_ppi8255_c_w )
{
	/*

		PC0     Memory bank select
		PC1     Memory bank select
		PC2     Memory bank select
		PC3     Not connected

	*/

	if (get_ramdisk_size(device->machine) > 0)
	{
		ramcard_set_banks(device->machine, data & 0x07);
	}
	else
	{
		bw2_set_banks(device->machine, data & 0x07);
	}
}

static READ8_DEVICE_HANDLER( bw2_ppi8255_c_r )
{
	/*

		PC4     BUSY from centronics printer
		PC5     M/FDBK motor feedback
		PC6     RLSD Carrier detect from RS232
		PC7     /PROT Write protected disk

	*/

	bw2_state *state = device->machine->driver_data;

	UINT8 data = 0;

	data |= centronics_busy_r(state->centronics) << 4;
	data |= state->mfdbk << 5;

	data |= floppy_drive_get_flag_state(get_floppy_image(device->machine, state->selected_drive), FLOPPY_DRIVE_DISK_WRITE_PROTECTED) ? 0x00 : 0x80;

	return data;
}

static const ppi8255_interface bw2_ppi8255_interface =
{
	DEVCB_NULL,
	DEVCB_HANDLER(bw2_ppi8255_b_r),
	DEVCB_HANDLER(bw2_ppi8255_c_r),
	DEVCB_HANDLER(bw2_ppi8255_a_w),
	DEVCB_NULL,
	DEVCB_HANDLER(bw2_ppi8255_c_w),
};

/* PIT */

static PIT8253_OUTPUT_CHANGED( bw2_timer0_w )
{
	bw2_state *driver_state = device->machine->driver_data;

	msm8251_transmit_clock(driver_state->msm8251);
	msm8251_receive_clock(driver_state->msm8251);
}

static PIT8253_OUTPUT_CHANGED( bw2_timer1_w )
{
	pit8253_set_clock_signal(device, 2, state);
}

static PIT8253_OUTPUT_CHANGED( bw2_timer2_w )
{
	bw2_state *driver_state = device->machine->driver_data;

	driver_state->mtron = state;
	driver_state->mfdbk = !state;

	floppy_drive_set_motor_state(get_floppy_image(device->machine, 0), !driver_state->mtron);
	floppy_drive_set_motor_state(get_floppy_image(device->machine, 1), !driver_state->mtron);

	floppy_drive_set_ready_state(get_floppy_image(device->machine, 0), 1, 1);
	floppy_drive_set_ready_state(get_floppy_image(device->machine, 1), 1, 1);
}

static const struct pit8253_config bw2_pit8253_interface =
{
	{
		{
			XTAL_4MHz,	/* 8251 USART TXC, RXC */
			bw2_timer0_w
		},
		{
			11000,		/* LCD controller */
			bw2_timer1_w
		},
		{
			0,		/* Floppy /MTRON */
			bw2_timer2_w
		}
	}
};


/* Video */

static PALETTE_INIT( bw2 )
{
    palette_set_color_rgb(machine, 0, 0xa5, 0xad, 0xa5);
    palette_set_color_rgb(machine, 1, 0x31, 0x39, 0x10);
}

static VIDEO_START( bw2 )
{
}

static VIDEO_UPDATE( bw2 )
{
	bw2_state *state = screen->machine->driver_data;

	msm6255_update(state->msm6255, bitmap, cliprect);

	return 0;
}

/* Machine */

static DRIVER_INIT( bw2 )
{
	bw2_state *state = machine->driver_data;

	/* allocate work memory */
	state->work_ram = auto_alloc_array(machine, UINT8, mess_ram_size);

	/* allocate video memory */
	state->video_ram = auto_alloc_array(machine, UINT8, BW2_VIDEORAM_SIZE);

	/* allocate RAMcard memory */
	state->ramcard_ram = auto_alloc_array(machine, UINT8, BW2_RAMCARD_SIZE);
}

static MACHINE_START( bw2 )
{
	bw2_state *state = machine->driver_data;
	const device_config *fdc = devtag_get_device(machine, "wd179x");

	wd17xx_set_density(fdc,DEN_MFM_LO);

	/* find devices */
	state->msm8251 = devtag_get_device(machine, MSM8251_TAG);
	state->msm6255 = devtag_get_device(machine, MSM6255_TAG);
	state->centronics = devtag_get_device(machine, CENTRONICS_TAG);

	/* register for state saving */
	state_save_register_global(machine, state->keyboard_row);
	state_save_register_global_pointer(machine, state->work_ram, mess_ram_size);
	state_save_register_global_pointer(machine, state->ramcard_ram, BW2_RAMCARD_SIZE);
	state_save_register_global(machine, state->bank);
	state_save_register_global(machine, state->selected_drive);
	state_save_register_global(machine, state->mtron);
	state_save_register_global(machine, state->mfdbk);
	state_save_register_global_pointer(machine, state->video_ram, BW2_VIDEORAM_SIZE);
}

static MACHINE_RESET( bw2 )
{
	bw2_state *state = machine->driver_data;

	if (get_ramdisk_size(machine) > 0)
	{
		// RAMCARD installed

		memory_configure_bank(machine, 1, BANK_RAM1, 1, state->work_ram, 0);
		memory_configure_bank(machine, 1, BANK_VRAM, 1, state->video_ram, 0);
		memory_configure_bank(machine, 1, BANK_RAMCARD_ROM, 1, memory_region(machine, "ramcard"), 0);
		memory_configure_bank(machine, 1, BANK_RAM3, 2, state->work_ram + 0x8000, 0x8000);
		memory_configure_bank(machine, 1, BANK_RAMCARD_RAM, 1, state->ramcard_ram, 0);
		memory_configure_bank(machine, 1, BANK_RAM6, 1, state->work_ram + 0x18000, 0);
		memory_configure_bank(machine, 1, BANK_ROM, 1, memory_region(machine, "ic1"), 0);

		memory_install_write8_handler(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_IO), 0x30, 0x30, 0, 0x0f, &ramcard_bank_w);
	}
	else
	{
		// no RAMCARD

		memory_configure_bank(machine, 1, BANK_RAM1, 1, state->work_ram, 0);
		memory_configure_bank(machine, 1, BANK_VRAM, 1, state->video_ram, 0);
		memory_configure_bank(machine, 1, BANK_RAM2, 5, state->work_ram + 0x8000, 0x8000);
		memory_configure_bank(machine, 1, BANK_ROM, 1, memory_region(machine, "ic1"), 0);

		memory_install_write8_handler(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_IO), 0x30, 0x30, 0, 0x0f, SMH_UNMAP);
	}

	memory_set_bank(machine, 1, BANK_ROM);
}

static ADDRESS_MAP_START( bw2_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK(1)
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( bw2_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( 0x00, 0x03 ) AM_DEVREADWRITE(PPI8255_TAG, ppi8255_r, ppi8255_w )
	AM_RANGE( 0x10, 0x13 ) AM_DEVREADWRITE(PIT8253_TAG, pit8253_r, pit8253_w )
	AM_RANGE( 0x20, 0x21 ) AM_DEVREADWRITE(MSM6255_TAG, msm6255_register_r, msm6255_register_w )
//	AM_RANGE( 0x30, 0x3f ) SLOT
	AM_RANGE( 0x40, 0x40 ) AM_DEVREADWRITE( MSM8251_TAG, msm8251_data_r, msm8251_data_w )
	AM_RANGE( 0x41, 0x41 ) AM_DEVREADWRITE( MSM8251_TAG, msm8251_status_r, msm8251_control_w )
	AM_RANGE( 0x50, 0x50 ) AM_DEVWRITE(CENTRONICS_TAG, centronics_data_w)
	AM_RANGE( 0x60, 0x63 ) AM_READWRITE( bw2_wd2797_r, bw2_wd2797_w )
//	AM_RANGE( 0x70, 0x7f ) MODEMSEL
ADDRESS_MAP_END

/*
  Keyboard matrix
        X0    X1    X2    X3    X4    X5    X6    X7
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y9 |CAPS |     |     |     |     |     |     |     |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y8 |SHIFT|     |SPACE|  Z  |  A  |  Q  | 2 " | F1  |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y7 |CTRL | - = | \ | |  X  |  S  |  W  | 3 # | F2  |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y6 | @ ` |  P  | UP  |  C  |  D  |  E  | 4 $ | F3  |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y5 | 1 ! |     |     |  V  |  F  |  R  | 5 % | F4  |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y4 | ESC |     |     |  B  |  G  |  T  | 6 & | F5  |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y3 | TAB | : * |ENTER|  N  |  H  |  Y  | 7 ' | F6  |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y2 |DOWN | ^ ~ | [ { |  M  |  J  |  U  | 8 ( | F7  |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y1 |RIGHT| ; + | ] } | , < |  K  |  I  | 9 ) | F8  |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y0 |LEFT | BS  | / ? | . > |  L  |  O  | 0 _ | DEL |
     +-----+-----+-----+-----+-----+-----+-----+-----+
*/

static INPUT_PORTS_START( bw2 )
	PORT_START("ROW0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('_')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL))

	PORT_START("ROW1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8))

	PORT_START("ROW2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F7))

	PORT_START("ROW3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB) PORT_CHAR(UCHAR_MAMEKEY(TAB))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6))

	PORT_START("ROW4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))

	PORT_START("ROW5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))

	PORT_START("ROW6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('@') PORT_CHAR('`')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))

	PORT_START("ROW7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))

	PORT_START("ROW8")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))

	PORT_START("ROW9")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("BAUD")
	PORT_CONFNAME( 0x05, 0x05, "Baud rate")
	PORT_CONFSETTING( 0x00, "9600 baud" )
	PORT_CONFSETTING( 0x01, "4800 baud" )
	PORT_CONFSETTING( 0x02, "2400 baud" )
	PORT_CONFSETTING( 0x03, "1200 baud" )
	PORT_CONFSETTING( 0x04, "600 baud" )
	PORT_CONFSETTING( 0x05, "300 baud" )

	PORT_START("RAMCARD")
	PORT_CONFNAME( 0x03, 0x00, "RAMCARD")
	PORT_CONFSETTING( 0x00, DEF_STR( None ) )
	PORT_CONFSETTING( 0x01, "256 KB" )
	PORT_CONFSETTING( 0x02, "512 KB" )
INPUT_PORTS_END

static MSM6255_CHAR_RAM_READ( bw2_charram_r )
{
	bw2_state *state = device->machine->driver_data;

	return state->video_ram[ma & 0x3fff];
}

static MSM6255_INTERFACE( bw2_msm6255_intf )
{
	SCREEN_TAG,
	0,
	bw2_charram_r,
};

static const wd17xx_interface bw2_wd17xx_interface = { bw2_wd17xx_callback, NULL };

static MACHINE_DRIVER_START( bw2 )
	MDRV_DRIVER_DATA(bw2_state)

	/* basic machine hardware */
	MDRV_CPU_ADD( Z80_TAG, Z80, XTAL_16MHz/4 )
	MDRV_CPU_PROGRAM_MAP( bw2_mem)
	MDRV_CPU_IO_MAP( bw2_io)

	MDRV_MACHINE_START( bw2 )
	MDRV_MACHINE_RESET( bw2 )

	MDRV_PIT8253_ADD( PIT8253_TAG, bw2_pit8253_interface )
	MDRV_PPI8255_ADD( PPI8255_TAG, bw2_ppi8255_interface )

	/* video hardware */
	MDRV_SCREEN_ADD( SCREEN_TAG, LCD )
	MDRV_SCREEN_REFRESH_RATE( 60 )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_SIZE( 640, 200 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 640-1, 0, 200-1 )
	MDRV_DEFAULT_LAYOUT( layout_lcd )

	MDRV_PALETTE_LENGTH( 2 )
	MDRV_PALETTE_INIT( bw2 )
	MDRV_VIDEO_START( bw2 )
	MDRV_VIDEO_UPDATE( bw2 )

	MDRV_MSM6255_ADD(MSM6255_TAG, XTAL_16MHz, bw2_msm6255_intf)

	/* printer */
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)

	/* uart */
	MDRV_MSM8251_ADD(MSM8251_TAG, default_msm8251_interface)

	MDRV_WD179X_ADD("wd179x", default_wd17xx_interface )
MACHINE_DRIVER_END

/***************************************************************************

  System driver(s)

***************************************************************************/

ROM_START( bw2 )
	ROM_REGION(0x10000, "ic1", 0)
	ROM_SYSTEM_BIOS(0, "20", "BW 2 v2.0")
	ROMX_LOAD("bw2-20.ic8", 0x0000, 0x1000, CRC(86f36471) SHA1(a3e2ba4edd50ff8424bb0675bdbb3b9f13c04c9d), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "12", "BW 2 v1.2")
	ROMX_LOAD("bw2-12.ic8", 0x0000, 0x1000, CRC(0ab42d10) SHA1(430b232631eee9b715151b8d191b7eb9449ac513), ROM_BIOS(2))

	ROM_REGION(0x4000, "ramcard", 0)
	ROM_LOAD("ramcard-10.bin", 0x0000, 0x4000, CRC(68cde1ba) SHA1(a776a27d64f7b857565594beb63aa2cd692dcf04))
ROM_END

static void bw2_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:
			info->i = 2;
			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:
			info->load = DEVICE_IMAGE_LOAD_NAME(bw2_floppy);
			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:
			strcpy(info->s = device_temp_str(), "dsk");
			break;

		default:
			legacybasicdsk_device_getinfo(devclass, state, info);
			break;
	}
}

static void bw2_serial_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* serial */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:
			info->i = IO_SERIAL;
			break;
		case MESS_DEVINFO_INT_READABLE:
			info->i = 1;
			break;
		case MESS_DEVINFO_INT_WRITEABLE:
			info->i = 0;
			break;
		case MESS_DEVINFO_INT_CREATABLE:
			info->i = 0;
			break;
		case MESS_DEVINFO_INT_COUNT:
			info->i = 1;
			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:
			info->start = DEVICE_START_NAME(serial_device);
			break;
		case MESS_DEVINFO_PTR_LOAD:
			info->load = DEVICE_IMAGE_LOAD_NAME(bw2_serial);
			break;
		case MESS_DEVINFO_PTR_UNLOAD:
			info->unload = DEVICE_IMAGE_UNLOAD_NAME(serial_device);
			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:
			strcpy(info->s = device_temp_str(), "txt");
			break;
	}
}

static SYSTEM_CONFIG_START( bw2 )
	CONFIG_DEVICE( bw2_floppy_getinfo )
	CONFIG_DEVICE( bw2_serial_getinfo )
	CONFIG_RAM_DEFAULT( 64 * 1024 )
	CONFIG_RAM( 96 * 1024 )
	CONFIG_RAM( 128 * 1024 )
	CONFIG_RAM( 160 * 1024 )
	CONFIG_RAM( 192 * 1024 )
	CONFIG_RAM( 224 * 1024 )
SYSTEM_CONFIG_END

/*    YEAR  NAME    PARENT  COMPAT  MACHINE   INPUT   INIT    CONFIG  COMPANY      FULLNAME  FLAGS */
COMP( 1985, bw2,    0,      0,      bw2,      bw2,    bw2,    bw2,    "Bondwell",  "BW 2",   0 )
