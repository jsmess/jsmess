/***************************************************************************

    IBM AT Compatibles

***************************************************************************/

/* mingw-gcc defines this */
#ifdef i386
#undef i386
#endif /* i386 */

#include "driver.h"
#include "cpu/i86/i86.h"
#include "cpu/i86/i286.h"
#include "cpu/i386/i386.h"
#include "sound/3812intf.h"
#include "machine/i8255a.h"
#include "machine/ins8250.h"
#include "machine/mc146818.h"
#include "machine/pic8259.h"
#include "machine/i82439tx.h"

#include "machine/pit8253.h"
#include "video/pc_vga.h"
#include "video/pc_cga.h"
#include "video/pc_mda.h"
#include "video/pc_ega.h"
#include "video/pc_video.h"
#include "includes/pc.h"

#include "machine/pc_hdc.h"
#include "includes/pc_ide.h"
#include "machine/pc_fdc.h"
#include "machine/pc_joy.h"
#include "machine/pckeybrd.h"
#include "machine/pc_lpt.h"
#include "audio/sblaster.h"
#include "includes/pc_mouse.h"

#include "includes/at.h"
#include "machine/8042kbdc.h"
#include "includes/ps2.h"

#include "machine/pcshare.h"

#include "devices/flopdrv.h"
#include "devices/harddriv.h"
#include "formats/pc_dsk.h"

#include "machine/8237dma.h"
#include "machine/pci.h"
#include "machine/kb_keytro.h"

#include "sound/dac.h"
#include "sound/speaker.h"
#include "sound/saa1099.h"

#include "memconv.h"

/* window resizing with dirtybuffering traping in xmess window */

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
  creativ labs game blaster (CMS creativ music system)
  2 x saa1099 chips
  also on sound blaster 1.0
  option on sound blaster 1.5

  jumperable? normally 0x220
*/
#define GAMEBLASTER

static ADDRESS_MAP_START( at16_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x09ffff) AM_MIRROR(0xff000000) AM_RAMBANK(10)
//  AM_RANGE(0x0a0000, 0x0bffff) AM_READWRITE(SMH_RAM, pc_video_videoram16le_w) AM_BASE((UINT16 **)&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x0c0000, 0x0c7fff) AM_ROM
	AM_RANGE(0x0c8000, 0x0cffff) AM_ROM
	AM_RANGE(0x0d0000, 0x0effff) AM_RAM
	AM_RANGE(0x0f0000, 0x0fffff) AM_ROM
	AM_RANGE(0xff0000, 0xffffff) AM_ROM AM_REGION("maincpu", 0x0f0000)
ADDRESS_MAP_END


