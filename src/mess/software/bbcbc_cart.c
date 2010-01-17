/***************************************************************************

  BBC Bridge Companion cartridges

***************************************************************************/

#include "driver.h"
#include "softlist.h"
#include "devices/cartslot.h"


SOFTWARE_START( bidding )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "masc_03a.bin", 0x0000, 0x8000, CRC(cbec2471) SHA1(770ab06b1fcc35da08ce8453b754b8f62520cdd0) )
SOFTWARE_END


SOFTWARE_START( defence )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "advco4a.bin", 0x0000, 0x8000, CRC(e5ff9113) SHA1(262f6f72bd0b63531102e8d4da0345b39ca3ea2f) )
SOFTWARE_END


SOFTWARE_START( bbuilder )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "bbc2_1.bin", 0x0000, 0x2000, CRC(ee348134) SHA1(0528f7c935549f5fe7c033f1f5e58cba8a03736b) )
	ROM_LOAD( "bbc2_2.bin", 0x2000, 0x2000, CRC(358f6dea) SHA1(b0f6afbef17727fbf5664b010cd7b70dd906ea80) )
	ROM_LOAD( "bbc2_3.bin", 0x4000, 0x2000, CRC(c94691ac) SHA1(16f52712db0d7122b850bada6200408b22d95a43) )
	ROM_LOAD( "bbc2_4.bin", 0x6000, 0x2000, CRC(ec6405e2) SHA1(eb59238b446e94428081fc5d3594eac60b873192) )
SOFTWARE_END


SOFTWARE_START( bbuildera )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "bbc4a.0", 0x0000, 0x4000, CRC(d1f20bc0) SHA1(368fa3ff5affea6ca355b09b4f00917299fa6a8e) )
	ROM_LOAD( "bbc4a.1", 0x4000, 0x4000, CRC(2ee770f9) SHA1(15e62090c2329954a41140041e73e953006be699) )
SOFTWARE_END


SOFTWARE_START( cplay1 )
	ROM_REGION( 0x6000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "cpc_2_1.bin", 0x0000, 0x2000, CRC(1efd1481) SHA1(d0483da7ae3abff4a1141a89d066e2b5879a52c1) )
	ROM_LOAD( "cpc_2_2.bin", 0x2000, 0x2000, CRC(93b263ec) SHA1(8d63244d91156a9d86da8460eb80d0b7d287675a) )
	ROM_LOAD( "cpc_2_3.bin", 0x4000, 0x2000, CRC(9ceb5493) SHA1(ebed605d297e34f85c831c023e5925ce800d57a3) )
SOFTWARE_END


SOFTWARE_START( cplay2 )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "2cp1_1.bin", 0x0000, 0x4000, CRC(5e18577d) SHA1(b22858fb42231453e80fa994a7431e9853b4a2eb) )
	ROM_LOAD( "2cp1_2.bin", 0x4000, 0x4000, CRC(6a99a161) SHA1(9d8e215dd7ef2cf50c28c43d0e73236ee17ab8a6) )
SOFTWARE_END


SOFTWARE_START( cplay2a )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "club_play_2.bin", 0x0000, 0x8000, CRC(2867fc62) SHA1(ef519def0ad37882da81a9c16371cf2b1d8919ee) )
SOFTWARE_END


SOFTWARE_START( cplay3 )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "cp3.bin", 0x0000, 0x8000, CRC(a17569d8) SHA1(151256d1d0957e3c6c07e2b0150c679384ce195d) )
SOFTWARE_END


SOFTWARE_START( convent1 )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "conc019.bin", 0x0000, 0x8000, CRC(968179b3) SHA1(eb23cd6506a2afea3ff72535be18102036723f94) )
SOFTWARE_END


SOFTWARE_START( duplict1 )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "dupc04a.bin", 0x0000, 0x8000, CRC(83d55b90) SHA1(61a01c0ccb19f01b3e875db467886969b8284259) )
SOFTWARE_END


SOFTWARE_START( mplay1 )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "mp_1_1.bin", 0x0000, 0x4000, CRC(d28d9995) SHA1(4288eae32eaabbe489b549a30e4eb35a4e671ae7) )
	ROM_LOAD( "mp_1_2.bin", 0x4000, 0x4000, CRC(b221ff05) SHA1(95b13a2ce554a394a4139e868e5be9455b091b26) )
SOFTWARE_END


SOFTWARE_LIST_START( bbcbc_cart )
	SOFTWARE( bidding,    0,         198x, "BBC", "Advanced Bidding", 0, 0 )
	SOFTWARE( defence,    0,         198x, "BBC", "Advanded Defence", 0, 0 )
	SOFTWARE( bbuilder,   0,         198x, "BBC", "Bridge Builder", 0, 0 )
	SOFTWARE( bbuildera,  bbuilder,  198x, "BBC", "Bridge Builder (Alt)", 0, 0 )
	SOFTWARE( cplay1,     0,         198x, "BBC", "Club Play 1", 0, 0 )
	SOFTWARE( cplay2,     0,         198x, "BBC", "Club Play 2", 0, 0 )
	SOFTWARE( cplay2a,    cplay2,    198x, "BBC", "Club Play 2 (Alt)", 0, 0 )
	SOFTWARE( cplay3,     0,         198x, "BBC", "Club Play 3", 0, 0 )
	SOFTWARE( convent1,   0,         198x, "BBC", "Conventions 1", 0, 0 )
	SOFTWARE( duplict1,   0,         198x, "BBC", "Duplicate 1", 0, 0 )
	SOFTWARE( mplay1,     0,         198x, "BBC", "Master Play 1", 0, 0 )
SOFTWARE_LIST_END


SOFTWARE_LIST( bbcbc_cart, "BBC Bridge Companion cartridges" )

