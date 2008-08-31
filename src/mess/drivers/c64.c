/***************************************************************************
    commodore c64 home computer

    PeT mess@utanet.at

    documentation
     www.funet.fi
***************************************************************************/

/*
------------------------------------
max     commodore max (vic10/ultimax/vickie prototype)
c64     commodore c64 (ntsc version)
c64pal  commodore c64 (pal version)
c64gs   commodore c64 game system (ntsc version)
sx64    commodore sx64 (pal version)
------------------------------------
(preliminary version)

if the game runs to fast with the ntsc version, try the pal version!

c64
 design like the vic20
 better videochip with sprites
 famous sid6581 sound chip
 64 kbyte ram
 2nd gameport
Educator 64-1
 standard c64
 bios color bios (as in pet64 series) when delivered with green monitor
max  (vic10,ultimax,vickey prototype)
 delivered in japan only?
 (all modules should work with c64)
 cartridges neccessary
 low cost c64
 flat design
 only 4 kbyte sram
 simplier banking chip
  no portlines from cpu
 only 1 cia6526 chip
  restore key connection?
  no serial bus
  no userport
 keyboard
 tape port
 2 gameports
  lightpen (port a only) and joystick mentioned in advertisement
  paddles
 cartridge/expansion port (some signals different to c64)
 no rom on board (minibasic with kernel delivered as cartridge?)
c64gs
 game console without keyboard
 standard c64 mainboard!
 modified kernal
 basic rom
 2. cia yes
 no userport
 no cbm serial port
 no keyboard connector
 no tapeport
cbm4064/pet64/educator64-2
 build in green monitor
 other case
 differences, versions???
(sx100 sx64 like prototype with build in black/white monitor)
sx64
 movable compact (and heavy) all in one comp
 build in vc1541
 build in small color monitor
 no tape connector
dx64 prototype
 two build in vc1541 (or 2 drives driven by one vc1541 circuit)

state
-----
rasterline based video system
 no cpu holding
 imperfect scrolling support (when 40 columns or 25 lines)
 lightpen support not finished
 rasterline not finished
no sound
cia6526's look in machine/cia6526.c
keyboard
gameport a
 paddles 1,2
 joystick 1
 2 button joystick/mouse joystick emulation
 no mouse
 lightpen (not finished)
gameport b
 paddles 3,4
 joystick 2
 2 button joystick/mouse joystick emulation
 no mouse
simple tape support
 (not working, cia timing?)
serial bus
 simple disk drives
 no printer or other devices
expansion modules c64
 rom cartridges (exrom)
 ultimax rom cartridges (game)
 no other rom cartridges (bankswitching logic in it, switching exrom, game)
 no ieee488 support
 no cpm cartridge
 no speech cartridge (no circuit diagram found)
 no fm sound cartridge
 no other expansion modules
expansion modules ultimax
 ultimax rom cartridges
 no other expansion modules
no userport
 no rs232/v.24 interface
no super cpu modification
no second sid modification
quickloader

Keys
----
Some PC-Keyboards does not behave well when special two or more keys are
pressed at the same time
(with my keyboard printscreen clears the pressed pause key!)

shift-cbm switches between upper-only and normal character set
(when wrong characters on screen this can help)
run (shift-stop) loads pogram from type and starts it

Lightpen
--------
Paddle 5 x-axe
Paddle 6 y-axe

Tape
----
(DAC 1 volume in noise volume)
loading of wav, prg and prg files in zip archiv
commandline -cassette image
wav:
 8 or 16(not tested) bit, mono, 125000 Hz minimum
 has the same problems like an original tape drive (tone head must
 be adjusted to get working(no load error,...) wav-files)
zip:
 must be placed in current directory
 prg's are played in the order of the files in zip file

use LOAD or LOAD"" or LOAD"",1 for loading of normal programs
use LOAD"",1,1 for loading programs to their special address

several programs relies on more features
(loading other file types, writing, ...)

Discs
-----
only file load from drive 8 and 9 implemented
 loads file from rom directory (*.prg,*.p00,*.t64) (must NOT be specified on
 commandline)
 or file from d64 image (here also directory LOAD"$",8 supported)
use LOAD"filename",8
or LOAD"filename",8,1 (for loading machine language programs at their address)
for loading
type RUN or the appropriate sys call to start them

several programs rely on more features
(loading other file types, writing, ...)

most games rely on starting own programs in the floppy drive
(and therefor cpu level emulation is needed)

Roms
----
.prg
.crt
.80 .90 .a0 .b0 .e0 .f0
files with boot-sign in it
  recogniced as roms

.prg files loaded at address in its first two bytes
.?0 files to address specified in extension
.crt roms to addresses in crt file

Quickloader
-----------
.prg, .p00 and .t64 files supported (.t64s must be standard single entry type
 with a start address of $0801 - which covers most T64 files)
loads program into memory and sets program end pointer
(works with most programs)
program ready to get started with RUN
loads first rom when you press quickload key (f8)

when problems start with -log and look into error.log file
 */

