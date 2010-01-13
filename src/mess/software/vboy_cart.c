/***************************************************************************

    Nintendo Virtual Boy cartridges

***************************************************************************/

#include "driver.h"
#include "softlist.h"
#include "devices/cartslot.h"


// to verify this!
#define VBOY_ROM_LOAD( set, name, offset, length, hash )	\
SOFTWARE_START( set ) \
	ROM_REGION( 0x200000, CARTRIDGE_REGION_ROM, 0 ) \
	ROM_LOAD(name, offset, length, hash) \
SOFTWARE_END


VBOY_ROM_LOAD( 3dtetris,  "3-d tetris (usa).bin",                                                                       0x000000,  0x100000,	 CRC(bb71b522) SHA1(5177015a91442e56bd76af39447bca365e06c272) )
VBOY_ROM_LOAD( galactic,  "galactic pinball (japan, usa).bin",                                                          0x000000,  0x100000,	 CRC(c9710a36) SHA1(93a25abf4f86e71a49c1c3a470140bb40cb693d6) )
VBOY_ROM_LOAD( golf,      "golf (usa).bin",                                                                             0x000000,  0x200000,	 CRC(2199af41) SHA1(23ce3c174789cdd306497d86cb2c4e76ba8b06e5) )
VBOY_ROM_LOAD( innsmout,  "innsmouth no yakata (japan).bin",                                                            0x000000,  0x100000,	 CRC(efd0ac36) SHA1(8bb91e681959207be068796d540120565c174d37) )
VBOY_ROM_LOAD( jackbros,  "jack bros. (usa).bin",                                                                       0x000000,  0x100000,	 CRC(a44de03c) SHA1(0e086d7ef2bd8b97315196943fd0b71da07aa8f1) )
VBOY_ROM_LOAD( jackbrosj, "jack bros. no meiro de hiihoo! (japan).bin",                                                 0x000000,  0x100000,	 CRC(cab61e8b) SHA1(a973406590382ee503037271330d68d2834b79db) )
VBOY_ROM_LOAD( mariocla,  "mario clash (japan, usa).bin",                                                               0x000000,  0x100000,	 CRC(a47de78c) SHA1(7556a778b60490bdb81774bcbaa7413fc84cb985) )
VBOY_ROM_LOAD( marioten,  "mario's tennis (japan, usa).bin",                                                            0x000000,  0x80000,	 CRC(7ce7460d) SHA1(5162f7fa9e3eae4338c53ccba641ba080c768754) )
VBOY_ROM_LOAD( nesterfb,  "nester's funky bowling (usa).bin",                                                           0x000000,  0x200000,	 CRC(df4d56b4) SHA1(f4a4c7928f15102cf14c90c5f044e5f7cc7c32f9) )
VBOY_ROM_LOAD( panicbom,  "panic bomber (usa).bin",                                                                     0x000000,  0x80000,	 CRC(19bb2dfb) SHA1(80216b2874cf162f301d571bb8aebc98d80b4f3e) )
VBOY_ROM_LOAD( redalarmj, "red alarm (japan).bin",                                                                      0x000000,  0x100000,	 CRC(7e85c45d) SHA1(f5057fa9bfd9d60b9dcfc004cfdd12aa8eb1cb4a) )
VBOY_ROM_LOAD( redalarm,  "red alarm (usa).bin",                                                                        0x000000,  0x100000,	 CRC(aa10a7b4) SHA1(494fa60cb8997880b4e8cd3270b21d6c2768a48a) )
VBOY_ROM_LOAD( sdgundam,  "sd gundam - dimension war (japan).bin",                                                      0x000000,  0x100000,	 CRC(44788197) SHA1(1a67f6bdb45db46e22b8aadea754a68ad379ae29) )
VBOY_ROM_LOAD( spaceinv,  "space invaders - virtual collection (japan).bin",                                            0x000000,  0x80000,	 CRC(fa44402d) SHA1(16ad904faafd24642dfd056d10a768e81f0d9bfa) )
VBOY_ROM_LOAD( ssquash,   "space squash (japan).bin",                                                                   0x000000,  0x80000,	 CRC(60895693) SHA1(3f08cae2b6f9e386529949ff954220860db1af64) )
VBOY_ROM_LOAD( vgolf,     "t&e virtual golf (japan).bin",                                                               0x000000,  0x200000,	 CRC(6ba07915) SHA1(c595285d42c69f14b2b418c1edfbe4a7f9a624b6) )
VBOY_ROM_LOAD( telerobo,  "teleroboxer (japan, usa).bin",                                                               0x000000,  0x100000,	 CRC(36103000) SHA1(c59e020f9674774c5cbc022317ebba0eb1d744f7) )
VBOY_ROM_LOAD( tobidase,  "tobidase! panibon (japan).bin",                                                              0x000000,  0x80000,	 CRC(40498f5e) SHA1(b8a12a9677afa5cbce6ed43eb0effb5a02875534) )
VBOY_ROM_LOAD( vtetris,   "v-tetris (japan).bin",                                                                       0x000000,  0x80000,	 CRC(3ccb67ae) SHA1(ff7d4dfc467e6e0d3fe8517102132a65a9d17589) )
VBOY_ROM_LOAD( vforcej,   "vertical force (japan).bin",                                                                 0x000000,  0x100000,	 CRC(9e9b8b92) SHA1(c7204ba5cfe7d26394b5e22badc580c8ed8c0b37) )
VBOY_ROM_LOAD( vforce,    "vertical force (usa).bin",                                                                   0x000000,  0x100000,	 CRC(4c32ba5e) SHA1(38f9008f60b09deef1151d46b905b090a0338200) )
VBOY_ROM_LOAD( vbowling,  "virtual bowling (japan).bin",                                                                0x000000,  0x100000,	 CRC(20688279) SHA1(a5be7654037050f0a781e70efea0191f43d26f06) )
VBOY_ROM_LOAD( wariolnd,  "virtual boy wario land (japan, usa).bin",                                                    0x000000,  0x200000,	 CRC(133e9372) SHA1(274c328fbd904f20e69172ab826bf8f94ced1bdb) )
VBOY_ROM_LOAD( vfishing,  "virtual fishing (japan).bin",                                                                0x000000,  0x100000,	 CRC(526cc969) SHA1(583b409b7215159219d08e789db46140062095f2) )
VBOY_ROM_LOAD( vlab,      "virtual lab (japan).bin",                                                                    0x000000,  0x100000,	 CRC(8989fe0a) SHA1(d96c9f8aac5b4ea012a8fe659bb68fb1159a9c6d) )
VBOY_ROM_LOAD( vleague,   "virtual league baseball (usa).bin",                                                          0x000000,  0x100000,	 CRC(736b40d6) SHA1(266fe615ee3df1160a20b456824699f16810fa28) )
VBOY_ROM_LOAD( vproyak,   "virtual pro yakyuu '95 (japan).bin",                                                         0x000000,  0x100000,	 CRC(9ba8bb5e) SHA1(ab8fa82b79512eefefccdccea6768078a374c4aa) )
VBOY_ROM_LOAD( waterwld,  "waterworld (usa).bin",                                                                       0x000000,  0x200000,	 CRC(82a95e51) SHA1(dcc46484bd0acab0ac1ea178f425a0f5ccfb8dc2) )

