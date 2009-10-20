/****************************************************************************************************

        XOR S-100-12

        12/05/2009 Skeleton driver.

    Execution of this monitor rom starts at F800.

    I/O sequence:   read 0A, discard result
            read 0B, output result to 0B
            send the sequence AA 40 4E 37 to 01, then to 03 (programming a device?)
            L1:read 03 to see if display device is ready
            write ascii character to 02 goto L1 until signon message is displayed
            in same fashion display top of memory (F800)
            display prompt character (*) wait for input
            read 03 to see if a key is being input, if so read 02 and display it
            if illegal input, display ? then display prompt on a new line

    To summarise:   out 01 and out 03 - program a device
            out 02 - display a character
            in 02 - read character from keyboard
            in 03 - get busy status of display (bit 0) and keyboard (bit 1) High=ready

*****************************************************************************************************/

/*

    TODO:

    - terminal
    - keyboard
    - ROM should be mirrored every 2K at boot
    - 2/4 MHz jumper J2
    - floppy
    - memory expansion
    - COM5016
    - CTC

*/

#include "driver.h"
#include "includes/xor100.h"
#include "cpu/z80/z80.h"
#include "formats/basicdsk.h"
#include "devices/flopdrv.h"
#include "devices/cassette.h"
#include "machine/com8116.h"
#include "machine/i8255a.h"
#include "machine/msm8251.h"
#include "machine/wd17xx.h"
#include "machine/z80ctc.h"
#include "devices/messram.h"

/* Read/Write Handlers */

static WRITE8_HANDLER( mmu_w )
{
	/*

        bit     description

        0       A16
        1       A17

    */
}

static WRITE8_HANDLER( prom_toggle_w )
{
	memory_set_bank(space->machine, 3, BIT(data, 0));
}

static READ8_HANDLER( prom_disable_r )
{
	memory_install_readwrite8_handler(space, 0x0000, 0xf7ff, 0, 0, SMH_BANK(1), SMH_BANK(2));
	memory_set_bank(space->machine, 1, 1);

	return 0xff;
}

static WRITE8_DEVICE_HANDLER( baud_w )
{
	com8116_str_w(device, 0, data & 0x0f);
	com8116_stt_w(device, 0, data >> 4);
}

/* Memory Maps */

static ADDRESS_MAP_START( xor100_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xf7ff) AM_READWRITE(SMH_BANK(1), SMH_BANK(2))
	AM_RANGE(0xf800, 0xffff) AM_READWRITE(SMH_BANK(3), SMH_BANK(4))
ADDRESS_MAP_END

