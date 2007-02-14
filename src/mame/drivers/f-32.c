/********************************************************************

 F-E1-32 driver


 Supported Games      PCB-ID
 ----------------------------------
 Mosaic               F-E1-32-009
 Wyvern Wings         F-E1-32-010-D

 driver by Pierpaolo Prazzoli

*********************************************************************/

#include "driver.h"
#include "machine/eeprom.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"

static UINT32 *vram;

static READ32_HANDLER( oki_32bit_r )
{
	return OKIM6295_status_0_r(0);
}

static WRITE32_HANDLER( oki_32bit_w )
{
	OKIM6295_data_0_w(0, data & 0xff);
}

static READ32_HANDLER( ym2151_status_32bit_r )
{
	return YM2151_status_port_0_r(0);
}

static WRITE32_HANDLER( ym2151_data_32bit_w )
{
	YM2151_data_port_0_w(0, data & 0xff);
}

static WRITE32_HANDLER( ym2151_register_32bit_w )
{
	YM2151_register_port_0_w(0,data & 0xff);
}

static WRITE32_HANDLER( vram_w )
{
	int x,y;

	COMBINE_DATA(&vram[offset]);

	y = offset >> 8;
	x = offset & 0xff;

	if(x < 320/2 && y < 224)
	{
		plot_pixel(tmpbitmap,x*2+0,y,(vram[offset] >> 16) & 0x7fff);
		plot_pixel(tmpbitmap,x*2+1,y,vram[offset] & 0x7fff);
	}
}

static READ32_HANDLER( eeprom_r )
{
	return EEPROM_read_bit();
}

static WRITE32_HANDLER( eeprom_bit_w )
{
	EEPROM_write_bit(data & 0x01);
}

static WRITE32_HANDLER( eeprom_cs_line_w )
{
	EEPROM_set_cs_line( (data & 0x01) ? CLEAR_LINE : ASSERT_LINE );
}

static WRITE32_HANDLER( eeprom_clock_line_w )
{
	EEPROM_set_clock_line( (~data & 0x01) ? ASSERT_LINE : CLEAR_LINE );
}

static WRITE32_HANDLER( wyvernwg_eeprom_w )
{
	//check
	EEPROM_write_bit(data & 0x01);
	EEPROM_set_cs_line((data & 0x04) ? CLEAR_LINE : ASSERT_LINE );
	EEPROM_set_clock_line((data & 0x02) ? ASSERT_LINE : CLEAR_LINE );

	// data & 8? -> video disabled?
}