#include "driver.h"
#include "sound/sid6581.h"
#include "machine/6526cia.h"
#include "devices/cassette.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "machine/cbmipt.h"
#include "video/vic6567.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"

#include "includes/c64.h"


static ADDRESS_MAP_START(ultimax_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_RAM AM_BASE(&c64_memory)
	AM_RANGE(0x8000, 0x9fff) AM_ROM AM_BASE(&c64_roml)
	AM_RANGE(0xd000, 0xd3ff) AM_READWRITE(vic2_port_r, vic2_port_w)
	AM_RANGE(0xd400, 0xd7ff) AM_READWRITE(sid6581_0_port_r, sid6581_0_port_w)
	AM_RANGE(0xd800, 0xdbff) AM_READWRITE(SMH_RAM, c64_colorram_write) AM_BASE(&c64_colorram) /* colorram  */
	AM_RANGE(0xdc00, 0xdcff) AM_READWRITE(cia_0_r, cia_0_w)
	AM_RANGE(0xe000, 0xffff) AM_ROM AM_BASE(&c64_romh)	   /* ram or kernel rom */
ADDRESS_MAP_END

static ADDRESS_MAP_START(c64_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x7fff) AM_RAM AM_BASE(&c64_memory)
	AM_RANGE(0x8000, 0x9fff) AM_READWRITE(SMH_BANK1, SMH_BANK2)		/* ram or external roml */
	AM_RANGE(0xa000, 0xbfff) AM_ROMBANK(3) AM_WRITEONLY				/* ram or basic rom or external romh */
	AM_RANGE(0xc000, 0xcfff) AM_RAM
	AM_RANGE(0xd000, 0xdfff) AM_READWRITE(c64_ioarea_r, c64_ioarea_w)
	AM_RANGE(0xe000, 0xffff) AM_READWRITE(SMH_BANK4, SMH_BANK5)	   /* ram or kernel rom or external romh */
ADDRESS_MAP_END


/*************************************
 *
 *  Input Ports
 *
 *************************************/

static INPUT_PORTS_START( c64 )
	PORT_INCLUDE( common_cbm_keyboard )		/* ROW0 -> ROW7 */

	PORT_INCLUDE( c64_special )				/* SPECIAL */

	PORT_INCLUDE( c64_controls )			/* JOY0, JOY1, PADDLE0 -> PADDLE3, TRACKX, TRACKY, TRACKIPT */

	PORT_INCLUDE( c64_control_cfg )			/* DSW0 */

	PORT_INCLUDE( c64_config )				/* CFG */
INPUT_PORTS_END


static INPUT_PORTS_START (ultimax)
	PORT_INCLUDE( c64 )

	PORT_MODIFY( "CFG" )
	PORT_BIT( 0x1c, 0x04, IPT_UNUSED )	/* only ultimax cartridges */
INPUT_PORTS_END

static INPUT_PORTS_START (c64gs)
	PORT_INCLUDE( c64 )
	
	/* 2008 FP: This has to be cleaned up later */
	/* C64gs should simply not scan these inputs */
	/* as a temporary solution, we keep PeT IPT_UNUSED shortcut */

	PORT_MODIFY( "ROW0" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW1" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW2" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW3" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW4" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW5" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW6" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW7" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "SPECIAL" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )

	PORT_MODIFY( "CFG" )
	PORT_BIT( 0xff00, 0x0000, IPT_UNUSED )
INPUT_PORTS_END


static INPUT_PORTS_START (sx64)
	PORT_INCLUDE( c64 )

	PORT_MODIFY( "CFG" )
	PORT_BIT( 0x7f00, 0x0000, IPT_UNUSED ) /* no tape */
INPUT_PORTS_END


static INPUT_PORTS_START (vic64s)
	PORT_INCLUDE( c64 )

	PORT_MODIFY( "ROW5" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xa5") PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('\xA5')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON)								PORT_CHAR(';') PORT_CHAR(']')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS)							PORT_CHAR('=')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS)								PORT_CHAR('-')
	PORT_MODIFY( "ROW6" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xa4") PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR('\xA4')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xb6") PORT_CODE(KEYCODE_QUOTE)		PORT_CHAR('\xB6')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE)						PORT_CHAR('@')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2)						PORT_CHAR(':') PORT_CHAR('*')
INPUT_PORTS_END

static INPUT_PORTS_START (vip64)
	PORT_INCLUDE( c64 )

	PORT_MODIFY( "ROW5" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xa5") PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('\xA5')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON)								PORT_CHAR(';') PORT_CHAR(']')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS)							PORT_CHAR('=')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS)								PORT_CHAR('-')
	PORT_MODIFY( "ROW6" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xa4") PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR('\xA4')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xb6") PORT_CODE(KEYCODE_QUOTE)		PORT_CHAR('\xB6')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE)						PORT_CHAR('@')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2)						PORT_CHAR(':') PORT_CHAR('*')
	PORT_MODIFY( "CFG" )
	PORT_BIT( 0x7f00, 0x00, IPT_UNUSED ) /* no tape */
