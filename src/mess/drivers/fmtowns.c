/*

	Fujitsu FM-Towns
	
	Japanese computer system released in 1989.
	
	CPU:  AMD 80386SX (80387 available as add-on)
	Sound:  Yamaha YM3438
	        Ricoh RF5c68
	Video:  VGA + some custom extra video hardware
	        320x200 - 640x480
	        16 - 32768 colours from a possible palette of between 4096 and
	          16.7m colours (depending on video mode)
	        1024 sprites (16x16)
	        
	        
	Fujitsu FM-Towns Marty
	
	Japanese console, based on the FM-Towns computer, released in 1993 


	16/5/09:  Skeleton driver.

*/

#include "driver.h"
#include "cpu/i386/i386.h"
#include "sound/2612intf.h"
#include "sound/rf5c68.h"

static ADDRESS_MAP_START(towns_mem, ADDRESS_SPACE_PROGRAM, 32)
  // memory map based on FM-Towns/Bochs (Bochs modified to emulate the FM-Towns)
  // may not be (and probably is not) correct
  AM_RANGE(0x000c0000, 0x000c7fff) AM_NOP  // GVRAM
  AM_RANGE(0x000c8000, 0x000cffff) AM_NOP  // TVRAM
  AM_RANGE(0x000d0000, 0x000d7fff) AM_ROM AM_REGION("user",0x100000)  // DIC ROM
  AM_RANGE(0x000d8000, 0x000d9fff) AM_NOP  // CMOS RAM
  AM_RANGE(0x000f8000, 0x000fffff) AM_ROM AM_REGION("user",0x238000)  // BOOT (SYSTEM) ROM
  AM_RANGE(0x80000000, 0x8007ffff) AM_NOP  // VRAM
  AM_RANGE(0x80100000, 0x8017ffff) AM_NOP  // VRAM (mirror? second page?)
  AM_RANGE(0x81000000, 0x8101ffff) AM_NOP  // Sprite RAM
  AM_RANGE(0xc2000000, 0xc207ffff) AM_ROM AM_REGION("user",0x000000)  // OS ROM
  AM_RANGE(0xc2080000, 0xc20fffff) AM_ROM AM_REGION("user",0x100000)  // DIC ROM
  AM_RANGE(0xc2100000, 0xc213ffff) AM_ROM AM_REGION("user",0x180000)  // FONT ROM
  AM_RANGE(0xc2140000, 0xc2141fff) AM_NOP  // CMOS (mirror?)
  AM_RANGE(0xc2200000, 0xc2200fff) AM_NOP  // WAVE RAM
  AM_RANGE(0xfffc0000, 0xffffffff) AM_ROM AM_REGION("user",0x200000)  // SYSTEM ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( towns_io , ADDRESS_SPACE_IO, 32)
  // I/O ports derived from FM Towns/Bochs, these are specific to the FM Towns
  // Some common PC ports are likely to also be used
  // System ports
  AM_RANGE(0x0020,0x0033) AM_NOP  // R/W
  AM_RANGE(0x006c,0x006f) AM_NOP  // R/W (0x6c)
  // DMA controllers (uPD71071?)
  AM_RANGE(0x00a0,0x00af) AM_NOP
  AM_RANGE(0x00b0,0x00bf) AM_NOP
  // Floppy controller
  AM_RANGE(0x0200,0x020f) AM_NOP  // all R/W
  // CRTC / Video
  AM_RANGE(0x0400,0x0403) AM_NOP  // R/O (0x400)
  AM_RANGE(0x0404,0x0407) AM_NOP  // R/W (0x404)
  AM_RANGE(0x0440,0x045f) AM_NOP
  // System port
  AM_RANGE(0x0480,0x0483) AM_NOP  // R/W (0x480)
  // CD-ROM
  AM_RANGE(0x04c0,0x04cf) AM_NOP
  // Sound (YM3438 [FM], RF5c68 [PCM])
  AM_RANGE(0x04d4,0x04d7) AM_NOP  // R/W  -- (0x4d5) mute?
  AM_RANGE(0x04d8,0x04db) AM_NOP  // R/W  -- (0x4d8) YM control port A / status port A
                                  // R/W  -- (0x4da) YM data port A / status port A
  AM_RANGE(0x04dc,0x04df) AM_NOP  // R/W  -- (0x4dc) YM control port B / status port A
                                  // R/W  -- (0x4de) YM data port B / status port A
  AM_RANGE(0x04e0,0x04e3) AM_NOP  // R/W  -- volume ports
  AM_RANGE(0x04e8,0x04ef) AM_NOP  // R/O  -- (0x4e9) FM IRQ flag (bit 0), PCM IRQ flag (bit 3)
                                  // (0x4ea) PCM IRQ mask
                                  // R/W  -- (0x4eb) PCM IRQ flag
                                  // W/O  -- (0x4ec) LED control
  AM_RANGE(0x04f0,0x04fb) AM_NOP  // W/O  -- (0x4f0-0x4f8) PCM registers
  // System ports
  AM_RANGE(0x05e8,0x05ef) AM_NOP  // R/O (0x5e8, 0x5ec)

ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( towns )
INPUT_PORTS_END


