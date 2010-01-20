/***************************************************************************

    Nintendo Virtual Boy cartridges

***************************************************************************/

#include "emu.h"
#include "softlist.h"
#include "devices/cartslot.h"

#define JAGUAR_ROM_LOAD( set, name, offset, length, hash )	\
SOFTWARE_START( set ) \
	ROM_REGION( 0x600000, CARTRIDGE_REGION_ROM, 0 ) \
	ROM_LOAD(name, offset, length, hash) \
SOFTWARE_END


JAGUAR_ROM_LOAD( aircars,   "air cars (world).j64",                                                                      0x000000 , 0x200000,	 CRC(cbfd822a) SHA1(9d1ecdb230f94d8145520ba03d187a19d6851a75) )
JAGUAR_ROM_LOAD( avsp,      "alien vs predator (world).j64",                                                             0x000000 , 0x400000,	 CRC(dc187f82) SHA1(fd8c89250ebc1e403838b2e589d1f69e3fe2fe02) )
JAGUAR_ROM_LOAD( atarikar,  "atari karts (world).j64",                                                                   0x000000 , 0x400000,	 CRC(e28756de) SHA1(9c96e6c2195ce56aa930e677a041b35bd9b8bc7e) )
JAGUAR_ROM_LOAD( mutntpng,  "attack of the mutant penguins (world).j64",                                                 0x000000 , 0x200000,	 CRC(cd5bf827) SHA1(01dc37e7a9d03246a93bcb8c6a3116cb6391ed31) )
JAGUAR_ROM_LOAD( battlesp,  "battle sphere gold (world).j64",                                                            0x000000 , 0x400000,	 CRC(67f9ab3a) SHA1(49716a6a3a4c6abeea36040f26c4e8cf2a545df7) )
JAGUAR_ROM_LOAD( brutalsp,  "brutal sports football (world).j64",                                                        0x000000 , 0x200000,	 CRC(bcb1a4bf) SHA1(cdfefdf28dd9127f7a9425088a1ee3ea88dfb618) )
JAGUAR_ROM_LOAD( bubsy,     "bubsy in fractured furry tales (world).j64",                                                0x000000 , 0x200000,	 CRC(2e17d5da) SHA1(752d6e1ed185f9db49230f42319a573a44e32397) )
JAGUAR_ROM_LOAD( cfodder,   "cannon fodder (world).j64",                                                                 0x000000 , 0x200000,	 CRC(bda405c6) SHA1(0c195676be099f4594ce1d5a38303f6cce899dc5) )
JAGUAR_ROM_LOAD( chekflag,  "checkered flag (world).j64",                                                                0x000000 , 0x200000,	 CRC(fa7775ae) SHA1(4d0cc8d248a8bd0fc4b8169c5d5772e59c073c59) )
JAGUAR_ROM_LOAD( clubdriv,  "club drive (world).j64",                                                                    0x000000 , 0x200000,	 CRC(eee8d61d) SHA1(6588f7d677463d0ef10cf905d44a2581fd9dd055) )
JAGUAR_ROM_LOAD( cybermor1, "cybermorph (world) (rev 1).j64",                                                            0x000000 , 0x200000,	 CRC(bde67498) SHA1(e00ab55f555fd1fe63b4fce66a9b8ffe3485963e) )
JAGUAR_ROM_LOAD( cybermor,  "cybermorph (world) (rev 2).j64",                                                            0x000000 , 0x100000,	 CRC(ecf854e7) SHA1(c9aa59769df207d85d9212d619c266c793c9063b) )
JAGUAR_ROM_LOAD( defender,  "defender 2000 (world).j64",                                                                 0x000000 , 0x400000,	 CRC(27594c6a) SHA1(13a4084bd5be1cca21d83d4c26b5b8e5cf1a3e30) )
JAGUAR_ROM_LOAD( doom,      "doom (world).j64",                                                                          0x000000 , 0x400000,	 CRC(5e2cdbc0) SHA1(20ddf412e42a50d526cbbb6411bf0ec4516db283) )
JAGUAR_ROM_LOAD( ddragon5,  "double dragon v - the shadow falls (world).j64",                                            0x000000 , 0x200000,	 CRC(348e6449) SHA1(d66b4e5f9ac27d53f0d781c741e420698630b9fc) )
JAGUAR_ROM_LOAD( dragon,    "dragon - the bruce lee story (world).j64",                                                  0x000000 , 0x200000,	 CRC(8fea5ab0) SHA1(da7f5dac4317ab0258f54da436abb8d89e530f27) )
JAGUAR_ROM_LOAD( dinodude,  "evolution - dino dudes (world).j64",                                                        0x000000 , 0x200000,	 CRC(0ec5369d) SHA1(179b511464abcda5a5de9f971bd5cb5775d8c313) )
JAGUAR_ROM_LOAD( feverpit,  "fever pitch soccer (world) (en,fr,de,es,it).j64",                                           0x000000 , 0x200000,	 CRC(3615af6a) SHA1(5be3da69a64b779d064d139968fadbe1c475c715) )
JAGUAR_ROM_LOAD( fforlife1, "fight for life (world) (alt).j64",                                                          0x000000 , 0x400000,	 CRC(c6c7ba62) SHA1(686176f5a7bdaef690d853523a338beb8a93d244) )
JAGUAR_ROM_LOAD( fforlife,  "fight for life (world).j64",                                                                0x000000 , 0x400000,	 CRC(b14c4753) SHA1(9f5b8844d58bb1c174da4824eabbfd8616ba28d2) )
JAGUAR_ROM_LOAD( flashbck,  "flashback - the quest for identity (world) (en,fr).j64",                                    0x000000 , 0x200000,	 CRC(de55dcc7) SHA1(824613451be0b99020fc02e5dba15c45e76fc8c2) )
JAGUAR_ROM_LOAD( flipout,   "flip out! (world).j64",                                                                     0x000000 , 0x400000,	 CRC(fae31dd0) SHA1(61d4db841c090ae3104eb20b6087160ff80e7a12) )
JAGUAR_ROM_LOAD( hstrike,   "hover strike (world).j64",                                                                  0x000000 , 0x200000,	 CRC(4899628f) SHA1(94e615e7a57f7c08645322870121bb32310298f5) )
JAGUAR_ROM_LOAD( hyperfor,  "hyper force (world).j64",                                                                   0x000000 , 0x200000,	 CRC(f0360db3) SHA1(037396347d65d1a8b56e7db361d737ab2070118f) )
JAGUAR_ROM_LOAD( iwar,      "i-war (world).j64",                                                                         0x000000 , 0x200000,	 CRC(97eb4651) SHA1(3db485aaa28a817f468739a85e3f25741c3855ad) )
JAGUAR_ROM_LOAD( ironsold,  "iron soldier (world) (v1.04).j64",                                                          0x000000 , 0x200000,	 CRC(08f15576) SHA1(8bdefc60dd1734315e1c145e29abf41800f908cb) )
JAGUAR_ROM_LOAD( ironsol2,  "iron soldier 2 (world).j64",                                                                0x000000 , 0x200000,	 CRC(d6c19e34) SHA1(47b9580eb9eea121cf080cdbdc4afe7fb7a19ab5) )
JAGUAR_ROM_LOAD( kasumi,    "kasumi ninja (world).j64",                                                                  0x000000 , 0x400000,	 CRC(0957a072) SHA1(2327112b9c6f9b0e49c55def0adb8119ee39ce29) )
JAGUAR_ROM_LOAD( missilec,  "missile command 3d (world).j64",                                                            0x000000 , 0x200000,	 CRC(da9c4162) SHA1(3ea9053a63a3b0165ccec91e85f0e699a998dc0a) )
JAGUAR_ROM_LOAD( nbajamte,  "nba jam t.e. (world).j64",                                                                  0x000000 , 0x400000,	 CRC(0ac83d77) SHA1(b9c8ff2d26fe1e91ecd6d59b9f3f76c5b5650f26) )
JAGUAR_ROM_LOAD( pinballf,  "pinball fantasies (world).j64",                                                             0x000000 , 0x200000,	 CRC(5cff14ab) SHA1(4733cf2ede2713467f2a80b052ab94edf6dfe534) )
JAGUAR_ROM_LOAD( pitfall,   "pitfall - the mayan adventure (world).j64",                                                 0x000000 , 0x400000,	 CRC(817a2273) SHA1(25611b1cff1384db72cc5422dbe70555e5bbd32c) )
JAGUAR_ROM_LOAD( pdrive,    "power drive rally (world).j64",                                                             0x000000 , 0x200000,	 CRC(1660f070) SHA1(75ee0ab0daba31284a1dff27f3c247885f5a427a) )
JAGUAR_ROM_LOAD( protect,   "protector - special edition (world).j64",                                                   0x000000 , 0x200000,	 CRC(a56d0798) SHA1(ddea86820dcaa1a67bd82cf8023f9b3c494357b0) )
JAGUAR_ROM_LOAD( raiden,    "raiden (world).j64",                                                                        0x000000 , 0x200000,	 CRC(0509c85e) SHA1(9d2f894da3f25944c2f33e3a109ae8d0d951d329) )
JAGUAR_ROM_LOAD( rayman,    "rayman (world).j64",                                                                        0x000000 , 0x400000,	 CRC(a9f8a00e) SHA1(d22912992fd966365a1221886e0fc5303ef04a59) )
JAGUAR_ROM_LOAD( ruinerp,   "ruiner pinball (world).j64",                                                                0x000000 , 0x200000,	 CRC(5b6bb205) SHA1(e0db2fdea7f95743c1ef452b7d289f5fe8158e33) )
JAGUAR_ROM_LOAD( sensible,  "sensible soccer - international edition (world).j64",                                       0x000000 , 0x200000,	 CRC(5a101212) SHA1(f0914be41331e3045bbc04225044dc3c73fd9de1) )
JAGUAR_ROM_LOAD( skyhamm,   "skyhammer (world).j64",                                                                     0x000000 , 0x400000,	 CRC(3c044941) SHA1(34a9941459e3b66c836f25b3ef55303734b9583e) )
JAGUAR_ROM_LOAD( soccerkd,  "soccer kid (world).j64",                                                                    0x000000 , 0x200000,	 CRC(42a13ec5) SHA1(a738a2df47f09c3bfdeddfc01b35f3e8b5c5d99c) )
JAGUAR_ROM_LOAD( spacewar,  "space war 2000 (world).j64",                                                                0x000000 , 0x400000,	 CRC(53df6440) SHA1(b5da0b1231a6e6532268c4e04f74e56f047dd5a5) )
JAGUAR_ROM_LOAD( sburnout,  "super burnout (world).j64",                                                                 0x000000 , 0x200000,	 CRC(6f8b2547) SHA1(9a58bc0c8843fd00a2871c01b91fec206408bf80) )
JAGUAR_ROM_LOAD( superx3d,  "supercross 3d (world).j64",                                                                 0x000000 , 0x200000,	 CRC(ec22f572) SHA1(c71e0bc9eeed8670c6e7ccc7a0fe3b815d7bac83) )
JAGUAR_ROM_LOAD( syndicat,  "syndicate (world).j64",                                                                     0x000000 , 0x200000,	 CRC(58272540) SHA1(a226086571c857f3796fd664bbc2c8fe99c78224) )
JAGUAR_ROM_LOAD( tempst2k,  "tempest 2000 (world).j64",                                                                  0x000000 , 0x200000,	 CRC(6b2b95ad) SHA1(3828d2f953224d06b5fe7ab3644d6a75fdfe79e3) )
JAGUAR_ROM_LOAD( themeprk,  "theme park (world).j64",                                                                    0x000000 , 0x200000,	 CRC(47ebc158) SHA1(24da4fe776ae9daa86cf643c703ec0f399c77521) )
JAGUAR_ROM_LOAD( totalcar,  "total carnage (world).j64",                                                                 0x000000 , 0x400000,	 CRC(c654681b) SHA1(4566b33799a7c65b3e9643383d0282172ff619a5) )
JAGUAR_ROM_LOAD( towers2,   "towers ii (world).j64",                                                                     0x000000 , 0x200000,	 CRC(caf33bd6) SHA1(f8c14fab50ab04a025d15c3410a6698f6e4c61d3) )
JAGUAR_ROM_LOAD( trevmcfr,  "trevor mcfur in the crescent galaxy (world).j64",                                           0x000000 , 0x200000,	 CRC(1e451446) SHA1(88e86085932af10dfb8c94032f73a5992a30b55c) )
JAGUAR_ROM_LOAD( troyaik,   "troy aikman nfl football (world).j64",                                                      0x000000 , 0x200000,	 CRC(38a130ed) SHA1(4c6a6432ea0bc198f900f4ebdb8a3a7a34475d84) )
JAGUAR_ROM_LOAD( ultravor1, "ultra vortek (world) (v0.94) (beta).j64",                                                   0x000000 , 0x400000,	 CRC(a27823d8) SHA1(e3bd433bc4a573f3b05652239a6614c3878a04d1) )
JAGUAR_ROM_LOAD( ultravor,  "ultra vortek (world).j64",                                                                  0x000000 , 0x400000,	 CRC(0f6a1c2c) SHA1(311e41ebba1b78f514bf16789ddb7e7e8dfb47ff) )
JAGUAR_ROM_LOAD( valdiser,  "val d'isere skiing and snowboarding (world).j64",                                           0x000000 , 0x200000,	 CRC(c9608717) SHA1(02253d93eef375c8334f1f77a2d8b72fff7a94c6) )
JAGUAR_ROM_LOAD( whitemen,  "white men can't jump (world).j64",                                                          0x000000 , 0x400000,	 CRC(14915f20) SHA1(4d16596ac2d991435599e1fe03292adfb25c346d) )
JAGUAR_ROM_LOAD( wolfn3d,   "wolfenstein 3d (world).j64",                                                                0x000000 , 0x200000,	 CRC(e91bd644) SHA1(ee553176f0a32683b517b84b12c6fae13c15c3d0) )
JAGUAR_ROM_LOAD( worms,     "worms (world).j64",                                                                         0x000000 , 0x200000,	 CRC(6eb774eb) SHA1(641b07f93e8df03ab8b9bf1e8dc56e2889f247f8) )
JAGUAR_ROM_LOAD( zero5,     "zero 5 (world).j64",                                                                        0x000000 , 0x200000,	 CRC(61c7eec0) SHA1(b68de2cd49cb70e92f2415837edfa7ab46770a78) )
JAGUAR_ROM_LOAD( zool2,     "zool 2 (world).j64",                                                                        0x000000 , 0x200000,	 CRC(8975f48b) SHA1(32ff3811f8f6df8a2bb56165c62227a3a480e55c) )
JAGUAR_ROM_LOAD( zoop,      "zoop! (world).j64",                                                                         0x000000 , 0x100000,	 CRC(c5562581) SHA1(52ec786f7536ab043dc24725af94068cad2b0ec3) )


