/****************************************************************************************************

        XOR S-100-12

*****************************************************************************************************/

/*

    TODO:

    - floppy images are bad
    - honor jumper settings
    - CTC signal header
    - serial printer
    - cassette?

*/

#include "emu.h"
#include "includes/xor100.h"
#include "cpu/z80/z80.h"
#include "formats/basicdsk.h"
#include "devices/flopdrv.h"
#include "machine/com8116.h"
#include "machine/ctronics.h"
#include "machine/i8255a.h"
#include "machine/msm8251.h"
#include "machine/terminal.h"
#include "machine/wd17xx.h"
#include "machine/z80ctc.h"
#include "devices/messram.h"

/* Read/Write Handlers */

enum
{
	EPROM_0000 = 0,
	EPROM_F800,
	EPROM_OFF
};

static void xor100_bankswitch(running_machine *machine)
{
	xor100_state *state = (xor100_state *)machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);
	running_device *messram = devtag_get_device(machine, "messram");
	int banks = messram_get_size(messram) / 0x10000;

	switch (state->mode)
	{
	case EPROM_0000:
		if (state->bank < banks)
		{
			memory_install_write_bank(program, 0x0000, 0xffff, 0, 0, "bank1");
			memory_set_bank(machine, "bank1", 1 + state->bank);
		}
		else
		{
			memory_unmap_write(program, 0x0000, 0xffff, 0, 0);
		}

		memory_install_read_bank(program, 0x0000, 0xf7ff, 0x07ff, 0, "bank2");
		memory_install_read_bank(program, 0xf800, 0xffff, 0, 0, "bank3");
		memory_set_bank(machine, "bank2", 0);
		memory_set_bank(machine, "bank3", 0);
		break;

	case EPROM_F800:
		if (state->bank < banks)
		{
			memory_install_write_bank(program, 0x0000, 0xffff, 0, 0, "bank1");
			memory_install_read_bank(program, 0x0000, 0xf7ff, 0, 0, "bank2");
			memory_set_bank(machine, "bank1", 1 + state->bank);
			memory_set_bank(machine, "bank2", 1 + state->bank);
		}
		else
		{
			memory_unmap_write(program, 0x0000, 0xffff, 0, 0);
			memory_unmap_read(program, 0x0000, 0xf7ff, 0, 0);
		}

		memory_install_read_bank(program, 0xf800, 0xffff, 0, 0, "bank3");
		memory_set_bank(machine, "bank3", 0);
		break;

	case EPROM_OFF:
		if (state->bank < banks)
		{
			memory_install_write_bank(program, 0x0000, 0xffff, 0, 0, "bank1");
			memory_install_read_bank(program, 0x0000, 0xf7ff, 0, 0, "bank2");
			memory_install_read_bank(program, 0xf800, 0xffff, 0, 0, "bank3");
			memory_set_bank(machine, "bank1", 1 + state->bank);
			memory_set_bank(machine, "bank2", 1 + state->bank);
			memory_set_bank(machine, "bank3", 1 + state->bank);
		}
		else
		{
			memory_unmap_write(program, 0x0000, 0xffff, 0, 0);
			memory_unmap_read(program, 0x0000, 0xf7ff, 0, 0);
			memory_unmap_read(program, 0xf800, 0xffff, 0, 0);
		}
		break;
	}
}

static WRITE8_HANDLER( mmu_w )
{
	/*

        bit     description

        0       A16
        1       A17
        2       A18
        3       A19
        4
        5
        6
        7

    */

	xor100_state *state = (xor100_state *)space->machine->driver_data;

	state->bank = data & 0x07;

	xor100_bankswitch(space->machine);
}

static WRITE8_HANDLER( prom_toggle_w )
{
	xor100_state *state = (xor100_state *)space->machine->driver_data;

	switch (state->mode)
	{
	case EPROM_OFF: state->mode = EPROM_F800; break;
	case EPROM_F800: state->mode = EPROM_OFF; break;
	}

	xor100_bankswitch(space->machine);
}

static READ8_HANDLER( prom_disable_r )
{
	xor100_state *state = (xor100_state *)space->machine->driver_data;

	state->mode = EPROM_F800;

	xor100_bankswitch(space->machine);

	return 0xff;
}

