/***************************************************************************

    IBM AT Compatibles

***************************************************************************/

/* mingw-gcc defines this */
#ifdef i386
#undef i386
#endif /* i386 */

#include "emu.h"
#include "cpu/i86/i86.h"
#include "cpu/i86/i286.h"
#include "cpu/i386/i386.h"
#include "sound/3812intf.h"
#include "machine/i8255a.h"
#include "machine/ins8250.h"
#include "machine/mc146818.h"
#include "machine/pic8259.h"
#include "machine/i82371ab.h"
#include "machine/i82439tx.h"
#include "machine/cs4031.h"
#include "machine/pit8253.h"
#include "video/pc_vga_mess.h"
#include "video/pc_cga.h"
#include "video/pc_ega.h"

#include "machine/pc_hdc.h"
#include "includes/pc_ide.h"
#include "machine/pc_fdc.h"
#include "machine/pc_joy.h"
#include "machine/pc_lpt.h"
#include "audio/sblaster.h"
#include "includes/pc_mouse.h"

#include "includes/at.h"
#include "machine/at_keybc.h"
#include "includes/ps2.h"

#include "imagedev/flopdrv.h"
#include "imagedev/harddriv.h"
#include "formats/pc_dsk.h"

#include "machine/8237dma.h"
#include "machine/pci.h"
#include "machine/kb_keytro.h"

#include "sound/dac.h"
#include "sound/speaker.h"
#include "sound/saa1099.h"
#include "machine/ram.h"
#include "machine/nvram.h"
#include "memconv.h"


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

static ADDRESS_MAP_START( at16_map, AS_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x09ffff) AM_MIRROR(0xff000000) AM_RAMBANK("bank10")
	AM_RANGE(0x0c0000, 0x0c7fff) AM_ROM
	AM_RANGE(0x0c8000, 0x0cffff) AM_ROM
	AM_RANGE(0x0d0000, 0x0effff) AM_RAM
	AM_RANGE(0x0f0000, 0x0fffff) AM_ROM
	AM_RANGE(0xff0000, 0xffffff) AM_ROM AM_REGION("maincpu", 0x0f0000)
ADDRESS_MAP_END



static ADDRESS_MAP_START( at386_map, AS_PROGRAM, 32 )
	ADDRESS_MAP_GLOBAL_MASK(0x00ffffff)
	AM_RANGE(0x00000000, 0x0009ffff) AM_RAMBANK("bank10")
	AM_RANGE(0x000a0000, 0x000bffff) AM_NOP
	AM_RANGE(0x000c0000, 0x000c7fff) AM_ROM
	AM_RANGE(0x000c8000, 0x000cffff) AM_ROM
	AM_RANGE(0x000d0000, 0x000effff) AM_ROM
	AM_RANGE(0x000f0000, 0x000fffff) AM_ROM
	AM_RANGE(0x00800000, 0x00800bff) AM_RAM AM_SHARE("nvram")
	AM_RANGE(0x00ff0000, 0x00ffffff) AM_ROM AM_REGION("maincpu", 0x0f0000)
ADDRESS_MAP_END

// memory is mostly handled by the chipset
static ADDRESS_MAP_START( ct486_map, AS_PROGRAM, 32 )
	AM_RANGE(0x00800000, 0x00800bff) AM_RAM AM_SHARE("nvram")
ADDRESS_MAP_END


static ADDRESS_MAP_START( at586_map, AS_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0009ffff) AM_RAMBANK("bank10")
	AM_RANGE(0x000a0000, 0x000bffff) AM_NOP
	AM_RANGE(0x00800000, 0x00800bff) AM_RAM AM_SHARE("nvram")
	AM_RANGE(0xfffe0000, 0xffffffff) AM_ROM AM_REGION("user1", 0x20000)
ADDRESS_MAP_END



static READ8_DEVICE_HANDLER(at_dma8237_1_r)
{
	return i8237_r( device, offset / 2);
}

static WRITE8_DEVICE_HANDLER(at_dma8237_1_w)
{
	i8237_w( device, offset / 2, data);
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

static READ8_HANDLER( at_keybc_r )
{
	switch (offset)
	{
	case 0: return downcast<at_keyboard_controller_device *>(space->machine().device("keybc"))->data_r(*space, 0);
	case 1: return at_portb_r(space, 0);
	}

	return 0xff;
}

static WRITE8_HANDLER( at_keybc_w )
{
	switch (offset)
	{
	case 0: downcast<at_keyboard_controller_device *>(space->machine().device("keybc"))->data_w(*space, 0, data); break;
	case 1: at_portb_w(space, 0, data); break;
	}
}

static ADDRESS_MAP_START(at16_io, AS_IO, 16)
	AM_RANGE(0x0000, 0x001f) AM_DEVREADWRITE8("dma8237_1", i8237_r, i8237_w, 0xffff)
	AM_RANGE(0x0020, 0x003f) AM_DEVREADWRITE8("pic8259_master", pic8259_r, pic8259_w, 0xffff)
	AM_RANGE(0x0040, 0x005f) AM_DEVREADWRITE8("pit8254", pit8253_r, pit8253_w, 0xffff)
	AM_RANGE(0x0060, 0x0063) AM_READWRITE8(at_keybc_r, at_keybc_w, 0xffff)
	AM_RANGE(0x0064, 0x0067) AM_DEVREADWRITE8_MODERN("keybc", at_keyboard_controller_device, status_r, command_w, 0xffff)
	AM_RANGE(0x0070, 0x007f) AM_DEVREADWRITE8_MODERN("rtc", mc146818_device, read, write , 0xffff)
	AM_RANGE(0x0080, 0x009f) AM_READWRITE8(at_page8_r,               at_page8_w, 0xffff)
	AM_RANGE(0x00a0, 0x00bf) AM_DEVREADWRITE8("pic8259_slave", pic8259_r, pic8259_w, 0xffff)
	AM_RANGE(0x00c0, 0x00df) AM_DEVREADWRITE8("dma8237_2", at_dma8237_1_r, at_dma8237_1_w, 0xffff)
	AM_RANGE(0x01f0, 0x01f7) AM_READWRITE8(at_mfm_0_r,               at_mfm_0_w, 0xffff)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE8(pc_JOY_r,                 pc_JOY_w, 0xffff)
	AM_RANGE(0x0220, 0x022f) AM_READWRITE8(soundblaster_r,           soundblaster_w, 0xffff)
	AM_RANGE(0x0278, 0x027f) AM_DEVREADWRITE8("lpt_2", pc_lpt_r, pc_lpt_w, 0x00ff)
	AM_RANGE(0x02e8, 0x02ef) AM_DEVREADWRITE8("ns16450_3", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE8("ns16450_1", ins8250_r, ins8250_w, 0xffff)
    AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc16le_HDC1_r, pc16le_HDC1_w)
    AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc16le_HDC2_r, pc16le_HDC2_w)
	AM_RANGE(0x0378, 0x037f) AM_DEVREADWRITE8("lpt_1", pc_lpt_r, pc_lpt_w, 0x00ff)
#ifdef ADLIB
	AM_RANGE(0x0388, 0x0389) AM_DEVREADWRITE("ym3812", at16_388_r, at16_388_w )
