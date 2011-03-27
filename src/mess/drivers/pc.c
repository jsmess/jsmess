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


Amstrad PC1640
==============

More information can be found at http://www.seasip.info/AmstradXT/1640tech/index.html


Tandy 1000
==========

Tandy 1000 machines are similar to the IBM 5160s with CGA graphics. Tandy
added some additional graphic capabilities similar, but not equal, to
those added for the IBM PC Jr.

Tandy 1000 (8088) variations:
1000                128KB-640KB RAM     4.77 MHz        v01.00.00, v01.01.00
1000A/1000HD        128KB-640KB RAM     4.77 MHz        v01.01.00
1000SX/1000AX       384KB-640KB RAM     7.16/4.77 MHz   v01.02.00
1000EX              256KB-640KB RAM     7.16/4.77 MHz   v01.02.00
1000HX              256KB-640KB RAM     7.16/4.77 MHz   v02.00.00

Tandy 1000 (8086) variations:
1000RL/1000RL-HD    512KB-768KB RAM     9.44/4.77 MHz   v02.00.00, v02.00.01
1000SL/1000PC       384KB-640KB RAM     8.0/4.77 MHz    v01.04.00, v01.04.01, v01.04.02, v02.00.01
1000SL/2            512KB-640KB RAM     8.0/4.77 MHz    v01.04.04

Tandy 1000 (80286) variations:
1000TX              640KB-768KB RAM     8.0/4.77 MHz    v01.03.00
1000TL              640KB-768KB RAM     8.0/4.77 MHz    v01.04.00, v01.04.01, v01.04.02
1000TL/2            640KB-768KB RAM     8.0/4.77 MHz    v02.00.00
1000TL/3            640KB-768KB RAM     10.0/5.0 MHz    v02.00.00
1000RLX             512KB-1024KB RAM    10.0/5.0 MHz    v02.00.00
1000RLX-HD          1024MB RAM          10.0/5.0 MHz    v02.00.00

Tandy 1000 (80386) variations:
1000RSX/1000RSX-HD  1M-9M RAM           25.0/8.0 MHz    v01.10.00

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
#include "cpu/nec/nec.h"
#include "cpu/i86/i86.h"
#include "cpu/i86/i286.h"
#include "sound/speaker.h"
#include "sound/saa1099.h"
#include "deprecat.h"

#include "machine/i8255a.h"
#include "machine/ins8250.h"
#include "machine/mc146818.h"
#include "machine/pic8259.h"

#include "machine/pit8253.h"
#include "video/pc_vga_mess.h"
#include "video/pc_cga.h"
#include "video/pc_aga.h"
#include "video/pc_t1t.h"
#include "video/pc_ega.h"

#include "includes/pc_ide.h"
#include "machine/pc_fdc.h"
#include "machine/pc_joy.h"
#include "machine/pckeybrd.h"
#include "machine/pc_lpt.h"
#include "audio/sblaster.h"
#include "includes/pc_mouse.h"

#include "includes/europc.h"
#include "includes/tandy1t.h"
#include "includes/amstr_pc.h"

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