SOFTWARE_LIST_START( vboy_cart )
	SOFTWARE( 3dtetris,  0,        19??, "<unknown>", "3-D Tetris (USA)", 0, 0 )
	SOFTWARE( galactic,  0,        19??, "<unknown>", "Galactic Pinball (Japan, USA)", 0, 0 )
	SOFTWARE( golf,      0,        19??, "<unknown>", "Golf (USA)", 0, 0 )
	SOFTWARE( innsmout,  0,        19??, "<unknown>", "Innsmouth no Yakata (Japan)", 0, 0 )
	SOFTWARE( jackbros,  0,        19??, "<unknown>", "Jack Bros. (USA)", 0, 0 )
	SOFTWARE( jackbrosj, jackbros, 19??, "<unknown>", "Jack Bros. no Meiro de Hiihoo! (Japan)", 0, 0 )
	SOFTWARE( mariocla,  0,        19??, "<unknown>", "Mario Clash (Japan, USA)", 0, 0 )
	SOFTWARE( marioten,  0,        19??, "<unknown>", "Mario's Tennis (Japan, USA)", 0, 0 )
	SOFTWARE( nesterfb,  0,        19??, "<unknown>", "Nester's Funky Bowling (USA)", 0, 0 )
	SOFTWARE( panicbom,  0,        19??, "<unknown>", "Panic Bomber (USA)", 0, 0 )
	SOFTWARE( redalarmj, redalarm, 19??, "<unknown>", "Red Alarm (Japan)", 0, 0 )
	SOFTWARE( redalarm,  0,        19??, "<unknown>", "Red Alarm (USA)", 0, 0 )
	SOFTWARE( sdgundam,  0,        19??, "<unknown>", "SD Gundam - Dimension War (Japan)", 0, 0 )
	SOFTWARE( spaceinv,  0,        19??, "<unknown>", "Space Invaders - Virtual Collection (Japan)", 0, 0 )
	SOFTWARE( ssquash,   0,        19??, "<unknown>", "Space Squash (Japan)", 0, 0 )
	SOFTWARE( vgolf,     0,        19??, "<unknown>", "T&E Virtual Golf (Japan)", 0, 0 )
	SOFTWARE( telerobo,  0,        19??, "<unknown>", "Teleroboxer (Japan, USA)", 0, 0 )
	SOFTWARE( tobidase,  0,        19??, "<unknown>", "Tobidase! Panibon (Japan)", 0, 0 )
	SOFTWARE( vtetris,   0,        19??, "<unknown>", "V-Tetris (Japan)", 0, 0 )
	SOFTWARE( vforcej,   vforce,   19??, "<unknown>", "Vertical Force (Japan)", 0, 0 )
	SOFTWARE( vforce,    0,        19??, "<unknown>", "Vertical Force (USA)", 0, 0 )
	SOFTWARE( vbowling,  0,        19??, "<unknown>", "Virtual Bowling (Japan)", 0, 0 )
	SOFTWARE( wariolnd,  0,        19??, "<unknown>", "Virtual Boy Wario Land (Japan, USA)", 0, 0 )
	SOFTWARE( vfishing,  0,        19??, "<unknown>", "Virtual Fishing (Japan)", 0, 0 )
	SOFTWARE( vlab,      0,        19??, "<unknown>", "Virtual Lab (Japan)", 0, 0 )
	SOFTWARE( vleague,   0,        19??, "<unknown>", "Virtual League Baseball (USA)", 0, 0 )
	SOFTWARE( vproyak,   0,        19??, "<unknown>", "Virtual Pro Yakyuu '95 (Japan)", 0, 0 )
	SOFTWARE( waterwld,  0,        19??, "<unknown>", "Waterworld (USA)", 0, 0 )
SOFTWARE_LIST_END

SOFTWARE_LIST( vboy_cart, "Nintendo Virtual Boy cartridges" )
