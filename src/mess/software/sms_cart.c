/***************************************************************************

    Sega Master System cartridges

***************************************************************************/

#include "emu.h"
#include "softlist.h"
#include "devices/cartslot.h"


#define SMS_ROM_LOAD( set, name, offset, length, hash )	\
SOFTWARE_START( set ) \
	ROM_REGION( 0x100000, CARTRIDGE_REGION_ROM, 0 ) \
	ROM_LOAD(name, offset, length, hash) \
SOFTWARE_END


SMS_ROM_LOAD( 20embr,    "20 em 1 (brazil).bin",                                                                   0x000000 , 0x40000,	 CRC(f0f35c22) SHA1(2012763dc08dedc68af8538698bd66618212785d) )
SMS_ROM_LOAD( aceoface,  "ace of aces (europe).bin",                                                               0x000000 , 0x40000,	 CRC(887d9f6b) SHA1(08a79905767b8e5af8a9c9c232342e6c47588093) )
SMS_ROM_LOAD( actionfg1, "action fighter (japan, europe) (v1.1).bin",                                              0x000000 , 0x20000,	 CRC(d91b340d) SHA1(5dbcfb75958f4cfa1b61d9ea114bab67787b113e) )
SMS_ROM_LOAD( actionfg,  "action fighter (usa, europe) (v1.2).bin",                                                0x000000 , 0x20000,	 CRC(3658f3e0) SHA1(b462246fed3cbb9dc3909a2d5befaec65d7a0014) )
SMS_ROM_LOAD( addamfam,  "addams family, the (europe).bin",                                                        0x000000 , 0x40000,	 CRC(72420f38) SHA1(3fc6ccc556a1e4eb376f77eef8f16b1ff76a17d0) )
SMS_ROM_LOAD( aerialas,  "aerial assault (europe).bin",                                                            0x000000 , 0x40000,	 CRC(ecf491cf) SHA1(2a9090ed365e7425ca7a59f87b942c16b376f0a3) )
SMS_ROM_LOAD( aerialasu, "aerial assault (usa).bin",                                                               0x000000 , 0x40000,	 CRC(15576613) SHA1(e57b1d4476ca33d18d9071e022ddc9cc702d6497) )
SMS_ROM_LOAD( afterb,    "after burner (world).bin",                                                               0x000000 , 0x80000,	 CRC(1c951f8e) SHA1(51531df038783c84640a0cab93122e0b59e3b69a) )
SMS_ROM_LOAD( airresc,   "air rescue (europe).bin",                                                                0x000000 , 0x40000,	 CRC(8b43d21d) SHA1(4edd1b62abbbf2220961a06eb139db1838fb700b) )
SMS_ROM_LOAD( aladdin,   "aladdin (europe).bin",                                                                   0x000000 , 0x80000,	 CRC(c8718d40) SHA1(585967f400473e289cda611e7686ae98ae43172e) )
SMS_ROM_LOAD( aleste,    "aleste (japan).bin",                                                                     0x000000 , 0x20000,	 CRC(d8c4165b) SHA1(cba75b0d54df3c9a8e399851a05c194c7a05fbfe) )
SMS_ROM_LOAD( alexhitw,  "alex kidd - high-tech world (usa, europe).bin",                                          0x000000 , 0x20000,	 CRC(013c0a94) SHA1(2d0a581da1c787b1407fb1cfefd0571e37899978) )
SMS_ROM_LOAD( alexlost,  "alex kidd - the lost stars (world).bin",                                                 0x000000 , 0x40000,	 CRC(c13896d5) SHA1(6dbf2684c3dfea7442d0b40a9ff7c8b8fc9b1b98) )
SMS_ROM_LOAD( alexbmx,   "alex kidd bmx trial (japan).bin",                                                        0x000000 , 0x20000,	 CRC(f9dbb533) SHA1(77cc767bfae01e9cc81612c780c939ed954a6312) )
SMS_ROM_LOAD( alexkiddb, "alex kidd in miracle world (b) (v1.1) [p1][!].bin",                                      0x000000 , 0x20000,	 CRC(7545d7c2) SHA1(823c3051018c7acce15a7ff1d704eab5dd686a1f) )
SMS_ROM_LOAD( alexkidd,  "alex kidd in miracle world (usa, europe) (v1.1).bin",                                    0x000000 , 0x20000,	 CRC(aed9aac4) SHA1(6d052e0cca3f2712434efd856f733c03011be41c) )
SMS_ROM_LOAD( alexkidd1, "alex kidd in miracle world (usa, europe).bin",                                           0x000000 , 0x20000,	 CRC(17a40e29) SHA1(8cecf8ed0f765163b2657be1b0a3ce2a9cb767f4) )
SMS_ROM_LOAD( alexshin,  "alex kidd in shinobi world (usa, europe).bin",                                           0x000000 , 0x40000,	 CRC(d2417ed7) SHA1(e7c7c24e208afb986ab389883f98a1b5a8249fea) )
SMS_ROM_LOAD( alexkiddj, "alex kidd no miracle world (japan).bin",                                                 0x000000 , 0x20000,	 CRC(08c9ec91) SHA1(62fdc25501e17b87c355378562c3b1966e5f9008) )
SMS_ROM_LOAD( alf,       "alf (usa).bin",                                                                          0x000000 , 0x20000,	 CRC(82038ad4) SHA1(7706485b735f5d7f7a59c7d626b13b23e8696087) )
SMS_ROM_LOAD( alien3,    "alien 3 (europe).bin",                                                                   0x000000 , 0x40000,	 CRC(b618b144) SHA1(be40ffc72ee19620a8bac89d5d96bbafcefc74e7) )
SMS_ROM_LOAD( astorm,    "alien storm (europe).bin",                                                               0x000000 , 0x40000,	 CRC(7f30f793) SHA1(aece64ecbfbe08b199b29df9bc75743773ea3d34) )
SMS_ROM_LOAD( asyndromj, "alien syndrome (japan).bin",                                                             0x000000 , 0x40000,	 CRC(4cc11df9) SHA1(5d786476b275de34efb95f576dd556cf4b335a83) )
SMS_ROM_LOAD( asyndrom,  "alien syndrome (usa, europe).bin",                                                       0x000000 , 0x40000,	 CRC(5cbfe997) SHA1(0da0b9755b6a6ef145ec3b95e651d2a384b3f7f9) )
SMS_ROM_LOAD( altbeast,  "altered beast (usa, europe).bin",                                                        0x000000 , 0x40000,	 CRC(bba2fe98) SHA1(413986f174799f094a8dd776d91dcf018ee17290) )
SMS_ROM_LOAD( ameribb,   "american baseball (europe).bin",                                                         0x000000 , 0x40000,	 CRC(7b27202f) SHA1(fff4006fe47de8138db246af8863e28b81718abe) )
SMS_ROM_LOAD( ameripf,   "american pro football (europe).bin",                                                     0x000000 , 0x40000,	 CRC(3727d8b2) SHA1(986b860465fc9748c6be1815c0e4c0ea94473040) )
SMS_ROM_LOAD( andreaga,  "andre agassi tennis (europe).bin",                                                       0x000000 , 0x40000,	 CRC(f499034d) SHA1(81dbacad4739b98281c750d9af21606275398fc8) )
SMS_ROM_LOAD( anmitsu,   "anmitsu hime (japan).bin",                                                               0x000000 , 0x20000,	 CRC(fff63b0b) SHA1(74057d3b7f384c91871a2db2fbc86d3e700c45e5) )
SMS_ROM_LOAD( arcadesh,  "arcade smash hits (europe).bin",                                                         0x000000 , 0x40000,	 CRC(e4163163) SHA1(44ed3aeaa4c8a627b88c099b184ca99710fac0ad) )
SMS_ROM_LOAD( argosnj1,  "argos no juujiken (j) [a1][!].bin",                                                      0x000000 , 0x20000,	 CRC(32da4b0d) SHA1(6c11879612dc00c18f6d2a1823bdd26522dc9c91) )
SMS_ROM_LOAD( argosnj,   "argos no juujiken (japan).bin",                                                          0x000000 , 0x20000,	 CRC(bae75805) SHA1(a4b63ed417380f8170091e66c0417123799a731f) )
SMS_ROM_LOAD( arielmer,  "ariel - the little mermaid (brazil).bin",                                                0x000000 , 0x40000,	 CRC(f4b3a7bd) SHA1(77a35c0b622786183d6703a5d7546728db44b68d) )
SMS_ROM_LOAD( ashura,    "ashura (japan).bin",                                                                     0x000000 , 0x20000,	 CRC(ae705699) SHA1(e9f90d63320295e4bd9a87e6078186c5efb7e84e) )
SMS_ROM_LOAD( assaultc,  "assault city (europe) (light phaser).bin",                                               0x000000 , 0x40000,	 CRC(861b6e79) SHA1(835217550ecb92422d887a3353ff43890c71566b) )
SMS_ROM_LOAD( assaultc1, "assault city (europe).bin",                                                              0x000000 , 0x40000,	 CRC(0bd8da96) SHA1(ea46350ce4827b282b73600a7f4feadbec7c0ed4) )
SMS_ROM_LOAD( asterix,   "asterix (europe) (en,fr) (v1.1).bin",                                                    0x000000 , 0x80000,	 CRC(8c9d5be8) SHA1(ae56c708bc197f462b68c3e5ff9f0379d841c8b0) )
SMS_ROM_LOAD( asterix1,  "asterix (europe) (en,fr).bin",                                                           0x000000 , 0x80000,	 CRC(147e02fa) SHA1(70e311421467acd01e55f1908eae653bf20de175) )
SMS_ROM_LOAD( astergre,  "asterix and the great rescue (europe) (en,fr,de,es,it).bin",                             0x000000 , 0x80000,	 CRC(f9b7d26b) SHA1(5e409247f6a611437380b7a9f0e3cccab5dd1987) )
SMS_ROM_LOAD( astermis,  "asterix and the secret mission (europe) (en,fr,de).bin",                                 0x000000 , 0x80000,	 CRC(def9b14e) SHA1(de6a32a548551553a4b3ae332f4bf98ed22d8ab5) )
SMS_ROM_LOAD( astrof1,   "astro flash (j) [p1][!].bin",                                                            0x000000 , 0x8000,	 CRC(0e21e6cf) SHA1(e522a7ad519091a0955a6cf54590a7bd9708c56f) )
SMS_ROM_LOAD( astrof,    "astro flash (japan).bin",                                                                0x000000 , 0x8000,	 CRC(c795182d) SHA1(8370957b1192349d0a610437cd5d91ea4e3892c4) )
SMS_ROM_LOAD( astrow,    "astro warrior & pit pot (europe).bin",                                                   0x000000 , 0x20000,	 CRC(69efd483) SHA1(2b3a9da256f2918b859ebcb6ffa1b36a09e7595d) )
SMS_ROM_LOAD( astrowj,   "astro warrior (japan, usa).bin",                                                         0x000000 , 0x20000,	 CRC(299cbb74) SHA1(901697a3535ad70190647f34ad5b30b695d54542) )
SMS_ROM_LOAD( ayrton,    "ayrton senna's super monaco gp ii (europe).bin",                                         0x000000 , 0x80000,	 CRC(e890331d) SHA1(b6819b014168aaa03b65dae97ba6cd5fa0d7f0d9) )
SMS_ROM_LOAD( aztecadv,  "aztec adventure - the golden road to paradise (world).bin",                              0x000000 , 0x20000,	 CRC(ff614eb3) SHA1(317775a17867530a8fe3a5b17b681d5ada351432) )
SMS_ROM_LOAD( backtof2,  "back to the future part ii (europe).bin",                                                0x000000 , 0x40000,	 CRC(e5ff50d8) SHA1(31af58e655e12728b01e7da64b46934979b82ecf) )
SMS_ROM_LOAD( backtof3,  "back to the future part iii (europe).bin",                                               0x000000 , 0x40000,	 CRC(2d48c1d3) SHA1(7d67dd38fea5dba4224a119cc4840f6fb8e023b9) )
SMS_ROM_LOAD( bakubaku,  "baku baku animal (brazil).bin",                                                          0x000000 , 0x40000,	 CRC(35d84dc2) SHA1(07d8f300b3a3542734fcd24fa8312fe99fbfef0e) )
SMS_ROM_LOAD( bankpani1, "bank panic (e) [p1][!].bin",                                                             0x000000 , 0x8000,	 CRC(b4dfb825) SHA1(983c10f9431e19e0f79627cf72d5425895fd32f8) )
SMS_ROM_LOAD( bankpani,  "bank panic (europe).bin",                                                                0x000000 , 0x8000,	 CRC(655fb1f4) SHA1(661bbe20f01b7afb242936d409fdd30420c6de5f) )
SMS_ROM_LOAD( basketni,  "basket ball nightmare (europe).bin",                                                     0x000000 , 0x40000,	 CRC(0df8597f) SHA1(0fa1156931c83763bc6906efce75045327cdd7aa) )
SMS_ROM_LOAD( batmanre,  "batman returns (europe).bin",                                                            0x000000 , 0x40000,	 CRC(b154ec38) SHA1(0ccc0e2d91a345c39a7406e148da147a2edce979) )
SMS_ROM_LOAD( battleor,  "battle out run (europe).bin",                                                            0x000000 , 0x40000,	 CRC(c19430ce) SHA1(cfa8721d4fc71b1f14e9a06f2db715a6f88be7dd) )
SMS_ROM_LOAD( battlem,   "battlemaniacs (brazil).bin",                                                             0x000000 , 0x40000,	 CRC(1cbb7bf1) SHA1(0854e36d3bb011e712a06633f188c0d64cd65893) )
SMS_ROM_LOAD( blackblt,  "black belt (usa, europe).bin",                                                           0x000000 , 0x20000,	 CRC(da3a2f57) SHA1(7c5524cff2de9b694e925297e8e74c7c8d292e46) )
SMS_ROM_LOAD( bladeag1,  "blade eagle (world) (beta).bin",                                                         0x000000 , 0x40000,	 CRC(58d5fc48) SHA1(9cdfda85b4fa4e689617d8bcbdd6478b19d215ca) )
SMS_ROM_LOAD( bladeag,   "blade eagle (world).bin",                                                                0x000000 , 0x40000,	 CRC(8ecd201c) SHA1(b786d15b26b914c24cd1c36a8fca41b215c0a4e7) )
SMS_ROM_LOAD( bomber,    "bomber raid (world).bin",                                                                0x000000 , 0x40000,	 CRC(3084cf11) SHA1(d754ef2b6e05c76502c02c71dbfcf6150ee12f6f) )
SMS_ROM_LOAD( bonanza,   "bonanza bros. (europe).bin",                                                             0x000000 , 0x40000,	 CRC(caea8002) SHA1(bbaedefa0bb489ece4bbd965f09a417be4b76cc7) )
SMS_ROM_LOAD( bonkers,   "bonkers wax up! (brazil).bin",                                                           0x000000 , 0x80000,	 CRC(b3768a7a) SHA1(e1f8da3897f0756c8764ece6605f940ce79e81ca) )
SMS_ROM_LOAD( bramst,    "bram stoker's dracula (europe).bin",                                                     0x000000 , 0x40000,	 CRC(1b10a951) SHA1(6c9f52cdae96541020eaaa543ca6f729763f3ada) )
SMS_ROM_LOAD( bublbobl,  "bubble bobble (europe).bin",                                                             0x000000 , 0x40000,	 CRC(e843ba7e) SHA1(44876b44089b4174858e54202e567e02efa76859) )
SMS_ROM_LOAD( buggyrun,  "buggy run (europe).bin",                                                                 0x000000 , 0x80000,	 CRC(b0fc4577) SHA1(4b1975190ac9d6281325de0925980283fdce51ca) )
SMS_ROM_LOAD( calgames,  "california games (usa, europe).bin",                                                     0x000000 , 0x40000,	 CRC(ac6009a7) SHA1(d0f8298bb2a30a3569c65372a959612df3b608db) )
SMS_ROM_LOAD( calgame2b, "california games ii (brazil, korea).bin",                                                0x000000 , 0x40000,	 CRC(45c50294) SHA1(4d6c46dedfe38fcfb740e948563b8eeec3bd4305) )
SMS_ROM_LOAD( calgame2,  "california games ii (europe).bin",                                                       0x000000 , 0x40000,	 CRC(c0e25d62) SHA1(5c7d99ba54caf9a674328df787a89e0ab4730de8) )
SMS_ROM_LOAD( captsilv,  "captain silver (japan, europe).bin",                                                     0x000000 , 0x40000,	 CRC(a4852757) SHA1(88402392e93b220632a61e0c07731a7ed087cbef) )
SMS_ROM_LOAD( captsilvu, "captain silver (usa).bin",                                                               0x000000 , 0x20000,	 CRC(b81f6fa5) SHA1(7c2b23f4a806c89a533f27e190499243e7311c47) )
SMS_ROM_LOAD( casino,    "casino games (usa, europe).bin",                                                         0x000000 , 0x40000,	 CRC(3cff6e80) SHA1(8353b86965a87c724b95bb768d00dc84eeadce96) )
SMS_ROM_LOAD( castelo,   "castelo ra-tim-bum (brazil).bin",                                                        0x000000 , 0x80000,	 CRC(31ffd7c3) SHA1(4e40155720957a0ca7cf67d7c99bbc178e2f0fd4) )
SMS_ROM_LOAD( castlills, "castle of illusion - starring mickey mouse (unknown) (sample).bin",                      0x000000 , 0x8000,	 CRC(bd610939) SHA1(f00a3c8200a67579dbc9f1d2290d5a509de36eae) )
SMS_ROM_LOAD( castlill,  "castle of illusion starring mickey mouse (europe).bin",                                  0x000000 , 0x40000,	 CRC(953f42e1) SHA1(c200b5e585d59f8bfcbb40fd6d4314de8abcfae3) )
SMS_ROM_LOAD( castlillu, "castle of illusion starring mickey mouse (usa).bin",                                     0x000000 , 0x40000,	 CRC(b9db4282) SHA1(c31d80429801e8c927cb0536d66a16d51788ff4f) )
SMS_ROM_LOAD( champeur,  "champions of europe (europe).bin",                                                       0x000000 , 0x40000,	 CRC(23163a12) SHA1(e1d0b1b25b7d9bb423dadfe792bd177e01bc2ca2) )
SMS_ROM_LOAD( chmphock,  "championship hockey (europe).bin",                                                       0x000000 , 0x40000,	 CRC(7e5839a0) SHA1(d11eefe122de42a73d221d9efde1086d4a8ce147) )
SMS_ROM_LOAD( chapolim,  "chapolim x dracula - um duelo assustador (brazil).bin",                                  0x000000 , 0x20000,	 CRC(492c7c6e) SHA1(6590080f5db87afab1286f11ce77c60c3167b2b7) )
SMS_ROM_LOAD( cheese,    "cheese cat-astrophe starring speedy gonzales (europe) (en,fr,de,es).bin",                0x000000 , 0x80000,	 CRC(46340c41) SHA1(09edc943fc6da8657231b09f75d5e5c6bbbac24d) )
SMS_ROM_LOAD( chopliftj, "choplifter (japan) (proto).bin",                                                         0x000000 , 0x20000,	 CRC(16ec3575) SHA1(c8e87b309bbae6af7cf05602ffbd28f9495c83d8) )
SMS_ROM_LOAD( choplift,  "choplifter (usa, europe).bin",                                                           0x000000 , 0x20000,	 CRC(4a21c15f) SHA1(856e741eec9692fcc3b22c5c5642f54482e6e00b) )
SMS_ROM_LOAD( borgman,   "chouon senshi borgman (japan).bin",                                                      0x000000 , 0x20000,	 CRC(e421e466) SHA1(9f987e022090a40506b78d89523e9f88b3bb0c0b) )
SMS_ROM_LOAD( borgmanp,  "chouon senshi borgman [proto] (jp).bin",                                                 0x000000,  0x20000,	 CRC(86f49ae9) SHA1(4323cf3db733c1ff3d48771dc084d342cdcf43b8) )
SMS_ROM_LOAD( chuckrck,  "chuck rock (europe).bin",                                                                0x000000 , 0x80000,	 CRC(dd0e2927) SHA1(0199c62afb5c09f09999a4815079875b480129f3) )
SMS_ROM_LOAD( chukrck2b, "chuck rock ii - son of chuck (brazil).bin",                                              0x000000 , 0x80000,	 CRC(87783c04) SHA1(b966120d9eacea683bc136c405c50a81763ecab8) )
SMS_ROM_LOAD( chukrck2,  "chuck rock ii - son of chuck (europe).bin",                                              0x000000 , 0x80000,	 CRC(c30e690a) SHA1(46c326d7eb73b0393de7fc40bf2ee094ebab482d) )
SMS_ROM_LOAD( circuit,   "circuit, the (japan).bin",                                                               0x000000 , 0x20000,	 CRC(8fb75994) SHA1(34307a745d3d52a4b814e9831b7041f25e8052d1) )
SMS_ROM_LOAD( cloudmst,  "cloud master (usa, europe).bin",                                                         0x000000 , 0x40000,	 CRC(e7f62e6d) SHA1(b936276b272d8361bca8d7b05d1ebc59f1f639bc) )
SMS_ROM_LOAD( colors,    "color & switch test (unknown) (v1.3).bin",                                               0x000000 , 0x8000,	 CRC(7253c3ec) SHA1(3d23afcc802cd414849b0eac69ce712bf2fa72ff) )
SMS_ROM_LOAD( columns,   "columns (usa, europe).bin",                                                              0x000000 , 0x20000,	 CRC(665fda92) SHA1(3d16b0954b5419b071de270b44d38fc6570a8439) )
SMS_ROM_LOAD( columnsp,  "columns [proto].bin",                                                                    0x000000,  0x20000,	 CRC(3fb40043) SHA1(c41bbfe7c20b1c582d43c16671829b9be703a3de) )
SMS_ROM_LOAD( comical,   "comical machine gun joe (japan).bin",                                                    0x000000 , 0x8000,	 CRC(9d549e08) SHA1(33c21d164fd3cdf7aa9e7e0fe1a3ae5a491bd9f5) )
SMS_ROM_LOAD( coolspot,  "cool spot (europe).bin",                                                                 0x000000 , 0x40000,	 CRC(13ac9023) SHA1(cf36c1900d1c658cbfd974464761d145af3467c8) )
SMS_ROM_LOAD( cosmic,    "cosmic spacehead (europe) (en,fr,de,es).bin",                                            0x000000 , 0x40000,	 CRC(29822980) SHA1(f46f716dd34a1a5013a2d8a59769f6ef7536a567) )
SMS_ROM_LOAD( cybers,    "cyber shinobi, the (europe).bin",                                                        0x000000 , 0x40000,	 CRC(1350e4f8) SHA1(991803feb42a6f0f93ac0e97854132736def2933) )
SMS_ROM_LOAD( cyborgh,   "cyborg hunter (usa, europe).bin",                                                        0x000000 , 0x20000,	 CRC(908e7524) SHA1(b6131585cb944d7fae69ad609802a1b5d51b442f) )
SMS_ROM_LOAD( daffy,     "daffy duck in hollywood (europe) (en,fr,de,es,it).bin",                                  0x000000 , 0x80000,	 CRC(71abef27) SHA1(50ab645971b7d9c25a0a93080f70f3a9c6910c59) )
SMS_ROM_LOAD( dallye,    "dallyeora pigu-wang (korea) (unl).bin",                                                  0x000000 , 0x6c000,	 CRC(89b79e77) SHA1(9ebd5a7b025e980795eaac206f3f6f5dc297084f) )
SMS_ROM_LOAD( danan,     "danan - the jungle fighter (europe).bin",                                                0x000000 , 0x40000,	 CRC(ae4a28d7) SHA1(c99f2562117a2bf7100a3992608e9a2bcb50df35) )
SMS_ROM_LOAD( deadang,   "dead angle (usa, europe).bin",                                                           0x000000 , 0x40000,	 CRC(e2f7193e) SHA1(0236c5239c924b425a99367260b9ebfa8b8e0bca) )
SMS_ROM_LOAD( deepduck,  "deep duck trouble starring donald duck (europe).bin",                                    0x000000 , 0x80000,	 CRC(42fc3a6e) SHA1(26ec82b96650a7329b66bf90b54b869c1ec12f6b) )
SMS_ROM_LOAD( desert,    "desert speedtrap starring road runner and wile e. coyote (europe) (en,fr,de,es,it).bin", 0x000000 , 0x40000,	 CRC(b137007a) SHA1(60e2b6ec69d73dd73c1ef846634942c81800655b) )
SMS_ROM_LOAD( dstrike,   "desert strike (europe) (en,fr,de,es).bin",                                               0x000000 , 0x80000,	 CRC(6c1433f9) SHA1(e6181baef80ecc88b3eb82a46cf93793f06e01f1) )
SMS_ROM_LOAD( dicktr,    "dick tracy (usa, europe).bin",                                                           0x000000 , 0x40000,	 CRC(f6fab48d) SHA1(b788f0394aafbc213e6fa6dcfae40ebb7659f533) )
SMS_ROM_LOAD( dinobash,  "dinobasher starring bignose the caveman (europe) (proto).bin",                           0x000000 , 0x40000,	 CRC(ea5c3a6f) SHA1(05b4f23e33ada08e0a8b1fc6feccd8a97c690a21) )
SMS_ROM_LOAD( dinodool,  "dinosaur dooley, the (korea) (unl).bin",                                                 0x000000 , 0x20000,	 CRC(32f4b791) SHA1(f9e14ea9ce5f11ca5cb831f3eaf87609d7aea8f2) )
SMS_ROM_LOAD( dokidoki,  "doki doki penguin land - uchuu daibouken (japan).bin",                                   0x000000 , 0x20000,	 CRC(2bcdb8fa) SHA1(c01cf44eee335d509dc20a165add8514e7fbb7c4) )
SMS_ROM_LOAD( ddragon,   "double dragon (world).bin",                                                              0x000000 , 0x40000,	 CRC(a55d89f3) SHA1(cad5532af94ed75c0ada8820a83fa04d22f7bef5) )
SMS_ROM_LOAD( doublhwk1, "double hawk (europe) (beta).bin",                                                        0x000000 , 0x40000,	 CRC(f76d5cee) SHA1(44e30fea309911a9c114b8e31ca7ec5584295b21) )
SMS_ROM_LOAD( doublhwk,  "double hawk (europe).bin",                                                               0x000000 , 0x40000,	 CRC(8370f6cd) SHA1(d2428baf22da8a70a08ff35389d59030ce764372) )
SMS_ROM_LOAD( doubltgt,  "double target - cynthia no nemuri (japan).bin",                                          0x000000 , 0x20000,	 CRC(52b83072) SHA1(51f9ce05e383983ce1fe930ec178406b277db69c) )
SMS_ROM_LOAD( drhello,   "dr. hello (k).bin",                                                                      0x000000 , 0x8000,	 CRC(16537865) SHA1(8206e4b1df9468f4f60e654a0a4044083bd6e5d1) )
SMS_ROM_LOAD( drrobotn,  "dr. robotnik's mean bean machine (europe).bin",                                          0x000000 , 0x40000,	 CRC(6c696221) SHA1(89df035da8de68517f82bdf176d3b3f2edcd9e31) )
SMS_ROM_LOAD( dragon,    "dragon - the bruce lee story (europe).bin",                                              0x000000 , 0x40000,	 CRC(c88a5064) SHA1(3d41a4e3b9ffc3e2ba87bd89baf13271f8560775) )
SMS_ROM_LOAD( dragoncr,  "dragon crystal (europe).bin",                                                            0x000000 , 0x20000,	 CRC(9549fce4) SHA1(021c6983fdab4b0215ca324734deef0d32c29562) )
SMS_ROM_LOAD( dynaduke,  "dynamite duke (europe).bin",                                                             0x000000 , 0x40000,	 CRC(07306947) SHA1(258901a74176fc78f9c669cd7d716da0c872ca67) )
SMS_ROM_LOAD( dynadux,   "dynamite dux (europe).bin",                                                              0x000000 , 0x40000,	 CRC(0e1cc1e0) SHA1(2a513aef0f0bdcdf4aaa71e7b26a15ce686db765) )
SMS_ROM_LOAD( dynahead,  "dynamite headdy (brazil).bin",                                                           0x000000 , 0x80000,	 CRC(7db5b0fa) SHA1(12877c1c9bfba6462606717cf0a94f700ac970e4) )
SMS_ROM_LOAD( eswatc,    "e-swat - city under siege (usa, europe) (easy version).bin",                             0x000000 , 0x40000,	 CRC(c4bb1676) SHA1(075297d2f3a8ec4c399eaeab6b60e246e11b41fe) )
SMS_ROM_LOAD( eswatc1,   "e-swat - city under siege (usa, europe) (hard version).bin",                             0x000000 , 0x40000,	 CRC(c10fce39) SHA1(c481b4e5ca136fbb4105ae465259125392faffd3) )
SMS_ROM_LOAD( ejim,      "earthworm jim (brazil).bin",                                                             0x000000 , 0x80000,	 CRC(c4d5efc5) SHA1(a1966c2d8e75ea17df461a46c4a1a8b0b5fecd4e) )
SMS_ROM_LOAD( eccotide,  "ecco - the tides of time (brazil).bin",                                                  0x000000 , 0x80000,	 CRC(7c28703a) SHA1(61cef405e5bc71f1a603881c025bc245a8d14be4) )
SMS_ROM_LOAD( ecco,      "ecco the dolphin (europe).bin",                                                          0x000000 , 0x80000,	 CRC(6687fab9) SHA1(3bfdef48f27f2e53e2c31cb9626606a1541889dd) )
SMS_ROM_LOAD( enduroj,   "enduro racer (japan).bin",                                                               0x000000 , 0x40000,	 CRC(5d5c50b3) SHA1(7a0216af8d4a5aeda1d42e2703f140d08b3f92f6) )
SMS_ROM_LOAD( enduro,    "enduro racer (usa, europe).bin",                                                         0x000000 , 0x20000,	 CRC(00e73541) SHA1(10dc132166c1aa47ca7b89fbb1f0a4e56b428359) )
SMS_ROM_LOAD( excdizzy,  "excellent dizzy collection, the (usa, europe) (en,fr,de,es,it) (proto).bin",             0x000000 , 0x40000,	 CRC(8813514b) SHA1(09a2acf3ed90101be2629384c5c702f6a5408035) )
SMS_ROM_LOAD( f16fight,  "f-16 fighter (usa, europe).bin",                                                         0x000000 , 0x8000,	 CRC(eaebf323) SHA1(de63ca2f59adb94fac87623fe68de75940397449) )
SMS_ROM_LOAD( f16falcj,  "f-16 fighting falcon (japan).bin",                                                       0x000000 , 0x8000,	 CRC(7ce06fce) SHA1(0f60f545ce99366860a94fbb115ce7d8531ab7ba) )
SMS_ROM_LOAD( f16falc,   "f-16 fighting falcon (usa).bin",                                                         0x000000 , 0x8000,	 CRC(184c23b7) SHA1(a8f083b3db721b672b5b023d673a64577cb48ef3) )
SMS_ROM_LOAD( f1,        "f1 (europe).bin",                                                                        0x000000 , 0x40000,	 CRC(ec788661) SHA1(247fd74485073695a88f6813482f67516860b3a0) )
SMS_ROM_LOAD( family,    "family games (japan).bin",                                                               0x000000 , 0x20000,	 CRC(7abc70e9) SHA1(dfbf0186d497cf53d1f21a9430991ed4124cc4c2) )
SMS_ROM_LOAD( fantdizz,  "fantastic dizzy (europe) (en,fr,de,es,it).bin",                                          0x000000 , 0x40000,	 CRC(b9664ae1) SHA1(4202ce26832046c7ca8209240f097a8a0a84d981) )
SMS_ROM_LOAD( fantzonej, "fantasy zone (japan).bin",                                                               0x000000 , 0x20000,	 CRC(0ffbcaa3) SHA1(b486d8be0cf669117e134f9e53a452bf71097479) )
SMS_ROM_LOAD( fantzone1, "fantasy zone (world) (v1.1) (beta).bin",                                                 0x000000 , 0x20000,	 CRC(f46264fe) SHA1(b46cb68c330dad61cd2a2ef1f5f5480af303816a) )
SMS_ROM_LOAD( fantzone,  "fantasy zone (world) (v1.2).bin",                                                        0x000000 , 0x20000,	 CRC(65d7e4e0) SHA1(0278cd120dc3a7707eda9314c46c7f27f9e8fdda) )
SMS_ROM_LOAD( fantzonm,  "fantasy zone - the maze (usa, europe).bin",                                              0x000000 , 0x20000,	 CRC(d29889ad) SHA1(a2cd356826c8116178394d8aba444cb636ef784e) )
SMS_ROM_LOAD( fantzon2j, "fantasy zone ii - opa-opa no namida (japan).bin",                                        0x000000 , 0x40000,	 CRC(c722fb42) SHA1(60d8135e8f15fe48f504d0ec69010a4b886dcda8) )
SMS_ROM_LOAD( fantzon2,  "fantasy zone ii - the tears of opa-opa (usa, europe).bin",                               0x000000 , 0x40000,	 CRC(b8b141f9) SHA1(b84831378c7c19798b6fd560647e2941842bb80a) )
SMS_ROM_LOAD( felipe,    "felipe em acao (b).bin",                                                                 0x000000 , 0x8000,	 CRC(ccb2cab4) SHA1(b7601ff0116490b85c9bd50109db175ce2dd9104) )
SMS_ROM_LOAD( ferias,    "ferias frustradas do pica pau (brazil).bin",                                             0x000000 , 0x80000,	 CRC(bf6c2e37) SHA1(81dd4d7f1376e639cabebecdc821e9b5a635952b) )
SMS_ROM_LOAD( fifa,      "fifa international soccer (brazil) (en,es,pt).bin",                                      0x000000 , 0x80000,	 CRC(9bb3b5f9) SHA1(360073cb28e87a05b7f3a5922a24601981330046) )
SMS_ROM_LOAD( fbubbobl,  "final bubble bobble (japan).bin",                                                        0x000000 , 0x40000,	 CRC(3ebb7457) SHA1(ab585612fddb90b5285e2804f63fd7fb7ba02900) )
SMS_ROM_LOAD( fireforg,  "fire & forget ii (europe).bin",                                                          0x000000 , 0x40000,	 CRC(f6ad7b1d) SHA1(f934b35d27330cc2737d6a2d590dcef56004b983) )
SMS_ROM_LOAD( fireice,   "fire & ice (brazil).bin",                                                                0x000000 , 0x40000,	 CRC(8b24a640) SHA1(0e12ce919cda400b8681e18bdad31ba74f07a92b) )
SMS_ROM_LOAD( flash,     "flash, the (europe).bin",                                                                0x000000 , 0x40000,	 CRC(be31d63f) SHA1(1732b4c13fd00dd5efc5bf1ccb1ab6ed3889c8ba) )
SMS_ROM_LOAD( flints,    "flintstones, the (europe).bin",                                                          0x000000 , 0x40000,	 CRC(ca5c78a5) SHA1(a0aa76d89f6831999c328877057a99e72c6b533b) )
SMS_ROM_LOAD( forgottn,  "forgotten worlds (europe).bin",                                                          0x000000 , 0x40000,	 CRC(38c53916) SHA1(3f034429b23b6976c961595c1bcbd68826cb760d) )
SMS_ROM_LOAD( fushigi1,  "fushigi no oshiro pit pot (j) [p1][!].bin",                                              0x000000 , 0x8000,	 CRC(5d08e823) SHA1(1298aab2a29071e258dcbb962c3b80f8fc337b7a) )
SMS_ROM_LOAD( fushigi,   "fushigi no oshiro pit pot (japan).bin",                                                  0x000000 , 0x8000,	 CRC(e6795c53) SHA1(b1afa682b2f70bfc4ab2020d7c3047aabbaf9a24) )
SMS_ROM_LOAD( glocair,   "g-loc air battle (europe).bin",                                                          0x000000 , 0x40000,	 CRC(05cdc24e) SHA1(144bd2f8f6e480829c50f71baa370a838e8cec41) )
SMS_ROM_LOAD( gaegujan,  "gaegujangi ggachi (korea) (unl).bin",                                                    0x000000 , 0x80000,	 CRC(8b40f6bf) SHA1(320e3491da0266b46ff52103c23d25da2176e91c) )
SMS_ROM_LOAD( gaingrnd,  "gain ground (europe).bin",                                                               0x000000 , 0x40000,	 CRC(3ec5e627) SHA1(62c0ca61ad8f679f90f253ab6bbffd0c7737a8c0) )
SMS_ROM_LOAD( gaingrndp, "gain ground [proto].bin",                                                                0x000000,  0x40000,	 CRC(d40d03c7) SHA1(990491d559ebada59c8c7d7c7eeeae7dffa0399d) )
SMS_ROM_LOAD( galactpr,  "galactic protector (japan).bin",                                                         0x000000 , 0x20000,	 CRC(a6fa42d0) SHA1(e8bcc3621e30ee445a74e6ddbe0842d0a6753f36) )
SMS_ROM_LOAD( galaxyf,   "galaxy force (europe).bin",                                                              0x000000 , 0x80000,	 CRC(a4ac35d8) SHA1(54fe2f686fb9ec3e34b41d58778118b11f920440) )
SMS_ROM_LOAD( galaxyfu,  "galaxy force (usa).bin",                                                                 0x000000 , 0x80000,	 CRC(6c827520) SHA1(874289a1e8110312db48c5111fbf8e70b2426b5f) )
SMS_ROM_LOAD( gamebox,   "game box serie esportes radicais (brazil).bin",                                          0x000000 , 0x40000,	 CRC(1890f407) SHA1(c0ee197e93c9ec81f5b788e8d6b20b3ab57d2259) )
SMS_ROM_LOAD( gangster,  "gangster town (usa, europe).bin",                                                        0x000000 , 0x20000,	 CRC(5fc74d2a) SHA1(57f0972109cb6f9a74c65d70d6497bc02fdfc942) )
SMS_ROM_LOAD( gauntlet,  "gauntlet (europe).bin",                                                                  0x000000 , 0x20000,	 CRC(d9190956) SHA1(4e583ce9b95e20ecddc6c1dac9941c28a3e80808) )
SMS_ROM_LOAD( foremnko,  "george foreman's ko boxing (europe).bin",                                                0x000000 , 0x40000,	 CRC(a64898ce) SHA1(d0ddb71c6ca823c53d7e927a0de7de4b56745331) )
SMS_ROM_LOAD( gerald,    "geraldinho (brazil).bin",                                                                0x000000 , 0x8000,	 CRC(956c416b) SHA1(d82145b582e21ae5cb562030b5042ec85d440add) )
SMS_ROM_LOAD( ghosthj1,  "ghost house (j) [p1][!].bin",                                                            0x000000 , 0x8000,	 CRC(dabcc054) SHA1(4f93c7185c9ebdd70d9f7be56761bf9031934c6a) )
SMS_ROM_LOAD( ghosthj,   "ghost house (japan).bin",                                                                0x000000 , 0x8000,	 CRC(c0f3ce7e) SHA1(051e74c499c6792f891668a7be23a11c2c4087af) )
SMS_ROM_LOAD( ghosthb,   "ghost house (usa, europe) (beta).bin",                                                   0x000000 , 0x8000,	 CRC(c3e7c1ed) SHA1(f341e68bad5029bf475922c2c1b79eb109f467a1) )
SMS_ROM_LOAD( ghosth,    "ghost house (usa, europe).bin",                                                          0x000000 , 0x8000,	 CRC(f1f8ff2d) SHA1(cbce4c5d819be524f874ec9b60ca9442047a6681) )
SMS_ROM_LOAD( ghostbst,  "ghostbusters (usa, europe).bin",                                                         0x000000 , 0x20000,	 CRC(1ddc3059) SHA1(8945a9dfc99a2081a6fb74bbabf8feaac83a7e1a) )
SMS_ROM_LOAD( ghouls,    "ghouls'n ghosts (usa, europe).bin",                                                      0x000000 , 0x40000,	 CRC(7a92eba6) SHA1(b193e624795b2beb741249981d621cb650c658db) )
SMS_ROM_LOAD( globalb,   "global defense (usa, europe) (beta).bin",                                                0x000000 , 0x20000,	 CRC(91a0fc4e) SHA1(f92e1a9f7499b344e7865b18c042e09e7d614796) )
SMS_ROM_LOAD( global,    "global defense (usa, europe).bin",                                                       0x000000 , 0x20000,	 CRC(b746a6f5) SHA1(189ee1d4250a1f33e97053aa804a97b4e1467728) )
SMS_ROM_LOAD( dumpmats,  "gokuaku doumei dump matsumoto (japan).bin",                                              0x000000 , 0x20000,	 CRC(a249fa9d) SHA1(77f1e788f43fb59456f982472f02f109f53c7918) )
SMS_ROM_LOAD( gaxe,      "golden axe (usa, europe).bin",                                                           0x000000 , 0x80000,	 CRC(c08132fb) SHA1(d92538cb16a7a456255fa0da2bd8d0f588cd12ab) )
SMS_ROM_LOAD( gaxewarr,  "golden axe warrior (usa, europe).bin",                                                   0x000000 , 0x40000,	 CRC(c7ded988) SHA1(fbda0486b393708a89756bb57d116ad6007484e4) )
SMS_ROM_LOAD( golfamanb, "golfamania (europe) (beta).bin",                                                         0x000000 , 0x40000,	 CRC(5dabfdc3) SHA1(da88dc3e84daa2f8b8d803b00a13b5fb3185d8c5) )
SMS_ROM_LOAD( golfaman,  "golfamania (europe).bin",                                                                0x000000 , 0x40000,	 CRC(48651325) SHA1(d0b964dd7cd8ccdd730de4d8e4bb2e87bea7686e) )
SMS_ROM_LOAD( golvell,   "golvellius (usa, europe).bin",                                                           0x000000 , 0x40000,	 CRC(a51376fe) SHA1(cb8c2de9a8e91c0e4e60e5d4d9958e671d84da4c) )
SMS_ROM_LOAD( gprider,   "gp rider (europe).bin",                                                                  0x000000 , 0x80000,	 CRC(ec2da554) SHA1(3083069782c7cfcf2cc1229aca38f8f2971cf284) )
SMS_ROM_LOAD( greatbasj1, "great baseball (j) [p1][!].bin",                                                        0x000000 , 0x8000,	 CRC(c1e699db) SHA1(878852cb66427b0de11fba76bf6c8830a751d29f) )
SMS_ROM_LOAD( greatbasj, "great baseball (japan).bin",                                                             0x000000 , 0x8000,	 CRC(89e98a7c) SHA1(e6eaaec61bec32dee5161ae59a7a0902f0d05dc9) )
SMS_ROM_LOAD( greatbas,  "great baseball (usa, europe).bin",                                                       0x000000 , 0x20000,	 CRC(10ed6b57) SHA1(b332344eb529bad29dfb582633e787f7e42f71ae) )
SMS_ROM_LOAD( greatbsk,  "great basketball (world).bin",                                                           0x000000 , 0x20000,	 CRC(2ac001eb) SHA1(2fdb7ebce61e316ded27b575535d4f475c9dd822) )
SMS_ROM_LOAD( greatftb,  "great football (world).bin",                                                             0x000000 , 0x20000,	 CRC(2055825f) SHA1(a768f44ce7e50083ffe8c4b5e3ac93ceb7bd3266) )
SMS_ROM_LOAD( greatglfj, "great golf (japan).bin",                                                                 0x000000 , 0x20000,	 CRC(6586bd1f) SHA1(417739aa248032f5aebe05750a5de85346e36712) )
SMS_ROM_LOAD( greatglf1, "great golf (ue) (v1.0) [!].bin",                                                         0x000000 , 0x20000,	 CRC(c6611c84) SHA1(eab0eed872dd26b13bcf0b2dd74fcbbc078812c9) )
SMS_ROM_LOAD( greatglfb, "great golf (world) (beta).bin",                                                          0x000000 , 0x20000,	 CRC(4847bc91) SHA1(ce0662179bb0ca4a6491ef2be8839168b993c04e) )
SMS_ROM_LOAD( greatglf,  "great golf (world).bin",                                                                 0x000000 , 0x20000,	 CRC(98e4ae4a) SHA1(3c0b12cfb70f2515429b6e88e0753d69dbb907ab) )
SMS_ROM_LOAD( greatice,  "great ice hockey (japan, usa).bin",                                                      0x000000 , 0x10000,	 CRC(0cb7e21f) SHA1(bbe6bd6b59e5ae20cd1a4a8c72276aac7d2964a3) )
SMS_ROM_LOAD( greatscr,  "great soccer (europe).bin",                                                              0x000000 , 0x8000,	 CRC(0ed170c9) SHA1(7d1a381be96474f18ba1dac8eaf6710010b0e481) )
SMS_ROM_LOAD( greatscrj1, "great soccer (j) [p1][!].bin",                                                          0x000000 , 0x8000,	 CRC(84665648) SHA1(88611c50fcaa307bfd70b2bd5effeb39c2c4f3ae) )
SMS_ROM_LOAD( greatscrj, "great soccer (japan).bin",                                                               0x000000 , 0x8000,	 CRC(2d7fd7ef) SHA1(110536303b7bccc193bef4437ba5a9eb6fd4ac8e) )
SMS_ROM_LOAD( greattns,  "great tennis (japan).bin",                                                               0x000000 , 0x8000,	 CRC(95cbf3dd) SHA1(e7f3529689cd29be3fa02f94266e4ee8e0795d7d) )
SMS_ROM_LOAD( greatvolj, "great volleyball (japan).bin",                                                           0x000000 , 0x20000,	 CRC(6819b0c0) SHA1(b9e3284c7b557eed84661c98bc733d32b46c7a07) )
SMS_ROM_LOAD( greatvol,  "great volleyball (usa, europe).bin",                                                     0x000000 , 0x20000,	 CRC(8d43ea95) SHA1(133ffdde0a5a0ce951c667a4c5d7f9d9f35e9658) )
SMS_ROM_LOAD( hajafuin,  "haja no fuuin (japan).bin",                                                              0x000000 , 0x40000,	 CRC(b9fdf6d9) SHA1(46a032004d49fec58099aa6bf0dd796997e95142) )
SMS_ROM_LOAD( hangonaw,  "hang-on & astro warrior (usa).bin",                                                      0x000000 , 0x20000,	 CRC(1c5059f0) SHA1(ae29f676846fc740a2cfc69756875b6480265f97) )
SMS_ROM_LOAD( hangonsh,  "hang-on & safari hunt (usa).bin",                                                        0x000000 , 0x20000,	 CRC(e167a561) SHA1(7c741493889788d4511979bcaa3c48708d6240ed) )
SMS_ROM_LOAD( hangon,    "hang-on (europe).bin",                                                                   0x000000 , 0x8000,	 CRC(071b045e) SHA1(e601257f6477b85eb0b25a5b6d46ebc070d8a05a) )
SMS_ROM_LOAD( hangonj,   "hang-on (japan).bin",                                                                    0x000000 , 0x8000,	 CRC(5c01adf9) SHA1(43552f58f0c0c292f3e4c1b1525fd0344dc220c6) )
SMS_ROM_LOAD( heavyw,    "heavyweight champ (europe).bin",                                                         0x000000 , 0x40000,	 CRC(fdab876a) SHA1(7e2523061df0c08b7df7b446b5504c4eb0fea163) )
SMS_ROM_LOAD( heroes,    "heroes of the lance (europe).bin",                                                       0x000000 , 0x80000,	 CRC(cde13ffb) SHA1(16b2219a1493c06d18c973fc550ea563c3116207) )
SMS_ROM_LOAD( highsc,    "high school! kimengumi (japan).bin",                                                     0x000000 , 0x20000,	 CRC(9eb1aa4f) SHA1(d3c0aeeacccef77c45ab4219c7d6d8ed04d467cb) )
SMS_ROM_LOAD( hokuto1,   "hokuto no ken (j) [p1][!].bin",                                                          0x000000 , 0x20000,	 CRC(656d1a3e) SHA1(e77c7150ad274fb9d7df378e460e64daa9b9e9b6) )
SMS_ROM_LOAD( hokuto,    "hokuto no ken (japan).bin",                                                              0x000000 , 0x20000,	 CRC(24f5fe8c) SHA1(26c5da3ee48bc0f8fd3d20f9408e584242edcd9d) )
SMS_ROM_LOAD( homea,     "home alone (europe).bin",                                                                0x000000 , 0x40000,	 CRC(c9dbf936) SHA1(9b9be300fdc386f864af516000ffa3a53f1613e2) )
SMS_ROM_LOAD( hook,      "hook (europe) (proto).bin",                                                              0x000000 , 0x40000,	 CRC(9ced34a7) SHA1(b7bbd78b301244d7ce83f79d72fd28c56a870905) )
SMS_ROM_LOAD( hoshiw,    "hoshi wo sagasite... (japan).bin",                                                       0x000000 , 0x40000,	 CRC(955a009e) SHA1(f9ce8b80d8671db6ab38ba5b7ce46324a65ebc3d) )
SMS_ROM_LOAD( hwaran,    "hwarang ui geom (korea).bin",                                                            0x000000 , 0x40000,	 CRC(e4b7c56a) SHA1(cfee7ce6fadf4be1e8418451ae4a1f019de012f8) )
SMS_ROM_LOAD( impmissb,  "impossible mission (europe) (beta).bin",                                                 0x000000 , 0x20000,	 CRC(71c4ca8f) SHA1(9c6c28610603d05664d9ae44b62b5c1ac47f829c) )
SMS_ROM_LOAD( impmiss,   "impossible mission (europe).bin",                                                        0x000000 , 0x20000,	 CRC(64d6af3b) SHA1(d883f28e77e575edca6dcb1c4cd1f2b1f11393b2) )
SMS_ROM_LOAD( incred,    "incredible crash dummies, the (europe).bin",                                             0x000000 , 0x40000,	 CRC(b4584dde) SHA1(94a4ba183de82fc0066a0edab2acaee5e8bdd0e7) )
SMS_ROM_LOAD( incrhulk,  "incredible hulk, the (europe).bin",                                                      0x000000 , 0x80000,	 CRC(be9a7071) SHA1(235ad7d259023610d8aa59d066aaf0dba2ff8138) )
SMS_ROM_LOAD( indycrusb, "indiana jones and the last crusade (europe) (beta).bin",                                 0x000000 , 0x40000,	 CRC(acec894d) SHA1(5f29a0fc637cf7bd175a3e7be720530bde23402e) )
SMS_ROM_LOAD( indycrus,  "indiana jones and the last crusade (europe).bin",                                        0x000000 , 0x40000,	 CRC(8aeb574b) SHA1(68e23692a12628dde805ded9de356c5e19e4eba6) )
SMS_ROM_LOAD( jamesb,    "james 'buster' douglas knockout boxing (usa).bin",                                       0x000000 , 0x40000,	 CRC(6a664405) SHA1(b07000feb0c74824f2e3e74fd415631a8f3c4da6) )
SMS_ROM_LOAD( jamesbp,   "james buster douglas knockout boxing [proto].bin",                                       0x000000,  0x40000,	 CRC(cfb4bd7b) SHA1(d96c174e63393406d4b442e5796228eebb7deb93) )
SMS_ROM_LOAD( jb007b,    "james bond 007 - the duel (brazil).bin",                                                 0x000000 , 0x40000,	 CRC(8feff688) SHA1(cc3eec4da3758fe9e407ab80fa88dc952d33cdd5) )
SMS_ROM_LOAD( jb007,     "james bond 007 - the duel (europe).bin",                                                 0x000000 , 0x40000,	 CRC(8d23587f) SHA1(89f86869b90af986bee2acff44defe420e405a1e) )
SMS_ROM_LOAD( robocod,   "james pond 2 - codename robocod (europe).bin",                                           0x000000 , 0x80000,	 CRC(102d5fea) SHA1(df4f55f7ff9236f65aee737743e7500c4d96cf12) )
SMS_ROM_LOAD( joemont,   "joe montana football (usa, europe).bin",                                                 0x000000 , 0x40000,	 CRC(0a9089e5) SHA1(7452f7286cee78ce4bbd05841a4d087fdfba12e3) )
SMS_ROM_LOAD( jungle,    "jungle book, the (europe).bin",                                                          0x000000 , 0x40000,	 CRC(695a9a15) SHA1(f65b5957b4b21bd16b4aa8a6e93fed944dd7d9ac) )
SMS_ROM_LOAD( jurassic,  "jurassic park (europe).bin",                                                             0x000000 , 0x80000,	 CRC(0667ed9f) SHA1(67a6e6c132362f3d9263dda68d77c279b08f1fde) )
SMS_ROM_LOAD( kenseidj,  "kenseiden (japan).bin",                                                                  0x000000 , 0x40000,	 CRC(05ea5353) SHA1(cd349833ff41821635c6242a0b8cef7e071103d5) )
SMS_ROM_LOAD( kenseid,   "kenseiden (usa, europe).bin",                                                            0x000000 , 0x40000,	 CRC(516ed32e) SHA1(3fce661d57e8bc764e7190ddbee4bf3d3e214c6c) )
SMS_ROM_LOAD( kingsqb,   "king's quest - quest for the crown (usa) (beta).bin",                                    0x000000 , 0x20000,	 CRC(b7fe0a9d) SHA1(1b6330199444e303aafb9a2fd3f2119cedab0712) )
SMS_ROM_LOAD( kingsq,    "king's quest - quest for the crown (usa).bin",                                           0x000000 , 0x20000,	 CRC(f8d33bc4) SHA1(d05ae9652b85c858f4e7db0b7b7c457a4e0a6a49) )
SMS_ROM_LOAD( klax,      "klax (europe).bin",                                                                      0x000000 , 0x20000,	 CRC(2b435fd6) SHA1(53ae621e66d8e5f2e7276e461e8771c3c2037a7a) )
SMS_ROM_LOAD( krusty,    "krusty's fun house (europe).bin",                                                        0x000000 , 0x40000,	 CRC(64a585eb) SHA1(54714a19c2d24260b117ebc5ae391d9b24ca9166) )
SMS_ROM_LOAD( kujaku,    "kujaku ou (japan).bin",                                                                  0x000000 , 0x80000,	 CRC(d11d32e4) SHA1(6974d27bc31c2634bec54c4e9935a28461fb60f7) )
SMS_ROM_LOAD( kungfuk,   "kung fu kid (usa, europe).bin",                                                          0x000000 , 0x20000,	 CRC(1e949d1f) SHA1(7e1c32f5abf9ff906ffe113ffab6eecd1c86b381) )
SMS_ROM_LOAD( landill,   "land of illusion starring mickey mouse (europe).bin",                                    0x000000 , 0x80000,	 CRC(24e97200) SHA1(a120f29c6bf2e733775d5b984bd3a156682699c6) )
SMS_ROM_LOAD( laserg,    "laser ghost (europe).bin",                                                               0x000000 , 0x40000,	 CRC(0ca95637) SHA1(a21286d282ca994c66d8e7a91ee0a05ff69c7981) )
SMS_ROM_LOAD( legndill,  "legend of illusion starring mickey mouse (brazil).bin",                                  0x000000 , 0x80000,	 CRC(6350e649) SHA1(62adbd8e5a41d08c4745e9fbb1c51f0091c9dea6) )
SMS_ROM_LOAD( lemmingsb, "lemmings (europe) (beta).bin",                                                           0x000000 , 0x40000,	 CRC(2c61ed88) SHA1(8d8692d363348f7c93f402c6485f5f831d1c8190) )
SMS_ROM_LOAD( lemmings,  "lemmings (europe).bin",                                                                  0x000000 , 0x40000,	 CRC(f369b2d8) SHA1(f3a853cce1249a0848bfc0344f3ee2db6efa4c01) )
SMS_ROM_LOAD( lineof,    "line of fire (europe).bin",                                                              0x000000 , 0x80000,	 CRC(cb09f355) SHA1(985f78bcaf64bb088d64517f80b0acc7f5034b24) )
SMS_ROM_LOAD( lionking,  "lion king, the (europe).bin",                                                            0x000000 , 0x80000,	 CRC(c352c7eb) SHA1(3a64657e3523a5da1b99db9d89b1a48ed4ccc5ed) )
SMS_ROM_LOAD( lordswrdj, "lord of sword (japan).bin",                                                              0x000000 , 0x40000,	 CRC(aa7d6f45) SHA1(6a08d913fd92a213b1ecf5aa7c5630362cccc6b4) )
SMS_ROM_LOAD( lordswrd,  "lord of the sword (usa, europe).bin",                                                    0x000000 , 0x40000,	 CRC(e8511b08) SHA1(a5326a0029f7c3101add3335a599a01ccd7634c5) )
SMS_ROM_LOAD( loretta,   "loretta no shouzou (japan).bin",                                                         0x000000 , 0x20000,	 CRC(323f357f) SHA1(0ddd7448f5bd437d0d33b85a44f2bcc2bf2ea05e) )
SMS_ROM_LOAD( luckydimb, "lucky dime caper starring donald duck, the (europe) (beta).bin",                         0x000000 , 0x40000,	 CRC(7f6d0df6) SHA1(5ac3e68b34ee4499ddbdee28b47a1440782a9c04) )
SMS_ROM_LOAD( luckydim,  "lucky dime caper starring donald duck, the (europe).bin",                                0x000000 , 0x40000,	 CRC(a1710f13) SHA1(a08815d27e431f0bee70b4ebfb870a3196c2a732) )
SMS_ROM_LOAD( mahjongb,  "mahjong sengoku jidai (japan) (beta).bin",                                               0x000000 , 0x20000,	 CRC(996b2a07) SHA1(169510c0575e4f53b9da8197fc48608993351182) )
SMS_ROM_LOAD( mahjong,   "mahjong sengoku jidai (japan).bin",                                                      0x000000 , 0x20000,	 CRC(bcfbfc67) SHA1(9b7cd3a25b2a1fc880683dcdca81457c93d46de5) )
SMS_ROM_LOAD( makairet,  "makai retsuden (japan).bin",                                                             0x000000 , 0x20000,	 CRC(ca860451) SHA1(59d520fdb2b6cbd5736b2bd6045ad3ee3ad2e3a6) )
SMS_ROM_LOAD( maougolv,  "maou golvellius (japan).bin",                                                            0x000000 , 0x40000,	 CRC(bf0411ad) SHA1(f62c5c9dea4368e6475517c4220a62e40fedd35d) )
SMS_ROM_LOAD( maougolvp, "maou golvellius [proto] (jp).bin",                                                       0x000000,  0x40000,	 CRC(21a21352) SHA1(9773ade9accd47de4541cb4fdb624e1f7feb2ab9) )
SMS_ROM_LOAD( marble,    "marble madness (europe).bin",                                                            0x000000 , 0x40000,	 CRC(bf6f3e5f) SHA1(f5efe0635e283a08f98272a9ff1bc7d37c35692c) )
SMS_ROM_LOAD( marksman,  "marksman shooting & trap shooting & safari hunt (europe).bin",                           0x000000 , 0x20000,	 CRC(e8215c2e) SHA1(eaae5c9d9de24c0991500122042b3aa2210d50d9) )
SMS_ROM_LOAD( marksmanu, "marksman shooting & trap shooting (usa).bin",                                            0x000000 , 0x20000,	 CRC(e8ea842c) SHA1(5491cce7b9c19cb49060da94ab8f9c4331e77cb3) )
SMS_ROM_LOAD( mastdark,  "master of darkness (europe).bin",                                                        0x000000 , 0x40000,	 CRC(96fb4d4b) SHA1(ed3569be5d5a49ff5a09b2b04ec0101d4edfa81e) )
SMS_ROM_LOAD( mastcomb,  "masters of combat (europe).bin",                                                         0x000000 , 0x40000,	 CRC(93141463) SHA1(9596400394f2bb3dbf7eb4a26b820cdfe5cd6094) )
SMS_ROM_LOAD( mazehunt,  "maze hunter 3-d (usa, europe).bin",                                                      0x000000 , 0x20000,	 CRC(31b8040b) SHA1(6d94c2159a67f3140d0c9158b58aa8f0474eaaba) )
SMS_ROM_LOAD( mazewalk,  "maze walker (japan).bin",                                                                0x000000 , 0x20000,	 CRC(871562b0) SHA1(ade756eccaf94c79e8b3636921b6f8669a163265) )
SMS_ROM_LOAD( megumi,    "megumi rescue (japan).bin",                                                              0x000000 , 0x20000,	 CRC(29bc7fad) SHA1(7bd156cf8dc2ad07c666ac58ccb3c0ff6671b93f) )
SMS_ROM_LOAD( mercs,     "mercs (europe).bin",                                                                     0x000000 , 0x80000,	 CRC(d7416b83) SHA1(f2cfad96a116bde9a91240eb1ad520dc448fa20f) )
SMS_ROM_LOAD( mwalkb,    "michael jackson's moonwalker (usa, europe) (beta).bin",                                  0x000000 , 0x40000,	 CRC(54123936) SHA1(67557e7551ad785802b1f2abc4b456e3268f564c) )
SMS_ROM_LOAD( mwalk,     "michael jackson's moonwalker (usa, europe).bin",                                         0x000000 , 0x40000,	 CRC(56cc906b) SHA1(939416cebb381458d28ff628afb3d1f80293afa9) )
SMS_ROM_LOAD( mickmack,  "mick & mack as the global gladiators (europe).bin",                                      0x000000 , 0x40000,	 CRC(b67ceb76) SHA1(e2e2f45e43f0d4fa5974327e96d7c3ee7f057fad) )
SMS_ROM_LOAD( mickeyuc,  "mickey's ultimate challenge (brazil).bin",                                               0x000000 , 0x40000,	 CRC(25051dd5) SHA1(7253b4df642c56f266e5542a60065ca2c00ad0df) )
SMS_ROM_LOAD( microm,    "micro machines (europe).bin",                                                            0x000000 , 0x40000,	 CRC(a577ce46) SHA1(425621f350d011fd021850238c6de9999625fd69) )
SMS_ROM_LOAD( miracleb,  "miracle warriors - seal of the dark lord (usa, europe) (beta).bin",                      0x000000 , 0x40000,	 CRC(301a59aa) SHA1(5f1cfc974a8ffc965f18216a89472fd1a980d7cb) )
SMS_ROM_LOAD( miracle,   "miracle warriors - seal of the dark lord (usa, europe).bin",                             0x000000 , 0x40000,	 CRC(0e333b6e) SHA1(f952406bca4918ee91a89b27e949e224eae96d85) )
SMS_ROM_LOAD( missil3d,  "missile defense 3-d (usa, europe).bin",                                                  0x000000 , 0x20000,	 CRC(fbe5cfbb) SHA1(86242fcc8b9f93cdad2241f40c3eebbf4c9ff213) )
SMS_ROM_LOAD( monica,    "monica no castelo do dragao (brazil).bin",                                               0x000000 , 0x40000,	 CRC(01d67c0b) SHA1(e05953a3772452f821c3815231a25940a1d93803) )
SMS_ROM_LOAD( monopolyu, "monopoly (usa).bin",                                                                     0x000000 , 0x20000,	 CRC(69538469) SHA1(8dd7bb4f666f70f7f57687823d0068c9100af8e5) )
SMS_ROM_LOAD( monopoly,  "monopoly (usa, europe).bin",                                                             0x000000 , 0x20000,	 CRC(026d94a4) SHA1(6bad7176011dd4bbd007498d167399daacde173d) )
SMS_ROM_LOAD( montezum,  "montezuma's revenge featuring panama joe (usa).bin",                                     0x000000 , 0x20000,	 CRC(82fda895) SHA1(a2665093b8588d5d6f24c6b329080fb3ebee896e) )
SMS_ROM_LOAD( montezump, "montezuma's revenge - featuring panama joe [proto].bin",                                 0x000000,  0x20000,	 CRC(575d0fcf) SHA1(4f93e297425cb19ab6de2107795d210544171396) )
SMS_ROM_LOAD( mk,        "mortal kombat (europe).bin",                                                             0x000000 , 0x80000,	 CRC(302dc686) SHA1(3f67e0e702f391839b51a43125f971ae1c5fad92) )
SMS_ROM_LOAD( mk3b,      "mortal kombat 3 (brazil).bin",                                                           0x000000 , 0x80000,	 CRC(395ae757) SHA1(f1f43f57982dd22caa8869a85b1a05fa61c349dd) )
SMS_ROM_LOAD( mk2,       "mortal kombat ii (europe).bin",                                                          0x000000 , 0x80000,	 CRC(2663bf18) SHA1(12bd887efb87f410d3b65bf9e6dfe2745b345539) )
SMS_ROM_LOAD( mspacman,  "ms. pac-man (europe).bin",                                                               0x000000 , 0x20000,	 CRC(3cd816c6) SHA1(4dd7cb303793792edd9a4c717a727f46db857dae) )
SMS_ROM_LOAD( myhero,    "my hero (usa, europe).bin",                                                              0x000000 , 0x8000,	 CRC(62f0c23d) SHA1(7583c5fb1963c070b7bda72b447cc3fd611ddf1a) )
SMS_ROM_LOAD( nekkyu,    "nekkyuu koushien (japan).bin",                                                           0x000000 , 0x40000,	 CRC(5b5f9106) SHA1(35e882723189178c1dc811a04125daec4487e693) )
SMS_ROM_LOAD( tnzs,      "new zealand story, the (europe).bin",                                                    0x000000 , 0x40000,	 CRC(c660ff34) SHA1(e433b21b505ed5428d1b2f07255e49c6db2edc6c) )
SMS_ROM_LOAD( ninjaj,    "ninja (japan).bin",                                                                      0x000000 , 0x20000,	 CRC(320313ec) SHA1(e9acdae112a898f7db090fc0b8f1ce9b86637234) )
SMS_ROM_LOAD( ninjagb,   "ninja gaiden (europe) (beta).bin",                                                       0x000000 , 0x40000,	 CRC(761e9396) SHA1(c67db6539dc609b08d3ca6f9e6f8f41daf150743) )
SMS_ROM_LOAD( ninjag,    "ninja gaiden (europe).bin",                                                              0x000000 , 0x40000,	 CRC(1b1d8cc2) SHA1(4c65db563e8407444020ab7fd93fc45193ae923a) )
SMS_ROM_LOAD( ninja,     "ninja, the (usa, europe).bin",                                                           0x000000 , 0x20000,	 CRC(66a15bd9) SHA1(76396a25902700e18adf6bc5c8668e2a8bdf03a9) )
SMS_ROM_LOAD( olympi,    "olympic gold (europe) (en,fr,de,es,it,nl,pt,sv).bin",                                    0x000000 , 0x40000,	 CRC(6a5a1e39) SHA1(21d7e4d99b1d35ad7c169cc16162a0873661fcd4) )
SMS_ROM_LOAD( olympik,   "olympic gold (korea) (en,fr,de,es,it,nl,pt,sv).bin",                                     0x000000 , 0x40000,	 CRC(d1a7b1aa) SHA1(d3b0aa53159403c59e53e486597f882730556d96) )
SMS_ROM_LOAD( opaopa,    "opa opa (japan).bin",                                                                    0x000000 , 0x20000,	 CRC(bea27d5c) SHA1(38bd50181b98216c9ccf1d7dd6bc2c0a21e9a283) )
SMS_ROM_LOAD( opwolf,    "operation wolf (europe).bin",                                                            0x000000 , 0x40000,	 CRC(205caae8) SHA1(064040452b6bacc75443dae7916a0fd573f1600d) )
SMS_ROM_LOAD( ottifant,  "ottifants, the (europe) (en,fr,de,es,it).bin",                                           0x000000 , 0x40000,	 CRC(82ef2a7d) SHA1(465e7a8cdfad8fc96587f6516770eb81a171f036) )
SMS_ROM_LOAD( outrun,    "out run (world).bin",                                                                    0x000000 , 0x40000,	 CRC(5589d8d2) SHA1(4f9b61b24f0d9fee0448cdbbe8fc05411dbb1102) )
SMS_ROM_LOAD( outrun3d,  "out run 3-d (europe).bin",                                                               0x000000 , 0x40000,	 CRC(d6f43dda) SHA1(93c3fbe23848556fc6a737db1b5182537db5961d) )
SMS_ROM_LOAD( outruneu,  "out run europa (europe).bin",                                                            0x000000 , 0x40000,	 CRC(3932adbc) SHA1(c8fbf18eabdcf90cd70fc77444cf309ff47f5827) )
SMS_ROM_LOAD( pacmania,  "pac-mania (europe).bin",                                                                 0x000000 , 0x20000,	 CRC(be57a9a5) SHA1(c0a11248bbb556b643accd3c76737be35cbada54) )
SMS_ROM_LOAD( paperboy,  "paperboy (europe).bin",                                                                  0x000000 , 0x20000,	 CRC(294e0759) SHA1(e91dae35a3ca3a475e30b7863c03375d656ec734) )
SMS_ROM_LOAD( paperboyu, "paperboy (usa).bin",                                                                     0x000000 , 0x20000,	 CRC(327a0b4c) SHA1(736718efab4737ebf9d06221ac35fa2fcc4593ce) )
SMS_ROM_LOAD( parlour,   "parlour games (usa, europe).bin",                                                        0x000000 , 0x20000,	 CRC(e030e66c) SHA1(06664daf208f07cb00b603b12eccfc3f01213a17) )
SMS_ROM_LOAD( patriley,  "pat riley basketball (usa) (proto).bin",                                                 0x000000 , 0x40000,	 CRC(9aefe664) SHA1(8b49c7772bd398665217bf81648353ff46485cac) )
SMS_ROM_LOAD( penguin,   "penguin land (usa, europe).bin",                                                         0x000000 , 0x20000,	 CRC(f97e9875) SHA1(8762239c339a084dfb8443cc38515301476bde28) )
SMS_ROM_LOAD( pgatour,   "pga tour golf (europe).bin",                                                             0x000000 , 0x40000,	 CRC(95b9ea95) SHA1(bdbb1337453234289fbd193d7d7fcf1ad3c3807c) )
SMS_ROM_LOAD( pstarb,    "phantasy star (brazil).bin",                                                             0x000000 , 0x80000,	 CRC(75971bef) SHA1(fd8dad6acb6fa75dc8e9bbaea2a7e9fd486fc2dd) )
SMS_ROM_LOAD( pstarj1,   "phantasy star (j) (from saturn collection cd) [!].bin",                                  0x000000 , 0x80000,	 CRC(07301f83) SHA1(b3ae447dc739256616b44cbd77cb903c9f19e980) )
SMS_ROM_LOAD( pstarj,    "phantasy star (japan).bin",                                                              0x000000 , 0x80000,	 CRC(6605d36a) SHA1(c9a40ddd217c58dddcd6b5c0fe66c3a50d3e68e4) )
SMS_ROM_LOAD( pstark,    "phantasy star (korea).bin",                                                              0x000000 , 0x80000,	 CRC(747e83b5) SHA1(52b2aa52a1c96e15869498a8e42b074705070007) )
SMS_ROM_LOAD( pstar1,    "phantasy star (usa, europe) (v1.2).bin",                                                 0x000000 , 0x80000,	 CRC(e4a65e79) SHA1(257ca76ebcd54c75a414ca7ce968fa59ea42f150) )
SMS_ROM_LOAD( pstar,     "phantasy star (usa, europe) (v1.3).bin",                                                 0x000000 , 0x80000,	 CRC(00bef1d7) SHA1(07fcf297be4f4c9d92cd3f119a7ac48467e06838) )
SMS_ROM_LOAD( pitfightb, "pit fighter (brazil).bin",                                                               0x000000 , 0x80000,	 CRC(aa4d4b5a) SHA1(409ccca9dcb78a20f7dd917699ce2c70f87f857f) )
SMS_ROM_LOAD( pitfight,  "pit fighter (europe).bin",                                                               0x000000 , 0x80000,	 CRC(b840a446) SHA1(e2856b4ae331aea100984ac778b5e726f5da8709) )
SMS_ROM_LOAD( populous,  "populous (europe).bin",                                                                  0x000000 , 0x40000,	 CRC(c7a1fdef) SHA1(d6142584b9796a96941b6a95bda14cf137b47085) )
SMS_ROM_LOAD( poseidon,  "poseidon wars 3-d (usa, europe).bin",                                                    0x000000 , 0x40000,	 CRC(abd48ad2) SHA1(c177effd5fd18a082393a2b4167c49bcc5db1f64) )
SMS_ROM_LOAD( powstrik,  "power strike (usa, europe).bin",                                                         0x000000 , 0x20000,	 CRC(4077efd9) SHA1(f6f245c41163b15bce95368e4684b045790a1148) )
SMS_ROM_LOAD( powstrk2,  "power strike ii (europe).bin",                                                           0x000000 , 0x80000,	 CRC(a109a6fe) SHA1(3bcffd47294f25b25cccb7f42c3a9c3f74333d73) )
SMS_ROM_LOAD( predator,  "predator 2 (europe).bin",                                                                0x000000 , 0x40000,	 CRC(0047b615) SHA1(ee26c9f5fd08ac73b5e28b20f35889d24c88c6db) )
SMS_ROM_LOAD( ppersia,   "prince of persia (europe).bin",                                                          0x000000 , 0x40000,	 CRC(7704287d) SHA1(7ef7b4e2fcec69946844c186c62836c0ae34665f) )
SMS_ROM_LOAD( prowrest,  "pro wrestling (usa, europe).bin",                                                        0x000000 , 0x20000,	 CRC(fbde42d3) SHA1(b0e4832af04b4fb832092ad093d982ce11160eef) )
SMS_ROM_LOAD( proyakyu,  "pro yakyuu pennant race, the (japan).bin",                                               0x000000 , 0x20000,	 CRC(da9be8f0) SHA1(1ee602f8711d82d13b006984cef95512a93c7783) )
SMS_ROM_LOAD( sms3samp,  "promocao especial m. system iii compact (brazil) (sample).bin",                          0x000000 , 0x8000,	 CRC(30af0233) SHA1(53b50a1c574479359f85327a8000d3f03f0963d5) )
SMS_ROM_LOAD( psychic,   "psychic world (europe).bin",                                                             0x000000 , 0x40000,	 CRC(5c0b1f0f) SHA1(5fa54329692e680a190291d0744580968aa8b3fe) )
SMS_ROM_LOAD( psychof,   "psycho fox (usa, europe).bin",                                                           0x000000 , 0x40000,	 CRC(97993479) SHA1(278cc3853905626138e83b6cfa39c26ba8e4f632) )
SMS_ROM_LOAD( puttputtb, "putt & putter (europe) (beta).bin",                                                      0x000000 , 0x20000,	 CRC(8167ccc4) SHA1(c6d3f256f938827899807884253817f432379d7c) )
SMS_ROM_LOAD( puttputt,  "putt & putter (europe).bin",                                                             0x000000 , 0x20000,	 CRC(357d4f78) SHA1(760d330047a77b98e2fff786052741dc7e3760e8) )
SMS_ROM_LOAD( quartet,   "quartet (usa, europe).bin",                                                              0x000000 , 0x20000,	 CRC(e0f34fa6) SHA1(08a3484e862a284f6038b7cd0dfc745a8b7c6c51) )
SMS_ROM_LOAD( questfsy,  "quest for the shaven yak starring ren hoek & stimpy (brazil).bin",                       0x000000 , 0x80000,	 CRC(f42e145c) SHA1(418ea57fdbd06aca4285447db2ecb2b0392a178d) )
SMS_ROM_LOAD( rtype,     "r-type (world).bin",                                                                     0x000000 , 0x80000,	 CRC(bb54b6b0) SHA1(08ec70a2cd98fcb2645f59857f845d41b0045115) )
SMS_ROM_LOAD( rcgp,      "r.c. grand prix (usa, europe).bin",                                                      0x000000 , 0x40000,	 CRC(54316fea) SHA1(94cc64f6febdb0fe4e89ecd9deeca96694e5bead) )
SMS_ROM_LOAD( rainbowb,  "rainbow islands - the story of bubble bobble 2 (brazil).bin",                            0x000000 , 0x40000,	 CRC(00ec173a) SHA1(a3958277129916a4fd32f3d5f2345b8d0fc23faf) )
SMS_ROM_LOAD( rainbow,   "rainbow islands - the story of bubble bobble 2 (europe).bin",                            0x000000 , 0x40000,	 CRC(c172a22c) SHA1(acff4b6175aaaed9075f6e41cf387cbfd03eb330) )
SMS_ROM_LOAD( rambo2,    "rambo - first blood part ii (usa).bin",                                                  0x000000 , 0x20000,	 CRC(bbda65f0) SHA1(dc44b090a01d6a4d9b5700ada764b00c62c00e91) )
SMS_ROM_LOAD( rambo3,    "rambo iii (usa, europe).bin",                                                            0x000000 , 0x40000,	 CRC(da5a7013) SHA1(6d9746c0e3c50e87fa773a64e2f0bb76f722d76c) )
SMS_ROM_LOAD( rampage,   "rampage (usa, europe).bin",                                                              0x000000 , 0x40000,	 CRC(42fc47ee) SHA1(cbf5b6f06bec42db8a99c7b9c00118521aded858) )
SMS_ROM_LOAD( rampart,   "rampart (europe).bin",                                                                   0x000000 , 0x40000,	 CRC(426e5c8a) SHA1(bfea7112a7e2eb2c5e7c20147197ffe3b06d5711) )
SMS_ROM_LOAD( rastan,    "rastan (usa, europe).bin",                                                               0x000000 , 0x40000,	 CRC(c547eb1b) SHA1(90f9ccf516db2a1cf20e199cfd5d31d4cfce0f1f) )
SMS_ROM_LOAD( regjacks,  "reggie jackson baseball (usa).bin",                                                      0x000000 , 0x40000,	 CRC(6d94bb0e) SHA1(b7c387256e58a95bd3c3c4358a8a106f2304e9f2) )
SMS_ROM_LOAD( renegade,  "renegade (europe).bin",                                                                  0x000000 , 0x40000,	 CRC(3be7f641) SHA1(c07a04f3ab811b52c97b9bf850670057148de6f0) )
SMS_ROM_LOAD( rescuems,  "rescue mission (usa, europe).bin",                                                       0x000000 , 0x20000,	 CRC(79ac8e7f) SHA1(6a561bff2f8022261708b91722caf1ec0e63f9c4) )
SMS_ROM_LOAD( roadrash,  "road rash (europe).bin",                                                                 0x000000 , 0x80000,	 CRC(b876fc74) SHA1(7976e717125757b1900a540a68e0ef3083839f85) )
SMS_ROM_LOAD( robocop,   "robocop 3 (europe).bin",                                                                 0x000000 , 0x40000,	 CRC(9f951756) SHA1(459a01f6e240e6f81726f174fadcfe06badce841) )
SMS_ROM_LOAD( roboterm,  "robocop versus the terminator (europe).bin",                                             0x000000 , 0x80000,	 CRC(8212b754) SHA1(a5fe99ce04cb172714a431b12ce14e39fcc573e4) )
SMS_ROM_LOAD( rocky,     "rocky (world).bin",                                                                      0x000000 , 0x40000,	 CRC(1bcc7be3) SHA1(8601927ca17419f5d61f757e9371ce533228f7bb) )
SMS_ROM_LOAD( running,   "running battle (europe).bin",                                                            0x000000 , 0x40000,	 CRC(1fdae719) SHA1(8f680b3a9782304a2f879f6590dc78ea4e366163) )
SMS_ROM_LOAD( sagaia,    "sagaia (europe).bin",                                                                    0x000000 , 0x40000,	 CRC(66388128) SHA1(2a3e859139f8ca83494bb800dc848fe4d02db82a) )
SMS_ROM_LOAD( sangok3,   "sangokushi 3 (korea) (unl).bin",                                                         0x000000 , 0x100000,	 CRC(97d03541) SHA1(c0256188b15271bb814bb8356b3311340e53ea3e) )
SMS_ROM_LOAD( sapomestr, "sapo xule - o mestre do kung fu (brazil).bin",                                           0x000000 , 0x40000,	 CRC(890e83e4) SHA1(f14a7448f38766640c0bc9ea3410eb55194da58f) )
SMS_ROM_LOAD( saposos,   "sapo xule - s.o.s lagoa poluida (brazil).bin",                                           0x000000 , 0x20000,	 CRC(7ab2946a) SHA1(2583b9027fd2a4d84ed324ed03fc82b9eedd1ff4) )
SMS_ROM_LOAD( sapoxu,    "sapo xule vs. os invasores do brejo (brazil).bin",                                       0x000000 , 0x40000,	 CRC(9a608327) SHA1(fb4205bf1c1df55455a7ab4d4a6a9c5fe7d12a0b) )
SMS_ROM_LOAD( satell7a,  "satellite 7 (j) [p1][!].bin",                                                            0x000000 , 0x8000,	 CRC(87b9ecb8) SHA1(a48b642ac12379944fc829585dacb8e7effdcc82) )
SMS_ROM_LOAD( satell7,   "satellite 7 (japan).bin",                                                                0x000000 , 0x8000,	 CRC(16249e19) SHA1(88fc5596773ea31eda8ae5a8baf6f0ce5c3f7e5e) )
SMS_ROM_LOAD( schtroumb, "schtroumpfs autour du monde, les (europe) (en,fr,de,es) (beta).bin",                     0x000000 , 0x40000,	 CRC(7982ae67) SHA1(a82627f47400555e5e6fdeca11ffac56d7296f80) )
SMS_ROM_LOAD( schtroum,  "schtroumpfs autour du monde, les (europe) (en,fr,de,es).bin",                            0x000000 , 0x40000,	 CRC(97e5bb7d) SHA1(16f756e00314781e07af84c871e82ec8e0a68a57) )
SMS_ROM_LOAD( scrambsp,  "scramble spirits (europe).bin",                                                          0x000000 , 0x40000,	 CRC(9a8b28ec) SHA1(430154dc7ac1bb580c7663019ca476e5333e0508) )
SMS_ROM_LOAD( sdi,       "sdi (japan).bin",                                                                        0x000000 , 0x20000,	 CRC(1de2c2d0) SHA1(2000b3b291dd7b76c3b8801a88fb0e293ca7e278) )
SMS_ROM_LOAD( secret,    "secret command (europe).bin",                                                            0x000000 , 0x20000,	 CRC(00529114) SHA1(e7a952f8bd6dddbb365870e906801021b240a533) )
SMS_ROM_LOAD( segachss,  "sega chess (europe).bin",                                                                0x000000 , 0x40000,	 CRC(a8061aef) SHA1(2c386825dce99b340084b28bdf90fb4ee7107317) )
SMS_ROM_LOAD( segawtg,   "sega world tournament golf (europe).bin",                                                0x000000 , 0x40000,	 CRC(296879dd) SHA1(fd5a92e74a3c5fa16727637f1839b28595449bf6) )
SMS_ROM_LOAD( seishun,   "seishun scandal (japan).bin",                                                            0x000000 , 0x8000,	 CRC(f0ba2bc6) SHA1(6942f38e608cc7d70cf9cc8c13ee8c22e4b81679) )
SMS_ROM_LOAD( seishun1,  "seishyun scandal (j) [p1][!].bin",                                                       0x000000 , 0x8000,	 CRC(bcd91d78) SHA1(3da61c3f36aaa885c3efa58807fd122f24643ded) )
SMS_ROM_LOAD( sensible,  "sensible soccer (europe).bin",                                                           0x000000 , 0x20000,	 CRC(f8176918) SHA1(08b4f5b8096bc811bc9a7750deb21def67711a9f) )
SMS_ROM_LOAD( shadow,    "shadow dancer (europe).bin",                                                             0x000000 , 0x80000,	 CRC(3793c01a) SHA1(99a5995f31dcf6fbbef56d3ea0d2094ef039479f) )
SMS_ROM_LOAD( beast,     "shadow of the beast (europe).bin",                                                       0x000000 , 0x40000,	 CRC(1575581d) SHA1(45943b021cbaee80a149b80ddb6f3fb5eb8b9e43) )
SMS_ROM_LOAD( shanghb,   "shanghai (usa, europe) (beta).bin",                                                      0x000000 , 0x20000,	 CRC(d5d25156) SHA1(98615e4994fcc1e973c7483303b580dfeb756eec) )
SMS_ROM_LOAD( shangh,    "shanghai (usa, europe).bin",                                                             0x000000 , 0x20000,	 CRC(aab67ec3) SHA1(58f01556d1f2da0af9dfcddcb3ac26cb299220d3) )
SMS_ROM_LOAD( shinobij,  "shinobi (japan).bin",                                                                    0x000000 , 0x40000,	 CRC(e1fff1bb) SHA1(26070deaa2d3c170d31ac395a50231204250bdf3) )
SMS_ROM_LOAD( shinobi,   "shinobi (usa, europe).bin",                                                              0x000000 , 0x40000,	 CRC(0c6fac4e) SHA1(7c0778c055dc9c2b0aae1d166dbdb4734e55b9d1) )
SMS_ROM_LOAD( shooting,  "shooting gallery (usa, europe).bin",                                                     0x000000 , 0x20000,	 CRC(4b051022) SHA1(6c22e3fa928c2aed468e925af65ea7f7c6292905) )
SMS_ROM_LOAD( bartvssm,  "simpsons, the - bart vs. the space mutants (europe).bin",                                0x000000 , 0x40000,	 CRC(d1cc08ee) SHA1(4fa839db6de21fd589f6a91791fff25ca2ab88f4) )
SMS_ROM_LOAD( bartvswd,  "simpsons, the - bart vs. the world (europe).bin",                                        0x000000 , 0x40000,	 CRC(f6b2370a) SHA1(038920e63437e05d8431e4cbab2131dd76fb3345) )
SMS_ROM_LOAD( sitio,     "sitio do picapau amarelo (brazil).bin",                                                  0x000000 , 0x100000,	 CRC(abdf3923) SHA1(9d626e9faa29a63ed396959894d4a481f1e7a01d) )
SMS_ROM_LOAD( slapshot,  "slap shot (europe).bin",                                                                 0x000000 , 0x40000,	 CRC(d33b296a) SHA1(5979ee84570f7c930f20be473e895fc1d2b9e3f4) )
SMS_ROM_LOAD( slapshotu, "slap shot (usa) (v1.1).bin",                                                             0x000000 , 0x40000,	 CRC(702c3e98) SHA1(2259dcf3ce702b5ddc8d0a5ed335263f45ecd2ce) )
SMS_ROM_LOAD( slapshotu1, "slap shot (usa).bin",                                                                   0x000000 , 0x40000,	 CRC(c93bd0e9) SHA1(6327fb7129ecfcebdcd6b9c941703fda15a8a195) )
SMS_ROM_LOAD( smurfs,    "smurfs, the (europe) (en,fr,de,es).bin",                                                 0x000000 , 0x40000,	 CRC(3e63768a) SHA1(82f75232e195b8bdcd9c0d852076d999899cc92e) )
SMS_ROM_LOAD( solomon,   "solomon no kagi - oujo rihita no namida (japan).bin",                                    0x000000 , 0x20000,	 CRC(11645549) SHA1(875f35d0775609776ec75ea2a8fa2297643e906c) )
SMS_ROM_LOAD( sonicbl,   "sonic blast (brazil).bin",                                                               0x000000 , 0x100000,	 CRC(96b3f29e) SHA1(4ad77a472e98002dc0d5c1463965720a257e1b8f) )
SMS_ROM_LOAD( sonicc,    "sonic chaos (europe).bin",                                                               0x000000 , 0x80000,	 CRC(aedf3bdf) SHA1(f64c8eea26a103582f09831c3e02c6045a6aff94) )
SMS_ROM_LOAD( sonicsp,   "sonic spinball (europe).bin",                                                            0x000000 , 0x80000,	 CRC(11c1bc8a) SHA1(a6aa8038feb54e84759fcdfced2270afbbef9bfc) )
SMS_ROM_LOAD( sonic,     "sonic the hedgehog (usa, europe).bin",                                                   0x000000 , 0x40000,	 CRC(b519e833) SHA1(6b9677e4a9abb37765d6db4658f4324251807e07) )
SMS_ROM_LOAD( sonic2,    "sonic the hedgehog 2 (europe) (v1.1).bin",                                               0x000000 , 0x80000,	 CRC(d6f2bfca) SHA1(689339bac11c3565dd774f8cd4d8ea1b27831118) )
SMS_ROM_LOAD( sonic2a,   "sonic the hedgehog 2 (europe).bin",                                                      0x000000 , 0x80000,	 CRC(5b3b922c) SHA1(acdb0b5e8bf9c1e9c9d1a8ac6d282cb8017d091c) )
SMS_ROM_LOAD( sonicedu,  "sonic's edusoft (unknown) (proto).bin",                                                  0x000000 , 0x40000,	 CRC(d9096263) SHA1(f1357505cbcebfab52327a5fc9c3437f54d8da40) )
SMS_ROM_LOAD( spacegun,  "space gun (europe).bin",                                                                 0x000000 , 0x80000,	 CRC(a908cff5) SHA1(c5afd5fa7b26da55d243df6822c16dae3a401ac1) )
SMS_ROM_LOAD( sharrj,    "space harrier (japan).bin",                                                              0x000000 , 0x40000,	 CRC(beddf80e) SHA1(51ba2185a2b93957c1c51b0a2e2b80394463bed8) )
SMS_ROM_LOAD( sharr,     "space harrier (usa, europe).bin",                                                        0x000000 , 0x40000,	 CRC(ca1d3752) SHA1(9e92d8e27fad71635c71612e8bdd632d760f9a2d) )
SMS_ROM_LOAD( sharr3d,   "space harrier 3-d (usa, europe).bin",                                                    0x000000 , 0x40000,	 CRC(6bd5c2bf) SHA1(ce3507f62563f7d4cb4b2fc6497317685626af92) )
SMS_ROM_LOAD( sharr3dj,  "space harrier 3d (japan).bin",                                                           0x000000 , 0x40000,	 CRC(156948f9) SHA1(7caf44ecc3de6daffedf7a494d449202888a6156) )
SMS_ROM_LOAD( scib,      "special criminal investigation (europe) (beta).bin",                                     0x000000 , 0x40000,	 CRC(1b7d2a20) SHA1(a0b4be5b62a0a836e227983647e0140df3eafe4d) )
SMS_ROM_LOAD( sci,       "special criminal investigation (europe).bin",                                            0x000000 , 0x40000,	 CRC(fa8e4ca0) SHA1(8daabf51099bd2702fe4418fd202eff532bc710a) )
SMS_ROM_LOAD( speedbl,   "speedball (europe) (mirrorsoft).bin",                                                    0x000000 , 0x20000,	 CRC(a57cad18) SHA1(e51cbdb9b74ce4b53747215b23b54eb62f8392b3) )
SMS_ROM_LOAD( speedblv,  "speedball (europe) (virgin).bin",                                                        0x000000 , 0x20000,	 CRC(5ccc1a65) SHA1(a4cf72c985d0fe51ac36388774c7f0bf982c19e3) )
SMS_ROM_LOAD( speedbl2,  "speedball 2 (europe).bin",                                                               0x000000 , 0x40000,	 CRC(0c7366a0) SHA1(4e0b456441f0acef737e463e6ee5bbcb377ea308) )
SMS_ROM_LOAD( spellcst,  "spellcaster (usa, europe).bin",                                                          0x000000 , 0x80000,	 CRC(4752cae7) SHA1(eed8b01fca86dbd8291d5ca2d1e6f6ca1b60fe68) )
SMS_ROM_LOAD( spidermn,  "spider-man - return of the sinister six (europe).bin",                                   0x000000 , 0x40000,	 CRC(ebe45388) SHA1(5d4fbf3873af14afcda10fadfdb3f4f8919b3b1e) )
SMS_ROM_LOAD( spidking,  "spider-man vs. the kingpin (usa, europe).bin",                                           0x000000 , 0x40000,	 CRC(908ff25c) SHA1(02ebee891d88bacdadd37a3e75e05763b7ad3c9b) )
SMS_ROM_LOAD( sportsft,  "sports pad football (usa).bin",                                                          0x000000 , 0x20000,	 CRC(e42e4998) SHA1(556d9ab4ba3c3a34440b36c6fc8e972f70f16d72) )
SMS_ROM_LOAD( sportssc,  "sports pad soccer (japan).bin",                                                          0x000000 , 0x20000,	 CRC(41c948bf) SHA1(7634ce39e87049dad1ee4f32a80d728e4bd1f81f) )
SMS_ROM_LOAD( spyvsspyj, "spy vs spy (japan).bin",                                                                 0x000000 , 0x8000,	 CRC(d41b9a08) SHA1(c5e004b34d6569e6e1d99bff6be940f308e2c39f) )
SMS_ROM_LOAD( spyvsspyu, "spy vs. spy (u) (display-unit cart) [!].bin",                                            0x000000 , 0x40000,	 CRC(b87e1b2b) SHA1(03eec0d33d7c3b376fe08ffec79f84091f58366b) )
SMS_ROM_LOAD( spyvsspy,  "spy vs. spy (usa, europe).bin",                                                          0x000000 , 0x8000,	 CRC(78d7faab) SHA1(feab16dd8b807b88395e91c67e9ff52f8f7aa7e4) )
SMS_ROM_LOAD( starwars,  "star wars (europe).bin",                                                                 0x000000 , 0x80000,	 CRC(d4b8f66d) SHA1(be75ca8ace66b72a063b4be2da2b1ed92f8449b0) )
SMS_ROM_LOAD( sf2,       "street fighter ii (brazil).bin",                                                         0x000000 , 0xc8000,	 CRC(0f8287ec) SHA1(1866752a92abbf0eb55fbf9de1cd1b731ec62a54) )
SMS_ROM_LOAD( sor,       "streets of rage (europe).bin",                                                           0x000000 , 0x80000,	 CRC(4ab3790f) SHA1(5bdec24d9ba0f6ed359dcb3b11910ec86866ec98) )
SMS_ROM_LOAD( sor2,      "streets of rage ii (europe).bin",                                                        0x000000 , 0x80000,	 CRC(04e9c089) SHA1(cc18171a860711f6ad18ff89254dd7bd05c54654) )
SMS_ROM_LOAD( strider,   "strider (usa, europe).bin",                                                              0x000000 , 0x80000,	 CRC(9802ed31) SHA1(051e72c8ffec7606c04409ef38244cfdd592252f) )
SMS_ROM_LOAD( strider2,  "strider ii (europe).bin",                                                                0x000000 , 0x40000,	 CRC(b8f0915a) SHA1(f9d58bd9a5a99f2fd26fb411b0783fcd220249a4) )
SMS_ROM_LOAD( submarin,  "submarine attack (europe).bin",                                                          0x000000 , 0x40000,	 CRC(d8f2f1b9) SHA1(44384ec8bf91e1ca5512ff88bbcf1ae1ce5a1a35) )
SMS_ROM_LOAD( suhocheo,  "suho cheonsa (kr).bin",                                                                  0x000000,  0x40000,	 CRC(01686d67) SHA1(1dae4088a7bf0138e8f6128d37b90aa705f8ac51) )
SMS_ROM_LOAD( sukeban,   "sukeban deka ii - shoujo tekkamen densetsu (japan).bin",                                 0x000000 , 0x20000,	 CRC(b13df647) SHA1(5cd041990c7418ba63cde54f83d3e0e323b42c3b) )
SMS_ROM_LOAD( summergb,  "summer games (europe) (beta).bin",                                                       0x000000 , 0x20000,	 CRC(80eb1fff) SHA1(91b46d3169e66549c94bdb4a5f92c2fc4bb2abe8) )
SMS_ROM_LOAD( summerg,   "summer games (europe).bin",                                                              0x000000 , 0x20000,	 CRC(8da5c93f) SHA1(f9638b693ca3a7c65cfc589aaea0bbab56fc7238) )
SMS_ROM_LOAD( suprbskt,  "super basketball (usa) (sample).bin",                                                    0x000000 , 0x10000,	 CRC(0dbf3b4a) SHA1(616b95baad61086f08d8688c2bf06838b902ac75) )
SMS_ROM_LOAD( suprkick,  "super kick off (europe) (en,fr,de,es,it,nl,pt,sv).bin",                                  0x000000 , 0x40000,	 CRC(406aa0c2) SHA1(4116e4a742e3209ca5db9887cd92219d15cf3c9a) )
SMS_ROM_LOAD( smgp,      "super monaco gp (europe).bin",                                                           0x000000 , 0x40000,	 CRC(55bf81a0) SHA1(0f11b9d7d6d16b09f7be0dace3b6c7d3524da725) )
SMS_ROM_LOAD( smgpu,     "super monaco gp (usa).bin",                                                              0x000000 , 0x40000,	 CRC(3ef12baa) SHA1(fc933d9ec3a7e699c1e2cda89b957665e7321a80) )
SMS_ROM_LOAD( superoff,  "super off road (europe).bin",                                                            0x000000 , 0x20000,	 CRC(54f68c2a) SHA1(804cd74bbcf452613f6c3a722be1c94338499813) )
SMS_ROM_LOAD( superrac,  "super racing (japan).bin",                                                               0x000000 , 0x40000,	 CRC(7e0ef8cb) SHA1(442f3ba8a66a0d9c49a46091df83fcdba4b63c3a) )
SMS_ROM_LOAD( ssmashtv,  "super smash t.v. (europe).bin",                                                          0x000000 , 0x40000,	 CRC(e0b1aff8) SHA1(b4515aad1cad31980d041632e23d3be82aa31828) )
SMS_ROM_LOAD( ssinv,     "super space invaders (europe).bin",                                                      0x000000 , 0x40000,	 CRC(1d6244ee) SHA1(b28d2a9c0fe597892e21fb2611798765f5435885) )
SMS_ROM_LOAD( superten,  "super tennis (usa, europe).bin",                                                         0x000000 , 0x8000,	 CRC(914514e3) SHA1(67787f3f29a5b5e74b5f6a636428da4517a0f992) )
SMS_ROM_LOAD( wboyj,     "super wonder boy (japan).bin",                                                           0x000000 , 0x20000,	 CRC(e2fcb6f3) SHA1(14210196f454b6d938f15cf7b52076796c5d0f7d) )
SMS_ROM_LOAD( wboymlndj, "super wonder boy - monster world (japan).bin",                                           0x000000 , 0x40000,	 CRC(b1da6a30) SHA1(f4cd1ee6f98bc77fb36e232bf755d61d88e219d7) )
SMS_ROM_LOAD( superman,  "superman - the man of steel (europe).bin",                                               0x000000 , 0x40000,	 CRC(6f9ac98f) SHA1(f12b0eddfc271888bbcb1de3df25072b96b024ec) )
SMS_ROM_LOAD( t2ag,      "t2 - the arcade game (europe).bin",                                                      0x000000 , 0x80000,	 CRC(93ca8152) SHA1(cfa4a899185fced837991d14f011cdaca81e9dd7) )
SMS_ROM_LOAD( chasehq,   "taito chase h.q. (europe).bin",                                                          0x000000 , 0x40000,	 CRC(85cfc9c9) SHA1(495e3ced83ccd938b549bc76905097dba0aaf32b) )
SMS_ROM_LOAD( tazmars,   "taz in escape from mars (brazil).bin",                                                   0x000000 , 0x80000,	 CRC(11ce074c) SHA1(36a67210ca9762f280364007fcacbd7b1416d6ee) )
SMS_ROM_LOAD( tazmaniab, "taz-mania (europe) (beta).bin",                                                          0x000000 , 0x40000,	 CRC(1b312e04) SHA1(274641b3b05245504f763a5e4bc359183d16a092) )
SMS_ROM_LOAD( tazmania,  "taz-mania (europe).bin",                                                                 0x000000 , 0x40000,	 CRC(7cc3e837) SHA1(ac98f23ddc24609cb77bb13102e0386f8c2a4a76) )
SMS_ROM_LOAD( tecmow92,  "tecmo world cup '92 (europe) (beta).bin",                                                0x000000 , 0x40000,	 CRC(96e75f48) SHA1(bfc473bcfd849c8955f24e82347423fef4f7faf5) )
SMS_ROM_LOAD( tecmow93,  "tecmo world cup '93 (europe).bin",                                                       0x000000 , 0x40000,	 CRC(5a1c3dde) SHA1(43a30da241f57cf086ab9b2ed25fe018171f2908) )
SMS_ROM_LOAD( teddyboy,  "teddy boy (usa, europe).bin",                                                            0x000000 , 0x8000,	 CRC(2728faa3) SHA1(6ae39718703dbf7126f71387ce24ad956710a315) )
SMS_ROM_LOAD( teddyboyj1, "teddy boy blues (j) [p1][!].bin",                                                       0x000000 , 0x8000,	 CRC(9dfa67ee) SHA1(839da36557b494299b427bda4e32d2ba61977b25) )
SMS_ROM_LOAD( teddyboyjp, "teddy boy blues (japan) (proto) (ep-mycard).bin",                                       0x000000 , 0xc000,	 CRC(d7508041) SHA1(51014d89cd8770df8a3d837a0208cb69d5bf7903) )
SMS_ROM_LOAD( teddyboyj, "teddy boy blues (japan).bin",                                                            0x000000 , 0x8000,	 CRC(316727dd) SHA1(fb61c04f30c83733fdbf503b16e17aa8086932df) )
SMS_ROM_LOAD( tennis,    "tennis ace (europe).bin",                                                                0x000000 , 0x40000,	 CRC(1a390b93) SHA1(0d202166d4a3bdfcf90514c711c77e4f20764552) )
SMS_ROM_LOAD( tensai,    "tensai bakabon (japan).bin",                                                             0x000000 , 0x40000,	 CRC(8132ab2c) SHA1(ca802b7bdbc0b458d02fda2da32c2e27e50eef19) )
SMS_ROM_LOAD( term2,     "terminator 2 - judgment day (europe).bin",                                               0x000000 , 0x40000,	 CRC(ac56104f) SHA1(a8fe50e27fa9d44f3bd05d249a964352a32d1799) )
SMS_ROM_LOAD( termntrb,  "terminator, the (brazil).bin",                                                           0x000000 , 0x40000,	 CRC(e3d5ce9a) SHA1(bea0a1d0271faa2273e4cf1e968241d018c81da1) )
SMS_ROM_LOAD( termntr,   "terminator, the (europe).bin",                                                           0x000000 , 0x40000,	 CRC(edc5c012) SHA1(eaf733d385e61526b90c1b194bf605078d43e2d3) )
SMS_ROM_LOAD( 3dragon,   "three dragon story, the (k).bin",                                                        0x000000 , 0x8000,	 CRC(8640e7e8) SHA1(a05a7a41295c38add5dfc71032d2d83b96f518b1) )
SMS_ROM_LOAD( thunderbj, "thunder blade (japan).bin",                                                              0x000000 , 0x40000,	 CRC(c0ce19b1) SHA1(535dbd339f4b5c5efd502cffbbe719d7b3e7f1c3) )
SMS_ROM_LOAD( thunderb,  "thunder blade (usa, europe).bin",                                                        0x000000 , 0x40000,	 CRC(ae920e4b) SHA1(c38f3eea4e7224bc6042723e88b26c85b1a56ddc) )
SMS_ROM_LOAD( timesold,  "time soldiers (usa, europe).bin",                                                        0x000000 , 0x40000,	 CRC(51bd14be) SHA1(89842cb30cc3ba626901a5da41481f6d157ebd15) )
SMS_ROM_LOAD( tomjerry,  "tom & jerry (europe) (beta).bin",                                                        0x000000 , 0x40000,	 CRC(0c2fc2de) SHA1(83133cc875f4124b6ede7a9afc29aa311b36c285) )
SMS_ROM_LOAD( tomjermv,  "tom and jerry - the movie (europe).bin",                                                 0x000000 , 0x40000,	 CRC(bf7b7285) SHA1(30224286c65beddf37dc83f688f1bd362f325227) )
SMS_ROM_LOAD( totowld3,  "toto world 3 (korea) (unl).bin",                                                         0x000000 , 0x40000,	 CRC(4f8d75ec) SHA1(55ca8a8b1f2a342fc8b8fc3f3ccd98ed44b2fe98) )
SMS_ROM_LOAD( transbot,  "transbot (usa, europe).bin",                                                             0x000000 , 0x8000,	 CRC(4bc42857) SHA1(73273e6d44ad7aea828b642d22f6f1c138be9d2b) )
SMS_ROM_LOAD( treinam,   "treinamento do mymo (b).bin",                                                            0x000000 , 0x40000,	 CRC(e94784f2) SHA1(327c3518363ee060b681ff34c0b2eea3ffea27e4) )
SMS_ROM_LOAD( trivial,   "trivial pursuit - genus edition (europe) (en,fr,de,es).bin",                             0x000000 , 0x80000,	 CRC(e5374022) SHA1(bafaaeaa376e109db5d52a484a3efcd6ed84d4d6) )
SMS_ROM_LOAD( ttoriui,   "ttoriui moheom (kr).bin",                                                                0x000000,  0x8000,	 CRC(178801d2) SHA1(2ed6220b583e9f50d839d5ccecfcb2994584ba39) )
SMS_ROM_LOAD( turmamon,  "turma da monica em o resgate (brazil).bin",                                              0x000000 , 0x40000,	 CRC(22cca9bb) SHA1(77b4ec8086d81029b596020f202df3e210df985d) )
SMS_ROM_LOAD( tvcolos,   "tv colosso (brazil).bin",                                                                0x000000 , 0x80000,	 CRC(e1714a88) SHA1(1b4f7eca8a3f04ead404e6f439a6c49a0d0500df) )
SMS_ROM_LOAD( ultima4b,  "ultima iv - quest of the avatar (europe) (beta).bin",                                    0x000000 , 0x80000,	 CRC(de9f8517) SHA1(bb1ae06b62a9f7d3259c51eee4cfded781eb5d30) )
SMS_ROM_LOAD( ultima4,   "ultima iv - quest of the avatar (europe).bin",                                           0x000000 , 0x80000,	 CRC(b52d60c8) SHA1(a90e21e5961bcf2e10b715a009c04e7c2017a3b1) )
SMS_ROM_LOAD( ultimscr,  "ultimate soccer (europe) (en,fr,de,es,it).bin",                                          0x000000 , 0x40000,	 CRC(15668ca4) SHA1(e0adf52b6e1f54260dd6ea80e99ffb8bf76fd49a) )
SMS_ROM_LOAD( vampire,   "vampire (europe) (beta).bin",                                                            0x000000 , 0x40000,	 CRC(20f40cae) SHA1(d03fb8fb31d6c49ce92e0a4c952768896f798dd5) )
SMS_ROM_LOAD( vigilant,  "vigilante (usa, europe).bin",                                                            0x000000 , 0x40000,	 CRC(dfb0b161) SHA1(0dc37f1104508c2c0e2593b10dffc3f268ae8ff9) )
SMS_ROM_LOAD( vf,        "virtua fighter animation (brazil).bin",                                                  0x000000 , 0x100000,	 CRC(57f1545b) SHA1(ee24af11f2066dc8ffbefb72a5048a2471576229) )
SMS_ROM_LOAD( waltpay,   "walter payton football (usa).bin",                                                       0x000000 , 0x40000,	 CRC(3d55759b) SHA1(2d1201523540ae1673fe75bd2c9d1db1cc61987d) )
SMS_ROM_LOAD( wanted,    "wanted (usa, europe).bin",                                                               0x000000 , 0x20000,	 CRC(5359762d) SHA1(6135e4d0f76812f9d35ddb5b3e7d34d56a5458b3) )
SMS_ROM_LOAD( wwrldb,    "where in the world is carmen sandiego (brazil).bin",                                     0x000000 , 0x20000,	 CRC(88aa8ca6) SHA1(c848e92899dc9b6f664e98ad247fed86c0e46a41) )
SMS_ROM_LOAD( wwrld,     "where in the world is carmen sandiego (usa).bin",                                        0x000000 , 0x20000,	 CRC(428b1e7c) SHA1(683317c8d7d8b974066d7ddb3ed64f99801aa9de) )
SMS_ROM_LOAD( wimbledn,  "wimbledon (europe).bin",                                                                 0x000000 , 0x40000,	 CRC(912d92af) SHA1(07bc7896efc9eae77a9be68c190071c99eb17a8a) )
SMS_ROM_LOAD( wimbled2,  "wimbledon ii (europe).bin",                                                              0x000000 , 0x40000,	 CRC(7f3afe58) SHA1(55dd4aaa92de4bdaa1d8b6463a34887f4a8baa28) )
SMS_ROM_LOAD( winterolb, "winter olympics - lillehammer '94 (brazil) (en,fr,de,es,it,pt,sv,no).bin",               0x000000 , 0x80000,	 CRC(2fec2b4a) SHA1(758f3dc68db93f35e315fbf2fcd904484cce2ad1) )
SMS_ROM_LOAD( winterol,  "winter olympics - lillehammer '94 (europe) (en,fr,de,es,it,pt,sv,no).bin",               0x000000 , 0x80000,	 CRC(a20290b6) SHA1(20b73d53e6868957f07b1a813b853943bfb90307) )
SMS_ROM_LOAD( wolfch,    "wolfchild (europe).bin",                                                                 0x000000 , 0x40000,	 CRC(1f8efa1d) SHA1(92873f8d7b81a57114b135a2dfffc58d45643703) )
SMS_ROM_LOAD( wboy,      "wonder boy (usa, europe).bin",                                                           0x000000 , 0x20000,	 CRC(73705c02) SHA1(63149f20bf69cd2f24d0e58841fcfcdace972f49) )
SMS_ROM_LOAD( wboy3,     "wonder boy iii - the dragon's trap (usa, europe).bin",                                   0x000000 , 0x40000,	 CRC(679e1676) SHA1(99e73de2ffe5ea5d40998faec16504c226f4c1ba) )
SMS_ROM_LOAD( wboymlnd,  "wonder boy in monster land (ue) (v1.1) [h1].bin",                                        0x000000 , 0x40000,	 CRC(7522cf0a) SHA1(24889396756fc5a1e043e89d70c5431c9d98df7d) )
SMS_ROM_LOAD( wboymlndp, "wonder boy in monster land [proto].bin",                                                 0x000000,  0x40000,	 CRC(8312c429) SHA1(09fdb724ccf427af7ddd8f89cf0581f2a0b2f7f1) )
SMS_ROM_LOAD( wboymlndu, "wonder boy in monster land (usa, europe).bin",                                           0x000000 , 0x40000,	 CRC(8cbef0c1) SHA1(51715db7a49a18452292dadb2bba0108aa6d6402) )
SMS_ROM_LOAD( wboymwldb, "wonder boy in monster world (europe) (beta).bin",                                        0x000000 , 0x80000,	 CRC(81bff9bb) SHA1(7a26de7a544c602daeadbd3507028ac68ef35e91) )
SMS_ROM_LOAD( wboymwld,  "wonder boy in monster world (europe).bin",                                               0x000000 , 0x80000,	 CRC(7d7ce80b) SHA1(da0acdb1b9e806aa67a0216a675cb02ed24abf8b) )
SMS_ROM_LOAD( woodypop,  "woody pop - shinjinrui no block kuzushi (japan).bin",                                    0x000000 , 0x8000,	 CRC(315917d4) SHA1(b74078c4a3e6d20d21ca81e88c0cb3381b0c84a4) )
SMS_ROM_LOAD( wclead,    "world class leader board (europe).bin",                                                  0x000000 , 0x40000,	 CRC(c9a449b7) SHA1(7bd82131944020a20167f37071ccc1dfa3ae0b3d) )
SMS_ROM_LOAD( wcup90,    "world cup italia '90 (europe).bin",                                                      0x000000 , 0x20000,	 CRC(6e1ad6fd) SHA1(dfa51a4f982d0bec61532e16a679edae605d0aea) )
SMS_ROM_LOAD( wcup94,    "world cup usa 94 (europe) (en,fr,de,es,it,nl,pt,sv).bin",                                0x000000 , 0x80000,	 CRC(a6bf8f9e) SHA1(f41d81710f24b08a2a3ac28f2679338a47ca5890) )
SMS_ROM_LOAD( worldgb,   "world games (europe) (beta).bin",                                                        0x000000 , 0x20000,	 CRC(914d3fc4) SHA1(6361f796248c71ecfaaa02aafe6ddbbaebf6ebba) )
SMS_ROM_LOAD( worldg,    "world games (europe).bin",                                                               0x000000 , 0x20000,	 CRC(a2a60bc8) SHA1(4f3c04e40dd4b94d5d090068ba99b8461af56f51) )
SMS_ROM_LOAD( worldgp,   "world grand prix (europe).bin",                                                          0x000000 , 0x20000,	 CRC(4aaad0d6) SHA1(650f15ebbd149f5c357f089d7bd305fcb20b068f) )
SMS_ROM_LOAD( worldgpu,  "world grand prix (usa).bin",                                                             0x000000 , 0x20000,	 CRC(7b369892) SHA1(feff411732ca2dc17dab6d7868749d79326993d7) )
SMS_ROM_LOAD( worldscr,  "world soccer (world).bin",                                                               0x000000 , 0x20000,	 CRC(72112b75) SHA1(bd385b69805c623ab9934174a19a30371584c4b0) )
SMS_ROM_LOAD( wwfwre,    "wwf wrestlemania - steel cage challenge (europe).bin",                                   0x000000 , 0x40000,	 CRC(2db21448) SHA1(6fd4f5af0f14e1e0a934cd9e39a6bb476eda7e97) )
SMS_ROM_LOAD( xmenmojo,  "x-men - mojo world (brazil).bin",                                                        0x000000 , 0x80000,	 CRC(3e1387f6) SHA1(6405a2f8b6f220b4349f8006c3d75dfcdcd6db6d) )
SMS_ROM_LOAD( xenon2,    "xenon 2 - megablast (europe) (image works).bin",                                         0x000000 , 0x40000,	 CRC(5c205ee1) SHA1(72cb8a24f63e9e79c65c26141abdf53f96c60c0c) )
SMS_ROM_LOAD( xenon2v,   "xenon 2 - megablast (europe) (virgin).bin",                                              0x000000 , 0x40000,	 CRC(ec726c0d) SHA1(860cff21eff077acd92b06a71d859bf3e81fe628) )
SMS_ROM_LOAD( ys,        "ys (japan).bin",                                                                         0x000000 , 0x40000,	 CRC(32759751) SHA1(614b589080b732e17cc0d253e17216a72a268955) )
SMS_ROM_LOAD( ysomens,   "ys - the vanished omens (usa, europe).bin",                                              0x000000 , 0x40000,	 CRC(b33e2827) SHA1(e73e836c353543e9f48315410b0d72278899ff59) )
SMS_ROM_LOAD( zaxxon3db, "zaxxon 3-d (world) (beta).bin",                                                          0x000000 , 0x40000,	 CRC(bba74147) SHA1(85a67064c71dfa58eb150cc090beb5ae6639b527) )
SMS_ROM_LOAD( zaxxon3d,  "zaxxon 3-d (world).bin",                                                                 0x000000 , 0x40000,	 CRC(a3ef13cb) SHA1(257946fe8a230ac1308fc60a8ed43851cfe6b915) )
SMS_ROM_LOAD( zillion,   "zillion (europe) (v1.2).bin",                                                            0x000000 , 0x20000,	 CRC(7ba54510) SHA1(5dbe3ac8c4888298f3f7008ed359d0187cf46695) )
SMS_ROM_LOAD( zillionj,  "zillion (japan, europe) (en,ja).bin",                                                    0x000000 , 0x20000,	 CRC(60c19645) SHA1(6a0a21426cadadb5567907d9cc4cdaf63195d5c3) )
SMS_ROM_LOAD( zillionu,  "zillion (usa) (v1.1).bin",                                                               0x000000 , 0x20000,	 CRC(5718762c) SHA1(3aab3e95dac0fa93612da20cf525dba4dc4ca6ba) )
SMS_ROM_LOAD( zillion2,  "zillion ii - the tri formation (world).bin",                                             0x000000 , 0x20000,	 CRC(2f2e3bc9) SHA1(1e08be828ecf4cf5db787704ab8779f4b5a444b5) )
SMS_ROM_LOAD( zool,      "zool - ninja of the 'nth' dimension (europe).bin",                                       0x000000 , 0x40000,	 CRC(9d9d0a5f) SHA1(aed98f2fc885c9a6e121982108f843388eb46304) )



