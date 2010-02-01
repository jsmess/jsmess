/***************************************************************************

    Sega 32X cartridges

***************************************************************************/

#include "emu.h"
#include "softlist.h"
#include "devices/cartslot.h"


#define S32X_ROM_LOAD( set, name, offset, length, hash )	\
SOFTWARE_START( set ) \
	ROM_REGION16_BE( 0x400000, CARTRIDGE_REGION_ROM, 0 ) \
	ROM_LOAD(name, offset, length, hash) \
SOFTWARE_END


S32X_ROM_LOAD( 36great,    "golf magazine 36 great holes starring fred couples (europe).bin",                  0x000000, 0x300000,   CRC(6ef99202) SHA1(55848f7c54533fd2a19011db661ef261cd8bdb22) )
S32X_ROM_LOAD( 36greatju,  "golf magazine 36 great holes starring fred couples (japan, usa).bin",              0x000000, 0x300000,   CRC(d3d0a2fe) SHA1(dc77b1e5c888c2c4284766915a5020bb14ee681d) )
S32X_ROM_LOAD( 36greatp,   "36 great holes starring fred couples(prototype - dec 21, 1994 - b).bin",           0x000000, 0x300000,   CRC(cc0334bb) SHA1(6a1801ef96cc8c4a6896ab55e105f7fa8d707f51) )
S32X_ROM_LOAD( 36greatp1,  "36 great holes starring fred couples(prototype - dec 21, 1994).bin",               0x000000, 0x300000,   CRC(3893dcfe) SHA1(23e84ec4d8cec09992fcff323f32f49f44d23cd2) )
S32X_ROM_LOAD( 36greatp2,  "36 great holes starring fred couples(prototype - dec 19, 1994).bin",               0x000000, 0x300000,   CRC(c073149f) SHA1(e28ab68e8aed7aa8e9f33a6e9c68678272df4d95) )
S32X_ROM_LOAD( 36greatp3,  "36 great holes starring fred couples(prototype - dec 14, 1994).bin",               0x000000, 0x300000,   CRC(a63b0636) SHA1(f78481e57111295c2f5aeb64cd306c3c5fbf6d97) )
S32X_ROM_LOAD( 36greatp4,  "36 great holes starring fred couples(prototype - dec 13, 1994).bin",               0x000000, 0x300000,   CRC(34bf7c6d) SHA1(6398a442595650d70a5ce20a52331899c0dfaf62) )
S32X_ROM_LOAD( 36greatp5,  "36 great holes starring fred couples(prototype - dec 06, 1994).bin",               0x000000, 0x300000,   CRC(ecfe5a0f) SHA1(af8eadcf49d0700c7479648865cc08cd40bc2808) )
S32X_ROM_LOAD( 36greatp6,  "36 great holes starring fred couples(prototype - dec 03, 1994).bin",               0x000000, 0x300000,   CRC(3f30c0d2) SHA1(7ca51ad03ebb95f017a1b900a4a1592876d0d5c2) )
S32X_ROM_LOAD( 36greatp7,  "36 great holes starring fred couples(prototype - dec 02, 1994).bin",               0x000000, 0x300000,   CRC(323c2935) SHA1(5b1140e33357a3d0fdc056ab5a350d588b5e5af9) )
S32X_ROM_LOAD( 36greatp8,  "36 great holes starring fred couples(prototype - nov 30, 1994).bin",               0x000000, 0x300000,   CRC(41e2d54d) SHA1(19681b2e5d1eafaa09401dca296bea884fbc08c1) )
S32X_ROM_LOAD( 36greatp9,  "36 great holes starring fred couples(prototype - nov 29, 1994).bin",               0x000000, 0x300000,   CRC(9c5011b0) SHA1(5658febd37534280962dc6801efc10604b07603f) )
S32X_ROM_LOAD( 36greatp10, "36 great holes starring fred couples(prototype - nov 28, 1994 - b).bin",           0x000000, 0x300000,   CRC(8f4f6479) SHA1(e341e5fca64def64c019310d4f551ff48908291b) )
S32X_ROM_LOAD( 36greatp11, "36 great holes starring fred couples(prototype - nov 28, 1994).bin",               0x000000, 0x300000,   CRC(38d09772) SHA1(f0d06074b1b5ce2df77fdaaf74620158efb7f875) )
S32X_ROM_LOAD( 36greatp12, "36 great holes starring fred couples(prototype - nov 27, 1994).bin",               0x000000, 0x300000,   CRC(b5e82f07) SHA1(afa1f5c2dab7de2114d3a34794785b0d6768d95c) )
S32X_ROM_LOAD( 36greatp13, "36 great holes starring fred couples(prototype - nov 26, 1994).bin",               0x000000, 0x300000,   CRC(2ece26db) SHA1(541ddfac44323c589ad23f51a5be984ecfa55539) )
S32X_ROM_LOAD( 36greatp14, "36 great holes starring fred couples(prototype - nov 23, 1994).bin",               0x000000, 0x2fdcd4,   CRC(0b58f6a5) SHA1(fa2f2d3887bb3e6010a5c13289f63681cac0404a) )
S32X_ROM_LOAD( 36greatp15, "36 great holes starring fred couples(prototype - nov 22, 1994 - b).bin",           0x000000, 0x2fdcd4,   CRC(6b656643) SHA1(0590ff4405fcd0ca8f2fc5293690330fdf63127a) )
S32X_ROM_LOAD( 36greatp16, "36 great holes starring fred couples(prototype - nov 22, 1994).bin",               0x000000, 0x2fdcd4,   CRC(e1e98cac) SHA1(a6bee7800f73e2ca12381432154b636ddd279412) )
S32X_ROM_LOAD( 36greatp17, "36 great holes starring fred couples(prototype - nov 21, 1994).bin",               0x000000, 0x2fdcd4,   CRC(8d0713ce) SHA1(34d7bf6b86c877ab068390f660073016a95711e3) )
S32X_ROM_LOAD( 36greatp18, "36 great holes starring fred couples(prototype - nov 19, 1994 - b).bin",           0x000000, 0x300000,   CRC(8fa32086) SHA1(5fc3837ba3b98f9f2c560d3775da969798a150ee) )
S32X_ROM_LOAD( 36greatp19, "36 great holes starring fred couples(prototype - nov 19, 1994).bin",               0x000000, 0x2fdce0,   CRC(da1026c8) SHA1(2ea5fcdffa9b8f22b1468eeb2e76134d3f1b3be6) )
S32X_ROM_LOAD( 36greatp20, "36 great holes starring fred couples(prototype - nov 18, 1994).bin",               0x000000, 0x2fdcc8,   CRC(dc4c7c27) SHA1(abe1d16dff2b2c1752e2f47bb15f97f049d6bfc0) )
S32X_ROM_LOAD( 36greatp21, "36 great holes starring fred couples(prototype - nov 16, 1994).bin",               0x000000, 0x2fdcc8,   CRC(ccb9fead) SHA1(3bff7d113fa7487d015a54d8f27050f2cf296106) )
S32X_ROM_LOAD( 36greatp22, "36 great holes starring fred couples(prototype - nov 10, 1994).bin",               0x000000, 0x2fdcc8,   CRC(5226ef71) SHA1(33620d0bb49c52a24563958b4caec883b2009a90) )
S32X_ROM_LOAD( 36greatp23, "36 great holes starring fred couples(prototype - nov 08, 1994).bin",               0x000000, 0x2fdcc8,   CRC(bcaf23fd) SHA1(f317903c11777991417abf95cc45d4888bee25d6) )
S32X_ROM_LOAD( 36greatp24, "36 great holes starring fred couples(prototype - nov 07, 1994).bin",               0x000000, 0x2fdcc8,   CRC(36b1708e) SHA1(ea7539dbeaa86fdc36132e85be48f6577fb6dbb9) )
S32X_ROM_LOAD( 36greatp25, "36 great holes starring fred couples(prototype - nov 05, 1994).bin",               0x000000, 0x2fdcc8,   CRC(1559541e) SHA1(eff8337257b0cc6221ce459d32191f7d63450d1c) )
S32X_ROM_LOAD( 36greatp26, "36 great holes starring fred couples(prototype - nov 03, 1994).bin",               0x000000, 0x2fdcc8,   CRC(d261a6c3) SHA1(7c295cb39408d21835b76c2c1aeb03b659094593) )
S32X_ROM_LOAD( 36greatp27, "36 great holes starring fred couples(prototype - nov 01, 1994).bin",               0x000000, 0x2fdda0,   CRC(91106a69) SHA1(f6b944876b40b5a52bf1293b71373cb3f5c71a14) )
S32X_ROM_LOAD( 36greatp28, "36 great holes starring fred couples(prototype - oct 26, 1994).bin",               0x000000, 0x2f2860,   CRC(37a79c9c) SHA1(91d78a15094c20fb6a8103d5addbaa4f90d199b8) )
S32X_ROM_LOAD( 36greatp29, "36 great holes starring fred couples(prototype - oct 24, 1994).bin",               0x000000, 0x2f2868,   CRC(ffe279cc) SHA1(2093a7437062dc21c4440a360dd177102ca6cc88) )
S32X_ROM_LOAD( 36greatp30, "36 great holes starring fred couples(prototype - oct 18, 1994).bin",               0x000000, 0x400000,   CRC(e39ba17f) SHA1(41e12cb8465acb6e0180deea9b34c2554e3f79e1) )
S32X_ROM_LOAD( 36greatp31, "36 great holes starring fred couples(prototype - oct 17, 1994).bin",               0x000000, 0x2dcb24,   CRC(cbd33fc7) SHA1(83b70bea9efa281dd5119334a28ef33bc7426325) )
S32X_ROM_LOAD( 36greatp32, "36 great holes starring fred couples(prototype - oct 11, 1994).bin",               0x000000, 0x2dcb24,   CRC(622a34a5) SHA1(94c994ce74377ec726b4c0803b51fd4ca6cc4389) )
S32X_ROM_LOAD( 36greatp33, "36 great holes starring fred couples (32x rev1.x) (prototype - jul 06, 1994).bin", 0x000000, 0x102162,   CRC(6726c100) SHA1(6a066c44d71199d62dbc368efbc371dd01cc95e1) )
S32X_ROM_LOAD( afterb,     "after burner complete (europe).bin",                                               0x000000, 0x200000,   CRC(029106f5) SHA1(c7d56cc75dc9a5206fda6a080cbedae9f48c82bc) )
S32X_ROM_LOAD( afterbju,   "after burner complete ~ after burner (japan, usa).bin",                            0x000000, 0x200000,   CRC(204044c4) SHA1(9cf575feb036e2f26e78350154d5eb2fd3825325) )
S32X_ROM_LOAD( spidwebf,   "amazing spider-man, the - web of fire (usa).bin",                                  0x000000, 0x300000,   CRC(29dce257) SHA1(7cc2ea1e10f110338ad880bd3e7ff3bce72e7e9e) )
S32X_ROM_LOAD( bcracers,   "bc racers (usa).bin",                                                              0x000000, 0x300000,   CRC(936c3d27) SHA1(9b5fd499eaa442d48a2c97fceb1d505dc8e8ddff) )
S32X_ROM_LOAD( blackthr,   "blackthorne (usa).bin",                                                            0x000000, 0x300000,   CRC(d1a60a47) SHA1(4bf120cf056fe1417ca5b02fa0372ef33cb8ec11) )
S32X_ROM_LOAD( brutal,     "brutal unleashed - above the claw (usa).bin",                                      0x000000, 0x300000,   CRC(7a72c939) SHA1(40aa2c787f37772cdbd7280b8be06b15421fabae) )
S32X_ROM_LOAD( cosmicc,    "cosmic carnage (europe).bin",                                                      0x000000, 0x300000,   CRC(9f3fdbc2) SHA1(900e8ecdcc4460b4c4af3873e1d6bc83da4aee99) )
S32X_ROM_LOAD( cyberb,     "cyber brawl ~ cosmic carnage (japan, usa).bin",                                    0x000000, 0x300000,   CRC(7c7be6a2) SHA1(9a563ed821b483148339561ebd2b876efa58847b) )
S32X_ROM_LOAD( cosmiccp,   "cosmic carnage(prototype - oct 11, 1994).bin",                                     0x000000, 0x300000,   CRC(e483498b) SHA1(bbdfe2b276c255247225efd1e8348be57a42ef4a) )
S32X_ROM_LOAD( cosmiccp1,  "cosmic carnage(prototype - oct 07, 1994).bin",                                     0x000000, 0x300000,   CRC(d8b04c24) SHA1(c051de6dccc48352d51650f2712dbc7d62a1ec1a) )
S32X_ROM_LOAD( cosmiccp2,  "cosmic carnage(prototype - oct 05, 1994).bin",                                     0x000000, 0x300000,   CRC(bfe27f56) SHA1(99bb4c98c7de0668934ab91a6eb01c9f680918b9) )
S32X_ROM_LOAD( cosmiccp3,  "cosmic carnage(prototype - oct 04, 1994).bin",                                     0x000000, 0x300000,   CRC(c6867772) SHA1(17cf3d4671a3313764df42e0f9a25133b0dc4ec9) )
S32X_ROM_LOAD( cosmiccp4,  "cosmic carnage(prototype - sep 28, 1994).bin",                                     0x000000, 0x300000,   CRC(cf89175f) SHA1(443c961bfc441b4d0ace5f6e0a8d1bb7abf8ac28) )
S32X_ROM_LOAD( cosmiccp5,  "cosmic carnage(prototype - sep 26, 1994).bin",                                     0x000000, 0x300000,   CRC(f04f4366) SHA1(512d3c9b51e2f3015265dc78757363f45fc0a788) )
S32X_ROM_LOAD( cosmiccp6,  "cosmic carnage(prototype - sep 21, 1994).bin",                                     0x000000, 0x400000,   CRC(7b58141c) SHA1(926996f4b1f935ce4c3b46e902171c4b4da16385) )
S32X_ROM_LOAD( cosmiccp7,  "cosmic carnage(prototype - sep 06, 1994).bin",                                     0x000000, 0x300000,   CRC(d4166241) SHA1(f84006cee56748bf2c7249dec3c356409cb0e0a5) )
S32X_ROM_LOAD( darxide,    "darxide (europe) (en,fr,de,es).bin",                                               0x000000, 0x200000,   CRC(22d7c906) SHA1(108b4ffed8643abdefa921cfb58389b119b47f3d) )
S32X_ROM_LOAD( doom,       "doom (europe).bin",                                                                0x000000, 0x300000,   CRC(53734e3a) SHA1(4ec03c2114ebffbbcd16859583340d4ea4cd8dd5) )
S32X_ROM_LOAD( doomju,     "doom (japan, usa).bin",                                                            0x000000, 0x300000,   CRC(208332fd) SHA1(b68e9c7af81853b8f05b8696033dfe4c80327e38) )
S32X_ROM_LOAD( doomp,      "doom(prototype - dec 01, 1994).bin",                                               0x000000, 0x2f5628,   CRC(7dcc1c18) SHA1(b2349f625ac077222b49f0e507c552b24c824bd2) )
S32X_ROM_LOAD( doompa,     "doom(prototype - oct 02, 1994).bin",                                               0x000000, 0x400000,   CRC(6182e821) SHA1(2945da1e6452b194fbbe44a685864a2000ea5cfd) )
S32X_ROM_LOAD( doompj,     "doom(prototype - sep 06, 1994).bin",                                               0x000000, 0x400000,   CRC(646d5c47) SHA1(4fda0184d688886314d929224d92b4a35c712675) )
S32X_ROM_LOAD( doompi,     "doom(prototype - sep 09, 1994).bin",                                               0x000000, 0x3b6e28,   CRC(8d757b51) SHA1(5697176636df575d8b65480708efa85544d44302) )
S32X_ROM_LOAD( doomph,     "doom(prototype - sep 14, 1994).bin",                                               0x000000, 0x3962bc,   CRC(102678b3) SHA1(1bf7bc1fc2eafe71d1e72608566d5c0d01d5c79c) )
S32X_ROM_LOAD( doompg,     "doom(prototype - sep 16, 1994).bin",                                               0x000000, 0x400000,   CRC(b554596f) SHA1(0530d326b75e97b04530761f50ced4a226b183d2) )
S32X_ROM_LOAD( doompf,     "doom(prototype - sep 21, 1994).bin",                                               0x000000, 0x358608,   CRC(2d09b919) SHA1(2a2c80adb1cf62a71581400357951b2007770e8b) )
S32X_ROM_LOAD( doompe,     "doom(prototype - sep 23, 1994).bin",                                               0x000000, 0x300000,   CRC(abc3eb89) SHA1(e472548171a4e6183f16618bfcb5b6c09e7d6744) )
S32X_ROM_LOAD( doompd,     "doom(prototype - sep 25, 1994).bin",                                               0x000000, 0x300000,   CRC(b9ae1316) SHA1(c3e78c38cfd2516f943a10f435664ba06790f1c4) )
S32X_ROM_LOAD( doompc,     "doom(prototype - sep 27, 1994).bin",                                               0x000000, 0x300000,   CRC(12991053) SHA1(f7539c406821b5ae4bcb0f41fe2839d07ab1fdb2) )
S32X_ROM_LOAD( doompb,     "doom(prototype - sep 28, 1994).bin",                                               0x000000, 0x300000,   CRC(12991053) SHA1(f7539c406821b5ae4bcb0f41fe2839d07ab1fdb2) )
S32X_ROM_LOAD( doomrr,     "doom rr(prototype - mar 07, 1995).bin",                                            0x000000, 0x300000,   CRC(955cf84a) SHA1(a2d5b5acc7e19abfb927bb0ed110d80d685510c9) )
S32X_ROM_LOAD( doomrr1,    "doom rr(prototype - feb 21, 1995).bin",                                            0x000000, 0x300000,   CRC(6f1572f9) SHA1(b298ad7bacca89770bcd6dea796e5dbbfb72e642) )
S32X_ROM_LOAD( doomrr2,    "doom rr(prototype - feb 15, 1995).bin",                                            0x000000, 0x300000,   CRC(48399bc5) SHA1(63568023c907d636133e1a2fd140afbadb910e98) )
S32X_ROM_LOAD( eccodemo,   "ecco the dolphin cinepak demo (japan, usa) (developer cart).bin",                  0x000000, 0x300000,   CRC(b06178df) SHA1(10409f2245b058e8a32cba51e1ea391ca4480108) )
S32X_ROM_LOAD( fifa96,     "fifa soccer '96 (europe) (en,fr,de,es,it,sv).bin",                                 0x000000, 0x300000,   CRC(fb14a7c8) SHA1(131ebb717dee4dd1d8f5ab2b9393c23785d3a359) )
S32X_ROM_LOAD( knuckle,    "knuckles' chaotix (europe).bin",                                                   0x000000, 0x300000,   CRC(41d63572) SHA1(5c1a2e327a656217604d4bae7e141764a7e59922) )
S32X_ROM_LOAD( chaotix,    "chaotix ~ knuckles' chaotix (japan, usa).bin",                                     0x000000, 0x300000,   CRC(d0b0b842) SHA1(0c2fff7bc79ed26507c08ac47464c3af19f7ced7) )
S32X_ROM_LOAD( knucklep,   "knuckles' chaotix(prototype 214 - feb 14, 1995, 06.46).bin",                       0x000000, 0x300000,   CRC(ba0a1108) SHA1(af3bfd9540098581aef08664d625725ffcc90be2) )
S32X_ROM_LOAD( knucklep1,  "knuckles' chaotix(prototype 0213 - feb 13, 1995, 07.30).bin",                      0x000000, 0x300000,   CRC(14f43e14) SHA1(0cbbcc96f23917feb844469163bc8ab14da94816) )
S32X_ROM_LOAD( knucklep2,  "knuckles' chaotix(prototype 213b - feb 13, 1995, 06.46).bin",                      0x000000, 0x300000,   CRC(aff06bff) SHA1(88b122398c1b25f432ae845b799f1308f51aa055) )
S32X_ROM_LOAD( knucklep3,  "knuckles' chaotix(prototype 210 - feb 10, 1995, 06.28).bin",                       0x000000, 0x300000,   CRC(7b716175) SHA1(4c13842abeda86ce6577da67ba5fc24fbfbdc0de) )
S32X_ROM_LOAD( knucklep4,  "knuckles' chaotix(prototype 209 - feb 09, 1995, 08.25).bin",                       0x000000, 0x300000,   CRC(e95d7f57) SHA1(01c9f4614f282a05b3e86701b4ca18a1a0e1889c) )
S32X_ROM_LOAD( knucklep5,  "knuckles' chaotix(prototype 208 - feb 08, 1995, 11.17).bin",                       0x000000, 0x300000,   CRC(b099e5f6) SHA1(d965d12c8cce09683d80c08d06c74b7c6e7b8dab) )
S32X_ROM_LOAD( knucklep6,  "knuckles' chaotix(prototype 0202 - feb 07, 1995, 15.25).bin",                      0x000000, 0x300000,   CRC(7b32e440) SHA1(4e4af26f944cf3b5d99f60f37b20dc67207a4fbe) )
S32X_ROM_LOAD( knucklep7,  "knuckles' chaotix(prototype 119 - jan 19, 1995, 07.04).bin",                       0x000000, 0x400000,   CRC(468ad032) SHA1(2b1a5f19113fa4015437ede5a195c5dfb377e5be) )
S32X_ROM_LOAD( knucklep8,  "knuckles' chaotix(prototype 0111 - jan 12, 1995, 09.36).bin",                      0x000000, 0x400000,   CRC(5523ea78) SHA1(882ce770f32f4af61ac422accecc8a84e1c52cd3) )
S32X_ROM_LOAD( knucklep9,  "knuckles' chaotix(prototype 1207 - dec 07, 1994, 07.15).bin",                      0x000000, 0x300000,   CRC(d62ae235) SHA1(150dace1482ebf38a0f6242be6d8b7ea19f8a737) )
S32X_ROM_LOAD( knucklep10, "knuckles' chaotix(prototype 1227 - dec 27, 1994, 10.28).bin",                      0x000000, 0x400000,   CRC(4307d738) SHA1(c62a8be339ffd9dd182a3f8e60b6e27225cfc5f2) )
S32X_ROM_LOAD( knucklep11, "knuckles' chaotix(prototype 1229 - dec 30, 1994, 15.31).bin",                      0x000000, 0x400000,   CRC(36a294e0) SHA1(883c7748c3c352b44238d2bdc9152ac9651e99fb) )
S32X_ROM_LOAD( kolibri,    "kolibri (usa, europe).bin",                                                        0x000000, 0x300000,   CRC(20ca53ef) SHA1(191ae0b525ecf32664086d8d748e0b35f776ddfe) )
S32X_ROM_LOAD( marsch,     "mars check program version 1.0 (japan, usa) (sdk build) (set 1).bin",              0x000000, 0x400000,   CRC(8f7260fb) SHA1(7654c6d3cf2883c30df51cf38d723ab7902280c4) )
S32X_ROM_LOAD( marsch1,    "mars check program version 1.0 (japan, usa) (sdk build) (set 2).bin",              0x000000, 0x10000,    CRC(a2da35fd) SHA1(bbc6a66ad9268a3bb6e35e565e770581e7fe8ac2) )
S32X_ROM_LOAD( marssa,     "mars sample program - gnu sierra (japan, usa) (sdk build).bin",                    0x000000, 0xa0000,    CRC(5f0df42c) SHA1(04efcf2802b11e130378734671ff8cfb78facd67) )
S32X_ROM_LOAD( marssa1,    "mars sample program - pharaoh (japan, usa) (sdk build).bin",                       0x000000, 0x40000,    CRC(969c80d4) SHA1(d038d88cf518116fcb4df72df51dbc0c374792e2) )
S32X_ROM_LOAD( marssa2,    "mars sample program - runlength mode test (japan, usa) (sdk build).bin",           0x000000, 0x40000,    CRC(d630e343) SHA1(56566575de6d1d42630f688eb0eeb1a93ebe2624) )
S32X_ROM_LOAD( marssa3,    "mars sample program - texture test (japan, usa) (sdk build).bin",                  0x000000, 0x6000,     CRC(9b2ad63f) SHA1(885abd1de7b67b115057610d94f24b1b4e294706) )
S32X_ROM_LOAD( metalhd,    "metal head (europe) (en,ja).bin",                                                  0x000000, 0x300000,   CRC(bf9b8376) SHA1(2a7767024e23b55fe193d586a96a3ba3c92e7ea8) )
S32X_ROM_LOAD( metalhdju,  "metal head (japan, usa) (en,ja).bin",                                              0x000000, 0x300000,   CRC(ef5553ff) SHA1(4e872fbb44ecb2bd730abd8cc8f32f96b10582c0) )
S32X_ROM_LOAD( metalhdp,   "metal head(prototype - dec 20, 1994).bin",                                         0x000000, 0x300000,   CRC(de99c9ce) SHA1(8d7a9f78860342643502e2ba77b445b4dc7e078e) )
S32X_ROM_LOAD( metalhdp1,  "metal head(prototype - dec 13, 1994).bin",                                         0x000000, 0x300000,   CRC(3302b1a1) SHA1(5a38dce482c7fe38a50c6fd8884b019f1acab2a1) )
S32X_ROM_LOAD( metalhdp2,  "metal head(prototype - dec 09, 1994).bin",                                         0x000000, 0x300000,   CRC(5369d23f) SHA1(4d795a7ecab65527e6538ef886c3c3783a1ba936) )
S32X_ROM_LOAD( metalhdp3,  "metal head(prototype - dec 07, 1994 - b).bin",                                     0x000000, 0x300000,   CRC(476d29b0) SHA1(438e172b22ef2b78f5bceb3195da408639ae463c) )
S32X_ROM_LOAD( metalhdp4,  "metal head(prototype - dec 07, 1994).bin",                                         0x000000, 0x300000,   CRC(fa0fc326) SHA1(d82b26783f0843faabe526167eabf296c84b0c47) )
S32X_ROM_LOAD( metalhdp5,  "metal head(prototype - dec 02, 1994).bin",                                         0x000000, 0x300000,   CRC(8b7bc8a4) SHA1(194f45a0651a59305e0e727e407801ff09968a62) )
S32X_ROM_LOAD( metalhdp6,  "metal head(prototype - nov 21, 1994).bin",                                         0x000000, 0x300000,   CRC(398bcb98) SHA1(e7667f4a4342a5a45b0a010f0fe2e61b2bf99fea) )
S32X_ROM_LOAD( metalhdp7,  "metal head(prototype - nov 18, 1994).bin",                                         0x000000, 0x300000,   CRC(a0343749) SHA1(a943ea135d6c685f87957503697cb9f894e12c52) )
S32X_ROM_LOAD( metalhdp8,  "metal head(prototype - nov 14, 1994).bin",                                         0x000000, 0x300000,   CRC(2ea0ff81) SHA1(a81db052a095360439a1da6e41ac19b91561ab27) )
S32X_ROM_LOAD( mk2,        "mortal kombat ii (europe).bin",                                                    0x000000, 0x400000,   CRC(211085ce) SHA1(f75698de887d0ef980f73e35fc4615887a9ad58f) )
S32X_ROM_LOAD( mk2ju,      "mortal kombat ii (japan, usa).bin",                                                0x000000, 0x400000,   CRC(773c0a85) SHA1(d55964bd935110ac2d9cbd3b085b7e8b71b11df2) )
S32X_ROM_LOAD( motox,      "motocross championship (europe).bin",                                              0x000000, 0x200000,   CRC(ae3364e9) SHA1(af0cb8626825e1b431eba07b8a9b571186745c16) )
S32X_ROM_LOAD( motoxu,     "motocross championship (usa).bin",                                                 0x000000, 0x200000,   CRC(a21c5761) SHA1(5f1a107991aaf9eff0b3ce864b2e3151f56abe7b) )
S32X_ROM_LOAD( motoxp,     "motocross championship(prototype - nov 11, 1994).bin",                             0x000000, 0x200000,   CRC(37f78d3d) SHA1(45f457930884c7a5ec35361e37b1fd8cd91e4487) )
S32X_ROM_LOAD( motoxpa,    "motocross championship(prototype - nov 08, 1994).bin",                             0x000000, 0x200000,   CRC(5ff9eab9) SHA1(93e13f7d3c4640e9aee7d577e974ab6581c07f91) )
S32X_ROM_LOAD( motoxpb,    "motocross championship(prototype - nov 07, 1994).bin",                             0x000000, 0x200000,   CRC(63db2fb0) SHA1(181a58cfc8348e18e5d34ed0a75e79acedcc004e) )
S32X_ROM_LOAD( motoxpc,    "motocross championship(prototype - nov 04, 1994).bin",                             0x000000, 0x1f36b8,   CRC(6e12d410) SHA1(ca4db761d26328b99445445b99304e90d7279c3e) )
S32X_ROM_LOAD( motoxpd,    "motocross championship(prototype - nov 03, 1994).bin",                             0x000000, 0x1f34c0,   CRC(c652c802) SHA1(5dc89f7f2cc03689991c9325a1a2d08c6df5dc1b) )
S32X_ROM_LOAD( motoxpe,    "motocross championship(prototype - nov 02, 1994 - b).bin",                         0x000000, 0x1f36f4,   CRC(efe307ff) SHA1(3b7cfe05c0a66fc025bbdd3a35ba7828b2fc3c7f) )
S32X_ROM_LOAD( motoxpf,    "motocross championship(prototype - nov 02, 1994).bin",                             0x000000, 0x1f36f4,   CRC(cefef428) SHA1(607b9d89e312294ca4e477de238bec2d1bea2a42) )
S32X_ROM_LOAD( motoxpg,    "motocross championship(prototype - nov 01, 1994).bin",                             0x000000, 0x1f361c,   CRC(46970275) SHA1(372a43df94cbed3859bbddad4982fa5301416aed) )
S32X_ROM_LOAD( motoxph,    "motocross championship(prototype - oct 28, 1994).bin",                             0x000000, 0x1c20ec,   CRC(59107472) SHA1(174158dd18a4d243d3c943c9f8cf080acd91cc3b) )
S32X_ROM_LOAD( motoxpi,    "motocross championship(prototype - oct 24, 1994).bin",                             0x000000, 0x1a7d8c,   CRC(20c49183) SHA1(d641e7b2fd0a44ad603cdc324921a25f1b897288) )
S32X_ROM_LOAD( motoxpj,    "motocross championship(prototype - oct 20, 1994).bin",                             0x000000, 0x195970,   CRC(ac6cf708) SHA1(95edb6101eee31588e7b0ce0b67d73206070eb81) )
S32X_ROM_LOAD( motoxpk,    "motocross championship(prototype - oct 19, 1994).bin",                             0x000000, 0x1917b0,   CRC(385ed8c7) SHA1(3862fe8d98a512e3332748b8f73fcdf84faba489) )
S32X_ROM_LOAD( motoxpl,    "motocross championship(prototype - oct 18, 1994).bin",                             0x000000, 0x18cabc,   CRC(2c2dc4df) SHA1(bafabf4eb40fb1197dd543fa6ae69df1f87e9d02) )
S32X_ROM_LOAD( motoxpm,    "motocross championship(prototype - oct 17, 1994).bin",                             0x000000, 0x188c58,   CRC(f3163d21) SHA1(670779a85db5673b23825b91693b1015f841fde3) )
S32X_ROM_LOAD( motoxpn,    "motocross championship(prototype - oct 12, 1994).bin",                             0x000000, 0x19cd48,   CRC(0a075812) SHA1(102e763c6f4bf65210b307d34d75473fd27d7ae4) )
S32X_ROM_LOAD( nbajamte,   "nba jam tournament edition (world).bin",                                           0x000000, 0x400000,   CRC(6b7994aa) SHA1(c8af3e74c49514669ba6652ec0c81bccf77873b6) )
S32X_ROM_LOAD( nflquart,   "nfl quarterback club (world).bin",                                                 0x000000, 0x300000,   CRC(0bc7018d) SHA1(a0dc24f2f3a7fc5bfd12791cf25af7f7888843cf) )
S32X_ROM_LOAD( pitfall,    "pitfall - the mayan adventure (usa).bin",                                          0x000000, 0x300000,   CRC(f9126f15) SHA1(ee864d1677c6d976d0846eb5f8d8edb839acfb76) )
S32X_ROM_LOAD( primal,     "primal rage (usa, europe).bin",                                                    0x000000, 0x400000,   CRC(e78a4d28) SHA1(5084dcca51d76173c383ab7d04cbc661673545f7) )
S32X_ROM_LOAD( rbi95,      "rbi baseball '95 (usa).bin",                                                       0x000000, 0x200000,   CRC(ff795fdc) SHA1(4f90433a4403fd74cafeea49272689046de4ae43) )
S32X_ROM_LOAD( sangoku4,   "sangokushi iv (japan).bin",                                                        0x000000, 0x400000,   CRC(e4de7625) SHA1(74a3ba27c55cff12409bf6c9324ece6247abbad1) )
S32X_ROM_LOAD( stellar,    "shadow squadron ~ stellar assault (usa, europe).bin",                              0x000000, 0x200000,   CRC(60c49e4d) SHA1(561c8c63dbcabc0b1b6f31673ca75a0bde7abc72) )
S32X_ROM_LOAD( stellarj,   "stellar assault (japan).bin",                                                      0x000000, 0x200000,   CRC(fce4c8c7) SHA1(ff4f1a2dded85f3ad43bf28a85c46ad8595d5614) )
S32X_ROM_LOAD( shadsqp,    "shadow squadron(prototype - mar 13, 1995 - b).bin",                                0x000000, 0x200000,   CRC(fd7fedb2) SHA1(9875b357fd2d651892054d233c608b6f3bbb40e7) )
S32X_ROM_LOAD( shadsqp1,   "shadow squadron(prototype - mar 13, 1995).bin",                                    0x000000, 0x200000,   CRC(58962105) SHA1(9bae40d4d07a096545b11f6b0434db778c238af5) )
S32X_ROM_LOAD( shadsqp2,   "shadow squadron(prototype - mar 06, 1995).bin",                                    0x000000, 0x200000,   CRC(7eec3665) SHA1(31a0a0a34719b1259de98458c24cf5eb78f7109c) )
S32X_ROM_LOAD( shadsqp3,   "shadow squadron(prototype - mar 02, 1995).bin",                                    0x000000, 0x200000,   CRC(25bdde61) SHA1(5a83e4ebffe8ac7108b45697be7468bd7f111642) )
S32X_ROM_LOAD( shadsqp4,   "shadow squadron(prototype - feb 22, 1995).bin",                                    0x000000, 0x200000,   CRC(48ef9ee9) SHA1(9d72335cb0468a9699f7e92927043387091bc0c9) )
S32X_ROM_LOAD( shadsqp5,   "shadow squadron(prototype - feb 13, 1995).bin",                                    0x000000, 0x200000,   CRC(ca0de10e) SHA1(71061c0a02b4cc3c3e39d872bf86b190421cb594) )
S32X_ROM_LOAD( shadsqp6,   "shadow squadron(prototype - feb 06, 1995).bin",                                    0x000000, 0x200000,   CRC(24057310) SHA1(6ff8cd79cfa073697dd13cd1c4dc767b4e0c558e) )
S32X_ROM_LOAD( sharrierp,  "space harrier(prototype - sep 20, 1994).bin",                                      0x000000, 0x200000,   CRC(2c65fa40) SHA1(05d24d6f6c645866b8b1fde96746e27d0a19bbc6) )
S32X_ROM_LOAD( sharrier,   "space harrier (europe).bin",                                                       0x000000, 0x200000,   CRC(5cac3587) SHA1(8b0495257fa5392ef9ddcc9c3ba1860ae58f4f3d) )
S32X_ROM_LOAD( sharrierju, "space harrier (japan, usa).bin",                                                   0x000000, 0x200000,   CRC(86e7f989) SHA1(f32a52a7082761982024e40291dbd962a835b231) )
S32X_ROM_LOAD( startr,     "star trek starfleet academy - starship bridge simulator (usa).bin",                0x000000, 0x200000,   CRC(dd9708b9) SHA1(e5248328b64a1ec4f1079c88ee53ef8d48e99e58) )
S32X_ROM_LOAD( swa,        "star wars arcade (europe).bin",                                                    0x000000, 0x280000,   CRC(82e82660) SHA1(a877cbf704fe7480966fd88a3c39efb6a39392ac) )
S32X_ROM_LOAD( swaj,       "star wars arcade (japan).bin",                                                     0x000000, 0x280000,   CRC(f4e9b846) SHA1(282767a07e453e868de52b46bf11def3d071bda6) )
S32X_ROM_LOAD( swau,       "star wars arcade (usa).bin",                                                       0x000000, 0x280000,   CRC(2f16b44a) SHA1(f4ffaaf1d8330ea971643021be3f3203e1ea065d) )
S32X_ROM_LOAD( swap,       "star wars arcade(prototype - oct 07, 1994 - b).bin",                               0x000000, 0x280000,   CRC(57afc1ac) SHA1(c75853fb88f13a01ddca05b2202ed150770c1284) )
S32X_ROM_LOAD( swap1,      "star wars arcade(prototype - oct 06, 1994 - p).bin",                               0x000000, 0x280000,   CRC(c00af920) SHA1(1ade331c81eade613748126882057841dc02a420) )
S32X_ROM_LOAD( swap2,      "star wars arcade(prototype - oct 05, 1994).bin",                                   0x000000, 0x266621,   CRC(9861e21f) SHA1(24e599b251d643b74a923323a83e43a26d21fafc) )
S32X_ROM_LOAD( swap3,      "star wars arcade(prototype - oct 03, 1994).bin",                                   0x000000, 0x300000,   CRC(a653a183) SHA1(38fde66801083a60c142c3f3e7bae2aa81518acf) )
S32X_ROM_LOAD( swap4,      "star wars arcade(prototype - sep 30, 1994 - b).bin",                               0x000000, 0x266739,   CRC(6074f26c) SHA1(f1786e62b7707b8a01b66720f2722d555ec9d4db) )
S32X_ROM_LOAD( swap5,      "star wars arcade(prototype - sep 29, 1994).bin",                                   0x000000, 0x266931,   CRC(7682fe72) SHA1(2f93b915add1357dfcf42135d4ce85e6bb668cf8) )
S32X_ROM_LOAD( swap6,      "star wars arcade(prototype - sep 23, 1994).bin",                                   0x000000, 0x266909,   CRC(8e7bfc7c) SHA1(75ce473164f678f9f394d719b35850e4899a8cd1) )
S32X_ROM_LOAD( swap7,      "star wars arcade(prototype - sep 18, 1994).bin",                                   0x000000, 0x300000,   CRC(02933d44) SHA1(5d924628b507464ec1b1ca8755cb509cf68b02af) )
S32X_ROM_LOAD( swap8,      "star wars arcade(prototype - sep 16, 1994).bin",                                   0x000000, 0x300000,   CRC(150a0dfd) SHA1(492816545c7309d2b7594b5d4f8d508f4a13e06a) )
S32X_ROM_LOAD( swap9,      "star wars arcade(prototype - sep 15, 1994).bin",                                   0x000000, 0x300000,   CRC(34c6a769) SHA1(9a492a52be224ee9ff41d4ea7df3cee67f028235) )
S32X_ROM_LOAD( swap10,     "star wars arcade(prototype - sep 14, 1994 - d).bin",                               0x000000, 0x263741,   CRC(efc980fe) SHA1(2ba71086a01e14051dce2a5c24bba49a991cb2e5) )
S32X_ROM_LOAD( swap11,     "star wars arcade(prototype - sep 13, 1994 - b).bin",                               0x000000, 0x25a7d1,   CRC(dc5628ba) SHA1(d4c262ba8118f2f2af720a45e461d4254d5163b4) )
S32X_ROM_LOAD( swap12,     "star wars arcade(prototype - sep 12, 1994).bin",                                   0x000000, 0x200000,   CRC(118a011c) SHA1(b328170014bfc4770479eacc017f76bcaee61389) )
S32X_ROM_LOAD( swap13,     "star wars arcade(prototype - sep 09, 1994).bin",                                   0x000000, 0x200000,   CRC(5bdd86d3) SHA1(761aa97ab8f7524c49a141804246209fd6c970f2) )
S32X_ROM_LOAD( swap14,     "star wars arcade(prototype - sep 07, 1994).bin",                                   0x000000, 0x200000,   CRC(c6a8582b) SHA1(7e84f2eafef33e211500a551b3d430155368b66e) )
S32X_ROM_LOAD( swap15,     "star wars arcade(prototype - sep 06, 1994).bin",                                   0x000000, 0x200000,   CRC(b8d9317c) SHA1(b7df724434b0e7ef2af198a56021d825f701cf4d) )
S32X_ROM_LOAD( swap16,     "star wars arcade(prototype - sep 01, 1994).bin",                                   0x000000, 0x200000,   CRC(b0bb29eb) SHA1(37da621707fec11bb08d59cec1fa2c554365afec) )
S32X_ROM_LOAD( swap17,     "star wars arcade(prototype - aug 30, 1994).bin",                                   0x000000, 0x200000,   CRC(10d4a078) SHA1(8e90bb4b3bde14b819de85944080d4a3caf11c1b) )
S32X_ROM_LOAD( tmek,       "t-mek (usa, europe).bin",                                                          0x000000, 0x300000,   CRC(66d2c48f) SHA1(173c8425921d83db3e8d181158e7599364f4c0f6) )
S32X_ROM_LOAD( tempo,      "tempo (japan, usa).bin",                                                           0x000000, 0x300000,   CRC(14e5c575) SHA1(6673ba83570b4f2c1b4a22415a56594c3cc6c6a9) )
S32X_ROM_LOAD( tempop,     "tempo(prototype - feb 07, 1995).bin",                                              0x000000, 0x300000,   CRC(a2ebc91c) SHA1(91bca4f05b97b51b96fb1ddcb9b54165f0b621ae) )
S32X_ROM_LOAD( tempop1,    "tempo(prototype - feb 06, 1995).bin",                                              0x000000, 0x300000,   CRC(e2502fe6) SHA1(2e9724e9b7f2d351ad1ddfb34c43014aff4b07dd) )
S32X_ROM_LOAD( tempop2,    "tempo(prototype - feb 06, 1995 - b).bin",                                          0x000000, 0x300000,   CRC(6d0775f3) SHA1(7c29092fb82711851f1c681fa717cd59b98f531a) )
S32X_ROM_LOAD( tempop3,    "tempo(prototype - feb 04, 1995).bin",                                              0x000000, 0x300000,   CRC(6e2bcc9f) SHA1(73d31074f08d9dd8061bf0996829dbad46b3d784) )
S32X_ROM_LOAD( tempop4,    "tempo(prototype - jan 26, 1995).bin",                                              0x000000, 0x300000,   CRC(86637ae4) SHA1(4ec1dd3abf3e5aa0d1a902f20cfa0d16a224b5bf) )
S32X_ROM_LOAD( tempop5,    "tempo(prototype - jan 24, 1995).bin",                                              0x000000, 0x300000,   CRC(7a02c17b) SHA1(1b8225ae3e688ebdde482015a389a6cebbdbfd5e) )
S32X_ROM_LOAD( tempop6,    "tempo(prototype - jan 21, 1995).bin",                                              0x000000, 0x300000,   CRC(48cc1645) SHA1(55fd5d5410a820867526d5f7f909bf39d25607c2) )
S32X_ROM_LOAD( tempop7,    "tempo(prototype - dec 29, 1994).bin",                                              0x000000, 0x300000,   CRC(7bfe49a3) SHA1(017ee73e0f582893ef3deebfbe74af4babd532a5) )
S32X_ROM_LOAD( toughman,   "toughman contest (usa, europe).bin",                                               0x000000, 0x400000,   CRC(14eac7a6) SHA1(7588b0b8f4e93d5fdc920d3ab7e464154e423da9) )
S32X_ROM_LOAD( vf,         "virtua fighter (europe).bin",                                                      0x000000, 0x400000,   CRC(25aea73c) SHA1(24b3063284a914c76d8d1f681e3ed6323b0d7d0b) )
S32X_ROM_LOAD( vfju,       "virtua fighter (japan, usa).bin",                                                  0x000000, 0x400000,   CRC(b5de9626) SHA1(f35754f4bfe3a53722d7a799f88face0fd13c424) )
S32X_ROM_LOAD( vfp,        "virtua fighter(prototype - jul 25, 1995).bin",                                     0x000000, 0x400000,   CRC(aff4d320) SHA1(7b881a7849320187389d4af5bd4bf8d3ee6fd796) )
S32X_ROM_LOAD( vfp1,       "virtua fighter(prototype - jul 24, 1995).bin",                                     0x000000, 0x400000,   CRC(d4f5484b) SHA1(4600fb1c80c968728688269dfbd9c480d4d70abe) )
S32X_ROM_LOAD( vfp2,       "virtua fighter(prototype - jul 21, 1995).bin",                                     0x000000, 0x400000,   CRC(1c4c13f3) SHA1(d26708a26a3add9e4d6083e6b9ccd48776cd4c56) )
S32X_ROM_LOAD( vfp3,       "virtua fighter(prototype - jul 17, 1995).bin",                                     0x000000, 0x400000,   CRC(6a364bce) SHA1(c931cd60543b41e8bfb29511dcc70e4648471cbf) )
S32X_ROM_LOAD( vfp4,       "virtua fighter(prototype - jun 30, 1995).bin",                                     0x000000, 0x400000,   CRC(3b88f987) SHA1(cc6a95d10a95a91337f995a0120cb353196ce080) )
S32X_ROM_LOAD( vfp5,       "virtua fighter(prototype - jun 15, 1995).bin",                                     0x000000, 0x400000,   CRC(8df6401c) SHA1(28e99cc511e7c539fec66acbf5662f81528389d3) )
S32X_ROM_LOAD( vfp6,       "virtua fighter(prototype - may 30, 1995).bin",                                     0x000000, 0x400000,   CRC(46a88625) SHA1(fbb0d96c22465abb1702499f0a8c68228dcf8fc6) )
S32X_ROM_LOAD( vham,       "virtua hamster (usa) (proto).bin",                                                 0x000000, 0x200000,   CRC(cd54ac37) SHA1(c10f98d9db73ae90be5e5d556b663a5aff015cef) )
S32X_ROM_LOAD( vrdx,       "virtua racing deluxe (europe).bin",                                                0x000000, 0x300000,   CRC(27f14b5f) SHA1(bb26ddde26cd07191d872e8a76a170ca3326f781) )
S32X_ROM_LOAD( vrdxj,      "mpr-17503+mpr-17504.bin",                                                          0x000000, 0x300000,   CRC(0908e888) SHA1(1c64e80eb164ca8d6c6587a55ad60385ba1daf27) )
S32X_ROM_LOAD( vrdxu,      "virtua racing deluxe (usa).bin",                                                   0x000000, 0x300000,   CRC(7896b62e) SHA1(18dfdeb50780c2623e60a6587d7ed701a1cf81f1) )
S32X_ROM_LOAD( vrdxp,      "virtua racing deluxe(prototype - sep 05, 1994).bin",                               0x000000, 0x300000,   CRC(99f38344) SHA1(88b3ef2bb0ccba25f3f83f218ca090e1a89512fe) )
S32X_ROM_LOAD( vrdxp1,     "virtua racing deluxe(prototype - aug 29, 1994).bin",                               0x000000, 0x300000,   CRC(86a318de) SHA1(b1385111a39795bf2ea57ab50a4feb479d7e502d) )
S32X_ROM_LOAD( vrdxp2,     "virtua racing deluxe(prototype - aug 22, 1994).bin",                               0x000000, 0x300000,   CRC(6ebb3eda) SHA1(d4e996fd7fb26919017d1056d0de608a34f05184) )
S32X_ROM_LOAD( wseries,    "world series baseball starring deion sanders (usa).bin",                           0x000000, 0x300000,   CRC(6de1bc75) SHA1(ab3026eae46a775adb7eaebc13702699557ddc41) )
S32X_ROM_LOAD( wwfraw,     "wwf raw (world).bin",                                                              0x000000, 0x400000,   CRC(8eb7cd2c) SHA1(94b974f2f69f0c10bc18b349fa4ff95ca56fa47b) )
S32X_ROM_LOAD( wwfwre,     "wwf wrestlemania - the arcade game (usa).bin",                                     0x000000, 0x400000,   CRC(61833503) SHA1(551eedc963cba0e1410b3d229b332ef9ea061469) )
S32X_ROM_LOAD( xmen,       "x-men (usa) (proto).bin",                                                          0x000000, 0x400000,   CRC(d61febc0) SHA1(cd3fa2ace2fdbf4b6402cb52eeb208010bf31029) )
S32X_ROM_LOAD( zaxxon2k,   "parasquad ~ zaxxon's motherbase 2000 (japan, usa).bin",                            0x000000, 0x200000,   CRC(447d44be) SHA1(60c390f76c394bdd221936c21aecbf98aec49a3d) )
S32X_ROM_LOAD( zaxxon2kp,  "zaxxon's motherbase 2000(prototype - mar 31, 1995).bin",                           0x000000, 0x200000,   CRC(1344f08b) SHA1(ed3b31494407f324f18db4f5056ad6df75dc1a10) )
S32X_ROM_LOAD( zaxxon2kp1, "zaxxon's motherbase 2000(prototype - mar 15, 1995).bin",                           0x000000, 0x200000,   CRC(adfd5bc6) SHA1(ed9d3414edc90b7fb6a23732038f5c57710de9d3) )
S32X_ROM_LOAD( zaxxon2kp2, "zaxxon's motherbase 2000(prototype - mar 10, 1995).bin",                           0x000000, 0x200000,   CRC(20faf0c3) SHA1(df6898e46cba85541bc7f92090bfcf7000c0e648) )
S32X_ROM_LOAD( zaxxon2kp3, "zaxxon's motherbase 2000(prototype - mar 06, 1995).bin",                           0x000000, 0x200000,   CRC(90692a18) SHA1(67afaca4956df2120f62259b1147feaad6098a53) )
S32X_ROM_LOAD( zaxxon2kp4, "zaxxon's motherbase 2000(prototype - mar 03, 1995).bin",                           0x000000, 0x200000,   CRC(5856082d) SHA1(22dfc9a059657f45c7218d3be84c4a8c257ccc38) )
S32X_ROM_LOAD( zaxxon2kp5, "zaxxon's motherbase 2000(prototype - feb 27, 1995).bin",                           0x000000, 0x200000,   CRC(a1ef9787) SHA1(e498f84bea01f073a430d854b47306b7d35ffe08) )
S32X_ROM_LOAD( zaxxon2kp6, "zaxxon's motherbase 2000(prototype - feb 23, 1995).bin",                           0x000000, 0x200000,   CRC(e92b36c5) SHA1(6c93f432610908950b594bb8d289fe81de88fbdf) )
S32X_ROM_LOAD( zaxxon2kp7, "zaxxon's motherbase 2000(prototype - feb 21, 1995).bin",                           0x000000, 0x200000,   CRC(061c0cf2) SHA1(3a8c794139fe7255e7107877b2d0018a6eaa0300) )
S32X_ROM_LOAD( zaxxon2kp8, "zaxxon's motherbase 2000(prototype - feb 10, 1995).bin",                           0x000000, 0x200000,   CRC(c65693af) SHA1(9db5a93ae5fc4045df342a9f53b904ef130debbf) )


