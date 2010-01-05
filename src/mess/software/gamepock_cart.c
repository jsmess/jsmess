/***************************************************************************

  Epoch Game Pocket Computers cartridges

***************************************************************************/

#include "driver.h"
#include "softlist.h"
#include "devices/cartslot.h"


SOFTWARE_START( astrobom )
	ROM_REGION( 0x2000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "astrobom.bin", 0, 0x2000, CRC(b0fd260f) SHA1(453a0f3c0952ebd8e691316c39960731f1996c09) )
SOFTWARE_END


SOFTWARE_START( blockmaz )
	ROM_REGION( 0x2000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "blockmaz.bin", 0, 0x2000, CRC(cfb3291b) SHA1(50dc5736200986b326b372c17c233c4180474471) )
SOFTWARE_END


SOFTWARE_START( pokemahj )
	ROM_REGION( 0x4000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "pokemahj.bin", 0, 0x4000, CRC(5c3eed48) SHA1(918e1caa16cfae6b74da2026f3426d0a5818061c) )
SOFTWARE_END


SOFTWARE_START( pokereve )
	ROM_REGION( 0x2000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "pokereve.bin", 0, 0x2000, CRC(1c461f91) SHA1(ead4a4efe5439e2ec1f6befb50c350f73919da8d) )
SOFTWARE_END


SOFTWARE_START( soukoban )
	ROM_REGION( 0x2000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "soukoban.bin", 0, 0x2000, CRC(5d6f7819) SHA1(61ef6483e8f9935dd8b6351fd2bdfda3af3899bd) )
SOFTWARE_END


SOFTWARE_LIST_START( gamepock_cart, "Epoch Game Pocket Computer cartridges" )
	SOFTWARE( astrobom, 0, 198x, "Epoch", "Astro Bomber", NULL, 0, 0 )
	SOFTWARE( blockmaz, 0, 198x, "Epoch", "Block Maze", NULL, 0, 0 )
	SOFTWARE( pokemahj, 0, 198x, "Epoch", "Pokekon Mahjongg", NULL, 0, 0 )
	SOFTWARE( pokereve, 0, 198x, "Epoch", "Pokekon Reversi", NULL, 0, 0 )
	SOFTWARE( soukoban, 0, 1985, "Epoch", "Soukoban - Store Keepers", NULL, 0, 0 )
SOFTWARE_LIST_END

