/***************************************************************************

    Victor 9000

	- very exciting hardware, disk controller is a direct descendant
	  of the Commodore drives (designed by Chuck Peddle)

    Skeleton driver

***************************************************************************/

#include "emu.h"
#include "includes/victor9k.h"
#include "cpu/i86/i86.h"
#include "devices/flopdrv.h"
#include "devices/messram.h"
#include "machine/ctronics.h"
#include "machine/6522via.h"
#include "machine/ieee488.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"
#include "machine/upd7201.h"
#include "video/mc6845.h"

/* Memory Maps */

static ADDRESS_MAP_START( victor9k_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x1ffff) AM_RAM
	AM_RANGE(0xe0000, 0xe0001) AM_DEVREADWRITE(I8259A_TAG, pic8259_r, pic8259_w)
	AM_RANGE(0xe0020, 0xe0023) AM_DEVREADWRITE(I8253_TAG, pit8253_r, pit8253_w)
	AM_RANGE(0xe0040, 0xe0043) AM_DEVREADWRITE(UPD7201_TAG, upd7201_cd_ba_r, upd7201_cd_ba_w)
	AM_RANGE(0xe8000, 0xe8000) AM_DEVREADWRITE(HD46505S_TAG, mc6845_register_r, mc6845_register_w)
	AM_RANGE(0xe8001, 0xe8001) AM_DEVREADWRITE(HD46505S_TAG, mc6845_status_r, mc6845_address_w)
	AM_RANGE(0xe8020, 0xe802f) AM_DEVREADWRITE(M6522_1_TAG, via_r, via_w)
	AM_RANGE(0xe8040, 0xe804f) AM_DEVREADWRITE(M6522_2_TAG, via_r, via_w)
//	AM_RANGE(0xe8060, 0xe806f) AM_DEVREADWRITE(M6852_TAG, m6852_r, m6852_w)
	AM_RANGE(0xe8080, 0xe808f) AM_DEVREADWRITE(M6522_3_TAG, via_r, via_w)
	AM_RANGE(0xe80a0, 0xe80af) AM_DEVREADWRITE(M6522_4_TAG, via_r, via_w)
	AM_RANGE(0xe80c0, 0xe80cf) AM_DEVREADWRITE(M6522_6_TAG, via_r, via_w)
	AM_RANGE(0xe80e0, 0xe80ef) AM_DEVREADWRITE(M6522_5_TAG, via_r, via_w)
	AM_RANGE(0xf0000, 0xf0fff) AM_RAM AM_BASE_MEMBER(victor9k_state, video_ram)
	AM_RANGE(0xfe000, 0xfffff) AM_ROM AM_REGION(I8088_TAG, 0)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( victor9k )
INPUT_PORTS_END

/* Video */

static MC6845_UPDATE_ROW( victor9k_update_row )
{
}

static WRITE_LINE_DEVICE_HANDLER( vsync_w )
{
	victor9k_state *driver_state = (victor9k_state *)device->machine->driver_data;

	pic8259_set_irq_line(driver_state->pic, 7, state);
}

static const mc6845_interface hd46505s_intf = 
{
	SCREEN_TAG,
	10,
	NULL,
	victor9k_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(vsync_w),
	NULL
};

static VIDEO_START( victor9k )
{
	victor9k_state *state = (victor9k_state *)machine->driver_data;

	/* find devices */
	state->crt = devtag_get_device(machine, HD46505S_TAG);
}

static VIDEO_UPDATE( victor9k )
{
	victor9k_state *state = (victor9k_state *)screen->machine->driver_data;

	mc6845_update(state->crt, bitmap, cliprect);

	return 0;
}

/* Intel 8253 Interface */

static PIT8253_OUTPUT_CHANGED( mux_serial_b_w )
{
}

static PIT8253_OUTPUT_CHANGED( mux_serial_a_w )
{
}

static PIT8253_OUTPUT_CHANGED( timer_w )
{
	victor9k_state *driver_state = (victor9k_state *)device->machine->driver_data;

	pic8259_set_irq_line(driver_state->pic, 2, state);
}

static const struct pit8253_config pit_intf =
{
	{
		{
			2500000,
			mux_serial_b_w
		}, {
			2500000,
			mux_serial_a_w
		}, {
			100000,
			timer_w
		}
	}
};