static ADDRESS_MAP_START( xor100_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_DEVREADWRITE(I8251_A_TAG, msm8251_data_r, msm8251_data_w)
	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE(I8251_A_TAG, msm8251_status_r, msm8251_control_w)
	AM_RANGE(0x02, 0x02) AM_DEVREADWRITE(I8251_B_TAG, msm8251_data_r, msm8251_data_w)
	AM_RANGE(0x03, 0x03) AM_DEVREADWRITE(I8251_B_TAG, msm8251_status_r, msm8251_control_w)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE(I8255A_TAG, i8255a_r, i8255a_w)
	AM_RANGE(0x08, 0x08) AM_WRITE(mmu_w)
	AM_RANGE(0x09, 0x09) AM_WRITE(prom_toggle_w)
	AM_RANGE(0x0a, 0x0a) AM_READ(prom_disable_r)
	AM_RANGE(0x0b, 0x0b) AM_READ_PORT("DSW0") AM_DEVWRITE(COM5016_TAG, baud_w)
	AM_RANGE(0x0c, 0x0f) AM_DEVREADWRITE(Z80CTC_TAG, z80ctc_r, z80ctc_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( xor100 )
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
INPUT_PORTS_END

/* Video */

static VIDEO_START( xor100 )
{
}

static VIDEO_UPDATE( xor100 )
{
    return 0;
}

/* COM5016 Interface */

static WRITE_LINE_DEVICE_HANDLER( com5016_fr_w )
{
	xor100_state *driver_state = device->machine->driver_data;

	msm8251_transmit_clock(driver_state->i8251_a);
	msm8251_receive_clock(driver_state->i8251_a);
}

static WRITE_LINE_DEVICE_HANDLER( com5016_ft_w )
{
	xor100_state *driver_state = device->machine->driver_data;

	msm8251_transmit_clock(driver_state->i8251_b);
	msm8251_receive_clock(driver_state->i8251_b);
}

static COM8116_INTERFACE( com5016_intf )
{
	DEVCB_NULL,					/* fX/4 output */
	DEVCB_LINE(com5016_fr_w),	/* fR output */
	DEVCB_LINE(com5016_ft_w),	/* fT output */
	{ 101376, 67584, 46080, 37686, 33792, 16896, 8448, 4224, 2816, 2534, 2112, 1408, 1056, 704, 528, 264 },	// WRONG
	{ 101376, 67584, 46080, 37686, 33792, 16896, 8448, 4224, 2816, 2534, 2112, 1408, 1056, 704, 528, 264 },	// WRONG
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

static I8255A_INTERFACE( printer_8255_intf )
{
	DEVCB_NULL,		// Port A read
	DEVCB_NULL,		// Port B read
	DEVCB_NULL,		// Port C read
	DEVCB_NULL,		// Port A write
	DEVCB_NULL,		// Port B write
	DEVCB_NULL		// Port C write
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

static const wd17xx_interface wd1795_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	{FLOPPY_0, FLOPPY_1, NULL, NULL}
};

/* Machine Initialization */

static MACHINE_START( xor100 )
{
	xor100_state *state = machine->driver_data;

	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	/* find devices */
	state->i8251_a = devtag_get_device(machine, I8251_A_TAG);
	state->i8251_b = devtag_get_device(machine, I8251_B_TAG);
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);

	/* setup memory banking */
	memory_install_readwrite8_handler(program, 0xf800, 0xffff, 0, 0, SMH_BANK(3), SMH_BANK(4));

	memory_configure_bank(machine, 1, 0, 1, memory_region(machine, Z80_TAG) + 0xf800, 0);
	memory_configure_bank(machine, 1, 1, 1, messram_get_ptr(devtag_get_device(machine, "messram")), 0);

	memory_configure_bank(machine, 2, 0, 1, messram_get_ptr(devtag_get_device(machine, "messram")), 0);
	memory_set_bank(machine, 2, 0);

	memory_configure_bank(machine, 3, 0, 1, memory_region(machine, Z80_TAG) + 0xf800, 0);
	memory_configure_bank(machine, 3, 1, 1, messram_get_ptr(devtag_get_device(machine, "messram")) + 0xf800, 0);

	memory_configure_bank(machine, 4, 0, 1, messram_get_ptr(devtag_get_device(machine, "messram")) + 0xf800, 0);
	memory_set_bank(machine, 4, 0);
}

static MACHINE_RESET( xor100 )
{
	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	memory_install_read8_handler(program, 0x0000, 0x07ff, 0, 0, SMH_BANK(1)); // TODO: should be mirrored every 2K
	memory_install_write8_handler(program, 0x0000, 0xf7ff, 0, 0, SMH_BANK(2));
	memory_set_bank(machine, 1, 0);
	memory_set_bank(machine, 3, 0);
}

/* Machine Driver */

static const cassette_config xor100_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED
};

static FLOPPY_OPTIONS_START( xor100 )
	FLOPPY_OPTION( xor100, "dsk", "XOR-100-12 disk image", basicdsk_identify_default, basicdsk_construct_default, // WRONG
		HEADS([1])
		TRACKS([80])
		SECTORS([10])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config xor100_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(xor100),
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
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(xor100)
    MDRV_VIDEO_UPDATE(xor100)

	/* devices */
	MDRV_MSM8251_ADD(I8251_A_TAG, /*XTAL_8MHz/2,*/ printer_8251_intf)
	MDRV_MSM8251_ADD(I8251_B_TAG, /*XTAL_8MHz/2,*/ terminal_8251_intf)
	MDRV_I8255A_ADD(I8255A_TAG, printer_8255_intf)
	MDRV_Z80CTC_ADD(Z80CTC_TAG, XTAL_8MHz/2, ctc_intf)
	MDRV_COM8116_ADD(COM5016_TAG, 5000000, com5016_intf) // COM5016
	MDRV_WD179X_ADD(WD1795_TAG, /*XTAL_8MHz/8,*/ wd1795_intf ) // WD1795-02
	MDRV_FLOPPY_2_DRIVES_ADD(xor100_floppy_config)

	MDRV_CASSETTE_ADD(CASSETTE_TAG, xor100_cassette_config)
	
	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")	
MACHINE_DRIVER_END

/* ROMs */

ROM_START( xor100 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "xp 185.8b", 0xf800, 0x0800, CRC(0d0bda8d) SHA1(11c83f7cd7e6a570641b44a2f2cc5737a7dd8ae3) )
ROM_END


/* System Drivers */

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT    CONFIG      COMPANY                 FULLNAME        FLAGS */
COMP( 1982, xor100,		0,		0,		xor100,		xor100,		0,		0,		"Xor Data Science",		"XOR S-100-12",	GAME_NOT_WORKING )