INPUT_PORTS_END



static PALETTE_INIT( pet64 )
{
	int i;
	for (i=0; i<16; i++)
		palette_set_color_rgb(machine, i, 0, vic2_palette[i*3+1], 0);
}

ROM_START (ultimax)
	ROM_REGION (0x10000, "main", ROMREGION_ERASEFF)
ROM_END

ROM_START (c64gs)
	ROM_REGION (0x19400, "main", 0)
	/* standard basic, modified kernel */
	ROM_LOAD ("390852.01", 0x10000, 0x4000, CRC(b0a9c2da) SHA1(21940ef5f1bfe67d7537164f7ca130a1095b067a))
	ROM_LOAD ("901225.01", 0x14000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa))
ROM_END

ROM_START (c64)
	ROM_REGION (0x19400, "main", 0)
	ROM_LOAD ("901226.01", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e))
	ROM_LOAD ("901227.03", 0x12000, 0x2000, CRC(dbe3e7c7) SHA1(1d503e56df85a62fee696e7618dc5b4e781df1bb))
	ROM_LOAD ("901225.01", 0x14000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa))
ROM_END

ROM_START (c64pal)
	ROM_REGION (0x19400, "main", 0)
	ROM_LOAD ("901226.01", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e))
	ROM_LOAD ("901227.03", 0x12000, 0x2000, CRC(dbe3e7c7) SHA1(1d503e56df85a62fee696e7618dc5b4e781df1bb))
	ROM_LOAD ("901225.01", 0x14000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa))
ROM_END

ROM_START (vic64s)
	ROM_REGION (0x19400, "main", 0)
	ROM_LOAD ("901226.01",	0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e))
	ROM_LOAD ("kernel.swe",	0x12000, 0x2000, CRC(f10c2c25) SHA1(e4f52d9b36c030eb94524eb49f6f0774c1d02e5e))
	ROM_LOAD ("charswe.bin",0x14000, 0x1000, CRC(bee9b3fd) SHA1(446ae58f7110d74d434301491209299f66798d8a))
ROM_END

ROM_START (sx64)
	ROM_REGION (0x19400, "main", 0)
	ROM_LOAD ("901226.01", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e))
	ROM_LOAD( "251104.04", 0x12000, 0x2000, CRC(2c5965d4) SHA1(aa136e91ecf3c5ac64f696b3dbcbfc5ba0871c98))
	ROM_LOAD ("901225.01", 0x14000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa))
	VC1541_ROM ("cpu_vc1540")
