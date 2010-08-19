/***************************************************************************

    ACT Apricot PC/Xi

    - Needs Intel 8089 support (I/O processor)
    - Dump of the keyboard MCU ROM needed (can be dumped using test mode)

***************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"
#include "machine/pit8253.h"
#include "machine/i8255a.h"
#include "machine/pic8259.h"
#include "machine/z80sio.h"
#include "machine/wd17xx.h"
#include "sound/sn76496.h"
#include "video/mc6845.h"
#include "devices/messram.h"
#include "devices/flopdrv.h"
#include "formats/apridisk.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

class apricot_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, apricot_state(machine)); }

	apricot_state(running_machine &machine)
		: driver_data_t(machine) { }

	running_device *pic8259;
	running_device *wd2793;
	running_device *mc6845;

	int video_mode;
	int display_on;
	int display_enabled;
	UINT16 *screen_buffer;
};


/***************************************************************************
    I/O
***************************************************************************/

static READ8_DEVICE_HANDLER( apricot_sysctrl_r )
{
	apricot_state *apricot = device->machine->driver_data<apricot_state>();
	UINT8 data = 0;

	data |= apricot->display_enabled << 3;

	return data;
}

static WRITE8_DEVICE_HANDLER( apricot_sysctrl_w )
{
	apricot_state *apricot = device->machine->driver_data<apricot_state>();

	apricot->display_on = BIT(data, 3);
	apricot->video_mode = BIT(data, 4);
	if (!BIT(data, 5)) wd17xx_set_drive(apricot->wd2793, BIT(data, 6));

	/* switch video modes */
	mc6845_set_clock(apricot->mc6845, apricot->video_mode ? XTAL_15MHz / 10 : XTAL_15MHz / 16);
	mc6845_set_hpixels_per_column(apricot->mc6845, apricot->video_mode ? 10 : 16);
}

static const i8255a_interface apricot_i8255a_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(apricot_sysctrl_r),
	DEVCB_NULL,
	DEVCB_HANDLER(apricot_sysctrl_w),
	DEVCB_NULL
};

static WRITE_LINE_DEVICE_HANDLER( apricot_pit8253_out1 )
{
	/* connected to the rs232c interface */
}

static WRITE_LINE_DEVICE_HANDLER( apricot_pit8253_out2 )
{
	/* connected to the rs232c interface */
}

static const struct pit8253_config apricot_pit8253_intf =
{
	{
		{ XTAL_4MHz / 16,      DEVCB_LINE_VCC, DEVCB_DEVICE_LINE("ic31", pic8259_ir6_w) },
		{ 0 /*XTAL_4MHz / 2*/, DEVCB_LINE_VCC, DEVCB_LINE(apricot_pit8253_out1) },
		{ 0 /*XTAL_4MHz / 2*/, DEVCB_LINE_VCC, DEVCB_LINE(apricot_pit8253_out2) }
	}
};

static void apricot_sio_irq_w(running_device *device, int state)
{
	apricot_state *apricot = device->machine->driver_data<apricot_state>();
	pic8259_ir5_w(apricot->pic8259, state);
}

static const z80sio_interface apricot_z80sio_intf =
{
	apricot_sio_irq_w,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};


/***************************************************************************
    INTERRUPTS
***************************************************************************/

static IRQ_CALLBACK( apricot_irq_ack )
{
	apricot_state *apricot = device->machine->driver_data<apricot_state>();
	return pic8259_acknowledge(apricot->pic8259);
}

static const struct pic8259_interface apricot_pic8259_intf =
{
	DEVCB_CPU_INPUT_LINE("maincpu", 0)
};


/***************************************************************************
    FLOPPY
***************************************************************************/

static WRITE_LINE_DEVICE_HANDLER( apricot_wd2793_intrq_w )
{
	apricot_state *apricot = device->machine->driver_data<apricot_state>();

	pic8259_ir4_w(apricot->pic8259, state);
//  i8089 external terminate channel 1
}

static WRITE_LINE_DEVICE_HANDLER( apricot_wd2793_drq_w )
{
//  i8089 data request channel 1
}

static const wd17xx_interface apricot_wd17xx_intf =
{
	DEVCB_LINE_GND,
	DEVCB_LINE(apricot_wd2793_intrq_w),
	DEVCB_LINE(apricot_wd2793_drq_w),
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};


/***************************************************************************
    VIDEO EMULATION
***************************************************************************/

static VIDEO_UPDATE( apricot )
{
	apricot_state *apricot = screen->machine->driver_data<apricot_state>();

	if (!apricot->display_on)
		mc6845_update(apricot->mc6845, bitmap, cliprect);
	else
		bitmap_fill(bitmap, cliprect, 0);

	return 0;
}

