/***************************************************************************

    Nintendo Pok?mon Mini cartridges

***************************************************************************/

#include "emu.h"
#include "softlist.h"
#include "devices/cartslot.h"


/* FIXME: at load the first 0x2100 should be skipped! */
#define POKEMINI_ROM_LOAD( set, name, offset, length, hash )	\
SOFTWARE_START( set ) \
	ROM_REGION( 0x200000, CARTRIDGE_REGION_ROM, 0 ) \
	ROM_LOAD(name, offset, length, hash) \
SOFTWARE_END


POKEMINI_ROM_LOAD( lunchtme,  "lunch time (usa) (gamecube).bin",                                                           0x000000,  0x80000,	 CRC(05c5c126) SHA1(3511d1bf4a3570a4011ca9d7d9d77ea43cc7543d) )
POKEMINI_ROM_LOAD( pichubro,  "pichu bros. mini (japan).bin",                                                              0x000000,  0x80000,	 CRC(00d8c968) SHA1(6f9bded4842b451102083d181db9deac2a7696e4) )
POKEMINI_ROM_LOAD( pichuhop,  "pichu bros. mini - hoppip's jump match (japan) (preview).bin",                              0x000000,  0x80000,	 CRC(be22866a) SHA1(1c928c1e989f37751536494e1e843ed01c7f2c93) )
POKEMINI_ROM_LOAD( pichunts,  "pichu bros. mini - netsukikyuu (japan) (preview).bin",                                      0x000000,  0x80000,	 CRC(a22d00a7) SHA1(041b6bea48d316eb63efcf930a9bda0dd3bd6618) )
POKEMINI_ROM_LOAD( pichuskt,  "pichu bros. mini - skateboard (japan) (preview).bin",                                       0x000000,  0x80000,	 CRC(dec65262) SHA1(b36ed9e3c86d37b128bcefbeb6088c4c66f11c52) )
POKEMINI_ROM_LOAD( pokeacrd,  "pokemon anime card daisakusen (japan).bin",                                                 0x000000,  0x80000,	 CRC(199d7ab1) SHA1(2c3aee0d0db8b4a80083732b8db07b0cc72608a8) )
POKEMINI_ROM_LOAD( pparty,    "pokemon party mini (europe).bin",                                                           0x000000,  0x80000,	 CRC(54acb670) SHA1(58c3210187497cffe989debcc095cb99c465e970) )
POKEMINI_ROM_LOAD( ppartyu,   "pokemon party mini (usa).bin",                                                              0x000000,  0x80000,	 CRC(938d3819) SHA1(ac94ed80d6f857e9530d17cc77c43aa809bf0415) )
POKEMINI_ROM_LOAD( pptbasej,  "pokemon party mini - baseline judge (europe) (gamecube).bin",                               0x000000,  0x80000,	 CRC(0e1e150b) SHA1(a309e00b2a8d7290070f27ec2fe10cfaba048283) )
POKEMINI_ROM_LOAD( pptbasejj, "pokemon party mini - baseline judge (japan) (gamecube).bin",                                0x000000,  0x80000,	 CRC(f5192440) SHA1(bdad2fc86506f18c16d2bd249cc1ad4ebf6113dd) )
POKEMINI_ROM_LOAD( pptbaseju, "pokemon party mini - baseline judge (usa) (gamecube).bin",                                  0x000000,  0x80000,	 CRC(0cfb4d1c) SHA1(20004ffc01eeb04e72402e70fc97170111aafa6a) )
POKEMINI_ROM_LOAD( pptdribl,  "pokemon party mini - chansey's dribble (europe) (gamecube).bin",                            0x000000,  0x80000,	 CRC(ba504497) SHA1(c9a5f335c93e9bacf198b976d430cf798bc517bc) )
POKEMINI_ROM_LOAD( pptrocks,  "pokemon party mini - pikachu's rocket start (europe) (gamecube).bin",                       0x000000,  0x80000,	 CRC(f15964af) SHA1(b1922f3d43f3a4e36876c3e0e8428809fd999efc) )
POKEMINI_ROM_LOAD( pptdriblj, "pokemon party mini - ricochet dribble (japan) (gamecube).bin",                              0x000000,  0x80000,	 CRC(d09db45c) SHA1(88b60902f9cf779ebcba1caee7c8477fc2e22598) )
POKEMINI_ROM_LOAD( pptdriblu, "pokemon party mini - ricochet dribble (usa) (gamecube).bin",                                0x000000,  0x80000,	 CRC(3322ed38) SHA1(ba1db1745933a941479d885fe0d1b52365b7f817) )
POKEMINI_ROM_LOAD( pptrocksu, "pokemon party mini - rocket start (usa) (gamecube).bin",                                    0x000000,  0x80000,	 CRC(44c5271e) SHA1(840d43fe55e8e056940ff0b7bc6d7e4e11b5e047) )
POKEMINI_ROM_LOAD( pptslowj,  "pokemon party mini - slowking's judge (europe) (gamecube).bin",                             0x000000,  0x80000,	 CRC(9fd4d48b) SHA1(e3bd506b97ea94e9291c2fa4e1395e1829bafeaf) )
POKEMINI_ROM_LOAD( ppinbjp,   "pokemon pinball mini (japan) (preview).bin",                                                0x000000,  0x80000,	 CRC(88ca8cc5) SHA1(a4209afea584757055aa6d4a77e58f943d2d6c6d) )
POKEMINI_ROM_LOAD( ppinbj,    "pokemon pinball mini (japan).bin",                                                          0x000000,  0x80000,	 CRC(398d47de) SHA1(57c78731f17f29d9cddfc7e21b4d8801e8569ca6) )
POKEMINI_ROM_LOAD( ppinbp,    "pokemon pinball mini (usa) (gamecube preview).bin",                                         0x000000,  0x80000,	 CRC(50ad9d57) SHA1(6704e372e20f137a4b46e2ab2d5c1231900ce1d3) )
POKEMINI_ROM_LOAD( ppinb,     "pokemon pinball mini (usa).bin",                                                            0x000000,  0x80000,	 CRC(a0091534) SHA1(3ec1e04de413c22e9034074789ff2c45646ec0ac) )
POKEMINI_ROM_LOAD( ppuzzlcfp, "pokemon puzzle collection (france) (gamecube preview).bin",                                 0x000000,  0x80000,	 CRC(7776f2bb) SHA1(d748f54bb50622858ae65cba74ba0cc82415fcbc) )
POKEMINI_ROM_LOAD( ppuzzlcgp, "pokemon puzzle collection (germany) (gamecube preview).bin",                                0x000000,  0x80000,	 CRC(0e7752e8) SHA1(7dc61e11b482b873d58657c827e3b166b3882db2) )
POKEMINI_ROM_LOAD( ppuzzlcg,  "pokemon puzzle collection (germany).bin",                                                   0x000000,  0x80000,	 CRC(22101046) SHA1(7c83682f595dc6eafc9c438d92eba7920ff3b2ca) )
POKEMINI_ROM_LOAD( ppuzzlcj1, "pokemon puzzle collection (japan) (gamecube).bin",                                          0x000000,  0x80000,	 CRC(da278854) SHA1(1ce915f0c03bc0ec2e505669336e6b6965f7476c) )
POKEMINI_ROM_LOAD( ppuzzlcj,  "pokemon puzzle collection (japan).bin",                                                     0x000000,  0x80000,	 CRC(95afae59) SHA1(2da4d24ed3192a88ab0b71ade7aec387987c5000) )
POKEMINI_ROM_LOAD( ppuzzlcp,  "pokemon puzzle collection (usa) (gamecube preview).bin",                                    0x000000,  0x80000,	 CRC(6f8ae656) SHA1(048bb1d30d6ff21be14dd123c52a375e12611238) )
POKEMINI_ROM_LOAD( ppuzzlc,   "pokemon puzzle collection (usa).bin",                                                       0x000000,  0x80000,	 CRC(2b348340) SHA1(9c5fa07cc9f93874b297f759a6c6fd98f6ec6f47) )
POKEMINI_ROM_LOAD( ppuzzlc2,  "pokemon puzzle collection vol. 2 (japan).bin",                                              0x000000,  0x80000,	 CRC(76a1bbf8) SHA1(11775bbac2e985ef90d4192ed6322dd3b8352ac9) )
POKEMINI_ROM_LOAD( pokeracep, "pokemon race mini (japan) (preview).bin",                                                   0x000000,  0x80000,	 CRC(8f731830) SHA1(6b8dbaa68ede37e64a22ee4b1cbb722f038cf5b5) )
POKEMINI_ROM_LOAD( pokerace,  "pokemon race mini (japan).bin",                                                             0x000000,  0x80000,	 CRC(f8c842b5) SHA1(12bfbd2c8a810b49338148feb6162e29a540ac3a) )
POKEMINI_ROM_LOAD( ptetrisj,  "pokemon shock tetris (japan).bin",                                                          0x000000,  0x80000,	 CRC(b4b5cc20) SHA1(e81c3576b0c96e6ca28b958f8dda11cea5fce078) )
POKEMINI_ROM_LOAD( pokesoda,  "pokemon sodateyasan mini (japan).bin",                                                      0x000000,  0x80000,	 CRC(d55fc4c8) SHA1(3b87d4d61ba3db92190b6ffa055f463adea2363c) )
POKEMINI_ROM_LOAD( ptetris,   "pokemon tetris (europe) (en,ja,fr).bin",                                                    0x000000,  0x80000,	 CRC(2fb23527) SHA1(5fd1198a4725e21629edd4869535b70edfe165d4) )
POKEMINI_ROM_LOAD( pokezcrd,  "pokemon zany cards (france).bin",                                                           0x000000,  0x80000,	 CRC(830da957) SHA1(7a8b3f6a3751cd39431d822fa3a8016ffd2d26d8) )
POKEMINI_ROM_LOAD( pokezcrdg, "pokemon zany cards (germany).bin",                                                          0x000000,  0x80000,	 CRC(c009d501) SHA1(0922e0147ff097e56d5ec98bf66ef050539329fa) )
POKEMINI_ROM_LOAD( snorlax,   "snorlax's lunch time (europe) (gamecube).bin",                                              0x000000,  0x80000,	 CRC(b9324f30) SHA1(22cc052d900909a9032b7c84a0c7ab7c366a4a57) )
POKEMINI_ROM_LOAD( snorlaxj,  "snorlax's lunch time (japan) (gamecube).bin",                                               0x000000,  0x80000,	 CRC(0186ea70) SHA1(a332feeca96c2b4cad2e1abd2d9b289804101a4a) )
POKEMINI_ROM_LOAD( togepip,   "togepi no daibouken (japan) (preview).bin",                                                 0x000000,  0x80000,	 CRC(8c557c89) SHA1(db7d8209594c6e8edee5932f799f36ecf1cfac9f) )
POKEMINI_ROM_LOAD( togepi,    "togepi no daibouken (japan).bin",                                                           0x000000,  0x80000,	 CRC(33b3492b) SHA1(0f56e0a3e72180b1113b900a6d3e4f8e8f570bb3) )