ROM_END

ROM_START (dx64)
	ROM_REGION (0x19400, "main", 0)
    ROM_LOAD ("901226.01", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e))
    ROM_LOAD( "dx64kern.bin",     0x12000, 0x2000, CRC(58065128))
    // vc1541 roms were not included in submission
    VC1541_ROM ("cpu_vc1540")
	//    VC1541_ROM (" ")
ROM_END

ROM_START (vip64)
	ROM_REGION (0x19400, "main", 0)
	ROM_LOAD ("901226.01", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e))
	ROM_LOAD( "kernelsx.swe",   0x12000, 0x2000, CRC(7858d3d7) SHA1(097cda60469492a8916c2677b7cce4e12a944bc0))
	ROM_LOAD ("charswe.bin", 0x14000, 0x1000, CRC(bee9b3fd) SHA1(446ae58f7110d74d434301491209299f66798d8a))
	VC1541_ROM ("cpu_vc1540")
ROM_END

ROM_START (pet64)
	ROM_REGION (0x19400, "main", 0)
	ROM_LOAD ("901226.01", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e))
	ROM_LOAD( "901246.01", 0x12000, 0x2000, CRC(789c8cc5) SHA1(6c4fa9465f6091b174df27dfe679499df447503c))
	ROM_LOAD ("901225.01", 0x14000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa))
ROM_END

#if 0
ROM_START (flash8)
	ROM_REGION (0x1009400, "main", 0)
#if 1
    ROM_LOAD ("flash8", 0x010000, 0x002000, CRC(3c4fb703)) // basic
    ROM_CONTINUE( 0x014000, 0x001000) // empty
    ROM_CONTINUE( 0x014000, 0x001000) // characterset
    ROM_CONTINUE( 0x012000, 0x002000) // c64 mode kernel
    ROM_CONTINUE( 0x015000, 0x002000) // kernel
#else
	ROM_LOAD ("flash8", 0x012000-0x6000, 0x008000, CRC(3c4fb703))
#endif
ROM_END
#endif

#if 0
     /* character rom */
	 ROM_LOAD ("901225.01", 0x14000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa))
	 ROM_LOAD ("charswe.bin", 0x14000, 0x1000, CRC(bee9b3fd) SHA1(446ae58f7110d74d434301491209299f66798d8a))

	/* basic */
	 ROM_LOAD ("901226.01", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e))

