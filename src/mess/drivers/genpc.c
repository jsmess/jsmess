/***************************************************************************

    drivers/genpc.c

	Driver file for geenric PC machines

***************************************************************************/


#include "emu.h"

#include "includes/genpc.h"

#include "cpu/nec/nec.h"
#include "cpu/i86/i86.h"

#include "machine/i8255a.h"
#include "machine/pic8259.h"
#include "machine/pit8253.h"
#include "machine/8237dma.h"

#include "video/pc_vga_mess.h"
#include "video/pc_cga.h"
#include "video/isa_mda.h"

#include "machine/ram.h"
#include "machine/isa.h"

#include "machine/isa_com.h"
#include "machine/isa_fdc.h"
#include "machine/isa_hdc.h"

#include "sound/speaker.h"

#include "machine/kb_keytro.h"

static ADDRESS_MAP_START( pc8_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK("bank10")
	AM_RANGE(0xa0000, 0xbffff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_ROM
	AM_RANGE(0xc8000, 0xcffff) AM_ROM
	AM_RANGE(0xd0000, 0xeffff) AM_NOP
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc16_map, ADDRESS_SPACE_PROGRAM, 16 )
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
	AM_RANGE(0x0080, 0x0087) AM_READWRITE(genpc_page_r,	genpc_page_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START(pc16_io, ADDRESS_SPACE_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE8("dma8237", i8237_r, i8237_w, 0xffff)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE8("pic8259", pic8259_r, pic8259_w, 0xffff)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE8("pit8253", pit8253_r, pit8253_w, 0xffff)
	AM_RANGE(0x0060, 0x0063) AM_DEVREADWRITE8("ppi8255", i8255a_r, i8255a_w, 0xffff)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE8(genpc_page_r, genpc_page_w, 0xffff)
ADDRESS_MAP_END


static INPUT_PORTS_START( pcgen )
	PORT_START("DSW0") /* IN1 */
	PORT_DIPNAME( 0xc0, 0x40, "Number of floppy drives")
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_DIPNAME( 0x30, 0x30, "Graphics adapter")
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_DIPNAME( 0x0c, 0x0c, "RAM banks")
	PORT_DIPSETTING(	0x00, "1 - 16/ 64/256K" )
	PORT_DIPSETTING(	0x04, "2 - 32/128/512K" )
	PORT_DIPSETTING(	0x08, "3 - 48/192/576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64/256/640K" )
	PORT_DIPNAME( 0x02, 0x00, "8087 installed")
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x02, DEF_STR(Yes) )
	PORT_DIPNAME( 0x01, 0x01, "Any floppy drive installed")
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x01, DEF_STR(Yes) )

	PORT_INCLUDE( kb_keytronic_pc )
INPUT_PORTS_END

static INPUT_PORTS_START( pccga )
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
	PORT_DIPNAME( 0x02, 0x00, "8087 installed")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x01, "Floppy installed")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )

	PORT_INCLUDE( kb_keytronic_pc )
	PORT_INCLUDE( pcvideo_cga )
INPUT_PORTS_END

static const unsigned i86_address_mask = 0x000fffff;

static const kb_keytronic_interface pc_keytronic_intf =
{
	DEVCB_MEMORY_HANDLER("maincpu", IO, genpc_kb_set_clock_signal),
	DEVCB_MEMORY_HANDLER("maincpu", IO, genpc_kb_set_data_signal),
};

static const isabus_interface isabus_intf =
{
	// interrupts
	DEVCB_DEVICE_LINE("pic8259", pic8259_ir2_w),
	DEVCB_DEVICE_LINE("pic8259", pic8259_ir3_w),
	DEVCB_DEVICE_LINE("pic8259", pic8259_ir4_w),
	DEVCB_DEVICE_LINE("pic8259", pic8259_ir5_w),
	DEVCB_DEVICE_LINE("pic8259", pic8259_ir6_w),
	DEVCB_DEVICE_LINE("pic8259", pic8259_ir7_w),

	// dma request
	DEVCB_DEVICE_LINE("dma8237", i8237_dreq1_w),
	DEVCB_DEVICE_LINE("dma8237", i8237_dreq2_w),
	DEVCB_DEVICE_LINE("dma8237", i8237_dreq3_w)
};