SOFTWARE_LIST_START( pokemini_cart )
	SOFTWARE( lunchtme,  snorlax,  2003, "Nintendo", "Lunch Time (USA, GameCube)", 0, 0 )
	SOFTWARE( pichubro,  0,        2002, "Nintendo", "Pichu Bros. Mini (Jpn)", 0, 0 )
	SOFTWARE( pichuhop,  0,        2002, "Nintendo", "Pichu Bros. Mini - Hoppip's Jump Match (Jpn, Preview)", 0, 0 )
	SOFTWARE( pichunts,  0,        2002, "Nintendo", "Pichu Bros. Mini - Netsukikyuu (Jpn, Preview)", 0, 0 )
	SOFTWARE( pichuskt,  0,        2002, "Nintendo", "Pichu Bros. Mini - Skateboard (Jpn, Preview)", 0, 0 )
	SOFTWARE( pokeacrd,  pokezcrd, 2001, "Nintendo", "Pok?mon Anime Card Daisakusen (Jpn)", 0, 0 )
	SOFTWARE( pparty,    0,        2001, "Nintendo", "Pok?mon Party Mini (Euro)", 0, 0 )
	SOFTWARE( ppartyu,   pparty,   2001, "Nintendo", "Pok?mon Party Mini (USA)", 0, 0 )
	SOFTWARE( pptbasej,  0,        2001, "Nintendo", "Pok?mon Party Mini - Baseline Judge (Euro, GameCube)", 0, 0 )
	SOFTWARE( pptbasejj, pptbasej, 2001, "Nintendo", "Pok?mon Party Mini - Baseline Judge (Jpn, GameCube)", 0, 0 )
	SOFTWARE( pptbaseju, pptbasej, 2001, "Nintendo", "Pok?mon Party Mini - Baseline Judge (USA, GameCube)", 0, 0 )
	SOFTWARE( pptdribl,  0,        2001, "Nintendo", "Pok?mon Party Mini - Chansey's Dribble (Euro, GameCube)", 0, 0 )
	SOFTWARE( pptrocks,  0,        2001, "Nintendo", "Pok?mon Party Mini - Pikachu's Rocket Start (Euro, GameCube)", 0, 0 )
	SOFTWARE( pptdriblj, pptdribl, 2001, "Nintendo", "Pok?mon Party Mini - Ricochet Dribble (Jpn, GameCube)", 0, 0 )
	SOFTWARE( pptdriblu, pptdribl, 2001, "Nintendo", "Pok?mon Party Mini - Ricochet Dribble (USA, GameCube)", 0, 0 )
	SOFTWARE( pptrocksu, pptrocks, 2001, "Nintendo", "Pok?mon Party Mini - Rocket Start (USA, GameCube)", 0, 0 )
	SOFTWARE( pptslowj,  0,        2001, "Nintendo", "Pok?mon Party Mini - Slowking's Judge (Euro, GameCube)", 0, 0 )
	SOFTWARE( ppinbjp,   ppinb,    2001, "Nintendo", "Pok?mon Pinball Mini (Jpn, Preview)", 0, 0 )
	SOFTWARE( ppinbj,    ppinb,    2001, "Nintendo", "Pok?mon Pinball Mini (Jpn)", 0, 0 )
	SOFTWARE( ppinbp,    ppinb,    2001, "Nintendo", "Pok?mon Pinball Mini (USA, GameCube Preview)", 0, 0 )
	SOFTWARE( ppinb,     0,        2001, "Nintendo", "Pok?mon Pinball Mini (USA)", 0, 0 )
	SOFTWARE( ppuzzlcfp, ppuzzlc,  2001, "Nintendo", "Pok?mon Puzzle Collection (France, GameCube Preview)", 0, 0 )
	SOFTWARE( ppuzzlcgp, ppuzzlc,  2001, "Nintendo", "Pok?mon Puzzle Collection (Germany, GameCube Preview)", 0, 0 )
	SOFTWARE( ppuzzlcg,  ppuzzlc,  2001, "Nintendo", "Pok?mon Puzzle Collection (Germany)", 0, 0 )
	SOFTWARE( ppuzzlcj1, ppuzzlc,  2001, "Nintendo", "Pok?mon Puzzle Collection (Jpn, GameCube)", 0, 0 )
	SOFTWARE( ppuzzlcj,  ppuzzlc,  2001, "Nintendo", "Pok?mon Puzzle Collection (Jpn)", 0, 0 )
	SOFTWARE( ppuzzlcp,  ppuzzlc,  2001, "Nintendo", "Pok?mon Puzzle Collection (USA, GameCube Preview)", 0, 0 )
	SOFTWARE( ppuzzlc,   0,        2001, "Nintendo", "Pok?mon Puzzle Collection (USA)", 0, 0 )
	SOFTWARE( ppuzzlc2,  0,        2002, "Nintendo", "Pok?mon Puzzle Collection Vol. 2 (Jpn)", 0, 0 )
	SOFTWARE( pokeracep, pokerace, 2002, "Nintendo", "Pok?mon Race Mini (Jpn, Preview)", 0, 0 )
	SOFTWARE( pokerace,  0,        2002, "Nintendo", "Pok?mon Race Mini (Jpn)", 0, 0 )
	SOFTWARE( ptetrisj,  ptetris,  2002, "Nintendo", "Pok?mon Shock Tetris (Jpn)", 0, 0 )
	SOFTWARE( pokesoda,  0,        2002, "Nintendo", "Pok?mon Sodateyasan Mini (Jpn)", 0, 0 )
	SOFTWARE( ptetris,   0,        2002, "Nintendo", "Pok?mon Tetris (Euro)", 0, 0 )
	SOFTWARE( pokezcrd,  0,        2001, "Nintendo", "Pok?mon Zany Cards (France)", 0, 0 )
	SOFTWARE( pokezcrdg, pokezcrd, 2001, "Nintendo", "Pok?mon Zany Cards (Germany)", 0, 0 )
	SOFTWARE( snorlax,   0,        2003, "Nintendo", "Snorlax's Lunch Time (Euro, GameCube)", 0, 0 )
	SOFTWARE( snorlaxj,  snorlax,  2003, "Nintendo", "Snorlax's Lunch Time (Jpn, GameCube)", 0, 0 )
	SOFTWARE( togepip,   togepi,   2002, "Nintendo", "Togepi no Daibouken (Jpn, Preview)", 0, 0 )
	SOFTWARE( togepi,    0,        2002, "Nintendo", "Togepi no Daibouken (Jpn)", 0, 0 )
SOFTWARE_LIST_END


SOFTWARE_LIST( pokemini_cart, "Nintendo Pok?mon Mini cartridges" )