/* in c16 and some other commodore machines:
   cbm version in kernel at 0xff80 (offset 0x3f80)
   0x80 means pal version */

	 /* scrap */
     /* modified for alec 64, not booting */
	 ROM_LOAD( "alec64.e0",   0x12000, 0x2000, CRC(2b1b7381 ))
     /* unique copyright, else speeddos? */
	 ROM_LOAD( "a.e0", 0x12000, 0x2000, CRC(b8f49365 ))
	 /* ? */
	 ROM_LOAD( "kernelx.e0",  0x12000, 0x2000, CRC(beed6d49 ))
	 ROM_LOAD( "kernelx2.e0",  0x12000, 0x2000, CRC(cfb58230 ))
	 /* basic x 2 */
	 ROM_LOAD( "frodo.e0",    0x12000, 0x2000, CRC(6ec94629 ))

     /* commodore versions */
	 /* 901227-01 */
	 ROM_LOAD( "901227.01",  0x12000, 0x2000, CRC(dce782fa ))
     /* 901227-02 */
	 ROM_LOAD( "901227.02", 0x12000, 0x2000, CRC(a5c687b3 ))
     /* 901227-03 */
	 ROM_LOAD( "901227.03",   0x12000, 0x2000, CRC(dbe3e7c7) SHA1(1d503e56df85a62fee696e7618dc5b4e781df1bb))
	 /* 901227-03? swedish  */
	 ROM_LOAD( "kernel.swe",   0x12000, 0x2000, CRC(f10c2c25) SHA1(e4f52d9b36c030eb94524eb49f6f0774c1d02e5e))
	 /* c64c 901225-01 + 901227-03 */
	 ROM_LOAD ("251913.01", 0x10000, 0x4000, CRC(0010ec31))
     /* c64gs 901225-01 with other fillbyte, modified kernel */
	 ROM_LOAD ("390852.01", 0x10000, 0x4000, CRC(b0a9c2da) SHA1(21940ef5f1bfe67d7537164f7ca130a1095b067a))
	 /* sx64 */
	 ROM_LOAD( "251104.04",     0x12000, 0x2000, CRC(2c5965d4 ))
     /* 251104.04? swedish */
	 ROM_LOAD( "kernel.swe",   0x12000, 0x2000, CRC(7858d3d7 ))
	 /* 4064, Pet64, Educator 64 */
	 ROM_LOAD( "901246.01",     0x12000, 0x2000, CRC(789c8cc5 ))

	 /* few differences to above versions */
	 ROM_LOAD( "901227.02b",  0x12000, 0x2000, CRC(f80eb87b ))
	 ROM_LOAD( "901227.03b",  0x12000, 0x2000, CRC(8e5c500d ))
	 ROM_LOAD( "901227.03c",  0x12000, 0x2000, CRC(c13310c2 ))

     /* 64er system v1
        ieee interface extension for c64 and vc1541!? */
     ROM_LOAD( "64ersys1.e0", 0x12000, 0x2000, CRC(97d9a4df ))
	 /* 64er system v3 */
	 ROM_LOAD( "64ersys3.e0", 0x12000, 0x2000, CRC(5096b3bd ))

	 /* exos v3 */
	 ROM_LOAD( "exosv3.e0",   0x12000, 0x2000, CRC(4e54d020 ))
     /* 2 bytes different */
	 ROM_LOAD( "exosv3.e0",   0x12000, 0x2000, CRC(26f3339e ))

	 /* jiffydos v6.01 by cmd */
	 ROM_LOAD( "jiffy.e0",    0x12000, 0x2000, CRC(2f79984c ))

	 /* dolphin with dolphin vc1541 */
	 ROM_LOAD( "mager.e0",    0x12000, 0x2000, CRC(c9bb21bc ))
	 ROM_LOAD( "dos20.e0",    0x12000, 0x2000, CRC(ffaeb9bc ))

	 /* speeddos plus
        parallel interface on userport to modified vc1541 !? */
	 ROM_LOAD( "speeddos.e0", 0x12000, 0x2000, CRC(8438e77b ))
	 /* speeddos plus + */
	 ROM_LOAD( "speeddos.e0", 0x12000, 0x2000, CRC(10aee0ae ))
	 /* speeddos plus and 80 column text */
	 ROM_LOAD( "rom80.e0",    0x12000, 0x2000, CRC(e801dadc ))
#endif

static const sid6581_interface c64_sound_interface =
{
	c64_paddle_read
};

static MACHINE_DRIVER_START( c64 )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", M6510, VIC6567_CLOCK)
	MDRV_CPU_PROGRAM_MAP(c64_mem, 0)
	MDRV_CPU_VBLANK_INT("main", c64_frame_interrupt)
	MDRV_CPU_PERIODIC_INT(vic2_raster_irq, VIC2_HRETRACERATE)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START( c64 )

	/* video hardware */
	MDRV_IMPORT_FROM( vh_vic2 )
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(VIC6567_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("sid", SID6581, VIC6567_CLOCK)
	MDRV_SOUND_CONFIG(c64_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_QUICKLOAD_ADD(cbm_c64, "p00,prg,t64", CBM_QUICKLOAD_DELAY_SECONDS)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ultimax )
	MDRV_IMPORT_FROM( c64 )
	MDRV_CPU_REPLACE( "main", M6510, 1000000)
	MDRV_CPU_PROGRAM_MAP( ultimax_mem, 0 )

	MDRV_SOUND_REPLACE("sid", SID6581, 1000000)
	MDRV_SOUND_CONFIG(c64_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pet64 )
	MDRV_IMPORT_FROM( c64 )
	MDRV_PALETTE_INIT( pet64 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( c64pal )
	MDRV_IMPORT_FROM( c64 )
	MDRV_CPU_REPLACE( "main", M6510, VIC6569_CLOCK)

	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(VIC6569_VRETRACERATE)

	MDRV_SOUND_REPLACE("sid", SID6581, VIC6569_CLOCK)
	MDRV_SOUND_CONFIG(c64_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( c64gs )
	MDRV_IMPORT_FROM( c64pal )
	MDRV_SOUND_REMOVE( "dac" )
	MDRV_QUICKLOAD_REMOVE
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sx64 )
	MDRV_IMPORT_FROM( c64pal )
	MDRV_IMPORT_FROM( cpu_vc1541 )
	MDRV_SOUND_REMOVE( "dac" )
#ifdef CPU_SYNC
	MDRV_INTERLEAVE(1)
#else
	MDRV_INTERLEAVE(3000)
#endif
MACHINE_DRIVER_END

#define rom_max rom_ultimax
#define rom_cbm4064 rom_pet64

static void c64_cbmcartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "crt,80"); break;

		default:										cbmcartslot_device_getinfo(devclass, state, info); break;
	}
}