/* Intel 8259 Interfaces */

/*

	pin		signal		description

	IR0		SYN			sync detect
	IR1		COMM		serial communications (7201)
	IR2		TIMER		8253 timer
	IR3		PARALLEL	all 6522 IRQ (including disk)
	IR4		IR4			expansion IR4
	IR5		IR5			expansion IR5
	IR6		KBINT		keyboard data ready
	IR7		VINT		vertical sync or nonspecific interrupt

*/

static WRITE_LINE_DEVICE_HANDLER( int0_w )
{
	cputag_set_input_line(device->machine, I8088_TAG, INPUT_LINE_IRQ0, state);
}

static const struct pic8259_interface pic_intf =
{
	int0_w
};

/* NEC uPD7201 Interface */

static WRITE_LINE_DEVICE_HANDLER( upd7201_int_w )
{
	victor9k_state *driver_state = (victor9k_state *)device->machine->driver_data;

	pic8259_set_irq_line(driver_state->pic, 1, state);
}

static UPD7201_INTERFACE( mpsc_intf )
{
	DEVCB_LINE(upd7201_int_w),	/* interrupt */
	{
		{
			0,					/* receive clock */
			0,					/* transmit clock */
			DEVCB_NULL,			/* receive DRQ */
			DEVCB_NULL,			/* transmit DRQ */
			DEVCB_NULL,			/* receive data */
			DEVCB_NULL,			/* transmit data */
			DEVCB_NULL,			/* clear to send */
			DEVCB_NULL,			/* data carrier detect */
			DEVCB_NULL,			/* ready to send */
			DEVCB_NULL,			/* data terminal ready */
			DEVCB_NULL,			/* wait */
			DEVCB_NULL			/* sync output */
		}, {
			0,					/* receive clock */
			0,					/* transmit clock */
			DEVCB_NULL,			/* receive DRQ */
			DEVCB_NULL,			/* transmit DRQ */
			DEVCB_NULL,			/* receive data */
			DEVCB_NULL,			/* transmit data */
			DEVCB_NULL,			/* clear to send */
			DEVCB_NULL,			/* data carrier detect */
			DEVCB_NULL,			/* ready to send */
			DEVCB_NULL,			/* data terminal ready */
			DEVCB_NULL,			/* wait */
			DEVCB_NULL			/* sync output */
		}
	}
};

/* M6522 Interface */

static READ8_DEVICE_HANDLER( via1_pa_r )
{
	/*

        bit     description

        PA0     DIO1
        PA1     DIO2
        PA2     DIO3
        PA3     DIO4
        PA4     DIO5
        PA5     DIO6
        PA6     DIO7
        PA7     DIO8

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( via1_pa_w )
{
	/*

        bit     description

        PA0     DIO1
        PA1     DIO2
        PA2     DIO3
        PA3     DIO4
        PA4     DIO5
        PA5     DIO6
        PA6     DIO7
        PA7     DIO8

    */
}

static READ8_DEVICE_HANDLER( via1_pb_r )
{
	/*

        bit     description

        PB0     DAV
        PB1     EOI
        PB2     REN
        PB3     ATN
        PB4     IFC
        PB5     SRQ
        PB6     NRFD
        PB7     NDAC

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( via1_pb_w )
{
	/*

        bit     description

        PB0     DAV
        PB1     EOI
        PB2     REN
        PB3     ATN
        PB4     IFC
        PB5     SRQ
        PB6     NRFD
        PB7     NDAC

    */
}

static WRITE_LINE_DEVICE_HANDLER( via1_irq_w )
{
	victor9k_state *driver_state = (victor9k_state *)device->machine->driver_data;

	driver_state->via1_irq = state;

	pic8259_set_irq_line(driver_state->pic, 3, driver_state->via1_irq | driver_state->via2_irq | driver_state->via3_irq | driver_state->via4_irq | driver_state->via5_irq | driver_state->via6_irq );
}

