/***************************************************************************

  Epoch Super Cassette Vision cartridges

***************************************************************************/

#include "emu.h"
#include "softlist.h"
#include "devices/cartslot.h"


SOFTWARE_START( astrwrs )
	ROM_REGION( 0x2000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "astrwrs.bin", 0x0000, 0x2000, CRC(2dbdb166) SHA1(53a9a308b6f0bcb2137704f70657698719551b5e) )
SOFTWARE_END

SOFTWARE_START( astrwrs2 )
	ROM_REGION( 0x2000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "astrwrs2.bin", 0x0000, 0x2000, BAD_DUMP CRC(19056c71) SHA1(7b1073339a4641d4e0f00b284c64fc3c6d385b7c) )
SOFTWARE_END

SOFTWARE_START( elevfght )
	ROM_REGION( 0x4000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "elevfght.bin", 0x0000, 0x4000, CRC(72dd6031) SHA1(8b2c185408309ee1e75d40299c003041db17b133) )
SOFTWARE_END

SOFTWARE_START( nebula )
	ROM_REGION( 0x4000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "nebula.bin", 0x0000, 0x4000, CRC(5e6a4caf) SHA1(699eaf434e73a42e71adf52426b22f0dd85e2897) )
SOFTWARE_END

SOFTWARE_START( punchboy )
	ROM_REGION( 0x4000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "punchboy.bin", 0x0000, 0x4000, CRC(4107f03a) SHA1(e7391c71ba7deede3db22191ea058ccf429d7bb5) )
SOFTWARE_END

SOFTWARE_START( baseball )
	ROM_REGION( 0x4000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "baseball.bin", 0x0000, 0x4000, CRC(56f1c473) SHA1(37a1dce6c1921950c588d6492cd1b57feb615175) )
SOFTWARE_END

SOFTWARE_START( suprgolf )
	ROM_REGION( 0x4000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "suprgolf.bin", 0x0000, 0x4000, CRC(0c0581c9) SHA1(2d3ab09907a4a9c5d66306f316c7211f498cbcc2) )
SOFTWARE_END

SOFTWARE_START( soccer )
	ROM_REGION( 0x4000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "soccer.bin", 0x0000, 0x4000, CRC(8283907c) SHA1(61e4b8e43cc593b3a2e838c08f6dad6cca77d712) )
SOFTWARE_END

SOFTWARE_START( soccera )
	ROM_REGION( 0x4000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "soccera.bin", 0x0000, 0x4000, CRC(590eeefd) SHA1(951ecd2a1d2e7d900c13e91ff028723a9df9e60e) )
SOFTWARE_END

SOFTWARE_START( wheelrcr )
	ROM_REGION( 0x4000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "wheelrcr.bin", 0x0000, 0x4000, CRC(9d9b29db) SHA1(479f767384e7b7bfe58eeadd67dbe642fe8a2fcf) )
SOFTWARE_END


SOFTWARE_LIST_START( svc_cart )
	SOFTWARE( astrwrs,   0,         1984, "Epoch", "Astro Wars - Invaders from Space", 0, 0 )
	SOFTWARE( astrwrs2,  0,         1984, "Epoch", "Astro Wars II - Battle in Galaxy", 0, 0 )	/* bad dump? refuses to work on real h/w */
	/* Basic Nyuumon */
	/* Boulder Dash */
	/* Comic Circus */
	/* Doraemon */
	/* Dragon Ball */
	/* Dragon Slayer */
	SOFTWARE( elevfght,  0,         1984, "Epoch", "Elevator Fight", 0, 0 )
	/* Lupin III */
	/* Mappy */
	/* Milky Princess */
	/* Miner 2049er */
	SOFTWARE( nebula,    0,         1984, "Epoch", "Nebula", 0, 0 )
	/* Nekketsu Kung Fu Road */
	/* Pole Position II */
	/* Pop and Chips */
	SOFTWARE( punchboy,  0,         1984, "Epoch", "Punch Boy", 0, 0 )
	/* Rantou Pro Wrestling */
	/* Shogi Nyuumon */
	/* Sky Kid */
	/* Star Speeder */
	SOFTWARE( baseball,  0,         1984, "Epoch", "Super Baseball", 0, 0 )				/* name taken from boxart */
	SOFTWARE( suprgolf,  0,         1984, "Epoch", "Super Golf", 0, 0 )				/* name taken from boxart */
	/* Super Mahjong */
	/* Super Sansuu-Puter */
	SOFTWARE( soccer,    0,         1985, "Epoch", "Super Soccer", 0, 0 )				/* jpn cart, name taken from boxart */
	SOFTWARE( soccera,   soccer,    1985, "Epoch", "Super Soccer (alt)", 0, 0 )			/* maybe pal, name taken from boxart */
	/* Ton Ton Ball */
	/* Wai Wai Monster Land */
	SOFTWARE( wheelrcr,  0,         1985, "Epoch", "Wheelie Racer", 0, 0 )
SOFTWARE_LIST_END


SOFTWARE_LIST( svc_cart, "Epoch Super Cassette Vision cartridges" )