static ADDRESS_MAP_START( at386_map, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_GLOBAL_MASK(0x00ffffff)
	AM_RANGE(0x00000000, 0x0009ffff) AM_RAMBANK(10)
	AM_RANGE(0x000a0000, 0x000bffff) AM_NOP
//  AM_RANGE(0x000b8000, 0x000bffff) AM_READWRITE(SMH_RAM, pc_video_videoram32_w) AM_BASE((UINT32 **) &videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x000c0000, 0x000c7fff) AM_ROM
	AM_RANGE(0x000c8000, 0x000cffff) AM_ROM
	AM_RANGE(0x000d0000, 0x000effff) AM_ROM
	AM_RANGE(0x000f0000, 0x000fffff) AM_ROM AM_REGION("maincpu", 0x0f0000)
	AM_RANGE(0x00ff0000, 0x00ffffff) AM_ROM AM_REGION("maincpu", 0x0f0000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( at586_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0009ffff) AM_RAMBANK(10)
	AM_RANGE(0x000a0000, 0x000bffff) AM_NOP
//  AM_RANGE(0x000b8000, 0x000bffff) AM_READWRITE(SMH_RAM, pc_video_videoram32_w) AM_BASE((UINT32 **) &videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0xfffe0000, 0xffffffff) AM_ROM AM_REGION("user1", 0x20000)
ADDRESS_MAP_END



static READ8_DEVICE_HANDLER(at_dma8237_1_r)
{
	return dma8237_r( device, offset / 2);
}

static WRITE8_DEVICE_HANDLER(at_dma8237_1_w)
{
	dma8237_w( device, offset / 2, data);
}

static READ16_DEVICE_HANDLER( at16_388_r )
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


static WRITE16_DEVICE_HANDLER( at16_388_w )
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

READ32_HANDLER( at_kbdc8042_32le_r )
{
	return read32le_with_read8_handler(at_kbdc8042_r, space, offset, mem_mask);
}

WRITE32_HANDLER( at_kbdc8042_32le_w )
{
	write32le_with_write8_handler(at_kbdc8042_w, space, offset, data, mem_mask);
}

static ADDRESS_MAP_START(at16_io, ADDRESS_SPACE_IO, 16)
	AM_RANGE(0x0000, 0x001f) AM_DEVREADWRITE8("dma8237_1", dma8237_r, dma8237_w, 0xffff)
	AM_RANGE(0x0020, 0x003f) AM_DEVREADWRITE8("pic8259_master", pic8259_r, pic8259_w, 0xffff)
	AM_RANGE(0x0040, 0x005f) AM_DEVREADWRITE8("pit8254", pit8253_r, pit8253_w, 0xffff)
	AM_RANGE(0x0060, 0x006f) AM_READWRITE8(at_kbdc8042_r,            at_kbdc8042_w, 0xffff)
	AM_RANGE(0x0070, 0x007f) AM_READWRITE8(mc146818_port_r,          mc146818_port_w, 0xffff)
	AM_RANGE(0x0080, 0x009f) AM_READWRITE8(at_page8_r,               at_page8_w, 0xffff)
	AM_RANGE(0x00a0, 0x00bf) AM_DEVREADWRITE8("pic8259_slave", pic8259_r, pic8259_w, 0xffff)
	AM_RANGE(0x00c0, 0x00df) AM_DEVREADWRITE8("dma8237_2", at_dma8237_1_r, at_dma8237_1_w, 0xffff)
	AM_RANGE(0x01f0, 0x01f7) AM_READWRITE8(at_mfm_0_r,               at_mfm_0_w, 0xffff)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE8(pc_JOY_r,                 pc_JOY_w, 0xffff)
	AM_RANGE(0x0220, 0x022f) AM_READWRITE8(soundblaster_r,           soundblaster_w, 0xffff)
	AM_RANGE(0x0278, 0x027f) AM_DEVREADWRITE8("lpt_2", pc_lpt_r, pc_lpt_w, 0x00ff)
	AM_RANGE(0x02e8, 0x02ef) AM_DEVREADWRITE8("ns16450_3", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE8("ns16450_1", ins8250_r, ins8250_w, 0xffff)
//  AM_RANGE(0x0320, 0x0323) AM_READWRITE8(pc_HDC1_r,                pc_HDC1_w, 0xffff)
//  AM_RANGE(0x0324, 0x0327) AM_READWRITE8(pc_HDC2_r,                pc_HDC2_w, 0xffff)
	AM_RANGE(0x0378, 0x037f) AM_DEVREADWRITE8("lpt_1", pc_lpt_r, pc_lpt_w, 0x00ff)
#ifdef ADLIB
	AM_RANGE(0x0388, 0x0389) AM_DEVREADWRITE("ym3812", at16_388_r, at16_388_w )
#endif
	AM_RANGE(0x03bc, 0x03bf) AM_DEVREADWRITE8("lpt_0", pc_lpt_r, pc_lpt_w, 0x00ff)
	AM_RANGE(0x03e8, 0x03ef) AM_DEVREADWRITE8("ns16450_2", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE8(pc_fdc_r,                 pc_fdc_w, 0xffff)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE8("ns16450_0", ins8250_r, ins8250_w, 0xffff)
ADDRESS_MAP_END


static ADDRESS_MAP_START(at386_io, ADDRESS_SPACE_IO, 32)
	AM_RANGE(0x0000, 0x001f) AM_DEVREADWRITE8("dma8237_1", dma8237_r, dma8237_w, 0xffffffff)
	AM_RANGE(0x0020, 0x003f) AM_DEVREADWRITE8("pic8259_master", pic8259_r, pic8259_w, 0xffffffff)
	AM_RANGE(0x0040, 0x005f) AM_DEVREADWRITE8("pit8254", pit8253_r, pit8253_w, 0xffffffff)
	AM_RANGE(0x0060, 0x006f) AM_READWRITE(at_kbdc8042_32le_r,      at_kbdc8042_32le_w)
	AM_RANGE(0x0070, 0x007f) AM_READWRITE(mc146818_port32le_r,		mc146818_port32le_w)
	AM_RANGE(0x0080, 0x009f) AM_READWRITE8(at_page8_r,				at_page8_w, 0xffffffff)
	AM_RANGE(0x00a0, 0x00bf) AM_DEVREADWRITE8("pic8259_slave", pic8259_r, pic8259_w, 0xffffffff)
	AM_RANGE(0x00c0, 0x00df) AM_DEVREADWRITE8("dma8237_2", at_dma8237_1_r, at_dma8237_1_w, 0xffffffff)
	AM_RANGE(0x0278, 0x027f) AM_DEVREADWRITE8("lpt_2", pc_lpt_r, pc_lpt_w, 0x000000ff)
	AM_RANGE(0x02e8, 0x02ef) AM_DEVREADWRITE8("ns16450_3", ins8250_r, ins8250_w, 0xffffffff)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE8("ns16450_1", ins8250_r, ins8250_w, 0xffffffff)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc32le_HDC1_r,			pc32le_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc32le_HDC2_r,			pc32le_HDC2_w)
	AM_RANGE(0x0378, 0x037f) AM_DEVREADWRITE8("lpt_1", pc_lpt_r, pc_lpt_w, 0x000000ff)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE8(pc_fdc_r,				pc_fdc_w, 0xffffffff)
	AM_RANGE(0x03bc, 0x03bf) AM_DEVREADWRITE8("lpt_0", pc_lpt_r, pc_lpt_w, 0x000000ff)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE8("ns16450_0", ins8250_r, ins8250_w, 0xffffffff)
ADDRESS_MAP_END



static ADDRESS_MAP_START(at586_io, ADDRESS_SPACE_IO, 32)
	AM_RANGE(0x0000, 0x001f) AM_DEVREADWRITE8("dma8237_1", dma8237_r, dma8237_w, 0xffffffff)
	AM_RANGE(0x0020, 0x003f) AM_DEVREADWRITE8("pic8259_master", pic8259_r, pic8259_w, 0xffffffff)
	AM_RANGE(0x0040, 0x005f) AM_DEVREADWRITE8("pit8254", pit8253_r, pit8253_w, 0xffffffff)
	AM_RANGE(0x0060, 0x006f) AM_READWRITE(at_kbdc8042_32le_r,      at_kbdc8042_32le_w)
	AM_RANGE(0x0070, 0x007f) AM_READWRITE(mc146818_port32le_r,		mc146818_port32le_w)
	AM_RANGE(0x0080, 0x009f) AM_READWRITE8(at_page8_r,				at_page8_w, 0xffffffff)
	AM_RANGE(0x00a0, 0x00bf) AM_DEVREADWRITE8("pic8259_slave", pic8259_r, pic8259_w, 0xffffffff)
	AM_RANGE(0x00c0, 0x00df) AM_DEVREADWRITE8("dma8237_2", at_dma8237_1_r, at_dma8237_1_w, 0xffffffff)
	AM_RANGE(0x0278, 0x027f) AM_DEVREADWRITE8("lpt_2", pc_lpt_r, pc_lpt_w, 0x000000ff)
	AM_RANGE(0x02e8, 0x02ef) AM_DEVREADWRITE8("ns16450_3", ins8250_r, ins8250_w, 0xffffffff)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE8("ns16450_1", ins8250_r, ins8250_w, 0xffffffff)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc32le_HDC1_r,				pc32le_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc32le_HDC2_r,				pc32le_HDC2_w)
	AM_RANGE(0x0378, 0x037f) AM_DEVREADWRITE8("lpt_1", pc_lpt_r, pc_lpt_w, 0x000000ff)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE8(pc_fdc_r,				pc_fdc_w, 0xffffffff)
	AM_RANGE(0x03bc, 0x03bf) AM_DEVREADWRITE8("lpt_0", pc_lpt_r, pc_lpt_w, 0x000000ff)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE8("ns16450_0", ins8250_r, ins8250_w, 0xffffffff)
	AM_RANGE(0x0cf8, 0x0cff) AM_DEVREADWRITE("pcibus", pci_32le_r,				pci_32le_w)
ADDRESS_MAP_END


#ifdef UNUSED_FUNCTION
static ADDRESS_MAP_START(ps2m30286_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(dma8237_0_r,				dma8237_0_w)
	AM_RANGE(0x0020, 0x003f) AM_READWRITE(pic8259_0_r,				pic8259_0_w)
	AM_RANGE(0x0040, 0x005f) AM_READWRITE(pit8253_0_r,				pit8253_0_w)
	AM_RANGE(0x0060, 0x006f) AM_READWRITE(kbdc8042_8_r,				kbdc8042_8_w)
	AM_RANGE(0x0070, 0x007f) AM_READWRITE(mc146818_port_r,			mc146818_port_w)
	AM_RANGE(0x0080, 0x009f) AM_READWRITE(at_page8_r,				at_page8_w)
	AM_RANGE(0x00a0, 0x00bf) AM_READWRITE(pic8259_1_r,				pic8259_1_w)
	AM_RANGE(0x00c0, 0x00df) AM_READWRITE(at_dma8237_1_r,			at_dma8237_1_w)
	AM_RANGE(0x0100, 0x0107) AM_READWRITE(ps2_pos_r,				ps2_pos_w)
	AM_RANGE(0x01f0, 0x01f7) AM_READWRITE(at_mfm_0_r,				at_mfm_0_w)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc_JOY_r,					pc_JOY_w)
	AM_RANGE(0x0220, 0x022f) AM_READWRITE(soundblaster_r,			soundblaster_w)
	AM_RANGE(0x0278, 0x027f) AM_READWRITE(pc_parallelport2_r,		pc_parallelport2_w)
	AM_RANGE(0x02e8, 0x02ef) AM_READWRITE(uart8250_3_r,				uart8250_3_w)
	AM_RANGE(0x02f8, 0x02ff) AM_READWRITE(uart8250_1_r,				uart8250_1_w)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc_HDC1_r,				pc_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc_HDC2_r,				pc_HDC2_w)
	AM_RANGE(0x0378, 0x037f) AM_READWRITE(pc_parallelport1_r,		pc_parallelport1_w)
#ifdef ADLIB
	AM_RANGE(0x0388, 0x0388) AM_DEVREADWRITE("ym3812", ym3812_status_port_r,ym3812_control_port_w)
	AM_RANGE(0x0389, 0x0389) AM_DEVWRITE("ym3812", ym3812_write_port_w)
#endif
	AM_RANGE(0x03bc, 0x03be) AM_READWRITE(pc_parallelport0_r,		pc_parallelport0_w)
	AM_RANGE(0x03e8, 0x03ef) AM_READWRITE(uart8250_2_r,				uart8250_2_w)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE(pc_fdc_r,					pc_fdc_w)
	AM_RANGE(0x03f8, 0x03ff) AM_READWRITE(uart8250_0_r,				uart8250_0_w)
ADDRESS_MAP_END
#endif

static INPUT_PORTS_START( atcga )
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
	PORT_DIPNAME( 0x30, 0x00, "Graphics adapter")
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

	PORT_INCLUDE( kb_keytronic_at )		/* IN4 - IN11 */
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
	PORT_INCLUDE( pcvideo_cga_at )
INPUT_PORTS_END

static INPUT_PORTS_START( atvga )
	PORT_START("IN0") /* IN0 */
	PORT_DIPNAME( 0x08, 0x08, "VGA 1")
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "VGA 2")
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "VGA 3")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x00, "VGA 4")
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

    PORT_START("DSW0") /* IN1 */
	PORT_DIPNAME( 0xc0, 0x40, "Number of floppy drives")
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_DIPNAME( 0x30, 0x00, "Graphics adapter")
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

	PORT_INCLUDE( kb_keytronic_at )		/* IN4 - IN11 */
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
INPUT_PORTS_END