static MC6845_UPDATE_ROW( apricot_update_row )
{
	apricot_state *apricot = device->machine->driver_data<apricot_state>();
	UINT8 *ram = messram_get_ptr(device->machine->device("messram"));
	int i, x;

	if (apricot->video_mode)
	{
		/* text mode */
		for (i = 0; i < x_count; i++)
		{
			UINT16 code = apricot->screen_buffer[(ma + i) & 0x7ff];
			UINT16 offset = ((code & 0x7ff) << 5) | (ra << 1);
			UINT16 data = ram[offset + 1] << 8 | ram[offset];
			int fill = 0;

			if (BIT(code, 12) && BIT(data, 14)) fill = 1; /* strike-through? */
			if (BIT(code, 13) && BIT(data, 15)) fill = 1; /* underline? */

			/* draw 10 pixels of the character */
			for (x = 0; x <= 10; x++)
			{
				int color = fill ? 1 : BIT(data, x);
				if (BIT(code, 15)) color = !color; /* reverse? */
				*BITMAP_ADDR16(bitmap, y, x + i*10) = color ? 1 + BIT(code, 14) : 0;
			}
		}
	}
	else
	{
		/* graphics mode */
		fatalerror("Graphics mode not implemented!");
	}
}

static WRITE_LINE_DEVICE_HANDLER( apricot_mc6845_de )
{
	apricot_state *apricot = device->machine->driver_data<apricot_state>();
	apricot->display_enabled = state;
}

static const mc6845_interface apricot_mc6845_intf =
{
	"screen",
	10,
	NULL,
	apricot_update_row,
	NULL,
	DEVCB_LINE(apricot_mc6845_de),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};


/***************************************************************************
    DRIVER INIT
***************************************************************************/

static DRIVER_INIT( apricot )
{
	apricot_state *apricot = machine->driver_data<apricot_state>();
	running_device *maincpu = machine->device("maincpu");
	address_space *prg = cpu_get_address_space(maincpu, ADDRESS_SPACE_PROGRAM);

	UINT8 *ram = messram_get_ptr(machine->device("messram"));
	UINT32 ram_size = messram_get_size(machine->device("messram"));

	memory_unmap_readwrite(prg, 0x40000, 0xeffff, 0, 0);
	memory_install_ram(prg, 0x00000, ram_size - 1, 0, 0, ram);

	cpu_set_irq_callback(maincpu, apricot_irq_ack);

	apricot->pic8259 = machine->device("ic31");
	apricot->wd2793 = machine->device("ic68");
	apricot->mc6845 = machine->device("ic30");

	apricot->video_mode = 0;
	apricot->display_on = 1;
}


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START( apricot_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x3ffff) AM_RAMBANK("standard_ram")
	AM_RANGE(0x40000, 0xeffff) AM_RAMBANK("expansion_ram")
	AM_RANGE(0xf0000, 0xf0fff) AM_MIRROR(0x7000) AM_RAM AM_BASE_MEMBER(apricot_state, screen_buffer)
	AM_RANGE(0xfc000, 0xfffff) AM_MIRROR(0x4000) AM_ROM AM_REGION("bootstrap", 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( apricot_io, ADDRESS_SPACE_IO, 16 )
	AM_RANGE(0x00, 0x03) AM_DEVREADWRITE8("ic31", pic8259_r, pic8259_w, 0x00ff)
	AM_RANGE(0x40, 0x47) AM_DEVREADWRITE8("ic68", wd17xx_r, wd17xx_w, 0x00ff)
	AM_RANGE(0x48, 0x4f) AM_DEVREADWRITE8("ic17", i8255a_r, i8255a_w, 0x00ff)
	AM_RANGE(0x50, 0x51) AM_MIRROR(0x06) AM_DEVWRITE8("ic7", sn76496_w, 0x00ff)
	AM_RANGE(0x58, 0x5f) AM_DEVREADWRITE8("ic16", pit8253_r, pit8253_w, 0x00ff)
	AM_RANGE(0x60, 0x67) AM_DEVREADWRITE8("ic15", z80sio_ba_cd_r, z80sio_ba_cd_w, 0x00ff)
	AM_RANGE(0x68, 0x69) AM_MIRROR(0x04) AM_DEVWRITE8("ic30", mc6845_address_w, 0x00ff)
	AM_RANGE(0x6a, 0x6b) AM_MIRROR(0x04) AM_DEVREADWRITE8("ic30", mc6845_register_r, mc6845_register_w, 0x00ff)
//  AM_RANGE(0x70, 0x71) AM_MIRROR(0x04) 8089 channel attention 1
//  AM_RANGE(0x72, 0x73) AM_MIRROR(0x04) 8089 channel attention 2
	AM_RANGE(0x78, 0x7f) AM_NOP /* unavailable */
ADDRESS_MAP_END


/***************************************************************************
    INPUT PORTS
***************************************************************************/

static INPUT_PORTS_START( apricot )
INPUT_PORTS_END


/***************************************************************************
    PALETTE
***************************************************************************/

static PALETTE_INIT( apricot )
{
	palette_set_color(machine, 0, MAKE_RGB(0x00, 0x00, 0x00)); /* black */
	palette_set_color(machine, 1, MAKE_RGB(0x00, 0x7f, 0x00)); /* low intensity */
	palette_set_color(machine, 2, MAKE_RGB(0x00, 0xff, 0x00)); /* high intensitiy */
}


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static FLOPPY_OPTIONS_START( apricot )
	FLOPPY_OPTION
	(
		apridisk, "dsk", "ACT Apricot disk image", apridisk_identify, apridisk_construct,
		HEADS(1-[2])
		TRACKS(70/[80])
		SECTORS([9]/18)
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1])
	)