/*
    Notes on GameId:
    - first Id is the Japanese one, second Id is the USA one. European Ids are the same as USA with
      a -5x appended to the end, depending on the country/language (generic English titles have a -50)
 */


/*
    Undumped:
     - Motherbase (Euro)

    Known undumped prototypes:
     - Soul Star
     - Spot Goes to Hollywood

*/

SOFTWARE_LIST_START( _32x_cart )
	/* Id: GM4006, 84507 - Developers: Sega Development Division #2 / Rutubo Games - Releases: 1994 (USA), 1995 (Euro), 1995-01-13 (JPN) */
	SOFTWARE( afterb,      0,        1994, "Sega", "After Burner Complete (Euro)", 0, 0 )
	SOFTWARE( afterbju,    afterb,   1994, "Sega", "After Burner (USA) ~ After Burner Complete (Jpn)", 0, 0 )

	/* Id: 84517 - Developers: BlueSky Software / Zono - Releases: 1996 (USA) */
	SOFTWARE( spidwebf,    0,        1996, "Sega", "The Amazing Spider-Man - Web of Fire (USA)", 0, 0 )

	/* Id: T-7901B - Developers: Core Design - Releases: 1995 (USA)  */
	SOFTWARE( bcracers,    0,        1995, "Front Street Publishing", "BC Racers (USA)", 0, 0 )

	/* Id: 84519 - Developers: Blizzard Entertainment / Paradox Development - Releases: 1995 (USA)  */
	SOFTWARE( blackthr,    0,        1994, "Sega", "Blackthorne (USA)", 0, 0 )

	/* Id: T-8301B - Developers: Alternative Reality Technologies - Releases: 1995 (USA) */
	SOFTWARE( brutal,      0,        1995, "GameTek", "Brutal - Above the Claw (USA)", 0, 0 )

	/* Id: GM4004, 84700 - Developers: Almanic - Releases: 1994 (Euro), 1995 (USA), 1995-01-27 (JPN)  */
	SOFTWARE( cosmicc,     0,        1994, "Sega", "Cosmic Carnage (Euro)", 0, 0 )
	SOFTWARE( cyberb,      cosmicc,  1994, "Sega", "Cyber Brawl (Jpn) ~ Cosmic Carnage (USA)", 0, 0 )
	SOFTWARE( cosmiccp,    cosmicc,  1994, "Sega", "Cosmic Carnage (Prototype, 19941011)", 0, 0 )
	SOFTWARE( cosmiccp1,   cosmicc,  1994, "Sega", "Cosmic Carnage (Prototype, 19941007)", 0, 0 )
	SOFTWARE( cosmiccp2,   cosmicc,  1994, "Sega", "Cosmic Carnage (Prototype, 19941005)", 0, 0 )
	SOFTWARE( cosmiccp3,   cosmicc,  1994, "Sega", "Cosmic Carnage (Prototype, 19941004)", 0, 0 )
	SOFTWARE( cosmiccp4,   cosmicc,  1994, "Sega", "Cosmic Carnage (Prototype, 19940928)", 0, 0 )
	SOFTWARE( cosmiccp5,   cosmicc,  1994, "Sega", "Cosmic Carnage (Prototype, 19940926)", 0, 0 )
	SOFTWARE( cosmiccp6,   cosmicc,  1994, "Sega", "Cosmic Carnage (Prototype, 19940921)", 0, 0 )
	SOFTWARE( cosmiccp7,   cosmicc,  1994, "Sega", "Cosmic Carnage (Prototype, 19940906)", 0, 0 )

	/* Id: 84580-50 - Developers: Frontier Developments - Releases: 1995 (Euro)  */
	SOFTWARE( darxide,     0,        1995, "Sega", "Darxide (Euro)", 0, 0 )

	/* Id: GM4003, 3284506 - Developers: id Software / Sega of America - Releases: 1994 (USA, EUR), 1994-12-03 (JPN)  */
	SOFTWARE( doom,        0,        1994, "Sega", "Doom (Euro)", 0, 0 )
	SOFTWARE( doomju,      doom,     1994, "Sega", "Doom (Jpn, USA)", 0, 0 )
	SOFTWARE( doomp,       doom,     1994, "Sega", "Doom (Prototype, 19941201)", 0, 0 )
	SOFTWARE( doompa,      doom,     1994, "Sega", "Doom (Prototype, 19941002)", 0, 0 )
	SOFTWARE( doompb,      doom,     1994, "Sega", "Doom (Prototype, 19940928)", 0, 0 )
	SOFTWARE( doompc,      doom,     1994, "Sega", "Doom (Prototype, 19940927)", 0, 0 )
	SOFTWARE( doompd,      doom,     1994, "Sega", "Doom (Prototype, 19940925)", 0, 0 )
	SOFTWARE( doompe,      doom,     1994, "Sega", "Doom (Prototype, 19940923)", 0, 0 )
	SOFTWARE( doompf,      doom,     1994, "Sega", "Doom (Prototype, 19940921)", 0, 0 )
	SOFTWARE( doompg,      doom,     1994, "Sega", "Doom (Prototype, 19940916)", 0, 0 )
	SOFTWARE( doomph,      doom,     1994, "Sega", "Doom (Prototype, 19940914)", 0, 0 )
	SOFTWARE( doompi,      doom,     1994, "Sega", "Doom (Prototype, 19940909)", 0, 0 )
	SOFTWARE( doompj,      doom,     1994, "Sega", "Doom (Prototype, 19940906)", 0, 0 )

	/* Id: ?? - Developers: ?? - Releases: -  */
	SOFTWARE( doomrr,      doom,     1995, "Sega", "Doom RR (Prototype, 19950307)", 0, 0 )
	SOFTWARE( doomrr1,     doom,     1995, "Sega", "Doom RR (Prototype, 19950221)", 0, 0 )
	SOFTWARE( doomrr2,     doom,     1995, "Sega", "Doom RR (Prototype, 19950215)", 0, 0 )

	/* Id: ?? - Developers: ?? - Releases: -  */
	SOFTWARE( eccodemo,    0,        199?, "<unknown>", "ECCO the Dolphin CinePak Demo (Jpn, USA, Developer Cart)", 0, 0 )

	/* Id: T-5002B - Developers: Extended Play / Probe - Releases: 1995 (Euro)  */
	SOFTWARE( fifa96,      0,        1995, "Electronic Arts", "FIFA Soccer 96 (Euro)", 0, 0 )

	/* Id: GM5002, 84602 - Developers: Flashpoint Productions - Releases: 1994 (USA), 1995 (Euro), 1995-02-24 (JPN) */
	SOFTWARE( 36great,     0,        1994, "Sega", "36 Great Holes Starring Fred Couples (Euro)", 0, 0 )
	SOFTWARE( 36greatju,   36great,  1994, "Sega", "36 Great Holes Starring Fred Couples (Jpn, USA)", 0, 0 )
	SOFTWARE( 36greatp,    36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941221-B)", 0, 0 )
	SOFTWARE( 36greatp1,   36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941221)", 0, 0 )
	SOFTWARE( 36greatp2,   36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941219)", 0, 0 )
	SOFTWARE( 36greatp3,   36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941214)", 0, 0 )
	SOFTWARE( 36greatp4,   36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941213)", 0, 0 )
	SOFTWARE( 36greatp5,   36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941206)", 0, 0 )
	SOFTWARE( 36greatp6,   36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941203)", 0, 0 )
	SOFTWARE( 36greatp7,   36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941202)", 0, 0 )
	SOFTWARE( 36greatp8,   36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941130)", 0, 0 )
	SOFTWARE( 36greatp9,   36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941129)", 0, 0 )
	SOFTWARE( 36greatp10,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941128-B)", 0, 0 )
	SOFTWARE( 36greatp11,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941128)", 0, 0 )
	SOFTWARE( 36greatp12,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941127)", 0, 0 )
	SOFTWARE( 36greatp13,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941126)", 0, 0 )
	SOFTWARE( 36greatp14,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941123)", 0, 0 )
	SOFTWARE( 36greatp15,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941122-B)", 0, 0 )
	SOFTWARE( 36greatp16,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941122)", 0, 0 )
	SOFTWARE( 36greatp17,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941121)", 0, 0 )
	SOFTWARE( 36greatp18,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941119-B)", 0, 0 )
	SOFTWARE( 36greatp19,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941119)", 0, 0 )
	SOFTWARE( 36greatp20,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941118)", 0, 0 )
	SOFTWARE( 36greatp21,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941116)", 0, 0 )
	SOFTWARE( 36greatp22,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941110)", 0, 0 )
	SOFTWARE( 36greatp23,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941108)", 0, 0 )
	SOFTWARE( 36greatp24,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941107)", 0, 0 ) // this is the earliest set that doesn't just crash it
	SOFTWARE( 36greatp25,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941105)", 0, 0 )
	SOFTWARE( 36greatp26,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941103)", 0, 0 )
	SOFTWARE( 36greatp27,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941101)", 0, 0 )
	SOFTWARE( 36greatp28,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941026)", 0, 0 )
	SOFTWARE( 36greatp29,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941024)", 0, 0 )
	SOFTWARE( 36greatp30,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941018)", 0, 0 )
	SOFTWARE( 36greatp31,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941017)", 0, 0 )
	SOFTWARE( 36greatp32,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (Prototype, 19941011)", 0, 0 )
	SOFTWARE( 36greatp33,  36great,  1994, "Sega", "36 Great Holes starring Fred Couples (v1.x Prototype, 19940706)", 0, 0 )

	/* Id: GM5003, 84503 - Developers: Sega AM7 (CS2) - Releases: 1995 (USA, EUR), 1995-04-21 (JPN)  */
	SOFTWARE( knuckle,     0,        1995, "Sega", "Knuckles' Chaotix (Euro)", 0, 0 )
	SOFTWARE( chaotix,     knuckle,  1995, "Sega", "Chaotix (Jpn) ~ Knuckles' Chaotix (USA)", 0, 0 )
	SOFTWARE( knucklep,    knuckle,  1995, "Sega", "Knuckles' Chaotix (Prototype 214, 19950214, 06.46)", 0, 0 )
	SOFTWARE( knucklep1,   knuckle,  1995, "Sega", "Knuckles' Chaotix (Prototype 0213, 19950213, 07.30)", 0, 0 )
	SOFTWARE( knucklep2,   knuckle,  1995, "Sega", "Knuckles' Chaotix (Prototype 213B, 19950213, 06.46)", 0, 0 )
	SOFTWARE( knucklep3,   knuckle,  1995, "Sega", "Knuckles' Chaotix (Prototype 210, 19950210, 06.28)", 0, 0 )
	SOFTWARE( knucklep4,   knuckle,  1995, "Sega", "Knuckles' Chaotix (Prototype 209, 19950209, 08.25)", 0, 0 )
	SOFTWARE( knucklep5,   knuckle,  1995, "Sega", "Knuckles' Chaotix (Prototype 208, 19950208, 11.17)", 0, 0 )
	SOFTWARE( knucklep6,   knuckle,  1995, "Sega", "Knuckles' Chaotix (Prototype 0202, 19950207, 15.25)", 0, 0 )
	SOFTWARE( knucklep7,   knuckle,  1995, "Sega", "Knuckles' Chaotix (Prototype 119, 19950119, 07.04)", 0, 0 )
	SOFTWARE( knucklep8,   knuckle,  1995, "Sega", "Knuckles' Chaotix (Prototype 0111, 19950112, 09.36)", 0, 0 )
	SOFTWARE( knucklep9,   knuckle,  1995, "Sega", "Knuckles' Chaotix (Prototype 1207, 19941207, 07.15)", 0, 0 )
	SOFTWARE( knucklep10,  knuckle,  1995, "Sega", "Knuckles' Chaotix (Prototype 1227, 19941227, 10.28)", 0, 0 )
	SOFTWARE( knucklep11,  knuckle,  1995, "Sega", "Knuckles' Chaotix (Prototype 1229, 19941230, 15.31)", 0, 0 )

	/* Id: 84518 - Developers: Novotrade International - Releases: 1995 (USA, EUR) */
	SOFTWARE( kolibri,     0,        1995, "Sega", "Kolibri (Euro, USA)", 0, 0 )

	/* Id: ?? - Developers: ?? - Releases: -  */
	SOFTWARE( marsch,      0,        199?, "<unknown>", "Mars Check Program Version 1.0 (Jpn, USA, SDK Build Set 1)", 0, 0 )
	SOFTWARE( marsch1,     marsch,   199?, "<unknown>", "Mars Check Program Version 1.0 (Jpn, USA, SDK Build Set 2)", 0, 0 )

	/* Id: ?? - Developers: ?? - Releases: -  */
	SOFTWARE( marssa,      0,        199?, "<unknown>", "Mars Sample Program - Gnu Sierra (Jpn, USA, SDK Build)", 0, 0 )
	SOFTWARE( marssa1,     marssa,   199?, "<unknown>", "Mars Sample Program - Pharaoh (Jpn, USA, SDK Build)", 0, 0 )
	SOFTWARE( marssa2,     marssa,   199?, "<unknown>", "Mars Sample Program - Runlength Mode Test (Jpn, USA, SDK Build)", 0, 0 )
	SOFTWARE( marssa3,     marssa,   199?, "<unknown>", "Mars Sample Program - Texture Test (Jpn, USA, SDK Build)", 0, 0 )

	/* Id: GM4008, 84511 - Developers: Sega - Releases: 1995 (USA, EUR), 1995-02-24 (JPN) */
	SOFTWARE( metalhd,     0,        1995, "Sega", "Metal Head (Euro)", 0, 0 )
	SOFTWARE( metalhdju,   metalhd,  1995, "Sega", "Metal Head (Jpn, USA)", 0, 0 )
	SOFTWARE( metalhdp,    metalhd,  1995, "Sega", "Metal Head (Prototype, 19941220)", 0, 0 )
	SOFTWARE( metalhdp1,   metalhd,  1995, "Sega", "Metal Head (Prototype, 19941213)", 0, 0 )
	SOFTWARE( metalhdp2,   metalhd,  1995, "Sega", "Metal Head (Prototype, 19941209)", 0, 0 )
	SOFTWARE( metalhdp3,   metalhd,  1995, "Sega", "Metal Head (Prototype, 19941207-B)", 0, 0 )
	SOFTWARE( metalhdp4,   metalhd,  1995, "Sega", "Metal Head (Prototype, 19941207)", 0, 0 )
	SOFTWARE( metalhdp5,   metalhd,  1995, "Sega", "Metal Head (Prototype, 19941202)", 0, 0 )
	SOFTWARE( metalhdp6,   metalhd,  1995, "Sega", "Metal Head (Prototype, 19941121)", 0, 0 )
	SOFTWARE( metalhdp7,   metalhd,  1995, "Sega", "Metal Head (Prototype, 19941118)", 0, 0 )
	SOFTWARE( metalhdp8,   metalhd,  1995, "Sega", "Metal Head (Prototype, 19941114)", 0, 0 )

	/* Id: T-8104A, T-8101B - Developers: Midway / Probe - Releases: 1994 (USA), 1995 (Euro), 1995-05-19 (JPN)  */
	SOFTWARE( mk2,         0,        1994, "Acclaim Entertainment", "Mortal Kombat II (Euro)", 0, 0 )
	SOFTWARE( mk2ju,       mk2,      1994, "Acclaim Entertainment", "Mortal Kombat II: Kyuukyoku Shinken (Jpn) ~ Mortal Kombat II (USA)", 0, 0 )

	/* Id: 84600 - Developers: Artech Studios - Releases: 1995 (USA, EUR) */
	SOFTWARE( motox,      0,         1994, "Sega", "Motocross Championship (Euro)", 0, 0 )
	SOFTWARE( motoxu,     motox,     1994, "Sega", "Motocross Championship (USA)", 0, 0 )
	SOFTWARE( motoxp,     motox,     1994, "Sega", "Motocross Championship (Prototype, 19941111)", 0, 0 )
	SOFTWARE( motoxpa,    motox,     1994, "Sega", "Motocross Championship (Prototype, 19941108)", 0, 0 )
	SOFTWARE( motoxpb,    motox,     1994, "Sega", "Motocross Championship (Prototype, 19941107)", 0, 0 )
	SOFTWARE( motoxpc,    motox,     1994, "Sega", "Motocross Championship (Prototype, 19941104)", 0, 0 )
	SOFTWARE( motoxpd,    motox,     1994, "Sega", "Motocross Championship (Prototype, 19941103)", 0, 0 )
	SOFTWARE( motoxpe,    motox,     1994, "Sega", "Motocross Championship (Prototype, 19941102-B)", 0, 0 )
	SOFTWARE( motoxpf,    motox,     1994, "Sega", "Motocross Championship (Prototype, 19941102)", 0, 0 )
	SOFTWARE( motoxpg,    motox,     1994, "Sega", "Motocross Championship (Prototype, 19941101)", 0, 0 )
	SOFTWARE( motoxph,    motox,     1994, "Sega", "Motocross Championship (Prototype, 19941028)", 0, 0 )
	SOFTWARE( motoxpi,    motox,     1994, "Sega", "Motocross Championship (Prototype, 19941024)", 0, 0 )
	SOFTWARE( motoxpj,    motox,     1994, "Sega", "Motocross Championship (Prototype, 19941020)", 0, 0 )
	SOFTWARE( motoxpk,    motox,     1994, "Sega", "Motocross Championship (Prototype, 19941019)", 0, 0 )
	SOFTWARE( motoxpl,    motox,     1994, "Sega", "Motocross Championship (Prototype, 19941018)", 0, 0 )
	SOFTWARE( motoxpm,    motox,     1994, "Sega", "Motocross Championship (Prototype, 19941017)", 0, 0 )
	SOFTWARE( motoxpn,    motox,     1994, "Sega", "Motocross Championship (Prototype, 19941012)", 0, 0 )

	/* Id: T-8102A, T-8104B - Developers: Midway / Iguana UK - Releases: 1995 (USA, EUR), 1995-09-01 (JPN)  */
	SOFTWARE( nbajamte,   0,         1995, "Acclaim Entertainment", "NBA Jam T.E. - Tournament Edition (World)", 0, 0 )

	/* Id: T-8103A, T-8102B - Developers: Iguana Entertainment - Releases: 1995 (USA, EUR), 1995-07-14 (JPN)  */
	SOFTWARE( nflquart,   0,         1995, "Acclaim Entertainment", "NFL Quarterback Club ~ NFL Quarterback '95 (World)", 0, 0 )

	/* Id: T-13001B - Developers: Activision / Big Bang Software / Zombie Inc. - Releases: 1995 (USA)  */
	SOFTWARE( pitfall,    0,         1995, "Activision", "Pitfall: The Mayan Adventure (USA)", 0, 0 )

	/* Id: 84705 - Developers: Atari / Probe - Releases: 1995 (USA), 1996 (Euro) */
	SOFTWARE( primal,     0,         1996, "Sega", "Primal Rage (Euro, USA)", 0, 0 )

	/* Id: T-4803F - Developers: Time Warner Interactive - Releases: 1995 (USA)  */
	SOFTWARE( rbi95,      0,         1995, "Time Warner Interactive", "RBI Baseball '95 (USA)", 0, 0 )

	/* Id: T-7601A - Developers: Koei - Releases: 1994-07-28 (JPN)  */
	SOFTWARE( sangoku4,   0,         1994, "Koei", "Sangokushi IV (Jpn)", 0, 0 )

	/* Id: GM4010, 84509 - Developers: Sega - Releases: 1994 (EUR, USA), 1995-04-26 (JPN) */
	SOFTWARE( stellar,    0,         1995, "Sega", "Shadow Squadron (USA) ~ Stellar Assault (Euro)", 0, 0 )
	SOFTWARE( stellarj,   stellar,   1995, "Sega", "Stellar Assault (Jpn)", 0, 0 )
	SOFTWARE( shadsqp,    stellar,   1995, "Sega", "Shadow Squadron (Prototype, 19950313-B)", 0, 0 )
	SOFTWARE( shadsqp1,   stellar,   1995, "Sega", "Shadow Squadron (Prototype, 19950313)", 0, 0 )
	SOFTWARE( shadsqp2,   stellar,   1995, "Sega", "Shadow Squadron (Prototype, 19950306)", 0, 0 )
	SOFTWARE( shadsqp3,   stellar,   1995, "Sega", "Shadow Squadron (Prototype, 19950302)", 0, 0 )
	SOFTWARE( shadsqp4,   stellar,   1995, "Sega", "Shadow Squadron (Prototype, 19950222)", 0, 0 )
	SOFTWARE( shadsqp5,   stellar,   1995, "Sega", "Shadow Squadron (Prototype, 19950213)", 0, 0 )
	SOFTWARE( shadsqp6,   stellar,   1995, "Sega", "Shadow Squadron (Prototype, 19950206)", 0, 0 )

	/* Id: GM4005, 84505 - Developers: Sega Development Division #2 / Rutubo Games - Releases: 1994 (EUR, USA), 1995-01-13 (JPN) */
	SOFTWARE( sharrier,   0,         1994, "Sega", "Space Harrier (Euro)", 0, 0 )
	SOFTWARE( sharrierju, sharrier,  1994, "Sega", "Space Harrier (Jpn, USA)", 0, 0 )
	SOFTWARE( sharrierp,  sharrier,  1994, "Sega", "Space Harrier (Prototype, 19940920)", 0, 0 )

	/* Id: 84521 - Developers: Interplay / High Voltage Software - Releases: 1995 (USA) */
	SOFTWARE( startr,     0,         1995, "Sega", "Star Trek: Starfleet Academy - Starship Bridge Simulator (USA)", 0, 0 )

	/* Id: GM4002, 84508 - Developers: Sega AM3 / Sega interActive - Releases: 1994 (EUR, USA), 1994-12-03 (JPN) */
	SOFTWARE( swa,        0,         1994, "Sega", "Star Wars Arcade (Euro)", 0, 0 )
	SOFTWARE( swaj,       swa,       1994, "Sega", "Star Wars Arcade (Jpn)", 0, 0 )
	SOFTWARE( swau,       swa,       1994, "Sega", "Star Wars Arcade (USA)", 0, 0 )
	SOFTWARE( swap,       swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19941007-B)", 0, 0 )
	SOFTWARE( swap1,      swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19941006-P)", 0, 0 )
	SOFTWARE( swap2,      swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19941005)", 0, 0 )
	SOFTWARE( swap3,      swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19941003)", 0, 0 )
	SOFTWARE( swap4,      swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19940930-B)", 0, 0 )
	SOFTWARE( swap5,      swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19940929)", 0, 0 )
	SOFTWARE( swap6,      swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19940923)", 0, 0 )
	SOFTWARE( swap7,      swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19940918)", 0, 0 )
	SOFTWARE( swap8,      swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19940916)", 0, 0 )
	SOFTWARE( swap9,      swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19940915)", 0, 0 )
	SOFTWARE( swap10,     swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19940914-D)", 0, 0 )
	SOFTWARE( swap11,     swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19940913-B)", 0, 0 )
	SOFTWARE( swap12,     swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19940912)", 0, 0 )
	SOFTWARE( swap13,     swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19940909)", 0, 0 )
	SOFTWARE( swap14,     swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19940907)", 0, 0 )
	SOFTWARE( swap15,     swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19940906)", 0, 0 )
	SOFTWARE( swap16,     swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19940901)", 0, 0 )
	SOFTWARE( swap17,     swa,       1994, "Sega", "Star Wars Arcade (Prototype, 19940830)", 0, 0 )

	/* Id: 84520 - Developers: Atari / Bits Studios - Releases: 1995 (USA), 1996 (Euro) */
	SOFTWARE( tmek,       0,         1995, "Sega", "T-Mek (Euro, USA)", 0, 0 )

	/* Id: GM4009, 84504 - Developers: Sega AM6 (CS1) - Releases: 1995 (USA), 1995-03-24 (JPN) */
	SOFTWARE( tempo,      0,         1995, "Sega", "Tempo (Jpn, USA)", 0, 0 )
	SOFTWARE( tempop,     tempo,     1995, "Sega", "Tempo (Prototype, 19950207)", 0, 0 )
	SOFTWARE( tempop1,    tempo,     1995, "Sega", "Tempo (Prototype, 19950206)", 0, 0 )
	SOFTWARE( tempop2,    tempo,     1995, "Sega", "Tempo (Prototype, 19950206-B)", 0, 0 )
	SOFTWARE( tempop3,    tempo,     1995, "Sega", "Tempo (Prototype, 19950204)", 0, 0 )
	SOFTWARE( tempop4,    tempo,     1995, "Sega", "Tempo (Prototype, 19950126)", 0, 0 )
	SOFTWARE( tempop5,    tempo,     1995, "Sega", "Tempo (Prototype, 19950124)", 0, 0 )
	SOFTWARE( tempop6,    tempo,     1995, "Sega", "Tempo (Prototype, 19950121)", 0, 0 )
	SOFTWARE( tempop7,    tempo,     1995, "Sega", "Tempo (Prototype, 19941229)", 0, 0 )

	/* Id: T-5001B - Developers: Visual Concepts - Releases: 1995 (EUR, USA)  */
	SOFTWARE( toughman,   0,         1995, "Electronic Arts", "Toughman Contest (Euro, USA)", 0, 0 )

	/* Id: GM4013, 84701 - Developers: Sega AM2 / Sega - Releases: 1995 (EUR, USA), 1995-10-20 (JPN) */
	SOFTWARE( vf,         0,         1995, "Sega", "Virtua Fighter (Euro)", 0, 0 )
	SOFTWARE( vfju,       vf,        1995, "Sega", "Virtua Fighter (Jpn, USA)", 0, 0 )
	SOFTWARE( vfp,        vf,        1995, "Sega", "Virtua Fighter (Prototype, 19950725)", 0, 0 )
	SOFTWARE( vfp1,       vf,        1995, "Sega", "Virtua Fighter (Prototype, 19950724)", 0, 0 )
	SOFTWARE( vfp2,       vf,        1995, "Sega", "Virtua Fighter (Prototype, 19950721)", 0, 0 )
	SOFTWARE( vfp3,       vf,        1995, "Sega", "Virtua Fighter (Prototype, 19950717)", 0, 0 )
	SOFTWARE( vfp4,       vf,        1995, "Sega", "Virtua Fighter (Prototype, 19950630)", 0, 0 )
	SOFTWARE( vfp5,       vf,        1995, "Sega", "Virtua Fighter (Prototype, 19950615)", 0, 0 )
	SOFTWARE( vfp6,       vf,        1995, "Sega", "Virtua Fighter (Prototype, 19950530)", 0, 0 )

	/* Id: ?? - Developers: David A. Palmer Productions - Releases: -  */
	SOFTWARE( vham,       0,         1994, "<unknown>", "Virtua Hamster (Usa, Prototype)", 0, 0 )

	/* Id: GM5001, 84601 - Developers: Sega AM2 - Releases: 1994 (EUR, USA), 1994-12-16 (JPN) */
	SOFTWARE( vrdx,       0,         1994, "Sega", "Virtua Racing Deluxe (Euro)", 0, 0 )
	SOFTWARE( vrdxj,      vrdx,      1994, "Sega", "Virtua Racing Deluxe (Jpn)", 0, 0 )
	SOFTWARE( vrdxu,      vrdx,      1994, "Sega", "Virtua Racing Deluxe (USA)", 0, 0 )
	SOFTWARE( vrdxp,      vrdx,      1994, "Sega", "Virtua Racing Deluxe (Prototype, 19940905)", 0, 0 )
	SOFTWARE( vrdxp1,     vrdx,      1994, "Sega", "Virtua Racing Deluxe (Prototype, 19940829)", 0, 0 )
	SOFTWARE( vrdxp2,     vrdx,      1994, "Sega", "Virtua Racing Deluxe (Prototype, 19940822)", 0, 0 )

	/* Id: 84605 - Developers: BlueSky Software - Releases: 1995 (USA) */
	SOFTWARE( wseries,    0,         1995, "Sega", "World Series Baseball Starring Deion Sanders (USA)", 0, 0 )

	/* Id: T-8101A, T-8103B - Developers: Sculptured Software - Releases: 1995 (EUR, USA), 1995-09-01 (JPN)  */
	SOFTWARE( wwfraw,     0,         1994, "Acclaim Entertainment", "WWF Raw (World)", 0, 0 )

	/* Id: T-8110B - Developers: Sculptured Software - Releases: 1995 (USA)  */
	SOFTWARE( wwfwre,     0,         1995, "Acclaim Entertainment", "WWF WrestleMania: The Arcade Game (USA)", 0, 0 )

	/* Id: ?? - Developers: Scavenger - Releases: -  */
	SOFTWARE( xmen,       0,         1995, "<unknown>", "X-Men (Usa, Prototype)", 0, 0 )

	/* Id: GM4012, 84512 - Developers: CRI - Releases: 1995 (EUR, USA), 1995-07-14 (JPN) */
	SOFTWARE( zaxxon2k,   0,          1995, "Sega", "Parasquad (Jpn) ~ Zaxxon's Motherbase 2000 (USA)", 0, 0 )
	SOFTWARE( zaxxon2kp,  zaxxon2k,   1995, "Sega", "Zaxxon's Motherbase 2000 (Prototype, 19950331)", 0, 0 )
	SOFTWARE( zaxxon2kp1, zaxxon2k,   1995, "Sega", "Zaxxon's Motherbase 2000 (Prototype, 19950315)", 0, 0 )
	SOFTWARE( zaxxon2kp2, zaxxon2k,   1995, "Sega", "Zaxxon's Motherbase 2000 (Prototype, 19950310)", 0, 0 )
	SOFTWARE( zaxxon2kp3, zaxxon2k,   1995, "Sega", "Zaxxon's Motherbase 2000 (Prototype, 19950306)", 0, 0 )
	SOFTWARE( zaxxon2kp4, zaxxon2k,   1995, "Sega", "Zaxxon's Motherbase 2000 (Prototype, 19950303)", 0, 0 )
	SOFTWARE( zaxxon2kp5, zaxxon2k,   1995, "Sega", "Zaxxon's Motherbase 2000 (Prototype, 19950227)", 0, 0 )
	SOFTWARE( zaxxon2kp6, zaxxon2k,   1995, "Sega", "Zaxxon's Motherbase 2000 (Prototype, 19950223)", 0, 0 )
	SOFTWARE( zaxxon2kp7, zaxxon2k,   1995, "Sega", "Zaxxon's Motherbase 2000 (Prototype, 19950221)", 0, 0 )
	SOFTWARE( zaxxon2kp8, zaxxon2k,   1995, "Sega", "Zaxxon's Motherbase 2000 (Prototype, 19950210)", 0, 0 )

SOFTWARE_LIST_END


SOFTWARE_LIST( _32x_cart, "Sega 32X cartridges" )


#if 0

// test software

S32X_ROM_LOAD( 32xbab, "32x babe picture by fonzie(pd) [a1].bin",                                            0x000000, 0x14f80,   CRC(7c0587f6) SHA1(524093f7c93f07ec117f2a6c6a61e4da93ea1549) )
S32X_ROM_LOAD( 32xba1, "32x babe picture by fonzie(pd).bin",                                                 0x000000, 0x14f80,   CRC(816b0cb4) SHA1(dc16d3170d5809b57192e03864b7136935eada64) )
S32X_ROM_LOAD( 32xqin, "32x qinter demo by fonzie(pd).bin",                                                  0x000000, 0x5d124,   CRC(93d4b0a3) SHA1(128bd0b6e048c749da1a2f4c3abd6a867539a293) )
S32X_ROM_LOAD( 32xsam, "32x sample program - pwm sound demo (usa) (sdk build).bin",                          0x000000, 0x80000,   CRC(542ad12e) SHA1(b2b7938d5c9a207efecf9f5054308f5772c8c25f) )
S32X_ROM_LOAD( backan, "back and forth rotating xor palette by devster(pd).bin",                             0x000000, 0x1258,   CRC(4e7ac3d4) SHA1(87f999f629feb873f8916483ab76152a88a9af53) )
S32X_ROM_LOAD( backwa, "backward rotating xor palette demo by devster(pd).bin",                              0x000000, 0x1228,   CRC(d4ae89da) SHA1(58b0e798329df3591a08c21492a8967ad20736c6) )
S32X_ROM_LOAD( devste, "devster owns! text demo(pd).bin",                                                    0x000000, 0x1064,   CRC(03f9f674) SHA1(96b92cca59a0e9005dc958d96207cf71a91cf0b9) )
S32X_ROM_LOAD( greenr, "green rotating no stretching xor palette demo by devster(pd).bin",                   0x000000, 0x12a58,   CRC(84251f0a) SHA1(4582aa4309baffe80a087efe50be4644e7d3b64d) )
S32X_ROM_LOAD( green1, "green rotating xor palette demo by devster(pd).bin",                                 0x000000, 0x1258,   CRC(e075ece6) SHA1(3c211234782c9e97e76f2dba587e879272b123af) )
S32X_ROM_LOAD( hotchi, "hot chick by devster(pd).bin",                                                       0x000000, 0x1235c,   CRC(da9c93c9) SHA1(a62652eb8ad8c62b36f6b1ffb96922d045c4e3ac) )
S32X_ROM_LOAD( hotch1, "hot chick drawn without the sh2s for emulators by devster(pd).bin",                  0x000000, 0x12fdc,   CRC(d581910d) SHA1(7d488bfd1bbd399d305da1b6cbf87edc882f3ed0) )
S32X_ROM_LOAD( hotch2, "hot chick drawn without the sh2s for hardware by devster(pd).bin",                   0x000000, 0x1303c,   CRC(02d37b32) SHA1(c490a19423c609ca3e9f89b21801de2307e4f87a) )
S32X_ROM_LOAD( hotch3, "hot chick in high quality 15bpp by devster(pd).bin",                                 0x000000, 0x24564,   CRC(938f4e1d) SHA1(ab7270121be53c6c82c4cb45f8f41dd24eb3a2a5) )
S32X_ROM_LOAD( hotch4, "hot chick in vdp mode 2 by devster(pd).bin",                                         0x000000, 0x12d58,   CRC(cbf2f87e) SHA1(376e67044c08373e49fd79892392a6a3a20059ef) )
S32X_ROM_LOAD( hotch5, "hot chick in vdp mode 3 by devster(pd).bin",                                         0x000000, 0x12d58,   CRC(f2758266) SHA1(bc16856dbcd88a2ad742ed636b35f405da8e9af2) )
S32X_ROM_LOAD( hotch6, "hot chick spinning demo by devster(pd).bin",                                         0x000000, 0x12c28,   CRC(73b25607) SHA1(86cf6c88ccedefeb0b691c3f9aab5b624c0120d5) )
S32X_ROM_LOAD( hotch7, "hot chick spinning slower demo by devster(pd).bin",                                  0x000000, 0x12c28,   CRC(3d1d1191) SHA1(221a74408653e18cef8ce2f9b4d33ed93e4218b7) )
S32X_ROM_LOAD( hotch8, "hot chick with genesis vdp overlay by devster(pd).bin",                              0x000000, 0x12d58,   CRC(817b7656) SHA1(99ee4fded53e4856a633676b0c6f83081186492c) )
S32X_ROM_LOAD( hotch9, "hot chick zoom shot by devster(pd).bin",                                             0x000000, 0x13230,   CRC(e6dd4f0c) SHA1(a7e3eb018063cd9470f25ca74dad6603b913e0c7) )
S32X_ROM_LOAD( optimi, "optimized rotating xor palette demo by devster(pd).bin",                             0x000000, 0x1238,   CRC(941351aa) SHA1(f42d0f5126f17641e59fcb4c18f2cc8d4488a27c) )
S32X_ROM_LOAD( rotati, "rotating no background fill xor palette demo by devster(pd).bin",                    0x000000, 0x11f8,   CRC(7fce8c27) SHA1(a6a0a26f7e0244265948b3360f0a8c37d56846bc) )
S32X_ROM_LOAD( rotat1, "rotating pixel skipping xor palette demo by devster(pd).bin",                        0x000000, 0x1228,   CRC(1b394d4f) SHA1(11b9912776e5331c9ffa93d45ca8f91586173b0b) )
S32X_ROM_LOAD( rotat2, "rotating xor palette demo by devster(pd).bin",                                       0x000000, 0x1638,   CRC(98c25033) SHA1(8d9ab3084bd29e60b8cdf4b9f1cb755eb4c88d29) )
S32X_ROM_LOAD( superx, "super-x raycasting engine test 1 by fonzie(pd).bin",                                 0x000000, 0x42e8,   CRC(3316c17b) SHA1(cb5d98ec0998c718f0778cc131ebbbbfc72edca7) )
S32X_ROM_LOAD( super1, "super-x raycasting engine test 2 by fonzie(pd).bin",                                 0x000000, 0x5aa8,   CRC(19f8fa8f) SHA1(dec8238b0d5c95bb4c742d5a57ac23d42dd25047) )
S32X_ROM_LOAD( switch, "switching cram palettes by devster(pd).bin",                                         0x000000, 0x8cc,   CRC(007cd5f2) SHA1(b28cfbd400e70acf0bee3ba9ee1793f195cfbdd6) )
S32X_ROM_LOAD( xorpal, "xor palette demo by devster(pd).bin",                                                0x000000, 0xd48,   CRC(c6428bb2) SHA1(75fd855f3a70e411ad28a483a3f5fb4da1ebf4b6) )

	SOFTWARE( 32xbab, 0,       199?, "Homebrew", "32X Babe Picture by Fonzie(PD) [a1]", 0, 0 )

	SOFTWARE( 32xba1, 32xbab, 199?, "Homebrew", "32X Babe Picture by Fonzie(PD)", 0, 0 )

	SOFTWARE( 32xqin, 0,       199?, "Homebrew", "32X Qinter Demo by Fonzie(PD)", 0, 0 )

	SOFTWARE( 32xsam, 0,       199?, "Homebrew", "32X Sample Program - PWM Sound Demo (USA) (SDK Build)", 0, 0 )

	SOFTWARE( backan, 0,       199?, "Homebrew", "Back and Forth Rotating XOR Palette by DevSter(PD)", 0, 0 )

	SOFTWARE( backwa, 0,       199?, "Homebrew", "Backward Rotating XOR Palette Demo by DevSter(PD)", 0, 0 )

	SOFTWARE( devste, 0,       199?, "Homebrew", "DevSter Owns! Text Demo(PD)", 0, 0 )

	SOFTWARE( greenr, 0,       199?, "Homebrew", "Green Rotating No Stretching XOR Palette Demo by DevSter(PD)", 0, 0 )

	SOFTWARE( green1, greenr, 199?, "Homebrew", "Green Rotating XOR Palette Demo by DevSter(PD)", 0, 0 )

	SOFTWARE( hotchi, 0,       199?, "Homebrew", "Hot Chick by DevSter(PD)", 0, 0 )
	SOFTWARE( hotch1, hotchi, 199?, "Homebrew", "Hot Chick Drawn Without the SH2s for Emulators by DevSter(PD)", 0, 0 )
	SOFTWARE( hotch2, hotchi, 199?, "Homebrew", "Hot Chick Drawn Without the SH2s for Hardware by DevSter(PD)", 0, 0 )
	SOFTWARE( hotch3, hotchi, 199?, "Homebrew", "Hot Chick in High Quality 15BPP by DevSter(PD)", 0, 0 )
	SOFTWARE( hotch4, hotchi, 199?, "Homebrew", "Hot Chick in VDP Mode 2 by DevSter(PD)", 0, 0 )
	SOFTWARE( hotch5, hotchi, 199?, "Homebrew", "Hot Chick in VDP Mode 3 by DevSter(PD)", 0, 0 )
	SOFTWARE( hotch6, hotchi, 199?, "Homebrew", "Hot Chick Spinning Demo by DevSter(PD)", 0, 0 )
	SOFTWARE( hotch7, hotchi, 199?, "Homebrew", "Hot Chick Spinning Slower Demo by DevSter(PD)", 0, 0 )
	SOFTWARE( hotch8, hotchi, 199?, "Homebrew", "Hot Chick with Genesis VDP Overlay by DevSter(PD)", 0, 0 )
	SOFTWARE( hotch9, hotchi, 199?, "Homebrew", "Hot Chick Zoom Shot by DevSter(PD)", 0, 0 )


	SOFTWARE( optimi, 0,       199?, "Homebrew", "Optimized Rotating XOR Palette Demo by DevSter(PD)", 0, 0 )

	SOFTWARE( rotati, 0,       199?, "Homebrew", "Rotating No Background Fill XOR Palette Demo by DevSter(PD)", 0, 0 )

	SOFTWARE( rotat1, rotati, 199?, "Homebrew", "Rotating Pixel Skipping XOR Palette Demo by DevSter(PD)", 0, 0 )

	SOFTWARE( rotat2, rotati, 199?, "Homebrew", "Rotating XOR Palette Demo by DevSter(PD)", 0, 0 )

	SOFTWARE( superx, 0,       199?, "Homebrew", "Super-X Raycasting Engine Test 1 by Fonzie(PD)", 0, 0 )
	SOFTWARE( super1, superx, 199?, "Homebrew", "Super-X Raycasting Engine Test 2 by Fonzie(PD)", 0, 0 )

	SOFTWARE( switch, 0,       199?, "Homebrew", "Switching CRAM Palettes by DevSter(PD)", 0, 0 )

	SOFTWARE( xorpal, 0,       199?, "Homebrew", "XOR Palette Demo by DevSter(PD)", 0, 0 )

#endif
