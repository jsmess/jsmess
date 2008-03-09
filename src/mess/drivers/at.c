/***************************************************************************

    IBM AT Compatibles

***************************************************************************/

#include "driver.h"
#include "sound/3812intf.h"
#include "machine/8255ppi.h"
#include "machine/uart8250.h"
#include "machine/mc146818.h"
#include "machine/pic8259.h"
#include "devices/printer.h"

#include "machine/pit8253.h"
#include "video/pc_vga.h"
#include "video/pc_cga.h"
#include "video/pc_mda.h"
#include "video/pc_video.h"
#include "includes/pc.h"

#include "machine/pc_hdc.h"
#include "includes/pc_ide.h"
#include "machine/pc_fdc.h"
#include "machine/pc_joy.h"
#include "machine/pckeybrd.h"
#include "includes/pclpt.h"
#include "audio/sblaster.h"
#include "includes/pc_mouse.h"

#include "includes/at.h"
#include "machine/8042kbdc.h"
#include "includes/ps2.h"

#include "machine/pcshare.h"
#include "audio/pc.h"

#include "devices/mflopimg.h"
#include "devices/harddriv.h"
#include "formats/pc_dsk.h"

#include "machine/8237dma.h"
#include "machine/pci.h"
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

static ADDRESS_MAP_START( at_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000000, 0x09ffff) AM_MIRROR(0xff000000) AM_RAMBANK(10)
	AM_RANGE(0x0a0000, 0x0affff) AM_NOP
	AM_RANGE(0x0b0000, 0x0b7fff) AM_NOP
	AM_RANGE(0x0b8000, 0x0bffff) AM_READWRITE(MRA8_RAM, pc_video_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x0c0000, 0x0c7fff) AM_ROM
	AM_RANGE(0x0c8000, 0x0c9fff) AM_ROM
	AM_RANGE(0x0ca000, 0x0cffff) AM_RAM
	AM_RANGE(0x0d0000, 0x0effff) AM_RAM
	AM_RANGE(0x0f0000, 0x0fffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( at386_map, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(24) )
	AM_RANGE(0x00000000, 0x0009ffff) AM_RAMBANK(10)
	AM_RANGE(0x000a0000, 0x000b7fff) AM_NOP
	AM_RANGE(0x000b8000, 0x000bffff) AM_READWRITE(MRA32_RAM, pc_video_videoram32_w) AM_BASE((UINT32 **) &videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x000c0000, 0x000c7fff) AM_ROM
	AM_RANGE(0x000c8000, 0x000cffff) AM_ROM
	AM_RANGE(0x000d0000, 0x000effff) AM_ROM
	AM_RANGE(0x000f0000, 0x000fffff) AM_ROM AM_REGION(REGION_CPU1, 0x0f0000)
	AM_RANGE(0x00ff0000, 0x00ffffff) AM_ROM AM_REGION(REGION_CPU1, 0x0f0000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( at586_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0009ffff) AM_RAMBANK(10)
	AM_RANGE(0x000a0000, 0x000b7fff) AM_NOP
	AM_RANGE(0x000b8000, 0x000bffff) AM_READWRITE(MRA32_RAM, pc_video_videoram32_w) AM_BASE((UINT32 **) &videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0xfffe0000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0x20000)
ADDRESS_MAP_END



static READ8_HANDLER(at_dma8237_1_r)
{
	return dma8237_1_r(machine, offset / 2);
}

static WRITE8_HANDLER(at_dma8237_1_w)
{
	dma8237_1_w(machine, offset / 2, data);
}

static READ32_HANDLER(at32_dma8237_1_r)
{
	return read32le_with_read8_handler(at_dma8237_1_r, machine, offset, mem_mask);
}

static WRITE32_HANDLER(at32_dma8237_1_w)
{
	write32le_with_write8_handler(at_dma8237_1_w, machine, offset, data, mem_mask);
}



static ADDRESS_MAP_START(at_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(dma8237_0_r,				dma8237_0_w)
	AM_RANGE(0x0020, 0x003f) AM_READWRITE(pic8259_0_r,				pic8259_0_w)
	AM_RANGE(0x0040, 0x005f) AM_READWRITE(pit8253_0_r,				pit8253_0_w)
	AM_RANGE(0x0060, 0x006f) AM_READWRITE(kbdc8042_8_r,				kbdc8042_8_w)
	AM_RANGE(0x0070, 0x007f) AM_READWRITE(mc146818_port_r,			mc146818_port_w)
	AM_RANGE(0x0080, 0x009f) AM_READWRITE(at_page8_r,				at_page8_w)
	AM_RANGE(0x00a0, 0x00bf) AM_READWRITE(pic8259_1_r,				pic8259_1_w)
	AM_RANGE(0x00c0, 0x00df) AM_READWRITE(at_dma8237_1_r,			at_dma8237_1_w)
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
	AM_RANGE(0x0388, 0x0388) AM_READWRITE(YM3812_status_port_0_r,	YM3812_control_port_0_w)
	AM_RANGE(0x0389, 0x0389) AM_WRITE(								YM3812_write_port_0_w)
#endif
	AM_RANGE(0x03bc, 0x03be) AM_READWRITE(pc_parallelport0_r,		pc_parallelport0_w)
	AM_RANGE(0x03e8, 0x03ef) AM_READWRITE(uart8250_2_r,				uart8250_2_w)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE(pc_fdc_r,					pc_fdc_w)
	AM_RANGE(0x03f8, 0x03ff) AM_READWRITE(uart8250_0_r,				uart8250_0_w)
ADDRESS_MAP_END



static ADDRESS_MAP_START(at386_io, ADDRESS_SPACE_IO, 32)
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(dma8237_32le_0_r,			dma8237_32le_0_w)
	AM_RANGE(0x0020, 0x003f) AM_READWRITE(pic8259_32le_0_r,			pic8259_32le_0_w)
	AM_RANGE(0x0040, 0x005f) AM_READWRITE(pit8253_32le_0_r,			pit8253_32le_0_w)
	AM_RANGE(0x0060, 0x006f) AM_READWRITE(kbdc8042_32le_r,			kbdc8042_32le_w)
	AM_RANGE(0x0070, 0x007f) AM_READWRITE(mc146818_port32le_r,		mc146818_port32le_w)
	AM_RANGE(0x0080, 0x009f) AM_READWRITE(at_page32_r,				at_page32_w)
	AM_RANGE(0x00a0, 0x00bf) AM_READWRITE(pic8259_32le_1_r,			pic8259_32le_1_w)
	AM_RANGE(0x00c0, 0x00df) AM_READWRITE(at32_dma8237_1_r,			at32_dma8237_1_w)
	AM_RANGE(0x0278, 0x027f) AM_READWRITE(pc32le_parallelport2_r,	pc32le_parallelport2_w)
	AM_RANGE(0x02e8, 0x02ef) AM_READWRITE(uart8250_32le_3_r,		uart8250_32le_3_w)
	AM_RANGE(0x02f8, 0x02ff) AM_READWRITE(uart8250_32le_1_r,		uart8250_32le_1_w)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc32le_HDC1_r,			pc32le_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc32le_HDC2_r,			pc32le_HDC2_w)
	AM_RANGE(0x0378, 0x037f) AM_READWRITE(pc32le_parallelport1_r,	pc32le_parallelport1_w)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE(pc32le_fdc_r,				pc32le_fdc_w)
	AM_RANGE(0x03bc, 0x03bf) AM_READWRITE(pc32le_parallelport0_r,	pc32le_parallelport0_w)
	AM_RANGE(0x03f8, 0x03ff) AM_READWRITE(uart8250_32le_0_r,		uart8250_32le_0_w)
ADDRESS_MAP_END



static ADDRESS_MAP_START(at586_io, ADDRESS_SPACE_IO, 32)
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(dma8237_32le_0_r,			dma8237_32le_0_w)
	AM_RANGE(0x0020, 0x003f) AM_READWRITE(pic8259_32le_0_r,			pic8259_32le_0_w)
	AM_RANGE(0x0040, 0x005f) AM_READWRITE(pit8253_32le_0_r,			pit8253_32le_0_w)
	AM_RANGE(0x0060, 0x006f) AM_READWRITE(kbdc8042_32le_r,			kbdc8042_32le_w)
	AM_RANGE(0x0070, 0x007f) AM_READWRITE(mc146818_port32le_r,		mc146818_port32le_w)
	AM_RANGE(0x0080, 0x009f) AM_READWRITE(at_page32_r,				at_page32_w)
	AM_RANGE(0x00a0, 0x00bf) AM_READWRITE(pic8259_32le_1_r,			pic8259_32le_1_w)
	AM_RANGE(0x00c0, 0x00df) AM_READWRITE(at32_dma8237_1_r,			at32_dma8237_1_w)
	AM_RANGE(0x0278, 0x027f) AM_READWRITE(pc32le_parallelport2_r,		pc32le_parallelport2_w)
	AM_RANGE(0x02e8, 0x02ef) AM_READWRITE(uart8250_32le_3_r,		uart8250_32le_3_w)
	AM_RANGE(0x02f8, 0x02ff) AM_READWRITE(uart8250_32le_1_r,		uart8250_32le_1_w)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc32le_HDC1_r,				pc32le_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc32le_HDC2_r,				pc32le_HDC2_w)
	AM_RANGE(0x0378, 0x037f) AM_READWRITE(pc32le_parallelport1_r,		pc32le_parallelport1_w)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE(pc32le_fdc_r,				pc32le_fdc_w)
	AM_RANGE(0x03bc, 0x03bf) AM_READWRITE(pc32le_parallelport0_r,		pc32le_parallelport0_w)
	AM_RANGE(0x03f8, 0x03ff) AM_READWRITE(uart8250_32le_0_r,		uart8250_32le_0_w)
	AM_RANGE(0x0cf8, 0x0cff) AM_READWRITE(pci_32le_r,				pci_32le_w)
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
	AM_RANGE(0x0388, 0x0388) AM_READWRITE(YM3812_status_port_0_r,	YM3812_control_port_0_w)
	AM_RANGE(0x0389, 0x0389) AM_WRITE(								YM3812_write_port_0_w)
#endif
	AM_RANGE(0x03bc, 0x03be) AM_READWRITE(pc_parallelport0_r,		pc_parallelport0_w)
	AM_RANGE(0x03e8, 0x03ef) AM_READWRITE(uart8250_2_r,				uart8250_2_w)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE(pc_fdc_r,					pc_fdc_w)
	AM_RANGE(0x03f8, 0x03ff) AM_READWRITE(uart8250_0_r,				uart8250_0_w)
ADDRESS_MAP_END
#endif

static INPUT_PORTS_START( atcga )
	PORT_START /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

    PORT_START /* IN1 */
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

	PORT_START /* IN2 */
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

    PORT_START /* IN3 */
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

	PORT_INCLUDE( at_keyboard )		/* IN4 - IN11 */
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
INPUT_PORTS_END

static INPUT_PORTS_START( atvga )
	PORT_START /* IN0 */
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

    PORT_START /* IN1 */
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

	PORT_START /* IN2 */
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

    PORT_START /* IN3 */
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

	PORT_INCLUDE( at_keyboard )		/* IN4 - IN11 */
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
INPUT_PORTS_END

static const unsigned i286_address_mask = 0x00ffffff;


#if defined(ADLIB)
/* irq line not connected to pc on adlib cards (and compatibles) */
static void irqhandler(int linestate) {}

static const struct YM3812interface ym3812_interface =
{
	irqhandler
};
#endif



#define MDRV_CPU_ATPC(mem, port, type, clock)	\
	MDRV_CPU_ADD_TAG("main", type, clock)					\
	MDRV_CPU_PROGRAM_MAP(mem##_map, 0)				\
	MDRV_CPU_IO_MAP(port##_io, 0)					\
	MDRV_CPU_CONFIG(i286_address_mask)

static MACHINE_DRIVER_START( atcga )
	/* basic machine hardware */
	MDRV_CPU_ATPC(at, at, I80286, 12000000)

	MDRV_MACHINE_START( at )
	MDRV_MACHINE_RESET( at )

	MDRV_IMPORT_FROM( pcvideo_cga )

	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(pc_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MDRV_SOUND_ADD(YM3812, ym3812_StdClock)
	MDRV_SOUND_CONFIG(ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD(SAA1099, 8000000)	/* running at 8 MHz ISA bus speed? */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD(SAA1099, 8000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif

	MDRV_NVRAM_HANDLER( mc146818 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ps2m30286 )
	/* basic machine hardware */
	MDRV_CPU_ATPC(at, at, I80286, 12000000)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	MDRV_IMPORT_FROM( pcvideo_vga )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(pc_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MDRV_SOUND_ADD(YM3812, ym3812_StdClock)
	MDRV_SOUND_CONFIG(ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD(SAA1099, 8000000)	/* running at 8 MHz ISA bus speed? */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD(SAA1099, 8000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif

	MDRV_NVRAM_HANDLER( mc146818 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( atvga )
	/* basic machine hardware */
	MDRV_CPU_ATPC(at, at, I80286, 12000000)

	MDRV_IMPORT_FROM( pcvideo_vga )

	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	MDRV_MACHINE_START( at )
	MDRV_MACHINE_RESET( at )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(pc_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MDRV_SOUND_ADD(YM3812, ym3812_StdClock)
	MDRV_SOUND_CONFIG(ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD(SAA1099, 8000000)	/* running at 8 MHz ISA bus speed? */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD(SAA1099, 8000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_NVRAM_HANDLER( mc146818 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( at386 )
    /* basic machine hardware */
	/* original at 6 mhz, at03 8 megahertz */
	MDRV_CPU_ATPC(at386, at386, I386, 12000000)

	MDRV_MACHINE_START( at )
	MDRV_MACHINE_RESET( at )

	MDRV_IMPORT_FROM( pcvideo_cga )

	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(pc_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MDRV_SOUND_ADD(YM3812, ym3812_StdClock)
	MDRV_SOUND_CONFIG(ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD(SAA1099, 8000000)	/* running at 8 MHz ISA bus speed? */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD(SAA1099, 8000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_NVRAM_HANDLER( mc146818 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( at486 )
	MDRV_IMPORT_FROM( at386 )

	MDRV_CPU_REPLACE("main", I486, 12000000)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( at586 )
	MDRV_IMPORT_FROM( at386 )

	MDRV_CPU_REPLACE("main", PENTIUM, 60000000)
	MDRV_CPU_PROGRAM_MAP(at586_map, 0)
	MDRV_CPU_IO_MAP(at586_io, 0)
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

ROM_START( ibmat )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD16_BYTE("at111585.0", 0xf0000, 0x8000, CRC(4995be7a) SHA1(8e8e5c863ae3b8c55fd394e345d8cca48b6e575c))
	ROM_RELOAD(0xff0000,0x8000)
    ROM_LOAD16_BYTE("at111585.1", 0xf0001, 0x8000, CRC(c32713e4) SHA1(22ed4e2be9f948682891e2fd056a97dbea01203c))
	ROM_RELOAD(0xff0001,0x8000)
	ROM_REGION(0x08100, REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))
ROM_END

ROM_START( i8530286 )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	// saved from running machine
    ROM_LOAD16_BYTE("ps2m30.0", 0xe0000, 0x10000, CRC(9965a634) SHA1(c237b1760f8a4561ec47dc70fe2e9df664e56596))
	ROM_RELOAD(0xfe0000,0x10000)
    ROM_LOAD16_BYTE("ps2m30.1", 0xe0001, 0x10000, CRC(1448d3cb) SHA1(13fa26d895ce084278cd5ab1208fc16c80115ebe))
	ROM_RELOAD(0xfe0001,0x10000)
ROM_END

ROM_START( at )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD16_BYTE("at110387.1", 0xf0001, 0x8000, CRC(679296a7) SHA1(ae891314cac614dfece686d8e1d74f4763cf40e3))
	ROM_RELOAD(0xff0001,0x8000)
    ROM_LOAD16_BYTE("at110387.0", 0xf0000, 0x8000, CRC(65ae1f97) SHA1(91a29c7deecf7a9afbba330e64e0eee9aafee4d1))
	ROM_RELOAD(0xff0000,0x8000)
	ROM_REGION(0x08100, REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))
ROM_END

ROM_START( atvga )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("et4000.bin", 0xc0000, 0x8000, CRC(f01e4be0) SHA1(95d75ff41bcb765e50bd87a8da01835fd0aa01d5))
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD16_BYTE("at110387.1", 0xf0001, 0x8000, CRC(679296a7) SHA1(ae891314cac614dfece686d8e1d74f4763cf40e3))
	ROM_RELOAD(0xff0001,0x8000)
    ROM_LOAD16_BYTE("at110387.0", 0xf0000, 0x8000, CRC(65ae1f97) SHA1(91a29c7deecf7a9afbba330e64e0eee9aafee4d1))
	ROM_RELOAD(0xff0000,0x8000)
ROM_END

ROM_START( neat )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD16_BYTE("at030389.0", 0xf0000, 0x8000, CRC(4c36e61d) SHA1(094e8d5e6819889163cb22a2cf559186de782582))
	ROM_RELOAD(0xff0000,0x8000)
    ROM_LOAD16_BYTE("at030389.1", 0xf0001, 0x8000, CRC(4e90f294) SHA1(18c21fd8d7e959e2292a9afbbaf78310f9cad12f))
	ROM_RELOAD(0xff0001,0x8000)
	ROM_REGION(0x08100, REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))
ROM_END

ROM_START( at386 )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD("at386.bin", 0xf0000, 0x10000, CRC(3df9732a) SHA1(def71567dee373dc67063f204ef44ffab9453ead))
	ROM_RELOAD(0xff0000,0x10000)
	ROM_REGION(0x08100, REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))
ROM_END

ROM_START( at486 )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD("at486.bin", 0xf0000, 0x10000, CRC(31214616) SHA1(51b41fa44d92151025fc9ad06e518e906935e689))
	ROM_RELOAD(0xff0000,0x10000)
	ROM_REGION(0x08100, REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))
ROM_END

ROM_START( at586 )
	ROM_REGION32_LE(0x40000, REGION_USER1, 0)
    ROM_LOAD("wdbios.rom",  0x08000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD("at586.bin",   0x20000, 0x20000, CRC(717037f5) SHA1(1d49d1b7a4a40d07d1a897b7f8c827754d76f824))

	ROM_REGION(0x08100, REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))
ROM_END

static void ibmat_printer_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* printer */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 3; break;

		default:										printer_device_getinfo(devclass, state, info); break;
	}
}

static void ibmat_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_FLOPPY_OPTIONS:				info->p = (void *) floppyoptions_pc; break;

		default:										floppy_device_getinfo(devclass, state, info); break;
	}
}

static void ibmat_harddisk_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* harddisk */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 4; break;

		default:										harddisk_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(ibmat)
	CONFIG_RAM_DEFAULT( (640+1024) * 1024 )
	CONFIG_DEVICE(ibmat_printer_getinfo)
	CONFIG_DEVICE(ibmat_floppy_getinfo)
	CONFIG_DEVICE(ibmat_harddisk_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*     YEAR     NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        CONFIG   COMPANY     FULLNAME */
COMP ( 1985,	ibmat,		0,		ibmpc,	atcga,		atcga,		atcga,	    ibmat,   "International Business Machines",  "IBM PC/AT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMP ( 1988,	i8530286,	ibmat,	0,		ps2m30286,	atvga,		ps2m30286,	ibmat,   "International Business Machines",  "IBM PS2 Model 30 286", GAME_NOT_WORKING )
COMP ( 1987,	at,			ibmat,	0,		atcga,      atcga,		atcga,	    ibmat,   "",  "PC/AT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMP ( 1989,	neat,		ibmat,	0,		atcga,      atcga,		atcga,	    ibmat,   "",  "NEAT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMP ( 1988,	at386,		ibmat,	0,		at386,      atcga,		at386,	    ibmat,   "MITAC INC",  "PC/AT 386(CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMP ( 1990,	at486,		ibmat,	0,		at486,      atcga,		at386,	    ibmat,   "",  "PC/AT 486(CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMP ( 1990,	at586,		ibmat,	0,		at586,      atcga,		at586,	    ibmat,   "",  "PC/AT 586(CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMP ( 1987,	atvga,		0,		0,		atvga,      atvga,		at_vga,     ibmat,   "",  "PC/AT (VGA, MF2 Keyboard)" , 0)