/*

Are the following undumped?
* Battlesphere - by ScatoLOGIC, 2000
* Breakout 2000 - by Telegames, 1996 
* Protector - by Songbird Productions, 1999

*/

SOFTWARE_LIST_START( jaguar_cart )
	SOFTWARE( aircars,   0,        1997, "ICD", "Air Cars (World)", 0, 0 )
	SOFTWARE( avsp,      0,        1994, "Atari", "Alien vs Predator (World)", 0, 0 )
	SOFTWARE( atarikar,  0,        1995, "Atari", "Atari Karts (World)", 0, 0 )
	SOFTWARE( mutntpng,  0,        1996, "Atari", "Attack of the Mutant Penguins (World)", 0, 0 )
	SOFTWARE( battlesp,  0,        2002, "ScatoLOGIC", "Battle Sphere Gold (World)", 0, 0 )
	SOFTWARE( brutalsp,  0,        1994, "Telegames", "Brutal Sports Football (World)", 0, 0 )
	SOFTWARE( bubsy,     0,        1994, "Atari", "Bubsy in Fractured Furry Tales (World)", 0, 0 )
	SOFTWARE( cfodder,   0,        1995, "Virgin Interactive", "Cannon Fodder (World)", 0, 0 )
	SOFTWARE( chekflag,  0,        1994, "Atari", "Checkered Flag (World)", 0, 0 )
	SOFTWARE( clubdriv,  0,        1994, "Atari", "Club Drive (World)", 0, 0 )
	SOFTWARE( cybermor1, cybermor, 1993, "Atari", "Cybermorph (World, Rev. 1)", 0, 0 )
	SOFTWARE( cybermor,  0,        1993, "Atari", "Cybermorph (World, Rev. 2)", 0, 0 )
	SOFTWARE( defender,  0,        1996, "Atari", "Defender 2000 (World)", 0, 0 )
	SOFTWARE( doom,      0,        1994, "Atari", "Doom (World)", 0, 0 )
	SOFTWARE( ddragon5,  0,        1995, "Tradewest", "Double Dragon V - The Shadow Falls (World)", 0, 0 )
	SOFTWARE( dragon,    0,        1994, "Virgin Interactibe", "Dragon - The Bruce Lee Story (World)", 0, 0 )
	SOFTWARE( dinodude,  0,        1994, "Atari", "Evolution - Dino Dudes (World)", 0, 0 )
	SOFTWARE( feverpit,  0,        1995, "Atari", "Fever Pitch Soccer (World)", 0, 0 )
	SOFTWARE( fforlife1, fforlife, 1996, "Atari", "Fight for Life (World, Alt)", 0, 0 )
	SOFTWARE( fforlife,  0,        1996, "Atari", "Fight for Life (World)", 0, 0 )
	SOFTWARE( flashbck,  0,        1995, "U.S. Gold", "Flashback - The Quest for Identity (World)", 0, 0 )
	SOFTWARE( flipout,   0,        1995, "Atari", "Flip Out! (World)", 0, 0 )
	SOFTWARE( hstrike,   0,        1995, "Atari", "Hover Strike (World)", 0, 0 )
	SOFTWARE( hyperfor,  0,        2000, "Songboard Productions", "Hyper Force (World)", 0, 0 )
	SOFTWARE( iwar,      0,        1995, "Atari", "I-War (World)", 0, 0 )
	SOFTWARE( ironsold,  0,        1994, "Atari", "Iron Soldier (World, v1.04)", 0, 0 )
	SOFTWARE( ironsol2,  0,        1997, "Telegames", "Iron Soldier 2 (World)", 0, 0 )
	SOFTWARE( kasumi,    0,        1994, "Atari", "Kasumi Ninja (World)", 0, 0 )
	SOFTWARE( missilec,  0,        1995, "Atari", "Missile Command 3D (World)", 0, 0 )
	SOFTWARE( nbajamte,  0,        1996, "Atari", "NBA Jam T.E. - Tournament Edition (World)", 0, 0 )
	SOFTWARE( pinballf,  0,        1995, "21st Century Entertainment", "Pinball Fantasies (World)", 0, 0 )
	SOFTWARE( pitfall,   0,        1995, "Atari", "Pitfall - The Mayan Adventure (World)", 0, 0 )
	SOFTWARE( pdrive,    0,        1995, "Time Warner Interactive", "Power Drive Rally (World)", 0, 0 )
	SOFTWARE( protect,   0,        2002, "Songbird Productions", "Protector - Special Edition (World)", 0, 0 )
	SOFTWARE( raiden,    0,        1994, "Atari", "Raiden (World)", 0, 0 )
	SOFTWARE( rayman,    0,        1995, "Ubisoft", "Rayman (World)", 0, 0 )
	SOFTWARE( ruinerp,   0,        1995, "Atari", "Ruiner Pinball (World)", 0, 0 )
	SOFTWARE( sensible,  0,        199?, "Telegames", "Sensible Soccer - International Edition (World)", 0, 0 )
	SOFTWARE( skyhamm,   0,        2000, "Songbird Productions", "Skyhammer (World)", 0, 0 )
	SOFTWARE( soccerkd,  0,        2000, "Songbird Productions", "Soccer Kid (World)", 0, 0 )
	SOFTWARE( spacewar,  0,        2001, "B&C Computervisions", "Space War 2000 (World)", 0, 0 )
	SOFTWARE( sburnout,  0,        1995, "Atari", "Super Burnout (World)", 0, 0 )
	SOFTWARE( superx3d,  0,        1995, "Atari", "SuperCross 3D (World)", 0, 0 )
	SOFTWARE( syndicat,  0,        1995, "Ocean", "Syndicate (World)", 0, 0 )
	SOFTWARE( tempst2k,  0,        1994, "Atari", "Tempest 2000 (World)", 0, 0 )
	SOFTWARE( themeprk,  0,        1995, "Ocean", "Theme Park (World)", 0, 0 )
	SOFTWARE( totalcar,  0,        2005, "Songbird Productions", "Total Carnage (World)", 0, 0 )
	SOFTWARE( towers2,   0,        1996, "Telegames", "Towers II - Plight of the Stargazer (World)", 0, 0 )
	SOFTWARE( trevmcfr,  0,        1993, "Atari", "Trevor McFur in the Crescent Galaxy (World)", 0, 0 )
	SOFTWARE( troyaik,   0,        1995, "Williams Entertainment", "Troy Aikman NFL Football (World)", 0, 0 )
	SOFTWARE( ultravor1, ultravor, 1995, "Atari", "Ultra Vortek (World, v0.94 Prototype)", 0, 0 )
	SOFTWARE( ultravor,  0,        1995, "Atari", "Ultra Vortek (World)", 0, 0 )
	SOFTWARE( valdiser,  0,        1994, "Atari", "Val d'Isère Skiing and Snowboarding (World)", 0, 0 )
	SOFTWARE( whitemen,  0,        1995, "Atari", "White Men Can't Jump (World)", 0, 0 )
	SOFTWARE( wolfn3d,   0,        1994, "Atari", "Wolfenstein 3D (World)", 0, 0 )
	SOFTWARE( worms,     0,        1998, "Telegames", "Worms (World)", 0, 0 )
	SOFTWARE( zero5,     0,        1997, "Telegames", "Zero 5 (World)", 0, 0 )
	SOFTWARE( zool2,     0,        1994, "Atari", "Zool 2 (World)", 0, 0 )
	SOFTWARE( zoop,      0,        1996, "Atari", "Zoop! (World)", 0, 0 )
SOFTWARE_LIST_END

SOFTWARE_LIST( jaguar_cart, "Atari Jaguar cartridges" )
