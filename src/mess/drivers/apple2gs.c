/***************************************************************************

    drivers/apple2gs.c
    Apple IIgs
    Driver by Nathan Woods and R. Belmont

    TODO:
    - Fix spurious interrupt problem
    - Fix 5.25" disks
    - Optimize video code
    - More RAM configurations

    NOTES:

    Video timing and the h/vcount registers:
                                  VCounts
    HCounts go like this:                     0xfa (start of frame, still in vblank)
    0 0x40 0x41 0x58 (first visible pixel)        0x7f
                 ____________________________________     0x100 (first visible scan line)
                |                                    |
                |                                    |
                |                                    |
                |                                    |
                |                                    |
    HBL region  |                                    |
                |                                    |
                |                                    |
                |                                    |
                |                                    |
                |                                    |   0x1c0 (first line of Vblank, c019 and heartbeat trigger here, only true VBL if in A2 classic modes)
                |                                    |
                 ____________________________________    0x1c8 (actual start of vblank in IIgs modes)

                                 0x1ff (end of frame, in vblank)

    There are 64 HCounts total, and 704 pixels total, so HCounts do not map to the pixel clock.
    VCounts do map directly to scanlines however, and count 262 of them.

=================================================================

***************************************************************************/

#include "emu.h"
#include "cpu/g65816/g65816.h"
#include "includes/apple2.h"
#include "machine/ay3600.h"
#include "imagedev/flopdrv.h"
#include "formats/ap2_dsk.h"
#include "formats/ap_dsk35.h"
#include "includes/apple2gs.h"
#include "devices/sonydriv.h"
#include "devices/appldriv.h"
#include "sound/es5503.h"
#include "machine/ap2_slot.h"
#include "machine/ap2_lang.h"
#include "machine/applefdc.h"
#include "machine/mockngbd.h"
#include "machine/8530scc.h"
#include "sound/ay8910.h"
#include "sound/speaker.h"
#include "imagedev/cassette.h"
#include "machine/ram.h"
#include "deprecat.h"

static const gfx_layout apple2gs_text_layout =
{
	14,8,		/* 14*8 characters */
	512,		/* 256 characters */
	1,			/* 1 bits per pixel */
	{ 0 },		/* no bitplanes; 1 bit per pixel */
	{ 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1 },   /* x offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8			/* every char takes 8 bytes */
};

static const gfx_layout apple2gs_dbltext_layout =
{
	7,8,		/* 7*8 characters */
	512,		/* 256 characters */
	1,			/* 1 bits per pixel */
	{ 0 },		/* no bitplanes; 1 bit per pixel */
	{ 7, 6, 5, 4, 3, 2, 1 },    /* x offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8			/* every char takes 8 bytes */
};

static GFXDECODE_START( apple2gs )
	GFXDECODE_ENTRY( "gfx1", 0x0000, apple2gs_text_layout, 0, 2 )
	GFXDECODE_ENTRY( "gfx1", 0x0000, apple2gs_dbltext_layout, 0, 2 )
GFXDECODE_END

static const unsigned char apple2gs_palette[] =
{
	0x0, 0x0, 0x0,	/* Black         $0              $0000 */
	0xD, 0x0, 0x3,	/* Deep Red      $1              $0D03 */
	0x0, 0x0, 0x9,	/* Dark Blue     $2              $0009 */
	0xD, 0x2, 0xD,	/* Purple        $3              $0D2D */
	0x0, 0x7, 0x2,	/* Dark Green    $4              $0072 */
	0x5, 0x5, 0x5,	/* Dark Gray     $5              $0555 */
	0x2, 0x2, 0xF,	/* Medium Blue   $6              $022F */
	0x6, 0xA, 0xF,	/* Light Blue    $7              $06AF */
	0x8, 0x5, 0x0,	/* Brown         $8              $0850 */
	0xF, 0x6, 0x0,	/* Orange        $9              $0F60 */
	0xA, 0xA, 0xA,	/* Light Gray    $A              $0AAA */
	0xF, 0x9, 0x8,	/* Pink          $B              $0F98 */
	0x1, 0xD, 0x0,	/* Light Green   $C              $01D0 */
	0xF, 0xF, 0x0,	/* Yellow        $D              $0FF0 */
	0x4, 0xF, 0x9,	/* Aquamarine    $E              $04F9 */
	0xF, 0xF, 0xF	/* White         $F              $0FFF */
};

