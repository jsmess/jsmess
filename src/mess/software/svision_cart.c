/***************************************************************************

    Watara Supervision cartridges

***************************************************************************/

#include "emu.h"
#include "softlist.h"
#include "devices/cartslot.h"


#define SVISION_ROM_LOAD( set, name, offset, length, hash )	\
SOFTWARE_START( set ) \
	ROM_REGION(0x20000, CARTRIDGE_REGION_ROM, 0 ) \
	ROM_LOAD(name, offset, length, hash) \
SOFTWARE_END


SVISION_ROM_LOAD( 2in1,     "2 in 1 - block buster + cross high (usa, europe).bin",                                       0x000000,  0x10000,	 CRC(7e05d84f) SHA1(f2b7902b8c522f63858c99caa81d63f4cdcb60cb) )
SVISION_ROM_LOAD( 2in1a,    "2 in 1 - hash blocks + eagle plan (usa, europe).bin",                                        0x000000,  0x10000,	 CRC(c1354952) SHA1(bca1c05ebac50a1acd4a8d3b440a29867edc9826) )
SVISION_ROM_LOAD( alien,    "alien (usa, europe).bin",                                                                    0x000000,  0x10000,	 CRC(8dbb2c53) SHA1(e20ae19d0555dc8d34da8f16d2d7bcce8c9856da) )
SVISION_ROM_LOAD( balloonf, "balloon fight (usa, europe).bin",                                                            0x000000,  0x10000,	 CRC(8188b755) SHA1(2d58bb254bb79f47d64f1505336bf736c7b5a4d8) )
SVISION_ROM_LOAD( blockbus, "block buster (usa, europe).bin",                                                             0x000000,  0x10000,	 CRC(0a3db285) SHA1(639c69afa4a68236dfea263b51e9c032c1aa71be) )
SVISION_ROM_LOAD( brainpow, "brain power (usa, europe).bin",                                                              0x000000,  0x10000,	 CRC(5e6706b9) SHA1(8a97572caa6cd91950bd333bcab5c61d60cf28d2) )
SVISION_ROM_LOAD( carrier1, "carrier (199x) (travellmate) [o1].bin",                                                      0x000000,  0x10000,	 CRC(8847c439) SHA1(fff0e56e3b9bb7799a8d2f1b9562aa86dc114502) )
SVISION_ROM_LOAD( carrier,  "carrier (usa, europe).bin",                                                                  0x000000,  0x8000,	 CRC(5ecfb674) SHA1(13061b97c7b6d74990b766670f59dbf1e7598120) )
SVISION_ROM_LOAD( cavewond, "cave wonders (usa, europe).bin",                                                             0x000000,  0x10000,	 CRC(e0266ce7) SHA1(af1232c2e781ac2ae12163dc200815a8579c269d) )
SVISION_ROM_LOAD( challtnk, "challenger tank (usa, europe).bin",                                                          0x000000,  0x10000,	 CRC(c92382ce) SHA1(70f7589fdf40c851cc34c0a5f644216f3765eb8c) )
SVISION_ROM_LOAD( chimera,  "chimera (usa, europe).bin",                                                                  0x000000,  0x10000,	 CRC(4a458aa8) SHA1(c384062ce1284a4c80e8fbb1ad0e8fa924c6881b) )
SVISION_ROM_LOAD( crystbal, "crystball (usa, europe).bin",                                                                0x000000,  0x10000,	 CRC(10dcc110) SHA1(f23f53f7486857db36a959a0ecc71b6b688c7af1) )
SVISION_ROM_LOAD( dhero,    "delta hero (usa, europe).bin",                                                               0x000000,  0x10000,	 CRC(32ccdf89) SHA1(ee68c188c41cf8f12f74458d6dd614afc7b8edbb) )
SVISION_ROM_LOAD( eaglplan, "eagle plan (usa, europe).bin",                                                               0x000000,  0x10000,	 CRC(01cb8364) SHA1(226e8f14498be513b73f97b5e618a524209c4595) )
SVISION_ROM_LOAD( earthdef, "earth defender (usa, europe).bin",                                                           0x000000,  0x10000,	 CRC(a6cbb074) SHA1(8b0550dde96661b9b7317a1d247174ecca813698) )
SVISION_ROM_LOAD( fatcraft, "fatal craft (usa, europe).bin",                                                              0x000000,  0x10000,	 CRC(917cab48) SHA1(9c6282caa6b06c63f53910fdb11b7d0f83d29310) )
SVISION_ROM_LOAD( fcombat,  "final combat (usa, europe).bin",                                                             0x000000,  0x8000,	 CRC(465d78cd) SHA1(ef87ef536bd82c1a9b707ffcb173beef74953cd5) )
SVISION_ROM_LOAD( galactic, "galactic crusader (usa, europe).bin",                                                        0x000000,  0x10000,	 CRC(b494bc5c) SHA1(7739826471d4ac2a69d34bdbd154bb6d748e90cc) )
SVISION_ROM_LOAD( galaxy,   "galaxy fighter (usa, europe).bin",                                                           0x000000,  0x10000,	 CRC(581703be) SHA1(721350e7c7761b9be13d2a236ebfc72e51755851) )
SVISION_ROM_LOAD( gp,       "grand prix (usa, europe).bin",                                                               0x000000,  0x10000,	 CRC(cef3f295) SHA1(a6b93e0bbb0d037a0fe4b0d8056b6808aea7ae6c) )
SVISION_ROM_LOAD( happyp,   "happy pairs (usa, europe).bin",                                                              0x000000,  0x10000,	 CRC(112f5eed) SHA1(d8a78bb3cb492a90464f7a16bb2486566f7d4baa) )
SVISION_ROM_LOAD( hashbloc, "hash blocks (usa, europe).bin",                                                              0x000000,  0x10000,	 CRC(6bd7c885) SHA1(d9836dfe4f113dca030b0472deb7a8d8bb9065dc) )
SVISION_ROM_LOAD( herokid,  "hero kid (usa, europe).bin",                                                                 0x000000,  0x10000,	 CRC(25ddd6e1) SHA1(3e5de3350e3bcf75e057b970d7e1c56634699bb1) )
SVISION_ROM_LOAD( honeybee, "honey bee (usa, europe).bin",                                                                0x000000,  0x10000,	 CRC(e856875a) SHA1(3412cb0d657c6c79eef71d29c6af06ac3082cb2f) )
SVISION_ROM_LOAD( jackyluc, "jacky lucky (usa, europe).bin",                                                              0x000000,  0x10000,	 CRC(7d8a607f) SHA1(f7311ccca24811aa3595114d50e5c1ac5772b4c9) )
SVISION_ROM_LOAD( jagbombr, "jaguar bomber (usa, europe).bin",                                                            0x000000,  0x10000,	 CRC(aa4372d4) SHA1(212f577b0d1df539aadfecdec4e85404e4d57201) )
SVISION_ROM_LOAD( johnadv,  "john adventure (usa, europe).bin",                                                           0x000000,  0x10000,	 CRC(e9ea3ae0) SHA1(b422814b073544b66f2ad86c76bae6403f7a4530) )
SVISION_ROM_LOAD( juggler,  "juggler (usa, europe).bin",                                                                  0x000000,  0x10000,	 CRC(32e68d6f) SHA1(a5c8beecff5444183dfed9e46dd67150c12c8bfd) )
SVISION_ROM_LOAD( kabiisl,  "kabi island - gold in island (usa, europe).bin",                                             0x000000,  0x10000,	 CRC(2bc03096) SHA1(517154e5340bf221392a99640733b0798c7490ee) )
SVISION_ROM_LOAD( linear,   "linear racing (usa, europe).bin",                                                            0x000000,  0x10000,	 CRC(6f8abaf9) SHA1(9f80a72d967d2a35168250ca3da3941164164bfc) )
SVISION_ROM_LOAD( maginx,   "magincross (usa, europe).bin",                                                               0x000000,  0x10000,	 CRC(e406a91c) SHA1(b7aec7d24175bdea1cfccc1359b5ab353e023d44) )
SVISION_ROM_LOAD( mattablt, "matta blatta (usa, europe).bin",                                                             0x000000,  0x10000,	 CRC(21864295) SHA1(fb6af626a6496907d02a26b9bb5132f4be88bda9) )
SVISION_ROM_LOAD( olympict, "olympic trials (usa, europe).bin",                                                           0x000000,  0x10000,	 CRC(85dc8111) SHA1(4b863616bb9b821adf397f20e0ea043a3daa026a) )
SVISION_ROM_LOAD( p52sea,   "p-52 sea battle (usa, europe).bin",                                                          0x000000,  0x10000,	 CRC(748f9dae) SHA1(794ce6fb7084274c680b277a41f269840402e61b) )
SVISION_ROM_LOAD( pacboy,   "pacboy & mouse (usa, europe).bin",                                                           0x000000,  0x10000,	 CRC(1a89ac88) SHA1(afa267bcced763cb51599cd5605062530ede1af3) )
SVISION_ROM_LOAD( penguin1, "penguin hideout (1992) (thin chen enterprise) [o1].bin",                                     0x000000,  0x10000,	 CRC(17a4ca40) SHA1(ed532dd29212754d8c96539da04452b56e89d17b) )
SVISION_ROM_LOAD( penguinh, "penguin hideout (usa, europe).bin",                                                          0x000000,  0x8000,	 CRC(fe5f9774) SHA1(9520221d8cfd3bdfb0fb6b7fb9fed66ba027a756) )
SVISION_ROM_LOAD( policebu, "police bust (usa, europe).bin",                                                              0x000000,  0x10000,	 CRC(531f0b51) SHA1(9fa06b4d171d9ade2f9e4a639e80feba15c963f9) )
SVISION_ROM_LOAD( pyramid,  "pyramid (usa, europe).bin",                                                                  0x000000,  0x8000,	 CRC(e0bfe163) SHA1(8fab2b0d3e4485e156c78159b92fd70aabc02994) )
SVISION_ROM_LOAD( soccerch, "soccer champion (usa, europe).bin",                                                          0x000000,  0x10000,	 CRC(45204dc4) SHA1(2b794e22f7d595902c515b3fd617980655a9d994) )
SVISION_ROM_LOAD( sssnake,  "sssnake (usa, europe).bin",                                                                  0x000000,  0x10000,	 CRC(be9b6f10) SHA1(da02d8e119fbd38048f16c78ce9714abf93bbfd6) )
SVISION_ROM_LOAD( sblock1,  "super block (1992) (bon treasure) [o1].bin",                                                 0x000000,  0x10000,	 CRC(d46ab5e0) SHA1(e16c13f1fe3a5bde2fa3a4d4c10d28514328b7c8) )
SVISION_ROM_LOAD( sblock,   "super block (usa, europe).bin",                                                              0x000000,  0x8000,	 CRC(02e2c7ad) SHA1(1fa3236ad83984d922893c294ed0d9e432201fb0) )
SVISION_ROM_LOAD( skong,    "super kong (usa, europe).bin",                                                               0x000000,  0x10000,	 CRC(59c7ff64) SHA1(6c59bc0d14a05b404292de8aba3a7da12d72f355) )
SVISION_ROM_LOAD( tasac,    "tasac 2010 (usa, europe).bin",                                                               0x000000,  0x10000,	 CRC(3d5f3964) SHA1(da884d97fbe7e7aca40dc1b5b0db6d7ff69377e3) )
SVISION_ROM_LOAD( tennis92, "tennis pro '92 (usa, europe).bin",                                                           0x000000,  0x10000,	 CRC(bd004cb7) SHA1(386f555fcedaaf9fc4d81ddf59ec0320f42af988) )
SVISION_ROM_LOAD( treasure, "treasure hunter (usa, europe).bin",                                                          0x000000,  0x10000,	 CRC(db35b809) SHA1(baf24b026023f44968fd1352637f1c01cbb89f3e) )