static const via6522_interface via1_intf =
{
	DEVCB_HANDLER(via1_pa_r),
	DEVCB_HANDLER(via1_pb_r),
	DEVCB_NULL, // NRFD
	DEVCB_NULL,
	DEVCB_NULL, // NDAC
	DEVCB_NULL,

	DEVCB_HANDLER(via1_pa_w),
	DEVCB_HANDLER(via1_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL, // CODEC VOL

	DEVCB_LINE(via1_irq_w)
};

static READ8_DEVICE_HANDLER( via2_pa_r )
{
	/*

        bit     description

        PA0     INT/EXTA
        PA1     INT/EXTB
        PA2     RIA
        PA3     DSRA
        PA4     RIB
        PA5     DSRB
        PA6     KBDATA
        PA7     VERT

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( via2_pa_w )
{
	/*

        bit     description

        PA0     INT/EXTA
        PA1     INT/EXTB
        PA2     RIA
        PA3     DSRA
        PA4     RIB
        PA5     DSRB
        PA6     KBDATA
        PA7     VERT

    */
}

static WRITE8_DEVICE_HANDLER( via2_pb_w )
{
	/*

        bit     description

        PB0     TALK/LISTEN
        PB1     KBACKCTL
        PB2     BRT0
        PB3     BRT1
        PB4     BRT2
        PB5     CONT0
        PB6     CONT1
        PB7     CONT2

    */
}

static WRITE_LINE_DEVICE_HANDLER( via2_irq_w )
{
	victor9k_state *driver_state = (victor9k_state *)device->machine->driver_data;

	driver_state->via2_irq = state;

	pic8259_set_irq_line(driver_state->pic, 3, driver_state->via1_irq | driver_state->via2_irq | driver_state->via3_irq | driver_state->via4_irq | driver_state->via5_irq | driver_state->via6_irq );
}

static const via6522_interface via2_intf =
{
	DEVCB_HANDLER(via2_pa_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL, // KBRDY
	DEVCB_NULL, // SRQ/BUSY
	DEVCB_NULL, // KBDATA

	DEVCB_HANDLER(via2_pa_w),
	DEVCB_HANDLER(via2_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_LINE(via2_irq_w)
};

static READ8_DEVICE_HANDLER( via3_pa_r )
{
	/*

        bit     description

        PA0     J5-16
        PA1     J5-18
        PA2     J5-20
        PA3     J5-22
        PA4     J5-24
        PA5     J5-26
        PA6     J5-28
        PA7     J5-30

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( via3_pa_w )
{
	/*

        bit     description

        PA0     J5-16
        PA1     J5-18
        PA2     J5-20
        PA3     J5-22
        PA4     J5-24
        PA5     J5-26
        PA6     J5-28
        PA7     J5-30

    */
}

static READ8_DEVICE_HANDLER( via3_pb_r )
{
	/*

        bit     description

        PB0     J5-32
        PB1     J5-34
        PB2     J5-36
        PB3     J5-38
        PB4     J5-40
        PB5     J5-42
        PB6     J5-44
        PB7     J5-46

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( via3_pb_w )
{
	/*

        bit     description

        PB0     J5-32
        PB1     J5-34
        PB2     J5-36
        PB3     J5-38
        PB4     J5-40
        PB5     J5-42
        PB6     J5-44
        PB7     J5-46

    */
}

static WRITE_LINE_DEVICE_HANDLER( via3_irq_w )
{
	victor9k_state *driver_state = (victor9k_state *)device->machine->driver_data;

	driver_state->via3_irq = state;

	pic8259_set_irq_line(driver_state->pic, 3, driver_state->via1_irq | driver_state->via2_irq | driver_state->via3_irq | driver_state->via4_irq | driver_state->via5_irq | driver_state->via6_irq );
}

static const via6522_interface via3_intf =
{
	DEVCB_HANDLER(via3_pa_r),
	DEVCB_HANDLER(via3_pb_r),
	DEVCB_NULL, // J5-12
	DEVCB_NULL, // J5-48
	DEVCB_NULL, // J5-14
	DEVCB_NULL, // J5-50

	DEVCB_HANDLER(via3_pa_w),
	DEVCB_HANDLER(via3_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_LINE(via3_irq_w)
};

static WRITE8_DEVICE_HANDLER( via4_pa_w )
{
	/*

        bit     description

        PA0     L0MS0
        PA1     L0MS1
        PA2     L0MS2
        PA3     L0MS3
        PA4     ST0A
        PA5     ST0B
        PA6     ST0C
        PA7     ST0D

    */
}

static WRITE8_DEVICE_HANDLER( via4_pb_w )
{
	/*

        bit     description

        PB0     L1MS0
        PB1     L1MS1
        PB2     L1MS2
        PB3		L1MS3
        PB4     ST1A
        PB5     ST1B
        PB6     ST1C
        PB7     ST1D

    */
}

static WRITE_LINE_DEVICE_HANDLER( via4_irq_w )
{
	victor9k_state *driver_state = (victor9k_state *)device->machine->driver_data;

	driver_state->via4_irq = state;

	pic8259_set_irq_line(driver_state->pic, 3, driver_state->via1_irq | driver_state->via2_irq | driver_state->via3_irq | driver_state->via4_irq | driver_state->via5_irq | driver_state->via6_irq );
}

static const via6522_interface via4_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL, // DS0
	DEVCB_NULL, // DS1
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_HANDLER(via4_pa_w),
	DEVCB_HANDLER(via4_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL, // MODE
	DEVCB_NULL,

	DEVCB_LINE(via4_irq_w)
};

static READ8_DEVICE_HANDLER( via5_pa_r )
{
	/*

        bit     description

        PA0     E0
        PA1     E1
        PA2     I1
        PA3     E2
        PA4     E4
        PA5     E5
        PA6     I7
        PA7     E6

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( via5_pb_w )
{
	/*

        bit     description

        PB0     WD0
        PB1     WD1
        PB2     WD2
        PB3     WD3
        PB4     WD4
        PB5     WD5
        PB6     WD6
        PB7     WD7

    */
}

static WRITE_LINE_DEVICE_HANDLER( via5_irq_w )
{
	victor9k_state *driver_state = (victor9k_state *)device->machine->driver_data;

	driver_state->via5_irq = state;

	pic8259_set_irq_line(driver_state->pic, 3, driver_state->via1_irq | driver_state->via2_irq | driver_state->via3_irq | driver_state->via4_irq | driver_state->via5_irq | driver_state->via6_irq );
}

static const via6522_interface via5_intf =
{
	DEVCB_HANDLER(via5_pa_r),
	DEVCB_NULL,
	DEVCB_NULL, // BRDY
	DEVCB_NULL,
	DEVCB_NULL, // RDY0
	DEVCB_NULL, // RDY1

	DEVCB_NULL,
	DEVCB_HANDLER(via5_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_LINE(via5_irq_w)
};

static READ8_DEVICE_HANDLER( via6_pa_r )
{
	/*

        bit     description

        PA0     LED0A
        PA1     TRK0D0
        PA2     LED1A
        PA3     TRK0D1
        PA4     SIDE SELECT
        PA5     DRIVE SELECT
        PA6     WPS
        PA7     SYNC

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( via6_pa_w )
{
	/*

        bit     description

        PA0     LED0A
        PA1     TRK0D0
        PA2     LED1A
        PA3     TRK0D1
        PA4     SIDE SELECT
        PA5     DRIVE SELECT
        PA6     WPS
        PA7     SYNC

    */
}

static READ8_DEVICE_HANDLER( via6_pb_r )
{
	/*

        bit     description

        PB0     RDY0
        PB1     RDY1
        PB2     SCRESET
        PB3     DS1
        PB4     DS0
        PB5     SINGLE/_DOUBLE SIDED
        PB6     stepper enable A
        PB7     stepper enable B

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( via6_pb_w )
{
	/*

        bit     description

        PB0     RDY0
        PB1     RDY1
        PB2     SCRESET
        PB3     DS1
        PB4     DS0
        PB5     SINGLE/_DOUBLE SIDED
        PB6     stepper enable A
        PB7     stepper enable B

    */
}

static WRITE_LINE_DEVICE_HANDLER( via6_irq_w )
{
	victor9k_state *driver_state = (victor9k_state *)device->machine->driver_data;

	driver_state->via6_irq = state;

	pic8259_set_irq_line(driver_state->pic, 3, driver_state->via1_irq | driver_state->via2_irq | driver_state->via3_irq | driver_state->via4_irq | driver_state->via5_irq | driver_state->via6_irq );
}

static const via6522_interface via6_intf =
{
	DEVCB_HANDLER(via6_pa_r),
	DEVCB_HANDLER(via6_pb_r),
	DEVCB_NULL, // GCRERR
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_HANDLER(via6_pa_w),
	DEVCB_HANDLER(via6_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL, // DRW
	DEVCB_NULL, // ERASE

	DEVCB_LINE(via6_irq_w)
};

/* Floppy Configuration */

static const floppy_config victor9k_floppy_config =
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

/* IEEE-488 Interface */

static IEEE488_DAISY( ieee488_daisy )
{
//	{ M6522_1_TAG },
	{ NULL }
};

/* Machine Initialization */

static MACHINE_START( victor9k )
{
	victor9k_state *state = (victor9k_state *)machine->driver_data;

	/* find devices */
	state->ieee488 = devtag_get_device(machine, IEEE488_TAG);
	state->pic = devtag_get_device(machine, I8259A_TAG);
}

/* Machine Driver */

static MACHINE_DRIVER_START( victor9k )
	MDRV_DRIVER_DATA(victor9k_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(I8088_TAG, I8088, 5000000)
	MDRV_CPU_PROGRAM_MAP(victor9k_mem)

	MDRV_MACHINE_START(victor9k)

    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(monochrome_green)

	MDRV_VIDEO_START(victor9k)
	MDRV_VIDEO_UPDATE(victor9k)

	MDRV_MC6845_ADD(HD46505S_TAG, H46505, 1000000, hd46505s_intf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
//	MDRV_M6852_ADD(M6852_TAG, 0, m6852_intf)
//	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_IEEE488_ADD(IEEE488_TAG, ieee488_daisy)
	MDRV_PIC8259_ADD(I8259A_TAG, pic_intf)
	MDRV_PIT8253_ADD(I8253_TAG, pit_intf)
	MDRV_UPD7201_ADD(UPD7201_TAG, 0, mpsc_intf)
	MDRV_VIA6522_ADD(M6522_1_TAG, 0, via1_intf)
	MDRV_VIA6522_ADD(M6522_2_TAG, 0, via2_intf)
	MDRV_VIA6522_ADD(M6522_3_TAG, 0, via3_intf)
	MDRV_VIA6522_ADD(M6522_4_TAG, 0, via4_intf)
	MDRV_VIA6522_ADD(M6522_5_TAG, 0, via5_intf)
	MDRV_VIA6522_ADD(M6522_6_TAG, 0, via6_intf)
	MDRV_FLOPPY_2_DRIVES_ADD(victor9k_floppy_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("128K")
	MDRV_RAM_EXTRA_OPTIONS("256K,384K,512K,640K,768K,896K")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( victor9k )
	ROM_REGION( 0x2000, I8088_TAG, 0 )
	ROM_LOAD( "102320.7j", 0x0000, 0x1000, CRC(3d615fd7) SHA1(b22f7e5d66404185395d8effbf57efded0079a92) )
	ROM_LOAD( "102322.8j", 0x1000, 0x1000, CRC(9209df0e) SHA1(3ee8e0c15186bbd5768b550ecc1fa3b6b1dbb928) )
	ROM_LOAD( "v9000_univ._fe_f3f7_13db.7j", 0x0000, 0x1000, CRC(25c7a59f) SHA1(8784e9aa7eb9439f81e18b8e223c94714e033911) )
	ROM_LOAD( "v9000_univ._ff_f3f7_39fe.8j", 0x1000, 0x1000, CRC(496c7467) SHA1(eccf428f62ef94ab85f4a43ba59ae6a066244a66) )

	ROM_REGION( 0x800, "gcr", 0 )
	ROM_LOAD( "100836-01.4k", 0x000, 0x800, CRC(adc601bd) SHA1(6eeff3d2063ae2d97452101aa61e27ef83a467e5) )

	ROM_REGION( 0x800, "motor", 0)
	ROM_LOAD( "disk spindle motor control i8048", 0x000, 0x800, NO_DUMP )

	ROM_REGION( 0x800, "keyboard", 0)
	ROM_LOAD( "keyboard controller i8048", 0x000, 0x800, NO_DUMP )
ROM_END

/* System Drivers */

/*    YEAR  NAME      PARENT  COMPAT  MACHINE   INPUT     INIT  COMPANY						FULLNAME		FLAGS */
COMP( 1982, victor9k, 0,      0,      victor9k, victor9k, 0,    "Victor Business Products", "Victor 9000",	GAME_NOT_WORKING | GAME_NO_SOUND )