static WRITE8_DEVICE_HANDLER( baud_w )
{
	com8116_str_w(device, 0, data & 0x0f);
	com8116_stt_w(device, 0, data >> 4);
}

static WRITE8_DEVICE_HANDLER( i8251_b_data_w )
{
	running_device *terminal = devtag_get_device(device->machine, TERMINAL_TAG);

	msm8251_data_w(device, 0, data);
	terminal_write(terminal, 0, data);
}

static READ8_HANDLER( fdc_wait_r )
{
	/*

        bit     description

        0
        1
        2
        3
        4
        5
        6
        7       FDC IRQ

    */

	xor100_state *state = (xor100_state *)space->machine->driver_data;

	if (!state->fdc_irq && !state->fdc_drq)
	{
		/* TODO: this is really connected to the Z80 _RDY line */
		cputag_set_input_line(space->machine, Z80_TAG, INPUT_LINE_HALT, ASSERT_LINE);
	}

	return !state->fdc_irq << 7;
}

static WRITE8_HANDLER( fdc_dcont_w )
{
	/*

        bit     description

        0       DS0
        1       DS1
        2       DS2
        3       DS3
        4
        5
        6
        7       _HLSTB

    */

	xor100_state *state = (xor100_state *)space->machine->driver_data;

	/* drive select */
	if (BIT(data, 0)) wd17xx_set_drive(state->wd1795, 0);
	if (BIT(data, 1)) wd17xx_set_drive(state->wd1795, 1);

	floppy_mon_w(floppy_get_device(space->machine, 0), CLEAR_LINE);
	floppy_mon_w(floppy_get_device(space->machine, 1), CLEAR_LINE);
	floppy_drive_set_ready_state(floppy_get_device(space->machine, 0), 1, 1);
	floppy_drive_set_ready_state(floppy_get_device(space->machine, 1), 1, 1);
}

static WRITE8_HANDLER( fdc_dsel_w )
{
	/*

        bit     description

        0       J
        1       K
        2
        3
        4
        5
        6
        7

    */

	xor100_state *state = (xor100_state *)space->machine->driver_data;

	switch (data & 0x03)
	{
	case 0: break;
	case 1: state->fdc_dden = 1; break;
	case 2: state->fdc_dden = 0; break;
	case 3: state->fdc_dden = state->fdc_dden; break;
	}

	wd17xx_dden_w(state->wd1795, state->fdc_dden);
}

/* Memory Maps */