SOFTWARE_LIST_START( svision_cart )
	SOFTWARE( 2in1,     0,        19??, "<unknown>", "2 in 1 - Block Buster + Cross High (Euro, USA)", 0, 0 )
	SOFTWARE( 2in1a,    0,        19??, "<unknown>", "2 in 1 - Hash Blocks + Eagle Plan (Euro, USA)", 0, 0 )
	SOFTWARE( alien,    0,        19??, "<unknown>", "Alien (Euro, USA)", 0, 0 )
	SOFTWARE( balloonf, 0,        19??, "<unknown>", "Balloon Fight (Euro, USA)", 0, 0 )
	SOFTWARE( blockbus, 0,        19??, "<unknown>", "Block Buster (Euro, USA)", 0, 0 )
	SOFTWARE( brainpow, 0,        19??, "<unknown>", "Brain Power (Euro, USA)", 0, 0 )
	SOFTWARE( carrier1, carrier,  19??, "<unknown>", "Carrier (199x) (Travellmate) [o1].bin", 0, 0 )
	SOFTWARE( carrier,  0,        19??, "<unknown>", "Carrier (Euro, USA)", 0, 0 )
	SOFTWARE( cavewond, 0,        19??, "<unknown>", "Cave Wonders (Euro, USA)", 0, 0 )
	SOFTWARE( challtnk, 0,        19??, "<unknown>", "Challenger Tank (Euro, USA)", 0, 0 )
	SOFTWARE( chimera,  0,        19??, "<unknown>", "Chimera (Euro, USA)", 0, 0 )
	SOFTWARE( crystbal, 0,        19??, "<unknown>", "Crystball (Euro, USA)", 0, 0 )
	SOFTWARE( dhero,    0,        19??, "<unknown>", "Delta Hero (Euro, USA)", 0, 0 )
	SOFTWARE( eaglplan, 0,        19??, "<unknown>", "Eagle Plan (Euro, USA)", 0, 0 )
	SOFTWARE( earthdef, 0,        19??, "<unknown>", "Earth Defender (Euro, USA)", 0, 0 )
	SOFTWARE( fatcraft, 0,        19??, "<unknown>", "Fatal Craft (Euro, USA)", 0, 0 )
	SOFTWARE( fcombat,  0,        19??, "<unknown>", "Final Combat (Euro, USA)", 0, 0 )
	SOFTWARE( galactic, 0,        19??, "<unknown>", "Galactic Crusader (Euro, USA)", 0, 0 )
	SOFTWARE( galaxy,   0,        19??, "<unknown>", "Galaxy Fighter (Euro, USA)", 0, 0 )
	SOFTWARE( gp,       0,        19??, "<unknown>", "Grand Prix (Euro, USA)", 0, 0 )
	SOFTWARE( happyp,   0,        19??, "<unknown>", "Happy Pairs (Euro, USA)", 0, 0 )
	SOFTWARE( hashbloc, 0,        19??, "<unknown>", "Hash Blocks (Euro, USA)", 0, 0 )
	SOFTWARE( herokid,  0,        19??, "<unknown>", "Hero Kid (Euro, USA)", 0, 0 )
	SOFTWARE( honeybee, 0,        19??, "<unknown>", "Honey Bee (Euro, USA)", 0, 0 )
	SOFTWARE( jackyluc, 0,        19??, "<unknown>", "Jacky Lucky (Euro, USA)", 0, 0 )
	SOFTWARE( jagbombr, 0,        19??, "<unknown>", "Jaguar Bomber (Euro, USA)", 0, 0 )
	SOFTWARE( johnadv,  0,        19??, "<unknown>", "John Adventure (Euro, USA)", 0, 0 )
	SOFTWARE( juggler,  0,        19??, "<unknown>", "Juggler (Euro, USA)", 0, 0 )
	SOFTWARE( kabiisl,  0,        19??, "<unknown>", "Kabi Island - Gold in Island (Euro, USA)", 0, 0 )
	SOFTWARE( linear,   0,        19??, "<unknown>", "Linear Racing (Euro, USA)", 0, 0 )
	SOFTWARE( maginx,   0,        19??, "<unknown>", "Magincross (Euro, USA)", 0, 0 )
	SOFTWARE( mattablt, 0,        19??, "<unknown>", "Matta Blatta (Euro, USA)", 0, 0 )
	SOFTWARE( olympict, 0,        19??, "<unknown>", "Olympic Trials (Euro, USA)", 0, 0 )
	SOFTWARE( p52sea,   0,        19??, "<unknown>", "P-52 Sea Battle (Euro, USA)", 0, 0 )
	SOFTWARE( pacboy,   0,        19??, "<unknown>", "PacBoy & Mouse (Euro, USA)", 0, 0 )
	SOFTWARE( penguin1, penguinh, 19??, "<unknown>", "Penguin Hideout (1992) (Thin Chen Enterprise) [o1].bin", 0, 0 )
	SOFTWARE( penguinh, 0,        19??, "<unknown>", "Penguin Hideout (Euro, USA)", 0, 0 )
	SOFTWARE( policebu, 0,        19??, "<unknown>", "Police Bust (Euro, USA)", 0, 0 )
	SOFTWARE( pyramid,  0,        19??, "<unknown>", "Pyramid (Euro, USA)", 0, 0 )
	SOFTWARE( soccerch, 0,        19??, "<unknown>", "Soccer Champion (Euro, USA)", 0, 0 )
	SOFTWARE( sssnake,  0,        19??, "<unknown>", "SSSnake (Euro, USA)", 0, 0 )
	SOFTWARE( sblock1,  sblock,   19??, "<unknown>", "Super Block (1992) (Bon Treasure) [o1].bin", 0, 0 )
	SOFTWARE( sblock,   0,        19??, "<unknown>", "Super Block (Euro, USA)", 0, 0 )
	SOFTWARE( skong,    0,        19??, "<unknown>", "Super Kong (Euro, USA)", 0, 0 )
	SOFTWARE( tasac,    0,        19??, "<unknown>", "Tasac 2010 (Euro, USA)", 0, 0 )
	SOFTWARE( tennis92, 0,        19??, "<unknown>", "Tennis Pro '92 (Euro, USA)", 0, 0 )
	SOFTWARE( treasure, 0,        19??, "<unknown>", "Treasure Hunter (Euro, USA)", 0, 0 )
SOFTWARE_LIST_END


SOFTWARE_LIST( svision_cart, "Watara Supervision cartridges" )