#endif
	AM_RANGE(0x03bc, 0x03bf) AM_DEVREADWRITE8("lpt_0", pc_lpt_r, pc_lpt_w, 0x00ff)
	AM_RANGE(0x03e8, 0x03ef) AM_DEVREADWRITE8("ns16450_2", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE8(pc_fdc_r,                 pc_fdc_w, 0xffff)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE8("ns16450_0", ins8250_r, ins8250_w, 0xffff)
ADDRESS_MAP_END


static ADDRESS_MAP_START(at386_io, AS_IO, 32)
	AM_RANGE(0x0000, 0x001f) AM_DEVREADWRITE8("dma8237_1", i8237_r, i8237_w, 0xffffffff)
	AM_RANGE(0x0020, 0x003f) AM_DEVREADWRITE8("pic8259_master", pic8259_r, pic8259_w, 0xffffffff)
	AM_RANGE(0x0040, 0x005f) AM_DEVREADWRITE8("pit8254", pit8253_r, pit8253_w, 0xffffffff)
	AM_RANGE(0x0060, 0x0063) AM_READWRITE8(at_keybc_r, at_keybc_w, 0xffff)
	AM_RANGE(0x0064, 0x0067) AM_DEVREADWRITE8_MODERN("keybc", at_keyboard_controller_device, status_r, command_w, 0xffff)
	AM_RANGE(0x0070, 0x007f) AM_DEVREADWRITE8_MODERN("rtc", mc146818_device, read, write , 0xffffffff)
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


static READ32_HANDLER( ct486_chipset_r )
{
	at_state *state = space->machine().driver_data<at_state>();

	if (ACCESSING_BITS_0_7)
		return pic8259_r(state->m_pic8259_master, 0);

	if (ACCESSING_BITS_8_15)
		return pic8259_r(state->m_pic8259_master, 1) << 8;

	if (ACCESSING_BITS_24_31)
		return downcast<cs4031_device *>(space->machine().device("cs4031"))->data_r(*space, 0, 0) << 24;

	return 0xffffffff;
}

static WRITE32_HANDLER( ct486_chipset_w )
{
	at_state *state = space->machine().driver_data<at_state>();

	if (ACCESSING_BITS_0_7)
		pic8259_w(state->m_pic8259_master, 0, data);

	if (ACCESSING_BITS_8_15)
		pic8259_w(state->m_pic8259_master, 1, data >> 8);

	if (ACCESSING_BITS_16_23)
		downcast<cs4031_device *>(space->machine().device("cs4031"))->address_w(*space, 0, data >> 16, 0);

	if (ACCESSING_BITS_24_31)
		downcast<cs4031_device *>(space->machine().device("cs4031"))->data_w(*space, 0, data >> 24, 0);
}

static ADDRESS_MAP_START( ct486_io, AS_IO, 32 )
	AM_RANGE(0x0000, 0x001f) AM_DEVREADWRITE8("dma8237_1", i8237_r, i8237_w, 0xffffffff)
	AM_RANGE(0x0020, 0x0023) AM_READWRITE(ct486_chipset_r, ct486_chipset_w)
	AM_RANGE(0x0040, 0x005f) AM_DEVREADWRITE8("pit8254", pit8253_r, pit8253_w, 0xffffffff)
	AM_RANGE(0x0060, 0x0063) AM_READWRITE8(at_keybc_r, at_keybc_w, 0xffff)
	AM_RANGE(0x0064, 0x0067) AM_DEVREADWRITE8_MODERN("keybc", at_keyboard_controller_device, status_r, command_w, 0xffff)
	AM_RANGE(0x0070, 0x007f) AM_DEVREADWRITE8_MODERN("rtc", mc146818_device, read, write , 0xffffffff)
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


static ADDRESS_MAP_START(at586_io, AS_IO, 32)
	AM_RANGE(0x0000, 0x001f) AM_DEVREADWRITE8("dma8237_1", i8237_r, i8237_w, 0xffffffff)
	AM_RANGE(0x0020, 0x003f) AM_DEVREADWRITE8("pic8259_master", pic8259_r, pic8259_w, 0xffffffff)
	AM_RANGE(0x0040, 0x005f) AM_DEVREADWRITE8("pit8254", pit8253_r, pit8253_w, 0xffffffff)
	AM_RANGE(0x0060, 0x0063) AM_READWRITE8(at_keybc_r, at_keybc_w, 0xffff)
	AM_RANGE(0x0064, 0x0067) AM_DEVREADWRITE8_MODERN("keybc", at_keyboard_controller_device, status_r, command_w, 0xffff)
	AM_RANGE(0x0070, 0x007f) AM_DEVREADWRITE8_MODERN("rtc", mc146818_device, read, write , 0xffffffff)
	AM_RANGE(0x0080, 0x009f) AM_READWRITE8(at_page8_r,				at_page8_w, 0xffffffff)
	AM_RANGE(0x00a0, 0x00bf) AM_DEVREADWRITE8("pic8259_slave", pic8259_r, pic8259_w, 0xffffffff)
	AM_RANGE(0x00c0, 0x00df) AM_DEVREADWRITE8("dma8237_2", at_dma8237_1_r, at_dma8237_1_w, 0xffffffff)
	AM_RANGE(0x00e0, 0x00ef) AM_NOP // used for timing?
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
static ADDRESS_MAP_START(ps2m30286_io, AS_IO, 8)
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

static ADDRESS_MAP_START(megapc_io, AS_IO, 32)
	AM_RANGE(0x0000, 0x001f) AM_DEVREADWRITE8("dma8237_1", i8237_r, i8237_w, 0xffffffff)
	AM_RANGE(0x0020, 0x003f) AM_DEVREADWRITE8("pic8259_master", pic8259_r, pic8259_w, 0xffffffff)
	AM_RANGE(0x0040, 0x005f) AM_DEVREADWRITE8("pit8254", pit8253_r, pit8253_w, 0xffffffff)
	//AM_RANGE(0x0060, 0x006f) AM_READWRITE(at_kbdc8042_32le_r,      at_kbdc8042_32le_w)
	AM_RANGE(0x0070, 0x007f) AM_DEVREADWRITE8_MODERN("rtc", mc146818_device, read, write , 0xffffffff)
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
	PORT_INCLUDE( pcvideo_cga )
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
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(pc),
	"floppy_5_25"
};

static const at_keyboard_controller_interface keyboard_controller_intf =
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_RESET),
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_A20),
	DEVCB_DEVICE_LINE("pic8259_master", pic8259_ir1_w),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE("keyboard", kb_keytronic_clock_w),
	DEVCB_DEVICE_LINE("keyboard", kb_keytronic_data_w)
};

static const kb_keytronic_interface at_keytronic_intf =
{
	DEVCB_DEVICE_LINE_MEMBER("keybc", at_keyboard_controller_device, keyboard_clock_w),
	DEVCB_DEVICE_LINE_MEMBER("keybc", at_keyboard_controller_device, keyboard_data_w)
};