static ADDRESS_MAP_START( xor100_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_WRITE_BANK("bank1")
	AM_RANGE(0x0000, 0xf7ff) AM_READ_BANK("bank2")
	AM_RANGE(0xf800, 0xffff) AM_READ_BANK("bank3")
ADDRESS_MAP_END

static ADDRESS_MAP_START( xor100_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_DEVREADWRITE(I8251_A_TAG, msm8251_data_r, msm8251_data_w)
	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE(I8251_A_TAG, msm8251_status_r, msm8251_control_w)
	AM_RANGE(0x02, 0x02) AM_DEVREADWRITE(I8251_B_TAG, msm8251_data_r, i8251_b_data_w)
	AM_RANGE(0x03, 0x03) AM_DEVREADWRITE(I8251_B_TAG, msm8251_status_r, msm8251_control_w)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE(I8255A_TAG, i8255a_r, i8255a_w)
	AM_RANGE(0x08, 0x08) AM_WRITE(mmu_w)
	AM_RANGE(0x09, 0x09) AM_WRITE(prom_toggle_w)
	AM_RANGE(0x0a, 0x0a) AM_READ(prom_disable_r)
	AM_RANGE(0x0b, 0x0b) AM_READ_PORT("DSW0") AM_DEVWRITE(COM5016_TAG, baud_w)
	AM_RANGE(0x0c, 0x0f) AM_DEVREADWRITE(Z80CTC_TAG, z80ctc_r, z80ctc_w)
	AM_RANGE(0xf8, 0xfb) AM_DEVREADWRITE(WD1795_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0xfc, 0xfc) AM_READWRITE(fdc_wait_r, fdc_dcont_w)
	AM_RANGE(0xfd, 0xfd) AM_WRITE(fdc_dsel_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( xor100 )
	PORT_INCLUDE(generic_terminal)

	PORT_START("DSW0")
	PORT_DIPNAME( 0x0f, 0x05, "Serial Port A" )
	PORT_DIPSETTING(    0x00, "50 baud" )
	PORT_DIPSETTING(    0x01, "75 baud" )
	PORT_DIPSETTING(    0x02, "110 baud" )
	PORT_DIPSETTING(    0x03, "134.5 baud" )
	PORT_DIPSETTING(    0x04, "150 baud" )
	PORT_DIPSETTING(    0x05, "300 baud" )
	PORT_DIPSETTING(    0x06, "600 baud" )
	PORT_DIPSETTING(    0x07, "1200 baud" )
	PORT_DIPSETTING(    0x08, "1800 baud" )
	PORT_DIPSETTING(    0x09, "2000 baud" )
	PORT_DIPSETTING(    0x0a, "2400 baud" )
	PORT_DIPSETTING(    0x0b, "3600 baud" )
	PORT_DIPSETTING(    0x0c, "4800 baud" )
	PORT_DIPSETTING(    0x0d, "7200 baud" )
	PORT_DIPSETTING(    0x0e, "9600 baud" )
	PORT_DIPSETTING(    0x0f, "19200 baud" )
	PORT_DIPNAME( 0xf0, 0xe0, "Serial Port B" )
	PORT_DIPSETTING(    0x00, "50 baud" )
	PORT_DIPSETTING(    0x10, "75 baud" )
	PORT_DIPSETTING(    0x20, "110 baud" )
	PORT_DIPSETTING(    0x30, "134.5 baud" )
	PORT_DIPSETTING(    0x40, "150 baud" )
	PORT_DIPSETTING(    0x50, "300 baud" )
	PORT_DIPSETTING(    0x60, "600 baud" )
	PORT_DIPSETTING(    0x70, "1200 baud" )
	PORT_DIPSETTING(    0x80, "1800 baud" )
	PORT_DIPSETTING(    0x90, "2000 baud" )
	PORT_DIPSETTING(    0xa0, "2400 baud" )
	PORT_DIPSETTING(    0xb0, "3600 baud" )
	PORT_DIPSETTING(    0xc0, "4800 baud" )
	PORT_DIPSETTING(    0xd0, "7200 baud" )
	PORT_DIPSETTING(    0xe0, "9600 baud" )
	PORT_DIPSETTING(    0xf0, "19200 baud" )

	PORT_START("J1")
	PORT_CONFNAME( 0x01, 0x01, "J1 Wait State")
	PORT_CONFSETTING( 0x00, "No Wait States" )
	PORT_CONFSETTING( 0x01, "1 M1 Wait State" )

	PORT_START("J2")
	PORT_CONFNAME( 0x01, 0x01, "J2 CPU Speed")
	PORT_CONFSETTING( 0x00, "2 MHz" )
	PORT_CONFSETTING( 0x01, "4 MHz" )

	PORT_START("J3")
	PORT_CONFNAME( 0x01, 0x00, "J3")
	PORT_CONFSETTING( 0x00, "" )
	PORT_CONFSETTING( 0x01, "" )

	PORT_START("J4-J5")
	PORT_CONFNAME( 0x01, 0x01, "J4/J5 EPROM Type")
	PORT_CONFSETTING( 0x00, "2708" )
	PORT_CONFSETTING( 0x01, "2716" )

	PORT_START("J6")
	PORT_CONFNAME( 0x01, 0x01, "J6 EPROM")
	PORT_CONFSETTING( 0x00, "Disabled" )
	PORT_CONFSETTING( 0x01, "Enabled" )

	PORT_START("J7")
	PORT_CONFNAME( 0x01, 0x00, "J7 I/O Port Addresses")
	PORT_CONFSETTING( 0x00, "00-0F" )
	PORT_CONFSETTING( 0x01, "10-1F" )

	PORT_START("J8")
	PORT_CONFNAME( 0x01, 0x00, "J8")
	PORT_CONFSETTING( 0x00, "" )
	PORT_CONFSETTING( 0x01, "" )

	PORT_START("J9")
	PORT_CONFNAME( 0x01, 0x01, "J9 Power On RAM")
	PORT_CONFSETTING( 0x00, "Enabled" )
	PORT_CONFSETTING( 0x01, "Disabled" )
INPUT_PORTS_END

/* COM5016 Interface */

static WRITE_LINE_DEVICE_HANDLER( com5016_fr_w )
{
	xor100_state *driver_state = (xor100_state *)device->machine->driver_data;

	msm8251_transmit_clock(driver_state->i8251_a);
	msm8251_receive_clock(driver_state->i8251_a);
}

static WRITE_LINE_DEVICE_HANDLER( com5016_ft_w )
{
	xor100_state *driver_state = (xor100_state *)device->machine->driver_data;

	msm8251_transmit_clock(driver_state->i8251_b);
	msm8251_receive_clock(driver_state->i8251_b);
}

static COM8116_INTERFACE( com5016_intf )
{
	DEVCB_NULL,					/* fX/4 output */
	DEVCB_LINE(com5016_fr_w),	/* fR output */
	DEVCB_LINE(com5016_ft_w),	/* fT output */
	{ 101376, 67584, 46080, 37686, 33792, 16896, 8448, 4224, 2816, 2534, 2112, 1408, 1056, 704, 528, 264 },	// WRONG?
	{ 101376, 67584, 46080, 37686, 33792, 16896, 8448, 4224, 2816, 2534, 2112, 1408, 1056, 704, 528, 264 },	// WRONG?
};

/* Printer 8251A Interface */

static msm8251_interface printer_8251_intf =
{
	NULL,
	NULL,
	NULL
};

/* Terminal 8251A Interface */

static msm8251_interface terminal_8251_intf =
{
	NULL,
	NULL,
	NULL
};

/* Printer 8255A Interface */

static READ8_DEVICE_HANDLER( i8255_pc_r )
{
	/*

        bit     description

        PC0
        PC1
        PC2
        PC3
        PC4     ON LINE
        PC5     BUSY
        PC6     _ACK
        PC7

    */

	xor100_state *driver_state = (xor100_state *)device->machine->driver_data;
	UINT8 data = 0;

	/* on line */
	data |= centronics_vcc_r(driver_state->centronics) << 4;

	/* busy */
	data |= centronics_busy_r(driver_state->centronics) << 5;

	return data;
}

static I8255A_INTERFACE( printer_8255_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(i8255_pc_r),
	DEVCB_DEVICE_HANDLER(CENTRONICS_TAG, centronics_data_w),
	DEVCB_DEVICE_LINE(CENTRONICS_TAG, centronics_strobe_w),
	DEVCB_NULL
};

static centronics_interface xor100_centronics_intf =
{
	0,
	DEVCB_DEVICE_LINE(I8255A_TAG, i8255a_pc4_w),
	DEVCB_NULL,
	DEVCB_NULL
};

/* Z80-CTC Interface */

static WRITE_LINE_DEVICE_HANDLER( ctc_z0_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z1_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z2_w )
{
}

static Z80CTC_INTERFACE( ctc_intf )
{
	0,              	/* timer disables */
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* interrupt handler */
	DEVCB_LINE(ctc_z0_w),			/* ZC/TO0 callback */
	DEVCB_LINE(ctc_z1_w),			/* ZC/TO1 callback */
	DEVCB_LINE(ctc_z2_w)    		/* ZC/TO2 callback */
};

/* WD1795-02 Interface */

static WRITE_LINE_DEVICE_HANDLER( fdc_irq_w )
{
	xor100_state *driver_state = (xor100_state *)device->machine->driver_data;

	driver_state->fdc_irq = state;
	z80ctc_trg0_w(driver_state->z80ctc, state);

	if (state)
	{
		/* TODO: this is really connected to the Z80 _RDY line */
		cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_HALT, CLEAR_LINE);
	}
}

static WRITE_LINE_DEVICE_HANDLER( fdc_drq_w )
{
	xor100_state *driver_state = (xor100_state *)device->machine->driver_data;

	driver_state->fdc_drq = state;

	if (state)
	{
		/* TODO: this is really connected to the Z80 _RDY line */
		cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_HALT, CLEAR_LINE);
	}
}

static const wd17xx_interface wd1795_intf =
{
	DEVCB_NULL,
	DEVCB_LINE(fdc_irq_w),
	DEVCB_LINE(fdc_drq_w),
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

/* Terminal Interface */

static WRITE8_DEVICE_HANDLER( xor100_kbd_put )
{
	xor100_state *state = (xor100_state *)device->machine->driver_data;

	msm8251_receive_character(state->i8251_b, data);
}

static GENERIC_TERMINAL_INTERFACE( xor100_terminal_intf )
{
	DEVCB_HANDLER(xor100_kbd_put)
};

/* Machine Initialization */

static MACHINE_START( xor100 )
{
	xor100_state *state = (xor100_state *)machine->driver_data;
	running_device *messram = devtag_get_device(machine, "messram");
	int banks = messram_get_size(messram) / 0x10000;

	/* find devices */
	state->i8251_a = devtag_get_device(machine, I8251_A_TAG);
	state->i8251_b = devtag_get_device(machine, I8251_B_TAG);
	state->wd1795 = devtag_get_device(machine, WD1795_TAG);
	state->z80ctc = devtag_get_device(machine, Z80CTC_TAG);
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);

	/* setup memory banking */
	memory_configure_bank(machine, "bank1", 1, banks, messram_get_ptr(messram), 0x10000);
	memory_configure_bank(machine, "bank2", 0, 1, memory_region(machine, Z80_TAG), 0);
	memory_configure_bank(machine, "bank2", 1, banks, messram_get_ptr(messram), 0x10000);
	memory_configure_bank(machine, "bank3", 0, 1, memory_region(machine, Z80_TAG), 0);
	memory_configure_bank(machine, "bank3", 1, banks, messram_get_ptr(messram) + 0xf800, 0x10000);

	/* register for state saving */
	state_save_register_global(machine, state->mode);
	state_save_register_global(machine, state->bank);
	state_save_register_global(machine, state->fdc_irq);
	state_save_register_global(machine, state->fdc_drq);
	state_save_register_global(machine, state->fdc_dden);
}

static MACHINE_RESET( xor100 )
{
	xor100_state *state = (xor100_state *)machine->driver_data;

	state->mode = EPROM_0000;

	xor100_bankswitch(machine);
}

/* Machine Driver */

static const floppy_config xor100_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(default),
	DO_NOT_KEEP_GEOMETRY
};