FLOPPY_OPTIONS_END

static const floppy_config apricot_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_SSDD_40,
	FLOPPY_OPTIONS_NAME(apricot),
	NULL
};

static MACHINE_DRIVER_START( apricot )
	MDRV_DRIVER_DATA(apricot_state)

	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", I8086, XTAL_15MHz / 3)
	MDRV_CPU_PROGRAM_MAP(apricot_mem)
	MDRV_CPU_IO_MAP(apricot_io)

//  MDRV_CPU_ADD("ic71", I8089, XTAL_15MHz / 3)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(800, 400)
	MDRV_SCREEN_VISIBLE_AREA(0, 800-1, 0, 400-1)
	MDRV_SCREEN_REFRESH_RATE(72)

	MDRV_PALETTE_LENGTH(3)
	MDRV_PALETTE_INIT(apricot)

	MDRV_VIDEO_UPDATE(apricot)

	MDRV_MC6845_ADD("ic30", MC6845, XTAL_15MHz / 10, apricot_mc6845_intf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("ic7", SN76489, XTAL_4MHz / 2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("256k")
	MDRV_RAM_EXTRA_OPTIONS("384k,512k") /* with 1 or 2 128k expansion boards */

	MDRV_I8255A_ADD("ic17", apricot_i8255a_intf)
	MDRV_PIC8259_ADD("ic31", apricot_pic8259_intf)
	MDRV_PIT8253_ADD("ic16", apricot_pit8253_intf)
	MDRV_Z80SIO_ADD("ic15", 0, apricot_z80sio_intf)

	/* floppy */
	MDRV_WD2793_ADD("ic68", apricot_wd17xx_intf)
	MDRV_FLOPPY_2_DRIVES_ADD(apricot_floppy_config)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( apricotxi )
	MDRV_IMPORT_FROM(apricot)
MACHINE_DRIVER_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

ROM_START( apricot )
	ROM_REGION(0x4000, "bootstrap", 0)
	ROM_LOAD16_BYTE("pc_bios_lo_001.bin", 0x0000, 0x2000, CRC(0c217cc2) SHA1(0d7a2b61e17966462b555115f962a175fadcf72a))
	ROM_LOAD16_BYTE("pc_bios_hi_001.bin", 0x0001, 0x2000, CRC(7c27f36c) SHA1(c801bbf904815f76ec6463e948f57e0118a26292))
ROM_END

ROM_START( apricotxi )
	ROM_REGION(0x4000, "bootstrap", 0)
	ROM_LOAD16_BYTE("lo_ve007.u11", 0x0000, 0x2000, CRC(e74e14d1) SHA1(569133b0266ce3563b21ae36fa5727308797deee)) /* LO Ve007 03.04.84 */
	ROM_LOAD16_BYTE("hi_ve007.u9",  0x0001, 0x2000, CRC(b04fb83e) SHA1(cc2b2392f1b4c04bb6ec8ee26f8122242c02e572)) /* HI Ve007 03.04.84 */
ROM_END


/***************************************************************************
    GAME DRIVERS
***************************************************************************/

/*    YEAR  NAME       PARENT   COMPAT  MACHINE    INPUT    INIT     COMPANY  FULLNAME      FLAGS */
COMP( 1983, apricot,   0,       0,      apricot,   apricot, apricot, "ACT",   "Apricot PC", GAME_NOT_WORKING )
COMP( 1984, apricotxi, apricot, 0,      apricotxi, apricot, apricot, "ACT",   "Apricot Xi", GAME_NOT_WORKING )