static MACHINE_CONFIG_START( ibm5170, at_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I80286, 6000000 /*6000000*/)
	MCFG_CPU_PROGRAM_MAP(at16_map)
	MCFG_CPU_IO_MAP(at16_io)
	MCFG_CPU_CONFIG(i286_address_mask)

	MCFG_QUANTUM_TIME(attotime::from_hz(60))

	MCFG_PIT8254_ADD( "pit8254", at_pit8254_config )

	MCFG_I8237_ADD( "dma8237_1", XTAL_14_31818MHz/3, at_dma8237_1_config )

	MCFG_I8237_ADD( "dma8237_2", XTAL_14_31818MHz/3, at_dma8237_2_config )

	MCFG_PIC8259_ADD( "pic8259_master", at_pic8259_master_config )

	MCFG_PIC8259_ADD( "pic8259_slave", at_pic8259_slave_config )

	MCFG_NS16450_ADD( "ns16450_0", ibm5170_com_interface[0] ) /* Verified: IBM P/N 6320947 Serial/Parallel card uses an NS16450N */
	MCFG_NS16450_ADD( "ns16450_1", ibm5170_com_interface[1] )
	MCFG_NS16450_ADD( "ns16450_2", ibm5170_com_interface[2] )
	MCFG_NS16450_ADD( "ns16450_3", ibm5170_com_interface[3] )
	MCFG_MACHINE_START( at )
	MCFG_MACHINE_RESET( at )

	MCFG_FRAGMENT_ADD( pcvideo_ega )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MCFG_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MCFG_SOUND_CONFIG(at_ym3812_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MCFG_SOUND_ADD("saa1099.1", SAA1099, 8000000)	/* running at 8 MHz ISA bus speed? */
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MCFG_SOUND_ADD("saa1099.2", SAA1099, 8000000)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif

	MCFG_AT_KEYBOARD_CONTROLLER_ADD("keybc", XTAL_12MHz, keyboard_controller_intf)
	MCFG_KB_KEYTRONIC_ADD("keyboard", at_keytronic_intf)

	MCFG_MC146818_ADD( "rtc", MC146818_STANDARD )

	/* printers */
	MCFG_PC_LPT_ADD("lpt_0", at_lpt_config)
	MCFG_PC_LPT_ADD("lpt_1", at_lpt_config)
	MCFG_PC_LPT_ADD("lpt_2", at_lpt_config)

	/* harddisk */
	MCFG_FRAGMENT_ADD( pc_hdc )

	MCFG_UPD765A_ADD("upd765", pc_fdc_upd765_not_connected_interface)

	MCFG_FLOPPY_2_DRIVES_ADD(ibmat_floppy_config)

	/* software lists */
	MCFG_SOFTWARE_LIST_ADD("disk_list","ibm5170")

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1664K")
MACHINE_CONFIG_END


static MACHINE_CONFIG_DERIVED( ibm5170a, ibm5170 )
	MCFG_CPU_MODIFY("maincpu")
	MCFG_CPU_CLOCK(8000000)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( ec1849, ibm5170 )
	MCFG_CPU_MODIFY("maincpu")
	MCFG_CPU_CLOCK(12000000)
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( ibm5162, at_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I80286, 6000000 /*6000000*/)
	MCFG_CPU_PROGRAM_MAP(at16_map)
	MCFG_CPU_IO_MAP(at16_io)
	MCFG_CPU_CONFIG(i286_address_mask)

	MCFG_QUANTUM_TIME(attotime::from_hz(60))

	MCFG_PIT8254_ADD( "pit8254", at_pit8254_config )

	MCFG_I8237_ADD( "dma8237_1", XTAL_14_31818MHz/3, at_dma8237_1_config )

	MCFG_I8237_ADD( "dma8237_2", XTAL_14_31818MHz/3, at_dma8237_2_config )

	MCFG_PIC8259_ADD( "pic8259_master", at_pic8259_master_config )

	MCFG_PIC8259_ADD( "pic8259_slave", at_pic8259_slave_config )

	MCFG_NS16450_ADD( "ns16450_0", ibm5170_com_interface[0] )			/* TODO: verify model */
	MCFG_NS16450_ADD( "ns16450_1", ibm5170_com_interface[1] )			/* TODO: verify model */
	MCFG_NS16450_ADD( "ns16450_2", ibm5170_com_interface[2] )			/* TODO: verify model */
	MCFG_NS16450_ADD( "ns16450_3", ibm5170_com_interface[3] )			/* TODO: verify model */

	MCFG_MACHINE_START( at )
	MCFG_MACHINE_RESET( at )

	MCFG_FRAGMENT_ADD( pcvideo_cga )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MCFG_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MCFG_SOUND_CONFIG(at_ym3812_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif

	MCFG_AT_KEYBOARD_CONTROLLER_ADD("keybc", XTAL_12MHz, keyboard_controller_intf)
	MCFG_KB_KEYTRONIC_ADD("keyboard", at_keytronic_intf)

	MCFG_MC146818_ADD( "rtc", MC146818_STANDARD )

	/* printers */
	MCFG_PC_LPT_ADD("lpt_0", at_lpt_config)
	MCFG_PC_LPT_ADD("lpt_1", at_lpt_config)
	MCFG_PC_LPT_ADD("lpt_2", at_lpt_config)

	/* harddisk */
	MCFG_FRAGMENT_ADD( pc_hdc )

	MCFG_UPD765A_ADD("upd765", pc_fdc_upd765_not_connected_interface)

	MCFG_FLOPPY_2_DRIVES_ADD(ibmat_floppy_config)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1664K")
MACHINE_CONFIG_END


static MACHINE_CONFIG_START( ps2m30286, at_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I80286, 12000000)
	MCFG_CPU_PROGRAM_MAP(at16_map)
	MCFG_CPU_IO_MAP(at16_io)
	MCFG_CPU_CONFIG(i286_address_mask)

	MCFG_PIT8254_ADD( "pit8254", at_pit8254_config )

	MCFG_I8237_ADD( "dma8237_1", XTAL_14_31818MHz/3, at_dma8237_1_config )

	MCFG_I8237_ADD( "dma8237_2", XTAL_14_31818MHz/3, at_dma8237_2_config )

	MCFG_PIC8259_ADD( "pic8259_master", at_pic8259_master_config )

	MCFG_PIC8259_ADD( "pic8259_slave", at_pic8259_slave_config )

	MCFG_NS16450_ADD( "ns16450_0", ibm5170_com_interface[0] )			/* TODO: verify model */
	MCFG_NS16450_ADD( "ns16450_1", ibm5170_com_interface[1] )			/* TODO: verify model */
	MCFG_NS16450_ADD( "ns16450_2", ibm5170_com_interface[2] )			/* TODO: verify model */
	MCFG_NS16450_ADD( "ns16450_3", ibm5170_com_interface[3] )			/* TODO: verify model */

	MCFG_MACHINE_START( at )
	MCFG_MACHINE_RESET( at )

	MCFG_FRAGMENT_ADD( pcvideo_vga )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MCFG_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MCFG_SOUND_CONFIG(at_ym3812_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MCFG_SOUND_ADD("saa1099.1", SAA1099, 8000000)	/* running at 8 MHz ISA bus speed? */
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MCFG_SOUND_ADD("saa1099.2", SAA1099, 8000000)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif

	MCFG_AT_KEYBOARD_CONTROLLER_ADD("keybc", XTAL_12MHz, keyboard_controller_intf)
	MCFG_KB_KEYTRONIC_ADD("keyboard", at_keytronic_intf)

	MCFG_MC146818_ADD( "rtc", MC146818_STANDARD )

	/* printers */
	MCFG_PC_LPT_ADD("lpt_0", at_lpt_config)
	MCFG_PC_LPT_ADD("lpt_1", at_lpt_config)
	MCFG_PC_LPT_ADD("lpt_2", at_lpt_config)

	/* harddisk */
	MCFG_FRAGMENT_ADD( pc_hdc )

	MCFG_UPD765A_ADD("upd765", pc_fdc_upd765_not_connected_interface)

	MCFG_FLOPPY_2_DRIVES_ADD(ibmat_floppy_config)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1664K")
MACHINE_CONFIG_END


static MACHINE_CONFIG_START( atvga, at_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I80286, 12000000)
	MCFG_CPU_PROGRAM_MAP(at16_map)
	MCFG_CPU_IO_MAP(at16_io)
	MCFG_CPU_CONFIG(i286_address_mask)

	MCFG_PIT8254_ADD( "pit8254", at_pit8254_config )

	MCFG_I8237_ADD( "dma8237_1", XTAL_14_31818MHz/3, at_dma8237_1_config )

	MCFG_I8237_ADD( "dma8237_2", XTAL_14_31818MHz/3, at_dma8237_2_config )

	MCFG_PIC8259_ADD( "pic8259_master", at_pic8259_master_config )

	MCFG_PIC8259_ADD( "pic8259_slave", at_pic8259_slave_config )

	MCFG_NS16450_ADD( "ns16450_0", ibm5170_com_interface[0] )			/* TODO: verify model */
	MCFG_NS16450_ADD( "ns16450_1", ibm5170_com_interface[1] )			/* TODO: verify model */
	MCFG_NS16450_ADD( "ns16450_2", ibm5170_com_interface[2] )			/* TODO: verify model */
	MCFG_NS16450_ADD( "ns16450_3", ibm5170_com_interface[3] )			/* TODO: verify model */

	MCFG_FRAGMENT_ADD( pcvideo_vga )

	MCFG_MACHINE_START( at )
	MCFG_MACHINE_RESET( at )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MCFG_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MCFG_SOUND_CONFIG(at_ym3812_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MCFG_SOUND_ADD("saa1099.1", SAA1099, 8000000)	/* running at 8 MHz ISA bus speed? */
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MCFG_SOUND_ADD("saa1099.2", SAA1099, 8000000)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif
	MCFG_SOUND_ADD("dac", DAC, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MCFG_AT_KEYBOARD_CONTROLLER_ADD("keybc", XTAL_12MHz, keyboard_controller_intf)
	MCFG_KB_KEYTRONIC_ADD("keyboard", at_keytronic_intf)

	MCFG_MC146818_ADD( "rtc", MC146818_STANDARD )

	/* printers */
	MCFG_PC_LPT_ADD("lpt_0", at_lpt_config)
	MCFG_PC_LPT_ADD("lpt_1", at_lpt_config)
	MCFG_PC_LPT_ADD("lpt_2", at_lpt_config)

	/* harddisk */
	MCFG_FRAGMENT_ADD( pc_hdc )

	MCFG_UPD765A_ADD("upd765", pc_fdc_upd765_not_connected_interface)

	MCFG_FLOPPY_2_DRIVES_ADD(ibmat_floppy_config)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1664K")
MACHINE_CONFIG_END


static MACHINE_CONFIG_START( at386, at_state )
    /* basic machine hardware */
	/* original at 6 MHz, at03 8 megahertz */
	MCFG_CPU_ADD("maincpu", I386, 12000000)
	MCFG_CPU_PROGRAM_MAP(at386_map)
	MCFG_CPU_IO_MAP(at386_io)
	//MCFG_CPU_CONFIG(i286_address_mask)

	MCFG_MACHINE_START( at )
	MCFG_MACHINE_RESET( at )

	MCFG_PIT8254_ADD( "pit8254", at_pit8254_config )

	MCFG_I8237_ADD( "dma8237_1", XTAL_14_31818MHz/3, at_dma8237_1_config )

	MCFG_I8237_ADD( "dma8237_2", XTAL_14_31818MHz/3, at_dma8237_2_config )

	MCFG_PIC8259_ADD( "pic8259_master", at_pic8259_master_config )

	MCFG_PIC8259_ADD( "pic8259_slave", at_pic8259_slave_config )

	MCFG_NS16450_ADD( "ns16450_0", ibm5170_com_interface[0] )			/* TODO: verify model */
	MCFG_NS16450_ADD( "ns16450_1", ibm5170_com_interface[1] )			/* TODO: verify model */
	MCFG_NS16450_ADD( "ns16450_2", ibm5170_com_interface[2] )			/* TODO: verify model */
	MCFG_NS16450_ADD( "ns16450_3", ibm5170_com_interface[3] )			/* TODO: verify model */

	MCFG_FRAGMENT_ADD( pcvideo_vga )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MCFG_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MCFG_SOUND_CONFIG(at_ym3812_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MCFG_SOUND_ADD("saa1099.1", SAA1099, 8000000)	/* running at 8 MHz ISA bus speed? */
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MCFG_SOUND_ADD("saa1099.2", SAA1099, 8000000)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif
	MCFG_SOUND_ADD("dac", DAC, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MCFG_AT_KEYBOARD_CONTROLLER_ADD("keybc", XTAL_12MHz, keyboard_controller_intf)
	MCFG_KB_KEYTRONIC_ADD("keyboard", at_keytronic_intf)

	MCFG_MC146818_ADD( "rtc", MC146818_STANDARD )
    MCFG_NVRAM_ADD_0FILL("nvram")

	/* printers */
	MCFG_PC_LPT_ADD("lpt_0", at_lpt_config)
	MCFG_PC_LPT_ADD("lpt_1", at_lpt_config)
	MCFG_PC_LPT_ADD("lpt_2", at_lpt_config)

	/* harddisk */
	MCFG_FRAGMENT_ADD( pc_hdc )

	MCFG_UPD765A_ADD("upd765", pc_fdc_upd765_not_connected_interface)

	MCFG_FLOPPY_2_DRIVES_ADD(ibmat_floppy_config)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1664K")
MACHINE_CONFIG_END


static MACHINE_CONFIG_DERIVED( at486, at386 )

	MCFG_CPU_REPLACE("maincpu", I486, 25000000)
	MCFG_CPU_PROGRAM_MAP(at386_map)
	MCFG_CPU_IO_MAP(at386_io)
MACHINE_CONFIG_END


static MACHINE_CONFIG_DERIVED( ct486, at386 )
	MCFG_CPU_REPLACE("maincpu", I486, 25000000)
	MCFG_CPU_PROGRAM_MAP(ct486_map)
	MCFG_CPU_IO_MAP(ct486_io)

	MCFG_CS4031_ADD("cs4031", "maincpu", "isa", "bios")

	MCFG_DEVICE_REMOVE(RAM_TAG)
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("4M")
	MCFG_RAM_EXTRA_OPTIONS("1M,2M,8M,16M,32M,64M")
MACHINE_CONFIG_END


static MACHINE_CONFIG_DERIVED( at586, at386 )

	MCFG_CPU_REPLACE("maincpu", PENTIUM, 60000000)
	MCFG_CPU_PROGRAM_MAP(at586_map)
	MCFG_CPU_IO_MAP(at586_io)

	MCFG_I82371AB_ADD("i82371ab")
	MCFG_I82439TX_ADD("i82439tx", "maincpu", "user1")

	MCFG_PCI_BUS_ADD("pcibus", 0)
	MCFG_PCI_BUS_DEVICE(0, "i82439tx", i82439tx_pci_read, i82439tx_pci_write)
	MCFG_PCI_BUS_DEVICE(1, "i82371ab", i82371ab_pci_read, i82371ab_pci_write)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( c386sx16, at386 )

	MCFG_CPU_REPLACE("maincpu", I386, 16000000)		/* 386SX */
	MCFG_CPU_PROGRAM_MAP(at386_map)
	MCFG_CPU_IO_MAP(at386_io)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( megapc, at386 )

	MCFG_CPU_REPLACE("maincpu", I386, XTAL_50MHz / 2)
	MCFG_CPU_PROGRAM_MAP(at386_map)
	MCFG_CPU_IO_MAP(megapc_io)

	/* software lists */
	MCFG_SOFTWARE_LIST_ADD("disk_list","megapc")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( megapcpl, megapc )

	MCFG_CPU_REPLACE("maincpu", I486, 66000000 / 2)
	MCFG_CPU_PROGRAM_MAP(at386_map)
	MCFG_CPU_IO_MAP(megapc_io)
MACHINE_CONFIG_END



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

ROM_START( ec1849 )
	ROM_REGION16_LE(0x1000000,"maincpu", 0)
	ROM_LOAD16_BYTE( "cpu-card_27c256_015.rom", 0xf0000, 0x8000, CRC(68eadf0a) SHA1(903a7f1c3ebc6b27c31b512b2908c483608b5c13))
	ROM_LOAD16_BYTE( "cpu-card_27c256_016.rom", 0xf0001, 0x8000, CRC(bc3924d6) SHA1(596be415e6c2bc4ff30a187f146664531565712c))
	ROM_LOAD16_BYTE( "video-card_573rf6( 2764)_040.rom", 0xc0001, 0x2000, CRC(a3ece315) SHA1(e800e11c3b1b6fcaf41bfb7d4058a9d34fdd2b3f))
	ROM_LOAD16_BYTE( "video-card_573rf6( 2764)_041.rom", 0xc0000, 0x2000, CRC(b0a2ba7f) SHA1(c8160e8bc97cd391558f1dddd3fd3ec4a19d030c))

	ROM_REGION(0x08100, "gfx1", 0)
	ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))

	ROM_REGION(0x50000, "gfx2", ROMREGION_ERASE00)
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

	/* Mainboard PALS */
	ROM_REGION( 0x2000, "pals", 0 )
	ROM_LOAD("59x7599.pal20l8.u27", 0x0000, 0x0800, NO_DUMP) /* MMI PAL20L8ACN5 8631 // N59X7599 IBM (C)85 K3 */
	ROM_LOAD("1503135.pal14l4.u81", 0x0800, 0x0800, NO_DUMP) /* MMI 1503135 8625 // (C) IBM CORP 83 */
	/* P/N 6320947 Serial/Parallel ISA expansion card PAL */
	ROM_LOAD("1503085.pal.u14", 0x1000, 0x0800, NO_DUMP) /* MMI 1503085 8449 // (C) IBM CORP 83 */ /* Not sure of type */

	/* Mainboard PROMS */
	ROM_REGION( 0x2000, "proms", 0 )
	ROM_LOAD("1501814.82s123.u72", 0x0000, 0x0800, NO_DUMP) /* N82S123AN 8623 // SK-U 1501814 */
	ROM_LOAD("59x7594.82s147.u90", 0x0800, 0x1000, NO_DUMP) /* S N82S147AN 8629 // VCT 59X7594 */
ROM_END


ROM_START( i8530286 )
    ROM_REGION(0x1000000,"maincpu", 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	// saved from running machine
    ROM_LOAD16_BYTE("ps2m30.0", 0xe0000, 0x10000, CRC(9965a634) SHA1(c237b1760f8a4561ec47dc70fe2e9df664e56596))
	ROM_RELOAD(0xfe0000,0x10000)
    ROM_LOAD16_BYTE("ps2m30.1", 0xe0001, 0x10000, CRC(1448d3cb) SHA1(13fa26d895ce084278cd5ab1208fc16c80115ebe))
	ROM_RELOAD(0xfe0001,0x10000)
ROM_END


ROM_START( i8555081 )
    ROM_REGION(0x1000000,"maincpu", 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	// saved from running machine
    ROM_LOAD16_BYTE("33fb8145.zm40", 0xe0000, 0x10000, CRC(0895894c) SHA1(7cee77828867ad1bdbe0ac223bc25d23c65b28a0))
	ROM_RELOAD(0xfe0000, 0x10000)
    ROM_LOAD16_BYTE("33fb8146.zm41", 0xe0001, 0x10000, CRC(c6020680) SHA1(b25a64e4b2dca07c567648401100e04e89bbcddb))
	ROM_RELOAD(0xfe0001, 0x10000)
ROM_END


ROM_START( at )
    ROM_REGION(0x1000000,"maincpu", 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	ROM_SYSTEM_BIOS(0, "at", "PC 286")
    ROMX_LOAD("at110387.1", 0xf0001, 0x8000, CRC(679296a7) SHA1(ae891314cac614dfece686d8e1d74f4763cf40e3),ROM_SKIP(1) | ROM_BIOS(1) )
    ROMX_LOAD("at110387.0", 0xf0000, 0x8000, CRC(65ae1f97) SHA1(91a29c7deecf7a9afbba330e64e0eee9aafee4d1),ROM_SKIP(1) | ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "ami211", "AMI 21.1")
	ROMX_LOAD( "ami211.bin",	 0xf0000, 0x10000,CRC(a0b5d269) SHA1(44db8227d35a09e39b93ed944f85dcddb0dd0d39), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "ami206", "AMI C 206.1")
	ROMX_LOAD( "amic206.bin",	 0xf0000, 0x10000,CRC(25a67c34) SHA1(91e9d8cdc2f1b40a601a23ceaff2189fd1245f3b), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS(3, "amic21", "AMI C 21.1")
	ROMX_LOAD( "amic21-1.bin",  0xf0001, 0x8000, CRC(5644ed38) SHA1(963555ec77845defc3b42b433280908e1797076e),ROM_SKIP(1) | ROM_BIOS(4) )
	ROMX_LOAD( "amic21-2.bin",  0xf0000, 0x8000, CRC(8ffe7752) SHA1(68215f07a170ee7bdcb3e52b370d470af1741f7e),ROM_SKIP(1) | ROM_BIOS(4) )
	ROM_SYSTEM_BIOS(4, "amicg", "AMI CG")
	ROMX_LOAD( "amicg.1",		 0xf0000, 0x10000,CRC(8408965a) SHA1(9893d3ac851e01b06a68a67d3721df36ca2c96f5), ROM_BIOS(5) )
	ROM_SYSTEM_BIOS(5, "ami101", "AMI HT 101.1")
	ROMX_LOAD( "amiht-h.bin",   0xf0001, 0x8000, CRC(8022545f) SHA1(42541d4392ad00b0e064b3a8ccf2786d875c7c19),ROM_SKIP(1) | ROM_BIOS(6) )
	ROMX_LOAD( "amiht-l.bin",   0xf0000, 0x8000, CRC(285f6b8f) SHA1(2fce4ec53b68c9a7580858e16c926dc907820872),ROM_SKIP(1) | ROM_BIOS(6) )
	ROM_SYSTEM_BIOS(6, "ami121", "AMI HT 12.1")
	ROMX_LOAD( "ami2ev86.bin",  0xf0001, 0x8000, CRC(55deb5c2) SHA1(19ce1a7cc985b5895c585e39211475de2e3b0dd1),ROM_SKIP(1) | ROM_BIOS(7) )
	ROMX_LOAD( "ami2od86.bin",  0xf0000, 0x8000, CRC(04a2cec4) SHA1(564d37a8b2c0f4d0e23cd1e280a09d47c9945da8),ROM_SKIP(1) | ROM_BIOS(7) )
	ROM_SYSTEM_BIOS(7, "ami122", "AMI HT 12.2")
	ROMX_LOAD( "ami2ev89.bin",  0xf0001, 0x8000, CRC(705d36e0) SHA1(0c9cfb71ced4587f109b9b6dfc2a9c92302fdb99),ROM_SKIP(1) | ROM_BIOS(8) )
	ROMX_LOAD( "ami2od89.bin",  0xf0000, 0x8000, CRC(7c81bbe8) SHA1(a2c7eca586f6e2e76b9101191e080a1f1cb8b833),ROM_SKIP(1) | ROM_BIOS(8) )
	ROM_SYSTEM_BIOS(8, "ami123", "AMI HT 12.3")
	ROMX_LOAD( "ht12h.bin",     0xf0001, 0x8000, CRC(db8b471e) SHA1(7b5fa1c131061fa7719247db3e282f6d30226778),ROM_SKIP(1) | ROM_BIOS(9) )
	ROMX_LOAD( "ht12l.bin",     0xf0000, 0x8000, CRC(74fd178a) SHA1(97c8283e574abbed962b701f3e8091fb82823b80),ROM_SKIP(1) | ROM_BIOS(9) )
	ROM_SYSTEM_BIOS(9, "ami181", "AMI HT 18.1")
	ROMX_LOAD( "ht18.bin",     0xf0000, 0x10000, CRC(f65a6f9a) SHA1(7dfdf7d243f9f645165dc009c5097dd515f86fbb), ROM_BIOS(10) )
	ROM_SYSTEM_BIOS(10, "amiht21", "AMI HT 21.1")
	ROMX_LOAD( "ht21e.bin",    0xf0000, 0x10000, CRC(e80f7fed) SHA1(62d958d98c95e9e4d1b290a6c1054ae98770f276), ROM_BIOS(11) )
	ROM_SYSTEM_BIOS(11, "amip1", "AMI P.1")
	ROMX_LOAD( "poisk-h.bin",   0xf0001, 0x8000, CRC(83fd3f8c) SHA1(ca94850bbd949b97b11710629886b0ee69489a81),ROM_SKIP(1) | ROM_BIOS(12) )
	ROMX_LOAD( "poisk-l.bin",   0xf0000, 0x8000, CRC(0b2ed291) SHA1(bb51a3f317cf4d429a6cfb44a46ca0ac39d9aaa7),ROM_SKIP(1) | ROM_BIOS(12) )
	ROM_SYSTEM_BIOS(12, "aw201", "Award 201")
	ROMX_LOAD( "83201-5h.bin",  0xf0001, 0x8000, CRC(968d1fc0) SHA1(dc4122a6c696f0b43e7894dc1b669346eed755d5),ROM_SKIP(1) | ROM_BIOS(13) )
	ROMX_LOAD( "83201-5l.bin",  0xf0000, 0x8000, CRC(bf50a89a) SHA1(2349a1db6017a7fb0673e99d3680c8753407be8d),ROM_SKIP(1) | ROM_BIOS(13) )

	/* Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f))

	ROM_REGION(0x50000, "gfx2", ROMREGION_ERASE00)
ROM_END

ROM_START( cmdpc30 )
    ROM_REGION(0x1000000,"maincpu", 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	ROMX_LOAD( "commodore pc 30 iii even.bin", 0xf8000, 0x4000, CRC(36307aa9) SHA1(50237ffea703b867de426ab9ebc2af46bac1d0e1),ROM_SKIP(1))
	ROMX_LOAD( "commodore pc 30 iii odd.bin",  0xf8001, 0x4000, CRC(41bae42d) SHA1(27d6ad9554be86359d44331f25591e3122a31519),ROM_SKIP(1))
	/* Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f))

	ROM_REGION(0x50000, "gfx2", ROMREGION_ERASE00)
ROM_END

ROM_START( atvga )
    ROM_REGION(0x1000000,"maincpu", 0)
    ROM_LOAD("et4000.bin", 0xc0000, 0x8000, CRC(f01e4be0) SHA1(95d75ff41bcb765e50bd87a8da01835fd0aa01d5))
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	ROM_SYSTEM_BIOS(0, "at", "PC 286")
    ROMX_LOAD("at110387.1", 0xf0001, 0x8000, CRC(679296a7) SHA1(ae891314cac614dfece686d8e1d74f4763cf40e3),ROM_SKIP(1) | ROM_BIOS(1) )
    ROMX_LOAD("at110387.0", 0xf0000, 0x8000, CRC(65ae1f97) SHA1(91a29c7deecf7a9afbba330e64e0eee9aafee4d1),ROM_SKIP(1) | ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "ami211", "AMI 21.1")
	ROMX_LOAD( "ami211.bin",	 0xf0000, 0x10000,CRC(a0b5d269) SHA1(44db8227d35a09e39b93ed944f85dcddb0dd0d39), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "ami206", "AMI C 206.1")
	ROMX_LOAD( "amic206.bin",	 0xf0000, 0x10000,CRC(25a67c34) SHA1(91e9d8cdc2f1b40a601a23ceaff2189fd1245f3b), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS(3, "amic21", "AMI C 21.1")
	ROMX_LOAD( "amic21-1.bin",  0xf0001, 0x8000, CRC(5644ed38) SHA1(963555ec77845defc3b42b433280908e1797076e),ROM_SKIP(1) | ROM_BIOS(4) )
	ROMX_LOAD( "amic21-2.bin",  0xf0000, 0x8000, CRC(8ffe7752) SHA1(68215f07a170ee7bdcb3e52b370d470af1741f7e),ROM_SKIP(1) | ROM_BIOS(4) )
	ROM_SYSTEM_BIOS(4, "amicg", "AMI CG")
	ROMX_LOAD( "amicg.1",		 0xf0000, 0x10000,CRC(8408965a) SHA1(9893d3ac851e01b06a68a67d3721df36ca2c96f5), ROM_BIOS(5) )
	ROM_SYSTEM_BIOS(5, "ami101", "AMI HT 101.1")
	ROMX_LOAD( "amiht-h.bin",   0xf0001, 0x8000, CRC(8022545f) SHA1(42541d4392ad00b0e064b3a8ccf2786d875c7c19),ROM_SKIP(1) | ROM_BIOS(6) )
	ROMX_LOAD( "amiht-l.bin",   0xf0000, 0x8000, CRC(285f6b8f) SHA1(2fce4ec53b68c9a7580858e16c926dc907820872),ROM_SKIP(1) | ROM_BIOS(6) )
	ROM_SYSTEM_BIOS(6, "ami121", "AMI HT 12.1")
	ROMX_LOAD( "ami2ev86.bin",  0xf0001, 0x8000, CRC(55deb5c2) SHA1(19ce1a7cc985b5895c585e39211475de2e3b0dd1),ROM_SKIP(1) | ROM_BIOS(7) )
	ROMX_LOAD( "ami2od86.bin",  0xf0000, 0x8000, CRC(04a2cec4) SHA1(564d37a8b2c0f4d0e23cd1e280a09d47c9945da8),ROM_SKIP(1) | ROM_BIOS(7) )
	ROM_SYSTEM_BIOS(7, "ami122", "AMI HT 12.2")
	ROMX_LOAD( "ami2ev89.bin",  0xf0001, 0x8000, CRC(705d36e0) SHA1(0c9cfb71ced4587f109b9b6dfc2a9c92302fdb99),ROM_SKIP(1) | ROM_BIOS(8) )
	ROMX_LOAD( "ami2od89.bin",  0xf0000, 0x8000, CRC(7c81bbe8) SHA1(a2c7eca586f6e2e76b9101191e080a1f1cb8b833),ROM_SKIP(1) | ROM_BIOS(8) )
	ROM_SYSTEM_BIOS(8, "ami123", "AMI HT 12.3")
	ROMX_LOAD( "ht12h.bin",     0xf0001, 0x8000, CRC(db8b471e) SHA1(7b5fa1c131061fa7719247db3e282f6d30226778),ROM_SKIP(1) | ROM_BIOS(9) )
	ROMX_LOAD( "ht12l.bin",     0xf0000, 0x8000, CRC(74fd178a) SHA1(97c8283e574abbed962b701f3e8091fb82823b80),ROM_SKIP(1) | ROM_BIOS(9) )
	ROM_SYSTEM_BIOS(9, "ami181", "AMI HT 18.1")
	ROMX_LOAD( "ht18.bin",     0xf0000, 0x10000, CRC(f65a6f9a) SHA1(7dfdf7d243f9f645165dc009c5097dd515f86fbb), ROM_BIOS(10) )
	ROM_SYSTEM_BIOS(10, "amiht21", "AMI HT 21.1")
	ROMX_LOAD( "ht21e.bin",    0xf0000, 0x10000, CRC(e80f7fed) SHA1(62d958d98c95e9e4d1b290a6c1054ae98770f276), ROM_BIOS(11) )
	ROM_SYSTEM_BIOS(11, "amip1", "AMI P.1")
	ROMX_LOAD( "poisk-h.bin",   0xf0001, 0x8000, CRC(83fd3f8c) SHA1(ca94850bbd949b97b11710629886b0ee69489a81),ROM_SKIP(1) | ROM_BIOS(12) )
	ROMX_LOAD( "poisk-l.bin",   0xf0000, 0x8000, CRC(0b2ed291) SHA1(bb51a3f317cf4d429a6cfb44a46ca0ac39d9aaa7),ROM_SKIP(1) | ROM_BIOS(12) )
ROM_END


ROM_START( neat )
    ROM_REGION(0x1000000,"maincpu", 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	//ROM_SYSTEM_BIOS(0, "neat286", "NEAT 286")
	ROM_LOAD16_BYTE("at030389.0", 0xf0000, 0x8000, CRC(4c36e61d) SHA1(094e8d5e6819889163cb22a2cf559186de782582))
	//ROM_RELOAD(0xff0000,0x8000)
	ROM_LOAD16_BYTE("at030389.1", 0xf0001, 0x8000, CRC(4e90f294) SHA1(18c21fd8d7e959e2292a9afbbaf78310f9cad12f))
	//ROM_RELOAD(0xff0001,0x8000)

	/* Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f))

	ROM_REGION(0x50000, "gfx2", ROMREGION_ERASE00)
ROM_END


ROM_START( at386 )
    ROM_REGION(0x1000000,"maincpu", 0)
    ROM_LOAD("et4000.bin", 0xc0000, 0x08000, CRC(f01e4be0) SHA1(95d75ff41bcb765e50bd87a8da01835fd0aa01d5))
    ROM_LOAD("wdbios.rom", 0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	ROM_SYSTEM_BIOS(0, "at386", "unknown 386")	// This dump possibly comes from a MITAC INC 386 board, given that the original driver had it as manufacturer
    ROMX_LOAD("at386.bin",  0xf0000, 0x10000, CRC(3df9732a) SHA1(def71567dee373dc67063f204ef44ffab9453ead), ROM_BIOS(1))
	//ROM_RELOAD(0xff0000,0x10000)
	ROM_SYSTEM_BIOS(1, "neatsx", "NEATsx 386sx")
	ROMX_LOAD("012l-u25.bin", 0xf0000, 0x8000, CRC(4AB1862D) SHA1(D4E8D0FF43731270478CA7671A129080FF350A4F),ROM_SKIP(1) | ROM_BIOS(2) )
	//ROM_RELOAD(0xff0000,0x8000)
	ROMX_LOAD("012h-u24.bin", 0xf0001, 0x8000, CRC(17472521) SHA1(7588C148FE53D9DC4CB2D0AB6E0FD51A39BB5D1A),ROM_SKIP(1) | ROM_BIOS(2) )
	//ROM_RELOAD(0xff0000,0x8000)
ROM_END


ROM_START( at486 )
	ROM_REGION(0x1000000, "maincpu", 0)
	ROM_LOAD("et4000.bin", 0xc0000, 0x08000, CRC(f01e4be0) SHA1(95d75ff41bcb765e50bd87a8da01835fd0aa01d5))
	ROM_LOAD("wdbios.rom", 0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))

	ROM_SYSTEM_BIOS(0, "at486", "PC/AT 486")	\
	ROMX_LOAD("at486.bin",   0x0f0000, 0x10000, CRC(31214616) SHA1(51b41fa44d92151025fc9ad06e518e906935e689), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "mg48602", "UMC MG-48602")	\
	ROMX_LOAD("mg48602.bin", 0x0f0000, 0x10000, CRC(45797823) SHA1(a5fab258aecabde615e1e97af5911d6cf9938c11), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "ft01232", "Free Tech 01-232")	\
	ROMX_LOAD("ft01232.bin", 0x0f0000, 0x10000, CRC(30efaf92) SHA1(665c8ef05ca052dcc06bb473c9539546bfef1e86), ROM_BIOS(3))

	/* 486 boards from FIC

    naming convention
    xxxxx101 --> for EPROM
    xxxxx701 --> for EEPROM using a Flash Utility v5.02
    xxxxBxxx --> NS 311/312 IO Core Logic
    xxxxCxxx --> NS 332 IO Core Logic
    xxxxGxxx --> Winbond W83787F IO Core Logic
    xxxxJxxx --> Winbond W83877F IO Core Logic

    */

	/* this is the year 2000 beta bios from FIC, supports GIO-VT, GAC-V, GAC-2, VIP-IO, VIO-VP and GVT-2 */
	ROM_SYSTEM_BIOS(3, "ficy2k", "FIC 486 3.276GN1") /* 1997-06-16, includes CL-GD5429 VGA BIOS 1.00a */
	ROMX_LOAD("3276gn1.bin",  0x0e0000, 0x20000, CRC(d4ff0cc4) SHA1(567b6bdbc9bff306c8c955f275e01ae4c45fd5f2), ROM_BIOS(4))

	ROM_SYSTEM_BIOS(4, "ficgac2", "FIC 486-GAC-2") /* 1994-04-29, includes CL-GD542X VGA BIOS 1.50 */
	ROMX_LOAD("att409be.bin", 0x0e0000, 0x20000, CRC(c58e017b) SHA1(14c19e720ce62eb2afe28a70f4e4ebafab0f9e77), ROM_BIOS(5))
	ROM_SYSTEM_BIOS(5, "ficgacv", "FIC 486-GAC-V 3.27GN1") /* 1996-04-08, includes CL-GD542X VGA BIOS 1.41 */
	ROMX_LOAD("327gn1.awd",   0x0e0000, 0x20000, CRC(017614d4) SHA1(2228c28f21a7e78033d24319449297936465b164), ROM_BIOS(6))
	ROM_SYSTEM_BIOS(6, "ficgiovp", "FIC 486-GIO-VP 3.15GN") /* 1994-05-06 */
	ROMX_LOAD("giovp315.rom", 0x0f0000, 0x10000, CRC(e102c3f5) SHA1(f15a7e9311cc17afe86da0b369607768b030ddec), ROM_BIOS(7))
	ROM_SYSTEM_BIOS(7, "ficgiovt", "FIC 486-GIO-VT 3.06G") /* 1994-11-20 */
	ROMX_LOAD("306gcd00.awd", 0x0f0000, 0x10000, CRC(75f3ded4) SHA1(999d4b58204e0b0f33262d0613c855b528bf9597), ROM_BIOS(8))

	ROM_SYSTEM_BIOS(8, "ficgiovt2_326", "FIC 486-GIO-VT2 3.26G")  /* 1994-07-06 */
	ROMX_LOAD("326g1c00.awd", 0x0f0000, 0x10000, CRC(2e729ab5) SHA1(b713f97fa0e0b62856dab917f417f5b21020b354), ROM_BIOS(9))
	ROM_SYSTEM_BIOS(9, "ficgiovt2_3276", "FIC 486-GIO-VT2 3.276") /* 1997-07-17 */
	ROMX_LOAD("32760000.bin", 0x0f0000, 0x10000, CRC(ad179128) SHA1(595f67ba4a1c8eb5e118d75bf657fff3803dcf4f), ROM_BIOS(10))

	ROM_SYSTEM_BIOS(10, "ficgvt2", "FIC 486-GVT-2 3.07G") /* 1994-11-02 */
	ROMX_LOAD("3073.bin",     0x0f0000, 0x10000, CRC(a6723863) SHA1(ee93a2f1ec84a3d67e267d0a490029f9165f1533), ROM_BIOS(11))
	ROM_SYSTEM_BIOS(11, "ficgpak2", "FIC 486-PAK-2 5.15S") /* 1995-06-27, includes Phoenix S3 TRIO64 Enhanced VGA BIOS 1.4-01 */
	ROMX_LOAD("515sbd8a.awd", 0x0e0000, 0x20000, CRC(778247e1) SHA1(07d8f0f2464abf507be1e8dfa06cd88737782411), ROM_BIOS(12))

	ROM_SYSTEM_BIOS(12, "ficpio3g7", "FIC 486-PIO-3 1.15G705") /* pnp */
	ROMX_LOAD("115g705.awd",  0x0e0000, 0x20000, CRC(ddb1544a) SHA1(d165c9ecdc9397789abddfe0fef69fdf954fa41b), ROM_BIOS(13))
	ROM_SYSTEM_BIOS(13, "ficpio3g1", "FIC 486-PIO-3 1.15G105") /* non-pnp */
	ROMX_LOAD("115g105.awd",  0x0e0000, 0x20000, CRC(b327eb83) SHA1(9e1ff53e07ca035d8d43951bac345fec7131678d), ROM_BIOS(14))

	ROM_SYSTEM_BIOS(14, "ficpos", "FIC 486-POS")
	ROMX_LOAD("116di6b7.bin", 0x0e0000, 0x20000, CRC(d1d84616) SHA1(2f2b27ce100cf784260d8e155b48db8cfbc63285), ROM_BIOS(15))
	ROM_SYSTEM_BIOS(15, "ficpvt", "FIC 486-PVT 5.15")          /* 1995-06-27 */
	ROMX_LOAD("5150eef3.awd", 0x0e0000, 0x20000, CRC(eb35785d) SHA1(1e601bc8da73f22f11effe9cdf5a84d52576142b), ROM_BIOS(16))
	ROM_SYSTEM_BIOS(16, "ficpvtio", "FIC 486-PVT-IO 5.162W2")  /* 1995-10-05 */
	ROMX_LOAD("5162cf37.awd", 0x0e0000, 0x20000, CRC(378d813d) SHA1(aa674eff5b972b31924941534c3c988f6f78dc93), ROM_BIOS(17))
	ROM_SYSTEM_BIOS(17, "ficvipio426", "FIC 486-VIP-IO 4.26GN2") /* 1994-12-07 */
	ROMX_LOAD("426gn2.awd",   0x0e0000, 0x20000, CRC(5f472aa9) SHA1(9160abefae32b450e973651c052657b4becc72ba), ROM_BIOS(18))
	ROM_SYSTEM_BIOS(18, "ficvipio427", "FIC 486-VIP-IO 4.27GN2A") /* 1996-02-14 */
	ROMX_LOAD("427gn2a.awd",  0x0e0000, 0x20000, CRC(035ad56d) SHA1(0086db3eff711fc710b30e7f422fc5b4ab8d47aa), ROM_BIOS(19))
	ROM_SYSTEM_BIOS(19, "ficvipio2", "FIC 486-VIP-IO2")
	ROMX_LOAD("1164g701.awd", 0x0e0000, 0x20000, CRC(7b762683) SHA1(84debce7239c8b1978246688ae538f7c4f519d13), ROM_BIOS(20))

	ROM_SYSTEM_BIOS(20, "qdi", "QDI PX486DX33/50P3")
	ROMX_LOAD("qdi_px486.u23", 0x0f0000, 0x10000, CRC(c80ecfb6) SHA1(34cc9ef68ff719cd0771297bf184efa83a805f3e), ROM_BIOS(21))
ROM_END


// Unknown 486 board with Chips & Technologies CS4031 chipset
ROM_START( ct486 )
	ROM_REGION(0x100000, "isa", 0)
	ROM_LOAD("et4000.bin",  0xc0000, 0x08000, CRC(f01e4be0) SHA1(95d75ff41bcb765e50bd87a8da01835fd0aa01d5))
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))

	ROM_REGION(0x100000, "bios", 0)
	ROM_LOAD("chips_1.ami", 0xf0000, 0x10000, CRC(a14a7511) SHA1(b88d09be66905ed2deddc26a6f8522e7d2d6f9a8))
ROM_END


// FIC 486-PIO-2 (4 ISA, 4 PCI)
// VIA VT82C505 + VT82C496G + VT82C406MV, NS311/312 or NS332 I/O
ROM_START( ficpio2 )
	ROM_REGION(0x1000000, "maincpu", 0)
	ROM_LOAD("et4000.bin", 0xc0000, 0x08000, CRC(f01e4be0) SHA1(95d75ff41bcb765e50bd87a8da01835fd0aa01d5))
	ROM_LOAD("wdbios.rom", 0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))

	ROM_SYSTEM_BIOS(0, "ficpio2c7", "FIC 486-PIO-2 1.15C701") /* pnp, i/o core: NS 332 */
	ROMX_LOAD("115c701.awd",  0x0e0000, 0x20000, CRC(b0dd7975) SHA1(bfde13b0fbd141bc945d37d92faca9f4f59b716d), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "ficpio2b7", "FIC 486-PIO-2 1.15B701") /* pnp, i/o core: NS 311/312 */
	ROMX_LOAD("115b701.awd",  0x0e0000, 0x20000, CRC(ac24abad) SHA1(01174d84ed32fb1d95cd632d09f773acb8666c83), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "ficpio2c1", "FIC 486-PIO-2 1.15C101") /* non-pnp, i/o core: NS 332  */
	ROMX_LOAD("115c101.awd",  0x0e0000, 0x20000, CRC(5fadde88) SHA1(eff79692c1ecf34b6ea3f02409d14ce1f5c51bf9), ROM_BIOS(3))
	ROM_SYSTEM_BIOS(3, "ficpio2b1", "FIC 486-PIO-2 1.15B101") /* non-pnp, i/o core: NS 311/312  */
	ROMX_LOAD("115b101.awd",  0x0e0000, 0x20000, CRC(ff69617d) SHA1(ecbfc7315dcf6bd3e5b59e3ae9258759f64fe7a0), ROM_BIOS(4))
ROM_END


ROM_START( at586 )
	ROM_REGION32_LE(0x40000, "user1", 0)
	ROM_LOAD("et4000.bin",  0x00000, 0x08000, CRC(f01e4be0) SHA1(95d75ff41bcb765e50bd87a8da01835fd0aa01d5))
    ROM_LOAD("wdbios.rom",  0x08000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	ROM_SYSTEM_BIOS(0, "sptx", "SP-586TX")
    ROMX_LOAD("sp586tx.bin",   0x20000, 0x20000, CRC(1003d72c) SHA1(ec9224ff9b0fdfd6e462cb7bbf419875414739d6), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "unisys", "Unisys 586") // probably bad dump due to need of hack in i82439tx to work
    ROMX_LOAD("at586.bin",     0x20000, 0x20000, CRC(717037f5) SHA1(1d49d1b7a4a40d07d1a897b7f8c827754d76f824), ROM_BIOS(2))

	ROM_SYSTEM_BIOS(2, "ga586t2", "Gigabyte GA-586T2") // ITE 8679 I/O
    ROMX_LOAD("gb_ga586t2.bin",  0x20000, 0x20000, CRC(3a50a6e1) SHA1(dea859b4f1492d0d08aacd260ed1e83e00ebac08), ROM_BIOS(3))
	ROM_SYSTEM_BIOS(3, "5tx52", "Acorp 5TX52") // W83877TF I/O
    ROMX_LOAD("acorp_5tx52.bin", 0x20000, 0x20000, CRC(04d69419) SHA1(983377674fef05e710c8665c14cc348c99166fb6), ROM_BIOS(4))
	ROM_SYSTEM_BIOS(4, "txp4", "ASUS TXP4") // W83977TF-A I/O
    ROMX_LOAD("asus_txp4.bin",   0x20000, 0x20000, CRC(a1321bb1) SHA1(92e5f14d8505119f85b148a63510617ac12bcdf3), ROM_BIOS(5))
ROM_END


ROM_START( c386sx16 )
	ROM_REGION(0x1000000,"maincpu", 0)
	/* actual VGA BIOS not dumped - uses a WD Paradise according to http://www.cbmhardware.de/pc/pc.php */
	ROM_LOAD("et4000.bin", 0xc0000, 0x08000, CRC(f01e4be0) SHA1(95d75ff41bcb765e50bd87a8da01835fd0aa01d5))
	ROM_LOAD("wdbios.rom", 0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))

	/* Commodore 80386SX BIOS Rev. 1.03 */
	/* Copyright (C) 1985-1990 Commodore Electronics Ltd. */
	/* Copyright (C) 1985-1990 Phoenix Technologies Ltd. */
	ROM_LOAD16_BYTE( "390914-01.u39", 0xf0000, 0x8000, CRC(8f849198) SHA1(550b04bac0d0807d6e95ec25391a81272779b41b)) /* 390914-01 V1.03 CS-2100 U39 Copyright (C) 1990 CBM */
	ROM_LOAD16_BYTE( "390915-01.u38", 0xf0001, 0x8000, CRC(ee4bad92) SHA1(6e02ef97a7ce336485814c06a1693bc099ce5cfb)) /* 390915-01 V1.03 CS-2100 U38 Copyright (C) 1990 CBM */
ROM_END

/* FIC VT-503 (Intel TX chipset, ITE 8679 Super I/O) */
ROM_START( ficvt503 )
	ROM_REGION32_LE(0x40000, "user1", 0)
	ROM_LOAD("et4000.bin",  0x00000, 0x08000, CRC(f01e4be0) SHA1(95d75ff41bcb765e50bd87a8da01835fd0aa01d5))
	ROM_LOAD("wdbios.rom",  0x08000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))

	ROM_SYSTEM_BIOS(0, "109gi13", "1.09GI13") /* 1997-10-02 */
	ROMX_LOAD("109gi13.bin", 0x20000, 0x20000, CRC(0c32af48) SHA1(2cce40a98598f1ed1f398975f7a90c8be4200667), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "109gi14", "1.09GI14") /* 1997-11-07 */
	ROMX_LOAD("109gi14.awd", 0x20000, 0x20000, CRC(588c5cc8) SHA1(710e5405850fd975b362a422bfe9bc6d6c9a36cd), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "109gi15", "1.09GI15") /* 1997-11-07 */
	ROMX_LOAD("109gi15.awd", 0x20000, 0x20000, CRC(649a3481) SHA1(e681c6ab55a67cec5978dfffa75fcddc2aa0de4d), ROM_BIOS(3))
	ROM_SYSTEM_BIOS(3, "109gi16", "1.09GI16") /* 2000-03-23 */
	ROMX_LOAD("109gi16.bin", 0x20000, 0x20000, CRC(a928f271) SHA1(127a83a60752cc33b3ca49774488e511ec7bac55), ROM_BIOS(4))
	ROM_SYSTEM_BIOS(4, "115gk140", "1.15GK140") /* 1999-03-03 */
	ROMX_LOAD("115gk140.awd", 0x20000, 0x20000, CRC(65e88956) SHA1(f94bb0732e00b5b0f18f4e349db24a289f8379c5), ROM_BIOS(5))
ROM_END

ROM_START( megapc )
    ROM_REGION(0x1000000,"maincpu", 0)
    ROM_LOAD("et4000.bin", 0xc0000, 0x08000, CRC(f01e4be0) SHA1(95d75ff41bcb765e50bd87a8da01835fd0aa01d5))
    ROM_LOAD("wdbios.rom", 0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))

	ROM_LOAD16_BYTE( "41651-bios lo.u18",  0xe0000, 0x10000, CRC(1e9bd3b7) SHA1(14fd39ec12df7fae99ccdb0484ee097d93bf8d95))
	ROM_LOAD16_BYTE( "211253-bios hi.u19", 0xe0001, 0x10000, CRC(6acb573f) SHA1(376d483db2bd1c775d46424e1176b24779591525))
ROM_END

ROM_START( megapcpl )
    ROM_REGION(0x1000000,"maincpu", 0)
    ROM_LOAD("et4000.bin", 0xc0000, 0x08000, CRC(f01e4be0) SHA1(95d75ff41bcb765e50bd87a8da01835fd0aa01d5))
    ROM_LOAD("wdbios.rom", 0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))

	ROM_LOAD16_BYTE( "41652.u18",  0xe0000, 0x10000, CRC(6f5b9a1c) SHA1(cae981a35a01234fcec99a96cb38075d7bf23474))
	ROM_LOAD16_BYTE( "486slc.u19", 0xe0001, 0x10000, CRC(6fb7e3e9) SHA1(c439cb5a0d83176ceb2a3555e295dc1f84d85103))
ROM_END

ROM_START( t2000sx )
    ROM_REGION( 0x1000000, "maincpu", 0 )
	ROM_LOAD( "014d.ic9", 0xe0000, 0x20000, CRC(e9010b02) SHA1(75688fc8e222640fa22bcc90343c6966fe0da87f))
ROM_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*     YEAR  NAME      PARENT   COMPAT   MACHINE    INPUT       INIT    COMPANY     FULLNAME */
COMP ( 1984, ibm5170,  0,       ibm5150, ibm5170,   atcga,	atega,      "International Business Machines",  "IBM PC/AT 5170", GAME_NOT_WORKING )
COMP ( 1985, ibm5170a, ibm5170, 0,       ibm5170a,  atcga,  atega,      "International Business Machines",  "IBM PC/AT 5170 8MHz", GAME_NOT_WORKING )
COMP ( 1993, ec1849,   ibm5170, 0,       ec1849,    atcga,	atcga,      "<unknown>",  "EC-1849", GAME_NOT_WORKING )
COMP ( 1985, ibm5162,  ibm5170, 0,       ibm5162,   atcga,  atcga,      "International Business Machines",  "IBM PC/XT-286 5162", GAME_NOT_WORKING )
COMP ( 1988, i8530286, ibm5170, 0,       ps2m30286, atvga,	ps2m30286,  "International Business Machines",  "IBM PS/2 Model 30-286", GAME_NOT_WORKING )
COMP ( 1989, i8555081, ibm5170, 0,       at386,		atvga,	at386,		"International Business Machines",  "IBM PS/2 Model 55SX", GAME_NOT_WORKING )
COMP ( 1987, at,       ibm5170, 0,       ibm5162,   atcga,	atcga,      "<generic>",  "PC/AT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMP ( 1988, cmdpc30,  ibm5170, 0,       ibm5162,   atcga,	atcga,      "Commodore Business Machines",  "PC 30 III", GAME_NOT_WORKING )
COMP ( 1989, neat,     ibm5170, 0,       ibm5162,   atcga,	atcga,      "<generic>",  "NEAT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMP ( 1987, atvga,    ibm5170, 0,       atvga,     atvga,	at_vga,     "<generic>",  "PC/AT (VGA, MF2 Keyboard)" , GAME_NOT_WORKING )
COMP ( 1988, at386,    ibm5170, 0,       at386,     atvga,	at386,      "<generic>",  "PC/AT 386 (VGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMP ( 1990, at486,    ibm5170, 0,       at486,     atvga,	at386,      "<generic>",  "PC/AT 486 (VGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMP ( 1993, ct486,    ibm5170, 0,       ct486,     atvga,	at386,      "<unknown>",  "PC/AT 486 with C&T chipset", GAME_NOT_WORKING )
COMP ( 1995, ficpio2,  ibm5170, 0,       at486,     atvga,  at386,      "FIC", "486-PIO-2", GAME_NOT_WORKING )
COMP ( 1990, at586,    ibm5170, 0,       at586,     atvga,	at386,      "<generic>",  "PC/AT 586 (VGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMP ( 1990, c386sx16, ibm5170, 0,       c386sx16,  atvga,  at386,      "Commodore Business Machines", "Commodore 386SX-16", GAME_NOT_WORKING )
COMP ( 1997, ficvt503, ibm5170, 0,       at586,     atvga,  at386,      "FIC", "VT-503", GAME_NOT_WORKING )
COMP ( 1993, megapc,   ibm5170, 0,       megapc,    atvga,  at386,      "Amstrad plc", "MegaPC", GAME_NOT_WORKING )
COMP ( 199?, megapcpl, ibm5170, 0,       megapcpl,  atvga,  at386,      "Amstrad plc", "MegaPC Plus", GAME_NOT_WORKING )
COMP ( 1991, t2000sx,  ibm5170, 0,       c386sx16,  atvga,	at386,      "Toshiba",  "T2000SX", GAME_NOT_WORKING )
