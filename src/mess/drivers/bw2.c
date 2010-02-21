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

#include "emu.h"
#include "includes/bw2.h"
#include "cpu/z80/z80.h"
#include "machine/serial.h"
#include "machine/i8255a.h"
#include "machine/ctronics.h"
#include "machine/msm8251.h"
#include "machine/wd17xx.h"
#include "machine/pit8253.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "video/msm6255.h"
#include "devices/messram.h"

static running_device *get_floppy_image(running_machine *machine, int drive)
{
	return floppy_get_device(machine, drive);
}

static int get_ramdisk_size(running_machine *machine)
{
	return input_port_read(machine, "RAMCARD") * 256;
}

/* Memory */

static void bw2_set_banks(running_machine *machine, UINT8 data)
{
	/*

        Y0  /RAM1   Memory bank 1
        Y1  /VRAM   Video memory
        Y2  /RAM2   Memory bank 2
        Y3  /RAM3   Memory bank 3
        Y4  /RAM4   Memory bank 4
        Y5  /RAM5   Memory bank 5
        Y6  /RAM6   Memory bank 6
        Y7  /ROM    ROM

    */

	bw2_state *state = (bw2_state *) machine->driver_data;

	int max_ram_bank = 0;

	state->bank = data & 0x07;

	switch (messram_get_size(devtag_get_device(machine, "messram")))
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
		memory_install_readwrite_bank(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, "bank1");
		break;

	case BANK_VRAM:
		memory_install_readwrite_bank(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x3fff, 0, 0x4000, "bank1");
		break;

	case BANK_RAM2:
	case BANK_RAM3:
	case BANK_RAM4:
	case BANK_RAM5:
	case BANK_RAM6:
		if (state->bank > max_ram_bank)
		{
			memory_unmap_readwrite(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0);
		}
		else
		{
			memory_install_readwrite_bank(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, "bank1");
		}
		break;

	case BANK_ROM:
		memory_install_read_bank(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, "bank1");
		memory_unmap_write(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0);
		break;
	}

	memory_set_bank(machine, "bank1", state->bank);
}

static void ramcard_set_banks(running_machine *machine, UINT8 data)
{
	/*

        Y0  /RAM1   Memory bank 1
        Y1  /VRAM   Video memory
        Y2  /RAM2   RAMCARD ROM
        Y3  /RAM3   Memory bank 3
        Y4  /RAM4   Memory bank 4
        Y5  /RAM5   RAMCARD RAM
        Y6  /RAM6   Memory bank 6
        Y7  /ROM    ROM

    */

	bw2_state *state = (bw2_state *) machine->driver_data;

	int max_ram_bank = BANK_RAM1;

	state->bank = data & 0x07;

	switch (messram_get_size(devtag_get_device(machine, "messram")))
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
		memory_install_readwrite_bank(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, "bank1");
		break;

	case BANK_VRAM:
		memory_install_readwrite_bank(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x3fff, 0, 0x4000, "bank1");
		break;

	case BANK_RAM3:
	case BANK_RAM4:
	case BANK_RAM6:
		if (state->bank > max_ram_bank)
		{
			memory_unmap_readwrite(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0);
		}
		else
		{
			memory_install_readwrite_bank(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, "bank1");
		}
		break;

	case BANK_RAMCARD_ROM:
		memory_install_read_bank(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x3fff, 0, 0x4000, "bank1");
		memory_unmap_write(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x3fff, 0, 0x4000);
		break;

	case BANK_ROM:
		memory_install_read_bank(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, "bank1");
		memory_unmap_write(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0);
		break;
	}

	memory_set_bank(machine, "bank1", state->bank);
}