static ADDRESS_MAP_START( pc8_map, AS_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK("bank10")
	AM_RANGE(0xa0000, 0xbffff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_ROM
	AM_RANGE(0xc8000, 0xcffff) AM_ROM
	AM_RANGE(0xd0000, 0xeffff) AM_NOP
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mc1502_map, AS_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK("bank10")
	AM_RANGE(0xa0000, 0xbffff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_ROM
	AM_RANGE(0xc8000, 0xcffff) AM_ROM
	AM_RANGE(0xd0000, 0xeffff) AM_ROM
	AM_RANGE(0xfc000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( zenith_map, AS_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK("bank10")
	AM_RANGE(0xa0000, 0xbffff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_ROM
	AM_RANGE(0xc8000, 0xcffff) AM_ROM
	AM_RANGE(0xd0000, 0xeffff) AM_NOP
	AM_RANGE(0xf0000, 0xf7fff) AM_RAM
	AM_RANGE(0xf8000, 0xfffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( pc16_map, AS_PROGRAM, 16 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK("bank10")
	AM_RANGE(0xa0000, 0xbffff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_ROM
	AM_RANGE(0xc8000, 0xcffff) AM_ROM
	AM_RANGE(0xd0000, 0xeffff) AM_NOP
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START(pc8_io, AS_IO, 8)
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


static READ16_DEVICE_HANDLER( pc16_388_r )
{
	if ( ACCESSING_BITS_0_7 )
	{
		return 0xFF;
	}
	else
	{
		return ym3812_status_port_r( device, offset );
	}
}


static WRITE16_DEVICE_HANDLER( pc16_388_w )
{
	if ( ACCESSING_BITS_0_7 )
	{
		ym3812_write_port_w( device, offset, data );
	}
	else
	{
		ym3812_control_port_w( device, offset, data );
	}
}


static ADDRESS_MAP_START(pc16_io, AS_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE8("dma8237", i8237_r, i8237_w, 0xffff)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE8("pic8259", pic8259_r, pic8259_w, 0xffff)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE8("pit8253", pit8253_r, pit8253_w, 0xffff)
	AM_RANGE(0x0060, 0x0063) AM_DEVREADWRITE8("ppi8255", i8255a_r, i8255a_w, 0xffff)
	AM_RANGE(0x0070, 0x007f) AM_RAM // needed for Poisk-2
	AM_RANGE(0x0080, 0x0087) AM_READWRITE8(pc_page_r,				pc_page_w, 0xffff)
	AM_RANGE(0x00a0, 0x00a1) AM_WRITE8( pc_nmi_enable_w, 0x00ff )
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc16le_JOY_r,				pc16le_JOY_w)
#ifdef EXP_ON
	AM_RANGE(0x0210, 0x0217) AM_READWRITE(pc_EXP_r,					pc_EXP_w)
#endif
	AM_RANGE(0x0240, 0x0257) AM_READWRITE(pc16le_rtc_r,				pc16le_rtc_w)
	AM_RANGE(0x0278, 0x027b) AM_DEVREADWRITE8("lpt_2", pc_lpt_r, pc_lpt_w, 0x00ff)
	AM_RANGE(0x02b0, 0x02bf) AM_RAM // needed for EC-18xx
	AM_RANGE(0x02e8, 0x02ef) AM_DEVREADWRITE8("ins8250_3", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE8("ins8250_1", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc16le_HDC1_r,			pc16le_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc16le_HDC2_r,			pc16le_HDC2_w)
	AM_RANGE(0x0340, 0x0357) AM_NOP /* anonymous bios should not recogniced realtimeclock */
	AM_RANGE(0x0378, 0x037f) AM_DEVREADWRITE8("lpt_1", pc_lpt_r, pc_lpt_w, 0x00ff)
#ifdef ADLIB
	AM_RANGE(0x0388, 0x0389) AM_DEVREADWRITE("ym3812", pc16_388_r, pc16_388_w )
#endif
	AM_RANGE(0x03bc, 0x03bf) AM_DEVREADWRITE8("lpt_0", pc_lpt_r, pc_lpt_w, 0x00ff)
	AM_RANGE(0x03e8, 0x03ef) AM_DEVREADWRITE8("ins8250_2", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE8(pc_fdc_r,				pc_fdc_w, 0xffff)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE8("ins8250_0", ins8250_r, ins8250_w, 0xffff)
ADDRESS_MAP_END



static ADDRESS_MAP_START( europc_map, AS_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK("bank10")
	AM_RANGE(0xa0000, 0xaffff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_NOP
	AM_RANGE(0xc8000, 0xcffff) AM_ROM
	AM_RANGE(0xd0000, 0xeffff) AM_NOP
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END



static ADDRESS_MAP_START(europc_io, AS_IO, 8)
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE("dma8237", i8237_r, i8237_w)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE("pic8259", pic8259_r, pic8259_w)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE("pit8253", pit8253_r, pit8253_w)
	AM_RANGE(0x0060, 0x0063) AM_READWRITE(europc_pio_r,			europc_pio_w)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE(pc_page_r,			pc_page_w)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc_JOY_r,				pc_JOY_w)
	AM_RANGE(0x0250, 0x025f) AM_READWRITE(europc_jim_r,			europc_jim_w)
	AM_RANGE(0x0278, 0x027b) AM_DEVREADWRITE("lpt_2", pc_lpt_r, pc_lpt_w)
	AM_RANGE(0x02e0, 0x02e0) AM_READ     (europc_jim2_r)
	AM_RANGE(0x02e8, 0x02ef) AM_DEVREADWRITE("ins8250_3", ins8250_r, ins8250_w)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE("ins8250_1", ins8250_r, ins8250_w)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc_HDC1_r,			pc_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc_HDC2_r,			pc_HDC2_w)
	AM_RANGE(0x0378, 0x037b) AM_DEVREADWRITE("lpt_1", pc_lpt_r, pc_lpt_w)
#ifdef ADLIB
	AM_RANGE(0x0388, 0x0388) AM_DEVREADWRITE("ym3812", ym3812_status_port_r,ym3812_control_port_w)
	AM_RANGE(0x0389, 0x0389) AM_DEVWRITE("ym3812", ym3812_write_port_w)
#endif
//  AM_RANGE(0x03bc, 0x03bf) AM_DEVREADWRITE("lpt_0", pc_lpt_r, pc_lpt_w)
	AM_RANGE(0x03e8, 0x03ef) AM_DEVREADWRITE("ins8250_2", ins8250_r, ins8250_w)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE(pc_fdc_r,				pc_fdc_w)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE("ins8250_0", ins8250_r, ins8250_w)
ADDRESS_MAP_END



static ADDRESS_MAP_START(tandy1000_map, AS_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK("bank10")
	AM_RANGE(0xa0000, 0xaffff) AM_RAM
	AM_RANGE(0xb0000, 0xb7fff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_NOP
	AM_RANGE(0xc8000, 0xc9fff) AM_ROM
	AM_RANGE(0xca000, 0xcffff) AM_NOP
	AM_RANGE(0xd0000, 0xeffff) AM_NOP
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END



static ADDRESS_MAP_START(tandy1000_io, AS_IO, 8)
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE("dma8237", i8237_r, i8237_w)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE("pic8259", pic8259_r, pic8259_w)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE("pit8253", pit8253_r, pit8253_w)
	AM_RANGE(0x0060, 0x0063) AM_READWRITE(tandy1000_pio_r,			tandy1000_pio_w)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE(pc_page_r,				pc_page_w)
	AM_RANGE(0x00c0, 0x00c0) AM_DEVWRITE("sn76496", 	sn76496_w)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc_JOY_r,					pc_JOY_w)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE("ins8250_1", ins8250_r, ins8250_w)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc_HDC1_r,				pc_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc_HDC2_r,				pc_HDC2_w)
	AM_RANGE(0x0378, 0x037f) AM_READWRITE(pc_t1t_p37x_r,			pc_t1t_p37x_w)
	AM_RANGE(0x03bc, 0x03be) AM_DEVREADWRITE("lpt_0", pc_lpt_r, pc_lpt_w)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE(pc_fdc_r,					pc_fdc_w)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE("ins8250_0", ins8250_r, ins8250_w)
ADDRESS_MAP_END



static ADDRESS_MAP_START(tandy1000_16_map, AS_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK("bank10")
	AM_RANGE(0xa0000, 0xaffff) AM_RAM
	AM_RANGE(0xb0000, 0xb7fff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_NOP
	AM_RANGE(0xc8000, 0xc9fff) AM_ROM
	AM_RANGE(0xca000, 0xcffff) AM_NOP
	AM_RANGE(0xe0000, 0xeffff) AM_ROMBANK("bank11")						/* Banked part of the BIOS */
	AM_RANGE(0xf0000, 0xfffff) AM_ROM AM_REGION("maincpu", 0x70000)
ADDRESS_MAP_END



static ADDRESS_MAP_START(tandy1000_16_io, AS_IO, 16)
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE8("dma8237", i8237_r, i8237_w, 0xffff)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE8("pic8259", pic8259_r, pic8259_w, 0xffff)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE8("pit8253", pit8253_r, pit8253_w, 0xffff)
	AM_RANGE(0x0060, 0x0063) AM_READWRITE8(tandy1000_pio_r,			tandy1000_pio_w, 0xffff)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE8(pc_page_r,				pc_page_w, 0xffff)
	AM_RANGE(0x00c0, 0x00c1) AM_DEVWRITE8("sn76496",	sn76496_w, 0xffff)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE8(pc_JOY_r,					pc_JOY_w, 0xffff)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE8("ins8250_1", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE8(pc_HDC1_r,				pc_HDC1_w, 0xffff)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE8(pc_HDC2_r,				pc_HDC2_w, 0xffff)
	AM_RANGE(0x0378, 0x037f) AM_READWRITE8(pc_t1t_p37x_r,			pc_t1t_p37x_w, 0xffff)
	AM_RANGE(0x03bc, 0x03bf) AM_DEVREADWRITE8("lpt_0", pc_lpt_r, pc_lpt_w, 0xffff)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE8(pc_fdc_r,					pc_fdc_w, 0xffff)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE8("ins8250_0", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0xffea, 0xffeb) AM_READWRITE8(tandy1000_bank_r, tandy1000_bank_w, 0xffff)
ADDRESS_MAP_END



static ADDRESS_MAP_START(tandy1000_286_map, AS_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK("bank10")
	AM_RANGE(0xa0000, 0xaffff) AM_RAM
	AM_RANGE(0xb0000, 0xb7fff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_NOP
	AM_RANGE(0xc8000, 0xc9fff) AM_ROM
	AM_RANGE(0xca000, 0xcffff) AM_NOP
	AM_RANGE(0xe0000, 0xeffff) AM_NOP
	AM_RANGE(0xf8000, 0xfffff) AM_ROM
ADDRESS_MAP_END



static ADDRESS_MAP_START(tandy1000_286_io, AS_IO, 16)
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE8("dma8237", i8237_r, i8237_w, 0xffff)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE8("pic8259", pic8259_r, pic8259_w, 0xffff)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE8("pit8253", pit8253_r, pit8253_w, 0xffff)
	AM_RANGE(0x0060, 0x0063) AM_READWRITE8(tandy1000_pio_r,         tandy1000_pio_w, 0xffff)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE8(pc_page_r,               pc_page_w, 0xffff)
	AM_RANGE(0x00c0, 0x00c1) AM_DEVWRITE8("sn76496",    sn76496_w, 0xffff)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE8(pc_JOY_r,                    pc_JOY_w, 0xffff)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE8("ins8250_1", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE8(pc_HDC1_r,               pc_HDC1_w, 0xffff)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE8(pc_HDC2_r,               pc_HDC2_w, 0xffff)
	AM_RANGE(0x0378, 0x037f) AM_READWRITE8(pc_t1t_p37x_r,           pc_t1t_p37x_w, 0xffff)
	AM_RANGE(0x03bc, 0x03bf) AM_DEVREADWRITE8("lpt_0", pc_lpt_r, pc_lpt_w, 0xffff)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE8(pc_fdc_r,                    pc_fdc_w, 0xffff)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE8("ins8250_0", ins8250_r, ins8250_w, 0xffff)
ADDRESS_MAP_END


static ADDRESS_MAP_START(ibmpcjr_map, AS_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK("bank10")
	AM_RANGE(0xa0000, 0xaffff) AM_RAM
	AM_RANGE(0xb0000, 0xb7fff) AM_NOP
	AM_RANGE(0xb8000, 0xbffff) AM_RAMBANK("bank14")
	AM_RANGE(0xc0000, 0xc7fff) AM_NOP
	AM_RANGE(0xc8000, 0xc9fff) AM_ROM
	AM_RANGE(0xca000, 0xcffff) AM_NOP
	AM_RANGE(0xd0000, 0xdffff) AM_ROM
	AM_RANGE(0xe0000, 0xeffff) AM_ROM
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START(ibmpcjr_io, AS_IO, 8)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE("pic8259", pic8259_r, pic8259_w)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE("pit8253", pit8253_r, pit8253_w)
	AM_RANGE(0x0060, 0x0063) AM_DEVREADWRITE("ppi8255", i8255a_r, i8255a_w)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE(pc_page_r,				pc_page_w)
	AM_RANGE(0x00a0, 0x00a0) AM_READWRITE( pcjr_nmi_enable_r, pc_nmi_enable_w )
	AM_RANGE(0x00c0, 0x00c0) AM_DEVWRITE("sn76496", 	sn76496_w)
	AM_RANGE(0x00f0, 0x00f7) AM_READWRITE(pc_fdc_r,					pcjr_fdc_w)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc_JOY_r,					pc_JOY_w)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE("ins8250_1", ins8250_r, ins8250_w)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc_HDC1_r,				pc_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc_HDC2_r,				pc_HDC2_w)
	AM_RANGE(0x0378, 0x037f) AM_READWRITE(pc_t1t_p37x_r,			pc_t1t_p37x_w)
	AM_RANGE(0x03bc, 0x03be) AM_DEVREADWRITE("lpt_0", pc_lpt_r, pc_lpt_w)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE("ins8250_0", ins8250_r, ins8250_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ppc512_map, AS_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x7ffff) AM_RAMBANK("bank10")
	AM_RANGE(0x80000, 0xbffff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_ROM
	AM_RANGE(0xc8000, 0xcffff) AM_ROM
	AM_RANGE(0xd0000, 0xeffff) AM_NOP
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ppc640_map, AS_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK("bank10")
	AM_RANGE(0xa0000, 0xbffff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_ROM
	AM_RANGE(0xc8000, 0xcffff) AM_ROM
	AM_RANGE(0xd0000, 0xeffff) AM_NOP
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(ppc512_io, AS_IO, 16)
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE8("dma8237", i8237_r, i8237_w, 0xffff)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE8("pic8259", pic8259_r, pic8259_w, 0xffff)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE8("pit8253", pit8253_r, pit8253_w, 0xffff)
	AM_RANGE(0x0060, 0x0065) AM_READWRITE(pc1640_16le_port60_r,			pc1640_16le_port60_w)
	AM_RANGE(0x0070, 0x0071) AM_DEVREADWRITE8_MODERN("rtc", mc146818_device, read, write, 0xffff)
	AM_RANGE(0x0078, 0x0079) AM_READWRITE(pc1640_16le_mouse_x_r,			pc1640_16le_mouse_x_w)
	AM_RANGE(0x007a, 0x007b) AM_READWRITE(pc1640_16le_mouse_y_r,			pc1640_16le_mouse_y_w)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE8(pc_page_r,					pc_page_w, 0xffff)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc16le_JOY_r,					pc16le_JOY_w)
	AM_RANGE(0x0278, 0x027b) AM_READ(pc200_16le_port278_r) AM_DEVWRITE8("lpt_2", pc_lpt_w, 0x00ff)
	AM_RANGE(0x02e8, 0x02ef) AM_DEVREADWRITE8("ins8250_3", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE8("ins8250_1", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc16le_HDC1_r,				pc16le_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc16le_HDC2_r,				pc16le_HDC2_w)
	AM_RANGE(0x0378, 0x037b) AM_READ(pc200_16le_port378_r) AM_DEVWRITE8("lpt_1", pc_lpt_w, 0x00ff)
	AM_RANGE(0x03bc, 0x03bf) AM_DEVREADWRITE8("lpt_0", pc_lpt_r, pc_lpt_w, 0x00ff)
	AM_RANGE(0x03e8, 0x03ef) AM_DEVREADWRITE8("ins8250_2", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE8(pc_fdc_r,					pc_fdc_w, 0xffff)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE8("ins8250_0", ins8250_r, ins8250_w, 0xffff)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc200_map, AS_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK("bank10")
	AM_RANGE(0xa0000, 0xbffff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_ROM
	AM_RANGE(0xc8000, 0xcffff) AM_ROM
	AM_RANGE(0xd0000, 0xeffff) AM_NOP
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(pc200_io, AS_IO, 16)
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE8("dma8237", i8237_r, i8237_w, 0xffff)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE8("pic8259", pic8259_r, pic8259_w, 0xffff)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE8("pit8253", pit8253_r, pit8253_w, 0xffff)
	AM_RANGE(0x0060, 0x0065) AM_READWRITE(pc1640_16le_port60_r,			pc1640_16le_port60_w)
	AM_RANGE(0x0078, 0x0079) AM_READWRITE(pc1640_16le_mouse_x_r,			pc1640_16le_mouse_x_w)
	AM_RANGE(0x007a, 0x007b) AM_READWRITE(pc1640_16le_mouse_y_r,			pc1640_16le_mouse_y_w)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE8(pc_page_r,					pc_page_w, 0xffff)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc16le_JOY_r,					pc16le_JOY_w)
	AM_RANGE(0x0278, 0x027b) AM_READ(pc200_16le_port278_r) AM_DEVWRITE8("lpt_2", pc_lpt_w, 0x00ff)
	AM_RANGE(0x02e8, 0x02ef) AM_DEVREADWRITE8("ins8250_3", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE8("ins8250_1", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc16le_HDC1_r,				pc16le_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc16le_HDC2_r,				pc16le_HDC2_w)
	AM_RANGE(0x0378, 0x037b) AM_READ(pc200_16le_port378_r) AM_DEVWRITE8("lpt_1", pc_lpt_w, 0x00ff)
	AM_RANGE(0x03bc, 0x03bf) AM_DEVREADWRITE8("lpt_0", pc_lpt_r, pc_lpt_w, 0x00ff)
	AM_RANGE(0x03e8, 0x03ef) AM_DEVREADWRITE8("ins8250_2", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE8(pc_fdc_r,					pc_fdc_w, 0xffff)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE8("ins8250_0", ins8250_r, ins8250_w, 0xffff)
ADDRESS_MAP_END



static ADDRESS_MAP_START( pc1640_map, AS_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK("bank10")
	AM_RANGE(0xa0000, 0xbffff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_ROM
	AM_RANGE(0xc8000, 0xcffff) AM_ROM
	AM_RANGE(0xd0000, 0xeffff) AM_NOP
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START(pc1640_io, AS_IO, 16)
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE8("dma8237", i8237_r, i8237_w, 0xffff)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE8("pic8259", pic8259_r, pic8259_w, 0xffff)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE8("pit8253", pit8253_r, pit8253_w, 0xffff)
	AM_RANGE(0x0060, 0x0065) AM_READWRITE(pc1640_16le_port60_r,			pc1640_16le_port60_w)
	AM_RANGE(0x0070, 0x0071) AM_DEVREADWRITE8_MODERN("rtc", mc146818_device, read, write, 0xffff)
	AM_RANGE(0x0078, 0x0079) AM_READWRITE(pc1640_16le_mouse_x_r,	pc1640_16le_mouse_x_w)
	AM_RANGE(0x007a, 0x007b) AM_READWRITE(pc1640_16le_mouse_y_r,	pc1640_16le_mouse_y_w)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE8(pc_page_r,				pc_page_w, 0xffff)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc16le_JOY_r,				pc16le_JOY_w)
	AM_RANGE(0x0278, 0x027b) AM_DEVREADWRITE8("lpt_2", pc_lpt_r, pc_lpt_w, 0x00ff)
	AM_RANGE(0x02e8, 0x02ef) AM_DEVREADWRITE8("ins8250_3", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE8("ins8250_1", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc16le_HDC1_r,			pc16le_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc16le_HDC2_r,			pc16le_HDC2_w)
	AM_RANGE(0x0378, 0x037b) AM_READ(pc1640_16le_port378_r) AM_DEVWRITE8("lpt_1", pc_lpt_w, 0x00ff)
	AM_RANGE(0x03bc, 0x03bf) AM_DEVREADWRITE8("lpt_0", pc_lpt_r, pc_lpt_w, 0x00ff)
	AM_RANGE(0x03e8, 0x03ef) AM_DEVREADWRITE8("ins8250_2", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE8(pc_fdc_r,				pc_fdc_w, 0xffff)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE8("ins8250_0", ins8250_r, ins8250_w, 0xffff)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc1512_map, AS_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK("bank10")
	AM_RANGE(0xa0000, 0xbffff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_ROM
	AM_RANGE(0xc8000, 0xcffff) AM_ROM
	AM_RANGE(0xd0000, 0xfbfff) AM_NOP
	AM_RANGE(0xfc000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(pc1512_io, AS_IO, 16)
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE8("dma8237", i8237_r, i8237_w, 0xffff)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE8("pic8259", pic8259_r, pic8259_w, 0xffff)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE8("pit8253", pit8253_r, pit8253_w, 0xffff)
	AM_RANGE(0x0060, 0x0065) AM_READWRITE(pc1640_16le_port60_r,			pc1640_16le_port60_w)
	AM_RANGE(0x0070, 0x0071) AM_DEVREADWRITE8_MODERN("rtc", mc146818_device, read, write, 0xffff)
	AM_RANGE(0x0078, 0x0079) AM_READWRITE(pc1640_16le_mouse_x_r,	pc1640_16le_mouse_x_w)
	AM_RANGE(0x007a, 0x007b) AM_READWRITE(pc1640_16le_mouse_y_r,	pc1640_16le_mouse_y_w)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE8(pc_page_r,				pc_page_w, 0xffff)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc16le_JOY_r,				pc16le_JOY_w)
	AM_RANGE(0x0278, 0x027b) AM_DEVREADWRITE8("lpt_2", pc_lpt_r, pc_lpt_w, 0x00ff)
	AM_RANGE(0x02e8, 0x02ef) AM_DEVREADWRITE8("ins8250_3", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE8("ins8250_1", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc16le_HDC1_r,			pc16le_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc16le_HDC2_r,			pc16le_HDC2_w)
	AM_RANGE(0x0378, 0x037b) AM_READ(pc1640_16le_port378_r) AM_DEVWRITE8("lpt_1", pc_lpt_w, 0x00ff)
	AM_RANGE(0x03bc, 0x03bf) AM_DEVREADWRITE8("lpt_0", pc_lpt_r, pc_lpt_w, 0x00ff)
	AM_RANGE(0x03e8, 0x03ef) AM_DEVREADWRITE8("ins8250_2", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE8(pc_fdc_r,				pc_fdc_w, 0xffff)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE8("ins8250_0", ins8250_r, ins8250_w, 0xffff)
ADDRESS_MAP_END

static INPUT_PORTS_START( pccga )
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

	PORT_INCLUDE( kb_keytronic_pc )
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
	PORT_INCLUDE( pcvideo_cga )
INPUT_PORTS_END

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

static INPUT_PORTS_START( europc )
	PORT_START("IN0") /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

	PORT_START("DSW0") /* IN1 */

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

	EUROPC_KEYBOARD

	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
INPUT_PORTS_END

static INPUT_PORTS_START( bondwell )
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
	PORT_DIPNAME( 0x02, 0x02, "Turbo Switch" )
	PORT_DIPSETTING(	0x00, "Off (4.77 MHz)" )
	PORT_DIPSETTING(	0x02, "On (12 MHz)" )
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

//  PORT_INCLUDE( at_keyboard )     /* IN4 - IN11 */
    PORT_INCLUDE( kb_keytronic_pc )
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
	PORT_INCLUDE( pcvideo_cga )
INPUT_PORTS_END

static INPUT_PORTS_START( xtcga )
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
//  PORT_DIPNAME( 0x02, 0x02, "Turbo Switch" )
//  PORT_DIPSETTING(    0x00, "Off (4.77 MHz)" )
//  PORT_DIPSETTING(    0x02, "On (12 MHz)" )
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	PORT_INCLUDE( pc_keyboard )		/* IN4 - IN11 */
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
	PORT_INCLUDE( pcvideo_cga )
INPUT_PORTS_END

static INPUT_PORTS_START( tandy1t )
	PORT_START("IN0") /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

	PORT_START("DSW0") /* IN1 */
	PORT_BIT ( 0xff, 0xff,	 IPT_UNUSED )

	PORT_START("DSW1") /* IN2 */
	PORT_DIPNAME( 0x80, 0x80, "COM1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "COM2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BIT ( 0x30, 0x00,	 IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x08, "LPT1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BIT ( 0x06, 0x00,	 IPT_UNUSED )
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

	PORT_INCLUDE( t1000_keyboard )
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
INPUT_PORTS_END

static INPUT_PORTS_START( pc200 )
	PORT_START("IN0") /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

	PORT_START("DSW0") /* IN1 */
	PORT_DIPNAME( 0x07, 0x07, "Name/Language")
	PORT_DIPSETTING(	0x00, "English/less checks" )
	PORT_DIPSETTING(	0x01, DEF_STR( Italian ) ) //prego attendere
	PORT_DIPSETTING(	0x02, "V.g. v\xC3\xA4nta" )
	PORT_DIPSETTING(	0x03, "Vent et cjeblik" ) // seldom c
	PORT_DIPSETTING(	0x04, DEF_STR( Spanish ) ) //Por favor
	PORT_DIPSETTING(	0x05, DEF_STR( French ) ) //patientez
	PORT_DIPSETTING(	0x06, DEF_STR( German ) ) // bitte warten
	PORT_DIPSETTING(	0x07, DEF_STR( English ) ) // please wait
	PORT_DIPNAME( 0x08, 0x00, "37a 0x40")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x08, "0x08" )
/* 2008-05 FP: This Dip Switch overlaps the next one.
Since pc200 is anyway NOT_WORKING, I comment out this one */
/*  PORT_DIPNAME( 0x10, 0x00, "37a 0x80")
    PORT_DIPSETTING(    0x00, "0x00" )
    PORT_DIPSETTING(    0x10, "0x10" ) */
	PORT_DIPNAME( 0x30, 0x00, "Integrated Graphics Adapter")
	PORT_DIPSETTING(	0x00, "CGA 1" )
	PORT_DIPSETTING(	0x10, "CGA 2" )
	PORT_DIPSETTING(	0x20, "external" )
	PORT_DIPSETTING(	0x30, "MDA" )
	PORT_DIPNAME( 0xc0, 0x80, "Startup Mode")
	PORT_DIPSETTING(	0x00, "external Color 80 Columns" )
	PORT_DIPSETTING(	0x40, "Color 40 Columns" )
	PORT_DIPSETTING(	0x80, "Color 80 Columns" )
	PORT_DIPSETTING(	0xc0, DEF_STR( Mono ) )

	PORT_START("DSW1") /* IN2 */
	PORT_BIT ( 0x80, 0x80,	 IPT_UNUSED ) // com 1 on motherboard
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
	PORT_BIT ( 0x04, 0x04,	 IPT_UNUSED ) // lpt 1 on motherboard
	PORT_DIPNAME( 0x02, 0x00, "LPT3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x00, "Game port enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

	PORT_START("DSW2") /* IN3 */
	PORT_DIPNAME( 0xf0, 0x00, "Serial mouse")
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

	PORT_INCLUDE( at_keyboard )		/* IN4 - IN11 */
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */

	PORT_START("VIDEO") /* IN20 */
	PORT_CONFNAME( 0x03, 0x03, "IDA character set")
	PORT_CONFSETTING(0x00, "Greek")
	PORT_CONFSETTING(0x01, "Norwegian (Codepage 860)")
	PORT_CONFSETTING(0x02, "Portugese (Codepage 865)")
	PORT_CONFSETTING(0x03, "Default (Codepage 437)")
	PORT_CONFNAME( 0x1C, 0x00, "CGA monitor type")
	PORT_CONFSETTING(0x00, "Colour RGB")
	PORT_CONFSETTING(0x04, "Mono RGB")
	PORT_CONFSETTING(0x0C, "Television")
	PORT_BIT ( 0xE0, 0x40, IPT_UNUSED )	/* Chipset is always PPC512 */

INPUT_PORTS_END

static INPUT_PORTS_START( pc1512 )
	PORT_START("IN0") /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

	PORT_START("DSW0") /* IN1 */
	PORT_DIPNAME( 0x07, 0x07, "Name/Language")
	PORT_DIPSETTING(	0x00, "English/less checks" )
	PORT_DIPSETTING(	0x01, DEF_STR( Italian ) ) //prego attendere
	PORT_DIPSETTING(	0x02, "V.g. v\xC3\xA4nta" )
	PORT_DIPSETTING(	0x03, "Vent et cjeblik" ) // seldom c
	PORT_DIPSETTING(	0x04, DEF_STR( Spanish ) ) //Por favor
	PORT_DIPSETTING(	0x05, DEF_STR( French ) ) //patientez
	PORT_DIPSETTING(	0x06, DEF_STR( German ) ) // bitte warten
	PORT_DIPSETTING(	0x07, DEF_STR( English ) ) // please wait
	PORT_BIT( 0x20, 0x20,	IPT_UNUSED ) // pc1512 integrated special cga
	PORT_BIT( 0xc0, 0x00,	IPT_UNUSED ) // not used in pc1512
	PORT_BIT( 0xe00, 0x00,	IPT_UNUSED ) // not used in pc1512
	PORT_BIT( 0xe000, 0x00,	IPT_UNUSED ) // not used in pc1512

	PORT_START("DSW1") /* IN2 */
	PORT_BIT ( 0x80, 0x80,	 IPT_UNUSED ) // com 1 on motherboard
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
	PORT_BIT ( 0x04, 0x04,	 IPT_UNUSED ) // lpt 1 on motherboard
	PORT_DIPNAME( 0x02, 0x00, "LPT3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x00, "Game port enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

	PORT_START("DSW2") /* IN3 */
	PORT_DIPNAME( 0xf0, 0x00, "Serial mouse")
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

	PORT_INCLUDE( amstrad_keyboard )	/* IN4 - IN14 */

	PORT_INCLUDE( pc_joystick_none )

	PORT_INCLUDE( pcvideo_pc1512 )
INPUT_PORTS_END

static INPUT_PORTS_START( pc1640 )
	PORT_START("IN0")	/* IN0 */
	PORT_DIPNAME( 0x08, 0x08, "VGA 1")
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "VGA 2")
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "VGA 3")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "VGA 4")
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Paradise EGA 5")
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Paradise EGA 6")
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Paradise EGA 7")
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Paradise EGA 8")
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START("DSW0") /* IN1 */
	PORT_DIPNAME( 0x07, 0x07, "Name/Language")
//  PORT_DIPSETTING(    0x00, "PC 512k" ) // machine crashes with ega bios at 0xc0000
	PORT_DIPSETTING(	0x01, DEF_STR( Italian ) ) //prego attendere
	PORT_DIPSETTING(	0x02, "V.g. v\xC3\xA4nta" )
	PORT_DIPSETTING(	0x03, "Vent et cjeblik" ) // seldom c
	PORT_DIPSETTING(	0x04, DEF_STR( Spanish ) ) //Por favor
	PORT_DIPSETTING(	0x05, DEF_STR( French ) ) //patientez
	PORT_DIPSETTING(	0x06, DEF_STR( German ) ) // bitte warten
	PORT_DIPSETTING(	0x07, DEF_STR( English ) ) // please wait
	PORT_BIT( 0x20, 0x00,	IPT_UNUSED ) // not pc1512 integrated special cga
	PORT_DIPNAME( 0x40, 0x00, "37a 0x40")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x40, "0x40" )
	PORT_DIPNAME( 0x80, 0x00, "37a 0x80")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x80, "0x80" )
#if 0
	PORT_DIPNAME( 0x200, 0x00, "37a 0x20 after 27[8a] read (font?)")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x200, "0x20" )
	PORT_DIPNAME( 0x400, 0x00, "37a 0x40 after 27[8a] read (font?)")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x400, "0x40" )
	PORT_DIPNAME( 0x800, 0x00, "37a 0x80 after 27[8a] read (font?)")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x800, "0x80" )
	PORT_DIPNAME( 0x2000, 0x00, "37a 0x20 after 427a read")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x2000, "0x20" )
	PORT_DIPNAME( 0x4000, 0x00, " 37a 0x40 after 427a read")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x4000, "0x40" )
	PORT_DIPNAME( 0x8000, 0x00, " 37a 0x80 after 427a read")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x8000, "0x80" )
#else
	PORT_DIPNAME( 0xe00, 0x600, "37a after 427a read")
	PORT_DIPSETTING(	0x000, "?0standard" ) // diaresis a, seldom c, acut u, circumflex e, grave a, acute e
	PORT_DIPSETTING(	0x200, "?scandinavian" ) //diaresis a, o slashed, acute u, circumflex e, grave a, acute e
	PORT_DIPSETTING(	0x600, "?spain" ) // tilde a, seldom c, acute u, circumflex e, grave a, acute e
	PORT_DIPSETTING(	0xa00, "?greeck" ) // E, circumflex ???,micro, I, Z, big kappa?
	PORT_DIPSETTING(	0xe00, "?standard" ) // diaresis a, seldom e, acute u, circumflex e, grave a, acute e
	PORT_DIPNAME( 0xe000, 0x00, "37a after 427a read")
	PORT_DIPSETTING(	0x0000, "?Integrated Graphic Adapter IGA (EGA)" )
	PORT_DIPSETTING(	0x2000, "?External EGA/VGA" )
	PORT_DIPSETTING(	0x6000, "CGA 40 Columns" )
	PORT_DIPSETTING(	0xa000, "CGA 80 Columns" )
	PORT_DIPSETTING(	0xe000, "MDA/Hercules/Multiple Graphic Adapters" )
#endif

	PORT_START("DSW1") /* IN2 */
	PORT_BIT ( 0x80, 0x80,	 IPT_UNUSED ) // com 1 on motherboard
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
	PORT_BIT ( 0x04, 0x04,	 IPT_UNUSED ) // lpt 1 on motherboard
	PORT_DIPNAME( 0x02, 0x00, "LPT3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x00, "Game port enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

	PORT_START("DSW2") /* IN3 */
	PORT_DIPNAME( 0xf0, 0x00, "Serial mouse")
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

	PORT_INCLUDE( amstrad_keyboard )	/* IN4 - IN14 */

//  PORT_INCLUDE( pc_mouse_microsoft )  /* IN12 - IN14 */

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

static GFXDECODE_START( ibm5150 )
	GFXDECODE_ENTRY( "gfx1", 0x0000, pc_16_charlayout, 3, 1 )
	GFXDECODE_ENTRY( "gfx1", 0x1000, pc_8_charlayout, 3, 1 )
GFXDECODE_END


static MACHINE_CONFIG_START( pccga, pc_state )
	/* basic machine hardware */
	MCFG_CPU_PC(pc8, pc8, I8088, 4772720, pc_frame_interrupt)	/* 4,77 MHz */

	MCFG_QUANTUM_TIME(attotime::from_hz(60))

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

static MACHINE_CONFIG_START( mc1502, pc_state )
	/* basic machine hardware */
	MCFG_CPU_PC(mc1502, pc8, I8088, 4772720, pc_frame_interrupt)	/* 4,77 MHz */

	MCFG_QUANTUM_TIME(attotime::from_hz(60))

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
	MCFG_FRAGMENT_ADD( pcvideo_cga32k )
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

static const gfx_layout europc_8_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*16					/* every char takes 16 bytes */
};

static const gfx_layout europc_16_charlayout =
{
	8, 16,					/* 8 x 16 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( europc )
	GFXDECODE_ENTRY( "gfx1", 0x0000, europc_8_charlayout, 3, 1 )
	GFXDECODE_ENTRY( "gfx1", 0x0800, europc_16_charlayout, 3, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( europc, pc_state )
	/* basic machine hardware */
	MCFG_CPU_PC(europc, europc, I8088, 4772720*2, pc_frame_interrupt)

	MCFG_MACHINE_START(pc)
	MCFG_MACHINE_RESET(pc)

	MCFG_PIT8253_ADD( "pit8253", ibm5150_pit8253_config )

	MCFG_I8237_ADD( "dma8237", XTAL_14_31818MHz/3, ibm5150_dma8237_config )

	MCFG_PIC8259_ADD( "pic8259", ibm5150_pic8259_config )

	MCFG_I8255A_ADD( "ppi8255", pc_ppi8255_interface )

	MCFG_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_2", ibm5150_com_interface[2] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_3", ibm5150_com_interface[3] )			/* TODO: Verify model */

	/* video hardware */
	MCFG_FRAGMENT_ADD( pcvideo_aga )
	MCFG_GFXDECODE(europc)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MCFG_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MCFG_SOUND_CONFIG(pc_ym3812_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif

	MCFG_NVRAM_HANDLER( europc_rtc )

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


static const gfx_layout pc200_charlayout =
{
	8, 16,					/* 8 x 16 characters */
	2048,					/* 2048 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( pc200 )
	GFXDECODE_ENTRY( "gfx1", 0x0000, pc200_charlayout, 3, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( pc200, pc_state )
	/* basic machine hardware */
	MCFG_CPU_PC(pc200, pc200, I8086, 8000000, pc_frame_interrupt)

	MCFG_MACHINE_START(pc)
	MCFG_MACHINE_RESET(pc)

	MCFG_PIT8253_ADD( "pit8253", ibm5150_pit8253_config )

	MCFG_I8237_ADD( "dma8237", XTAL_14_31818MHz/3, ibm5150_dma8237_config )

	MCFG_PIC8259_ADD( "pic8259", ibm5150_pic8259_config )

	MCFG_I8255A_ADD( "ppi8255", pc_ppi8255_interface )

	MCFG_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_2", ibm5150_com_interface[2] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_3", ibm5150_com_interface[3] )			/* TODO: Verify model */

	/* video hardware */
	MCFG_FRAGMENT_ADD( pcvideo_pc200 )
	MCFG_GFXDECODE(pc200)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

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




static const gfx_layout pc1512_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	1024,					/* 1024 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( pc1512 )
	GFXDECODE_ENTRY( "gfx1", 0x0000, pc1512_charlayout, 3, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( ppc512, pc_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", V30, 8000000)
	MCFG_CPU_PROGRAM_MAP(ppc512_map)
	MCFG_CPU_IO_MAP(ppc512_io)
	MCFG_CPU_VBLANK_INT_HACK(pc_frame_interrupt, 4)


	MCFG_MACHINE_START(pc)
	MCFG_MACHINE_RESET(pc)

	MCFG_PIT8253_ADD( "pit8253", ibm5150_pit8253_config )

	MCFG_I8237_ADD( "dma8237", XTAL_14_31818MHz/3, ibm5150_dma8237_config )

	MCFG_PIC8259_ADD( "pic8259", ibm5150_pic8259_config )

	MCFG_I8255A_ADD( "ppi8255", pc_ppi8255_interface )

	MCFG_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_2", ibm5150_com_interface[2] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_3", ibm5150_com_interface[3] )			/* TODO: Verify model */

	/* video hardware */
	MCFG_FRAGMENT_ADD( pcvideo_pc200 )
	MCFG_GFXDECODE(pc200)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* printer */
	MCFG_PC_LPT_ADD("lpt_0", pc_lpt_config)
	MCFG_PC_LPT_ADD("lpt_1", pc_lpt_config)
	MCFG_PC_LPT_ADD("lpt_2", pc_lpt_config)

	/* harddisk */
	MCFG_FRAGMENT_ADD( pc_hdc )

	MCFG_UPD765A_ADD("upd765", pc_fdc_upd765_not_connected_interface)

	MCFG_FLOPPY_2_DRIVES_ADD(ibmpc_floppy_config)

	MCFG_MC146818_ADD( "rtc", MC146818_IGNORE_CENTURY )

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("512K")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( ppc640, ppc512 )
	MCFG_CPU_MODIFY("maincpu")
	MCFG_CPU_PROGRAM_MAP(ppc640_map)

	/* internal ram */
	MCFG_RAM_MODIFY(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("640K")
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( pc1512, pc_state )
	/* basic machine hardware */
	MCFG_CPU_PC(pc1512, pc1512, I8086, 8000000, pc_frame_interrupt)

	MCFG_MACHINE_START(pc)
	MCFG_MACHINE_RESET(pc)

	MCFG_PIT8253_ADD( "pit8253", ibm5150_pit8253_config )

	MCFG_I8237_ADD( "dma8237", XTAL_14_31818MHz/3, ibm5150_dma8237_config )

	MCFG_PIC8259_ADD( "pic8259", ibm5150_pic8259_config )

	MCFG_I8255A_ADD( "ppi8255", pc_ppi8255_interface )

	MCFG_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_2", ibm5150_com_interface[2] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_3", ibm5150_com_interface[3] )			/* TODO: Verify model */

	/* video hardware */
	MCFG_FRAGMENT_ADD( pcvideo_pc1512 )
	MCFG_GFXDECODE(pc1512)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MCFG_MC146818_ADD( "rtc", MC146818_IGNORE_CENTURY )

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


static MACHINE_CONFIG_START( pc1640, pc_state )
	/* basic machine hardware */
	MCFG_CPU_PC(pc1640, pc1640, I8086, 8000000, pc_vga_frame_interrupt)

	MCFG_MACHINE_START(pc)
	MCFG_MACHINE_RESET(pc)

	MCFG_PIT8253_ADD( "pit8253", ibm5150_pit8253_config )

	MCFG_I8237_ADD( "dma8237", XTAL_14_31818MHz/3, ibm5150_dma8237_config )

	MCFG_PIC8259_ADD( "pic8259", ibm5150_pic8259_config )

	MCFG_I8255A_ADD( "ppi8255", pc_ppi8255_interface )

	MCFG_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_2", ibm5150_com_interface[2] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_3", ibm5150_com_interface[3] )			/* TODO: Verify model */

	/* video hardware */
	MCFG_FRAGMENT_ADD( pcvideo_ega )
	MCFG_GFXDECODE(pc1512)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MCFG_MC146818_ADD( "rtc", MC146818_IGNORE_CENTURY )

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


static MACHINE_CONFIG_START( t1000hx, pc_state )
	/* basic machine hardware */
	MCFG_CPU_PC(tandy1000, tandy1000, I8088, 8000000, pc_frame_interrupt)

	MCFG_MACHINE_START(pc)
	MCFG_MACHINE_RESET(pc)

	MCFG_PIT8253_ADD( "pit8253", ibm5150_pit8253_config )

	MCFG_I8237_ADD( "dma8237", XTAL_14_31818MHz/3, ibm5150_dma8237_config )

	MCFG_PIC8259_ADD( "pic8259", ibm5150_pic8259_config )

	MCFG_I8255A_ADD( "ppi8255", pc_ppi8255_interface )

	MCFG_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */

	/* video hardware */
	MCFG_FRAGMENT_ADD( pcvideo_t1000 )
	MCFG_GFXDECODE(europc)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_ADD("sn76496", NCR7496, 2386360)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MCFG_NVRAM_HANDLER( tandy1000 )

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


static MACHINE_CONFIG_START( t1000_16, pc_state )
	/* basic machine hardware */
	MCFG_CPU_PC(tandy1000_16, tandy1000_16, I8086, XTAL_28_63636MHz / 3, pc_frame_interrupt)

	MCFG_MACHINE_START(pc)
	MCFG_MACHINE_RESET(tandy1000rl)

	MCFG_PIT8253_ADD( "pit8253", ibm5150_pit8253_config )

	MCFG_I8237_ADD( "dma8237", XTAL_14_31818MHz/3, ibm5150_dma8237_config )

	MCFG_PIC8259_ADD( "pic8259", ibm5150_pic8259_config )

	MCFG_I8255A_ADD( "ppi8255", pc_ppi8255_interface )

	MCFG_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */

	/* video hardware */
	MCFG_FRAGMENT_ADD( pcvideo_t1000 )
	MCFG_GFXDECODE(europc)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_ADD("sn76496", NCR7496, 2386360)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MCFG_NVRAM_HANDLER( tandy1000 )

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


static MACHINE_CONFIG_START( t1000_286, pc_state )
	/* basic machine hardware */
	MCFG_CPU_PC(tandy1000_286, tandy1000_286, I80286, XTAL_28_63636MHz / 2, pc_frame_interrupt)

	MCFG_MACHINE_START(pc)
	MCFG_MACHINE_RESET(pc)

	MCFG_PIT8253_ADD( "pit8253", ibm5150_pit8253_config )

	MCFG_I8237_ADD( "dma8237", XTAL_14_31818MHz/3, ibm5150_dma8237_config )

	MCFG_PIC8259_ADD( "pic8259", ibm5150_pic8259_config )

	MCFG_I8255A_ADD( "ppi8255", pc_ppi8255_interface )

	MCFG_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */

	/* video hardware */
	MCFG_FRAGMENT_ADD( pcvideo_t1000 )
	MCFG_GFXDECODE(europc)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_ADD("sn76496", NCR7496, 2386360)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MCFG_NVRAM_HANDLER( tandy1000 )

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


static GFXDECODE_START( ibmpcjr )
	GFXDECODE_ENTRY( "gfx1", 0x0000, pc_8_charlayout, 3, 1 )
GFXDECODE_END

static const cassette_config ibm5150_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_PLAY | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED),
	NULL
};

static MACHINE_CONFIG_START( ibmpcjr, pc_state )
	/* basic machine hardware */
	MCFG_CPU_PC(ibmpcjr, ibmpcjr, I8088, 4900000, pcjr_frame_interrupt)	/* TODO: Get correct cpu frequency, probably XTAL_14_31818MHz/3 */

	MCFG_MACHINE_START(pcjr)
	MCFG_MACHINE_RESET(pcjr)

	MCFG_PIT8253_ADD( "pit8253", pcjr_pit8253_config )

	MCFG_PIC8259_ADD( "pic8259", pcjr_pic8259_config )

	MCFG_I8255A_ADD( "ppi8255", pcjr_ppi8255_interface )

	MCFG_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MCFG_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */

	/* video hardware */
	MCFG_FRAGMENT_ADD( pcvideo_pcjr )
	MCFG_GFXDECODE(ibmpcjr)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_ADD("sn76496", SN76496, 2386360)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MCFG_NVRAM_HANDLER( tandy1000 )

	/* printer */
	MCFG_PC_LPT_ADD("lpt_0", pc_lpt_config)
	MCFG_PC_LPT_ADD("lpt_1", pc_lpt_config)
	MCFG_PC_LPT_ADD("lpt_2", pc_lpt_config)

	/* cassette */
	MCFG_CASSETTE_ADD( "cassette", ibm5150_cassette_config )

	MCFG_UPD765A_ADD("upd765", pc_fdc_upd765_not_connected_interface)

	MCFG_FLOPPY_2_DRIVES_ADD(ibmpc_floppy_config)

	/* cartridge */
	MCFG_CARTSLOT_ADD("cart1")
	MCFG_CARTSLOT_INTERFACE("ibmpcjr_cart")
	MCFG_CARTSLOT_EXTENSION_LIST("jrc")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(pcjr_cartridge)
	MCFG_CARTSLOT_ADD("cart2")
	MCFG_CARTSLOT_INTERFACE("ibmpcjr_cart")
	MCFG_CARTSLOT_EXTENSION_LIST("jrc")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(pcjr_cartridge)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("640K")

	/* Software lists */
	MCFG_SOFTWARE_LIST_ADD("cart_list","ibmpcjr_cart")
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( iskr1031, pc_state )
	/* basic machine hardware */
	MCFG_CPU_PC(pc16, pc16, I8086, 4772720, pc_frame_interrupt)

	MCFG_QUANTUM_TIME(attotime::from_hz(60))

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


static MACHINE_CONFIG_START( poisk2, pc_state )
	/* basic machine hardware */
	MCFG_CPU_PC(pc16, pc16, I8086, 4772720, pc_frame_interrupt)

	MCFG_QUANTUM_TIME(attotime::from_hz(60))

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
	MCFG_FRAGMENT_ADD( pcvideo_poisk2 )
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

static MACHINE_CONFIG_START( zenith, pc_state )
	/* basic machine hardware */
	MCFG_CPU_PC(zenith, pc8, I8088, XTAL_14_31818MHz/3, pc_frame_interrupt)	/* 4,77 MHz */

	MCFG_QUANTUM_TIME(attotime::from_hz(60))

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

static MACHINE_CONFIG_START( olivetti, pc_state )
	/* basic machine hardware */
	MCFG_CPU_PC(pc16, pc16, I8086, 8000000, pc_vga_frame_interrupt)

	MCFG_QUANTUM_TIME(attotime::from_hz(60))

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

#if 0
	//pcjr roms? (incomplete dump, most likely 64 kbyte)
	// basic c1.20
	ROM_LOAD("basic.rom", 0xf6000, 0x8000, CRC(0c19c1a8))
	// ???
	ROM_LOAD("bios.rom", 0x??000, 0x2000, CRC(98463f95))

	/* turbo xt */
	/* basic c1.10 */
	ROM_LOAD("rom05.bin", 0xf6000, 0x2000, CRC(80d3cf5d))
	ROM_LOAD("rom04.bin", 0xf8000, 0x2000, CRC(673a4acc))
	ROM_LOAD("rom03.bin", 0xfa000, 0x2000, CRC(aac3fc37))
	ROM_LOAD("rom02.bin", 0xfc000, 0x2000, CRC(3062b3fc))
	/* sw1 0x60 readback fails write 301 to screen fe3b7 */
	/* disk problems no disk gives 601 */
	/* 5000-026 08/16/82 */
	ROM_LOAD("rom01.bin", 0xfe000, 0x2000, CRC(5c3f0256))

	/* anonymous works nice */
	ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, CRC(031aafad))

	ROM_LOAD("bondwell.bin", 0xfe000, 0x2000, CRC(d435a405))

	/* europc */
	ROM_LOAD("50145", 0xf8000, 0x8000, CRC(1775a11d)) // V2.07
//  ROM_LOAD("eurobios.bin", 0xf8000, 0x8000, CRC(52185223)) scrap
	/* cga, hercules character set */
	ROM_LOAD("50146", 0x00000, 0x02000, CRC(1305dcf5)) //D1.0

	// ibm pc
	// most likely 8 kbyte chips
	ROM_LOAD("basicpc.bin", 0xf6000, 0x8000, CRC(ebacb791)) // IBM C1.1
	// split into 8 kbyte parts
	// the same as in the basic c1.10 as in the turboxt
	// 1501-476 10/27/82
	ROM_LOAD("biospc.bin", 0xfe000, 0x2000, CRC(e88792b3))

	/* tandy 1000 hx */
	ROM_LOAD("tandy1t.rom", 0xf0000, 0x10000, CRC(d37a1d5f))

	// ibm xt
	ROM_LOAD("xthdd.c8", 0xc8000, 0x2000, CRC(a96317da))
	ROM_LOAD("biosxt.bin", 0xf0000, 0x10000, CRC(36c32fde)) // BASIC C1.1
	// split into 2 chips for 16 bit access
	ROM_LOAD_EVEN("ibmxt.0", 0xf0000, 0x8000, CRC(83727c42))
	ROM_LOAD_ODD("ibmxt.1", 0xf0000, 0x8000, CRC(2a629953))

	/* pc xt mfm controller
       2 harddisks 17 sectors, 4 head, 613 tracks
       serves 2 controllers? 0x320-3, 0x324-7, dma 3, irq5
       movable, works at 0xee000 */
	/* western digital 06/28/89 */
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */

	/* lcs 6210d asic i2.1 09/01/1988 */
	/* problematic, currently showing menu and calls int21 (hangs)! */
	ROM_LOAD("xthdd.rom",  0xc8000, 0x02000, CRC(a96317da))
#endif

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

ROM_START( bw230 )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */ // taken from other machine
	ROM_LOAD("bondwell.bin", 0xfe000, 0x2000, CRC(d435a405) SHA1(a57c705d1144c7b61940b6f5c05d785c272fc9bb))

	/* IBM 1501981(CGA) and 1501985(MDA) Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f)) /* "AMI 8412PI // 5788005 // (C) IBM CORP. 1981 // KOREA" */
ROM_END


ROM_START( pc )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */
//  ROM_LOAD("xthdd.rom",  0xc8000, 0x02000, CRC(a96317da))
	ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, CRC(031aafad) SHA1(a641b505bbac97b8775f91fe9b83d9afdf4d038f))

	/* IBM 1501981(CGA) and 1501985(MDA) Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f)) /* "AMI 8412PI // 5788005 // (C) IBM CORP. 1981 // KOREA" */
ROM_END


ROM_START( zdsupers )
    ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD( "z184m v3.1d.10d", 0xf8000, 0x8000, CRC(44012c3b) SHA1(f2f28979798874386ca8ba3dd3ead24ae7c2aeb4))
	/* IBM 1501981(CGA) and 1501985(MDA) Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f)) /* "AMI 8412PI // 5788005 // (C) IBM CORP. 1981 // KOREA" */
ROM_END

ROM_START( europc )
	ROM_REGION(0x100000,"maincpu", 0)
	// hdd bios integrated!
	ROM_LOAD("50145", 0xf8000, 0x8000, CRC(1775a11d) SHA1(54430d4d0462860860397487c9c109e6f70db8e3)) // V2.07
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("50146 char d1.0 euro.u16", 0x00000, 0x02000, CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //D1.0
ROM_END


ROM_START( ibmpcjr )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("bios.rom", 0xf0000, 0x10000,CRC(31e3a7aa) SHA1(1f5f7013f18c08ff50d7942e76c4fbd782412414))

	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd)) // from an unknown clone cga card
ROM_END

ROM_START( ibmpcjx )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("5601jda.bin", 0xf0000, 0x10000, CRC(b1e12366) SHA1(751feb16b985aa4f1ec1437493ff77e2ebd5e6a6))
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd)) // from an unknown clone cga card
ROM_END

#ifdef UNUSED_DEFINITION
ROM_START( t1000 )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */	// not sure about this one
	// partlist says it has 1 128kbyte rom
	ROM_LOAD("t1000hx.e0", 0xe0000, 0x10000, CRC(61dbf242) SHA1(555b58d8aa8e0b0839259621c44b832d993beaef))	// not sure about this one
	ROM_SYSTEM_BIOS( 0, "v010000", "v010000" )
	ROMX_LOAD("v010000.f0", 0xf0000, 0x10000, NO_DUMP, ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v010100", "v010100" )
	ROMX_LOAD("v010100.f0", 0xf0000, 0x10000, CRC(b6760881) SHA1(8275e4c48ac09cf36685db227434ca438aebe0b9), ROM_BIOS(2))
	ROM_REGION(0x08000,"gfx1", 0)
	// expects 8x9 charset!
	ROM_LOAD("50146", 0x00000, 0x02000, BAD_DUMP CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //taken from europc, 9th blank
ROM_END

ROM_START( t1000a )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */	// not sure about this one
	// partlist says it has 1 128kbyte rom
	ROM_LOAD("t1000hx.e0", 0xe0000, 0x10000, CRC(61dbf242) SHA1(555b58d8aa8e0b0839259621c44b832d993beaef))	// not sure about this one
	ROM_LOAD("v010100.f0", 0xf0000, 0x10000, CRC(b6760881) SHA1(8275e4c48ac09cf36685db227434ca438aebe0b9))
	ROM_REGION(0x08000,"gfx1", 0)
	// expects 8x9 charset!
	ROM_LOAD("50146", 0x00000, 0x02000, BAD_DUMP CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //taken from europc, 9th blank
ROM_END

ROM_START( t1000ex )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */	// not sure about this one
	// partlist says it has 1 128kbyte rom
	ROM_LOAD("t1000hx.e0", 0xe0000, 0x10000, CRC(61dbf242) SHA1(555b58d8aa8e0b0839259621c44b832d993beaef))	// not sure about this one
	ROM_LOAD("v010200.f0", 0xf0000, 0x10000, CRC(0e016ecf) SHA1(2f5ac8921b7cba56b02122ef772f5f11bbf6d8a2))
	ROM_REGION(0x08000,"gfx1", 0)
	// expects 8x9 charset!
	ROM_LOAD("50146", 0x00000, 0x02000, BAD_DUMP CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //taken from europc, 9th blank
ROM_END
#endif

ROM_START( t1000hx )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */
	// partlist says it has 1 128kbyte rom
	ROM_LOAD("t1000hx.e0", 0xe0000, 0x10000, CRC(61dbf242) SHA1(555b58d8aa8e0b0839259621c44b832d993beaef))
	ROM_LOAD("v020000.f0", 0xf0000, 0x10000, CRC(d37a1d5f) SHA1(5ec031c31a7967cc3fd53a535d81833e4a1c385e))
	ROM_REGION(0x08000,"gfx1", 0)
	// expects 8x9 charset!
	ROM_LOAD("50146", 0x00000, 0x02000, BAD_DUMP CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //taken from europc, 9th blank
ROM_END

#ifdef UNUSED_DEFINITION
ROM_START( t1000sl )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */	// not sure about this one
	// partlist says it has 1 128kbyte rom
	ROM_LOAD("t1000hx.e0", 0xe0000, 0x10000, CRC(61dbf242) SHA1(555b58d8aa8e0b0839259621c44b832d993beaef))	// not sure about this one
	ROM_SYSTEM_BIOS( 0, "v010400", "v010400" )
	ROMX_LOAD("v010400.f0", 0xf0000, 0x10000, NO_DUMP, ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "v010401", "v010401" )
	ROMX_LOAD("v010401.f0", 0xf0000, 0x10000, NO_DUMP, ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "v010402", "v010402" )
	ROMX_LOAD("v010402.f0", 0xf0000, 0x10000, NO_DUMP, ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "v020001", "v020001" )
	ROMX_LOAD("v020001.f0", 0xf0000, 0x10000, NO_DUMP, ROM_BIOS(4) )
	ROM_REGION(0x08000,"gfx1", 0)
	// expects 8x9 charset!
	ROM_LOAD("50146", 0x00000, 0x02000, BAD_DUMP CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //taken from europc, 9th blank
ROM_END

ROM_START( t1000sl2 )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */	// not sure about this one
	// partlist says it has 1 128kbyte rom
	ROM_LOAD("t1000hx.e0", 0xe0000, 0x10000, CRC(61dbf242) SHA1(555b58d8aa8e0b0839259621c44b832d993beaef))	// not sure about this one
	ROM_LOAD("v010404.f0", 0xf0000, 0x10000, NO_DUMP )
	ROM_REGION(0x08000,"gfx1", 0)
	// expects 8x9 charset!
	ROM_LOAD("50146", 0x00000, 0x02000, BAD_DUMP CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //taken from europc, 9th blank
ROM_END
#endif

ROM_START( t1000sx )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */	// not sure about this one
	// partlist says it has 1 128kbyte rom
	ROM_LOAD("t1000hx.e0", 0xe0000, 0x10000, CRC(61dbf242) SHA1(555b58d8aa8e0b0839259621c44b832d993beaef))	// not sure about this one
	ROM_LOAD("v010200.f0", 0xf0000, 0x10000, CRC(0e016ecf) SHA1(2f5ac8921b7cba56b02122ef772f5f11bbf6d8a2))
	ROM_REGION(0x08000,"gfx1", 0)
	// expects 8x9 charset!
	ROM_LOAD("50146", 0x00000, 0x02000, BAD_DUMP CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //taken from europc, 9th blank
ROM_END


ROM_START( t1000tx )
    ROM_REGION(0x100000,"maincpu", 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */   // not sure about this one
	/* There might be a second 32KB rom, but it seems to work fine with just this one */
    ROM_LOAD("t1000tx.bin", 0xf8000, 0x8000, CRC(9b34765c) SHA1(0b07e87f6843393f7d4ca4634b832b0c0bec304e))

    ROM_REGION(0x08000,"gfx1", 0)
    // expects 8x9 charset!
    ROM_LOAD("50146", 0x00000, 0x02000, BAD_DUMP CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //taken from europc, 9th blank
ROM_END


ROM_START( t1000rl )
	ROM_REGION(0x100000,"maincpu", 0)
//  ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */   // not sure about this one

	/* v2.0.0.1 */
	/* Rom is labeled "(C) TANDY CORP. 1990 // 8079073 // LH534G70 JAPAN // 9034 D" */
	ROM_LOAD("8079073.u23", 0x00000, 0x80000, CRC(6fab50f7) SHA1(2ccc02bee4c250dc1b7c17faef2590bc158860b0) )

	ROM_REGION(0x08000,"gfx1", 0)
	// expects 8x9 charset!
	/* Character rom located at U3 w/label "8079027 // NCR // 609-2495004 // F841030 A9025" */
	ROM_LOAD("50146", 0x00000, 0x02000, BAD_DUMP CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //taken from europc, 9th blank
ROM_END


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


/*
Sinclair PC200 ROMs (from a v1.2 PC200):

40109.ic159     -- Character set, the same as in the 1.5 PC200. Label:

            AMSTRAD
            40109
            8827 B

40184.ic132 -- BIOS v1.2.
40185.ic129    Labels are:

            AMSTRAD         AMSTRAD
            40184           40185
            V1.2:5EA8       V1.2:A058
*/
ROM_START( pc200 )
//  ROM_REGION(0x100000,"maincpu", 0)
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */
	// special bios at 0xe0000 !?
	ROM_SYSTEM_BIOS(0, "v15", "v1.5")
	ROMX_LOAD("40185-2.ic129", 0xfc001, 0x2000, CRC(41302eb8) SHA1(8b4b2afea543b96b45d6a30365281decc15f2932), ROM_SKIP(1) | ROM_BIOS(1)) // v2
	ROMX_LOAD("40184-2.ic132", 0xfc000, 0x2000, CRC(71b84616) SHA1(4135102a491b25fc659d70b957e07649f3eacf24), ROM_SKIP(1) | ROM_BIOS(1)) // v2
	ROM_SYSTEM_BIOS(1, "v12", "v1.2")
	ROMX_LOAD("40185.ic129", 0xfc001, 0x2000, CRC(c2b4eeac) SHA1(f11015fadf0c16d86ce2c5047be3e6a4782044f7), ROM_SKIP(1) | ROM_BIOS(2))
	ROMX_LOAD("40184.ic132", 0xfc000, 0x2000, CRC(b22704a6) SHA1(dadd573db6cd34f339f2f0ae55b07537924c024a), ROM_SKIP(1) | ROM_BIOS(2))
	// also mapped to f0000, f4000, f8000
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("40109.ic159",     0x00000, 0x08000, CRC(a8b67639) SHA1(99663bfb61798526e092205575370c2ad34249a1))
ROM_END


ROM_START( pc20 )
//  ROM_REGION(0x100000,"maincpu", 0)
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */

	// special bios at 0xe0000 !?
	// This is probably referring to a check for the Amstrad RP5-2 diagnostic
	// card, which can be plugged into an Amstrad XT for troubleshooting purposes.
	// - John Elliott
	ROM_LOAD16_BYTE("pc20v2.0", 0xfc001, 0x2000, CRC(41302eb8) SHA1(8b4b2afea543b96b45d6a30365281decc15f2932)) // v2
	ROM_LOAD16_BYTE("pc20v2.1", 0xfc000, 0x2000, CRC(71b84616) SHA1(4135102a491b25fc659d70b957e07649f3eacf24)) // v2
	// also mapped to f0000, f4000, f8000
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("40109.bin",     0x00000, 0x08000, CRC(a8b67639) SHA1(99663bfb61798526e092205575370c2ad34249a1))
ROM_END


ROM_START( ppc512 )
//  ROM_REGION(0x100000,"maincpu", 0)
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */
	// special bios at 0xe0000 !?
	ROM_LOAD16_BYTE("40107.v1", 0xfc000, 0x2000, CRC(4e37e769) SHA1(88be3d3375ec3b0a7041dbcea225b197e50d4bfe)) // v1.9
	ROM_LOAD16_BYTE("40108.v1", 0xfc001, 0x2000, CRC(4f0302d9) SHA1(e4d69ca98c3b98f3705a2902b16746360043f039)) // v1.9
	// also mapped to f0000, f4000, f8000
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("40109.bin",     0x00000, 0x08000, CRC(a8b67639) SHA1(99663bfb61798526e092205575370c2ad34249a1))
ROM_END


ROM_START( ppc640 )
//  ROM_REGION(0x100000,"maincpu", 0)
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */
	// special bios at 0xe0000 !?
	ROM_LOAD16_BYTE("40107.v2", 0xfc000, 0x2000, CRC(0785b63e) SHA1(4dbde6b9e9500298bb6241a8daefd85927f1ad28)) // v2.1
	ROM_LOAD16_BYTE("40108.v2", 0xfc001, 0x2000, CRC(5351cf8c) SHA1(b4dbf11b39378ab4afd2107d3fe54a99fffdedeb)) // v2.1
	// also mapped to f0000, f4000, f8000
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("40109.bin",     0x00000, 0x08000, CRC(a8b67639) SHA1(99663bfb61798526e092205575370c2ad34249a1))
ROM_END


ROM_START( pc1512 )
//  ROM_REGION(0x100000,"maincpu", 0)
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */
	ROM_LOAD16_BYTE("40044.v1", 0xfc001, 0x2000, CRC(668fcc94) SHA1(74002f5cc542df442eec9e2e7a18db3598d8c482)) // v1
	ROM_LOAD16_BYTE("40043.v1", 0xfc000, 0x2000, CRC(f72f1582) SHA1(7781d4717917262805d514b331ba113b1e05a247)) // v1
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("40045.bin",     0x00000, 0x02000, CRC(dd5e030f) SHA1(7d858bbb2e8d6143aa67ab712edf5f753c2788a7))
ROM_END


ROM_START( pc1512v2 )
//  ROM_REGION(0x100000,"maincpu", 0)
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */
	ROM_LOAD16_BYTE("40043.v2", 0xfc001, 0x2000, CRC(d2d4d2de) SHA1(c376fd1ad23025081ae16c7949e88eea7f56e1bb)) // v2
	ROM_LOAD16_BYTE("40044.v2", 0xfc000, 0x2000, CRC(1aec54fa) SHA1(b12fd73cfc35a240ed6da4dcc4b6c9910be611e0)) // v2
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("40078.bin",     0x00000, 0x02000, CRC(ae9c0d04) SHA1(bc8dc4dcedeea5bc1c04986b1f105ad93cb2ebcd))
ROM_END


ROM_START( pc1640 )
//  ROM_REGION(0x100000,"maincpu", 0)
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROM_LOAD("40100", 0xc0000, 0x8000, CRC(d2d1f1ae) SHA1(98302006ee38a17c09bd75504cc18c0649174e33)) /* Internal Graphics Adapter ROM */
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */

	ROM_LOAD16_BYTE("40043.v3", 0xfc001, 0x2000, CRC(e40a1513) SHA1(447eff2057e682e51b1c7593cb6fad0e53879fa8)) // v3
	ROM_RELOAD(0xf8001,0x2000)
	ROM_RELOAD(0xf4001,0x2000)
	ROM_RELOAD(0xf0001,0x2000)
	ROM_LOAD16_BYTE("40044.v3", 0xfc000, 0x2000, CRC(f1c074f3) SHA1(a055ea7e933d137623c22fe24004e870653c7952))
	ROM_RELOAD(0xf8000,0x2000)
	ROM_RELOAD(0xf4000,0x2000)
	ROM_RELOAD(0xf0000,0x2000)

	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("40045.bin",     0x00000, 0x02000, CRC(dd5e030f) SHA1(7d858bbb2e8d6143aa67ab712edf5f753c2788a7))

	ROM_REGION(0x50000, "gfx2", ROMREGION_ERASE00)
ROM_END


ROM_START( dgone )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */
	ROM_LOAD( "dgone.bin",  0xf8000, 0x08000, CRC(2c38c86e) SHA1(c0f85a000d1d13cd354965689e925d677822549e))

	/* IBM 1501981(CGA) and 1501985(MDA) Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f)) /* "AMI 8412PI // 5788005 // (C) IBM CORP. 1981 // KOREA" */
ROM_END

ROM_START( iskr1031 )
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROM_SYSTEM_BIOS(0, "v1", "v1")
	ROMX_LOAD( "150-02.bin", 0xfc000, 0x2000, CRC(e33fb974) SHA1(f5f3ece67c025c0033716ff516e1a34fbeb32749), ROM_SKIP(1) | ROM_BIOS(1))
	ROMX_LOAD( "150-03.bin", 0xfc001, 0x2000, CRC(8c482258) SHA1(90ef48955e0df556dc06a000a797ef42ccf430c5), ROM_SKIP(1) | ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "v2", "v2")
	ROMX_LOAD( "150-06.bin", 0xfc000, 0x2000, CRC(1adbf969) SHA1(08c0a0fc50a75e6207b1987bae389cca60893eac), ROM_SKIP(1) | ROM_BIOS(2))
	ROMX_LOAD( "150-07.bin", 0xfc001, 0x2000, CRC(0dc4b65a) SHA1(c96f066251a7343eac8113ea9dcb2cb12d0334d5), ROM_SKIP(1) | ROM_BIOS(2))

	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD( "iskra-1031_font.bin", 0x0000, 0x2000, CRC(f4d62e80) SHA1(ad7e81a0c9abc224671422bbcf6f6262da92b510))
ROM_END

ROM_START( iskr1030m )
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROMX_LOAD( "iskra-1030m_0.rom", 0xfc000, 0x2000, CRC(0d698e19) SHA1(2fe117c9f4f8c4b59085d5a41f919d743c425fdd), ROM_SKIP(1))
	ROMX_LOAD( "iskra-1030m_1.rom", 0xfc001, 0x2000, CRC(fe808337) SHA1(b0b7ebe14324ada8aa9a6926a82b18e80f78a257), ROM_SKIP(1))
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD( "iskra-1030m.chr", 0x0000, 0x2000, CRC(50b162eb) SHA1(5bd7cb1705a69bd16115a4c9ed1c2748a5c8ad51))
ROM_END

ROM_START( ec1840 )
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROM_SYSTEM_BIOS(0, "v4", "EC-1840.04")
	ROMX_LOAD( "000-04-971b.bin", 0xfe000, 0x0800, CRC(06aeaee8) SHA1(9f954e4c48156d573a8e0109e7ca652be9e6036a), ROM_SKIP(1) | ROM_BIOS(1))
	ROMX_LOAD( "001-04-92b7.bin", 0xff000, 0x0800, CRC(3fae650a) SHA1(c98b777fdeceadd72d6eb9465b3501b9ead55a08), ROM_SKIP(1) | ROM_BIOS(1))
	ROMX_LOAD( "002-04-9e17.bin", 0xfe001, 0x0800, CRC(d59712df) SHA1(02ea1b3ae9662f5c64c58920a32ca9db0f6fbd12), ROM_SKIP(1) | ROM_BIOS(1))
	ROMX_LOAD( "003-04-3ccb.bin", 0xff001, 0x0800, CRC(7fc362c7) SHA1(538e13639ad2b4c30bd72582e323181e63513306), ROM_SKIP(1) | ROM_BIOS(1))

	ROM_REGION(0x2000,"gfx1", ROMREGION_ERASE00)
ROM_END

ROM_START( ec1841 )
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROM_SYSTEM_BIOS(0, "v1", "EC-1841.01")
	ROMX_LOAD( "012-01-3107.bin", 0xfc000, 0x0800, CRC(77957396) SHA1(785f1dceb6e2b4618f5c5f0af15eb74a8c951448), ROM_SKIP(1) | ROM_BIOS(1))
	ROMX_LOAD( "013-01-203f.bin", 0xfc001, 0x0800, CRC(768bd3d5) SHA1(2e948f2ad262de306d889b7964c3f1aad45ff5bc), ROM_SKIP(1) | ROM_BIOS(1))
	ROMX_LOAD( "014-01-fa40.bin", 0xfd000, 0x0800, CRC(47722b58) SHA1(a6339ee8af516f834826b7828a5cf79cb650480c), ROM_SKIP(1) | ROM_BIOS(1))
	ROMX_LOAD( "015-01-bf1d.bin", 0xfd001, 0x0800, CRC(b585b5ea) SHA1(d0ebed586eb13031477c2e071c50416682f80489), ROM_SKIP(1) | ROM_BIOS(1))
	ROMX_LOAD( "016-01-65f7.bin", 0xfe000, 0x0800, CRC(28a07db4) SHA1(17fbcd60dacd1d3f8d8355db429f97e4d1d1ac88), ROM_SKIP(1) | ROM_BIOS(1))
	ROMX_LOAD( "017-01-5be1.bin", 0xfe001, 0x0800, CRC(928bda26) SHA1(ee889184067e2680b29a8ef1c3a76cf5afd4c78d), ROM_SKIP(1) | ROM_BIOS(1))
	ROMX_LOAD( "018-01-7090.bin", 0xff000, 0x0800, CRC(75ca7d7e) SHA1(6356426820c5326a7893a437d54b02f250ef8609), ROM_SKIP(1) | ROM_BIOS(1))
	ROMX_LOAD( "019-01-0492.bin", 0xff001, 0x0800, CRC(8a9d593e) SHA1(f3936d2cb4e6d130dd732973f126c3aa20612463), ROM_SKIP(1) | ROM_BIOS(1))

	ROM_SYSTEM_BIOS(1, "v2", "EC-1841.02")
	ROMX_LOAD( "012-02-37f6.bin", 0xfc000, 0x0800, CRC(8f5c6a20) SHA1(874b62f9cee8d3b974f33732f94eff10fc002c44), ROM_SKIP(1) | ROM_BIOS(2))
	ROMX_LOAD( "013-02-2552.bin", 0xfc001, 0x0800, CRC(e3c10128) SHA1(d6ed743ebe9c130925c9f17aad1a45db9194c967), ROM_SKIP(1) | ROM_BIOS(2))
	ROMX_LOAD( "014-02-0fbe.bin", 0xfd000, 0x0800, CRC(f8517e5e) SHA1(8034cd6ff5778365dc9daa494524f1753a74f1ed), ROM_SKIP(1) | ROM_BIOS(2))
	ROMX_LOAD( "015-02-d736.bin", 0xfd001, 0x0800, CRC(8538c52a) SHA1(ee981ce90870b6546a18f2a2e64d71b0038ce0dd), ROM_SKIP(1) | ROM_BIOS(2))
	ROMX_LOAD( "016-02-5b2c.bin", 0xfe000, 0x0800, CRC(3d1d1e67) SHA1(c527e29796537787c0f6c329f3c203f6131ca77f), ROM_SKIP(1) | ROM_BIOS(2))
	ROMX_LOAD( "017-02-4b9d.bin", 0xfe001, 0x0800, CRC(1b985264) SHA1(5ddcb9c13564be208c5068c105444a87159c67ee), ROM_SKIP(1) | ROM_BIOS(2))
	ROMX_LOAD( "018-02-7090.bin", 0xff000, 0x0800, CRC(75ca7d7e) SHA1(6356426820c5326a7893a437d54b02f250ef8609), ROM_SKIP(1) | ROM_BIOS(2))
	ROMX_LOAD( "019-02-0493.bin", 0xff001, 0x0800, CRC(61aae23d) SHA1(7b3aa24a63ee31b194297eb1e61c3827edfcb95a), ROM_SKIP(1) | ROM_BIOS(2))

	ROM_SYSTEM_BIOS(2, "v3", "EC-1841.03")
	ROMX_LOAD( "012-03-37e7.bin", 0xfc000, 0x0800, CRC(49992bd5) SHA1(119121e1b4af1c44b9b8c2edabe7dc1d3019c4a6), ROM_SKIP(1) | ROM_BIOS(3))
	ROMX_LOAD( "013-03-2554.bin", 0xfc001, 0x0800, CRC(834bd7d7) SHA1(e37514fc4cb8a5cbe68e7564e0e07d5116c4021a), ROM_SKIP(1) | ROM_BIOS(3))
	ROMX_LOAD( "014-03-0fbe.bin", 0xfd000, 0x0800, CRC(f8517e5e) SHA1(8034cd6ff5778365dc9daa494524f1753a74f1ed), ROM_SKIP(1) | ROM_BIOS(3))
	ROMX_LOAD( "015-03-d736.bin", 0xfd001, 0x0800, CRC(8538c52a) SHA1(ee981ce90870b6546a18f2a2e64d71b0038ce0dd), ROM_SKIP(1) | ROM_BIOS(3))
	ROMX_LOAD( "016-03-5b2c.bin", 0xfe000, 0x0800, CRC(3d1d1e67) SHA1(c527e29796537787c0f6c329f3c203f6131ca77f), ROM_SKIP(1) | ROM_BIOS(3))
	ROMX_LOAD( "017-03-4b9d.bin", 0xfe001, 0x0800, CRC(1b985264) SHA1(5ddcb9c13564be208c5068c105444a87159c67ee), ROM_SKIP(1) | ROM_BIOS(3))
	ROMX_LOAD( "018-03-7090.bin", 0xff000, 0x0800, CRC(75ca7d7e) SHA1(6356426820c5326a7893a437d54b02f250ef8609), ROM_SKIP(1) | ROM_BIOS(3))
	ROMX_LOAD( "019-03-0493.bin", 0xff001, 0x0800, CRC(61aae23d) SHA1(7b3aa24a63ee31b194297eb1e61c3827edfcb95a), ROM_SKIP(1) | ROM_BIOS(3))

	ROM_REGION(0x2000,"gfx1", ROMREGION_ERASE00)
ROM_END

ROM_START( ec1845 )
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROMX_LOAD( "184500.bin", 0xfc000, 0x0800, CRC(7c472ef7) SHA1(3af53f27b49bbc731bf51f9300fbada23a1bfcfc), ROM_SKIP(1))
	ROMX_LOAD( "184501.bin", 0xfc001, 0x0800, CRC(db240dc6) SHA1(d7bb022213d09bbf2a8107fe4f1cd27b23939e18), ROM_SKIP(1))
	ROMX_LOAD( "184502.bin", 0xfd000, 0x0800, CRC(149e7e29) SHA1(7f2a297588fef1bc750c57e6ae0d5acf3d27c486), ROM_SKIP(1))
	ROMX_LOAD( "184503.bin", 0xfd001, 0x0800, CRC(e28cbd74) SHA1(cf1fba4e67c8e1dd8cdda547118e84b704029b03), ROM_SKIP(1))
	ROMX_LOAD( "184504.bin", 0xfe000, 0x0800, CRC(55fa7a1d) SHA1(58f7abab08b9d2f0a1c1636e11bb72af2694c95f), ROM_SKIP(1))
	ROMX_LOAD( "184505.bin", 0xfe001, 0x0800, CRC(c807e3f5) SHA1(08117e449f0d04f96041cff8d34893f500f3760d), ROM_SKIP(1))
	ROMX_LOAD( "184506.bin", 0xff000, 0x0800, CRC(24f5c27c) SHA1(7822dd7f715ef00ccf6d8408be8bbfe01c2eba20), ROM_SKIP(1))
	ROMX_LOAD( "184507.bin", 0xff001, 0x0800, CRC(75122203) SHA1(7b0fbdf1315230633e39574ac7360163bc7361e1), ROM_SKIP(1))
	ROM_REGION(0x2000,"gfx1", ROMREGION_ERASE00)
ROM_END


ROM_START( mk88 )
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROM_DEFAULT_BIOS("v392")
	ROM_SYSTEM_BIOS(0, "v290", "v2.90")
	ROMX_LOAD( "mk88m.bin", 0xfc000, 0x2000, CRC(09c9da3b) SHA1(d1e7ad23b5f5b3576ad128c1198294129754f39f), ROM_BIOS(1))
	ROMX_LOAD( "mk88b.bin", 0xfe000, 0x2000, CRC(8a922476) SHA1(c19c3644ab92fd12e13f32b410cd26e3c844a03b), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "v391", "v3.91")
	ROMX_LOAD( "mkm.bin", 0xfc000, 0x2000, CRC(65f979e8) SHA1(13e85be9bc8ceb5ab9e559e7d0089e26fbbb84fc), ROM_BIOS(2))
	ROMX_LOAD( "mkb.bin", 0xfe000, 0x2000, CRC(830a0447) SHA1(11bc200fdbcfbbe335f4c282020750c0b5ca4167), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "v392", "v3.92")
	ROMX_LOAD( "m88.bin", 0xfc000, 0x2000, CRC(fe1b4e36) SHA1(fcb420af0ff09a7d43fcb9b7d0b0233a2071c159), ROM_BIOS(3))
	ROMX_LOAD( "b88.bin", 0xfe000, 0x2000, CRC(58a418df) SHA1(216398d4e4302ee7efcc2c8f9ff9d8a1161229ea), ROM_BIOS(3))
	ROM_REGION(0x2000,"gfx1", ROMREGION_ERASE00)
	// Here CGA rom with cyrillic support should be added
ROM_END

ROM_START( poisk1 )
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROM_LOAD( "b_hd_v11.rf2", 0xc8000, 0x0800, CRC(a19c39b2) SHA1(57faa56b320abf801fedbed578cf97d253e5b777)) // HDD controller ver 1.1
	ROM_LOAD( "b942_5mb.bin", 0x00000, 0x0800, CRC(a3cfa240) SHA1(0b0aa1ce839a957153bfbbe70310480ca9fe21b6)) // HDD controller ver 1.4

	ROM_LOAD( "b_ngmd_n.rf2", 0x0000, 0x0800, CRC(967e172a) SHA1(95117c40fd9f624fee08ccf37f615b16ff249688)) // Floppy
	ROM_LOAD( "b_ngmd_t.rf2", 0x0000, 0x0800, CRC(630010b1) SHA1(50876fe4f5f4f32a242faa70f9154574cd315ec4)) // Floppy
	ROM_SYSTEM_BIOS(0, "v89", "1989")
	ROMX_LOAD( "biosp1s.rf4",    0xfe000, 0x2000, CRC(1a85f671) SHA1(f0e59b2c4d92164abca55a96a58071ce869ff988), ROM_BIOS(1)) // Main BIOS
	ROM_SYSTEM_BIOS(1, "v91", "1991")
	ROMX_LOAD( "poisk_1991.bin", 0xfe000, 0x2000, CRC(d61c56fd) SHA1(de202e1f7422d585a1385a002a4fcf9d756236e5), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "t1", "Test I/O")
	ROMX_LOAD( "p1_t_i_o.rf4", 0xfe000, 0x2000, CRC(18a781de) SHA1(7267970ee27e3ea1d972bee8e74b17bac1051619), ROM_BIOS(3))
	ROM_SYSTEM_BIOS(3, "t2", "Test MB")
	ROMX_LOAD( "p1_t_pls.rf4", 0xfe000, 0x2000, CRC(c8210ffb) SHA1(f2d1a6c90e4708bcc56186b2fb906fa852667084), ROM_BIOS(4))
	ROM_SYSTEM_BIOS(4, "t3", "Test RAM")
	ROMX_LOAD( "p1_t_ram.rf4", 0xfe000, 0x2000, CRC(e42f5a61) SHA1(ce2554eae8f0d2b6d482890dd198cf7e2d29c655), ROM_BIOS(5))
	
	ROM_LOAD( "boot_net.rf4", 0x0000, 0x2000, CRC(316c2030) SHA1(d043325596455772252e465b85321f1b5c529d0b)) // NET BUIS
	ROM_REGION(0x2000,"gfx1", ROMREGION_ERASE00)
	ROM_LOAD( "poisk.cga", 0x0000, 0x0800, CRC(f6eb39f0) SHA1(0b788d8d7a8e92cc612d044abcb2523ad964c200))
ROM_END

ROM_START( poisk2 )
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROM_SYSTEM_BIOS(0, "v20", "v2.0")
	ROMX_LOAD( "b_p2_20h.rf4", 0xfc001, 0x2000, CRC(d53189b7) SHA1(ace40f1a40642b51fe5d2874acef81e48768b23b), ROM_SKIP(1) | ROM_BIOS(1))
	ROMX_LOAD( "b_p2_20l.rf4", 0xfc000, 0x2000, CRC(2d61fcc9) SHA1(11873c8741ba37d6c2fe1f482296aece514b7618), ROM_SKIP(1) | ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "v21", "v2.1")
	ROMX_LOAD( "b_p2_21h.rf4", 0xfc001, 0x2000, CRC(22197297) SHA1(506c7e63027f734d62ef537f484024548546011f), ROM_SKIP(1) | ROM_BIOS(2))
	ROMX_LOAD( "b_p2_21l.rf4", 0xfc000, 0x2000, CRC(0eb2ea7f) SHA1(67bb5fec53ebfa2a5cad2a3d3d595678d6023024), ROM_SKIP(1) | ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "v24", "v2.4")
	ROMX_LOAD( "b_p2_24h.rf4", 0xfc001, 0x2000, CRC(ea842c9e) SHA1(dcdbf27374149dae0ef76d410cc6c615d9b99372), ROM_SKIP(1) | ROM_BIOS(3))
	ROMX_LOAD( "b_p2_24l.rf4", 0xfc000, 0x2000, CRC(02f21250) SHA1(f0b133fb4470bddf2f7bf59688cf68198ed8ce55), ROM_SKIP(1) | ROM_BIOS(3))
	ROM_SYSTEM_BIOS(3, "v21d", "v2.1d")
	ROMX_LOAD( "opp2_1h.rf4", 0xfc001, 0x2000, CRC(b7cd7f4f) SHA1(ac473822fb44d7b898d628732cf0a27fcb4d26d6), ROM_SKIP(1) | ROM_BIOS(4))
	ROMX_LOAD( "opp2_1l.rf4", 0xfc000, 0x2000, CRC(1971dca3) SHA1(ecd61cc7952af834d8abc11db372c3e70775489d), ROM_SKIP(1) | ROM_BIOS(4))
	ROM_SYSTEM_BIOS(4, "v22d", "v2.2d")
	ROMX_LOAD( "opp2_2h.rf4", 0xfc001, 0x2000, CRC(b9e3a5cc) SHA1(0a28afbff612471ee81d69a98789e75253c57a30), ROM_SKIP(1) | ROM_BIOS(5))
	ROMX_LOAD( "opp2_2l.rf4", 0xfc000, 0x2000, CRC(6877aad6) SHA1(1d0031d044beb4f9f321e3c8fdedf57467958900), ROM_SKIP(1) | ROM_BIOS(5))
	ROM_SYSTEM_BIOS(5, "v23d", "v2.3d")
	ROMX_LOAD( "opp2_3h.rf4", 0xfc001, 0x2000, CRC(ac7d4f06) SHA1(858d6e084a38814280b3e29fb54971f4f532e484), ROM_SKIP(1) | ROM_BIOS(6))
	ROMX_LOAD( "opp2_3l.rf4", 0xfc000, 0x2000, CRC(3c877ea1) SHA1(0753168659653538311c0ad1df851cbbdba426f4), ROM_SKIP(1) | ROM_BIOS(6))
	ROM_REGION(0x2000,"gfx1", ROMREGION_ERASE00)
	ROM_LOAD( "p2_ecga.rf4", 0x0000, 0x2000, CRC(d537f665) SHA1(d70f085b9b0cbd53df7c3122fbe7592998ba8fed))
ROM_END

ROM_START( mc1702 )
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROM_LOAD16_BYTE( "2764_2_(573rf4).rom", 0xfc000,  0x2000, CRC(34a0c8fb) SHA1(88dc247f2e417c2848a2fd3e9b52258ad22a2c07))
	ROM_LOAD16_BYTE( "2764_3_(573rf4).rom", 0xfc001, 0x2000, CRC(68ab212b) SHA1(f3313f77392877d28ce290ffa3432f0a32fc4619))
	ROM_LOAD( "ba1m_(573rf5).rom", 0x0000, 0x0800, CRC(08d938e8) SHA1(957b6c691dbef75c1c735e8e4e81669d056971e4))

	/* IBM 1501981(CGA) and 1501985(MDA) Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f)) /* "AMI 8412PI // 5788005 // (C) IBM CORP. 1981 // KOREA" */
ROM_END


ROM_START( mc1502 )
	ROM_REGION16_LE(0x100000,"maincpu", 0)
    ROM_LOAD( "basic.rom",		  0xe8000, 0x8000, CRC(173d69fa) SHA1(003f872e12f00800e22ab6bbc009d36bfde67b9d))
	ROM_SYSTEM_BIOS(0, "v50", "v5.0")
	ROMX_LOAD( "monitor_5_0.rom",  0xfc000, 0x4000, CRC(9e97c6a0) SHA1(16a304e8de69ec4d8b92acda6bf28454c361a24f),ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "v52", "v5.2")
	ROMX_LOAD( "monitor_5_2.rom",  0xfc000, 0x4000, CRC(0e65491e) SHA1(8a4d556473b5e0e59b05fab77c79c29f4d562412),ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "v531", "v5.31")
	ROMX_LOAD( "monitor_5_31.rom", 0xfc000, 0x4000, CRC(a48295d5) SHA1(6f38977c22f9cc6c2bc6f6e53edc4048ca6b6721),ROM_BIOS(3))
	ROM_REGION(0x2000,"gfx1", ROMREGION_ERASE00)
	ROM_LOAD( "symgen.rom", 0x0000, 0x2000, CRC(b2747a52) SHA1(6766d275467672436e91ac2997ac6b77700eba1e))
ROM_END

ROM_START( m24 )
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */
	
    ROMX_LOAD("olivetti_m24_version_1.43_high.bin",0xfc001, 0x2000, CRC(04e697ba) SHA1(1066dcc849e6289b5ac6372c84a590e456d497a6), ROM_SKIP(1))
    ROMX_LOAD("olivetti_m24_version_1.43_low.bin", 0xfc000, 0x2000, CRC(ff7e0f10) SHA1(13423011a9bae3f3193e8c199f98a496cab48c0f), ROM_SKIP(1))

	/* IBM 1501981(CGA) and 1501985(MDA) Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f)) /* "AMI 8412PI // 5788005 // (C) IBM CORP. 1981 // KOREA" */
ROM_END

ROM_START( m240 )
	ROM_REGION16_LE(0x100000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */
	
	ROMX_LOAD("olivetti_m240_pch5_2.04_high.bin", 0xf8001, 0x4000, CRC(ceb97b59) SHA1(84fabbeab355e0a4c9445910f2b7d1ec98886642), ROM_SKIP(1))
	ROMX_LOAD("olivetti_m240_pch6_2.04_low.bin",  0xf8000, 0x4000, CRC(c463aa94) SHA1(a30c763c1ace9f3ff79e7136b252d624108a50ae), ROM_SKIP(1))

	/* IBM 1501981(CGA) and 1501985(MDA) Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f)) /* "AMI 8412PI // 5788005 // (C) IBM CORP. 1981 // KOREA" */
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*     YEAR     NAME        PARENT      COMPAT  	MACHINE     INPUT       INIT        COMPANY     FULLNAME */
COMP(  1984,	dgone,      ibm5150,	0,	pccga,  	pccga,		pccga,	"Data General",  "Data General/One" , GAME_NOT_WORKING)	/* CGA, 2x 3.5" disk drives */
COMP(  1985,	bw230,      ibm5150,	0,	pccga,		bondwell,   bondwell,   "Bondwell Holding",  "BW230 (PRO28 Series)", 0 )
COMP(  1988,	europc,     ibm5150,	0,	europc, 	europc,		europc,     "Schneider Rdf. AG",  "EURO PC", GAME_NOT_WORKING)

// pcjr (better graphics, better sound)
COMP(  1983,	ibmpcjr,    ibm5150,	0,	ibmpcjr, 	tandy1t,	pcjr,       "International Business Machines",  "IBM PC Jr", GAME_IMPERFECT_COLORS )
COMP(  1985,	ibmpcjx,    ibm5150,	0,	ibmpcjr,    tandy1t,	pcjr,       "International Business Machines",  "IBM PC JX", GAME_IMPERFECT_COLORS | GAME_NOT_WORKING)

// tandy 1000
COMP(  1987,	t1000hx,    ibm5150,	0,	t1000hx,    tandy1t,	t1000hx,    "Tandy Radio Shack",  "Tandy 1000 HX", 0)
COMP(  1987,	t1000sx,    ibm5150,	0,	t1000hx,    tandy1t,	t1000hx,    "Tandy Radio Shack",  "Tandy 1000 SX", GAME_NOT_WORKING)
COMP(  1987,	t1000tx,    ibm5150,	0,	t1000_286,  tandy1t,	t1000hx,    "Tandy Radio Shack",  "Tandy 1000 TX", 0)
COMP(  1989,	t1000rl,    ibm5150,	0,	t1000_16,   tandy1t,    t1000hx,    "Tandy Radio Shack",  "Tandy 1000 RL", 0)

// xt class (pc but 8086)
COMP(  1988,	pc200,      ibm5150,	0,	pc200,		pc200,	pc200,	"Sinclair Research Ltd",  "PC200 Professional Series", GAME_NOT_WORKING)
COMP(  1988,	pc20,       ibm5150,	0,	pc200,		pc200,	pc200,	"Amstrad plc",  "Amstrad PC20" , GAME_NOT_WORKING)
COMP(  1987,	ppc512,     ibm5150,	0,	ppc512,		pc200,	ppc512,	"Amstrad plc",  "Amstrad PPC512", GAME_NOT_WORKING)
COMP(  1987,	ppc640,     ibm5150,	0,	ppc640,		pc200,	ppc512,	"Amstrad plc",  "Amstrad PPC640", GAME_NOT_WORKING)
COMP(  1986,	pc1512,     ibm5150,	0,	pc1512,     pc1512,	pc1512,	"Amstrad plc",  "Amstrad PC1512 (version 1)", GAME_NOT_WORKING)
COMP(  198?,	pc1512v2,   ibm5150,	0,	pc1512,     pc1512,	pc1512,	"Amstrad plc",  "Amstrad PC1512 (version 2)", GAME_NOT_WORKING)
COMP(  1987,	pc1640,     ibm5150,	0,	pc1640,     pc1640,	pc1640,	"Amstrad plc",  "Amstrad PC1640 / PC6400 (US)", GAME_NOT_WORKING )
// pc2086 pc1512 with vga??
COMP ( 1989,	iskr1031,   ibm5150,	0,	iskr1031,	pccga,	pccga,	"<unknown>",  "Iskra-1031" , GAME_NOT_WORKING)
COMP ( 1989,	iskr1030m,  ibm5150,	0,	iskr1031,	pccga,	pccga,	"<unknown>",  "Iskra-1030M" , GAME_NOT_WORKING)
COMP ( 1987,	ec1840,     ibm5150,	0,	iskr1031,	pccga,	pccga,	"<unknown>",  "EC-1840" , GAME_NOT_WORKING)
COMP ( 1987,	ec1841,     ibm5150,	0,	iskr1031,	pccga,	pccga,	"<unknown>",  "EC-1841" , GAME_NOT_WORKING)
COMP ( 1989,	ec1845,     ibm5150,	0,	iskr1031,	pccga,	pccga,	"<unknown>",  "EC-1845" , GAME_NOT_WORKING)
COMP ( 1989,	mk88,       ibm5150,	0,	iskr1031,	pccga,	pccga,	"<unknown>",  "MK-88" , GAME_NOT_WORKING)
COMP ( 1990,	poisk1,     ibm5150,	0,	iskr1031,	pccga,	pccga,	"<unknown>",  "Poisk-1" , GAME_NOT_WORKING)
COMP ( 1991,	poisk2,     ibm5150,	0,	poisk2,		pccga,	pccga,	"<unknown>",  "Poisk-2" , GAME_NOT_WORKING)
COMP ( 1990,	mc1702,     ibm5150,	0,	pccga,		pccga,	pccga,	"<unknown>",  "Elektronika MC-1702" , GAME_NOT_WORKING)
COMP ( 19??,	mc1502,     ibm5150,	0,	mc1502,		pccga,	pccga,	"<unknown>",  "Elektronika MC-1502" , GAME_NOT_WORKING)

COMP(  1987,	zdsupers,   ibm5150,	0,	zenith,		pccga,	pccga,	"Zenith Data Systems",  "SuperSport" , 0)

COMP ( 1983,	m24,        ibm5150,	0,	olivetti,	pccga,	pccga,	"Olivetti",  "M24" , GAME_NOT_WORKING)
COMP ( 1987,	m240,        ibm5150,	0,	olivetti,	pccga,	pccga,	"Olivetti",  "M240" , GAME_NOT_WORKING)