static MACHINE_CONFIG_START( pcmda, genpc_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", V20, 4772720)
	MCFG_CPU_PROGRAM_MAP(pc8_map)
	MCFG_CPU_IO_MAP(pc8_io)

	MCFG_MACHINE_START(genpc)
	MCFG_MACHINE_RESET(genpc)

	MCFG_PIT8253_ADD( "pit8253", genpc_pit8253_config )

	MCFG_I8237_ADD( "dma8237", XTAL_14_31818MHz/3, genpc_dma8237_config )

	MCFG_PIC8259_ADD( "pic8259", genpc_pic8259_config )

	MCFG_I8255A_ADD( "ppi8255", genpc_ppi8255_interface )

	MCFG_PALETTE_LENGTH( 256 )
		
	MCFG_ISA8_BUS_ADD("isa", "maincpu", isabus_intf)
	MCFG_ISA8_BUS_DEVICE("isa", 0, "mda", ISA8_MDA)
	MCFG_ISA8_BUS_DEVICE("isa", 1, "com", ISA8_COM)
	MCFG_ISA8_BUS_DEVICE("isa", 2, "fdc", ISA8_FDC)
	MCFG_ISA8_BUS_DEVICE("isa", 3, "hdc", ISA8_HDC)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* keyboard */
	MCFG_KB_KEYTRONIC_ADD("keyboard", pc_keytronic_intf)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("640K")
MACHINE_CONFIG_END


static MACHINE_CONFIG_START( pcherc, genpc_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", V20, 4772720)
	MCFG_CPU_PROGRAM_MAP(pc8_map)
	MCFG_CPU_IO_MAP(pc8_io)

	MCFG_MACHINE_START(genpc)
	MCFG_MACHINE_RESET(genpc)

	MCFG_PIT8253_ADD( "pit8253", genpc_pit8253_config )

	MCFG_I8237_ADD( "dma8237", XTAL_14_31818MHz/3, genpc_dma8237_config )

	MCFG_PIC8259_ADD( "pic8259", genpc_pic8259_config )

	MCFG_I8255A_ADD( "ppi8255", genpc_ppi8255_interface )

	MCFG_PALETTE_LENGTH( 256 )	
	
	MCFG_ISA8_BUS_ADD("isa", "maincpu", isabus_intf)
	MCFG_ISA8_BUS_DEVICE("isa", 0, "hercules", ISA8_HERCULES)
	MCFG_ISA8_BUS_DEVICE("isa", 1, "com", ISA8_COM)
	MCFG_ISA8_BUS_DEVICE("isa", 2, "fdc", ISA8_FDC)
	MCFG_ISA8_BUS_DEVICE("isa", 3, "hdc", ISA8_HDC)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* keyboard */
	MCFG_KB_KEYTRONIC_ADD("keyboard", pc_keytronic_intf)
	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("640K")
MACHINE_CONFIG_END


static MACHINE_CONFIG_START( pccga, genpc_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",  I8086, 4772720)				
	MCFG_CPU_PROGRAM_MAP(pc16_map)	
	MCFG_CPU_IO_MAP(pc16_io)		
	MCFG_CPU_CONFIG(i86_address_mask)
	
	MCFG_MACHINE_START(genpc)
	MCFG_MACHINE_RESET(genpc)

	MCFG_PIT8253_ADD( "pit8253", genpc_pit8253_config )

	MCFG_I8237_ADD( "dma8237", XTAL_14_31818MHz/3, genpc_dma8237_config )

	MCFG_PIC8259_ADD( "pic8259", genpc_pic8259_config )

	MCFG_I8255A_ADD( "ppi8255", genpc_ppi8255_interface )

	/* video hardware */
	MCFG_FRAGMENT_ADD( pcvideo_cga )

	MCFG_ISA8_BUS_ADD("isa", "maincpu", isabus_intf)
	MCFG_ISA8_BUS_DEVICE("isa", 1, "com", ISA8_COM)
	MCFG_ISA8_BUS_DEVICE("isa", 2, "fdc", ISA8_FDC)
	MCFG_ISA8_BUS_DEVICE("isa", 3, "hdc", ISA8_HDC)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* keyboard */
	MCFG_KB_KEYTRONIC_ADD("keyboard", pc_keytronic_intf)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("640K")
MACHINE_CONFIG_END


static MACHINE_CONFIG_START( xtvga, genpc_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",  I8086, 4772720)				
	MCFG_CPU_PROGRAM_MAP(pc16_map)	
	MCFG_CPU_IO_MAP(pc16_io)	
	MCFG_CPU_CONFIG(i86_address_mask)
	
	MCFG_MACHINE_START(genpc)
	MCFG_MACHINE_RESET(genpc)

	MCFG_PIT8253_ADD( "pit8253", genpc_pit8253_config )

	MCFG_I8237_ADD( "dma8237", XTAL_14_31818MHz/3, genpc_dma8237_config )

	MCFG_PIC8259_ADD( "pic8259", genpc_pic8259_config )

	MCFG_I8255A_ADD( "ppi8255", genpc_ppi8255_interface )

	MCFG_ISA8_BUS_ADD("isa", "maincpu", isabus_intf)
	MCFG_ISA8_BUS_DEVICE("isa", 1, "com", ISA8_COM)
	MCFG_ISA8_BUS_DEVICE("isa", 2, "fdc", ISA8_FDC)
	MCFG_ISA8_BUS_DEVICE("isa", 3, "hdc", ISA8_HDC)
	/* video hardware */
	MCFG_FRAGMENT_ADD( pcvideo_vga )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* keyboard */
	MCFG_KB_KEYTRONIC_ADD("keyboard", pc_keytronic_intf)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("640K")
MACHINE_CONFIG_END


ROM_START( pcmda )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, CRC(031aafad) SHA1(a641b505bbac97b8775f91fe9b83d9afdf4d038f))
ROM_END


ROM_START( pcherc )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, CRC(031aafad) SHA1(a641b505bbac97b8775f91fe9b83d9afdf4d038f))
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

ROM_START( xtvga )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("et4000.bin", 0xc0000, 0x8000, CRC(f01e4be0) SHA1(95d75ff41bcb765e50bd87a8da01835fd0aa01d5)) // from unknown revision/model of Tseng ET4000 Video card
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC IDE Superbios 2.0 (06/28/89) Expansion Rom C8000-C9FFF  */
	ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, CRC(031aafad) SHA1(a641b505bbac97b8775f91fe9b83d9afdf4d038f))
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*     YEAR     NAME        PARENT      COMPAT  MACHINE     INPUT       INIT        COMPANY     FULLNAME */
COMP(  1987,	pc,         ibm5150,	0,		pccga,		pccga,		genpc,		"<generic>",  "PC (CGA)" , 0)
COMP ( 1987,	pcmda,      ibm5150,	0,		pcmda,      pcgen,		genpc,    	"<generic>",  "PC (MDA)" , 0)
COMP ( 1987,	pcherc,     ibm5150,	0,		pcherc,     pcgen,      genpc,    	"<generic>",  "PC (Hercules)" , 0)
COMP ( 1987,	xtvga,      ibm5150,	0,		xtvga,      pcgen,		genpcvga,	"<generic>",  "PC (VGA)" , GAME_NOT_WORKING)