static ADDRESS_MAP_START( common_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM
	AM_RANGE(0x40000000, 0x4003ffff) AM_WRITE(vram_w) AM_BASE(&vram)
	AM_RANGE(0x80000000, 0x80ffffff) AM_ROM AM_REGION(REGION_USER2,0)
	AM_RANGE(0xfff00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1,0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mosaicf2_io, ADDRESS_SPACE_IO, 32 )
	AM_RANGE(0x4000, 0x4003) AM_READ(oki_32bit_r)
	AM_RANGE(0x4810, 0x4813) AM_READ(ym2151_status_32bit_r)
	AM_RANGE(0x5000, 0x5003) AM_READ(input_port_0_dword_r)
	AM_RANGE(0x5200, 0x5203) AM_READ(input_port_1_dword_r)
	AM_RANGE(0x5400, 0x5403) AM_READ(eeprom_r)
	AM_RANGE(0x6000, 0x6003) AM_WRITE(oki_32bit_w)
	AM_RANGE(0x6800, 0x6803) AM_WRITE(ym2151_data_32bit_w)
	AM_RANGE(0x6810, 0x6813) AM_WRITE(ym2151_register_32bit_w)
	AM_RANGE(0x7000, 0x7003) AM_WRITE(eeprom_clock_line_w)
	AM_RANGE(0x7200, 0x7203) AM_WRITE(eeprom_cs_line_w)
	AM_RANGE(0x7400, 0x7403) AM_WRITE(eeprom_bit_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( wyvernwg_io, ADDRESS_SPACE_IO, 32 )
	AM_RANGE(0x2800, 0x2803) AM_READ(input_port_0_dword_r)
	AM_RANGE(0x3000, 0x3003) AM_READ(input_port_1_dword_r)
	AM_RANGE(0x7000, 0x7003) AM_WRITE(wyvernwg_eeprom_w)
	AM_RANGE(0x7c00, 0x7c03) AM_READ(eeprom_r)
ADDRESS_MAP_END

INPUT_PORTS_START( mosaicf2 )
	PORT_START
	PORT_BIT( 0x0000ffff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(1)
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(1)
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(1)
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x00400000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xff000000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x000000ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x00000200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE( 0x00000400, IP_ACTIVE_LOW )
	PORT_BIT( 0x00007800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00008000, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(2)
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(2)
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(2)
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x00400000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xff000000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( wyvernwg )
	PORT_START
	PORT_BIT( 0xffffffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xffffffff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static MACHINE_DRIVER_START( mosaicf2 )
	MDRV_CPU_ADD_TAG("main", E132XT, 20000000) // E132XN actually! /* 20 MHz */
	MDRV_CPU_PROGRAM_MAP(common_map,0)
	MDRV_CPU_IO_MAP(mosaicf2_io,0)
	MDRV_CPU_VBLANK_INT(irq5_line_pulse, 1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB15)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 0, 223)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 14318180/4)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 1789772.5)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1_pin7high)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( wyvernwg )
	MDRV_CPU_ADD_TAG("main", E132XT, 50000000) // E132T actually! /* 50 MHz */
	MDRV_CPU_PROGRAM_MAP(common_map,0)
	MDRV_CPU_IO_MAP(wyvernwg_io,0)
	MDRV_CPU_VBLANK_INT(irq5_line_pulse, 1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB15)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 0, 223)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	/* sound hardware */
	// TODO
MACHINE_DRIVER_END


/*

Mosaic (c) 1999 F2 System

CPU: Hyperstone E1-32XN
 Video: QuickLogic QL2003-XPL84C
 Sound: OKI 6295, BS901 (YM2151) & BS902 (YM3012)
   OSC: 20MHz & 14.31818MHz
EEPROM: 93C46

F-E1-32-009
+------------------------------------------------------------------+
|            VOL                               +---------+         |
+-+                              YM3812        |   SND   |         |
  |                                            +---------+         |
+-+                              YM2151            OKI6295         |
|                                                                  |
|                                   +---------------+              |
|                                   |               |              |
|J                   +-------+      |               |              |
|A                   | VRAML |      | QuickLogic    |  14.31818MHz |
|M                   +-------+      | QL2003-XPL84C |              |
|M                   +-------+      | 9819 BA       |   +-----+    |
|A                   | VRAMU |      |               |   |93C46|    |
|                    +-------+      +---------------+   +-----+    |
|C                                                                 |
|O                                      +---------+   +---------+  |
|N                                      |   L00   |   |   U00   |  |
|N                                      |         |   |         |  |
|E                                      +---------+   +---------+  |
|C                   +------------+     +---------+   +---------+  |
|T                   |            |     |   L01   |   |   U01   |  |
|O                   |            |     |         |   |         |  |
|R                   | HyperStone |     +---------+   +---------+  |
|                    |  E1-32XN   |     +---------+   +---------+  |
|                    |            |     |   L02   |   |   U02   |  |
|          +-----+   |            |     |         |   |         |  |
|          |DRAML|   +------------+     +---------+   +---------+  |
+-+        +-----+                      +---------+   +---------+  |
  |        +-----+               20MHz  |   L03   |   |   U03   |  |
+-+        |DRAMU|                      |         |   |         |  |
|          +-----+    +----------+      +---------+   +---------+  |
|  +--+ +--+          |   ROM1   |                                 |
|  |S3| |S1|          +----------+                                 |
+------------------------------------------------------------------+

S3 is a reset button
S1 is the setup button

VRAML & VRAMU are KM6161002CJ-12
DRAML & DRAMU are GM71C18163CJ6

ROM1 & SND are stardard 27C040 and/or 27C020 eproms
L00-L03 & U00-U03 are 29F1610ML Flash roms
*/

