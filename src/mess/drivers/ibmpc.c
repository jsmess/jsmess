/***************************************************************************

    drivers/pc.c

Driver file for IBM PC, IBM PC XT, and related machines.

    PC-XT memory map

    00000-9FFFF   RAM
    A0000-AFFFF   NOP       or videoram EGA/VGA
    B0000-B7FFF   videoram  MDA, page #0
    B8000-BFFFF   videoram  CGA and/or MDA page #1, T1T mapped RAM
    C0000-C7FFF   NOP       or ROM EGA/VGA
    C8000-C9FFF   ROM       XT HDC #1
    CA000-CBFFF   ROM       XT HDC #2
    D0000-EFFFF   NOP       or 'adapter RAM'
    F0000-FDFFF   NOP       or ROM Basic + other Extensions
    FE000-FFFFF   ROM

IBM PC 5150
===========

- Intel 8088 at 4.77 MHz derived from a 14.31818 MHz crystal
- Onboard RAM: Revision A mainboard - min. 16KB, max. 64KB
               Revision B mainboard - min. 64KB, max. 256KB
- Total RAM (using 5161 expansion chassis or ISA memory board): 512KB (rev 1 or rev 2 bios) or 640KB (rev 3 bios)
- Graphics: MDA, CGA, or MDA and CGA
- Cassette port
- Five ISA expansion slots (short type)
- PC/XT keyboard 83-key 'IBM Model F' (some early 'IBM Model M' keyboards can produce scancodes compatible with this as well, but it was an undocumented feature unsupported by IBM)
- Optional 8087 co-processor
- Optional up to 4 (2 internal, 2 external) 160KB single-sided or 360KB double-sided 5 1/4" floppy drives
- Optional 10MB hard disk (using 5161 expansion chassis)
- Optional Game port joystick ISA card (two analog joysticks with 2 buttons each)
- Optional Parallel port ISA card, Unidirectional (upgradable to bidirectional (as is on the ps/2) with some minor hacking; IBM had all the circuitry there but it wasn't hooked up properly)
- Optional Serial port ISA card


IBM PC-JR
=========


IBM PC-XT 5160
==============

- Intel 8088 at 4.77 MHz derived from a 14.31818 MHz crystal
- Onboard RAM: Revision A mainboard (P/N 6181655) - min. 64KB, max. 256KB (4x 64KB banks of 9 IMS2600 64kx1 drams plus parity); could be upgraded to 640KB with a hack; may support the weird 256k stacked-chip dram the AT rev 1 uses)
               Revision B mainboard - min. 256KB, max 640KB (2x 256KB, 2x 64KB banks)
- Total RAM (using 5161 expansion chassis, ISA memory board, or rev B mainboard): 640KB
- Graphics: MDA, CGA, MDA and CGA, EGA, EGA and MDA, or EGA and CGA
- One internal 360KB double-sided 5 1/4" floppy drive
- PC/XT keyboard 83-key 'IBM Model F' (BIOS revisions 1 and 2) (some early 'IBM Model M' keyboards can produce scancodes compatible with this as well, but it was an undocumented feature unsupported by IBM)
- 84-key 'AT/Enhanced' keyboard or 101-key 'IBM Model M' (BIOS revisions 3 and 4)
- Eight ISA expansion slots (short type)
- Optional 8087 co-processor
- Optional second internal 360KB double-sided 5 1/4" floppy drive, if no internal hard disk
- Optional 'half height' 720KB double density 3 1/2" floppy drive
- Optional 10MB hard disk via 5161 expansion chassis
- Optional 10MB or 20MB Seagate ST-412 hard disk (in place of second floppy drive)
- Optional up to 2 external 360KB double-sided 5 1/4" floppy drive
- Optional Game port joystick ISA card (two analog joysticks with 2 buttons each)
- Optional Parallel port ISA card, Unidirectional (upgradable to bidirectional (as is on the ps/2) with some minor hacking; IBM had all the circuitry there but it wasn't hooked up properly)
- Optional Serial port ISA card


IBM PC Jr:

TODO: Which clock signals are available in a PC Jr?
      - What clock is Y1? This eventually gets passed on to the CPU (ZM40?) and some other components by a 8284 (ZM8?).
      - Is the clock attached to the Video Gate Array (ZM36?) exactly 14MHz?

IBM CGA/MDA:
Several different font roms were available, depending on what region the card was purchased in;
Currently known: (probably exist for all the standard codepages)
5788005: US (code page 437)
???????: Greek (code page 737) NOT DUMPED!
???????: Estonian/Lithuanian/Latvian (code page 775) NOT DUMPED!
???????: Icelandic (code page 861, characters 0x8B-0x8D, 0x95, 0x97-0x98, 0x9B, 0x9D, 0xA4-0xA7 differ from cp437) NOT DUMPED!
4733197: Danish/Norwegian (code page 865, characters 0x9B and 0x9D differ from cp437) NOT DUMPED!
???????: Hebrew (code page 862) NOT DUMPED!

***************************************************************************/


#include "emu.h"
#include "cpu/i86/i86.h"
#include "sound/speaker.h"
#include "sound/saa1099.h"