static INPUT_PORTS_START( apple2gs )
	PORT_INCLUDE( apple2ep )

	PORT_START("adb_mouse_x")
	PORT_BIT( 0x7f, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CODE(MOUSECODE_BUTTON2) PORT_NAME("Mouse Button 1")

	PORT_START("adb_mouse_y")
	PORT_BIT( 0x7f, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CODE(MOUSECODE_BUTTON1) PORT_NAME("Mouse Button 0")

INPUT_PORTS_END



/* Initialize the palette */
static PALETTE_INIT( apple2gs )
{
	int i;

	PALETTE_INIT_CALL(apple2);

	for (i = 0; i < 16; i++)
	{
		palette_set_color_rgb(machine, i,
			apple2gs_palette[(3*i)]*17,
			apple2gs_palette[(3*i)+1]*17,
			apple2gs_palette[(3*i)+2]*17);
	}
}

static READ8_DEVICE_HANDLER( apple2gs_adc_read )
{
	return 0x80;
}

static const es5503_interface apple2gs_es5503_interface =
{
	apple2gs_doc_irq,
	apple2gs_adc_read,
	NULL
};

#ifdef UNUSED_FUNCTION
static void apple2gs_floppy35_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
    // 3.5" floppy
    switch(state)
    {
        case MESS_DEVINFO_INT_SONYDRIV_ALLOWABLE_SIZES:     info->i = SONY_FLOPPY_ALLOW400K | SONY_FLOPPY_ALLOW800K | SONY_FLOPPY_SUPPORT2IMG; break;

        case MESS_DEVINFO_STR_NAME+0:                       strcpy(info->s = device_temp_str(), "slot5disk1"); break;
        case MESS_DEVINFO_STR_NAME+1:                       strcpy(info->s = device_temp_str(), "slot5disk2"); break;
        case MESS_DEVINFO_STR_SHORT_NAME+0:                 strcpy(info->s = device_temp_str(), "s5d1"); break;
        case MESS_DEVINFO_STR_SHORT_NAME+1:                 strcpy(info->s = device_temp_str(), "s5d2"); break;
        case MESS_DEVINFO_STR_DESCRIPTION+0:                    strcpy(info->s = device_temp_str(), "Slot 5 Disk #1"); break;
        case MESS_DEVINFO_STR_DESCRIPTION+1:                    strcpy(info->s = device_temp_str(), "Slot 5 Disk #2"); break;

        default:                                        sonydriv_device_getinfo(devclass, state, info); break;
    }
}



static void apple2gs_floppy525_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
    // 5.25" floppy
    switch(state)
    {
        case MESS_DEVINFO_INT_APPLE525_SPINFRACT_DIVIDEND:  info->i = 15; break;
        case MESS_DEVINFO_INT_APPLE525_SPINFRACT_DIVISOR:   info->i = 16; break;

        case MESS_DEVINFO_STR_NAME+0:                       strcpy(info->s = device_temp_str(), "slot6disk1"); break;
        case MESS_DEVINFO_STR_NAME+1:                       strcpy(info->s = device_temp_str(), "slot6disk2"); break;
        case MESS_DEVINFO_STR_SHORT_NAME+0:                 strcpy(info->s = device_temp_str(), "s6d1"); break;
        case MESS_DEVINFO_STR_SHORT_NAME+1:                 strcpy(info->s = device_temp_str(), "s6d2"); break;
        case MESS_DEVINFO_STR_DESCRIPTION+0:                    strcpy(info->s = device_temp_str(), "Slot 6 Disk #1"); break;
        case MESS_DEVINFO_STR_DESCRIPTION+1:                    strcpy(info->s = device_temp_str(), "Slot 6 Disk #2"); break;

        default:                                        apple525_device_getinfo(devclass, state, info); break;
    }
}
#endif

static const floppy_config apple2gs_floppy35_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(apple35_iigs),
	NULL
};

static const floppy_config apple2gs_floppy525_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(apple2),
	NULL
};

static const cassette_config apple2gs_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED),
	NULL
};

static ADDRESS_MAP_START( apple2gs_map, AS_PROGRAM, 8 )
	/* nothing in the address map - everything is added dynamically */
ADDRESS_MAP_END


static const ay8910_interface apple2_ay8910_interface =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_NULL
};