ROM_START( mosaicf2 )
	ROM_REGION32_BE( 0x100000, REGION_USER1, ROMREGION_ERASE00 ) /* Hyperstone CPU Code */
	/* 0 - 0x80000 empty */
	ROM_LOAD( "rom1.bin",            0x80000, 0x080000, CRC(fceb6f83) SHA1(b98afb477627c3b2d584c0f0fb26c4dd5b1a31e2) )

	ROM_REGION32_BE( 0x1000000, REGION_USER2, 0 )  /* gfx data */
	ROM_LOAD32_WORD_SWAP( "u00.bin", 0x000000, 0x200000, CRC(a2329675) SHA1(bff8974fab9120274821c9c9646744317f47c79c) )
	ROM_LOAD32_WORD_SWAP( "l00.bin", 0x000002, 0x200000, CRC(d96fe93b) SHA1(005d9889077825fc0e308d2981f6fca5e6b51fe8) )
	ROM_LOAD32_WORD_SWAP( "u01.bin", 0x400000, 0x200000, CRC(6379e73f) SHA1(fe5abafbcbd828795cb06a08763fae1bbe2a75ad) )
	ROM_LOAD32_WORD_SWAP( "l01.bin", 0x400002, 0x200000, CRC(a269ea82) SHA1(d962a8b3293c6f46dbefa49859b2b3e594e7a386) )
	ROM_LOAD32_WORD_SWAP( "u02.bin", 0x800000, 0x200000, CRC(c17f95cd) SHA1(1c701185be138b615d2851866288647f40809c28) )
	ROM_LOAD32_WORD_SWAP( "l02.bin", 0x800002, 0x200000, CRC(69cd9c5c) SHA1(6b4d204a6ab5f36dfba9053bb3be2d094fcfdd00) )
	ROM_LOAD32_WORD_SWAP( "u03.bin", 0xc00000, 0x200000, CRC(0e47df20) SHA1(6f6c3e7fc8c99db7ddc73d8d10a661373bb72a1a) )
	ROM_LOAD32_WORD_SWAP( "l03.bin", 0xc00002, 0x200000, CRC(d79f6ca8) SHA1(4735dda9269aa05ba1251d335dc73914f5cb43b0) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* Oki Samples */
	ROM_LOAD( "snd.bin",             0x000000, 0x040000, CRC(4584589c) SHA1(5f9824724f840767c3dc1dc04b203ddf3d78b84c) )
ROM_END

/*

Wyvern Wings (c) 2001 Semicom, Game Vision License


   CPU: Hyperstone E1-32T
 Video: 2 QuickLogic QL12x16B-XPL84 FPGA
 Sound: AdMOS QDSP1000 with QDSP QS1001A sample rom
   OSC: 50MHz, 28MHz & 24MHz
EEPROM: 93C46

F-E1-32-010-D
+------------------------------------------------------------------+
|    VOL    +-------+  +---------+                                 |
+-+         | QPSD  |  |  U15A   |      +---------+   +---------+  |
  |         |QS1001A|  |         |      | ROMH00  |   | ROML00  |  |
+-+         +-------+  +---------+      |         |   |         |  |
|           +-------+                   +---------+   +---------+  |
|           |QPSD   |   +----------+    +---------+   +---------+  |
|           |QS1000 |   |    U7    |    | ROMH01  |   | ROML01  |  |
|J   24MHz  +-------+   +----------+    |         |   |         |  |
|A                                      +---------+   +---------+  |
|M   50MHz           +-----+            +---------+   +---------+  |
|M                   |DRAM2|            | ROMH02  |   | ROML02  |  |
|A     +----------+  +-----+    +-----+ |         |   |         |  |
|      |          |  +-----+    |93C46| +---------+   +---------+  |
|C     |HyperStone|  |DRAM1|    +-----+ +---------+   +---------+  |
|O     |  E1-32T  |  +-----+            | ROMH03  |   | ROML03  |  |
|N     |          |              28MHz  |         |   |         |  |
|N     +----------+                     +---------+   +---------+  |
|E                                                                 |
|C           +----------+           +------------+ +------------+  |
|T           |   GAL1   |           | QuickLogic | | QuickLogic |  |
|O           +----------+           | 0048 BH    | | 0048 BH    |  |
|R           +----------+           | QL12X16B   | | QL12X16B   |  |
|            |   ROM2   |           | -XPL84C    | | -XPL84C    |  |
|            +----------+           +------------+ +------------+  |
|            +----------+            +----+                        |
|            |   ROM1   |            |MEM3|                        |
+-++--+      +----------+            +----+                        |
  ||S1|    +-----+                   |MEM2|                        |
+-++--+    |CRAM2|                   +----+                        |
|  +--+    +-----+                   |MEM7|                        |
|  |S2|    |CRAM1|                   +----+                        |
|  +--+    +-----+                   |MEM6|                        |
+------------------------------------+----+------------------------+

S1 is the setup button
S2 is a reset button

ROMH & ROML are all MX 29F1610MC-16 flash roms
u15A is a MX 29F1610MC-16 flash rom
u7 is a ST 27c1001
ROM1 & ROM2 are both ST 27c4000D
*/

ROM_START( wyvernwg )
	ROM_REGION32_BE( 0x100000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD( "rom1.bin", 0x000000, 0x080000, CRC(66bf3a5c) SHA1(037d5e7a6ef6f5b4ac08a9c811498c668a9d2522) )
	ROM_LOAD( "rom2.bin", 0x080000, 0x080000, CRC(fd9b5911) SHA1(a01e8c6e5a9009024af385268ba3ba90e1ebec50) )

	ROM_REGION32_BE( 0x1000000, REGION_USER2, 0 )  /* gfx data */
	ROM_LOAD32_WORD_SWAP( "romh00.bin", 0x000000, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD_SWAP( "roml00.bin", 0x000002, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD_SWAP( "romh01.bin", 0x400000, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD_SWAP( "roml01.bin", 0x400002, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD_SWAP( "romh02.bin", 0x800000, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD_SWAP( "roml02.bin", 0x800002, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD_SWAP( "romh03.bin", 0xc00000, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD_SWAP( "roml03.bin", 0xc00002, 0x200000, NO_DUMP )

	ROM_REGION( 0x020000, REGION_CPU2, 0 ) /* QDSP ('51) Code */
	ROM_LOAD( "u7", 0x0000, 0x20000, CRC(00a3f705) SHA1(f0a6bafd16bea53d4c05c8cc108983cbd41e5757) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* Music data / QDSP samples (SFX)?? */
	ROM_LOAD( "u15a", 0x00000, 0x200000, NO_DUMP )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_ERASEFF ) /* QDSP wavetable rom */
	ROM_LOAD( "qs1001a",  0x000000, 0x80000, CRC(d13c6407) SHA1(57b14f97c7d4f9b5d9745d3571a0b7115fbe3176) )
ROM_END




GAME( 1999, mosaicf2, 0, mosaicf2, mosaicf2, 0, ROT0, "F2 System", "Mosaic (F2 System)", GAME_SUPPORTS_SAVE )
GAME( 2001, wyvernwg, 0, wyvernwg, wyvernwg, 0, ROT0, "Semicom (Game Vision License)", "Wyvern Wings", GAME_NOT_WORKING|GAME_NO_SOUND )