static MACHINE_DRIVER_START( xor100 )
	MDRV_DRIVER_DATA(xor100_state)

    /* basic machine hardware */
    MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_8MHz/2)
    MDRV_CPU_PROGRAM_MAP(xor100_mem)
    MDRV_CPU_IO_MAP(xor100_io)

    MDRV_MACHINE_START(xor100)
    MDRV_MACHINE_RESET(xor100)

    /* video hardware */
	MDRV_IMPORT_FROM( generic_terminal )

	/* devices */
	MDRV_MSM8251_ADD(I8251_A_TAG, /*XTAL_8MHz/2,*/ printer_8251_intf)
	MDRV_MSM8251_ADD(I8251_B_TAG, /*XTAL_8MHz/2,*/ terminal_8251_intf)
	MDRV_I8255A_ADD(I8255A_TAG, printer_8255_intf)
	MDRV_Z80CTC_ADD(Z80CTC_TAG, XTAL_8MHz/2, ctc_intf)
	MDRV_COM8116_ADD(COM5016_TAG, 5000000, com5016_intf)
	MDRV_WD179X_ADD(WD1795_TAG, /*XTAL_8MHz/8,*/ wd1795_intf)
	MDRV_FLOPPY_2_DRIVES_ADD(xor100_floppy_config)
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, xor100_centronics_intf)
	MDRV_GENERIC_TERMINAL_ADD(TERMINAL_TAG, xor100_terminal_intf)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
	MDRV_RAM_EXTRA_OPTIONS("128K,192K,256K,320K,384K,448K,512K")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( xor100 )
	ROM_REGION( 0x800, Z80_TAG, 0 )
	ROM_SYSTEM_BIOS( 0, "v185", "v1.85" )
	ROMX_LOAD( "xp 185.8b", 0x000, 0x800, CRC(0d0bda8d) SHA1(11c83f7cd7e6a570641b44a2f2cc5737a7dd8ae3), ROM_BIOS(1) )
ROM_END

/* System Drivers */

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT    COMPANY                 FULLNAME        FLAGS */
COMP( 1980, xor100,		0,		0,		xor100,		xor100,		0,		"Xor Data Science",		"XOR S-100-12",	GAME_SUPPORTS_SAVE | GAME_NO_SOUND)