static WRITE8_HANDLER( ramcard_bank_w )
{
	bw2_state *state = (bw2_state *) space->machine->driver_data;

	UINT8 ramcard_bank = data & 0x0f;
	UINT32 bank_offset = ramcard_bank * 0x8000;

	if ((get_ramdisk_size(space->machine) == 256) && (ramcard_bank > 7))
	{
		memory_unmap_readwrite(cputag_get_address_space(space->machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0);
	}
	else
	{
		memory_install_readwrite_bank(cputag_get_address_space(space->machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, "bank1");
	}

	memory_configure_bank(space->machine, "bank1", BANK_RAMCARD_RAM, 1, state->ramcard_ram + bank_offset, 0);
	memory_set_bank(space->machine, "bank1", state->bank);
}

/* Floppy */
static WRITE_LINE_DEVICE_HANDLER( bw2_wd17xx_drq_w )
{
	if (state)
	{
		if (cpu_get_reg(devtag_get_device(device->machine, Z80_TAG), Z80_HALT))
			cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_NMI, HOLD_LINE);
	}
	else
	{
		cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_NMI, CLEAR_LINE);
	}
}

static READ8_HANDLER( bw2_wd2797_r )
{
	UINT8 result = 0xff;
	running_device *fdc = devtag_get_device(space->machine, "wd179x");

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
	running_device *fdc = devtag_get_device(space->machine, "wd179x");
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

static WRITE8_DEVICE_HANDLER( bw2_8255_a_w )
{
	running_device *fdc = devtag_get_device(device->machine, "wd179x");
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

	bw2_state *state = (bw2_state *) device->machine->driver_data;

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

static READ8_DEVICE_HANDLER( bw2_8255_b_r )
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

	bw2_state *state = (bw2_state *) device->machine->driver_data;

	UINT8 row;
	static const char *const rownames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7", "ROW8", "ROW9" };

	row = state->keyboard_row;

	if (row <= 9)
		return input_port_read(device->machine, rownames[row]);

	return 0xff;
}

static WRITE8_DEVICE_HANDLER( bw2_8255_c_w )
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

static READ8_DEVICE_HANDLER( bw2_8255_c_r )
{
	/*

        PC4     BUSY from centronics printer
        PC5     M/FDBK motor feedback
        PC6     RLSD Carrier detect from RS232
        PC7     /PROT Write protected disk

    */

	bw2_state *state = (bw2_state *) device->machine->driver_data;

	UINT8 data = 0;

	data |= centronics_busy_r(state->centronics) << 4;
	data |= state->mfdbk << 5;

	data |= floppy_wpt_r(get_floppy_image(device->machine, state->selected_drive)) << 7;

	return data;
}

static I8255A_INTERFACE( bw2_8255_interface )
{
	DEVCB_NULL,
	DEVCB_HANDLER(bw2_8255_b_r),
	DEVCB_HANDLER(bw2_8255_c_r),
	DEVCB_HANDLER(bw2_8255_a_w),
	DEVCB_NULL,
	DEVCB_HANDLER(bw2_8255_c_w),
};

/* PIT */

static PIT8253_OUTPUT_CHANGED( bw2_timer0_w )
{
	bw2_state *driver_state = (bw2_state *)device->machine->driver_data;

	msm8251_transmit_clock(driver_state->msm8251);
	msm8251_receive_clock(driver_state->msm8251);
}

static PIT8253_OUTPUT_CHANGED( bw2_timer1_w )
{
	pit8253_set_clock_signal(device, 2, state);
}

static PIT8253_OUTPUT_CHANGED( bw2_timer2_w )
{
	bw2_state *driver_state = (bw2_state *)device->machine->driver_data;

	driver_state->mtron = state;
	driver_state->mfdbk = !state;

	floppy_mon_w(get_floppy_image(device->machine, 0), driver_state->mtron);
	floppy_mon_w(get_floppy_image(device->machine, 1), driver_state->mtron);

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
	bw2_state *state = (bw2_state *) screen->machine->driver_data;

	msm6255_update(state->msm6255, bitmap, cliprect);

	return 0;
}

/* Machine */

static DRIVER_INIT( bw2 )
{
	bw2_state *state = (bw2_state *) machine->driver_data;

	/* allocate work memory */
	state->work_ram = auto_alloc_array(machine, UINT8, messram_get_size(devtag_get_device(machine, "messram")));

	/* allocate video memory */
	state->video_ram = auto_alloc_array(machine, UINT8, BW2_VIDEORAM_SIZE);

	/* allocate RAMcard memory */
	state->ramcard_ram = auto_alloc_array(machine, UINT8, BW2_RAMCARD_SIZE);
}

static MACHINE_START( bw2 )
{
	bw2_state *state = (bw2_state *) machine->driver_data;

	/* find devices */
	state->msm8251 = devtag_get_device(machine, MSM8251_TAG);
	state->msm6255 = devtag_get_device(machine, MSM6255_TAG);
	state->centronics = devtag_get_device(machine, CENTRONICS_TAG);

	/* memory banking */
	memory_configure_bank(machine, "bank1", BANK_RAM1, 1, state->work_ram, 0);
	memory_configure_bank(machine, "bank1", BANK_VRAM, 1, state->video_ram, 0);
	memory_configure_bank(machine, "bank1", BANK_ROM, 1, memory_region(machine, "ic1"), 0);
	
	/* register for state saving */
	state_save_register_global(machine, state->keyboard_row);
	state_save_register_global_pointer(machine, state->work_ram, messram_get_size(devtag_get_device(machine, "messram")));
	state_save_register_global_pointer(machine, state->ramcard_ram, BW2_RAMCARD_SIZE);
	state_save_register_global(machine, state->bank);
	state_save_register_global(machine, state->selected_drive);
	state_save_register_global(machine, state->mtron);
	state_save_register_global(machine, state->mfdbk);
	state_save_register_global_pointer(machine, state->video_ram, BW2_VIDEORAM_SIZE);
}

static MACHINE_RESET( bw2 )
{
	bw2_state *state = (bw2_state *) machine->driver_data;

	if (get_ramdisk_size(machine) > 0)
	{
		// RAMCARD installed

		memory_configure_bank(machine, "bank1", BANK_RAMCARD_ROM, 1, memory_region(machine, "ramcard"), 0);
		memory_configure_bank(machine, "bank1", BANK_RAM3, 2, state->work_ram + 0x8000, 0x8000);
		memory_configure_bank(machine, "bank1", BANK_RAMCARD_RAM, 1, state->ramcard_ram, 0);
		memory_configure_bank(machine, "bank1", BANK_RAM6, 1, state->work_ram + 0x18000, 0);

		memory_install_write8_handler(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_IO), 0x30, 0x30, 0, 0x0f, &ramcard_bank_w);
	}
	else
	{
		// no RAMCARD

		memory_configure_bank(machine, "bank1", BANK_RAM2, 5, state->work_ram + 0x8000, 0x8000);

		memory_unmap_write(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_IO), 0x30, 0x30, 0, 0x0f);
	}

	memory_set_bank(machine, "bank1", BANK_ROM);
}