static MACHINE_RESET( towns )
{
}

static VIDEO_START( towns )
{
}

static VIDEO_UPDATE( towns )
{
    return 0;
}

static MACHINE_DRIVER_START( towns )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I386, 16000000)
    MDRV_CPU_PROGRAM_MAP(towns_mem)
    MDRV_CPU_IO_MAP(towns_io)

    MDRV_MACHINE_RESET(towns)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    
    /* sound hardware */
    MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("fm", YM3438, 4000000) // actual clock speed unknown
//	MDRV_SOUND_CONFIG(ym3438_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD("pcm", RF5C68, 4000000)  // actual clock speed unknown
//	MDRV_SOUND_CONFIG(rf5c68_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
    
    MDRV_VIDEO_START(towns)
    MDRV_VIDEO_UPDATE(towns)
MACHINE_DRIVER_END

/* ROM definitions */
ROM_START( fmtowns )
  ROM_REGION32_LE( 0x280000, "user", 0)
	ROM_LOAD("fmt_dos.rom",  0x000000, 0x080000, CRC(112872ee) SHA1(57fd146478226f7f215caf63154c763a6d52165e) )
	ROM_LOAD("fmt_f20.rom",  0x080000, 0x080000, CRC(9f55a20c) SHA1(1920711cb66340bb741a760de187de2f76040b8c) )
	ROM_LOAD("fmt_dic.rom",  0x100000, 0x080000, CRC(82d1daa2) SHA1(7564020dba71deee27184824b84dbbbb7c72aa4e) )
	ROM_LOAD("fmt_fnt.rom",  0x180000, 0x040000, CRC(dd6fd544) SHA1(a216482ea3162f348fcf77fea78e0b2e4288091a) )
	ROM_LOAD("fmt_sys.rom",  0x200000, 0x040000, CRC(afe4ebcf) SHA1(4cd51de4fca9bd7a3d91d09ad636fa6b47a41df5) )
ROM_END

ROM_START( fmtownsa )
  ROM_REGION32_LE( 0x280000, "user", 0)
	ROM_LOAD("fmt_dos.rom",  0x000000, 0x080000, CRC(22270e9f) SHA1(a7e97b25ff72b14121146137db8b45d6c66af2ae) )
	ROM_LOAD("fmt_f20.rom",  0x080000, 0x080000, CRC(75660aac) SHA1(6a521e1d2a632c26e53b83d2cc4b0edecfc1e68c) )
	ROM_LOAD("fmt_dic.rom",  0x100000, 0x080000, CRC(74b1d152) SHA1(f63602a1bd67c2ad63122bfb4ffdaf483510f6a8) )
	ROM_LOAD("fmt_fnt.rom",  0x180000, 0x040000, CRC(0108a090) SHA1(1b5dd9d342a96b8e64070a22c3a158ca419894e1) )
	ROM_LOAD("fmt_sys.rom",  0x200000, 0x040000, CRC(92f3fa67) SHA1(be21404098b23465d24c4201a81c96ac01aff7ab) )
ROM_END

ROM_START( fmtmarty )
  ROM_REGION32_LE( 0x400000, "user", 0)
	ROM_LOAD("mrom.m36",  0x000000, 0x200000, CRC(9c0c060c) SHA1(5721c5f9657c570638352fa9acac57fa8d0b94bd) )
	ROM_LOAD("mrom.m37",  0x200000, 0x200000, CRC(fb66bb56) SHA1(e273b5fa618373bdf7536495cd53c8aac1cce9a5) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT    INIT    CONFIG COMPANY      FULLNAME            FLAGS */
COMP( 1989, fmtowns,  0,    0, 		towns, 		towns, 	 0,  	 0,  	"Fujitsu",   "FM-Towns",		 GAME_NOT_WORKING)
COMP( 1989, fmtownsa, fmtowns, 0, 	towns, 		towns, 	 0,  	 0,  	"Fujitsu",   "FM-Towns (alternate)", GAME_NOT_WORKING)
CONS( 1993, fmtmarty, 0,    0, 		towns, 		towns, 	 0,  	 0,  	"Fujitsu",   "FM-Towns Marty",	 GAME_NOT_WORKING)