static MACHINE_CONFIG_START( apple2gs, apple2gs_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", G65816, APPLE2GS_14M/5)
	MCFG_CPU_PROGRAM_MAP(apple2gs_map)
	MCFG_CPU_VBLANK_INT_HACK(apple2_interrupt, 192/8)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))

	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(704, 262)	// 640+32+32 for the borders
	MCFG_SCREEN_VISIBLE_AREA(0,703,0,230)
	MCFG_SCREEN_UPDATE( apple2gs )

	MCFG_PALETTE_LENGTH( 16+256 )
	MCFG_GFXDECODE( apple2gs )

	MCFG_MACHINE_START( apple2gs )
	MCFG_MACHINE_RESET( apple2gs )

	MCFG_PALETTE_INIT( apple2gs )
	MCFG_VIDEO_START( apple2gs )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("a2speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MCFG_SOUND_ADD("ay8913.1", AY8913, 1022727)
	MCFG_SOUND_CONFIG(apple2_ay8910_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_ADD("ay8913.2", AY8913, 1022727)
	MCFG_SOUND_CONFIG(apple2_ay8910_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MCFG_SOUND_ADD("es5503", ES5503, APPLE2GS_7M)
	MCFG_SOUND_CONFIG(apple2gs_es5503_interface)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1.0)

	/* slot devices */
	MCFG_APPLE2_LANGCARD_ADD("langcard")
	MCFG_MOCKINGBOARD_ADD("mockingboard")
	MCFG_IWM_ADD("fdc", apple2_fdc_interface)

	/* slots */
	MCFG_APPLE2_SLOT_ADD(0, "langcard", apple2_langcard_r, apple2_langcard_w, 0, 0, 0, 0)
	MCFG_APPLE2_SLOT_ADD(4, "mockingboard", mockingboard_r, mockingboard_w, 0, 0, 0, 0)
	MCFG_APPLE2_SLOT_ADD(6, "fdc", applefdc_r, applefdc_w, 0, 0, 0, 0)

	/* SCC */
	MCFG_SCC8530_ADD("scc", APPLE2GS_14M/2)

	MCFG_FLOPPY_APPLE_2_DRIVES_ADD(apple2gs_floppy525_floppy_config,15,16)
	MCFG_FLOPPY_SONY_2_DRIVES_ADDITIONAL_ADD(apple2gs_floppy35_floppy_config)

	MCFG_NVRAM_HANDLER( apple2gs )

	MCFG_CASSETTE_ADD( "cassette", apple2gs_cassette_config )

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("2M")      // 1M on board + 1M in the expansion slot was common for ROM 03
	MCFG_RAM_EXTRA_OPTIONS("1M,3M,4M,5M,6M,7M,8M")
	MCFG_RAM_DEFAULT_VALUE(0x00)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( apple2gsr1, apple2gs )
	MCFG_MACHINE_START( apple2gsr1 )

    MCFG_RAM_MODIFY(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1280K")  // 256K on board + 1M in the expansion slot was common for ROM 01
	MCFG_RAM_EXTRA_OPTIONS("256K,512K,768K,1M,2M,3M,4M,5M,6M,7M,8M")
	MCFG_RAM_DEFAULT_VALUE(0x00)
MACHINE_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(apple2gs)
	ROM_REGION(0x1000,"keyboard",0)
	ROM_LOAD( "341s0632-2.bin", 0x000000, 0x001000, CRC(e1c11fb0) SHA1(141d18c36a617ab9dce668445440d34354be0672) )

	ROM_REGION(0x1000,"gfx1",0)
	ROM_LOAD ( "apple2gs.chr", 0x0000, 0x1000, CRC(91e53cd8) SHA1(34e2443e2ef960a36c047a09ed5a93f471797f89)) /* need label/part number */

	ROM_REGION(0x40000,"maincpu",0)
	ROM_LOAD("341-0737", 0x0000, 0x20000, CRC(8d410067) SHA1(c0f4704233ead14cb8e1e8a68fbd7063c56afd27)) /* Needs verification; 341-0737: IIgs ROM03 FC-FD */
	ROM_LOAD("341-0748", 0x20000, 0x20000, CRC(d4c50550) SHA1(2784cdd7ac7094b3e494409db3e72b4e6d2d9e81)) /* Needs verification; 341-0748: IIgs ROM03 FE-FF */
ROM_END

ROM_START(apple2gsr3p)
	ROM_REGION(0x1000,"keyboard",0)
	ROM_LOAD( "341s0632-2.bin", 0x000000, 0x001000, CRC(e1c11fb0) SHA1(141d18c36a617ab9dce668445440d34354be0672) )

	ROM_REGION(0x1000,"gfx1",0)
	ROM_LOAD ( "apple2gs.chr", 0x0000, 0x1000, CRC(91e53cd8) SHA1(34e2443e2ef960a36c047a09ed5a93f471797f89)) /* need label/part number */

	ROM_REGION(0x40000,"maincpu",0)
	ROM_LOAD("341-0728", 0x0000, 0x20000, NO_DUMP) /* 341-0728: IIgs ROM03 prototype FC-FD */
	ROM_LOAD("341-0729", 0x20000, 0x20000, NO_DUMP) /* 341-0729: IIgs ROM03 prototype FE-FF */
ROM_END

ROM_START(apple2gsr3lp)
	ROM_REGION(0x1000,"keyboard",0)
	ROM_LOAD( "341s0632-2.bin", 0x000000, 0x001000, CRC(e1c11fb0) SHA1(141d18c36a617ab9dce668445440d34354be0672) )

	ROM_REGION(0x1000,"gfx1",0)
	ROM_LOAD ( "apple2gs.chr", 0x0000, 0x1000, CRC(91e53cd8) SHA1(34e2443e2ef960a36c047a09ed5a93f471797f89)) /* need label/part number */

	ROM_REGION(0x40000,"maincpu",0)
	ROM_LOAD("341-0737", 0x0000, 0x20000, CRC(8d410067) SHA1(c0f4704233ead14cb8e1e8a68fbd7063c56afd27)) /* 341-0737: IIgs ROM03 FC-FD */
	ROM_LOAD("341-0749", 0x20000, 0x20000, NO_DUMP) /* 341-0749: unknown ?post? ROM03 IIgs prototype? FE-FF */
ROM_END

ROM_START(apple2gsr1)
	ROM_REGION(0xc00,"keyboard",0)
	ROM_LOAD( "341s0345.bin", 0x000000, 0x000c00, CRC(48cd5779) SHA1(97e421f5247c00a0ca34cd08b6209df573101480) )

	ROM_REGION(0x1000,"gfx1",0)
	ROM_LOAD ( "apple2gs.chr", 0x0000, 0x1000, CRC(91e53cd8) SHA1(34e2443e2ef960a36c047a09ed5a93f471797f89)) /* need label/part number */

	ROM_REGION(0x20000,"maincpu",0)
	ROM_LOAD("342-0077-b", 0x0000, 0x20000, CRC(42f124b0) SHA1(e4fc7560b69d062cb2da5b1ffbe11cd1ca03cc37)) /* 342-0077-B: IIgs ROM01 */
ROM_END

ROM_START(apple2gsr0)
	ROM_REGION(0xc00,"keyboard",0)
	ROM_LOAD( "341s0345.bin", 0x000000, 0x000c00, CRC(48cd5779) SHA1(97e421f5247c00a0ca34cd08b6209df573101480) )

	ROM_REGION(0x1000,"gfx1",0)
	ROM_LOAD ( "apple2gs.chr", 0x0000, 0x1000, CRC(91e53cd8) SHA1(34e2443e2ef960a36c047a09ed5a93f471797f89))

	ROM_REGION(0x20000,"maincpu",0)
	/* Should these roms really be split like this? according to the unofficial apple rom list, IIgs ROM00 was on one rom labeled 342-0077-A */
	ROM_LOAD("rom0a.bin", 0x0000,  0x8000, CRC(9cc78238) SHA1(0ea82e10720a01b68722ab7d9f66efec672a44d3))
	ROM_LOAD("rom0b.bin", 0x8000,  0x8000, CRC(8baf2a79) SHA1(91beeb11827932fe10475252d8036a63a2edbb1c))
	ROM_LOAD("rom0c.bin", 0x10000, 0x8000, CRC(94c32caa) SHA1(4806d50d676b06f5213b181693fc1585956b98bb))
	ROM_LOAD("rom0d.bin", 0x18000, 0x8000, CRC(200a15b8) SHA1(0c2890bb169ead63369738bbd5f33b869f24c42a))
ROM_END

static DRIVER_INIT(apple2gs)
{
	apple2gs_state *state = machine.driver_data<apple2gs_state>();
	es5503_set_base(machine.device("es5503"), state->m_docram);
	state->save_item(NAME(state->m_docram));
}

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT       INIT      COMPANY            FULLNAME */
COMP( 1989, apple2gs, 0,        apple2, apple2gs, apple2gs,   apple2gs, "Apple Computer", "Apple IIgs (ROM03)", 0 )
COMP( 198?, apple2gsr3p, apple2gs, 0,   apple2gs, apple2gs,   apple2gs, "Apple Computer", "Apple IIgs (ROM03 prototype)", GAME_NOT_WORKING )
COMP( 1989, apple2gsr3lp, apple2gs, 0,  apple2gs, apple2gs,   apple2gs, "Apple Computer", "Apple IIgs (ROM03 late prototype?)", GAME_NOT_WORKING )
COMP( 1987, apple2gsr1, apple2gs, 0,    apple2gsr1, apple2gs,   apple2gs, "Apple Computer", "Apple IIgs (ROM01)", 0 )
COMP( 1986, apple2gsr0, apple2gs, 0,    apple2gsr1, apple2gs,   apple2gs, "Apple Computer", "Apple IIgs (ROM00)", 0 )