static ADDRESS_MAP_START( bw2_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK("bank1")
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( bw2_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( 0x00, 0x03 ) AM_DEVREADWRITE(I8255A_TAG, i8255a_r, i8255a_w )
	AM_RANGE( 0x10, 0x13 ) AM_DEVREADWRITE(PIT8253_TAG, pit8253_r, pit8253_w )
	AM_RANGE( 0x20, 0x21 ) AM_DEVREADWRITE(MSM6255_TAG, msm6255_register_r, msm6255_register_w )
//  AM_RANGE( 0x30, 0x3f ) SLOT
	AM_RANGE( 0x40, 0x40 ) AM_DEVREADWRITE( MSM8251_TAG, msm8251_data_r, msm8251_data_w )
	AM_RANGE( 0x41, 0x41 ) AM_DEVREADWRITE( MSM8251_TAG, msm8251_status_r, msm8251_control_w )
	AM_RANGE( 0x50, 0x50 ) AM_DEVWRITE(CENTRONICS_TAG, centronics_data_w)
	AM_RANGE( 0x60, 0x63 ) AM_READWRITE( bw2_wd2797_r, bw2_wd2797_w )
//  AM_RANGE( 0x70, 0x7f ) MODEMSEL
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
	bw2_state *state = (bw2_state *) device->machine->driver_data;

	return state->video_ram[ma & 0x3fff];
}

static MSM6255_INTERFACE( bw2_msm6255_intf )
{
	SCREEN_TAG,
	0,
	bw2_charram_r,
};

static FLOPPY_OPTIONS_START(bw2)
	FLOPPY_OPTION(bw2, "dsk", "BW2 340K disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([80])
		SECTORS([17])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
	FLOPPY_OPTION(bw2, "dsk", "BW2 360K disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([80])
		SECTORS([18])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
FLOPPY_OPTIONS_END

static const floppy_config bw2_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(bw2),
	DO_NOT_KEEP_GEOMETRY
};

static const wd17xx_interface bw2_wd17xx_interface =
{
	DEVCB_LINE_GND,
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),
	DEVCB_LINE(bw2_wd17xx_drq_w),
	{FLOPPY_0, FLOPPY_1, NULL, NULL}
};

/* Serial */

static DEVICE_IMAGE_LOAD( bw2_serial )
{
	bw2_state *state = (bw2_state *) image->machine->driver_data;

	if (device_load_serial(image) == INIT_PASS)
	{
		serial_device_setup(image, 9600 >> input_port_read(image->machine, "BAUD"), 8, 1, SERIAL_PARITY_NONE);

		msm8251_connect_to_serial_device(state->msm8251, image);

		serial_device_set_transmit_state(image, 1);

		return INIT_PASS;
	}

	return INIT_FAIL;
}


static DEVICE_GET_INFO( bw2_serial )
{
	switch ( state )
	{
		case DEVINFO_FCT_IMAGE_LOAD:		        info->f = (genf *) DEVICE_IMAGE_LOAD_NAME( bw2_serial );    break;
		case DEVINFO_STR_NAME:		                strcpy(info->s, "BW2 serial port");	                    break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:	    strcpy(info->s, "txt");                                         break;
		case DEVINFO_INT_IMAGE_READABLE:            info->i = 1;                                        	break;
		case DEVINFO_INT_IMAGE_WRITEABLE:			info->i = 0;                                        	break;
		case DEVINFO_INT_IMAGE_CREATABLE:	    	info->i = 0;                                        	break;
		default:									DEVICE_GET_INFO_CALL(serial);	break;
	}
}

#define BW2_SERIAL	DEVICE_GET_INFO_NAME(bw2_serial)

#define MDRV_BW2_SERIAL_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, BW2_SERIAL, 0)


static MACHINE_DRIVER_START( bw2 )
	MDRV_DRIVER_DATA(bw2_state)

	/* basic machine hardware */
	MDRV_CPU_ADD( Z80_TAG, Z80, XTAL_16MHz/4 )
	MDRV_CPU_PROGRAM_MAP( bw2_mem)
	MDRV_CPU_IO_MAP( bw2_io)

	MDRV_MACHINE_START( bw2 )
	MDRV_MACHINE_RESET( bw2 )

	MDRV_PIT8253_ADD( PIT8253_TAG, bw2_pit8253_interface )
	MDRV_I8255A_ADD( I8255A_TAG, bw2_8255_interface )

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

	MDRV_WD179X_ADD("wd179x", bw2_wd17xx_interface )

	MDRV_FLOPPY_2_DRIVES_ADD(bw2_floppy_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
	MDRV_RAM_EXTRA_OPTIONS("96K,128K,160K,192K,224K")

	MDRV_BW2_SERIAL_ADD("serial")
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

/*    YEAR  NAME    PARENT  COMPAT  MACHINE   INPUT   INIT    COMPANY      FULLNAME  FLAGS */
COMP( 1985, bw2,    0,      0,      bw2,      bw2,    bw2,    "Bondwell",  "BW 2",   GAME_NO_SOUND )