#include "machine/i8255a.h"
#include "machine/ins8250.h"
#include "machine/mc146818.h"
#include "machine/pic8259.h"

#include "machine/pit8253.h"
#include "video/pc_vga_mess.h"
#include "video/pc_cga.h"
#include "video/pc_mda.h"
#include "video/pc_ega.h"

#include "includes/pc_ide.h"
#include "machine/pc_fdc.h"
#include "machine/pc_joy.h"
#include "machine/pckeybrd.h"
#include "machine/pc_lpt.h"
#include "audio/sblaster.h"
#include "includes/pc_mouse.h"

#include "machine/pcshare.h"
#include "includes/pc.h"

#include "machine/pc_hdc.h"
#include "imagedev/flopdrv.h"
#include "imagedev/harddriv.h"
#include "imagedev/cassette.h"
#include "imagedev/cartslot.h"
#include "formats/pc_dsk.h"

#include "machine/8237dma.h"
#include "sound/sn76496.h"
#include "sound/3812intf.h"

#include "machine/kb_keytro.h"
#include "machine/ram.h"

#define ym3812_StdClock 3579545

/*
  adlib (YM3812/OPL2 chip), part of many many soundcards (soundblaster)
  soundblaster: YM3812 also accessible at 0x228/9 (address jumperable)
  soundblaster pro version 1: 2 YM3812 chips
   at 0x388 both accessed,
   at 0x220/1 left?, 0x222/3 right? (jumperable)
  soundblaster pro version 2: 1 OPL3 chip

  pro audio spectrum +: 2 OPL2
  pro audio spectrum 16: 1 OPL3
 */
#define ADLIB	/* YM3812/OPL2 Chip */
/*
  creative labs game blaster (CMS creative music system)
  2 x saa1099 chips
  also on sound blaster 1.0
  option on sound blaster 1.5

  jumperable? normally 0x220
*/
#define GAMEBLASTER


// IO Expansion, only a little bit for ibm bios self tests
//#define EXP_ON

static ADDRESS_MAP_START( pc8_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK("bank10")
	AM_RANGE(0xa0000, 0xbffff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_ROM
	AM_RANGE(0xc8000, 0xcffff) AM_ROM
	AM_RANGE(0xd0000, 0xeffff) AM_NOP
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(pc8_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE("dma8237", i8237_r, i8237_w)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE("pic8259", pic8259_r, pic8259_w)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE("pit8253", pit8253_r, pit8253_w)
	AM_RANGE(0x0060, 0x0063) AM_DEVREADWRITE("ppi8255", i8255a_r, i8255a_w)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE(pc_page_r,			pc_page_w)
	AM_RANGE(0x00a0, 0x00a0) AM_WRITE( pc_nmi_enable_w )
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc_JOY_r,				pc_JOY_w)
#ifdef EXP_ON
	AM_RANGE(0x0210, 0x0217) AM_READWRITE(pc_EXP_r,				pc_EXP_w)
#endif
	AM_RANGE(0x0240, 0x0257) AM_READWRITE(pc_rtc_r,				pc_rtc_w)
	AM_RANGE(0x0278, 0x027b) AM_DEVREADWRITE("lpt_2", pc_lpt_r, pc_lpt_w)
	AM_RANGE(0x02e8, 0x02ef) AM_DEVREADWRITE("ins8250_3", ins8250_r, ins8250_w)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE("ins8250_1", ins8250_r, ins8250_w)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc_HDC1_r,			pc_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc_HDC2_r,			pc_HDC2_w)
	AM_RANGE(0x0340, 0x0357) AM_NOP /* anonymous bios should not recogniced realtimeclock */
	AM_RANGE(0x0378, 0x037f) AM_DEVREADWRITE("lpt_1", pc_lpt_r, pc_lpt_w)
#ifdef ADLIB
	AM_RANGE(0x0388, 0x0388) AM_DEVREADWRITE("ym3812", ym3812_status_port_r,ym3812_control_port_w)
	AM_RANGE(0x0389, 0x0389) AM_DEVWRITE("ym3812", ym3812_write_port_w)
#endif
	AM_RANGE(0x03bc, 0x03be) AM_DEVREADWRITE("lpt_0", pc_lpt_r, pc_lpt_w)
	AM_RANGE(0x03e8, 0x03ef) AM_DEVREADWRITE("ins8250_2", ins8250_r, ins8250_w)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE(pc_fdc_r,				pc_fdc_w)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE("ins8250_0", ins8250_r, ins8250_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( ibm5150 )
	PORT_START("IN0") /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

	PORT_START("DSW0") /* IN1 */
	PORT_DIPNAME( 0xc0, 0x40, "Number of floppy drives")
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_DIPNAME( 0x30, 0x20, "Graphics adapter")
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_DIPNAME( 0x0c, 0x0c, "RAM banks")
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_DIPNAME( 0x02, 0x00, "80387 installed")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x01, "Floppy installed")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )

	PORT_START("DSW1") /* IN2 */
	PORT_DIPNAME( 0x80, 0x80, "COM1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "COM2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "COM3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, "COM4: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "LPT1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x00, "LPT2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, "LPT3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x00, "Game port enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

	PORT_START("DSW2") /* IN3 */
	PORT_DIPNAME( 0xf0, 0x80, "Serial mouse")
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x08, 0x08, "HDC1 (C800:0 port 320-323)")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, "HDC2 (CA00:0 port 324-327)")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	PORT_INCLUDE( kb_keytronic_pc )		/* IN4 - IN11 */
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
	PORT_INCLUDE( pcvideo_cga )