/* 
    Notes on GameId:
    - first Id is the Japanese one, second Id is the USA one. European Ids are the same as USA with 
      a -5x appended to the end, depending on the country/language (generic English titles have a -50)
 */

SOFTWARE_LIST_START( sms_cart )
	SOFTWARE( 20embr,     0,        1995, "Tec Toy", "20 em 1 (Brazil)", 0, 0 )	/* Id: 0002 */
	SOFTWARE( aceoface,   0,        1991, "Sega", "Ace of Aces (Euro)", 0, 0 )	/* Id: 7054 */
	SOFTWARE( actionfg1,  actionfg, 1986, "Sega", "Action Fighter (Euro, Jpn, v1.1)", 0, 0 )	/* Id: G-1305, 5055 - Releases: 1986-08-17 (JPN) */
	SOFTWARE( actionfg,   0,        1986, "Sega", "Action Fighter (Euro, USA, v1.2)", 0, 0 )
	SOFTWARE( addamfam,   0,        1993, "Flying Edge", "The Addams Family (Euro)", 0, 0 )	/* Id: 27067-50 */
	SOFTWARE( aerialas,   0,        1990, "Sega", "Aerial Assault (Euro)", 0, 0 )	/* Id: 7041 */
	SOFTWARE( aerialasu,  aerialas, 1990, "Sega", "Aerial Assault (USA)", 0, 0 )
	SOFTWARE( afterb,     0,        1987, "Sega", "After Burner (World)", 0, 0 )	/* Id: G-1340, 9001 - Releases: 1987-12-12 (JPN) - Notes: FM support */
	SOFTWARE( airresc,    0,        1992, "Sega", "Air Rescue (Euro)", 0, 0 )	/* Id: 7102 */
	SOFTWARE( aladdin,    0,        1994, "Sega", "Aladdin (Euro)", 0, 0 )	/* Id: 9027 */
	SOFTWARE( alexhitw,   0,        1989, "Sega", "Alex Kidd - High-Tech World (Euro, USA)", 0, 0 )
	SOFTWARE( anmitsu,    alexhitw, 1987, "Sega", "Anmitsu Hime (Jpn)", 0, 0 )	/* Id: G-1328, 5116 - Releases: 1987-07-19 (JPN) - Note: the Western release, "Alex Kidd Hiigh-Tech World" is quite different */
	SOFTWARE( alexlost,   0,        1988, "Sega", "Alex Kidd - The Lost Stars (World)", 0, 0 )	/* Id: G-1347, 7005 - Releases: 1988-03-10 (JPN) - Notes: FM support */
	SOFTWARE( alexbmx,    0,        1987, "Sega", "Alex Kidd BMX Trial (Jpn)", 0, 0 )	/* Id: G-1330 - Releases: 1987-11-15 (JPN) - Notes: FM support */
	SOFTWARE( alexkidd,   0,        1986, "Sega", "Alex Kidd in Miracle World (Euro, USA, v1.1)", 0, 0 )
	SOFTWARE( alexkiddj,  alexkidd, 1986, "Sega", "Alex Kidd no Miracle World (Jpn)", 0, 0 )	/* Id: G-1306, 5067 - Releases: 1986-11-01 (JPN) */
	SOFTWARE( alexkiddb,  alexkidd, 1986, "<pirate>", "Alex Kidd in Miracle World (Brazil, v1.1, Pirate)", 0, 0 )
	SOFTWARE( alexkidd1,  alexkidd, 1986, "Sega", "Alex Kidd in Miracle World (Euro, USA)", 0, 0 )
	SOFTWARE( alexshin,   0,        1990, "Sega", "Alex Kidd in Shinobi World (Euro, USA)", 0, 0 )	/* Id: 7044 */
	SOFTWARE( alf,        0,        1989, "Sega", "Alf (USA)", 0, 0 )	/* Id: 5111? */
	SOFTWARE( alien3,     0,        1992, "Arena", "Alien 3 (Euro)", 0, 0 )	/* Id: 27043-50 */
	SOFTWARE( astorm,     0,        1991, "Sega", "Alien Storm (Euro)", 0, 0 )	/* Id: 7075 */
	SOFTWARE( asyndromj,  asyndrom, 1987, "Sega", "Alien Syndrome (Jpn)", 0, 0 )	/* Id: G-1339, 7006 - Releases: 1987-10-18 (JPN) - Notes: FM support */
	SOFTWARE( asyndrom,   0,        1987, "Sega", "Alien Syndrome (Euro, USA)", 0, 0 )
	SOFTWARE( altbeast,   0,        1989, "Sega", "Altered Beast (Euro, USA)", 0, 0 )	/* Id: 7018 */
	SOFTWARE( ameribb,    0,        1989, "Sega", "American Baseball (Euro)", 0, 0 )	/* Id: 7019 */
	SOFTWARE( ameripf,    0,        1989, "Sega", "American Pro Football (Euro)", 0, 0 )	/* Id: 7020 */
	SOFTWARE( andreaga,   0,        1993, "TecMagik", "Andre Agassi Tennis (Euro)", 0, 0 )	/* Id: 27051-50 */
	SOFTWARE( arcadesh,   0,        1992, "Virgin Interactive", "Arcade Smash Hits (Euro)", 0, 0 )	/* Id: 27032-50 */
	SOFTWARE( argosnj1,   argosnj , 1988, "Tecmo", "Argos no Juujiken (Jpn, Alt)", 0, 0 )	/* S-001 - Releases: 1988-03-25 (JPN) */
	SOFTWARE( argosnj,    0,        1988, "Tecmo", "Argos no Juujiken (Jpn)", 0, 0 )
	SOFTWARE( arielmer,   0,        1997, "Tec Toy", "Ariel - The Little Mermaid (Brazil)", 0, 0 )	/* Id: 026.350 - Releases: 1997 (BRA) */
	SOFTWARE( assaultc,   0,        1990, "Sega", "Assault City (Euro, Light Phaser version)", 0, 0 )	/* Id: 7040 */
	SOFTWARE( assaultc1,  assaultc, 1990, "Sega", "Assault City (Euro)", 0, 0 )	/* Id: 7034 */
	SOFTWARE( asterix,    0,        1991, "Sega", "Asterix (Euro, v1.1)", 0, 0 )	/* Id: 9008 */
	SOFTWARE( asterix1,   asterix,  1991, "Sega", "Asterix (Euro)", 0, 0 )
	SOFTWARE( tvcolos,    asterix,  1996, "Tec Toy", "As Aventuras da TV Colosso (Brazil)", 0, 0 )
	SOFTWARE( astergre,   0,        1993, "Sega", "Asterix and the Great Rescue (Euro)", 0, 0 )	/* Id: 9032 */
	SOFTWARE( astermis,   0,        1993, "Sega", "Asterix and the Secret Mission (Euro)", 0, 0 )	/* Id: 9023 */
	SOFTWARE( astrow,     0,        1987, "Sega", "Astro Warrior & Pit Pot (Euro)", 0, 0 )	/* Id: 6081 */
	SOFTWARE( astrowj,    astrow,   1986, "Sega", "Astro Warrior (Jpn, USA)", 0, 0 )	/* Id: G-1312, 5069 - Releases: 1986-11-16 (JPN) */
	SOFTWARE( fushigi,    astrow,   1985, "Sega", "Fushigi no Oshiro Pit Pot (Jpn)", 0, 0 )	/* Id: C-507 - Releases: 1985-12-14 (Jpn, card) */
	SOFTWARE( fushigi1,   astrow,   1985, "<pirate>", "Fushigi no Oshiro Pit Pot (Jpn, Pirate)", 0, 0 )
	SOFTWARE( saposos,    astrow,   1995, "Tec Toy", "Sapo Xulé - S.O.S Lagoa Poluida (Brazil)", 0, 0 )
	SOFTWARE( ayrton,     0,        1992, "Sega", "Ayrton Senna's Super Monaco GP II (Euro)", 0, 0 )	/* Id: 9011 */
	SOFTWARE( aztecadv,   0,        1987, "Sega", "Aztec Adventure - The Golden Road to Paradise (World) ~ Nazca '88 - The Golden Road to Paradise (Jpn)", 0, 0 )	/* Id: G-1335, 5100 - Releases: 1987-09-20 (JPN) - Notes: FM support */
	SOFTWARE( backtof2,   0,        1990, "Image Works", "Back to the Future Part II (Euro)", 0, 0 )	/* Id: 27010-50 */
	SOFTWARE( backtof3,   0,        1992, "Image Works", "Back to the Future Part III (Euro)", 0, 0 )	/* Id: 27020-50 */
	SOFTWARE( bakubaku,   0,        1996, "Tec Toy", "Baku Baku(Brazil)", 0, 0 )	/* Id: 025470 - Notes: Title screen says "Baku Baku Animal" */
	SOFTWARE( bankpani,   0,        1987, "Sega", "Bank Panic (Euro)", 0, 0 )	/* Id: 4084 (Card), 4584 (Cart) */
	SOFTWARE( bankpani1,  bankpani, 1987, "<pirate>", "Bank Panic (Euro, Pirate)", 0, 0 )
	SOFTWARE( basketni,   0,        1989, "Sega", "Basketball Nightmare (Euro)", 0, 0 )	/* Id: 7025 - Notes: Title screen says "Basket Ball Nightmare" */
	SOFTWARE( batmanre,   0,        1993, "Sega", "Batman Returns (Euro)", 0, 0 )	/* Id: 7112 */
	SOFTWARE( battleor,   0,        1989, "Sega", "Battle Out Run (Euro)", 0, 0 )	/* Id: 7033 */
	SOFTWARE( battlem,    0,        1994, "Tec Toy", "Battlemaniacs (Brazil)", 0, 0 )	/* Id: 027-370 */
	SOFTWARE( blackblt,   0,        1986, "Sega", "Black Belt (Euro, USA)", 0, 0 )
	SOFTWARE( hokuto1,    blackblt, 1986, "<pirate>", "Hokuto no Ken (Jpn, Pirate)", 0, 0 )
	SOFTWARE( hokuto,     blackblt, 1986, "Sega", "Hokuto no Ken (Jpn)", 0, 0 )	/* Id: G-1303, 5054 - Releases: 1986-07-20 (JPN) - Motes: The Western version "Black Belt" is quite different */
	SOFTWARE( bladeag,    0,        1988, "Sega", "Blade Eagle (World)", 0, 0 )	/* Id: G-1351, 8005 - Releases: 1988-03-26 (JPN) - Notes: FM support, 3D glasses support */
	SOFTWARE( bladeag1,   bladeag,  1988, "Sega", "Blade Eagle (World, Prototype)", 0, 0 )
	SOFTWARE( bomber,     0,        1989, "Sega", "Bomber Raid (World)", 0, 0 )	/* Id: G-1373, 27006 - Releases: 1989-02-04 (JPN) - Notes: FM support */
	SOFTWARE( bonanza,    0,        1991, "Sega", "Bonanza Bros. (Euro)", 0, 0 )	/* Id: 7073 */
	SOFTWARE( bonkers,    0,        1994, "Tec Toy", "Bonkers Wax Up! (Brazil)", 0, 0 )	/* Id: 028600 */
	SOFTWARE( bramst,     0,        1993, "Sony Imagesoft", "Bram Stoker's Dracula (Euro)", 0, 0 )	/* Id: 27065-50 */
	SOFTWARE( bublbobl,   0,        1988, "Sega", "Bubble Bobble (Euro)", 0, 0 )
	SOFTWARE( fbubbobl,   bublbobl, 1988, "Sega ", "Final Bubble Bobble (Jpn)", 0, 0 )	/* Id: G-1362, 7077 - Releases: 1988-07-02 (JPN) */
	SOFTWARE( buggyrun,   0,        1993, "Sega", "Buggy Run (Euro)", 0, 0 )	/* Id: 9025 */
	SOFTWARE( calgames,   0,        1989, "Sega", "California Games (Euro, USA)", 0, 0 )	/* Id: 7014 */
	SOFTWARE( calgame2,   0,        1993, "Sega", "California Games II (Euro)", 0, 0 )	/* Id: 7105 */
	SOFTWARE( calgame2b,  calgame2, 1993, "Sega", "California Games II (Brazil, Korea)", 0, 0 )
	SOFTWARE( captsilv,   0,        1988, "Sega", "Captain Silver (Euro, Jpn)", 0, 0 )	/* Id: G-1356, 5117 - Releases: 1988-07-02 (JPN) - Notes: FM support */
	SOFTWARE( captsilvu,  captsilv, 1988, "Sega", "Captain Silver (USA)", 0, 0 )
	SOFTWARE( casino,     0,        1989, "Sega", "Casino Games (Euro, USA)", 0, 0 )	/* Id: 7021 */
	SOFTWARE( castelo,    0,        1997, "Tec Toy", "Castelo Ra-Tim-Bum (Brazil)", 0, 0 )	/* Id: 028.720 */
	SOFTWARE( castlill,   0,        1990, "Sega", "Castle of Illusion Starring Mickey Mouse (Euro)", 0, 0 )	/* Id: 7053 */
	SOFTWARE( castlills,  castlill, 1990, "Sega", "Castle of Illusion Starring Mickey Mouse (Display Unit Sample)", 0, 0 )
	SOFTWARE( castlillu,  castlill, 1990, "Sega", "Castle of Illusion Starring Mickey Mouse (USA)", 0, 0 )
	SOFTWARE( champeur,   0,        1992, "TecMagik", "Champions of Europe (Euro)", 0, 0 )	/* Id: 7073 */
	SOFTWARE( chmphock,   0,        1994, "U.S. Gold", "Championship Hockey (Euro)", 0, 0 )	/* Id: 27084-50 */
	SOFTWARE( chapolim,   0,        1990, "Tec Toy", "Chapolim x Dracula - Um Duelo Assustador (Brazil)", 0, 0 )	/* Id: 023430 */
	SOFTWARE( cheese,     0,        1995, "Sega", "Cheese Cat-astrophe Starring Speedy Gonzales (Euro)", 0, 0 )	/* Id: 9033 */
	SOFTWARE( chopliftj,  choplift, 1986, "Sega", "Choplifter (Jpn, Prototype)", 0, 0 )
	SOFTWARE( choplift,   0,        1986, "Sega", "Choplifter (Euro, USA)", 0, 0 )	/* Id: 5051 */
	SOFTWARE( borgman,    cyborgh,  1988, "Sega", "Chouon Senshi Borgman (Jpn)", 0, 0 )	/* Id: G-1371 - Releases: 1988-12-01 (JPN) - Notes: FM support */
	SOFTWARE( borgmanp,   cyborgh,  1988, "Sega", "Chouon Senshi Borgman (Jpn, Prototype)", 0, 0 )
	SOFTWARE( chuckrck,   0,        1992, "Virgin Interactive", "Chuck Rock (Euro)", 0, 0 )	/* Id: 29004 */
	SOFTWARE( chukrck2b,  chukrck2, 1993, "Tec Toy", "Chuck Rock II - Son of Chuck (Brazil)", 0, 0 )
	SOFTWARE( chukrck2,   0,        1993, "Core Design", "Chuck Rock II - Son of Chuck (Euro)", 0, 0 )	/* Id: 29019 */
	SOFTWARE( circuit,    worldgp,  1986, "Sega", "The Circuit (Jpn)", 0, 0 )	/* Id: G-1304 - Releases: 1986-09-21 (JPN) */
	SOFTWARE( cloudmst,   0,        1989, "Sega", "Cloud Master (Euro, USA)", 0, 0 )	/* Id: 7027 */
	SOFTWARE( columns,    0,        1990, "Sega", "Columns (Euro, USA)", 0, 0 )	/* Id: 5120 */
	SOFTWARE( columnsp,   columns,  1990, "Sega", "Columns (Euro, USA, Prototype)", 0, 0 )
	SOFTWARE( comical,    0,        1986, "Sega", "Comical Machine Gun Joe (Jpn)", 0, 0 )	/* Id: C-511 - Releases: 1986-04-21 (Jpn, card) */
	SOFTWARE( coolspot,   0,        1993, "Virgin Interactive", "Cool Spot (Euro)", 0, 0 )	/* Id: 27068-50 */
	SOFTWARE( cosmic,     0,        1993, "Codemasters", "Cosmic Spacehead (Euro)", 0, 0 )	/* Id: 27074-50 */
	SOFTWARE( cybers,     0,        1990, "Sega", "The Cyber Shinobi (Euro)", 0, 0 )	/* Id: 7050 */
	SOFTWARE( cyborgh,    0,        1988, "Sega", "Cyborg Hunter (Euro, USA)", 0, 0 )
	SOFTWARE( daffy,      0,        1994, "Sega", "Daffy Duck in Hollywood (Euro)", 0, 0 )	/* Id: 9031 */
	SOFTWARE( dallye,     0,        1995, "Game Line", "Dallyeora Pigu-Wang (Korea)", 0, 0 )
	SOFTWARE( danan,      0,        1990, "Sega", "Danan - The Jungle Fighter (Euro)", 0, 0 )	/* Id: 7049 */
	SOFTWARE( deadang,    0,        1989, "Sega", "Dead Angle (Euro, USA)", 0, 0 )	/* Id: 7030 */
	SOFTWARE( deepduck,   0,        1993, "Sega", "Deep Duck Trouble Starring Donald Duck (Euro)", 0, 0 )	/* Id: 9022 */
	SOFTWARE( desert,     0,        1993, "Sega", "Desert Speedtrap Starring Road Runner and Wile E. Coyote (Euro)", 0, 0 )	/* Id: 7122 */
	SOFTWARE( dstrike,    0,        1992, "Domark", "Desert Strike (Euro)", 0, 0 )	/* Id: 29010-50 */
	SOFTWARE( dicktr,     0,        1990, "Sega", "Dick Tracy (Euro, USA)", 0, 0 )	/* Id: 7057 */
	SOFTWARE( dinobash,   0,        19??, "Codemasters", "Dinobasher Starring Bignose the Caveman (Euro, Prototype)", 0, 0 )
	SOFTWARE( dinodool,   0,        19??, "<unknown>", "The Dinosaur Dooley (Korea)", 0, 0 )
	SOFTWARE( ddragon,    0,        1988, "Sega", "Double Dragon (World)", 0, 0 )	/* Id: G-1369, 7012 - Releases: 1988-10-01 (JPN) - Notes: FM support */
	SOFTWARE( doublhwk1,  doublhwk, 1990, "Sega", "Double Hawk (Euro, Prototype)", 0, 0 )
	SOFTWARE( doublhwk,   0,        1990, "Sega", "Double Hawk (Euro)", 0, 0 )	/* Id: 7036 */
	SOFTWARE( doubltgt,   quartet,  1987, "Sega", "Double Target - Cynthia no Nemuri (Jpn)", 0, 0 )	/* Id: G-1314 - Releases: 1987-01-18 (JPN) */
	SOFTWARE( drhello,    0,        19??, "<unknown>", "Dr. HELLO (Korea)", 0, 0 )
	SOFTWARE( drrobotn,   0,        1993, "Sega", "Dr. Robotnik's Mean Bean Machine (Euro)", 0, 0 )	/* Id: 7123 */
	SOFTWARE( dragon,     0,        1994, "Virgin Interactive", "Dragon - The Bruce Lee Story (Euro)", 0, 0 )	/* Id: 27079-50 */
	SOFTWARE( dragoncr,   0,        1991, "Sega", "Dragon Crystal (Euro)", 0, 0 )	/* Id: 5123 */
	SOFTWARE( dynaduke,   0,        1991, "Sega", "Dynamite Duke (Euro)", 0, 0 )	/* Id: 7067 */
	SOFTWARE( dynadux,    0,        1989, "Sega", "Dynamite Dux (Euro)", 0, 0 )	/* Id: 7029 */
	SOFTWARE( dynahead,   0,        1994, "Tec Toy", "Dynamite Headdy (Brazil)", 0, 0 )	/* Id: 028.570 */
	SOFTWARE( eswatc,     0,        1990, "Sega", "E-SWAT - City Under Siege (Euro, USA, Easy Version)", 0, 0 )	/* Id: 7042 */
	SOFTWARE( eswatc1,    0,        1990, "Sega", "E-SWAT - City Under Siege (Euro, USA, Hard Version)", 0, 0 )
	SOFTWARE( ejim,       0,        1996, "Tec Toy", "Earthworm Jim (Brazil)", 0, 0 )	/* Id: 028.690 */
	SOFTWARE( eccotide,   0,        1996, "Tec Toy", "Ecco - The Tides of Time (Brazil)", 0, 0 )	/* Id: 028.630 */
	SOFTWARE( ecco,       0,        1993, "Sega", "Ecco the Dolphin (Euro)", 0, 0 )	/* Id: 9029 */
	SOFTWARE( enduroj,    enduro,   1987, "Sega", "Enduro Racer (Jpn)", 0, 0 )	/* Id: G-1322, 5077 - Releases: 1987-05-18 (JPN) */
	SOFTWARE( enduro,     0,        1987, "Sega", "Enduro Racer (Euro, USA)", 0, 0 )
	SOFTWARE( excdizzy,   0,        19??, "Codemasters", "The Excellent Dizzy Collection (Euro, USA, Prototype)", 0, 0 )
	SOFTWARE( f16fight,   0,        1986, "Sega", "F-16 Fighter (Euro, USA)", 0, 0 )	/* Id: 4581 (Euro Cart re-release?) */
	SOFTWARE( f16falcj,   f16fight, 1985, "Sega", "F-16 Fighting Falcon (Jpn)", 0, 0 )	/* Id: C-508, 4081 - Releases: 1985-12-22 (Jpn, card) */
	SOFTWARE( f16falc,    f16fight, 1985, "Sega", "F-16 Fighting Falcon (USA)", 0, 0 )
	SOFTWARE( f1,         0,        1993, "Domark", "F1 (Euro)", 0, 0 )	/* Id: 27072-50 */
	SOFTWARE( fantdizz,   0,        1993, "Codemasters", "Fantastic Dizzy (Euro)", 0, 0 )	/* Id: 27074-50 */
	SOFTWARE( fantzonej,  fantzone, 1986, "Sega", "Fantasy Zone (Jpn)", 0, 0 )	/* Id: G-1301, 5052 - Releases: 1986-06-15 (JPN) */
	SOFTWARE( fantzone1,  fantzone, 1986, "Sega", "Fantasy Zone (World, v1.1 Prototype)", 0, 0 )
	SOFTWARE( fantzone,   0,        1986, "Sega", "Fantasy Zone (World, v1.2)", 0, 0 )
	SOFTWARE( fantzonm,   0,        1987, "Sega", "Fantasy Zone - The Maze (Euro, USA)", 0, 0 )
	SOFTWARE( opaopa,     fantzonm, 1987, "Sega", "Opa Opa (Jpn)", 0, 0 )	/* Id: G-1343 - Releases: 1987-12-20 (JPN) - Notes: FM support */
	SOFTWARE( fantzon2,   0,        1987, "Sega", "Fantasy Zone II - The Tears of Opa-Opa (Euro, USA)", 0, 0 )	/* Id: 5108 */
	SOFTWARE( fantzon2j,  fantzon2, 1987, "Sega", "Fantasy Zone II - Opa Opa no Namida (Jpn)", 0, 0 )	/* Id: G-1329, 7004 - Releases: 1987-10-17 (JPN) - Notes: FM support */
	SOFTWARE( felipe,     0,        19??, "Tec Toy", "Felipe em Acao (Brazil)", 0, 0 )
	SOFTWARE( ferias,     0,        1996, "Tec Toy", "Ferias Frustradas do Pica Pau (Brazil)", 0, 0 )	/* Id: 028.660 */
	SOFTWARE( fifa,       0,        1994, "Tec Toy", "FIFA International Soccer (Brazil)", 0, 0 )	/* Id: 028.670 */
	SOFTWARE( fireforg,   0,        1990, "Titus Arcade", "Fire & Forget II (Euro)", 0, 0 )	/* Id: 27009-50 */
	SOFTWARE( fireice,    0,        1993, "Tec Toy", "Fire & Ice (Brazil)", 0, 0 )	/* Id: 027.360 */
	SOFTWARE( flash,      0,        1993, "Sega", "The Flash (Euro)", 0, 0 )	/* Id: 7506 */
	SOFTWARE( flints,     0,        1991, "Grandslam", "The Flintstones (Euro)", 0, 0 )	/* Id: 27013-50 */
	SOFTWARE( forgottn,   0,        1991, "Sega", "Forgotten Worlds (Euro)", 0, 0 )	/* Id: 7056 */
	SOFTWARE( glocair,    0,        1991, "Sega", "G-LOC Air Battle (Euro)", 0, 0 )	/* Id: 7071 */
	SOFTWARE( gaegujan,   0,        19??, "<unknown>", "Gaegujangi Ggachi (Korea)", 0, 0 )
	SOFTWARE( gaingrnd,   0,        1990, "Sega", "Gain Ground (Euro)", 0, 0 )	/* Id: 7045 */
	SOFTWARE( gaingrndp,  gaingrnd, 1990, "Sega", "Gain Ground (Euro, Prototype)", 0, 0 )	/* Id: 7045 */
	SOFTWARE( galactpr,   0,        1988, "Sega", "Galactic Protector (Jpn)", 0, 0 )	/* Id: G-1348 - Releases: 1988-02-21 (JPN) - Notes: FM support */
	SOFTWARE( galaxyf,    0,        1989, "Sega", "Galaxy Force (Euro)", 0, 0 )	/* Id: 29001 */
	SOFTWARE( galaxyfu,   0,        1989, "Activision", "Galaxy Force (USA)", 0, 0 )
	SOFTWARE( gamebox,    0,        19??, "Tec Toy", "Game Box Serie Esportes Radicais (Brazil)", 0, 0 )
	SOFTWARE( gangster,   0,        1987, "Sega", "Gangster Town (Euro, USA)", 0, 0 )	/* Id: 5074 */
	SOFTWARE( gauntlet,   0,        1990, "U.S. Gold", "Gauntlet (Euro)", 0, 0 )	/* Id: 25006 */
	SOFTWARE( foremnko,    0,        1992, "Flying Edge", "George Foreman's KO Boxing (Euro)", 0, 0 )	/* Id: 27041-50 */
	SOFTWARE( ghosthj1,   ghosth,   1986, "<pirate>", "Ghost House (Jpn, Pirate)", 0, 0 )
	SOFTWARE( ghosthj,    ghosth,   1986, "Sega", "Ghost House (Jpn)", 0, 0 )	/* Id: C-512, 4002 (Card), 4502 (Cart) - Releases: 1986-04-21 (Jpn, card) */
	SOFTWARE( ghosthb,    ghosth,   1986, "Sega", "Ghost House (Euro, USA, Prototype)", 0, 0 )
	SOFTWARE( ghosth,     0,        1986, "Sega", "Ghost House (Euro, USA)", 0, 0 )
	SOFTWARE( ghostbst,   0,        1989, "Sega", "Ghostbusters (Euro, USA)", 0, 0 )	/* Id: 5065 */
	SOFTWARE( ghouls,     0,        1988, "Sega", "Ghouls'n Ghosts (Euro, USA)", 0, 0 )	/* Id: 7055 */
	SOFTWARE( globalb,    global,   1987, "Sega", "Global Defense (Euro, USA, Prototype)", 0, 0 )
	SOFTWARE( global,     0,        1987, "Sega", "Global Defense (Euro, USA)", 0, 0 )
	SOFTWARE( sdi,        global,   1987, "Sega", "SDI (Jpn)", 0, 0 )	/* Id: G-1338, 5102 - Releases: 1987-10-24 (JPN) - Notes: FM support */
	SOFTWARE( gaxe,       0,        1989, "Sega", "Golden Axe (Euro, USA)", 0, 0 )	/* Id: 9004 */
	SOFTWARE( gaxewarr,   0,        1990, "Sega", "Golden Axe Warrior (Euro, USA)", 0, 0 )	/* Id: 7505 */
	SOFTWARE( golfamanb,  golfaman, 1990, "Sega", "Golfamania (Euro, Prototype)", 0, 0 )
	SOFTWARE( golfaman,   0,        1990, "Sega", "Golfamania (Euro)", 0, 0 )	/* Id: 7502 */
	SOFTWARE( golvell,    0,        1988, "Sega", "Golvellius (Euro, USA)", 0, 0 )	/* Id: 7017 */
	SOFTWARE( maougolv,   golvell,  1988, "Sega", "Maou Golvellius (Jpn)", 0, 0 )	/* Id: G-1363, 7017 - Releases: 1988-08-14 (JPN) - Notes: FM support */
	SOFTWARE( maougolvp,  golvell,  1988, "Sega", "Maou Golvellius (Jpn, Prototype)", 0, 0 )
	SOFTWARE( gprider,    0,        1993, "Sega", "GP Rider (Euro)", 0, 0 )	/* Id: 9013 */
	SOFTWARE( greatbasj1, greatbas, 1985, "<pirate>", "Great Baseball (Jpn, Pirate)", 0, 0 )
	SOFTWARE( greatbasj,  greatbas, 1985, "Sega", "Great Baseball (Jpn)", 0, 0 )	/* Id: C-505, 5061 - Releases: 1985-12-15 (Jpn, card) */
	SOFTWARE( greatbas,   0,        1985, "Sega", "Great Baseball (Euro, USA)", 0, 0 )
	SOFTWARE( greatbsk,   0,        1987, "Sega", "Great Basketball (World)", 0, 0 )	/* Id: G-1320, 5071 - Releases: 1987-03-29 (JPN) */
	SOFTWARE( greatftb,   0,        1987, "Sega", "Great Football (World)", 0, 0 )	/* Id: G-1321, 5058 - Releases: 1987-04-29 (JPN) */
	SOFTWARE( greatglfj,  0,        1986, "Sega", "Great Golf (Jpn)", 0, 0 )	/* Id: G-1313 - Releases: 1986-12-20 (JPN) */
	SOFTWARE( greatglf1,  greatglf, 1987, "Sega", "Great Golf (Euro, USA, v1.0)", 0, 0 )
	SOFTWARE( greatglfb,  greatglf, 1987, "Sega", "Great Golf (World, Prototype)", 0, 0 )
	SOFTWARE( greatglf,   0,        1987, "Sega", "Great Golf (World) ~ Masters Golf (Japan)", 0, 0 )	/* Id: G-1332, 5057 - Releases: 1987-10-10 (JPN) - Notes: FM support */
	SOFTWARE( greatice,   0,        1987, "Sega", "Great Ice Hockey (Jpn, USA)", 0, 0 )	/* Id: 5062 (USA) - Releases: 1986 (USA), 1987 (Jpn, only 1000 copies as a prize for a contest) - Notes: FM support */
	SOFTWARE( greatscr,   0,        1985, "Sega", "Great Soccer (Euro)", 0, 0 )
	SOFTWARE( greatscrj1, greatscr, 1985, "<pirate>", "Great Soccer (Jpn, Pirate)", 0, 0 )
	SOFTWARE( greatscrj,  greatscr, 1985, "Sega", "Great Soccer (Jpn)", 0, 0 )	/* Id: C-504, 4082 (Euro card), 5059 - Releases: 1985-10-27 (Jpn, card) */
	SOFTWARE( greattns,   superten, 1985, "Sega", "Great Tennis (Jpn)", 0, 0 )	/* Id: C-515 - Releases: 1985-12-22 (Jpn, card) */
	SOFTWARE( greatvolj,  greatvol, 1987, "Sega", "Great Volleyball (Jpn)", 0, 0 )	/* Id: G-1317 - Releases: 1987-03-29 (JPN) */
	SOFTWARE( greatvol,   0,        1987, "Sega", "Great Volleyball (Euro, USA)", 0, 0 )	/* Id: 5070 */
	SOFTWARE( hangonaw,   hangon,   1986, "Sega", "Hang-On & Astro Warrior (USA)", 0, 0 )	/* Id: 6001 */
	SOFTWARE( hangonsh,   hangon,   1986, "Sega", "Hang-On & Safari Hunt (USA)", 0, 0 )
	SOFTWARE( hangon,     0,        1985, "Sega", "Hang-On (Euro)", 0, 0 )
	SOFTWARE( hangonj,    hangon,   1985, "Sega", "Hang-On (Jpn)", 0, 0 )	/* Id: C-502, 4080 (Card), 4580 (Cart) - Releases: 1985-10-20 (Jpn, card) */
	SOFTWARE( heavyw,     0,        1991, "Sega", "Heavyweight Champ (Euro)", 0, 0 )	/* Id:7063 */
	SOFTWARE( heroes,     0,        1991, "U.S. Gold", "Advanced Dungeon & Dragons - Heroes of the Lance (Euro)", 0, 0 )	/* Id: 29003-50 */
	SOFTWARE( highsc,     0,        1986, "Sega", "High School! Kimengumi (Jpn)", 0, 0 )	/* Id: G-1309 - Releases: 1986-12-15 (JPN) */
	SOFTWARE( homea,      0,        1993, "Sega", "Home Alone (Euro)", 0, 0 )	/* Id: 7109 */
	SOFTWARE( hook,       0,        19??, "Sega", "Hook (Euro, Prototype)", 0, 0 )
	SOFTWARE( hoshiw,     0,        1988, "Sega", "Hoshi wo Sagashite... (Jpn)", 0, 0 )	/* Id: G-1354 - Releases: 1988-04-02 (JPN) - Notes: FM support */
	SOFTWARE( hwaran,     0,        1988, "Samsung", "Hwarang Ui Geom (Korea)", 0, 0 )
	SOFTWARE( impmissb,   impmiss,  1990, "U.S. Gold", "Impossible Mission (Euro, Prototype)", 0, 0 )
	SOFTWARE( impmiss,    0,        1990, "U.S. Gold", "Impossible Mission (Euro)", 0, 0 )	/* Id: 25007-50 */
	SOFTWARE( incred,     0,        1993, "Flying Edge", "The Incredible Crash Dummies (Euro)", 0, 0 )	/* Id: 27057-50 */
	SOFTWARE( incrhulk,   0,        1994, "U.S. Gold", "The Incredible Hulk (Euro)", 0, 0 )	/* Id: 29016-50 */
	SOFTWARE( indycrusb,  indycrus, 1990, "U.S. Gold", "Indiana Jones and the Last Crusade (Euro, Prototype)", 0, 0 )
	SOFTWARE( indycrus,   0,        1990, "U.S. Gold", "Indiana Jones and the Last Crusade (Euro)", 0, 0 )	/* Id: 27008-50 */
	SOFTWARE( jamesb,     0,        1990, "Sega", "James 'Buster' Douglas Knockout Boxing (USA)", 0, 0 )	/* Id: 7063 */
	SOFTWARE( jamesbp,    jamesb,   1990, "Sega", "James 'Buster' Douglas Knockout Boxing (USA, Prototype)", 0, 0 )
	SOFTWARE( jb007b,     jb007,    1993, "Tec Toy", "James Bond 007 - The Duel (Brazil)", 0, 0 )
	SOFTWARE( jb007,      0,        1993, "Domark", "James Bond 007 - The Duel (Euro)", 0, 0 )	/* Id: 27046-50 */
	SOFTWARE( robocod,    0,        1993, "U.S. Gold", "James Pond 2 - Codename RoboCod (Euro)", 0, 0 )	/* Id: 29013-50 */
	SOFTWARE( joemont,    0,        1990, "Sega", "Joe Montana Football (Euro, USA)", 0, 0 )	/* Id: 7062 */
	SOFTWARE( jungle,     0,        1994, "Virgin Interactive", "The Jungle Book (Euro)", 0, 0 )	/* Id: 27069-5* */
	SOFTWARE( jurassic,   0,        1993, "Sega", "Jurassic Park (Euro)", 0, 0 )	/* Id: 9030 */
	SOFTWARE( kenseidj,   kenseid,  1988, "Sega", "Kenseiden (Jpn)", 0, 0 )	/* Id: G-1358, 7013 - Releases: 1988-06-02 (JPN) - Notes: FM support */
	SOFTWARE( kenseid,    0,        1988, "Sega", "Kenseiden (Euro, USA)", 0, 0 )
	SOFTWARE( kingsqb,    kingsq,   1989, "Parker Brothers", "King's Quest - Quest for the Crown (USA, Prototype)", 0, 0 )
	SOFTWARE( kingsq,     0,        1989, "Parker Brothers", "King's Quest - Quest for the Crown (USA)", 0, 0 )	/* Id: 4360 */
	SOFTWARE( klax,       0,        1991, "Tengen", "Klax (Euro)", 0, 0 )	/* Id: 301040-0160 */
	SOFTWARE( krusty,     0,        1992, "Flying Edge", "Krusty's Fun House (Euro)", 0, 0 )	/* Id: 27056-50 */
	SOFTWARE( kungfuk,    0,        1987, "Sega", "Kung Fu Kid (Euro, USA)", 0, 0 )	/* Id: 5078 */
	SOFTWARE( landill,    0,        1992, "Sega", "Land of Illusion Starring Mickey Mouse (Euro)", 0, 0 )	/* Id: 9014 */
	SOFTWARE( laserg,     0,        1991, "Sega", "Laser Ghost (Euro)", 0, 0 )	/* Id: 7074 */
	SOFTWARE( legndill,   0,        1994, "Tec Toy", "Legend of Illusion Starring Mickey Mouse (Brazil)", 0, 0 )	/* Id: 028750 */
	SOFTWARE( lemmingsb,  lemmings, 1991, "Sega", "Lemmings (Euro, Prototype)", 0, 0 )
	SOFTWARE( lemmings,   0,        1991, "Sega", "Lemmings (Euro)", 0, 0 )	/* Id: 7108 */
	SOFTWARE( lineof,     0,        1991, "Sega", "Line of Fire (Euro)", 0, 0 )	/* Id: 9006 */
	SOFTWARE( lionking,   0,        1994, "Virgin Interactive", "Disney's The Lion King (Euro)", 0, 0 )	/* Id: 27081-5* */
	SOFTWARE( lordswrdj,  lordswrd, 1988, "Sega", "Lord of Sword (Jpn)", 0, 0 )	/* Id: G-1361, 7016 - Releases: 1988-06-02 (JPN) - Notes: FM support */
	SOFTWARE( lordswrd,   0,        1988, "Sega", "Lord of the Sword (Euro, USA)", 0, 0 )
	SOFTWARE( loretta,    0,        1987, "Sega", "Loretta no Shouzou (Jpn)", 0, 0 )	/* Id: G-1315 - Releases: 1987-02-18 (JPN) */
	SOFTWARE( luckydimb,  luckydim, 1991, "Sega", "The Lucky Dime Caper Starring Donald Duck (Euro, Prototype)", 0, 0 )
	SOFTWARE( luckydim,   0,        1991, "Sega", "The Lucky Dime Caper Starring Donald Duck (Euro)", 0, 0 )	/* Id: 7072 */
	SOFTWARE( mahjongb,   mahjong,  1987, "Sega", "Mahjong Sengoku Jidai (Jpn, Prototype)", 0, 0 )
	SOFTWARE( mahjong,    0,        1987, "Sega", "Mahjong Sengoku Jidai (Jpn)", 0, 0 )	/* Id: G-1337 - Releases: 1987-10-18 (JPN) */
	SOFTWARE( makairet,   kungfuk,  1987, "Sega", "Makai Retsuden (Jpn)", 0, 0 )	/* Id: G-1324 - Releases: 1987-05-17 (JPN) */
	SOFTWARE( marble,     0,        1992, "Virgin Interactive", "Marble Madness (Euro)", 0, 0 )	/* Id: 27024-50 */
	SOFTWARE( marksman,   0,        1986, "Sega", "Marksman Shooting & Trap Shooting & Safari Hunt (Euro)", 0, 0 )	/* Id: 6080 */
	SOFTWARE( marksmanu,  marksman, 1986, "Sega", "Marksman Shooting & Trap Shooting (USA)", 0, 0 )
	SOFTWARE( mastdark,   0,        1992, "Sega", "Master of Darkness (Euro)", 0, 0 )	/* Id: 7107 */
	SOFTWARE( mastcomb,   0,        1993, "Sega", "Masters of Combat (Euro)", 0, 0 )	/* Id: 7124 */
	SOFTWARE( mazehunt,   0,        1988, "Sega", "Maze Hunter 3-D (Euro, USA)", 0, 0 )
	SOFTWARE( mazewalk,   mazehunt, 1988, "Sega", "Maze Walker (Jpn)", 0, 0 )	/* Id: G-1345, 8003 - Releases: 1988-01-31 (JPN) - Notes: FM support, 3D glasses support */
	SOFTWARE( megumi,     0,        1988, "Sega", "Megumi Rescue (Jpn)", 0, 0 )	/* Id: G-1359 - Releases: 1988-07-30 (JPN) - Notes: FM support */
	SOFTWARE( mercs,      0,        1990, "Sega", "Mercs (Euro)", 0, 0 )	/* Id: 9007 */
	SOFTWARE( mwalkb,     mwalk,    1990, "Sega", "Michael Jackson's Moonwalker (Euro, USA, Prototype)", 0, 0 )
	SOFTWARE( mwalk,      0,        1990, "Sega", "Michael Jackson's Moonwalker (Euro, USA)", 0, 0 )	/* Id: 7052 */
	SOFTWARE( mickmack,   0,        1993, "Virgin Interactive", "Mick & Mack as the Global Gladiators (Euro)", 0, 0 )	/* Id: 27062-50 */
	SOFTWARE( mickeyuc,   0,        1998, "Tec Toy", "Mickey's Ultimate Challenge (Brazil)", 0, 0 )	/* Id: 028.700 */
	SOFTWARE( microm,     0,        1994, "Codemasters", "Micro Machines (Euro)", 0, 0 )	/* Id: 19001 */
	SOFTWARE( miracle,    0,        1987, "Sega", "Miracle Warriors - Seal of the Dark Lord (Euro, USA)", 0, 0 )
	SOFTWARE( miracleb,   miracle,  1987, "Sega", "Miracle Warriors - Seal of the Dark Lord (Euro, USA, Prototype)", 0, 0 )
	SOFTWARE( hajafuin,   miracle,  1987, "Sega", "Haja no Fuuin (Jpn)", 0, 0 )	/* Id: G-1331, 7500 - Releases: 1987-10-18 (JPN) - Notes: FM support */
	SOFTWARE( missil3d,   0,        19??, "Sega", "Missile Defense 3-D (Euro, USA)", 0, 0 )	/* Id: 8001 */
	SOFTWARE( monica,     0,        19??, "Tec Toy", "Monica no Castelo do Dragao (Brazil)", 0, 0 )
	SOFTWARE( monopolyu,  monopoly, 1988, "Sega", "Monopoly (USA)", 0, 0 )	/* Id: 5500 */
	SOFTWARE( monopoly,   0,        1988, "Sega", "Monopoly (Euro, USA)", 0, 0 )
	SOFTWARE( montezum,   0,        1989, "Parker Brothers", "Montezuma's Revenge Featuring Panama Joe (USA)", 0, 0 )	/* Id: 4370 */
	SOFTWARE( montezump,  montezum, 1989, "Parker Brothers", "Montezuma's Revenge Featuring Panama Joe (USA, Prototype)", 0, 0 )
	SOFTWARE( mk,         0,        1993, "Arena", "Mortal Kombat (Euro)", 0, 0 )	/* Id: 29021-50 */
	SOFTWARE( mk3b,       0,        1996, "Tec Toy", "Mortal Kombat 3 (Brazil)", 0, 0 )	/* Id: 028730 */
	SOFTWARE( mk2,        0,        1994, "Acclaim Entertainment", "Mortal Kombat II (Euro)", 0, 0 )	/* Id: 29029-50 */
	SOFTWARE( mspacman,   0,        1991, "Tengen", "Ms. Pac-Man (Euro)", 0, 0 )	/* Id: 301030-0160 */
	SOFTWARE( myhero,     0,        1986, "Sega", "My Hero (Euro, USA)", 0, 0 )
	SOFTWARE( seishun,    myhero,   1986, "Sega", "Seishun Scandal (Jpn)", 0, 0 )	/* Id: C-510, 4001 (card), 4501 (cart) - Releases: 1986-01-31 (Jpn, card) */
	SOFTWARE( seishun1,   myhero,   1986, "<pirate>", "Seishyun Scandal (Jpn, Pirate)", 0, 0 )
	SOFTWARE( nekkyu,     0,        1988, "Sega", "Nekkyuu Koushien (Jpn)", 0, 0 )	/* Id: G-1367 - Releases: 1988-09-09 (JPN) - Notes: FM support */
	SOFTWARE( tnzs  ,     0,        1992, "TecMagik", "The New Zealand Story (Euro)", 0, 0 )	/* Id: 27035-50 */
	SOFTWARE( ninjagb,    ninjag,   1992, "Sega", "Ninja Gaiden (Euro, Prototype)", 0, 0 )
	SOFTWARE( ninjag,     0,        1992, "Sega", "Ninja Gaiden (Euro)", 0, 0 )	/* Id: 7101 */
	SOFTWARE( ninja,      0,        1986, "Sega", "The Ninja (Euro, USA)", 0, 0 )
	SOFTWARE( ninjaj,     ninja,    1986, "Sega", "The Ninja (Jpn)", 0, 0 )	/* Id: G-1308, 5066 - Releases: 1986-11-08 (JPN) */
	SOFTWARE( olympi,     0,        1992, "U.S. Gold", "Olympic Gold (Euro)", 0, 0 )	/* Id: 27030-50 */
	SOFTWARE( olympik,    olympi,   1992, "U.S. Gold", "Olympic Gold (Korea)", 0, 0 )
	SOFTWARE( opwolf,     0,        1990, "Sega", "Operation Wolf (Euro)", 0, 0 )	/* Id: 7039 */
	SOFTWARE( ottifant,   0,        1993, "Sega", "The Ottifants (Euro)", 0, 0 )	/* Id: 7120 */
	SOFTWARE( outrun,     0,        1987, "Sega", "Out Run (World)", 0, 0 )	/* Id: G-1326, 7003 - Releases: 1987-06-30 (JPN) - Notes: FM support */
	SOFTWARE( outrun3d,   0,        1991, "Sega", "Out Run 3-D (Euro)", 0, 0 )	/* Id: 8007 */
	SOFTWARE( outruneu,   0,        1991, "U.S. Gold", "Out Run Europa (Euro)", 0, 0 )	/* Id: 27016-50 */
	SOFTWARE( pacmania,   0,        1991, "Tengen", "Pac-Mania (Euro)", 0, 0 )	/* Id: 25010-50 */
	SOFTWARE( paperboy,   0,        1990, "U.S. Gold", "Paperboy (Euro)", 0, 0 )	/* Id: 5121 */
	SOFTWARE( paperboyu,  0,        1990, "Sega", "Paperboy (USA)", 0, 0 )
	SOFTWARE( parlour,    0,        1987, "Sega", "Parlour Games (Euro, USA)", 0, 0 )
	SOFTWARE( family,     parlour,  1987, "Sega", "Family Games (Jpn)", 0, 0 )	/* Id: G-1342, 5103 - Releases: 1987-12-27 (JPN) - Notes: FM support */
	SOFTWARE( patriley,   0,        19??, "Sega", "Pat Riley Basketball (USA, Prototype)", 0, 0 )
	SOFTWARE( penguin,    0,        1987, "Sega", "Penguin Land (Euro, USA)", 0, 0 )	/* Id: 5501 */
	SOFTWARE( dokidoki,   penguin,  1987, "Sega", "Doki Doki Penguin Land - Uchuu Daibouken (Jpn)", 0, 0 )	/* Id: G-1334 - Releases: 1987-08-18 (JPN) - Notes: FM support */
	SOFTWARE( pgatour,    0,        1991, "Tengen", "PGA Tour Golf (Euro)", 0, 0 )	/* Id: 27070-50 */
	SOFTWARE( pstarb,     pstar,    1987, "Tec Toy", "Phantasy Star (Brazil)", 0, 0 )
	SOFTWARE( pstarj1,    pstar,    1987, "Sega", "Phantasy Star (Jpn, Saturn Collection CD)", 0, 0 )
	SOFTWARE( pstarj,     pstar,    1987, "Sega", "Phantasy Star (Jpn)", 0, 0 )	/* Id: G-1341, 9500 - Releases: 1987-12-20 (JPN) - Notes: FM support */
	SOFTWARE( pstark,     pstar,    1987, "Sega", "Phantasy Star (Korea)", 0, 0 )
	SOFTWARE( pstar1,     pstar,    1987, "Sega", "Phantasy Star (Euro, USA, v1.2)", 0, 0 )
	SOFTWARE( pstar,      0,        1987, "Sega", "Phantasy Star (Euro, USA, v1.3)", 0, 0 )
	SOFTWARE( pitfightb,  pitfight, 1991, "Tec Toy", "Pit Fighter (Brazil)", 0, 0 )
	SOFTWARE( pitfight,   0,        1991, "Domark", "Pit Fighter (Euro)", 0, 0 )	/* Id: 29009-50 */
	SOFTWARE( populous,   0,        1989, "TecMagik", "Populous (Euro)", 0, 0 )	/* Id: 27014-50 */
	SOFTWARE( poseidon,   0,        1988, "Sega", "Poseidon Wars 3-D (Euro, USA)", 0, 0 )	/* Id: 8006 */
	SOFTWARE( powstrik,   0,        1988, "Sega", "Power Strike (Euro, USA)", 0, 0 )
	SOFTWARE( aleste,     powstrik, 1988, "Sega", "Aleste (Jpn)", 0, 0 )	/* Id: G-1352, 5109 - Releases: 1988-02-29 (JPN) - Notes: FM support */
	SOFTWARE( powstrk2,   0,        1993, "Sega", "Power Strike II (Euro)", 0, 0 )	/* Id: 9024 */
	SOFTWARE( predator,   0,        1992, "Arena", "Predator 2 (Euro)", 0, 0 )	/* Id: 27026-50 */
	SOFTWARE( ppersia,    0,        1992, "Domark", "Prince of Persia (Euro)", 0, 0 )	/* Id: 27022-50 */
	SOFTWARE( prowrest,   0,        1986, "Sega", "Pro Wrestling (Euro, USA)", 0, 0 )
	SOFTWARE( dumpmats,   prowrest, 1986, "Sega", "Gokuaku Doumei Dump Matsumoto (Jpn)", 0, 0 )	/* Id: G-1302, 5056 - Releases: 1986-07-20 (JPN) */
	SOFTWARE( proyakyu,   0,        1987, "Sega", "The Pro Yakyuu Pennant Race (Jpn)", 0, 0 )	/* Id: G-1323 - Releases: 1987-08-17 (JPN) */
	SOFTWARE( sms3samp,   0,        19??, "Tec Toy", "Promocao Especial M. System III Compact (Brazil, Sample)", 0, 0 )
	SOFTWARE( psychic,    0,        1991, "Sega", "Psychic World (Euro)", 0, 0 )	/* Id: 7066 */
	SOFTWARE( psychof,    0,        1989, "Sega", "Psycho Fox (Euro, USA)", 0, 0 )	/* Id: 7032 */
	SOFTWARE( sapomestr,  psychof,  1995, "Tec Toy", "Sapo Xulé - O Mestre do Kung Fu (Brazil)", 0, 0 )
	SOFTWARE( puttputtb,  puttputt, 1992, "Sega", "Putt & Putter (Euro, Prototype)", 0, 0 )
	SOFTWARE( puttputt,   0,        1992, "Sega", "Putt & Putter (Euro)", 0, 0 )	/* Id: 5122 */
	SOFTWARE( quartet,    0,        1987, "Sega", "Quartet (Euro, USA)", 0, 0 )	/* Id: 5073 */
	SOFTWARE( questfsy,   0,        1993, "Tec Toy", "Quest for the Shaven Yak Starring Ren Hoek & Stimpy (Brazil)", 0, 0 )	/* Id: 028.540 */
	SOFTWARE( rtype,      0,        1988, "Sega", "R-Type (World)", 0, 0 )	/* Id: G-1364, 9002 - Releases: 1988-10-01 (JPN) - Notes: FM support */
	SOFTWARE( rcgp,       0,        1989, "Sega", "R.C. Grand Prix (Euro, USA)", 0, 0 )	/* Id: 27007 */
	SOFTWARE( rainbowb,   rainbow,  1993, "Tec Toy", "Rainbow Islands - The Story of Bubble Bobble 2 (Brazil)", 0, 0 )
	SOFTWARE( rainbow,    0,        1993, "Sega", "Rainbow Islands - The Story of Bubble Bobble 2 (Euro)", 0, 0 )	/* Id: 7117 */
	SOFTWARE( rambo3,     0,        1988, "Sega", "Rambo III (Euro, USA)", 0, 0 )	/* Id: 7015 */
	SOFTWARE( rampage,    0,        1988, "Sega", "Rampage (Euro, USA)", 0, 0 )	/* Id: 27005, QC-001 (USA) */
	SOFTWARE( rampart,    0,        1991, "Tengen", "Rampart (Euro)", 0, 0 )	/* Id: 301026-0160 */
	SOFTWARE( rastan,     0,        1988, "Sega", "Rastan (Euro, USA)", 0, 0 )	/* Id: 7022 */
	SOFTWARE( regjacks,   0,        1988, "Sega", "Reggie Jackson Baseball (USA)", 0, 0 )	/* Id: 7019 */
	SOFTWARE( renegade,   0,        1993, "Sega", "Renegade (Euro)", 0, 0 )	/* Id: 7116 */
	SOFTWARE( rescuems,   0,        1988, "Sega", "Rescue Mission (Euro, USA)", 0, 0 )	/* Id: 5106 */
	SOFTWARE( roadrash,   0,        1993, "U.S. Gold", "Road Rash (Euro)", 0, 0 )	/* Id: 29012-50 */
	SOFTWARE( robocop,    0,        1993, "Flying Edge", "RoboCop 3 (Euro)", 0, 0 )	/* Id: 27064-50 */
	SOFTWARE( roboterm,   0,        1993, "Virgin Interactive", "RoboCop versus The Terminator (Euro)", 0, 0 )	/* Id: 29022-50 */
	SOFTWARE( rocky,      0,        1987, "Sega", "Rocky (World)", 0, 0 )	/* Id: G-1319, 7002 - Releases: 1987-04-19 (JPN) */
	SOFTWARE( running,    0,        1991, "Sega", "Running Battle (Euro)", 0, 0 )	/* Id: 7037 */
	SOFTWARE( sagaia,     0,        1992, "Sega", "Sagaia (Euro)", 0, 0 )	/* Id: 7078 */
	SOFTWARE( sangok3,    0,        1994, "Game Line", "Sangokushi 3 (Korea)", 0, 0 )
	SOFTWARE( sapoxu,     0,        1995, "Tec Toy", "Sapo Xulé vs. Os Invasores do Brejo (Brazil)", 0, 0 )
	SOFTWARE( satell7a,   satell7,  1985, "<pirate>", "Satellite 7 (Jpn, Pirate)", 0, 0 )
	SOFTWARE( satell7,    0,        1985, "Sega", "Satellite 7 (Jpn)", 0, 0 )	/* Id: C-506 - Releases: 1985-12-20 (Jpn, card) */
	SOFTWARE( schtroumb,  schtroum, 1996, "Infogrames", "Les Schtroumpfs Autour du Monde (Euro, Prototype)", 0, 0 )
	SOFTWARE( schtroum,   0,        1996, "Infogrames", "Les Schtroumpfs Autour du Monde (Euro)", 0, 0 )	/* Id: 27085-50 */
	SOFTWARE( scrambsp,   0,        1989, "Sega", "Scramble Spirits (Euro)", 0, 0 )	/* Id: 7031 */
	SOFTWARE( secret,     0,        1986, "Sega", "Secret Command (Euro)", 0, 0 )
	SOFTWARE( ashura,     secret,   1986, "Sega", "Ashura (Jpn)", 0, 0 )	/* Id: G-1307, 5064 (USA), 5081 (EUR) - Releases: 1986-11-16 (JPN) */
	SOFTWARE( rambo2,     secret,   1986, "Sega", "Rambo - First Blood Part II (USA)", 0, 0 )
	SOFTWARE( segachss,   0,        1991, "Sega", "Sega Chess (Euro)", 0, 0 )	/* Id: 7069 */
	SOFTWARE( segawtg,    0,        1993, "Sega", "Sega World Tournament Golf (Euro)", 0, 0 )	/* Id: 7103 */
	SOFTWARE( sensible,   0,        1993, "Sony Imagesoft", "Sensible Soccer (Euro)", 0, 0 )	/* Id: 8011 */
	SOFTWARE( shadow,     0,        1991, "Sega", "Shadow Dancer (Euro)", 0, 0 )	/* Id: 9009 */
	SOFTWARE( beast,      0,        1991, "TecMagik", "Shadow of the Beast (Euro)", 0, 0 )	/* Id: 27019-50 */
	SOFTWARE( shanghb,    shangh,   1988, "Sega", "Shanghai (Euro, USA, Prototype)", 0, 0 )
	SOFTWARE( shangh,     0,        1988, "Sega", "Shanghai (Euro, USA)", 0, 0 )	/* Id: 5110 */
	SOFTWARE( shinobij,   0,        1988, "Sega", "Shinobi (Jpn)", 0, 0 )	/* Id: G-1353 - Releases: 1988-06-19 (JPN) - Notes: FM support */
	SOFTWARE( shinobi,    0,        1988, "Sega", "Shinobi (Euro, USA)", 0, 0 )	/* Id: 7009 */
	SOFTWARE( shooting,   0,        1987, "Sega", "Shooting Gallery (Euro, USA)", 0, 0 )	/* Id: 5072 */
	SOFTWARE( bartvssm,   0,        1992, "Flying Edge", "The Simpsons - Bart vs. The Space Mutants (Euro)", 0, 0 )	/* Id: 27031-50 */
	SOFTWARE( bartvswd,   0,        1993, "Flying Edge", "The Simpsons - Bart vs. The World (Euro)", 0, 0 )	/* Id: 27053-50 */
	SOFTWARE( sitio,      0,        1997, "Tec Toy", "Sitio do Picapau Amarelo (Brazil)", 0, 0 )
	SOFTWARE( slapshot,   0,        1990, "Sega", "Slap Shot (Euro)", 0, 0 )	/* Id: 7035 */
	SOFTWARE( slapshotu,  slapshot, 1990, "Sega", "Slap Shot (USA, v1.1)", 0, 0 )
	SOFTWARE( slapshotu1, slapshot, 1990, "Sega", "Slap Shot (USA)", 0, 0 )
	SOFTWARE( smurfs,     0,        1994, "Infogrames", "The Smurfs (Euro)", 0, 0 )	/* Id: 27082-50 */
	SOFTWARE( solomon,    0,        1988, "Salio", "Solomon no Kagi - Oujo Rihita no Namida (Jpn)", 0, 0 )	/* 002 - Releases: 1988-04-17 (JPN) - Notes: FM support */
	SOFTWARE( sonicbl,    0,        1997, "Tec Toy", "Sonic Blast (Brazil)", 0, 0 )	/* Id: 030.030 */
	SOFTWARE( sonic,      0,        1991, "Sega", "Sonic The Hedgehog (Euro, USA)", 0, 0 )	/* Id: 7076 */
	SOFTWARE( sonic2,     0,        1992, "Sega", "Sonic The Hedgehog 2 (Euro, v1.1)", 0, 0 )	/* Id: 9015 */
	SOFTWARE( sonic2a,    sonic2,   1992, "Sega", "Sonic The Hedgehog 2 (Euro)", 0, 0 )
	SOFTWARE( sonicc,     0,        1993, "Sega", "Sonic The Hedgehog Chaos (Euro)", 0, 0 )	/* Id: 9021 */
	SOFTWARE( sonicsp,    0,        1994, "Sega", "Sonic The Hedgehog Spinball (Euro)", 0, 0 )	/* Id: 9034 */
	SOFTWARE( sonicedu,   0,        19??, "Sega", "Sonic's Edusoft (Prototype)", 0, 0 )
	SOFTWARE( spacegun,   0,        1992, "Sega", "Space Gun (Euro)", 0, 0 )	/* Id: 9010 */
	SOFTWARE( sharrj,     sharr,    1986, "Sega", "Space Harrier (Jpn)", 0, 0 )	/* Id: G-1310, 7001, 7080 - Releases: 1986-12-21 (JPN) */
	SOFTWARE( sharr,      0,        1986, "Sega", "Space Harrier (Euro, USA)", 0, 0 )
	SOFTWARE( sharr3d,    0,        1988, "Sega", "Space Harrier 3-D (Euro, USA)", 0, 0 )
	SOFTWARE( sharr3dj,   sharr3d,  1988, "Sega", "Space Harrier 3D (Jpn)", 0, 0 )	/* Id: G-1349, 8004 - Releases: 1988-02-29 (JPN) - Notes: FM support, 3D glasses support  */
	SOFTWARE( scib,       sci,      1992, "Sega", "Special Criminal Investigation (Euro, Prototype)", 0, 0 )
	SOFTWARE( sci,        0,        1992, "Sega", "Special Criminal Investigation (Euro)", 0, 0 )	/* Id: 7079 */
	SOFTWARE( speedbl,    0,        1990, "Image Works", "Speedball (Mirrorsoft) (Euro)", 0, 0 )	/* Id: 25009-50 */
	SOFTWARE( speedblv,   0,        1992, "Virgin Interactive", "Speedball (Virgin) (Euro, USA)", 0, 0 )	/* Id: 25013-50 */
	SOFTWARE( speedbl2,   0,        1992, "Virgin Interactive", "Speedball 2 (Euro)", 0, 0 )
	SOFTWARE( spellcst,   0,        1988, "Sega", "SpellCaster (Euro, USA)", 0, 0 )
	SOFTWARE( kujaku,     spellcst, 1988, "Sega", "Kujaku Ou (Jpn)", 0, 0 )	/* Id: G-1366, 9003 - Releases: 1988-09-23 (JPN) - Notes: FM support */
	SOFTWARE( spidermn,   0,        1992, "Flying Edge", "Spider-Man - Return of the Sinister Six (Euro)", 0, 0 )	/* Id: 27055-50 */
	SOFTWARE( spidking,   0,        1990, "Sega", "Spider-Man vs. The Kingpin (Euro, USA)", 0, 0 )	/* Id: 7065 */
	SOFTWARE( sportsft,   0,        1987, "Sega", "Sports Pad Football (USA)", 0, 0 )	/* Id: 5060 */
	SOFTWARE( sportssc,   0,        1988, "Sega", "Sports Pad Soccer (Jpn)", 0, 0 )	/* Id: G-1365 - Releases: 1988-10-29 (JPN) - Notes: Sports Pad support */
	SOFTWARE( spyvsspyj,  spyvsspy, 1986, "Sega", "Spy vs Spy (Jpn)", 0, 0 )	/* Id: C-514, 4083 (Card), 4583 (Cart) - Releases: 1986-09-20 (Jpn, card) */
	SOFTWARE( spyvsspyu,  spyvsspy, 1986, "Sega", "Spy vs. Spy (USA, Display Unit Sample)", 0, 0 )
	SOFTWARE( spyvsspy,   0,        1986, "Sega", "Spy vs. Spy (Euro, USA)", 0, 0 )
	SOFTWARE( starwars,   0,        1993, "U.S. Gold", "Star Wars (Euro)", 0, 0 )	/* Id: 29014-50 */
	SOFTWARE( sf2,        0,        1997, "Tec Toy", "Street Fighter II (Brazil)", 0, 0 )	/* Id: 030.010 */
	SOFTWARE( sor,        0,        1993, "Sega", "Streets of Rage (Euro)", 0, 0 )	/* Id: 9019 */
	SOFTWARE( sor2,       0,        1993, "Sega", "Streets of Rage II (Euro)", 0, 0 )	/* Id: 9026 */
	SOFTWARE( strider,    0,        1993, "Sega", "Strider (Euro, USA)", 0, 0 )	/* Id: 9005 */
	SOFTWARE( strider2,   0,        1992, "U.S. Gold", "Strider II (Euro)", 0, 0 )	/* Id: 29005-50 */
	SOFTWARE( submarin,   0,        1990, "Sega", "Submarine Attack (Euro)", 0, 0 )	/* Id: 7048 */
	SOFTWARE( sukeban,    0,        1987, "Sega", "Sukeban Deka II - Shoujo Tekkamen Densetsu (Jpn)", 0, 0 )	/* Id: G-1318 - Releases: 1987-04-19 (JPN) */
	SOFTWARE( summergb,   summerg,  1991, "Sega", "Summer Games (Euro, Prototype)", 0, 0 )
	SOFTWARE( summerg,    0,        1991, "Sega", "Summer Games (Euro)", 0, 0 )	/* Id: 5119 */
	SOFTWARE( suprbskt,   0,        19??, "<unknown>", "Super Basketball (USA, Sample)", 0, 0 )
	SOFTWARE( suprkick,   0,        1991, "U.S. Gold", "Super Kick Off (Euro)", 0, 0 )	/* Id: 27017-50 */
	SOFTWARE( smgp,       0,        1990, "Sega", "Super Monaco GP (Euro)", 0, 0 )	/* Id: 7043 */
	SOFTWARE( smgpu,      smgp,     1990, "Sega", "Super Monaco GP (USA)", 0, 0 )
	SOFTWARE( suhocheo,   0,        19??, "<unknown>", "Suho Cheonsha (Korea)", 0, 0 )
	SOFTWARE( superoff,   0,        1992, "Virgin Interactive", "Super Off Road (Euro)", 0, 0 )	/* Id: 27059-50 */
	SOFTWARE( superrac,   0,        1988, "Sega", "Super Racing (Jpn)", 0, 0 )	/* Id: G-1357 - Releases: 1988-07-02 (JPN) - Notes: FM support */
	SOFTWARE( ssmashtv,   0,        1992, "Flying Edge", "Super Smash T.V. (Euro)", 0, 0 )	/* Id: 27044-50 */
	SOFTWARE( ssinv,      0,        1991, "Domark", "Super Space Invaders (Euro)", 0, 0 )	/* Id: 27023-50 */
	SOFTWARE( superten,   0,        1985, "Sega", "Super Tennis (Euro, USA)", 0, 0 )	/* Id: 4507 */
	SOFTWARE( superman,   0,        1993, "Virgin Interactive", "Superman - The Man of Steel (Euro)", 0, 0 )	/* Id: 27050-50 */
	SOFTWARE( t2ag,       0,        1993, "Arena", "T2 - The Arcade Game (Euro)", 0, 0 )	/* Id: 27061-50 */
	SOFTWARE( chasehq,    0,        1990, "Sega", "Taito Chase H.Q. (Euro)", 0, 0 )	/* Id: 7038 */
	SOFTWARE( tazmars,    0,        19??, "Tec Toy", "Taz in Escape from Mars (Brazil)", 0, 0 )	/* Id: 028.620 */
	SOFTWARE( tazmaniab,  tazmania, 1992, "Sega", "Taz-Mania (Euro, Prototype)", 0, 0 )
	SOFTWARE( tazmania,   0,        1992, "Sega", "Taz-Mania (Euro)", 0, 0 )	/* Id: 7111 */
	SOFTWARE( tecmow92,   0,        19??, "Sega", "Tecmo World Cup '92 (Euro, Prototype)", 0, 0 )
	SOFTWARE( tecmow93,   0,        1993, "Sega", "Tecmo World Cup '93 (Euro)", 0, 0 )	/* Id: 7106 */
	SOFTWARE( teddyboy,   0,        1985, "Sega", "Teddy Boy (Euro, USA)", 0, 0 )
	SOFTWARE( teddyboyj1, teddyboy, 1985, "<pirate>", "Teddy Boy Blues (Jpn, Pirate)", 0, 0 )
	SOFTWARE( teddyboyjp, teddyboy, 1985, "Sega", "Teddy Boy Blues (Jpn, Ep-MyCard, Prototype)", 0, 0 )
	SOFTWARE( teddyboyj,  teddyboy, 1985, "Sega", "Teddy Boy Blues (Jpn)", 0, 0 )	/* Id: C-501, 4003 (Card), 4503 (Cart) - Releases: 1985-10-20 (Jpn, card) */
	SOFTWARE( gerald,     teddyboy, 19??, "Tec Toy", "Geraldinho (Brazil)", 0, 0 )
	SOFTWARE( tennis,     0,        1989, "Sega", "Tennis Ace (Euro)", 0, 0 )	/* Id: 7028 */
	SOFTWARE( tensai,     0,        1988, "Sega", "Tensai Bakabon (Jpn)", 0, 0 )	/* Id: G-1355 - Releases: 1988-06-02 (JPN) - Notes: FM support */
	SOFTWARE( term2,      0,        1993, "Flying Edge", "Terminator 2 - Judgment Day (Euro)", 0, 0 )	/* Id: 27052-50 */
	SOFTWARE( termntrb,   0,        1992, "Tec Toy", "The Terminator (Brazil)", 0, 0 )
	SOFTWARE( termntr,    0,        1992, "Virgin Interactive", "The Terminator (Euro)", 0, 0 )	/* Id: 27025-50 */
	SOFTWARE( 3dragon,    0,        19??, "<unknown>", "The Three Dragon Story (Korea)", 0, 0 )
	SOFTWARE( thunderbj,  0,        1988, "Sega", "Thunder Blade (Jpn)", 0, 0 )	/* Id: G-1360, 7011 - Releases: 1988-07-30 (JPN) - Notes: FM support */
	SOFTWARE( thunderb,   0,        1988, "Sega", "Thunder Blade (Euro, USA)", 0, 0 )
	SOFTWARE( timesold,   0,        1989, "Sega", "Time Soldiers (Euro, USA)", 0, 0 )	/* Id: 7024 */
	SOFTWARE( tomjerry,   0,        1992, "Sega", "Tom & Jerry (Euro, Prototype)", 0, 0 )
	SOFTWARE( tomjermv,   0,        1992, "Sega", "Tom and Jerry - The Movie (Euro)", 0, 0 )	/* Id: 7070 */
	SOFTWARE( totowld3,   0,        1993, "Open Corp.", "Toto World 3 (Korea)", 0, 0 )
	SOFTWARE( transbot,   0,        1985, "Sega", "TransBot (Euro, USA)", 0, 0 )
	SOFTWARE( astrof,     transbot, 1985, "Sega", "Astro Flash (Jpn)", 0, 0 )	/* Id: C-503, 4004 (Card), 4504 (Cart) - Releases: 1985-12-22 (Jpn, card) */
	SOFTWARE( astrof1,    transbot, 1985, "<pirate>", "Astro Flash (Jpn, Pirate)", 0, 0 )
	SOFTWARE( treinam,    0,        19??, "Tec Toy", "Treinamento Do Mymo (Brazil)", 0, 0 )
	SOFTWARE( trivial,    0,        1992, "Domark", "Trivial Pursuit - Genus Edition (Euro)", 0, 0 )	/* Id: 29008-50 */
	SOFTWARE( ttoriui,    0,        19??, "<unknown>", "Ttoriui Moheom  (Korea)", 0, 0 )
	SOFTWARE( ultima4b,   0,        1990, "Sega", "Ultima IV - Quest of the Avatar (Euro, Prototype)", 0, 0 )
	SOFTWARE( ultima4,    0,        1990, "Sega", "Ultima IV - Quest of the Avatar (Euro)", 0, 0 )	/* Id: 9501 */
	SOFTWARE( ultimscr,   0,        1993, "Sega", "Ultimate Soccer (Euro)", 0, 0 )	/* Id: 7119 */
	SOFTWARE( vampire,    0,        19??, "<unknown>", "Vampire (Euro, Prototype)", 0, 0 )
	SOFTWARE( vigilant,   0,        1989, "Sega", "Vigilante (Euro, USA)", 0, 0 )	/* Id: 7023 */
	SOFTWARE( vf,         0,        1997, "Tec Toy", "Virtua Fighter Animation (Brazil)", 0, 0 )	/* Id: 030.020 */
	SOFTWARE( waltpay,    0,        1989, "Sega", "Walter Payton Football (USA)", 0, 0 )	/* Id: 7020 */
	SOFTWARE( wanted,     0,        1989, "Sega", "Wanted (Euro, USA)", 0, 0 )	/* Id: 5118 */
	SOFTWARE( wwrldb,     wwrld,    1989, "Tec Toy", "Where in the World is Carmen Sandiego (Brazil)", 0, 0 )
	SOFTWARE( wwrld,      0,        1989, "Parker Brothers", "Where in the World is Carmen Sandiego (USA)", 0, 0 )	/* Id: 4350 */
	SOFTWARE( wimbledn,   0,        1992, "Sega", "Wimbledon (Euro)", 0, 0 )	/* Id: 7100 */
	SOFTWARE( wimbled2,   0,        1993, "Sega", "Wimbledon II (Euro)", 0, 0 )	/* Id: 7115 */
	SOFTWARE( winterolb,  winterol, 1993, "U.S. Gold", "Winter Olympics - Lillehammer '94 (Brazil)", 0, 0 )
	SOFTWARE( winterol,   0,        1993, "U.S. Gold", "Winter Olympics - Lillehammer '94 (Euro)", 0, 0 )	/* Id: 29015-50 */
	SOFTWARE( wolfch,     0,        1993, "Virgin Interactive", "Wolfchild (Euro)", 0, 0 )	/* Id: 27060-50 */
	SOFTWARE( wboy,       0,        1987, "Sega", "Wonder Boy (Euro, USA)", 0, 0 )
	SOFTWARE( wboyj,      wboy,     1987, "Sega", "Super Wonder Boy (Jpn)", 0, 0 )	/* Id: G-1316, 5068 - Releases: 1987-03-22 (JPN) */
	SOFTWARE( wboy3,      0,        1989, "Sega", "Wonder Boy III - The Dragon's Trap (Euro, USA)", 0, 0 )	/* Id: 7026 */
	SOFTWARE( turmamon,   wboy3,    1993, "Tec Toy", "Turma da Monica em O Resgate (Brazil)", 0, 0 )	/* Id: 026.260 */
	SOFTWARE( wboymlnd,   0,        1988, "Sega", "Wonder Boy in Monster Land (Euro, USA, v1.1, Hacked?)", 0, 0 )
	SOFTWARE( wboymlndp,  wboymlnd, 1988, "Sega", "Wonder Boy in Monster Land (Prototype)", 0, 0 )
	SOFTWARE( wboymlndu,  wboymlnd, 1988, "Sega", "Wonder Boy in Monster Land (Euro, USA)", 0, 0 )
	SOFTWARE( wboymlndj,  wboymlnd, 1988, "Sega", "Super Wonder Boy - Monster World (Jpn)", 0, 0 )	/* Id: G-1346, 7007 - Releases: 1988-01-31 (JPN) - Notes: FM support */
	SOFTWARE( wboymwldb,  wboymwld, 1993, "Sega", "Wonder Boy in Monster World (Euro, Prototype)", 0, 0 )
	SOFTWARE( wboymwld,   0,        1993, "Sega", "Wonder Boy in Monster World (Euro)", 0, 0 )	/* Id: 9012 */
	SOFTWARE( woodypop,   0,        1987, "Sega", "Woody Pop - Shinjinrui no Block Kuzushi (Jpn)", 0, 0 )	/* Id: C-519 - Releases: 1987-03-15 (Jpn, card) */
	SOFTWARE( wclead,     0,        1991, "U.S. Gold", "World Class Leader Board (Euro)", 0, 0 )	/* Id: 27015-50 */
	SOFTWARE( wcup90,     0,        1990, "Sega", "World Cup Italia '90 (Euro)", 0, 0 )	/* Id: 5084 */
	SOFTWARE( wcup94,     0,        1992, "Sega", "World Cup USA 94 (Euro)", 0, 0 )	/* Id: 29028-5* */
	SOFTWARE( worldgb,    worldg,   1989, "Sega", "World Games (Euro, Prototype)", 0, 0 )
	SOFTWARE( worldg,     0,        1989, "Sega", "World Games (Euro)", 0, 0 )	/* Id: 5114 */
	SOFTWARE( worldgp,    0,        1986, "Sega", "World Grand Prix (Euro)", 0, 0 )	/* Id: 5080, 5053? */
	SOFTWARE( worldgpu,   worldgp,  1986, "Sega", "World Grand Prix (USA)", 0, 0 )
	SOFTWARE( worldscr,   0,        1987, "Sega", "World Soccer (World)", 0, 0 )	/* Id: G-1327, 5059 - Releases: 1987-07-19 (JPN) */
	SOFTWARE( wwfwre,     0,        1993, "Flying Edge", "WWF Wrestlemania - Steel Cage Challenge (Euro)", 0, 0 )	/* Id: 27054-50 */
	SOFTWARE( xmenmojo,   0,        1996, "Tec Toy", "X-Men - Mojo World (Brazil)", 0, 0 )
	SOFTWARE( xenon2,     0,        1991, "Image Works", "Xenon 2 - Megablast (Image Works) (Euro)", 0, 0 )	/* Id: 27012-50 */
	SOFTWARE( xenon2v,    0,        1991, "Virgin Interactive", "Xenon 2 - Megablast (Virgin) (Euro)", 0, 0 )	/* Id: 27038-50 */
	SOFTWARE( ys,         ysomens,  1988, "Sega", "Ys (Jpn)", 0, 0 )	/* Id: G-1370, 7501 - Releases: 1988-10-15 (JPN) - Notes: FM support */
	SOFTWARE( ysomens,    0,        1988, "Sega", "Ys - The Vanished Omens (Euro, USA)", 0, 0 )
	SOFTWARE( zaxxon3db,  zaxxon3d, 1987, "Sega", "Zaxxon 3-D (World, Prototype)", 0, 0 )
	SOFTWARE( zaxxon3d,   0,        1987, "Sega", "Zaxxon 3-D (World)", 0, 0 )	/* Id: G-1336, 8002 - Releases: 1987-11-07 (JPN) - Notes: FM support, 3D glasses support */
	SOFTWARE( zillion,    0,        1987, "Sega", "Zillion (Euro, v1.2)", 0, 0 )
	SOFTWARE( zillionj,   zillion,  1987, "Sega", "Zillion (Euro) ~ Akai Koudan Zillion (Jpn)", 0, 0 )	/* Id: G-1325, 5075 - Releases: 1987-05-24 (JPN) */
	SOFTWARE( zillionu,   zillion,  1987, "Sega", "Zillion (USA, v1.1)", 0, 0 )
	SOFTWARE( zillion2,   0,        1987, "Sega", "Zillion II - The Tri Formation (Euro, USA) ~ Tri Formation (Jpn)", 0, 0 )	/* Id: G-1344, 5105 - Releases: 1987-12-13 (JPN) - Notes: FM support */
	SOFTWARE( zool,       0,        1993, "Gremlin Interactive", "Zool - Ninja of the 'Nth' Dimension (Euro)", 0, 0 )	/* Id: 27075-50 */


	SOFTWARE( colors,     0,        19??, "<unknown>", "Color & Switch Test (v1.3)", 0, 0 )
SOFTWARE_LIST_END


SOFTWARE_LIST( sms_cart, "Sega Master System cartridges" )