static const unsigned i286_address_mask = 0x00ffffff;


#if defined(ADLIB)
static const ym3812_interface at_ym3812_interface =
{
	/* irq line not connected to pc on adlib cards (and compatibles) */
	NULL
};
#endif

static const pc_lpt_interface at_lpt_config =
{
	DEVCB_CPU_INPUT_LINE("maincpu", 0)
};

static const floppy_config ibmat_floppy_config =
{
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(pc),
	DO_NOT_KEEP_GEOMETRY
};

static MACHINE_DRIVER_START( ibm5170 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", I80286, 6000000 /*6000000*/)
	MDRV_CPU_PROGRAM_MAP(at16_map)
	MDRV_CPU_IO_MAP(at16_io)
	MDRV_CPU_CONFIG(i286_address_mask)

	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_PIT8254_ADD( "pit8254", at_pit8254_config )

	MDRV_DMA8237_ADD( "dma8237_1", at_dma8237_1_config )

	MDRV_DMA8237_ADD( "dma8237_2", at_dma8237_2_config )

	MDRV_PIC8259_ADD( "pic8259_master", at_pic8259_master_config )

	MDRV_PIC8259_ADD( "pic8259_slave", at_pic8259_slave_config )

	MDRV_NS16450_ADD( "ns16450_0", ibm5170_com_interface[0] ) /* Verified: IBM P/N 6320947 Serial/Parallel card uses an NS16450N */
	MDRV_NS16450_ADD( "ns16450_1", ibm5170_com_interface[1] )
	MDRV_NS16450_ADD( "ns16450_2", ibm5170_com_interface[2] )
	MDRV_NS16450_ADD( "ns16450_3", ibm5170_com_interface[3] )
	MDRV_MACHINE_START( at )
	MDRV_MACHINE_RESET( at )

	MDRV_IMPORT_FROM( pcvideo_ega )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MDRV_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MDRV_SOUND_CONFIG(at_ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD("saa1099.1", SAA1099, 8000000)	/* running at 8 MHz ISA bus speed? */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD("saa1099.2", SAA1099, 8000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif
	MDRV_IMPORT_FROM( at_kbdc8042 )

	MDRV_IMPORT_FROM( kb_keytronic )

	MDRV_NVRAM_HANDLER( mc146818 )

	/* printers */
	MDRV_PC_LPT_ADD("lpt_0", at_lpt_config)
	MDRV_PC_LPT_ADD("lpt_1", at_lpt_config)
	MDRV_PC_LPT_ADD("lpt_2", at_lpt_config)

	/* harddisk */
	MDRV_IMPORT_FROM( pc_hdc )

	MDRV_NEC765A_ADD("nec765", pc_fdc_nec765_not_connected_interface)

	MDRV_FLOPPY_2_DRIVES_ADD(ibmat_floppy_config)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ibm5170a )
	MDRV_IMPORT_FROM( ibm5170 )
	MDRV_CPU_REPLACE("maincpu", I80286, 8000000)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ibm5162 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", I80286, 6000000 /*6000000*/)
	MDRV_CPU_PROGRAM_MAP(at16_map)
	MDRV_CPU_IO_MAP(at16_io)
	MDRV_CPU_CONFIG(i286_address_mask)

	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_PIT8254_ADD( "pit8254", at_pit8254_config )

	MDRV_DMA8237_ADD( "dma8237_1", at_dma8237_1_config )

	MDRV_DMA8237_ADD( "dma8237_2", at_dma8237_2_config )

	MDRV_PIC8259_ADD( "pic8259_master", at_pic8259_master_config )

	MDRV_PIC8259_ADD( "pic8259_slave", at_pic8259_slave_config )

	MDRV_NS16450_ADD( "ns16450_0", ibm5170_com_interface[0] )			/* TODO: verify model */
	MDRV_NS16450_ADD( "ns16450_1", ibm5170_com_interface[1] )			/* TODO: verify model */
	MDRV_NS16450_ADD( "ns16450_2", ibm5170_com_interface[2] )			/* TODO: verify model */
	MDRV_NS16450_ADD( "ns16450_3", ibm5170_com_interface[3] )			/* TODO: verify model */

	MDRV_MACHINE_START( at )
	MDRV_MACHINE_RESET( at )

	MDRV_IMPORT_FROM( pcvideo_cga )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MDRV_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MDRV_SOUND_CONFIG(at_ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif

	MDRV_IMPORT_FROM( at_kbdc8042 )

	MDRV_IMPORT_FROM( kb_keytronic )

	MDRV_NVRAM_HANDLER( mc146818 )

	/* printers */
	MDRV_PC_LPT_ADD("lpt_0", at_lpt_config)
	MDRV_PC_LPT_ADD("lpt_1", at_lpt_config)
	MDRV_PC_LPT_ADD("lpt_2", at_lpt_config)

	/* harddisk */
	MDRV_IMPORT_FROM( pc_hdc )

	MDRV_NEC765A_ADD("nec765", pc_fdc_nec765_not_connected_interface)

	MDRV_FLOPPY_2_DRIVES_ADD(ibmat_floppy_config)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ps2m30286 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", I80286, 12000000)
	MDRV_CPU_PROGRAM_MAP(at16_map)
	MDRV_CPU_IO_MAP(at16_io)
	MDRV_CPU_CONFIG(i286_address_mask)

	MDRV_PIT8254_ADD( "pit8254", at_pit8254_config )

	MDRV_DMA8237_ADD( "dma8237_1", at_dma8237_1_config )

	MDRV_DMA8237_ADD( "dma8237_2", at_dma8237_2_config )

	MDRV_PIC8259_ADD( "pic8259_master", at_pic8259_master_config )

	MDRV_PIC8259_ADD( "pic8259_slave", at_pic8259_slave_config )

	MDRV_NS16450_ADD( "ns16450_0", ibm5170_com_interface[0] )			/* TODO: verify model */
	MDRV_NS16450_ADD( "ns16450_1", ibm5170_com_interface[1] )			/* TODO: verify model */
	MDRV_NS16450_ADD( "ns16450_2", ibm5170_com_interface[2] )			/* TODO: verify model */
	MDRV_NS16450_ADD( "ns16450_3", ibm5170_com_interface[3] )			/* TODO: verify model */

	MDRV_MACHINE_START( at )
	MDRV_MACHINE_RESET( at )

	MDRV_IMPORT_FROM( pcvideo_vga )

	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MDRV_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MDRV_SOUND_CONFIG(at_ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD("saa1099.1", SAA1099, 8000000)	/* running at 8 MHz ISA bus speed? */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD("saa1099.2", SAA1099, 8000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif

	MDRV_IMPORT_FROM( at_kbdc8042 )

	MDRV_IMPORT_FROM( kb_keytronic )

	MDRV_NVRAM_HANDLER( mc146818 )

	/* printers */
	MDRV_PC_LPT_ADD("lpt_0", at_lpt_config)
	MDRV_PC_LPT_ADD("lpt_1", at_lpt_config)
	MDRV_PC_LPT_ADD("lpt_2", at_lpt_config)

	/* harddisk */
	MDRV_IMPORT_FROM( pc_hdc )

	MDRV_NEC765A_ADD("nec765", pc_fdc_nec765_not_connected_interface)

	MDRV_FLOPPY_2_DRIVES_ADD(ibmat_floppy_config)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( atvga )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", I80286, 12000000)
	MDRV_CPU_PROGRAM_MAP(at16_map)
	MDRV_CPU_IO_MAP(at16_io)
	MDRV_CPU_CONFIG(i286_address_mask)

	MDRV_PIT8254_ADD( "pit8254", at_pit8254_config )

	MDRV_DMA8237_ADD( "dma8237_1", at_dma8237_1_config )

	MDRV_DMA8237_ADD( "dma8237_2", at_dma8237_2_config )

	MDRV_PIC8259_ADD( "pic8259_master", at_pic8259_master_config )

	MDRV_PIC8259_ADD( "pic8259_slave", at_pic8259_slave_config )

	MDRV_NS16450_ADD( "ns16450_0", ibm5170_com_interface[0] )			/* TODO: verify model */
	MDRV_NS16450_ADD( "ns16450_1", ibm5170_com_interface[1] )			/* TODO: verify model */
	MDRV_NS16450_ADD( "ns16450_2", ibm5170_com_interface[2] )			/* TODO: verify model */
	MDRV_NS16450_ADD( "ns16450_3", ibm5170_com_interface[3] )			/* TODO: verify model */

	MDRV_IMPORT_FROM( pcvideo_vga )

	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	MDRV_MACHINE_START( at )
	MDRV_MACHINE_RESET( at )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MDRV_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MDRV_SOUND_CONFIG(at_ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD("saa1099.1", SAA1099, 8000000)	/* running at 8 MHz ISA bus speed? */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD("saa1099.2", SAA1099, 8000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif
	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_IMPORT_FROM( at_kbdc8042 )

	MDRV_IMPORT_FROM( kb_keytronic )

	MDRV_NVRAM_HANDLER( mc146818 )

	/* printers */
	MDRV_PC_LPT_ADD("lpt_0", at_lpt_config)
	MDRV_PC_LPT_ADD("lpt_1", at_lpt_config)
	MDRV_PC_LPT_ADD("lpt_2", at_lpt_config)

	/* harddisk */
	MDRV_IMPORT_FROM( pc_hdc )

	MDRV_NEC765A_ADD("nec765", pc_fdc_nec765_not_connected_interface)

	MDRV_FLOPPY_2_DRIVES_ADD(ibmat_floppy_config)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( at386 )
    /* basic machine hardware */
	/* original at 6 MHz, at03 8 megahertz */
	MDRV_CPU_ADD("maincpu", I386, 12000000)
	MDRV_CPU_PROGRAM_MAP(at386_map)
	MDRV_CPU_IO_MAP(at386_io)
	MDRV_CPU_CONFIG(i286_address_mask)

	MDRV_MACHINE_START( at )
	MDRV_MACHINE_RESET( at )

	MDRV_PIT8254_ADD( "pit8254", at_pit8254_config )

	MDRV_DMA8237_ADD( "dma8237_1", at_dma8237_1_config )

	MDRV_DMA8237_ADD( "dma8237_2", at_dma8237_2_config )

	MDRV_PIC8259_ADD( "pic8259_master", at_pic8259_master_config )

	MDRV_PIC8259_ADD( "pic8259_slave", at_pic8259_slave_config )

	MDRV_NS16450_ADD( "ns16450_0", ibm5170_com_interface[0] )			/* TODO: verify model */
	MDRV_NS16450_ADD( "ns16450_1", ibm5170_com_interface[1] )			/* TODO: verify model */
	MDRV_NS16450_ADD( "ns16450_2", ibm5170_com_interface[2] )			/* TODO: verify model */
	MDRV_NS16450_ADD( "ns16450_3", ibm5170_com_interface[3] )			/* TODO: verify model */

	MDRV_IMPORT_FROM( pcvideo_cga )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MDRV_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MDRV_SOUND_CONFIG(at_ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD("saa1099.1", SAA1099, 8000000)	/* running at 8 MHz ISA bus speed? */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD("saa1099.2", SAA1099, 8000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif
	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_IMPORT_FROM( at_kbdc8042 )

	MDRV_IMPORT_FROM( kb_keytronic )

	MDRV_NVRAM_HANDLER( mc146818 )

	/* printers */
	MDRV_PC_LPT_ADD("lpt_0", at_lpt_config)
	MDRV_PC_LPT_ADD("lpt_1", at_lpt_config)
	MDRV_PC_LPT_ADD("lpt_2", at_lpt_config)

	/* harddisk */
	MDRV_IMPORT_FROM( pc_hdc )

	MDRV_NEC765A_ADD("nec765", pc_fdc_nec765_not_connected_interface)

	MDRV_FLOPPY_2_DRIVES_ADD(ibmat_floppy_config)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( at486 )
	MDRV_IMPORT_FROM( at386 )

	MDRV_CPU_REPLACE("maincpu", I486, 12000000)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( at586 )
	MDRV_IMPORT_FROM( at386 )

	MDRV_CPU_REPLACE("maincpu", PENTIUM, 60000000)
	MDRV_CPU_PROGRAM_MAP(at586_map)
	MDRV_CPU_IO_MAP(at586_io)

	MDRV_PCI_BUS_ADD("pcibus", 0)
	MDRV_PCI_BUS_DEVICE(0, NULL, intel82439tx_pci_read, intel82439tx_pci_write)
MACHINE_DRIVER_END



#if 0
	// ibm at
	// most likely 2 32 kbyte chips for 16 bit access
    ROM_LOAD("atbios.bin", 0xf0000, 0x10000, CRC(674426be)) // BASIC C1.1, beeps
	// split into 2 chips for 16 bit access
    ROM_LOAD_EVEN("ibmat.0", 0xf0000, 0x8000, CRC(4995be7a))
    ROM_LOAD_ODD("ibmat.1", 0xf0000, 0x8000, CRC(c32713e4))

	/* I know about a 1984 version in 2 32kb roms */

	/* at, ami bios and diagnostics */
    ROM_LOAD_EVEN("rom01.bin", 0xf0000, 0x8000, CRC(679296a7))
    ROM_LOAD_ODD("rom02.bin", 0xf0000, 0x8000, CRC(65ae1f97))

	/* */
    ROM_LOAD("neat286.bin", 0xf0000, 0x10000, CRC(07985d9b))
	// split into 2 chips for 16 bit access
    ROM_LOAD_EVEN("neat.0", 0xf0000, 0x8000, CRC(4c36e61d))
    ROM_LOAD_ODD("neat.1", 0xf0000, 0x8000, CRC(4e90f294))

	/* most likely 1 chip!, for lower costs */
    ROM_LOAD("at386.bin", 0xf0000, 0x10000, CRC(3df9732a))

	/* at486 */
    ROM_LOAD("at486.bin", 0xf0000, 0x10000, CRC(31214616))

    ROM_LOAD("", 0x??000, 0x2000, CRC())
#endif

ROM_START( ibm5170 )
	ROM_REGION(0x100000,"maincpu", 0)

	ROM_SYSTEM_BIOS( 0, "rev1", "IBM PC/AT 5170 01/10/84")
	ROMX_LOAD("6181028.u27", 0xf0000, 0x8000, CRC(f6573f2a) SHA1(3e52cfa6a6a62b4e8576f4fe076c858c220e6c1a), ROM_SKIP(1) | ROM_BIOS(1)) /* T 6181028 8506AAA // TMM23256P-5878 // (C)IBM CORP 1981,-1984 */
	ROMX_LOAD("6181029.u47", 0xf0001, 0x8000, CRC(7075fbb2) SHA1(a7b885cfd38710c9bc509da1e3ba9b543a2760be), ROM_SKIP(1) | ROM_BIOS(1)) /* T 6181029 8506AAA // TMM23256P-5879 // (C)IBM CORP 1981,-1984 */

	ROM_SYSTEM_BIOS( 1, "rev2", "IBM PC/AT 5170 06/10/85")	/* Another verifaction of these crcs would be nice */
	ROMX_LOAD("6480090.u27", 0xf0000, 0x8000, CRC(99703aa9) SHA1(18022e93a0412c8477e58f8c61a87718a0b9ab0e), ROM_SKIP(1) | ROM_BIOS(2))
	ROMX_LOAD("6480091.u47", 0xf0001, 0x8000, CRC(013ef44b) SHA1(bfa15d2180a1902cb6d38c6eed3740f5617afd16), ROM_SKIP(1) | ROM_BIOS(2))

//  ROM_SYSTEM_BIOS( 2, "atdiag", "IBM PC/AT 5170 w/Super Diagnostics")
//  ROMX_LOAD("atdiage.bin", 0xf8000, 0x4000, CRC(e8855d0c) SHA1(c9d53e61c08da0a64f43d691bf6cadae5393843a), ROM_SKIP(1) | ROM_BIOS(3))
//  ROMX_LOAD("atdiago.bin", 0xf8001, 0x4000, CRC(606fa71d) SHA1(165e45bae7ae2da274f1e645c763c5bfcbde027b), ROM_SKIP(1) | ROM_BIOS(3))

	ROM_REGION(0x08100, "gfx1", 0)
	ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))

	ROM_REGION(0x50000, "gfx2", ROMREGION_ERASE00)

	/* This region holds the original EGA Video bios */
	ROM_REGION(0x4000, "user1", 0)
	ROM_LOAD("6277356.u44", 0x0000, 0x4000, CRC(dc146448) SHA1(dc0794499b3e499c5777b3aa39554bbf0f2cc19b))

	/* 8042 keyboard controller */
	ROM_REGION( 0x0800, "kbdc8042", 0 )
	ROM_LOAD("1503033.bin", 0x0000, 0x0800, CRC(5a81c0d2) SHA1(0100f8789fb4de74706ae7f9473a12ec2b9bd729))

	/* 8051 Keytronic KB3270-PC keyboard row/column decoder */
	ROM_REGION( 0x2000, KEYTRONIC_KB3270PC_CPU, 0 )
	ROM_LOAD("14166.bin", 0x0000, 0x2000, CRC(1aea1b53) SHA1(b75b6d4509036406052157bc34159f7039cdc72e))

	/* Mainboard PALS */
	ROM_REGION( 0x2000, "pals", 0 )
	ROM_LOAD("1501824.pal14l4.u87", 0x0000, 0x0800, NO_DUMP) /* MMI 1501824 717750 // (C)1983 IBM(M) */
	ROM_LOAD("1503135.pal14l4.u130", 0x0800, 0x0800, NO_DUMP) /* MMI 1503135 705075 // (C) IBM CORP 83 */
	/* P/N 6320947 Serial/Parallel ISA expansion card PAL */
	ROM_LOAD("1503085.pal.u14", 0x1000, 0x0800, NO_DUMP) /* MMI 1503085 8449 // (C) IBM CORP 83 */ /* Not sure of type */

	/* Mainboard PROMS */
	ROM_REGION( 0x2000, "proms", 0 )
	ROM_LOAD("1501814.82s123.u115", 0x0000, 0x0800, NO_DUMP) /* N82S123AN 8713 // SK-D 1501814 */
	ROM_LOAD("55x8041.82s147.u72", 0x0800, 0x1000, NO_DUMP) /* S N82S147AN 8709 // V-C55X8041 */
ROM_END


ROM_START( ibm5170a )
	ROM_REGION(0x100000,"maincpu", 0)
//    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	ROM_SYSTEM_BIOS( 0, "rev3", "IBM PC/AT 5170 11/15/85")
	ROMX_LOAD("61x9266.u27", 0xf0000, 0x8000, CRC(4995be7a) SHA1(8e8e5c863ae3b8c55fd394e345d8cca48b6e575c), ROM_SKIP(1) | ROM_BIOS(1))
	ROMX_LOAD("61x9265.u47", 0xf0001, 0x8000, CRC(c32713e4) SHA1(22ed4e2be9f948682891e2fd056a97dbea01203c), ROM_SKIP(1) | ROM_BIOS(1))

	ROM_SYSTEM_BIOS( 1, "3270at", "IBM 3270 PC/AT 5281 11/15/85") /* pretty much just a part string and checksum change from the 5170 rev3 */
	ROMX_LOAD("62x0820.u27", 0xf0000, 0x8000, CRC(e9cc3761) SHA1(ff9373c1a1f34a32fb6acdabc189c61b01acf9aa), ROM_SKIP(1) | ROM_BIOS(2)) /* T 62X0820-U27 8714HAK // TMM23256P-6746 // (C)IBM CORP 1981,-1985 */
	ROMX_LOAD("62x0821.u47", 0xf0001, 0x8000, CRC(b5978ccb) SHA1(2a1aeb9ae3cd7e60fc4c383ca026208b82156810), ROM_SKIP(1) | ROM_BIOS(2)) /* T 62X0821-U47 8715HAK // TMM23256P-6747 // (C)IBM CORP 1981,-1985 */

	ROM_REGION(0x08100, "gfx1", 0)
	ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))

	ROM_REGION(0x50000, "gfx2", ROMREGION_ERASE00)

	/* This region holds the original EGA Video bios */
	ROM_REGION(0x4000, "user1", 0)
	ROM_LOAD("6277356.u44", 0x0000, 0x4000, CRC(dc146448) SHA1(dc0794499b3e499c5777b3aa39554bbf0f2cc19b))

	/* 8042 keyboard controller */
	ROM_REGION( 0x0800, "kbdc8042", 0 )
	ROM_LOAD("1503033.bin", 0x0000, 0x0800, CRC(5a81c0d2) SHA1(0100f8789fb4de74706ae7f9473a12ec2b9bd729))

	/* 8051 Keytronic KB3270-PC keyboard row/column decoder */
	ROM_REGION( 0x2000, KEYTRONIC_KB3270PC_CPU, 0 )
	ROM_LOAD("14166.bin", 0x0000, 0x2000, CRC(1aea1b53) SHA1(b75b6d4509036406052157bc34159f7039cdc72e))

	/* Mainboard PALS */
	ROM_REGION( 0x2000, "pals", 0 )
	ROM_LOAD("1501824.pal14l4.u87", 0x0000, 0x0800, NO_DUMP) /* MMI 1501824 717750 // (C)1983 IBM(M) */
	ROM_LOAD("1503135.pal14l4.u130", 0x0800, 0x0800, NO_DUMP) /* MMI 1503135 705075 // (C) IBM CORP 83 */
	/* P/N 6320947 Serial/Parallel ISA expansion card PAL */
	ROM_LOAD("1503085.pal.u14", 0x1000, 0x0800, NO_DUMP) /* MMI 1503085 8449 // (C) IBM CORP 83 */ /* Not sure of type */

	/* Mainboard PROMS */
	ROM_REGION( 0x2000, "proms", 0 )
	ROM_LOAD("1501814.82s123.u115", 0x0000, 0x0800, NO_DUMP) /* N82S123AN 8713 // SK-D 1501814 */
	ROM_LOAD("55x8041.82s147.u72", 0x0800, 0x1000, NO_DUMP) /* S N82S147AN 8709 // V-C55X8041 */
ROM_END


ROM_START( ibm5162 ) //MB p/n 62x1168
	ROM_REGION16_LE(0x1000000,"maincpu", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))

	ROM_LOAD16_BYTE("78x7460.u34", 0xf0000, 0x8000, CRC(1db4bd8f) SHA1(7be669fbb998d8b4626fefa7cd1208d3b2a88c31)) /* 78X7460 U34 // (C) IBM CORP // 1981-1986 */
	ROM_LOAD16_BYTE("78x7461.u35", 0xf0001, 0x8000, CRC(be14b453) SHA1(ec7c10087dbd53f9c6d1174e8f14212e2aec1818)) /* 78X7461 U35 // (C) IBM CORP // 1981-1986 */

	/* Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f))

	/* 8042 keyboard controller */
	ROM_REGION( 0x0800, "kbdc8042", 0 )
	ROM_LOAD("1503033.bin", 0x0000, 0x0800, CRC(5a81c0d2) SHA1(0100f8789fb4de74706ae7f9473a12ec2b9bd729))

	/* 8051 Keytronic KB3270-PC keyboard row/column decoder */
	ROM_REGION( 0x2000, KEYTRONIC_KB3270PC_CPU, 0 )
	ROM_LOAD("14166.bin", 0x0000, 0x2000, CRC(1aea1b53) SHA1(b75b6d4509036406052157bc34159f7039cdc72e))

	/* Mainboard PALS */
	ROM_REGION( 0x2000, "pals", 0 )
	ROM_LOAD("59x7599.pal20l8.u27", 0x0000, 0x0800, NO_DUMP) /* MMI PAL20L8ACN5 8631 // N59X7599 IBM (C)85 K3 */
	ROM_LOAD("1503135.pal14l4.u81", 0x0800, 0x0800, NO_DUMP) /* MMI 1503135 8625 // (C) IBM CORP 83 */
	/* P/N 6320947 Serial/Parallel ISA expansion card PAL */
	ROM_LOAD("1503085.pal.u14", 0x1000, 0x0800, NO_DUMP) /* MMI 1503085 8449 // (C) IBM CORP 83 */ /* Not sure of type */

	/* Mainboard PROMS */
	ROM_REGION( 0x2000, "proms", 0 )
	ROM_LOAD("1501814.82s123.u72", 0x0000, 0x0800, NO_DUMP) /* N82S123AN 8623 // SK-U 1501814 */
	ROM_LOAD("68x7594.82s147.u90", 0x0800, 0x1000, NO_DUMP) /* S N82S147AN 8629 // VCT 68X7594 (warning, this is an educated guess because it is REALLY hard to read on http://www.yesterpc.com/Hardware/ISA%20motherboard/IBM%20XT286/slideshow/IMG_3608.JPG due to that damn watermark and EXTREME crappy jpeg artifacting. The only letters I'm really sure of are ...7.94 */
ROM_END


ROM_START( i8530286 )
    ROM_REGION(0x1000000,"maincpu", 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	// saved from running machine
    ROM_LOAD16_BYTE("ps2m30.0", 0xe0000, 0x10000, CRC(9965a634) SHA1(c237b1760f8a4561ec47dc70fe2e9df664e56596))
	ROM_RELOAD(0xfe0000,0x10000)
    ROM_LOAD16_BYTE("ps2m30.1", 0xe0001, 0x10000, CRC(1448d3cb) SHA1(13fa26d895ce084278cd5ab1208fc16c80115ebe))
	ROM_RELOAD(0xfe0001,0x10000)

	/* 8042 keyboard controller */
	ROM_REGION( 0x0800, "kbdc8042", 0 )
	ROM_LOAD("1503033.bin", 0x0000, 0x0800, CRC(5a81c0d2) SHA1(0100f8789fb4de74706ae7f9473a12ec2b9bd729))

	/* 8051 Keytronic KB3270-PC keyboard row/column decoder */
	ROM_REGION( 0x2000, KEYTRONIC_KB3270PC_CPU, 0 )
	ROM_LOAD("14166.bin", 0x0000, 0x2000, CRC(1aea1b53) SHA1(b75b6d4509036406052157bc34159f7039cdc72e))
ROM_END


ROM_START( at )
    ROM_REGION(0x1000000,"maincpu", 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD16_BYTE("at110387.1", 0xf0001, 0x8000, CRC(679296a7) SHA1(ae891314cac614dfece686d8e1d74f4763cf40e3))
	ROM_RELOAD(0xff0001,0x8000)
    ROM_LOAD16_BYTE("at110387.0", 0xf0000, 0x8000, CRC(65ae1f97) SHA1(91a29c7deecf7a9afbba330e64e0eee9aafee4d1))
	ROM_RELOAD(0xff0000,0x8000)
	ROM_REGION(0x08100, "gfx1", 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))

	ROM_REGION(0x50000, "gfx2", ROMREGION_ERASE00)

	/* 8042 keyboard controller */
	ROM_REGION( 0x0800, "kbdc8042", 0 )
	ROM_LOAD("1503033.bin", 0x0000, 0x0800, CRC(5a81c0d2) SHA1(0100f8789fb4de74706ae7f9473a12ec2b9bd729))

	/* 8051 Keytronic KB3270-PC keyboard row/column decoder */
	ROM_REGION( 0x2000, KEYTRONIC_KB3270PC_CPU, 0 )
	ROM_LOAD("14166.bin", 0x0000, 0x2000, CRC(1aea1b53) SHA1(b75b6d4509036406052157bc34159f7039cdc72e))
ROM_END


ROM_START( atvga )
    ROM_REGION(0x1000000,"maincpu", 0)
    ROM_LOAD("et4000.bin", 0xc0000, 0x8000, CRC(f01e4be0) SHA1(95d75ff41bcb765e50bd87a8da01835fd0aa01d5))
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD16_BYTE("at110387.1", 0xf0001, 0x8000, CRC(679296a7) SHA1(ae891314cac614dfece686d8e1d74f4763cf40e3))
	ROM_RELOAD(0xff0001,0x8000)
    ROM_LOAD16_BYTE("at110387.0", 0xf0000, 0x8000, CRC(65ae1f97) SHA1(91a29c7deecf7a9afbba330e64e0eee9aafee4d1))
	ROM_RELOAD(0xff0000,0x8000)

	/* 8042 keyboard controller */
	ROM_REGION( 0x0800, "kbdc8042", 0 )
	ROM_LOAD("1503033.bin", 0x0000, 0x0800, CRC(5a81c0d2) SHA1(0100f8789fb4de74706ae7f9473a12ec2b9bd729))

	/* 8051 Keytronic KB3270-PC keyboard row/column decoder */
	ROM_REGION( 0x2000, KEYTRONIC_KB3270PC_CPU, 0 )
	ROM_LOAD("14166.bin", 0x0000, 0x2000, CRC(1aea1b53) SHA1(b75b6d4509036406052157bc34159f7039cdc72e))
ROM_END


ROM_START( neat )
    ROM_REGION(0x1000000,"maincpu", 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD16_BYTE("at030389.0", 0xf0000, 0x8000, CRC(4c36e61d) SHA1(094e8d5e6819889163cb22a2cf559186de782582))
	ROM_RELOAD(0xff0000,0x8000)
    ROM_LOAD16_BYTE("at030389.1", 0xf0001, 0x8000, CRC(4e90f294) SHA1(18c21fd8d7e959e2292a9afbbaf78310f9cad12f))
	ROM_RELOAD(0xff0001,0x8000)
	ROM_REGION(0x08100, "gfx1", 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))

	ROM_REGION(0x50000, "gfx2", ROMREGION_ERASE00)

	/* 8042 keyboard controller */
	ROM_REGION( 0x0800, "kbdc8042", 0 )
	ROM_LOAD("1503033.bin", 0x0000, 0x0800, CRC(5a81c0d2) SHA1(0100f8789fb4de74706ae7f9473a12ec2b9bd729))

	/* 8051 Keytronic KB3270-PC keyboard row/column decoder */
	ROM_REGION( 0x2000, KEYTRONIC_KB3270PC_CPU, 0 )
	ROM_LOAD("14166.bin", 0x0000, 0x2000, CRC(1aea1b53) SHA1(b75b6d4509036406052157bc34159f7039cdc72e))
ROM_END


ROM_START( at386 )
    ROM_REGION(0x1000000,"maincpu", 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD("at386.bin", 0xf0000, 0x10000, CRC(3df9732a) SHA1(def71567dee373dc67063f204ef44ffab9453ead))
	ROM_RELOAD(0xff0000,0x10000)
	ROM_REGION(0x08100, "gfx1", 0)
    ROM_LOAD("cga.chr",     0x01000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))

	/* 8042 keyboard controller */
	ROM_REGION( 0x0800, "kbdc8042", 0 )
	ROM_LOAD("1503033.bin", 0x0000, 0x0800, CRC(5a81c0d2) SHA1(0100f8789fb4de74706ae7f9473a12ec2b9bd729))

	/* 8051 Keytronic KB3270-PC keyboard row/column decoder */
	ROM_REGION( 0x2000, KEYTRONIC_KB3270PC_CPU, 0 )
	ROM_LOAD("14166.bin", 0x0000, 0x2000, CRC(1aea1b53) SHA1(b75b6d4509036406052157bc34159f7039cdc72e))
ROM_END


ROM_START( at486 )
	ROM_REGION(0x1000000, "maincpu", 0)
	ROM_LOAD("wdbios.rom", 0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))

	ROM_SYSTEM_BIOS(0, "at486", "PC/AT 486")	\
	ROMX_LOAD("at486.bin",   0x0f0000, 0x10000, CRC(31214616) SHA1(51b41fa44d92151025fc9ad06e518e906935e689), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "mg48602", "UMC MG-48602")	\
	ROMX_LOAD("mg48602.bin", 0x0f0000, 0x10000, CRC(45797823) SHA1(a5fab258aecabde615e1e97af5911d6cf9938c11), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "ft01232", "Free Tech 01-232")	\
	ROMX_LOAD("ft01232.bin", 0x0f0000, 0x10000, CRC(30efaf92) SHA1(665c8ef05ca052dcc06bb473c9539546bfef1e86), ROM_BIOS(3))
	ROM_COPY("maincpu", 0, 0xff0000, 0x10000)

	ROM_REGION(0x08100, "gfx1", 0)
	ROM_LOAD("cga.chr", 0x01000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))

	/* 8042 keyboard controller */
	ROM_REGION( 0x0800, "kbdc8042", 0 )
	ROM_LOAD("1503033.bin", 0x0000, 0x0800, CRC(5a81c0d2) SHA1(0100f8789fb4de74706ae7f9473a12ec2b9bd729))

	/* 8051 Keytronic KB3270-PC keyboard row/column decoder */
	ROM_REGION( 0x2000, KEYTRONIC_KB3270PC_CPU, 0 )
	ROM_LOAD("14166.bin", 0x0000, 0x2000, CRC(1aea1b53) SHA1(b75b6d4509036406052157bc34159f7039cdc72e))
ROM_END


ROM_START( at586 )
	ROM_REGION32_LE(0x40000, "user1", 0)
    ROM_LOAD("wdbios.rom",  0x08000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD("at586.bin",   0x20000, 0x20000, CRC(717037f5) SHA1(1d49d1b7a4a40d07d1a897b7f8c827754d76f824))

	ROM_REGION(0x08100, "gfx1", 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))

	/* 8042 keyboard controller */
	ROM_REGION( 0x0800, "kbdc8042", 0 )
	ROM_LOAD("1503033.bin", 0x0000, 0x0800, CRC(5a81c0d2) SHA1(0100f8789fb4de74706ae7f9473a12ec2b9bd729))

	/* 8051 Keytronic KB3270-PC keyboard row/column decoder */
	ROM_REGION( 0x2000, KEYTRONIC_KB3270PC_CPU, 0 )
	ROM_LOAD("14166.bin", 0x0000, 0x2000, CRC(1aea1b53) SHA1(b75b6d4509036406052157bc34159f7039cdc72e))
ROM_END

static SYSTEM_CONFIG_START(ibmat)
	CONFIG_RAM_DEFAULT( (640+1024) * 1024 )
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*     YEAR  NAME      PARENT   COMPAT   MACHINE    INPUT       INIT        CONFIG   COMPANY     FULLNAME */
COMP ( 1984, ibm5170,  0,       ibm5160, ibm5170,   atcga,	atega,	    ibmat,   "International Business Machines",  "IBM PC/AT 5170", GAME_NOT_WORKING )
COMP ( 1985, ibm5170a, ibm5170, 0,       ibm5170a,  atcga,      atega,      ibmat,   "International Business Machines",  "IBM PC/AT 5170 8MHz", GAME_NOT_WORKING )
COMP ( 1985, ibm5162,  ibm5170, 0,       ibm5162,   atcga,      atcga,      ibmat,   "International Business Machines",  "IBM PC/XT-286 5162", GAME_NOT_WORKING )
COMP ( 1988, i8530286, ibm5170, 0,       ps2m30286, atvga,	ps2m30286,  ibmat,   "International Business Machines",  "IBM PS2 Model 30 286", GAME_NOT_WORKING )
COMP ( 1987, at,       ibm5170, 0,       ibm5170a,  atcga,	atcga,	    ibmat,   "",  "PC/AT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMP ( 1989, neat,     ibm5170, 0,       ibm5170a,  atcga,	atcga,	    ibmat,   "",  "NEAT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMP ( 1988, at386,    ibm5170, 0,       at386,     atcga,	at386,	    ibmat,   "MITAC INC",  "PC/AT 386(CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMP ( 1990, at486,    ibm5170, 0,       at486,     atcga,	at386,	    ibmat,   "",  "PC/AT 486(CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMP ( 1990, at586,    ibm5170, 0,       at586,     atcga,	at586,	    ibmat,   "",  "PC/AT 586(CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMP ( 1987, atvga,    ibm5170, 0,       atvga,     atvga,	at_vga,     ibmat,   "",  "PC/AT (VGA, MF2 Keyboard)" , GAME_NOT_WORKING )
