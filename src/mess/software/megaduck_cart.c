/***************************************************************************

  Creatronic Mega Duck cartridges

***************************************************************************/

#include "emu.h"
#include "softlist.h"
#include "devices/cartslot.h"


SOFTWARE_START( arczone )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "arczone.bin", 0x0000, 0x8000, CRC(f88f2d25) SHA1(4f3022126f33964e0fb5505fa0caeaa13b995309) )
SOFTWARE_END

SOFTWARE_START( aforce )
	ROM_REGION( 0x10000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "aforce.bin", 0x0000, 0x10000, CRC(eca6c0ad) SHA1(aa2c3774eb48df0854d4b61997653f79a810f4cc) )
SOFTWARE_END

SOFTWARE_START( bstfgtr )
	ROM_REGION( 0x20000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "bstfgtr.bin", 0x0000, 0x20000, CRC(2adb7253) SHA1(9ff96b40e8636fad37b1b95f83f3a9584f3dfe3e) )
SOFTWARE_END

SOFTWARE_START( bftale )
	ROM_REGION( 0x10000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "bftale.bin", 0x0000, 0x10000, CRC(211d268b) SHA1(3d62acc2db46bb7b6a2909d640dec6ce095aeec3) )
SOFTWARE_END

SOFTWARE_START( c5in1 )
	ROM_REGION( 0x10000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "5in1.bin", 0x00000, 0x10000, CRC(ff0da355) SHA1(fb4ba6c9823808c2705f597294b784837342cbcb) )
SOFTWARE_END

SOFTWARE_START( magitowr )
	ROM_REGION( 0x10000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "magitowr.bin", 0x00000, 0x10000, CRC(98694021) SHA1(6eefbf8742eba1279f943c321b5d55e45a727a67) )
SOFTWARE_END

SOFTWARE_START( md4in1 )
	ROM_REGION( 0x10000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "md4in1.bin", 0x00000, 0x10000, CRC(8046ea70) SHA1(2a1c5bea602ab27ecf680c88cfe9884186b61438) )
SOFTWARE_END

SOFTWARE_START( pilewndr )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "pilewndr.bin", 0x0000, 0x8000, CRC(d71f1770) SHA1(69222a0c5c3920d0784d0e58c3407b74c1b2bfe8) )
SOFTWARE_END

SOFTWARE_START( plewndra )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "plewndra.bin", 0x0000, 0x8000, CRC(61c81e06) SHA1(13d8882371823e5dfb74191ef0addafc43aa1de4) )
SOFTWARE_END

SOFTWARE_START( puptkngt )
	ROM_REGION( 0x10000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "puptkngt.bin", 0x00000, 0x10000, CRC(35fb1616) SHA1(4849CD6F881B5B78E1D3A08EDB727E79C77F9D39) )
SOFTWARE_END

SOFTWARE_START( railway )
	ROM_REGION( 0x10000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "railway.bin", 0x00000, 0x10000, CRC(2bb6aeb9) SHA1(f1a9a8365f1b78057ed5cc647fd68efafc0c49e8) )
SOFTWARE_END

SOFTWARE_START( snakeroy )
	ROM_REGION( 0x10000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "snakeroy.bin", 0x00000, 0x10000, CRC(315ef9f2) SHA1(c76a57047f0f72252cddd7974a3b694392753469) )
SOFTWARE_END

SOFTWARE_START( srider )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "srider.bin", 0x0000, 0x8000, CRC(74c3377f) SHA1(9b7d69f9b0a44eeaa36971aabec553ebd3bfdc0e) )
SOFTWARE_END

SOFTWARE_START( treasure )
	ROM_REGION( 0x10000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "treasure.bin", 0x00000, 0x10000, CRC(cd2730ac) SHA1(022d7e635ea64c6a525db5d31c30a7d8636f4c18) )
SOFTWARE_END

SOFTWARE_START( brckwall )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "brckwall.bin", 0x0000, 0x8000, CRC(58efe338) SHA1(9b050e41b9aa18dbf5c043f19b7c945c89811cc1) )
SOFTWARE_END


SOFTWARE_LIST_START( megaduck_cart )
	SOFTWARE( arczone,  0,         1993, "Commin", "Arctic Zone", 0, 0 )
	SOFTWARE( aforce,   0,         1993, "Commin", "Armour Force", 0, 0 )
	SOFTWARE( bstfgtr,  0,         1993, "Sachen", "Beast Fighter", 0, 0 )
	SOFTWARE( bftale,   0,         1993, "Commin", "Black Forest Tale", 0, 0 )
	SOFTWARE( c5in1,    0,         1993, "Commin", "Commin 5 in 1", 0, 0 )
	SOFTWARE( magitowr, 0,         1993, "Sachen", "Magical Tower", 0, 0 )
	SOFTWARE( md4in1,   0,         1993, "Sachen", "Mega Duck 4 in 1 Game", 0, 0 )
	SOFTWARE( pilewndr, 0,         1993, "Commin", "Pile Wonder", 0, 0 )
	SOFTWARE( plewndra, pilewndr,  1993, "Commin", "Pile Wonder (alt)", 0, 0 )
	SOFTWARE( puptkngt, 0,         1993, "Commin", "Puppet Knight", 0, 0 )
	SOFTWARE( railway,  0,         1993, "Sachen", "Railway", 0, 0 )
	SOFTWARE( snakeroy, 0,         1993, "Sachen", "Snake Roy", 0, 0 )
	SOFTWARE( srider,   0,         1993, "Commin", "Street Rider", 0, 0 )
	SOFTWARE( treasure, 0,         1993, "Commin", "Suleimans Treasure", 0, 0 )
	SOFTWARE( brckwall, 0,         1993, "Timlex International", "The Brick Wall", 0, 0 )
SOFTWARE_LIST_END


SOFTWARE_LIST( megaduck_cart, "Creatronic Mega Duck cartridges" )
