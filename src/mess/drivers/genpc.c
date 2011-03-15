/***************************************************************************

    drivers/genpc.c

	Driver file for geenric PC machines

***************************************************************************/


#include "emu.h"

#include "includes/genpc.h"

#include "cpu/nec/nec.h"
#include "cpu/i86/i86.h"

#include "video/pc_vga_mess.h"
#include "video/pc_cga.h"
#include "video/isa_mda.h"

#include "machine/ram.h"
#include "machine/isa.h"

#include "machine/isa_com.h"
#include "machine/isa_fdc.h"
#include "machine/isa_hdc.h"

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
ADDRESS_MAP_END

static ADDRESS_MAP_START(pc16_io, ADDRESS_SPACE_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END


static INPUT_PORTS_START( pcgen )
	PORT_INCLUDE( kb_keytronic_pc )
INPUT_PORTS_END

static INPUT_PORTS_START( pccga )
	PORT_INCLUDE( kb_keytronic_pc )
	PORT_INCLUDE( pcvideo_cga )
INPUT_PORTS_END

static INPUT_PORTS_START( xtvga )
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

	PORT_INCLUDE( kb_keytronic_pc )
INPUT_PORTS_END
	
static const unsigned i86_address_mask = 0x000fffff;

static const kb_keytronic_interface pc_keytronic_intf =
{
	DEVCB_DEVICE_HANDLER("mb",genpc_kb_set_clock_signal),	
	DEVCB_DEVICE_HANDLER("mb",genpc_kb_set_data_signal)
};


static MACHINE_CONFIG_START( pcmda, genpc_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", V20, 4772720)
	MCFG_CPU_PROGRAM_MAP(pc8_map)
	MCFG_CPU_IO_MAP(pc8_io)

	MCFG_PC_MOTHERBOARD_ADD("mb","maincpu")
	
	MCFG_ISA8_BUS_DEVICE("mb:isa", 0, "mda", ISA8_MDA)
	MCFG_ISA8_BUS_DEVICE("mb:isa", 1, "com", ISA8_COM)
	MCFG_ISA8_BUS_DEVICE("mb:isa", 2, "fdc", ISA8_FDC)
	MCFG_ISA8_BUS_DEVICE("mb:isa", 3, "hdc", ISA8_HDC)

	/* video hardware */
	MCFG_PALETTE_LENGTH( 256 )
	
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
	
	MCFG_PC_MOTHERBOARD_ADD("mb","maincpu")
	
	MCFG_ISA8_BUS_DEVICE("mb:isa", 0, "hercules", ISA8_HERCULES)
	MCFG_ISA8_BUS_DEVICE("mb:isa", 1, "com", ISA8_COM)
	MCFG_ISA8_BUS_DEVICE("mb:isa", 2, "fdc", ISA8_FDC)
	MCFG_ISA8_BUS_DEVICE("mb:isa", 3, "hdc", ISA8_HDC)

	/* video hardware */
	MCFG_PALETTE_LENGTH( 256 )
	
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
	
	MCFG_PC_MOTHERBOARD_ADD("mb","maincpu")
	
	/* video hardware */
	MCFG_FRAGMENT_ADD( pcvideo_cga )
	MCFG_PALETTE_LENGTH( 256 )

    MCFG_ISA8_BUS_DEVICE("mb:isa", 1, "com", ISA8_COM)
	MCFG_ISA8_BUS_DEVICE("mb:isa", 2, "fdc", ISA8_FDC)
	MCFG_ISA8_BUS_DEVICE("mb:isa", 3, "hdc", ISA8_HDC)

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
	
	MCFG_PC_MOTHERBOARD_ADD("mb","maincpu")
	
	MCFG_ISA8_BUS_DEVICE("mb:isa", 1, "com", ISA8_COM)
	MCFG_ISA8_BUS_DEVICE("mb:isa", 2, "fdc", ISA8_FDC)
	MCFG_ISA8_BUS_DEVICE("mb:isa", 3, "hdc", ISA8_HDC)
	
	/* video hardware */
	MCFG_FRAGMENT_ADD( pcvideo_vga )
	MCFG_PALETTE_LENGTH( 256 )

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
//  ROM_LOAD("xthdd.rom",  0xc8000, 0x02000, CRC(a96317da))
	ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, CRC(031aafad) SHA1(a641b505bbac97b8775f91fe9b83d9afdf4d038f))

	/* IBM 1501981(CGA) and 1501985(MDA) Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f)) /* "AMI 8412PI // 5788005 // (C) IBM CORP. 1981 // KOREA" */
ROM_END

ROM_START( xtvga )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD("et4000.bin", 0xc0000, 0x8000, CRC(f01e4be0) SHA1(95d75ff41bcb765e50bd87a8da01835fd0aa01d5)) // from unknown revision/model of Tseng ET4000 Video card
	ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, CRC(031aafad) SHA1(a641b505bbac97b8775f91fe9b83d9afdf4d038f))
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*     YEAR     NAME        PARENT      COMPAT  MACHINE     INPUT       INIT        COMPANY     FULLNAME */
COMP(  1987,	pc,         ibm5150,	0,		pccga,		pccga,		genpccga,	"<generic>",  "PC (CGA)" , 0)
COMP ( 1987,	pcmda,      ibm5150,	0,		pcmda,      pcgen,		genpc,    	"<generic>",  "PC (MDA)" , 0)
COMP ( 1987,	pcherc,     ibm5150,	0,		pcherc,     pcgen,      genpc,    	"<generic>",  "PC (Hercules)" , 0)
COMP ( 1987,	xtvga,      ibm5150,	0,		xtvga,      xtvga,		genpcvga,	"<generic>",  "PC (VGA)" , GAME_NOT_WORKING)