INPUT_PORTS_END

static const unsigned i86_address_mask = 0x000fffff;

#if defined(ADLIB)
/* irq line not connected to pc on adlib cards (and compatibles) */
static void pc_irqhandler(device_t *device, int linestate) {}

static const ym3812_interface pc_ym3812_interface =
{
	pc_irqhandler
};
#endif


static const pc_lpt_interface pc_lpt_config =
{
	DEVCB_CPU_INPUT_LINE("maincpu", 0)
};

static const floppy_config ibmpc_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(pc),
	NULL
};

static const kb_keytronic_interface pc_keytronic_intf =
{
	DEVCB_MEMORY_HANDLER("maincpu", IO, ibm5150_kb_set_clock_signal),
	DEVCB_MEMORY_HANDLER("maincpu", IO, ibm5150_kb_set_data_signal),
};


#define MCFG_CPU_PC(mem, port, type, clock, vblankfunc)	\
	MCFG_CPU_ADD("maincpu", type, clock)				\
	MCFG_CPU_PROGRAM_MAP(mem##_map)	\
	MCFG_CPU_IO_MAP(port##_io)	\
	MCFG_CPU_VBLANK_INT_HACK(vblankfunc, 4)					\
	MCFG_CPU_CONFIG(i86_address_mask)


/* F4 Character Displayer */
static const gfx_layout pc_16_charlayout =
{
	8, 16,					/* 8 x 16 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 2048*8, 2049*8, 2050*8, 2051*8, 2052*8, 2053*8, 2054*8, 2055*8 },
	8*8					/* every char takes 2 x 8 bytes */
};

static const gfx_layout pc_8_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	512,					/* 512 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static const cassette_config ibm5150_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_PLAY | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED),
	NULL
};

static GFXDECODE_START( ibm5150 )
	GFXDECODE_ENTRY( "gfx1", 0x0000, pc_16_charlayout, 3, 1 )
	GFXDECODE_ENTRY( "gfx1", 0x1000, pc_8_charlayout, 3, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( ibm5150, pc_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I8088, XTAL_14_31818MHz/3)
	MCFG_CPU_PROGRAM_MAP(pc8_map)
	MCFG_CPU_IO_MAP(pc8_io)
	MCFG_CPU_CONFIG(i86_address_mask)

	MCFG_MACHINE_START(pc)
	MCFG_MACHINE_RESET(pc)

	MCFG_PIT8253_ADD( "pit8253", ibm5150_pit8253_config )

	MCFG_I8237_ADD( "dma8237", XTAL_14_31818MHz/3, ibm5150_dma8237_config )

	MCFG_PIC8259_ADD( "pic8259", ibm5150_pic8259_config )

	MCFG_I8255A_ADD( "ppi8255", ibm5150_ppi8255_interface )

	MCFG_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_2", ibm5150_com_interface[2] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_3", ibm5150_com_interface[3] )			/* TODO: Verify model */

	/* video hardware */
	MCFG_FRAGMENT_ADD( pcvideo_cga )
	MCFG_GFXDECODE(ibm5150)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MCFG_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MCFG_SOUND_CONFIG(pc_ym3812_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MCFG_SOUND_ADD("saa1099.1", SAA1099, 4772720)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MCFG_SOUND_ADD("saa1099.2", SAA1099, 4772720)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif

	/* keyboard */
	MCFG_KB_KEYTRONIC_ADD("keyboard", pc_keytronic_intf)

	/* printer */
	MCFG_PC_LPT_ADD("lpt_0", pc_lpt_config)
	MCFG_PC_LPT_ADD("lpt_1", pc_lpt_config)
	MCFG_PC_LPT_ADD("lpt_2", pc_lpt_config)

	/* harddisk */
	MCFG_FRAGMENT_ADD( pc_hdc )

	MCFG_CASSETTE_ADD( "cassette", ibm5150_cassette_config )

	MCFG_UPD765A_ADD("upd765", pc_fdc_upd765_not_connected_interface)

	MCFG_FLOPPY_2_DRIVES_ADD(ibmpc_floppy_config)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("640K")
MACHINE_CONFIG_END


static MACHINE_CONFIG_START( ibm5160, pc_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I8088, XTAL_14_31818MHz/3)
	MCFG_CPU_PROGRAM_MAP(pc8_map)
	MCFG_CPU_IO_MAP(pc8_io)
	MCFG_CPU_CONFIG(i86_address_mask)

	MCFG_MACHINE_START(pc)
	MCFG_MACHINE_RESET(pc)

	MCFG_PIT8253_ADD( "pit8253", ibm5150_pit8253_config )

	MCFG_I8237_ADD( "dma8237", XTAL_14_31818MHz/3, ibm5150_dma8237_config )

	MCFG_PIC8259_ADD( "pic8259", ibm5150_pic8259_config )

	MCFG_I8255A_ADD( "ppi8255", ibm5160_ppi8255_interface )

	MCFG_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_2", ibm5150_com_interface[2] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_3", ibm5150_com_interface[3] )			/* TODO: Verify model */

	/* video hardware */
	MCFG_FRAGMENT_ADD( pcvideo_cga )
	MCFG_GFXDECODE(ibm5150)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MCFG_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MCFG_SOUND_CONFIG(pc_ym3812_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MCFG_SOUND_ADD("saa1099.1", SAA1099, 4772720)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MCFG_SOUND_ADD("saa1099.2", SAA1099, 4772720)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif

	/* keyboard */
	MCFG_KB_KEYTRONIC_ADD("keyboard", pc_keytronic_intf)

	/* printer */
	MCFG_PC_LPT_ADD("lpt_0", pc_lpt_config)
	MCFG_PC_LPT_ADD("lpt_1", pc_lpt_config)
	MCFG_PC_LPT_ADD("lpt_2", pc_lpt_config)

	/* harddisk */
	MCFG_FRAGMENT_ADD( pc_hdc )

	MCFG_UPD765A_ADD("upd765", pc_fdc_upd765_not_connected_interface)

	MCFG_FLOPPY_2_DRIVES_ADD(ibmpc_floppy_config)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("640K")
MACHINE_CONFIG_END


ROM_START( ibm5150 )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */
//  ROM_LOAD("600963.u12", 0xc8000, 0x02000, CRC(f3daf85f) SHA1(3bd29538832d3084cbddeec92593988772755283))  /* Tandon/Western Digital Fixed Disk Adapter 600963-001__TYPE_5.U12.2764.bin - Meant for an IBM PC or XT which lacked bios support for HDDs */

	/* Xebec 1210 and 1220 Z80-based ST409/ST412 MFM controllers */
//  ROM_LOAD("5000059.12d", 0xc8000, 0x02000, CRC(03e0ee9a) SHA1(6691be4f6a8d690c696ad8b259708d3e7e87ad89)) /* Xebec 1210 IBM OEM Fixed Disk Adapter - Revision 1, supplied with rev1 and rev2 XT, and PC/3270. supports 4 hdd types, selectable by jumpers (which on the older XTs were usually soldered to one setting) */
//  ROM_LOAD("62x0822.12d", 0xc8000, 0x02000, CRC(4cdd2193) SHA1(fe8f88333b5e13e170bf637a9a0090383dee454d)) /* Xebec 1210 IBM OEM Fixed Disk Adapter 62X0822__(M)_AMI_8621MAB__S68B364-P__(C)IBM_CORP_1982,1985__PHILIPPINES.12D.2364.bin - Revision 2, supplied with rev3 and rev4 XT. supports 4 hdd types, selectable by jumpers (which unlike the older XTs, were changable without requiring soldering )*/
//  ROM_LOAD("unknown.12d", 0xc8000, 0x02000, NO_DUMP ) /* Xebec 1210 Non-IBM/Retail Fixed Disk Adapter - supports 4 hdd types, selectable by jumpers (changable without soldering) */
//  ROM_LOAD("unknown.???", 0xc8000, 0x02000, NO_DUMP ) /* Xebec 1220 Non-IBM/Retail Fixed Disk Adapter plus Floppy Disk Adapter - supports 4 hdd types, selectable by jumpers (changable without soldering) */

	/* IBM PC 5150 (rev 3: 1501-476 10/27/82) 5-screw case 64-256k MB w/1501981 CGA Card, ROM Basic 1.1 */
	ROM_DEFAULT_BIOS( "rev3" )

	ROM_SYSTEM_BIOS( 0, "rev3", "IBM PC 5150 1501476 10/27/82" )
	ROMX_LOAD("5000019.u29", 0xf6000, 0x2000, CRC(80d3cf5d) SHA1(64769b7a8b60ffeefa04e4afbec778069a2840c9), ROM_BIOS(1))		/* ROM Basic 1.1 F6000-F7FFF; IBM P/N: 5000019, FRU: 6359109 */
	ROMX_LOAD("5000021.u30", 0xf8000, 0x2000, CRC(673a4acc) SHA1(082ae803994048e225150f771794ca305f73d731), ROM_BIOS(1))		/* ROM Basic 1.1 F8000-F9FFF; IBM P/N: 5000021, FRU: 6359111 */
	ROMX_LOAD("5000022.u31", 0xfa000, 0x2000, CRC(aac3fc37) SHA1(c9e0529470edf04da093bb8c8ae2536c688c1a74), ROM_BIOS(1))		/* ROM Basic 1.1 FA000-FBFFF; IBM P/N: 5000022, FRU: 6359112 */
	ROMX_LOAD("5000023.u32", 0xfc000, 0x2000, CRC(3062b3fc) SHA1(5134dd64721cbf093d059ee5d3fd09c7f86604c7), ROM_BIOS(1))		/* ROM Basic 1.1 FC000-FDFFF; IBM P/N: 5000023, FRU: 6359113 */
	ROMX_LOAD("1501476.u33", 0xfe000, 0x2000, CRC(e88792b3) SHA1(40fce6a94dda4328a8b608c7ae2f39d1dc688af4), ROM_BIOS(1))

	/* IBM PC 5150 (rev 1: 04/24/81) 2-screw case 16-64k MB w/MDA Card, ROM Basic 1.0 */
	ROM_SYSTEM_BIOS( 1, "rev1", "IBM PC 5150 5700051 04/24/81" )
	ROMX_LOAD("5700019.u29", 0xf6000, 0x2000, CRC(b59e8f6c) SHA1(7a5db95370194c73b7921f2d69267268c69d2511), ROM_BIOS(2))		/* ROM Basic 1.0 F6000-F7FFF */
	ROMX_LOAD("5700027.u30", 0xf8000, 0x2000, CRC(bfff99b8) SHA1(ca2f126ba69c1613b7b5a4137d8d8cf1db36a8e6), ROM_BIOS(2))		/* ROM Basic 1.0 F8000-F9FFF */
	ROMX_LOAD("5700035.u31", 0xfa000, 0x2000, CRC(9fe4ec11) SHA1(89af8138185938c3da3386f97d3b0549a51de5ef), ROM_BIOS(2))		/* ROM Basic 1.0 FA000-FBFFF */
	ROMX_LOAD("5700043.u32", 0xfc000, 0x2000, CRC(ea2794e6) SHA1(22fe58bc853ffd393d5e2f98defda7456924b04f), ROM_BIOS(2))		/* ROM Basic 1.0 FC000-FDFFF */
	ROMX_LOAD("5700051.u33", 0xfe000, 0x2000, CRC(12d33fb8) SHA1(f046058faa016ad13aed5a082a45b21dea43d346), ROM_BIOS(2))

	/* IBM PC 5150 (rev 2: 10/19/81) 2-screw case, 16-64k MB w/MDA Card, ROM Basic 1.0 */
	ROM_SYSTEM_BIOS( 2, "rev2", "IBM PC 5150 5700671 10/19/81" )
	ROMX_LOAD("5700019.u29", 0xf6000, 0x2000, CRC(b59e8f6c) SHA1(7a5db95370194c73b7921f2d69267268c69d2511), ROM_BIOS(3))		/* ROM Basic 1.0 F6000-F7FFF */
	ROMX_LOAD("5700027.u30", 0xf8000, 0x2000, CRC(bfff99b8) SHA1(ca2f126ba69c1613b7b5a4137d8d8cf1db36a8e6), ROM_BIOS(3))		/* ROM Basic 1.0 F8000-F9FFF */
	ROMX_LOAD("5700035.u31", 0xfa000, 0x2000, CRC(9fe4ec11) SHA1(89af8138185938c3da3386f97d3b0549a51de5ef), ROM_BIOS(3))		/* ROM Basic 1.0 FA000-FBFFF */
	ROMX_LOAD("5700043.u32", 0xfc000, 0x2000, CRC(ea2794e6) SHA1(22fe58bc853ffd393d5e2f98defda7456924b04f), ROM_BIOS(3))		/* ROM Basic 1.0 FC000-FDFFF */
	ROMX_LOAD("5700671.u33", 0xfe000, 0x2000, CRC(b7d4ec46) SHA1(bdb06f846c4768f39eeff7e16b6dbff8cd2117d2), ROM_BIOS(3))

	/* Z80 on the Xebec 1210 and 1220 Hard Disk Controllers */
//  ROM_REGION(0x10000, "cpu1", 0)
//  ROM_LOAD("104839re.12a", 0x0000, 0x1000, CRC(3ad32fcc) SHA1(0127fa520aaee91285cb46a640ed835b4554e4b3))  /* Xebec 1210 IBM OEM Hard Disk Controller, silkscreened "104839RE // COPYRIGHT // XEBEC 1986" - Common for both XEBEC 1210 IBM OEM revisions. Some cards have the rom marked 104839E instead (John Eliott's card is like this), but contents are the same. */
//  /* Other versions probably exist for the non-IBM/Retail 1210 and the 1220 */

	/* IBM 1501981(CGA) and 1501985(MDA) Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f)) /* "AMI 8412PI // 5788005 // (C) IBM CORP. 1981 // KOREA" */
ROM_END
ROM_START( ibm5155 )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */
    ROM_LOAD("5000027.u19", 0xf0000, 0x8000, CRC(fc982309) SHA1(2aa781a698a21c332398d9bc8503d4f580df0a05))
	ROM_LOAD("1501512.u18", 0xf8000, 0x8000, CRC(79522c3d) SHA1(6bac726d8d033491d52507278aa388ec04cf8b7e))
	/* IBM 1501981(CGA) and 1501985(MDA) Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f)) /* "AMI 8412PI // 5788005 // (C) IBM CORP. 1981 // KOREA" */
ROM_END
ROM_START( ibm5140 )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */
	ROM_LOAD("7396917.bin", 0xf0000, 0x08000, CRC(95c35652) SHA1(2bdac30715dba114fbe0895b8b4723f8dc26a90d))
	ROM_LOAD("7396918.bin", 0xf8000, 0x08000, CRC(1b4202b0) SHA1(4797ff853ba1675860f293b6368832d05e2f3ea9))
	/* IBM 1501981(CGA) and 1501985(MDA) Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f)) /* "AMI 8412PI // 5788005 // (C) IBM CORP. 1981 // KOREA" */
ROM_END
#ifdef UNUSED_DEFINITION
ROM_START( ibmpca )
	ROM_REGION(0x100000,"maincpu",0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */
	ROM_LOAD("basicc11.f6", 0xf6000, 0x2000, CRC(80d3cf5d) SHA1(64769b7a8b60ffeefa04e4afbec778069a2840c9))
	ROM_LOAD("basicc11.f8", 0xf8000, 0x2000, CRC(673a4acc) SHA1(082ae803994048e225150f771794ca305f73d731))
	ROM_LOAD("basicc11.fa", 0xfa000, 0x2000, CRC(aac3fc37) SHA1(c9e0529470edf04da093bb8c8ae2536c688c1a74))
	ROM_LOAD("basicc11.fc", 0xfc000, 0x2000, CRC(3062b3fc) SHA1(5134dd64721cbf093d059ee5d3fd09c7f86604c7))
	ROM_LOAD("pc081682.bin", 0xfe000, 0x2000, CRC(5c3f0256) SHA1(b42c78abd0a9c630a2f972ad2bae46d83c3a2a09))

	/* IBM 1501981(CGA) and 1501985(MDA) Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f)) /* "AMI 8412PI // 5788005 // (C) IBM CORP. 1981 // KOREA" */
ROM_END
#endif


ROM_START( ibm5160 )
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */
//  ROM_LOAD("600963.u12", 0xc8000, 0x02000, CRC(f3daf85f) SHA1(3bd29538832d3084cbddeec92593988772755283))  /* Tandon/Western Digital Fixed Disk Adapter 600963-001__TYPE_5.U12.2764.bin */

	/* PC/3270 has a 3270 keyboard controller card, plus a rom on that card to tell the pc how to run it.
        * Unlike the much more complex keyboard controller used in the AT/3270, this one only has one rom,
          a motorola made "(M)1503828 // XE // 8434A XM // SC81155P" custom (an MCU?; the more complicated
          3270/AT keyboard card uses this same exact chip), an 8254, and some logic chips.
          Thanks to high resolution pictures provided by John Elliott, I can see that the location of the
                  chips is unlabeled (except for by absolute pin position on the back), and there are no pals or proms.
        * The board is stickered "2683114 // 874999 // 8446 SU" on the front.
        * The board has a single DE-9 connector where the keyboard dongle connects to.
        * The keyboard dongle has two connectors on it: a DIN-5 connector which connects to the Motherboard's
          keyboard port, plus an RJ45-lookalike socket which the 3270 keyboard connects to.
        * The rom is mapped very strangely to avoid hitting the hard disk controller:
          The first 0x800 bytes appear at C0000-C07FF, and the last 0x1800 bytes appear at 0xCA000-CB7FF
    */
//  ROM_LOAD("6323581.bin", 0xc0000, 0x00800, CRC(cf323cbd) SHA1(93c1ef2ede02772a46dab075c32e179faa045f81))
//  ROM_LOAD("6323581.bin", 0xca000, 0x01800, CRC(cf323cbd) SHA1(93c1ef2ede02772a46dab075c32e179faa045f81) ROM_SKIP(0x800))

	/* Xebec 1210 and 1220 Z80-based ST409/ST412 MFM controllers */
//  ROM_LOAD("5000059.12d", 0xc8000, 0x02000, CRC(03e0ee9a) SHA1(6691be4f6a8d690c696ad8b259708d3e7e87ad89)) /* Xebec 1210 IBM OEM Fixed Disk Adapter - Revision 1, supplied with rev1 and rev2 XT, and PC/3270. supports 4 hdd types, selectable by jumpers (which on the older XTs were usually soldered to one setting) */
//  ROM_LOAD("62x0822.12d", 0xc8000, 0x02000, CRC(4cdd2193) SHA1(fe8f88333b5e13e170bf637a9a0090383dee454d)) /* Xebec 1210 IBM OEM Fixed Disk Adapter 62X0822__(M)_AMI_8621MAB__S68B364-P__(C)IBM_CORP_1982,1985__PHILIPPINES.12D.2364.bin - Revision 2, supplied with rev3 and rev4 XT. supports 4 hdd types, selectable by jumpers (which unlike the older XTs, were changable without requiring soldering )*/
//  ROM_LOAD("unknown.12d", 0xc8000, 0x02000, NO_DUMP ) /* Xebec 1210 Non-IBM/Retail Fixed Disk Adapter - supports 4 hdd types, selectable by jumpers (changable without soldering) */
//  ROM_LOAD("unknown.???", 0xc8000, 0x02000, NO_DUMP ) /* Xebec 1220 Non-IBM/Retail Fixed Disk Adapter plus Floppy Disk Adapter - supports 4 hdd types, selectable by jumpers (changable without soldering) */


	ROM_DEFAULT_BIOS( "rev4" )

	ROM_SYSTEM_BIOS( 0, "rev1", "IBM XT 5160 08/16/82" )	/* ROM at u18 marked as BAD_DUMP for now, as current dump, while likely correct, was regenerated from a number of smaller dumps, and needs a proper redump. */
	ROMX_LOAD("5000027.u19", 0xf0000, 0x8000, CRC(fc982309) SHA1(2aa781a698a21c332398d9bc8503d4f580df0a05), ROM_BIOS(1) )
	ROMX_LOAD("5000026.u18", 0xf8000, 0x8000, BAD_DUMP CRC(3c9b0ac3) SHA1(271c9f4cef5029a1560075550b67c3395db09fef), ROM_BIOS(1) ) /* This is probably a good dump, and works fine, but as it was manually regenerated based on a partial dump, it needs to be reverified. It's a very rare rom revision and may have only appeared on XT prototypes. */

	ROM_SYSTEM_BIOS( 1, "rev2", "IBM XT 5160 11/08/82" )	/* Same as PC 5155 BIOS and PC/3270 BIOS */
	ROMX_LOAD("5000027.u19", 0xf0000, 0x8000, CRC(fc982309) SHA1(2aa781a698a21c332398d9bc8503d4f580df0a05), ROM_BIOS(2) ) /* silkscreen "MK37050N-4 // 5000027" - FRU: 6359116 - Contents repeat 4 times; Alt Silkscreen (from yesterpc.com): "(M) // 5000027 // (C) 1983 IBM CORP // X E // 8425B NM"*/
	ROMX_LOAD("1501512.u18", 0xf8000, 0x8000, CRC(79522c3d) SHA1(6bac726d8d033491d52507278aa388ec04cf8b7e), ROM_BIOS(2) ) /* silkscreen "MK38036N-25 // 1501512 // ZA // (C)IBM CORP // 1981,1983 // D MALAYSIA // 8438 AP"*/

	ROM_SYSTEM_BIOS( 2, "rev3", "IBM XT 5160 01/10/86" )    /* Has enhanced keyboard support and a 3.5" drive */
	ROMX_LOAD("62x0854.u19", 0xf0000, 0x8000, CRC(b5fb0e83) SHA1(937b43759ffd472da4fb0fe775b3842f5fb4c3b3), ROM_BIOS(3) )
	ROMX_LOAD("62x0851.u18", 0xf8000, 0x8000, CRC(1054f7bd) SHA1(e7d0155813e4c650085144327581f05486ed1484), ROM_BIOS(3) )

	ROM_SYSTEM_BIOS( 3, "rev4", "IBM XT 5160 05/09/86" )	/* Minor bugfixes to keyboard code, supposedly */
	ROMX_LOAD("68x4370.u19", 0xf0000, 0x8000, CRC(758ff036) SHA1(045e27a70407d89b7956ecae4d275bd2f6b0f8e2), ROM_BIOS(4))
	ROMX_LOAD("62x0890.u18", 0xf8000, 0x8000, CRC(4f417635) SHA1(daa61762d3afdd7262e34edf1a3d2df9a05bcebb), ROM_BIOS(4))

//  ROM_SYSTEM_BIOS( 4, "xtdiag", "IBM XT 5160 w/Supersoft Diagnostics" )    /* ROMs marked as BAD_DUMP for now. We expect the data to be in a different ROM chip layout */
//  ROMX_LOAD("basicc11.f6", 0xf6000, 0x2000, BAD_DUMP CRC(80d3cf5d) SHA1(64769b7a8b60ffeefa04e4afbec778069a2840c9), ROM_BIOS(5) )
//  ROMX_LOAD("basicc11.f8", 0xf8000, 0x2000, BAD_DUMP CRC(673a4acc) SHA1(082ae803994048e225150f771794ca305f73d731), ROM_BIOS(5) )
//  ROMX_LOAD("basicc11.fa", 0xfa000, 0x2000, BAD_DUMP CRC(aac3fc37) SHA1(c9e0529470edf04da093bb8c8ae2536c688c1a74), ROM_BIOS(5) )
//  ROMX_LOAD("basicc11.fc", 0xfc000, 0x2000, BAD_DUMP CRC(3062b3fc) SHA1(5134dd64721cbf093d059ee5d3fd09c7f86604c7), ROM_BIOS(5) )
//  ROMX_LOAD("xtdiag.bin", 0xfe000, 0x2000, CRC(4e89a4d8) SHA1(39a28fb2fe9f1aeea24ed2c0255cebca76e37ed7), ROM_BIOS(5) )

	/* IBM 1501981(CGA) and 1501985(MDA) Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f)) /* "AMI 8412PI // 5788005 // (C) IBM CORP. 1981 // KOREA" */

	/* Z80 ROM on the Xebec 1210 and 1220 Hard Disk Controllers */
//  ROM_REGION(0x10000, "cpu1", 0)
//  ROM_LOAD("104839re.12a", 0x0000, 0x1000, CRC(3ad32fcc) SHA1(0127fa520aaee91285cb46a640ed835b4554e4b3))  /* Xebec 1210 IBM OEM Hard Disk Controller, silkscreened "104839RE // COPYRIGHT // XEBEC 1986" - Common for both XEBEC 1210 IBM OEM revisions. Some cards have the rom marked 104839E instead (John Eliott's card is like this), but contents are the same. */
//  /* Other versions probably exist for the non-IBM/Retail 1210 and the 1220 */


	/* PC/3270 and AT/3270 have a set of two (optionally 3) 3270PGC programmable graphics controller cards on them which
           have 2 extra roms, plus a number of custom chips and at least one MCU.
       Information on these three boards plus the keyboard interface can be found at:
       http://www.seasip.info/VintagePC/5271.html
        *** The descriptions below are based on the cards from the AT/3270, which are slightly
            different from the PC/3270 cards. Changes between the PC/3270 and AT/3270 video cards are
            listed:
            1. The card lip edge is changed on the APA and PS cards to allow them to be placed
               in a 16-bit ISA slot.
            2. The Main Display board is exactly the same PCB (hence cannot be placed in a 16-bit
               ISA slot), but the socket to the left of the 8255 is now populated, and has a lot of
               rework wires connected to various places on the PCB.
            3. The APA board was completely redone, and no longer has an 8254 (though it does have an
               empty socket) and has ?twice as much memory on it? (not sure about this). The APA
               board also now connects to both main board connectors 1 and 3, instead of only
               connector 1.
            4. The PS board has been minorly redone to allow clearance for a 16-bit ISA connector,
               but no other significant chip changes were made. The connector 3 still exists on the
               board but is unpopulated. Connector 2 still connects to the Main display board as
               before.

        ** The Main Display Board (with one 48-pin custom, 3 40 pin customs at least one of which is
           an MCU, four 2016BP-10 srams, an 8254 and an 8255 on it, two crystals (16.257MHz and
           21.676MHz) plus two mask roms ) is stickered "61X6579 // 983623 // 6390 SU" on the front.
        *  The pcb is trace-marked "6320987" on both the front and back.
        *  The card has a DE-9 connector on it for a monitor.
        *  The customs are marked:
           "1503192 // TC15G008P-0009 // JAPAN       8549A" (40 pins, at U52)
           "1503193 // TC15G008AP-0020 // JAPAN       8610A" (48 pins, at U29)
           "(M)1503194 // XE KGA005 // 8616N XM // SC81156P" (40 pins, at U36, likely an MCU)
           "S8613 // SCN2672B // C4N40 A // CP3303" (40 pins, at U24, also possibly an MCU)

        ** The All Points Addressable (Frame buffer?) card (with 2 48-pin customs on it which are
           probably gate arrays and not MCUs, an empty socket (28 pins, U46), an Intel Id2147H-3,
           a bank of twelve 16k*4-bit inmos ims2620p-15 DRAMs (tms4416 equivalent), and an Intel
           D2147K 4096*1 byte SRAM) is stickered
           "6487836 // A24969 // 6400 SU" on the back.
        *  The pcb is trace-marked "EC 999040" on the back, and silkscreened "RC 2682819" on the front
        *  The customs are marked:
           "6323259 // TC15G008AP-0028 // JAPAN       8606A" (48 pins, at U67)
           "6323260 // TC15G022AP-0018 // JAPAN       8606A" (48 pins, at U45)

        ** The optional Programmable Symbol Card (with an AMD AM9128-10PC, and six tms4416-15NL DRAMS,
           and a fleet of discrete logic chips, but no roms, pals, or proms) is stickered
           "6347750 // A24866 // 6285 SU" on the front.
        *  The PCB is trace-marked "PROGAMMABLE SYMBOL P/N 6347751 // ASSY. NO. 6347750" on the front,
           and trace-marked "|||CIM0286 ECA2466 // 94V-O" on the back.
    */
//  ROM_REGION(0x4000,"gfx2", 0)
//      ROM_LOAD("1504161.u11", 0x00000, 0x2000, CRC(d9246cf5) SHA1(2eaed495893a4e6649b04d10dada7b5ef4abd140)) /* silkscreen: "AMI 8613MAJ // 9591-041 // S2364B // 1504161 // PHILIPPINES" - Purpose: Pixels 0 thru 7 of built-in 3270 terminal font*/
//      ROM_LOAD("1504162.u26", 0x02000, 0x2000, CRC(59e1dc32) SHA1(337b5cced203345a5acfb02532d6b5f526902ee7)) /* silkscreen: "AMI 8607MAH // 9591-042 // S2364B // 1504162 // PHILIPPINES" - Purpose: Pixel 8 of built-in 3270 terminal font*/
ROM_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

/*     YEAR     NAME        PARENT      COMPAT  MACHINE     INPUT       INIT        COMPANY     FULLNAME */
COMP(  1981,	ibm5150,    0,			0,		ibm5150,    ibm5150,	ibm5150,    "International Business Machines",  "IBM PC 5150" , 0)
COMP(  1982,	ibm5155,    ibm5150,	0,		ibm5150,    ibm5150,	ibm5150,    "International Business Machines",  "IBM PC 5155" , 0)
COMP(  1985,	ibm5140,    ibm5150,	0,		ibm5150,    ibm5150,	ibm5150,    "International Business Machines",  "IBM PC 5140 Convertible" , GAME_NOT_WORKING)

// xt class (pc but 8086)
COMP(  1982,	ibm5160,    ibm5150,	0,		ibm5160,    ibm5150,	ibm5150,    "International Business Machines",  "IBM XT 5160" , 0)
