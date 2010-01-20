/***************************************************************************

    Nintendo Virtual Boy cartridges

***************************************************************************/

#include "emu.h"
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
	SOFTWARE( 3dtetris,  0,        1996, "Nintendo", "3-D Tetris (USA)", 0, 0 )                       /* Id: VUE-VPBE-USA - Developers: T&E Soft */
	SOFTWARE( galactic,  0,        1995, "Nintendo", "Galactic Pinball (Jpn, USA)", 0, 0 )            /* Id: VUE-VGPJ-JPN/USA - Developers: Intelligent Systems - Releases: 1995-07-01 (JPN) */
	SOFTWARE( golf,      0,        1995, "Nintendo", "Golf (USA)", 0, 0 )                             /* Id: VUE-VVGE-USA - Developers: T&E Soft */   
	SOFTWARE( innsmout,  0,        1995, "I'Max", "Innsmouth no Yakata (Jpn)", 0, 0 )                 /* Id: VUE-VIMJ-JPN - Developers: Betop (I'Max) - Releases: 1995-10-13 (JPN) */
	SOFTWARE( jackbros,  0,        1995, "Atlus", "Jack Bros. (USA)", 0, 0 )                          /* Id: VUE-VJBE-USA - Developers: Atlus  */
	SOFTWARE( jackbrosj, jackbros, 1995, "Atlus", "Jack Bros. no Meiro de Hiihoo! (Jpn)", 0, 0 )      /* Id: VUE-VJBJ-JPN - Developers: Atlus - Releases: 1995-09-29 (JPN) */
	SOFTWARE( mariocla,  0,        1995, "Nintendo", "Mario Clash (Jpn, USA)", 0, 0 )                 /* Id: VUE-VMCJ-JPN/USA - Developers: Nintendo R&D1 - Releases: 1995-09-28 (JPN) */
	SOFTWARE( marioten,  0,        1995, "Nintendo", "Mario's Tennis (Jpn, USA)", 0, 0 )              /* Id: VUE-VMTJ-JPN/USA - Developers: Nintendo R&D1 / Tose Software - Releases: 1995-07-21 (JPN) */
	SOFTWARE( nesterfb,  0,        1996, "Nintendo", "Nester's Funky Bowling (USA)", 0, 0 )           /* Id: VUE-VNFE-USA - Developers: Nintendo IRD */ 
	SOFTWARE( panicbom,  0,        1995, "Nintendo", "Panic Bomber (USA)", 0, 0 )                     /* Id: VUE-VH2E-USA - Developers: Raizing */ 
	SOFTWARE( redalarmj, redalarm, 1995, "T&E Soft", "Red Alarm (Jpn)", 0, 0 )                        /* Id: VUE-VREJ-JPN - Developers: T&E Soft - Releases: 1995-07-21 (JPN) */
	SOFTWARE( redalarm,  0,        1995, "Nintendo", "Red Alarm (USA)", 0, 0 )                        /* Id: VUE-VREE-USA - Developers: T&E Soft */
	SOFTWARE( sdgundam,  0,        1995, "Bandai", "SD Gundam - Dimension War (Jpn)", 0, 0 )          /* Id: VUE-VSDJ-JPN - Developers: Bandai? - Releases: 1995-12-22 (JPN) */
	SOFTWARE( spaceinv,  0,        1995, "Taito", "Space Invaders - Virtual Collection (Jpn)", 0, 0 ) /* Id: VUE-VSPJ-JPN - Developers: Taito - Releases: 1995-12-01 (JPN) */
	SOFTWARE( ssquash,   0,        1995, "Coconuts Japan Entertainment", "Space Squash (Jpn)", 0, 0 ) /* Id: VUE-VSSJ-JPN - Developers: Tomcat System - Releases: 1995-09-29 (JPN) */
	SOFTWARE( vgolf,     0,        1995, "T&E Soft", "T&E Virtual Golf (Jpn)", 0, 0 )                 /* Id: VUE-VVGJ-JPN - Developers: T&E Soft - Releases: 1995-08-11 (JPN) */
	SOFTWARE( telerobo,  0,        1995, "Nintendo", "Teleroboxer (Jpn, USA)", 0, 0 )                 /* Id: VUE-VTBJ-JPN/USA - Developers: Nintendo R&D1 - Releases: 1995-07-21 (JPN) */
	SOFTWARE( tobidase,  0,        1995, "Hudson", "Tobidase! Panibon (Jpn)", 0, 0 )                  /* Id: VUE-VH2J_JPN - Developers: Raizing - Releases: 1995-07-21 (JPN) */
	SOFTWARE( vtetris,   0,        1995, "Bullet-Proof Software", "V-Tetris (Jpn)", 0, 0 )            /* Id: VUE-VTRJ-JPN - Developers: Locomotive - Releases: 1995-08-25 (JPN) */
	SOFTWARE( vforcej,   vforce,   1995, "Hudson", "Vertical Force (Jpn)", 0, 0 )                     /* Id: VUE-VH3J-JPN - Developers: Hudson - Releases: 1995-08-12 (JPN) */
	SOFTWARE( vforce,    0,        1995, "Nintendo", "Vertical Force (USA)", 0, 0 )                   /* Id: VUE-VH3E-USA - Developers: Hudson */
	SOFTWARE( vbowling,  0,        1995, "Athena", "Virtual Bowling (Jpn)", 0, 0 )                    /* Id: VUE-VVBJ-JPN - Developers: Athena - Releases: 1995-12-22 (JPN) */
	SOFTWARE( wariolnd,  0,        1995, "Nintendo", "Virtual Boy Wario Land (USA) ~ Virtual Boy Wario Land - Awazon no Hihou (Jpn)", 0, 0 ) /* Id: VUE-VWCJ-JPN/USA - Developers: Nintendo R&D1 - Releases: 1995-12-01 (JPN) */
	SOFTWARE( vfishing,  0,        1995, "Pack-In-Video", "Virtual Fishing (Jpn)", 0, 0 )             /* Id: VUE-VVFJ-JPN - Developers: Pack-In-Video - Releases: 1995-10-16 (JPN) */
	SOFTWARE( vlab,      0,        1995, "J-Wing", "Virtual Lab (Jpn)", 0, 0 )                        /* Id: VUE-VJVJ-JPN - Developers: J-Wing - Releases: 1995-12-08 (JPN) */
	SOFTWARE( vleague,   0,        1995, "Kemco", "Virtual League Baseball (USA)", 0, 0 )             /* Id: VUE-VVPE-USA - Developers: Kemco */
	SOFTWARE( vproyak,   0,        1995, "Kemco", "Virtual Pro Yakyuu '95 (Jpn)", 0, 0 )              /* Id: VUE-VVPJ-JPN - Developers: Kemco - Releases: 1995-08-11 (JPN) */
	SOFTWARE( waterwld,  0,        1995, "Ocean", "Waterworld (USA)", 0, 0 )                          /* Id: VUE-VWEE-USA - Developers: Ocean */
SOFTWARE_LIST_END

SOFTWARE_LIST( vboy_cart, "Nintendo Virtual Boy cartridges" )
