/***************************************************************************

    SNK Neo Geo Pocket & Neo Geo Pocket Color cartridges

***************************************************************************/

#include "driver.h"
#include "softlist.h"
#include "devices/cartslot.h"


#define COLOR_ONLY      0x01


#define NGP_ROM_LOAD( set, name, offset, length, hash )	\
SOFTWARE_START( set ) \
	ROM_REGION( 0x400000, CARTRIDGE_REGION_ROM, ROMREGION_ERASEFF ) \
	ROM_LOAD(name, offset, length, hash) \
SOFTWARE_END


// NGP
NGP_ROM_LOAD( baseball, "baseball stars (japan, europe) (en,ja).bin",                                                0x000000,  0x100000,	 CRC(4781ae84) SHA1(8ece00f21ae49ed4777a9c990b2782465ef95380) )
NGP_ROM_LOAD( dokodemo, "dokodemo mahjong (japan).bin",                                                              0x000000,  0x80000,	 CRC(78c21b5e) SHA1(d7edf6bb3773a5319dfd2a20e204c70e8cc6971e) )
NGP_ROM_LOAD( kof_mlon, "king of fighters r-1 & melon-chan no seichou nikki (japan) (beta).bin",                     0x000000,  0x200000,	 CRC(2660bfc4) SHA1(a02d1b1113dbeac895832f1501029e40d9aab493) )
NGP_ROM_LOAD( kofr1,    "king of fighters r-1 (japan, europe) (en,ja).bin",                                          0x000000,  0x200000,	 CRC(dceb7e11) SHA1(4a7adbcccb05b6047a38bb861523cbae7d956bfa) )
NGP_ROM_LOAD( melonchn, "melon-chan no seichou nikki (japan).bin",                                                   0x000000,  0x100000,	 CRC(cb42cfa4) SHA1(2dbf9c40ab0167ff172f10354d4674e5e6c901d5) )
NGP_ROM_LOAD( neocher,  "neo cherry master - real casino series (japan, europe) (en,ja).bin",                        0x000000,  0x100000,	 CRC(5c067579) SHA1(c171d8d22d667d713bfc73c2b1c04bc77aacbb5c) )
NGP_ROM_LOAD( neocup98, "neo geo cup '98 (japan, europe) (en,ja).bin",                                               0x000000,  0x100000,	 CRC(33add5bd) SHA1(445743119c65a6765a0bdc849a12e9038192840c) )
NGP_ROM_LOAD( ptennis,  "pocket tennis - pocket sports series (japan, europe) (en,ja).bin",                          0x000000,  0x80000,	 CRC(89410850) SHA1(59602e44c631a00b2db53196204a4bc22ccd7025) )
NGP_ROM_LOAD( renketsu, "renketsu puzzle tsunagete pon! (japan).bin",                                                0x000000,  0x80000,	 CRC(37e79afc) SHA1(7a75e3b6874a18b29ba636ff22308d944a7901e6) )
NGP_ROM_LOAD( shougi,   "shougi no tatsujin (japan).bin",                                                            0x000000,  0x80000,	 CRC(f34d0c9b) SHA1(ab0c33236f72d2b317d91d034756b56cfca1b0b4) )


