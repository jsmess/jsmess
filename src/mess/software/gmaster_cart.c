/***************************************************************************

  Hartung Game Master cartridges

***************************************************************************/

#include "driver.h"
#include "softlist.h"
#include "devices/cartslot.h"


SOFTWARE_START( bublboy )
	ROM_REGION( 0x4000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "bubbleboy.bin", 0x00000, 0x4000, CRC(edd0679d) SHA1(5f6f81b1a5ba6d21e5fb5bc38aa9ee0770b46b97) )
SOFTWARE_END

SOFTWARE_START( cg2020 )
	ROM_REGION( 0x4000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "cg2020.bin", 0x00000, 0x4000, CRC(fedec9d5) SHA1(f2afe09e8b6368cf2fb956afe7fd055107f943ee) )
SOFTWARE_END

SOFTWARE_START( dungeona )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "dungeona.bin", 0x00000, 0x8000, CRC(60afb4b5) SHA1(6048a84ac5a5a51a6522c662398e44cdf2934588) )
SOFTWARE_END

SOFTWARE_START( fallbloc )
	ROM_REGION( 0x2000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "fallblock.bin", 0x00000, 0x2000, CRC(58e409fb) SHA1(f6f7e3dae361fbd56d4a8457d81bb4f3828c48aB) )
SOFTWARE_END

SOFTWARE_START( gobang )
	ROM_REGION( 0x4000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "gobang.bin", 0x00000, 0x4000, CRC(fda2c4e6) SHA1(959c16b1657154bb352cb4d6a3e177d7b46850c6) )
SOFTWARE_END

SOFTWARE_START( hspace )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "hyperspace.bin", 0x00000, 0x8000, CRC(95e0ceac) SHA1(b3f95ea3dbc881262e734a2e35feb44a1ddd5641) )
SOFTWARE_END

SOFTWARE_START( kungfu )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "kungfu.bin", 0x00000, 0x8000, CRC(3ea0670b) SHA1(01647f87f59f2b05bb46f7bdb2fab38bcd8e3d6e) )
SOFTWARE_END

SOFTWARE_START( pinball )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "pinball.bin", 0x00000, 0x8000, CRC(f5ffc768) SHA1(1a30e6b969a8f3824e1aae0bbeb38dabbc75da8b) )
SOFTWARE_END

SOFTWARE_START( scastle )
	ROM_REGION( 0x4000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "scastle.bin", 0x00000, 0x4000, CRC(11d734f1) SHA1(6b213a19fc65e967b1b6beba4c4e9e1491b59268) )
SOFTWARE_END

SOFTWARE_START( tennism )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "tennism.bin", 0x00000, 0x8000, CRC(48d425cf) SHA1(080642009569d7cfdb3d1afe0ce5315842ce4ae8) )
SOFTWARE_END

SOFTWARE_START( uchamp )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "urbanchamp.bin", 0x00000, 0x8000, CRC(d53e6cac) SHA1(0bea8b120b3cb02136519010cb217fd0b819617d) )
SOFTWARE_END


SOFTWARE_LIST_START( gmaster_cart )
	/* Bomb Disposer */
	SOFTWARE( bublboy,    0,         1990, "Hartung", "Bubble Boy", 0, 0 )
	/* Car Racing */
	SOFTWARE( cg2020,     0,         1990, "Hartung", "Continental Galaxy 2020", 0, 0 )
	SOFTWARE( dungeona,   0,         1990, "Hartung", "Dungeon Adventure", 0, 0 )
	SOFTWARE( fallbloc,   0,         1990, "Hartung", "Falling Block!", 0, 0 )
	SOFTWARE( gobang,     0,         1990, "Hartung", "Go Bang", 0, 0 )
	SOFTWARE( hspace,     0,         1990, "Hartung", "Hyper Space", 0, 0 )
	/* Invader */
	SOFTWARE( kungfu,     0,         1990, "Hartung", "Kung Fu Challenge", 0, 0 )
	/* Move It */
	SOFTWARE( pinball,    0,         1990, "Hartung", "Pin Ball", 0, 0 )
	/* S-Race */
	/* Soccer (Fuss-Ball) */
	SOFTWARE( scastle,    0,         1990, "Hartung", "Space Castle", 0, 0 )
	/* Tank War */
	SOFTWARE( tennism,    0,         1990, "Hartung", "Tennis Master", 0, 0 )
	SOFTWARE( uchamp,     0,         1990, "Hartung", "Urban Champion", 0, 0 )
SOFTWARE_LIST_END


SOFTWARE_LIST( gmaster_cart, "Hartung Game Master cartridges" )