static void ultimax_cbmcartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "crt,e0,f0"); break;

		default:										cbmcartslot_device_getinfo(devclass, state, info); break;
	}
}

static void datasette_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
	case MESS_DEVINFO_INT_COUNT:					info->i = 1; break;

	case MESS_DEVINFO_INT_CASSETTE_DEFAULT_STATE:	info->i = CASSETTE_STOPPED | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED; break;

	default:										cassette_device_getinfo( devclass, state, info ); break;
	}
}

static SYSTEM_CONFIG_START(c64)
	CONFIG_DEVICE(c64_cbmcartslot_getinfo)
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
	CONFIG_DEVICE(datasette_device_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(sx64)
	CONFIG_DEVICE(c64_cbmcartslot_getinfo)
	CONFIG_DEVICE(vc1541_device_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(ultimax)
	CONFIG_DEVICE(ultimax_cbmcartslot_getinfo)
	CONFIG_DEVICE(datasette_device_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(c64gs)
	CONFIG_DEVICE(c64_cbmcartslot_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*   YEAR  NAME     PARENT  COMPAT  MACHINE         INPUT   INIT    CONFIG      COMPANY                            FULLNAME */
COMP(1982, c64,		0,		0,		c64,			c64,	c64,	c64,		"Commodore Business Machines Co.", "Commodore 64 (NTSC)", 0)
COMP(1982, c64pal, 	c64,	0,		c64pal, 		c64,	c64pal, c64,		"Commodore Business Machines Co.", "Commodore 64/VC64/VIC64 (PAL)", 0)
COMP(1982, cbm4064,	c64,	0,		pet64,			c64,	c64,	c64,		"Commodore Business Machines Co.", "CBM4064/PET64/Educator64 (NTSC)", 0)
COMP(1982, vic64s, 	c64,	0,		c64pal, 		vic64s,	c64pal, c64,		"Commodore Business Machines Co.", "Commodore 64 Swedish (PAL)", 0)
COMP(1982, max,		0,		0,		ultimax,		ultimax,ultimax,ultimax,	"Commodore Business Machines Co.", "Commodore Max (Ultimax/VC10)", 0)
CONS(1987, c64gs,	c64,	0,		c64gs,			c64gs,	c64gs,	c64gs,		"Commodore Business Machines Co.", "C64GS (PAL)", 0)

/* testdrivers */
COMP(1983, sx64,	c64,	0,		sx64,			sx64,	sx64,	sx64,		"Commodore Business Machines Co.", "SX64 (PAL)",                      GAME_NOT_WORKING)
COMP(1983, vip64,	c64,	0,		sx64,			vip64,	sx64,	sx64,		"Commodore Business Machines Co.", "VIP64 (SX64 PAL), Swedish Expansion Kit", GAME_NOT_WORKING)
// sx64 with second disk drive
COMP(198?, dx64,	c64,	0,		sx64,			sx64,	sx64,	sx64,		"Commodore Business Machines Co.", "DX64 (Prototype, PAL)",                      GAME_NOT_WORKING)
/*c64 II (cbm named it still c64) */
/*c64c (bios in 1 chip) */
/*c64g late 8500/8580 based c64, sold at aldi/germany */
/*c64cgs late c64, sold in ireland, gs bios?, but with keyboard */