// NGP Color
NGP_ROM_LOAD( bakumats,   "bakumatsu rouman tokubetsu hen - gekka no kenshi - tsuki ni saku hana, chiri yuku hana (japan).bin",0x000000,  0x200000,	 CRC(12afacb0) SHA1(aa17bd65849707cf13ca4891df44200f58d13599) )
NGP_ROM_LOAD( bstarsc,    "baseball stars color (world) (en,ja).bin",                                                  0x000000,  0x100000,	 CRC(ffa7e88d) SHA1(5a9792d7674ead92c08fb6f2d108b26e47a696bf) )
NGP_ROM_LOAD( bigbang,    "big bang pro wrestling (japan) (en,ja).bin",                                                0x000000,  0x200000,	 CRC(b783c372) SHA1(51b9f5a0b233b0c9782cf522324a3a437c06c26f) )
NGP_ROM_LOAD( bikkuri,    "bikkuriman 2000 - viva! pocket festiva! (japan).bin",                                       0x000000,  0x100000,	 CRC(7a2d7635) SHA1(62898d43cfc9ace86a53520ecf62abc6b0c404f3) )
NGP_ROM_LOAD( biomotorj,  "biomotor unitron (japan).bin",                                                              0x000000,  0x100000,	 CRC(ecee99f3) SHA1(74ef25a02284ea81dfba00a02098ab46656ed4a9) )
NGP_ROM_LOAD( biomotor,   "biomotor unitron (usa, europe).bin",                                                        0x000000,  0x100000,	 CRC(3807df4f) SHA1(ff74d5877bf58164419710888923e46b9cfb415e) )
NGP_ROM_LOAD( bustamove,  "bust-a-move pocket (usa).bin",                                                              0x000000,  0x100000,	 CRC(85fe6ae3) SHA1(5e8c84b9a4b10aa1c3abbfdea60aef90a8b7db7d) )
NGP_ROM_LOAD( coolboar,   "cool boarders pocket (japan, europe) (en,ja).bin",                                          0x000000,  0x100000,	 CRC(833f9c22) SHA1(2930551088bc97a407824178f23dfa8d0b6a551f) )
NGP_ROM_LOAD( coolcoolj,  "cool cool jam (japan).bin",                                                                 0x000000,  0x200000,	 CRC(4fda5d8a) SHA1(f0f9fb71be6fde25f9db8dcb8c6d6aa859b21697) )
NGP_ROM_LOAD( coolcool,   "cool cool jam (usa, europe) (sample).bin",                                                  0x000000,  0x200000,	 CRC(4d8d27f0) SHA1(77d7bb74844d43520dcd6e9cd912560c995c6247) )
NGP_ROM_LOAD( crushrol,   "crush roller (world) (en,ja).bin",                                                          0x000000,  0x100000,	 CRC(6200c65e) SHA1(da34a11d3602e249dbc322d6cce823423330c53b) )
NGP_ROM_LOAD( darkarms,   "dark arms - beast buster 1999 (world) (en,ja).bin",                                         0x000000,  0x200000,	 CRC(6f353f34) SHA1(727d8100fa7c4a39a6be9133bd903b868b4cc834) )
NGP_ROM_LOAD( deltawrp,   "delta warp (japan).bin",                                                                    0x000000,  0x80000,	 CRC(add4fdff) SHA1(4ab8f0b074a1299a0adee3d7ca46d070a9ccae6d) )
NGP_ROM_LOAD( ogrebatl,   "densetsu no ogre battle gaiden - zenobia no ouji (japan).bin",                              0x000000,  0x200000,	 CRC(eeeefd6a) SHA1(5ac794c0f125bfb97efe9cd06c2aca3afdf549ea) )
NGP_ROM_LOAD( dendego2b,  "densha de go! 2 on neo geo pocket (japan) (beta).bin",                                      0x000000,  0x400000,	 CRC(2fc4623e) SHA1(4934987cd275a013d05d02b1ddd8fd1fb2ded0da) )
NGP_ROM_LOAD( dendego2,   "densha de go! 2 on neo geo pocket (japan).bin",                                             0x000000,  0x400000,	 CRC(d5ced9e9) SHA1(cee5fa9dc0acfba95ee6bd802a0ff80d4ecacc79) )
NGP_ROM_LOAD( divealer1,  "dive alert - becky's version (usa, europe).bin",                                            0x000000,  0x200000,	 CRC(83db5772) SHA1(64467acf90be6931f9aa532ed8e1fd4dc60946c6) )
NGP_ROM_LOAD( divealerj,  "dive alert - burn hen (japan).bin",                                                         0x000000,  0x200000,	 CRC(c213941d) SHA1(66624028ec986084a95237c74173849a8a729398) )
NGP_ROM_LOAD( divealer,   "dive alert - matt's version (usa, europe).bin",                                             0x000000,  0x200000,	 CRC(ef75081b) SHA1(0abdf64dd402a8192b0af8127a6ee214173dfe3c) )
NGP_ROM_LOAD( divealerj1, "dive alert - rebecca hen (japan).bin",                                                      0x000000,  0x200000,	 CRC(3c4af4f5) SHA1(ac81de4f9c1473c5ecd761dc449ae5af0e98915f) )
NGP_ROM_LOAD( dynaslug,   "dynamite slugger (japan, europe) (en,ja).bin",                                              0x000000,  0x100000,	 CRC(7f1779cd) SHA1(90104d5917db4cacfd1d0ef8b2b01fea09679a93) )
NGP_ROM_LOAD( evolutio,   "evolution - eternal dungeons (europe).bin",                                                 0x000000,  0x200000,	 CRC(be47e531) SHA1(9f607095b038ccdbf98e6a8fce2dd67740ae5571) )
NGP_ROM_LOAD( cotton,     "fantastic night dreams cotton (europe).bin",                                                0x000000,  0x100000,	 CRC(1bf412f5) SHA1(a9d8be97ab7bab1aaa1bedd2ce6914ec7fb8cb85) )
NGP_ROM_LOAD( cottonj,    "fantastic night dreams cotton (japan).bin",                                                 0x000000,  0x100000,	 CRC(b8a12409) SHA1(935b9d671df660192d9fc215aa03d90a486e743a) )
NGP_ROM_LOAD( faselei,    "faselei! (europe).bin",                                                                     0x000000,  0x200000,	 CRC(e705e30e) SHA1(48c04c8a261b25a942a9fe68abace8aad9deae23) )
NGP_ROM_LOAD( faseleij,   "faselei! (japan).bin",                                                                      0x000000,  0x200000,	 CRC(8f585838) SHA1(04a65a7529098bfb2295ab4bc3782ea45ec116ee) )
NGP_ROM_LOAD( fatfury,    "fatal fury f-contact (world) (en,ja).bin",                                                  0x000000,  0x200000,	 CRC(a9119b5a) SHA1(ecfccd7e971d8e9d46b97aa05dc7eb61d69e16cd) )
NGP_ROM_LOAD( ganbaren,   "ganbare neo poke-kun (ka) (japan).bin",                                                     0x000000,  0x200000,	 CRC(6df986a3) SHA1(bc8be263618601d5967538d125be1d58103ca7b7) )
NGP_ROM_LOAD( infinity,   "infinity cure (japan).bin",                                                                 0x000000,  0x100000,	 CRC(32dc2aa2) SHA1(b0f3f57c5ad084a009631e7e0fa497451415e73b) )
NGP_ROM_LOAD( kikousei,   "kikou seiki unitron - sono tsuide. hikari umareru chi yori. (japan).bin",                   0x000000,  0x200000,	 CRC(84580d66) SHA1(b732c22631b05895ed7eebabfc3a59e28b04b364) )
NGP_ROM_LOAD( kofr2b,     "king of fighters r-2 (world) (en,ja) (beta).bin",                                           0x000000,  0x200000,	 CRC(e3ae79c0) SHA1(6d138c34f3dc3c7f1b943c1f57f885007edfb12a) )
NGP_ROM_LOAD( kofr2,      "king of fighters r-2 (world) (en,ja).bin",                                                  0x000000,  0x200000,	 CRC(47e490a2) SHA1(7f2257fe6f9b90292d0332c06b9d68bfaeb2ed08) )
NGP_ROM_LOAD( kofpara,    "king of fighters, the - battle de paradise (japan).bin",                                    0x000000,  0x200000,	 CRC(77e37bac) SHA1(8aabdd45ba2b9e24bdcd8e51f6653736257d4644) )
NGP_ROM_LOAD( koikoi,     "koi koi mahjong (japan).bin",                                                               0x000000,  0x80000,	 CRC(b51c3eba) SHA1(d705dfa87cf26c8232cf4885f8ae326eb690df59) )
NGP_ROM_LOAD( lastblad,   "last blade, the - beyond the destiny (europe).bin",                                         0x000000,  0x200000,	 CRC(94fcfd1e) SHA1(b7df1e56bc47266b65226413095e38896b38da07) )
NGP_ROM_LOAD( magdropj,   "magical drop pocket (japan).bin",                                                           0x000000,  0x100000,	 CRC(6c465716) SHA1(9baa97ef2b825f36ee3d3e894d1b6156ac414c0c) )
NGP_ROM_LOAD( magdrop,    "magical drop pocket (usa, europe).bin",                                                     0x000000,  0x100000,	 CRC(72184de7) SHA1(05ebf0147961eda2ed804dc369cd0a4dee923c9c) )
NGP_ROM_LOAD( memories,   "memories off - pure (japan).bin",                                                           0x000000,  0x100000,	 CRC(a7926e90) SHA1(17d76b25f97acf0c803e5e30922c950331e4be8b) )
NGP_ROM_LOAD( mslug1st,   "metal slug - 1st mission (world) (en,ja).bin",                                              0x000000,  0x200000,	 CRC(4ff91807) SHA1(3540bb6efbbc4f37992d2a871b8d83b4fca0e76e) )
NGP_ROM_LOAD( mslug2nd,   "metal slug - 2nd mission (world) (en,ja).bin",                                              0x000000,  0x400000,	 CRC(ac549144) SHA1(9c1a829d4678dd9c51fdfe30c76b7f9670311bfb) )
NGP_ROM_LOAD( mezase,     "mezase! kanji ou (japan).bin",                                                              0x000000,  0x200000,	 CRC(a52e1c82) SHA1(030a59f990e96649001582be2d5acc9af18424fe) )
NGP_ROM_LOAD( mizuki,     "mizuki shigeru no youkai shashinkan (japan).bin",                                           0x000000,  0x200000,	 CRC(dde08335) SHA1(8025d09368598951da94f8601729c16972b96830) )
NGP_ROM_LOAD( neo21,      "neo 21 - real casino series (world).bin",                                                   0x000000,  0x100000,	 CRC(0a2a2f28) SHA1(359dc14577c7b8fb8f2c8cfa63c12bccb30fcc11) )
NGP_ROM_LOAD( neobacca,   "neo baccarat - real casino series (world).bin",                                             0x000000,  0x100000,	 CRC(22aab454) SHA1(80218804eb591e50831d098b5bd213a3586849a5) )
NGP_ROM_LOAD( neochercb,  "neo cherry master color - real casino series (world) (en,ja) (beta).bin",                   0x000000,  0x100000,	 CRC(939a9912) SHA1(9b53e545388d487a2782e5ea7232979ee33e2b4f) )
NGP_ROM_LOAD( neocherc,   "neo cherry master color - real casino series (world) (en,ja).bin",                          0x000000,  0x100000,	 CRC(a6dca584) SHA1(17d51967cf1992b6aa779a2583dac46a6590d50a) )
NGP_ROM_LOAD( neoderby,   "neo derby champ daiyosou (japan).bin",                                                      0x000000,  0x200000,	 CRC(5a053559) SHA1(b75a0de4eec70c39616dfff92b09f4fb1c1e0881) )
NGP_ROM_LOAD( neodrag1,   "neo dragon's wild - real casino series (world) (en,ja) (v1.11).bin",                        0x000000,  0x100000,	 CRC(225fd7f9) SHA1(b80a2ab886c80ad17a6cb0968ea3ad6218922fa1) )
NGP_ROM_LOAD( neodrag,    "neo dragon's wild - real casino series (world) (en,ja) (v1.13).bin",                        0x000000,  0x100000,	 CRC(5d028c99) SHA1(b83f713370169e146a1687424ae8ce56b35a23da) )
NGP_ROM_LOAD( neocuppl,   "neo geo cup '98 plus (world) (en,ja).bin",                                                  0x000000,  0x100000,	 CRC(c0da4fe9) SHA1(a8bb64f093b418a88e674d9fab62a34fc220ea9f) )
NGP_ROM_LOAD( neomystr,   "neo mystery bonus - real casino series (world) (en,ja).bin",                                0x000000,  0x100000,	 CRC(e79dc5b3) SHA1(0d137a2cac0a03ab55dfeb4dbaf3929477cfb5d0) )
NGP_ROM_LOAD( neoproyk,   "neo poke pro yakyuu (japan).bin",                                                           0x000000,  0x100000,	 CRC(f4d78f12) SHA1(2052929b68092af6d530cbb97ff092805807f381) )
NGP_ROM_LOAD( neoturfm,   "neo turf masters (world) (en,ja).bin",                                                      0x000000,  0x200000,	 CRC(317a66d2) SHA1(d425bb812290b93a800032925d7be14f75820159) )
NGP_ROM_LOAD( nigeronp,   "nigeronpa (japan).bin",                                                                     0x000000,  0x100000,	 CRC(3cc3e269) SHA1(b5d355ea808dd01cf746daef7704e54d7766bd3b) )
NGP_ROM_LOAD( oekakip,    "oekaki puzzle (japan).bin",                                                                 0x000000,  0x80000,	 CRC(59ab1e0f) SHA1(792a6cbb736526f4518df97b5ac79ecb45e46eec) )
NGP_ROM_LOAD( pacworld,   "pac-man (world) (en,ja).bin",                                                               0x000000,  0x80000,	 CRC(21e8cc15) SHA1(ec64ab4be669f9860c216151094b9eea898c64b9) )
NGP_ROM_LOAD( pslotazt,   "pachi-slot aruze oukoku pocket - azteca (japan).bin",                                       0x000000,  0x80000,	 CRC(c56ef8c3) SHA1(3f814c84429629d763d6dc62d513013a0cc19a0c) )
NGP_ROM_LOAD( pslotdk2,   "pachi-slot aruze oukoku pocket - dekahel 2 (japan).bin",                                    0x000000,  0x100000,	 CRC(346f3c46) SHA1(7f5446cfe6535db63678d6fc26f598ed3ff73b7f) )
NGP_ROM_LOAD( pslotds2,   "pachi-slot aruze oukoku pocket - delsol 2 (japan).bin",                                     0x000000,  0x100000,	 CRC(1ea69db1) SHA1(df3da196c01be73b0dea64b86ab27ec7dde5b594) )
NGP_ROM_LOAD( pslotecp,   "pachi-slot aruze oukoku pocket - e-cup (japan).bin",                                        0x000000,  0x100000,	 CRC(0a7c32df) SHA1(e46aa1eb7c33c2f71bb236d279ca1d7c5d841e74) )
NGP_ROM_LOAD( pslothan1,  "pachi-slot aruze oukoku pocket - hanabi (japan) (v1.02).bin",                               0x000000,  0x80000,	 CRC(cfd1c8f2) SHA1(e4b48d1290cd9dcf7a05ea276226ecba10be752e) )
NGP_ROM_LOAD( pslothan,   "pachi-slot aruze oukoku pocket - hanabi (japan) (v1.04).bin",                               0x000000,  0x80000,	 CRC(a0c26f9b) SHA1(9d30e0c67851bc99e4a77e89e3fbc101b358b458) )
NGP_ROM_LOAD( pslotooh,   "pachi-slot aruze oukoku pocket - oohanabi (japan).bin",                                     0x000000,  0x100000,	 CRC(8a88c50f) SHA1(a99e8aabefd36a5a297fef59e376cf24f8f619f8) )
NGP_ROM_LOAD( pslotpc2,   "pachi-slot aruze oukoku pocket - porcano 2 (japan).bin",                                    0x000000,  0x100000,	 CRC(96ddfbe3) SHA1(fd51a5a649cebfe5692ee498d11c48fd84661dc4) )
NGP_ROM_LOAD( pslotwrd,   "pachi-slot aruze oukoku pocket - ward of lights (japan).bin",                               0x000000,  0x80000,	 CRC(d46cbc54) SHA1(c17a535195719e0dafb5de76663c7649d209f0ff) )
NGP_ROM_LOAD( pachinko,   "pachinko hisshou guide - pocket parlor (japan).bin",                                        0x000000,  0x100000,	 CRC(c3d6b28b) SHA1(941b168ecee1de8dd773464a8754c6c8c736b492) )
NGP_ROM_LOAD( prtymail,   "party mail (japan).bin",                                                                    0x000000,  0x100000,	 CRC(4da8a1c0) SHA1(b805c908c1bef29c0792a73b6a6a05dd27cc5ff0) )
NGP_ROM_LOAD( picturep,   "picture puzzle (usa, europe).bin",                                                          0x000000,  0x100000,	 CRC(67e6dc56) SHA1(387d37cf60562da4e95ba18440ad6e0a4cccb537) )
NGP_ROM_LOAD( pocklove,   "pocket love if (japan).bin",                                                                0x000000,  0x200000,	 CRC(c2ee1ee5) SHA1(2b5d9628b7b5911f90c28321227d47a527a719ea) )
NGP_ROM_LOAD( pockrev,    "pocket reversi (europe).bin",                                                               0x000000,  0x100000,	 CRC(20f09880) SHA1(0405bbf7ab34c818f52793aca80a73ab40788cac) )
NGP_ROM_LOAD( pockrevj,   "pocket reversi (japan).bin",                                                                0x000000,  0x80000,	 CRC(fb0b55b8) SHA1(b13159d9944aaa19720d03d17b223d6ff12c1e29) )
NGP_ROM_LOAD( ptennisc,   "pocket tennis color - pocket sports series (world) (en,ja).bin",                            0x000000,  0x80000,	 CRC(d8590b96) SHA1(d14251209ca5da8000643cd5bf6e5196a5e56ee4) )
NGP_ROM_LOAD( puyopop1,   "puyo pop (world) (en,ja) (v1.05).bin",                                                      0x000000,  0x100000,	 CRC(3090b2fd) SHA1(27c3250fb17f291832fa7189a35478d017a0ef5e) )
NGP_ROM_LOAD( puyopop,    "puyo pop (world) (en,ja) (v1.06).bin",                                                      0x000000,  0x100000,	 CRC(5c649d1e) SHA1(8971d3d64e47950cf01648d7b341d4b01a2c4eb0) )
NGP_ROM_LOAD( pbobbleb,   "puzzle bobble mini (japan, europe) (en,ja) (beta).bin",                                     0x000000,  0x100000,	 CRC(e3d9f38e) SHA1(deccefd50c3d2654f7190bdd629cd1f0ba3c30df) )
NGP_ROM_LOAD( pbobblea,   "puzzle bobble mini (japan, europe) (v1.09).bin",                                            0x000000,  0x100000,	 CRC(c94b173a) SHA1(2cb115d011ffb82ed2b2560697b8f538e298c03f) )
NGP_ROM_LOAD( pbobble,    "puzzle bobble mini (japan, europe) (v1.10).bin",                                            0x000000,  0x100000,	 CRC(e69eae3e) SHA1(c217bc81012c8011ec268e6a21adf5b26566b4b5) )
NGP_ROM_LOAD( puzzlink,   "puzzle link (europe).bin",                                                                  0x000000,  0x80000,	 CRC(fe95938c) SHA1(6e7b48b59d4df857dd76a964c521b93cb0c4c9a7) )
NGP_ROM_LOAD( puzzlnk2,   "puzzle link 2 (usa, europe).bin",                                                           0x000000,  0x100000,	 CRC(e2a09bb7) SHA1(d4522d1a454968cccd57b03c2aff99616d0b5174) )
NGP_ROM_LOAD( renketsc,   "renketsu puzzle tsunagete pon! color (japan).bin",                                          0x000000,  0x80000,	 CRC(615f1f2f) SHA1(048b85a8e8e0cbd9324bc373fc875288f81c21db) )
NGP_ROM_LOAD( rockmanb,   "rockman - battle & fighters (japan).bin",                                                   0x000000,  0x200000,	 CRC(9c861e49) SHA1(c0a6cdf9ba6c4e8f227ba52489f15f5053f0889c) )
NGP_ROM_LOAD( samsho,     "samurai shodown! - pocket fighting series (japan, europe) (en,ja).bin",                     0x000000,  0x200000,	 CRC(32e4696a) SHA1(2c6adfa69e7aeada7d4198b373436eee35b70d01) )
NGP_ROM_LOAD( samsho2,    "samurai shodown! 2 - pocket fighting series (world) (en,ja).bin",                           0x000000,  0x200000,	 CRC(4f7fb156) SHA1(2ec4da369d3b91af15defa64429bd9daac7ee9b5) )
NGP_ROM_LOAD( shanghai,   "shanghai mini (world) (en,ja).bin",                                                         0x000000,  0x100000,	 CRC(90732d53) SHA1(3e3e625852ac4997d3e3739e59c16e9a74f40f19) )
NGP_ROM_LOAD( evolutioj,  "shinki sekai evolution - hateshinai dungeon (japan).bin",                                   0x000000,  0x200000,	 CRC(e006f42f) SHA1(61827b7a13b3efe6a6b60f314fb0589a29c1bfd2) )
NGP_ROM_LOAD( shougic,    "shougi no tatsujin color (japan).bin",                                                      0x000000,  0x80000,	 CRC(1f2872ed) SHA1(9f420d0a7cc1115f60bda1c0916247935118ce98) )
NGP_ROM_LOAD( snkgalsj,   "snk gals' fighters (japan).bin",                                                            0x000000,  0x200000,	 CRC(6a9ffa47) SHA1(2f806c926a19b261c42e5a60d1ddd5d132ada942) )
NGP_ROM_LOAD( snkgals,    "snk gals' fighters (usa, europe).bin",                                                      0x000000,  0x200000,	 CRC(b02c2be7) SHA1(09a74b42ef9b63aca675aead5854512ad0b585a1) )
NGP_ROM_LOAD( svccard2,   "snk vs. capcom - card fighters 2 - expand edition (japan).bin",                             0x000000,  0x200000,	 CRC(ccbcfda7) SHA1(2c4fc8594f8ba2bf39e62fe8bcb79fc2df5a83b6) )
NGP_ROM_LOAD( svccardc,   "snk vs. capcom - card fighters' clash - capcom version (usa, europe).bin",                  0x000000,  0x200000,	 CRC(80ce137b) SHA1(2a95ab1fe62b6a9dc30a05131ad212ba2de5c3cd) )
NGP_ROM_LOAD( svccards,   "snk vs. capcom - card fighters' clash - snk version (usa, europe).bin",                     0x000000,  0x200000,	 CRC(94b63a97) SHA1(dfb232a527647337d6274ddf9d8b3d94281fd86d) )
NGP_ROM_LOAD( svccardj,   "snk vs. capcom - gekitotsu card fighters (japan) (beta).bin",                               0x000000,  0x200000,	 CRC(4a41a56e) SHA1(4b2f613f978cefae8e952a43440d2d395b1012d5) )
NGP_ROM_LOAD( svccardcj,  "snk vs. capcom - gekitotsu card fighters - capcom supporter version (japan).bin",           0x000000,  0x200000,	 CRC(9217159b) SHA1(3b1d5ac5b4aaf1aa04479e46961c4d7c458d70fb) )
NGP_ROM_LOAD( svccardsj1, "snk vs. capcom - gekitotsu card fighters - snk supporter version (japan) (beta).bin",       0x000000,  0x200000,	 CRC(e70e3f1a) SHA1(584b30b050c22082dae3abe685f406dbe4decf98) )
NGP_ROM_LOAD( svccardsj,  "snk vs. capcom - gekitotsu card fighters - snk supporter version (japan).bin",              0x000000,  0x200000,	 CRC(7065295c) SHA1(551ceb97d1e3e6384221be738b11efb5d936ffd6) )
NGP_ROM_LOAD( svc,        "snk vs. capcom - the match of the millennium (world) (en,ja).bin",                          0x000000,  0x400000,	 CRC(b173030a) SHA1(7629ea5520d3daee4ef658d4f4b48f45e17217ae) )
NGP_ROM_LOAD( sonicb,     "sonic the hedgehog - pocket adventure (world) (beta).bin",                                  0x000000,  0x200000,	 CRC(622176ce) SHA1(a11283703fe6007c2e96ce81dfc56081eff6f593) )
NGP_ROM_LOAD( sonic,      "sonic the hedgehog - pocket adventure (world).bin",                                         0x000000,  0x200000,	 CRC(356f0849) SHA1(3fe15d4374c4ca073aa0393fcfcad0055bd345ab) )
NGP_ROM_LOAD( soreike,    "soreike!! hanafuda doujou (japan).bin",                                                     0x000000,  0x80000,	 CRC(05fa3eb0) SHA1(f4a4264eaf1605086ccbc526f21da222aa6341bf) )
NGP_ROM_LOAD( srmp,       "super real mahjong - premium collection (japan).bin",                                       0x000000,  0x200000,	 CRC(c6e620c3) SHA1(c61847bca43c5387d19096a86ef109560d71ca9b) )
NGP_ROM_LOAD( tsunapn2,   "tsunagete pon! 2 (japan).bin",                                                              0x000000,  0x100000,	 CRC(98acc37a) SHA1(4912c271a140291dbfc523017844a1afc68bd5e4) )
NGP_ROM_LOAD( wrestmad,   "wrestling madness (usa, europe) (beta).bin",                                                0x000000,  0x200000,	 CRC(16b0be9b) SHA1(2dd82db967e2bc8c2d8b569a364793d56176a68e) )


SOFTWARE_LIST_START( ngp_cart )
	SOFTWARE( baseball,   0,        1998, "SNK", "Pocket Sports Series - Baseball Stars (Euro, Jpn)", 0, 0 )     /* Id: NeoP00070 - Developer: SNK - Releases: 1998-11-26 (JPN) */
	SOFTWARE( kof_mlon,   0,        199?, "SNK", "King of Fighters R-1 & Melon-chan no Seichou Nikki (Jpn, Prototype)", 0, 0 )
	SOFTWARE( kofr1,      0,        1998, "SNK", "Pocket Fighting Series - King of Fighters R-1 (Euro, Jpn)", 0, 0 ) /* Id: NeoP00010 - Developer: SNK - Releases: 1998-10-28 (JPN) */
	SOFTWARE( melonchn,   0,        1998, "SNK", "Melon-chan no Seichou Nikki (Jpn)", 0, 0 )                     /* Id: NeoP00040 - Developer: ADK - Releases: 1998-10-28 (JPN) */
	SOFTWARE( neocher,    0,        1998, "SNK", "Pocket Casino Series - Neo Cherry Master (Euro, Jpn)", 0, 0 )  /* Id: NeoP00140 - Developer: Dyna - Releases: 1998-12-25 (JPN) */
	SOFTWARE( neocup98,   0,        1998, "SNK", "Pocket Sports Series - Neo Geo Cup '98 (Euro, Jpn)", 0, 0 )    /* Id: NeoP00020 - Developer: SNK - Releases: 1998-10-28 (JPN) */
	SOFTWARE( ptennis,    0,        1998, "SNK", "Pocket Sports Series - Pocket Tennis (Euro, Jpn)", 0, 0 )      /* Id: NeoP00090 - Developer: Yumekobo - Releases: 1998-10-28 (JPN) */
	SOFTWARE( renketsu,   0,        1998, "SNK", "Renketsu Puzzle Tsunagete Pon! (Jpn)", 0, 0	)                  /* Id: NeoP00120 - Developer: Yumekobo - Releases: 1998-10-28 (JPN) */
	SOFTWARE( shougi,     0,        1998, "SNK", "Shougi no Tatsujin (Jpn)", 0, 0 )                              /* Id: NeoP00050 - Developer: ADK - Releases: 1998-11-20 (JPN) */
SOFTWARE_LIST_END


/* FIXME: Euro & USA games still miss Id & COLOR_ONLY info */
SOFTWARE_LIST_START( ngpc_cart )
	SOFTWARE( bakumats,   0,        2000, "SNK", "Bakumatsu Rouman Tokubetsu Hen - Gekka no Kenshi - Tsuki ni Saku Hana, Chiri Yuku Hana (Jpn)", COLOR_ONLY, 0 )  /* Id: NeoP00640 - Developer: SNK - Releases: 2000-03-16 (JPN) */
	SOFTWARE( bstarsc,    0,        1999, "SNK", "Pocket Sport Series - Baseball Stars Color (World)", 0, 0 )            /* Id: NeoP00250 - Developer: SNK - Releases: 1999-03-19 (JPN) */
	SOFTWARE( bigbang,    0,        2000, "SNK", "Big Bang Pro Wrestling (Jpn)", COLOR_ONLY, 0 )                         /* Id: NeoP00660 - Developer: S-Neo - Releases: 2000-11-23 (JPN) */
	SOFTWARE( bikkuri,    0,        2000, "Sega Toys", "Bikkuriman 2000 - Viva! Pocket Festiva! (Jpn)", COLOR_ONLY, 0 )  /* Id: NeoP00830 - Developer: Sega Toys - Releases: 2000-03-16 (JPN) */
	SOFTWARE( biomotorj,  biomotor, 1999, "SNK", "BioMotor Unitron (Jpn)", 0, 0 )                                        /* Id: NeoP00100 - Developer: Yumekobo - Releases: 1999-04-15 (JPN) */
	SOFTWARE( biomotor,   0,        1999, "SNK", "BioMotor Unitron (Euro, USA)", 0, 0 )
	SOFTWARE( bustamove,  pbobble,  1999, "SNK", "Bust-A-Move Pocket (USA)", 0, 0 )
	SOFTWARE( coolboar,   0,        2000, "UEP Systems", "Cool Boarders Pocket (Euro, Jpn)", COLOR_ONLY, 0 )             /* Id: NeoP00750 - Developer: UEP Systems - Releases: 2000-02-24 (JPN) */
	SOFTWARE( coolcoolj,  coolcool, 2000, "SNK", "Cool Cool Jam (Jpn)", COLOR_ONLY, 0 )                                  /* Id: NeoP01000 - Developer: SNK - Releases: 2000-08-10 (JPN) */
	SOFTWARE( coolcool,   0,        2000, "SNK", "Cool Cool Jam (Euro, USA, Sample)", 0, 0 )
	SOFTWARE( crushrol,   0,        1999, "SNK", "Crush Roller (World)", 0, 0 )                                          /* Id: NeoP00380 - Developer: ADK - Releases: 1999-04-15 (JPN) */
	SOFTWARE( darkarms,   0,        1999, "SNK", "Dark Arms - Beast Buster 1999 (World) ~ Beast Buster - Yami no Seitai Heiki (Jpn)", 0, 0 )  /* Id: NeoP00450 - Developer: SNK - Releases: 1999-10-21 (JPN) */
	SOFTWARE( deltawrp,   0,        2000, "SNK", "Delta Warp (Jpn)", COLOR_ONLY, 0 )                                     /* Id: NeoP01030 - Developer: IOSYS - Releases: 2000-08-10 (JPN) */
	SOFTWARE( ogrebatl,   0,        2000, "SNK", "Densetsu no Ogre Battle Gaiden - Zenobia no Ouji (Jpn)", COLOR_ONLY, 0 )  /* Id: NeoP00850 - Developer: Quest - Releases: 2000-06-22 (JPN) */
	SOFTWARE( dendego2b,  dendego2, 1999, "SNK", "Densha de Go! 2 on Neo Geo Pocket (Jpn, Prototype)", 0, 0 )
	SOFTWARE( dendego2,   0,        1999, "SNK", "Densha de Go! 2 on Neo Geo Pocket (Jpn)", 0, 0 )                       /* Id: NeoP00600 - Developer: Taito - Releases: 1999-10-21 (JPN) */
	SOFTWARE( divealer1,  divealer, 1999, "SNK", "Dive Alert - Becky's Version (Euro, USA)", 0, 0 )
	SOFTWARE( divealerj,  divealer, 1999, "SNK", "Dive Alert - Barn Hen (Jpn)", 0, 0 )                                   /* Id: NeoP00370 - Developer: Sacnoth - Releases: 1999-08-19 (JPN) */
	SOFTWARE( divealer,   0,        1999, "SNK", "Dive Alert - Matt's Version (Euro, USA)", 0, 0 )
	SOFTWARE( divealerj1, divealer, 1999, "SNK", "Dive Alert - Rebecca Hen (Jpn)", 0, 0 )                                /* Id: NeoP00360 - Developer: Sacnoth - Releases: 1999-08-19 (JPN) */
	SOFTWARE( dokodemo,   0,        1999, "SNK", "Dokodemo Mahjong (Jpn)", 0, 0 )                                        /* Id: NeoP00330 - Developer: ADK - Releases: 1999-04-29 (JPN) */
	SOFTWARE( dynaslug,   0,        2000, "SNK", "Dynamite Slugger (Euro, Jpn)", COLOR_ONLY, 0 )                         /* Id: NeoP00710 - Developer: ADK - Releases: 2000-05-25 (JPN) */
	SOFTWARE( evolutio,   0,        2000, "SNK", "Evolution - Eternal Dungeons (Euro)", 0, 0 )
	SOFTWARE( cotton,     0,        2000, "SNK", "Fantastic Night Dreams Cotton (Euro)", 0, 0 )
	SOFTWARE( cottonj,    cotton,   2000, "Success", "Fantastic Night Dreams Cotton (Jpn)", 0, 0 )                       /* Id: NeoP00800 - Developer: Magitec - Releases: 2000-03-23 (JPN) */
	SOFTWARE( faselei,    0,        2000, "SNK", "Faselei! (Euro)", 0, 0 )
	SOFTWARE( faseleij,   faselei,  1999, "SNK", "Faselei! (Jpn)", 0, 0 )                                                /* Id: NeoP00510 - Developer: Sacnoth - Releases: 1999-12-29 (JPN) */
	SOFTWARE( fatfury,    0,        1999, "SNK", "Pocket Fighting Series - Garou Densetsu First Contact ~ Fatal Fury F-Contact (World)", 0, 0 )    /* Id: NeoP00110 - Developer: SNK - Releases: 1999-05-27 (JPN) */
	SOFTWARE( ganbaren,   0,        19??, "SNK", "Ganbare Neo Poke-kun (Jpn)", COLOR_ONLY, 0 )                           /* Id: NeoP00970 - Developer: SNK - Releases: 2000-06-08 (JPN) */
	SOFTWARE( infinity,   0,        2000, "KID", "Infinity Cure (Jpn)", COLOR_ONLY, 0 )                                  /* Id: NeoP01090 - Developer: KID - Releases: 2000-11-23 (JPN) */
	SOFTWARE( kikousei,   0,        2000, "SNK", "Kikou Seiki Unitron - Sono Tsuide. Hikari Umareru Chi Yori. (Jpn)", COLOR_ONLY, 0 )  /* Id: NeoP00500 - Developer: Yumekobo - Releases: 2000-01-20 (JPN) */
	SOFTWARE( kofr2b,     kofr2,    1999, "SNK", "Pocket Fighting Series - King of Fighters R-2 (World, Prototype)", 0, 0 )
	SOFTWARE( kofr2,      0,        1999, "SNK", "Pocket Fighting Series - King of Fighters R-2 (World)", 0, 0 )          /* Id: NeoP00230 - Developer: SNK - Releases: 1999-03-19 (JPN) */
	SOFTWARE( kofpara,    0,        2000, "SNK", "The King of Fighters - Battle de Paradise (Jpn)", COLOR_ONLY, 0 )       /* Id: NeoP00920 - Developer: SNK - Releases: 2000-07-06 (JPN) */
	SOFTWARE( koikoi,     0,        2000, "SNK", "Koi Koi Mahjong (Jpn)", COLOR_ONLY, 0 )                                 /* Id: NeoP00910 - Developer: Visco - Releases: 2000-03-09 (JPN) */
	SOFTWARE( lastblad,   0,        2000, "SNK", "The Last Blade - Beyond the Destiny (Euro)", 0, 0 )
	SOFTWARE( magdropj,   magdrop,  1999, "Data East", "Magical Drop Pocket (Jpn)", 0, 0 )                                /* Id: NeoP00480 - Developer: Sakata SAS - Releases: 1999-06-24 (JPN) */
	SOFTWARE( magdrop,    0,        1999, "SNK", "Magical Drop Pocket (Euro, USA)", 0, 0 )
	SOFTWARE( memories,   0,        2000, "KID", "Memories Off - Pure (Jpn)", COLOR_ONLY, 0 )                             /* Id: NeoP00870 - Developer: KID - Releases: 2000-04-27 (JPN) */
	SOFTWARE( mslug1st,   0,        1999, "SNK", "Metal Slug - 1st Mission (World)", 0, 0 )                               /* Id: NeoP00210 - Developer: Ukiyotei - Releases: 1999-05-27 (JPN) */
	SOFTWARE( mslug2nd,   0,        2000, "SNK", "Metal Slug - 2nd Mission (World)", COLOR_ONLY, 0 )                      /* Id: NeoP00610 - Developer: Ukiyotei - Releases: 2000-03-09 (JPN) */
	SOFTWARE( mezase,     0,        2000, "SNK", "Mezase! Kanji Ou (Jpn)", COLOR_ONLY, 0 )                                /* Id: NeoP00630 - Developer: SNK - Releases: 2000-01-20 (JPN) */
	SOFTWARE( mizuki,     0,        1999, "SNK", "Mizuki Shigeru no Youkai Shashinkan (Jpn)", COLOR_ONLY, 0 )             /* Id: NeoP00620 - Developer: Ukiyotei - Releases: 1999-12-29 (JPN) */
	SOFTWARE( neo21,      0,        1999, "SNK", "Pocket Casino Series - Neo 21 (World)", 0, 0 )                          /* Id: NeoP00180 - Developer: Dyna - Releases: 1999-12-29 (JPN) */
	SOFTWARE( neobacca,   0,        2000, "SNK", "Pocket Casino Series - Neo Baccarat (World)", 0, 0 )                    /* Id: NeoP00190 - Developer: Dyna - Releases: 2000-06-22 (JPN) */
	SOFTWARE( neochercb,  neocherc, 1999, "SNK", "Pocket Casino Series - Neo Cherry Master Color (World, Prototype)", 0, 0 )
	SOFTWARE( neocherc,   0,        1999, "SNK", "Pocket Casino Series - Neo Cherry Master Color (World)", 0, 0 )         /* Id: NeoP00240 - Developer: Dyna - Releases: 1999-03-19 (JPN) */
	SOFTWARE( neodrag1,   neodrag,  1999, "SNK", "Pocket Casino Series - Neo Dragon's Wild (World, v1.11)", 0, 0 )
	SOFTWARE( neodrag,    0,        1999, "SNK", "Pocket Casino Series - Neo Dragon's Wild (World, v1.13)", 0, 0 )        /* Id: NeoP00150 - Developer: Dyna - Releases: 1999-03-19 (JPN) */
	SOFTWARE( neoderby,   0,        1999, "SNK", "Neo Derby Champ Daiyosou (Jpn)", 0, 0 )                                 /* Id: NeoP00170 - Developer: Dyna - Releases: 1999-04-22 (JPN) */
	SOFTWARE( neocuppl,   0,        1999, "SNK", "Pocket Sport Series - Neo Geo Cup '98 Plus (World)", 0, 0 )             /* Id: NeoP00390 - Developer: SNK - Releases: 1999-04-15 (JPN) */
	SOFTWARE( neomystr,   0,        1999, "SNK", "Pocket Casino Series - Neo Mystery Bonus (World)", 0, 0 )               /* Id: NeoP00160 - Developer: Dyna - Releases: 1999-03-19 (JPN) */
	SOFTWARE( neoproyk,   0,        1999, "SNK", "Pocket Sport Series - Neo Poke Pro Yakyuu (Jpn)", 0, 0 )                /* Id: NeoP00260 - Developer: ADK - Releases: 1999-05-27 (JPN) */
	SOFTWARE( neoturfm,   0,        1999, "SNK", "Pocket Sport Series - Big Tournament Golf ~ Neo Turf Masters (World)", 0, 0 )    /* Id: NeoP00350 - Developer: SNK - Releases: 1999-07-29 (JPN) */
	SOFTWARE( nigeronp,   0,        2000, "SNK", "Nige-ron-pa (Jpn)", COLOR_ONLY, 0 )                                     /* Id: NeoP00960 - Developer: Denno Eizo - Releases: 2000-11-23 (JPN) */
	SOFTWARE( oekakip,    0,        2000, "Success", "Oekaki Puzzle (Jpn)", COLOR_ONLY, 0 )                               /* Id: NeoP00810 - Developer: Success - Releases: 2000-04-27 (JPN) */
	SOFTWARE( pacworld,   0,        1999, "SNK", "Pac-Man (World)", 0, 0 )                                                /* Id: NeoP00550 - Developer: Namco - Releases: 1999-08-26 (JPN) */
	SOFTWARE( pslotazt,   0,        2000, "Aruze", "Pachi-Slot Aruze Oukoku Pocket - Azteca (Jpn)", COLOR_ONLY, 0 )       /* Id: NeoP00740 - Developer: Aruze - Releases: 2000-02-10 (JPN) */
	SOFTWARE( pslotdk2,   0,        2001, "Aruze", "Pachi-Slot Aruze Oukoku Pocket - Dekahel 2 (Jpn)", COLOR_ONLY, 0 )    /* Id: NeoP01080 - Developer: Aruze - Releases: 2001-01-25 (JPN) */
	SOFTWARE( pslotds2,   0,        2000, "Aruze", "Pachi-Slot Aruze Oukoku Pocket - Delsol 2 (Jpn)", COLOR_ONLY, 0 )     /* Id: NeoP01070 - Developer: Aruze - Releases: 2000-10-26 (JPN) */
	SOFTWARE( pslotecp,   0,        2001, "Aruze", "Pachi-Slot Aruze Oukoku Pocket - e-Cup (Jpn)", COLOR_ONLY, 0 )        /* Id: NeoP01120 - Developer: Aruze - Releases: 2001-03-09 (JPN) */
	SOFTWARE( pslothan1,  pslothan, 1999, "Aruze", "Pachi-Slot Aruze Oukoku Pocket - Hanabi (Jpn, v1.02)", 0, 0 )
	SOFTWARE( pslothan,   0,        1999, "Aruze", "Pachi-Slot Aruze Oukoku Pocket - Hanabi (Jpn, v1.04)", 0, 0 )         /* Id: NeoP00520 - Developer: Aruze - Releases: 1999-10-21 (JPN) */
	SOFTWARE( pslotooh,   0,        2000, "Aruze", "Pachi-Slot Aruze Oukoku Pocket - Oohanabi (Jpn)", COLOR_ONLY, 0 )     /* Id: NeoP01100 - Developer: Aruze - Releases: 2000-12-14 (JPN) */
	SOFTWARE( pslotpc2,   0,        2000, "Aruze", "Pachi-Slot Aruze Oukoku Pocket - Porcano 2 (Jpn)", COLOR_ONLY, 0 )    /* Id: NeoP01020 - Developer: Aruze - Releases: 2000-07-20 (JPN) */
	SOFTWARE( pslotwrd,   0,        2000, "Aruze", "Pachi-Slot Aruze Oukoku Pocket - Ward of Lights (Jpn)", COLOR_ONLY, 0 )  /* Id: NeoP00840 - Developer: Aruze - Releases: 2000-03-16 (JPN) */
	SOFTWARE( pachinko,   0,        1999, "Japan Vistec", "Pachinko Hisshou Guide - Pocket Parlor (Jpn)", 0, 0 )          /* Id: NeoP00460 - Developer: Japan Vistec - Releases: 1999-11-25 (JPN) */
	SOFTWARE( prtymail,   0,        1999, "SNK", "Party Mail (Jpn)", 0, 0 )                                               /* Id: NeoP00320 - Developer: ADK - Releases: 1999-12-22 (JPN) */
	SOFTWARE( picturep,   0,        2000, "SNK", "Picture Puzzle (Euro, USA)", 0, 0 )
	SOFTWARE( pocklove,   0,        1999, "KID", "Pocket Love if (Jpn)", 0, 0 )                                           /* Id: NeoP00440 - Developer: KID - Releases: 1999-10-21 (JPN) */
	SOFTWARE( pockrev,    0,        2000, "SNK", "Pocket Reversi (Euro)", 0, 0 )
	SOFTWARE( pockrevj,   0,        2000, "Success", "Pocket Reversi (Jpn)", COLOR_ONLY, 0 )                              /* Id: NeoP00790 - Developer: Success - Releases: 2000-01-27 (JPN) */
	SOFTWARE( ptennisc,   0,        1999, "SNK", "Pocket Sport Series - Pocket Tennis Color (World)", 0, 0 )              /* Id: NeoP00280 - Developer: Yumekobo - Releases: 1999-03-19 (JPN) */
	SOFTWARE( puyopop1,   puyopop,  1999, "SNK", "Puyo Pop (World, v1.05) ~ Puyo Pyuo Tsuu (Jpn, v1.05)", 0, 0 )
	SOFTWARE( puyopop,    0,        1999, "SNK", "Puyo Pop (World, v1.06) ~ Puyo Pyuo Tsuu (Jpn, v1.06)", 0, 0 )          /* Id: NeoP00410 - Developer: Sega - Releases: 1999-07-22 (JPN) */
	SOFTWARE( pbobbleb,   pbobble,  1999, "SNK", "Puzzle Bobble Mini (Euro, Jpn, Prototype)", 0, 0 )
	SOFTWARE( pbobblea,   pbobble,  1999, "SNK", "Puzzle Bobble Mini (Euro, Jpn, v1.09)", 0, 0 )
	SOFTWARE( pbobble,    0,        1999, "SNK", "Puzzle Bobble Mini (Euro, Jpn, v1.10)", 0, 0 )                          /* Id: NeoP00200 - Developer: Taito / Ukiyotei - Releases: 1999-03-26 (JPN) */
	SOFTWARE( puzzlink,   0,        1999, "SNK", "Puzzle Link (Euro)", 0, 0 )
	SOFTWARE( puzzlnk2,   0,        2000, "SNK", "Puzzle Link 2 (Euro, USA)", 0, 0 )
	SOFTWARE( renketsc,   0,        1999, "SNK", "Renketsu Puzzle Tsunagete Pon! Color (Jpn)", 0, 0 )                     /* Id: NeoP00290 - Developer: Yumekobo - Releases: 1999-03-19 (JPN) */
	SOFTWARE( rockmanb,   0,        2000, "Capcom", "Rockman - Battle & Fighters (Jpn)", 0, 0 )                           /* Id: NeoP00940 - Developer: Capcom - Releases: 2000-07-06 (JPN) */
	SOFTWARE( samsho,     0,        1998, "SNK", "Pocket Fighting Series - Samurai Spirit! (Jpn) ~ Samurai Shodown! (Euro)", 0, 0 )	/* Id: NeoP00080 - Developer: SNK - Releases: 1998-12-25 (JPN) */
	SOFTWARE( samsho2,    0,        1999, "SNK", "Pocket Fighting Series - Samurai Shodown! 2 (World)", 0, 0 )            /* Id: NeoP00300 - Developer: SNK - Releases: 1999-06-10 (JPN) */
	SOFTWARE( shanghai,   0,        1999, "SNK", "Shanghai Mini (World)", 0, 0 )                                          /* Id: NeoP00260 - Developer: Activision - Releases: 1999-07-29 (JPN) */
	SOFTWARE( evolutioj,  evolutio, 2000, "SNK", "Shinki Sekai Evolution - Hateshinai Dungeon (Jpn)", COLOR_ONLY, 0 )     /* Id: NeoP00820 - Developer: Entertainment Software Publishing - Releases: 2000-02-10 (JPN) */
	SOFTWARE( shougic,    0,        1999, "SNK", "Shougi no Tatsujin Color (Jpn)", 0, 0 )                                 /* Id: NeoP00270 - Developer: ADK - Releases: 1999-03-19 (JPN) */
	SOFTWARE( snkgalsj,   snkgals,  2000, "SNK", "SNK Gals' Fighters (Jpn)", COLOR_ONLY, 0 )                              /* Id: NeoP00650 - Developer: Yumekobo - Releases: 2000-01-27 (JPN) */
	SOFTWARE( snkgals,    0,        2000, "SNK", "SNK Gals' Fighters (Euro, USA)", 0, 0 )
	SOFTWARE( svccard2,   0,        2001, "SNK", "SNK vs. Capcom - Card Fighters 2 - Expand Edition (Jpn)", COLOR_ONLY, 0 )  /* Id: NeoP01160 - Developer: SNK - Releases: 2001-09-13 (JPN) */
	SOFTWARE( svccardc,   svccards, 1999, "SNK", "SNK vs. Capcom - Card Fighters' Clash - Capcom Cardfighter's Version (Euro, USA)", 0, 0 )
	SOFTWARE( svccards,   0,        1999, "SNK", "SNK vs. Capcom - Card Fighters' Clash - SNK Cardfighter's Version (Euro, USA)", 0, 0 )
	SOFTWARE( svccardj,   svccards, 1999, "SNK", "SNK vs. Capcom - Gekitotsu Card Fighters (Jpn, Prototype)", 0, 0 )
	SOFTWARE( svccardcj,  svccards, 1999, "SNK", "SNK vs. Capcom - Gekitotsu Card Fighters - Capcom Supporters Version (Jpn)", 0, 0 )  /* Id: NeoP00570 - Developer: SNK - Releases: 1999-10-21 (JPN) */
	SOFTWARE( svccardsj1, svccards, 1999, "SNK", "SNK vs. Capcom - Gekitotsu Card Fighters - SNK Supporters Version (Jpn, Prototype)", 0, 0 )
	SOFTWARE( svccardsj,  svccards, 1999, "SNK", "SNK vs. Capcom - Gekitotsu Card Fighters - SNK Supporters Version (Jpn)", 0, 0 )  /* Id: NeoP00560 - Developer: SNK - Releases: 1999-10-21 (JPN) */
	SOFTWARE( svc,        0,        1999, "SNK", "SNK vs. Capcom - The Match of the Millennium (World) ~ Choujou Kessen Saikyou Fighters - SNK vs Capcom (Jpn)", COLOR_ONLY, 0 )  /* Id: NeoP00690 - Developer: SNK - Releases: 1999-12-22 (JPN) */
	SOFTWARE( sonicb,     sonic,    2000, "SNK", "Sonic the Hedgehog - Pocket Adventure (World, Prototype)", COLOR_ONLY, 0 )
	SOFTWARE( sonic,      0,        2000, "SNK", "Sonic the Hedgehog - Pocket Adventure (World)", COLOR_ONLY, 0 )         /* Id: NeoP00590 - Developer: Sega - Releases: 2000-05-25 (JPN) */
	SOFTWARE( soreike,    0,        1999, "Dyna", "Soreike!! Hanafuda Doujou (Jpn)", 0, 0 )                               /* Id: NeoP00780 - Developer: Dyna - Releases: 1999-12-16 (JPN) */
	SOFTWARE( srmp,       0,        2001, "SNK", "Super Real Mahjong - Premium Collection (Jpn)", COLOR_ONLY, 0 )         /* Id: NeoP01110 - Developer: Seta - Releases: 2001-03-09 (JPN) */
	SOFTWARE( tsunapn2,   0,        1999, "SNK", "Tsunagete Pon! 2 (Jpn)", COLOR_ONLY, 0 )                                /* Id: NeoP00490 - Developer: Yumekobo - Releases: 1999-11-11 (JPN) */
	SOFTWARE( wrestmad,   0,        19??, "<unknown>", "Wrestling Madness (Euro, USA, Prototype)", 0, 0 )
SOFTWARE_LIST_END

SOFTWARE_LIST( ngp_cart, "SNK Neo Geo Pocket cartridges" )
SOFTWARE_LIST( ngpc_cart, "SNK Neo Geo Pocket Color cartridges" )
