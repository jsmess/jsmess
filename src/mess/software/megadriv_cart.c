/***************************************************************************

  Sega MegaDrive / Genesis cartridges

***************************************************************************/

#include "driver.h"
#include "softlist.h"
#include "devices/cartslot.h"


/****** --PROPER-- dumps ********/

/* NHL94, Cart manufactured by Electronics Art, Includes ROM, RAM, Battery etc. */
SOFTWARE_START( nhl94 )
	ROM_REGION( 0x400000, CARTRIDGE_REGION_ROM, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "nhl94_hl9402", 0x000000, 0x100000, CRC(acecd225) SHA1(5a11c7e3c925a6e256d2000b292ad7aa530bda0f) )
	/*
    Rom Labeled

    NHL94 HL9402
    9347
    (C)ELECTRONIC ARTS 1993
    ALL RIGHTS RESERVED
    */
SOFTWARE_END

/* James Pond 2 - Robocod, Cart manufactured By Electronic Arts, Electronic Arts (c)1991 on board */
SOFTWARE_START( robocod )
	ROM_REGION( 0x400000, CARTRIDGE_REGION_ROM, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pond_ii_robocod_ro_b02", 0x000000, 0x080000, CRC(c32b5d66) SHA1(20e70c2a8236915a6e4746b6ad1b603563aecf48) )
	/*
    Rom Labeled

    Pond II: Robocod RO B02
    (c)Electronic Arts 1991
    All Rights Reserved
    */
SOFTWARE_END

/* Splatterhouse Part 2 (Jpn) - proper dump */
SOFTWARE_START( splatt2j )
	ROM_REGION( 0x400000, CARTRIDGE_REGION_ROM, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "sh2j.bin", 0x000000, 0x100000, CRC(adbd991b) SHA1(1a0032faec53f8cf21d3178939bbc5b2f844782a) )
SOFTWARE_END



// work out which sets these relate to
/* Lotus */
SOFTWARE_START( lotust )
	ROM_REGION( 0x400000, CARTRIDGE_REGION_ROM, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "lotus_turbo_lot03", 0x000000, 0x100000, CRC(b83ccb7a) SHA1(922c29fd0195e3e0f72f6fc803d3d5c7299d9f0d) )
SOFTWARE_END

SOFTWARE_START( fifa97 )
	ROM_REGION( 0x400000, CARTRIDGE_REGION_ROM, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "fifa_97_b1", 0x000000, 0x200000, CRC(2bedb061) SHA1(9337ad0318061e70e235a5bfba4504c738d7870c) )
SOFTWARE_END

SOFTWARE_START( riserbot )
	ROM_REGION( 0x400000, CARTRIDGE_REGION_ROM, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "acclaim,es133-1,rise_of,the_robots.u1", 0x000000, 0x200000, CRC(ed583ef7) SHA1(b9f43d5bf31819a1d76c1495e81cfa1d38bcde1c) )
	ROM_LOAD16_WORD_SWAP( "acclaim,es133-2,rise_of,the_robots.u2", 0x200000, 0x100000, CRC(fcf18470) SHA1(09f8ba0b295da42359c354e71b9b7c780a465046) )
SOFTWARE_END

SOFTWARE_START( f22 )
	ROM_REGION( 0x400000, CARTRIDGE_REGION_ROM, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "f-22_intercept_f-202_4.u1", 0x000000, 0x080000, CRC(649771f8) SHA1(6daeca39841f06549373f3a4fd746f3e1c95c328) )
	ROM_LOAD16_BYTE( "f-22_intercept_f-202_1.u2", 0x080001, 0x020000, CRC(d3d7cbb8) SHA1(72e56f858bfe88c2529939304ba49eee3fe14353) )
	// no even rom (it becomes a 0x1a fill in the cart copier dump)
SOFTWARE_END


/********************************************************************

   Dumps below are not confirmed as good

********************************************************************/

/* to sort */

SOFTWARE_START( truc96 )
	ROM_REGION( 0x400000, CARTRIDGE_REGION_ROM, 0 ) /* 68000 Code */
	ROM_LOAD( "truco96.bin", 0x000000, 0x100000, CRC(cddbecfc) SHA1(7a087d08cf7a565c7ad22ca9824971d8cc351ca3) )
SOFTWARE_END

SOFTWARE_START( tc2000 )
	ROM_REGION( 0x400000, CARTRIDGE_REGION_ROM, 0 ) /* 68000 Code */
	ROM_LOAD( "tc2000.bin", 0x000000, 0x100000, CRC(cef91bdb) SHA1(80a2f21304cea6876bc07a48dcd1f5e53a5b3adb) )
SOFTWARE_END


/* shall we emulate these as separate systems? */

SOFTWARE_START( radicav1 )
	ROM_REGION( 0xc00000, CARTRIDGE_REGION_ROM, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "radicav1.bin", 0x000000, 0x400000, CRC(3b4c8438) SHA1(5ed9c053f9ebc8d4bf571d57e562cf347585d158) )
	ROM_RELOAD( 0x400000, 0x400000 ) // for bankswitch
	ROM_RELOAD( 0x800000, 0x400000 ) // allow bank to wrap
SOFTWARE_END

SOFTWARE_START( radicasf )
	ROM_REGION( 0xc00000, CARTRIDGE_REGION_ROM, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "radicasf.bin", 0x000000, 0x400000, CRC(868afb44) SHA1(f4339e36272c18b1d49aa4095127ed18e0961df6) )
	ROM_RELOAD( 0x400000, 0x400000 ) // for bankswitch
	ROM_RELOAD( 0x800000, 0x400000 ) // allow bank to wrap
SOFTWARE_END



#define MEGADRIVE_ROM_LOAD( set, name, offset, length, hash )	\
SOFTWARE_START( set ) \
	ROM_REGION( 0x400000, CARTRIDGE_REGION_ROM, 0 ) \
	ROM_LOAD(name, offset, length, hash) \
SOFTWARE_END


/* These need larger region and start loading at offset = 0x400000 */
#define MEGADRIVE_SSF2_ROM_LOAD( set, name, length, hash )	\
SOFTWARE_START( set ) \
	ROM_REGION( 0x1400000, CARTRIDGE_REGION_ROM, 0 ) \
	ROM_LOAD(name, 0x400000, length, hash) \
SOFTWARE_END

MEGADRIVE_SSF2_ROM_LOAD( ssf2,     "super street fighter ii - the new challengers (europe).bin",                         0x500000,  CRC(682c192f) SHA1(56768a0524fa13fcd76474c8bb89b995bb471847) )
MEGADRIVE_SSF2_ROM_LOAD( ssf2j,    "super street fighter ii - the new challengers (japan).bin",                          0x500000,  CRC(d8eeb2bd) SHA1(24c8634f59a481118f8350125fa6e00d33e04c95) )
MEGADRIVE_SSF2_ROM_LOAD( ssf2u,    "super street fighter ii - the new challengers (usa).bin",                            0x500000,  CRC(165defbf) SHA1(9ce6e69db9d28386f7542dacd3e3ead28eacf2a4) )
MEGADRIVE_SSF2_ROM_LOAD( 12inun,   "12-in-1 (unlicensed).bin",                                                           0x200000,  CRC(a98bf454) SHA1(7313c20071de0ab1cd84ac1352cb0ed1c4a4afa8) )
MEGADRIVE_SSF2_ROM_LOAD( 4in1pb,   "4-in-1 (pirate).bin",                                                                0x200000,  CRC(be72857b) SHA1(2aceca16f13d6a7a6a1bff8543d31bded179df3b) )
MEGADRIVE_SSF2_ROM_LOAD( chinf3,   "chinese fighter iii (unlicensed).bin",                                               0x200000,  CRC(e833bc6e) SHA1(ecca9d2d21c8e27fc7584d53f557fdd8b4cbffa7) )
MEGADRIVE_SSF2_ROM_LOAD( earthdef, "earth defense (usa) (unlicensed).bin",                                               0x80000,   CRC(3519c422) SHA1(9bf4cda850495d7811df578592289018862df575) )
MEGADRIVE_SSF2_ROM_LOAD( funnywld, "funny world & balloon boy (usa) (unlicensed).bin",                                   0x80000,   CRC(a82f05f0) SHA1(17481c8327433bfce8f7bae493fc044194e400a4) )
MEGADRIVE_SSF2_ROM_LOAD( golden10, "golden 10-in-1 (bad dump).bin",                                                      0x100000, BAD_DUMP CRC(4fa3f82b) SHA1(04edbd35fe4916f61b516016b492352d96a8de7f) )
MEGADRIVE_SSF2_ROM_LOAD( lionkin3, "lion king 3 (unlicensed).bin",                                                       0x200000,  CRC(c004219d) SHA1(54ffd355b0805741f58329fa38ed3d9f8f7c80ca) )
MEGADRIVE_SSF2_ROM_LOAD( pokestad, "pokemon stadium (unlicensed).bin",                                                   0x200000,  CRC(fe187c5d) SHA1(f65af5d86aba33bd3f4f91a0cd7428778bcceedf) )
MEGADRIVE_SSF2_ROM_LOAD( s15in1,   "super 15-in-1 (pirate).bin",                                                         0x200000,  CRC(6d17dfff) SHA1(6b2a6de2622735f6d56c6c9c01f74daa90e355cb) )
MEGADRIVE_SSF2_ROM_LOAD( s19in1,   "super 19-in-1 (pirate).bin",                                                         0x400000,  CRC(0ad2b342) SHA1(e1c041ba69da087c428dcda16850159f3caebd4b) )
MEGADRIVE_SSF2_ROM_LOAD( sdkong99, "super donkey kong 99 (unlicensed) (protected).bin",                                  0x300000,  CRC(43be4dd5) SHA1(5d3c84bd18f821b20212941a6f7a1a272eb0d7e3) )
MEGADRIVE_SSF2_ROM_LOAD( skkong99, "super king kong 99 (unlicensed).bin",                                                0x200000,  CRC(413dfee2) SHA1(6973598d77a755beafff31ce85595f9610f8afa5) )
MEGADRIVE_SSF2_ROM_LOAD( redcliff, "the battle of red cliffs - romance of the three kingdoms (unlicensed).bin",          0x200005,  CRC(44463492) SHA1(244334583fde808a56059c0b0eef77742c18274d) )
MEGADRIVE_SSF2_ROM_LOAD( topfight, "top fighter 2000 mk viii (unlicensed).bin",                                          0x200000,  CRC(f75519dc) SHA1(617be8de1444ae0c6610d73967f3f0e67541b05a) )
MEGADRIVE_SSF2_ROM_LOAD( whacacri, "whac-a-critter (usa) (unlicensed).bin",                                              0x80000,   CRC(1bdd02b8) SHA1(4b45801b112a641fee936e41a31728ee7aa2f834) )


MEGADRIVE_ROM_LOAD( 007shitou, "007 shitou - the duel (japan).bin",                                                      0x000000, 0x80000,   CRC(aeb4b262) SHA1(7e0de7011a60a1462dc48594f3caa956ff942281) )
MEGADRIVE_ROM_LOAD( 16ton,     "16 ton (japan) (seganet).bin",                                                           0x000000, 0x40000,   CRC(537f04b6) SHA1(fd978f7f643311de21de17ed7ed1e7c737b518ee) )
MEGADRIVE_ROM_LOAD( 16ton1,    "16 ton (japan) (game no kandume megacd rip).bin",                                        0x000000, 0x40000,   CRC(98149eaf) SHA1(ff564e8db95697103caca81c45efb29f47e7e7e0) )
MEGADRIVE_ROM_LOAD( 16zhan,    "16 zhang ma jiang (china) (unlicensed).bin",                                             0x000000, 0x100000,  CRC(36407c82) SHA1(7857c797245b52641a3d1d4512089bccb0ed5359) )
MEGADRIVE_ROM_LOAD( 2020to,    "2020 toshi super baseball (japan).bin",                                                  0x000000, 0x200000,  CRC(2bbee127) SHA1(3d00178aecd7611c7e4ad09ea07e8436de6a5375) )
MEGADRIVE_ROM_LOAD( 3ninja,    "3 ninjas kick back (usa).bin",                                                           0x000000, 0x200000,  CRC(e5a24999) SHA1(d634e3f04672b8a01c3a6e13f8722d5cbbc6b900) )
MEGADRIVE_ROM_LOAD( 3in1fl,    "3-in-1 flashback - world champ. soccer - tecmo world cup 92 (pirate).bin",               0x000000, 0x200000,  CRC(a8fd28d7) SHA1(a0dd99783667af20589d473a2054d4bbd62d943e) )
MEGADRIVE_ROM_LOAD( 3in1ro,    "3-in-1 road rash - ms. pac-man - block out (pirate).bin",                                0x000000, 0x100000,  CRC(13c96154) SHA1(fd7255c2abdf90698f779a039ea1e560ca19639a) )
MEGADRIVE_ROM_LOAD( 6pakus,    "6-pak (usa).bin",                                                                        0x000000, 0x300000,  CRC(1a6f45dc) SHA1(aa276048f010e94020f0c139dcf5829fb9ea6b21) )
MEGADRIVE_ROM_LOAD( 688atsub,  "mpr-13956.bin",                                                                          0x000000, 0x100000,  CRC(f2c58bf7) SHA1(6795b9fc9a21167d94a0b4c9c38d4e11214e1ea7) )
MEGADRIVE_ROM_LOAD( aresshik,  "a ressha de ikou md (japan).bin",                                                        0x000000, 0x80000,   CRC(3d45de4f) SHA1(d0d0aaad68978fd15c5857773141ae639de1abc7) )
MEGADRIVE_ROM_LOAD( aaharima,  "aa harimanada (japan).bin",                                                              0x000000, 0x200000,  CRC(065f6021) SHA1(fcc4b27aaaea0439fb256f40fb79c823d90a9d88) )
MEGADRIVE_ROM_LOAD( aaahhrm,   "aaahh!!! real monsters (europe).bin",                                                    0x000000, 0x200000,  CRC(7ad115ff) SHA1(9f6361fecbeee1c703b5c988d10a5cb09751ad2a) )
MEGADRIVE_ROM_LOAD( aaahhrmu,  "aaahh!!! real monsters (usa).bin",                                                       0x000000, 0x200000,  CRC(fdc80bfc) SHA1(fa579a7e47ee4a1bdb080bbdec1eef480a85293e) )
MEGADRIVE_ROM_LOAD( action52,  "action 52 (usa) (unlicensed).bin",                                                       0x000000, 0x200000,  CRC(29ff58ae) SHA1(1d5b26a5598eea268d15fa16d43816f8c3e4f8c6) )
MEGADRIVE_ROM_LOAD( action52a, "action 52 (usa) (alt) (unlicensed).bin",                                                 0x000000, 0x200000,  CRC(8809d666) SHA1(fe9936517f45bd0262613ce4422ace873112210a) )
MEGADRIVE_ROM_LOAD( addfamv,   "addams family values (europe) (en,fr,de).bin",                                           0x000000, 0x200000,  CRC(b906b992) SHA1(b7f138e7658a0151ad154ddaed18aea10e114c46) )
MEGADRIVE_ROM_LOAD( addamfam,  "addams family, the (usa, europe).bin",                                                   0x000000, 0x100000,  CRC(71f58614) SHA1(1cd7ac493a448c7486eefde2300240c3675986a9) )
MEGADRIVE_ROM_LOAD( addamfam1, "addams family, the (usa) (beta) (alt).bin",                                              0x000000, 0x100000,  CRC(3a173e1f) SHA1(632b321b860021f8a473fccc7bf8a03bd9c2d581) )
MEGADRIVE_ROM_LOAD( addamfam2, "addams family, the (usa) (beta).bin",                                                    0x000000, 0x100000,  CRC(2803a5ca) SHA1(34deb427637d0458ce229b4fe3e55ad7901a0a34) )
MEGADRIVE_ROM_LOAD( advanc,    "advanced busterhawk gleylancer (japan).bin",                                             0x000000, 0x100000,  CRC(42cf9b5b) SHA1(529d88f96eb7082bfbc00be3f42a1b2e365c34b7) )
MEGADRIVE_ROM_LOAD( advdai,    "mpr-13842a.bin",                                                                         0x000000, 0x100000,  CRC(e0d5e18a) SHA1(842430c0465d810576e75851023929fec873580d) )
MEGADRIVE_ROM_LOAD( advbatr,   "adventures of batman & robin, the (europe).bin",                                         0x000000, 0x200000,  CRC(355e8c39) SHA1(c08c48236c38263df8ea38a5820d16644bddb1a2) )
MEGADRIVE_ROM_LOAD( advbatru,  "adventures of batman & robin, the (usa).bin",                                            0x000000, 0x200000,  CRC(0caaa4ac) SHA1(1da4b3fc92a6572e70d748ebad346aeb291f89f3) )
MEGADRIVE_ROM_LOAD( advbatrp1, "adventures of batman & robin, the (prototype - apr 06, 1995).bin",                       0x000000, 0x200000,  CRC(f05cf389) SHA1(bb7ed12f68a7df4956891338d6db0f29acb0f5df) )
MEGADRIVE_ROM_LOAD( advbatrp2, "adventures of batman & robin, the (prototype - apr 10, 1995).bin",                       0x000000, 0x200000,  CRC(d4f3a070) SHA1(27b06143ca3d31504e75429610acc34be4bbe514) )
MEGADRIVE_ROM_LOAD( advbatrp3, "adventures of batman & robin, the (prototype - apr 11, 1995).bin",                       0x000000, 0x200000,  CRC(b5b5a290) SHA1(901fe36018b031ce6a226795ceb7fd868b28ead6) )
MEGADRIVE_ROM_LOAD( advbatrp4, "adventures of batman & robin, the (prototype - apr 18, 1995).bin",                       0x000000, 0x200000,  CRC(6874142b) SHA1(5cb7d30f81de290da7cd5298a38acccb7ab1f9c7) )
MEGADRIVE_ROM_LOAD( advbatrp5, "adventures of batman & robin, the (prototype - apr 19, 1995).bin",                       0x000000, 0x200000,  CRC(7002da56) SHA1(f76e8669934668abfe739c043a40cbf8ed24e78a) )
MEGADRIVE_ROM_LOAD( advbatrp6, "adventures of batman & robin, the (prototype - apr 21, 1995).bin",                       0x000000, 0x200000,  CRC(61f02f13) SHA1(531973851bb71a589be3544952cf0f7694f8d072) )
MEGADRIVE_ROM_LOAD( advbatrp7, "adventures of batman & robin, the (prototype - apr 22, 1995).bin",                       0x000000, 0x200000,  CRC(eb8236b2) SHA1(97c91de74e744ab0eb52bdcceffb79467a9c5474) )
MEGADRIVE_ROM_LOAD( advbatrp8, "adventures of batman & robin, the (prototype - apr 24, 1995).bin",                       0x000000, 0x200000,  CRC(253947f8) SHA1(1dfd9a4fd06cbe6e28b2519e84b303dbe03a9674) )
MEGADRIVE_ROM_LOAD( advbatrp9, "adventures of batman & robin, the (prototype - apr 26, 1995).bin",                       0x000000, 0x200000,  CRC(bc79f7ee) SHA1(6160fd2b45be920c68b31e2a88c27a967dde6d84) )
MEGADRIVE_ROM_LOAD( advbatp10, "adventures of batman & robin, the (prototype - apr 27, 1995).bin",                       0x000000, 0x200000,  CRC(fdac8190) SHA1(1a2bece0e3f66dcdd44b555ce45deda510b8ca3a) )
MEGADRIVE_ROM_LOAD( advbatp11, "adventures of batman & robin, the (prototype - apr 28, 1995).bin",                       0x000000, 0x200000,  CRC(27a1524b) SHA1(2e41a3b9c64d93de1d05c830f6d8471edfc2090c) )
MEGADRIVE_ROM_LOAD( advbatp12, "adventures of batman & robin, the (prototype - may 01, 1995).bin",                       0x000000, 0x200000,  CRC(589aa203) SHA1(10467cc121afd7d74699b3ab6b7243b370c09e47) )
MEGADRIVE_ROM_LOAD( advemm,    "adventures of mighty max, the (europe).bin",                                             0x000000, 0x100000,  CRC(24f1a3bb) SHA1(e60eec1d39b32ce5cc2125cffd3016b4070a65c3) )
MEGADRIVE_ROM_LOAD( advemmu,   "adventures of mighty max, the (usa).bin",                                                0x000000, 0x100000,  CRC(55f13a00) SHA1(147364ce2de49a85bb64dd7f1075d9687d4fe89e) )
MEGADRIVE_ROM_LOAD( adverb,    "adventures of rocky and bullwinkle and friends, the (usa).bin",                          0x000000, 0x100000,  CRC(ef02d57b) SHA1(a70e83f25ff8ae8b0590920531558faa61738d0f) )
MEGADRIVE_ROM_LOAD( adveboy,   "adventurous boy - mao xian xiao zi (china) (unlicensed).bin",                            0x000000, 0x80000,   CRC(d4403913) SHA1(f6e5bba7254f7f73a007f234f1001ddcb0b62484) )
MEGADRIVE_ROM_LOAD( aerobl,    "aero blasters (japan).bin",                                                              0x000000, 0x80000,   CRC(a00da987) SHA1(64d1964c3b6293f521427d3d83e4defe363cc129) )
MEGADRIVE_ROM_LOAD( aerothb,   "aero the acro-bat (beta).bin",                                                           0x000000, 0x100000,  CRC(dcd14b10) SHA1(105316ad1eb7e3386cbd3ffa2d9d841796193e65) )
MEGADRIVE_ROM_LOAD( aeroth,    "mpr-16603.bin",                                                                          0x000000, 0x100000,  CRC(1a3eaf57) SHA1(0a4c99a31e507bcee5b955e4bf3773f4ded827b6) )
MEGADRIVE_ROM_LOAD( aerothu,   "aero the acro-bat (usa).bin",                                                            0x000000, 0x100000,  CRC(a3a7a8b5) SHA1(438fec09e05337a063f986632b52c9d60dd03ba1) )
MEGADRIVE_ROM_LOAD( aero2,     "mpr-17303.bin",                                                                          0x000000, 0x200000,  CRC(a451f9a1) SHA1(529200d5cea7a5560debd42b547631e7cef38b8b) )
MEGADRIVE_ROM_LOAD( aero2u,    "aero the acro-bat 2 (usa).bin",                                                          0x000000, 0x200000,  CRC(39eb74eb) SHA1(6284a8a6c39137493b1ccc01fcd115cdfb775750) )
MEGADRIVE_ROM_LOAD( aerobi,    "aerobiz (usa).bin",                                                                      0x000000, 0x100000,  CRC(cfaa9bce) SHA1(104ad3c6f5aaba08270faf427991d6a256c6e67c) )
MEGADRIVE_ROM_LOAD( aerobs,    "aerobiz supersonic (usa).bin",                                                           0x000000, 0x100000,  CRC(9377f1b5) SHA1(f29b148c052f50a89e3b877ce027a0d5aaa387d1) )
MEGADRIVE_ROM_LOAD( aftbur2j,  "mpr-12941.bin",                                                                          0x000000, 0x80000,   CRC(4ff37e66) SHA1(8f1e6a3e83094e5d0e237d6a1ffcc87171595ea5) )
MEGADRIVE_ROM_LOAD( aftbur2,   "mpr-13261.bin",                                                                          0x000000, 0x80000,   CRC(ccafe00e) SHA1(523e4abdf19794a167a347b7eeca79907416e084) )
MEGADRIVE_ROM_LOAD( airbus,    "air buster (usa).bin",                                                                   0x000000, 0x80000,   CRC(f3d65baa) SHA1(239636cc38a865359b2deeb5f8dc3fd68da41209) )
MEGADRIVE_ROM_LOAD( airdiverj, "air diver (japan).bin",                                                                  0x000000, 0x80000,   CRC(9e2d5b91) SHA1(540460e95f6a82256ca2a16f347a7b6524f3053f) )
MEGADRIVE_ROM_LOAD( airdiver,  "air diver (usa).bin",                                                                    0x000000, 0x80000,   CRC(2041885e) SHA1(0ea311e3ed667d5d8b7ca4fad66f9d069f0bcb77) )
MEGADRIVE_ROM_LOAD( airman,    "air management - oozora ni kakeru (japan).bin",                                          0x000000, 0x100000,  CRC(b3db0c71) SHA1(b4e5752453cb3e4b3f6cb4fd9ad6473b4b8addd4) )
MEGADRIVE_ROM_LOAD( airmas,    "air management ii - koukuuou o mezase (japan).bin",                                      0x000000, 0x100000,  CRC(4582817b) SHA1(6ed89fc3302023bc95c2bba0d45fe1d30e1c5d86) )
MEGADRIVE_ROM_LOAD( akumaj,    "akumajou dracula - vampire killer (japan).bin",                                          0x000000, 0x100000,  CRC(91b57d2b) SHA1(3e709bd27577056abbbd6735021eaffd90caa140) )
MEGADRIVE_ROM_LOAD( aladdin,   "mpr-15939.bin",                                                                          0x000000, 0x200000,  CRC(d1845e8f) SHA1(42debba01ba3555f61d1e9b445542a05d01451dd) )
MEGADRIVE_ROM_LOAD( aladdinj,  "aladdin (japan).bin",                                                                    0x000000, 0x200000,  CRC(fb5aacf0) SHA1(43753dafd0b816c39aca87fc0788e598fb4bb4f3) )
MEGADRIVE_ROM_LOAD( aladdinub, "aladdin (usa) (beta).bin",                                                               0x000000, 0x200000,  CRC(8c60ef73) SHA1(1f8d4f888b761a878dcc5ffe2dc7c6fef46db1ca) )
MEGADRIVE_ROM_LOAD( aladdinu,  "aladdin (usa).bin",                                                                      0x000000, 0x200000,  CRC(ed427ea9) SHA1(d21c085b8429edc2c5092cd74ef3c36d01bf987f) )
MEGADRIVE_ROM_LOAD( aladdin2,  "aladdin ii (unlicensed).bin",                                                            0x000000, 0x200000,  CRC(be5f9397) SHA1(9980997458dff7897009301a873cf84441f8a01f) )
MEGADRIVE_ROM_LOAD( alexkiddk, "alex kidd - cheongong maseong (korea).bin",                                              0x000000, 0x40000,   CRC(5b0678fb) SHA1(aa9f686045fe83ee5f86640e5a711f80cad16b1e) )
MEGADRIVE_ROM_LOAD( alexkiddj, "alex kidd - tenkuu majou (japan).bin",                                                   0x000000, 0x40000,   CRC(8a5ed856) SHA1(0bd83099edd3938a0f127ed399cb01046e36ac32) )
MEGADRIVE_ROM_LOAD( alexkidd,  "mpr-12608a.bin",                                                                         0x000000, 0x40000,   CRC(778a0f00) SHA1(b7e96b33ab1715fe265ed0de81a26dde969698d5) )
MEGADRIVE_ROM_LOAD( alexkidd1, "alex kidd in the enchanted castle (europe).bin",                                         0x000000, 0x40000,   CRC(c3a52529) SHA1(fd033ad9bd60d57adf52fa8cd0c1a5968d083f46) )
MEGADRIVE_ROM_LOAD( alexkiddu, "alex kidd in the enchanted castle (usa).bin",                                            0x000000, 0x40000,   CRC(47dba0ac) SHA1(bbb204c1eb5ca1b6d180c32722955b2c39d2c17b) )
MEGADRIVE_ROM_LOAD( alien3,    "mpr-15061a.bin",                                                                         0x000000, 0x80000,   CRC(b327fd1b) SHA1(26959752d0683298146c2a89599fa598d009651b) )
MEGADRIVE_ROM_LOAD( alien31,   "mpr-15061.bin",                                                                          0x000000, 0x80000,   CRC(a3b00d6e) SHA1(1f4b969592f98d2692cb06eca550da9c03062593) )
MEGADRIVE_ROM_LOAD( aliens,    "alien soldier (europe).bin",                                                             0x000000, 0x200000,  CRC(0496e06c) SHA1(fa141778bd6540775194d77318f27d2a934e1ac1) )
MEGADRIVE_ROM_LOAD( aliensj,   "alien soldier (japan).bin",                                                              0x000000, 0x200000,  CRC(90fa1539) SHA1(8f6eb584ed9487b8504fbc21d86783f58e6c9cd6) )
MEGADRIVE_ROM_LOAD( alienstm,  "mpr-13941.bin",                                                                          0x000000, 0x80000,   CRC(f5ac8de5) SHA1(e4f8774c5f96db76a781c31476d06203ec16811a) )
MEGADRIVE_ROM_LOAD( alisia,    "alisia dragoon (europe).bin",                                                            0x000000, 0x100000,  CRC(28165bd1) SHA1(c8a3667631fdbd4d0073e42ada9f7199d09c0cfa) )
MEGADRIVE_ROM_LOAD( alisiaj,   "alisia dragoon (japan).bin",                                                             0x000000, 0x100000,  CRC(4d476722) SHA1(04cf02a0004f2eba16af781dd55b946176bb9dbf) )
MEGADRIVE_ROM_LOAD( alisiau,   "alisia dragoon (usa).bin",                                                               0x000000, 0x100000,  CRC(d28d5c40) SHA1(69bf7a9ebcb851e0d571f93b26d785ca87621b01) )
MEGADRIVE_ROM_LOAD( alteredb,  "mpr-12538.bin",                                                                          0x000000, 0x80000,   CRC(154d59bb) SHA1(38945360d824d2fb9535b4fd7f25b9aa9b32f019) )
MEGADRIVE_ROM_LOAD( americ,    "american gladiators (usa).bin",                                                          0x000000, 0x100000,  CRC(9952fa85) SHA1(996581f1b1f7887f3f103ed170ddd9e03ce8c74c) )
MEGADRIVE_ROM_LOAD( andreaga,  "mpr-15488.bin",                                                                          0x000000, 0x80000,   CRC(224256c7) SHA1(70fa6185a4ebbbc9a6b2c7428c489ba5303859b0) )
MEGADRIVE_ROM_LOAD( andreagaub,"andre agassi tennis (usa) (beta).bin",                                                   0x000000, 0x80000,   CRC(3bbf700d) SHA1(f4a05902002273788572fcd49502db44e5dff963) )
MEGADRIVE_ROM_LOAD( andreagau, "andre agassi tennis (usa).bin",                                                          0x000000, 0x80000,   CRC(e755dd51) SHA1(aadf22d36cd745fc35676c07f04df3de579c422c) )
MEGADRIVE_ROM_LOAD( animan,    "fx014a1.bin",                                                                            0x000000, 0x100000,  CRC(92b6f255) SHA1(c474d13afb04bfdb291cfabe43ffc0931be42dbc) )
MEGADRIVE_ROM_LOAD( animanu,   "animaniacs (usa).bin",                                                                   0x000000, 0x100000,  CRC(86224d86) SHA1(7e9d70b5172b4ea9e1b3f5b6009325fa39315a7c) )
MEGADRIVE_ROM_LOAD( anotherw,  "another world (europe).bin",                                                             0x000000, 0x100000,  CRC(e9742041) SHA1(9d98d6817b3e3651837bb2692f7a2a60a608c055) )
MEGADRIVE_ROM_LOAD( aokioo,    "aoki ookami to shiroki meshika - genchou hishi (japan).bin",                             0x000000, 0x140000,  CRC(10be1d93) SHA1(0de0f798f636285da2b4d248f9894bf975b45304) )
MEGADRIVE_ROM_LOAD( aqrenk,    "aq renkan awa (china) (unlicensed).bin",                                                 0x000000, 0x100000,  CRC(2c6cbd77) SHA1(fcd1c7fcfb9027ae7baad9c67847ef5f1090991a) )
MEGADRIVE_ROM_LOAD( aquaticg,  "aquatic games starring james pond and the aquabats, the (usa, europe).bin",              0x000000, 0x80000,   CRC(400f4ba7) SHA1(3bbd0853099f655cd33b52d32811f8ccb64b0418) )
MEGADRIVE_ROM_LOAD( arcadecl,  "mpr-18815.bin",                                                                          0x000000, 0x80000,   CRC(8aed2090) SHA1(ec29aec7848dbcea6678adb4b31deba0a6ecf1e2) )
MEGADRIVE_ROM_LOAD( archriv,   "mpr-14764.bin",                                                                          0x000000, 0x80000,   CRC(e389d7e7) SHA1(2bfbe4698f13ade720dbfe10cebf02fe51e5e6ef) )
MEGADRIVE_ROM_LOAD( arcusj,    "arcus odyssey (japan).bin",                                                              0x000000, 0x100000,  CRC(41c5fb4f) SHA1(8e1a9a9f9ba50bca0a377bd8f795b7c9c1275815) )
MEGADRIVE_ROM_LOAD( arcus,     "arcus odyssey (usa).bin",                                                                0x000000, 0x100000,  CRC(bc4d9b20) SHA1(26feaafad8c464ce3fa96912c2a85c7596e5fa4e) )
MEGADRIVE_ROM_LOAD( arielmer,  "mpr-15153.bin",                                                                          0x000000, 0x80000,   CRC(58e297df) SHA1(201105569535b7c8f11bd97b93cbee884c7845c4) )
MEGADRIVE_ROM_LOAD( arnold,    "mpr-12645.bin",                                                                          0x000000, 0x80000,   CRC(35b995ef) SHA1(76ab194beafcf9e9d5bc40a8e70e2a01d7e42a5b) )
MEGADRIVE_ROM_LOAD( arrow,     "mpr-13396.bin",                                                                          0x000000, 0x80000,   CRC(d49f8444) SHA1(5d2ca55704b7fe8d83fa7564fb1efc62834d3148) )
MEGADRIVE_ROM_LOAD( arrow1,    "arrow flash (world) (alt).bin",                                                          0x000000, 0x80000,   CRC(4d89e66b) SHA1(916524d2b403a633108cf457eec13b4df7384d95) )
MEGADRIVE_ROM_LOAD( artalive,  "mpr-14384.bin",                                                                          0x000000, 0x20000,   CRC(f1b72cdd) SHA1(2c57e38592a206a1847e9e202341595832798587) )
MEGADRIVE_ROM_LOAD( aof,       "art of fighting (europe).bin",                                                           0x000000, 0x200000,  CRC(9970c422) SHA1(a58b1efbbdfa8c4ee6f3d06d474c3771ebe36ca4) )
MEGADRIVE_ROM_LOAD( aofp,      "art of fighting (prototype - jul 11, 1994).bin",                                         0x000000, 0x200000,  CRC(19ce567d) SHA1(221c6ee35e9dfc446f076f84762af153cc3f4208) )
MEGADRIVE_ROM_LOAD( aofu,      "art of fighting (usa).bin",                                                              0x000000, 0x200000,  CRC(c9a57e07) SHA1(7603f33a98994e8145c942e1ed28e6b072332324) )
MEGADRIVE_ROM_LOAD( assaul,    "assault suit leynos (japan).bin",                                                        0x000000, 0x80000,   CRC(81a2c800) SHA1(332886d9fe7823092fed6530781269b60f24d792) )
MEGADRIVE_ROM_LOAD( asterix,   "mpr-15961+mpr-15962.bin",                                                                0x000000, 0x200000,  CRC(4735fee6) SHA1(e6353362ba261f1f8efb31e624250e74e1ca0da1) )
// two chips cart, needs a proper dump

MEGADRIVE_ROM_LOAD( asterixu,  "asterix and the great rescue (usa).bin",                                                 0x000000, 0x200000,  CRC(7f112cd8) SHA1(6d73f37fa31dbcc6a8e9e1590f552e084346088c) )
MEGADRIVE_ROM_LOAD( asterpb,   "asterix and the power of the gods (europe) (beta).bin",                                  0x000000, 0x200000,  CRC(45c8b5b7) SHA1(0905252a58dc5ce0aaa06f1129516fefbda65ddb) )
MEGADRIVE_ROM_LOAD( asterpg,   "mpr-17719.bin",                                                                          0x000000, 0x200000,  CRC(4ff1d83f) SHA1(189c1e7dd280d0d621eb9e895831fb8109e3e3ab) )
MEGADRIVE_ROM_LOAD( atomroboj, "mpr-13483.bin",                                                                          0x000000, 0x80000,   CRC(e833067e) SHA1(9bed099693c27a6575b394bdd150efb7cc53c5c6) )
MEGADRIVE_ROM_LOAD( atomrobo,  "atomic robo-kid (usa).bin",                                                              0x000000, 0x80000,   CRC(7cd8169e) SHA1(38115bc07f11885b4e4c19f92508f729ad6c8765) )
MEGADRIVE_ROM_LOAD( atomrun,   "mpr-15286.bin",                                                                          0x000000, 0x100000,  CRC(b3c05418) SHA1(ab84274bc98a1f8f808bee3f41645884c95cc840) )
MEGADRIVE_ROM_LOAD( atomrunu,  "atomic runner (usa).bin",                                                                0x000000, 0x100000,  CRC(0677c210) SHA1(8a6e868fe36f2e5ac01af2557d3798a892e34799) )
MEGADRIVE_ROM_LOAD( atptour,   "atp tour (europe).bin",                                                                  0x000000, 0x200000,  CRC(1a3da8c5) SHA1(c5fe0fe967369e9d9e855fd3c7826c8f583c49e3) )
MEGADRIVE_ROM_LOAD( atptour1,  "atp tour championship tennis (prototype - aug 02, 1994).bin",                            0x000000, 0x200000,  CRC(686a9fa1) SHA1(3e29c757cedf2334d830f4375242c066f83e0d36) )
MEGADRIVE_ROM_LOAD( atptour2,  "atp tour championship tennis (prototype - aug 05, 1994).bin",                            0x000000, 0x200000,  CRC(a1fef967) SHA1(9fae94e52de4bde42ac45f0e3d3964ccd094b375) )
MEGADRIVE_ROM_LOAD( atptour3,  "atp tour championship tennis (prototype - aug 08, 1994).bin",                            0x000000, 0x200000,  CRC(e6398864) SHA1(2cade1465fd5a835523b688bb675f67a7012e67d) )
MEGADRIVE_ROM_LOAD( atptour4,  "atp tour championship tennis (prototype - jul 19, 1994).bin",                            0x000000, 0x200000,  CRC(cb927488) SHA1(e0e94be5c1f76465151cf6c6357d67ba68170676) )
MEGADRIVE_ROM_LOAD( atptour5,  "atp tour championship tennis (prototype - jul 23, 1994).bin",                            0x000000, 0x200000,  CRC(a15d5051) SHA1(88005e79f325e20c804e04a7a310a6d19b7f7cce) )
MEGADRIVE_ROM_LOAD( atptour6,  "atp tour championship tennis (prototype - jul 25, 1994).bin",                            0x000000, 0x200000,  CRC(a15d5051) SHA1(88005e79f325e20c804e04a7a310a6d19b7f7cce) )
MEGADRIVE_ROM_LOAD( atptour7,  "atp tour championship tennis (prototype - may 09, 1994).bin",                            0x000000, 0x200000,  CRC(b17a8dbc) SHA1(dbaa2f60df5811026539d1f4c6ad50b596b1356a) )
MEGADRIVE_ROM_LOAD( atptour8,  "atp tour championship tennis (prototype - sep 08, 1994).bin",                            0x000000, 0x200000,  CRC(b83f4ca4) SHA1(c2e277d1cf4fa9def71014dc7cf6ebe34d521281) )
MEGADRIVE_ROM_LOAD( atptouru,  "atp tour championship tennis (usa).bin",                                                 0x000000, 0x200000,  CRC(8c822884) SHA1(1ccd027cac63ee56b24a54a84706646d22d0b610) )
MEGADRIVE_ROM_LOAD( austrarl,  "australian rugby league (europe).bin",                                                   0x000000, 0x200000,  CRC(ac5bc26a) SHA1(b54754180f22d52fc56bab3aeb7a1edd64c13fef) )
MEGADRIVE_ROM_LOAD( awesob,    "awesome possum (usa) (beta).bin",                                                        0x000000, 0x200000,  CRC(0158dc53) SHA1(8728f7d18c75e132b98917b96ade5d42c0d4a0cb) )
MEGADRIVE_ROM_LOAD( aweso,     "awesome possum (usa).bin",                                                               0x000000, 0x200000,  CRC(1f07577f) SHA1(a0fe802de7874c95c355ec12b29136651fe0af28) )
MEGADRIVE_ROM_LOAD( aworgj,    "aworg (japan) (seganet).bin",                                                            0x000000, 0x40000,   CRC(069c27c1) SHA1(8316433112fb4b80bbbaf905f93bbfa95f5cb8b2) )
MEGADRIVE_ROM_LOAD( awspro,    "aws pro moves soccer (usa).bin",                                                         0x000000, 0x80000,   CRC(707017e5) SHA1(59b47ce071b38eb740a916c42b256af77e2e7a61) )
MEGADRIVE_ROM_LOAD( assmgpj,   "ayrton senna's super monaco gp ii (japan, europe) (en,ja).bin",                          0x000000, 0x100000,  CRC(60af0f76) SHA1(373fb1744170a114ef99802db987bc9aae009032) )
MEGADRIVE_ROM_LOAD( assmgp,    "ayrton senna's super monaco gp ii (usa) (en,ja).bin",                                    0x000000, 0x100000,  CRC(eac8ded6) SHA1(1ee87744d86c4bdd4958cc70d77538351aa206e6) )
MEGADRIVE_ROM_LOAD( bobb,      "b.o.b. (usa) (beta).bin",                                                                0x000000, 0x100000,  CRC(e3e8421e) SHA1(eb5e4221b13372f2155ac80e88eb1114a6e2ccc7) )
MEGADRIVE_ROM_LOAD( bob,       "b.o.b. (usa, europe).bin",                                                               0x000000, 0x100000,  CRC(eaa2acb7) SHA1(84d63c848d68d844dbcf360040a87a0c93d67f74) )
MEGADRIVE_ROM_LOAD( babyboom,  "baby boom (prototype - aug 11, 1994).bin",                                               0x000000, 0x200000,  CRC(bd697054) SHA1(7baa515001aff4fa93c871189c891e5bd2eaab11) )
MEGADRIVE_ROM_LOAD( babyboom1, "baby boom (prototype - jun 03, 1994).bin",                                               0x000000, 0x200000,  CRC(c0d97f6f) SHA1(24b5a84fb68b89a5ac4e7a9e85af95880067fc5f) )
MEGADRIVE_ROM_LOAD( babyboom2, "baby boom (prototype - jun 06, 1994).bin",                                               0x000000, 0x200000,  CRC(6e0cf48d) SHA1(22c8e6ac65de990a3f30aedf777f7336f7791e66) )
MEGADRIVE_ROM_LOAD( babydo,    "baby's day out (usa) (prototype) (earlier).bin",                                         0x000000, 0x100000,  CRC(459b891c) SHA1(99983282e75e1cf01a47d00f0be15e20eaae0907) )
MEGADRIVE_ROM_LOAD( babyd,     "baby's day out (usa) (prototype).bin",                                                   0x000000, 0x100000,  CRC(b2e7cc49) SHA1(5cc32a2826b3cc5c581043fe6c481ffb753321dd) )
MEGADRIVE_ROM_LOAD( backtof3,  "mpr-14328.bin",                                                                          0x000000, 0x80000,   CRC(2737f92e) SHA1(c808ee7f6f61c096ab73b68dd181e25fdcfde243) )
MEGADRIVE_ROM_LOAD( backtof3u, "back to the future part iii (usa).bin",                                                  0x000000, 0x80000,   CRC(66a388c3) SHA1(a704a1538fb4392e4631e96dce11eeff99d9b04a) )
MEGADRIVE_ROM_LOAD( badomen,   "bad omen (japan, korea).bin",                                                            0x000000, 0x80000,   CRC(975693ce) SHA1(5378af243fb6f592f1e1cd17e3722e2c5e807e72) )
MEGADRIVE_ROM_LOAD( bahamu,    "mpr-13677.bin",                                                                          0x000000, 0x80000,   CRC(b1e268da) SHA1(cee49b613298e060d938de523dfcbb27e790b5af) )
MEGADRIVE_ROM_LOAD( ballja,    "ball jacks (japan, europe).bin",                                                         0x000000, 0x40000,   CRC(f5c3c54f) SHA1(c8aa71c5632a5cc59da430ca3870cffb37fbd30f) )
MEGADRIVE_ROM_LOAD( ballz3d,   "ballz 3d - fighting at its ballziest (usa, europe).bin",                                 0x000000, 0x200000,  CRC(b362b705) SHA1(4825cb9245a701cc59900c57a7a5fa70edc160f0) )
MEGADRIVE_ROM_LOAD( barbiesm,  "barbie super model (usa).bin",                                                           0x000000, 0x100000,  CRC(81c9662b) SHA1(490f4f7a45ca7f3e13069a498218ae8eaa563e85) )
MEGADRIVE_ROM_LOAD( barbvac,   "barbie vacation adventure (usa) (prototype).bin",                                        0x000000, 0x100000,  CRC(10e0ba69) SHA1(51fe102543e0419a9ccd0f3e016150fdb3666c24) )
MEGADRIVE_ROM_LOAD( barkley,   "barkley shut up and jam! (usa, europe).bin",                                             0x000000, 0x100000,  CRC(63fbf497) SHA1(fc60a682412b4f7f851c5eb7f6ae68fcee3d2dd1) )
MEGADRIVE_ROM_LOAD( barkley2b, "barkley shut up and jam! 2 (usa) (beta).bin",                                            0x000000, 0x200000,  CRC(de27357b) SHA1(a9f9fa95106d2e1d00800386ba3d56e1483c83db) )
MEGADRIVE_ROM_LOAD( barkley2,  "barkley shut up and jam! 2 (usa).bin",                                                   0x000000, 0x200000,  CRC(321bb6bd) SHA1(b13f13ccc1a21dacd295f30c66695bf97bbeff8d) )
MEGADRIVE_ROM_LOAD( barney,    "barney's hide & seek game (usa).bin",                                                    0x000000, 0x100000,  CRC(1efa9d53) SHA1(64ebe54459267efaa400d801fc80be2097a6c60f) )
MEGADRIVE_ROM_LOAD( barver,    "barver battle saga - tai kong zhan shi (china) (unlicensed).bin",                        0x000000, 0x200000,  CRC(d37a37c6) SHA1(6d49a0db7687ccad3441f47fbc483c87cd6eab53) )
MEGADRIVE_ROM_LOAD( bassma,    "bass masters classic (usa).bin",                                                         0x000000, 0x200000,  CRC(cf1ff00a) SHA1(62ece2382d37930b84b62e600b1107723fbbc77f) )
MEGADRIVE_ROM_LOAD( bassmp,    "bass masters classic - pro edition (usa).bin",                                           0x000000, 0x200000,  CRC(9eddeb3d) SHA1(9af4f7138d261122dfbc88c8afcbc769ce923816) )
MEGADRIVE_ROM_LOAD( batman,    "mpr-14561.bin",                                                                          0x000000, 0x80000,   CRC(61c60c35) SHA1(c7279c6d45e6533f9de14f65098c289b7534beb3) )
MEGADRIVE_ROM_LOAD( batmanj,   "batman (japan).bin",                                                                     0x000000, 0x80000,   CRC(d7b4febf) SHA1(7f96c72cc7bbdb4a7847d1058f5a751b2dc00ab5) )
MEGADRIVE_ROM_LOAD( batmanu,   "batman (usa).bin",                                                                       0x000000, 0x80000,   CRC(017410ae) SHA1(ffa7ddd8ecf436daecf3628d52dfae6b522ed829) )
MEGADRIVE_ROM_LOAD( batmanrj,  "batman - revenge of the joker (usa).bin",                                                0x000000, 0x100000,  CRC(caa044a1) SHA1(e780c949571e427cf1444e1e35efe33fc9500c81) )
MEGADRIVE_ROM_LOAD( batmanfr,  "batman forever (world).bin",                                                             0x000000, 0x300000,  CRC(8b723d01) SHA1(6e6776b2c9d0f74e8497287b946ead7f630a35ce) )
MEGADRIVE_ROM_LOAD( batmanrt,  "mpr-14998.bin",                                                                          0x000000, 0x100000,  CRC(4a3225c0) SHA1(b173d388485461b9f8b27d299a014d226aef7aa1) )
MEGADRIVE_ROM_LOAD( battlyui,  "battle golfer yui (japan).bin",                                                          0x000000, 0x80000,   CRC(4aa03e4e) SHA1(84db25ff7a0d2db6afddcfe4cdc7c26e20d0dd72) )
MEGADRIVE_ROM_LOAD( battlema,  "battle mania (japan).bin",                                                               0x000000, 0x80000,   CRC(a76c4a29) SHA1(9886dd39cfd8fe505860fc4d0119aacec7484a4e) )
MEGADRIVE_ROM_LOAD( battlemd,  "battle mania daiginjou (japan, korea).bin",                                              0x000000, 0x100000,  CRC(312fa0f2) SHA1(fbcf7e899ff964f52ade160793d6cb22bf68375c) )
MEGADRIVE_ROM_LOAD( battlesq,  "battle squadron (usa, europe).bin",                                                      0x000000, 0x80000,   CRC(0feaa8bf) SHA1(f003f7af0f7edccc317c944b88e57f4c9b66935a) )
MEGADRIVE_ROM_LOAD( battlems,  "battlemaster (usa).bin",                                                                 0x000000, 0x80000,   CRC(fd2b35e3) SHA1(c27ad7070ec068cb2eb13a9e6bdfb3b70e55d4ad) )
MEGADRIVE_ROM_LOAD( battletc,  "battletech - a game of armored combat (usa).bin",                                        0x000000, 0x200000,  CRC(409e5d14) SHA1(ee22ae12053928b652f9b9f513499181b01c8429) )
MEGADRIVE_ROM_LOAD( btoadd,    "battletoads & double dragon (usa).bin",                                                  0x000000, 0x100000,  CRC(8239dd17) SHA1(5b79623e90806206e575b3f15499cab823065783) )
MEGADRIVE_ROM_LOAD( btoads,    "battletoads (world).bin",                                                                0x000000, 0x80000,   CRC(d10e103a) SHA1(5ef3c29b6bdd04d24552ab200d0530f647afdb08) )
MEGADRIVE_ROM_LOAD( beastwj,   "beast warriors (japan).bin",                                                             0x000000, 0x100000,  CRC(4646c694) SHA1(b504a09e3283716f0f9b0464ea9c20cd40110408) )
MEGADRIVE_ROM_LOAD( beastw,    "beast wrestler (usa).bin",                                                               0x000000, 0x100000,  CRC(0ca5bb64) SHA1(b8752043027a6fb9717543b02f42c99759dfd45e) )
MEGADRIVE_ROM_LOAD( beauty,    "beauty and the beast - belle's quest (usa).bin",                                         0x000000, 0x100000,  CRC(befb6fae) SHA1(f88a712ae085ac67f49cb0a8fa16a47e82e780cf) )
MEGADRIVE_ROM_LOAD( beautyrb,  "beauty and the beast - roar of the beast (usa).bin",                                     0x000000, 0x100000,  CRC(13e7b519) SHA1(e98c7b232e213a71f79563cf0617caf0b3699cbf) )
MEGADRIVE_ROM_LOAD( beavis,    "beavis and butt-head (europe).bin",                                                      0x000000, 0x200000,  CRC(c7b6435e) SHA1(0d132afbc76589b95a0c617d39122f0715eab2c6) )
MEGADRIVE_ROM_LOAD( beavisub,  "beavis and butt-head (usa) (beta).bin",                                                  0x000000, 0x200000,  CRC(81ed5335) SHA1(f9ed871867f543a8d94c47270cd530a5ca410664) )
MEGADRIVE_ROM_LOAD( beavisu,   "beavis and butt-head (usa).bin",                                                         0x000000, 0x200000,  CRC(f5d7b948) SHA1(9abfbf8c5d07a11f090a151c2801caee251f1599) )
MEGADRIVE_ROM_LOAD( berens,    "berenstain bears' camping adventure, the (usa).bin",                                     0x000000, 0x100000,  CRC(1f86237b) SHA1(5fd62e60fd3dd19f78d63a93684881c7a81485d6) )
MEGADRIVE_ROM_LOAD( beren1,    "berenstain bears' camping adventure, the (prototype - apr 28, 1994).bin",                0x000000, 0x100000,  CRC(f222e946) SHA1(349fc63222fe59bc8e7e71e6b0358ac7b03a00ca) )
MEGADRIVE_ROM_LOAD( beren2,    "berenstain bears' camping adventure, the (prototype - apr 29, 1994).bin",                0x000000, 0x100000,  CRC(e21493b4) SHA1(4607bf96959809c86c4e5ed244c9d29ad2584637) )
MEGADRIVE_ROM_LOAD( beren3,    "berenstain bears' camping adventure, the (prototype - aug 01, 1994).bin",                0x000000, 0x100000,  CRC(b7cd465c) SHA1(d60e4e0542d1c0a4bdbb7a433373051718549055) )
MEGADRIVE_ROM_LOAD( beren4,    "berenstain bears' camping adventure, the (prototype - aug 02, 1994).bin",                0x000000, 0x100000,  CRC(383ad564) SHA1(faf4801049124ab2f63d514f04adc757a66382b5) )
MEGADRIVE_ROM_LOAD( beren5,    "berenstain bears' camping adventure, the (prototype - aug 03, 1994).bin",                0x000000, 0x100000,  CRC(18f268a6) SHA1(5f496994fa770db96afac4579d7314d56a8f15d5) )
MEGADRIVE_ROM_LOAD( beren6,    "berenstain bears' camping adventure, the (prototype - aug 05, 1994).bin",                0x000000, 0x100000,  CRC(24159b6f) SHA1(1b4e21aeffba372820c25517d98c181731bbd007) )
MEGADRIVE_ROM_LOAD( beren7,    "berenstain bears' camping adventure, the (prototype - aug 08, 1994).bin",                0x000000, 0x100000,  CRC(58b6b0fc) SHA1(ba0e3fc9d04e37a81b37d4e9ff21097bfc4fc0f1) )
MEGADRIVE_ROM_LOAD( beren8,    "berenstain bears' camping adventure, the (prototype - jul 09, 1994).bin",                0x000000, 0x100000,  CRC(bf6aa405) SHA1(8172369e0a73d1be60e1124a2ea8df41cfddfb55) )
MEGADRIVE_ROM_LOAD( beren9,    "berenstain bears' camping adventure, the (prototype - jul 16, 1994).bin",                0x000000, 0x100000,  CRC(c84aad8e) SHA1(758f5e7072d8bc4bd6325fb7629ebb9f11d1ce10) )
MEGADRIVE_ROM_LOAD( bere10,    "berenstain bears' camping adventure, the (prototype - jul 20, 1994).bin",                0x000000, 0x100000,  CRC(abee4c5e) SHA1(ebc3d134494ce02932b44bbda3720176089d7604) )
MEGADRIVE_ROM_LOAD( bere11,    "berenstain bears' camping adventure, the (prototype - jun 02, 1994).bin",                0x000000, 0x100000,  CRC(83cf19e4) SHA1(fe2fa9060a2cf13720c79a5fc8568a33c2adad0a) )
MEGADRIVE_ROM_LOAD( bere12,    "berenstain bears' camping adventure, the (prototype - jun 10, 1994).bin",                0x000000, 0x100000,  CRC(a748e2cf) SHA1(32ee3d750ea8ff06adb28c7b5812670f4e92d952) )
MEGADRIVE_ROM_LOAD( bere13,    "berenstain bears' camping adventure, the (prototype - mar 23, 1994).bin",                0x000000, 0x100000,  CRC(bfbcd7cc) SHA1(2ace984e92740fe93b762c66ee12663c7d8805b9) )
MEGADRIVE_ROM_LOAD( bere14,    "berenstain bears' camping adventure, the (prototype - may 06, 1994).bin",                0x000000, 0x100000,  CRC(713c14d8) SHA1(fedd6c08d4bd7f8f554bcfd1e7431ba9248c1fe7) )
MEGADRIVE_ROM_LOAD( bere15,    "berenstain bears' camping adventure, the (prototype - may 11, 1994).bin",                0x000000, 0x100000,  CRC(f7b81c6a) SHA1(9ab467062576cf087f73bd9ef9ecc428ce638f3a) )
MEGADRIVE_ROM_LOAD( bere16,    "berenstain bears' camping adventure, the (prototype - may 17, 1994).bin",                0x000000, 0x100000,  CRC(e1855ade) SHA1(391c1bdeb7546bc19c3f866cb17079f529044b3c) )
MEGADRIVE_ROM_LOAD( bere17,    "berenstain bears' camping adventure, the (prototype - may 19, 1994).bin",                0x000000, 0x100000,  CRC(07c9f71f) SHA1(617284c4964117e2c7ac9fab8ef4ea19a89fa0ea) )
MEGADRIVE_ROM_LOAD( bere18,    "berenstain bears' camping adventure, the (prototype - may 23, 1994).bin",                0x000000, 0x100000,  CRC(913887b7) SHA1(4b866f0b0f491a3d73d1ce1895df9720c93a79f2) )
MEGADRIVE_ROM_LOAD( bere19,    "berenstain bears' camping adventure, the (prototype - may 26, 1994).bin",                0x000000, 0x100000,  CRC(d33c0fa4) SHA1(4315949e9c7f3c938d0ef15f0ce361e7f47042a1) )
MEGADRIVE_ROM_LOAD( bere20,    "berenstain bears' camping adventure, the (prototype - may 30, 1994).bin",                0x000000, 0x100000,  CRC(9c3bf429) SHA1(33c292b28518f25e1d87c41fff3cf5f2da635993) )
MEGADRIVE_ROM_LOAD( bestof,    "best of the best - championship karate (europe).bin",                                    0x000000, 0x100000,  CRC(f842240b) SHA1(c7ffaa2af35d0365340075555b6a5c3f252d33b3) )
MEGADRIVE_ROM_LOAD( bestofu,   "best of the best - championship karate (usa).bin",                                       0x000000, 0x100000,  CRC(c3d6a5d4) SHA1(1fafc2d289215d0f835fa6c74a8bf68f8d36bcc8) )
MEGADRIVE_ROM_LOAD( beyoasisp, "beyond oasis (prototype - nov 01, 1994).bin",                                            0x000000, 0x300000,  CRC(fa59f847) SHA1(cb0606faeab0398244d4721d71cf7e1c5724a9ef) )
MEGADRIVE_ROM_LOAD( beyoasis,  "beyond oasis (usa).bin",                                                                 0x000000, 0x300000,  CRC(c4728225) SHA1(2944910c07c02eace98c17d78d07bef7859d386a) )
MEGADRIVE_ROM_LOAD( beyondzt,  "beyond zero tolerance (usa) (prototype).bin",                                            0x000000, 0x200000,  CRC(c61ed2ed) SHA1(663a1a6c7e25ba05232e22f102de36c1af2f48f8) )
MEGADRIVE_ROM_LOAD( bibleadv,  "bible adventures (usa) (unlicensed).bin",                                                0x000000, 0x80000,   CRC(64446b77) SHA1(0163b6cd6397ab4c1016f9fcf4f2e6d2bca8454f) )
MEGADRIVE_ROM_LOAD( billwa,    "bwcf01.bin",                                                                             0x000000, 0x100000,  CRC(3ed83362) SHA1(2bbb454900ac99172a2d72d1e6f96a96b8d6840b) )
MEGADRIVE_ROM_LOAD( billwa95,  "bill walsh college football 95 (usa).bin",                                               0x000000, 0x200000,  CRC(a582f45a) SHA1(2ae000f45474b3cdedd08eeca7f5e195959ba689) )
MEGADRIVE_ROM_LOAD( bimini,    "bimini run (usa).bin",                                                                   0x000000, 0x80000,   CRC(d4dc5188) SHA1(62a6a1780f4846dcfefeae11de35a18ec039f424) )
MEGADRIVE_ROM_LOAD( biohzb,    "bio hazard battle (usa) (beta).bin",                                                     0x000000, 0x100000,  CRC(dd10dd1a) SHA1(1445b0babb52d252bf822d8d2eec0eda05b63229) )
MEGADRIVE_ROM_LOAD( biohz,     "mpr-15204.bin",                                                                          0x000000, 0x100000,  CRC(95b0ea2b) SHA1(dca9d505302ce9ff1f98c4da95505139c7d3cafc) )
MEGADRIVE_ROM_LOAD( bishou,    "bishoujo senshi sailor moon (japan).bin",                                                0x000000, 0x200000,  CRC(5e246938) SHA1(7565b0b19fb830ded5e90399f552c14c2aacdeb8) )
MEGADRIVE_ROM_LOAD( blades,    "blades of vengeance (usa, europe).bin",                                                  0x000000, 0x100000,  CRC(74c65a49) SHA1(11f342ec4be17dcf7a4a6a80649b6b5ff19940a5) )
MEGADRIVE_ROM_LOAD( blastb,    "blaster master 2 (usa) (beta).bin",                                                      0x000000, 0x100000,  CRC(08f78c70) SHA1(973cd6b8c7d29c9d14ac45631b99fd4ae4ca5713) )
MEGADRIVE_ROM_LOAD( blast,     "blaster master 2 (usa).bin",                                                             0x000000, 0x100000,  CRC(c11e4ba1) SHA1(20c9b85f543ef8e66a97e6403cf486045f295d48) )
MEGADRIVE_ROM_LOAD( blockb,    "blockbuster world video game championship ii (usa).bin",                                 0x000000, 0x400000,  CRC(4385e366) SHA1(1a06213a3a26c9105fc3013a141c22d212045a0b) )
MEGADRIVE_ROM_LOAD( blocko,    "bloc-u1_blo03.bin+bloc-u2_blo03.bin,mpr-14247.bin",                                      0x000000, 0x20000,   CRC(5e2966f1) SHA1(f6620d3b712f3bd333d0bb355c08cf992af6e12d) )
// Western release contains two chips (blo03.u1, blo03.u2), Japanese release only one (mpr-14247), but aggregated checksum is the same; needs a proper dump

MEGADRIVE_ROM_LOAD( bloods,    "es210.bin",                                                                              0x000000, 0x200000,  CRC(f9f2bceb) SHA1(513005efd123539a905986130d15125085837559) )
MEGADRIVE_ROM_LOAD( bluealma,  "blue almanac (japan).bin",                                                               0x000000, 0x100000,  CRC(7222ebb3) SHA1(0be0d4d3e192beb4106c0a95c1fb0aec952c3917) )
MEGADRIVE_ROM_LOAD( bodycob,   "body count (europe) (en,fr,de,es,it) (beta).bin",                                        0x000000, 0x100000,  CRC(b4ffb6ce) SHA1(a01971cdce4d98e770d9c083a5accb8cb5260112) )
MEGADRIVE_ROM_LOAD( bodyco,    "body count (europe) (en,fr,de,es,it).bin",                                               0x000000, 0x100000,  CRC(3575a030) SHA1(3098f3c7ea9e25bf86219d7ed0795bf363338714) )
MEGADRIVE_ROM_LOAD( bodycop,   "body count (prototype - feb 08, 1994).bin",                                              0x000000, 0xff900,   CRC(c5fed897) SHA1(1c13134c32dc620dd663f33150c82d12cc69d442) )
MEGADRIVE_ROM_LOAD( bodycop1,  "body count (prototype - feb 28, 1994 - u).bin",                                          0x000000, 0xff900,   CRC(c5fed897) SHA1(1c13134c32dc620dd663f33150c82d12cc69d442) )
MEGADRIVE_ROM_LOAD( bodycop2,  "body count (prototype - jan 27, 1994).bin",                                              0x000000, 0x100000,  CRC(66ca4e71) SHA1(8cbea2b2c0435fadc6031a2704d7a60c1a386615) )
MEGADRIVE_ROM_LOAD( bodycop3,  "body count (prototype - mar 03, 1994).bin",                                              0x000000, 0x100000,  CRC(6abc6e77) SHA1(0c50d81ff0630fb79a600f65dfacb894dfe62f4d) )
MEGADRIVE_ROM_LOAD( bodycop4,  "body count (prototype - mar 08, 1994 - a).bin",                                          0x000000, 0x100000,  CRC(8965213f) SHA1(dbd4fec001f61c6a8414b2035e884ebfbf48b899) )
MEGADRIVE_ROM_LOAD( bodycop5,  "body count (prototype - mar 08, 1994).bin",                                              0x000000, 0x100000,  CRC(8965213f) SHA1(dbd4fec001f61c6a8414b2035e884ebfbf48b899) )
MEGADRIVE_ROM_LOAD( bodycop6,  "body count (prototype - mar 09, 1994).bin",                                              0x000000, 0x100000,  CRC(649607d1) SHA1(d881a5281c619d3b80bc740a2783634818a8fc4c) )
MEGADRIVE_ROM_LOAD( bonanz,    "mpr-13905a.bin",                                                                         0x000000, 0x80000,   CRC(c6aac589) SHA1(f7313ecaf7143873597e3a6ebf58d542d06c3ef3) )
MEGADRIVE_ROM_LOAD( bonanz1,   "bonanza bros. (japan, europe).bin",                                                      0x000000, 0x80000,   CRC(adf6476c) SHA1(ce46f2b53988f06e0bdfaad1b0dc3e9ca936e1e8) )
MEGADRIVE_ROM_LOAD( bonanz2,   "bonanza bros. (usa, korea).bin",                                                         0x000000, 0x80000,   CRC(20d1ad4c) SHA1(31c589bc0d1605502cdd04069dc4877811e84e58) )
MEGADRIVE_ROM_LOAD( bonkersp,  "bonkers (prototype - mar 28, 1994).bin",                                                 0x000000, 0x100000,  CRC(cd67c588) SHA1(3f08ea3bc211f4a02d216d87c9abcdab11612ccb) )
MEGADRIVE_ROM_LOAD( bonkersp1, "bonkers (prototype - may 03, 1994).bin",                                                 0x000000, 0x100000,  CRC(e4cd0d61) SHA1(7c4798c73548f6992b18bf2bbd2f8c3144fcff58) )
MEGADRIVE_ROM_LOAD( bonkersp2, "bonkers (prototype - oct 04, 1994).bin",                                                 0x000000, 0x100000,  CRC(913cc834) SHA1(435c0dc4c72184e0b7dd535fe5e7d5323d787571) )
MEGADRIVE_ROM_LOAD( bonkersp3, "bonkers (prototype - oct 25, 1994).bin",                                                 0x000000, 0x100000,  CRC(3e5edc4f) SHA1(a1bfc26fe6ab4fe1e5b8ddbc84b840209d9cb798) )
MEGADRIVE_ROM_LOAD( bonkersp4, "bonkers (prototype - oct 29, 1994).bin",                                                 0x000000, 0x100000,  CRC(7b84793e) SHA1(994a0d7f7c21edbe35e31ca2a28b3e2017683355) )
MEGADRIVE_ROM_LOAD( bonkers,   "bonkers (usa, europe).bin",                                                              0x000000, 0x100000,  CRC(d1e66017) SHA1(938642252fdb1c5aedc785bce2ba383fc683c917) )
MEGADRIVE_ROM_LOAD( booger,    "boogerman - a pick and flick adventure (europe).bin",                                    0x000000, 0x300000,  CRC(dbc4340c) SHA1(4fd33eeaf1e804d005793a6e1185122fd7e9a751) )
MEGADRIVE_ROM_LOAD( boogeru,   "boogerman - a pick and flick adventure (usa).bin",                                       0x000000, 0x300000,  CRC(1a7a2bec) SHA1(bb1d23ca4c48d37bf3170d320cc30bfbc2acdfff) )
MEGADRIVE_ROM_LOAD( boogie,    "boogie woogie bowling (japan).bin",                                                      0x000000, 0x80000,   CRC(ccf52828) SHA1(57599254461bf05ac0822c42e0bb223a68d4cd71) )
MEGADRIVE_ROM_LOAD( boxing,    "boxing legends of the ring (usa).bin",                                                   0x000000, 0x100000,  CRC(00f225ac) SHA1(4004af082d0e917d6b05c137ab5cff55e11c12a9) )
MEGADRIVE_ROM_LOAD( bramst,    "bram stoker's dracula (europe).bin",                                                     0x000000, 0x100000,  CRC(9ba5a063) SHA1(d1e2bb4febf973e3510118d2ea71b4c6594480a9) )
MEGADRIVE_ROM_LOAD( bramstu,   "bram stoker's dracula (usa).bin",                                                        0x000000, 0x100000,  CRC(077084a6) SHA1(4ede0e75054655acab63f2a41b8c57e1cf137e58) )
MEGADRIVE_ROM_LOAD( bretth,    "brett hull hockey '95 (usa).bin",                                                        0x000000, 0x200000,  CRC(f7775a09) SHA1(bcc9d8a737b5b6ccc5ddcd5906202508e4307f79) )
MEGADRIVE_ROM_LOAD( brianl,    "brian lara cricket (europe) (june 1995).bin",                                            0x000000, 0x100000,  CRC(408cf5c3) SHA1(032eb1a15c95540b392b52ee17eef7eee6ac436d) )
MEGADRIVE_ROM_LOAD( brianl1,   "brian lara cricket (europe) (march 1995).bin",                                           0x000000, 0x100000,  CRC(90f5c2b7) SHA1(0f4bec98af08027fc034b9801c28bb299a04be35) )
MEGADRIVE_ROM_LOAD( brianl96,  "brian lara cricket 96 (europe) (april 1996).bin",                                        0x000000, 0x100000,  CRC(fe52f7e1) SHA1(3e1ef39e9008a4a55fd57b25948668c8c52ba9e3) )
MEGADRIVE_ROM_LOAD( brianl96a, "mdbl_9844.bin",                                                                          0x000000, 0x100000,  CRC(fa3024af) SHA1(0b08788a0f8214c5d07b8e2293f0b954dd05bef5) )
MEGADRIVE_ROM_LOAD( brutal,    "brutal - paws of fury (europe).bin",                                                     0x000000, 0x200000,  CRC(7e9a8d32) SHA1(8667fa820e90911f12b682fcd1ac870b84b6b60b) )
MEGADRIVE_ROM_LOAD( brutalu,   "brutal - paws of fury (usa).bin",                                                        0x000000, 0x200000,  CRC(98d502cd) SHA1(51b712e87e8f9cf7a93cfc78ec27d2e80b316ec3) )
MEGADRIVE_ROM_LOAD( bubbab,    "bubba'n'stix (europe) (beta).bin",                                                       0x000000, 0x100000,  CRC(a8731cb4) SHA1(3e10ed88ac29b084d3665579a79c23ff2921b832) )
MEGADRIVE_ROM_LOAD( bubba,     "bubba'n'stix (europe).bin",                                                              0x000000, 0x100000,  CRC(b467432e) SHA1(4039dfb41c08d17047d2acf90d0ab8bb7932cabd) )
MEGADRIVE_ROM_LOAD( bubbau,    "bubba'n'stix - a strategy adventure (usa).bin",                                          0x000000, 0x100000,  CRC(d45cb46f) SHA1(907c875794b8e3836e5811c1f28aa90cc2c8ffed) )
MEGADRIVE_ROM_LOAD( bubble,    "bubble and squeak (europe).bin",                                                         0x000000, 0x80000,   CRC(86151bf1) SHA1(092b21f33991e42f993fb8ee1de1c31553a75f68) )
MEGADRIVE_ROM_LOAD( bubbleu,   "bubble and squeak (usa).bin",                                                            0x000000, 0x80000,   CRC(28c4a006) SHA1(0457b0220c86f58d8249cbd5987f63c5439d460c) )
MEGADRIVE_ROM_LOAD( bubsy2,    "bu2sg.bin",                                                                              0x000000, 0x200000,  CRC(f8beff56) SHA1(0cfb6c619798ba47f35069dea094fbc96f974ecb) )
MEGADRIVE_ROM_LOAD( bubsy,     "bubsy in claws encounters of the furred kind (usa, europe).bin",                         0x000000, 0x200000,  CRC(3e30d365) SHA1(719140754763e5062947ef9e76ee748cfad38202) )
MEGADRIVE_ROM_LOAD( buckro,    "buc05.bin",                                                                              0x000000, 0x100000,  CRC(44e3bfff) SHA1(89c39f00745f2a8798fe985ad8ce28411b977f9e) )
MEGADRIVE_ROM_LOAD( budokan,   "budokan - the martial spirit (europe).bin",                                              0x000000, 0x80000,   CRC(97add5bd) SHA1(004f3d6f333795315a072f3f0661ce4e5e91a4ae) )
MEGADRIVE_ROM_LOAD( budokanu,  "budokan - the martial spirit (usa).bin",                                                 0x000000, 0x80000,   CRC(acd9f5fc) SHA1(93bc8242106bc9b2e0a8a974a3f65b559dd2941d) )
MEGADRIVE_ROM_LOAD( bugslife,  "bug's life, a (unlicensed).bin",                                                         0x000000, 0x100000,  CRC(10458e09) SHA1(b620c2bebd5bab39bc9258a925169b4c93614599) )
MEGADRIVE_ROM_LOAD( bugsbun,   "bugs bunny in double trouble (usa).bin",                                                 0x000000, 0x200000,  CRC(365305a2) SHA1(50b18d9f9935a46854641c9cfc5b3d3b230edd5e) )
MEGADRIVE_ROM_LOAD( bullvsbl,  "bulls versus blazers and the nba playoffs (usa, europe).bin",                            0x000000, 0x100000,  CRC(d4e4b4e8) SHA1(6db65704e2132c2f6e0501e8481abde7f2c7078d) )
MEGADRIVE_ROM_LOAD( bullvsla,  "bulls vs lakers and the nba playoffs (usa, europe).bin",                                 0x000000, 0x100000,  CRC(e56023a0) SHA1(102652dcd218e3420ea9c4116231fa62f8fcd770) )
MEGADRIVE_ROM_LOAD( burninf,   "burning force (europe).bin",                                                             0x000000, 0x80000,   CRC(776ff6ff) SHA1(a25930ee55a2d88838e3999fb5939d9392fd0efa) )
MEGADRIVE_ROM_LOAD( burninfj,  "burning force (japan).bin",                                                              0x000000, 0x80000,   CRC(0c1deb47) SHA1(8849253262f545fbaf6140bfa5ca67a3caac9a80) )
MEGADRIVE_ROM_LOAD( burninfu,  "burning force (usa).bin",                                                                0x000000, 0x80000,   CRC(bdc8f02c) SHA1(28fcb1c9b5c72255443ab5bb950a52030baaf409) )
MEGADRIVE_ROM_LOAD( cadash,    "cadash (usa, asia, korea).bin",                                                          0x000000, 0x80000,   CRC(13bdf374) SHA1(5791cc9a1b118c58fc5209bf2e64156ffdb80134) )
MEGADRIVE_ROM_LOAD( caesar,    "caesar no yabou (japan).bin",                                                            0x000000, 0x100000,  CRC(69796e93) SHA1(4a5b9169262caf81fb7ae76fc3835107769f28a1) )
MEGADRIVE_ROM_LOAD( caesar2,   "caesar no yabou ii (japan).bin",                                                         0x000000, 0x100000,  CRC(4f327b3a) SHA1(cdb2f47bde3ff412c7b1f560637f2ccec023980f) )
MEGADRIVE_ROM_LOAD( caesarpl,  "caesars palace (usa).bin",                                                               0x000000, 0x80000,   CRC(8fdaa9bb) SHA1(83c16e539ce332e1d33ab196b84abc1be4beb982) )
MEGADRIVE_ROM_LOAD( calrip,    "cal ripken jr. baseball (usa).bin",                                                      0x000000, 0x100000,  CRC(9b1c96c0) SHA1(47579111ed8f9d4f4eb48ec700272bc73ee35295) )
MEGADRIVE_ROM_LOAD( calibe50,  "caliber .50 (usa).bin",                                                                  0x000000, 0x100000,  CRC(44f4fa05) SHA1(a68f3b9350a3a05850c17d157b56de88556cd26a) )
MEGADRIVE_ROM_LOAD( calgames,  "mpr-14372.bin",                                                                          0x000000, 0x80000,   CRC(43b1b672) SHA1(0417ff05bb8bd696cfae8f795c09786665cb60ef) )
MEGADRIVE_ROM_LOAD( cannon,    "cannon fodder (europe).bin",                                                             0x000000, 0x180000,  CRC(ad217654) SHA1(ced5f0967e30c3b4c4c2a81007a7db2910b1885d) )
MEGADRIVE_ROM_LOAD( captaven,  "mpr-15499.bin",                                                                          0x000000, 0x100000,  CRC(43225612) SHA1(66ec647175251d8c109c6c21440d415e13e14001) )
MEGADRIVE_ROM_LOAD( captavenub,"captain america and the avengers (usa) (beta).bin",                                      0x000000, 0x100000,  CRC(baac59c0) SHA1(af8271c0c99637413006bdd8c66a7b5412e0ebd7) )
MEGADRIVE_ROM_LOAD( captavenu, "captain america and the avengers (usa).bin",                                             0x000000, 0x100000,  CRC(e0639ca2) SHA1(ae71b5744de7ca6aa00d72ccd7713146d87ca002) )
MEGADRIVE_ROM_LOAD( captplan,  "captain planet and the planeteers (europe).bin",                                         0x000000, 0x80000,   CRC(7672efa5) SHA1(b048f922db83802a4a78d1e8197a5ec52b73a89f) )
MEGADRIVE_ROM_LOAD( captplanu, "captain planet and the planeteers (usa).bin",                                            0x000000, 0x80000,   CRC(bf2cbd3a) SHA1(750770bb43a2d1310b9011f7588cc91e0476826b) )
MEGADRIVE_ROM_LOAD( castlillj, "mpr-13498.bin",                                                                          0x000000, 0x80000,   CRC(ce8333c6) SHA1(0679162757f375751a677fd05195c9248abc84f0) )
MEGADRIVE_ROM_LOAD( castlill,  "castle of illusion starring mickey mouse (usa, europe).bin",                             0x000000, 0x80000,   CRC(ba4e9fd0) SHA1(4ac3687634a5acc55ac7f156c6de9749158713e4) )
MEGADRIVE_ROM_LOAD( cvania,    "castlevania - the new generation (europe).bin",                                          0x000000, 0x100000,  CRC(4dd4e4a5) SHA1(61aabb1053f090fb6c13968c86170357c5df4eba) )
MEGADRIVE_ROM_LOAD( cvaniab,   "castlevania - the new generation (europe) (beta).bin",                                   0x000000, 0x100000,  CRC(84cd103a) SHA1(8c4004c0d8cbe211ffa3919fd609653221a0078b) )
MEGADRIVE_ROM_LOAD( cvaniau,   "castlevania - bloodlines (usa).bin",                                                     0x000000, 0x100000,  CRC(fb1ea6df) SHA1(4809cf80ced70e77bc7479bb652a9d9fe22ce7e6) )
MEGADRIVE_ROM_LOAD( centur,    "centurion - defender of rome (usa, europe).bin",                                         0x000000, 0xc0000,   CRC(21283b14) SHA1(8cc2a28309b2dc61da68c77d8a6545af05660e8b) )
// Centurion seems to suffer the same dumping problems of F22 September 1991 revision, needs a proper redump

MEGADRIVE_ROM_LOAD( chakan,    "chakan (usa, europe).bin",                                                               0x000000, 0x100000,  CRC(046a48de) SHA1(7eae088f0b15e4bcd9a6a38849df5a20446be548) )
MEGADRIVE_ROM_LOAD( chamel,    "chameleon kid (japan).bin",                                                              0x000000, 0x100000,  CRC(50217c80) SHA1(237ff4041f3e8ce5047f06f695fb55dca51354b8) )
MEGADRIVE_ROM_LOAD( champwcs,  "s357.bin",                                                                               0x000000, 0x100000,  CRC(883e33db) SHA1(2c072aa60c8cec143f5626fde4b704fd13a5f845) )
MEGADRIVE_ROM_LOAD( champbow,  "championship bowling (usa).bin",                                                         0x000000, 0x80000,   CRC(1bf92520) SHA1(ffb469106bc7f475cd89d717035a54cab149c818) )
MEGADRIVE_ROM_LOAD( champl,    "championship pool (usa).bin",                                                            0x000000, 0x100000,  CRC(253512cf) SHA1(443b96c518120078fb33f3ff9586a2b7ebc141c7) )
MEGADRIVE_ROM_LOAD( cproam,    "championship pro-am (usa).bin",                                                          0x000000, 0x40000,   CRC(b496de28) SHA1(6e3cc6e97d33890996dcbad9a88b1aef361ca2d9) )
MEGADRIVE_ROM_LOAD( chaoji,    "chao ji da fu weng (china) (unlicensed).bin",                                            0x000000, 0x100000,  CRC(2e2ea687) SHA1(be1a66bd1f75f5b1b37b8ae18e6334c36291e63d) )
MEGADRIVE_ROM_LOAD( chaose2p,  "chaos engine 2, the (europe) (prototype).bin",                                           0x000000, 0x100000,  CRC(3fb045c2) SHA1(9e2e12e90b60e6ca28af6a5afb213cafe1868a95) )
MEGADRIVE_ROM_LOAD( chaose,    "chaos engine, the (europe).bin",                                                         0x000000, 0x180000,   CRC(bd9eecf4) SHA1(b72d36565e13ab04dc20c547e0dcee1f67bcdb42) )
MEGADRIVE_ROM_LOAD( chasehq2,  "chase h.q. ii (usa).bin",                                                                0x000000, 0x80000,   CRC(f39e4bf2) SHA1(47c8c173980749aca075b9b3278c0df89a21303f) )
MEGADRIVE_ROM_LOAD( chavez,    "chavez ii (usa).bin",                                                                    0x000000, 0x100000,  CRC(5bc0dbb8) SHA1(5df09bfd8523166d5ad514eaeaf4f4dc76a8a06d) )
MEGADRIVE_ROM_LOAD( cheese,    "cheese cat-astrophe starring speedy gonzales (europe).bin",                              0x000000, 0x200000,  CRC(ff634b28) SHA1(12bb0d838c675b34cecc9041d3560fe35145ec39) )
MEGADRIVE_ROM_LOAD( chelnov,   "chelnov (japan).bin",                                                                    0x000000, 0x100000,  CRC(b2fe74d8) SHA1(ac1ae7b38f6498472bc4f072726a4ca069e41204) )
MEGADRIVE_ROM_LOAD( chess,     "chess (unlicensed) [!].bin",                                                             0x000000, 0x80000,   CRC(47380edd) SHA1(42e56fc5543dcb40da73447b582c84c4ff50a825) )
MEGADRIVE_ROM_LOAD( chester,   "chester cheetah - too cool to fool (usa).bin",                                           0x000000, 0x100000,  CRC(250e3ec3) SHA1(0e1d659d2b6bae32a25365c6592abf64bde94fe2) )
MEGADRIVE_ROM_LOAD( chesterw,  "chester cheetah - wild wild quest (usa).bin",                                            0x000000, 0x100000,  CRC(b97b735d) SHA1(3a0a01338320a4326a365447dc8983727738a21e) )
MEGADRIVE_ROM_LOAD( chichi,    "chi chi's pro challenge golf (usa).bin",                                                 0x000000, 0x100000,  CRC(9c3973a4) SHA1(d5e3a698b780c8f67def5f6df87b94d5419beefc) )
MEGADRIVE_ROM_LOAD( chibim,    "chibi maruko-chan - wakuwaku shopping (japan).bin",                                      0x000000, 0x80000,   CRC(91a144b8) SHA1(39d4da607d020bd20063c34b0a617a5e6cb7c5aa) )
MEGADRIVE_ROM_LOAD( chikij,    "chiki chiki boys (japan, korea).bin",                                                    0x000000, 0x100000,  CRC(06918c17) SHA1(77191eaee3775a425147ae7140ddf6ed3b6b41d2) )
MEGADRIVE_ROM_LOAD( chiki,     "chiki chiki boys (usa, europe).bin",                                                     0x000000, 0x100000,  CRC(813a7d62) SHA1(3cbeb068751c39790116aa8f422dd6f333be42e0) )
MEGADRIVE_ROM_LOAD( chinesec,  "chinese chess (unlicensed).bin",                                                         0x000000, 0x80000,   CRC(475215a0) SHA1(3907bf058493e7b9db9720493030f0284797908c) )
MEGADRIVE_ROM_LOAD( chinfb,    "chinese fighter iii (unlicensed) (bootleg).bin",                                         0x000000, 0x200000,  CRC(6f98247d) SHA1(cc212b1564dc7c73ffdc55f9fde3269a83fee399) )
MEGADRIVE_ROM_LOAD( chouky,    "chou kyuukai miracle nine (japan).bin",                                                  0x000000, 0x200000,  CRC(6d8c2206) SHA1(af2fd89dc7fb4ac0647c09edff462c7ae92dc771) )
MEGADRIVE_ROM_LOAD( chouto,    "chou touryuu retsuden dino land (japan).bin",                                            0x000000, 0x80000,   CRC(81f939de) SHA1(4bee752cd2205d60b9d3c7a58e0e90a4837522de) )
MEGADRIVE_ROM_LOAD( chuckrck,  "chuck rock (europe).bin",                                                                0x000000, 0x100000,  CRC(7cd40bea) SHA1(832a18eb028630e31b5bacd05f9694f4a827268b) )
MEGADRIVE_ROM_LOAD( chuckrcku, "chuck rock (usa).bin",                                                                   0x000000, 0x100000,  CRC(f8ac454a) SHA1(f6177b4c9ac48325c53fa26531cdd9bbc673dda3) )
MEGADRIVE_ROM_LOAD( chukrck2j, "chuck rock ii (japan).bin",                                                              0x000000, 0x100000,  CRC(bfd24be8) SHA1(e9217b089ade7f7b9566233e7bff66ba363ad6cb) )
MEGADRIVE_ROM_LOAD( chukrck2,  "chuck rock ii - son of chuck (europe).bin",                                              0x000000, 0x100000,  CRC(1ade9488) SHA1(f57ed0e6201b706abd5e837d9043723a1b3b4de5) )
MEGADRIVE_ROM_LOAD( chukrck2b, "chuck rock ii - son of chuck (usa) (beta).bin",                                          0x000000, 0x100000,  CRC(d6a3b324) SHA1(12faedb14e8ce02461b3529cce81ac9d4e854110) )
MEGADRIVE_ROM_LOAD( chukrck2u, "chuck rock ii - son of chuck (usa).bin",                                                 0x000000, 0x100000,  CRC(408b1cdb) SHA1(f5dc902a3d722842a4fb536644102de600e7dca3) )
MEGADRIVE_ROM_LOAD( chucks,    "chuck's excellent art tool animator (u).bin",                                            0x000000, 0x60000,   CRC(6360ee58) SHA1(191726e84fb80ccb992d7cf4188008e23f644432) )
MEGADRIVE_ROM_LOAD( classicc,  "classic collection (europe).bin",                                                        0x000000, 0x200000,  CRC(73f948b1) SHA1(80629bb91a1123ae832f6997f9f3c0e070ce81ca) )
MEGADRIVE_ROM_LOAD( clay,      "mpr-17510.bin",                                                                          0x000000, 0x200000,  CRC(1aaf7707) SHA1(1cde95b5c2571555dfd6461c3fd7f15912197d0d) )
MEGADRIVE_ROM_LOAD( clayu,     "clay fighter (usa).bin",                                                                 0x000000, 0x200000,  CRC(b12c1bc1) SHA1(9cd15c84f4ee85ad8f3a512a0fed000724251758) )
MEGADRIVE_ROM_LOAD( cliffb,    "cliffhanger (beta).bin",                                                                 0x000000, 0x100000,  CRC(628251fd) SHA1(af567a1ea2fe5ef080352f74d4b70a4e0a33f319) )
MEGADRIVE_ROM_LOAD( cliff,     "cliffhanger (europe).bin",                                                               0x000000, 0x100000,  CRC(35bff1fd) SHA1(c453130b2789fa5682367f9c071206e06952b951) )
MEGADRIVE_ROM_LOAD( cliffu,    "cliffhanger (usa).bin",                                                                  0x000000, 0x100000,  CRC(9cbf44d3) SHA1(bed365b6b7a1ef96cbdf64b35ad42b54d7d3fb1c) )
MEGADRIVE_ROM_LOAD( clueus,    "clue (usa).bin",                                                                         0x000000, 0x80000,   CRC(7753a296) SHA1(d1f9114f41a3d6237e24392629fea5fbeb3f0b87) )
MEGADRIVE_ROM_LOAD( coachk,    "coach k college basketball (usa).bin",                                                   0x000000, 0x200000,  CRC(67c309c6) SHA1(8ff5d7a7fcc47f030a3ea69f4534d9c892f58ce2) )
MEGADRIVE_ROM_LOAD( colleg96,  "college football usa 96 (usa).bin",                                                      0x000000, 0x200000,  CRC(b9075385) SHA1(3079bdc5f2d29dcf3798f899a3098736cdc2cd88) )
MEGADRIVE_ROM_LOAD( colleg97,  "college football usa 97 (usa).bin",                                                      0x000000, 0x200000,  CRC(2ebb90a3) SHA1(9b93035ecdc2b6f0815281764ef647f2de039e7b) )
MEGADRIVE_ROM_LOAD( collncp,   "college football's national championship (prototype - apr 13, 1994).bin",                0x000000, 0x200000,  CRC(d9772def) SHA1(2eb0daad82caff6bcefb438297a2d701c99173c5) )
MEGADRIVE_ROM_LOAD( collncp1,  "college football's national championship (prototype - apr 18, 1994).bin",                0x000000, 0x200000,  CRC(e0a1705f) SHA1(ea19e141c64cc4abc6e7d7eea7bbb6783569a05a) )
MEGADRIVE_ROM_LOAD( collncp2,  "college football's national championship (prototype - apr 19, 1994).bin",                0x000000, 0x200000,  CRC(d5fe66c3) SHA1(9961db2c46f0189c419da2e335e6ca974eaa5379) )
MEGADRIVE_ROM_LOAD( collncp3,  "college football's national championship (prototype - apr 29, 1994).bin",                0x000000, 0x200000,  CRC(99da1973) SHA1(11e333a326ea71f77b816add76defc8f2846710d) )
MEGADRIVE_ROM_LOAD( collncp4,  "college football's national championship (prototype - jun 01, 1994).bin",                0x000000, 0x200000,  CRC(994dbc8f) SHA1(99e5ec2705fac1566e47fd813d6cf5b5e7f7daf4) )
MEGADRIVE_ROM_LOAD( collncp5,  "college football's national championship (prototype - jun 03, 1994).bin",                0x000000, 0x200000,  CRC(9ca29321) SHA1(1e155744a1c089cd2332c27cdad48e7f243c2fc8) )
MEGADRIVE_ROM_LOAD( collncp6,  "college football's national championship (prototype - jun 07, 1994).bin",                0x000000, 0x200000,  CRC(b40b791e) SHA1(334daa4f48dea4d85145fcd1bfb03f522532a9ae) )
MEGADRIVE_ROM_LOAD( collncp7,  "college football's national championship (prototype - jun 08, 1994).bin",                0x000000, 0x200000,  CRC(d008debc) SHA1(1b69304213ef1732c0b9b2f059179a7cf18b2c75) )
MEGADRIVE_ROM_LOAD( collncp8,  "college football's national championship (prototype - jun 14, 1994).bin",                0x000000, 0x200000,  CRC(ecef7de7) SHA1(923ce9034f37167b65aec97e160f6fe34ea2da33) )
MEGADRIVE_ROM_LOAD( collncp9,  "college football's national championship (prototype - jun 15, 1994).bin",                0x000000, 0x200000,  CRC(c7e5a998) SHA1(c9d010a0ffccecc2c01412daf64bf1b0eaf5055e) )
MEGADRIVE_ROM_LOAD( collnp10,  "college football's national championship (prototype - jun 18, 1994).bin",                0x000000, 0x200000,  CRC(74988a9d) SHA1(164e3fc32aa295b0d87e1508dd5fe75f9a7cadb9) )
MEGADRIVE_ROM_LOAD( collnp11,  "college football's national championship (prototype - jun 20, 1994).bin",                0x000000, 0x200000,  CRC(898c17fa) SHA1(4b30eea2fb1187cf3c9150f9dee5b5b9571c76f5) )
MEGADRIVE_ROM_LOAD( collnp12,  "college football's national championship (prototype - may 03, 1994).bin",                0x000000, 0x200000,  CRC(db1a3f66) SHA1(d9db6ecb032fd88443d0575b01e61cb4aeea5703) )
MEGADRIVE_ROM_LOAD( collnp13,  "college football's national championship (prototype - may 06, 1994).bin",                0x000000, 0x200000,  CRC(0a538e75) SHA1(ee673500aef188ca7cf086fb1cf616b58896fdcb) )
MEGADRIVE_ROM_LOAD( collnp14,  "college football's national championship (prototype - may 11, 1994 - a).bin",            0x000000, 0x200000,  CRC(ce65b294) SHA1(b16e953b695148b8284f87be566774379c4c2453) )
MEGADRIVE_ROM_LOAD( collnp15,  "college football's national championship (prototype - may 17, 1994 - a).bin",            0x000000, 0x200000,  CRC(8cfd4c86) SHA1(01ed2026a930383d926d71192b2a8f9417dfb245) )
MEGADRIVE_ROM_LOAD( collnp16,  "college football's national championship (prototype - may 20, 1994).bin",                0x000000, 0x200000,  CRC(f285385e) SHA1(0b26c5748e976a64c02864e1934f2b50f6953cba) )
MEGADRIVE_ROM_LOAD( collnp17,  "college football's national championship (prototype - may 25, 1994).bin",                0x000000, 0x200000,  CRC(482e334a) SHA1(e2fb264a11e08d57acf2756688880cd6fc353aba) )
MEGADRIVE_ROM_LOAD( collnp18,  "college football's national championship (prototype - may 31, 1994).bin",                0x000000, 0x200000,  CRC(c6766745) SHA1(157b65be9d946c46f77a90e4a5847fa41f2692b9) )
MEGADRIVE_ROM_LOAD( collnc,    "college football's national championship (usa).bin",                                     0x000000, 0x200000,  CRC(172c5dbb) SHA1(a3db8661e160e07b09bca03ba0d20ba4e80a4c59) )
MEGADRIVE_ROM_LOAD( collnc2,   "college football's national championship ii (usa).bin",                                  0x000000, 0x200000,  CRC(65b64413) SHA1(9609f9934a80dba183dab603ae07f445f02b919d) )
MEGADRIVE_ROM_LOAD( collslam,  "college slam (usa).bin",                                                                 0x000000, 0x400000,  CRC(96a42431) SHA1(0dbbe740b14077fe8648955f7e17965ea25f382a) )
MEGADRIVE_ROM_LOAD( columns,   "columns (world) (v1.1).bin",                                                             0x000000, 0x20000,   CRC(d783c244) SHA1(17ae2595e4d3fb705c9f8f66d5938deca3f95c4e) )
MEGADRIVE_ROM_LOAD( columns1,  "mpr-13193.bin",                                                                          0x000000, 0x20000,   CRC(03163d7a) SHA1(b262a4c2738a499f070777dbe05e2629d211a107) )
MEGADRIVE_ROM_LOAD( columns3,  "columns iii - revenge of columns (usa).bin",                                             0x000000, 0x80000,   CRC(dc678f6d) SHA1(8e52a5d0adbff3b2a15f32e9299b4ffdf35f5541) )
MEGADRIVE_ROM_LOAD( columns3a, "columns iii - taiketsu! columns world (japan, korea).bin",                               0x000000, 0x80000,   CRC(cd07462f) SHA1(2e850c2b737098b9926ac0fc9b8b2116fc5aa48a) )
MEGADRIVE_ROM_LOAD( combat,    "combat aces (beta).bin",                                                                 0x000000, 0x80000,   CRC(84560d5a) SHA1(0492bcd5341e768455bd353e786fac74837289af) )
MEGADRIVE_ROM_LOAD( combatca,  "combat cars (usa, europe).bin",                                                          0x000000, 0x100000,  CRC(e439b101) SHA1(0c7ca93b412c8ab5753ae047de49a3e41271cc3b) )
MEGADRIVE_ROM_LOAD( comix1,    "comix zone (e) (prototype - jun 28, 1995).bin",                                          0x000000, 0x200000,  CRC(939efd4c) SHA1(62b6f4ac864862bf5360e72f3fb8b45700e4fcfa) )
MEGADRIVE_ROM_LOAD( comix,     "mpr-18301.bin",                                                                          0x000000, 0x200000,  CRC(1318e923) SHA1(9523cf8e485a3246027f5a02ecbcee3c5ba690f0) )
MEGADRIVE_ROM_LOAD( comixj,    "comix zone (japan).bin",                                                                 0x000000, 0x200000,  CRC(7a6027b8) SHA1(44f8c2a102971d0afcb0d9bd9081ccf51ff830a9) )
MEGADRIVE_ROM_LOAD( comixk,    "comix zone (k) (prototype - jun 09, 1995).bin",                                          0x000000, 0x200000,  CRC(e45a15f4) SHA1(2427e7e3932da731eb172ae22ad779c0abfced13) )
MEGADRIVE_ROM_LOAD( comixp1,   "comix zone (prototype - jul 12, 1995 - fulscr).bin",                                     0x000000, 0x200000,  CRC(e28c89c1) SHA1(4cf1fb69c8184257664caacaf7bc7a1b4e802b8b) )
MEGADRIVE_ROM_LOAD( comixp2,   "comix zone (prototype - jul 12, 1995).bin",                                              0x000000, 0x200000,  CRC(e28c89c1) SHA1(4cf1fb69c8184257664caacaf7bc7a1b4e802b8b) )
MEGADRIVE_ROM_LOAD( comixp3,   "comix zone (prototype - jun 01, 1995 - b).bin",                                          0x000000, 0x200000,  CRC(bbe03cb6) SHA1(4223ee8f7ab2a62532fac57f8933bb52e3dde4d5) )
MEGADRIVE_ROM_LOAD( comixp4,   "comix zone (prototype - jun 01, 1995 - c).bin",                                          0x000000, 0x200000,  CRC(413b9a94) SHA1(aae5dd875e36b9416c8169bdf5dd51e4612e98db) )
MEGADRIVE_ROM_LOAD( comixp5,   "comix zone (prototype - jun 01, 1995 - d).bin",                                          0x000000, 0x200000,  CRC(9ae93c9d) SHA1(3599c2d52a1edc9876befb2a754be3506959a12b) )
MEGADRIVE_ROM_LOAD( comixp6,   "comix zone (prototype - jun 01, 1995).bin",                                              0x000000, 0x200000,  CRC(ba506835) SHA1(45de8279bb10cae44f3485057b29810479f82798) )
MEGADRIVE_ROM_LOAD( comixp7,   "comix zone (prototype - jun 02, 1995 - b).bin",                                          0x000000, 0x200000,  CRC(b5f0dae5) SHA1(1d8e133a0ce52e4586593beca095ed269f01473e) )
MEGADRIVE_ROM_LOAD( comixp8,   "comix zone (prototype - jun 02, 1995).bin",                                              0x000000, 0x200000,  CRC(1a4abcf3) SHA1(fa1e69531857d4146497de4ae95957d096bceb20) )
MEGADRIVE_ROM_LOAD( comixp9,   "comix zone (prototype - jun 03, 1995).bin",                                              0x000000, 0x200000,  CRC(aff32614) SHA1(9cfead71b55cfa48fa6fb66d2d7c6294776fe4ed) )
MEGADRIVE_ROM_LOAD( comip10,   "comix zone (prototype - may 26, 1995).bin",                                              0x000000, 0x200000,  CRC(36029a1f) SHA1(dbce6c71b1b1c193a57416dd4ccacc876a9e64af) )
MEGADRIVE_ROM_LOAD( comip11,   "comix zone (prototype - may 30, 1995).bin",                                              0x000000, 0x200000,  CRC(a72eca2c) SHA1(c751ec1ba09abda3ea3252a3e211383f054a507c) )
MEGADRIVE_ROM_LOAD( comixsc,   "comix zone (sega channel) (prototype - jun 12, 1995).bin",                               0x000000, 0x200000,  CRC(c77db9e1) SHA1(5da563561e3d74f5db672c8694891083db869999) )
MEGADRIVE_ROM_LOAD( comixub,   "comix zone (usa) (beta).bin",                                                            0x000000, 0x200000,  CRC(2efcb6ee) SHA1(af73f6d0d9e54416496a39dbfafc610bf16b3c0c) )
MEGADRIVE_ROM_LOAD( comixu,    "comix zone (usa).bin",                                                                   0x000000, 0x200000,  CRC(17da0354) SHA1(e8747eefdf61172be9da8787ba5be447ec73180f) )
MEGADRIVE_ROM_LOAD( congo,     "congo (unknown) (prototype).bin",                                                        0x000000, 0xf7d36,   CRC(13746716) SHA1(976d7610eb80691f50466f5102ccd49cc3a2b9f7) )
MEGADRIVE_ROM_LOAD( contra,    "contra - hard corps (usa, korea).bin",                                                   0x000000, 0x200000,  CRC(c579f45e) SHA1(68ea84146105bda91f6056932ff4fb42aa3eb4a7) )
MEGADRIVE_ROM_LOAD( contraj,   "contra - the hard corps (japan).bin",                                                    0x000000, 0x200000,  CRC(2ab26380) SHA1(0a9d263490497c85d7010979765c48f98d9927bd) )
MEGADRIVE_ROM_LOAD( coolsp,    "mpr-15477.bin",                                                                          0x000000, 0x100000,  CRC(5f09fa41) SHA1(b6dc5d4c29b2161f7252828cf267117e726d8e82) )
MEGADRIVE_ROM_LOAD( coolspj,   "cool spot (japan, korea).bin",                                                           0x000000, 0x100000,  CRC(e869efb1) SHA1(e32826ca9ae5173d5ef9722b52bfcb4ad390a7bb) )
MEGADRIVE_ROM_LOAD( coolspub,  "cool spot (usa) (beta).bin",                                                             0x000000, 0x100000,  CRC(0ebaa4a8) SHA1(9b42bb33186ddc759a469ed3e0ee12e5dee0b809) )
MEGADRIVE_ROM_LOAD( coolspu,   "cool spot (usa).bin",                                                                    0x000000, 0x100000,  CRC(f024c1a1) SHA1(9a214e0eab58ddb8e9d752e41fce2ce08e6c39a7) )
MEGADRIVE_ROM_LOAD( corpor,    "mpr-15065.bin",                                                                          0x000000, 0x100000,  CRC(a80d18aa) SHA1(520e01abe76120dbd680b9fc34eb1303c780b069) )
MEGADRIVE_ROM_LOAD( cosmic,    "cosmic spacehead (usa, europe) (en,fr,de,es).bin",                                       0x000000, 0x100000,  CRC(c593d31c) SHA1(95a3eb13e5d28db8c8ea5ff3e95b0d3e614def69) )
MEGADRIVE_ROM_LOAD( crackd,    "mpr-13578a.bin",                                                                         0x000000, 0x80000,   CRC(d012a47a) SHA1(7c146c24216bb333eaa3b08e358582e4465b145e) )
MEGADRIVE_ROM_LOAD( crackd1,   "crack down (japan, europe).bin",                                                         0x000000, 0x80000,   CRC(538aaa5d) SHA1(1082c89920699dbc6f6672c9b0519b3d0f626ba5) )
MEGADRIVE_ROM_LOAD( crackdu,   "crack down (usa).bin",                                                                   0x000000, 0x80000,   CRC(b9ce9051) SHA1(949cd961a99e9c35388cf6a4db5e866102ed27a2) )
MEGADRIVE_ROM_LOAD( crayon,    "crayon shin-chan - arashi o yobu enji (japan).bin",                                      0x000000, 0x200000,  CRC(97fc42d2) SHA1(46214898990ebc5e74413a53f7304ad466875fbe) )
MEGADRIVE_ROM_LOAD( crossf,    "cross fire (usa).bin",                                                                   0x000000, 0x80000,   CRC(cc73f3a9) SHA1(cc681bb62483dfb3ee3ef976dff29cc80ad01820) )
MEGADRIVE_ROM_LOAD( crudeb,    "crude buster (japan).bin",                                                               0x000000, 0x100000,  CRC(affb4b00) SHA1(9122ef0e920133449266cf437a87a110e0343425) )
MEGADRIVE_ROM_LOAD( cruej,     "crue ball (japan).bin",                                                                  0x000000, 0x80000,   CRC(514c53e2) SHA1(1ceb4370dacaa8e3ab4b8beb6ff008db269d0387) )
MEGADRIVE_ROM_LOAD( crue,      "crue06.bin",                                                                             0x000000, 0x80000,   CRC(4b195fc0) SHA1(f0c62f1beb4126d1d1d1b634d11fcd81f2723704) )
MEGADRIVE_ROM_LOAD( crusader,  "crusader of centy (usa).bin",                                                            0x000000, 0x200000,  CRC(41858f6f) SHA1(bf2e8d122f4670865bedbc305ef991ee5f52d647) )
MEGADRIVE_ROM_LOAD( crying,    "crying - aseimei sensou (japan).bin",                                                    0x000000, 0x100000,  CRC(4aba1d6a) SHA1(c45b6da77021d57df6a9cb511cc93a5bf83ecf1c) )
MEGADRIVE_ROM_LOAD( cryst0,    "crystal's pony tale (prototype - jul 01, 1994).bin",                                     0x000000, 0x100000,  CRC(16ac2e4f) SHA1(37e6369f0c5f8969003fd4ec1c291e06900c617a) )
MEGADRIVE_ROM_LOAD( cryst1,    "crystal's pony tale (prototype - jul 02, 1994).bin",                                     0x000000, 0x100000,  CRC(24bc4354) SHA1(fa14565f90a254bbe8cff18e62f65e9ff1da1736) )
MEGADRIVE_ROM_LOAD( cryst2,    "crystal's pony tale (prototype - jul 03, 1994).bin",                                     0x000000, 0x100000,  CRC(6d67a87c) SHA1(decec86674df91d1b9d29881745e279759470125) )
MEGADRIVE_ROM_LOAD( cryst3,    "crystal's pony tale (prototype - jul 12, 1994 - b).bin",                                 0x000000, 0x100000,  CRC(3577dc72) SHA1(db225126e319380d89c0a5ab58d9f75d97419e6a) )
MEGADRIVE_ROM_LOAD( cryst4,    "crystal's pony tale (prototype - jul 12, 1994).bin",                                     0x000000, 0xff554,   CRC(2bfc0a53) SHA1(9ac7eba7cb238602925ed2a23f870b81b413991e) )
MEGADRIVE_ROM_LOAD( cryst5,    "crystal's pony tale (prototype - jul 13, 1994).bin",                                     0x000000, 0x100000,  CRC(c67b6dba) SHA1(58e56c2cbb94f81205c6f38656a0c75dcf27e267) )
MEGADRIVE_ROM_LOAD( cryst6,    "crystal's pony tale (prototype - jun 01, 1994).bin",                                     0x000000, 0x100000,  CRC(14c6347c) SHA1(170db22a5bfc30afe19ece372ad0cceea1961cd7) )
MEGADRIVE_ROM_LOAD( cryst7,    "crystal's pony tale (prototype - jun 06, 1994).bin",                                     0x000000, 0x100000,  CRC(83e8cac9) SHA1(2941ae87e22de10eadd0b7f75bf2e003396d4c99) )
MEGADRIVE_ROM_LOAD( cryst8,    "crystal's pony tale (prototype - jun 10, 1994).bin",                                     0x000000, 0x100000,  CRC(9e1dd267) SHA1(cd0ebcdca3b8fdd763c8748bc2ebc91690bebd7f) )
MEGADRIVE_ROM_LOAD( cryst9,    "crystal's pony tale (prototype - jun 23, 1994).bin",                                     0x000000, 0x100000,  CRC(4ccc19af) SHA1(2220bcb6baa349f5086c3a42ead63c7efeb49631) )
MEGADRIVE_ROM_LOAD( crys10,    "crystal's pony tale (prototype - jun 28, 1994).bin",                                     0x000000, 0x100000,  CRC(03be7f66) SHA1(ca3abcc3403c34fa7b2ea6ff56b1967267d96fce) )
MEGADRIVE_ROM_LOAD( crys11,    "crystal's pony tale (prototype - jun 30, 1994).bin",                                     0x000000, 0x100000,  CRC(dd8c73fb) SHA1(4db4a0b358a88f9c04f2f3c73d7b9ce1ac87a8da) )
MEGADRIVE_ROM_LOAD( crys12,    "crystal's pony tale (prototype - may 11, 1994).bin",                                     0x000000, 0x100000,  CRC(eac9d74b) SHA1(53cd0e287b54a49ddf1d6075fab4b3a7f1ddc396) )
MEGADRIVE_ROM_LOAD( crys13,    "crystal's pony tale (prototype - may 19, 1994).bin",                                     0x000000, 0x100000,  CRC(dc50b741) SHA1(b9c271296977e2b304a8a808cb2d5e9fb44467b1) )
MEGADRIVE_ROM_LOAD( crys14,    "crystal's pony tale (prototype - may 26, 1994).bin",                                     0x000000, 0x100000,  CRC(a2eaba55) SHA1(16278beee4d39ea9f70ce6225373b193881cb2b0) )
MEGADRIVE_ROM_LOAD( crysta,    "crystal's pony tale (usa).bin",                                                          0x000000, 0x100000,  CRC(6cf7a4df) SHA1(dc99ed62da89745559f49b040d0365038140efd9) )
MEGADRIVE_ROM_LOAD( cursej,    "curse (japan).bin",                                                                      0x000000, 0x80000,   CRC(a4fbf9a9) SHA1(978780d9575022450d415591f32e1118c7fac5ad) )
MEGADRIVE_ROM_LOAD( cutiesuz,  "cutie suzuki no ringside angel (japan).bin",                                             0x000000, 0x80000,   CRC(13795dca) SHA1(4e3f222c7333d39fc5452b37aebc14fa09ff8d5e) )
MEGADRIVE_ROM_LOAD( cutthr,    "cutthroat island (usa, europe).bin",                                                     0x000000, 0x200000,  CRC(ebabbc70) SHA1(1b1648e02bb2e915286c35f01476358a8401608f) )
MEGADRIVE_ROM_LOAD( cyberp,    "mpr-13192.bin",                                                                          0x000000, 0x80000,   CRC(87b636a2) SHA1(353f7d136a2c464ee976402b4620b0a42b8b7267) )
MEGADRIVE_ROM_LOAD( cybercop,  "cyber-cop (usa).bin",                                                                    0x000000, 0x100000,  CRC(01e719c8) SHA1(72470ea83064046a0b1dfba3047032c26d610df8) )
MEGADRIVE_ROM_LOAD( cyberbal,  "mpr-13201.bin",                                                                          0x000000, 0x80000,   CRC(76120e96) SHA1(4e459751ced8956326602c581b8b169f8e716545) )
MEGADRIVE_ROM_LOAD( cyborb,    "cyborg justice (beta).bin",                                                              0x000000, 0x80000,   CRC(91daf11e) SHA1(27c2932b2e25b17fdf5f2ebd74ebbde4015118b6) )
MEGADRIVE_ROM_LOAD( cybor,     "mpr-15468.bin",                                                                          0x000000, 0x80000,   CRC(ab0d1269) SHA1(6d0c72fa5e53d897390707eb4c6d3e86e6772215) )
MEGADRIVE_ROM_LOAD( daffyb,    "daffy duck in hollywood (europe) (beta).bin",                                            0x000000, 0x200000,  CRC(0eaa4740) SHA1(7d869208846ce10ccf799c581cb09034c81aa3e5) )
MEGADRIVE_ROM_LOAD( daffy,     "daffy duck in hollywood (europe) (en,fr,de,es,it).bin",                                  0x000000, 0x200000,  CRC(1fdc66b0) SHA1(67cc61b724d4ccb94dc2b59cc8ea1b0eb9a8cf4e) )
MEGADRIVE_ROM_LOAD( dahnamk,   "mpr-14636.bin",                                                                          0x000000, 0x100000,  CRC(10829ea1) SHA1(655486eaf3b197063902224a8f1cc15b664f7637) )
MEGADRIVE_ROM_LOAD( dahnam,    "dahna megami tanjou (japan).bin",                                                        0x000000, 0x100000,  CRC(4602584f) SHA1(da0246a063e6f70933e251b9ae84587fe620d4f0) )
MEGADRIVE_ROM_LOAD( daimakai,  "dai makaimura (japan).bin",                                                              0x000000, 0xa0000,   CRC(5659f379) SHA1(a08a5764dc8651d31ef72466028cfb87fb6dd166) )
MEGADRIVE_ROM_LOAD( daik,      "daikoukai jidai (japan).bin",                                                            0x000000, 0x100000,  CRC(5a652458) SHA1(a4552b23079b161da9ad47ac7cb9c4ecb3731967) )
MEGADRIVE_ROM_LOAD( daik2,     "daikoukai jidai ii (japan).bin",                                                         0x000000, 0x200000,  CRC(e040f0da) SHA1(74f61092067d82127cae3306d5a66d3efe946bc3) )
MEGADRIVE_ROM_LOAD( daisen,    "daisenpuu / twin hawk (japan, europe).bin",                                              0x000000, 0x80000,   CRC(a2ec8c67) SHA1(f884f2a41dd50f4c1a17c26da7f2d31093bb36b6) )
MEGADRIVE_ROM_LOAD( danger,    "dangerous seed (japan).bin",                                                             0x000000, 0x80000,   CRC(a2990031) SHA1(32b2b1de0947fb713787020679efacffd29ec04e) )
MEGADRIVE_ROM_LOAD( darius2,   "darius ii (japan).bin",                                                                  0x000000, 0x100000,  CRC(25dfe62a) SHA1(93593e4014e13db90fedc1903a402c6f7d885a2f) )
MEGADRIVE_ROM_LOAD( darkcast,  "dark castle (usa, europe).bin",                                                          0x000000, 0x80000,   CRC(0464aca4) SHA1(23e1ad9822338362113e55087d60fb9a1674bf8a) )
MEGADRIVE_ROM_LOAD( darwin,    "darwin 4081 (japan, korea).bin",                                                         0x000000, 0x80000,   CRC(7a33b0cb) SHA1(e9decc48451aba62d949a7710e466e1d041a2210) )
MEGADRIVE_ROM_LOAD( dashba,    "dashin' desperadoes (beta).bin",                                                         0x000000, 0x100000,  CRC(adaffc3f) SHA1(4b04d7f6fc731275d9fcf3b7566674d4987c2154) )
MEGADRIVE_ROM_LOAD( dashb,     "dashin' desperadoes (usa) (beta).bin",                                                   0x000000, 0x100000,  CRC(98d224a5) SHA1(d5793276f7a07f4dd8a9035dbb70a7f3ef2448a3) )
MEGADRIVE_ROM_LOAD( dash,      "dashin' desperadoes (usa).bin",                                                          0x000000, 0x100000,  CRC(dcb76fb7) SHA1(1d9a12b0df291a21f1e1070af2b13c1f15dbbe67) )
MEGADRIVE_ROM_LOAD( davidc,    "david crane's amazing tennis (usa).bin",                                                 0x000000, 0x100000,  CRC(9177088c) SHA1(1543d9790f4311847fd4426b6f395b9c63365a9e) )
MEGADRIVE_ROM_LOAD( drscj,     "david robinson basketball (japan).bin",                                                  0x000000, 0x80000,   CRC(56164b00) SHA1(eeac902df31240c246afe2b97a5b25104dd9f0e5) )
MEGADRIVE_ROM_LOAD( drsc,      "mpr-14835.bin",                                                                          0x000000, 0x80000,   CRC(512b7599) SHA1(adcee1a54caa0db0aab0927b877d2be828964275) )
MEGADRIVE_ROM_LOAD( dcup2a,    "davis cup ii (usa) (prototype).bin",                                                     0x000000, 0x200000,  CRC(76f2bed8) SHA1(5b548c9794a8af0d9fba4d7abfb6ca96a7f01709) )
MEGADRIVE_ROM_LOAD( dcup,      "davis cup world tour (usa, europe) (july 1993).bin",                                     0x000000, 0x100000,  CRC(894686f0) SHA1(735cf7c84869bfa795114f5eff835a74252a4adc) )
MEGADRIVE_ROM_LOAD( dcupa,     "davis cup world tour (usa, europe) (june 1993).bin",                                     0x000000, 0x100000,  CRC(7c6b0646) SHA1(bce5937d930e016bb14eca80c833071df4bdc0f8) )
MEGADRIVE_ROM_LOAD( dcup2,     "davis cup world tour tennis 2 (beta 1).bin",                                             0x000000, 0x200000,  CRC(6f4183c0) SHA1(3c8527d1a01a4c7ab97b2664046f9c11283e3332) )
MEGADRIVE_ROM_LOAD( dazeb,     "daze before christmas (australia) (beta).bin",                                           0x000000, 0x200000,  CRC(317c9491) SHA1(a63cdb54c31b1c1219467987a81e761ee92feee3) )
MEGADRIVE_ROM_LOAD( daze,      "daze before christmas (australia).bin",                                                  0x000000, 0x200000,  CRC(b95e25c9) SHA1(240c0d9487d7659a4b2999e0394d552ab17bee8a) )
MEGADRIVE_ROM_LOAD( deadly,    "deadly moves (usa).bin",                                                                 0x000000, 0x100000,  CRC(35cbd237) SHA1(8733d179292d4dc5c3513459539d96484b6d018f) )
MEGADRIVE_ROM_LOAD( deathret,  "death and return of superman, the (usa).bin",                                            0x000000, 0x200000,  CRC(982242d3) SHA1(3534d17801bd5756b10a7f8d7d95f3a8d9b74844) )
MEGADRIVE_ROM_LOAD( deathdl,   "death duel (usa).bin",                                                                   0x000000, 0x100000,  CRC(a9804dcc) SHA1(21390cc3036047f3de4a58c5f41f588079b0e56f) )
MEGADRIVE_ROM_LOAD( decapatt,  "mpr-14249.bin",                                                                          0x000000, 0x80000,   CRC(73dc0dd8) SHA1(9665f54a6149d71ea72db9c168755e62cb61649c) )
MEGADRIVE_ROM_LOAD( demob,     "demolition man (usa) (beta).bin",                                                        0x000000, 0x200000,  CRC(57ffad7a) SHA1(33a33bff7277c2aab45d0d843d13728ce2c62ab2) )
MEGADRIVE_ROM_LOAD( demo,      "demolition man (usa, europe).bin",                                                       0x000000, 0x200000,  CRC(5ff71877) SHA1(40d71f6bd6cd44f8003bfaff8c953b0693ec1b01) )
MEGADRIVE_ROM_LOAD( desert0,   "desert demolition (prototype - dec 06, 1994).bin",                                       0x000000, 0xed0ad,   CRC(d27fe9e5) SHA1(355c55039490a4ed882506327d61ec40e583ca12) )
MEGADRIVE_ROM_LOAD( desert1,   "desert demolition (prototype - dec 08, 1994).bin",                                       0x000000, 0xfdfb8,   CRC(69aaeab6) SHA1(781d636634da8d41addd28c9e34367dacda38576) )
MEGADRIVE_ROM_LOAD( desert2,   "desert demolition (prototype - dec 12, 1994 - b).bin",                                   0x000000, 0x100000,  CRC(16f19283) SHA1(e031ffd85b14507ef5833f4b854ceb2e522f9fa7) )
MEGADRIVE_ROM_LOAD( desert3,   "desert demolition (prototype - dec 12, 1994).bin",                                       0x000000, 0xec988,   CRC(5dad49d7) SHA1(f7dbf2b88ac9f36eb7cc34ba3f61024dc1754772) )
MEGADRIVE_ROM_LOAD( desert4,   "desert demolition (prototype - dec 13, 1994).bin",                                       0x000000, 0xfde9c,   CRC(375dee15) SHA1(c287b81ef983fe832bbd44155cd9a880985aa11c) )
MEGADRIVE_ROM_LOAD( desert5,   "desert demolition (prototype - dec 14, 1994).bin",                                       0x000000, 0xfde9c,   CRC(39f9f304) SHA1(1d6dd50944dd32d14cb83541ed33ef69479700c2) )
MEGADRIVE_ROM_LOAD( desert6,   "desert demolition (prototype - dec 15, 1994).bin",                                       0x000000, 0xfde9c,   CRC(5109736a) SHA1(cbb8b864039f1bff4446086c6b44de468069093e) )
MEGADRIVE_ROM_LOAD( desert7,   "desert demolition (prototype - dec 16, 1994).bin",                                       0x000000, 0x100000,  CRC(573fabfb) SHA1(6d32d81e1a7a06f0acf58af88a4c909621251576) )
MEGADRIVE_ROM_LOAD( desert8,   "desert demolition (prototype - dec 19, 1994).bin",                                       0x000000, 0xfde9c,   CRC(b0ece758) SHA1(f081d9d1affee698efb00188c07443a4c64618b6) )
MEGADRIVE_ROM_LOAD( desert, "desert demolition starring road runner and wile e. coyote (usa, europe).bin",               0x000000, 0x100000,  CRC(c287343d) SHA1(afc95f7ce66e30abbe10f8d5cd6b791407c7a0bc) )
MEGADRIVE_ROM_LOAD( dstrikej, "desert strike (japan, korea).bin",                                                        0x000000, 0x100000,  CRC(1e740145) SHA1(9e01c6c45670e2950242facfba6c5b79052550ed) )
MEGADRIVE_ROM_LOAD( dstrike, "desert strike (usa, europe).bin",                                                          0x000000, 0x100000,  CRC(67a9860b) SHA1(d7e7d8c358eb845b84fb08f904cc0b95d0a4053d) )
MEGADRIVE_ROM_LOAD( devilc, "devil crash md (japan).bin",                                                                0x000000, 0x80000,   CRC(4c4caad8) SHA1(58d447ecd6a6af9845cdd90ac3df0b5503535117) )
MEGADRIVE_ROM_LOAD( devilish, "devilish - the next possession (usa).bin",                                                0x000000, 0x80000,   CRC(d3f300ac) SHA1(bc6f346a162cf91a788a88e44daa246d3d31ee8b) )
MEGADRIVE_ROM_LOAD( dialqo, "dial q o mawase! (japan) (unlicensed).bin",                                                 0x000000, 0x100000,  CRC(c632e5af) SHA1(08967e04d992264f193ecdfd0e0457baaf25f4f2) )
MEGADRIVE_ROM_LOAD( dicktr, "mpr-13661.bin",                                                                             0x000000, 0x80000,   CRC(ef887533) SHA1(320e527847ebae79d2686c5a500c5100b080ff98) )
MEGADRIVE_ROM_LOAD( dickvi, "dick vitale's 'awesome, baby!' college hoops (usa).bin",                                    0x000000, 0x200000,  CRC(1312cf22) SHA1(2aac7a1cf92e51a14fd3e12a830e135fee255044) )
MEGADRIVE_ROM_LOAD( dinodini, "dino dini's soccer (europe).bin",                                                         0x000000, 0x100000,  CRC(4608f53a) SHA1(49d4a654dd2f393e43a363ed171e73cd4c8ff4f4) )
MEGADRIVE_ROM_LOAD( dinoland, "dino land (usa).bin",                                                                     0x000000, 0x80000,   CRC(5fe351b8) SHA1(e55937e48a0c348ba3db731a8722dae7f39dede5) )
MEGADRIVE_ROM_LOAD( dinotale, "dinosaur's tale, a (usa).bin",                                                            0x000000, 0x100000,  CRC(70155b5b) SHA1(b1f8e741399fd2c28dfb1c3340af868d222b1c14) )
MEGADRIVE_ROM_LOAD( dinohirea, "dinosaurs for hire (prototype - apr 26, 1993).bin",                                      0x000000, 0x100000,  CRC(54c77d3e) SHA1(101b97769ddd54e04f6c0c1d46d194fc78cb6f22) )
MEGADRIVE_ROM_LOAD( dinohireb, "dinosaurs for hire (prototype - apr 27, 1993).bin",                                      0x000000, 0x100000,  CRC(16000453) SHA1(944af2e25e9ab5ecf82fe5da1c55113edeee8709) )
MEGADRIVE_ROM_LOAD( dinohirec, "dinosaurs for hire (prototype - may 02, 1993).bin",                                      0x000000, 0x100000,  CRC(8954dee9) SHA1(2a04b30cf4930631bdb6c031ecd12c23387c278b) )
MEGADRIVE_ROM_LOAD( dinohire, "dinosaurs for hire (usa).bin",                                                            0x000000, 0x100000,  CRC(39351146) SHA1(d006efbf1d811e018271745925fe00ca6d93f24f) )
MEGADRIVE_ROM_LOAD( disney, "disney collection, the (europe).bin",                                                       0x000000, 0x100000,  CRC(adfde883) SHA1(991e22d7f258cba94306d16d5dd37172a34da9d3) )
MEGADRIVE_ROM_LOAD( divine, "divine sealing (japan) (unlicensed).bin",                                                   0x000000, 0x100000,  CRC(ca72973c) SHA1(29cc95622a1c9602e7981bdc5a66164c47939028) )
MEGADRIVE_ROM_LOAD( djboy, "mpr-14560.bin",                                                                              0x000000, 0x80000,   CRC(860e749a) SHA1(d2f111c240d0165a231c236e9ae6e62e73ca9caa) )
MEGADRIVE_ROM_LOAD( djboyj, "dj boy (japan).bin",                                                                        0x000000, 0x80000,   CRC(202abaa8) SHA1(c66b2e00b8a9172a76f5e699730c7d88740b5dd7) )
MEGADRIVE_ROM_LOAD( djboyu, "dj boy (usa).bin",                                                                          0x000000, 0x80000,   CRC(dc9f02db) SHA1(981f0eeffa28210cc55702413305244aaf36d71c) )
MEGADRIVE_ROM_LOAD( dokidoki, "doki doki penguin land md (japan) (game no kandume megacd rip).bin",                      0x000000, 0x40000,   CRC(22736650) SHA1(e64e4c127158db366e95e8c3cc6b4d5906ad78ec) )
MEGADRIVE_ROM_LOAD( domino, "domino (unlicensed).bin",                                                                   0x000000, 0x100000,  CRC(a64409be) SHA1(7000ea86d91bbb5642425b6a6f577fab9e2b3a51) )
MEGADRIVE_ROM_LOAD( dominus, "dominus (usa) (prototype).bin",                                                            0x000000, 0xc0000,   CRC(30006ebd) SHA1(fde8160bf51575463e81cbd0b5d12fb5d49eb695) )
MEGADRIVE_ROM_LOAD( mauimall, "donald in maui mallard (europe).bin",                                                     0x000000, 0x300000,  CRC(b2dd857f) SHA1(cd3c50b7f9c2f97d7bb0042e4239a05066ae72e0) )
MEGADRIVE_ROM_LOAD( donggu, "dong gu ri te chi jak jeon (korea).bin",                                                    0x000000, 0x100000,  CRC(e32f0b1c) SHA1(a698c4eda62032d1f98b3e4f824b6475c2612859) )
MEGADRIVE_ROM_LOAD( doomtr, "doom troopers - the mutant chronicles (usa).bin",                                           0x000000, 0x200000,  CRC(11194414) SHA1(aff5e0651fecbc07d718ed3cea225717ce18a7aa) )
MEGADRIVE_ROM_LOAD( doraemon, "doraemon - yume dorobou to 7 nin no gozans (japan).bin",                                  0x000000, 0x80000,   CRC(eeed1130) SHA1(aa92c04afd6b1916fcc6c64285d8dffe0b2b895f) )
MEGADRIVE_ROM_LOAD( doublecl, "double clutch (europe).bin",                                                              0x000000, 0x40000,   CRC(d98c623c) SHA1(e19905cfcf74185e56fa94ae292f78451c8f4e2e) )
MEGADRIVE_ROM_LOAD( ddragon, "double dragon (usa, europe).bin",                                                          0x000000, 0x80000,   CRC(054f5d53) SHA1(f054fb2c16a78ce5f5ce20a36bf0f2634f169969) )
MEGADRIVE_ROM_LOAD( ddragon3, "double dragon 3 - the arcade game (usa, europe).bin",                                     0x000000, 0x100000,  CRC(b36ab75c) SHA1(663dfeebd21409942bcc446633b9b9f0dd238aa8) )
MEGADRIVE_ROM_LOAD( ddragon2, "double dragon ii - the revenge (japan).bin",                                              0x000000, 0x80000,   CRC(a8bfdbd6) SHA1(68dc151ada307ed0ed34f98873e0be5f65f1b573) )
MEGADRIVE_ROM_LOAD( ddragon5, "double dragon v - the shadow falls (usa).bin",                                            0x000000, 0x300000,  CRC(27e59e35) SHA1(53402b7c43cd20bf4cbaf40d1ba5062e3823f4bd) )
MEGADRIVE_ROM_LOAD( ddribble, "double dribble - the playoff edition (usa).bin",                                          0x000000, 0x200000,  CRC(8352b1d0) SHA1(d97bbe009e9667709e04a1bed9f71cea7893a495) )
MEGADRIVE_ROM_LOAD( drrobotn, "dr. robotnik's mean bean machine (europe).bin",                                           0x000000, 0x100000,  CRC(70680706) SHA1(8cdaca024585aab557e9a09732a298e5112ee15b) )
MEGADRIVE_ROM_LOAD( drrobotn1, "dr. robotnik's mean bean machine (usa) (beta).bin",                                      0x000000, 0x100000,  CRC(4d0e5273) SHA1(312f9a283bebc5d612a63afd2cf67eb923f4f074) )
MEGADRIVE_ROM_LOAD( drrobotnu, "dr. robotnik's mean bean machine (usa).bin",                                             0x000000, 0x100000,  CRC(c7ca517f) SHA1(aa6b60103fa92bc95fcc824bf1675e411627c8d3) )
MEGADRIVE_ROM_LOAD( dragon, "dragon - the bruce lee story (europe).bin",                                                 0x000000, 0x200000,  CRC(fdeed51d) SHA1(d6fb86d73e1abc7b7f1aecf77a52fa3f759aedb1) )
MEGADRIVE_ROM_LOAD( dragonu, "dragon - the bruce lee story (usa).bin",                                                   0x000000, 0x200000,  CRC(efe850e5) SHA1(01b45b9865282124253264c5f2e3d3338858ff92) )
MEGADRIVE_ROM_LOAD( dbz, "dragon ball z - buyuu retsuden (japan).bin",                                                   0x000000, 0x200000,  CRC(af8f3371) SHA1(dc6336dfbbe76c24ada735009d0d667ce27843f6) )
MEGADRIVE_ROM_LOAD( dbzf, "dragon ball z - l'appel du destin (france).bin",                                              0x000000, 0x200000,  CRC(f035c737) SHA1(5ff71986f4911b5dfd16598a5a3a9ba398c92c60) )
MEGADRIVE_ROM_LOAD( dslayer, "dragon slayer - eiyuu densetsu (japan).bin",                                               0x000000, 0x200000,  CRC(01bc1604) SHA1(f67c9139bbc93f171e274a5cd3fba66480cd8244) )
MEGADRIVE_ROM_LOAD( dslayer2, "dragon slayer - eiyuu densetsu ii (japan).bin",                                           0x000000, 0x200000,  CRC(46924dc3) SHA1(79b6201301acb5d9e9c56dcf65a9bcf9d9a931ab) )
MEGADRIVE_ROM_LOAD( shai3, "dragon's eye plus - shanghai iii (japan).bin",                                               0x000000, 0x40000,   CRC(81f0c3cf) SHA1(b5a2a3b0b65058614d24853c525505b709f00851) )
MEGADRIVE_ROM_LOAD( dfury, "dragon's fury (usa, europe).bin",                                                            0x000000, 0x80000,   CRC(58037bc6) SHA1(bcbafa6c4ab0b16ddb4f316a1ef8c0eecd0cd990) )
MEGADRIVE_ROM_LOAD( dragrev, "dragon's revenge (usa, europe).bin",                                                       0x000000, 0x100000,  CRC(841edbc0) SHA1(75854a732d4cf9a310c4359092cf5c2482df49a7) )
MEGADRIVE_ROM_LOAD( dreamteam, "dream team usa (japan).bin",                                                             0x000000, 0x100000,  CRC(e2e21b72) SHA1(48b36852b31f74d33e00b33df04ae1e9e9b0cb1c) )
MEGADRIVE_ROM_LOAD( dukenu3d, "duke nukem 3d (brazil).bin",                                                              0x000000, 0x400000,  CRC(6bd2accb) SHA1(a4663f2b96787a92db604a92491fa27e2b5ced9e) )
MEGADRIVE_ROM_LOAD( duneth, "dune - the battle for arrakis (usa).bin",                                                   0x000000, 0x100000,  CRC(4dea40ba) SHA1(0f7c1c130cb39abc97f57545933e1ef6c481783d) )
MEGADRIVE_ROM_LOAD( dune2g, "dune ii - kampf um den wustenplaneten (germany).bin",                                       0x000000, 0x100000,  CRC(39790728) SHA1(55996cd262df518e92271bceee4d2a657cd7e02c) )
MEGADRIVE_ROM_LOAD( dune2, "dune ii - the battle for arrakis (europe).bin",                                              0x000000, 0x100000,  CRC(b58ae71d) SHA1(133cc86b43afe133fc9c9142b448340c17fa668e) )
MEGADRIVE_ROM_LOAD( ddwares, "dungeons & dragons - warriors of the eternal sun (usa, europe).bin",                       0x000000, 0x100000,  CRC(af4a9cd7) SHA1(9135f7fda03ef7da92dfade9c0df75808214f693) )
MEGADRIVE_ROM_LOAD( dynabr, "dyna brothers (japan).bin",                                                                 0x000000, 0x100000,  CRC(360c1b20) SHA1(7e57c6fd5c20e356f9c967dbf168db53574eda84) )
MEGADRIVE_ROM_LOAD( dynabr2, "dyna brothers 2 (japan).bin",                                                              0x000000, 0x200000,  CRC(47e0a64e) SHA1(0d6c3d9eb0cb9a56ab91b7507b09473b078e773c) )
MEGADRIVE_ROM_LOAD( dynaduke, "mpr-13438a.bin",                                                                          0x000000, 0x80000,   CRC(246f0bda) SHA1(965ec4e6a239a707160d0a67973bc6da8212b53d) )
MEGADRIVE_ROM_LOAD( dynaduke1, "mpr-13438.bin",                                                                          0x000000, 0x80000,   CRC(39d01c8c) SHA1(1bd77ad31665f7bdda85d9dfb9f08c0338ec4da9) )
MEGADRIVE_ROM_LOAD( dheadja, "dynamite headdy (japan) (beta).bin",                                                       0x000000, 0x200000,  CRC(5c25e934) SHA1(9889510234dd8771fd14c4022710028c5928d152) )
MEGADRIVE_ROM_LOAD( dheadj, "dynamite headdy (japan).bin",                                                               0x000000, 0x200000,  CRC(d03cdb53) SHA1(02727a217e654f3cdf5a3fcd33b2f38d404a467d) )
MEGADRIVE_ROM_LOAD( dheadp1, "dynamite headdy (prototype - jun 14, 1994 - cabeza).bin",                                  0x000000, 0x200000,  CRC(971ef24c) SHA1(c90417bd43bea29a58fcacacb48b2602ecb65cc3) )
MEGADRIVE_ROM_LOAD( dheadp2, "dynamite headdy (prototype - jun 15, 1994).bin",                                           0x000000, 0x200000,  CRC(971ef24c) SHA1(c90417bd43bea29a58fcacacb48b2602ecb65cc3) )
MEGADRIVE_ROM_LOAD( dheadp3, "dynamite headdy (prototype - jun 16, 1994).bin",                                           0x000000, 0x200000,  CRC(f8a96719) SHA1(84f4e06fe51a448148e6f86a5d11d07577df7029) )
MEGADRIVE_ROM_LOAD( dheadp4, "dynamite headdy (prototype - jun 22, 1994).bin",                                           0x000000, 0x200000,  CRC(61001ec8) SHA1(74a1d84ccde678d9209f7aa872b50f0a98ec07aa) )
MEGADRIVE_ROM_LOAD( dhead, "mpr-16990.bin",                                                                              0x000000, 0x200000,  CRC(3dfeeb77) SHA1(e843decdff262791b1237f1545f5b17c56712d5f) )
MEGADRIVE_ROM_LOAD( eahockey, "ea hockey (europe).bin",                                                                  0x000000, 0x80000,   CRC(9bfc279c) SHA1(118419e596bf07047a18d1828290019fbc4afc45) )
MEGADRIVE_ROM_LOAD( eahockeyj, "ea hockey (japan).bin",                                                                  0x000000, 0x80000,   CRC(9dcdc894) SHA1(40166e9c2b40f35ba7d7cf5d07865c824c323757) )
MEGADRIVE_ROM_LOAD( easports, "ea304.bin",                                                                               0x000000, 0x100000,  CRC(a0b54cbc) SHA1(499047852b5892fcdaca191a5aab19257d6a85a8) )
MEGADRIVE_ROM_LOAD( earnest, "earnest evans (usa).bin",                                                                  0x000000, 0x100000,  CRC(a243816d) SHA1(cb5a2a928e2c2016f915e07e1d148672563183f0) )
MEGADRIVE_ROM_LOAD( ejim, "earthworm jim (europe).bin",                                                                  0x000000, 0x300000,  CRC(1c07b337) SHA1(d5dc11009e3a5cc2381dd2a75cb81ce2e7428342) )
MEGADRIVE_ROM_LOAD( ejimu, "earthworm jim (usa).bin",                                                                    0x000000, 0x300000,  CRC(df3acf59) SHA1(a544211d1ebab1f096f6e72a0d724f74f9ddbce8) )
MEGADRIVE_ROM_LOAD( ejim2, "earthworm jim 2 (europe).bin",                                                               0x000000, 0x300000,  CRC(af235fdf) SHA1(b8e93ea8b42c688a218b83797e4a18eda659f3e0) )
MEGADRIVE_ROM_LOAD( ejim2u, "earthworm jim 2 (usa).bin",                                                                 0x000000, 0x300000,  CRC(d57f8ba7) SHA1(ef7cccfc5eafa32fc6acc71dd9b71693f64eac94) )
MEGADRIVE_ROM_LOAD( ecco2b, "ecco - the tides of time (beta).bin",                                                       0x000000, 0x200000,  CRC(1d1470ea) SHA1(364523cd30615ce4a94793ebbc189c0db6adc38f) )
MEGADRIVE_ROM_LOAD( ecco2, "ecco - the tides of time (europe).bin",                                                      0x000000, 0x200000,  CRC(7b1bf89c) SHA1(cb7d44a40992cff6c31d685866814d6cb85add59) )
MEGADRIVE_ROM_LOAD( ecco2x, "ecco - the tides of time (prototype x11 - apr 13, 1994).bin",                               0x000000, 0x200000,  CRC(b5d64817) SHA1(6cc1ea0b6c4be29ed7a02cd5e01f9504d272323e) )
MEGADRIVE_ROM_LOAD( ecco2p, "ecco - the tides of time (usa) (beta).bin",                                                 0x000000, 0x200000,  CRC(8db60749) SHA1(3120625e8ff6b289ae6c398b8afe1ab5ff6396e3) )
MEGADRIVE_ROM_LOAD( ecco2u, "ecco - the tides of time (usa).bin",                                                        0x000000, 0x200000,  CRC(ccb21f98) SHA1(82ec466a81b95942ad849c5e2f88781bef28acc8) )
MEGADRIVE_ROM_LOAD( eccojr, "ecco jr. (usa, australia) (february 1995).bin",                                             0x000000, 0x100000,  CRC(6c6f4b89) SHA1(0b493ef23874f82606d4fd22c2380b289247aa9f) )
MEGADRIVE_ROM_LOAD( eccojr1, "ecco jr. (usa, australia) (march 1995).bin",                                               0x000000, 0x100000,  CRC(3c517975) SHA1(636d2fb5f865f916e4a9fe0ff1819fcbc61b4258) )
MEGADRIVE_ROM_LOAD( eccoj, "mpr-15702.bin",                                                                              0x000000, 0x100000,  CRC(6520304d) SHA1(1440fb5821ebb08048f73a0a71ac22e0cdbcf394) )
MEGADRIVE_ROM_LOAD( ecco, "mpr-15265.bin",                                                                               0x000000, 0x100000,  CRC(45547390) SHA1(2cad130f3118c189d39fd1d46c5c31a5060ce894) )
MEGADRIVE_ROM_LOAD( ecco2j, "mpr-16996.bin",                                                                             0x000000, 0x200000,  CRC(062d439c) SHA1(0ca535d0e0d430c67e413ab904ef867516ce9fad) )
MEGADRIVE_ROM_LOAD( elvientj, "el viento (japan).bin",                                                                   0x000000, 0x100000,  CRC(6091c36e) SHA1(564ffd2a40b9bebe30587182a644050de474aa5a) )
MEGADRIVE_ROM_LOAD( elvient, "mpr-14336.bin",                                                                            0x000000, 0x100000,  CRC(070a1ceb) SHA1(b53e901725fd6220d8e6d19ef8df42e89e0874db) )
MEGADRIVE_ROM_LOAD( elemastj, "elemental master (japan).bin",                                                            0x000000, 0x80000,   CRC(5f553e29) SHA1(6cbe2cd607aa9850306970a8a61cb9fa82445293) )
MEGADRIVE_ROM_LOAD( elemast, "elemental master (usa).bin",                                                               0x000000, 0x80000,   CRC(390918c6) SHA1(fd47e4f8c2d6cc82860ae281faad50e3dee2d033) )
MEGADRIVE_ROM_LOAD( elfwor, "elf wor (china) (unlicensed).bin",                                                          0x000000, 0x100000,  CRC(e24ac6b2) SHA1(5fc4591fbb1acc64e184466c7b6287c7f64e0b7a) )
MEGADRIVE_ROM_LOAD( elimin, "eliminate down (japan).bin",                                                                0x000000, 0x100000,  CRC(48467542) SHA1(53b418bde065011e161e703dd7c175aa48a04fe5) )
MEGADRIVE_ROM_LOAD( elit95, "swed9584.bin",                                                                              0x000000, 0x200000,  CRC(e10a25c0) SHA1(5f2c8303099ce13fe1e5760b7ef598a2967bfa8d) )
MEGADRIVE_ROM_LOAD( elit96, "elitserien 96 (sweden).bin",                                                                0x000000, 0x200000,  CRC(9821d0a3) SHA1(085fb8e6f0d2ff0f399de5c57eb13d9c9325dbae) )
MEGADRIVE_ROM_LOAD( empire, "empire of steel (europe).bin",                                                              0x000000, 0x100000,  CRC(e5517b77) SHA1(00109e6de5b823525b3ba519222fecc4e229f6d1) )
MEGADRIVE_ROM_LOAD( tazem, "taz in escape from mars (europe).bin",                                                       0x000000, 0x200000,  CRC(62100099) SHA1(1edd44d9f9d1d410b6a9ec37647a04d9b50b549e) )
MEGADRIVE_ROM_LOAD( tazemu, "taz in escape from mars (usa).bin",                                                         0x000000, 0x200000,  CRC(62009f8c) SHA1(7aa9c7b74541d414d81aa61d150803c3b3b1701b) )
MEGADRIVE_ROM_LOAD( tazema, "taz in escape from mars (prototype - apr 18, 1994).bin",                                    0x000000, 0x200000,  CRC(bfa776ec) SHA1(e35e8019b3a89319fd22dfee444741b955820afd) )
MEGADRIVE_ROM_LOAD( tazemb, "taz in escape from mars (prototype - jun 02, 1994).bin",                                    0x000000, 0x200000,  CRC(39f345c2) SHA1(1ccdb04cea4a0dfc816e8fd9552f77d201504002) )
MEGADRIVE_ROM_LOAD( tazemc, "taz in escape from mars (prototype - jun 07, 1994).bin",                                    0x000000, 0x200000,  CRC(3481e709) SHA1(5ecc3a4754507410830039e6b27d81b675727d43) )
MEGADRIVE_ROM_LOAD( tazemd, "taz in escape from mars (prototype - jun 10, 1994).bin",                                    0x000000, 0x200000,  CRC(832f4cb4) SHA1(48199482c13bcf49461fd5459617e75497fadcf9) )
MEGADRIVE_ROM_LOAD( tazeme, "taz in escape from mars (prototype - jun 14, 1994).bin",                                    0x000000, 0x200000,  CRC(2badb65b) SHA1(74b701be56a7a7ba286dcf354fbe646d0b9ce73a) )
MEGADRIVE_ROM_LOAD( tazemf, "taz in escape from mars (prototype - jun 18, 1994 - a).bin",                                0x000000, 0x200000,  CRC(3c4b1e9c) SHA1(b81fe7b3656f859f51f60bd7d368ffc62c8bb615) )
MEGADRIVE_ROM_LOAD( tazemg, "taz in escape from mars (prototype - jun 20, 1994).bin",                                    0x000000, 0x200000,  CRC(f00f4203) SHA1(de7927962c29477b3bf0392f27e07b44daf185c9) )
MEGADRIVE_ROM_LOAD( tazemh, "taz in escape from mars (prototype - mar 09, 1994).bin",                                    0x000000, 0x1d9936,  CRC(e3c2271e) SHA1(587e57673dfec493231f410677e7b380484bf7d2) )
MEGADRIVE_ROM_LOAD( tazemi, "taz in escape from mars (prototype - may 09, 1994).bin",                                    0x000000, 0x200000,  CRC(11727086) SHA1(a2d00ac89176dc61d4f736a8c65bd7153c184293) )
MEGADRIVE_ROM_LOAD( tazemj, "taz in escape from mars (prototype - may 18, 1994).bin",                                    0x000000, 0xb9584,   CRC(6e2666de) SHA1(c046885fbf4d654b636c0c1ea9fd65acc3b18da3) )
MEGADRIVE_ROM_LOAD( tazemk, "taz in escape from mars (prototype - may 23, 1994).bin",                                    0x000000, 0x200000,  CRC(ea344fd2) SHA1(92a7563beb8e4957251d7fff29354f0dc6aa0394) )
MEGADRIVE_ROM_LOAD( espnba, "espn baseball tonight (usa).bin",                                                           0x000000, 0x200000,  CRC(96d8440c) SHA1(8353eb5ccf4fe6676465eb2619b8ecb594191e0a) )
MEGADRIVE_ROM_LOAD( espnhna, "espn national hockey night (usa) (beta).bin",                                              0x000000, 0x200000,  CRC(a427814a) SHA1(c890fc232ca46a9499bb4e3107a519ae2d8edb81) )
MEGADRIVE_ROM_LOAD( espnhn, "espn national hockey night (usa).bin",                                                      0x000000, 0x200000,  CRC(1d08828c) SHA1(cfd65e3ffb17e1718356ef8de7c527e2c9fd8940) )
MEGADRIVE_ROM_LOAD( espnspb, "espn speed world (usa) (beta).bin",                                                        0x000000, 0x200000,  CRC(ab12849a) SHA1(2d69701a22929a751483e6f3ccf126dcccdbd38a) )
MEGADRIVE_ROM_LOAD( espnsp, "espn speed world (usa).bin",                                                                0x000000, 0x200000,  CRC(f50be478) SHA1(9400a2ee865f11d68c766059318d6fe69987d89b) )
MEGADRIVE_ROM_LOAD( espnfb, "espn sunday night nfl (usa) (beta).bin",                                                    0x000000, 0x200000,  CRC(d13aecdc) SHA1(8eeace9ef641d806afc1eaeafa244a076123b118) )
MEGADRIVE_ROM_LOAD( espnf, "espn sunday night nfl (usa).bin",                                                            0x000000, 0x200000,  CRC(61e9c309) SHA1(03f8c8805ebd4313c8a7d76b34121339bad33f89) )
MEGADRIVE_ROM_LOAD( eswatc, "eswat - city under siege (usa).bin",                                                        0x000000, 0x80000,   CRC(e72f8a36) SHA1(5cb96061bd2b00c82f8d6b46ab9802e2b1820c86) )
MEGADRIVE_ROM_LOAD( eternalcb, "eternal champions (europe) (beta).bin",                                                  0x000000, 0x300000,  CRC(e0665f06) SHA1(c06f2d9ff29b6e6ec2dbec06eab2eac21e80e423) )
MEGADRIVE_ROM_LOAD( eternalc, "mpr-16212+mpr-16213.bin",                                                                 0x000000, 0x300000,  CRC(b9512f5e) SHA1(578475a736bef8c76ba158110cebadf495c2f252) )
// two chips cart, needs a proper dump

MEGADRIVE_ROM_LOAD( eternalcp, "eternal champions (japan) (prototype - nov 17, 1993).bin",                               0x000000, 0x2ff4b0,  CRC(c486b439) SHA1(ba82947bef2c42021cc35512c21d91430c6fc55c) )
MEGADRIVE_ROM_LOAD( eternalcj, "eternal champions (japan, korea).bin",                                                   0x000000, 0x300000,  CRC(66aa3c64) SHA1(1b115e64e138ca045acfe64e2337fc68172be576) )
MEGADRIVE_ROM_LOAD( eternalcu, "eternal champions (usa).bin",                                                            0x000000, 0x300000,  CRC(48f1a42e) SHA1(5e978217c10b679a42d7d2966a4ccb77e1715962) )
MEGADRIVE_ROM_LOAD( europa, "europa sensen (japan).bin",                                                                 0x000000, 0x100000,  CRC(b0416c60) SHA1(d382f977ea3071f133a947ceb3528904e72f9bc9) )
MEGADRIVE_ROM_LOAD( europe, "mpr-14776.bin",                                                                             0x000000, 0x80000,   CRC(6a5cf104) SHA1(03e1a6c7fb8003e196c0f0bf787276a14daa313e) )
MEGADRIVE_ROM_LOAD( evander, "mpr-14860.bin",                                                                            0x000000, 0x80000,   CRC(4fef37c8) SHA1(eb4aca22f8b5837a0a0b10491c46714948b09844) )
MEGADRIVE_ROM_LOAD( exmutant, "ex-mutants (usa, europe).bin",                                                            0x000000, 0x100000,  CRC(33b1979f) SHA1(20b282ff8220291d98853888cfe504692c04e654) )
MEGADRIVE_ROM_LOAD( exranzab, "ex-ranza (japan) (beta).bin",                                                             0x000000, 0x100000,  CRC(c642fdf4) SHA1(f1c883f3bc6343c8a8181261ead3a534520a447f) )
MEGADRIVE_ROM_LOAD( exranza, "mpr-15585.bin",                                                                            0x000000, 0x100000,  CRC(349bb68d) SHA1(0006f55e826148cff9e717b582a39a04adf100df) )
MEGADRIVE_ROM_LOAD( exileu, "exile (usa).bin",                                                                           0x000000, 0x100000,  CRC(1b569dc2) SHA1(28377e9a68c2dcdfdb4133c2eb0b634aec552958) )
MEGADRIVE_ROM_LOAD( exilet, "exile - toki no hazama e (japan).bin",                                                      0x000000, 0x100000,  CRC(880bf311) SHA1(14ef77cbf6c023365e168f54456d5486679292ef) )
MEGADRIVE_ROM_LOAD( exosqu, "exo squad (europe).bin",                                                                    0x000000, 0x100000,  CRC(b599b9f9) SHA1(672c12b9cdfb2c7d8e0b1c389627c02a7c970cb3) )
MEGADRIVE_ROM_LOAD( exosq1, "exo squad (usa) (beta).bin",                                                                0x000000, 0x100000,  CRC(70edf964) SHA1(d165e0057a5d290ae02ff6cae9f99c2ebc8a3f7c) )
MEGADRIVE_ROM_LOAD( exosq2, "exo squad (usa).bin",                                                                       0x000000, 0x100000,  CRC(10ec03f3) SHA1(d958c3f2365162cb2ffa37fcea36695a1d4ab287) )
MEGADRIVE_ROM_LOAD( exodus, "exodus - journey to the promised land (usa) (unlicensed).bin",                              0x000000, 0x80000,   CRC(22e6fc04) SHA1(a85fb15d29dc43d3bf4a06de83506c77aba8a7d5) )
MEGADRIVE_ROM_LOAD( f117ni, "f-117 night storm (usa, europe).bin",                                                       0x000000, 0x200000,  CRC(1bf67a07) SHA1(906c7ac3ea3c460e9093d5082c68e35f1bb1bb0e) )
MEGADRIVE_ROM_LOAD( f117st, "f-117 stealth - operation night storm (japan).bin",                                         0x000000, 0x200000,  CRC(ea6e421a) SHA1(ded62d63676664c5b9e8a38e36ce1d2659d1c8d6) )
MEGADRIVE_ROM_LOAD( f15str, "f-15 strike eagle ii (europe).bin",                                                         0x000000, 0x100000,  CRC(e98ee370) SHA1(190508c5fe65c47f289d85768f7d79b1a235f3d5) )
MEGADRIVE_ROM_LOAD( f15str1, "f-15 strike eagle ii (usa) (beta).bin",                                                    0x000000, 0x100000,  CRC(fd4f5a01) SHA1(2ee364462f3206d220bbcebe50ca35e039967cf2) )
MEGADRIVE_ROM_LOAD( f15str2, "f-15 strike eagle ii (usa).bin",                                                           0x000000, 0x100000,  CRC(412c4d60) SHA1(d5f0d3d3b9cb7557d85bd54465072555f60c25a1) )
MEGADRIVE_ROM_LOAD( f22j, "f-22 interceptor (japan).bin",                                                                0x000000, 0xc0000,   CRC(fb55c785) SHA1(a6d0cf179d40ab7844d2a9f8a412a320a0639eec) )
// the Japanese F22 release doesn't seem to suffer the same dumping problems of the September 1991 revision

MEGADRIVE_ROM_LOAD( f22b, "f-22 interceptor (usa, europe) (beta).bin",                                                   0x000000, 0xc0000,   CRC(d6a880a4) SHA1(f104e6fab831fb482b0426bae98f04a65a05f392) )
MEGADRIVE_ROM_LOAD( f22o, "f-22 interceptor (usa, europe) (june 1992).bin",                                              0x000000, 0xc0000,   CRC(dd19b2b3) SHA1(32a479ed7571c5f99b2181829afc517370e3051a) )
// this needs a proper dump like the September 1991 revision

MEGADRIVE_ROM_LOAD( f1euro, "mpr-15834.bin",                                                                             0x000000, 0x100000,  CRC(fbdd4520) SHA1(a8327157f6537f4cd5daaff648864e8e0bf945f1) )
MEGADRIVE_ROM_LOAD( f1wcb, "f1 - world championship edition (europe) (beta).bin",                                        0x000000, 0x200000,  CRC(2269ed6b) SHA1(6b9984e92eee0044ac245ea534ed667425747e7b) )
MEGADRIVE_ROM_LOAD( f1wc, "f1 - world championship edition (europe).bin",                                                0x000000, 0x200000,  CRC(74cee0a7) SHA1(c50f66c7a220c58ae29a4610faa9ea6e39a54dfe) )
MEGADRIVE_ROM_LOAD( f1circ, "f1 circus md (japan).bin",                                                                  0x000000, 0x80000,   CRC(5d30befb) SHA1(2efb8d46163e785a57421e726991328024ecd2a7) )
MEGADRIVE_ROM_LOAD( faerytal, "faery tale adventure, the (usa, europe).bin",                                             0x000000, 0x80000,   CRC(963f4969) SHA1(5d9a448d741743c7e75e5fe462e0da0ea3d18a13) )
MEGADRIVE_ROM_LOAD( family, "family feud (usa).bin",                                                                     0x000000, 0x80000,   CRC(1aa628b0) SHA1(8277f86faf6968e277638ab6dfa410a5b3daf5b6) )
MEGADRIVE_ROM_LOAD( fantasia, "mpr-14028a.bin",                                                                          0x000000, 0x80000,   CRC(fc43df2d) SHA1(e3ba172d93505f8e0c1763b600f68af11b68f261) )
MEGADRIVE_ROM_LOAD( fantasia1, "mpr-14028.bin",                                                                          0x000000, 0x80000,   CRC(d351b242) SHA1(ce0f0813878700e78b3819ff0db49bd297add09e) )
MEGADRIVE_ROM_LOAD( fantdz, "fantastic dizzy (usa, europe) (en,fr,de,es,it).bin",                                        0x000000, 0x80000,   CRC(46447e7a) SHA1(b320174d3b43f30b477818a27b4da30462a52003) )
MEGADRIVE_ROM_LOAD( fantdz1, "fantastic dizzy (ue) (m5) [a1].bin",                                                       0x000000, 0x80000,   CRC(86b2a235) SHA1(ddda3664e0e7e2999815cce50e3c02170a8fec52) )
MEGADRIVE_ROM_LOAD( fastest1, "fastest 1 (japan).bin",                                                                   0x000000, 0x80000,   CRC(bb43f0de) SHA1(837a19b4b821076f1cd01d53d90b4553a0252340) )
MEGADRIVE_ROM_LOAD( fatalf, "fatal fury (europe, korea).bin",                                                            0x000000, 0x180000,  CRC(2e730a91) SHA1(e0a74238cd4592d900f88d033314c668d1522b72) )
MEGADRIVE_ROM_LOAD( fatalfu, "fatal fury (usa).bin",                                                                     0x000000, 0x180000,  CRC(98d49170) SHA1(48b6277fc73368d8e96ba407a66f908c3bd87c39) )
MEGADRIVE_ROM_LOAD( fatalf2, "fatal fury 2 (usa, korea).bin",                                                            0x000000, 0x300000,  CRC(1b1754cb) SHA1(52a269de38ed43ea5c6623906af6b64f01696ffb) )
MEGADRIVE_ROM_LOAD( labdeath1, "shi no meikyuu - labyrinth of death (japan) (game no kandume megacd rip).bin",           0x000000, 0x20000,   CRC(40c44cd3) SHA1(a854b4fb96e34183fbe462e23a2a0bf7dd6d9f09) )
MEGADRIVE_ROM_LOAD( labdeath, "shi no meikyuu - labyrinth of death (japan) (seganet).bin",                               0x000000, 0x40000,   CRC(a6d7e02d) SHA1(513732ae1c5a40276959967bbf4775fce1c83a7e) )
MEGADRIVE_ROM_LOAD( fatlab, "fatal labyrinth (usa, europe).bin",                                                         0x000000, 0x20000,   CRC(5f0bd984) SHA1(c136cb3fae1914a8fe079eb28c4533af5d8774e2) )
MEGADRIVE_ROM_LOAD( fatalrew, "fatal rewind (usa, europe).bin",                                                          0x000000, 0x80000,   CRC(e91aed05) SHA1(02634d919ec7d08f3c6833f229b5127dd52c9e8a) )
MEGADRIVE_ROM_LOAD( fatman, "fatman (japan).bin",                                                                        0x000000, 0xa0000,   CRC(7867da3a) SHA1(9c2791e44f1ff236cbab742c2942defc7b378e2c) )
MEGADRIVE_ROM_LOAD( fengku, "feng kuang tao hua yuan (china) (unlicensed).bin",                                          0x000000, 0x100000,  CRC(8d40d64f) SHA1(0330611a5fcb1f3ca173fcb3387fd35b865f1131) )
MEGADRIVE_ROM_LOAD( fengsh, "feng shen ying jie chuan (china) (unlicensed).bin",                                         0x000000, 0x200000,  CRC(6a382b60) SHA1(7a6e06846a94df2df2417d6509e398c29354dc68) )
MEGADRIVE_ROM_LOAD( ferias, "ferias frustradas do pica-pau (brazil).bin",                                                0x000000, 0x100000,  CRC(7b2e416d) SHA1(ec546d1c00c88554b5e9135f097cb1c388cfd68f) )
MEGADRIVE_ROM_LOAD( ferrb, "ferrari grand prix challenge (beta).bin",                                                    0x000000, 0x100000,  CRC(d48d02d4) SHA1(133e362ab58a309d5b77fa0544423591cc535ec9) )
MEGADRIVE_ROM_LOAD( ferr, "mpr-14770a.bin",                                                                              0x000000, 0x100000,  CRC(250111df) SHA1(0e33965994a6f53aca93c3cb164726b913789fc1) )
MEGADRIVE_ROM_LOAD( ferru, "ferrari grand prix challenge (usa).bin",                                                     0x000000, 0x100000,  CRC(f73f6bec) SHA1(bd08e2e0857c7b385c15d96e555f25d12457c917) )
MEGADRIVE_ROM_LOAD( feverp, "fever pitch soccer (europe) (en,fr,de,es,it).bin",                                          0x000000, 0x200000,  CRC(fac29677) SHA1(e3489b80a4b21049170fedee7630111773fe592c) )
MEGADRIVE_ROM_LOAD( fidodido, "fido dido (usa) (prototype).bin",                                                         0x000000, 0x100000,  CRC(c6d4a240) SHA1(fa69728de541321a5d55fd2c11ce8222d7daac45) )
MEGADRIVE_ROM_LOAD( fifa98, "fifa 98 - road to world cup (europe) (en,fr,es,it,sv).bin",                                 0x000000, 0x200000,  CRC(96947f57) SHA1(6613f13da5494aaaba3222ed5e730ec9ce3c09a7) )
MEGADRIVE_ROM_LOAD( fifa, "soccer10.bin",                                                                                0x000000, 0x200000,  CRC(bddbb763) SHA1(1cbef8c4541311b84d7388365d12a93a1f712dc4) )
MEGADRIVE_ROM_LOAD( fifa2k, "fifa soccer 2000 gold edition (unlicensed) (m6).bin",                                       0x000000, 0x200000,  CRC(647df504) SHA1(ed29312dbd9574514c06e5701e24c6474ed84898) )
MEGADRIVE_ROM_LOAD( fifa95k, "lh5370hd.bin",                                                                             0x000000, 0x200000,  CRC(012591f9) SHA1(ad1202a2e4166f8266d5633b8c5beb59c6cbd005) )
MEGADRIVE_ROM_LOAD( fifa95, "fifa95b1.bin",                                                                              0x000000, 0x200000,  CRC(b389d036) SHA1(586f9d0f218cf6bb3388a8610b44b6ebb9538fb5) )
MEGADRIVE_ROM_LOAD( fifa96, "fifa01.bin",                                                                                0x000000, 0x200000,  CRC(bad30ffa) SHA1(a7fcfe478b368d7d33bcbca65245f5faed9a1e07) )
MEGADRIVE_ROM_LOAD( fifa99, "fifa soccer 99 (russia) (unlicensed).bin",                                                  0x000000, 0x200000,  CRC(c5c5a4b0) SHA1(2c8c1dc0aaa711e3ab3fe0d74b79184f33127350) )
MEGADRIVE_ROM_LOAD( fightmj, "fighting masters (japan, korea).bin",                                                      0x000000, 0x80000,   CRC(39be80ec) SHA1(f944a8314d615ef58f0207e5760767fb417cbeb9) )
MEGADRIVE_ROM_LOAD( fightm, "fighting masters (usa).bin",                                                                0x000000, 0x80000,   CRC(5f51983b) SHA1(2a860f06473e436041c2017342deb9125c1c7af5) )
MEGADRIVE_ROM_LOAD( finalb, "final blow (japan).bin",                                                                    0x000000, 0x80000,   CRC(48ad505d) SHA1(610c3136e6ddccc41ab216affd07034fa46341a8) )
MEGADRIVE_ROM_LOAD( firemust, "fire mustang (japan).bin",                                                                0x000000, 0x80000,   CRC(eb7e36c3) SHA1(6daa07738b8f62659f2a3be01d3adc8557b879c5) )
MEGADRIVE_ROM_LOAD( firesh, "mpr-14341.bin",                                                                             0x000000, 0x80000,   CRC(2351ce61) SHA1(54b9060699187bf32048c005a3379fda72c0fb96) )
MEGADRIVE_ROM_LOAD( fireshu, "fire shark (usa).bin",                                                                     0x000000, 0x80000,   CRC(570b5024) SHA1(453ca331d15c47171c42312c14585541a3613802) )
MEGADRIVE_ROM_LOAD( fireshu1, "fire shark (usa) (alt).bin",                                                              0x000000, 0x80000,   CRC(9c175146) SHA1(020169eb2a4b3ad63fa2cbaa1927ab7c33b6add4) )
MEGADRIVE_ROM_LOAD( flashb, "mpr-15410a.bin",                                                                            0x000000, 0x180000,  CRC(6f311c83) SHA1(31372b2c056eacb747de0a706de3899d224f2c92) )
MEGADRIVE_ROM_LOAD( flashbj, "flashback (japan).bin",                                                                    0x000000, 0x180000,  CRC(b790e3b4) SHA1(5082180974a125b5f9c01c96410c0fdbfb707d2b) )
MEGADRIVE_ROM_LOAD( flashbu1, "flashback - the quest for identity (usa) (alt).bin",                                      0x000000, 0x180000,  CRC(33cd2b65) SHA1(225b405274d39541e07488fdd33de8f854624bf1) )
MEGADRIVE_ROM_LOAD( flashbu, "flashback - the quest for identity (usa).bin",                                             0x000000, 0x180000,  CRC(23a9616d) SHA1(bce40031f6adab48670c8a2d73e42f3a3dcba97c) )
MEGADRIVE_ROM_LOAD( flicky, "flicky (usa, europe).bin",                                                                  0x000000, 0x20000,   CRC(4291c8ab) SHA1(83d8bbf0a9b38c42a0bf492d105cc3abe9644a96) )
MEGADRIVE_ROM_LOAD( flink, "the misadventures of flink (europe).bin",                                                    0x000000, 0x100000,  CRC(bef9a4f4) SHA1(f37518380d3e41598cbff16519ca15fcb751de57) )
MEGADRIVE_ROM_LOAD( flintj, "flintstone (japan).bin",                                                                    0x000000, 0x80000,   CRC(920a3031) SHA1(a3dc66bb5b35916a188b2ef4b6b783f3c5ffc03c) )
MEGADRIVE_ROM_LOAD( flint, "flintstones, the (europe).bin",                                                              0x000000, 0x80000,   CRC(21845d61) SHA1(6350da9fa81a84a7a73d0238be135b1331147599) )
MEGADRIVE_ROM_LOAD( flintu, "flintstones, the (usa).bin",                                                                0x000000, 0x80000,   CRC(7c982c59) SHA1(5541579ffaee1570da8bdd6b2c20da2e395065b0) )
MEGADRIVE_ROM_LOAD( fluxeu, "flux (europe) (requires megacd) (program).bin",                                             0x000000, 0x40000,   CRC(2a1da08c) SHA1(6357f38db012278bc889c77c5780a82d535760fd) )
MEGADRIVE_ROM_LOAD( foreman, "foreman for real (world).bin",                                                             0x000000, 0x300000,  CRC(36248f90) SHA1(e1faa22d62652f7d7cf8c8b581f1df232f076f86) )
MEGADRIVE_ROM_LOAD( forgot, "forgotten worlds (world) (v1.1).bin",                                                       0x000000, 0x80000,   CRC(95513985) SHA1(79aadecc1069f47a2a8b4a0a1d55712d4f9cb8ef) )
MEGADRIVE_ROM_LOAD( forgot1, "mpr-12672.bin",                                                                            0x000000, 0x80000,   CRC(d0ee6434) SHA1(8b9a37c206c332ef23dc71f09ec40e1a92b1f83a) )
MEGADRIVE_ROM_LOAD( formula1, "formula one (usa).bin",                                                                   0x000000, 0x100000,  CRC(ccd73738) SHA1(8f8edd8e6846cbba1b46f8eb9015b195ccc4acf9) )
MEGADRIVE_ROM_LOAD( frankt, "frank thomas big hurt baseball (usa, europe).bin",                                          0x000000, 0x400000,  CRC(863e0950) SHA1(9c978aaab10e16be59558561b07a0c610c74b43e) )
MEGADRIVE_ROM_LOAD( frogger, "frogger (usa).bin",                                                                        0x000000, 0x80000,   CRC(ea2e48c0) SHA1(c0ccfec43ea859ab1e83293a38cfb302c0191719) )
MEGADRIVE_ROM_LOAD( slamdunk, "from tv animation slam dunk - kyougou makkou taiketsu! (japan).bin",                      0x000000, 0x200000,  CRC(cdf5678f) SHA1(7161a12c2a477cff8e29fa51403eea12c03180c7) )
MEGADRIVE_ROM_LOAD( funnga, "fun 'n' games (europe).bin",                                                                0x000000, 0x100000,  CRC(da4ab3cd) SHA1(3677dfe5450c0800d29cfff31f226389696bfb32) )
MEGADRIVE_ROM_LOAD( funngau, "fun 'n' games (usa).bin",                                                                  0x000000, 0x100000,  CRC(b5ae351d) SHA1(6085d3033edb32c468a63e0db95e5977e6853b48) )
MEGADRIVE_ROM_LOAD( funcar, "fun car rally (usa) (prototype).bin",                                                       0x000000, 0x100000,  CRC(42e27845) SHA1(779aba11bf77ca59e6ae981854805bf3fabe4b0e) )
MEGADRIVE_ROM_LOAD( fushigi, "fushigi no umi no nadia (japan).bin",                                                      0x000000, 0x100000,  CRC(4762062a) SHA1(bbcd26d5d1f1422051467aacaa835e1f38383f64) )
MEGADRIVE_ROM_LOAD( futbol, "futbol argentino 98 - pasion de multitudes (unlicensed).bin",                               0x000000, 0x200000,  CRC(5c015888) SHA1(44adb1cf6c1ce6e53314dc336168c5cf3313a739) )
MEGADRIVE_ROM_LOAD( fzsenk, "fz senki axis / final zone (japan, usa).bin",                                               0x000000, 0x80000,   CRC(731fa4a1) SHA1(e891e5c89fa13e3f9813d5a45feeed0cb7710acb) )
MEGADRIVE_ROM_LOAD( glocb, "g-loc air battle (world) (beta).bin",                                                        0x000000, 0x100000,  CRC(175c7e63) SHA1(ccbedc9d3c05e212e4e2bea6824999a3c1fd2006) )
MEGADRIVE_ROM_LOAD( gloc, "mpr-15326.bin",                                                                               0x000000, 0x100000,  CRC(f2af886e) SHA1(81bb08c4080ca9a8af65597d1c7b11ce902c8d9e) )
MEGADRIVE_ROM_LOAD( gadget, "gadget twins (usa).bin",                                                                    0x000000, 0x100000,  CRC(7ae5e248) SHA1(ce565d7df0abee02879b85866de0e1a609729ad8) )
MEGADRIVE_ROM_LOAD( gaiares, "gaiares (japan, usa).bin",                                                                 0x000000, 0x100000,  CRC(5d8bf68b) SHA1(f62e8be872dc116c4cc331c50ae63a63f013eb58) )
MEGADRIVE_ROM_LOAD( gaingrnd1, "gain ground (world) (alt).bin",                                                          0x000000, 0x80000,   CRC(83e7b8ae) SHA1(3cc501086f794ac663aad14d5c5a75b648041151) )
MEGADRIVE_ROM_LOAD( gaingrnd, "gain ground (world).bin",                                                                 0x000000, 0x80000,   CRC(8641a2ab) SHA1(a5017e44b5f470e0499f4a9b494385c567632864) )
MEGADRIVE_ROM_LOAD( galaxyf2, "mpr-14248b.bin",                                                                          0x000000, 0x100000,  CRC(d15f5c3c) SHA1(db1615fc239cb0ed9fdc792217964c33e1e700fc) )
MEGADRIVE_ROM_LOAD( galaxyf2a, "galaxy force ii (world).bin",                                                            0x000000, 0x100000,  CRC(cae883c5) SHA1(d4143bf5f49b0d03f4b8fe270c2ecc23fa6627e0) )
MEGADRIVE_ROM_LOAD( gamblerj, "gambler jiko chuushinha - katayama masayuki no mahjong doujou (japan).bin",               0x000000, 0x80000,   CRC(05650b7a) SHA1(42814f8921f42c023c1fff433a2e9399aaff5d2e) )
MEGADRIVE_ROM_LOAD( gameno, "game no kandume otokuyou (japan).bin",                                                      0x000000, 0x300000,  CRC(cdad7e6b) SHA1(31c66bd13abf4ae8271c09ec5286a0ee0289dbbc) )
// apparently a rip from Dream Passport 3 for Dreamcast, further info needed

MEGADRIVE_ROM_LOAD( gameto, "mpr-12750a.bin",                                                                            0x000000, 0x20000,   CRC(c185c819) SHA1(9673b519c16883c146924c347ec6764281fa3a57) )
MEGADRIVE_ROM_LOAD( garfield, "garfield - caught in the act (usa, europe).bin",                                          0x000000, 0x200000,  CRC(f0ff078e) SHA1(9fff7dea16c4d0e6c9d6dbaade20c7048bb485ec) )
MEGADRIVE_ROM_LOAD( gargoyle, "gargoyles (usa).bin",                                                                     0x000000, 0x300000,  CRC(2d965364) SHA1(2b76764ca1e5a26e406e42c5f8dbd5b8df915522) )
MEGADRIVE_ROM_LOAD( garoud, "garou densetsu - shukumei no tatakai (japan).bin",                                          0x000000, 0x180000,  CRC(bf3e3fa4) SHA1(b9eb37dbc77d0c6919ad78e587fa4c3403e9e9af) )
MEGADRIVE_ROM_LOAD( garou2, "mpr-16704+mpr-16710.bin",                                                                   0x000000, 0x300000,  CRC(2af4427f) SHA1(08eadaf6177d884b8e1fb66c5949850f10c5d77c) )
// two chips cart, needs a proper dump

MEGADRIVE_ROM_LOAD( gauntj, "gauntlet (japan) (en,ja).bin",                                                              0x000000, 0x100000,  CRC(f9872055) SHA1(30208982dd1f50634943d894e7458a556127f8e4) )
MEGADRIVE_ROM_LOAD( gaunto, "gauntlet iv (usa, europe) (en,ja) (august 1993).bin",                                       0x000000, 0x100000,  CRC(3bf46dce) SHA1(26c26ee2bb9571d51537d9328a5fd2a91b4e9dc1) )
MEGADRIVE_ROM_LOAD( gaunt, "gauntlet iv (usa, europe) (en,ja) (september 1993).bin",                                     0x000000, 0x100000,  CRC(f9d60510) SHA1(d28e22207121f0e2980dd409b4fb24f9fb8967ae) )
MEGADRIVE_ROM_LOAD( gemfire, "gemfire (usa).bin",                                                                        0x000000, 0x100000,  CRC(3d36135b) SHA1(98da5fdec3147edb75210ccf662601e502e23c31) )
MEGADRIVE_ROM_LOAD( general, "general chaos (usa, europe).bin",                                                          0x000000, 0x100000,  CRC(f1ecc4df) SHA1(aea1dfa67b0e583a3d367a67499948998cb92f56) )
MEGADRIVE_ROM_LOAD( generalj, "general chaos daikonsen (japan).bin",                                                     0x000000, 0x100000,  CRC(05cc7369) SHA1(9f205fc916df523ec84e0defbd0f1400a495cf8a) )
MEGADRIVE_ROM_LOAD( genelost, "generations lost (usa, europe).bin",                                                      0x000000, 0x100000,  CRC(131f36a6) SHA1(86af34198a8c67bd92fb03241d14861b2a9e270a) )
MEGADRIVE_ROM_LOAD( genghis, "genghis khan ii - clan of the gray wolf (usa).bin",                                        0x000000, 0x100000,  CRC(87a281ae) SHA1(4c1151413a261ad271543eeb64f512053f261a35) )
MEGADRIVE_ROM_LOAD( georgeko, "george foreman's ko boxing (europe).bin",                                                 0x000000, 0x100000,  CRC(bd556381) SHA1(c93298ee3ad6164ce497bd49d0ab6638854acb79) )
MEGADRIVE_ROM_LOAD( georgekou, "george foreman's ko boxing (usa).bin",                                                   0x000000, 0x100000,  CRC(e1fdc787) SHA1(62a953cc6ea8535aa7f3f59b40cff0e285d4392a) )
MEGADRIVE_ROM_LOAD( ghostbst, "ghostbusters (world) (v1.1).bin",                                                         0x000000, 0x80000,   CRC(792df93b) SHA1(6fceffee406679c0c8221a8b6cfad447695e99fb) )
MEGADRIVE_ROM_LOAD( ghostbst1, "ghostbusters (world).bin",                                                               0x000000, 0x80000,   CRC(00419da3) SHA1(2a1589781fc4aca2c1ba97ec9ecf1acf563b7bfb) )
MEGADRIVE_ROM_LOAD( ghouls, "mpr-12605a+mpr-12606a.bin",                                                                 0x000000, 0xa0000,   CRC(4f2561d5) SHA1(aab6d20f01db51576b1d7cafab46e613dddf7f8a) )
// two chips cart, needs a proper dump

MEGADRIVE_ROM_LOAD( ghouls1, "mpr-12605+mpr-12606.bin",                                                                  0x000000, 0xa0000,   CRC(d31bd910) SHA1(3a3e9eb5caaf6cfb75e99ed2843d58b3dda7284f) )
// two chips cart, needs a proper dump

MEGADRIVE_ROM_LOAD( global, "mpr-15237.bin",                                                                             0x000000, 0x100000,  CRC(5c218c6a) SHA1(daf1ded2439c626c0fd227550be6563cd1b09612) )
MEGADRIVE_ROM_LOAD( gods, "gods (europe).bin",                                                                           0x000000, 0x100000,  CRC(6c415016) SHA1(404bc6e67cd4942615ccb7bd894d780278ec6da7) )
MEGADRIVE_ROM_LOAD( godsj, "gods (japan).bin",                                                                           0x000000, 0x100000,  CRC(e4f50206) SHA1(804fd783c6fb7c226fbe4b227ed5c665d668ff57) )
MEGADRIVE_ROM_LOAD( godsub, "gods (usa) (beta).bin",                                                                     0x000000, 0x100000,  CRC(2c06bb64) SHA1(dd9c03eaf3160303775ca1bca048101614507203) )
MEGADRIVE_ROM_LOAD( godsu, "gods (usa).bin",                                                                             0x000000, 0x100000,  CRC(fd234ccd) SHA1(bfc84beba074c7dc58b0b4fcac73fffcf0c6b585) )
MEGADRIVE_ROM_LOAD( gaxe, "golden axe (world) (v1.1).bin",                                                               0x000000, 0x80000,   CRC(665d7df9) SHA1(2ce17105ca916fbbe3ac9ae3a2086e66b07996dd) )
MEGADRIVE_ROM_LOAD( gaxea, "mpr-12806.bin",                                                                              0x000000, 0x80000,   CRC(e8182b90) SHA1(564e51f6b7fe5281f281d5fcb66767ab83ecf7b9) )
MEGADRIVE_ROM_LOAD( gaxe2b, "golden axe ii (world) (beta).bin",                                                          0x000000, 0x80000,   CRC(e62ea1bb) SHA1(97e49394321e97b8488e6cf57bade4dc20aec0f4) )
MEGADRIVE_ROM_LOAD( gaxe2, "mpr-14272.bin",                                                                              0x000000, 0x80000,   CRC(725e0a18) SHA1(31f12a21af018cdf88b3f2170af5389b84fba7e7) )
MEGADRIVE_ROM_LOAD( gaxe3, "golden axe iii (japan).bin",                                                                 0x000000, 0x100000,  CRC(c7862ea3) SHA1(cd9ecc1df4e01d69af9bebcf45bbd944f1b17f9f) )
MEGADRIVE_ROM_LOAD( goofys, "goofy's hysterical history tour (usa).bin",                                                 0x000000, 0x100000,  CRC(4e1cc833) SHA1(caaeebc269b3b68e2a279864c44d518976d67d8b) )
MEGADRIVE_ROM_LOAD( gouket, "gouketsuji ichizoku (japan).bin",                                                           0x000000, 0x300000,  CRC(abe9c415) SHA1(db6b2046e8b6373f141e8c5db68450f2db377dd8) )
MEGADRIVE_ROM_LOAD( granada, "granada (japan, usa) (v1.1).bin",                                                          0x000000, 0x80000,   CRC(e89d1e66) SHA1(4eb8bbdb9ee8adcefaa202281cb88c19970437f7) )
MEGADRIVE_ROM_LOAD( granada1, "granada (japan, usa).bin",                                                                0x000000, 0x80000,   CRC(7f45719b) SHA1(6830d6ca1d7e2b220eb4431b42d33382e16e2791) )
MEGADRIVE_ROM_LOAD( grandslj, "grandslam - the tennis tournament '92 (japan).bin",                                       0x000000, 0x80000,   CRC(30cf37d0) SHA1(f51c4332235a9545b86a7b121ba30644c33be098) )
MEGADRIVE_ROM_LOAD( grandsl, "mpr-15208.bin",                                                                            0x000000, 0x80000,   CRC(8c2670de) SHA1(4fd5ce8207c4568da07bb28c6ab38a23084c801a) )
MEGADRIVE_ROM_LOAD( greatc, "great circus mystery - mickey to minnie magical adventure 2 (japan).bin",                   0x000000, 0x200000,  CRC(5aa0f3a0) SHA1(0341c231c268b61add922c5c3bf5ab2f9dfbf88f) )
MEGADRIVE_ROM_LOAD( greatcu, "great circus mystery starring mickey & minnie, the (usa).bin",                             0x000000, 0x200000,  CRC(14744883) SHA1(f2df9807fe2659e8f8e6ea43b2031a6abd980873) )
MEGADRIVE_ROM_LOAD( greatw, "great waldo search, the (usa).bin",                                                         0x000000, 0x100000,  CRC(8c5c93b8) SHA1(fe7601c92a67fb4deec28164c5fb0516ef0058c4) )
MEGADRIVE_ROM_LOAD( ghw, "mpr-16211.bin",                                                                                0x000000, 0x200000,  CRC(9778c181) SHA1(db28d70f9b744004cadd06eb77d6223534f59215) )
MEGADRIVE_ROM_LOAD( ghwj, "greatest heavyweights (japan).bin",                                                           0x000000, 0x200000,  CRC(7ef8b162) SHA1(1d5fe812df75e0dc1ff379c563058e078b839a09) )
MEGADRIVE_ROM_LOAD( ghwu, "greatest heavyweights (usa).bin",                                                             0x000000, 0x200000,  CRC(6e3621d5) SHA1(4ef7aec80003aab0e1d2260b7fc0d22d63d90038) )
MEGADRIVE_ROM_LOAD( greendog, "mpr-14993.bin",                                                                           0x000000, 0x80000,   CRC(c4820a03) SHA1(fe47514e9aeecabaed954a65d7241079dfec3d9e) )
MEGADRIVE_ROM_LOAD( grinds, "grind stormer (usa).bin",                                                                   0x000000, 0x100000,  CRC(7e6bef15) SHA1(8f93445e2d0b1798f680dda26a3d31f8aee88f01) )
MEGADRIVE_ROM_LOAD( growlu, "growl (usa).bin",                                                                           0x000000, 0x80000,   CRC(f60ef143) SHA1(63a08057fe9fd489590f39c5c06709266f625ab0) )
MEGADRIVE_ROM_LOAD( gunship, "gunship (europe).bin",                                                                     0x000000, 0x100000,  CRC(da1440c9) SHA1(3820eac00b888b17c02f271fab5fd08af99d5ef9) )
MEGADRIVE_ROM_LOAD( gunstar, "gunstar heroes (europe).bin",                                                              0x000000, 0x100000,  CRC(866ed9d0) SHA1(a7b265f49ec74f7febb8463c64535ceda15c8398) )
MEGADRIVE_ROM_LOAD( gunstarjs, "gunstar heroes (japan) (sample).bin",                                                    0x000000, 0x100000,  CRC(6f90b502) SHA1(a8d7f502d877b0ea3ff2431c94001e9d5dd84868) )
MEGADRIVE_ROM_LOAD( gunstarj, "gunstar heroes (japan).bin",                                                              0x000000, 0x100000,  CRC(1cfd0383) SHA1(64ae3ef9d063d21b290849809902c221f6ab10d5) )
MEGADRIVE_ROM_LOAD( gunstaru, "gunstar heroes (usa).bin",                                                                0x000000, 0x100000,  CRC(b813cf0d) SHA1(7721e302555f16e3242ad4a47ed87a1de1882122) )
MEGADRIVE_ROM_LOAD( gynoug, "mpr-14439.bin",                                                                             0x000000, 0x80000,   CRC(03405102) SHA1(fada42bed3a97e1d1b0c6128ddb1f202a50cb1b0) )
MEGADRIVE_ROM_LOAD( gynougj, "gynoug (japan).bin",                                                                       0x000000, 0x80000,   CRC(1b69241f) SHA1(9b99b109f5f1f221f08672a7c4752a94067b62b0) )
MEGADRIVE_ROM_LOAD( harddr, "mpr-13489.bin",                                                                             0x000000, 0x40000,   CRC(3225baaf) SHA1(e15a3e704900b9736bc274bcb49d4421f4a3605b) )
MEGADRIVE_ROM_LOAD( hard94, "hardball '94 (usa, europe).bin",                                                            0x000000, 0x200000,  CRC(ea9c4878) SHA1(eceeecb3d520f9b350e41d0dd010abefbcfbbdab) )
MEGADRIVE_ROM_LOAD( hard95, "hardball '95 (usa).bin",                                                                    0x000000, 0x300000,  CRC(ed10bc9e) SHA1(6a63fba59add9ba8e1845cbfcf4722833893113f) )
MEGADRIVE_ROM_LOAD( hard3, "hardball iii (usa).bin",                                                                     0x000000, 0x200000,  CRC(a4f2f011) SHA1(88812ae494288fbf3dda86ccc69161ad960a2e3b) )
MEGADRIVE_ROM_LOAD( hard, "hardball! (usa).bin",                                                                         0x000000, 0x100000,  CRC(bd1b9a04) SHA1(42d42af36b4a69f0adb38aaa7fec32eb8c44c349) )
MEGADRIVE_ROM_LOAD( haunting, "haunting starring polterguy (usa, europe).bin",                                           0x000000, 0x200000,  CRC(c9fc876d) SHA1(d6227ef00fdded9184676edf5dd2f6cae9b244a5) )
MEGADRIVE_ROM_LOAD( havoc, "capt'n havoc (europe).bin",                                                                  0x000000, 0x100000,  CRC(76e6d20d) SHA1(6e0344aace03b703cb05a9a0e10c47ebe404a247) )
MEGADRIVE_ROM_LOAD( headon, "head-on soccer (usa).bin",                                                                  0x000000, 0x200000,  CRC(dcffa327) SHA1(440835a94ea7c23fc0b0b86564020337a99514f1) )
MEGADRIVE_ROM_LOAD( heavyn, "heavy nova (usa).bin",                                                                      0x000000, 0x100000,  CRC(f6b6a9d8) SHA1(f5bda60615c7436a40b5a31a726099583ed85413) )
MEGADRIVE_ROM_LOAD( heavyu, "heavy unit - mega drive special (japan).bin",                                               0x000000, 0x80000,   CRC(1acbe608) SHA1(198e243cc21e4aea97e2c6aca376cb2ae70bc1c9) )
MEGADRIVE_ROM_LOAD( heitao, "hei tao 2 - super big 2 (china) (unlicensed).bin",                                          0x000000, 0x100000,  CRC(31449113) SHA1(e1e4c439c5c22fa5cfcecaab421c55bf1746b5de) )
MEGADRIVE_ROM_LOAD( hellfire, "mpr-14430.bin",                                                                           0x000000, 0x80000,   CRC(cf30acec) SHA1(3cee325f4e8d12157b20c4ca7093bf806f5f9148) )
MEGADRIVE_ROM_LOAD( hellfirej, "hellfire (japan).bin",                                                                   0x000000, 0x80000,   CRC(8e5e13ba) SHA1(17bbf3da65757cad3c7ff82c094e54cc68fd2f73) )
MEGADRIVE_ROM_LOAD( hellfireu, "hellfire (usa).bin",                                                                     0x000000, 0x80000,   CRC(184018f9) SHA1(9a91bef8a07f709e1f79f8519da79eac34d1796d) )
MEGADRIVE_ROM_LOAD( hercul, "hercules (unlicensed).bin",                                                                 0x000000, 0x100000,  CRC(ff75d9d0) SHA1(6d89f589aeefaceda85a6f592d5d5b9062472b7a) )
MEGADRIVE_ROM_LOAD( herc2, "hercules 2 (unlicensed).bin",                                                                0x000000, 0x200000,  CRC(292623db) SHA1(7104a37f588f291b85eb8f62685cb1111373572c) )
MEGADRIVE_ROM_LOAD( herzogj, "herzog zwei (japan).bin",                                                                  0x000000, 0x80000,   CRC(4cf676b3) SHA1(851be7abad64a4fb05e4c51ce26fbe5efe12ea42) )
MEGADRIVE_ROM_LOAD( herzog, "herzog zwei (usa, europe).bin",                                                             0x000000, 0x80000,   CRC(a605b65b) SHA1(8f7262102c2b2334f0bc88ee6fd6b08797919176) )
MEGADRIVE_ROM_LOAD( highseas, "high seas havoc (usa).bin",                                                               0x000000, 0x100000,  CRC(17be551c) SHA1(0dc1969098716ba332978b89356f62961417682b) )
MEGADRIVE_ROM_LOAD( hitthe, "hit the ice (usa).bin",                                                                     0x000000, 0x80000,   CRC(85b23606) SHA1(d0eec70787362b415ffdcf09524e0e6cf0f9f910) )
MEGADRIVE_ROM_LOAD( hokuto, "hokuto no ken - shin seikimatsu kyuuseishu densetsu (japan).bin",                           0x000000, 0x80000,   CRC(1b6585e7) SHA1(7d489e1087a0816e2091261f6550f42a61474915) )
MEGADRIVE_ROM_LOAD( homeab, "home alone (usa) (beta).bin",                                                               0x000000, 0x100000,  CRC(3a235fb9) SHA1(1ace5abd524c6ab5b434374c51f64f59dbd6ec7a) )
MEGADRIVE_ROM_LOAD( homea, "mpr-14996.bin",                                                                              0x000000, 0x80000,   CRC(aa0d4387) SHA1(c3794ef25d53cb7811dd0df73ba6fadd4cecb116) )
MEGADRIVE_ROM_LOAD( homea2, "home alone 2 - lost in new york (usa).bin",                                                 0x000000, 0x80000,   CRC(cbf87c14) SHA1(24232a572b7eabc3e0ed5f483042a7085bd77c48) )
MEGADRIVE_ROM_LOAD( honoon, "mpr-14856.bin",                                                                             0x000000, 0x80000,   CRC(630f07c6) SHA1(ebdf20fd8aaeb3c7ec97302089b3330265118cf0) )
MEGADRIVE_ROM_LOAD( hookus, "hook (usa).bin",                                                                            0x000000, 0x100000,  CRC(2c48e712) SHA1(67031af6ec4b771bd8d69a44c9945562a063593e) )
MEGADRIVE_ROM_LOAD( huamul, "hua mu lan - mulan (china) (unlicensed).bin",                                               0x000000, 0x200000,  CRC(796882b8) SHA1(d8936c1023db646e1e20f9208b68271afbd6dbf4) )
MEGADRIVE_ROM_LOAD( huanle, "huan le tao qi shu - smart mouse (china) (unlicensed).bin",                                 0x000000, 0x80000,   CRC(decdf740) SHA1(df7a2527875317406b466175f0614d343dd32117) )
MEGADRIVE_ROM_LOAD( humans, "humans, the (usa).bin",                                                                     0x000000, 0x100000,  CRC(a0cf4366) SHA1(21ad6238edf0aa0563782ef17439b0c73a668059) )
MEGADRIVE_ROM_LOAD( hurric, "hurricanes (europe).bin",                                                                   0x000000, 0x200000,  CRC(deccc874) SHA1(87ea7e663a0573f02440f9c5661b1df91b4d3ffb) )
MEGADRIVE_ROM_LOAD( hybridfb, "hybrid front, the (japan) (beta).bin",                                                    0x000000, 0x300000,  CRC(04f02687) SHA1(b6e72b69a22869f966c52e7d58d146bc48b5eb84) )
MEGADRIVE_ROM_LOAD( hybridf, "hybrid front, the (japan).bin",                                                            0x000000, 0x200000,  CRC(a1f1cfe7) SHA1(ed592c78ef60d91a6c5723d11cd553d3798524e1) )
MEGADRIVE_ROM_LOAD( hyokko, "hyokkori hyoutanjima - daitouryou wo mezase! (japan).bin",                                  0x000000, 0x80000,   CRC(72253bdb) SHA1(32a73559f84dbab5460bfd1e266acfc8f4391ae4) )
MEGADRIVE_ROM_LOAD( hyperd, "hyper dunk (europe).bin",                                                                   0x000000, 0x200000,  CRC(f27c576a) SHA1(91560ddf53f50bb5b9f4b48be906ce27e9c05742) )
MEGADRIVE_ROM_LOAD( hyperdjb, "hyper dunk - the playoff edition (japan) (beta).in",                                      0x000000, 0x200000,  CRC(db124bbb) SHA1(87a2d58614fab2799e2a4456a08436fc7acc745b) )
MEGADRIVE_ROM_LOAD( hyperdj, "fz009a1.bin",                                                                              0x000000, 0x200000,  CRC(5baf53d7) SHA1(ef5c13926ee6eb32593b9e750ec342b98f48d1ef) )
MEGADRIVE_ROM_LOAD( hyperm, "hyper marbles (japan) (seganet).bin",                                                       0x000000, 0x40000,   CRC(83bb2799) SHA1(b2c5c78084b988de1873c89eedfa984124404c0c) )
MEGADRIVE_ROM_LOAD( hyperma, "hyper marbles (japan) (game no kandume megacd rip).bin",                                   0x000000, 0x40000,   CRC(9ed72146) SHA1(d8dc95512aec41db7c758d4c971dc0194ddb960a) )
MEGADRIVE_ROM_LOAD( ikazus, "ikazuse! koi no doki doki penguin land md (japan) (seganet).bin",                           0x000000, 0x40000,   CRC(0053bfd6) SHA1(37bc8429a6259582b840e219a2cf412cea5fc873) )
MEGADRIVE_ROM_LOAD( imgint, "img international tour tennis (usa, europe).bin",                                           0x000000, 0x200000,  CRC(e04ffc2b) SHA1(1f1b410d17b39851785dee3eee332fff489db395) )
MEGADRIVE_ROM_LOAD( immortal, "immortal, the (usa, europe).bin",                                                         0x000000, 0x100000,  CRC(f653c508) SHA1(71fc189fe2ba5687e9d45b68830baed27194f627) )
MEGADRIVE_ROM_LOAD( incredb, "incredible crash dummies, the (usa) (beta).bin",                                           0x000000, 0x100000,  CRC(623a920f) SHA1(6c1db6d8ec7ace551b70ee60026e4a553d50f964) )
MEGADRIVE_ROM_LOAD( incred, "mpr-16259.bin",                                                                             0x000000, 0x100000,  CRC(1f6e574a) SHA1(ca0115dd843e072815c4be86a7a491b26e3c4762) )
MEGADRIVE_ROM_LOAD( incrhulk, "incredible hulk, the (usa, europe).bin",                                                  0x000000, 0x200000,  CRC(84a5a2dc) SHA1(41247e3166ec42b7cada33615da49e53d48fd809) )
MEGADRIVE_ROM_LOAD( indycrus, "mpr-15207.bin",                                                                           0x000000, 0x100000,  CRC(eb8f4374) SHA1(70b5b4bc3f6d7ee7fbc77489bcfa4a96a831b88d) )
MEGADRIVE_ROM_LOAD( indycrusu, "indiana jones and the last crusade (usa).bin",                                           0x000000, 0x100000,  CRC(3599a3fd) SHA1(82758a8a47c4f1f0e990bd50b773b2c4300f616e) )
MEGADRIVE_ROM_LOAD( indy, "indiana jones' greatest adventures (release candidate).bin",                                  0x000000, 0x200000,  CRC(9a01974e) SHA1(d59bb77b83cf912e3cb8a7598cabcf87a273e429) )
MEGADRIVE_ROM_LOAD( insectxj, "insector x (japan, korea).bin",                                                           0x000000, 0x80000,   CRC(9625c434) SHA1(204351b97aff579853cdba646b427ea299b1eddf) )
MEGADRIVE_ROM_LOAD( insectx, "insector x (usa).bin",                                                                     0x000000, 0x80000,   CRC(70626304) SHA1(4f4ecff167d5ada699b0b1d4b0e547b5ed9964d5) )
MEGADRIVE_ROM_LOAD( instchb, "instruments of chaos starring young indiana jones (usa) (beta).bin",                       0x000000, 0x180000,  CRC(ad6c2050) SHA1(63f5ac7dddb166fba76d74c58daf320568d6b016) )
MEGADRIVE_ROM_LOAD( instch, "instruments of chaos starring young indiana jones (usa).bin",                               0x000000, 0x100000,  CRC(4e384ef0) SHA1(7ffd31a8fd0f2111ef3dcce1b8493e6ea6e24deb) )
MEGADRIVE_ROM_LOAD( intrugby, "international rugby (europe).bin",                                                        0x000000, 0x80000,   CRC(d97d1699) SHA1(8cea50f668fbfba7dd10244a001172c1e648c352) )
MEGADRIVE_ROM_LOAD( issdx, "international superstar soccer deluxe (europe).bin",                                         0x000000, 0x200000,  CRC(9bb3b180) SHA1(ccc60352b43f8c3d536267dd05a8f2c0f3b73df6) )
MEGADRIVE_ROM_LOAD( iraqwar, "iraq war 2003 (unlicensed).bin",                                                           0x000000, 0x100000,  CRC(49dd6f52) SHA1(cc8b69debd68ba7c6d72d47d4c33530a1e7ef07c) )
MEGADRIVE_ROM_LOAD( ishido, "mb834200a.bin",                                                                             0x000000, 0x20000,   CRC(b1de7d5e) SHA1(3bd159e323d86e69031bf1ee9febeb6f9bb078d4) )
MEGADRIVE_ROM_LOAD( itcame, "it came from the desert (usa).bin",                                                         0x000000, 0x80000,   CRC(25afb4f7) SHA1(245816c9744552856fc2745253db7b37ce9d9251) )
MEGADRIVE_ROM_LOAD( itchy, "itchy and scratchy game, the (usa) (prototype).bin",                                         0x000000, 0x100000,  CRC(81b7725d) SHA1(860d9bd7501d59da8304ab284b385afa4def13a0) )
MEGADRIVE_ROM_LOAD( izzyqst, "izzy's quest for the olympic rings (usa, europe).bin",                                     0x000000, 0x200000,  CRC(77b416e4) SHA1(0f05e0c333d6ee58a254ee420a70d6020488ac54) )
MEGADRIVE_ROM_LOAD( jlcs, "j. league champion soccer (japan).bin",                                                       0x000000, 0x80000,   CRC(453c405e) SHA1(c28d8dac0f5582c3c2754342605764ee6e5b8af3) )
MEGADRIVE_ROM_LOAD( jlpsa, "j. league pro striker (japan) (v1.0).bin",                                                   0x000000, 0x100000,  CRC(ec229156) SHA1(d3dcc24e50373234988061d3ef56c16d28e580ad) )
MEGADRIVE_ROM_LOAD( jlps, "j. league pro striker (japan) (v1.3).bin",                                                    0x000000, 0x100000,  CRC(2d5b7a11) SHA1(1e437182fab2980156b101a53623c3c2f27c3a6c) )
MEGADRIVE_ROM_LOAD( jlps2, "j. league pro striker 2 (japan).bin",                                                        0x000000, 0x200000,  CRC(9fe71002) SHA1(924f1ae3d90bec7326a5531cd1d598cdeba30d36) )
MEGADRIVE_ROM_LOAD( jlpsfs, "j. league pro striker final stage (japan).bin",                                             0x000000, 0x200000,  CRC(e35e25fb) SHA1(74e4a3ac4b93e25ace6ec8c3818e0df2390cffa2) )
MEGADRIVE_ROM_LOAD( jlpsp, "j. league pro striker perfect (japan).bin",                                                  0x000000, 0x100000,  CRC(0abed379) SHA1(7cfd8c9119d0565ee9a7708dc46bb34dd3258e37) )
MEGADRIVE_ROM_LOAD( jacknick, "jack nicklaus' power challenge golf (usa, europe).bin",                                   0x000000, 0x100000,  CRC(5545e909) SHA1(8d7edfe87da732ecd9820a6afbb9c5700cce43b2) )
MEGADRIVE_ROM_LOAD( jamesb, "mpr-13262.bin",                                                                             0x000000, 0x80000,   CRC(87bbcf2a) SHA1(617b61da9bceb6f4d8362d074dc1dad3f7304584) )
MEGADRIVE_ROM_LOAD( jb007, "mpr-15312a.bin",                                                                             0x000000, 0x80000,   CRC(291a3e4b) SHA1(e880bfad98d0e6f9b73db63ea85fb0a103b0e1e7) )
MEGADRIVE_ROM_LOAD( jb007u, "james bond 007 - the duel (usa).bin",                                                       0x000000, 0x80000,   CRC(4e614548) SHA1(847de1168bf737ac7faa4f77eb0749738014ed87) )
MEGADRIVE_ROM_LOAD( jpond, "jam03.bin",                                                                                  0x000000, 0x80000,   CRC(d0e7b466) SHA1(b4e1c945c3ccea2e76b296d6694c0931a1ec1310) )
MEGADRIVE_ROM_LOAD( jpond3, "james pond 3 - operation starfish (usa, europe).bin",                                       0x000000, 0x200000,  CRC(26f64b2a) SHA1(df7fca887e7988e24ab2d08b015c6db2902fe571) )
MEGADRIVE_ROM_LOAD( robocodj, "james pond ii - codename robocod (japan, korea).bin",                                     0x000000, 0x80000,   CRC(98794702) SHA1(fa7d4e77fd98eb1fc9f8e1d66269bf86881c695d) )
MEGADRIVE_ROM_LOAD( jammit, "jammit (usa).bin",                                                                          0x000000, 0x200000,  CRC(d91b52b8) SHA1(2f43bfd04f563a67f4a2c8b8e36c541c19913a50) )
MEGADRIVE_ROM_LOAD( janout, "janou touryuumon (japan).bin",                                                              0x000000, 0x100000,  CRC(b5ef97c6) SHA1(4f307e0146e944fbbd4537f5cdc5da136204fc9b) )
MEGADRIVE_ROM_LOAD( janout1, "janou touryuumon (japan) (alt).bin",                                                       0x000000, 0x100000,  CRC(7011e8eb) SHA1(620ecac6c340e617bbde922bf742a9e6d7f8d786) )
// Xacrow's dump info report only the other CRC, this is probably a bad dump

MEGADRIVE_ROM_LOAD( jantei, "jantei monogatari (japan).bin",                                                             0x000000, 0x100000,  CRC(8a1b19ad) SHA1(8e109961a89e1366c83808fb0bb33333f88a82b7) )
MEGADRIVE_ROM_LOAD( jashin, "jashin draxos (japan, korea).bin",                                                          0x000000, 0x100000,  CRC(1ea07af2) SHA1(9652b8ed0e36ee17ef6c0b007ca3ad237c13a7a0) )
MEGADRIVE_ROM_LOAD( jellyboy, "jelly boy (europe) (prototype).bin",                                                      0x000000, 0x100000,  CRC(7cfadc16) SHA1(75be8d5b305e669848b4a5a48cdcfa43b951dc20) )
MEGADRIVE_ROM_LOAD( jennif, "jennifer capriati tennis (usa).bin",                                                        0x000000, 0x80000,   CRC(ab2abc8e) SHA1(8d72ea31c87b1a229098407e9c59a46e65f996a2) )
MEGADRIVE_ROM_LOAD( jeopardy, "jeopardy! (usa).bin",                                                                     0x000000, 0x80000,   CRC(56cff3f1) SHA1(2a5c5ec5648b1d29768c808ab3c21118dc77fa27) )
MEGADRIVE_ROM_LOAD( jeopardd, "jeopardy! deluxe (usa).bin",                                                              0x000000, 0x80000,   CRC(25e2f9d2) SHA1(1817433ac701c407eb748e87f01a0bc8639e3ac2) )
MEGADRIVE_ROM_LOAD( jeopards, "jeopardy! sports edition (usa).bin",                                                      0x000000, 0x80000,   CRC(13f924d2) SHA1(bc20e31c387ed091fe0288e86ea9b421063953bb) )
MEGADRIVE_ROM_LOAD( jerryg, "jerry glanville's pigskin footbrawl (usa).bin",                                             0x000000, 0x100000,  CRC(e7f48d30) SHA1(fbcd0e7cb8dfc327b6d019afbdde9728f656957a) )
MEGADRIVE_ROM_LOAD( jewelmsj, "jewel master (japan).bin",                                                                0x000000, 0x80000,   CRC(2cf6926c) SHA1(52fbcf9902a4a3f70aeb9c3df31019e07e679ea8) )
MEGADRIVE_ROM_LOAD( jewelms, "mpr-14118a.bin",                                                                           0x000000, 0x80000,   CRC(cee98813) SHA1(9a6e4ca71546e798e1c98e78c4ab72aba46374c5) )
MEGADRIVE_ROM_LOAD( jimpow, "jim power - the arcade game (usa) (prototype) (bad dump).bin",                              0x000000, 0x100000, BAD_DUMP CRC(1cf3238b) SHA1(38adc1f792b06637e109d4b76fbfbf57623faf3b) )
MEGADRIVE_ROM_LOAD( jimmyw, "jimmy white's whirlwind snooker (europe).bin",                                              0x000000, 0x80000,   CRC(0aef5b1f) SHA1(040c810ccbe6310d369aa147471213d898ec2ad5) )
MEGADRIVE_ROM_LOAD( jiujim, "jiu ji ma jiang ii - ye yan bian (china) (unlicensed).bin",                                 0x000000, 0x100000,  CRC(e9829b22) SHA1(c112602c58f9867377bb6a1204d1d7e49a50aa10) )
MEGADRIVE_ROM_LOAD( joemac, "joe & mac (usa).bin",                                                                       0x000000, 0x100000,  CRC(85bcc1c7) SHA1(d238ecdbc76affb0b92946a1ee984399b6e8fe27) )
MEGADRIVE_ROM_LOAD( joemont, "joe montana football (world).bin",                                                         0x000000, 0x80000,   CRC(8aa6a1dd) SHA1(64b03ad0b17c022c831d057b9d6087c3b719147a) )
MEGADRIVE_ROM_LOAD( joemont2, "joe montana ii sports talk football (world) (rev a).bin",                                 0x000000, 0x100000,  CRC(a45da893) SHA1(ce006ff3b9bcd71fef4591a63a80d887004abe77) )
MEGADRIVE_ROM_LOAD( joemont2a, "joe montana ii sports talk football (world).bin",                                        0x000000, 0x100000,  CRC(f2363a4a) SHA1(3925623fafb79a7e8467e9b6fe70361c147cbfd3) )
MEGADRIVE_ROM_LOAD( madden92, "john madden football '92 (usa, europe).bin",                                              0x000000, 0x80000,   CRC(046e3945) SHA1(1a4c1dcc2de5018142a770f753ff42667b83e5be) )
MEGADRIVE_ROM_LOAD( madden93, "john madden football '93 (usa, europe).bin",                                              0x000000, 0x100000,  CRC(ca323b3e) SHA1(59d2352ecb31bc1305128a9d8df894a3bfd684cf) )
MEGADRIVE_ROM_LOAD( madd93ce, "john madden football '93 - championship edition (usa).bin",                               0x000000, 0x100000,  CRC(ca534b1a) SHA1(417acacb9f9ef90d08b3cfb81972a9d8b56f4293) )
MEGADRIVE_ROM_LOAD( madden, "john madden football (usa, europe).bin",                                                    0x000000, 0x80000,   CRC(90fb8818) SHA1(10682f1763711b281542fcd5e192e1633809dc75) )
MEGADRIVE_ROM_LOAD( maddenj, "john madden football - pro football (japan).bin",                                          0x000000, 0x80000,   CRC(0460611c) SHA1(c4cf3681d86861a823fa3e7ffe0cd451fbafcee6) )
MEGADRIVE_ROM_LOAD( jordanb, "jordan vs bird (usa, europe) (v1.1).bin",                                                  0x000000, 0x80000,   CRC(4d3ddd7c) SHA1(4c3c6696157a3629f10aa60626f504cd64c36a58) )
MEGADRIVE_ROM_LOAD( jordanb1, "jordan vs bird (usa, europe).bin",                                                        0x000000, 0x80000,   CRC(22d77e6d) SHA1(977430962510867b16c113cff1436cb75c0485ac) )
MEGADRIVE_ROM_LOAD( jordanbj, "jordan vs bird - one on one (japan).bin",                                                 0x000000, 0x80000,   CRC(8837e896) SHA1(769d7ed746ccbec0bcbe271f81767ffcccd36cea) )
MEGADRIVE_ROM_LOAD( joshua, "joshua & the battle of jericho (usa) (unlicensed).bin",                                     0x000000, 0x40000,   CRC(da9e25aa) SHA1(a6c47babc7d84f8f411e77b9acdf01753d3a5951) )
MEGADRIVE_ROM_LOAD( judgeua, "judge dredd (usa) (beta) (alt).bin",                                                       0x000000, 0x200000,  CRC(e649f784) SHA1(21210afaa9a53762108936e28380a9075b7c5c05) )
MEGADRIVE_ROM_LOAD( judgeub, "judge dredd (usa) (beta).bin",                                                             0x000000, 0x200000,  CRC(8d46f4da) SHA1(84bb0ee7c42612abfd41c5676e7ecb1b828ee42a) )
MEGADRIVE_ROM_LOAD( judge, "judge dredd (world).bin",                                                                    0x000000, 0x200000,  CRC(ea342ed8) SHA1(6553b4eabcba9d2823cbffd08554408a9f1067c9) )
MEGADRIVE_ROM_LOAD( jujude, "juju densetsu / toki - going ape spit (world) (rev a).bin",                                 0x000000, 0x80000,   CRC(7362c3f4) SHA1(77270a6ded838d0263284dcc075aa4b2b2aef234) )
MEGADRIVE_ROM_LOAD( jujude1, "juju densetsu / toki - going ape spit (world).bin",                                        0x000000, 0x80000,   CRC(d09b1ef1) SHA1(1a94cde6392d385271797a6b21ad0eaad920a8da) )
MEGADRIVE_ROM_LOAD( junction, "junction (japan, usa).bin",                                                               0x000000, 0x80000,   CRC(94cdce8d) SHA1(7981e5764458f5230184c7b2cf77469e1ed34270) )
MEGADRIVE_ROM_LOAD( jungle, "jungle book, the (europe).bin",                                                             0x000000, 0x200000,  CRC(b9709a99) SHA1(b96dcaf595a713eeec54257e8cee6306d7748baf) )
MEGADRIVE_ROM_LOAD( jungleu, "jungle book, the (usa).bin",                                                               0x000000, 0x200000,  CRC(3fb6d92e) SHA1(306021e52501c9d51afd9c51acf384356c84ffbf) )
MEGADRIVE_ROM_LOAD( jstrikeb, "jungle strike (usa) (beta).bin",                                                          0x000000, 0x200000,  CRC(0cd540d4) SHA1(b76be43101e72100b63c107d1710fa8f9ad4cfd6) )
MEGADRIVE_ROM_LOAD( jstrike, "jungle strike (usa, europe).bin",                                                          0x000000, 0x200000,  CRC(a5d29735) SHA1(7a7568e39341b1bb218280ee05c2b37c273317b5) )
MEGADRIVE_ROM_LOAD( jstrikej, "jungle strike - uketsugareta kyouki (japan).bin",                                         0x000000, 0x200000,  CRC(ba7a870b) SHA1(e35c554c52ce31de837c0e73bf2014720657f4d5) )
MEGADRIVE_ROM_LOAD( junker, "junker's high (japan) (beta) (bad dump).bin",                                               0x000000, 0x100000, BAD_DUMP CRC(23534949) SHA1(81a9b6d29878b4e9313a887b5349c12846b9d77f) )
MEGADRIVE_ROM_LOAD( jurass, "jurassic park (europe).bin",                                                                0x000000, 0x200000,  CRC(448341f6) SHA1(2dca183ddb79805b6300d8fcca1163fce88dd9db) )
MEGADRIVE_ROM_LOAD( jurassj, "jurassic park (japan).bin",                                                                0x000000, 0x200000,  CRC(ec8e5783) SHA1(d5d0e2ed4c00435ec4a343583bb399b07927de21) )
MEGADRIVE_ROM_LOAD( jurassub, "jurassic park (usa) (beta).bin",                                                          0x000000, 0x200000,  CRC(cf890eed) SHA1(ac2097b5f2a30787d7ca1ed8ab4eac7f4be77f0f) )
MEGADRIVE_ROM_LOAD( jurassu, "jurassic park (usa).bin",                                                                  0x000000, 0x200000,  CRC(7b31deef) SHA1(e7a1f49d362b5c7e13d9c7942d4a83fe003cfbd2) )
MEGADRIVE_ROM_LOAD( jprmpa, "jurassic park - rampage edition (prototype - jul 08, 1994).bin",                            0x000000, 0x200000,  CRC(f4c78bd0) SHA1(7dca5ec1440222ce38f085663d0ab6539693feaa) )
MEGADRIVE_ROM_LOAD( jprmpb, "jurassic park - rampage edition (prototype - jul 13, 1994).bin",                            0x000000, 0x200000,  CRC(8475a105) SHA1(b840cc1b00bf2f7e75320c1395da3dceaa637071) )
MEGADRIVE_ROM_LOAD( jprmpc, "jurassic park - rampage edition (prototype - jul 14, 1994).bin",                            0x000000, 0x200000,  CRC(9bf0e28d) SHA1(534eea52e7c6d8d0fc0a93480c89f89568196d36) )
MEGADRIVE_ROM_LOAD( jprmpd, "jurassic park - rampage edition (prototype - jul 15, 1994).bin",                            0x000000, 0x200000,  CRC(d2202be5) SHA1(b10fae841b410525ac83e729983f3e0cda2e7365) )
MEGADRIVE_ROM_LOAD( jprmpe, "jurassic park - rampage edition (prototype - jul 17, 1994).bin",                            0x000000, 0x200000,  CRC(2eae6140) SHA1(2d817ad10e9b6b93b6eee23a76f788a73827bbc5) )
MEGADRIVE_ROM_LOAD( jprmpf, "jurassic park - rampage edition (prototype - jul 18, 1994).bin",                            0x000000, 0x200000,  CRC(1a62b826) SHA1(cbb44d5d5da1263a4524c747be9afd0aa8547a63) )
MEGADRIVE_ROM_LOAD( jprmpg, "jurassic park - rampage edition (prototype - jun 20, 1994).bin",                            0x000000, 0x1f6e60,  CRC(1280fa5c) SHA1(f8f52d23dcd36e8167ec273cf9978ae44f21658b) )
MEGADRIVE_ROM_LOAD( jprmph, "jurassic park - rampage edition (prototype - jun 22, 1994).bin",                            0x000000, 0x200000,  CRC(53582f2b) SHA1(6607cdeb4a5ddb229201f1417d3e693063094a7d) )
MEGADRIVE_ROM_LOAD( jprmpi, "jurassic park - rampage edition (prototype - jun 30, 1994).bin",                            0x000000, 0x200000,  CRC(bff4b396) SHA1(f1f763e087065483ed7ff53f95b99bac16bb8484) )
MEGADRIVE_ROM_LOAD( jprmp, "jurassic park - rampage edition (usa, europe).bin",                                          0x000000, 0x200000,  CRC(98b4aa1b) SHA1(535c78d91f76302a882d69ff40b3d0f030a5b6ae) )
MEGADRIVE_ROM_LOAD( justicel, "justice league task force (world).bin",                                                   0x000000, 0x300000,  CRC(2a60ebe9) SHA1(1be166689726b98fc5924028e736fc8007f958ef) )
MEGADRIVE_ROM_LOAD( juuouki, "juuouki (japan) (v1.1).bin",                                                               0x000000, 0x80000,   CRC(b2233e87) SHA1(d0713d6da20145fe9e13b56a103fce23bc6051d7) )
MEGADRIVE_ROM_LOAD( juuouki1, "juuouki (japan).bin",                                                                     0x000000, 0x80000,   CRC(1b7c96c0) SHA1(bf5782b6f25dcf2bd1ff987e2821441c77da3c5d) )
MEGADRIVE_ROM_LOAD( kagekij, "ka-ge-ki (japan).bin",                                                                     0x000000, 0x100000,  CRC(391866a1) SHA1(9424e8b759609004748dd6cd4779f917211264ae) )
MEGADRIVE_ROM_LOAD( kageki, "ka-ge-ki - fists of steel (usa).bin",                                                       0x000000, 0x100000,  CRC(effc0fa6) SHA1(893b1bae5242f25494b6de64a861b1aa1dc6cf14) )
MEGADRIVE_ROM_LOAD( kawab, "kawasaki superbike challenge (usa) (beta).bin",                                              0x000000, 0x100000,  CRC(55934d1b) SHA1(a66446d5c3d07ce211d7198faf4ea3ff6dbfa0b9) )
MEGADRIVE_ROM_LOAD( kawa, "kawasaki superbike challenge (usa, europe).bin",                                              0x000000, 0x100000,  CRC(631cc8e9) SHA1(fa7e07bbab70a7b5c32a0f2713494b2c10ce8e1e) )
MEGADRIVE_ROM_LOAD( kickbo, "mpr-15333.bin",                                                                             0x000000, 0x100000,  CRC(9bdc230c) SHA1(c3f74c01e8124d5d08f817ff6e0c9416545b8302) )
MEGADRIVE_ROM_LOAD( kickoff3, "kick off 3 - european challenge (europe).bin",                                            0x000000, 0x100000,  CRC(bc37401a) SHA1(e449cfd4f9d59cf28b4842d465022a399964d0d6) )
MEGADRIVE_ROM_LOAD( kidcha, "kid chameleon (usa, europe).bin",                                                           0x000000, 0x100000,  CRC(ce36e6cc) SHA1(28b904000b2863b6760531807760b571f1a5fc1d) )
MEGADRIVE_ROM_LOAD( kidouk, "kidou keisatsu patlabor - 98-shiki kidou seyo! (japan).bin",                                0x000000, 0x80000,   CRC(21a0e749) SHA1(5595422530e6891042a4a005d11b79af7f09fe9b) )
MEGADRIVE_ROM_LOAD( killin, "killing game show, the (j).bin",                                                            0x000000, 0x80000,   CRC(21dbb69d) SHA1(cabd42d2edd333871269a7bc03a68f6765d254ce) )
MEGADRIVE_ROM_LOAD( killin1, "killing game show, the (japan).bin",                                                       0x000000, 0x100000,  CRC(b8e7668a) SHA1(47cdd668998139c92305e1b5abf7e196901490d6) )
MEGADRIVE_ROM_LOAD( kof98, "king of fighters '98, the (unlicensed) [!].bin",                                             0x000000, 0x200000,  CRC(cbc38eea) SHA1(aeee33bfc2c440b6b861ac0d1b9bc9bface24861) )
MEGADRIVE_ROM_LOAD( kof98a, "king of fighters '98, the (unlicensed) (pirate).bin",                                       0x000000, 0x200000,  CRC(c79e1074) SHA1(6eb3a12e082ce4074e88ad3cb2b3c51f9a72225c) )
MEGADRIVE_ROM_LOAD( kotm, "king of the monsters (europe).bin",                                                           0x000000, 0x100000,  CRC(7a94fd49) SHA1(46dc75f1e4d79fd159fcb4a256881375b63d9a2b) )
MEGADRIVE_ROM_LOAD( kotmu, "king of the monsters (usa).bin",                                                             0x000000, 0x100000,  CRC(f390d406) SHA1(b88555b50b12f7d9470e5a9870882ca38100342c) )
MEGADRIVE_ROM_LOAD( kotm2, "king of the monsters 2 (usa).bin",                                                           0x000000, 0x200000,  CRC(ee1638ac) SHA1(9fa2d329ea443b3e030206fd64d7faa7778d492d) )
MEGADRIVE_ROM_LOAD( kingsj, "king salmon (japan).bin",                                                                   0x000000, 0x80000,   CRC(2cfc9f61) SHA1(62bea17b1e9152bde2355deb98347a802518d08d) )
MEGADRIVE_ROM_LOAD( kings, "king salmon - the big catch (usa).bin",                                                      0x000000, 0x80000,   CRC(f516e7d9) SHA1(c2d8e2c569a2c275677ae85094e0dfad7fdf680e) )
MEGADRIVE_ROM_LOAD( kingsbty, "king's bounty - the conqueror's quest (usa, europe).bin",                                 0x000000, 0x80000,   CRC(aa68a92e) SHA1(32f90806f44a0bd1d65d84ceeb644681b9cee967) )
MEGADRIVE_ROM_LOAD( kishid, "kishi densetsu (japan).bin",                                                                0x000000, 0x180000,  CRC(22e1f04a) SHA1(2884f79b8f717fc9e244dac0fb441bdc44c68203) )
MEGADRIVE_ROM_LOAD( kissshot, "kiss shot (japan) (seganet).bin",                                                         0x000000, 0x40000,   CRC(e487088c) SHA1(bc47dceea512c0195e51172ab3a2ff5cad03c9bd) )
MEGADRIVE_ROM_LOAD( klaxj, "klax (japan).bin",                                                                           0x000000, 0x40000,   CRC(1afcc1da) SHA1(f084f7a3851161523ab7e9cffb2f729563c17643) )
MEGADRIVE_ROM_LOAD( klax, "klax (usa, europe).bin",                                                                      0x000000, 0x40000,   CRC(248cd09e) SHA1(b7c07baf74b945549e067405566eeaa6856dd6b1) )
MEGADRIVE_ROM_LOAD( knuckl, "knuckles in sonic 2 (prototype 0524 - may 27, 1994, 10.46).bin",                            0x000000, 0x400000,  CRC(8878c1a8) SHA1(3a827d575f5a484c24e6c31417f32d6c5f0045de) )
MEGADRIVE_ROM_LOAD( koutet, "koutetsu teikoku (japan).bin",                                                              0x000000, 0x100000,  CRC(755d0b8a) SHA1(7ff88050ae9b2f12afe80432781ba25aa3a15e1c) )
MEGADRIVE_ROM_LOAD( krusty, "krusty's super fun house (usa, europe) (v1.1).bin",                                         0x000000, 0x80000,   CRC(56976261) SHA1(6aa026e394dba0e5584c4cf99ad1c166d91f3923) )
MEGADRIVE_ROM_LOAD( krusty1, "krusty's super fun house (usa, europe).bin",                                               0x000000, 0x80000,   CRC(f764005e) SHA1(ab794df527e9fc5823cc5f08130dc856456980b0) )
MEGADRIVE_ROM_LOAD( kujaku, "kujaku ou 2 - geneijou (japan).bin",                                                        0x000000, 0x60000,   CRC(affd56bc) SHA1(d48ed88269b4ea4c62a85f8607658f9ce566590c) )
MEGADRIVE_ROM_LOAD( kuuga, "kuuga - operation code 'vapor trail' (japan).bin",                                           0x000000, 0x100000,  CRC(83b6b6ba) SHA1(54d257ad1f941fdb39f6f7b2a3a168eb30ebd9ff) )
MEGADRIVE_ROM_LOAD( kyuuka, "kyuukai douchuuki (japan).bin",                                                             0x000000, 0x80000,   CRC(de48dce3) SHA1(40e771ace8f89e40d3315be698aa68effd617c5c) )
MEGADRIVE_ROM_LOAD( kyuuky, "kyuukyoku tiger (japan).bin",                                                               0x000000, 0xa0000,   CRC(61276d21) SHA1(cbe207732c6ce5e5e5846e44847ce902315f2bc3) )
MEGADRIVE_ROM_LOAD( larussa, "la russa baseball 95 (usa, australia).bin",                                                0x000000, 0x200000,  CRC(3f848a92) SHA1(0e73742113aa3f0aa5b010bb847569589cd3a5b0) )
MEGADRIVE_ROM_LOAD( lakers, "lakers versus celtics and the nba playoffs (usa).bin",                                      0x000000, 0x80000,   CRC(0e33fc75) SHA1(b70b5f884dd7b26ffe2d6d50625dd61fec8f2899) )
MEGADRIVE_ROM_LOAD( landstlkb, "landstalker (usa) (beta).bin",                                                           0x000000, 0x200000,  CRC(70483d03) SHA1(dfca19397479852584d4ac6fcbe27412f9bc1af0) )
MEGADRIVE_ROM_LOAD( landstlku, "landstalker (usa).bin",                                                                  0x000000, 0x200000,  CRC(fbbb5b97) SHA1(24345e29427b000b90df778965dd8834300a9dde) )
MEGADRIVE_ROM_LOAD( landstlkg, "landstalker - die schatze von konig nolo (germany).bin",                                 0x000000, 0x200000,  CRC(10fedb8f) SHA1(b0b447158cabb562e6bf4c0b829ed938b34afb52) )
MEGADRIVE_ROM_LOAD( landstlkj, "landstalker - koutei no zaihou (japan).bin",                                             0x000000, 0x200000,  CRC(60d4cedb) SHA1(cdbc7cd9ceb181cad9e49b641ff717072546f0d9) )
MEGADRIVE_ROM_LOAD( landstlkf, "landstalker - le tresor du roi nole (france).bin",                                       0x000000, 0x200000,  CRC(5de7d917) SHA1(86db4b22b54e8583e35717927ad66b7535bf33b4) )
MEGADRIVE_ROM_LOAD( landstlk, "landstalker - the treasures of king nole (europe).bin",                                   0x000000, 0x200000,  CRC(e3c65277) SHA1(9fbadc86319936855831ecd096d82d716b304215) )
MEGADRIVE_ROM_LOAD( langriss, "langrisser (japan).bin",                                                                  0x000000, 0x80000,   CRC(b6ea5016) SHA1(cc67c5a3b91e706b495eb561a95a038fff72b5da) )
MEGADRIVE_ROM_LOAD( langris2a, "langrisser ii (japan) (v1.1).bin",                                                       0x000000, 0x200000,  CRC(0caa0593) SHA1(4ba6591dfd85aa75ffe8dc21137a9291aa7f5603) )
MEGADRIVE_ROM_LOAD( langris2, "langrisser ii (japan) (v1.2).bin",                                                        0x000000, 0x200000,  CRC(4967c9f9) SHA1(167944091348c89ce43dfa4854f8a51ed7276dde) )
MEGADRIVE_ROM_LOAD( langris2b, "langrisser ii (japan).bin",                                                              0x000000, 0x200000,  CRC(7f891dfc) SHA1(4bbc2502784a61eedf45eca5303dc68062964ff4) )
MEGADRIVE_ROM_LOAD( lastact, "last action hero (usa, europe).bin",                                                       0x000000, 0x100000,  CRC(15357dde) SHA1(8efa876894c8bfb0ea457e86173eb5b233861cd0) )
MEGADRIVE_ROM_LOAD( lastbatt, "mpr-12578.bin",                                                                           0x000000, 0x80000,   CRC(bbfaad77) SHA1(6ebef9c86779040bd2996564caa8631f4c41cf03) )
MEGADRIVE_ROM_LOAD( lawnmowr, "lawnmower man, the (usa, europe).bin",                                                    0x000000, 0x100000,  CRC(a7cacd59) SHA1(9e19ac92bc06954985dd97a9a7f55ef87e8c7364) )
MEGADRIVE_ROM_LOAD( legend, "legend of galahad, the (usa, europe).bin",                                                  0x000000, 0x100000,  CRC(679557bc) SHA1(3368af01da1f3f9e0ea7c80783170aeb08f5c24d) )
MEGADRIVE_ROM_LOAD( legendtho, "legende de thor, la (france).bin",                                                       0x000000, 0x300000,  CRC(b97cca1c) SHA1(677e1fbf2f6f90dd5a016f3bd5f305547249205a) )
MEGADRIVE_ROM_LOAD( lemmings, "lemmings (europe).bin",                                                                   0x000000, 0x100000,  CRC(6a1a4579) SHA1(f4b86031e348edb4dcffaf969998f368955828ce) )
MEGADRIVE_ROM_LOAD( lemmingsu, "sunsoft_lem8_j.bin",                                                                     0x000000, 0x100000,  CRC(68c70362) SHA1(61d468378c06a3d1044a8e11255294b46d0c094d) )
MEGADRIVE_ROM_LOAD( lemmingsu1, "lemmings (japan, usa).bin",                                                             0x000000, 0x100000,  CRC(f015c2ad) SHA1(83aebf600069cf053282e813d3ab4910f586706e) )
MEGADRIVE_ROM_LOAD( lemming2, "lemmings 2 - the tribes (europe).bin",                                                    0x000000, 0x200000,  CRC(741eb624) SHA1(b56f9e78dee0186c8f8103c7d125e8b497eb0196) )
MEGADRIVE_ROM_LOAD( lemming2u, "lemmings 2 - the tribes (usa).bin",                                                      0x000000, 0x200000,  CRC(de59a3a3) SHA1(1de84f5c9b25f6af4c2c3e18bb710b9572fc0a10) )
MEGADRIVE_ROM_LOAD( le, "lethal enforcers (europe).bin",                                                                 0x000000, 0x200000,  CRC(ca2bf99d) SHA1(b9cc7ff3c6a50b2624358785bcadd1451e23993e) )
MEGADRIVE_ROM_LOAD( lej, "lethal enforcers (japan).bin",                                                                 0x000000, 0x200000,  CRC(f25f1e49) SHA1(14245fccf4d7e5d1dd4ad5f426507516e71e3a06) )
MEGADRIVE_ROM_LOAD( leu, "lethal enforcers (usa).bin",                                                                   0x000000, 0x200000,  CRC(51d9a84a) SHA1(309b41c6d4159f4f07fe9a76aca4fc4ddf45de63) )
MEGADRIVE_ROM_LOAD( le2, "lethal enforcers ii - gun fighters (europe).bin",                                              0x000000, 0x200000,  CRC(4bfe045c) SHA1(0c99f93ef90d6242b198c99a1e940be432ec861f) )
MEGADRIVE_ROM_LOAD( le2u, "lethal enforcers ii - gun fighters (usa).bin",                                                0x000000, 0x200000,  CRC(e5fdd28b) SHA1(823a59b1177665313a1114a622ee98a795141eec) )
MEGADRIVE_ROM_LOAD( lhxj, "lhx attack chopper (japan).bin",                                                              0x000000, 0x100000,  CRC(224ff103) SHA1(4e98a59fbd25db9fa9a05c84ecfa79154dd49d12) )
MEGADRIVE_ROM_LOAD( lhx, "lhx attack chopper (usa, europe).bin",                                                         0x000000, 0x100000,  CRC(70c3428d) SHA1(f23c6d0bc6daae11a3398d73961f621e508c9229) )
MEGADRIVE_ROM_LOAD( liberty, "liberty or death (usa).bin",                                                               0x000000, 0x200000,  CRC(2adb0364) SHA1(c0ddfc2149cd84fbb0c5860b98c3a16f6000b85e) )
MEGADRIVE_ROM_LOAD( lightc, "light crusader (europe) (en,fr,de,es).bin",                                                 0x000000, 0x200000,  CRC(52c7252b) SHA1(4352ae7ba1316e4384c4632be80f2fe277443f51) )
MEGADRIVE_ROM_LOAD( lightcj, "light crusader (japan).bin",                                                               0x000000, 0x200000,  CRC(237076a4) SHA1(2b8c0931c33c143d41ef7cff6a8dbb9b8351d613) )
MEGADRIVE_ROM_LOAD( lightck, "light crusader (korea) (en,ko).bin",                                                       0x000000, 0x200000,  CRC(6d0cbcb2) SHA1(0f018f95b4933bb5b3b3a91cee8b9a8ecb376942) )
MEGADRIVE_ROM_LOAD( lightcp, "light crusader (prototype - jun 08, 1995).bin",                                            0x000000, 0x200000,  CRC(e350ccfa) SHA1(0bf26dba5324471532db776da1ee4c0c06add1e5) )
MEGADRIVE_ROM_LOAD( lightcu, "light crusader (usa).bin",                                                                 0x000000, 0x200000,  CRC(beb715dc) SHA1(df58fbcbede4b9659740b5505641d4cc7dd1b7f8) )
MEGADRIVE_ROM_LOAD( lighte, "lightening force - quest for the darkstar (usa).bin",                                       0x000000, 0x100000,  CRC(c8f8c0e0) SHA1(ab5bd9ddbfc07d860f44b9c72d098aef2581d1d8) )
MEGADRIVE_ROM_LOAD( lionkin2, "lion king ii, the (unlicensed) [!].bin",                                                  0x000000, 0x200000,  CRC(aff46765) SHA1(5649fa1fbfb28d58b0608e8ebc5dc7bd5c4c9678) )
MEGADRIVE_ROM_LOAD( lionkin2a, "lion king ii, the (unlicensed) (pirate).bin",                                            0x000000, 0x200000,  CRC(721b4981) SHA1(70eb5b423948e5a124de4d5d24c14b2c64bfb282) )
MEGADRIVE_ROM_LOAD( lionking, "lion king, the (world).bin",                                                              0x000000, 0x300000,  CRC(5696a5bc) SHA1(24cbe4e75ec10e8e0ffbdb400cf86ecb072d4da9) )
MEGADRIVE_ROM_LOAD( lobo, "lobo (usa) (prototype).bin",                                                                  0x000000, 0x300000,  CRC(b5e09338) SHA1(749fe1f7581352562a38997cb7323019b6ff1d93) )
MEGADRIVE_ROM_LOAD( longch, "long (china) (unlicensed).bin",                                                             0x000000, 0x40000,   CRC(1b86e623) SHA1(09e4b59da3344f16ce6173c432c88ee9a12a3561) )
MEGADRIVE_ROM_LOAD( lordmo, "lord monarch - tokoton sentou densetsu (japan).bin",                                        0x000000, 0x200000,  CRC(238bf5db) SHA1(9b5c71d70de132c8ba6f2adfdeba43077f76ac3e) )
MEGADRIVE_ROM_LOAD( lostvikb, "lost vikings, the (europe) (beta).bin",                                                   0x000000, 0x100000,  CRC(17bed25f) SHA1(375eaa9845692db4fdbd0b51985aa0892a8fe425) )
MEGADRIVE_ROM_LOAD( lostvik, "lost vikings, the (europe).bin",                                                           0x000000, 0x100000,  CRC(1f14efc6) SHA1(c977a21d287187c3931202b3501063d71fcaf714) )
MEGADRIVE_ROM_LOAD( lostviku, "lost vikings, the (usa).bin",                                                             0x000000, 0x100000,  CRC(7ba49edb) SHA1(f00464c111b57c8b23698760cbd377c3c8cfe712) )
MEGADRIVE_ROM_LOAD( lostwrld, "lost world, the - jurassic park (usa, europe).bin",                                       0x000000, 0x400000,  CRC(140a284c) SHA1(25a93bbcfbbe286a36f0b973adf86b0f0f3cfa3f) )
MEGADRIVE_ROM_LOAD( lotus2b, "lotus ii (usa) (beta).bin",                                                                0x000000, 0x100000,  CRC(2997b7d4) SHA1(e108d612735dcc768bed68a58d6fd45d71de565d) )
MEGADRIVE_ROM_LOAD( lotus2, "lotus ii (usa, europe).bin",                                                                0x000000, 0x100000,  CRC(1d8ee010) SHA1(5a00e8aaae8d987ee0f18fc4604230265aea699e) )
MEGADRIVE_ROM_LOAD( m1abrams, "m-1 abrams battle tank (usa, europe).bin",                                                0x000000, 0x80000,   CRC(1e2f74cf) SHA1(65248727b0b52106007ec1193832f16545db5378) )
MEGADRIVE_ROM_LOAD( majian, "ma jiang qing ren - ji ma jiang zhi (china) (unlicensed).bin",                              0x000000, 0x100000,  CRC(ddd02ba4) SHA1(fe9ec21bd206ad1a178c54a2fee80b553c478fc4) )
MEGADRIVE_ROM_LOAD( maqiao, "ma qiao e mo ta - devilish mahjong tower (china) (unlicensed).bin",                         0x000000, 0x100000,  CRC(12e35994) SHA1(84e8bf546283c73396e40c4cfa05986ebeb123bb) )
MEGADRIVE_ROM_LOAD( madden94, "madden nfl '94 (usa, europe).bin",                                                        0x000000, 0x200000,  CRC(d14b811b) SHA1(856d68d3e8589df3452096434feef823684d11eb) )
MEGADRIVE_ROM_LOAD( madden95, "madden nfl 95 (usa, europe).bin",                                                         0x000000, 0x200000,  CRC(db0be0c2) SHA1(41cde6211da87a8e61e2ffd42cef5de588f9b9fc) )
MEGADRIVE_ROM_LOAD( madden96, "madden nfl 96 (usa, europe).bin",                                                         0x000000, 0x200000,  CRC(f126918b) SHA1(35a4241eed51f10de2e63c843f162ce5d92c70a2) )
MEGADRIVE_ROM_LOAD( madden97, "madden nfl 97 (usa, europe).bin",                                                         0x000000, 0x200000,  CRC(c4b4e112) SHA1(63544d2a0230be102f2558c03a74855fc712b865) )
MEGADRIVE_ROM_LOAD( madden98, "madden nfl 98 (usa).bin",                                                                 0x000000, 0x200000,  CRC(e051ea62) SHA1(761e0903798a8d0ad9e7ab72e6d2762fc9d366d2) )
MEGADRIVE_ROM_LOAD( madoum, "madou monogatari i (japan).bin",                                                            0x000000, 0x200000,  CRC(dd82c401) SHA1(143456600e44f543796cf6ade77830115a8f2f99) )
MEGADRIVE_ROM_LOAD( msbpa, "magic school bus, the (prototype - apr 11, 1995).bin",                                       0x000000, 0xfdfbc,   CRC(883023bc) SHA1(3291ce39fbff343916aab43e53121166e2879db5) )
MEGADRIVE_ROM_LOAD( msbpb, "magic school bus, the (prototype - apr 21, 1995).bin",                                       0x000000, 0xfcfdc,   CRC(7844b7ad) SHA1(7acc08c2489c970b1208e1162791bfda30e5df67) )
MEGADRIVE_ROM_LOAD( msbpc, "magic school bus, the (prototype - apr 25, 1995).bin",                                       0x000000, 0xfd5e0,   CRC(b6bd2d87) SHA1(6a746535e4d20385d506cddaa05f0da5693107b8) )
MEGADRIVE_ROM_LOAD( msbpd, "magic school bus, the (prototype - apr 28, 1995).bin",                                       0x000000, 0xfe89c,   CRC(9b6bae87) SHA1(1eead88af324c58ebaf95e2d6f24ad969feabc76) )
MEGADRIVE_ROM_LOAD( msbpe, "magic school bus, the (prototype - feb 02, 1995).bin",                                       0x000000, 0x100000,  CRC(7805b5c9) SHA1(06bd39d487f63fe4bebdd651247223767540e00a) )
MEGADRIVE_ROM_LOAD( msbpf, "magic school bus, the (prototype - feb 17, 1995).bin",                                       0x000000, 0xdf6bc,   CRC(2b7a88c0) SHA1(ee81e0809b2d5636d68869f6cf8d25a1c91627a0) )
MEGADRIVE_ROM_LOAD( msbpg, "magic school bus, the (prototype - jan 12, 1995).bin",                                       0x000000, 0xeb2e4,   CRC(946346f9) SHA1(1351b74d8bdbdeb7e1d042c35e2d770bd925beb3) )
MEGADRIVE_ROM_LOAD( msbph, "magic school bus, the (prototype - mar 07, 1995).bin",                                       0x000000, 0xe9aa8,   CRC(26c71eb8) SHA1(ae82c6711051ed66b5c510d3632eb280fb10570c) )
MEGADRIVE_ROM_LOAD( msbpi, "magic school bus, the (prototype - mar 14, 1995).bin",                                       0x000000, 0xf1c30,   CRC(37a6a1af) SHA1(4100d0be97abdd04d207b732b45940673613928a) )
MEGADRIVE_ROM_LOAD( msbpj, "magic school bus, the (prototype - mar 27, 1995).bin",                                       0x000000, 0xf52e8,   CRC(b065da40) SHA1(792e039ce0760c908c80872363fd5d309fb33872) )
MEGADRIVE_ROM_LOAD( msbpk, "magic school bus, the (prototype - mar 31, 1995).bin",                                       0x000000, 0xf93c0,   CRC(cd788def) SHA1(ea591191bd537c7a1c57ea275bb02f3a75c55c5a) )
MEGADRIVE_ROM_LOAD( msbpl, "magic school bus, the (prototype - may 05, 1995).bin",                                       0x000000, 0xfea00,   CRC(a2315f1c) SHA1(46a3d386495d71b14979a92e71e3d87b9b7296a6) )
MEGADRIVE_ROM_LOAD( msb, "magic school bus, the (usa).bin",                                                              0x000000, 0x100000,  CRC(1a5d4412) SHA1(d7c075d98430f2864d9120d7c9f2efb92e8d350e) )
MEGADRIVE_ROM_LOAD( mhat, "magical hat no buttobi turbo! daibouken (japan).bin",                                         0x000000, 0x80000,   CRC(e43e853d) SHA1(1a741125a80ba9207c74a32e5b21dbb347c3c34a) )
MEGADRIVE_ROM_LOAD( mtaru, "magical taruruuto-kun (japan).bin",                                                          0x000000, 0x80000,   CRC(f11060a5) SHA1(208f1d83bb410f9cc5e5308942b83c0e84f64294) )
MEGADRIVE_ROM_LOAD( mahjongc, "mahjong cop ryuu - shiro ookami no yabou (japan).bin",                                    0x000000, 0x40000,   CRC(1ccbc782) SHA1(7fd5866347067e6111739833595278e192d275fb) )
MEGADRIVE_ROM_LOAD( mamono, "mamono hunter youko - dai 7 no keishou (japan).bin",                                        0x000000, 0x80000,   CRC(10bb359b) SHA1(5c8c600ca7468871b0ef301fd82a1909aa8b3b10) )
MEGADRIVE_ROM_LOAD( manover, "man overboard! (europe).bin",                                                              0x000000, 0x100000,  CRC(cae0e3a6) SHA1(36c66a1cbf44f4019b84538a6bf452c369239bc9) )
MEGADRIVE_ROM_LOAD( maoure, "maou renjishi (japan).bin",                                                                 0x000000, 0x80000,   CRC(24a7f28c) SHA1(56253adf20d420233722d428170a646262be226f) )
MEGADRIVE_ROM_LOAD( marble, "marble madness (usa, europe).bin",                                                          0x000000, 0x80000,   CRC(79eba28a) SHA1(059e99fde8726a45a584007186913eb9a01f738e) )
MEGADRIVE_ROM_LOAD( marioand, "mario andretti racing (usa, europe).bin",                                                 0x000000, 0x200000,  CRC(7f1dc0aa) SHA1(d28d79177cf25c8cbf54b28a7d357ac86b7820b5) )
MEGADRIVE_ROM_LOAD( mariolh, "mario lemieux hockey (usa, europe).bin",                                                   0x000000, 0x80000,   CRC(f664eb6c) SHA1(34cb9c26030c5a0e66dc7de3302bb50633e4dbb6) )
MEGADRIVE_ROM_LOAD( markob, "marko's magic football (europe) (en,fr,de,es) (beta).bin",                                  0x000000, 0x200000,  CRC(0273e564) SHA1(42b871c3a91697e1d7b8c6f1eae9d2a8b07a0fca) )
MEGADRIVE_ROM_LOAD( marko, "marko's magic football (europe) (en,fr,de,es).bin",                                          0x000000, 0x200000,  CRC(2307b905) SHA1(717e924db1ac7cfd099adb6031a08606fcb30219) )
MEGADRIVE_ROM_LOAD( markou, "marko's magic football (usa).bin",                                                          0x000000, 0x200000,  CRC(2b8c8cce) SHA1(b73d01599d8201dc3bd98fdc9cc262e14dbc52b4) )
MEGADRIVE_ROM_LOAD( marsup, "marsupilami (europe) (en,fr,de,es,it).bin",                                                 0x000000, 0x200000,  CRC(e09bbd70) SHA1(d2b8358ef261f8b5ad54a58e89f3999312d0cec9) )
MEGADRIVE_ROM_LOAD( marsupu, "marsupilami (usa) (en,fr,de,es,it).bin",                                                   0x000000, 0x200000,  CRC(c76558df) SHA1(3e761bdbf4ab8cb43d4a8d99e22b7de896884819) )
MEGADRIVE_ROM_LOAD( mvlndj, "marvel land (japan).bin",                                                                   0x000000, 0x100000,  CRC(5d162d21) SHA1(2f7ef2f956d62373dcd5f3808e7501e3660c0658) )
MEGADRIVE_ROM_LOAD( mvlnd, "marvel land (usa).bin",                                                                      0x000000, 0x100000,  CRC(cd7eeeb7) SHA1(29f290c80f992542e73c9ea95190403cb262b6ad) )
MEGADRIVE_ROM_LOAD( marysh, "mary shelley's frankenstein (usa).bin",                                                     0x000000, 0x200000,  CRC(48993dc3) SHA1(2dd34478495a2988fe5839ef7281499f08bf7294) )
MEGADRIVE_ROM_LOAD( master, "master of monsters (japan).bin",                                                            0x000000, 0x80000,   CRC(d51ee8c2) SHA1(ca62d376be5cc7b944bbe9b5f2610f8bb55a2fed) )
MEGADRIVE_ROM_LOAD( masteru, "master of monsters (usa).bin",                                                             0x000000, 0x80000,   CRC(91354820) SHA1(28f38617911a99504542b30f70c0d9c81996ef65) )
MEGADRIVE_ROM_LOAD( mweap, "master of weapon (japan).bin",                                                               0x000000, 0x80000,   CRC(12ad6178) SHA1(574da95ab171b12546daa56dc28761c6bcc4a5fc) )
MEGADRIVE_ROM_LOAD( maten, "maten no soumetsu (japan).bin",                                                              0x000000, 0x100000,  CRC(b804a105) SHA1(31bc44b019310e18174eb5e1a6d8e7d351103e4e) )
MEGADRIVE_ROM_LOAD( mathbl, "math blaster - episode 1 (usa).bin",                                                        0x000000, 0x100000,  CRC(d055a462) SHA1(119c2615db2607b8102ccb57d862ac6084e37c9d) )
MEGADRIVE_ROM_LOAD( mazins, "mazin saga (asia).bin",                                                                     0x000000, 0x100000,  CRC(36459b59) SHA1(3509fdf8bd0a589c216a5032eef0c09a51c7a578) )
MEGADRIVE_ROM_LOAD( mazins1, "mazin saga (japan, korea).bin",                                                            0x000000, 0x100000,  CRC(45b3a34b) SHA1(bcaf2820f22d4a7bcf6324f288698192996875bd) )
MEGADRIVE_ROM_LOAD( mazins2, "mazin saga mutant fighter (usa).bin",                                                      0x000000, 0x100000,  CRC(1bd9fef1) SHA1(5cb87bb4efc8fa8754e687b62d0fd858e624997d) )
MEGADRIVE_ROM_LOAD( mazinw, "mazin wars (europe).bin",                                                                   0x000000, 0x100000,  CRC(4b07a105) SHA1(40e48ce531ed013d5a4a6f689e70781df3e0095c) )
MEGADRIVE_ROM_LOAD( mcdonald, "mcdonald's treasure land adventure (europe).bin",                                         0x000000, 0x100000,  CRC(6ab6a8da) SHA1(f6178018102df3c92d05a48ff5949db9416acd5c) )
MEGADRIVE_ROM_LOAD( mcdonaldjb, "mcdonald's treasure land adventure (japan) (beta).bin",                                 0x000000, 0x100000,  CRC(7bf477e8) SHA1(ddf496f9a95b2963fe50f3bcaef8e3b592e2fc64) )
MEGADRIVE_ROM_LOAD( mcdonaldj, "mcdonald's treasure land adventure (japan).bin",                                         0x000000, 0x100000,  CRC(febcfd06) SHA1(aee7fd6a08ec22f42159b188f23406cbe3229f3d) )
MEGADRIVE_ROM_LOAD( mcdonaldu, "mcdonald's treasure land adventure (usa).bin",                                           0x000000, 0x100000,  CRC(04ef4899) SHA1(bcb77c10bc8f3322599269214e0f8dde32b01a5c) )
MEGADRIVE_ROM_LOAD( medalc, "medal city (japan) (seganet).bin",                                                          0x000000, 0x40000,   CRC(3ef4135d) SHA1(809bbb203dd2f2542e3b9987380e040631b788cc) )
MEGADRIVE_ROM_LOAD( megaan, "mega anser (japan) (program).bin",                                                          0x000000, 0x80000,   CRC(08ece367) SHA1(d353e673bd332aa71370fa541e6fd26d918b5a2b) )
MEGADRIVE_ROM_LOAD( megabomb, "mpr-17108.bin",                                                                           0x000000, 0x100000,  CRC(54ab3beb) SHA1(01f76e2f719bdae5f21ff0e5a1ac1262c2def279) )
MEGADRIVE_ROM_LOAD( megabombu, "mega bomberman (usa).bin",                                                               0x000000, 0x100000,  CRC(4bd6667d) SHA1(89495a8acbcb0ddcadfe5b7bada50f4d9efd0ddd) )
MEGADRIVE_ROM_LOAD( megabm8, "mega bomberman - 8 player demo (unlicensed).bin",                                          0x000000, 0x100000,  CRC(d41c0d81) SHA1(6cc338e314533d7e4724715ef0d156f5c3f873f3) )
MEGADRIVE_ROM_LOAD( megaga, "mega games 10 (brazil).bin",                                                                0x000000, 0x400000,  CRC(c19ae368) SHA1(791a4de13cfe2af4302aac969753d67dd76e89c8) )
MEGADRIVE_ROM_LOAD( megaga1, "mega games 2 (europe).bin",                                                                0x000000, 0x200000,  CRC(30d59f2f) SHA1(1cf425556de9352c1efe60b07e592b610e78cfbd) )
MEGADRIVE_ROM_LOAD( megaga2, "mega games 3 (europe, asia).bin",                                                          0x000000, 0x200000,  CRC(b4247d98) SHA1(4329d7fa2c4b8e6ed0b4adfc64dca474d0ee5a51) )
MEGADRIVE_ROM_LOAD( megaga3, "mega games 6 vol. 1 (europe).bin",                                                         0x000000, 0x300000,  CRC(b66fb80d) SHA1(42f29050e071c3086be35452d917debf25648b23) )
MEGADRIVE_ROM_LOAD( megaga4, "mega games 6 vol. 2 (europe).bin",                                                         0x000000, 0x300000,  CRC(e8d10db9) SHA1(1fc777b946874216add624414ef6f5878bab202b) )
MEGADRIVE_ROM_LOAD( megaga5, "mega games 6 vol. 3 (europe).bin",                                                         0x000000, 0x300000,  CRC(fe3e7e4f) SHA1(cca818d624e95c2d07cfc1b22c44eb53e4bdcd02) )
MEGADRIVE_ROM_LOAD( megaga6, "mega games i (europe).bin",                                                                0x000000, 0x100000,  CRC(db753224) SHA1(076df34a01094ce0893f32600e24323567e2a23b) )
MEGADRIVE_ROM_LOAD( megaman, "mega man - the wily wars (europe).bin",                                                    0x000000, 0x200000,  CRC(dcf6e8b2) SHA1(ea9ae2043c97db716a8d31ee90e581c3d75f4e3e) )
MEGADRIVE_ROM_LOAD( megaswivb, "mega swiv (e) (pirate).bin",                                                             0x000000, 0x100000,  CRC(1ec66bf7) SHA1(6afc7c86dd9f03d24ca976437d1f70e742a7928d) )
MEGADRIVE_ROM_LOAD( megaswiv, "mega swiv (europe).bin",                                                                  0x000000, 0x100000,  CRC(78c2f046) SHA1(6396fb0f204c9f23d0af0b39d069ff0883e191aa) )
MEGADRIVE_ROM_LOAD( megat, "mega turrican (europe).bin",                                                                 0x000000, 0x100000,  CRC(b1d15d0f) SHA1(00ad2cf231bedbd373253b169e170e8b0db4c86a) )
MEGADRIVE_ROM_LOAD( megatu, "mega turrican (usa).bin",                                                                   0x000000, 0x100000,  CRC(fe898cc9) SHA1(180285dbfc1613489f1c20e9fd6c2b154dec7fe2) )
MEGADRIVE_ROM_LOAD( megalo, "mega-lo-mania (europe) (v1.1).bin",                                                         0x000000, 0x100000,  CRC(ab9fed30) SHA1(3794cb708b1f54675eb6fb272cfee01e6dbcecc1) )
MEGADRIVE_ROM_LOAD( megalo1, "mega-lo-mania (europe).bin",                                                               0x000000, 0x100000,  CRC(2148d56d) SHA1(e861551edabe55181ed1b7260169e953af903b5e) )
MEGADRIVE_ROM_LOAD( megalof, "mega-lo-mania (france).bin",                                                               0x000000, 0x100000,  CRC(3b3231ed) SHA1(1fcfc9ee3bffc25388735782b0cdb829a7e40507) )
MEGADRIVE_ROM_LOAD( megaloj, "mega-lo-mania (japan).bin",                                                                0x000000, 0x100000,  CRC(a60d8619) SHA1(66e391c69dfbe7329654550ebf4464b4f426c5a0) )
MEGADRIVE_ROM_LOAD( megamind, "megamind (japan) (seganet).bin",                                                          0x000000, 0x40000,   CRC(76df2ae2) SHA1(dc5697f6baeeafe04250b39324464bb6dcdcac7f) )
MEGADRIVE_ROM_LOAD( meganet, "meganet (brazil) (program).bin",                                                           0x000000, 0x40000,   CRC(fab33cc2) SHA1(7969fe1b43d29b65a1e24e118f4a6fea27437e9c) )
MEGADRIVE_ROM_LOAD( megapanl, "megapanel (japan).bin",                                                                   0x000000, 0x40000,   CRC(6240f579) SHA1(1822905930f5f3627e9f9109760205e617295fda) )
MEGADRIVE_ROM_LOAD( megatrax, "megatrax (japan).bin",                                                                    0x000000, 0x80000,   CRC(a0837741) SHA1(9e611f2a70fb2505a661d0906b535c484db99d0b) )
MEGADRIVE_ROM_LOAD( menacer, "menacer 6-game cartridge (usa, europe).bin",                                               0x000000, 0x100000,  CRC(936b85f7) SHA1(2357ed0b9da25e36f6a937c99ebc32729b8c10d2) )
MEGADRIVE_ROM_LOAD( menghu, "meng huan shui guo pan - 777 casino (china) (unlicensed).bin",                              0x000000, 0x100000,  CRC(42dc03e4) SHA1(df20a28d03a2cd481af134ef7602062636c3cc79) )
MEGADRIVE_ROM_LOAD( metalf, "metal fangs (japan).bin",                                                                   0x000000, 0x80000,   CRC(a8df1c4c) SHA1(489e81d2ac81810d571829e8466374f242e81621) )
MEGADRIVE_ROM_LOAD( mwalk, "michael jackson's moonwalker (world) (rev a).bin",                                           0x000000, 0x80000,   CRC(11ce1f9e) SHA1(70d9b760c87196af364492512104fa18c9d69cce) )
MEGADRIVE_ROM_LOAD( mwalk1, "michael jackson's moonwalker (world).bin",                                                  0x000000, 0x80000,   CRC(6a70791b) SHA1(8960bac2027cdeadb07e535a77597fb783e1433b) )
MEGADRIVE_ROM_LOAD( mickmb, "mick & mack as the global gladiators (usa) (beta).bin",                                     0x000000, 0x100000,  CRC(08c2af21) SHA1(433db3e145499ebad4f71aae66ee6726ef30d5db) )
MEGADRIVE_ROM_LOAD( mickm, "mick & mack as the global gladiators (usa).bin",                                             0x000000, 0x100000,  CRC(40f17bb3) SHA1(4fd2818888a3c265e148e9be76525654e76347e4) )
MEGADRIVE_ROM_LOAD( mmania, "mickey mania - the timeless adventures of mickey mouse (europe).bin",                       0x000000, 0x200000,  CRC(cb5a8b85) SHA1(6bcbf683a5d0e9f67b9ed8f79bc593fab594d84a) )
MEGADRIVE_ROM_LOAD( mmaniaj, "mickey mania - the timeless adventures of mickey mouse (japan).bin",                       0x000000, 0x200000,  CRC(23180cf7) SHA1(afe3e1c9267e2a7f0bec5e3e951307ebea7f0b04) )
MEGADRIVE_ROM_LOAD( mmaniaub, "mickey mania - the timeless adventures of mickey mouse (usa) (beta).bin",                 0x000000, 0x200000,  CRC(7fc1bdf0) SHA1(cd80dcd53c9996744eb94ba8702f84d784e83f34) )
MEGADRIVE_ROM_LOAD( mmaniau, "mickey mania - the timeless adventures of mickey mouse (usa).bin",                         0x000000, 0x200000,  CRC(629e5963) SHA1(20779867821bab019f63ce42e3067ffafb4fe480) )
MEGADRIVE_ROM_LOAD( mickeyuc, "mickey's ultimate challenge (usa).bin",                                                   0x000000, 0x100000,  CRC(30b512ee) SHA1(def06da570df4ff73df36591ef05cce6d409b950) )
MEGADRIVE_ROM_LOAD( micromc, "micro machines (c).bin",                                                                   0x000000, 0x80000,   CRC(54e4cff1) SHA1(0d4d023a1f9dc8b794bd60bf6e70465b712ffded) )
MEGADRIVE_ROM_LOAD( microma, "micro machines (usa, europe) (alt).bin",                                                   0x000000, 0x80000,   CRC(e5cf560d) SHA1(bbcf8a40e7bfe09225fdc8fc3f22f8b8cc710d06) )
MEGADRIVE_ROM_LOAD( micromb, "micro machines (usa, europe) (mdmm acd3).bin",                                             0x000000, 0x80000,   CRC(50081a0b) SHA1(56a8844c376f2e79e92cf128681fa3fef81c36d6) )
MEGADRIVE_ROM_LOAD( microm, "micro machines (usa, europe).bin",                                                          0x000000, 0x80000,   CRC(7ffbd1ad) SHA1(6c822ab42a37657aa2a4e70e44b201f8e8365a98) )
MEGADRIVE_ROM_LOAD( microm2a, "micro machines 2 - turbo tournament (europe) (j-cart) (alt).bin",                         0x000000, 0x100000,  CRC(01c22a5d) SHA1(cb5fb33212592809639b37c2babd72a7953fa102) )
MEGADRIVE_ROM_LOAD( microm2, "micro machines 2 - turbo tournament (europe) (j-cart).bin",                                0x000000, 0x100000,  CRC(42bfb7eb) SHA1(ab29077a6a5c2ccc777b0bf22f4d5908401f4d47) )
MEGADRIVE_ROM_LOAD( micromm, "micro machines military (europe) (j-cart).bin",                                            0x000000, 0x100000,  CRC(b3abb15e) SHA1(6d3df64ab8bb0b559f216adca62d1cdd74704a26) )
MEGADRIVE_ROM_LOAD( micromma, "micro machines military - it's a blast! (e) [x].bin",                                     0x000000, 0x100000,  CRC(a1ad9f97) SHA1(e58572080a6a7918b60a65461f6432582ef1641c) )
MEGADRIVE_ROM_LOAD( micro96a, "micro machines turbo tournament 96 (europe) (j-cart).bin",                                0x000000, 0x100000,  CRC(7492b1de) SHA1(9f47fcc7bb2f5921cb1c3beb06b668ffb292cb08) )
MEGADRIVE_ROM_LOAD( micro96b, "micro machines turbo tournament 96 (europe) (v1.1) (j-cart).bin",                         0x000000, 0x100000,  CRC(23319d0d) SHA1(e8ff759679a0df2b3f9ece37ef686f248d3cf37b) )
MEGADRIVE_ROM_LOAD( micro96, "micro machines turbo tournament 96 (v1.1) (e).bin",                                        0x000000, 0x100000,  CRC(3137b3c4) SHA1(7e59fc5800137cea3d4526ee549f7caef150dcaa) )
MEGADRIVE_ROM_LOAD( midresj, "midnight resistance (japan).bin",                                                          0x000000, 0x100000,  CRC(8f3f6e4d) SHA1(b11db9fde0955c8aa0ae2d1a234d3295eda75d12) )
MEGADRIVE_ROM_LOAD( midres, "midnight resistance (usa).bin",                                                             0x000000, 0x100000,  CRC(187c6af6) SHA1(6cdd9083e2ff72cfb0099fd57a7f9eade9a74dda) )
MEGADRIVE_ROM_LOAD( midway, "midway presents arcade's greatest hits (europe).bin",                                       0x000000, 0x80000,   CRC(c0dce0e5) SHA1(97e35e8fbe1546b9cf075ab75c8f790edcd9db93) )
MEGADRIVE_ROM_LOAD( mig29fi, "mig-29 fighter pilot (europe).bin",                                                        0x000000, 0x100000,  CRC(70b0a5d7) SHA1(5a2e7f8c752dfd8b301d2c044619c003a15894aa) )
MEGADRIVE_ROM_LOAD( mig29f1, "mig-29 fighter pilot (japan).bin",                                                         0x000000, 0x100000,  CRC(3d239046) SHA1(ef268701f4bce40174997c3cc8ab5148e2656e91) )
MEGADRIVE_ROM_LOAD( mig29f2, "mig-29 fighter pilot (usa).bin",                                                           0x000000, 0x100000,  CRC(59ccabb2) SHA1(140245f688c42cb2f6bfd740785742286c71be65) )
MEGADRIVE_ROM_LOAD( mightmag, "might and magic - gates to another world (usa, europe).bin",                              0x000000, 0xc0000,   CRC(f509145f) SHA1(52e7ade244d48bc282db003d87c408e96dcb3d85) )
MEGADRIVE_ROM_LOAD( mmagic3p, "might and magic iii - isles of terra (usa) (prototype).bin",                              0x000000, 0x200000,  CRC(6ef7104a) SHA1(d366d05644eb59a14baf3c2e7281c1584630c021) )
MEGADRIVE_ROM_LOAD( mmpr, "mighty morphin power rangers (europe).bin",                                                   0x000000, 0x200000,  CRC(7f96e663) SHA1(7bad0db9a96eafa413562b6631487ca847b02466) )
MEGADRIVE_ROM_LOAD( mmprpa, "mighty morphin power rangers (prototype - aug 04, 1994).bin",                               0x000000, 0x200000,  CRC(f3ae5aaf) SHA1(c15f23e20aee4d0664897e6cd29b77bd39228fc7) )
MEGADRIVE_ROM_LOAD( mmprpb, "mighty morphin power rangers (prototype - aug 08, 1994).bin",                               0x000000, 0x200000,  CRC(57644549) SHA1(e1651f13dd1bcedad0acc7d2c21298f9662ccf55) )
MEGADRIVE_ROM_LOAD( mmprpc, "mighty morphin power rangers (prototype - aug 09, 1994).bin",                               0x000000, 0x200000,  CRC(4356fe0a) SHA1(a91cf34ae5178696e66a71770872b3720265d61c) )
MEGADRIVE_ROM_LOAD( mmprpd, "mighty morphin power rangers (prototype - aug 10, 1994).bin",                               0x000000, 0x200000,  CRC(e79cd214) SHA1(c5e82c5786b52675d376872b3b985927d08feed2) )
MEGADRIVE_ROM_LOAD( mmprpe, "mighty morphin power rangers (prototype - jul 08, 1994).bin",                               0x000000, 0x200000,  CRC(e6916c54) SHA1(49f545dfa50fccd15d799fcaaefd3d39c507bfb1) )
MEGADRIVE_ROM_LOAD( mmprpf, "mighty morphin power rangers (prototype - jul 18, 1994).bin",                               0x000000, 0x200000,  CRC(5accdb1a) SHA1(0103bbf1d12a6a72ad00515429155b3c6020cfbb) )
MEGADRIVE_ROM_LOAD( mmpru, "mighty morphin power rangers (usa).bin",                                                     0x000000, 0x200000,  CRC(715158a9) SHA1(dbe0c63c9e659255b091760889787001e85016a9) )
MEGADRIVE_ROM_LOAD( mmprtm, "mighty morphin power rangers - the movie (europe).bin",                                     0x000000, 0x200000,  CRC(254a4972) SHA1(b3d105d9f7a8d2fd92015e1ac98d13a4094de5ef) )
MEGADRIVE_ROM_LOAD( mmprtmpa, "mighty morphin power rangers - the movie (prototype - jul 13, 1995).bin",                 0x000000, 0x200000,  CRC(227bf7fd) SHA1(ee3b33ab836ae27bf2a89c602206661f0b557f2e) )
MEGADRIVE_ROM_LOAD( mmprtmpb, "mighty morphin power rangers - the movie (prototype - jul 17, 1995).bin",                 0x000000, 0x200000,  CRC(579de657) SHA1(480cdadf79d884319daf0d23a062555e816187ff) )
MEGADRIVE_ROM_LOAD( mmprtmpc, "mighty morphin power rangers - the movie (prototype - jul 22, 1995).bin",                 0x000000, 0x200000,  CRC(c2ca3a8b) SHA1(a10a3ca13c57030b411752231da7be2c1a6e81e6) )
MEGADRIVE_ROM_LOAD( mmprtmpd, "mighty morphin power rangers - the movie (prototype - jul 24, 1995).bin",                 0x000000, 0x200000,  CRC(3429fa3a) SHA1(7a567082b0314d3e8d677d07b8bccaf307c8c9c2) )
MEGADRIVE_ROM_LOAD( mmprtmu, "mighty morphin power rangers - the movie (usa).bin",                                       0x000000, 0x200000,  CRC(aa941cbc) SHA1(866d7ccc204e45d188594dde99c2ea836912a136) )
MEGADRIVE_ROM_LOAD( mikedi, "mike ditka power football (usa, europe) (alt).bin",                                         0x000000, 0x100000,  CRC(de50ca8e) SHA1(e0832fcd63fb164cac66c3df4b5dfb23eecbb0f6) )
MEGADRIVE_ROM_LOAD( miked1, "mike ditka power football (usa, europe).bin",                                               0x000000, 0x100000,  CRC(6078b310) SHA1(e49b9eb91b2e951efe4509ae8a9a5a083afeb920) )
MEGADRIVE_ROM_LOAD( minato, "minato no traysia (japan).bin",                                                             0x000000, 0x100000,  CRC(bd89fd09) SHA1(dea227a41a5ba28f8c8ea75cba12965bbc5ff8da) )
MEGADRIVE_ROM_LOAD( minnesot, "minnesota fats - pool legend (usa).bin",                                                  0x000000, 0x100000,  CRC(38174f40) SHA1(cf3086b664312d03c749f5439f1bdc6785f035cc) )
MEGADRIVE_ROM_LOAD( miracle, "miracle piano teaching system (usa).bin",                                                  0x000000, 0x80000,   CRC(a719542e) SHA1(5141cb7cc03bc087c17cc663ea3cd889e6faa16c) )
MEGADRIVE_ROM_LOAD( mk5mor, "mk 5 - mortal combat - subzero (unlicensed).bin",                                           0x000000, 0x200000,  CRC(11e367a1) SHA1(8f92ce78be753748daeae6e16e1eed785f99d287) )
MEGADRIVE_ROM_LOAD( mk5mor1, "mk 5 - mortal combat - subzero (unlicensed) (pirate).bin",                                 0x000000, 0x200000,  CRC(41203006) SHA1(a558ad8de61c4d21c35d4dbaaede85d771e84f33) )
MEGADRIVE_ROM_LOAD( mlbpab, "mlbpa baseball (usa).bin",                                                                  0x000000, 0x200000,  CRC(14a8064d) SHA1(5a85c659db9dd7485ed1463a252f0941346aba24) )
MEGADRIVE_ROM_LOAD( monob, "monopoly (usa) (beta).bin",                                                                  0x000000, 0x80000,   CRC(dfbcc3fa) SHA1(ca808cc27e3acb7c663a8a229aed75cb2407366e) )
MEGADRIVE_ROM_LOAD( mono, "monopoly (usa).bin",                                                                          0x000000, 0x80000,   CRC(c10268da) SHA1(8ff34d2557270f91e2032cba100cd65eb51129ca) )
MEGADRIVE_ROM_LOAD( mworld4, "monster world iv (japan).bin",                                                             0x000000, 0x200000,  CRC(36a3aaa4) SHA1(46ba5e8775a2223fe5056f54555d9caa7d04f4e1) )
MEGADRIVE_ROM_LOAD( mk, "mortal kombat (world) (v1.1).bin",                                                              0x000000, 0x200000,  CRC(33f19ab6) SHA1(2c4a0618cc93ef7be8329a82ca6d2d16f49b23e0) )
MEGADRIVE_ROM_LOAD( mka, "mortal kombat (world).bin",                                                                    0x000000, 0x200000,  CRC(1aa3a207) SHA1(c098bf38ddd755ab7caa4612d025be2039009eb2) )
MEGADRIVE_ROM_LOAD( mk3, "mortal kombat 3 (europe).bin",                                                                 0x000000, 0x400000,  CRC(af6de3e8) SHA1(7f555d647972fee4e86b66e840848e91082f9c2d) )
MEGADRIVE_ROM_LOAD( mk3u, "mortal kombat 3 (usa).bin",                                                                   0x000000, 0x400000,  CRC(dd638af6) SHA1(55cdcba77f7fcd9994e748524d40c98089344160) )
MEGADRIVE_ROM_LOAD( mk2, "mortal kombat ii (world).bin",                                                                 0x000000, 0x300000,  CRC(a9e013d8) SHA1(af6d2db16f2b76940ff5a9738f1e00c4e7ea485e) )
MEGADRIVE_ROM_LOAD( mrnutz, "mr. nutz (europe).bin",                                                                     0x000000, 0x100000,  CRC(0786ea0b) SHA1(318ff3a44554b75260d5b9b9e7b81a3cfd07581a) )
MEGADRIVE_ROM_LOAD( mspacman, "ms. pac-man (usa, europe).bin",                                                           0x000000, 0x20000,   CRC(af041be6) SHA1(29fff97e19a00904846ad99baf6b9037b28df15f) )
MEGADRIVE_ROM_LOAD( mspacmanu, "ms. pac-man (usa) (pirate).bin",                                                         0x000000, 0x20000,   CRC(39b51b26) SHA1(7f163f9dd6d8ab6a0f4a33d3073f9ca520abf271) )
MEGADRIVE_ROM_LOAD( mugens, "mugen senshi valis (japan).bin",                                                            0x000000, 0x100000,  CRC(24431625) SHA1(06208a19ca5f2b25bef7c972ec175d4aff235a77) )
MEGADRIVE_ROM_LOAD( muhamm, "muhammad ali heavyweight boxing (europe).bin",                                              0x000000, 0x100000,  CRC(8ea4717b) SHA1(d79218ef96d2f1d9763dec2a9e6ad7c2907d024b) )
MEGADRIVE_ROM_LOAD( muhammub, "muhammad ali heavyweight boxing (usa) (beta).bin",                                        0x000000, 0x100000,  CRC(7b852653) SHA1(f509c3a49cf18af44da3e4605db8790a1ab70a32) )
MEGADRIVE_ROM_LOAD( muhammu, "muhammad ali heavyweight boxing (usa).bin",                                                0x000000, 0x100000,  CRC(b638b6a3) SHA1(caad1bc67fd3d7f7add0a230def4a0d5a7f196ef) )
MEGADRIVE_ROM_LOAD( musha, "musha - metallic uniframe super hybrid armor (usa).bin",                                     0x000000, 0x80000,   CRC(58a7f7b4) SHA1(821eea5d357f26710a4e2430a2f349a80df5f2f6) )
MEGADRIVE_ROM_LOAD( mushaj, "musha aleste - full metal fighter ellinor (japan).bin",                                     0x000000, 0x80000,   CRC(8fde18ab) SHA1(71d363186b9ee023fd2ae1fb9f518c59bf7b8bee) )
MEGADRIVE_ROM_LOAD( mutantlfj, "mutant league football (japan).bin",                                                     0x000000, 0x100000,  CRC(2a97e6af) SHA1(ce68da5d70dddc0291b09b1e5790739ef6ac0748) )
MEGADRIVE_ROM_LOAD( mutantlf, "mutant league football (usa, europe).bin",                                                0x000000, 0x100000,  CRC(dce29c9d) SHA1(6f8638a1c56229ddcb71c9da9f652b49c2978f44) )
MEGADRIVE_ROM_LOAD( mlhockey, "mutant league hockey (usa, europe).bin",                                                  0x000000, 0x200000,  CRC(3529180f) SHA1(84e203c5226bc1913a485804e59c6418e939bd3d) )
MEGADRIVE_ROM_LOAD( mysticd, "mystic defender (usa, europe) (rev a).bin",                                                0x000000, 0x80000,   CRC(50fd5d93) SHA1(aec8aefa11233699eacefdf2cf17d62f68cbdd98) )
MEGADRIVE_ROM_LOAD( mysticd1, "mystic defender (usa, europe).bin",                                                       0x000000, 0x80000,   CRC(f9ce1ab8) SHA1(fd82ea9badc892bfabacaad426f57ba04c6183b1) )
MEGADRIVE_ROM_LOAD( mysticf, "mystical fighter (usa).bin",                                                               0x000000, 0x80000,   CRC(b2f2a69b) SHA1(b8a602692e925d7e394afacd3269f2472d03635c) )
MEGADRIVE_ROM_LOAD( nakaf1gp, "nakajima satoru kanshuu f1 grand prix (japan).bin",                                       0x000000, 0x100000,  CRC(93be47cf) SHA1(e108342d53e1542d572d2e45524efbfe9d5dc964) )
MEGADRIVE_ROM_LOAD( nakaf1he, "nakajima satoru kanshuu f1 hero md (japan).bin",                                          0x000000, 0x100000,  CRC(24f87987) SHA1(1a61809b4637ba48d193d29a5c25d72ab2c6d72d) )
MEGADRIVE_ROM_LOAD( nakaf1sl, "nakajima satoru kanshuu f1 super license (japan).bin",                                    0x000000, 0x100000,  CRC(8774bc79) SHA1(ad5456259890bcb32098f82f14a1d5355af83f7e) )
MEGADRIVE_ROM_LOAD( naomichi, "naomichi ozaki no super masters (japan).bin",                                             0x000000, 0x80000,   CRC(088ba825) SHA1(662a87d679066aad0638b2ffb01c807d9f376121) )
MEGADRIVE_ROM_LOAD( nbaa, "nba action (usa).bin",                                                                        0x000000, 0x200000,  CRC(99c348ba) SHA1(e2b5290d656219636e2422fcf93424ae602c4d29) )
MEGADRIVE_ROM_LOAD( nbaa95a, "nba action '95 (prototype - dec 02, 1994 - b).bin",                                        0x000000, 0x200000,  CRC(04bf6f6e) SHA1(42c55adad0249bb09350d1ac7c9bfb737ed091c8) )
MEGADRIVE_ROM_LOAD( nbaa95b, "nba action '95 (prototype - dec 09, 1994).bin",                                            0x000000, 0x200000,  CRC(2b198487) SHA1(2c34500bf06bbac610e8fca45db48382e32c8807) )
MEGADRIVE_ROM_LOAD( nbaa95c, "nba action '95 (prototype - dec 15, 1994).bin",                                            0x000000, 0x200000,  CRC(10a3b46d) SHA1(c3f88b334af683e8ac98cafafa9abf4dfe65a4b7) )
MEGADRIVE_ROM_LOAD( nbaa95d, "nba action '95 (prototype - dec 22, 1994 - a).bin",                                        0x000000, 0x200000,  CRC(f92ba323) SHA1(35f7436fa15591234edcb6fe72da24d091963d30) )
MEGADRIVE_ROM_LOAD( nbaa95e, "nba action '95 (prototype - dec 24, 1994 - a).bin",                                        0x000000, 0x200000,  CRC(ddb04550) SHA1(248e67cad67118a1449de308bac0437641bda3ec) )
MEGADRIVE_ROM_LOAD( nbaa95f, "nba action '95 (prototype - dec 29, 1994).bin",                                            0x000000, 0x200000,  CRC(dcebe32a) SHA1(2604a03c1dc59538a82e32bcc6f8a995bd8af609) )
MEGADRIVE_ROM_LOAD( nbaa95g, "nba action '95 (prototype - dec 30, 1994).bin",                                            0x000000, 0x200000,  CRC(d87956c9) SHA1(4ddbe2f458915db0da45fa490d653f2d94ec1263) )
MEGADRIVE_ROM_LOAD( nbaa95h, "nba action '95 (prototype - dec 31, 1994).bin",                                            0x000000, 0x200000,  CRC(582a378f) SHA1(ba08a3f042f96b4b3bb889bbacb6a5e13b114f0c) )
MEGADRIVE_ROM_LOAD( nbaa95i, "nba action '95 (prototype - feb 01, 1995).bin",                                            0x000000, 0x200000,  CRC(2c02a79d) SHA1(3dbc1a80005eb6783feeb4d3604d382d1cf688bc) )
MEGADRIVE_ROM_LOAD( nbaa95j, "nba action '95 (prototype - feb 1, 1995).bin",                                             0x000000, 0x1ff49e,  CRC(c4a0a624) SHA1(58c88d26baffd0f68c2b5d95284323ba99db9b5a) )
MEGADRIVE_ROM_LOAD( nbaa95k, "nba action '95 (prototype - feb 2, 1995).bin",                                             0x000000, 0x1fe1f0,  CRC(a9167903) SHA1(3a5a95a79b1b2da0b35e8cde02d8645fe474fdde) )
MEGADRIVE_ROM_LOAD( nbaa95l, "nba action '95 (prototype - jan 03, 1995).bin",                                            0x000000, 0x200000,  CRC(374af160) SHA1(b50be710436a3cb1f7644fdfac5d5098cd9dbb2b) )
MEGADRIVE_ROM_LOAD( nbaa95m, "nba action '95 (prototype - jan 08, 1995).bin",                                            0x000000, 0x200000,  CRC(0a6b7b9e) SHA1(3a60332ee684ff8accd96aef404346e66e267b6f) )
MEGADRIVE_ROM_LOAD( nbaa95n, "nba action '95 (prototype - jan 12, 1995).bin",                                            0x000000, 0x200000,  CRC(b47ff025) SHA1(9f6a2ea386d383aee3be06d6b74fda67b1ebd960) )
MEGADRIVE_ROM_LOAD( nbaa95o, "nba action '95 (prototype - jan 15, 1995 - a).bin",                                        0x000000, 0x200000,  CRC(9c6a1f27) SHA1(2fdb8879d50d963d984c280cab2e279b9479081f) )
MEGADRIVE_ROM_LOAD( nbaa95p, "nba action '95 (prototype - jan 21, 1995).bin",                                            0x000000, 0x200000,  CRC(e337bdfb) SHA1(591288688956cec3d0aca3dd099b3e0985ca947a) )
MEGADRIVE_ROM_LOAD( nbaa95q, "nba action '95 (prototype - jan 22, 1995 - b).bin",                                        0x000000, 0x200000,  CRC(7bd7ca47) SHA1(f219274a65c31d76a2d6633b7e7cf65462850f47) )
MEGADRIVE_ROM_LOAD( nbaa95r, "nba action '95 (prototype - jan 24, 1995 - b).bin",                                        0x000000, 0x200000,  CRC(15abcf41) SHA1(eb99f0e1cac800f94743fde873448e56edb46333) )
MEGADRIVE_ROM_LOAD( nbaa95s, "nba action '95 (prototype - jan 27, 1995 - a).bin",                                        0x000000, 0x1fe190,  CRC(79c7fb00) SHA1(90c246dcb8ccea0f30ae5582b610721fc802f937) )
MEGADRIVE_ROM_LOAD( nbaa95t, "nba action '95 (prototype - jan 27, 1995 - b).bin",                                        0x000000, 0x1fe190,  CRC(ef7f608b) SHA1(32104d3585dcbf190904accdb9528f0b7105eb4b) )
MEGADRIVE_ROM_LOAD( nbaa95u, "nba action '95 (prototype - jan 28, 1995 - a).bin",                                        0x000000, 0x200000,  CRC(57c2c69c) SHA1(f1affb6e01ca23b67aee9e1c1767f9ec13849823) )
MEGADRIVE_ROM_LOAD( nbaa95v, "nba action '95 (prototype - jan 28, 1995).bin",                                            0x000000, 0x1fe19e,  CRC(97d7075d) SHA1(dd1256efa397a56d461e2bc7aec9f72aff9b04fb) )
MEGADRIVE_ROM_LOAD( nbaa95w, "nba action '95 (prototype - jan 30, 1995).bin",                                            0x000000, 0x200000,  CRC(2fac80b2) SHA1(ab90daf9791fa7347fb8f040e27f91b6bae46e1e) )
MEGADRIVE_ROM_LOAD( nbaa95x, "nba action '95 (prototype - nov 18, 1994).bin",                                            0x000000, 0x200000,  CRC(2d411e4b) SHA1(492776fc2659091d435a79588efcdd8e06d3cd0b) )
MEGADRIVE_ROM_LOAD( nbaa95y, "nba action '95 (prototype - nov 23, 1994 - a).bin",                                        0x000000, 0x200000,  CRC(4650cfcc) SHA1(0eee2e7296e3eed5dbf85954ce14a53622ae3d64) )
MEGADRIVE_ROM_LOAD( nbaa95, "nba action '95 starring david robinson (usa, europe).bin",                                  0x000000, 0x200000,  CRC(aa7006d6) SHA1(34e2df219e09c24c95c588a37d2a2c5e15814d68) )
MEGADRIVE_ROM_LOAD( nbaap1, "nba action (prototype - jan 04, 1994).bin",                                                 0x000000, 0x200000,  CRC(2491df2f) SHA1(af63f96417a189f5061f81ba0354d4e38b0b7d76) )
MEGADRIVE_ROM_LOAD( nbaap2, "nba action (prototype - jan 16, 1994).bin",                                                 0x000000, 0x200000,  CRC(fe43c79d) SHA1(da6fcc1d7069e315797dd40a89e21963ab766b9e) )
MEGADRIVE_ROM_LOAD( nbaap3, "nba action (prototype - jan 27, 1994) (broken - c08 missing).bin",                          0x000000, 0x200000,  CRC(176a4bc5) SHA1(d5a6fbf8fe19bf70631a847e6b5e6a35878c7ae8) )
MEGADRIVE_ROM_LOAD( nbaallst, "nba all-star challenge (usa, europe).bin",                                                0x000000, 0x100000,  CRC(c4674adf) SHA1(fc55c83df4318a17b55418ce14619e42805e497f) )
MEGADRIVE_ROM_LOAD( nbahang, "nba hang time (europe).bin",                                                               0x000000, 0x300000,  CRC(edb4d4aa) SHA1(4594ba338a07dd79639c11b5b96c7f1a6e283d0c) )
MEGADRIVE_ROM_LOAD( nbahangu, "nba hang time (usa).bin",                                                                 0x000000, 0x300000,  CRC(176b0338) SHA1(bff36de7e0ca875b1fab84928f00999b48ff8f02) )
MEGADRIVE_ROM_LOAD( nbajamj, "nba jam (japan).bin",                                                                      0x000000, 0x200000,  CRC(a6c6305a) SHA1(2a88b2e1ecf115fa6246397d829448b755a5385e) )
MEGADRIVE_ROM_LOAD( nbajam, "nba jam (usa, europe) (v1.1).bin",                                                          0x000000, 0x200000,  CRC(eb8360e6) SHA1(55f2b26a932c69b2c7cb4f24f56b43f24f113a7c) )
MEGADRIVE_ROM_LOAD( nbajam1, "nba jam (usa, europe).bin",                                                                0x000000, 0x200000,  CRC(10fa248f) SHA1(99c5bc57fdea7f9df0cd8dec54160b162342344d) )
MEGADRIVE_ROM_LOAD( nbajamte, "nba jam tournament edition (world).bin",                                                  0x000000, 0x300000,  CRC(e9ffcb37) SHA1(ddbf09c5e6ed5d528ef5ec816129a332c685f103) )
MEGADRIVE_ROM_LOAD( nbajamtef, "nba jam tournament edition (world) (fixed).bin",                                          0x000000, 0x300000,  CRC(6e25ebf0) SHA1(0f5bb5d5352fe2ebe4b4051a1dd9b9fde4b505ab) )
// this "v1.1" dump was supposedly released in 2002 by one of the original programmers to fix a bug in the first version, was it ever found on a cart?

MEGADRIVE_ROM_LOAD( nbal95k, "nba live 95 (korea) (lh5370hc).bin",                                                       0x000000, 0x200000,  CRC(779c1244) SHA1(115c2e8cfa6bc45767ba47efc00aede424a6de66) )
MEGADRIVE_ROM_LOAD( nbal95, "nba live 95 (usa, europe).bin",                                                             0x000000, 0x200000,  CRC(66018abc) SHA1(f86bc9601751ac94119ab2f3ecce2029d5678f01) )
MEGADRIVE_ROM_LOAD( nbal96, "nba live 96 (usa, europe).bin",                                                             0x000000, 0x200000,  CRC(49de0062) SHA1(5fca106c839d3dea11cbf6842d1d7650db06ca72) )
MEGADRIVE_ROM_LOAD( nbal97, "nba live 97 (usa, europe).bin",                                                             0x000000, 0x200000,  CRC(7024843a) SHA1(1671451ab4ab6991e13db70671054c0f2c652a95) )
MEGADRIVE_ROM_LOAD( nbal98, "nba live 98 (usa).bin",                                                                     0x000000, 0x200000,  CRC(23473a8a) SHA1(89b98867c7393371a364de58ba6955e0798fa10f) )
MEGADRIVE_ROM_LOAD( nbaplay, "nba playoffs - bulls vs blazers (japan).bin",                                              0x000000, 0x100000,  CRC(eae8c000) SHA1(66a25cf2d7ddf17dde137e29381202f48456d173) )
MEGADRIVE_ROM_LOAD( nbaplay1, "nba playoffs - bulls vs blazers (japan) (alt).bin",                                       0x000000, 0x100000,  CRC(4565ce1f) SHA1(77a048094269fbf5cf9f0ad02ea781ba3bcbecf3) )
// Xacrow's dump info report only the other CRC, this is probably a bad dump

MEGADRIVE_ROM_LOAD( nbapro94, "nba pro basketball '94 (japan).bin",                                                      0x000000, 0x200000,  CRC(eea19bce) SHA1(99c91fe3a5401e84e7b3fd2218dcb3aeaf10db74) )
MEGADRIVE_ROM_LOAD( nbapro, "nba pro basketball - bulls vs lakers (japan).bin",                                          0x000000, 0x100000,  CRC(4416ce39) SHA1(003b01282aff14ccea9390551aba5ac4a9e53825) )
MEGADRIVE_ROM_LOAD( nbashowb, "nba showdown '94 (usa) (beta).bin",                                                       0x000000, 0x200000,  CRC(6643a308) SHA1(e804ca0f4da505056f0813f00df9b139248f59af) )
MEGADRIVE_ROM_LOAD( nbashow, "nba showdown '94 (usa, europe).bin",                                                       0x000000, 0x200000,  CRC(160b7090) SHA1(3134a3cb63115d2e16e63a76c2708cdaecab83e4) )
MEGADRIVE_ROM_LOAD( ncaabask, "ncaa final four basketball (usa).bin",                                                    0x000000, 0x180000,  CRC(ed0c1303) SHA1(29021b8c3bbcc62606c692a3de90d4e7a71b6361) )
MEGADRIVE_ROM_LOAD( ncaafoot, "ncaa football (usa).bin",                                                                 0x000000, 0x100000,  CRC(081012f0) SHA1(227e3c650d01c35a80de8a3ef9b18f96c07ecd38) )
MEGADRIVE_ROM_LOAD( nekketsu, "nekketsu koukou dodgeball bu - soccer hen md (japan).bin",                                0x000000, 0x80000,   CRC(f49c3a86) SHA1(d865b01e58a269400de369fc1fbb3b3e84e1add0) )
MEGADRIVE_ROM_LOAD( n3dgdevi, "new 3d golf simulation devil's course (japan).bin",                                       0x000000, 0x180000,  CRC(bd090c67) SHA1(4509c10ab263175c605111c72c6c63c57321046a) )
MEGADRIVE_ROM_LOAD( n3dgaugu, "mpr-16076.bin",                                                                           0x000000, 0x180000,  CRC(d2a9bf92) SHA1(d023f9fc5d7c7f2873a5bf79f6035111b78cdd5d) )
MEGADRIVE_ROM_LOAD( n3dgpebb, "new 3d golf simulation pebble beach no hatou (japan).bin",                                0x000000, 0x180000,  CRC(96ed2e5d) SHA1(dee8430f4f15b4d04e1e3c0cc9d7d7f55ca1ad7b) )
MEGADRIVE_ROM_LOAD( n3dgwaia, "new 3d golf simulation waialae no kiseki (japan).bin",                                    0x000000, 0x180000,  CRC(cbe2c1f6) SHA1(5d78c5cf3a514275a7df6d9fdbd708584b42e697) )
MEGADRIVE_ROM_LOAD( tnzs, "new zealand story, the (japan).bin",                                                          0x000000, 0x80000,   CRC(1c77ad21) SHA1(07b75c12667c6c5620aa9143b4c5f35b76b15418) )
MEGADRIVE_ROM_LOAD( newman, "newman haas indy car featuring nigel mansell (world).bin",                                  0x000000, 0x200000,  CRC(1233a229) SHA1(e7083aaa4a4f6539b10b054aa4ecdcee52f3f8cc) )
MEGADRIVE_ROM_LOAD( nfl95a, "nfl '95 (prototype - aug 01, 1994).bin",                                                    0x000000, 0x200000,  CRC(5a57ea4e) SHA1(e7a8421a83195a71a4cf129e853d532b6114b1f6) )
MEGADRIVE_ROM_LOAD( nfl95b, "nfl '95 (prototype - aug 05, 1994).bin",                                                    0x000000, 0x200000,  CRC(88d51773) SHA1(7f453ca6499263054e2c649b508811b46d6edf4f) )
MEGADRIVE_ROM_LOAD( nfl95c, "nfl '95 (prototype - aug 10, 1994).bin",                                                    0x000000, 0x200000,  CRC(a3c5710b) SHA1(c55ff8f24d99c57c23ad1ef9fe43a7a142dcdc32) )
MEGADRIVE_ROM_LOAD( nfl95d, "nfl '95 (prototype - aug 12, 1994).bin",                                                    0x000000, 0x200000,  CRC(3d23633b) SHA1(7cfb0e06cc41d11ec51ca5f1959a5616010892ad) )
MEGADRIVE_ROM_LOAD( nfl95e, "nfl '95 (prototype - aug 12, 1994)_.bin",                                                   0x000000, 0x200000,  CRC(cc86a259) SHA1(8a7320033449cc9972b2db25752150fe4c2a1fab) )
MEGADRIVE_ROM_LOAD( nfl95f, "nfl '95 (prototype - aug 17, 1994 - b).bin",                                                0x000000, 0x200000,  CRC(a6015c29) SHA1(839de31449298d0970bde787381a5e331e1fd31f) )
MEGADRIVE_ROM_LOAD( nfl95g, "nfl '95 (prototype - aug 17, 1994).bin",                                                    0x000000, 0x200000,  CRC(655966c8) SHA1(bb949355b764a48015edc4fbbf0f89d4c7400c31) )
MEGADRIVE_ROM_LOAD( nfl95h, "nfl '95 (prototype - aug 22, 1994).bin",                                                    0x000000, 0x200000,  CRC(b01434e5) SHA1(d1a48818a982025b0833c06a584382122d1ccfb2) )
MEGADRIVE_ROM_LOAD( nfl95i, "nfl '95 (prototype - aug 30, 1994).bin",                                                    0x000000, 0x200000,  CRC(d0e0a436) SHA1(c42a2170449e4e26beaa59cc4909e505be751aa9) )
MEGADRIVE_ROM_LOAD( nfl95j, "nfl '95 (prototype - aug 31, 1994).bin",                                                    0x000000, 0x200000,  CRC(c360521e) SHA1(fd345d92799224475f0107751512adf584899c6b) )
MEGADRIVE_ROM_LOAD( nfl95k, "nfl '95 (prototype - sep 01, 1994).bin",                                                    0x000000, 0x200000,  CRC(9240fcaa) SHA1(ea1126f9cade3ec680dd029af10a5268cd5afa72) )
MEGADRIVE_ROM_LOAD( nfl95l, "nfl '95 (prototype - sep 02, 1994).bin",                                                    0x000000, 0x200000,  CRC(ff049a49) SHA1(83c70177164e418b988ed9144d9d12c0e7052c3b) )
MEGADRIVE_ROM_LOAD( nfl95m, "nfl '95 (prototype - sep 04, 1994).bin",                                                    0x000000, 0x200000,  CRC(60604d40) SHA1(c07558f34b7480e063eb60126fc33fba36ae3daa) )
MEGADRIVE_ROM_LOAD( nfl95n, "nfl '95 (prototype - sep 05, 1994 - b).bin",                                                0x000000, 0x200000,  CRC(123985a5) SHA1(f25cbabd3f6284387e66eaa72fa4124f3768121e) )
MEGADRIVE_ROM_LOAD( nfl95o, "nfl '95 (prototype - sep 05, 1994).bin",                                                    0x000000, 0x200000,  CRC(dbdf61ac) SHA1(787f8093d8a5c55ee9dcaa49925752169527fa62) )
MEGADRIVE_ROM_LOAD( nfl95p, "nfl '95 (prototype - sep 06, 1994).bin",                                                    0x000000, 0x200000,  CRC(624b8699) SHA1(59c548514bee4be7a5e6e3483ac3122d410a7e4e) )
MEGADRIVE_ROM_LOAD( nfl95q, "nfl '95 (prototype - sep 07, 1994).bin",                                                    0x000000, 0x200000,  CRC(ae8aa4be) SHA1(ddf2b988bc035a0736ed3a1d7c8e6aa8cd2625f8) )
MEGADRIVE_ROM_LOAD( nfl95r, "nfl '95 (prototype - sep 08, 1994).bin",                                                    0x000000, 0x200000,  CRC(22c5e289) SHA1(f8689fd01dedced28ca417bf7ff8a9eea9973c3d) )
MEGADRIVE_ROM_LOAD( nfl95s, "nfl '95 (prototype - sep 09, 1994).bin",                                                    0x000000, 0x200000,  CRC(55df1066) SHA1(2ba79a9a638894abbcd0702ba380b68ccaebb50f) )
MEGADRIVE_ROM_LOAD( nfl95t, "nfl '95 (prototype - sep 11, 1994 - b).bin",                                                0x000000, 0x200000,  CRC(111ac6a0) SHA1(2e3e044bd6061002444bdc4bbe5de1f8c8417ce6) )
MEGADRIVE_ROM_LOAD( nfl95u, "nfl '95 (prototype - sep 11, 1994).bin",                                                    0x000000, 0x200000,  CRC(21614c4f) SHA1(91babf6f0d86e19e82c3f045d01e99ea9fe0253c) )
MEGADRIVE_ROM_LOAD( nfl95, "nfl '95 (usa, europe).bin",                                                                  0x000000, 0x200000,  CRC(b58e4a81) SHA1(327249456f96ccb3e9758c0162c5f3e3f389072f) )
MEGADRIVE_ROM_LOAD( nfl98, "nfl 98 (usa).bin",                                                                           0x000000, 0x200000,  CRC(f73ec54c) SHA1(27ea0133251e96c091b5a96024eb099cdca21e40) )
MEGADRIVE_ROM_LOAD( nfl94j, "nfl football '94 (japan).bin",                                                              0x000000, 0x200000,  CRC(e490dc4a) SHA1(a530a93d124ccecfbf54f0534b6d3269026e5988) )
MEGADRIVE_ROM_LOAD( nfl94, "nfl football '94 starring joe montana (usa).bin",                                            0x000000, 0x200000,  CRC(0d486ed5) SHA1(ad0150d0c0cabe1ab05e3d6ac4eb7aa4b38a467c) )
MEGADRIVE_ROM_LOAD( nflqua, "nfl quarterback club (world).bin",                                                          0x000000, 0x300000,  CRC(94542eaf) SHA1(60744af955df83278f119df3478baeebd735a26c) )
MEGADRIVE_ROM_LOAD( nflqua96, "nfl quarterback club 96 (usa, europe).bin",                                               0x000000, 0x400000,  CRC(d5a37cab) SHA1(2948419532a6079404f05348bc4bbf2dd989622d) )
MEGADRIVE_ROM_LOAD( nflsport, "nfl sports talk football '93 starring joe montana (usa, europe).bin",                     0x000000, 0x180000,  CRC(ce0b1fe1) SHA1(3c97396cb4cc566013d06f055384105983582067) )
MEGADRIVE_ROM_LOAD( nhktaiga, "nhk taiga drama - taiheiki (japan).bin",                                                  0x000000, 0x100000,  CRC(09fbb30e) SHA1(67358048fc9bc93d8835af852bc437124e015a61) )
MEGADRIVE_ROM_LOAD( nhl95, "nhl 95 (usa, europe).bin",                                                                   0x000000, 0x200000,  CRC(e8ee917e) SHA1(09e87b076aa4cd6f057a1d65bb50fd889b509b44) )
MEGADRIVE_ROM_LOAD( nhl96, "nhl 96 (usa, europe).bin",                                                                   0x000000, 0x200000,  CRC(8135702c) SHA1(7204633fbb9966ac637e7966d02ba15c5acdee6b) )
MEGADRIVE_ROM_LOAD( nhl97, "nhl 97 (usa, europe).bin",                                                                   0x000000, 0x200000,  CRC(f067c103) SHA1(8a90d7921ab8380c0abb0b5515a6b9f96ca6023c) )
MEGADRIVE_ROM_LOAD( nhl98, "nhl 98 (usa).bin",                                                                           0x000000, 0x200000,  CRC(7b64cd98) SHA1(6771e9b660bde010cf28656cafb70f69249a3591) )
MEGADRIVE_ROM_LOAD( nhlasa, "nhl all-star hockey '95 (prototype - dec 01, 1994 - b).bin",                                0x000000, 0x1ffffa,  CRC(c209d720) SHA1(ff9e08e779ed07f0d055ba7ba34d602c5e6f892d) )
MEGADRIVE_ROM_LOAD( nhlasb, "nhl all-star hockey '95 (prototype - dec 01, 1994).bin",                                    0x000000, 0x1ffffa,  CRC(112e4cb3) SHA1(212449ee98b48e3aa8e7bee5791e6359f10f06de) )
MEGADRIVE_ROM_LOAD( nhlasc, "nhl all-star hockey '95 (prototype - dec 02, 1994).bin",                                    0x000000, 0x1ffffa,  CRC(01d6a6c7) SHA1(130f95bccd159234c117890b0412b415b43161a9) )
MEGADRIVE_ROM_LOAD( nhlasd, "nhl all-star hockey '95 (prototype - nov 07, 1994).bin",                                    0x000000, 0x200000,  CRC(e8f2c88f) SHA1(3f62734a8c7e56ccde24cc6d02c7ce1a95b4eec5) )
MEGADRIVE_ROM_LOAD( nhlase, "nhl all-star hockey '95 (prototype - nov 09, 1994).bin",                                    0x000000, 0x1ffff9,  CRC(82624073) SHA1(668a8453bf88e8d27a6f6d123198aadda05215c5) )
MEGADRIVE_ROM_LOAD( nhlasf, "nhl all-star hockey '95 (prototype - nov 19, 1994).bin",                                    0x000000, 0x200000,  CRC(cd0f588a) SHA1(c2133c68fbc0daa9b0bd97d21dc73d03278805dd) )
MEGADRIVE_ROM_LOAD( nhlasg, "nhl all-star hockey '95 (prototype - nov 21, 1994).bin",                                    0x000000, 0x200000,  CRC(cfe4059d) SHA1(8e1806d57d2eb876543fe212e470d73f2d36a3e5) )
MEGADRIVE_ROM_LOAD( nhlash, "nhl all-star hockey '95 (prototype - nov 22, 1994).bin",                                    0x000000, 0x1ffffa,  CRC(81d30fe6) SHA1(5d88854550a5b69b2623b5ba74424b1e191b9682) )
MEGADRIVE_ROM_LOAD( nhlasi, "nhl all-star hockey '95 (prototype - nov 23, 1994).bin",                                    0x000000, 0x1ffffa,  CRC(88d4fc38) SHA1(4550baf4f39ff20aacb52aade9d03598c47ce1b2) )
MEGADRIVE_ROM_LOAD( nhlasj, "nhl all-star hockey '95 (prototype - nov 27, 1994).bin",                                    0x000000, 0x1ffffa,  CRC(658ffe52) SHA1(c36c767bc1cfcc3301ca84fa37bc22bfb1e3678d) )
MEGADRIVE_ROM_LOAD( nhlask, "nhl all-star hockey '95 (prototype - nov 28, 1994).bin",                                    0x000000, 0x1ffffa,  CRC(51ad0056) SHA1(5e54fe1f0a3dee5a9cec0722ab9eb5f17b6862db) )
MEGADRIVE_ROM_LOAD( nhlasl, "nhl all-star hockey '95 (prototype - nov 29, 1994).bin",                                    0x000000, 0x1ffffa,  CRC(a060a60b) SHA1(574bbf1b0890ca46fbee9e3f211060f15c02d037) )
MEGADRIVE_ROM_LOAD( nhlasm, "nhl all-star hockey '95 (prototype - oct 01, 1994).bin",                                    0x000000, 0x1ffff9,  CRC(b1c0fbb2) SHA1(2529a10980dea625a3ea386dec05ee3f1c5e5ee8) )
MEGADRIVE_ROM_LOAD( nhlasn, "nhl all-star hockey '95 (prototype - oct 21, 1994).bin",                                    0x000000, 0x200000,  CRC(17eb2238) SHA1(7e94021e7e00dae9287b827e11a2c7ba510c815d) )
MEGADRIVE_ROM_LOAD( nhlaso, "nhl all-star hockey '95 (prototype - sep 14, 1994).bin",                                    0x000000, 0x200000,  CRC(5d4bc48d) SHA1(b2fff4f0e1d87dd001aad44edfbcde0266f5fb7c) )
MEGADRIVE_ROM_LOAD( nhlasp, "nhl all-star hockey '95 (prototype - sep 29, 1994).bin",                                    0x000000, 0x1ffff9,  CRC(96bae44f) SHA1(edae68144c9b90764e7591be23e27ff1a34b2ab2) )
MEGADRIVE_ROM_LOAD( nhlas, "nhl all-star hockey '95 (usa).bin",                                                          0x000000, 0x200000,  CRC(e6c0218b) SHA1(f2069295f99b5d50444668cfa9d4216d988d5f46) )
MEGADRIVE_ROM_LOAD( nhlhoc, "nhl hockey (usa).bin",                                                                      0x000000, 0x80000,   CRC(2641653f) SHA1(2337ad3c065f46edccf19007d7a5be80a97520d5) )
MEGADRIVE_ROM_LOAD( nhlpah, "nhlpa hockey 93 (usa, europe) (v1.1).bin",                                                  0x000000, 0x80000,   CRC(f361d0bf) SHA1(2ab048fc7209df28b00ef47f2f686f5b7208466e) )
MEGADRIVE_ROM_LOAD( nhlpah1, "nhlpa hockey 93 (usa, europe).bin",                                                        0x000000, 0x80000,   CRC(cbbf4262) SHA1(8efc1cacb079ea223966dda065c16c49e584cac2) )
MEGADRIVE_ROM_LOAD( nigelm, "nigel mansell's world championship racing (europe).bin",                                    0x000000, 0x100000,  CRC(3fe3d63b) SHA1(81b6d231998c0dd90caa9325b9cc6e50c6e622bb) )
MEGADRIVE_ROM_LOAD( nigelmu, "nigel mansell's world championship racing (usa).bin",                                      0x000000, 0x100000,  CRC(6bc57b2c) SHA1(a64b3c8e8ad221cd02d47d551f3492515658ee1d) )
MEGADRIVE_ROM_LOAD( ncirc, "nightmare circus (brazil).bin",                                                              0x000000, 0x200000,  CRC(06da3217) SHA1(3c80ff5ba54abe4702a3bb7d812571d3dc50c00f) )
MEGADRIVE_ROM_LOAD( ncirc1, "nightmare circus (brazil) (alt).bin",                                                       0x000000, 0x200000,  CRC(31de5a94) SHA1(61b7d25e15011d379d07a8dad4f8c5bb5f75e29f) )
// this is marked as unconfirmed beta in Goodgen, but seems an edited final release, needs a redump

MEGADRIVE_ROM_LOAD( ncircp, "nightmare circus (prototype).bin",                                                          0x000000, 0x400000,  CRC(ee96f1b8) SHA1(c48a0f275a46a4a814a4aa0926c084d27812f5ec) )
MEGADRIVE_ROM_LOAD( nikkan, "nikkan sports pro yakyuu van (japan).bin",                                                  0x000000, 0x80000,   CRC(c3655a59) SHA1(d29f7e70feae3b4f1b423026b7a124514ee48dcf) )
MEGADRIVE_ROM_LOAD( ninjab, "ninja burai densetsu (japan).bin",                                                          0x000000, 0x100000,  CRC(a8d828a0) SHA1(9cc3419ca7ecaf0d106aa896ffc0266c7145fff7) )
MEGADRIVE_ROM_LOAD( ninjag, "ninja gaiden (japan) (prototype) (bad dump).bin",                                           0x000000, 0x100000, BAD_DUMP CRC(0d7f74ba) SHA1(dced2e4eb53ae5d9e979b13f093f70d466634c5b) )
MEGADRIVE_ROM_LOAD( noescape, "no escape (usa).bin",                                                                     0x000000, 0x200000,  CRC(44ee5f20) SHA1(d72302927b618165cd94ec2e072c8d8bbf85cbb9) )
MEGADRIVE_ROM_LOAD( nobubus, "nobunaga no yabou - bushou fuuunroku (japan).bin",                                         0x000000, 0x100000,  CRC(30bf8637) SHA1(1ed50cae48405ec8ad9257df3d445521e60636d8) )
MEGADRIVE_ROM_LOAD( nobuhao, "nobunaga no yabou - haouden (japan).bin",                                                  0x000000, 0x200000,  CRC(96c01fc6) SHA1(9246a29b25e276dc1f7edfae87229549cf87d92e) )
MEGADRIVE_ROM_LOAD( nobuzen, "nobunaga no yabou - zenkoku ban (japan).bin",                                              0x000000, 0x80000,   CRC(1381b313) SHA1(a1f64afbf3bcdc85b61bce96454105128ca6746a) )
MEGADRIVE_ROM_LOAD( nobuamb, "nobunaga's ambition (usa).bin",                                                            0x000000, 0x80000,   CRC(b9bc07bc) SHA1(394c6b46a9e9b9177a135fd8fd43392eb1099666) )
MEGADRIVE_ROM_LOAD( normys, "normy's beach babe-o-rama (usa, europe).bin",                                               0x000000, 0x100000,  CRC(b56a8220) SHA1(286d4f922fa9a95740e791c284d141a49983d871) )
MEGADRIVE_ROM_LOAD( olympi, "olympic gold (europe).bin",                                                                 0x000000, 0x80000,   CRC(924e57d3) SHA1(55702a7dee0cd2092a751b19c04d81694b0c0d0f) )
MEGADRIVE_ROM_LOAD( olymp1, "olympic gold (japan, korea).bin",                                                           0x000000, 0x80000,   CRC(e9c925b8) SHA1(ce640bb979fcb285729b58de4da421c5493ef5e2) )
MEGADRIVE_ROM_LOAD( olymp2, "olympic gold (usa) (alt).bin",                                                              0x000000, 0x80000,   CRC(af639376) SHA1(089c4013e942fcb3e2e6645cf3e702cdc3f9fc36) )
MEGADRIVE_ROM_LOAD( olymp3, "olympic gold (usa).bin",                                                                    0x000000, 0x80000,   CRC(339594b4) SHA1(f9febd976d98545dee35c10b69755908d6929fd4) )
MEGADRIVE_ROM_LOAD( olympsum, "olympic summer games (usa, europe).bin",                                                  0x000000, 0x200000,  CRC(9e470fb9) SHA1(88c8627d0b5bd5f4847d4b62f3681fbadc477829) )
MEGADRIVE_ROM_LOAD( ondalj, "on dal jang goon (korea).bin",                                                              0x000000, 0x80000,   CRC(67ccb1ca) SHA1(821bcb001cb37b22454979973ed68cee36286abc) )
MEGADRIVE_ROM_LOAD( onslau, "onslaught (usa, europe).bin",                                                               0x000000, 0x80000,   CRC(9f19d6df) SHA1(dc542ddfa878f2aed3a7fcedc4b0f8d503eb5d70) )
MEGADRIVE_ROM_LOAD( oozepa, "ooze, the (e) (prototype - jul 28, 1995).bin",                                              0x000000, 0x100000,  CRC(7cf868e7) SHA1(1c34cb3fffbaa81d350aea0b81b74c28e86c9bf5) )
MEGADRIVE_ROM_LOAD( ooze, "ooze, the (europe).bin",                                                                      0x000000, 0x100000,  CRC(e16b102c) SHA1(0e45b84e875aa4e27bf003cbc0f857f2e5659625) )
MEGADRIVE_ROM_LOAD( oozeju, "ooze, the (japan, usa).bin",                                                                0x000000, 0x100000,  CRC(1c0dd42f) SHA1(dafcda90285e71151072ba36134f94f1e9d76a23) )
MEGADRIVE_ROM_LOAD( oozepb, "ooze, the (prototype - jun 15, 1995).bin",                                                  0x000000, 0xe35ce,   CRC(921106fd) SHA1(62da42d68ee70b92886a9435bbe86472ef7271b8) )
MEGADRIVE_ROM_LOAD( oozepc, "ooze, the (prototype - jun 19, 1995).bin",                                                  0x000000, 0x100000,  CRC(fa39281d) SHA1(7cf62d9861a076aab2ccc24f8af42098595d31e7) )
MEGADRIVE_ROM_LOAD( oozepd, "ooze, the (prototype - jun 29, 1995 - b).bin",                                              0x000000, 0xee456,   CRC(ca93e93b) SHA1(1a26f78560b70ba233b54e9506a2d6d444e03bc8) )
MEGADRIVE_ROM_LOAD( oozepe, "ooze, the (prototype - jun 29, 1995).bin",                                                  0x000000, 0x100000,  CRC(1776763b) SHA1(9b48fdcea1f6238ff7a9998632f6910f4a3be63b) )
MEGADRIVE_ROM_LOAD( oozetf, "ooze, the (prototype 104 - jun 22, 1995).bin",                                              0x000000, 0xebea8,   CRC(ce1f139f) SHA1(678a3bc0b0e1cadc3e4eb48fdf685e1e256e0ac3) )
MEGADRIVE_ROM_LOAD( oozetg, "ooze, the (prototype 112 - jun 26, 1995).bin",                                              0x000000, 0xec570,   CRC(70419442) SHA1(de12e7c7a8fdbd032a1892f744d2b75c735ee3b1) )
MEGADRIVE_ROM_LOAD( operat, "operation europe - path to victory 1939-45 (usa).bin",                                      0x000000, 0x100000,  CRC(e7cba1d8) SHA1(e2e484c6db2bb058d04288b4dfea0ed199108d24) )
MEGADRIVE_ROM_LOAD( osomat, "osomatsu-kun hachamecha gekijou (japan).bin",                                               0x000000, 0x40000,   CRC(2453350c) SHA1(bf82cb1c4d2144bd0e0d45629f346988cc4cc5b2) )
MEGADRIVE_ROM_LOAD( ottifa, "ottifants, the (europe).bin",                                                               0x000000, 0x100000,  CRC(41ac8003) SHA1(4bb7cfc41ea7eaaed79ec402ec0d551c1e5c5bb6) )
MEGADRIVE_ROM_LOAD( ottifag, "ottifants, the (germany) (beta).bin",                                                      0x000000, 0x100000,  CRC(c6e3dd23) SHA1(76d2606d12a94b10208324ad1ff3e295ef89f5f4) )
MEGADRIVE_ROM_LOAD( ootwb, "out of this world (beta).bin",                                                               0x000000, 0x100000,  CRC(3aad905a) SHA1(9698cc5acc0031e26d71bb074742295142dafdb9) )
MEGADRIVE_ROM_LOAD( ootw, "out of this world (usa).bin",                                                                 0x000000, 0x100000,  CRC(2da36e01) SHA1(f00f96fb1b346d5b13c6bc8bc586477fed743800) )
MEGADRIVE_ROM_LOAD( outlandr, "outlander (europe).bin",                                                                  0x000000, 0x100000,  CRC(fe6f2350) SHA1(48b764d85c435ddebba39dc160fd70509b76d94e) )
MEGADRIVE_ROM_LOAD( outlandru, "outlander (usa).bin",                                                                    0x000000, 0x100000,  CRC(c5ba7bbf) SHA1(3b59fb2bfd94de10fdeec1222f27284bb81217c3) )
MEGADRIVE_ROM_LOAD( outrunj, "outrun (japan).bin",                                                                       0x000000, 0x100000,  CRC(ee7d9f4a) SHA1(a94c96ffb53080b2c098be86e715f1dc727be07d) )
MEGADRIVE_ROM_LOAD( outrun, "outrun (usa, europe).bin",                                                                  0x000000, 0x100000,  CRC(fdd9a8d2) SHA1(95d2055ffd679ab19f0d4ca0af62a0d565bc258e) )
MEGADRIVE_ROM_LOAD( o2019, "outrun 2019 (europe).bin",                                                                   0x000000, 0x100000,  CRC(5cb3536a) SHA1(f1a26bd827e9a674cf1dde74dc2c36f61c827c76) )
MEGADRIVE_ROM_LOAD( o2019j, "outrun 2019 (japan).bin",                                                                   0x000000, 0x100000,  CRC(0eac7440) SHA1(3b26b50f4194408cccd3fb9484ac83b97e94e67f) )
MEGADRIVE_ROM_LOAD( o2019b, "outrun 2019 (usa) (beta).bin",                                                              0x000000, 0x100000,  CRC(d2ecddfa) SHA1(6aca01bd23b99ffbcbd135f7c1dcaa42b8f89e61) )
MEGADRIVE_ROM_LOAD( o2019u, "outrun 2019 (usa).bin",                                                                     0x000000, 0x100000,  CRC(e32e17e2) SHA1(d4a9f1992d69d849f7d4d770a1f4f665a5352c78) )
MEGADRIVE_ROM_LOAD( outrunrj, "outrunners (japan).bin",                                                                  0x000000, 0x200000,  CRC(e164a09f) SHA1(4225162ed3cf51e16bfe800ae74a9b026f5dee56) )
MEGADRIVE_ROM_LOAD( outrunr, "outrunners (usa).bin",                                                                     0x000000, 0x200000,  CRC(ede636b9) SHA1(5d00ad7d9dccd9067c4bd716c2ab9c0a18930ae2) )
MEGADRIVE_ROM_LOAD( pacattck, "pac-attack (usa).bin",                                                                    0x000000, 0x40000,   CRC(5df382f7) SHA1(55e9122e72eeb3439c62cc0de810ea5df74de893) )
MEGADRIVE_ROM_LOAD( pacman, "pac-man 2 - the new adventures (usa).bin",                                                  0x000000, 0x200000,  CRC(fe7a7ed1) SHA1(9ed27068b00345d04d9dd1052ba0606c172e0090) )
MEGADRIVE_ROM_LOAD( pacmania, "pac-mania (usa, europe).bin",                                                             0x000000, 0x40000,   CRC(74bba09b) SHA1(e21546f653607d95f2747305703ddd6bf118f90a) )
MEGADRIVE_ROM_LOAD( pachinko, "pachinko kuunyan (japan).bin",                                                            0x000000, 0x100000,  CRC(9d137e7f) SHA1(84c3395c6dc123737d9da94e0392f3f30be59182) )
MEGADRIVE_ROM_LOAD( pacific, "pacific theater of operations (usa).bin",                                                  0x000000, 0x100000,  CRC(d9d4c6e2) SHA1(41d1d1956104133388b4ea69792fccca8013524a) )
MEGADRIVE_ROM_LOAD( paddle, "paddle fighter (japan) (seganet).bin",                                                      0x000000, 0x40000,   CRC(3d8147e6) SHA1(a768f64f394d4185f99da300517521091b6f0de5) )
MEGADRIVE_ROM_LOAD( paddle1, "paddle fighter (japan) (game no kandume megacd rip).bin",                                  0x000000, 0x40000,   CRC(ebef05a4) SHA1(0516b59e35b3c36651852fba231e351cfc239b22) )
MEGADRIVE_ROM_LOAD( pagemast, "pagemaster, the (europe).bin",                                                            0x000000, 0x200000,  CRC(79a180e2) SHA1(c4cf7695127b7ec5e003b5e7e15fdbfb172d1fcb) )
MEGADRIVE_ROM_LOAD( pagemastub, "pagemaster, the (usa) (beta).bin",                                                      0x000000, 0x200000,  CRC(29895e3d) SHA1(8a81ea7fca88fffff3fc1e932019e36e71be4978) )
MEGADRIVE_ROM_LOAD( pagemastu, "pagemaster, the (usa).bin",                                                              0x000000, 0x200000,  CRC(75a96d4e) SHA1(46d0495638e618bb9ac7fb07f46f8f0d61a8cedc) )
MEGADRIVE_ROM_LOAD( panora, "panorama cotton (japan).bin",                                                               0x000000, 0x280000,  CRC(9e57d92e) SHA1(63d9b71cfe0b7aaa6913ae06c1f2547d6d2aae5c) )
MEGADRIVE_ROM_LOAD( paperboyj, "paperboy (japan).bin",                                                                   0x000000, 0x80000,   CRC(e14250ae) SHA1(3384e238edaf8b2f6950cd1660864599e476d29f) )
MEGADRIVE_ROM_LOAD( paperboy, "paperboy (usa, europe).bin",                                                              0x000000, 0x80000,   CRC(0a44819b) SHA1(10faaf47e585423eec45285be4f5ce1015ae3386) )
MEGADRIVE_ROM_LOAD( paperby2, "paperboy 2 (usa, europe).bin",                                                            0x000000, 0x100000,  CRC(1de28bb1) SHA1(6421c26000bec21d0c808252c5d9ce75b75cb8a4) )
MEGADRIVE_ROM_LOAD( partyq, "party quiz mega q (japan).bin",                                                             0x000000, 0x100000,  CRC(9d4b447a) SHA1(3f82b2f028345b6fc84801fa38d70475645d6aa2) )
MEGADRIVE_ROM_LOAD( patriley, "pat riley basketball (usa).bin",                                                          0x000000, 0x80000,   CRC(3d9318a7) SHA1(0655ac1043c27c68d40ae089f1af2826613e5f80) )
MEGADRIVE_ROM_LOAD( pebble, "pebble beach golf links (europe).bin",                                                      0x000000, 0x200000,  CRC(6cfc7297) SHA1(53e8e8f11fb9950732409b770b7d9c8dabe85a19) )
MEGADRIVE_ROM_LOAD( pebbleu, "pebble beach golf links (usa).bin",                                                        0x000000, 0x200000,  CRC(95823c43) SHA1(c9971710651056b8211b9bbd68eb5d1569e39e1f) )
MEGADRIVE_ROM_LOAD( peleii, "pele ii - world tournament soccer (usa, europe).bin",                                       0x000000, 0x200000,  CRC(05a486e9) SHA1(96498065545ed122aa29471d762a2ae1362b2dea) )
MEGADRIVE_ROM_LOAD( pele, "pele! (usa, europe).bin",                                                                     0x000000, 0x100000,  CRC(5a8abe51) SHA1(c0a91e2d10b98174faa647fb732c335d7438abf7) )
MEGADRIVE_ROM_LOAD( pepenga, "pepenga pengo (japan).bin",                                                                0x000000, 0x100000,  CRC(d1e2324b) SHA1(c88c30d9e1fb6fb3a8aadde047158a3683bb6b1a) )
MEGADRIVE_ROM_LOAD( psampras, "mdst6636.bin",                                                                            0x000000, 0x100000,  CRC(94e505b2) SHA1(4c830ace4590294bb374b4cab71ebebf44d9a07a) )
MEGADRIVE_ROM_LOAD( psampras1, "mdstee_13.bin",                                                                          0x000000, 0x100000,  CRC(aa8b19bc) SHA1(3f73dcabcdf0d781834fcc673533301b82f0e91b) )
MEGADRIVE_ROM_LOAD( psampras2, "pete sampras tennis (usa, europe) (j-cart).bin",                                         0x000000, 0x100000,  CRC(9ef5bbd1) SHA1(222a66cdb8865a7f89e5a72418413888bb400176) )
MEGADRIVE_ROM_LOAD( pgaeuro, "pga european tour (usa, europe).bin",                                                      0x000000, 0x100000,  CRC(8ca45acd) SHA1(640615be6891a8457d94bb81b0e8e1fa7c5119a8) )
MEGADRIVE_ROM_LOAD( pga96, "pga tour 96 (usa, europe).bin",                                                              0x000000, 0x200000,  CRC(9698bbde) SHA1(abc2a8d773724cd8fb1aeae483f5ca72f47e77fa) )
MEGADRIVE_ROM_LOAD( pgaa, "pga tour golf (usa, europe) (v1.1).bin",                                                      0x000000, 0x80000,   CRC(0489ff8e) SHA1(73935bfbdf63d3400284a16e464286b7630964aa) )
MEGADRIVE_ROM_LOAD( pga, "pga tour golf (usa, europe) (v1.2).bin",                                                       0x000000, 0x80000,   CRC(c1f0b4e1) SHA1(1b173a1b674a6d5bdcd539c4fe8fe984586c3a8a) )
MEGADRIVE_ROM_LOAD( pga2j, "pga tour golf ii (japan).bin",                                                               0x000000, 0x100000,  CRC(c05b7a4a) SHA1(a5896f2f019530929194a6d80828d18b859b9174) )
MEGADRIVE_ROM_LOAD( pga2, "pga tour golf ii (usa, europe) (v1.1).bin",                                                   0x000000, 0x100000,  CRC(e82b8606) SHA1(12d5236a4ff23c5b1e4f452b3abd3d48e6e55314) )
MEGADRIVE_ROM_LOAD( pga2a, "pga tour golf ii (usa, europe).bin",                                                         0x000000, 0x100000,  CRC(16b2d816) SHA1(cab753b958b336dab731407bd46429de16c6919f) )
MEGADRIVE_ROM_LOAD( pga3, "pga tour golf iii (usa, europe).bin",                                                         0x000000, 0x200000,  CRC(aeb3f65f) SHA1(702707efcbfe229f6e190f2b6c71b6f53ae9ec36) )
MEGADRIVE_ROM_LOAD( pstar4j, "phantasy star - sennenki no owari ni (japan).bin",                                         0x000000, 0x300000,  CRC(f0bfad42) SHA1(9d330d5e395b7caae11fae92f71d259b8391904b) )
MEGADRIVE_ROM_LOAD( pstar2b, "phantasy star ii (brazil).bin",                                                            0x000000, 0xc0000,   CRC(e6688b66) SHA1(ab62fac112cb4dd1f19cb973bd7952b161f6d100) )
MEGADRIVE_ROM_LOAD( pstar2, "phantasy star ii (usa, europe) (rev a).bin",                                                0x000000, 0xc0000,   CRC(904fa047) SHA1(0711080e968490a6b8c5fafbb9db3e62ba597231) )
MEGADRIVE_ROM_LOAD( pstar2a, "phantasy star ii (usa, europe).bin",                                                       0x000000, 0xc0000,   CRC(0d07d0ef) SHA1(fcd032ded2235171f51db316ad1b7688fbbdafe4) )
MEGADRIVE_ROM_LOAD( ps2aa, "phantasy star ii - amia's adventure (japan) (seganet).bin",                                  0x000000, 0x40000,   CRC(a3a724aa) SHA1(08c6beab6f02ad533fb814a8a877849da67fc1a4) )
MEGADRIVE_ROM_LOAD( ps2ab, "phantasy star ii - anne's adventure (japan) (seganet).bin",                                  0x000000, 0x40000,   CRC(fafa5b6f) SHA1(cf93d2d5eafc72b9de7aaa6d35b81a813862d9b7) )
MEGADRIVE_ROM_LOAD( ps2ac, "phantasy star ii - huey's adventure (japan) (seganet).bin",                                  0x000000, 0x40000,   CRC(1a076f83) SHA1(abc5a7ac58aefc055ed8ca5fd0f7c802ab3e1ad5) )
MEGADRIVE_ROM_LOAD( pstar2j, "phantasy star ii - kaerazaru toki no owari ni (japan).bin",                                0x000000, 0xc0000,   CRC(bec8eb5a) SHA1(fc186c681e110723b4be5590f242c73d5004c8a7) )
MEGADRIVE_ROM_LOAD( ps2ad, "phantasy star ii - kinds's adventure (japan) (seganet).bin",                                 0x000000, 0x40000,   CRC(c334f308) SHA1(f2bd41741add13e8551250cf6afeb746e8cea316) )
MEGADRIVE_ROM_LOAD( ps2ae, "phantasy star ii - nei's adventure (japan) (seganet).bin",                                   0x000000, 0x40000,   CRC(3d9ad465) SHA1(bc1b2955d1ead44d744dacdc072481dff42e4d81) )
MEGADRIVE_ROM_LOAD( ps2af, "phantasy star ii - rudger's adventure (japan) (seganet).bin",                                0x000000, 0x40000,   CRC(6b5916d2) SHA1(dda0783cd5c6340c8dde665be22366108fadf50b) )
MEGADRIVE_ROM_LOAD( ps2ag, "phantasy star ii - shilka's adventure (japan) (seganet).bin",                                0x000000, 0x40000,   CRC(1f83beb2) SHA1(114f37a287b51664e52ba483f07911a660f9e7df) )
MEGADRIVE_ROM_LOAD( ps2ah, "phantasy star ii - yushis's adventure (japan) (seganet).bin",                                0x000000, 0x40000,   CRC(d40c76d6) SHA1(85679e11000387b6ef002c53bb03593f1c3f1c11) )
MEGADRIVE_ROM_LOAD( pstar3, "phantasy star iii - generations of doom (brazil).bin",                                      0x000000, 0xc0000,   CRC(2e9b4c23) SHA1(59ccfc6b85b95666d0e2c85e3e08c847c4a7ad34) )
MEGADRIVE_ROM_LOAD( pstar3a, "phantasy star iii - generations of doom (usa, europe, korea).bin",                         0x000000, 0xc0000,   CRC(c6b42b0f) SHA1(59d4914e652672fd1e453c76b8250d17e8ca154e) )
MEGADRIVE_ROM_LOAD( pstar3b, "phantasy star iii - toki no keishousha (japan).bin",                                       0x000000, 0xc0000,   CRC(6c48c06f) SHA1(68b0f8e73dea5dca1b6ac8c0e12bc1d9761edf32) )
MEGADRIVE_ROM_LOAD( pstar4a, "phantasy star iv (prototype - aug 15, 1994).bin",                                          0x000000, 0x300000,  CRC(60137f25) SHA1(81fe74c288bcf97b70758442c520ec47308cfcee) )
MEGADRIVE_ROM_LOAD( pstar4b, "phantasy star iv (prototype - jun 08, 1994).bin",                                          0x000000, 0x300000,  CRC(dc2e1c0a) SHA1(d72b9d68cfb54b4221f3a2416eac30a94accd427) )
MEGADRIVE_ROM_LOAD( pstar4c, "phantasy star iv (prototype - may 30, 1994).bin",                                          0x000000, 0x300000,  CRC(b32b17e1) SHA1(803877f317eeaaec57af07451a5ccf9309db513c) )
MEGADRIVE_ROM_LOAD( pstar4d, "phantasy star iv (prototype - nov 07, 1994).bin",                                          0x000000, 0x300000,  CRC(bda29cdf) SHA1(e0c3de9991a03fd48dc85caae6415aaac016ab4a) )
MEGADRIVE_ROM_LOAD( pstar4e, "phantasy star iv (prototype - oct 27, 1994).bin",                                          0x000000, 0x300000,  CRC(12a2590a) SHA1(6720d3afcd28cad06ac15749843d2a6995f403dc) )
MEGADRIVE_ROM_LOAD( pstar4, "phantasy star iv (usa).bin",                                                                0x000000, 0x300000,  CRC(fe236442) SHA1(bc7ff6d6a8408f38562bc610f24645cad6c42629) )
MEGADRIVE_ROM_LOAD( phantom, "phantom 2040 (europe).bin",                                                                0x000000, 0x200000,  CRC(b024882e) SHA1(6d588fac0fed0e2303082e29cde12d94719cca32) )
MEGADRIVE_ROM_LOAD( phantomu, "phantom 2040 (usa).bin",                                                                  0x000000, 0x200000,  CRC(fb36e1f3) SHA1(260b9f8fbd681bb1fc445538726836f0d423b4cc) )
MEGADRIVE_ROM_LOAD( phelios, "phelios (europe).bin",                                                                     0x000000, 0x80000,   CRC(13abc2b2) SHA1(5b95529dd491266648134f1808f4da55a2ab9296) )
MEGADRIVE_ROM_LOAD( pheliosj, "phelios (japan).bin",                                                                     0x000000, 0x80000,   CRC(94596174) SHA1(9834c9963debebb6ee80ffac4191ff260b85e511) )
MEGADRIVE_ROM_LOAD( pheliosu, "phelios (usa).bin",                                                                       0x000000, 0x80000,   CRC(11c79320) SHA1(c2e34b7eb411ac78bb480e30434808b2ae4989ea) )
MEGADRIVE_ROM_LOAD( pinkb, "pink goes to hollywood (usa) (beta).bin",                                                    0x000000, 0x100000,  CRC(56087cff) SHA1(5f2631b1850875129efd98d501f6419eb12b9817) )
MEGADRIVE_ROM_LOAD( pink, "pink goes to hollywood (usa).bin",                                                            0x000000, 0x100000,  CRC(b5804771) SHA1(fc63d2a2a723f60f2141bc50bd94acef74ba9ab3) )
MEGADRIVE_ROM_LOAD( pinocc, "pinocchio (europe).bin",                                                                    0x000000, 0x300000,  CRC(28014bdc) SHA1(7b6c9fdd201341f5319be85b5042870a84e82ae0) )
MEGADRIVE_ROM_LOAD( pinoc1, "pinocchio (usa).bin",                                                                       0x000000, 0x300000,  CRC(cd4128d8) SHA1(ad6098fe3642489d0281b1f168319a753e8bd02a) )
MEGADRIVE_ROM_LOAD( pirate, "pirates of dark water, the (usa) (january 1994).bin",                                       0x000000, 0x200000,  CRC(0c45b9f7) SHA1(a95eabc00ed9f4867f394c1e5e03afad0381843b) )
MEGADRIVE_ROM_LOAD( pirate1, "pirates of dark water, the (usa, europe) (may 1994).bin",                                  0x000000, 0x200000,  CRC(0a62de34) SHA1(55496714f32eb7f57335201f90b3f437a1500c49) )
MEGADRIVE_ROM_LOAD( pgoldb, "pirates! gold (usa) (beta).bin",                                                            0x000000, 0x100000,  CRC(0a525641) SHA1(6449aea7af69e6b91c91b654c9ea952a72f0dd7c) )
MEGADRIVE_ROM_LOAD( pgold, "pirates! gold (usa).bin",                                                                    0x000000, 0x100000,  CRC(ed50e75c) SHA1(ea597dbefc8f804524606af3c1c4fe6ba55e86e9) )
MEGADRIVE_ROM_LOAD( pitfight, "mpr-14320a.bin",                                                                          0x000000, 0x100000,  CRC(d48a8b02) SHA1(8cd0ffaf3c29b3aa1f9068cd89cb64f492066d74) )
MEGADRIVE_ROM_LOAD( pitfight1, "pit-fighter (world).bin",                                                                0x000000, 0x100000,  CRC(1e0e0831) SHA1(19f81e5d0c4564e06fdccd7a7e67d98fa04360f4) )
MEGADRIVE_ROM_LOAD( pitfall, "pitfall - the mayan adventure (europe).bin",                                               0x000000, 0x200000,  CRC(c9198e19) SHA1(b7c5f6e5f422c7bb5ec3177328d963c9beace24f) )
MEGADRIVE_ROM_LOAD( pitfallu, "pitfall - the mayan adventure (usa).bin",                                                 0x000000, 0x200000,  CRC(f917e34f) SHA1(f2067e7d974f03bf922286241119dac7c0ecabff) )
MEGADRIVE_ROM_LOAD( pocaho, "pocahontas (europe).bin",                                                                   0x000000, 0x400000,  CRC(165e7987) SHA1(5ffcdc3e01151837e707ae225a3a845a8b6d3394) )
MEGADRIVE_ROM_LOAD( pocahou, "pocahontas (usa).bin",                                                                     0x000000, 0x400000,  CRC(6ddd1c6d) SHA1(b57ae70f1d735c8052991053297a0c878f8a9417) )
MEGADRIVE_ROM_LOAD( pokemon, "pocket monsters (unlicensed).bin",                                                         0x000000, 0x200000,  CRC(f68f6367) SHA1(d10de935e099c520384c986b1f00fd5e72d64e03) )
MEGADRIVE_ROM_LOAD( pokemona, "pocket monsters (unlicensed) [alt].bin",                                                  0x000000, 0x200000,  CRC(fb176667) SHA1(3d667aea1b6fa980dddcc10e65845a6831491792) )
MEGADRIVE_ROM_LOAD( pokemon2, "pocket monsters 2 (unlicensed).bin",                                                      0x000000, 0x200000,  CRC(30f7031f) SHA1(dae100dfaee1b5b7816731cb2f43bcda3da273b7) )
MEGADRIVE_ROM_LOAD( pokecd, "pokemon crazy drummer (unlicensed).bin",                                                    0x000000, 0x200000,  CRC(8838a25d) SHA1(780a81fe6dd2fb9575ccdc506e7dbee13213d01d) )
MEGADRIVE_ROM_LOAD( populous, "populous (europe).bin",                                                                   0x000000, 0x80000,   CRC(83d56f64) SHA1(fe388caefbad7c08b699ed2d0e8a62ff1d697a16) )
MEGADRIVE_ROM_LOAD( populousj, "populous (japan).bin",                                                                   0x000000, 0x80000,   CRC(97c26818) SHA1(9abd1806c6e921a6df0f71361aee8024d5c3f049) )
MEGADRIVE_ROM_LOAD( populousu, "populous (usa).bin",                                                                     0x000000, 0x80000,   CRC(bd74b31e) SHA1(89907c4ba4fd9db4e8ef2271c0253bb0e4b6d52d) )
MEGADRIVE_ROM_LOAD( powerath, "power athlete (japan, korea).bin",                                                        0x000000, 0x100000,  CRC(b41b77cf) SHA1(d371e339c5d85b69c34007dc514c1adb524dac2a) )
MEGADRIVE_ROM_LOAD( powerd, "power drive (europe) (en,fr,de,es,pt).bin",                                                 0x000000, 0x100000,  CRC(8c00ad61) SHA1(6ffaaac637fb41a2f94912c382385a9cb17639a9) )
MEGADRIVE_ROM_LOAD( pmongerj, "power monger (japan, korea).bin",                                                         0x000000, 0x100000,  CRC(553289b3) SHA1(f0a48f25d87f7a6d17ff76f5e29ea7be2d430ce4) )
MEGADRIVE_ROM_LOAD( pmonger, "power monger (usa, europe).bin",                                                           0x000000, 0x100000,  CRC(fb599b86) SHA1(f088e8cdb90c037b4ce1c5ae600a75f4e4219c87) )
MEGADRIVE_ROM_LOAD( powerb, "powerball (usa).bin",                                                                       0x000000, 0x80000,   CRC(7adf232f) SHA1(217d64da0b09b88dfa3fd91b7fa081ec7a8666e0) )
MEGADRIVE_ROM_LOAD( predatr2, "predator 2 (usa, europe).bin",                                                            0x000000, 0x100000,  CRC(bdba113e) SHA1(0d482bae2922c81c8bc7500a62c396b038978114) )
MEGADRIVE_ROM_LOAD( premierm, "premier manager (europe).bin",                                                            0x000000, 0x100000,  CRC(303b889f) SHA1(2916e5ef628e077cde87be873e0ea2507ef5c844) )
MEGADRIVE_ROM_LOAD( premrm97, "premier manager 97 (europe).bin",                                                         0x000000, 0x100000,  CRC(fccbf69b) SHA1(e2df3b48170e1a7bde46af2adbf939803e267e13) )
MEGADRIVE_ROM_LOAD( pretty, "pretty girl mahjongg (ch).bin",                                                             0x000000, 0x100000,  CRC(69f24500) SHA1(0b4f63b8de2dcc4359a26c3ff21f910f206b6110) )
MEGADRIVE_ROM_LOAD( primal, "primal rage (usa, europe).bin",                                                             0x000000, 0x300000,  CRC(2884c6d1) SHA1(18fc82ae49948128f2347b8b6d7174c8a0c1bf5f) )
MEGADRIVE_ROM_LOAD( primetim, "prime time nfl starring deion sanders (usa).bin",                                         0x000000, 0x200000,  CRC(5aa53cbc) SHA1(7d7a5a920ac30831556b58caac66f4a8dde1632a) )
MEGADRIVE_ROM_LOAD( ppersiaa, "prince of persia (europe) (beta) (earlier).bin",                                          0x000000, 0x40000,   CRC(425e6a87) SHA1(224516e54a4bac00089e61c8e0a4794eac92d8df) )
MEGADRIVE_ROM_LOAD( ppersiab, "prince of persia (europe) (beta).bin",                                                    0x000000, 0x100000,  CRC(505314b6) SHA1(addd33ce2f9c022433be1c3ea803371e7f6b694b) )
MEGADRIVE_ROM_LOAD( ppersia, "prince of persia (europe).bin",                                                            0x000000, 0x100000,  CRC(61de6fe0) SHA1(6e645b791e6e2b84a206dca6cf47e8f955e60a72) )
MEGADRIVE_ROM_LOAD( ppersiau, "prince of persia (usa).bin",                                                              0x000000, 0x100000,  CRC(13c181a4) SHA1(30080c7a8617ba3aaf67587970f32cd846234611) )
MEGADRIVE_ROM_LOAD( ppersia2, "prince of persia 2 - the shadow and the flame (europe) (prototype).bin",                  0x000000, 0x200000,  CRC(3ab44d46) SHA1(ba63dc1e521bee68b4121b626061ebb203ac63c6) )
MEGADRIVE_ROM_LOAD( proquart, "pro quarterback (usa).bin",                                                               0x000000, 0x100000,  CRC(cc8b2b69) SHA1(a878d6149a3d5c4ccca7741af45659f753b32ed7) )
MEGADRIVE_ROM_LOAD( proyakyu, "pro yakyuu super league '91 (japan).bin",                                                 0x000000, 0x80000,   CRC(a948ab7e) SHA1(9d446471b8fe37b08dc7c4a99de22de9d56a1d16) )
MEGADRIVE_ROM_LOAD( probot, "probotector (europe).bin",                                                                  0x000000, 0x200000,  CRC(bc597d48) SHA1(69d905751ffa6a921586767263e45ddeb73333c2) )
MEGADRIVE_ROM_LOAD( psyobl, "psy-o-blade (japan).bin",                                                                   0x000000, 0xa0000,   CRC(8ba7e6c5) SHA1(274d98e0c04caed378f1dd489dc3741b9ad13a88) )
MEGADRIVE_ROM_LOAD( psycho, "psycho pinball (europe) (en,fr,de,es,it) (october 1994).bin",                               0x000000, 0x180000,  CRC(d704784b) SHA1(699db175ab8a6abdd941b8a3d81cf203e01053f5) )
MEGADRIVE_ROM_LOAD( psycho1, "psycho pinball (europe) (en,fr,de,es,it) (september 1994).bin",                            0x000000, 0x180000,  CRC(11e9c3f2) SHA1(2ea7befb7e52334111385667030ceb647157a397) )
MEGADRIVE_ROM_LOAD( puggsyb, "puggsy (beta).bin",                                                                        0x000000, 0x100000,  CRC(37fa4877) SHA1(d1b4e6528e90319d5bab47de98dd691070998df4) )
MEGADRIVE_ROM_LOAD( puggsy, "puggsy (europe).bin",                                                                       0x000000, 0x100000,  CRC(5d5c9ade) SHA1(89a347f35c4d0d364c284f2f32f708d0378b8735) )
MEGADRIVE_ROM_LOAD( puggsyu, "puggsy (usa).bin",                                                                         0x000000, 0x100000,  CRC(70132168) SHA1(5f4d1acd1b8580e83952e1083cfa881b0ec5b9fd) )
MEGADRIVE_ROM_LOAD( pulseman, "pulseman (japan).bin",                                                                    0x000000, 0x200000,  CRC(138a104e) SHA1(5fd76a5f80e4684b5f9d445ddb893679985684e6) )
MEGADRIVE_ROM_LOAD( punisher, "punisher, the (europe).bin",                                                              0x000000, 0x200000,  CRC(729edd17) SHA1(d11ec72726498e4608eefc1ef2b8a3cadb025394) )
MEGADRIVE_ROM_LOAD( punisheru, "punisher, the (usa).bin",                                                                0x000000, 0x200000,  CRC(695cd8b8) SHA1(4ef2675e728903925a7b865daa75ed66bbe24829) )
MEGADRIVE_ROM_LOAD( putter, "putter golf (japan) (seganet).bin",                                                         0x000000, 0x40000,   CRC(20f168a6) SHA1(345af8995cf9d88f7c4d8cfd81c8ccd45e6478fc) )
MEGADRIVE_ROM_LOAD( putter1, "putter golf (japan) (game no kandume megacd rip).bin",                                     0x000000, 0x40000,   CRC(a6557faa) SHA1(9c4bc513a00f5bb5f7c72b41999d9c8c12345acf) )
MEGADRIVE_ROM_LOAD( puyopuyo, "puyo puyo (japan).bin",                                                                   0x000000, 0x80000,   CRC(7f26614e) SHA1(02656b5707cf9452d4cf48378ffd3a95dc84e9c5) )
MEGADRIVE_ROM_LOAD( puyo2, "puyo puyo 2 (japan) (v1.1).bin",                                                             0x000000, 0x200000,  CRC(25b7b2aa) SHA1(d9c0edaadf66f215565a9dd4178313581fec811c) )
MEGADRIVE_ROM_LOAD( puyo2a, "puyo puyo 2 (japan).bin",                                                                   0x000000, 0x200000,  CRC(51ad7797) SHA1(b9fd6e14f446a16927b54de0234bcd981a86488f) )
MEGADRIVE_ROM_LOAD( ichir, "puzzle & action - ichidanto-r (japan).bin",                                                  0x000000, 0x200000,  CRC(7bdec762) SHA1(2e43ea1870dd3352e3c153373507554d97d51edf) )
MEGADRIVE_ROM_LOAD( tantr, "puzzle & action - tanto-r (japan).bin",                                                      0x000000, 0x200000,  CRC(d2d2d437) SHA1(2bf6965ee883a70b4f0842e9efa17c5e20b5cb47) )
MEGADRIVE_ROM_LOAD( pyramid, "pyramid magic (japan) (seganet).bin",                                                      0x000000, 0x40000,   CRC(306d839e) SHA1(e32382b419d63186dc4f5521045cae2145cb7975) )
MEGADRIVE_ROM_LOAD( pyramida, "pyramid magic (japan) (game no kandume megacd rip).bin",                                  0x000000, 0x40000,   CRC(b20272ea) SHA1(8fa4936dabbb662f006367e9fd31595e9bfc7907) )
MEGADRIVE_ROM_LOAD( pyramid2, "pyramid magic ii (japan) (seganet).bin",                                                  0x000000, 0x40000,   CRC(c9ddac72) SHA1(b74feb0b7f5f6eaac72f0895b51a8f8421dcb768) )
MEGADRIVE_ROM_LOAD( pyramid3, "pyramid magic iii (japan) (seganet).bin",                                                 0x000000, 0x40000,   CRC(8329820a) SHA1(c386a1fc6cf5d0d6f0d2eba0b2ddda649143da67) )
MEGADRIVE_ROM_LOAD( pyramids, "pyramid magic special (japan) (seganet).bin",                                             0x000000, 0x40000,   CRC(153a3afa) SHA1(010a336bedc1022846f3498693cd6b5218a05b4c) )
MEGADRIVE_ROM_LOAD( quacksht, "quackshot starring donald duck / quackshot - guruzia ou no hihou (world).bin",            0x000000, 0x80000,   CRC(5dd28dd7) SHA1(ca02845fa49cd46ccb0a4023b91b10695573668e) )
// dump without mirrored data, need proper emulation

MEGADRIVE_ROM_LOAD( quacksht1, "quackshot starring donald duck / quackshot - guruzia ou no hihou (world) (alt).bin",     0x000000, 0x80000,   CRC(88c8dd94) SHA1(3acacac6c20d9b5f9ddeacb82b82b3039c4e2485) )
MEGADRIVE_ROM_LOAD( quadchal, "quad challenge (usa).bin",                                                                0x000000, 0x80000,   CRC(74736a80) SHA1(c7db4cd8b5dc8c0927824e9ddf1257591305d99a) )
MEGADRIVE_ROM_LOAD( rbi93, "r.b.i. baseball '93 (usa).bin",                                                              0x000000, 0x100000,  CRC(beafce84) SHA1(4db6cca411ba3d32ad43d572389612935a42e3b3) )
MEGADRIVE_ROM_LOAD( rbi94, "r.b.i. baseball '94 (usa, europe).bin",                                                      0x000000, 0x200000,  CRC(4eb4d5e4) SHA1(3e94f52d311fae140da870f55e3cb103d7923d44) )
MEGADRIVE_ROM_LOAD( rbi3, "r.b.i. baseball 3 (usa).bin",                                                                 0x000000, 0x80000,   CRC(4840348c) SHA1(fe6d17d8670e046a90d126ee6a11149735a1aeb8) )
MEGADRIVE_ROM_LOAD( rbi4j, "r.b.i. baseball 4 (japan).bin",                                                              0x000000, 0x100000,  CRC(8f4e1005) SHA1(1cde0942bbbc9e88973ee56da06ccaabdcd34805) )
MEGADRIVE_ROM_LOAD( rbi4, "r.b.i. baseball 4 (usa).bin",                                                                 0x000000, 0x100000,  CRC(fecf9b94) SHA1(37aaac7eda046968f4bff766566ca15b473f9060) )
MEGADRIVE_ROM_LOAD( rbi4b, "r.b.i. baseball 4 (beta).bin",                                                               0x000000, 0x100000,  CRC(f7420278) SHA1(d999e11813dca4fe0ac05f371ff7900806238a69) )
MEGADRIVE_ROM_LOAD( racedriv, "race drivin' (usa).bin",                                                                  0x000000, 0x80000,   CRC(d737cf3d) SHA1(7961b37d364c8296d96f08a79577985113f72ba5) )
MEGADRIVE_ROM_LOAD( radica, "radical rex (europe).bin",                                                                  0x000000, 0x100000,  CRC(d02d3282) SHA1(c8dc73b7cabf43813f9cf09370a4437af4f1573f) )
MEGADRIVE_ROM_LOAD( radica1, "radical rex (usa).bin",                                                                    0x000000, 0x100000,  CRC(2e6eec7e) SHA1(51019abbb8a4fc246eacd5f553628b74394fab66) )
MEGADRIVE_ROM_LOAD( ragnacen, "ragnacenty (korea).bin",                                                                  0x000000, 0x200000,  CRC(77b5b10b) SHA1(40265aae1b43f004434194038d32ca6b4841707d) )
MEGADRIVE_ROM_LOAD( raiden, "raiden densetsu / raiden trad (japan, usa).bin",                                            0x000000, 0x100000,  CRC(f839a811) SHA1(9d4011d650179c62acdba0ed58143aa77b97d0e9) )
MEGADRIVE_ROM_LOAD( rainbow, "rainbow islands extra (japan).bin",                                                        0x000000, 0x80000,   CRC(c74dcb35) SHA1(e35fcc88e37d1691cd2572967e1ae193fcd303eb) )
MEGADRIVE_ROM_LOAD( rambo3, "rambo iii (world) (v1.1).bin",                                                              0x000000, 0x40000,   CRC(4d47a647) SHA1(e337e618653830f84c77789e81724c5fc69888be) )
MEGADRIVE_ROM_LOAD( rambo3a, "rambo iii (world).bin",                                                                    0x000000, 0x40000,   CRC(2232f03d) SHA1(ece53156a43fe14597a44f36a4e65e13cfc6d9d5) )
MEGADRIVE_ROM_LOAD( rampartj, "rampart (japan, korea).bin",                                                              0x000000, 0x80000,   CRC(16ead64c) SHA1(15f9284da9b27b2803bc53790b51b9dac5185a09) )
MEGADRIVE_ROM_LOAD( rampart, "rampart (usa).bin",                                                                        0x000000, 0x80000,   CRC(9c4dd057) SHA1(9dd4ee99e2ccf8759446d355584a9cbf685a3d8b) )
MEGADRIVE_ROM_LOAD( rangerx, "ranger-x (europe).bin",                                                                    0x000000, 0x100000,  CRC(b8c04804) SHA1(53908ee1c47f0d9d49e81fc295379552e2198948) )
MEGADRIVE_ROM_LOAD( rangerxu, "ranger-x (usa).bin",                                                                      0x000000, 0x100000,  CRC(55915915) SHA1(2611c818ecb53edc7d561fce1a349acad6abff4a) )
MEGADRIVE_ROM_LOAD( ransei, "ransei no hasha (japan).bin",                                                               0x000000, 0x100000,  CRC(a9a0083d) SHA1(7c431e26226b99d09d61c4dc64131b2d81ae870e) )
MEGADRIVE_ROM_LOAD( rastans2j, "rastan saga ii (japan).bin",                                                             0x000000, 0x80000,   CRC(ebacfb5a) SHA1(e7bd8367ae962af48a2e937bf6ced253f428b03d) )
MEGADRIVE_ROM_LOAD( rastans2, "rastan saga ii (usa).bin",                                                                0x000000, 0x80000,   CRC(c7ee8965) SHA1(adec2cb70344ffc720ecdfeb68712bb3d1d0fd9c) )
MEGADRIVE_ROM_LOAD( redzone, "red zone (usa, europe).bin",                                                               0x000000, 0x200000,  CRC(56512ee5) SHA1(2e01a1ee8ff31b3eee0c422d30b529aedd7ffdc1) )
MEGADRIVE_ROM_LOAD( renstim, "ren & stimpy show presents stimpy's invention, the (europe).bin",                          0x000000, 0x100000,  CRC(c276c220) SHA1(34d520d8a835e386c26793f216b1dc8f78d8f67a) )
MEGADRIVE_ROM_LOAD( renstimu1, "ren & stimpy show presents stimpy's invention, the (usa) (beta).bin",                    0x000000, 0x100000,  CRC(fcb86336) SHA1(85e2e41d31bde9acc98070014640c00ba8dba6f8) )
MEGADRIVE_ROM_LOAD( renstimu, "ren & stimpy show presents stimpy's invention, the (usa).bin",                            0x000000, 0x100000,  CRC(d9503ba5) SHA1(29788359cc3311107e82f275868e908314d3d426) )
MEGADRIVE_ROM_LOAD( renthero, "rent a hero (japan).bin",                                                                 0x000000, 0x100000,  CRC(2e515f82) SHA1(a07e4d56c9842e1177df9016fb9478060dc4c1c6) )
MEGADRIVE_ROM_LOAD( resq, "resq (europe) (prototype).bin",                                                               0x000000, 0x100000,  CRC(558e35e0) SHA1(f572443bd1e0cf956c87e6cd925412cb0f35045f) )
MEGADRIVE_ROM_LOAD( revshin, "revenge of shinobi, the (usa, europe) (rev a).bin",                                        0x000000, 0x80000,   CRC(fe91ab7e) SHA1(76af6ca97df08f2370f3a7fc2311781a0f147f4b) )
MEGADRIVE_ROM_LOAD( revshin1, "revenge of shinobi, the (usa, europe) (rev b).bin",                                       0x000000, 0x80000,   CRC(4d35ebe4) SHA1(7ed6f142884a08bb3c07a2e6e35405440343502f) )
MEGADRIVE_ROM_LOAD( revshin2, "revenge of shinobi, the (usa, europe).bin",                                               0x000000, 0x80000,   CRC(05f27994) SHA1(a88546e159bc882bf05eba66c09a3b21dee4154e) )
MEGADRIVE_ROM_LOAD( revx, "revolution x (usa, europe).bin",                                                              0x000000, 0x400000,  CRC(5fb0c5d4) SHA1(e1c8011ff878c7d99236ddd4008dd284e1c28333) )
MEGADRIVE_ROM_LOAD( rsbtpa, "richard scarry's busytown (prototype - aug 09, 1994).bin",                                  0x000000, 0x1fecec,  CRC(c48cbc30) SHA1(10cdf9b4284586402afbcaf02f4875ec1e49cd5d) )
MEGADRIVE_ROM_LOAD( rsbtpb, "richard scarry's busytown (prototype - aug 15, 1994).bin",                                  0x000000, 0x200000,  CRC(650ed917) SHA1(e186b2c54514653374657b6033a05f5645c6aac2) )
MEGADRIVE_ROM_LOAD( rsbtpc, "richard scarry's busytown (prototype - aug 16, 1994 - b).bin",                              0x000000, 0x200000,  CRC(760cff62) SHA1(85e5c63e0b8a536a134e20036c978724158be80e) )
MEGADRIVE_ROM_LOAD( rsbtpd, "richard scarry's busytown (prototype - aug 17, 1994).bin",                                  0x000000, 0x1ffaa4,  CRC(3b9c3f4a) SHA1(6e70fafecdf133895715b52fd6614297e8b70763) )
MEGADRIVE_ROM_LOAD( rsbtpe, "richard scarry's busytown (prototype - aug 25, 1994).bin",                                  0x000000, 0x200000,  CRC(a0c013ba) SHA1(8051a0961ae0003786d6b33e07f442e3b72bf771) )
MEGADRIVE_ROM_LOAD( rsbtpf, "richard scarry's busytown (prototype - aug 26, 1994).bin",                                  0x000000, 0x200000,  CRC(1ad270f3) SHA1(375f358a24ae63be8b8bc1010f61b27d013f8212) )
MEGADRIVE_ROM_LOAD( rsbtpg, "richard scarry's busytown (prototype - jul 21, 1994).bin",                                  0x000000, 0x1f3008,  CRC(1e92d6ff) SHA1(cae77f948369d11f0152d68d75311b80c038f36a) )
MEGADRIVE_ROM_LOAD( rsbt, "richard scarry's busytown (usa).bin",                                                         0x000000, 0x200000,  CRC(7bb60c3f) SHA1(42e5ac265e3bf82c44dca70deb71b0aa08acf1a9) )
MEGADRIVE_ROM_LOAD( riddle, "riddle wired (japan) (seganet).bin",                                                        0x000000, 0x40000,   CRC(fae3d720) SHA1(662ffc946978030125dc1274aeec5196bc9a721b) )
MEGADRIVE_ROM_LOAD( ringspow, "rings of power (usa, europe).bin",                                                        0x000000, 0x100000,  CRC(41fcc497) SHA1(84851ce4527761b8d74ce581c19e15d0dd17f368) )
MEGADRIVE_ROM_LOAD( risk, "risk (usa).bin",                                                                              0x000000, 0x80000,   CRC(80416d0d) SHA1(6f4bf5e1dc0de79e42934e5867641e1baadba0d9) )
MEGADRIVE_ROM_LOAD( riskyw, "risky woods (usa, europe).bin",                                                             0x000000, 0x100000,  CRC(d975e93c) SHA1(5664febf187d26274996c888778b0ff5268474f2) )
MEGADRIVE_ROM_LOAD( ristarpa, "ristar (prototype - aug 12, 1994).bin",                                                   0x000000, 0x200000,  CRC(4371f046) SHA1(078846cd7a6f86c6fe71c95b1d13e89e66bd9b25) )
MEGADRIVE_ROM_LOAD( ristarpb, "ristar (prototype - aug 26, 1994).bin",                                                   0x000000, 0x200000,  CRC(d0c74fdd) SHA1(a54553ffa55fbdfc43cfb61af10ca0a79683ec75) )
MEGADRIVE_ROM_LOAD( ristarpc, "ristar (prototype - jul 01, 1994).bin",                                                   0x000000, 0x200000,  CRC(7c5c7a0a) SHA1(d887378bed61a5be60664d3fe6559f78cc95d119) )
MEGADRIVE_ROM_LOAD( ristarpd, "ristar (prototype - jul 18, 1994).bin",                                                   0x000000, 0x200000,  CRC(6becccc9) SHA1(85b82470e5395e96e01a7339c81b60832ea3ab1a) )
MEGADRIVE_ROM_LOAD( ristaro, "ristar (usa, europe) (august 1994).bin",                                                   0x000000, 0x200000,  CRC(9700139b) SHA1(cf0215feddd38f19cd2d27bfa96dd4d742ba8bf7) )
MEGADRIVE_ROM_LOAD( ristar, "ristar (usa, europe) (september 1994).bin",                                                 0x000000, 0x200000,  CRC(6511aa61) SHA1(1d15ff596dd4f3b2c1212a2e0c6e2b72f62c001e) )
MEGADRIVE_ROM_LOAD( ristarj, "ristar - the shooting star (japan, korea).bin",                                            0x000000, 0x200000,  CRC(ce464f0e) SHA1(a4dd8dc0b673afe7372ba9c05c8d6090302dcf4e) )
MEGADRIVE_ROM_LOAD( roadrash, "road rash (usa, europe).bin",                                                             0x000000, 0xc0000,   CRC(dea53d19) SHA1(8ff2b2a58b09c42eeb95f633bd8ea5227ac71358) )
// Road Rash seems to suffer the same dumping problems of F22 September 1991 revision, needs a proper dump

MEGADRIVE_ROM_LOAD( rrash3a, "road rash 3 (usa) (alpha).bin",                                                            0x000000, 0x200000,  CRC(b6626083) SHA1(2cbba62bc9ab1dc3b3ec18a5eb867643021ac552) )
MEGADRIVE_ROM_LOAD( rrash3, "road rash 3 (usa, europe).bin",                                                             0x000000, 0x200000,  CRC(15785956) SHA1(b2324833bf81223a9626a6060cafc203dd203468) )
MEGADRIVE_ROM_LOAD( rrash2, "road rash ii (japan).bin",                                                                  0x000000, 0x100000,  CRC(9a5723b6) SHA1(d89d7707cd4f30eef1fd2fd7e322d760ba8d6786) )
MEGADRIVE_ROM_LOAD( rrash2a, "road rash ii (usa, europe) (v1.2).bin",                                                    0x000000, 0x100000,  CRC(0876e992) SHA1(9afea43ed627671b50dd4a2abdd043b235414b91) )
MEGADRIVE_ROM_LOAD( rrash2b, "road rash ii (usa, europe).bin",                                                           0x000000, 0x100000,  CRC(7b29c209) SHA1(6fa6420a2abcf5c2c4620c19b1f2a831996af481) )
MEGADRIVE_ROM_LOAD( roadbj, "roadblasters (japan).bin",                                                                  0x000000, 0x80000,   CRC(a0015440) SHA1(7127e76ea33512d14cd78e6cac72ff8578454e1e) )
MEGADRIVE_ROM_LOAD( roadb, "roadblasters (usa).bin",                                                                     0x000000, 0x80000,   CRC(ec6cd5f0) SHA1(927cdb93e688152f2ff9a89a1b22cd6517ada652) )
MEGADRIVE_ROM_LOAD( robocop, "robocop 3 (usa, europe).bin",                                                              0x000000, 0x80000,   CRC(34fb7b27) SHA1(7860eb700d85801831ea14501b47bcaa1753c9fc) )
MEGADRIVE_ROM_LOAD( robotermb1, "robocop versus the terminator (beta 1).bin",                                            0x000000, 0x200000,  CRC(ecebff29) SHA1(4d35cdf21f50c1cf73bacc9b8747a9eac1141e46) )
MEGADRIVE_ROM_LOAD( robotermb, "robocop versus the terminator (europe) (beta).bin",                                      0x000000, 0x200000,  CRC(2487049f) SHA1(a1625783cf1b42d808ad4f56dde5ce1b1884218a) )
MEGADRIVE_ROM_LOAD( roboterm, "robocop versus the terminator (europe).bin",                                              0x000000, 0x200000,  CRC(85a93f8d) SHA1(21348f50ccb0031108ce3fb999026688e9bef4ed) )
MEGADRIVE_ROM_LOAD( robotermu, "robocop versus the terminator (usa).bin",                                                0x000000, 0x200000,  CRC(bbad77a4) SHA1(b67b708006145406f2a8dacb237517ededec359a) )
MEGADRIVE_ROM_LOAD( robotb, "robot battler (japan) (seganet).bin",                                                       0x000000, 0x40000,   CRC(fdf23eff) SHA1(1e5cbfa3dfbab399a84f5f249c7f5dec9ecd0026) )
MEGADRIVE_ROM_LOAD( robotw, "robot wreckage (usa) (prototype).bin",                                                      0x000000, 0x100000,  CRC(c67ddb14) SHA1(8cecca591f781a638b45f99f69bf2f18b7abb289) )
MEGADRIVE_ROM_LOAD( rocknr, "rock n' roll racing (europe).bin",                                                          0x000000, 0x100000,  CRC(bc5a0562) SHA1(c4d84425c007711f006a67d6577fd90f421605e5) )
MEGADRIVE_ROM_LOAD( rocknru, "rock n' roll racing (usa).bin",                                                            0x000000, 0x100000,  CRC(6abab577) SHA1(9793572b6d64c85c1add0721c3be388ac18777d6) )
MEGADRIVE_ROM_LOAD( rocket, "rocket knight adventures (europe).bin",                                                     0x000000, 0x100000,  CRC(8eedfd51) SHA1(cb741f868b0eba17819a27e37cda96f33674c342) )
MEGADRIVE_ROM_LOAD( rocketj, "rocket knight adventures (japan).bin",                                                     0x000000, 0x100000,  CRC(d1c8c1c5) SHA1(76d33c8549cd86cb1a4d17ac6eb8dd47c2f07d03) )
MEGADRIVE_ROM_LOAD( rocketu, "rocket knight adventures (usa).bin",                                                       0x000000, 0x100000,  CRC(a6efec47) SHA1(49634bb09c38fa03549577f977e6afb6cebaac48) )
MEGADRIVE_ROM_LOAD( rockman1, "rockman mega world (japan) (alt).bin",                                                    0x000000, 0x200000,  CRC(85c956ef) SHA1(a435ac53589a29dbb655662c942daab425d3f6bd) )
MEGADRIVE_ROM_LOAD( rockman, "rockman mega world (japan).bin",                                                           0x000000, 0x200000,  CRC(4d87235e) SHA1(4dfcf5c07106f6db4d8b7e73a053bb46a21686c3) )
MEGADRIVE_ROM_LOAD( rockmnx3, "rockman x3 (unlicensed).bin",                                                             0x000000, 0x200000,  CRC(3ee639f0) SHA1(37024d0088fab1d76c082014663c58921cdf33df) )
MEGADRIVE_ROM_LOAD( rogerc, "roger clements mvp baseball (usa).bin",                                                     0x000000, 0x100000,  CRC(83699e34) SHA1(10b4be6f7e8046ec527ea487ce5f2678990e92c6) )
MEGADRIVE_ROM_LOAD( rthun2, "rolling thunder 2 (europe).bin",                                                            0x000000, 0x100000,  CRC(c440f292) SHA1(e8ae09b2945eac555d6f0d39a1f768724b221357) )
MEGADRIVE_ROM_LOAD( rthun2j, "rolling thunder 2 (japan).bin",                                                            0x000000, 0x100000,  CRC(965b2628) SHA1(6e15ce792fbe01b918b197deea320f953aa989cc) )
MEGADRIVE_ROM_LOAD( rthun2u, "rolling thunder 2 (usa).bin",                                                              0x000000, 0x100000,  CRC(3ace429b) SHA1(1b451389910e6b353c713f56137863ce2aff109d) )
MEGADRIVE_ROM_LOAD( rthun3, "rolling thunder 3 (usa).bin",                                                               0x000000, 0x180000,  CRC(64fb13aa) SHA1(34c2b5df456a1f8ff35963ca737372e221e89ef6) )
MEGADRIVE_ROM_LOAD( rolo, "rolo to the rescue (usa, europe).bin",                                                        0x000000, 0x80000,   CRC(306861a2) SHA1(8f27907df777124311b7415cd52775641276cf0d) )
MEGADRIVE_ROM_LOAD( r3k2, "romance of the three kingdoms ii (usa).bin",                                                  0x000000, 0x100000,  CRC(3d842478) SHA1(89fc2203b0369565124184690a5a5037ac9c54d7) )
MEGADRIVE_ROM_LOAD( r3k3, "romance of the three kingdoms iii - dragon of destiny (usa).bin",                             0x000000, 0x140000,  CRC(7e41c8fe) SHA1(a47b0ac13ef8229fbfc89216c8ecb66d72f6f73a) )
MEGADRIVE_ROM_LOAD( ronald, "ronaldinho 98 (brazil) (pirate).bin",                                                       0x000000, 0x200000,  CRC(dd27c84e) SHA1(183ba7d8fe6bcddf0b7738c9ef2aa163725eb261) )
MEGADRIVE_ROM_LOAD( royalb, "royal blood (japan).bin",                                                                   0x000000, 0x100000,  CRC(0e0107f1) SHA1(914f684f085927257020ffaa1ca536ec057e1603) )
MEGADRIVE_ROM_LOAD( rugbyw, "rugby world cup 1995 (usa, europe) (en,fr,it).bin",                                         0x000000, 0x200000,  CRC(61f90a8a) SHA1(9b435c82b612e23cb512efaebf4d35b203339e44) )
MEGADRIVE_ROM_LOAD( runark, "runark (japan, korea).bin",                                                                 0x000000, 0x80000,   CRC(0894d8fb) SHA1(25a0322db3b82e3da6f1dab550b3e52f21be6b58) )
MEGADRIVE_ROM_LOAD( ryuuko, "ryuuko no ken (japan).bin",                                                                 0x000000, 0x200000,  CRC(054cf5f6) SHA1(c4adc3fc6e1130f64440c7dc2673b239f1523626) )
MEGADRIVE_ROM_LOAD( sagaia, "sagaia (usa).bin",                                                                          0x000000, 0x100000,  CRC(f1e22f43) SHA1(0aa2632da5ec3c1db21b273bcb8630d84b7bb805) )
MEGADRIVE_ROM_LOAD( sswordj, "saint sword (japan).bin",                                                                  0x000000, 0x80000,   CRC(3960a00f) SHA1(e1e2bbbaf3e64c62fa695d1d1ee3496826951d13) )
MEGADRIVE_ROM_LOAD( ssword, "saint sword (usa).bin",                                                                     0x000000, 0x80000,   CRC(44f66bff) SHA1(53e376faed7fe20c5ffe78a568dd8d2cf3fe2d1a) )
MEGADRIVE_ROM_LOAD( samesame, "same! same! same! (japan).bin",                                                           0x000000, 0x80000,   CRC(77bbd841) SHA1(b0d2552c5aae75dbe2d63600c0dbd64868c2f2c5) )
MEGADRIVE_ROM_LOAD( sampra, "sampras tennis 96 (europe) (j-cart).bin",                                                   0x000000, 0x200000,  CRC(14e3fb7b) SHA1(139b9cea418c7c932033ddff87bf10466828cbfa) )
MEGADRIVE_ROM_LOAD( samsho, "samurai shodown (europe).bin",                                                              0x000000, 0x300000,  CRC(c972014f) SHA1(86384123c49c05b6f90cffd7327c11882f01e9c0) )
MEGADRIVE_ROM_LOAD( samshou, "samurai shodown (usa).bin",                                                                0x000000, 0x300000,  CRC(5bb8b2d4) SHA1(504cc7a948f96ce1989f8e91ffb42acc6a956e47) )
MEGADRIVE_ROM_LOAD( samspir, "samurai spirits (japan).bin",                                                              0x000000, 0x300000,  CRC(0ea2ae36) SHA1(54a54996035b220ce72aa7e725f99860e3cd66a0) )
MEGADRIVE_ROM_LOAD( sanguo, "san guo zhi lie zhuan - luan shi qun ying (china).bin",                                     0x000000, 0x100000,  CRC(3b5cc398) SHA1(7aaa0cc1dafaa14e6d62d6c1dbd462e69617bf7e) )
MEGADRIVE_ROM_LOAD( sanguo5, "san guo zhi v (china) (unlicensed).bin",                                                   0x000000, 0x200000,  CRC(cd7e53d0) SHA1(1a3333983f40dd242ad86187e50d1abed68d5ae9) )
MEGADRIVE_ROM_LOAD( sangok2, "sangokushi ii (japan).bin",                                                                0x000000, 0x100000,  CRC(437ba326) SHA1(d9a912caacaa476a890564f62b6ce9cc8f60496e) )
MEGADRIVE_ROM_LOAD( sangok3, "sangokushi iii (japan).bin",                                                               0x000000, 0x180000,  CRC(a8de6aea) SHA1(c85d03a6bcfeccc66ea3fa3cbda006c83576815f) )
MEGADRIVE_ROM_LOAD( sangokr, "sangokushi retsuden - ransei no eiyuutachi (japan).bin",                                   0x000000, 0x100000,  CRC(0f56785a) SHA1(9b79f91060e68b9f6f48c8a5f38bc9873d03c4a9) )
MEGADRIVE_ROM_LOAD( slammast, "saturday night slammasters (europe).bin",                                                 0x000000, 0x400000,  CRC(08fa5a3f) SHA1(f67f324165abdf148f80aabb319375dc3a504e17) )
MEGADRIVE_ROM_LOAD( slammastu, "saturday night slammasters (usa).bin",                                                   0x000000, 0x400000,  CRC(2fb4eaba) SHA1(7252006bf41e41b92c6261c6722766a1352a6dca) )
MEGADRIVE_ROM_LOAD( scooby, "scooby doo mystery (usa).bin",                                                              0x000000, 0x200000,  CRC(7bb9dd9b) SHA1(ccc9542d964de5bdc1e9101620cae40dd98e7127) )
MEGADRIVE_ROM_LOAD( scrabble, "scrabble (europe) (prototype).bin",                                                       0x000000, 0x100000,  CRC(360b2610) SHA1(b2a06070eda30eb2ef745dcb2676dc25d39a07ed) )
MEGADRIVE_ROM_LOAD( sdvalis, "sd valis (japan).bin",                                                                     0x000000, 0x80000,   CRC(1aef72ea) SHA1(4b4509936a75cb9b3b6fbf75b617be475cb99ffd) )
MEGADRIVE_ROM_LOAD( seaquest, "seaquest dsv (europe).bin",                                                               0x000000, 0x200000,  CRC(0bb511bd) SHA1(e0324a23bc26b40f7e564ae864afb8ede1776f6e) )
MEGADRIVE_ROM_LOAD( seaquestu, "seaquest dsv (usa).bin",                                                                 0x000000, 0x200000,  CRC(25b05480) SHA1(ba34c2016684f1d3fb0854de4f8a92390e24bc52) )
MEGADRIVE_ROM_LOAD( second, "second samurai (europe).bin",                                                               0x000000, 0x100000,  CRC(78e92143) SHA1(72bfe0d4c56d9600a39fae00399fef157eb1fe6f) )
MEGADRIVE_ROM_LOAD( segachnl, "sega channel (usa) (program).bin",                                                        0x000000, 0x40000,   CRC(bc79b6ed) SHA1(23a32e7709fc78b1b217a9d4bf2556117d7b431a) )
MEGADRIVE_ROM_LOAD( segasp, "sega sports 1 (europe).bin",                                                                0x000000, 0x280000,  CRC(07fedaf1) SHA1(ff03bb2aced48c82de0ddfb048b114ec84daf16a) )
MEGADRIVE_ROM_LOAD( segatop, "sega top five (brazil).bin",                                                               0x000000, 0x200000,  CRC(61069069) SHA1(b13ba8e49fb9e8b41835e3d4af86ad25f27c804c) )
MEGADRIVE_ROM_LOAD( mercs, "mercs / senjou no ookami ii (world).bin",                                                    0x000000, 0x100000,  CRC(16113a72) SHA1(ab633913147325190ef13bd10f3ef5b06d3a9e28) )
MEGADRIVE_ROM_LOAD( sensibleb, "sensible soccer (beta).bin",                                                             0x000000, 0x80000,   CRC(ef52664d) SHA1(c12a644cf81886050a4ae108b17b3c742055f5c3) )
MEGADRIVE_ROM_LOAD( sensible, "sensible soccer (europe) (en,fr,de,it).bin",                                              0x000000, 0x80000,   CRC(f9b396b8) SHA1(2ee1ad4aac354b6d227b16976f0875f4625ff604) )
MEGADRIVE_ROM_LOAD( sensibie, "sensible soccer - international edition (europe) (en,fr,de,it).bin",                      0x000000, 0x80000,   CRC(04e3bcca) SHA1(b88833ba92e084d4433e2b22eeb67e71ed36cd5c) )
MEGADRIVE_ROM_LOAD( sesame, "sesame street counting cafe (usa).bin",                                                     0x000000, 0x100000,  CRC(0a4f48c3) SHA1(d5ef2b50cf1a22e07401f6a14c7946df66d8b605) )
MEGADRIVE_ROM_LOAD( shadow, "shadow blasters (usa).bin",                                                                 0x000000, 0x80000,   CRC(713d377b) SHA1(8485453264fd67ff8e4549a9db50b4f314a6fcb5) )
MEGADRIVE_ROM_LOAD( shdanc, "shadow dancer - the secret of shinobi (world).bin",                                         0x000000, 0x80000,   CRC(ebe9ad10) SHA1(c5f096b08470a564a737140b71247748608c32b6) )
MEGADRIVE_ROM_LOAD( beast, "shadow of the beast (usa, europe).bin",                                                      0x000000, 0x100000,  CRC(bd385c27) SHA1(2f6f5165d3aa40213c8020149d5e4b29f86dfba8) )
MEGADRIVE_ROM_LOAD( beastj, "shadow of the beast - mashou no okite (japan).bin",                                         0x000000, 0x100000,  CRC(0cd09d31) SHA1(6e4bb7fb1a0f642d72cf58376bf2637e0ac6ef50) )
MEGADRIVE_ROM_LOAD( beast2, "shadow of the beast ii (usa, europe).bin",                                                  0x000000, 0x100000,  CRC(2dede3db) SHA1(260a497348313c22f2d1a626150b0915990720c2) )
MEGADRIVE_ROM_LOAD( shdwrunj, "shadowrun (japan).bin",                                                                   0x000000, 0x200000,  CRC(d32199f7) SHA1(e28bb51227abcc71aaf18d445d3651054247c662) )
MEGADRIVE_ROM_LOAD( shdwruna, "shadowrun (prototype - dec 28, 1993).bin",                                                0x000000, 0x200000,  CRC(2455add2) SHA1(2d52cb444802ae6fda8d50b1af5fab10826bc00d) )
MEGADRIVE_ROM_LOAD( shdwrunb, "shadowrun (prototype - dec 31, 1993).bin",                                                0x000000, 0x200000,  CRC(bbb5e2fa) SHA1(4bb4f490ff22c380df471b63faeb8835f9cf5cb6) )
MEGADRIVE_ROM_LOAD( shdwrunc, "shadowrun (prototype - jan 25, 1994 - c).bin",                                            0x000000, 0x200000,  CRC(6e2bbca8) SHA1(37b0aecf9d72fa662cadec9c2eba9bbe6fea35d2) )
MEGADRIVE_ROM_LOAD( shdwrund, "shadowrun (prototype - jan 25, 1994).bin",                                                0x000000, 0x200000,  CRC(2a964bcd) SHA1(01591ae041031a4820caeb0bfcb1772e8b633352) )
MEGADRIVE_ROM_LOAD( shdwrun, "shadowrun (usa).bin",                                                                      0x000000, 0x200000,  CRC(fbb92909) SHA1(a06a281d39e845bff446a541b2ff48e1d93143c2) )
MEGADRIVE_ROM_LOAD( shanew, "shane warne cricket (australia).bin",                                                       0x000000, 0x100000,  CRC(68865f6f) SHA1(4264625be91c3d96edca9c5bfe6259d00ca8b737) )
MEGADRIVE_ROM_LOAD( shai2b, "shanghai ii - dragon's eye (beta).bin",                                                     0x000000, 0x100000,  CRC(5e33867b) SHA1(bd136fd6485b653456a8bf27837ce20139b28dcc) )
MEGADRIVE_ROM_LOAD( shai2a, "shanghai ii - dragon's eye (usa) (beta).bin",                                               0x000000, 0x100000,  CRC(154aca2d) SHA1(89150c5d976870868e2a11b474b4c0f9e2a786a9) )
MEGADRIVE_ROM_LOAD( shai2, "shanghai ii - dragon's eye (usa).bin",                                                       0x000000, 0x100000,  CRC(ebe9e840) SHA1(7d6dc54b8943880a5fd26af10364eb943e9724c2) )
MEGADRIVE_ROM_LOAD( shaqfu, "shaq fu (usa, europe).bin",                                                                 0x000000, 0x300000,  CRC(499955f2) SHA1(1ad8cd54f8fe474da01d1baba98491411c4cedaf) )
MEGADRIVE_ROM_LOAD( shijie, "shi jie zhi bang zheng ba zhan - world pro baseball 94 (china) (unlicensed).bin",           0x000000, 0x200000,  CRC(72dd884f) SHA1(d05c770312feec38c45b910ad09204b87961c01a) )
MEGADRIVE_ROM_LOAD( shijou, "shijou saidai no soukoban (japan).bin",                                                     0x000000, 0x40000,   CRC(40f4aacc) SHA1(4f5341f3cd0b648c2116593b3f706b634a7e0128) )
MEGADRIVE_ROM_LOAD( shikinjo, "shikinjoh (japan).bin",                                                                   0x000000, 0x80000,   CRC(5ea0c97d) SHA1(34add3f084636d671b48e77a771329f23f8083cd) )
MEGADRIVE_ROM_LOAD( shinso, "shin souseiki ragnacenty (japan).bin",                                                      0x000000, 0x200000,  CRC(6a3f5ae2) SHA1(e0df9f3cb64beb3ea921653eccdd45aca6abc0aa) )
MEGADRIVE_ROM_LOAD( shindarkj, "shining and the darkness (japan).bin",                                                   0x000000, 0x100000,  CRC(496af51c) SHA1(c09ff5bcee1a29a48c65be4ad584708b85ca549b) )
MEGADRIVE_ROM_LOAD( shinfrceb, "shining force (usa) (beta).bin",                                                         0x000000, 0x180000,  CRC(ce67143a) SHA1(f026c9243431c9c2d4b0660e340158816a22b869) )
MEGADRIVE_ROM_LOAD( shinfrce, "shining force (usa).bin",                                                                 0x000000, 0x180000,  CRC(e0594abe) SHA1(7cbb3ed31c982750d70a273b9561a9e1b2c04eea) )
MEGADRIVE_ROM_LOAD( shinfrcej, "shining force - kamigami no isan (japan).bin",                                           0x000000, 0x180000,  CRC(9378fbcd) SHA1(ef8afbdc9af931d9da34d72efc8a76f0d5f4379d) )
MEGADRIVE_ROM_LOAD( shinfrc2, "shining force ii (europe).bin",                                                           0x000000, 0x200000,  CRC(83cb46d1) SHA1(a2ba86f4d756f98886f8f9a56f20cbf8c3b2945e) )
MEGADRIVE_ROM_LOAD( shinfrc2pa, "shining force ii (prototype - apr 04, 1994).bin",                                       0x000000, 0x200000,  CRC(5843670c) SHA1(cd98c45ec170ce72fabe2237cc55dc4f0b6aa884) )
MEGADRIVE_ROM_LOAD( shinfrc2pb, "shining force ii (prototype - jun 07, 1994).bin",                                       0x000000, 0x200000,  CRC(03b68bba) SHA1(caeb48ed31991614c21121bf7d7a899aea946a81) )
MEGADRIVE_ROM_LOAD( shinfrc2u, "shining force ii (usa).bin",                                                             0x000000, 0x200000,  CRC(4815e075) SHA1(22defc2e8e6c1dbb20421b906796538725b3d893) )
MEGADRIVE_ROM_LOAD( shinfrc2j, "shining force ii - koe no fuuin (japan).bin",                                            0x000000, 0x200000,  CRC(0288f3e1) SHA1(8e1f1a510af4d43716d9ee34d47becf907dec147) )
MEGADRIVE_ROM_LOAD( shindarkb, "shining in the darkness (brazil).bin",                                                   0x000000, 0x100000,  CRC(3ee2bbc4) SHA1(08c6e884d48329c45d9f090aeea03efd4c1918c0) )
MEGADRIVE_ROM_LOAD( shindark, "shining in the darkness (usa, europe).bin",                                               0x000000, 0x100000,  CRC(4d2785bc) SHA1(4e10c90199d6edd2030a4ba1c42c7c166bf309ec) )
MEGADRIVE_ROM_LOAD( shinob3, "shinobi iii - return of the ninja master (europe).bin",                                    0x000000, 0x100000,  CRC(0b6d3eb5) SHA1(23579c8f7e2396080b478b113aff36d2382395a3) )
MEGADRIVE_ROM_LOAD( shinob3u, "shinobi iii - return of the ninja master (usa).bin",                                      0x000000, 0x100000,  CRC(5381506f) SHA1(1e07d7998e3048fcfba4238ae96496460e91b3a5) )
MEGADRIVE_ROM_LOAD( ship, "ship (unreleased).bin",                                                                       0x000000, 0x20000,   CRC(4cdc9f16) SHA1(95bab798ecd769567300e1dddfbed3aeee206e87) )
MEGADRIVE_ROM_LOAD( shiten, "shiten myouou (japan).bin",                                                                 0x000000, 0x80000,   CRC(7e729693) SHA1(7bc3be0753b4ba8bbab2f5096e0efa0c0884dd98) )
MEGADRIVE_ROM_LOAD( shougi, "shougi no hoshi (japan).bin",                                                               0x000000, 0x40000,   CRC(4148f816) SHA1(097fc6e7b7d9152f538d973a1837c09b711bf9d2) )
MEGADRIVE_ROM_LOAD( shoveit, "shove it! ...the warehouse game (usa).bin",                                                0x000000, 0x20000,   CRC(c51f40cb) SHA1(e4094c5a575f8d7325e7ec7425ecf022a6bf434e) )
MEGADRIVE_ROM_LOAD( sdm, "show do milhao (brazil).bin",                                                                  0x000000, 0x200000,  CRC(0a22df04) SHA1(6f6ade812eb18e093a53773b8481c78f5a25631c) )
MEGADRIVE_ROM_LOAD( sdm2a, "show do milhao volume 2 (brazil) (alt).bin",                                                 0x000000, 0x200000,  CRC(d8c9ac6d) SHA1(d9c3b494086fef2fa59e031436756236f3cfdf22) )
MEGADRIVE_ROM_LOAD( sdm2, "show do milhao volume 2 (brazil).bin",                                                        0x000000, 0x200000,  CRC(48ee66cb) SHA1(8f713ef6035b616877ed9a09bac07913470c1f2f) )
MEGADRIVE_ROM_LOAD( shuihu, "shui hu - feng yun zhuan (china) (unlicensed).bin",                                         0x000000, 0x200000,  CRC(3e9e010c) SHA1(7ac1b7a9e36dde9a03b3d0861b473ac9e1324175) )
MEGADRIVE_ROM_LOAD( shuihu1, "shui hu zhuan (china) (unlicensed).bin",                                                   0x000000, 0x200000,  CRC(61e458c3) SHA1(f58c522bd0629833d3943ef4091c3c349c134879) )
MEGADRIVE_ROM_LOAD( shuramon, "shura no mon (japan).bin",                                                                0x000000, 0x100000,  CRC(e19da6e5) SHA1(22b58751fe7320c53a49e7c2c3e72b0500192d58) )
MEGADRIVE_ROM_LOAD( sidepock, "side pocket (europe).bin",                                                                0x000000, 0x100000,  CRC(36e08145) SHA1(a19cd561397dfda55942cf4f9771b0e815f95f65) )
MEGADRIVE_ROM_LOAD( sidepockj, "side pocket (japan).bin",                                                                0x000000, 0x100000,  CRC(336bbf3e) SHA1(161c1f67b712d8f4f82115877072486832b913fb) )
MEGADRIVE_ROM_LOAD( sidepocku, "side pocket (usa).bin",                                                                  0x000000, 0x100000,  CRC(af9f275d) SHA1(2b30982d04628edc620d8d99f7dceb4ed87b41e3) )
MEGADRIVE_ROM_LOAD( bartvssm, "mpr-14759a.bin",                                                                          0x000000, 0x80000,   CRC(db70e8ca) SHA1(2d42143c83ec3b4167860520ee0a9030ef563333) )
MEGADRIVE_ROM_LOAD( bartvssm1, "simpsons, the - bart vs the space mutants (usa, europe).bin",                            0x000000, 0x80000,   CRC(c8620574) SHA1(3cf4447a0a883c78645c6faded28c51e0d8c0d63) )
MEGADRIVE_ROM_LOAD( bartnigt, "simpsons, the - bart's nightmare (usa, europe).bin",                                      0x000000, 0x100000,  CRC(24d7507c) SHA1(fb95b7fdf12dcf62883dabf65d2bf8ffa83786fc) )
MEGADRIVE_ROM_LOAD( skelet, "skeleton krew (europe).bin",                                                                0x000000, 0x200000,  CRC(5f872737) SHA1(9752fbd8508492dae252ae749393281ed9527de0) )
MEGADRIVE_ROM_LOAD( skeletu, "skeleton krew (usa).bin",                                                                  0x000000, 0x200000,  CRC(c2e05acb) SHA1(2a6f6ea7d2fc1f3a396269f9455011ef95266ffc) )
MEGADRIVE_ROM_LOAD( skitchin, "skitchin (usa, europe).bin",                                                              0x000000, 0x200000,  CRC(f785f9d7) SHA1(98be93964c14ebc91727b429dd7a7a563b4e2e9f) )
MEGADRIVE_ROM_LOAD( slamshaq, "slam - shaq vs. the legends (prototype).bin",                                             0x000000, 0x1f485a,  CRC(c19c200e) SHA1(beee61d339b307b2e3124ceac4a49375f7a04c81) )
MEGADRIVE_ROM_LOAD( slapfigh, "slap fight md (japan).bin",                                                               0x000000, 0x100000,  CRC(d6695695) SHA1(3cb992c70b4a5880a3e5f89211f29fe90486c0e0) )
MEGADRIVE_ROM_LOAD( slaughtr, "slaughter sport (usa).bin",                                                               0x000000, 0xa0000,   CRC(af9f9d9c) SHA1(9734c8a4e2beb6e6bb2d14ce0d332825537384e0) )
MEGADRIVE_ROM_LOAD( slimew, "slime world (japan).bin",                                                                   0x000000, 0x80000,   CRC(7ff5529f) SHA1(02cf05687a7f3f8177b2aff0a0cfbd5c490e557d) )
MEGADRIVE_ROM_LOAD( smurfs2, "smurfs travel the world, the (europe) (en,fr,de,es).bin",                                  0x000000, 0x100000,  CRC(b28bdd69) SHA1(b4368369e1d5b9a60bc565fe09a9c5fff6b79fd4) )
MEGADRIVE_ROM_LOAD( smurfs, "smurfs, the (europe) (en,fr,de,es,it) (rev a).bin",                                         0x000000, 0x100000,  CRC(88b30eff) SHA1(0da7e621e05dc9160122d728e1fca645ff11e670) )
MEGADRIVE_ROM_LOAD( snakernr, "snake rattle n' roll (europe).bin",                                                       0x000000, 0x80000,   CRC(543bed30) SHA1(0773ae487319b68695e9a6c3dcd0223695c02609) )
MEGADRIVE_ROM_LOAD( snowbros, "snow bros. - nick & tom (japan).bin",                                                     0x000000, 0x100000,  CRC(11b56228) SHA1(27caf554f48d2e3c9c6745f32dbff231eca66744) )
MEGADRIVE_ROM_LOAD( socket, "socket (usa).bin",                                                                          0x000000, 0x100000,  CRC(3c14e15a) SHA1(fee9eb272362bd6f36d552a9ebfd25f5f0db7d2f) )
MEGADRIVE_ROM_LOAD( soldeace, "sol-deace (usa).bin",                                                                     0x000000, 0x100000,  CRC(a77e4e9f) SHA1(c10545e20a147d4fbf228db4a2b6e309799708c3) )
MEGADRIVE_ROM_LOAD( soldfort, "soldiers of fortune (usa).bin",                                                           0x000000, 0x180000,  CRC(a84d28a1) SHA1(619faee3d78532478aad405db335d00fb93e6850) )
MEGADRIVE_ROM_LOAD( soleil, "soleil (europe).bin",                                                                       0x000000, 0x200000,  CRC(a30ebdb1) SHA1(110a61671b83fe17fba768ab85b535ca1cc6d7ea) )
MEGADRIVE_ROM_LOAD( soleilf, "soleil (france).bin",                                                                      0x000000, 0x200000,  CRC(08dc1ead) SHA1(7890074018f165eeb1281d81039fb07ccde7d197) )
MEGADRIVE_ROM_LOAD( soleilg, "soleil (germany).bin",                                                                     0x000000, 0x200000,  CRC(332b9ecd) SHA1(65c8b7ab94b05812d009b4bebda3c49891a6bfbe) )
MEGADRIVE_ROM_LOAD( soleilj, "soleil (japan) (beta).bin",                                                                0x000000, 0x100000,  CRC(43797455) SHA1(963f13f74d648c573ced103b2803d15d0a2742e8) )
MEGADRIVE_ROM_LOAD( soleils, "soleil (spain).bin",                                                                       0x000000, 0x200000,  CRC(9ed4c323) SHA1(95ae529cecb7dbca281df25c3c4e7cb8f48d936c) )
MEGADRIVE_ROM_LOAD( ska, "sonic & knuckles (prototype 0525 - may 25, 1994, 15.28).bin",                                  0x000000, 0x400000,  CRC(8e8dadd0) SHA1(5f2c4dc4e739d562e9f0525361ba83f3e0551e21) )
MEGADRIVE_ROM_LOAD( skb, "sonic & knuckles (prototype 0606 - jun 06, 1994, 10.02).bin",                                  0x000000, 0x200000,  CRC(03a52f63) SHA1(173524ffae78438b5ddd039ec8b8def786f28aee) )
MEGADRIVE_ROM_LOAD( skc, "sonic & knuckles (prototype 0608 - jun 08, 1994, 05.03).bin",                                  0x000000, 0x200000,  CRC(7a6c1317) SHA1(a55b784590e602a82719d6721b2789f686564d2b) )
MEGADRIVE_ROM_LOAD( skd, "sonic & knuckles (prototype 0610 - jun 10, 1994, 07.49).bin",                                  0x000000, 0x200000,  CRC(7092f368) SHA1(bc67cb9edb8958a66723a8346c9d58d43d18ca80) )
MEGADRIVE_ROM_LOAD( ske, "sonic & knuckles (prototype 0612 - jun 12, 1994, 18.27).bin",                                  0x000000, 0x200000,  CRC(b0a253e8) SHA1(8585b617e6828bf0b255d4ef747384db7ff67826) )
MEGADRIVE_ROM_LOAD( skf, "sonic & knuckles (prototype 0618 - jun 18, 1994, 09.15).bin",                                  0x000000, 0x200000,  CRC(2615f5dc) SHA1(e365c42a2c754b9df98b5b74dbc3e7f94c1a84f4) )
MEGADRIVE_ROM_LOAD( skg, "sonic & knuckles (prototype 0619 - jun 19, 1994, 08.18).bin",                                  0x000000, 0x200000,  CRC(1ea5b9d1) SHA1(b84d197e646ceece8681d35af3b4014c1bbeae35) )
MEGADRIVE_ROM_LOAD( skh, "sonic & knuckles (s2k chip) (prototype 0606 - jun 05, 1994, 22.25).bin",                       0x000000, 0x40000,   CRC(bd619ea9) SHA1(f557741d93f8d3e8a84099ab88f0331ced2875c8) )
MEGADRIVE_ROM_LOAD( ski, "sonic & knuckles (s2k chip) (prototype 0608 - jun 08, 1994, 03.35).bin",                       0x000000, 0x40000,   CRC(6a5dcbe6) SHA1(f27434f63ce45de92c361b0b3aabdfde80c237da) )
MEGADRIVE_ROM_LOAD( skj, "sonic & knuckles (s2k chip) (prototype 0610 - jun 10, 1994, 03.11).bin",                       0x000000, 0x40000,   CRC(0f6ff22b) SHA1(d2b4003a7110f258641b7b76d7716acbcb3b62ab) )
MEGADRIVE_ROM_LOAD( skk, "sonic & knuckles (s2k chip) (prototype 0612 - jun 12, 1994, 18.18).bin",                       0x000000, 0x40000,   CRC(9a5f8183) SHA1(eb29700a161a1949ed3057a3580911c7932315a5) )
MEGADRIVE_ROM_LOAD( skl, "sonic & knuckles (s2k chip) (prototype 0618 - jun 18, 1994, 9.07).bin",                        0x000000, 0x40000,   CRC(4dcfd55c) SHA1(70429f1d80503a0632f603bf762fe0bbaa881d22) )
MEGADRIVE_ROM_LOAD( sk, "sonic & knuckles (world).bin",                                                                  0x000000, 0x200000,  CRC(0658f691) SHA1(88d6499d874dcb5721ff58d76fe1b9af811192e3) )
MEGADRIVE_ROM_LOAD( sks1, "sonic & knuckles + sonic the hedgehog (world).bin",                                           0x000000, 0x280000,  CRC(e01f6ed5) SHA1(a3084262f5af481df1a5c5ab03c4862551a53c91) )
MEGADRIVE_ROM_LOAD( sks2, "sonic & knuckles + sonic the hedgehog 2 (world).bin",                                         0x000000, 0x340000,  CRC(2ac1e7c6) SHA1(6cd0537a3aee0e012bb86d5837ddff9342595004) )
MEGADRIVE_ROM_LOAD( sks3, "sonic & knuckles + sonic the hedgehog 3 (world).bin",                                         0x000000, 0x400000,  CRC(63522553) SHA1(cfbf98c36c776677290a872547ac47c53d2761d6) )
MEGADRIVE_ROM_LOAD( sonic3c, "sonic 3c (prototype 0408 - apr 08, 1994, 17.29).bin",                                      0x000000, 0x400000,  CRC(59d23df5) SHA1(5f96ddccff1e95d82201687b939973c642a05394) )
MEGADRIVE_ROM_LOAD( sonic3ca, "sonic 3c (prototype 0517 - may 17, 1994, 17.08).bin",                                     0x000000, 0x400000,  CRC(766c4b81) SHA1(d6012af0f7856476892982e50b3d512d606dcb96) )
MEGADRIVE_ROM_LOAD( s3da, "sonic 3d blast (prototype 73 - jul 03, 1996, 13.58).bin",                                     0x000000, 0x200000,  CRC(93b75e99) SHA1(5343001b4608e2eef02e11d4ca2d36df0a34b8a0) )
MEGADRIVE_ROM_LOAD( s3db, "sonic 3d blast (prototype 814 - aug 15, 1996, 07.55).bin",                                    0x000000, 0x400000,  CRC(d64e7675) SHA1(65ceb54f904cbc55c61547705455d1ef9832fee6) )
MEGADRIVE_ROM_LOAD( s3dc, "sonic 3d blast (prototype 819 - aug 19, 1996, 19.49).bin",                                    0x000000, 0x400000,  CRC(2c43f43a) SHA1(334ed5ca3e1aea2d9714bfd4c9a5697ae6a862d9) )
MEGADRIVE_ROM_LOAD( s3dd, "sonic 3d blast (prototype 825 - aug 26, 1996, 15.46).bin",                                    0x000000, 0x400000,  CRC(465bcfbd) SHA1(228ee9a01afc7d6a65c64fb0b0ba66f5a1d72718) )
MEGADRIVE_ROM_LOAD( s3de, "sonic 3d blast (prototype 830 - aug 31, 1996, 08.19).bin",                                    0x000000, 0x3fff06,  CRC(57640422) SHA1(44fa57e640380a46580f75f1a1460d3f6ecc12b8) )
MEGADRIVE_ROM_LOAD( s3df, "sonic 3d blast (prototype 831 - sep 03, 1996, 10.07).bin",                                    0x000000, 0x400000,  CRC(5933e453) SHA1(1ec28a2495ca70c897dd2b513db7e8fdcc30d7f6) )
MEGADRIVE_ROM_LOAD( s3dg, "sonic 3d blast (prototype 94 - sep 04, 1996, 12.01).bin",                                     0x000000, 0x400000,  CRC(2f2a4271) SHA1(d8aea4a5b3b593985e66ed37104f4f536f0d291b) )
MEGADRIVE_ROM_LOAD( s3dh, "sonic 3d blast (usa) (beta).bin",                                                             0x000000, 0x400000,  CRC(d20f385b) SHA1(3b77d50db02235ce095bd8b30b680700be5deb3e) )
MEGADRIVE_ROM_LOAD( s3d, "sonic 3d blast / sonic 3d - flickies' island (usa, europe, korea).bin",                        0x000000, 0x400000,  CRC(44a2ca44) SHA1(89957568386a5023d198ac2251ded9dfb2ab65e7) )
MEGADRIVE_ROM_LOAD( soniccmp1, "mpr-18203+mpr-18204.bin",                                                                0x000000, 0x300000,  CRC(8c70b84e) SHA1(109e6d7b31d00fbdb9d4ecb304d4ea19e96c8607) )
MEGADRIVE_ROM_LOAD( soniccmp, "mpr-19693.bin",                                                                           0x000000, 0x300000,  CRC(95b5e8d7) SHA1(2196cbe754cfcd1d7bbbd0a8a45ae44de4deb2fb) )
MEGADRIVE_ROM_LOAD( scrack, "sonic crackers (japan) (prototype).bin",                                                    0x000000, 0x100000,  CRC(7fada88d) SHA1(05f460a6ebbc1f86eb40b5f762083a59fb29f3e2) )
MEGADRIVE_ROM_LOAD( sonicer, "sonic eraser (japan) (seganet).bin",                                                       0x000000, 0x40000,   CRC(62d8a0e7) SHA1(be70d4246be49c0301a1402bd93f28c58b558a8d) )
MEGADRIVE_ROM_LOAD( sonicjam, "sonic jam 6 (unlicensed).bin",                                                            0x000000, 0x200000,  CRC(bf39d897) SHA1(3a6fe5a92dc2ada7e9ab17ac120c7e50d1f9a1ed) )
MEGADRIVE_ROM_LOAD( sonicjam1, "sonic jam 6 (unlicensed) (pirate).bin",                                                  0x000000, 0x200000,  CRC(04f0c93e) SHA1(a9e316ccde5b71f6aa85485b6897c1cfc780742d) )
MEGADRIVE_ROM_LOAD( sspin, "sonic spinball (europe).bin",                                                                0x000000, 0x100000,  CRC(aea0786d) SHA1(f61a568314133b60de82ac162b5b52473adc9e1c) )
MEGADRIVE_ROM_LOAD( sspina, "sonic spinball (japan).bin",                                                                0x000000, 0x100000,  CRC(acd08ce8) SHA1(43b2fdac9c747d6f6a629347c589599074408cd9) )
MEGADRIVE_ROM_LOAD( sspinb, "sonic spinball (usa) (alt).bin",                                                            0x000000, 0x100000,  CRC(e9960371) SHA1(8f372e3552e309d3462adeb700242b251f59def1) )
MEGADRIVE_ROM_LOAD( sspinc, "sonic spinball (usa) (beta).bin",                                                           0x000000, 0x140000,  CRC(b1524979) SHA1(b426457e5b440ed33ee1756bc6dad2bdcd0c0d9f) )
MEGADRIVE_ROM_LOAD( sspind, "sonic spinball (usa).bin",                                                                  0x000000, 0x100000,  CRC(677206cb) SHA1(24bf6342b98c09775089c9f39cfb2f6fbe7806f7) )
MEGADRIVE_ROM_LOAD( sonica, "mpr-13933.bin",                                                                             0x000000, 0x80000,   CRC(afe05eee) SHA1(69e102855d4389c3fd1a8f3dc7d193f8eee5fe5b) )
MEGADRIVE_ROM_LOAD( sonic, "mpr-13913.bin",                                                                              0x000000, 0x80000,   CRC(f9394e97) SHA1(6ddb7de1e17e7f6cdb88927bd906352030daa194) )
MEGADRIVE_ROM_LOAD( sonicb, "sonic the hedgehog (world) (pirate).bin",                                                   0x000000, 0x80000,   CRC(7a093f0b) SHA1(ad7502fa15b1819eeb4089783b4a850d898bc71e) )
MEGADRIVE_ROM_LOAD( sonic2b, "sonic the hedgehog 2 (beta 4 - sep 18, 1992, 16.26).bin",                                  0x000000, 0x100000,  CRC(8fda5cc5) SHA1(66ec1392f13a6e8ed8d0837492ca32daad769900) )
MEGADRIVE_ROM_LOAD( sonic2c, "sonic the hedgehog 2 (beta 5 - sep 21, 1992, 12.06).bin",                                  0x000000, 0x100000,  CRC(066b9a89) SHA1(b60f424e77be28ffa6937de3a08256ca55e6fe40) )
MEGADRIVE_ROM_LOAD( sonic2d, "sonic the hedgehog 2 (beta 6 - sep 22, 1992, 18.47).bin",                                  0x000000, 0x100000,  CRC(cebc64e0) SHA1(21dfc1142809622d5548e83f2c3544c34e0b2320) )
MEGADRIVE_ROM_LOAD( sonic2e, "sonic the hedgehog 2 (beta 6 - sep 22, 1992, 19.42).bin",                                  0x000000, 0x100000,  CRC(cb036e6c) SHA1(c9fdce433de4661e5f6f276a9b8c1b2b139b6709) )
MEGADRIVE_ROM_LOAD( sonic2f, "sonic the hedgehog 2 (beta 7 - sep 24, 1992, 09.26).bin",                                  0x000000, 0x100000,  CRC(dc7be12c) SHA1(14c6ec07bcc1450d7ed3afcc9994c7d994a9b52d) )
MEGADRIVE_ROM_LOAD( sonic2g, "sonic the hedgehog 2 (beta 8 - sep 24, 1992, 19.27).bin",                                  0x000000, 0x100000,  CRC(6089fadd) SHA1(5f2dce167d03566d356dbdbcdd38b1b1dfb56b09) )
MEGADRIVE_ROM_LOAD( sonic2h, "sonic the hedgehog 2 (prototype).bin",                                                     0x000000, 0x100000,  CRC(eea21b5c) SHA1(2f3228088b000260c2e00961efb0ed76629c84e9) )
MEGADRIVE_ROM_LOAD( sonic2i, "sonic the hedgehog 2 (world) (rev 01a).bin",                                               0x000000, 0x100000,  CRC(92d8817d) SHA1(4050c7ba90a53138407c66dcae9891e5a5c73e8f) )
// rip from console Sonic collections?

MEGADRIVE_ROM_LOAD( sonic2j, "sonic the hedgehog 2 (world) (rev sc02).bin",                                              0x000000, 0x100000,  CRC(f23ad4b3) SHA1(2af1003247aec262089c8df22d05e80d04a1b5e4) )
// rip from console Sonic collections?

MEGADRIVE_ROM_LOAD( sonic2k, "sonic the hedgehog 2 (world) (beta).bin",                                                  0x000000, 0x100000,  CRC(39faaa70) SHA1(5b51b4d98cb4a7a38157dc4ab9462164dd224bfd) )
MEGADRIVE_ROM_LOAD( sonic2, "mpr-15000a.bin",                                                                            0x000000, 0x100000,  CRC(7b905383) SHA1(8bca5dcef1af3e00098666fd892dc1c2a76333f9) )
MEGADRIVE_ROM_LOAD( sonic2a, "sonic the hedgehog 2 (world).bin",                                                         0x000000, 0x100000,  CRC(24ab4c3a) SHA1(14dd06fc3aa19a59a818ea1f6de150c9061b14d4) )
MEGADRIVE_ROM_LOAD( sonic3, "sonic the hedgehog 3 (europe).bin",                                                         0x000000, 0x200000,  CRC(6a632503) SHA1(2ff45bb056ede0f745e52f8d02c54b4ca724ca4c) )
MEGADRIVE_ROM_LOAD( sonic3a, "sonic the hedgehog 3 (argentinian pirate).bin",                                            0x000000, 0x200000,  CRC(c818f6fd) SHA1(bc2b67803bbba89a456a464e679cde4b4bb567fb) )
MEGADRIVE_ROM_LOAD( sonic3u, "sonic the hedgehog 3 (usa).bin",                                                           0x000000, 0x200000,  CRC(9bc192ce) SHA1(75e9c4705259d84112b3e697a6c00a0813d47d71) )
MEGADRIVE_ROM_LOAD( sonic3j, "sonic the hedgehog 3 (japan, korea).bin",                                                  0x000000, 0x200000,  CRC(f4951d1f) SHA1(7b98b21b7274233e962132bc22a7ccdf548c0ddb) )
MEGADRIVE_ROM_LOAD( sorcerkj, "sorcer kingdom (japan).bin",                                                              0x000000, 0x100000,  CRC(944135ca) SHA1(16394aebece9d03f43505eab0827889e1c61857f) )
MEGADRIVE_ROM_LOAD( sorcerk, "sorcerer's kingdom (usa) (v1.1).bin",                                                      0x000000, 0x100000,  CRC(bb1fc9ce) SHA1(87759abb603f1f97c2e136682dc78eea545338ce) )
MEGADRIVE_ROM_LOAD( sorcerk1, "sorcerer's kingdom (usa).bin",                                                            0x000000, 0x100000,  CRC(cbe6c1ea) SHA1(57322c1714fd4e42e1a10d56bfd795bcbc3380d7) )
MEGADRIVE_ROM_LOAD( sorcern, "sorcerian (japan).bin",                                                                    0x000000, 0x80000,   CRC(a143a8c5) SHA1(cbd7f0693a0d127138977da7cdf5a7f9440dfd43) )
MEGADRIVE_ROM_LOAD( soulblad, "soul blade (unlicensed).bin",                                                             0x000000, 0x400000,  CRC(f26f88d1) SHA1(7714e01819ab4a0f424d7e306e9f891031d053a8) )
MEGADRIVE_ROM_LOAD( spacef, "space funky b.o.b. (japan).bin",                                                            0x000000, 0x100000,  CRC(e9310d3b) SHA1(2d0e4423e28d7175fc27a9c5b1cb86f1d5cedd3e) )
MEGADRIVE_ROM_LOAD( sharrj, "space harrier ii (japan) (launch cart).bin",                                                0x000000, 0x80000,   CRC(edc0fb28) SHA1(5836fbe907610ff15286911457049933b7cdd49c) )
MEGADRIVE_ROM_LOAD( sharr, "mpr-12355.bin",                                                                              0x000000, 0x80000,   CRC(e5c9cbb0) SHA1(db4285e4ffb69aa9f1ca68c4103fbfd0843f7b86) )
MEGADRIVE_ROM_LOAD( sinv90, "space invaders 90 (japan).bin",                                                             0x000000, 0x40000,   CRC(22adbd66) SHA1(d15830fd1070960d1696c1a9d48c9f7db3aa89e4) )
MEGADRIVE_ROM_LOAD( sinv91, "space invaders 91 (usa).bin",                                                               0x000000, 0x40000,   CRC(bb83b528) SHA1(d8046f1c703ea7c2d7f9f3f08702db7706f56cb4) )
MEGADRIVE_ROM_LOAD( sparks, "sparkster (europe).bin",                                                                    0x000000, 0x100000,  CRC(d63e9f2d) SHA1(91057f22c5cea9bf08edf62862c56b939d570770) )
MEGADRIVE_ROM_LOAD( sparksu, "sparkster (usa).bin",                                                                      0x000000, 0x100000,  CRC(6bdb14ed) SHA1(d6e2f4aa87633aeea2ec1c05a6100b8905549095) )
MEGADRIVE_ROM_LOAD( sparksj, "sparkster - rocket knight adventures 2 (japan).bin",                                       0x000000, 0x100000,  CRC(914ec662) SHA1(49e813e108751d502bfb242551b40121c71c5442) )
MEGADRIVE_ROM_LOAD( speedbl2, "speedball 2 (europe).bin",                                                                0x000000, 0x80000,   CRC(056a6e03) SHA1(9c989b31de7de38bc488f825575b7f6c1db9dbee) )
MEGADRIVE_ROM_LOAD( speedbl2j, "speedball 2 (japan).bin",                                                                0x000000, 0x80000,   CRC(f5442334) SHA1(7da217a45fafb569baee190be6a660af428cf3e5) )
MEGADRIVE_ROM_LOAD( speedbl2u, "speedball 2 - brutal deluxe (usa).bin",                                                  0x000000, 0x80000,   CRC(9fc340a7) SHA1(4598f1714fd05e74ab758c09263f7948c8ca1883) )
MEGADRIVE_ROM_LOAD( spidrmanb, "spider-man (usa) (acclaim) (beta) (earlier).bin",                                        0x000000, 0x200000,  CRC(83e1fe76) SHA1(9b9f443499a547acdf029111df58b5a73ec4058f) )
MEGADRIVE_ROM_LOAD( spidrmanb1, "spider-man (usa) (acclaim) (beta).bin",                                                 0x000000, 0x200000,  CRC(b88a710d) SHA1(0bb3131108ed57c0cd12b646eb2b6dd73eb679f9) )
MEGADRIVE_ROM_LOAD( spidrman, "spider-man (usa, europe) (acclaim).bin",                                                  0x000000, 0x200000,  CRC(11b5b590) SHA1(a38d5ad7d503999b7fea3ebf59f3dda9d667758b) )
MEGADRIVE_ROM_LOAD( spidsega, "spider-man (world) (sega).bin",                                                           0x000000, 0x80000,   CRC(70ab775f) SHA1(250f7a7301a028450eef2f2a9dcec91f99ecccbd) )
MEGADRIVE_ROM_LOAD( sp_mc, "spider-man and venom - maximum carnage (world).bin",                                         0x000000, 0x200000,  CRC(8fa0b6e6) SHA1(43624536cbebd65232abe4af042c0fa4b0d9e3b7) )
MEGADRIVE_ROM_LOAD( sp_sa, "spider-man and venom - separation anxiety (usa, europe).bin",                                0x000000, 0x300000,  CRC(512ade32) SHA1(563b45254c168aa0de0bba8fadd75a2d1e8e094b) )
MEGADRIVE_ROM_LOAD( sp_xm, "spider-man and x-men - arcade's revenge (usa).bin",                                          0x000000, 0x100000,  CRC(4a4414ea) SHA1(978dabcc7d098edebc9d3f2fef04f27fd6aeab19) )
MEGADRIVE_ROM_LOAD( spirit, "spiritual warfare (usa) (unlicensed).bin",                                                  0x000000, 0x80000,   CRC(d9a364ff) SHA1(efd7f8c1d7daf7a0b6cac974de093f224f6e1c32) )
MEGADRIVE_ROM_LOAD( spirou, "spirou (europe) (en,fr,de,es).bin",                                                         0x000000, 0x100000,  CRC(6634b130) SHA1(5f18db9c85df4eaf4647a0519c9dc966aee583fa) )
MEGADRIVE_ROM_LOAD( splatt2, "splatterhouse 2 (europe).bin",                                                             0x000000, 0x100000,  CRC(2559e03c) SHA1(e01940808006a346b8711a74fbfa173ec872624f) )
MEGADRIVE_ROM_LOAD( splatt2u, "splatterhouse 2 (usa).bin",                                                               0x000000, 0x100000,  CRC(2d1766e9) SHA1(59ec19ec442989d2738c055b9290661661d13f8f) )
MEGADRIVE_ROM_LOAD( splatt3, "splatterhouse 3 (usa).bin",                                                                0x000000, 0x200000,  CRC(00f05d07) SHA1(7f29f00ec724e20cb93907f1e33ac4af16879827) )
MEGADRIVE_ROM_LOAD( splatt3j, "splatterhouse part 3 (japan, korea).bin",                                                 0x000000, 0x200000,  CRC(31b83d22) SHA1(1fcb8adfdb19cb772adabac14e78c560d4f2e718) )
MEGADRIVE_ROM_LOAD( splatt3j1, "splatterhouse part 3 (japan) (alt).bin",                                                 0x000000, 0x200000,  CRC(ed68373a) SHA1(a42fc3b2a4f4c2db2f598244c2b137862b8e79ad) )
// Xacrow's dump info report only the other CRC, this is probably a bad dump

MEGADRIVE_ROM_LOAD( sportg, "sport games (brazil).bin",                                                                  0x000000, 0x200000,  CRC(7e3ecabf) SHA1(41604a07b0ac7dff9e01e6829cf84ca911620729) )
MEGADRIVE_ROM_LOAD( sports, "sports talk baseball (usa).bin",                                                            0x000000, 0x100000,  CRC(0deb79c2) SHA1(e223513d9bcecb49a6798720f3195dbd1c34681c) )
MEGADRIVE_ROM_LOAD( spotgo, "spot goes to hollywood (europe).bin",                                                       0x000000, 0x300000,  CRC(fbe254ea) SHA1(be1144c3d9d49dce2d5e1ab598ef0e2b730950b7) )
MEGADRIVE_ROM_LOAD( spotgou, "spot goes to hollywood (usa).bin",                                                         0x000000, 0x300000,  CRC(bdad1cbc) SHA1(064a384f745eeffee3621d55a0278c133abdbc11) )
MEGADRIVE_ROM_LOAD( squirrel, "squirrel king (china) (unlicensed).bin",                                                  0x000000, 0x100000,  CRC(b8261ff5) SHA1(2a561b6e47c93272fe5947084837d9f6f514ed38) )
MEGADRIVE_ROM_LOAD( starctrl, "star control (usa).bin",                                                                  0x000000, 0x180000,  CRC(8e2bceaf) SHA1(108e00cdbcb339834b54fb11819591627fe4b351) )
MEGADRIVE_ROM_LOAD( starcr, "star cruiser (japan).bin",                                                                  0x000000, 0x80000,   CRC(2b75b52f) SHA1(cb099ecde141beffdfed6bb7f1d3dc6340da81d1) )
MEGADRIVE_ROM_LOAD( stds9, "star trek - deep space nine - crossroads of time (europe).bin",                              0x000000, 0x100000,  CRC(d4b122f9) SHA1(efc96336ccd83c31ab48ab48fe1e262c3ebcf0be) )
MEGADRIVE_ROM_LOAD( stds9u, "star trek - deep space nine - crossroads of time (usa).bin",                                0x000000, 0x100000,  CRC(a771e1a4) SHA1(3918dcf48d99d075a6062d967a0f27fb56f89ad2) )
MEGADRIVE_ROM_LOAD( sttngb, "star trek - the next generation - echoes from the past (prototype - dec 28, 1994).bin",     0x000000, 0x200000,  CRC(3ba670f9) SHA1(b3f800ae3009296614d1b0943b4245b4510a30f0) )
MEGADRIVE_ROM_LOAD( sttngc, "star trek - the next generation - echoes from the past (prototype - dec 29, 1994).bin",     0x000000, 0x200000,  CRC(316bbc43) SHA1(e01d14c9dc9cd63800a793350e88af5de71b699b) )
MEGADRIVE_ROM_LOAD( sttngd, "star trek - the next generation - echoes from the past (prototype - jan 03, 1994).bin",     0x000000, 0x200000,  CRC(cac86b68) SHA1(c6d702396d090b80fa1bb86b5079870de1c2dd70) )
MEGADRIVE_ROM_LOAD( sttnge, "star trek - the next generation - echoes from the past (prototype - jan 10, 1994).bin",     0x000000, 0x200000,  CRC(d138ca3e) SHA1(4ed46dc5faafa8e92c753fcad5f37dd7c563e28c) )
MEGADRIVE_ROM_LOAD( sttngf, "star trek - the next generation - echoes from the past (prototype - jan 18, 1994).bin",     0x000000, 0x200000,  CRC(d8dab97a) SHA1(94a7b71aada60f0a622f6156a31f22700c484505) )
MEGADRIVE_ROM_LOAD( sttngg, "star trek - the next generation - echoes from the past (prototype - jan 25, 1994).bin",     0x000000, 0x200000,  CRC(63f29e6f) SHA1(7e1b9bd971088d083c4f9447ae5daff50bcc67c2) )
MEGADRIVE_ROM_LOAD( sttng, "star trek - the next generation - echoes from the past (usa) (v1.1).bin",                    0x000000, 0x200000,  CRC(ef840ef2) SHA1(fe72aff307182dc6970048e88eaa5f03348781f5) )
MEGADRIVE_ROM_LOAD( sttnga, "star trek - the next generation - echoes from the past (usa).bin",                          0x000000, 0x200000,  CRC(272153fb) SHA1(ca0cf81784262fe6c00502cb495ede7daf3685c0) )
MEGADRIVE_ROM_LOAD( starfl, "starflight (usa, europe) (v1.1).bin",                                                       0x000000, 0x100000,  CRC(1217dbea) SHA1(41722d8c0b4865ca8044fb53a818b13eded758fc) )
MEGADRIVE_ROM_LOAD( starfl1, "starflight (usa, europe).bin",                                                             0x000000, 0x100000,  CRC(d550c928) SHA1(24252ca934be3f005436cdb85671413a9e6b0489) )
MEGADRIVE_ROM_LOAD( stargb, "stargate (europe) (beta).bin",                                                              0x000000, 0x200000,  CRC(8dc8ab23) SHA1(95a0dab802a313281d74fe66b21c7877078c57e9) )
MEGADRIVE_ROM_LOAD( starg, "stargate (usa, europe).bin",                                                                 0x000000, 0x200000,  CRC(e587069e) SHA1(d0843442059c89b11db02615670632fda2b2ee85) )
MEGADRIVE_ROM_LOAD( steelemp, "steel empire (usa).bin",                                                                  0x000000, 0x100000,  CRC(d0e7a0b6) SHA1(45c0b9d85b4a8053c3f3432828626dae47022634) )
MEGADRIVE_ROM_LOAD( steeltalj, "steel talons (japan, korea).bin",                                                        0x000000, 0x80000,   CRC(04f388e6) SHA1(93c51ec9fcc56d858642018905b3defc17442c26) )
MEGADRIVE_ROM_LOAD( steeltalb, "steel talons (usa) (beta).bin",                                                          0x000000, 0x80000,   CRC(c4052f18) SHA1(c3edd1f6b9f4fb16104d09c17b66feebb3785f4e) )
MEGADRIVE_ROM_LOAD( steeltal, "steel talons (usa, europe).bin",                                                          0x000000, 0x80000,   CRC(10e4ec63) SHA1(b4f0a13646c13911be5550103301af25827fcd0c) )
MEGADRIVE_ROM_LOAD( slordj, "stormlord (japan).bin",                                                                     0x000000, 0x80000,   CRC(0b440fed) SHA1(fe06ea2d7fcccecce337a535ae683c31aae4a637) )
MEGADRIVE_ROM_LOAD( slord, "stormlord (usa).bin",                                                                        0x000000, 0x80000,   CRC(39ab50a5) SHA1(1bf4b58d50fdc0fdc173ce3dcadcc5d9b58f0723) )
MEGADRIVE_ROM_LOAD( storytho, "story of thor, the (europe).bin",                                                         0x000000, 0x300000,  CRC(1110b0db) SHA1(178ef742dad227d4128fa81dddb116bad0cabe1d) )
MEGADRIVE_ROM_LOAD( storytho1, "story of thor, the (germany).bin",                                                       0x000000, 0x300000,  CRC(fa20d011) SHA1(a82ffb7c4bf4b0f89f42a9cdc6600bc5bac1c854) )
MEGADRIVE_ROM_LOAD( storytho2, "story of thor, the (japan) (beta) (bad dump).bin",                                       0x000000, 0x1b0000, BAD_DUMP CRC(bfc11649) SHA1(ac1952f2f7cd4109561376f3097c4daec2ae64ae) )
MEGADRIVE_ROM_LOAD( storytho3, "story of thor, the (korea).bin",                                                         0x000000, 0x300000,  CRC(ee1603c5) SHA1(e0a43fb3d6da940b1fda449753bffae637a802cd) )
MEGADRIVE_ROM_LOAD( storytho4, "story of thor, the (prototype - nov 01, 1994).bin",                                      0x000000, 0x300000,  CRC(fa59f847) SHA1(cb0606faeab0398244d4721d71cf7e1c5724a9ef) )
MEGADRIVE_ROM_LOAD( storytho5, "story of thor, the (prototype - oct 04, 1994).bin",                                      0x000000, 0x300000,  CRC(9e486f91) SHA1(499dd47c325874a231a8d8430aca0bb6feeb3dcd) )
MEGADRIVE_ROM_LOAD( storytho6, "story of thor, the (prototype - oct 17, 1994).bin",                                      0x000000, 0x300000,  CRC(aa43d34a) SHA1(e4b25941aefb58073784616acf7ca7458b213bee) )
MEGADRIVE_ROM_LOAD( storytho7, "story of thor, the (spain).bin",                                                         0x000000, 0x300000,  CRC(4631f941) SHA1(0fcc02355176e1c96043f4d827a3ff88d2d272df) )
MEGADRIVE_ROM_LOAD( storytho8, "story of thor, the - hikari o tsugumono (japan).bin",                                    0x000000, 0x300000,  CRC(4f39783c) SHA1(54296f5cf1917c568bb29b0086641c282b8884bd) )
MEGADRIVE_ROM_LOAD( sf2, "street fighter ii' - special champion edition (europe).bin",                                   0x000000, 0x300000,  CRC(56d41136) SHA1(2a406e2e4743de98785c85322f858abfb8221ae0) )
MEGADRIVE_ROM_LOAD( sf2u, "street fighter ii' - special champion edition (usa).bin",                                     0x000000, 0x300000,  CRC(13fe08a1) SHA1(a5aad1d108046d9388e33247610dafb4c6516e0b) )
MEGADRIVE_ROM_LOAD( sf2p, "street fighter ii' plus (japan, asia, korea).bin",                                            0x000000, 0x300000,  CRC(2e487ee3) SHA1(0d624f1a34014ead022dd8d5df1134a88eca69bb) )
MEGADRIVE_ROM_LOAD( sf2tb, "street fighter ii' turbo (beta).bin",                                                        0x000000, 0x200000,  CRC(a85491ae) SHA1(23e1e1b587a7d2d1a82599d82d01c9931ca7b4cf) )
MEGADRIVE_ROM_LOAD( sracer, "street racer (europe).bin",                                                                 0x000000, 0x100000,  CRC(1a58d5fe) SHA1(95aa250ea47d14d60da9f0fed5b1aad1ff2c1862) )
MEGADRIVE_ROM_LOAD( ssmart, "street smart (japan, usa).bin",                                                             0x000000, 0x80000,   CRC(b1dedfad) SHA1(a9cb2295b9cd42475904cba19b983e075a1b6014) )
MEGADRIVE_ROM_LOAD( sor, "streets of rage / bare knuckle - ikari no tetsuken (world) (rev a).bin",                       0x000000, 0x80000,   CRC(4052e845) SHA1(731cdf182fe647e4977477ba4dd2e2b46b9b878a) )
MEGADRIVE_ROM_LOAD( sora, "streets of rage / bare knuckle - ikari no tetsuken (world).bin",                              0x000000, 0x80000,   CRC(bff227c6) SHA1(3d74dbc81f3472a5bde45bf265e636a72a314667) )
MEGADRIVE_ROM_LOAD( sor2, "streets of rage 2 (usa).bin",                                                                 0x000000, 0x200000,  CRC(e01fa526) SHA1(8b656eec9692d88bbbb84787142aa732b44ce0be) )
MEGADRIVE_ROM_LOAD( bk2b, "bare knuckle ii (japan) (beta).bin",                                                          0x000000, 0x130000,  CRC(0cf2acbe) SHA1(ee3bee676944bc5a3cab163c209825f678dc8204) )
MEGADRIVE_ROM_LOAD( bk2, "bare knuckle ii - shitou heno chingonka / streets of rage ii (japan, europe).bin",             0x000000, 0x200000,  CRC(42e3efdc) SHA1(a0d3a216278aef5564dcbed83df0dd59222812c8) )
MEGADRIVE_ROM_LOAD( sor3k, "streets of rage 3 (korea).bin",                                                              0x000000, 0x300000,  CRC(90ef991e) SHA1(8c0bc5b66703efbdeef2d6ede5745aad89bd8a44) )
MEGADRIVE_ROM_LOAD( sor3pa, "streets of rage 3 (e) (prototype - apr 12, 1994).bin",                                      0x000000, 0x300000,  CRC(a17ce5ab) SHA1(0b0fa81973bf2f0313e0f2041c8cdd1c220e9839) )
MEGADRIVE_ROM_LOAD( sor3pb, "streets of rage 3 (e) (prototype - apr 13, 1994).bin",                                      0x000000, 0x300000,  CRC(164e42ae) SHA1(32df74ccf217e8258dee9acc50db540bbc68ca02) )
MEGADRIVE_ROM_LOAD( sor3pc, "streets of rage 3 (e) (prototype - apr 15, 1994).bin",                                      0x000000, 0x300000,  CRC(c64f1e6b) SHA1(118e4a8a3891956e0b60bcf5a6dd631ba93794a3) )
MEGADRIVE_ROM_LOAD( sor3pd, "streets of rage 3 (e) (prototype - apr 20, 1994).bin",                                      0x000000, 0x300000,  CRC(6ae4bd8e) SHA1(58feae0110f239ba7a58e447b86158705249b35b) )
MEGADRIVE_ROM_LOAD( sor3pe, "streets of rage 3 (e) (prototype - apr 25, 1994).bin",                                      0x000000, 0x300000,  CRC(7033878a) SHA1(8a8cfc1e75f89cf72d7d783b29238f4a8b3ff568) )
MEGADRIVE_ROM_LOAD( sor3, "streets of rage 3 (europe).bin",                                                              0x000000, 0x300000,  CRC(3b78135f) SHA1(5419f5eaf12201a662f03a79298a1b661005f73a) )
MEGADRIVE_ROM_LOAD( sor3pf, "streets of rage 3 (prototype - apr 01, 1994).bin",                                          0x000000, 0x300000,  CRC(797e75b7) SHA1(9f2f64507fd11114c2bacdd54aa25aecfa4f25bb) )
MEGADRIVE_ROM_LOAD( sor3pg, "streets of rage 3 (prototype - apr 04, 1994).bin",                                          0x000000, 0x300000,  CRC(6b968f13) SHA1(e2645121252f51d1eb5ea4ffc9af0092941ad7c9) )
MEGADRIVE_ROM_LOAD( sor3ph, "streets of rage 3 (prototype - apr 08, 1994).bin",                                          0x000000, 0x300000,  CRC(d4ba76c2) SHA1(94cc6697534cf90a369b5c7dfb4e53f1de96df1c) )
MEGADRIVE_ROM_LOAD( sor3pi, "streets of rage 3 (prototype - apr 11, 1994).bin",                                          0x000000, 0x300000,  CRC(fa5e5a82) SHA1(5592e80501219dc2b5fe5f07819948520b8fa051) )
MEGADRIVE_ROM_LOAD( sor3pj, "streets of rage 3 (prototype - apr 12, 1994).bin",                                          0x000000, 0x300000,  CRC(3f52cb72) SHA1(60e47f8393a79dd30c8ad273bc448610bef602f8) )
MEGADRIVE_ROM_LOAD( sor3pk, "streets of rage 3 (prototype - apr 13, 1994).bin",                                          0x000000, 0x300000,  CRC(6b675807) SHA1(ffe3b23352b15f62f00653631bc3a606b7cda4b0) )
MEGADRIVE_ROM_LOAD( sor3pl, "streets of rage 3 (prototype - mar 08, 1994).bin",                                          0x000000, 0x300000,  CRC(ea50b551) SHA1(76f4d071e3f95a740a21c14d68a732aac4700739) )
MEGADRIVE_ROM_LOAD( sor3pm, "streets of rage 3 (prototype - mar 17, 1994).bin",                                          0x000000, 0x300000,  CRC(39ad962b) SHA1(2764b00b7257e41928c6b7387606104990f4505a) )
MEGADRIVE_ROM_LOAD( sor3pn, "streets of rage 3 (prototype - mar 18, 1994).bin",                                          0x000000, 0x300000,  CRC(60142484) SHA1(9a97bedde6ccb06e9d56a9f86308ba97502994f6) )
MEGADRIVE_ROM_LOAD( sor3po, "streets of rage 3 (prototype - mar 28, 1994).bin",                                          0x000000, 0x300000,  CRC(8757f797) SHA1(0b5160bdb791033c51aaf924eb280f977fc387d4) )
MEGADRIVE_ROM_LOAD( sor3u, "streets of rage 3 (usa).bin",                                                                0x000000, 0x300000,  CRC(d5bb15d9) SHA1(40a33dd6f9dab0aff26c7525c9b8f342482c7af6) )
MEGADRIVE_ROM_LOAD( bk3b,  "bare knuckle iii (japan) (beta).bin",                                                        0x000000, 0x300000,  CRC(e7ff99db) SHA1(1d75372571ce970545f6ce63977cea9fa811e23f) )
MEGADRIVE_ROM_LOAD( bk3,   "bare knuckle iii (japan).bin",                                                               0x000000, 0x300000,  CRC(5d09236f) SHA1(b9b2b4a98a9d8f4c49aa1e5395e2279339517fdb) )
MEGADRIVE_ROM_LOAD( strider, "strider (usa, europe).bin",                                                                0x000000, 0x100000,  CRC(b9d099a4) SHA1(26fe42d13a01c8789bbad722ebac05b8a829eb37) )
MEGADRIVE_ROM_LOAD( striderj, "strider hiryuu (japan, korea).bin",                                                       0x000000, 0x100000,  CRC(859173f2) SHA1(4198030057a1a0479b382fc2d69cfe32a523fa32) )
MEGADRIVE_ROM_LOAD( strider2, "strider ii (europe).bin",                                                                 0x000000, 0x100000,  CRC(e85e5270) SHA1(c048bf092745654bb60a437ef1543abfd407093c) )
MEGADRIVE_ROM_LOAD( strider2u, "strider returns - journey from darkness (usa).bin",                                      0x000000, 0x100000,  CRC(42589b79) SHA1(8febb4aff32f40148f572d54e158f4b791736f55) )
MEGADRIVE_ROM_LOAD( strikerb, "striker (europe) (beta).bin",                                                             0x000000, 0x200000,  CRC(c10b270e) SHA1(128395e635e948005e89c7f4a6cd5b209be1ffbc) )
MEGADRIVE_ROM_LOAD( striker, "striker (europe).bin",                                                                     0x000000, 0x200000,  CRC(cc5d7ab2) SHA1(9917c35a263cc9bd922d55bf59d01bc2733b4e24) )
MEGADRIVE_ROM_LOAD( subtp, "subterrania (prototype - feb 02, 1994).bin",                                                 0x000000, 0x200000,  CRC(b368e394) SHA1(91609b5083fac0d8c00a25d3831eff5fb02eac5b) )
MEGADRIVE_ROM_LOAD( subt, "subterrania (europe).bin",                                                                    0x000000, 0x200000,  CRC(e8ced28d) SHA1(23c6a0616f170f6616bc8214f3d45f1f293bba9f) )
MEGADRIVE_ROM_LOAD( subtj, "subterrania (japan).bin",                                                                    0x000000, 0x200000,  CRC(7638ea91) SHA1(e4e25a7a9a583be0dd8e7aa0f1ab3e96b2180bc6) )
MEGADRIVE_ROM_LOAD( subtua, "subterrania (usa) (beta) (earlier).bin",                                                    0x000000, 0x140000,  CRC(9c13d25c) SHA1(3766bfaf355cc239187255a142daddcd42d1fc58) )
MEGADRIVE_ROM_LOAD( subtub, "subterrania (usa) (beta).bin",                                                              0x000000, 0x200000,  CRC(3a1022d1) SHA1(57279fa2bc9baf07d701be44e51e42d7f1e0e2a2) )
MEGADRIVE_ROM_LOAD( subtu, "subterrania (usa).bin",                                                                      0x000000, 0x200000,  CRC(dc3c6c45) SHA1(70a5d4da311dd8a92492d01676bf9170fa4bd095) )
MEGADRIVE_ROM_LOAD( summer, "summer challenge (usa, europe).bin",                                                        0x000000, 0x200000,  CRC(d7d53dc1) SHA1(28d18ba3d2ac3f8fece0bf652e87d491c565b8df) )
MEGADRIVE_ROM_LOAD( sunset, "sunset riders (europe).bin",                                                                0x000000, 0x80000,   CRC(0ff33054) SHA1(4a4f2cf397ade091e83e07bb3ffc7aa5862aeedd) )
MEGADRIVE_ROM_LOAD( sunsetu, "sunset riders (usa).bin",                                                                  0x000000, 0x80000,   CRC(ac30c297) SHA1(b40ea5b00f477d7b7448447f15b4c571f5e8ff0d) )
MEGADRIVE_ROM_LOAD( superair, "super airwolf (japan).bin",                                                               0x000000, 0x80000,   CRC(fa451982) SHA1(d24a3d23c9f12eebfbd233fdaab91c4acc362962) )
MEGADRIVE_ROM_LOAD( sb2020, "super baseball 2020 (usa, europe).bin",                                                     0x000000, 0x200000,  CRC(c17acee5) SHA1(a6ea6dcc33d60cf4d3be75b1cc867699811f8b3a) )
MEGADRIVE_ROM_LOAD( sbship, "super battleship (usa).bin",                                                                0x000000, 0x80000,   CRC(99ca1bfb) SHA1(fd1cdde1448f20388ccb3f66498867573f8c6fc2) )
MEGADRIVE_ROM_LOAD( sbtank, "super battletank - war in the gulf (usa).bin",                                              0x000000, 0x80000,   CRC(b0b5e3c9) SHA1(4ce9aaaa9d3f98e1747af12ad488b6bdbde1afb4) )
MEGADRIVE_ROM_LOAD( supbub, "super bubble bobble (china) (unlicensed).bin",                                              0x000000, 0x100000,  CRC(4820a161) SHA1(03f40c14624f1522d6e3105997d14e8eaba12257) )
MEGADRIVE_ROM_LOAD( superd, "super daisenryaku (japan).bin",                                                             0x000000, 0x80000,   CRC(d50a166f) SHA1(56d9366b50cea65b16ed621b9a5bf355ef89e6b5) )
MEGADRIVE_ROM_LOAD( sdkong99a, "super donkey kong 99 (unlicensed) [!].bin",                                              0x000000, 0x200000,  CRC(8e7d9177) SHA1(91f6b10ada917e6dfdafdd5ad9d476723498a7a4) )
MEGADRIVE_ROM_LOAD( sfzone, "super fantasy zone (europe).bin",                                                           0x000000, 0x100000,  CRC(927975be) SHA1(ed4f8a98eed7838d29fa31a6f34d11f6d8887c7f) )
MEGADRIVE_ROM_LOAD( sfzonej, "super fantasy zone (japan).bin",                                                           0x000000, 0x100000,  CRC(767780d7) SHA1(14dc8568205f3b1b89ce14b9412541ce4ae47f91) )
MEGADRIVE_ROM_LOAD( superhq, "super h.q. (japan).bin",                                                                   0x000000, 0x80000,   CRC(ab2c52b0) SHA1(0598e452e3d42b5e714bf5b32834000607483813) )
MEGADRIVE_ROM_LOAD( shangon, "super hang-on (world) (en,ja) (rev a).bin",                                                0x000000, 0x80000,   CRC(3877d107) SHA1(e58a8e6c472a34d9ecf3b450137df8a63ec9c791) )
MEGADRIVE_ROM_LOAD( shangona, "super hang-on (world) (en,ja).bin",                                                       0x000000, 0x80000,   CRC(cb2201a3) SHA1(ecfd7b3bf4dcbee472ddf2f9cdbe968a05b814e0) )
MEGADRIVE_ROM_LOAD( shi, "super high impact (usa).bin",                                                                  0x000000, 0x100000,  CRC(b870c2f7) SHA1(a5ce32a8e96d39424d4377c45befa6fd8389b941) )
MEGADRIVE_ROM_LOAD( suphy, "super hydlide (europe).bin",                                                                 0x000000, 0x80000,   CRC(1fe2d90b) SHA1(6c0a4b72b90ecfe8c324691bc6e54243746043c1) )
MEGADRIVE_ROM_LOAD( suphyj, "super hydlide (japan).bin",                                                                 0x000000, 0x80000,   CRC(599be386) SHA1(76bceb5915f3546c68c1fdb5fb0e205cd56a3fe6) )
MEGADRIVE_ROM_LOAD( suphyu, "super hydlide (usa).bin",                                                                   0x000000, 0x80000,   CRC(1335ddaa) SHA1(5ff65139c7e10539dd5a12bdf56073504c998471) )
MEGADRIVE_ROM_LOAD( suprkick, "super kick off (europe).bin",                                                             0x000000, 0x80000,   CRC(f43793ff) SHA1(4e270b13a399d78d919157e50ab11f4645aa6d32) )
MEGADRIVE_ROM_LOAD( suplg, "super league (europe).bin",                                                                  0x000000, 0x80000,   CRC(55baec6e) SHA1(fefc7abc2f9fbcc7992b1420b62eb3eb3d5ad1bb) )
MEGADRIVE_ROM_LOAD( suplgj, "super league (japan).bin",                                                                  0x000000, 0x80000,   CRC(ea13cb1d) SHA1(9e975ba4c396617ff9535d51d0929f6d592478fc) )
MEGADRIVE_ROM_LOAD( smario2, "super mario 2 1998 (unlicensed) [!].bin",                                                  0x000000, 0x200000,  CRC(f7e1b3e1) SHA1(de10115ce6a7eb416de9cd167df9cf24e34687b1) )
MEGADRIVE_ROM_LOAD( smb, "super mario bros. (unlicensed) [!].bin",                                                       0x000000, 0x200000,  CRC(9cfa2bd8) SHA1(5011e16f0ab3a6487a1406b85c6090ad2d1fe345) )
MEGADRIVE_ROM_LOAD( smw, "super mario world (unlicensed) [!].bin",                                                       0x000000, 0x200000,  CRC(cf540ba6) SHA1(517c3a6b091c2c4e8505112a84bae2871243e92c) )
MEGADRIVE_ROM_LOAD( smwa, "super mario world (unlicensed) (pirate).bin",                                                 0x000000, 0x200000,  CRC(97c2695e) SHA1(ff6661d39b2bad26f460e16106ca369421388596) )
MEGADRIVE_ROM_LOAD( smgpj, "super monaco gp (japan) (en,ja).bin",                                                        0x000000, 0x80000,   CRC(90f9bab3) SHA1(631b72e27b394ae6b5a1188dfa980333fc675379) )
MEGADRIVE_ROM_LOAD( smgpa, "super monaco gp (japan, europe) (en,ja) (rev a).bin",                                        0x000000, 0x80000,   CRC(be91b28a) SHA1(1e49a449367f0ec7ba0331b7b0d074f796e48d58) )
MEGADRIVE_ROM_LOAD( smgpb, "super monaco gp (japan, europe) (en,ja).bin",                                                0x000000, 0x80000,   CRC(b1823595) SHA1(ed6f80546a7847bf06cf4a62b34d1c3b989e4d3e) )
MEGADRIVE_ROM_LOAD( smgp, "super monaco gp (usa) (en,ja).bin",                                                           0x000000, 0x80000,   CRC(725018ee) SHA1(1947d41598daa3880ecb826303abae2accd1857f) )
MEGADRIVE_ROM_LOAD( superoff, "super off road (usa, europe).bin",                                                        0x000000, 0x80000,   CRC(8f2fdada) SHA1(89f264afba7aa8764b301d46cc8c51f74c23919e) )
MEGADRIVE_ROM_LOAD( srb, "super real basketball (europe).bin",                                                           0x000000, 0x80000,   CRC(f04765ba) SHA1(0197df59951085dc7078c4ec66c75be84566530a) )
MEGADRIVE_ROM_LOAD( srbj, "mpr-12904.bin",                                                                               0x000000, 0x80000,   CRC(4346e11a) SHA1(c86725780027ef9783cb7884c8770cc030b0cd0d) )
MEGADRIVE_ROM_LOAD( sups2a, "super shinobi ii, the (japan) (beta) (earlier).bin",                                        0x000000, 0x100000,  CRC(1ee5bce3) SHA1(182f65e634b6eecdadff55f41cd9b2d0e9e283f6) )
MEGADRIVE_ROM_LOAD( sups2b, "super shinobi ii, the (japan) (beta).bin",                                                  0x000000, 0x100000,  CRC(c47e8aea) SHA1(e1eca5faec785f4e1093b088421a818e2fab29c1) )
MEGADRIVE_ROM_LOAD( sups2, "super shinobi ii, the (japan, korea).bin",                                                   0x000000, 0x100000,  CRC(5b412816) SHA1(e3dbe326aa93320d405c561c306fc3954ab8ea7c) )
MEGADRIVE_ROM_LOAD( supshi, "super shinobi, the (japan).bin",                                                            0x000000, 0x80000,   CRC(5c7e5ea6) SHA1(d5807a44d2059aa4ff27ecb7bdc749fbb0382550) )
MEGADRIVE_ROM_LOAD( sskidb, "super skidmarks (europe) (beta).bin",                                                       0x000000, 0x200000,  CRC(a61a0f0c) SHA1(1ec5fb1e7c8adfaf147271bd40454826fffed13e) )
MEGADRIVE_ROM_LOAD( sskid, "super skidmarks (europe) (j-cart).bin",                                                      0x000000, 0x200000,  CRC(4a9c62f9) SHA1(957850ce688dcf9c5966f6712b4804f0a5b7457d) )
MEGADRIVE_ROM_LOAD( ssmashtv, "super smash tv (usa, europe).bin",                                                        0x000000, 0x80000,   CRC(f22412b6) SHA1(0459f7c61f152fa0afa98d96ef9fbe4964641f34) )
MEGADRIVE_ROM_LOAD( stbladj, "super thunder blade (japan) (launch cart).bin",                                            0x000000, 0x80000,   CRC(8bd77836) SHA1(f40bef7a5a2e414d87335e3e56a2c34fb4f83fca) )
MEGADRIVE_ROM_LOAD( stblad, "mpr-12354.bin",                                                                             0x000000, 0x80000,   CRC(b13087ee) SHA1(21810b4a309a5b9a70965dd440e9aeed0b6ca4c5) )
MEGADRIVE_ROM_LOAD( svbj, "super volley ball (japan).bin",                                                               0x000000, 0x40000,   CRC(9b5c28ea) SHA1(d0a57b14dc08fad001e96de72a4111f6c9b22825) )
MEGADRIVE_ROM_LOAD( svb1, "super volley ball (usa) (alt).bin",                                                           0x000000, 0x40000,   CRC(85102799) SHA1(3f00e7f8de50e73346cba06dd44b6dd9461d5c9c) )
MEGADRIVE_ROM_LOAD( svb, "super volley ball (usa).bin",                                                                  0x000000, 0x40000,   CRC(a88fee44) SHA1(4fbf379fc806453c763980a3e6ee7f858f32ee3e) )
MEGADRIVE_ROM_LOAD( supmanb, "superman (usa) (beta).bin",                                                                0x000000, 0x100000,  CRC(5cd0e1d4) SHA1(9d94f0e364b5170ac1f1f2098091c3fc89d07389) )
MEGADRIVE_ROM_LOAD( supmanu, "superman (usa).bin",                                                                       0x000000, 0x100000,  CRC(543a5869) SHA1(ca14653589fd36e6394ec99f223ed9b18f70fd6a) )
MEGADRIVE_ROM_LOAD( supman, "superman - the man of steel (europe).bin",                                                  0x000000, 0x100000,  CRC(7db434ba) SHA1(c956730af44b737ee3d0c1e83c147f32e3504383) )
MEGADRIVE_ROM_LOAD( surgin, "surging aura (japan).bin",                                                                  0x000000, 0x200000,  CRC(65ac1d2b) SHA1(9f14cd11cfa499cdd58248de81db30f9308326e5) )
MEGADRIVE_ROM_LOAD( swordsodj, "sword of sodan (japan).bin",                                                             0x000000, 0x80000,   CRC(58edb3f3) SHA1(9c17187e3eb0842c5300f69616a58baa86b769e7) )
MEGADRIVE_ROM_LOAD( swordsod, "sword of sodan (usa, europe).bin",                                                        0x000000, 0x80000,   CRC(9cb8468f) SHA1(088b81c3bcda86b9803b7e3f8067beb21d1d2553) )
MEGADRIVE_ROM_LOAD( swordver, "sword of vermilion (usa, europe).bin",                                                    0x000000, 0xa0000,   CRC(ea1bc9ab) SHA1(c18fc75e0c5fa0e98c8664903e978ec4f73ef5d2) )
MEGADRIVE_ROM_LOAD( sydofv, "syd of valis (usa).bin",                                                                    0x000000, 0x80000,   CRC(37dc0108) SHA1(36e010f16791816108d395fce39b39ab0a49268c) )
MEGADRIVE_ROM_LOAD( sylves, "sylvester & tweety in cagey capers (europe).bin",                                           0x000000, 0x200000,  CRC(89fc54ce) SHA1(bf50d0afe82966907671a46060d27b7b5d92a752) )
MEGADRIVE_ROM_LOAD( sylvesu, "sylvester and tweety in cagey capers (usa).bin",                                           0x000000, 0x200000,  CRC(9d9c786b) SHA1(d1ca731869b2ffd452bd4f2b40bb0afdc997936d) )
MEGADRIVE_ROM_LOAD( syndic, "syndicate (usa, europe).bin",                                                               0x000000, 0x200000,  CRC(95bbf87b) SHA1(87442ecc50df508d54d241cbd468b41c926b974d) )
MEGADRIVE_ROM_LOAD( t2term, "t2 - terminator 2 - judgment day (usa, europe).bin",                                        0x000000, 0x100000,  CRC(2f75e896) SHA1(6144fbb941c1bf0df285f6d13906432c23af2ba6) )
MEGADRIVE_ROM_LOAD( t2agj, "t2 - the arcade game (japan).bin",                                                           0x000000, 0x100000,  CRC(5e6fe52c) SHA1(64d0778d38b5eb0664c1b85e0689de637a416654) )
MEGADRIVE_ROM_LOAD( t2agb, "t2 - the arcade game (usa) (beta).bin",                                                      0x000000, 0x100000,  CRC(94255703) SHA1(87660389d70d73e5f0f68d672a0843712f7c4c85) )
MEGADRIVE_ROM_LOAD( t2ag, "t2 - the arcade game (usa, europe).bin",                                                      0x000000, 0x100000,  CRC(a1264f17) SHA1(85cc1cf3379d3ce23ca3c03d84fe6e2b3adc9c56) )
MEGADRIVE_ROM_LOAD( taikou, "taikou risshiden (japan).bin",                                                              0x000000, 0x140000,  CRC(f96fe15b) SHA1(a96d6492d4e89687d970bc010eb0b93ee2481a44) )
MEGADRIVE_ROM_LOAD( taiwan, "taiwan daheng (china) (unlicensed).bin",                                                    0x000000, 0x100000,  CRC(baf20f81) SHA1(88726c11e5ed7927830bf5ae0b83d85dfff4a2a4) )
MEGADRIVE_ROM_LOAD( talespin, "talespin (usa, europe).bin",                                                              0x000000, 0x80000,   CRC(f5c0c8d0) SHA1(9b6ab86fea23adb3cfba38b893278d856540c8b8) )
MEGADRIVE_ROM_LOAD( talmit, "talmit's adventure (europe).bin",                                                           0x000000, 0x100000,  CRC(05dc3ffc) SHA1(be80f96bee64bab159614d29f882442abef9de76) )
MEGADRIVE_ROM_LOAD( target, "target earth (usa).bin",                                                                    0x000000, 0x80000,   CRC(cddf62d3) SHA1(e19f7a02f140882e0364f11cd096aec712e56f83) )
MEGADRIVE_ROM_LOAD( tfhj, "task force harrier ex (japan).bin",                                                           0x000000, 0x100000,  CRC(e9a54eed) SHA1(d365d22d9076e98966eb12fdb1d93c3c101f519e) )
MEGADRIVE_ROM_LOAD( tfh, "task force harrier ex (usa).bin",                                                              0x000000, 0x100000,  CRC(c8bb0257) SHA1(1d36bd69e356f276c582fee247af2f71af1f3bf4) )
MEGADRIVE_ROM_LOAD( tatsujin, "tatsujin / truxton (world).bin",                                                          0x000000, 0x80000,   CRC(5bd0882d) SHA1(90039844478e7cb99951fdff1979c3bda04d080a) )
MEGADRIVE_ROM_LOAD( tazmania, "taz-mania (world).bin",                                                                   0x000000, 0x80000,   CRC(0e901f45) SHA1(01875bb6484d44a844f3d3e1ae141864664b73b8) )
MEGADRIVE_ROM_LOAD( teamusa, "team usa basketball (usa, europe).bin",                                                    0x000000, 0x100000,  CRC(a0caf97e) SHA1(8d8d833dfc88663408bd7cf9fb821608ad2bef3d) )
MEGADRIVE_ROM_LOAD( technocl, "techno clash (usa, europe).bin",                                                          0x000000, 0x100000,  CRC(4e65e483) SHA1(686afbd7130fe8487d9126e690bf53800ae953ba) )
MEGADRIVE_ROM_LOAD( tcop, "technocop (usa).bin",                                                                         0x000000, 0x80000,   CRC(7459ad06) SHA1(739421dd98d8073030d43cf8f50e411acb82f7d6) )
MEGADRIVE_ROM_LOAD( tecmoc, "tecmo cup (j) (pirate).bin",                                                                0x000000, 0x80000,   CRC(e889e218) SHA1(e78f42104c66f49e0492e65c64354baac599369e) )
MEGADRIVE_ROM_LOAD( tecmoc1, "tecmo cup (japan) (prototype) (bad dump).bin",                                             0x000000, 0x80000,  BAD_DUMP CRC(88fdd060) SHA1(67e9261097e1ed9c8d7d11fd45a7c7e04ac1145f) )
MEGADRIVE_ROM_LOAD( tecmos, "tecmo super baseball (usa).bin",                                                            0x000000, 0x100000,  CRC(227a1178) SHA1(f2ecc7b32cef29f22eb4a21a22be122bd4bad212) )
MEGADRIVE_ROM_LOAD( tsbj, "tecmo super bowl (japan).bin",                                                                0x000000, 0x100000,  CRC(90c6e20c) SHA1(477880e7976ac0f7203fddabba4a6e8799aa604d) )
MEGADRIVE_ROM_LOAD( tsb, "tecmo super bowl (usa) (october 1993).bin",                                                    0x000000, 0x100000,  CRC(21f27d34) SHA1(8d34ffac312caeac853876415c74ab6fe63d8dc2) )
MEGADRIVE_ROM_LOAD( tsbo, "tecmo super bowl (usa) (september 1993).bin",                                                 0x000000, 0x100000,  CRC(bd5933ee) SHA1(529b8e86b97c326592540f5e427198a205c127d0) )
MEGADRIVE_ROM_LOAD( tsb2j, "tecmo super bowl ii - special edition (japan).bin",                                          0x000000, 0x200000,  CRC(32fb633d) SHA1(cb9e8cc1651b719054f05e1e1a9e0fbbc3876ebd) )
MEGADRIVE_ROM_LOAD( tsb2, "tecmo super bowl ii - special edition (usa).bin",                                             0x000000, 0x200000,  CRC(0a0e67d8) SHA1(5fadb2a0e780ec868671b0e888fad5d7c203f59f) )
MEGADRIVE_ROM_LOAD( tsb3, "tecmo super bowl iii - final edition (usa).bin",                                              0x000000, 0x200000,  CRC(aae4089f) SHA1(d5d1609cdf72d98f5e5daa47a9585ae7ca87a410) )
MEGADRIVE_ROM_LOAD( tsh, "tecmo super hockey (usa).bin",                                                                 0x000000, 0x100000,  CRC(5f86ddc9) SHA1(4d4fd22d2fafd7e56790029be9b02e61995df11c) )
MEGADRIVE_ROM_LOAD( tsbbj, "tecmo super nba basketball (japan).bin",                                                     0x000000, 0x100000,  CRC(79f33eb6) SHA1(07f160e6eb7e358f54e4fdabfc95bb5525c57fc9) )
MEGADRIVE_ROM_LOAD( tsbb, "tecmo super nba basketball (usa).bin",                                                        0x000000, 0x100000,  CRC(53913991) SHA1(ac7aa724d6464fbd8e3144a49f3821ff6e42f67a) )
MEGADRIVE_ROM_LOAD( tecmow92, "tecmo world cup '92 (japan).bin",                                                         0x000000, 0x40000,   CRC(5e93c8b0) SHA1(ebab8f8b4f25aae44900d266c674621db2a831d9) )
MEGADRIVE_ROM_LOAD( tecmow, "tecmo world cup (usa).bin",                                                                 0x000000, 0x40000,   CRC(caf8eb2c) SHA1(53e47c40ac550c334147482610b3465fe9f6a535) )
MEGADRIVE_ROM_LOAD( teddyboy, "teddy boy blues (japan) (seganet).bin",                                                   0x000000, 0x40000,   CRC(733d2eb3) SHA1(dc858342be31ab9491acfaebf1524ece2c6ef9a3) )
MEGADRIVE_ROM_LOAD( tmhsh, "teenage mutant hero turtles - the hyperstone heist (europe).bin",                            0x000000, 0x100000,  CRC(966d5286) SHA1(ed6c32cae0813cbcf590fad715fa045fbeab6d78) )
MEGADRIVE_ROM_LOAD( tmtf, "teenage mutant hero turtles - tournament fighters (europe).bin",                              0x000000, 0x200000,  CRC(3cd2b7e6) SHA1(6a6c4ae9d944ad1d459d46ae40d3af09e60b5d7d) )
MEGADRIVE_ROM_LOAD( tmhshj, "teenage mutant ninja turtles - return of the shredder (japan).bin",                         0x000000, 0x100000,  CRC(1b003498) SHA1(f64556be092a13de8eaacc78dd630ac9d7bb75ee) )
MEGADRIVE_ROM_LOAD( tmhshu, "teenage mutant ninja turtles - the hyperstone heist (usa).bin",                             0x000000, 0x100000,  CRC(679c41de) SHA1(f440dfa689f65e782a150c1686ab90d7e5cc6355) )
MEGADRIVE_ROM_LOAD( tmtfj, "teenage mutant ninja turtles - tournament fighters (japan).bin",                             0x000000, 0x200000,  CRC(8843f2c9) SHA1(5be86d03abdb49b824104e9bbf0ac80023e4908c) )
MEGADRIVE_ROM_LOAD( tmtfu, "teenage mutant ninja turtles - tournament fighters (usa).bin",                               0x000000, 0x200000,  CRC(95b5484d) SHA1(1a27be1e7f8f47eb539b874eaa48586fe2dab9c0) )
MEGADRIVE_ROM_LOAD( teitoku, "teitoku no ketsudan (japan).bin",                                                          0x000000, 0x100000,  CRC(9b08e4e4) SHA1(adc4c03636dbd8ca5449a8a66c6a7b7ef281893a) )
MEGADRIVE_ROM_LOAD( tekk3sp, "tekken 3 special (unlicensed).bin",                                                        0x000000, 0x200000,  CRC(7fcae658) SHA1(99349bfe7966d65a4e782615aad9aa688905ad41) )
MEGADRIVE_ROM_LOAD( telmah, "tel-tel mahjong (japan).bin",                                                               0x000000, 0x40000,   CRC(44817e92) SHA1(8edaec4944c4b9d876601ee5f8921247a5ffe057) )
MEGADRIVE_ROM_LOAD( telsta, "tel-tel stadium (japan).bin",                                                               0x000000, 0x80000,   CRC(54cf8c29) SHA1(2924ed0b4266edddbb981f97acb93bbdf90494e6) )
MEGADRIVE_ROM_LOAD( telebr, "telebradesco residencia (brazil).bin",                                                      0x000000, 0x40000,   CRC(1db99045) SHA1(917c0f72e21c98ee2ee367c4d42c992c886a3aa2) )
MEGADRIVE_ROM_LOAD( termintr, "terminator, the (europe).bin",                                                            0x000000, 0x100000,  CRC(15f4d302) SHA1(c8ee275f2e30aaf6ad713c6cd915a4ede65328e0) )
MEGADRIVE_ROM_LOAD( termintru, "terminator, the (usa).bin",                                                              0x000000, 0x100000,  CRC(31a629be) SHA1(2a1894e7f40b9001961f7bf1c70672351aa525f9) )
MEGADRIVE_ROM_LOAD( testdrv2, "test drive ii - the duel (usa, europe).bin",                                              0x000000, 0x100000,  CRC(f9bdf8c5) SHA1(cb70e5de149521f20723413cd11c5e661ec63c3e) )
MEGADRIVE_ROM_LOAD( tetris, "tetris (japan).bin",                                                                        0x000000, 0x40000,   CRC(4ce90db0) SHA1(2f2b559c5855e34500e43fb5cc8aff04dd72eb56) )
MEGADRIVE_ROM_LOAD( kof99, "the king of fighters 99 (unlicensed).bin",                                                   0x000000, 0x300000,  CRC(54638c11) SHA1(cdef3008dec2ce1a214af8b9cb000053671a3c36) )
MEGADRIVE_ROM_LOAD( themeprk, "theme park (usa, europe).bin",                                                            0x000000, 0x200000,  CRC(289da2c5) SHA1(03c6504b5d797f10c7c361735d801902e4b00981) )
MEGADRIVE_ROM_LOAD( thomas, "thomas the tank engine & friends (usa).bin",                                                0x000000, 0x100000,  CRC(1a406299) SHA1(e714e9faa9c1687a2dfcb0ada22a75c7a4ee01a6) )
MEGADRIVE_ROM_LOAD( tfii, "thunder force ii (usa, europe).bin",                                                          0x000000, 0x80000,   CRC(9b1561b3) SHA1(b81e7ebc4ceb6c1ae2975d27e0a78ba1e8546b5f) )
MEGADRIVE_ROM_LOAD( tfiij, "thunder force ii md (japan).bin",                                                            0x000000, 0x80000,   CRC(e75ec3e0) SHA1(44b173b74225e5b562cdf3982926a051f05ed98e) )
MEGADRIVE_ROM_LOAD( tfiii, "thunder force iii (japan, usa).bin",                                                         0x000000, 0x80000,   CRC(1b3f399a) SHA1(9eadee76eb0509d5a0f16372fc9eac7a883e5f2d) )
MEGADRIVE_ROM_LOAD( tfiv, "thunder force iv (europe).bin",                                                               0x000000, 0x100000,  CRC(e7e3c05b) SHA1(ecbc2bfc4f3d8bbd46b398274ed2f5cc3db68454) )
MEGADRIVE_ROM_LOAD( tfivj, "thunder force iv (japan).bin",                                                               0x000000, 0x100000,  CRC(8d606480) SHA1(44be21dead3b55f21762a7c5cf640eec0a1769d9) )
MEGADRIVE_ROM_LOAD( tfoxj, "thunder fox (japan).bin",                                                                    0x000000, 0x100000,  CRC(eca6cffa) SHA1(4e6dc58327bdbb5d0d885b3cecf89fe139b32532) )
MEGADRIVE_ROM_LOAD( tfox, "thunder fox (usa).bin",                                                                       0x000000, 0x100000,  CRC(5463f50f) SHA1(c1699ccabb89c2877dd616471e80d175434bffe3) )
MEGADRIVE_ROM_LOAD( tpwres, "thunder pro wrestling retsuden (japan).bin",                                                0x000000, 0x80000,   CRC(24408c73) SHA1(7f41afbf62d83424067f872a5dfe2c0f0ec40052) )
MEGADRIVE_ROM_LOAD( tick, "tick, the (usa).bin",                                                                         0x000000, 0x200000,  CRC(425132f0) SHA1(e0fe77f1d512a753938ce4c5c7c0badb5edfc407) )
MEGADRIVE_ROM_LOAD( timedom1, "time dominator 1st (japan, korea).bin",                                                   0x000000, 0x100000,  CRC(7eba7a5c) SHA1(c9358fb57314be5792af5e97748f7b886a7194d2) )
MEGADRIVE_ROM_LOAD( timekill, "time killers (europe).bin",                                                               0x000000, 0x200000,  CRC(a4f48a1a) SHA1(5bc883edd092602aac162b42462442e462d3c881) )
MEGADRIVE_ROM_LOAD( timekillu, "time killers (usa).bin",                                                                 0x000000, 0x200000,  CRC(4b5f52ac) SHA1(91b2dd5463261ca240c4977f58d9c8fd7e770624) )
MEGADRIVE_ROM_LOAD( tinhead, "tinhead (usa).bin",                                                                        0x000000, 0x100000,  CRC(d6724b84) SHA1(e5acf758e76c95017a6ad50ab0f6ae2db5c9e8bc) )
MEGADRIVE_ROM_LOAD( tintin, "tintin au tibet (europe) (en,fr,de,es,nl,sv).bin",                                          0x000000, 0x200000,  CRC(4243caf3) SHA1(54fb11f601be37418b5bba3e0762d8b87068177a) )
MEGADRIVE_ROM_LOAD( tinyk, "tiny toon adventures (korea).bin",                                                           0x000000, 0x80000,   CRC(4ca3a8fb) SHA1(365d190088d78813f65610ff2b5b50c0e4060e24) )
MEGADRIVE_ROM_LOAD( ttacme, "tiny toon adventures - acme all-stars (europe).bin",                                        0x000000, 0x100000,  CRC(1227b2b2) SHA1(2672018d9e005a9a3b5006fa8f61e08f2d1909aa) )
MEGADRIVE_ROM_LOAD( ttacmeu, "tiny toon adventures - acme all-stars (usa, korea).bin",                                   0x000000, 0x100000,  CRC(2f9faa1d) SHA1(d64736a69fca430fc6a84a60335add0c765feb71) )
MEGADRIVE_ROM_LOAD( tiny, "tiny toon adventures - buster's hidden treasure (europe).bin",                                0x000000, 0x80000,   CRC(d10fba51) SHA1(9e63b150cc2ec0ef141e68ffda862aa8db604441) )
MEGADRIVE_ROM_LOAD( tinyu, "tiny toon adventures - buster's hidden treasure (usa).bin",                                  0x000000, 0x80000,   CRC(a26d3ae0) SHA1(d8d159c7c5a365242f989cc3aad2352fb27e3af3) )
MEGADRIVE_ROM_LOAD( tnnbass, "tnn bass tournament of champions (usa).bin",                                               0x000000, 0x100000,  CRC(c83ffa1b) SHA1(f76acb6d5da07377685d42daf1ce4ca53be5d6b9) )
MEGADRIVE_ROM_LOAD( tnnout, "tnn outdoors bass tournament '96 (usa).bin",                                                0x000000, 0x200000,  CRC(5c523c0b) SHA1(0ca72f28e88675066c466246977143599240b09f) )
MEGADRIVE_ROM_LOAD( toddslim, "todd's adventures in slime world (usa).bin",                                              0x000000, 0x80000,   CRC(652e8b7d) SHA1(e558e39e3e556d20c789bf2823af64e1a5c78784) )
MEGADRIVE_ROM_LOAD( toejam, "toe jam & earl (world) (rev a).bin",                                                        0x000000, 0x100000,  CRC(7a588f4b) SHA1(85e8d0a4fac591b25b77c35680ac4175976f251b) )
MEGADRIVE_ROM_LOAD( toejam1, "toe jam & earl (world).bin",                                                               0x000000, 0x100000,  CRC(d1b36786) SHA1(7f82d8b57fff88bdca5d8aff85b01e231dc1239a) )
MEGADRIVE_ROM_LOAD( tje2g, "toe jam & earl in panic auf funkotron (germany).bin",                                        0x000000, 0x200000,  CRC(4081b9f2) SHA1(9d4e4b358147ab913c3fcff2811558eac7b8b466) )
MEGADRIVE_ROM_LOAD( tje2, "toe jam & earl in panic on funkotron (europe).bin",                                           0x000000, 0x200000,  CRC(47b0a871) SHA1(ac05701d86ba8957adb8fe0b67d8b4bb51328d98) )
MEGADRIVE_ROM_LOAD( tje2j, "toe jam & earl in panic on funkotron (japan).bin",                                           0x000000, 0x200000,  CRC(e1b36850) SHA1(141af8fd1e5b4ba5118a384771b0d75f40af312f) )
MEGADRIVE_ROM_LOAD( tje2u, "toe jam & earl in panic on funkotron (usa).bin",                                             0x000000, 0x200000,  CRC(aa021bdd) SHA1(0ea0da09183eb01d030515beeb40c1427c6e1f07) )
MEGADRIVE_ROM_LOAD( tomjerry, "tom and jerry - frantic antics (usa) (1993).bin",                                         0x000000, 0x100000,  CRC(b9992e1c) SHA1(a785c642bdcce284e6a607bb68e3993a176f4361) )
MEGADRIVE_ROM_LOAD( tomjerry1, "tom and jerry - frantic antics (usa) (1994).bin",                                        0x000000, 0x100000,  CRC(3044460c) SHA1(09dd23ab18dcaf4e21754992a898504188bd76f2) )
MEGADRIVE_ROM_LOAD( tommylb, "tommy lasorda baseball (usa).bin",                                                         0x000000, 0x80000,   CRC(4fb50304) SHA1(dde1b2465f54f4f6f485c1ba063ff47c5d1baf27) )
MEGADRIVE_ROM_LOAD( tonylrb, "tony la russa baseball (usa, australia).bin",                                              0x000000, 0x100000,  CRC(24629c78) SHA1(0e3f7ebf8661cac9bd7a0c5af64f260e7b5f0a0b) )
MEGADRIVE_ROM_LOAD( topgear2, "top gear 2 (usa).bin",                                                                    0x000000, 0x100000,  CRC(bd3074d2) SHA1(e548e2ed4f69c32dd601a2b90bcf4eeb34d36c49) )
MEGADRIVE_ROM_LOAD( tpgolf, "top pro golf (japan).bin",                                                                  0x000000, 0x100000,  CRC(62bad606) SHA1(1663289933b37526e4e07a6ee7fcd5e6cc2b489a) )
MEGADRIVE_ROM_LOAD( tpgolf2, "top pro golf 2 (japan).bin",                                                               0x000000, 0x100000,  CRC(b8ce98b3) SHA1(95a8918c98420fdfd59a4cb0be8fd5ec69d3593e) )
MEGADRIVE_ROM_LOAD( totlfoot, "total football (europe).bin",                                                             0x000000, 0x200000,  CRC(8360b66a) SHA1(271b13e1c0697d17ad702e97297ed1ea09ddb53b) )
MEGADRIVE_ROM_LOAD( toughman, "toughman contest (usa, europe).bin",                                                      0x000000, 0x400000,  CRC(e19fbc93) SHA1(c7c533b25a50b9c1ccd4c9772bf50957f728c074) )
MEGADRIVE_ROM_LOAD( tougiou, "tougiou king colossus (japan).bin",                                                        0x000000, 0x100000,  CRC(ffe7b3c7) SHA1(92bfb3548ebff18eedebe07751bf2170f95780d7) )
MEGADRIVE_ROM_LOAD( toxicc, "toxic crusaders (usa).bin",                                                                 0x000000, 0x80000,   CRC(11fd46ce) SHA1(fd83f309b6d4261f0c98ba97a8627cfb5212093b) )
MEGADRIVE_ROM_LOAD( toysto, "toy story (europe).bin",                                                                    0x000000, 0x400000,  CRC(8e89a9f3) SHA1(6e7bb9b191389973922a5ab9978205bb9d2664cc) )
MEGADRIVE_ROM_LOAD( toystou, "toy story (usa).bin",                                                                      0x000000, 0x400000,  CRC(829fe313) SHA1(49be571cd943fd594949c318a0bdbe6263fdd512) )
MEGADRIVE_ROM_LOAD( toys, "toys (usa).bin",                                                                              0x000000, 0x100000,  CRC(cbc9951b) SHA1(debc3a571c2c08a731758113550c040dfcda4782) )
MEGADRIVE_ROM_LOAD( trampoli, "trampoline terror! (usa).bin",                                                            0x000000, 0x40000,   CRC(aabb349f) SHA1(d0dc2acdc17a1e1da25828f7d07d4ba9e3c9bd78) )
MEGADRIVE_ROM_LOAD( traysia, "traysia (usa).bin",                                                                        0x000000, 0x100000,  CRC(96184f4f) SHA1(ff0efe6da308919f843a7593e8af7fae82160b0b) )
MEGADRIVE_ROM_LOAD( triple96, "triple play '96 (usa).bin",                                                               0x000000, 0x400000,  CRC(f1748e91) SHA1(c0981b524d5e1c5368f9e74a4ce9c57d87fe323a) )
MEGADRIVE_ROM_LOAD( tpglda, "triple play gold (u) [a1].bin",                                                             0x000000, 0x400000,  CRC(a89638a0) SHA1(cb7f4b9b89fbf6162d7d4182229c8ac473f91cf4) )
MEGADRIVE_ROM_LOAD( tpgld, "triple play gold (usa).bin",                                                                 0x000000, 0x400000,  CRC(bbe69017) SHA1(007bee242384db1887c5831657470584ff77a163) )
MEGADRIVE_ROM_LOAD( troubsht, "trouble shooter (usa).bin",                                                               0x000000, 0x80000,   CRC(becfc39b) SHA1(3fc9ffa49ece5e9cbba1f2a5ba1dfd068b86c65d) )
MEGADRIVE_ROM_LOAD( troyaik, "troy aikman nfl football (usa).bin",                                                       0x000000, 0x200000,  CRC(015f2713) SHA1(820efb4a4d3d29036911d9077bb6c0a4ce7f36d4) )
MEGADRIVE_ROM_LOAD( truelies, "true lies (world).bin",                                                                   0x000000, 0x200000,  CRC(18c09468) SHA1(d39174bed46ede85531b86df7ba49123ce2f8411) )
MEGADRIVE_ROM_LOAD( tunshi, "tun shi tian di iii (china) (simple chinese) (unlicensed).bin",                             0x000000, 0x200000,  CRC(ea57b668) SHA1(7040e96c053f29c75cf0524ddb168a83d0fb526f) )
MEGADRIVE_ROM_LOAD( tunshi1, "tun shi tian di iii (china) (unlicensed).bin",                                             0x000000, 0x200000,  CRC(b23c4166) SHA1(534bf8f951ee30d47df18202246245b998c0eced) )
MEGADRIVE_ROM_LOAD( turboout, "turbo outrun (japan, europe).bin",                                                        0x000000, 0x80000,   CRC(0c661369) SHA1(3e4b9881dde758ef7bb090b39d3556d9bc0d9f1e) )
MEGADRIVE_ROM_LOAD( turmad, "turma da monica na terra dos monstros (brazil).bin",                                        0x000000, 0x100000,  CRC(f8288de1) SHA1(80fc2a6a6b8b943f781598094f3b5a5fe4f05ede) )
MEGADRIVE_ROM_LOAD( turrican, "turrican (usa, europe).bin",                                                              0x000000, 0x80000,   CRC(634d67a6) SHA1(5a471a276909dcc428cd66c51047aa8a142c76a8) )
MEGADRIVE_ROM_LOAD( twincobr, "twin cobra (usa).bin",                                                                    0x000000, 0xa0000,   CRC(2c708248) SHA1(c386c617703a3f5278d24b310c6bc15e3e180bdf) )
MEGADRIVE_ROM_LOAD( twinklet, "twinkle tale (japan).bin",                                                                0x000000, 0x100000,  CRC(d757f924) SHA1(5377e96b4cb14038675c41d165f1d92ae067cd9b) )
MEGADRIVE_ROM_LOAD( twistedf, "twisted flipper (usa) (beta).bin",                                                        0x000000, 0x80000,   CRC(6dd47554) SHA1(a6eac3e1b9df9d138fceeccbe34113015d3a676f) )
MEGADRIVE_ROM_LOAD( twocrude, "two crude dudes (europe).bin",                                                            0x000000, 0x100000,  CRC(b6d90a10) SHA1(babf6ec6dac62e7563c4fe9cb278179dc4343ea4) )
MEGADRIVE_ROM_LOAD( twocrudeu, "two crude dudes (usa).bin",                                                              0x000000, 0x100000,  CRC(721b5744) SHA1(e8c2faa0de6d370a889426b38538e75c264c4456) )
MEGADRIVE_ROM_LOAD( twotribe, "two tribes - populous ii (europe).bin",                                                   0x000000, 0x100000,  CRC(ee988bd9) SHA1(556a79febff1fe4beb41e4e8a2629ff02b20c38f) )
MEGADRIVE_ROM_LOAD( tyrant, "tyrants - fight through time (usa).bin",                                                    0x000000, 0x100000,  CRC(a744921e) SHA1(99ae974fccebfbf6f5a4738e953ad55181144a99) )
MEGADRIVE_ROM_LOAD( uchuusen, "uchuu senkan gomora (japan).bin",                                                         0x000000, 0x100000,  CRC(c511e8d2) SHA1(4e1d77cc1bf081e42abfd1c489fbd0073f0236af) )
MEGADRIVE_ROM_LOAD( umk3, "ultimate mortal kombat 3 (europe).bin",                                                       0x000000, 0x400000,  CRC(ecfb5cb4) SHA1(044bfdb3761df7c4d54a25898353fabcd3f604a3) )
MEGADRIVE_ROM_LOAD( umk3u, "ultimate mortal kombat 3 (usa).bin",                                                         0x000000, 0x400000,  CRC(7290770d) SHA1(bf2da4a7ae7aa428b0b316581f65b280dc3ba356) )
MEGADRIVE_ROM_LOAD( uqix, "ultimate qix (usa).bin",                                                                      0x000000, 0x40000,   CRC(d83369d9) SHA1(400edf467e20f8f43b7f7c8f18f5f46ed54eac86) )
MEGADRIVE_ROM_LOAD( usoccr, "ultimate soccer (europe).bin",                                                              0x000000, 0x100000,  CRC(83db6e58) SHA1(91781d0561f84de0d304221bbc26f4035f62010f) )
MEGADRIVE_ROM_LOAD( ultraman, "ultraman (japan).bin",                                                                    0x000000, 0x80000,   CRC(83b4d5fb) SHA1(7af1b66555c636664a73ca2091ef92ac800dc5d8) )
MEGADRIVE_ROM_LOAD( unchw, "uncharted waters (usa).bin",                                                                 0x000000, 0x100000,  CRC(4edaec59) SHA1(a76cf7dd06784cba15fa0c3be0ae92cba71ccade) )
MEGADRIVE_ROM_LOAD( unchw2, "uncharted waters - new horizons (usa).bin",                                                 0x000000, 0x200000,  CRC(ead69824) SHA1(9fd375cd212a132db24c40a8977c50d0f7b81524) )
MEGADRIVE_ROM_LOAD( undead, "undead line (japan).bin",                                                                   0x000000, 0x100000,  CRC(fb3ca1e6) SHA1(cb9f248bfd19b16ed8a11639a73a6e90fa7ee79e) )
MEGADRIVE_ROM_LOAD( universs, "universal soldier (usa, europe).bin",                                                     0x000000, 0x100000,  CRC(352ebd49) SHA1(9ac1416641ba0e6632369ffe69599b08fc3c225f) )
MEGADRIVE_ROM_LOAD( unnecess, "unnecessary roughness 95 (usa).bin",                                                      0x000000, 0x200000,  CRC(9920e7b7) SHA1(7e100cb56c30498c1fee3867ff3612567287b656) )
MEGADRIVE_ROM_LOAD( urbanstr, "urban strike (usa, europe).bin",                                                          0x000000, 0x200000,  CRC(cf690a75) SHA1(897a8f6a08d6d7d6e5316f8047532b3e4603e705) )
MEGADRIVE_ROM_LOAD( uzukeo, "uzu keobukseon (korea).bin",                                                                0x000000, 0x80000,   CRC(a7255ba8) SHA1(1d59864916be04640a117082c62453c09bcbf8b8) )
MEGADRIVE_ROM_LOAD( vvjapa, "v-v (japan).bin",                                                                           0x000000, 0x100000,  CRC(ad9d0ec0) SHA1(ca344521cb5015d142bdbce0eb44cea050b8e86b) )
MEGADRIVE_ROM_LOAD( valis, "valis (usa).bin",                                                                            0x000000, 0x100000,  CRC(13bc5b72) SHA1(fed29e779aa0d75645a59608f9f3a13f39d43888) )
MEGADRIVE_ROM_LOAD( valis3j, "valis iii (japan) (rev a).bin",                                                            0x000000, 0x100000,  CRC(4d49a166) SHA1(640c3d4f341c8a9b19e5deafae33e7742f109e2d) )
MEGADRIVE_ROM_LOAD( valis3, "valis iii (usa).bin",                                                                       0x000000, 0x100000,  CRC(59a2a368) SHA1(bb051779d6c4c68a8a4571177990f7d190696b4a) )
MEGADRIVE_ROM_LOAD( vaportr, "vapor trail (usa).bin",                                                                    0x000000, 0x100000,  CRC(c49e3a0a) SHA1(d8eb087eeda31f202e6a1a0c4de891ced162abc5) )
MEGADRIVE_ROM_LOAD( vecmana, "vectorman (beta).bin",                                                                     0x000000, 0x200000,  CRC(2084d3da) SHA1(e6c0854ff0f5a0b53760677743e9b901b3e5a4b7) )
MEGADRIVE_ROM_LOAD( vecmanb, "vectorman (prototype - jul 24, 1995).bin",                                                 0x000000, 0x200000,  CRC(0145738b) SHA1(f96a51e792edde1ed203054ea9e23fd226d1ed70) )
MEGADRIVE_ROM_LOAD( vecmanc, "vectorman (usa) (beta).bin",                                                               0x000000, 0x200000,  CRC(a315c8aa) SHA1(54d611a0519d34c25ef6b9963543ece4afc23e19) )
MEGADRIVE_ROM_LOAD( vecman, "vectorman (usa, europe).bin",                                                               0x000000, 0x200000,  CRC(d38b3354) SHA1(57a64d08028b539dc236a693d383f2e1269a5dd4) )
MEGADRIVE_ROM_LOAD( vecman2b, "vectorman 2 (prototype - aug 15, 1996).bin",                                              0x000000, 0x300000,  CRC(998b087c) SHA1(0bbbc09e9ac38940ee4a6c927de67196f42e2bdc) )
MEGADRIVE_ROM_LOAD( vecman2c, "vectorman 2 (prototype - aug 16, 1996).bin",                                              0x000000, 0x300000,  CRC(2986e5ce) SHA1(343527612a0710703486c7335b5939b7099eebba) )
MEGADRIVE_ROM_LOAD( vecman2d, "vectorman 2 (prototype - aug 19, 1996).bin",                                              0x000000, 0x300000,  CRC(49efab7e) SHA1(fb949c8734655e48a787f0c869b8c48f4b0a113e) )
MEGADRIVE_ROM_LOAD( vecman2e, "vectorman 2 (prototype - aug 26, 1996).bin",                                              0x000000, 0x2a8e46,  CRC(eb8124c8) SHA1(39f496d8d99a4cb5716f731d9db656e0ca594a74) )
MEGADRIVE_ROM_LOAD( vecman2f, "vectorman 2 (prototype - aug 27, 1996).bin",                                              0x000000, 0x300000,  CRC(6333d89a) SHA1(d142ce853becc6450c7a3e8b7fe65839e5b68521) )
MEGADRIVE_ROM_LOAD( vecman2a, "vectorman 2 (usa) (beta).bin",                                                            0x000000, 0x200000,  CRC(ada2b0ef) SHA1(65dc6261179e51d21c3c5fe0c0353befbcc95a2d) )
MEGADRIVE_ROM_LOAD( vecman2, "vectorman 2 (usa).bin",                                                                    0x000000, 0x300000,  CRC(c1a24088) SHA1(c5adca10408f055c0431e1ffc01d4fbab53ade01) )
MEGADRIVE_ROM_LOAD( vermil, "vermilion (japan).bin",                                                                     0x000000, 0xa0000,   CRC(e400dfc3) SHA1(697fb165051179a2bbca77c8cfd0c929e334f8c1) )
MEGADRIVE_ROM_LOAD( verytex, "verytex (japan).bin",                                                                      0x000000, 0x80000,   CRC(bafc375f) SHA1(420780933e34da0f9b2a22b6bbb0739e363aab3a) )
MEGADRIVE_ROM_LOAD( viewpb, "viewpoint (usa) (beta).bin",                                                                0x000000, 0x180000,  CRC(f2e69ce7) SHA1(7c7a7812e8d1ab438907233ad03ec0b763a9b556) )
MEGADRIVE_ROM_LOAD( viewp, "viewpoint (usa).bin",                                                                        0x000000, 0x200000,  CRC(59c71866) SHA1(f25f770464448da4e49eab3832100ba480c9844a) )
MEGADRIVE_ROM_LOAD( vf2a, "mpr-19369.bin",                                                                               0x000000, 0x400000,  CRC(a95d0949) SHA1(dff095c9c252bf4af5aae52b83bc3c9d243176b7) )
MEGADRIVE_ROM_LOAD( vf2b, "virtua fighter 2 (prototype - aug 19, 1996).bin",                                             0x000000, 0x400000,  CRC(c5ee3974) SHA1(8d9e1739a40cf64adf0b96918028bac9368b2d89) )
MEGADRIVE_ROM_LOAD( vf2c, "virtua fighter 2 (prototype - aug 30, 1996).bin",                                             0x000000, 0x400000,  CRC(6a0f3a7b) SHA1(b08e11a477694f50bed2e89ee3269f557b041ab1) )
MEGADRIVE_ROM_LOAD( vf2d, "virtua fighter 2 (prototype - sep 13, 1996).bin",                                             0x000000, 0x400000,  CRC(fa5bad91) SHA1(b6ae0d56d9fd41df5a577b36fc3717d7399b80fc) )
MEGADRIVE_ROM_LOAD( vf2e, "virtua fighter 2 (prototype - sep 20, 1996).bin",                                             0x000000, 0x400000,  CRC(af516dad) SHA1(740190e416965ab4b63e82a9a4806178d3463fa9) )
MEGADRIVE_ROM_LOAD( vf2f, "virtua fighter 2 (prototype - sep 27, 1996).bin",                                             0x000000, 0x400000,  CRC(95ab6ab6) SHA1(4375b574ab6c3aa41576a91f5a263196f6ec8fc1) )
MEGADRIVE_ROM_LOAD( vf2, "virtua fighter 2 (usa, europe).bin",                                                           0x000000, 0x400000,  CRC(937380f3) SHA1(c283bf31b646489c2341f8325c52fb8b788a3702) )
MEGADRIVE_ROM_LOAD( vf2tek, "virtua fighter 2 vs tekken 2 (unlicensed).bin",                                             0x000000, 0x200000,  CRC(2cdb499d) SHA1(0a5be6d37db5579b9de991b71442a960afcfe902) )
MEGADRIVE_ROM_LOAD( vra, "virtua racing (e) [a1].bin",                                                                   0x000000, 0x200000,  CRC(5a943df9) SHA1(2c08ea556c79d48e88ff5202944c161ae1b41c63) )
MEGADRIVE_ROM_LOAD( vr, "virtua racing (europe).bin",                                                                    0x000000, 0x200000,  CRC(9624d4ef) SHA1(2c3812f8a010571e51269a33a989598787d27c2d) )
MEGADRIVE_ROM_LOAD( vrj, "virtua racing (japan).bin",                                                                    0x000000, 0x200000,  CRC(53a293b5) SHA1(0ad38a3ab1cc99edac72184f8ae420e13df5cac6) )
MEGADRIVE_ROM_LOAD( vru, "virtua racing (usa).bin",                                                                      0x000000, 0x200000,  CRC(7e1a324a) SHA1(ff969ae53120cc4e7cb1a8a7e47458f2eb8a2165) )
MEGADRIVE_ROM_LOAD( vbart, "virtual bart (world).bin",                                                                   0x000000, 0x200000,  CRC(8db9f378) SHA1(5675fdafb27bb3e23f7d9bf4e74d313e42b26c65) )
MEGADRIVE_ROM_LOAD( virtpb, "virtual pinball (usa, europe).bin",                                                         0x000000, 0x100000,  CRC(d63473aa) SHA1(cd066bb54e0a4c21821639728893462b0218597e) )
MEGADRIVE_ROM_LOAD( vixen357, "vixen 357 (japan).bin",                                                                   0x000000, 0x100000,  CRC(3afa2d7b) SHA1(460037301df0d67947bd17eddb38a3011896cb43) )
MEGADRIVE_ROM_LOAD( volfied, "volfied (japan).bin",                                                                      0x000000, 0x40000,   CRC(b0c5e3f7) SHA1(7de808400f64dd0e4f468049fff7d29212ea0215) )
MEGADRIVE_ROM_LOAD( vrtroop, "vr troopers (usa, europe).bin",                                                            0x000000, 0x200000,  CRC(2f35516e) SHA1(e4c5271ef2034532841fde323f59d728365d7f6a) )
MEGADRIVE_ROM_LOAD( wackyr, "wacky races (usa) (prototype).bin",                                                         0x000000, 0x200000,  CRC(1b173f09) SHA1(5f310699e017e0becb4f41e7f00cb86606dbc0ed) )
MEGADRIVE_ROM_LOAD( wwa, "wacky worlds (prototype - aug 08, 1994).bin",                                                  0x000000, 0x100000,  CRC(48d33ea0) SHA1(6526dbef07792ae5d854c20452c4dfee656c299c) )
MEGADRIVE_ROM_LOAD( wwb, "wacky worlds (prototype - aug 17, 1994).bin",                                                  0x000000, 0xfcbb0,   CRC(4615970b) SHA1(e6409dd65c522af90f65209e8c00a9e1f248c52a) )
MEGADRIVE_ROM_LOAD( wwc, "wacky worlds (prototype - aug 19, 1994).bin",                                                  0x000000, 0xfcb96,   CRC(7f286925) SHA1(42a8927f0738ed9baa88f8115a23941431fa1ae7) )
MEGADRIVE_ROM_LOAD( ww, "wacky worlds creativity studio (usa).bin",                                                      0x000000, 0x100000,  CRC(8af4552d) SHA1(e331c57ce6a176ab9ff1461e9423514756c5558d) )
MEGADRIVE_ROM_LOAD( waniwani, "wani wani world (japan).bin",                                                             0x000000, 0x80000,   CRC(56f0dbb2) SHA1(4cbe0ad0dbd70fd54432d6ae52d185c39c45260e) )
MEGADRIVE_ROM_LOAD( wardner, "wardner (usa).bin",                                                                        0x000000, 0x80000,   CRC(1e369ae2) SHA1(23bd6421f0e3710350e12f9322e75160a699ace8) )
MEGADRIVE_ROM_LOAD( wardnerj, "wardner no mori special (japan).bin",                                                     0x000000, 0x80000,   CRC(80f1035c) SHA1(28844399b73628a3507ca38e855d0afe24c59f4b) )
MEGADRIVE_ROM_LOAD( warlockb, "warlock (usa) (beta).bin",                                                                0x000000, 0x200000,  CRC(c9b6edb3) SHA1(870ce90ae5a58838c1e0531abdc0442389189234) )
MEGADRIVE_ROM_LOAD( warlock, "warlock (usa, europe).bin",                                                                0x000000, 0x200000,  CRC(0a46539b) SHA1(b64f3d0fa74ec93782b4c0441653d72b675e23a7) )
MEGADRIVE_ROM_LOAD( warpsp, "warpspeed (usa).bin",                                                                       0x000000, 0x200000,  CRC(143697ed) SHA1(a51564e9cf60fd5df6a7c00cdbbaa310cfa48d19) )
MEGADRIVE_ROM_LOAD( warrior, "warrior of rome (usa).bin",                                                                0x000000, 0x100000,  CRC(5be10c6a) SHA1(d75eb583e7ec83d6b8308f6dc7cdb31c62b4dbf9) )
MEGADRIVE_ROM_LOAD( warrior2, "warrior of rome ii (usa).bin",                                                            0x000000, 0x100000,  CRC(cd8c472a) SHA1(fe4e3684212f1e695bdf4a4c41999fac773259f4) )
MEGADRIVE_ROM_LOAD( warsong, "warsong (usa).bin",                                                                        0x000000, 0x80000,   CRC(4b680285) SHA1(9b13a85f39b3f4cc31f54077df29bbe812405a08) )
MEGADRIVE_ROM_LOAD( waterwld, "waterworld (europe) (prototype).bin",                                                     0x000000, 0x200000,  CRC(51c80498) SHA1(35e4186654a677a43d16861f9832199c5cb5e0ef) )
MEGADRIVE_ROM_LOAD( wayneg, "wayne gretzky and the nhlpa all-stars (usa, europe).bin",                                   0x000000, 0x200000,  CRC(c2c13b81) SHA1(0b068f684e206139bcd592daba4613cbf634dd56) )
MEGADRIVE_ROM_LOAD( waynewld, "wayne's world (usa).bin",                                                                 0x000000, 0x100000,  CRC(d2cf6ebe) SHA1(2b55be87cd53514b261828fd264108fbad1312cd) )
MEGADRIVE_ROM_LOAD( weapon, "weaponlord (usa).bin",                                                                      0x000000, 0x300000,  CRC(b9895365) SHA1(04128e3859d6eca6e044f456f8b0d06b63e3fb0c) )
MEGADRIVE_ROM_LOAD( wheelfor, "wheel of fortune (usa).bin",                                                              0x000000, 0x80000,   CRC(c8d8efc3) SHA1(ab00e1caa8c86a37e7766b3102d02d433f77470e) )
MEGADRIVE_ROM_LOAD( wwrldb, "where in the world is carmen sandiego (brazil) (es,pt).bin",                                0x000000, 0x100000,  CRC(7d4450ad) SHA1(8441f0eba23c0b013ca3932914f9f8d364e61a01) )
MEGADRIVE_ROM_LOAD( wwrld, "where in the world is carmen sandiego (usa, europe) (en,fr,de,es,it).bin",                   0x000000, 0x100000,  CRC(eef372e8) SHA1(97c1f36e375ac33fbc5724bbb4bb296d974b114f) )
MEGADRIVE_ROM_LOAD( wtimeb, "where in time is carmen sandiego (brazil) (es,pt).bin",                                     0x000000, 0x100000,  CRC(d523b552) SHA1(213b6dfc129fe245f2ecd73ad91c772efd628462) )
MEGADRIVE_ROM_LOAD( wtime, "where in time is carmen sandiego (usa, europe) (en,fr,de,es,it).bin",                        0x000000, 0x100000,  CRC(ea19d4a4) SHA1(0b726481cd9333d26aa3fe53fa2f293c0c385509) )
MEGADRIVE_ROM_LOAD( whiprush, "whip rush (usa).bin",                                                                     0x000000, 0x80000,   CRC(7eb6b86b) SHA1(f05560b088565c1c32c0039dc7bf58caa5310680) )
MEGADRIVE_ROM_LOAD( whiprushj, "whip rush - wakusei voltegas no nazo (japan).bin",                                       0x000000, 0x80000,   CRC(8084b4d1) SHA1(c7aac47bd0ecd6018ec1f1f4b42e53f27e318b15) )
MEGADRIVE_ROM_LOAD( wildsn, "wild snake (usa) (prototype).bin",                                                          0x000000, 0x80000,   CRC(0c1a49e5) SHA1(68241498209c5e1f09ca335aee1b0f55ce19ff6e) )
MEGADRIVE_ROM_LOAD( williams, "williams arcade's greatest hits (usa).bin",                                               0x000000, 0x80000,   CRC(d68e9c00) SHA1(8c648318fdd54e2c75c44429a72c1c1a846e67cb) )
MEGADRIVE_ROM_LOAD( wimbled, "wimbledon championship tennis (europe).bin",                                               0x000000, 0x100000,  CRC(b791a435) SHA1(6b7d8aa9d4b9d10c26dc079ab78e11766982cef2) )
MEGADRIVE_ROM_LOAD( wimbledj, "wimbledon championship tennis (japan).bin",                                               0x000000, 0x100000,  CRC(3e0c9daf) SHA1(73a6dad0bedc5552459e8a74a9b8ae242ad5a78e) )
MEGADRIVE_ROM_LOAD( wimbledub, "wimbledon championship tennis (usa) (beta).bin",                                         0x000000, 0x100000,  CRC(9febc760) SHA1(402bdc507647d861ee7bb80599f528d3d5aeaf0f) )
MEGADRIVE_ROM_LOAD( wimbledu, "wimbledon championship tennis (usa).bin",                                                 0x000000, 0x100000,  CRC(f9142aee) SHA1(db9083fd257d7cb718a010bdc525980f254e2c66) )
MEGADRIVE_ROM_LOAD( wingswor, "wings of wor (usa).bin",                                                                  0x000000, 0x80000,   CRC(210a2fcd) SHA1(91caad60355f6bd71949118bd30ea16d0f4c066e) )
MEGADRIVE_ROM_LOAD( wintcb, "winter challenge (beta).bin",                                                               0x000000, 0x100000,  CRC(60d2a8c4) SHA1(61748b459900f26832da72d6b4c51f886994d7cf) )
MEGADRIVE_ROM_LOAD( wintc, "winter challenge (usa, europe) (rev 1).bin",                                                 0x000000, 0x100000,  CRC(f57c7068) SHA1(6c948c98bbf52b849ffa1920e127c74ae04d75d1) )
MEGADRIVE_ROM_LOAD( wintca, "winter challenge (usa, europe).bin",                                                        0x000000, 0x100000,  CRC(dbc3ed1c) SHA1(d2afa782f8d05afc0c3a3d6684bd4966034705c6) )
MEGADRIVE_ROM_LOAD( wintolu, "winter olympic games (usa).bin",                                                           0x000000, 0x200000,  CRC(c5834437) SHA1(da5a2a0c1a0a6b7b29492f8845eca1167158ebea) )
MEGADRIVE_ROM_LOAD( wintol, "winter olympics (europe).bin",                                                              0x000000, 0x200000,  CRC(fa537a45) SHA1(84528efaf0729637167774d59a00694deadd5d6d) )
MEGADRIVE_ROM_LOAD( wintolj, "winter olympics (japan).bin",                                                              0x000000, 0x200000,  CRC(654a4684) SHA1(aa2fc21eaa833640eaf882d66ca6ceb4f8adabcf) )
MEGADRIVE_ROM_LOAD( wiznliz, "wiz'n'liz (usa).bin",                                                                      0x000000, 0x100000,  CRC(df036b62) SHA1(1cbd0f820aaf35dd6328a865ecc4b9eedc740807) )
MEGADRIVE_ROM_LOAD( wiznliz1, "wiz'n'liz - the frantic wabbit wescue (europe).bin",                                      0x000000, 0x100000,  CRC(f09353b4) SHA1(2e7a1724a72e89a3f9a66720d9a7c6f293263798) )
MEGADRIVE_ROM_LOAD( wizard, "wizard of the immortal (japan).bin",                                                        0x000000, 0x200000,  CRC(c99fad92) SHA1(f36d088c37bbdcf473615b23e1cb21c0b70d8f05) )
MEGADRIVE_ROM_LOAD( wolfch, "wolfchild (usa).bin",                                                                       0x000000, 0x100000,  CRC(eb5b1cbf) SHA1(000d45769e7e9f72f665e160910ba9fb79eec43f) )
MEGADRIVE_ROM_LOAD( wolver, "wolverine - adamantium rage (usa, europe).bin",                                             0x000000, 0x200000,  CRC(d2437bb7) SHA1(261f8ed75586ca7cdec176eb81550458bf1ff437) )
MEGADRIVE_ROM_LOAD( wboy3, "wonder boy iii - monster lair (japan, europe).bin",                                          0x000000, 0x80000,   CRC(c24bc5e4) SHA1(1fd3f77a2223ebeda547b81e49f3dfc9d0197439) )
MEGADRIVE_ROM_LOAD( wbmw, "wonder boy in monster world (usa, europe).bin",                                               0x000000, 0xc0000,   CRC(1592f5b0) SHA1(87a968f773c7e807e647c0737132457b06b78276) )
MEGADRIVE_ROM_LOAD( wboy5, "wonder boy v - monster world iii (japan, korea).bin",                                        0x000000, 0xa0000,   CRC(45a50f96) SHA1(1582f159e1969ff0541319a9bd7e6f7a53505d01) )
MEGADRIVE_ROM_LOAD( wondlib, "wonder library (japan) (program).bin",                                                     0x000000, 0x80000,   CRC(9350e754) SHA1(57b97d9ddecfa2e9a75c4a05cd2b7e821210155a) )
MEGADRIVE_ROM_LOAD( wcs2a, "world championship soccer ii (prototype - feb 23, 1994).bin",                                0x000000, 0x100000,  CRC(aab9e240) SHA1(ee4251c32961dc003a78bedbf42b231a31cc0acf) )
MEGADRIVE_ROM_LOAD( wcs2b, "world championship soccer ii (prototype - mar 09, 1994).bin",                                0x000000, 0x100000,  CRC(71fa89cc) SHA1(7b2654e7828989cc776b2645d635271d951f671f) )
MEGADRIVE_ROM_LOAD( wcs2c, "world championship soccer ii (prototype - mar 14, 1994).bin",                                0x000000, 0x100000,  CRC(43ec107c) SHA1(9226eb23e1a91856300b310cb2b8263a832ba231) )
MEGADRIVE_ROM_LOAD( wcs2d, "world championship soccer ii (prototype - mar 23, 1994).bin",                                0x000000, 0x100000,  CRC(3852e514) SHA1(1e6f3ba839d23f7ba3e01d144b6c1c635207fc7d) )
MEGADRIVE_ROM_LOAD( wcs2e, "world championship soccer ii (prototype - mar 24, 1994).bin",                                0x000000, 0x100000,  CRC(a6d43b4a) SHA1(6d50cc35d2da2d4ec8f3f090e9866747c07164f3) )
MEGADRIVE_ROM_LOAD( wcs2f, "world championship soccer ii (prototype - mar 25, 1994).bin",                                0x000000, 0x100000,  CRC(6b3624fb) SHA1(4e64d9aa93107f9553e9406d8f0f1a998ea5aabd) )
MEGADRIVE_ROM_LOAD( wcs2g, "world championship soccer ii (prototype - mar 26, 1994).bin",                                0x000000, 0x100000,  CRC(1ad7ed9c) SHA1(a82cfecf5a384f77f592251100d3447c5fa1e1c7) )
MEGADRIVE_ROM_LOAD( wcs2h, "world championship soccer ii (prototype - mar 27, 1994).bin",                                0x000000, 0x100000,  CRC(4e141509) SHA1(b3301897af0590ab7c8cc2b2028b40192012aa65) )
MEGADRIVE_ROM_LOAD( wcs2i, "world championship soccer ii (prototype - mar 29, 1994 - b).bin",                            0x000000, 0x100000,  CRC(a6759340) SHA1(27af9bbaa449c38395afb8f29b2626056a4ae891) )
MEGADRIVE_ROM_LOAD( wcs2j, "world championship soccer ii (prototype - mar 29, 1994).bin",                                0x000000, 0x100000,  CRC(5f9c51f7) SHA1(400fd9a69a383468abec66032401d0ab6d8888fd) )
MEGADRIVE_ROM_LOAD( wcs2k, "world championship soccer ii (prototype - mar 30, 1994).bin",                                0x000000, 0x100000,  CRC(8dd49c92) SHA1(ac31599d964a2b8ad69eebc47db2947c82768e98) )
MEGADRIVE_ROM_LOAD( wcs2l, "world championship soccer ii (prototype - may 23, 1994).bin",                                0x000000, 0x100000,  CRC(6065774d) SHA1(5f5a31a3e8e3b56aa0a3acdb8049246160679a8f) )
MEGADRIVE_ROM_LOAD( wcs2m, "world championship soccer ii (prototype g - feb 22, 1994).bin",                              0x000000, 0x100000,  CRC(6fbfa14e) SHA1(188fa0adc8662d7a8eeac1f174811f794e081552) )
MEGADRIVE_ROM_LOAD( wcs2n, "world championship soccer ii (prototype j - feb 28, 1994).bin",                              0x000000, 0x100000,  CRC(e210a74c) SHA1(f633f6ef930426a12c34958d3485c815a82a2276) )
MEGADRIVE_ROM_LOAD( wcs2o, "world championship soccer ii (prototype n - mar 03, 1994).bin",                              0x000000, 0x100000,  CRC(07b387a4) SHA1(0b8e79d7fb38ec8816dc610ef4aee33cabbf08f3) )
MEGADRIVE_ROM_LOAD( wcs2p, "world championship soccer ii (prototype o - mar 03, 1994).bin",                              0x000000, 0x100000,  CRC(5a458d42) SHA1(170aa426472cfeb3de1a6b98ed825f435b60b1a5) )
MEGADRIVE_ROM_LOAD( wcs2q, "world championship soccer ii (prototype p - mar 04, 1994).bin",                              0x000000, 0x100000,  CRC(8fe9f6ef) SHA1(ba98b890ca71447bbd7620526f3277e9e9de10fa) )
MEGADRIVE_ROM_LOAD( wcs2r, "world championship soccer ii (prototype r - mar 09, 1994).bin",                              0x000000, 0x100000,  CRC(94625572) SHA1(8e706299b04efc4b0e5e0b9b693c816cb8ccda72) )
MEGADRIVE_ROM_LOAD( wcs2s, "world championship soccer ii (prototype u - mar 14, 1994).bin",                              0x000000, 0x100000,  CRC(43ec107c) SHA1(9226eb23e1a91856300b310cb2b8263a832ba231) )
MEGADRIVE_ROM_LOAD( wcs2t, "world championship soccer ii (prototype y - mar 18, 1994).bin",                              0x000000, 0x100000,  CRC(f6735b61) SHA1(d3a4c99d46f3506821137779226ae09edfae0760) )
MEGADRIVE_ROM_LOAD( wcs2u, "world championship soccer ii (usa) (beta).bin",                                              0x000000, 0x100000,  CRC(c1e21c1a) SHA1(d3f4f2f5e165738bde6c6011c3d68322c27d97ed) )
MEGADRIVE_ROM_LOAD( wcs2, "world championship soccer ii (usa).bin",                                                      0x000000, 0x100000,  CRC(c1dd1c8e) SHA1(f6ce0b826e028599942957729d72c7a8955c5e35) )
MEGADRIVE_ROM_LOAD( wclead, "world class leaderboard golf (europe).bin",                                                 0x000000, 0x80000,   CRC(daca01c3) SHA1(c307a731763c7f858ef27058b4f46017868749d6) )
MEGADRIVE_ROM_LOAD( wcleadu, "world class leaderboard golf (usa).bin",                                                   0x000000, 0x80000,   CRC(53434bab) SHA1(db53b7c661392b8558d0eb1f1dd51c8a0a17fa4e) )
MEGADRIVE_ROM_LOAD( wc90, "world cup italia '90 (europe).bin",                                                           0x000000, 0x40000,   CRC(dd95f829) SHA1(c233313214418300a39afc446e8426cc11f99c6c) )
MEGADRIVE_ROM_LOAD( wcs, "world cup soccer / world championship soccer (japan, usa) (rev b).bin",                        0x000000, 0x40000,   CRC(bf272bcb) SHA1(9da376f266c98c93ae387bec541c513fec84a0e9) )
MEGADRIVE_ROM_LOAD( wcsa, "world cup soccer / world championship soccer (japan, usa) (v1.2).bin",                        0x000000, 0x40000,   CRC(bf84ede6) SHA1(ef8c106acad9c3b4a5db2cd0d311762491d28392) )
MEGADRIVE_ROM_LOAD( wcsb, "world cup soccer / world championship soccer (japan, usa).bin",                               0x000000, 0x40000,   CRC(b01c3d70) SHA1(39bbab6430aad3fa9bf024c4b42387ba4ba3e488) )
MEGADRIVE_ROM_LOAD( wcup94, "world cup usa 94 (usa, europe, korea).bin",                                                 0x000000, 0x100000,  CRC(0171b47f) SHA1(af0e8fada3db7e746aef2c0070deb19602c6d32a) )
MEGADRIVE_ROM_LOAD( wherob, "world heroes (e) (prototype - mar 31, 1994 - b).bin",                                       0x000000, 0x200000,  CRC(d210aa6e) SHA1(de2166ed7b4f51aca63abb63db5aee9cff381a1c) )
MEGADRIVE_ROM_LOAD( wheroc, "world heroes (e) (prototype - mar 31, 1994).bin",                                           0x000000, 0x200000,  CRC(92e1bf14) SHA1(aea91bc2c65c33a54b0ec1873be5433beb8685b8) )
MEGADRIVE_ROM_LOAD( wherod, "world heroes (j) (prototype - apr 08, 1994).bin",                                           0x000000, 0x200000,  CRC(2bea7215) SHA1(eb981388d7d2183a64673810a4a047780743c637) )
MEGADRIVE_ROM_LOAD( wheroe, "world heroes (j) (prototype - apr 15, 1994).bin",                                           0x000000, 0x200000,  CRC(6dec07b3) SHA1(a47ac5e017315422194a233d4f0aec9f5cc0e71a) )
MEGADRIVE_ROM_LOAD( wherof, "world heroes (j) (prototype - apr 20, 1994 - b).bin",                                       0x000000, 0x200000,  CRC(f5ffb191) SHA1(f1e661306009254797fee5224d4feed4ccaa8610) )
MEGADRIVE_ROM_LOAD( wherog, "world heroes (j) (prototype - apr 20, 1994) (broken - c05 missing).bin",                    0x000000, 0x200000,  CRC(b43b518c) SHA1(b4af1c44e32e65f2c167d8fc94e884ed8ff703d1) )
MEGADRIVE_ROM_LOAD( wheroh, "world heroes (j) (prototype - mar 30, 1994).bin",                                           0x000000, 0x200000,  CRC(3f93181a) SHA1(cafdd71037782d60b41508e7db7fb5fec654ff71) )
MEGADRIVE_ROM_LOAD( wheroa, "world heroes (japan).bin",                                                                  0x000000, 0x200000,  CRC(56e3ceff) SHA1(0c806b701069ae72b6cfd19b7b65a123192cff23) )
MEGADRIVE_ROM_LOAD( wheroi, "world heroes (u) (prototype - feb 23, 1994).bin",                                           0x000000, 0x200000,  CRC(f5db477e) SHA1(154d8b2415bcbce62656eef63e4fad026ee2d8d0) )
MEGADRIVE_ROM_LOAD( wheroj, "world heroes (u) (prototype - feb 24, 1994).bin",                                           0x000000, 0x200000,  CRC(f5db477e) SHA1(154d8b2415bcbce62656eef63e4fad026ee2d8d0) )
MEGADRIVE_ROM_LOAD( wherok, "world heroes (u) (prototype - mar 03, 1994).bin",                                           0x000000, 0x200000,  CRC(24a63aef) SHA1(0caea512a3e3a56da25ee729213427ecb3ec6380) )
MEGADRIVE_ROM_LOAD( wherol, "world heroes (u) (prototype - mar 07, 1994).bin",                                           0x000000, 0x200000,  CRC(45c29350) SHA1(f479d845e22dd22f9adf94bf961585bec937898f) )
MEGADRIVE_ROM_LOAD( wherom, "world heroes (u) (prototype - mar 09, 1994).bin",                                           0x000000, 0x200000,  CRC(2c7f9e64) SHA1(cf7ab91e4cca7f3ab7203d2468e8375adb8fd7c0) )
MEGADRIVE_ROM_LOAD( wheron, "world heroes (u) (prototype - mar 15, 1994).bin",                                           0x000000, 0x200000,  CRC(2c0b2f4f) SHA1(d02031fe3130d3d313ab2262e82abb0d921a971c) )
MEGADRIVE_ROM_LOAD( wheroo, "world heroes (u) (prototype - mar 16, 1994).bin",                                           0x000000, 0x200000,  CRC(19c74e2a) SHA1(592e3500a2700059efc878c4360070c5b9f4b056) )
MEGADRIVE_ROM_LOAD( wherop, "world heroes (u) (prototype - mar 18, 1994).bin",                                           0x000000, 0x200000,  CRC(2bc2a6bb) SHA1(fa925428f0f9b651a2037aa3bd3e05e6f22183d8) )
MEGADRIVE_ROM_LOAD( wheroq, "world heroes (u) (prototype - mar 22, 1994) (broken - c07 missing).bin",                    0x000000, 0x200000,  CRC(5c3c0931) SHA1(0bd56a492bc9adf862d4b745e6e66909b8acc0d2) )
MEGADRIVE_ROM_LOAD( wheror, "world heroes (u) (prototype - mar 23, 1994).bin",                                           0x000000, 0x200000,  CRC(587e6739) SHA1(2d0fa212a52a1b418fb061926d515200b3a511fa) )
MEGADRIVE_ROM_LOAD( wheros, "world heroes (u) (prototype - mar 24, 1994).bin",                                           0x000000, 0x200000,  CRC(8a656942) SHA1(4a03d5d1b5e629ae69f9f8c5f0cbc96bf46a3843) )
MEGADRIVE_ROM_LOAD( wherot, "world heroes (u) (prototype - mar 30, 1994).bin",                                           0x000000, 0x200000,  CRC(50c797cb) SHA1(8c4dc2b06e0734fc8e39997f8da6295d187e316e) )
MEGADRIVE_ROM_LOAD( whero, "world heroes (usa).bin",                                                                     0x000000, 0x200000,  CRC(0f4d22ec) SHA1(5a563f441c3013ac4b3f9e08d5e5a6e05efc5de0) )
MEGADRIVE_ROM_LOAD( worldilljb, "world of illusion - fushigi na magic box (japan) (beta).bin",                           0x000000, 0x100000,  CRC(577f680f) SHA1(c3d721dc3d6660156f28728d729a2c0d4bb23fdf) )
MEGADRIVE_ROM_LOAD( worldillj, "world of illusion - fushigi na magic box (japan).bin",                                   0x000000, 0x100000,  CRC(cb9ee238) SHA1(c79935fdeb680a1a2e76db1aef2c6897e6ee8e4d) )
MEGADRIVE_ROM_LOAD( worldill, "world of illusion starring mickey mouse and donald duck (europe).bin",                    0x000000, 0x100000,  CRC(121c6a49) SHA1(4b7aa8de517516edd9ee5288124f77238fc9ba6b) )
MEGADRIVE_ROM_LOAD( worldillu, "world of illusion starring mickey mouse and donald duck (usa, korea).bin",               0x000000, 0x100000,  CRC(921ebd1c) SHA1(adb0a2edebb6f978c3217075a2f29003a8b025c6) )
MEGADRIVE_ROM_LOAD( wsb95a, "world series baseball '95 (prototype - dec 08, 1994).bin",                                  0x000000, 0x300000,  CRC(8846b050) SHA1(aad244dcad0e5d4ca72e480cd5f18c7977d2a02f) )
MEGADRIVE_ROM_LOAD( wsb95b, "world series baseball '95 (prototype - dec 14, 1994).bin",                                  0x000000, 0x300000,  CRC(385be354) SHA1(406b2d3540c4664356044c38cf4613bd8f76aefa) )
MEGADRIVE_ROM_LOAD( wsb95c, "world series baseball '95 (prototype - dec 28, 1994 - sb).bin",                             0x000000, 0x300000,  CRC(9e6dbc7c) SHA1(5cd3e9cff259cc8d1d6d43be4c1abc5a938f7979) )
MEGADRIVE_ROM_LOAD( wsb95d, "world series baseball '95 (prototype - feb 02, 1995).bin",                                  0x000000, 0x300000,  CRC(a947fe5c) SHA1(7049bfd6797a0b45cccc5241d748a76f68ca6257) )
MEGADRIVE_ROM_LOAD( wsb95e, "world series baseball '95 (prototype - feb 03, 1995).bin",                                  0x000000, 0x300000,  CRC(1f3754fc) SHA1(2d70804a21c93f8e1c2267404ee0d92905a4a50c) )
MEGADRIVE_ROM_LOAD( wsb95f, "world series baseball '95 (prototype - feb 07, 1995).bin",                                  0x000000, 0x300000,  CRC(e4056559) SHA1(9d729e57d6c137fb409646725933ff623f78863e) )
MEGADRIVE_ROM_LOAD( wsb95g, "world series baseball '95 (prototype - feb 09, 1995 - b).bin",                              0x000000, 0x300000,  CRC(33bb9eda) SHA1(18ae5ad695dabfb9ddbe3dd66e2a0582eb5987c8) )
MEGADRIVE_ROM_LOAD( wsb95h, "world series baseball '95 (prototype - feb 09, 1995).bin",                                  0x000000, 0x300000,  CRC(83be98cf) SHA1(3c17b99184097e5f968b6f2fb2fc31d25c924971) )
MEGADRIVE_ROM_LOAD( wsb95i, "world series baseball '95 (prototype - feb 11, 1995).bin",                                  0x000000, 0x300000,  CRC(a0664fe9) SHA1(d6dbe744b0970a1e6e0d0c5d7d5c8ad6ece06c8a) )
MEGADRIVE_ROM_LOAD( wsb95j, "world series baseball '95 (prototype - feb 12, 1995).bin",                                  0x000000, 0x300000,  CRC(45da8f18) SHA1(dd1df8b0ee600d5bef5460cd8e036b66bfe39394) )
MEGADRIVE_ROM_LOAD( wsb95k, "world series baseball '95 (prototype - feb 13, 1995).bin",                                  0x000000, 0x300000,  CRC(9f30ae30) SHA1(c5e594842430b824e37993ee8902fd99a2d5086e) )
MEGADRIVE_ROM_LOAD( wsb95l, "world series baseball '95 (prototype - feb 14, 1995).bin",                                  0x000000, 0x300000,  CRC(f300a857) SHA1(6271037557f92943ac1a363b42f816b9fd8ca8cb) )
MEGADRIVE_ROM_LOAD( wsb95m, "world series baseball '95 (prototype - jan 01, 1995 - tst).bin",                            0x000000, 0x300000,  CRC(c7574372) SHA1(b9d6cf1a3e342f0e7e0c36a47e92869a6c2bda6d) )
MEGADRIVE_ROM_LOAD( wsb95n, "world series baseball '95 (prototype - jan 03, 1995 - tst).bin",                            0x000000, 0x300000,  CRC(498363f7) SHA1(1c97922c9632e99fda30465341fe39627611d538) )
MEGADRIVE_ROM_LOAD( wsb95o, "world series baseball '95 (prototype - jan 05, 1995).bin",                                  0x000000, 0x300000,  CRC(1820abfb) SHA1(956313af554f82ca10052c8c182493284c0567fd) )
MEGADRIVE_ROM_LOAD( wsb95p, "world series baseball '95 (prototype - jan 09, 1995 - tst).bin",                            0x000000, 0x300000,  CRC(3de7cdbc) SHA1(fde563a0b84e6d6f39498964e237d500196a449a) )
MEGADRIVE_ROM_LOAD( wsb95q, "world series baseball '95 (prototype - jan 10, 1995).bin",                                  0x000000, 0x300000,  CRC(ac2772b0) SHA1(0fbc4f86ef7dcca024d9e62568288f8792850797) )
MEGADRIVE_ROM_LOAD( wsb95r, "world series baseball '95 (prototype - jan 14, 1995 - rm).bin",                             0x000000, 0x301655,  CRC(f70b07cf) SHA1(b301931fdee91cf8b67f9de4e6935310e643078b) )
MEGADRIVE_ROM_LOAD( wsb95s, "world series baseball '95 (prototype - jan 16, 1995).bin",                                  0x000000, 0x300000,  CRC(f0e22b21) SHA1(d2428f24a615a3557c51cbcb750f381cd0db5c50) )
MEGADRIVE_ROM_LOAD( wsb95t, "world series baseball '95 (prototype - jan 18, 1995 - rm).bin",                             0x000000, 0x3023d3,  CRC(5ce70f8f) SHA1(f9e1e8424e12ca7c95e926e3bb4d24233eb09c5d) )
MEGADRIVE_ROM_LOAD( wsb95u, "world series baseball '95 (prototype - jan 20, 1995).bin",                                  0x000000, 0x300000,  CRC(16c15e46) SHA1(91e5d6a7c703d3b47886bc05100ab278b89e2ebd) )
MEGADRIVE_ROM_LOAD( wsb95v, "world series baseball '95 (prototype - jan 25, 1995).bin",                                  0x000000, 0x300000,  CRC(b45d9e33) SHA1(124fa734eee59420f98fc61e0368dbf1ea9ed255) )
MEGADRIVE_ROM_LOAD( wsb95w, "world series baseball '95 (prototype - jan 30, 1995).bin",                                  0x000000, 0x300000,  CRC(6aa76a9d) SHA1(eac7dc5656e27e4c95dfc8b04db85c1c4967b2d5) )
MEGADRIVE_ROM_LOAD( wsb95, "world series baseball '95 (usa).bin",                                                        0x000000, 0x300000,  CRC(25130077) SHA1(878e9fdbbc0b20b27f25d56e4087efbde1e8979a) )
MEGADRIVE_ROM_LOAD( wsb96, "world series baseball '96 (usa).bin",                                                        0x000000, 0x300000,  CRC(04ee8272) SHA1(f91eeedbadd277904f821dfaae9e46f6078ff207) )
MEGADRIVE_ROM_LOAD( wsb98, "world series baseball '98 (usa).bin",                                                        0x000000, 0x300000,  CRC(05b1ab53) SHA1(0881077fd253d19d43ad45de6089d66e75d856b3) )
MEGADRIVE_ROM_LOAD( wsba, "world series baseball (prototype - dec 22, 1993).bin",                                        0x000000, 0x1fc05d,  CRC(0dc0558d) SHA1(a6a3c1a6b04f6318876412a47a8e7c6a92910187) )
MEGADRIVE_ROM_LOAD( wsbb, "world series baseball (prototype - dec 26, 1993).bin",                                        0x000000, 0x200000,  CRC(472e6a58) SHA1(763dcaf06befe2a95963f9d41393eacf151d5a97) )
MEGADRIVE_ROM_LOAD( wsbc, "world series baseball (prototype - dec 29, 1993).bin",                                        0x000000, 0x200000,  CRC(ed949936) SHA1(fda84389295622191f29b9e0fc94673891fa782a) )
MEGADRIVE_ROM_LOAD( wsbd, "world series baseball (prototype - feb 18, 1994).bin",                                        0x000000, 0x200000,  CRC(3cadfc50) SHA1(63af6038d8904d33ef3ab50820e6df84b44b7ada) )
MEGADRIVE_ROM_LOAD( wsbe, "world series baseball (prototype - jan 03, 1994).bin",                                        0x000000, 0x200000,  CRC(4b53f035) SHA1(449092852822a842a607f40a5685ac73d94f4a93) )
MEGADRIVE_ROM_LOAD( wsbf, "world series baseball (prototype - jan 06, 1994).bin",                                        0x000000, 0x200000,  CRC(b07de2d3) SHA1(4622f2a90007c923f87455a8fd6472b66c44f16c) )
MEGADRIVE_ROM_LOAD( wsbg, "world series baseball (prototype - jan 16, 1994).bin",                                        0x000000, 0x200000,  CRC(5322133e) SHA1(d72d21e5295aaa2be1e14dba382c3cca25eec6ac) )
MEGADRIVE_ROM_LOAD( wsbh, "world series baseball (prototype - mar 04, 1994).bin",                                        0x000000, 0x1feaa2,  CRC(edda44a5) SHA1(05ef4f0a2bcc399c7269449c204e61227d1946c5) )
MEGADRIVE_ROM_LOAD( wsbi, "world series baseball (prototype - may 27, 1994).bin",                                        0x000000, 0x200000,  CRC(e473728a) SHA1(ecbf253f2363a1ac5c3797eb80ffee08a8629ae8) )
MEGADRIVE_ROM_LOAD( wsbj, "world series baseball (prototype - oct 01, 1993).bin",                                        0x000000, 0x200000,  CRC(4f7ab92e) SHA1(f2c3e43100739410a69c08408652632f9e7c1be9) )
MEGADRIVE_ROM_LOAD( wsb, "world series baseball (usa).bin",                                                              0x000000, 0x200000,  CRC(57c1d5ec) SHA1(e63cbd1f00eac2ccd2ee0290e7bf1bb47c1288e4) )
MEGADRIVE_ROM_LOAD( worldt, "world trophy soccer (usa).bin",                                                             0x000000, 0x80000,   CRC(6e3edc7c) SHA1(0a0ac6d37d284ec29b9331776bbf6b78edcdbe81) )
MEGADRIVE_ROM_LOAD( wormsb, "worms (europe) (beta).bin",                                                                 0x000000, 0x200000,  CRC(1d191694) SHA1(23127e9b3a98eea13fb97bed2d8e206adb495d97) )
MEGADRIVE_ROM_LOAD( worms, "worms (europe).bin",                                                                         0x000000, 0x200000,  CRC(b9a8b299) SHA1(1a15447a4a791c02b6ad0a609f788d39fe6c3aa6) )
MEGADRIVE_ROM_LOAD( wreswb, "wrestle war (japan) (beta).bin",                                                            0x000000, 0x80000,   CRC(1cdee87b) SHA1(65c4815c6271bb7d526d84dac1bf177741e35364) )
MEGADRIVE_ROM_LOAD( wresw, "wrestle war (japan, europe).bin",                                                            0x000000, 0x80000,   CRC(2d162a85) SHA1(1eac51029fd7fb1da1c5546cbd959220244cf3e5) )
MEGADRIVE_ROM_LOAD( wball, "wrestleball (japan).bin",                                                                    0x000000, 0x80000,   CRC(d563e07f) SHA1(bb6e6cd4f80ad69265b8c0d16b7581c629fd1770) )
MEGADRIVE_ROM_LOAD( wukong, "wu kong wai zhuan (china) (unlicensed).bin",                                                0x000000, 0x200000,  CRC(880a916e) SHA1(dd4d7f7433ab82680d7d36124beca0bacebcd6e4) )
MEGADRIVE_ROM_LOAD( wwfraw, "wwf raw (world).bin",                                                                       0x000000, 0x300000,  CRC(4ef5d411) SHA1(94be19287c64ab8e164ab1105085cb3548c6b179) )
MEGADRIVE_ROM_LOAD( wwfroy, "wwf royal rumble (world).bin",                                                              0x000000, 0x200000,  CRC(b69dc53e) SHA1(34e85015b8681ce15ad4777a60c81297ccf718b1) )
MEGADRIVE_ROM_LOAD( wwfsup, "wwf super wrestlemania (usa, europe).bin",                                                  0x000000, 0x100000,  CRC(b929d6c5) SHA1(a6df2e9887a3f33139e505b5ac739158b987069f) )
MEGADRIVE_ROM_LOAD( wwfaga, "wwf wrestlemania - the arcade game (usa) (alpha).bin",                                      0x000000, 0x40000,   CRC(719d6155) SHA1(7b76ddf26f11ad0e2da81f1cfe96e60a45fa7c33) )
MEGADRIVE_ROM_LOAD( wwfag, "wwf wrestlemania - the arcade game (usa, europe).bin",                                       0x000000, 0x400000,  CRC(a5d023f9) SHA1(e49ad2f9119b0788bbbb7258908d605c032989b4) )
MEGADRIVE_ROM_LOAD( xmen, "x-men (europe).bin",                                                                          0x000000, 0x100000,  CRC(0b78ca97) SHA1(111d696b317ad5b57bc66e037a935d3f123d41c2) )
MEGADRIVE_ROM_LOAD( xmenus, "x-men (usa).bin",                                                                           0x000000, 0x100000,  CRC(f71b21b4) SHA1(2f2b5d018c98b78faf9ab6b172947f5cd65d5cf0) )
MEGADRIVE_ROM_LOAD( xmen2a, "x-men 2 - clone wars (prototype - dec 02, 1994).bin",                                       0x000000, 0x1fbe22,  CRC(7ad7a4e9) SHA1(70b31ade3f5713a3e86e75edfc4139fdbeeb0440) )
MEGADRIVE_ROM_LOAD( xmen2b, "x-men 2 - clone wars (prototype - dec 03, 1994).bin",                                       0x000000, 0x200000,  CRC(669b939c) SHA1(b13ca62340374b06890e5226f3536b643c6d0cc9) )
MEGADRIVE_ROM_LOAD( xmen2c, "x-men 2 - clone wars (prototype - dec 06, 1994).bin",                                       0x000000, 0x200000,  CRC(5b1d0cf5) SHA1(027fb3bd1a225c93871dfe21b0928192470079b5) )
MEGADRIVE_ROM_LOAD( xmen2d, "x-men 2 - clone wars (prototype - dec 07, 1994).bin",                                       0x000000, 0x200000,  CRC(246a30da) SHA1(4c9d8bdcac4daa9832ed7285c8d2a484570b8bfe) )
MEGADRIVE_ROM_LOAD( xmen2e, "x-men 2 - clone wars (prototype - dec 08, 1994).bin",                                       0x000000, 0x1feedc,  CRC(c6d90b1c) SHA1(a36862895851df014935028711818d535e9619fc) )
MEGADRIVE_ROM_LOAD( xmen2f, "x-men 2 - clone wars (prototype - dec 09, 1994).bin",                                       0x000000, 0x200000,  CRC(28307b58) SHA1(2a911569e36fc210fd8adec445814434f1bbec5f) )
MEGADRIVE_ROM_LOAD( xmen2g, "x-men 2 - clone wars (prototype - dec 10, 1994).bin",                                       0x000000, 0x1ffaf0,  CRC(73e2effe) SHA1(35c66c01de1f423720f418f63b536ba5ae7870d9) )
MEGADRIVE_ROM_LOAD( xmen2h, "x-men 2 - clone wars (prototype - dec 11, 1994 - a).bin",                                   0x000000, 0x200000,  CRC(11415b86) SHA1(70e12e86cd340c0ee30024e4299b0c48671787c2) )
MEGADRIVE_ROM_LOAD( xmen2i, "x-men 2 - clone wars (prototype - dec 11, 1994).bin",                                       0x000000, 0x200000,  CRC(51876259) SHA1(31d240defc07674bbfad1f2f1928492c706b19f6) )
MEGADRIVE_ROM_LOAD( xmen2j, "x-men 2 - clone wars (prototype - dec 14, 1994).bin",                                       0x000000, 0x200000,  CRC(cd662de6) SHA1(39341c80532d0888f759169da3622e28f83641f8) )
MEGADRIVE_ROM_LOAD( xmen2k, "x-men 2 - clone wars (prototype - dec 15, 1994).bin",                                       0x000000, 0x1ffe54,  CRC(a5188e30) SHA1(04316a517f50edf7357b44802e4f13144b4c033a) )
MEGADRIVE_ROM_LOAD( xmen2l, "x-men 2 - clone wars (prototype - dec 16, 1994).bin",                                       0x000000, 0x200000,  CRC(e8c40972) SHA1(d2467bec53adfa25ef257e4de11542360cc14c89) )
MEGADRIVE_ROM_LOAD( xmen2m, "x-men 2 - clone wars (prototype - may 06, 1994).bin",                                       0x000000, 0x200000,  CRC(a720ebb4) SHA1(c92510f6b01a0b23b72154d7b390b8be844a05ec) )
MEGADRIVE_ROM_LOAD( xmen2n, "x-men 2 - clone wars (prototype - may 10, 1994).bin",                                       0x000000, 0x200000,  CRC(9b5dd185) SHA1(59855bc241f8882e95638a36f1a6b73bd754782a) )
MEGADRIVE_ROM_LOAD( xmen2o, "x-men 2 - clone wars (prototype - nov 17, 1994).bin",                                       0x000000, 0x200000,  CRC(d90cd0de) SHA1(8318d99432c735b7f202b99f400b5f462ce019cb) )
MEGADRIVE_ROM_LOAD( xmen2p, "x-men 2 - clone wars (prototype - nov 23, 1994).bin",                                       0x000000, 0x200000,  CRC(4386a381) SHA1(e5b4f11cda43f88975e6fbf231774f30960e1ee4) )
MEGADRIVE_ROM_LOAD( xmen2q, "x-men 2 - clone wars (prototype - nov 28, 1994).bin",                                       0x000000, 0x200000,  CRC(70383a4d) SHA1(e49cc2d205193c249a563cd4e2ca6371cf764e06) )
MEGADRIVE_ROM_LOAD( xmen2r, "x-men 2 - clone wars (prototype - nov 30, 1994).bin",                                       0x000000, 0x200000,  CRC(331dae0a) SHA1(aafcf6782d723abefff14e31ff56c82105cfafed) )
MEGADRIVE_ROM_LOAD( xmen2s, "x-men 2 - clone wars (prototype - oct 18, 1994).bin",                                       0x000000, 0x200000,  CRC(52cff37d) SHA1(ab2e816ba2fd525ef5c481088acc440714f739db) )
MEGADRIVE_ROM_LOAD( xmen2, "x-men 2 - clone wars (usa, europe).bin",                                                     0x000000, 0x200000,  CRC(710bc628) SHA1(61409e6cf6065ab67d8952b891d8edcf47777193) )
MEGADRIVE_ROM_LOAD( xpertp, "x-perts (prototype).bin",                                                                   0x000000, 0x400000,  CRC(9d067249) SHA1(97a2fef09c0a43a46d7f4b4c7b7291d87474dcf0) )
MEGADRIVE_ROM_LOAD( xpert, "x-perts (usa).bin",                                                                          0x000000, 0x400000,  CRC(57e8abfd) SHA1(be1df5a63f4363d11d340db9a37ba54db5f5ea38) )
MEGADRIVE_ROM_LOAD( xdrxda, "xdr - x-dazedly-ray (japan).bin",                                                           0x000000, 0x80000,   CRC(ab22d002) SHA1(5c700cd69964645f80a852d308d0f0de22d88f83) )
MEGADRIVE_ROM_LOAD( xenon2m, "xenon 2 megablast (europe).bin",                                                           0x000000, 0x80000,   CRC(59abe7f9) SHA1(055b554f7d61ed671eddc385bce950b2baa32249) )
MEGADRIVE_ROM_LOAD( xiaomo, "xiao monv - magic girl (china) (unlicensed).bin",                                           0x000000, 0x80000,   CRC(8e49a92e) SHA1(22a48404e77cab473ee65e5d5167d17c28884a7a) )
MEGADRIVE_ROM_LOAD( xinqig, "xin qi gai wang zi (china) (alt) (unlicensed).bin",                                         0x000000, 0x400000,  CRC(da5a4bfe) SHA1(75f8003a6388814c1880347882b244549da62158) )
MEGADRIVE_ROM_LOAD( xinqi1, "xin qi gai wang zi (china) (unlicensed).bin",                                               0x000000, 0x400000,  CRC(dd2f38b5) SHA1(4a7494d8601149f43ba7e3595a0b2340cde2e9ba) )
MEGADRIVE_ROM_LOAD( yasech, "ya se chuan shuo (china) (unlicensed).bin",                                                 0x000000, 0x200000,  CRC(095b9a15) SHA1(8fe0806427e123717ba20478ab1410c25fa942e6) )
MEGADRIVE_ROM_LOAD( yangji, "yang jia jiang - yang warrior family (china) (unlicensed).bin",                             0x000000, 0x200000,  CRC(6604a79e) SHA1(6fcc3102fc22b42049e6eae9a1c30c8a7f022d14) )
MEGADRIVE_ROM_LOAD( yogibear, "yogi bear's cartoon capers (europe).bin",                                                 0x000000, 0x100000,  CRC(204f97d8) SHA1(9f235fd4ac2612fb398ecbe423f40c901be8f564) )
MEGADRIVE_ROM_LOAD( yindyh, "young indiana jones - instrument of chaos (prototype - dec 28, 1993).bin",                  0x000000, 0x100000,  CRC(51ca641c) SHA1(e6b8d13344f37aff131edea4f3f3b20100f63e72) )
MEGADRIVE_ROM_LOAD( yindyb, "young indiana jones - instrument of chaos (prototype - dec 28, 1994 - a).bin",              0x000000, 0xff73c,   CRC(f285ad46) SHA1(4f0ef01ada3090d6a4004ec6d46dea5c20e34e1a) )
MEGADRIVE_ROM_LOAD( yindya, "young indiana jones - instrument of chaos (prototype - dec 29, 1994).bin",                  0x000000, 0x100000,  CRC(94b02351) SHA1(10ce37b58d423fdab76e0c53a3b2d379ae9f1221) )
MEGADRIVE_ROM_LOAD( yindyc, "young indiana jones - instrument of chaos (prototype - jan 01, 1994).bin",                  0x000000, 0x100000,  CRC(263f1a4c) SHA1(99cb157adb1b6295ab04955efdc8868ba5ac4d9f) )
MEGADRIVE_ROM_LOAD( yindyd, "young indiana jones - instrument of chaos (prototype - jan 03, 1994).bin",                  0x000000, 0x100000,  CRC(3c46d83d) SHA1(e4afc2d09815495e7d15111a1698e8f324e5873d) )
MEGADRIVE_ROM_LOAD( yindye, "young indiana jones - instrument of chaos (prototype - jan 26, 1994).bin",                  0x000000, 0x100000,  CRC(f3498542) SHA1(e3dc69f339f6ec6e766578edee4cc05f429a689a) )
MEGADRIVE_ROM_LOAD( yindyf, "young indiana jones - instrument of chaos (prototype - jan 27, 1994).bin",                  0x000000, 0x100000,  CRC(f43112c3) SHA1(2174482b62d11a73fd5edff55d111f9bcac53d97) )
MEGADRIVE_ROM_LOAD( yindyg, "young indiana jones - instrument of chaos (prototype - sep 23, 1994 - a).bin",              0x000000, 0xfe3c4,   CRC(14443a29) SHA1(ed02300b73f66b6ca41c282a5b9ab55c30b4c7c8) )
MEGADRIVE_ROM_LOAD( yindy, "young indiana jones chronicles, the (usa) (prototype).bin",                                  0x000000, 0x80000,   CRC(44f6be35) SHA1(1b1a2dc24e4decebc6f1717d387d560a1ae9332c) )
MEGADRIVE_ROM_LOAD( yswand, "ys - wanderers from ys (japan).bin",                                                        0x000000, 0x100000,  CRC(52da4e76) SHA1(28dc01d5dde8d569bae3fafce2af55ee9b836454) )
MEGADRIVE_ROM_LOAD( ysiiiu, "ys iii (usa).bin",                                                                          0x000000, 0x100000,  CRC(ea27976e) SHA1(b3331b41a9a5e2c3f4fb3b64c5b005037a3b6fdd) )
MEGADRIVE_ROM_LOAD( yuusf, "yuu yuu hakusho - makyou toitsusen (japan).bin",                                             0x000000, 0x300000,  CRC(71ceac6f) SHA1(89a70d374a3f3e2dc5b430e6d444bea243193b74) )
MEGADRIVE_ROM_LOAD( yuusfb, "yuu yuu hakusho - sunset fighters (brazil).bin",                                            0x000000, 0x300000,  CRC(fe3fb8ee) SHA1(e307363837626d47060f5347d0fccfc568a12b18) )
MEGADRIVE_ROM_LOAD( yuugai, "yuu yuu hakusho gaiden (japan).bin",                                                        0x000000, 0x200000,  CRC(7dc98176) SHA1(274b86709b509852ca004ceaa244f3ae1d455c50) )
MEGADRIVE_ROM_LOAD( zanyasha, "zan yasha enbukyoku (japan).bin",                                                         0x000000, 0x80000,   CRC(637fe8f3) SHA1(f1ea9a88233b76e3df32526be0b64fb653c13b7d) )
MEGADRIVE_ROM_LOAD( zanygolf, "zany golf (usa, europe) (v1.1).bin",                                                      0x000000, 0x80000,   CRC(74ed7607) SHA1(f1c3b211c91edfed9c96f422cd10633afa43fdf0) )
MEGADRIVE_ROM_LOAD( zanygolf1, "zany golf (usa, europe).bin",                                                            0x000000, 0x80000,   CRC(ed5d12ea) SHA1(4f9bea2d8f489bfbc963718a8dca212e033fb5a2) )
MEGADRIVE_ROM_LOAD( zerokami, "zero the kamikaze squirrel (europe).bin",                                                 0x000000, 0x200000,  CRC(45ff0b4b) SHA1(11a501f5dbde889edcc14e366b67bb75b0c5bc13) )
MEGADRIVE_ROM_LOAD( zerokamiu, "zero the kamikaze squirrel (usa).bin",                                                   0x000000, 0x200000,  CRC(423968df) SHA1(db110fa33c185a7da4c80e92dbe4c7f23ccec0d6) )
MEGADRIVE_ROM_LOAD( zerotol, "zero tolerance (usa, europe).bin",                                                         0x000000, 0x200000,  CRC(23f603f5) SHA1(d6d9733a619ba6be0dd76591d8dec621e4fdc17e) )
MEGADRIVE_ROM_LOAD( zerowing, "zero wing (europe).bin",                                                                  0x000000, 0x100000,  CRC(89b744a3) SHA1(98335b97c5e21f7f8c5436427621836660b91075) )
MEGADRIVE_ROM_LOAD( zerowingj, "zero wing (japan).bin",                                                                  0x000000, 0x100000,  CRC(7e203d2b) SHA1(fad499e21ac55a3b8513110dc1f6e3f6cdeca8dd) )
MEGADRIVE_ROM_LOAD( zhuogu, "zhuo gui da shi - ghost hunter (china) (unlicensed).bin",                                   0x000000, 0x80000,   CRC(76c62a8b) SHA1(3424892e913c20754d2e340c6e79476a9eb6761b) )
MEGADRIVE_ROM_LOAD( zhs, "zombie high (usa) (prototype).bin",                                                            0x000000, 0xfa0ef,   CRC(7bea6194) SHA1(56f1a0b616e178315bdb9f2e43ddc942dcf3780e) )
MEGADRIVE_ROM_LOAD( zombies, "zombies (europe).bin",                                                                     0x000000, 0x100000,  CRC(179a1aa2) SHA1(f0b447196d648b282050a52bdfdd9bf0d8f3d57e) )
MEGADRIVE_ROM_LOAD( zombiesu, "zombies ate my neighbors (usa).bin",                                                      0x000000, 0x100000,  CRC(2bf3626f) SHA1(57090434ee3ffb060b33a1dcce91d1108bf60c47) )
MEGADRIVE_ROM_LOAD( zool, "zool - ninja of the 'nth' dimension (europe).bin",                                            0x000000, 0x100000,  CRC(1ee58b03) SHA1(cab14f63b7d00b35a11a3a7f60cf231199121dc8) )
MEGADRIVE_ROM_LOAD( zoolu, "zool - ninja of the 'nth' dimension (usa).bin",                                              0x000000, 0x100000,  CRC(cb2939f1) SHA1(ee016f127b81f1ca25657e8fa47fac0c4ed15c97) )
MEGADRIVE_ROM_LOAD( zoom, "zoom! (world).bin",                                                                           0x000000, 0x40000,   CRC(724d6965) SHA1(05efc300b3f68496200cb73a2a462a97f930a011) )
MEGADRIVE_ROM_LOAD( zoop, "zoop (europe).bin",                                                                           0x000000, 0x80000,   CRC(2fdac6ab) SHA1(9914956f974b71d24b65e82ae3c980d0eb23c2e5) )
MEGADRIVE_ROM_LOAD( zoopu, "zoop (usa).bin",                                                                             0x000000, 0x80000,   CRC(a899befa) SHA1(e6669f16902caf35337de60027ed2013deed0d40) )
MEGADRIVE_ROM_LOAD( zouzou, "zou! zou! zou! rescue daisakusen (japan).bin",                                              0x000000, 0x80000,   CRC(1a761e67) SHA1(0cf2e2a7f00fb3d7a1a6424152693138ece586f1) )


MEGADRIVE_ROM_LOAD( actrepl,   "action replay (europe) (program).bin",                                                   0x000000, 0x8000,    CRC(95ff7c3e) SHA1(1e0f246826be4ebc7b99bb3f9de7f1de347122e5) )
MEGADRIVE_ROM_LOAD( gamege, "game genie (usa, europe) (program).bin",                                                    0x000000, 0x8000,    CRC(5f293e4c) SHA1(ea4b0418d90bc47996f6788ad455391d07cad6cc) )
MEGADRIVE_ROM_LOAD( gamege1, "game genie (usa, europe) (rev a) (program).bin",                                           0x000000, 0x8000,    CRC(14dbce4a) SHA1(937e1878ebd104f489e6bdbc410a184f79f1144a) )
MEGADRIVE_ROM_LOAD( proact, "pro action replay (europe) (program).bin",                                                  0x000000, 0x20000,   CRC(17255224) SHA1(b0bbfe0c201b2ab2fc17b37e5709785fedc07170) )
MEGADRIVE_ROM_LOAD( proact2a, "pro action replay 2 (europe) (alt) (program).bin",                                        0x000000, 0x10000,   CRC(e6669a61) SHA1(00e4266f3cd7ab2178ca37d564bc5315efdbe967) )
MEGADRIVE_ROM_LOAD( proact2, "pro action replay 2 (europe) (program).bin",                                               0x000000, 0x10000,   CRC(a73abd27) SHA1(c488883a0270766d8fff503fd2e4b5e25a15d523) )



MEGADRIVE_ROM_LOAD( unknown, "unknown chinese game 1 (ch).bin",                                                          0x000000, 0x200000,  CRC(dfacb9ff) SHA1(4283bb9aec05098b9f6b1739e1b02c1bb1f8242f) )


SOFTWARE_LIST_START( megadriv_cart )
	/* Proper dumps */
	SOFTWARE( nhl94, 0, 199?, "Sega License", "NHL '94 (Euro, USA)", 0, 0 )
	SOFTWARE( robocod, 0, 199?, "Sega License", "James Pond II - Codename Robocod (Euro, USA)", 0, 0 )
	SOFTWARE( lotust, 0, 199?, "Sega License", "Lotus Turbo Challenge (Euro, USA)", 0, 0 )
	SOFTWARE( fifa97, 0, 199?, "Electronic Arts", "FIFA Soccer 97 (Euro, USA)", 0, 0 )
	SOFTWARE( riserbot, 0, 199?, "Sega License", "Rise of the Robots (Euro)", 0, 0 )
	SOFTWARE( f22, 0, 199?, "Sega License", "F-22 Interceptor (September 1991) (Euro, USA)", 0, 0 )

	/* Old scrambled/interleaved dumps */
	SOFTWARE( 12inun, 0, 199?, "<unlicensed>", "12-in-1", 0, 0 )

	SOFTWARE( 16ton1, 16ton, 199?, "Sega License", "16 Ton (Jpn, Game no Kandume MegaCD Rip)", 0, 0 )
	SOFTWARE( 16ton, 0, 199?, "Sega License", "16 Ton (Jpn, SegaNet)", 0, 0 )

	SOFTWARE( 16zhan, 0, 199?, "<unlicensed>", "16 Zhang Ma Jiang (China)", 0, 0 )

	SOFTWARE( 3ninja, 0, 199?, "Sega License", "3 Ninjas Kick Back (USA)", 0, 0 )

	SOFTWARE( 3in1fl, 0, 199?, "Sega License", "3-in-1 Flashback - World Champ. Soccer - Tecmo World Cup 92 (pirate)", 0, 0 )

	SOFTWARE( 3in1ro, 0, 199?, "Sega License", "3-in-1 Road Rash - Ms. Pac-Man - Block Out (pirate)", 0, 0 )

	SOFTWARE( 4in1pb, 0, 199?, "Sega License", "4-in-1 (pirate)", 0, 0 )

	SOFTWARE( 6pakus, 0, 199?, "Sega License", "6-Pak (USA)", 0, 0 )

	SOFTWARE( 688atsub, 0, 199?, "Sega License", "688 Attack Sub (Euro, USA)", 0, 0 )

	SOFTWARE( aresshik, 0, 199?, "Sega License", "A Ressha de Ikou MD (Jpn)", 0, 0 )

	SOFTWARE( aaharima, 0, 199?, "Sega License", "Aa Harimanada (Jpn)", 0, 0 )

	SOFTWARE( aaahhrm, 0, 199?, "Sega License", "AAAHH!!! Real Monsters (Euro)", 0, 0 )
	SOFTWARE( aaahhrmu, aaahhrm, 199?, "Sega License", "AAAHH!!! Real Monsters (USA)", 0, 0 )

	SOFTWARE( action52, 0, 199?, "Active Enterprises", "Action 52 (USA)", 0, 0 )
	SOFTWARE( action52a, action52, 199?, "Active Enterprises", "Action 52 (USA, Alt)", 0, 0 )

	SOFTWARE( addfamv,	 addamfam, 199?, "Sega License", "Addams Family Values (Euro)", 0, 0 )

	SOFTWARE( addamfam, 0, 199?, "Sega License", "The Addams Family (Euro, USA)", 0, 0 )
	SOFTWARE( addamfam1, addamfam, 199?, "Sega License", "The Addams Family (USA, Prototype Alt)", 0, 0 )
	SOFTWARE( addamfam2, addamfam, 199?, "Sega License", "The Addams Family (USA, Prototype)", 0, 0 )

	SOFTWARE( advanc, 0, 199?, "Sega License", "Advanced Busterhawk Gleylancer (Jpn)", 0, 0 )

	SOFTWARE( advdai, 0, 199?, "Sega License", "Advanced Daisenryaku - Deutsch Dengeki Sakusen (Jpn, Rev. A)", 0, 0 )

	SOFTWARE( advbatr, 0, 199?, "Sega License", "The Adventures of Batman & Robin (Euro)", 0, 0 )
	SOFTWARE( advbatru, advbatr, 199?, "Sega License", "The Adventures of Batman & Robin (USA)", 0, 0 )
	SOFTWARE( advbatrp1, advbatr, 199?, "Sega License", "The Adventures of Batman & Robin (Prototype, 19950406)", 0, 0 )
	SOFTWARE( advbatrp2, advbatr, 199?, "Sega License", "The Adventures of Batman & Robin (Prototype, 19950410)", 0, 0 )
	SOFTWARE( advbatrp3, advbatr, 199?, "Sega License", "The Adventures of Batman & Robin (Prototype, 19950411)", 0, 0 )
	SOFTWARE( advbatrp4, advbatr, 199?, "Sega License", "The Adventures of Batman & Robin (Prototype, 19950418)", 0, 0 )
	SOFTWARE( advbatrp5, advbatr, 199?, "Sega License", "The Adventures of Batman & Robin (Prototype, 19950419)", 0, 0 )
	SOFTWARE( advbatrp6, advbatr, 199?, "Sega License", "The Adventures of Batman & Robin (Prototype, 19950421)", 0, 0 )
	SOFTWARE( advbatrp7, advbatr, 199?, "Sega License", "The Adventures of Batman & Robin (Prototype, 19950422)", 0, 0 )
	SOFTWARE( advbatrp8, advbatr, 199?, "Sega License", "The Adventures of Batman & Robin (Prototype, 19950424)", 0, 0 )
	SOFTWARE( advbatrp9, advbatr, 199?, "Sega License", "The Adventures of Batman & Robin (Prototype, 19950426)", 0, 0 )
	SOFTWARE( advbatp10, advbatr, 199?, "Sega License", "The Adventures of Batman & Robin (Prototype, 19950427)", 0, 0 )
	SOFTWARE( advbatp11, advbatr, 199?, "Sega License", "The Adventures of Batman & Robin (Prototype, 19950428)", 0, 0 )
	SOFTWARE( advbatp12, advbatr, 199?, "Sega License", "The Adventures of Batman & Robin (Prototype, 19950501)", 0, 0 )

	SOFTWARE( advemm, 0, 199?, "Sega License", "The Adventures of Mighty Max (Euro)", 0, 0 )
	SOFTWARE( advemmu, advemm, 199?, "Sega License", "The Adventures of Mighty Max (USA)", 0, 0 )

	SOFTWARE( adverb, 0, 199?, "Sega License", "The Adventures of Rocky and Bullwinkle and Friends (USA)", 0, 0 )

	SOFTWARE( adveboy, 0, 199?, "<unlicensed>", "Adventurous Boy - Mao Xian Xiao Zi (China)", 0, 0 )

	SOFTWARE( aerothb, aeroth, 199?, "Sega License", "Aero the Acro-Bat (Prototype)", 0, 0 )
	SOFTWARE( aeroth, 0, 199?, "Sega License", "Aero the Acro-Bat (Euro)", 0, 0 )
	SOFTWARE( aerothu, aeroth, 199?, "Sega License", "Aero the Acro-Bat (USA)", 0, 0 )

	SOFTWARE( aero2, 0, 199?, "Sega License", "Aero the Acro-Bat 2 (Euro)", 0, 0 )
	SOFTWARE( aero2u, aero2, 199?, "Sega License", "Aero the Acro-Bat 2 (USA)", 0, 0 )

	SOFTWARE( aerobi, 0, 199?, "Sega License", "Aerobiz (USA)", 0, 0 )
	SOFTWARE( airman, aerobi, 199?, "Sega License", "Air Management - Oozora ni Kakeru (Jpn)", 0, 0 )

	SOFTWARE( aerobs, 0, 199?, "Sega License", "Aerobiz Supersonic (USA)", 0, 0 )
	SOFTWARE( airmas, aerobs, 199?, "Sega License", "Air Management II - Koukuuou o Mezase (Jpn)", 0, 0 )

	SOFTWARE( aftbur2, 0, 199?, "Sega License", "After Burner II (Euro, USA)", 0, 0 )
	SOFTWARE( aftbur2j, abur2, 199?, "Sega License", "After Burner II (Jpn)", 0, 0 )

	SOFTWARE( airbus, 0, 199?, "Sega License", "Air Buster (USA)", 0, 0 )
	SOFTWARE( aerobl, airbus, 199?, "Sega License", "Aero Blasters (Jpn)", 0, 0 )

	SOFTWARE( airdiver, 0, 199?, "Sega License", "Air Diver (USA)", 0, 0 )
	SOFTWARE( airdiverj, airdv, 199?, "Sega License", "Air Diver (Jpn)", 0, 0 )

	SOFTWARE( aladdin, 0, 199?, "Sega License", "Aladdin (Euro)", 0, 0 )
	SOFTWARE( aladdinj, aladdin, 199?, "Sega License", "Aladdin (Jpn)", 0, 0 )
	SOFTWARE( aladdinub, aladdin, 199?, "Sega License", "Aladdin (USA, Prototype)", 0, 0 )
	SOFTWARE( aladdinu, aladdin, 199?, "Sega License", "Aladdin (USA)", 0, 0 )

	SOFTWARE( aladdin2, 0, 199?, "<unlicensed>", "Aladdin II", 0, 0 )

	SOFTWARE( alexkidd, 0, 199?, "Sega License", "Alex Kidd in the Enchanted Castle (Euro, Rev. A)", 0, 0 )
	SOFTWARE( alexkiddk, alexkidd, 199?, "Sega License", "Alex Kidd - Cheongong Maseong (Korea)", 0, 0 )
	SOFTWARE( alexkiddj, alexkidd, 199?, "Sega License", "Alex Kidd - Tenkuu Majou (Jpn)", 0, 0 )
	SOFTWARE( alexkidd1, alexkidd, 199?, "Sega License", "Alex Kidd in the Enchanted Castle (Euro)", 0, 0 )
	SOFTWARE( alexkiddu, alexkidd, 199?, "Sega License", "Alex Kidd in the Enchanted Castle (USA)", 0, 0 )

	SOFTWARE( alien3, 0, 199?, "Sega License", "Alien 3 (Euro, USA, Rev. A)", 0, 0 )
	SOFTWARE( alien31, alien3, 199?, "Sega License", "Alien 3 (Euro, USA)", 0, 0 )

	SOFTWARE( aliens, 0, 199?, "Sega License", "Alien Soldier (Euro)", 0, 0 )
	SOFTWARE( aliensj, aliens, 199?, "Sega License", "Alien Soldier (Jpn)", 0, 0 )

	SOFTWARE( alienstm, 0, 199?, "Sega License", "Alien Storm (World)", 0, 0 )

	SOFTWARE( alisia, 0, 199?, "Sega License", "Alisia Dragoon (Euro)", 0, 0 )
	SOFTWARE( alisiaj, alisia, 199?, "Sega License", "Alisia Dragoon (Jpn)", 0, 0 )
	SOFTWARE( alisiau, alisia, 199?, "Sega License", "Alisia Dragoon (USA)", 0, 0 )

	SOFTWARE( alteredb, 0, 199?, "Sega License", "Altered Beast (Euro, USA)", 0, 0 )
	SOFTWARE( juuouki, alteredb, 199?, "Sega License", "Juuouki (Jpn, v1.1)", 0, 0 )
	SOFTWARE( juuouki1, alteredb, 199?, "Sega License", "Juuouki (Jpn)", 0, 0 )

	SOFTWARE( americ, 0, 199?, "Sega License", "American Gladiators (USA)", 0, 0 )

	SOFTWARE( andreaga, 0, 199?, "Sega License", "Andre Agassi Tennis (Euro)", 0, 0 )
	SOFTWARE( andreagaub,andreaga, 199?, "Sega License", "Andre Agassi Tennis (USA, Prototype)", 0, 0 )
	SOFTWARE( andreagau, andreaga, 199?, "Sega License", "Andre Agassi Tennis (USA)", 0, 0 )

	SOFTWARE( animan, 0, 199?, "Sega License", "Animaniacs (Euro)", 0, 0 )
	SOFTWARE( animanu, animan, 199?, "Sega License", "Animaniacs (USA)", 0, 0 )

	SOFTWARE( anotherw, 0, 199?, "Sega License", "Another World (Euro)", 0, 0 )
	SOFTWARE( ootw, anotherw, 199?, "Sega License", "Out of This World (USA)", 0, 0 )
	SOFTWARE( ootwb, anotherw, 199?, "Sega License", "Out of this World (Prototype)", 0, 0 )

	SOFTWARE( aokioo, 0, 199?, "Sega License", "Aoki Ookami to Shiroki Meshika - Genchou Hishi (Jpn)", 0, 0 )

	SOFTWARE( aqrenk, 0, 199?, "<unlicensed>", "Aq Renkan Awa (China)", 0, 0 )

	SOFTWARE( aquaticg, 0, 199?, "Sega License", "The Aquatic Games Starring James Pond and the Aquabats (Euro, USA)", 0, 0 )

	SOFTWARE( arcadecl, 0, 199?, "Sega License", "Arcade Classics (Euro, USA)", 0, 0 )

	SOFTWARE( archriv, 0, 199?, "Sega License", "Arch Rivals - The Arcade Game (Euro, USA)", 0, 0 )

	SOFTWARE( arcus, 0, 199?, "Sega License", "Arcus Odyssey (USA)", 0, 0 )
	SOFTWARE( arcusj, arcus, 199?, "Sega License", "Arcus Odyssey (Jpn)", 0, 0 )

	SOFTWARE( arielmer, 0, 199?, "Sega License", "Ariel the Little Mermaid (Euro, USA)", 0, 0 )

	SOFTWARE( arnold, 0, 199?, "Sega License", "Arnold Palmer Tournament Golf (Euro, USA)", 0, 0 )

	SOFTWARE( arrow, 0, 199?, "Sega License", "Arrow Flash (World)", 0, 0 )
	SOFTWARE( arrow1, arrow, 199?, "Sega License", "Arrow Flash (World, Alt)", 0, 0 )

	SOFTWARE( artalive, 0, 199?, "Sega License", "Art Alive (World)", 0, 0 )

	SOFTWARE( aof, 0, 199?, "Sega License", "Art of Fighting (Euro)", 0, 0 )
	SOFTWARE( aofp, aof, 199?, "Sega License", "Art of Fighting (Prototype, 19940711)", 0, 0 )
	SOFTWARE( aofu, aof, 199?, "Sega License", "Art of Fighting (USA)", 0, 0 )
	SOFTWARE( ryuuko, aof, 199?, "Sega License", "Ryuuko no Ken (Jpn)", 0, 0 )

	SOFTWARE( asterix, 0, 199?, "Sega License", "Asterix and the Great Rescue (Euro)", 0, 0 )
	SOFTWARE( asterixu, asteri, 199?, "Sega License", "Asterix and the Great Rescue (USA)", 0, 0 )

	SOFTWARE( asterpg, 0, 199?, "Sega License", "Asterix and the Power of the Gods (Euro)", 0, 0 )
	SOFTWARE( asterpb, astepg, 199?, "Sega License", "Asterix and the Power of the Gods (Euro, Prototype)", 0, 0 )

	SOFTWARE( atomrobo, 0, 199?, "Sega License", "Atomic Robo-Kid (USA)", 0, 0 )
	SOFTWARE( atomroboj, arobo, 199?, "Sega License", "Atomic Robo-Kid (Jpn)", 0, 0 )

	SOFTWARE( atomrun, 0, 199?, "Sega License", "Atomic Runner (Euro)", 0, 0 )
	SOFTWARE( atomrunu, atomrun, 199?, "Sega License", "Atomic Runner (USA)", 0, 0 )
	SOFTWARE( chelnov, atomrun, 199?, "Sega License", "Chelnov (Jpn)", 0, 0 )

	SOFTWARE( atptour, 0, 199?, "Sega License", "ATP Tour (Euro)", 0, 0 )
	SOFTWARE( atptouru, atptour, 199?, "Sega License", "ATP Tour Championship Tennis (USA)", 0, 0 )
	SOFTWARE( atptour1, atptour, 199?, "Sega License", "ATP Tour Championship Tennis (Prototype, 19940802)", 0, 0 )
	SOFTWARE( atptour2, atptour, 199?, "Sega License", "ATP Tour Championship Tennis (Prototype, 19940805)", 0, 0 )
	SOFTWARE( atptour3, atptour, 199?, "Sega License", "ATP Tour Championship Tennis (Prototype, 19940808)", 0, 0 )
	SOFTWARE( atptour4, atptour, 199?, "Sega License", "ATP Tour Championship Tennis (Prototype, 19940719)", 0, 0 )
	SOFTWARE( atptour5, atptour, 199?, "Sega License", "ATP Tour Championship Tennis (Prototype, 19940723)", 0, 0 )
	SOFTWARE( atptour6, atptour, 199?, "Sega License", "ATP Tour Championship Tennis (Prototype, 19940725)", 0, 0 )
	SOFTWARE( atptour7, atptour, 199?, "Sega License", "ATP Tour Championship Tennis (Prototype, 19940509)", 0, 0 )
	SOFTWARE( atptour8, atptour, 199?, "Sega License", "ATP Tour Championship Tennis (Prototype, 19940908)", 0, 0 )

	SOFTWARE( austrarl, 0, 199?, "Sega License", "Australian Rugby League (Euro)", 0, 0 )

	SOFTWARE( aweso, 0, 199?, "Sega License", "Awesome Possum (USA)", 0, 0 )
	SOFTWARE( awesob, atomrn, 199?, "Sega License", "Awesome Possum (USA, Prototype)", 0, 0 )

	SOFTWARE( aworgj, 0, 199?, "Sega License", "Aworg (Jpn, SegaNet)", 0, 0 )

	SOFTWARE( awspro, 0, 199?, "Sega License", "AWS Pro Moves Soccer (USA)", 0, 0 )

	SOFTWARE( assmgp, 0, 199?, "Sega License", "Ayrton Senna's Super Monaco GP II (USA)", 0, 0 )
	SOFTWARE( assmgpj, asmgp, 199?, "Sega License", "Ayrton Senna's Super Monaco GP II (Euro, Jpn)", 0, 0 )

	SOFTWARE( bob, 0, 199?, "Sega License", "B.O.B. (Euro, USA)", 0, 0 )
	SOFTWARE( bobb, bob, 199?, "Sega License", "B.O.B. (USA, Prototype)", 0, 0 )
	SOFTWARE( spacef, bob, 199?, "Sega License", "Space Funky B.O.B. (Jpn)", 0, 0 )

	SOFTWARE( babyboom, 0, 199?, "Sega License", "Baby Boom (Prototype, 19940811)", 0, 0 )
	SOFTWARE( babyboom1, babyboom, 199?, "Sega License", "Baby Boom (Prototype, 19940603)", 0, 0 )
	SOFTWARE( babyboom2, babyboom, 199?, "Sega License", "Baby Boom (Prototype, 19940606)", 0, 0 )

	SOFTWARE( babyd, 0, 199?, "Sega License", "Baby's Day Out (Prototype) (USA)", 0, 0 )
	SOFTWARE( babydo, babyd, 199?, "Sega License", "Baby's Day Out (Prototype, Earlier) (USA)", 0, 0 )

	SOFTWARE( backtof3, 0, 199?, "Sega License", "Back to the Future Part III (Euro)", 0, 0 )
	SOFTWARE( backtof3u, backtof3, 199?, "Sega License", "Back to the Future Part III (USA)", 0, 0 )

	SOFTWARE( bahamu, 0, 199?, "Sega License", "Bahamut Senki (Jpn)", 0, 0 )

	SOFTWARE( ballja, 0, 199?, "Sega License", "Ball Jacks (Euro, Jpn)", 0, 0 )

	SOFTWARE( ballz3d, 0, 199?, "Sega License", "Ballz 3D - Fighting at Its Ballziest (Euro, USA)", 0, 0 )

	SOFTWARE( barbiesm, 0, 199?, "Sega License", "Barbie Super Model (USA)", 0, 0 )

	SOFTWARE( barbvac, 0, 199?, "Sega License", "Barbie Vacation Adventure (Prototype) (USA)", 0, 0 )

	SOFTWARE( barkley, 0, 199?, "Sega License", "Barkley Shut Up and Jam! (Euro, USA)", 0, 0 )

	SOFTWARE( barkley2, 0, 199?, "Sega License", "Barkley Shut Up and Jam! 2 (USA)", 0, 0 )
	SOFTWARE( barkley2b, barkley2, 199?, "Sega License", "Barkley Shut Up and Jam! 2 (USA, Prototype)", 0, 0 )

	SOFTWARE( barney, 0, 199?, "Sega License", "Barney's Hide & Seek Game (USA)", 0, 0 )

	SOFTWARE( barver, 0, 199?, "<unlicensed>", "Barver Battle Saga - Tai Kong Zhan Shi (China)", 0, 0 )

	SOFTWARE( bassma, 0, 199?, "Sega License", "Bass Masters Classic (USA)", 0, 0 )

	SOFTWARE( bassmp, 0, 199?, "Sega License", "Bass Masters Classic - Pro Edition (USA)", 0, 0 )

	SOFTWARE( batman, 0, 199?, "Sega License", "Batman (Euro)", 0, 0 )
	SOFTWARE( batmanj, batman, 199?, "Sega License", "Batman (Jpn)", 0, 0 )
	SOFTWARE( batmanu, batman, 199?, "Sega License", "Batman (USA)", 0, 0 )

	SOFTWARE( batmanrj, 0, 199?, "Sega License", "Batman - Revenge of the Joker (USA)", 0, 0 )

	SOFTWARE( batmanfr, 0, 199?, "Sega License", "Batman Forever (World)", 0, 0 )

	SOFTWARE( batmanrt, 0, 199?, "Sega License", "Batman Returns (World)", 0, 0 )

	SOFTWARE( battlyui, 0, 199?, "Sega License", "Battle Golfer Yui (Jpn)", 0, 0 )

	SOFTWARE( battlemd, 0, 199?, "Sega License", "Battle Mania Daiginjou (Jpn, Korea)", 0, 0 )

	SOFTWARE( battlesq, 0, 199?, "Sega License", "Battle Squadron (Euro, USA)", 0, 0 )

	SOFTWARE( battlems, 0, 199?, "Sega License", "Battlemaster (USA)", 0, 0 )

	SOFTWARE( battletc, 0, 199?, "Sega License", "BattleTech - A Game of Armored Combat (USA)", 0, 0 )

	SOFTWARE( btoadd, 0, 199?, "Sega License", "Battletoads & Double Dragon (USA)", 0, 0 )

	SOFTWARE( btoads, 0, 199?, "Sega License", "Battletoads (World)", 0, 0 )

	SOFTWARE( beastw, 0, 199?, "Sega License", "Beast Wrestler (USA)", 0, 0 )
	SOFTWARE( beastwj, beastw, 199?, "Sega License", "Beast Warriors (Jpn)", 0, 0 )

	SOFTWARE( beauty, 0, 199?, "Sega License", "Beauty and the Beast - Belle's Quest (USA)", 0, 0 )

	SOFTWARE( beautyrb, 0, 199?, "Sega License", "Beauty and the Beast - Roar of the Beast (USA)", 0, 0 )

	SOFTWARE( beavis, 0, 199?, "Sega License", "Beavis and Butt-Head (Euro)", 0, 0 )
	SOFTWARE( beavisub, beavis, 199?, "Sega License", "Beavis and Butt-Head (USA, Prototype)", 0, 0 )
	SOFTWARE( beavisu, beavis, 199?, "Sega License", "Beavis and Butt-Head (USA)", 0, 0 )

	SOFTWARE( berens, 0, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (USA)", 0, 0 )
	SOFTWARE( beren1, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940428)", 0, 0 )
	SOFTWARE( beren2, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940429)", 0, 0 )
	SOFTWARE( beren3, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940801)", 0, 0 )
	SOFTWARE( beren4, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940802)", 0, 0 )
	SOFTWARE( beren5, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940803)", 0, 0 )
	SOFTWARE( beren6, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940805)", 0, 0 )
	SOFTWARE( beren7, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940808)", 0, 0 )
	SOFTWARE( beren8, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940709)", 0, 0 )
	SOFTWARE( beren9, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940716)", 0, 0 )
	SOFTWARE( bere10, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940720)", 0, 0 )
	SOFTWARE( bere11, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940602)", 0, 0 )
	SOFTWARE( bere12, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940610)", 0, 0 )
	SOFTWARE( bere13, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940323)", 0, 0 )
	SOFTWARE( bere14, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940506)", 0, 0 )
	SOFTWARE( bere15, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940511)", 0, 0 )
	SOFTWARE( bere16, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940517)", 0, 0 )
	SOFTWARE( bere17, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940519)", 0, 0 )
	SOFTWARE( bere18, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940523)", 0, 0 )
	SOFTWARE( bere19, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940526)", 0, 0 )
	SOFTWARE( bere20, berens, 199?, "Sega License", "The Berenstain Bears' Camping Adventure (Prototype, 19940530)", 0, 0 )

	SOFTWARE( bestof, 0, 199?, "Sega License", "Best of the Best - Championship Karate (Euro)", 0, 0 )
	SOFTWARE( bestofu, bestof, 199?, "Sega License", "Best of the Best - Championship Karate (USA)", 0, 0 )
	SOFTWARE( kickbo, bestof, 199?, "Sega License", "The Kick Boxing (Jpn, Korea)", 0, 0 )

	SOFTWARE( beyondzt, 0, 199?, "Sega License", "Beyond Zero Tolerance (Prototype) (USA)", 0, 0 )

	SOFTWARE( bibleadv, 0, 199?, "Wisdom Tree", "Bible Adventures (USA)", 0, 0 )

	SOFTWARE( billwa, 0, 199?, "Sega License", "Bill Walsh College Football (Euro, USA)", 0, 0 )

	SOFTWARE( billwa95, 0, 199?, "Sega License", "Bill Walsh College Football 95 (USA)", 0, 0 )

	SOFTWARE( bimini, 0, 199?, "Sega License", "Bimini Run (USA)", 0, 0 )

	SOFTWARE( biohz, 0, 199?, "Sega License", "Bio Hazard Battle (Euro, USA)", 0, 0 )
	SOFTWARE( biohzb, biohz, 199?, "Sega License", "Bio Hazard Battle (USA, Prototype)", 0, 0 )
	SOFTWARE( crying, biohz, 199?, "Sega License", "Crying - Aseimei Sensou (Jpn)", 0, 0 )

	SOFTWARE( bishou, 0, 199?, "Sega License", "Bishoujo Senshi Sailor Moon (Jpn)", 0, 0 )

	SOFTWARE( blades, 0, 199?, "Sega License", "Blades of Vengeance (Euro, USA)", 0, 0 )

	SOFTWARE( blast, 0, 199?, "Sega License", "Blaster Master 2 (USA)", 0, 0 )
	SOFTWARE( blastb, blast, 199?, "Sega License", "Blaster Master 2 (USA, Prototype)", 0, 0 )

	SOFTWARE( blockb, 0, 199?, "Sega License", "Blockbuster World Video Game Championship II (USA)", 0, 0 )

	SOFTWARE( blocko, 0, 199?, "Sega License", "Blockout (World)", 0, 0 )

	SOFTWARE( bloods, 0, 199?, "Sega License", "Bloodshot / Battle Frenzy (Euro)", 0, 0 )

	SOFTWARE( bluealma, 0, 199?, "Sega License", "Blue Almanac (Jpn)", 0, 0 )

	SOFTWARE( bodyco, 0, 199?, "Sega License", "Body Count (Euro)", 0, 0 )
	SOFTWARE( bodycob, bodyco, 199?, "Sega License", "Body Count (Euro, Prototype)", 0, 0 )
	SOFTWARE( bodycop, bodyco, 199?, "Sega License", "Body Count (Prototype, 19940208)", 0, 0 )
	SOFTWARE( bodycop1, bodyco, 199?, "Sega License", "Body Count (Prototype, 19940228-U)", 0, 0 )
	SOFTWARE( bodycop2, bodyco, 199?, "Sega License", "Body Count (Prototype, 19940127)", 0, 0 )
	SOFTWARE( bodycop3, bodyco, 199?, "Sega License", "Body Count (Prototype, 19940303)", 0, 0 )
	SOFTWARE( bodycop4, bodyco, 199?, "Sega License", "Body Count (Prototype, 19940308-A)", 0, 0 )
	SOFTWARE( bodycop5, bodyco, 199?, "Sega License", "Body Count (Prototype, 19940308)", 0, 0 )
	SOFTWARE( bodycop6, bodyco, 199?, "Sega License", "Body Count (Prototype, 19940309)", 0, 0 )

	SOFTWARE( bonanz, 0, 199?, "Sega License", "Bonanza Bros. (Euro, Jpn, Rev. A)", 0, 0 )
	SOFTWARE( bonanz1, bonanz, 199?, "Sega License", "Bonanza Bros. (Euro, Jpn)", 0, 0 )
	SOFTWARE( bonanz2, bonanz, 199?, "Sega License", "Bonanza Bros. (Korea, USA)", 0, 0 )

	SOFTWARE( bonkers, 0, 199?, "Sega License", "Bonkers (Euro, USA)", 0, 0 )
	SOFTWARE( bonkersp, bonkers, 199?, "Sega License", "Bonkers (Prototype, 19940328)", 0, 0 )
	SOFTWARE( bonkersp1, bonkers, 199?, "Sega License", "Bonkers (Prototype, 19940503)", 0, 0 )
	SOFTWARE( bonkersp2, bonkers, 199?, "Sega License", "Bonkers (Prototype, 19941004)", 0, 0 )
	SOFTWARE( bonkersp3, bonkers, 199?, "Sega License", "Bonkers (Prototype, 19941025)", 0, 0 )
	SOFTWARE( bonkersp4, bonkers, 199?, "Sega License", "Bonkers (Prototype, 19941029)", 0, 0 )

	SOFTWARE( booger, 0, 199?, "Sega License", "Boogerman - A Pick and Flick Adventure (Euro)", 0, 0 )
	SOFTWARE( boogeru, booger, 199?, "Sega License", "Boogerman - A Pick and Flick Adventure (USA)", 0, 0 )

	SOFTWARE( boxing, 0, 199?, "Sega License", "Boxing Legends of the Ring (USA)", 0, 0 )
	SOFTWARE( chavez, boxing, 199?, "Sega License", "Chavez II (USA)", 0, 0 )

	SOFTWARE( bramst, 0, 199?, "Sega License", "Bram Stoker's Dracula (Euro)", 0, 0 )
	SOFTWARE( bramstu, bramst, 199?, "Sega License", "Bram Stoker's Dracula (USA)", 0, 0 )

	SOFTWARE( bretth, 0, 199?, "Sega License", "Brett Hull Hockey '95 (USA)", 0, 0 )

	SOFTWARE( brianl, 0, 199?, "Sega License", "Brian Lara Cricket (June 1995) (Euro)", 0, 0 )
	SOFTWARE( brianl1, brianl, 199?, "Sega License", "Brian Lara Cricket (March 1995) (Euro)", 0, 0 )

	SOFTWARE( brianl96, 0, 199?, "Sega License", "Brian Lara Cricket 96 (April 1996) (Euro)", 0, 0 )
	SOFTWARE( brianl96a, brianl96, 199?, "Sega License", "Brian Lara Cricket 96 (March 1996) (Euro)", 0, 0 )

	SOFTWARE( brutal, 0, 199?, "Sega License", "Brutal - Paws of Fury (Euro)", 0, 0 )
	SOFTWARE( brutalu, brutal, 199?, "Sega License", "Brutal - Paws of Fury (USA)", 0, 0 )

	SOFTWARE( bubba, 0, 199?, "Sega License", "Bubba'n'Stix (Euro)", 0, 0 )
	SOFTWARE( bubbab, bubba, 199?, "Sega License", "Bubba'n'Stix (Euro, Prototype)", 0, 0 )
	SOFTWARE( bubbau, bubba, 199?, "Sega License", "Bubba'n'Stix - A Strategy Adventure (USA)", 0, 0 )

	SOFTWARE( bubble, 0, 199?, "Sega License", "Bubble and Squeak (Euro)", 0, 0 )
	SOFTWARE( bubbleu, bubble, 199?, "Sega License", "Bubble and Squeak (USA)", 0, 0 )

	SOFTWARE( bubsy, 0, 199?, "Sega License", "Bubsy in Claws Encounters of the Furred Kind (Euro, USA)", 0, 0 )

	SOFTWARE( bubsy2, 0, 199?, "Sega License", "Bubsy II (Euro, USA)", 0, 0 )

	SOFTWARE( buckro, 0, 199?, "Sega License", "Buck Rogers - Countdown to Doomsday (Euro, USA)", 0, 0 )

	SOFTWARE( budokan, 0, 199?, "Sega License", "Budokan - The Martial Spirit (Euro)", 0, 0 )
	SOFTWARE( budokanu, budokan, 199?, "Sega License", "Budokan - The Martial Spirit (USA)", 0, 0 )

	SOFTWARE( bugslife, 0, 199?, "<unlicensed>", "A Bug's Life", 0, 0 )

	SOFTWARE( bugsbun, 0, 199?, "Sega License", "Bugs Bunny in Double Trouble (USA)", 0, 0 )

	SOFTWARE( bullvsbl, 0, 199?, "Sega License", "Bulls versus Blazers and the NBA Playoffs (Euro, USA)", 0, 0 )

	SOFTWARE( bullvsla, 0, 199?, "Sega License", "Bulls Vs Lakers and the NBA Playoffs (Euro, USA)", 0, 0 )

	SOFTWARE( burninf, 0, 199?, "Sega License", "Burning Force (Euro)", 0, 0 )
	SOFTWARE( burninfj, burninf, 199?, "Sega License", "Burning Force (Jpn)", 0, 0 )
	SOFTWARE( burninfu, burninf, 199?, "Sega License", "Burning Force (USA)", 0, 0 )

	SOFTWARE( cadash, 0, 199?, "Sega License", "Cadash (Korea, USA, Asia)", 0, 0 )

	SOFTWARE( caesar, 0, 199?, "Micronet", "Caesar no Yabou (Jpn)", 0, 0 )

	SOFTWARE( caesar2, 0, 199?, "Micronet", "Caesar no Yabou II (Jpn)", 0, 0 )

	SOFTWARE( caesarpl, 0, 199?, "Sega License", "Caesars Palace (USA)", 0, 0 )

	SOFTWARE( calrip, 0, 199?, "Sega License", "Cal Ripken Jr. Baseball (USA)", 0, 0 )

	SOFTWARE( calibe50, 0, 199?, "Sega License", "Caliber .50 (USA)", 0, 0 )

	SOFTWARE( calgames, 0, 199?, "Sega License", "California Games (Euro, USA)", 0, 0 )

	SOFTWARE( cannon, 0, 199?, "Sega License", "Cannon Fodder (Euro)", 0, 0 )

	SOFTWARE( captaven, 0, 199?, "Sega License", "Captain America and the Avengers (Euro)", 0, 0 )
	SOFTWARE( captavenub,captaven, 199?, "Sega License", "Captain America and the Avengers (USA, Prototype)", 0, 0 )
	SOFTWARE( captavenu, captaven, 199?, "Sega License", "Captain America and the Avengers (USA)", 0, 0 )

	SOFTWARE( captplan, 0, 199?, "Sega License", "Captain Planet and the Planeteers (Euro)", 0, 0 )
	SOFTWARE( captplanu, captplan, 199?, "Sega License", "Captain Planet and the Planeteers (USA)", 0, 0 )

	SOFTWARE( castlill, 0, 199?, "Sega License", "Castle of Illusion Starring Mickey Mouse (Euro, USA)", 0, 0 )
	SOFTWARE( castlillj, castlill, 199?, "Sega License", "Castle of Illusion - Fushigi no Oshiro Daibouken (Jpn)", 0, 0 )

	SOFTWARE( cvania, 0, 199?, "Konami", "Castlevania - The New Generation (Euro)", 0, 0 )
	SOFTWARE( cvaniau, cvania, 199?, "Konami", "Castlevania - Bloodlines (USA)", 0, 0 )
	SOFTWARE( cvaniab, cvania, 199?, "Konami", "Castlevania - The New Generation (Euro, Prototype)", 0, 0 )
	SOFTWARE( akumaj, cvania, 199?, "Konami", "Akumajou Dracula - Vampire Killer (Jpn)", 0, 0 )

	SOFTWARE( centur, 0, 199?, "Electronic Arts", "Centurion - Defender of Rome (Euro, USA)", 0, 0 )

	SOFTWARE( chakan, 0, 199?, "Sega License", "Chakan (Euro, USA)", 0, 0 )

	SOFTWARE( champwcs, 0, 199?, "Sega License", "Champions World Class Soccer (World)", 0, 0 )

	SOFTWARE( champbow, 0, 199?, "Sega License", "Championship Bowling (USA)", 0, 0 )
	SOFTWARE( boogie, champbow, 199?, "Sega License", "Boogie Woogie Bowling (Jpn)", 0, 0 )

	SOFTWARE( champl, 0, 199?, "Sega License", "Championship Pool (USA)", 0, 0 )

	SOFTWARE( cproam, 0, 199?, "Sega License", "Championship Pro-Am (USA)", 0, 0 )

	SOFTWARE( chaoji, 0, 199?, "<unlicensed>", "Chao Ji Da Fu Weng (China)", 0, 0 )

	SOFTWARE( chaose, 0, 199?, "Sega License", "The Chaos Engine (Euro)", 0, 0 )
	SOFTWARE( soldfort, chaose, 199?, "Sega License", "Soldiers of Fortune (USA)", 0, 0 )

	SOFTWARE( chaose2p, 0, 199?, "Sega License", "The Chaos Engine 2 (Prototype) (Euro)", 0, 0 )

	SOFTWARE( chasehq2, 0, 199?, "Sega License", "Chase H.Q. II (USA)", 0, 0 )

	SOFTWARE( cheese, 0, 199?, "Sega License", "Cheese Cat-Astrophe Starring Speedy Gonzales (Euro)", 0, 0 )

	SOFTWARE( chess, 0, 199?, "<unlicensed>", "Chess", 0, 0 )	// [!]

	SOFTWARE( chester, 0, 199?, "Sega License", "Chester Cheetah - Too Cool to Fool (USA)", 0, 0 )

	SOFTWARE( chesterw, 0, 199?, "Sega License", "Chester Cheetah - Wild Wild Quest (USA)", 0, 0 )

	SOFTWARE( chichi, 0, 199?, "Sega License", "Chi Chi's Pro Challenge Golf (USA)", 0, 0 )

	SOFTWARE( chibim, 0, 199?, "Sega License", "Chibi Maruko-chan - Wakuwaku Shopping (Jpn)", 0, 0 )

	SOFTWARE( chiki, 0, 199?, "Sega License", "Chiki Chiki Boys (Euro, USA)", 0, 0 )
	SOFTWARE( chikij, chiki, 199?, "Sega License", "Chiki Chiki Boys (Jpn, Korea)", 0, 0 )

	SOFTWARE( chinesec, 0, 199?, "<unlicensed>", "Chinese Chess", 0, 0 )

	SOFTWARE( chinf3, 0, 199?, "<unlicensed>", "Chinese Fighter III", 0, 0 )
	SOFTWARE( chinfb, chinf3, 199?, "<unlicensed>", "Chinese Fighter III (Bootleg)", 0, 0 )

	SOFTWARE( chouky, 0, 199?, "Sega License", "Chou Kyuukai Miracle Nine (Jpn)", 0, 0 )
	SOFTWARE( chouto, 0, 199?, "Sega License", "Chou Touryuu Retsuden Dino Land (Jpn)", 0, 0 )

	SOFTWARE( chuckrck, 0, 199?, "Sega License", "Chuck Rock (Euro)", 0, 0 )
	SOFTWARE( chuckrcku, chuckr, 199?, "Sega License", "Chuck Rock (USA)", 0, 0 )

	SOFTWARE( chukrck2, 0, 199?, "Sega License", "Chuck Rock II - Son of Chuck (Euro)", 0, 0 )
	SOFTWARE( chukrck2j, chukrck2, 199?, "Sega License", "Chuck Rock II (Jpn)", 0, 0 )
	SOFTWARE( chukrck2b, chukrck2, 199?, "Sega License", "Chuck Rock II - Son of Chuck (USA, Prototype)", 0, 0 )
	SOFTWARE( chukrck2u, chukrck2, 199?, "Sega License", "Chuck Rock II - Son of Chuck (USA)", 0, 0 )

	SOFTWARE( chucks, 0, 199?, "Sega License", "Chuck's Excellent Art Tool Animator (USA)", 0, 0 )

	SOFTWARE( classicc, 0, 199?, "Sega License", "Classic Collection (Euro)", 0, 0 )

	SOFTWARE( clay, 0, 199?, "Sega License", "Clay Fighter (Euro)", 0, 0 )
	SOFTWARE( clayu, clay, 199?, "Sega License", "Clay Fighter (USA)", 0, 0 )

	SOFTWARE( cliff, 0, 199?, "Sega License", "Cliffhanger (Euro)", 0, 0 )
	SOFTWARE( cliffu, cliff, 199?, "Sega License", "Cliffhanger (USA)", 0, 0 )
	SOFTWARE( cliffb, cliff, 199?, "Sega License", "Cliffhanger (Prototype)", 0, 0 )

	SOFTWARE( clueus, 0, 199?, "Sega License", "Clue (USA)", 0, 0 )

	SOFTWARE( coachk, 0, 199?, "Sega License", "Coach K College Basketball (USA)", 0, 0 )

	SOFTWARE( colleg96, 0, 199?, "Sega License", "College Football USA 96 (USA)", 0, 0 )

	SOFTWARE( colleg97, 0, 199?, "Sega License", "College Football USA 97 (USA)", 0, 0 )

	SOFTWARE( collnc, 0, 199?, "Sega License", "College Football's National Championship (USA)", 0, 0 )
	SOFTWARE( collncp, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940413)", 0, 0 )
	SOFTWARE( collncp1, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940418)", 0, 0 )
	SOFTWARE( collncp2, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940419)", 0, 0 )
	SOFTWARE( collncp3, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940429)", 0, 0 )
	SOFTWARE( collncp4, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940601)", 0, 0 )
	SOFTWARE( collncp5, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940603)", 0, 0 )
	SOFTWARE( collncp6, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940607)", 0, 0 )
	SOFTWARE( collncp7, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940608)", 0, 0 )
	SOFTWARE( collncp8, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940614)", 0, 0 )
	SOFTWARE( collncp9, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940615)", 0, 0 )
	SOFTWARE( collnp10, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940618)", 0, 0 )
	SOFTWARE( collnp11, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940620)", 0, 0 )
	SOFTWARE( collnp12, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940503)", 0, 0 )
	SOFTWARE( collnp13, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940506)", 0, 0 )
	SOFTWARE( collnp14, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940511-A)", 0, 0 )
	SOFTWARE( collnp15, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940511-A)", 0, 0 )
	SOFTWARE( collnp16, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940520)", 0, 0 )
	SOFTWARE( collnp17, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940525)", 0, 0 )
	SOFTWARE( collnp18, collnc, 199?, "Sega License", "College Football's National Championship (Prototype, 19940531)", 0, 0 )

	SOFTWARE( collnc2, 0, 199?, "Sega License", "College Football's National Championship II (USA)", 0, 0 )

	SOFTWARE( collslam, 0, 199?, "Sega License", "College Slam (USA)", 0, 0 )

	SOFTWARE( columns, 0, 199?, "Sega License", "Columns (World, v1.1)", 0, 0 )
	SOFTWARE( columns1, columns, 199?, "Sega License", "Columns (World)", 0, 0 )

	SOFTWARE( columns3, 0, 199?, "Sega License", "Columns III - Revenge of Columns (USA)", 0, 0 )
	SOFTWARE( columns3a, columns3, 199?, "Sega License", "Columns III - Taiketsu! Columns World (Jpn, Korea)", 0, 0 )

	SOFTWARE( combat, 0, 199?, "Sega License", "Combat Aces (Prototype)", 0, 0 )

	SOFTWARE( combatca, 0, 199?, "Sega License", "Combat Cars (Euro, USA)", 0, 0 )

	SOFTWARE( comix, 0, 199?, "Sega License", "Comix Zone (Euro)", 0, 0 )
	SOFTWARE( comix1, comix, 199?, "Sega License", "Comix Zone (Prototype, 19950628) (Euro)", 0, 0 )
	SOFTWARE( comixj, comix, 199?, "Sega License", "Comix Zone (Jpn)", 0, 0 )
	SOFTWARE( comixk, comix, 199?, "Sega License", "Comix Zone (Prototype, 19950609) (Korea)", 0, 0 )
	SOFTWARE( comixp1, comix, 199?, "Sega License", "Comix Zone (Prototype, 19950712-FULSCR)", 0, 0 )
	SOFTWARE( comixp2, comix, 199?, "Sega License", "Comix Zone (Prototype, 19950712)", 0, 0 )
	SOFTWARE( comixp3, comix, 199?, "Sega License", "Comix Zone (Prototype, 19950601-B)", 0, 0 )
	SOFTWARE( comixp4, comix, 199?, "Sega License", "Comix Zone (Prototype, 19950601-C)", 0, 0 )
	SOFTWARE( comixp5, comix, 199?, "Sega License", "Comix Zone (Prototype, 19950601-D)", 0, 0 )
	SOFTWARE( comixp6, comix, 199?, "Sega License", "Comix Zone (Prototype, 19950601)", 0, 0 )
	SOFTWARE( comixp7, comix, 199?, "Sega License", "Comix Zone (Prototype, 19950602-B)", 0, 0 )
	SOFTWARE( comixp8, comix, 199?, "Sega License", "Comix Zone (Prototype, 19950602)", 0, 0 )
	SOFTWARE( comixp9, comix, 199?, "Sega License", "Comix Zone (Prototype, 19950603)", 0, 0 )
	SOFTWARE( comip10, comix, 199?, "Sega License", "Comix Zone (Prototype, 19950526)", 0, 0 )
	SOFTWARE( comip11, comix, 199?, "Sega License", "Comix Zone (Prototype, 19950530)", 0, 0 )
	SOFTWARE( comixsc, comix, 199?, "Sega License", "Comix Zone (Sega Channel) (Prototype, 19950612)", 0, 0 )
	SOFTWARE( comixub, comix, 199?, "Sega License", "Comix Zone (USA, Prototype)", 0, 0 )
	SOFTWARE( comixu, comix, 199?, "Sega License", "Comix Zone (USA)", 0, 0 )

	SOFTWARE( congo, 0, 199?, "Sega License", "Congo (Unknown) (Prototype)", 0, 0 )

	SOFTWARE( coolsp, 0, 199?, "Sega License", "Cool Spot (Euro)", 0, 0 )
	SOFTWARE( coolspj, coolsp, 199?, "Sega License", "Cool Spot (Jpn, Korea)", 0, 0 )
	SOFTWARE( coolspub, coolsp, 199?, "Sega License", "Cool Spot (USA, Prototype)", 0, 0 )
	SOFTWARE( coolspu, coolsp, 199?, "Sega License", "Cool Spot (USA)", 0, 0 )

	SOFTWARE( corpor, 0, 199?, "Sega License", "Corporation (Euro)", 0, 0 )

	SOFTWARE( cosmic, 0, 199?, "Sega License", "Cosmic Spacehead (Euro, USA)", 0, 0 )

	SOFTWARE( crackd, 0, 199?, "Sega License", "Crack Down (Euro, Jpn, Rev. A)", 0, 0 )
	SOFTWARE( crackd1, crackd, 199?, "Sega License", "Crack Down (Euro, Jpn)", 0, 0 )
	SOFTWARE( crackdu, crackd, 199?, "Sega License", "Crack Down (USA)", 0, 0 )

	SOFTWARE( crayon, 0, 199?, "Sega License", "Crayon Shin-chan - Arashi o Yobu Enji (Jpn)", 0, 0 )

	SOFTWARE( crossf, 0, 199?, "Sega License", "Cross Fire (USA)", 0, 0 )
	SOFTWARE( superair, crossf, 199?, "Sega License", "Super Airwolf (Jpn)", 0, 0 )

	SOFTWARE( crue, 0, 199?, "Sega License", "Crue Ball - Heavy Metal Pinball (Euro, USA)", 0, 0 )
	SOFTWARE( cruej, crue, 199?, "Sega License", "Crue Ball (Jpn)", 0, 0 )
	SOFTWARE( twistedf, crue, 199?, "Sega License", "Twisted Flipper (USA, Prototype)", 0, 0 )

	SOFTWARE( crysta, 0, 199?, "Sega License", "Crystal's Pony Tale (USA)", 0, 0 )
	SOFTWARE( cryst1, crysta, 199?, "Sega License", "Crystal's Pony Tale (Prototype, 19940702)", 0, 0 )
	SOFTWARE( cryst0, crysta, 199?, "Sega License", "Crystal's Pony Tale (Prototype, 19940701)", 0, 0 )
	SOFTWARE( cryst2, crysta, 199?, "Sega License", "Crystal's Pony Tale (Prototype, 19940703)", 0, 0 )
	SOFTWARE( cryst3, crysta, 199?, "Sega License", "Crystal's Pony Tale (Prototype, 19940712-B)", 0, 0 )
	SOFTWARE( cryst4, crysta, 199?, "Sega License", "Crystal's Pony Tale (Prototype, 19940712)", 0, 0 )
	SOFTWARE( cryst5, crysta, 199?, "Sega License", "Crystal's Pony Tale (Prototype, 19940713)", 0, 0 )
	SOFTWARE( cryst6, crysta, 199?, "Sega License", "Crystal's Pony Tale (Prototype, 19940601)", 0, 0 )
	SOFTWARE( cryst7, crysta, 199?, "Sega License", "Crystal's Pony Tale (Prototype, 19940606)", 0, 0 )
	SOFTWARE( cryst8, crysta, 199?, "Sega License", "Crystal's Pony Tale (Prototype, 19940610)", 0, 0 )
	SOFTWARE( cryst9, crysta, 199?, "Sega License", "Crystal's Pony Tale (Prototype, 19940623)", 0, 0 )
	SOFTWARE( crys10, crysta, 199?, "Sega License", "Crystal's Pony Tale (Prototype, 19940628)", 0, 0 )
	SOFTWARE( crys11, crysta, 199?, "Sega License", "Crystal's Pony Tale (Prototype, 19940630)", 0, 0 )
	SOFTWARE( crys12, crysta, 199?, "Sega License", "Crystal's Pony Tale (Prototype, 19940511)", 0, 0 )
	SOFTWARE( crys13, crysta, 199?, "Sega License", "Crystal's Pony Tale (Prototype, 19940519)", 0, 0 )
	SOFTWARE( crys14, crysta, 199?, "Sega License", "Crystal's Pony Tale (Prototype, 19940526)", 0, 0 )

	SOFTWARE( cursej, 0, 199?, "Sega License", "Curse (Jpn)", 0, 0 )

	SOFTWARE( cutiesuz, 0, 199?, "Sega License", "Cutie Suzuki no Ringside Angel (Jpn)", 0, 0 )

	SOFTWARE( cutthr, 0, 199?, "Sega License", "CutThroat Island (Euro, USA)", 0, 0 )

	SOFTWARE( cybercop, 0, 199?, "Sega License", "Cyber-Cop (USA)", 0, 0 )

	SOFTWARE( cyberbal, 0, 199?, "Sega License", "CyberBall (World)", 0, 0 )

	SOFTWARE( cybor, 0, 199?, "Sega License", "Cyborg Justice (Euro, USA)", 0, 0 )
	SOFTWARE( cyborb, cybor, 199?, "Sega License", "Cyborg Justice (Prototype)", 0, 0 )

	SOFTWARE( daffy, 0, 199?, "Sega License", "Daffy Duck in Hollywood (Euro)", 0, 0 )
	SOFTWARE( daffyb, daffy, 199?, "Sega License", "Daffy Duck in Hollywood (Euro, Prototype)", 0, 0 )

	SOFTWARE( dahnam, 0, 199?, "Sega License", "Dahna Megami Tanjou (Jpn)", 0, 0 )
	SOFTWARE( dahnamk, dahnam, 199?, "Sega License", "Dahna (Korea)", 0, 0 )
	SOFTWARE( hercul, dahnam, 199?, "<unlicensed>", "Hercules", 0, 0 )	// [!]

	SOFTWARE( daisen, 0, 199?, "Sega License", "Daisenpuu / Twin Hawk (Euro, Jpn)", 0, 0 )

	SOFTWARE( danger, 0, 199?, "Sega License", "Dangerous Seed (Jpn)", 0, 0 )

	SOFTWARE( darkcast, 0, 199?, "Sega License", "Dark Castle (Euro, USA)", 0, 0 )

	SOFTWARE( darwin, 0, 199?, "Sega License", "Darwin 4081 (Jpn, Korea)", 0, 0 )

	SOFTWARE( dash, 0, 199?, "Sega License", "Dashin' Desperadoes (USA)", 0, 0 )
	SOFTWARE( dashb, dash, 199?, "Sega License", "Dashin' Desperadoes (USA, Prototype)", 0, 0 )
	SOFTWARE( dashba, dash, 199?, "Sega License", "Dashin' Desperadoes (USA, Prototype Alt)", 0, 0 )

	SOFTWARE( davidc, 0, 199?, "Sega License", "David Crane's Amazing Tennis (USA)", 0, 0 )

	SOFTWARE( drsc, 0, 199?, "Sega License", "David Robinson's Supreme Court (Euro, USA)", 0, 0 )
	SOFTWARE( drscj, drsc, 199?, "Sega License", "David Robinson Basketball (Jpn)", 0, 0 )

	SOFTWARE( dcup, 0, 199?, "Sega License", "Davis Cup World Tour (July 1993) (Euro, USA)", 0, 0 )
	SOFTWARE( dcupa, dcup, 199?, "Sega License", "Davis Cup World Tour (June 1993) (Euro, USA)", 0, 0 )

	SOFTWARE( dcup2, 0, 199?, "Sega License", "Davis Cup World Tour Tennis 2 (Prototype 1)", 0, 0 )
	SOFTWARE( dcup2a, dcup2, 199?, "Sega License", "Davis Cup II (USA, Prototype)", 0, 0 )

	SOFTWARE( daze, 0, 199?, "Sega License", "Daze Before Christmas (Oceania)", 0, 0 )
	SOFTWARE( dazeb, daze, 199?, "Sega License", "Daze Before Christmas (Oceania, Prototype)", 0, 0 )

	SOFTWARE( deadly, 0, 199?, "Sega License", "Deadly Moves (USA)", 0, 0 )
	SOFTWARE( powerath, deadly, 199?, "Sega License", "Power Athlete (Jpn, Korea)", 0, 0 )

	SOFTWARE( deathret, 0, 199?, "Sega License", "The Death and Return of Superman (USA)", 0, 0 )

	SOFTWARE( deathdl, 0, 199?, "Sega License", "Death Duel (USA)", 0, 0 )

	SOFTWARE( decapatt, 0, 199?, "Sega License", "DecapAttack (Euro, Korea, USA)", 0, 0 )

	SOFTWARE( demo, 0, 199?, "Sega License", "Demolition Man (Euro, USA)", 0, 0 )
	SOFTWARE( demob, demo, 199?, "Sega License", "Demolition Man (USA, Prototype)", 0, 0 )

	SOFTWARE( desert, 0, 199?, "Sega License", "Desert Demolition Starring Road Runner and Wile E. Coyote (Euro, USA)", 0, 0 )
	SOFTWARE( desert0, desert, 199?, "Sega License", "Desert Demolition (Prototype, 19941206)", 0, 0 )
	SOFTWARE( desert1, desert, 199?, "Sega License", "Desert Demolition (Prototype, 19941208)", 0, 0 )
	SOFTWARE( desert2, desert, 199?, "Sega License", "Desert Demolition (Prototype, 19941212-B)", 0, 0 )
	SOFTWARE( desert3, desert, 199?, "Sega License", "Desert Demolition (Prototype, 19941212)", 0, 0 )
	SOFTWARE( desert4, desert, 199?, "Sega License", "Desert Demolition (Prototype, 19941213)", 0, 0 )
	SOFTWARE( desert5, desert, 199?, "Sega License", "Desert Demolition (Prototype, 19941214)", 0, 0 )
	SOFTWARE( desert6, desert, 199?, "Sega License", "Desert Demolition (Prototype, 19941215)", 0, 0 )
	SOFTWARE( desert7, desert, 199?, "Sega License", "Desert Demolition (Prototype, 19941216)", 0, 0 )
	SOFTWARE( desert8, desert, 199?, "Sega License", "Desert Demolition (Prototype, 19941219)", 0, 0 )

	SOFTWARE( dstrike, 0, 199?, "Sega License", "Desert Strike (Euro, USA)", 0, 0 )
	SOFTWARE( dstrikej, dstrike, 199?, "Sega License", "Desert Strike (Jpn, Korea)", 0, 0 )

	SOFTWARE( devilc, 0, 199?, "Sega License", "Devil Crash MD (Jpn)", 0, 0 )

	SOFTWARE( devilish, 0, 199?, "Sega License", "Devilish - The Next Possession (USA)", 0, 0 )
	SOFTWARE( badomen, devilish, 199?, "Sega License", "Bad Omen (Jpn, Korea)", 0, 0 )

	SOFTWARE( dialqo, 0, 199?, "<unlicensed>", "Dial Q o Mawase! (Jpn)", 0, 0 )

	SOFTWARE( dicktr, 0, 199?, "Sega License", "Dick Tracy (World)", 0, 0 )

	SOFTWARE( dickvi, 0, 199?, "Sega License", "Dick Vitale's 'Awesome, Baby!' College Hoops (USA)", 0, 0 )

	SOFTWARE( dinodini, 0, 199?, "Sega License", "Dino Dini's Soccer (Euro)", 0, 0 )

	SOFTWARE( dinoland, 0, 199?, "Sega License", "Dino Land (USA)", 0, 0 )

	SOFTWARE( dinotale, 0, 199?, "Sega License", "A Dinosaur's Tale (USA)", 0, 0 )

	SOFTWARE( dinohire, 0, 199?, "Sega License", "Dinosaurs for Hire (USA)", 0, 0 )
	SOFTWARE( dinohirea, dinohire, 199?, "Sega License", "Dinosaurs for Hire (Prototype, 19930426)", 0, 0 )
	SOFTWARE( dinohireb, dinohire, 199?, "Sega License", "Dinosaurs for Hire (Prototype, 19930427)", 0, 0 )
	SOFTWARE( dinohirec, dinohire, 199?, "Sega License", "Dinosaurs for Hire (Prototype, 19930502)", 0, 0 )

	SOFTWARE( disney, 0, 199?, "Sega License", "The Disney Collection (Euro)", 0, 0 )

	SOFTWARE( divine, 0, 199?, "<unlicensed>", "Divine Sealing (Jpn)", 0, 0 )

	SOFTWARE( djboy, 0, 199?, "Sega License", "DJ Boy (Euro)", 0, 0 )
	SOFTWARE( djboyj, djboy, 199?, "Sega License", "DJ Boy (Jpn)", 0, 0 )
	SOFTWARE( djboyu, djboy, 199?, "Sega License", "DJ Boy (USA)", 0, 0 )

	SOFTWARE( dokidoki, 0, 199?, "Sega License", "Doki Doki Penguin Land MD (Jpn, Game no Kandume MegaCD Rip)", 0, 0 )
	SOFTWARE( ikazus, dokidoki, 199?, "Sega License", "Ikazuse! Koi no Doki Doki Penguin Land MD (Jpn, SegaNet)", 0, 0 )

	SOFTWARE( domino, 0, 199?, "<unlicensed>", "Domino", 0, 0 )

	SOFTWARE( dominus, 0, 199?, "Sega License", "Dominus (Prototype) (USA)", 0, 0 )

	SOFTWARE( mauimall, 0, 199?, "Sega License", "Donald in Maui Mallard (Euro)", 0, 0 )

	SOFTWARE( donggu, 0, 199?, "Sega License", "Dong Gu Ri Te Chi Jak Jeon (Korea)", 0, 0 )

	SOFTWARE( doomtr, 0, 199?, "Sega License", "Doom Troopers - The Mutant Chronicles (USA)", 0, 0 )

	SOFTWARE( doraemon, 0, 199?, "Sega License", "Doraemon - Yume Dorobou to 7 Nin no Gozans (Jpn)", 0, 0 )

	SOFTWARE( doublecl, 0, 199?, "Sega License", "Double Clutch (Euro)", 0, 0 )

	SOFTWARE( ddragon, 0, 199?, "Sega License", "Double Dragon (Euro, USA)", 0, 0 )

	SOFTWARE( ddragon2, 0, 199?, "Sega License", "Double Dragon II - The Revenge (Jpn)", 0, 0 )

	SOFTWARE( ddragon3, 0, 199?, "Sega License", "Double Dragon 3 - The Arcade Game (Euro, USA)", 0, 0 )

	SOFTWARE( ddragon5, 0, 199?, "Sega License", "Double Dragon V - The Shadow Falls (USA)", 0, 0 )

	SOFTWARE( ddribble, 0, 199?, "Sega License", "Double Dribble - The Playoff Edition (USA)", 0, 0 )

	SOFTWARE( drrobotn, 0, 199?, "Sega License", "Dr. Robotnik's Mean Bean Machine (Euro)", 0, 0 )
	SOFTWARE( drrobotn1, drrobotn, 199?, "Sega License", "Dr. Robotnik's Mean Bean Machine (USA, Prototype)", 0, 0 )
	SOFTWARE( drrobotnu, drrobotn, 199?, "Sega License", "Dr. Robotnik's Mean Bean Machine (USA)", 0, 0 )

	SOFTWARE( dragon, 0, 199?, "Sega License", "Dragon - The Bruce Lee Story (Euro)", 0, 0 )
	SOFTWARE( dragonu, dragon, 199?, "Sega License", "Dragon - The Bruce Lee Story (USA)", 0, 0 )

	SOFTWARE( dbz, 0, 199?, "Sega License", "Dragon Ball Z - Buyuu Retsuden (Jpn)", 0, 0 )
	SOFTWARE( dbzf, dbz, 199?, "Sega License", "Dragon Ball Z - L'Appel du Destin (France) (Euro)", 0, 0 )

	SOFTWARE( dslayer, 0, 199?, "Sega License", "Dragon Slayer - Eiyuu Densetsu (Jpn)", 0, 0 )
	SOFTWARE( dslayer2, 0, 199?, "Sega License", "Dragon Slayer - Eiyuu Densetsu II (Jpn)", 0, 0 )

	SOFTWARE( shai3, 0, 199?, "Sega License", "Dragon's Eye Plus - Shanghai III (Jpn)", 0, 0 )

	SOFTWARE( dfury, 0, 199?, "Sega License", "Dragon's Fury (Euro, USA)", 0, 0 )

	SOFTWARE( dragrev, 0, 199?, "Sega License", "Dragon's Revenge (Euro, USA)", 0, 0 )

	SOFTWARE( dukenu3d, 0, 199?, "Sega License", "Duke Nukem 3D (Brazil)", 0, 0 )

	SOFTWARE( dune2, 0, 199?, "Sega License", "Dune II - The Battle for Arrakis (Euro)", 0, 0 )
	SOFTWARE( dune2g, dune2, 199?, "Sega License", "Dune II - Kampf um den Wustenplaneten (Germany) (Euro)", 0, 0 )
	SOFTWARE( duneth, dune2, 199?, "Sega License", "Dune - The Battle for Arrakis (USA)", 0, 0 )

	SOFTWARE( ddwares, 0, 199?, "Sega License", "Dungeons & Dragons - Warriors of the Eternal Sun (Euro, USA)", 0, 0 )

	SOFTWARE( dynabr, 0, 199?, "Sega License", "Dyna Brothers (Jpn)", 0, 0 )

	SOFTWARE( dynabr2, 0, 199?, "Sega License", "Dyna Brothers 2 (Jpn)", 0, 0 )

	SOFTWARE( dynaduke, 0, 199?, "Sega License", "Dynamite Duke (World, Rev. A)", 0, 0 )
	SOFTWARE( dynaduke1, dynami, 199?, "Sega License", "Dynamite Duke (World)", 0, 0 )

	SOFTWARE( dhead, 0, 199?, "Sega License", "Dynamite Headdy (Euro, USA)", 0, 0 )
	SOFTWARE( dheadj, dhead, 199?, "Sega License", "Dynamite Headdy (Jpn)", 0, 0 )
	SOFTWARE( dheadja, dhead, 199?, "Sega License", "Dynamite Headdy (Jpn, Prototype)", 0, 0 )
	SOFTWARE( dheadp1, dhead, 199?, "Sega License", "Dynamite Headdy (Prototype, 19940614-CABEZA)", 0, 0 )
	SOFTWARE( dheadp2, dhead, 199?, "Sega License", "Dynamite Headdy (Prototype, 19940615)", 0, 0 )
	SOFTWARE( dheadp3, dhead, 199?, "Sega License", "Dynamite Headdy (Prototype, 19940616)", 0, 0 )
	SOFTWARE( dheadp4, dhead, 199?, "Sega License", "Dynamite Headdy (Prototype, 19940622)", 0, 0 )

	SOFTWARE( eahockey, 0, 199?, "Sega License", "EA Hockey (Euro)", 0, 0 )
	SOFTWARE( eahockeyj, eahockey, 199?, "Sega License", "EA Hockey (Jpn)", 0, 0 )

	SOFTWARE( easports, 0, 199?, "Sega License", "EA Sports Double Header (Euro)", 0, 0 )

	SOFTWARE( earnest, 0, 199?, "Sega License", "Earnest Evans (USA)", 0, 0 )

	SOFTWARE( earthdef, 0, 199?, "<unlicensed>", "Earth Defense (USA)", 0, 0 )

	SOFTWARE( ejim, 0, 199?, "Sega License", "Earthworm Jim (Euro)", 0, 0 )
	SOFTWARE( ejimu, ejim, 199?, "Sega License", "Earthworm Jim (USA)", 0, 0 )

	SOFTWARE( ejim2, 0, 199?, "Sega License", "Earthworm Jim 2 (Euro)", 0, 0 )
	SOFTWARE( ejim2u, ejim2, 199?, "Sega License", "Earthworm Jim 2 (USA)", 0, 0 )

	SOFTWARE( ecco, 0, 199?, "Sega License", "Ecco the Dolphin (Euro, Korea, USA)", 0, 0 )
	SOFTWARE( eccoj, ecco, 199?, "Sega License", "Ecco the Dolphin (Jpn)", 0, 0 )

	SOFTWARE( ecco2, 0, 199?, "Sega License", "Ecco - The Tides of Time (Euro)", 0, 0 )
	SOFTWARE( ecco2b, ecco2, 199?, "Sega License", "Ecco - The Tides of Time (Prototype)", 0, 0 )
	SOFTWARE( ecco2x, ecco2, 199?, "Sega License", "Ecco - The Tides of Time (Prototype X11, 19940413)", 0, 0 )
	SOFTWARE( ecco2p, ecco2, 199?, "Sega License", "Ecco - The Tides of Time (USA, Prototype)", 0, 0 )
	SOFTWARE( ecco2u, ecco2, 199?, "Sega License", "Ecco - The Tides of Time (USA)", 0, 0 )
	SOFTWARE( ecco2j, ecco2, 199?, "Sega License", "Ecco the Dolphin II (Jpn)", 0, 0 )

	SOFTWARE( eccojr, 0, 199?, "Sega License", "Ecco Jr. (February 1995) (Oceania, USA)", 0, 0 )
	SOFTWARE( eccojr1, eccojr, 199?, "Sega License", "Ecco Jr. (March 1995) (Oceania, USA)", 0, 0 )

	SOFTWARE( elvient, 0, 199?, "Sega License", "El Viento (USA)", 0, 0 )
	SOFTWARE( elvientj, elvient, 199?, "Sega License", "El Viento (Jpn)", 0, 0 )

	SOFTWARE( elemast, 0, 199?, "Sega License", "Elemental Master (USA)", 0, 0 )
	SOFTWARE( elemastj, elemast, 199?, "Sega License", "Elemental Master (Jpn)", 0, 0 )

	SOFTWARE( elfwor, 0, 199?, "<unlicensed>", "Elf Wor (China)", 0, 0 )

	SOFTWARE( elimin, 0, 199?, "Sega License", "Eliminate Down (Jpn)", 0, 0 )

	SOFTWARE( elit95, 0, 199?, "Sega License", "Elitserien 95 (Sweden) (Euro)", 0, 0 )

	SOFTWARE( elit96, 0, 199?, "Sega License", "Elitserien 96 (Sweden) (Euro)", 0, 0 )

	SOFTWARE( empire, 0, 199?, "Sega License", "Empire of Steel (Euro)", 0, 0 )
	SOFTWARE( steelemp, empire, 199?, "Sega License", "Steel Empire (USA)", 0, 0 )

	SOFTWARE( tazem, 0, 199?, "Sega License", "Taz in Escape from Mars (Euro)", 0, 0 )
	SOFTWARE( tazemu, tazem, 199?, "Sega License", "Taz in Escape from Mars (USA)", 0, 0 )
	SOFTWARE( tazema, tazem, 199?, "Sega License", "Taz in Escape from Mars (Prototype, 19940418)", 0, 0 )
	SOFTWARE( tazemb, tazem, 199?, "Sega License", "Taz in Escape from Mars (Prototype, 19940602)", 0, 0 )
	SOFTWARE( tazemc, tazem, 199?, "Sega License", "Taz in Escape from Mars (Prototype, 19940607)", 0, 0 )
	SOFTWARE( tazemd, tazem, 199?, "Sega License", "Taz in Escape from Mars (Prototype, 19940610)", 0, 0 )
	SOFTWARE( tazeme, tazem, 199?, "Sega License", "Taz in Escape from Mars (Prototype, 19940614)", 0, 0 )
	SOFTWARE( tazemf, tazem, 199?, "Sega License", "Taz in Escape from Mars (Prototype, 19940618-A)", 0, 0 )
	SOFTWARE( tazemg, tazem, 199?, "Sega License", "Taz in Escape from Mars (Prototype, 19940620)", 0, 0 )
	SOFTWARE( tazemh, tazem, 199?, "Sega License", "Taz in Escape from Mars (Prototype, 19940309)", 0, 0 )
	SOFTWARE( tazemi, tazem, 199?, "Sega License", "Taz in Escape from Mars (Prototype, 19940509)", 0, 0 )
	SOFTWARE( tazemj, tazem, 199?, "Sega License", "Taz in Escape from Mars (Prototype, 19940518)", 0, 0 )
	SOFTWARE( tazemk, tazem, 199?, "Sega License", "Taz in Escape from Mars (Prototype, 19940523)", 0, 0 )

	SOFTWARE( espnba, 0, 199?, "Sega License", "ESPN Baseball Tonight (USA)", 0, 0 )

	SOFTWARE( espnhn, 0, 199?, "Sega License", "ESPN National Hockey Night (USA)", 0, 0 )
	SOFTWARE( espnhna, espnhn, 199?, "Sega License", "ESPN National Hockey Night (USA, Prototype)", 0, 0 )

	SOFTWARE( espnsp, 0, 199?, "Sega License", "ESPN Speed World (USA)", 0, 0 )
	SOFTWARE( espnspb, espnsp, 199?, "Sega License", "ESPN Speed World (USA, Prototype)", 0, 0 )

	SOFTWARE( espnf, 0, 199?, "Sega License", "ESPN Sunday Night NFL (USA)", 0, 0 )
	SOFTWARE( espnfb, espnf, 199?, "Sega License", "ESPN Sunday Night NFL (USA, Prototype)", 0, 0 )

	SOFTWARE( eswatc, 0, 199?, "Sega License", "ESWAT - City Under Siege (USA)", 0, 0 )
	SOFTWARE( cyberp, eswatc, 199?, "Sega License", "Cyber Police ESWAT (Jpn)", 0, 0 )

	SOFTWARE( eternalc, 0, 199?, "Sega License", "Eternal Champions (Euro)", 0, 0 )
	SOFTWARE( eternalcb, eternalc, 199?, "Sega License", "Eternal Champions (Euro, Prototype)", 0, 0 )
	SOFTWARE( eternalcp, eternalc, 199?, "Sega License", "Eternal Champions (Prototype, 19931117) (Jpn)", 0, 0 )
	SOFTWARE( eternalcj, eternalc, 199?, "Sega License", "Eternal Champions (Jpn, Korea)", 0, 0 )
	SOFTWARE( eternalcu, eternalc, 199?, "Sega License", "Eternal Champions (USA)", 0, 0 )

	SOFTWARE( europe, 0, 199?, "Sega License", "European Club Soccer (Euro)", 0, 0 )

	SOFTWARE( evander, 0, 199?, "Sega License", "Evander Holyfield's 'Real Deal' Boxing (World)", 0, 0 )

	SOFTWARE( exmutant, 0, 199?, "Sega License", "Ex-Mutants (Euro, USA)", 0, 0 )

	SOFTWARE( exileu, 0, 199?, "Sega License", "Exile (USA)", 0, 0 )
	SOFTWARE( exilet, exileu, 199?, "Sega License", "Exile - Toki no Hazama e (Jpn)", 0, 0 )

	SOFTWARE( exosqu, 0, 199?, "Sega License", "Exo Squad (Euro)", 0, 0 )
	SOFTWARE( exosq1, exosqu, 199?, "Sega License", "Exo Squad (USA, Prototype)", 0, 0 )
	SOFTWARE( exosq2, exosqu, 199?, "Sega License", "Exo Squad (USA)", 0, 0 )

	SOFTWARE( exodus, 0, 199?, "Wisdom Tree", "Exodus - Journey to the Promised Land (USA)", 0, 0 )

	SOFTWARE( f117ni, 0, 199?, "Sega License", "F-117 Night Storm (Euro, USA)", 0, 0 )
	SOFTWARE( f117st, f117ni, 199?, "Sega License", "F-117 Stealth - Operation Night Storm (Jpn)", 0, 0 )

	SOFTWARE( f15str, 0, 199?, "Sega License", "F-15 Strike Eagle II (Euro)", 0, 0 )
	SOFTWARE( f15str1, f15str, 199?, "Sega License", "F-15 Strike Eagle II (USA, Prototype)", 0, 0 )
	SOFTWARE( f15str2, f15str, 199?, "Sega License", "F-15 Strike Eagle II (USA)", 0, 0 )

	SOFTWARE( f22o, f22, 199?, "Sega License", "F-22 Interceptor (June 1992) (Euro, USA)", 0, 0 )
	SOFTWARE( f22b, f22, 199?, "Sega License", "F-22 Interceptor (Euro, USA, Prototype)", 0, 0 )
	SOFTWARE( f22j, f22, 199?, "Sega License", "F-22 Interceptor (Jpn)", 0, 0 )

	SOFTWARE( f1euro, 0, 199?, "Sega License", "F1 (Euro)", 0, 0 )

	SOFTWARE( f1wc, 0, 199?, "Sega License", "F1 - World Championship Edition (Euro)", 0, 0 )
	SOFTWARE( f1wcb, f1wc, 199?, "Sega License", "F1 - World Championship Edition (Euro, Prototype)", 0, 0 )

	SOFTWARE( f1circ, 0, 199?, "Sega License", "F1 Circus MD (Jpn)", 0, 0 )

	SOFTWARE( faerytal, 0, 199?, "Sega License", "The Faery Tale Adventure (Euro, USA)", 0, 0 )

	SOFTWARE( family, 0, 199?, "Sega License", "Family Feud (USA)", 0, 0 )

	SOFTWARE( fantasia, 0, 199?, "Sega License", "Fantasia (World, Rev. A)", 0, 0 )
	SOFTWARE( fantasia1, fantasia, 199?, "Sega License", "Fantasia (World)", 0, 0 )

	SOFTWARE( fantdz, 0, 199?, "Sega License", "Fantastic Dizzy (Euro, USA)", 0, 0 )
	SOFTWARE( fantdz1, fantdz, 199?, "Sega License", "Fantastic Dizzy (Euro, USA, Alt)", 0, 0 )

	SOFTWARE( fastest1, 0, 199?, "Sega License", "Fastest 1 (Jpn)", 0, 0 )

	SOFTWARE( fatalf, 0, 199?, "Sega License", "Fatal Fury (Euro, Korea)", 0, 0 )
	SOFTWARE( fatalfu, fatalf, 199?, "Sega License", "Fatal Fury (USA)", 0, 0 )
	SOFTWARE( garoud, fatalf, 199?, "Sega License", "Garou Densetsu - Shukumei no Tatakai (Jpn)", 0, 0 )

	SOFTWARE( fatalf2, 0, 199?, "Sega License", "Fatal Fury 2 (Korea, USA)", 0, 0 )
	SOFTWARE( garou2, fatalf2, 199?, "Sega License", "Garou Densetsu 2 - Aratanaru Tatakai (Jpn)", 0, 0 )

	SOFTWARE( fatlab, 0, 199?, "Sega License", "Fatal Labyrinth (Euro, USA)", 0, 0 )
	SOFTWARE( labdeath1, fatlab, 199?, "Sega License", "Shi no Meikyuu - Labyrinth of Death (Jpn, Game no Kandume MegaCD Rip)", 0, 0 )
	SOFTWARE( labdeath, fatlab, 199?, "Sega License", "Shi no Meikyuu - Labyrinth of Death (Jpn, SegaNet)", 0, 0 )

	SOFTWARE( fatalrew, 0, 199?, "Sega License", "Fatal Rewind (Euro, USA)", 0, 0 )
	SOFTWARE( killin, fatalrew, 199?, "Sega License", "The Killing Game Show (Jpn)", 0, 0 )
	SOFTWARE( killin1, fatalrew, 199?, "Sega License", "The Killing Game Show (Jpn, Alt)", 0, 0 )

	SOFTWARE( fengku, 0, 199?, "<unlicensed>", "Feng Kuang Tao Hua Yuan (China)", 0, 0 )

	SOFTWARE( fengsh, 0, 199?, "<unlicensed>", "Feng Shen Ying Jie Chuan (China)", 0, 0 )

	SOFTWARE( ferias, 0, 199?, "Sega License", "Ferias Frustradas do Pica-Pau (Brazil)", 0, 0 )

	SOFTWARE( ferr, 0, 199?, "Sega License", "Ferrari Grand Prix Challenge (Euro, Rev. A)", 0, 0 )
	SOFTWARE( ferrb, ferr, 199?, "Sega License", "Ferrari Grand Prix Challenge (Prototype)", 0, 0 )
	SOFTWARE( ferru, ferr, 199?, "Sega License", "Ferrari Grand Prix Challenge (USA)", 0, 0 )

	SOFTWARE( feverp, 0, 199?, "Sega License", "Fever Pitch Soccer (Euro)", 0, 0 )
	SOFTWARE( headon, feverp, 199?, "Sega License", "Head-On Soccer (USA)", 0, 0 )

	SOFTWARE( fidodido, 0, 199?, "Sega License", "Fido Dido (Prototype) (USA)", 0, 0 )

	SOFTWARE( fifa, 0, 199?, "Electronic Arts", "FIFA International Soccer (Euro, USA)", 0, 0 )

	SOFTWARE( fifa95, 0, 199?, "Electronic Arts", "FIFA Soccer 95 (Euro, USA)", 0, 0 )
	SOFTWARE( fifa95k, fifa95, 199?, "Electronic Arts", "FIFA Soccer 95 (Korea)", 0, 0 )
	SOFTWARE( futbol, fifa95, 199?, "<unlicensed>", "Futbol Argentino 98 - Pasion de Multitudes", 0, 0 )

	SOFTWARE( fifa96, 0, 199?, "Electronic Arts", "FIFA Soccer 96 (Euro, USA)", 0, 0 )
	SOFTWARE( fifa99, fifa96, 199?, "<unlicensed>", "FIFA Soccer 99 (Russia)", 0, 0 )

	SOFTWARE( fifa2k, fifa97, 199?, "<unlicensed>", "FIFA Soccer 2000 Gold Edition", 0, 0 )

	SOFTWARE( fifa98, 0, 199?, "Electronic Arts", "FIFA 98 - Road to World Cup (Euro)", 0, 0 )

	SOFTWARE( fightm, 0, 199?, "Sega License", "Fighting Masters (USA)", 0, 0 )
	SOFTWARE( fightmj, fightm, 199?, "Sega License", "Fighting Masters (Jpn, Korea)", 0, 0 )

	SOFTWARE( firemust, 0, 199?, "Sega License", "Fire Mustang (Jpn)", 0, 0 )

	SOFTWARE( firesh, 0, 199?, "Sega License", "Fire Shark (Euro)", 0, 0 )
	SOFTWARE( fireshu, firesh, 199?, "Sega License", "Fire Shark (USA)", 0, 0 )
	SOFTWARE( fireshu1, firesh, 199?, "Sega License", "Fire Shark (USA, Alt)", 0, 0 )
	SOFTWARE( samesame, firesh, 199?, "Sega License", "Same! Same! Same! (Jpn)", 0, 0 )

	SOFTWARE( flashb, 0, 199?, "Sega License", "Flashback (Euro, Rev. A)", 0, 0 )
	SOFTWARE( flashbj, flashb, 199?, "Sega License", "Flashback (Jpn)", 0, 0 )
	SOFTWARE( flashbu1, flashb, 199?, "Sega License", "Flashback - The Quest for Identity (USA, Alt)", 0, 0 )
	SOFTWARE( flashbu, flashb, 199?, "Sega License", "Flashback - The Quest for Identity (USA)", 0, 0 )

	SOFTWARE( flicky, 0, 199?, "Sega License", "Flicky (Euro, USA)", 0, 0 )

	SOFTWARE( flink, 0, 199?, "Sega License", "The Misadventures of Flink (Euro)", 0, 0 )

	SOFTWARE( flint, 0, 199?, "Sega License", "The Flintstones (Euro)", 0, 0 )
	SOFTWARE( flintu, flint, 199?, "Sega License", "The Flintstones (USA)", 0, 0 )
	SOFTWARE( flintj, flint, 199?, "Sega License", "Flintstone (Jpn)", 0, 0 )

	SOFTWARE( foreman, 0, 199?, "Sega License", "Foreman for Real (World)", 0, 0 )

	SOFTWARE( forgot, 0, 199?, "Sega License", "Forgotten Worlds (World, v1.1)", 0, 0 )
	SOFTWARE( forgot1, forgot, 199?, "Sega License", "Forgotten Worlds (World)", 0, 0 )

	SOFTWARE( formula1, 0, 199?, "Sega License", "Formula One (USA)", 0, 0 )

	SOFTWARE( frankt, 0, 199?, "Sega License", "Frank Thomas Big Hurt Baseball (Euro, USA)", 0, 0 )

	SOFTWARE( frogger, 0, 199?, "Sega License", "Frogger (USA)", 0, 0 )

	SOFTWARE( slamdunk, 0, 199?, "Sega License", "From TV Animation Slam Dunk - Kyougou Makkou Taiketsu! (Jpn)", 0, 0 )

	SOFTWARE( funnga, 0, 199?, "Sega License", "Fun 'N' Games (Euro)", 0, 0 )
	SOFTWARE( funngau, funnga, 199?, "Sega License", "Fun 'N' Games (USA)", 0, 0 )

	SOFTWARE( funcar, 0, 199?, "Sega License", "Fun Car Rally (Prototype) (USA)", 0, 0 )

	SOFTWARE( funnywld, 0, 199?, "<unlicensed>", "Funny World & Balloon Boy (USA)", 0, 0 )

	SOFTWARE( fushigi, 0, 199?, "Namcot", "Fushigi no Umi no Nadia (Jpn)", 0, 0 )

	SOFTWARE( fzsenk, 0, 199?, "Sega License", "FZ Senki Axis / Final Zone (Jpn, USA)", 0, 0 )

	SOFTWARE( gloc, 0, 199?, "Sega License", "G-LOC Air Battle (World)", 0, 0 )
	SOFTWARE( glocb, gloc, 199?, "Sega License", "G-LOC Air Battle (World, Prototype)", 0, 0 )

	SOFTWARE( gadget, 0, 199?, "Sega License", "Gadget Twins (USA)", 0, 0 )

	SOFTWARE( gaiares, 0, 199?, "Sega License", "Gaiares (Jpn, USA)", 0, 0 )

	SOFTWARE( gaingrnd, 0, 199?, "Sega License", "Gain Ground (World)", 0, 0 )
	SOFTWARE( gaingrnd1, gaingr, 199?, "Sega License", "Gain Ground (World, Alt)", 0, 0 )

	SOFTWARE( galaxyf2, 0, 199?, "Sega License", "Galaxy Force II (World, Rev. B)", 0, 0 )
	SOFTWARE( galaxyf2a, galaxy, 199?, "Sega License", "Galaxy Force II (World)", 0, 0 )

	SOFTWARE( gamblerj, 0, 199?, "Sega License", "Gambler Jiko Chuushinha - Katayama Masayuki no Mahjong Doujou (Jpn)", 0, 0 )

	SOFTWARE( gameno, 0, 199?, "Sega License", "Game no Kandume Otokuyou (Jpn)", 0, 0 )

	SOFTWARE( garfield, 0, 199?, "Sega License", "Garfield - Caught in the Act (Euro, USA)", 0, 0 )

	SOFTWARE( gargoyle, 0, 199?, "Sega License", "Gargoyles (USA)", 0, 0 )

	SOFTWARE( gaunt, 0, 199?, "Sega License", "Gauntlet IV (September 1993) (Euro, USA)", 0, 0 )
	SOFTWARE( gaunto, gaunt, 199?, "Sega License", "Gauntlet IV (August 1993) (Euro, USA)", 0, 0 )
	SOFTWARE( gauntj, gaunt, 199?, "Sega License", "Gauntlet (Jpn)", 0, 0 )

	SOFTWARE( gemfire, 0, 199?, "Koei", "Gemfire (USA)", 0, 0 )
	SOFTWARE( royalb, gemfire, 199?, "Koei", "Royal Blood (Jpn)", 0, 0 )

	SOFTWARE( general, 0, 199?, "Electronic Arts", "General Chaos (Euro, USA)", 0, 0 )
	SOFTWARE( generalj, general, 199?, "Electronic Arts", "General Chaos Daikonsen (Jpn)", 0, 0 )

	SOFTWARE( genelost, 0, 199?, "Sega License", "Generations Lost (Euro, USA)", 0, 0 )

	SOFTWARE( genghis, 0, 199?, "Koei", "Genghis Khan II - Clan of the Gray Wolf (USA)", 0, 0 )

	SOFTWARE( georgeko, 0, 199?, "Sega License", "George Foreman's KO Boxing (Euro)", 0, 0 )
	SOFTWARE( georgekou, georgeko, 199?, "Sega License", "George Foreman's KO Boxing (USA)", 0, 0 )

	SOFTWARE( ghostbst, 0, 199?, "Sega License", "Ghostbusters (World, v1.1)", 0, 0 )
	SOFTWARE( ghostbst1, ghostbst, 199?, "Sega License", "Ghostbusters (World)", 0, 0 )

	SOFTWARE( ghouls, 0, 199?, "Sega License", "Ghouls 'n Ghosts (Euro, Korea, USA, Rev. A)", 0, 0 )
	SOFTWARE( ghouls1, ghouls, 199?, "Sega License", "Ghouls 'n Ghosts (Euro, USA)", 0, 0 )
	SOFTWARE( daimakai, ghouls, 199?, "Sega License", "Dai Makaimura (Jpn)", 0, 0 )

	SOFTWARE( global, 0, 199?, "Sega License", "Global Gladiators (Euro)", 0, 0 )
	SOFTWARE( mickm, global, 199?, "Sega License", "Mick & Mack as the Global Gladiators (USA)", 0, 0 )
	SOFTWARE( mickmb, global, 199?, "Sega License", "Mick & Mack as the Global Gladiators (USA, Prototype)", 0, 0 )

	SOFTWARE( gods, 0, 199?, "Sega License", "Gods (Euro)", 0, 0 )
	SOFTWARE( godsj, gods, 199?, "Sega License", "Gods (Jpn)", 0, 0 )
	SOFTWARE( godsub, gods, 199?, "Sega License", "Gods (USA, Prototype)", 0, 0 )
	SOFTWARE( godsu, gods, 199?, "Sega License", "Gods (USA)", 0, 0 )

	SOFTWARE( golden10, 0, 199?, "<unlicensed>", "Golden 10-in-1", 0, 0 ) // this contains only the first bank, so most of the games are missing

	SOFTWARE( gaxe, 0, 199?, "Sega License", "Golden Axe (World, v1.1)", 0, 0 )
	SOFTWARE( gaxea, gaxe, 199?, "Sega License", "Golden Axe (World)", 0, 0 )

	SOFTWARE( gaxe2, 0, 199?, "Sega License", "Golden Axe II (World)", 0, 0 )
	SOFTWARE( gaxe2b, gaxe2, 199?, "Sega License", "Golden Axe II (World, Prototype)", 0, 0 )

	SOFTWARE( gaxe3, 0, 199?, "Sega License", "Golden Axe III (Jpn)", 0, 0 )

	SOFTWARE( goofys, 0, 199?, "Sega License", "Goofy's Hysterical History Tour (USA)", 0, 0 )

	SOFTWARE( gouket, 0, 199?, "Sega License", "Gouketsuji Ichizoku (Jpn)", 0, 0 ) // power instinct

	SOFTWARE( granada, 0, 199?, "Sega License", "Granada (Jpn, USA, v1.1)", 0, 0 )
	SOFTWARE( granada1, granada, 199?, "Sega License", "Granada (Jpn, USA)", 0, 0 )

	SOFTWARE( grandslj, grandsl, 199?, "Sega License", "GrandSlam - The Tennis Tournament '92 (Jpn)", 0, 0 )
	SOFTWARE( grandsl, 0, 199?, "Sega License", "GrandSlam - The Tennis Tournament (Euro)", 0, 0 )
	SOFTWARE( jennif, grandsl, 199?, "Sega License", "Jennifer Capriati Tennis (USA)", 0, 0 )

	SOFTWARE( greatc, 0, 199?, "Sega License", "Great Circus Mystery - Mickey to Minnie Magical Adventure 2 (Jpn)", 0, 0 )
	SOFTWARE( greatcu, greatc, 199?, "Sega License", "The Great Circus Mystery Starring Mickey & Minnie (USA)", 0, 0 )

	SOFTWARE( greatw, 0, 199?, "Sega License", "The Great Waldo Search (USA)", 0, 0 )

	SOFTWARE( ghw, 0, 199?, "Sega License", "Greatest Heavyweights (Euro)", 0, 0 )
	SOFTWARE( ghwj, ghw, 199?, "Sega License", "Greatest Heavyweights (Jpn)", 0, 0 )
	SOFTWARE( ghwu, ghw, 199?, "Sega License", "Greatest Heavyweights (USA)", 0, 0 )

	SOFTWARE( greendog, 0, 199?, "Sega License", "Greendog - The Beached Surfer Dude! (Euro, USA)", 0, 0 )

	SOFTWARE( grinds, 0, 199?, "Sega License", "Grind Stormer (USA)", 0, 0 )
	SOFTWARE( vvjapa, grinds, 199?, "Sega License", "V-Five (Jpn)", 0, 0 )

	SOFTWARE( growlu, 0, 199?, "Sega License", "Growl (USA)", 0, 0 )
	SOFTWARE( runark, growlu, 199?, "Sega License", "Runark (Jpn, Korea)", 0, 0 )

	SOFTWARE( gunship, 0, 199?, "Sega License", "Gunship (Euro)", 0, 0 )

	SOFTWARE( gunstar, 0, 199?, "Sega License", "Gunstar Heroes (Euro)", 0, 0 )
	SOFTWARE( gunstarjs, gunstar, 199?, "Sega License", "Gunstar Heroes (Sample) (Jpn)", 0, 0 )
	SOFTWARE( gunstarj, gunstar, 199?, "Sega License", "Gunstar Heroes (Jpn)", 0, 0 )
	SOFTWARE( gunstaru, gunstar, 199?, "Sega License", "Gunstar Heroes (USA)", 0, 0 )

	SOFTWARE( gynoug, 0, 199?, "Sega License", "Gynoug (Euro)", 0, 0 )
	SOFTWARE( gynougj, gynoug, 199?, "Sega License", "Gynoug (Jpn)", 0, 0 )

	SOFTWARE( harddr, 0, 199?, "Sega License", "Hard Drivin' (World)", 0, 0 )

	SOFTWARE( hard94, 0, 199?, "Sega License", "HardBall '94 (Euro, USA)", 0, 0 )

	SOFTWARE( hard95, 0, 199?, "Sega License", "HardBall '95 (USA)", 0, 0 )

	SOFTWARE( hard3, 0, 199?, "Sega License", "HardBall III (USA)", 0, 0 )

	SOFTWARE( hard, 0, 199?, "Sega License", "HardBall! (USA)", 0, 0 )

	SOFTWARE( haunting, 0, 199?, "Sega License", "Haunting Starring Polterguy (Euro, USA)", 0, 0 )

	SOFTWARE( havoc, 0, 199?, "Sega License", "Capt'n Havoc (Euro)", 0, 0 )
	SOFTWARE( highseas, havoc, 199?, "Sega License", "High Seas Havoc (USA)", 0, 0 )

	SOFTWARE( heavyn, 0, 199?, "Sega License", "Heavy Nova (USA)", 0, 0 )

	SOFTWARE( heavyu, 0, 199?, "Sega License", "Heavy Unit - Mega Drive Special (Jpn)", 0, 0 )

	SOFTWARE( heitao, 0, 199?, "<unlicensed>", "Hei Tao 2 - Super Big 2 (China)", 0, 0 )

	SOFTWARE( hellfire, 0, 199?, "Sega License", "Hellfire (Euro)", 0, 0 )
	SOFTWARE( hellfirej, hellfire, 199?, "Sega License", "Hellfire (Jpn)", 0, 0 )
	SOFTWARE( hellfireu, hellfire, 199?, "Sega License", "Hellfire (USA)", 0, 0 )

	SOFTWARE( herc2, 0, 199?, "<unlicensed>", "Hercules 2", 0, 0 )

	SOFTWARE( herzog, 0, 199?, "Sega License", "Herzog Zwei (Euro, USA)", 0, 0 )
	SOFTWARE( herzogj, herzog, 199?, "Sega License", "Herzog Zwei (Jpn)", 0, 0 )

	SOFTWARE( hitthe, 0, 199?, "Sega License", "Hit the Ice (USA)", 0, 0 )

	SOFTWARE( homea, 0, 199?, "Sega License", "Home Alone (Euro, USA)", 0, 0 )
	SOFTWARE( homeab, homea, 199?, "Sega License", "Home Alone (USA, Prototype)", 0, 0 )

	SOFTWARE( homea2, 0, 199?, "Sega License", "Home Alone 2 - Lost in New York (USA)", 0, 0 )

	SOFTWARE( honoon, 0, 199?, "Sega License", "Honoo no Toukyuuji Dodge Danpei (Jpn)", 0, 0 ) // dodgeball

	SOFTWARE( hookus, 0, 199?, "Sega License", "Hook (USA)", 0, 0 )

	SOFTWARE( huamul, 0, 199?, "<unlicensed>", "Hua Mu Lan - Mulan (China)", 0, 0 )

	SOFTWARE( huanle, 0, 199?, "<unlicensed>", "Huan Le Tao Qi Shu - Smart Mouse (China)", 0, 0 )

	SOFTWARE( humans, 0, 199?, "Sega License", "The Humans (USA)", 0, 0 )

	SOFTWARE( hurric, 0, 199?, "Sega License", "Hurricanes (Euro)", 0, 0 )

	SOFTWARE( hybridf, 0, 199?, "Sega License", "The Hybrid Front (Jpn)", 0, 0 )
	SOFTWARE( hybridfb, hybridf, 199?, "Sega License", "The Hybrid Front (Jpn, Prototype)", 0, 0 )

	SOFTWARE( hyokko, 0, 199?, "Sega License", "Hyokkori Hyoutanjima - Daitouryou wo Mezase! (Jpn)", 0, 0 )

	SOFTWARE( hyperd, 0, 199?, "Sega License", "Hyper Dunk (Euro)", 0, 0 )
	SOFTWARE( hyperdj, hyperd, 199?, "Sega License", "Hyper Dunk - The Playoff Edition (Jpn)", 0, 0 )
	SOFTWARE( hyperdjb, hyperd, 199?, "Sega License", "Hyper Dunk - The Playoff Edition (Jpn, Prototype)", 0, 0 )

	SOFTWARE( hyperm, 0, 199?, "Sega License", "Hyper Marbles (Jpn, SegaNet)", 0, 0 )
	SOFTWARE( hyperma, hypem, 199?, "Sega License", "Hyper Marbles (Jpn, Game no Kandume MegaCD Rip)", 0, 0 )

	SOFTWARE( imgint, 0, 199?, "Sega License", "IMG International Tour Tennis (Euro, USA)", 0, 0 )

	SOFTWARE( immortal, 0, 199?, "Sega License", "The Immortal (Euro, USA)", 0, 0 )
	SOFTWARE( wizard, immortal, 199?, "Sega License", "Wizard of the Immortal (Jpn)", 0, 0 )

	SOFTWARE( incred, 0, 199?, "Sega License", "The Incredible Crash Dummies (Euro, USA)", 0, 0 )
	SOFTWARE( incredb, incred, 199?, "Sega License", "The Incredible Crash Dummies (USA, Prototype)", 0, 0 )

	SOFTWARE( incrhulk, 0, 199?, "Sega License", "The Incredible Hulk (Euro, USA)", 0, 0 )

	SOFTWARE( indycrus, 0, 199?, "Sega License", "Indiana Jones and the Last Crusade (Euro)", 0, 0 )
	SOFTWARE( indycrusu, indycrus, 199?, "Sega License", "Indiana Jones and the Last Crusade (USA)", 0, 0 )

	SOFTWARE( indy, 0, 199?, "Sega License", "Indiana Jones' Greatest Adventures (Release Candidate)", 0, 0 )

	SOFTWARE( insectx, 0, 199?, "Sega License", "Insector X (USA)", 0, 0 )
	SOFTWARE( insectxj, insectx, 199?, "Sega License", "Insector X (Jpn, Korea)", 0, 0 )

	SOFTWARE( instch, 0, 199?, "Sega License", "Instruments of Chaos Starring Young Indiana Jones (USA)", 0, 0 )
	SOFTWARE( instchb, instch, 199?, "Sega License", "Instruments of Chaos Starring Young Indiana Jones (USA, Prototype)", 0, 0 )

	SOFTWARE( intrugby, 0, 199?, "Sega License", "International Rugby (Euro)", 0, 0 )

	SOFTWARE( issdx, 0, 199?, "Sega License", "International Superstar Soccer Deluxe (Euro)", 0, 0 )
	SOFTWARE( ronald, issdx, 199?, "Sega License", "Ronaldinho 98 (Brazil)", 0, 0 )

	SOFTWARE( iraqwar, 0, 199?, "<unlicensed>", "Iraq War 2003", 0, 0 )

	SOFTWARE( ishido, 0, 199?, "Sega License", "Ishido - The Way of Stones (USA)", 0, 0 )

	SOFTWARE( itcame, 0, 199?, "Sega License", "It Came from the Desert (USA)", 0, 0 )

	SOFTWARE( itchy, 0, 199?, "Sega License", "The Itchy and Scratchy Game (Prototype) (USA)", 0, 0 )

	SOFTWARE( izzyqst, 0, 199?, "Sega License", "Izzy's Quest for the Olympic Rings (Euro, USA)", 0, 0 )

	SOFTWARE( jlcs, 0, 199?, "Sega License", "J. League Champion Soccer (Jpn)", 0, 0 )

	SOFTWARE( jlpsa, jlps, 199?, "Sega License", "J. League Pro Striker (Jpn, v1.0)", 0, 0 )
	SOFTWARE( jlps, 0, 199?, "Sega License", "J. League Pro Striker (Jpn, v1.3)", 0, 0 )

	SOFTWARE( jlps2, 0, 199?, "Sega License", "J. League Pro Striker 2 (Jpn)", 0, 0 )

	SOFTWARE( jlpsfs, 0, 199?, "Sega License", "J. League Pro Striker Final Stage (Jpn)", 0, 0 )

	SOFTWARE( jlpsp, 0, 199?, "Sega License", "J. League Pro Striker Perfect (Jpn)", 0, 0 )

	SOFTWARE( jacknick, 0, 199?, "Sega License", "Jack Nicklaus' Power Challenge Golf (Euro, USA)", 0, 0 )

	SOFTWARE( jamesb, 0, 199?, "Sega License", "James 'Buster' Douglas Knockout Boxing (Euro, USA)", 0, 0 )
	SOFTWARE( finalb, jamesb, 199?, "Sega License", "Final Blow (Jpn)", 0, 0 )

	SOFTWARE( jb007, 0, 199?, "Sega License", "James Bond 007 - The Duel (Euro, Rev. A)", 0, 0 )
	SOFTWARE( jb007u, jb007, 199?, "Sega License", "James Bond 007 - The Duel (USA)", 0, 0 )
	SOFTWARE( 007shitou, jb007, 199?, "Sega License", "007 Shitou - The Duel (Jpn)", 0, 0 )

	SOFTWARE( jpond, 0, 199?, "Sega License", "James Pond - Underwater Agent (Euro, USA)", 0, 0 )

	SOFTWARE( jpond3, 0, 199?, "Sega License", "James Pond 3 - Operation Starfish (Euro, USA)", 0, 0 )

	SOFTWARE( robocodj, robocod, 199?, "Sega License", "James Pond II - Codename Robocod (Jpn, Korea)", 0, 0 )

	SOFTWARE( jammit, 0, 199?, "Sega License", "Jammit (USA)", 0, 0 )

	SOFTWARE( janout, 0, 199?, "Sega License", "Janou Touryuumon (Jpn)", 0, 0 )
	SOFTWARE( janout1, janout, 199?, "Sega License", "Janou Touryuumon (Jpn, Alt)", 0, 0 )

	SOFTWARE( jantei, 0, 199?, "Telnet Japan / Atlus", "Jantei Monogatari (Jpn)", 0, 0 )

	SOFTWARE( jellyboy, 0, 199?, "Sega License", "Jelly Boy (Prototype) (Euro)", 0, 0 )

	SOFTWARE( jeopardy, 0, 199?, "Sega License", "Jeopardy! (USA)", 0, 0 )

	SOFTWARE( jeopardd, 0, 199?, "Sega License", "Jeopardy! Deluxe (USA)", 0, 0 )

	SOFTWARE( jeopards, 0, 199?, "Sega License", "Jeopardy! Sports Edition (USA)", 0, 0 )

	SOFTWARE( jerryg, 0, 199?, "Sega License", "Jerry Glanville's Pigskin Footbrawl (USA)", 0, 0 )

	SOFTWARE( jewelms, 0, 199?, "Sega License", "Jewel Master (Euro, USA, Rev. A)", 0, 0 )
	SOFTWARE( jewelmsj, jewelms, 199?, "Sega License", "Jewel Master (Jpn)", 0, 0 )

	SOFTWARE( jimpow, 0, 199?, "Sega License", "Jim Power - The Arcade Game (Prototype) (USA)", 0, 0 )	// BAD DUMP

	SOFTWARE( jimmyw, 0, 199?, "Sega License", "Jimmy White's Whirlwind Snooker (Euro)", 0, 0 )

	SOFTWARE( jiujim, 0, 199?, "<unlicensed>", "Jiu Ji Ma Jiang II - Ye Yan Bian (China)", 0, 0 )

	SOFTWARE( joemac, 0, 199?, "Sega License", "Joe & Mac (USA)", 0, 0 )

	SOFTWARE( joemont, 0, 199?, "Sega License", "Joe Montana Football (World)", 0, 0 )

	SOFTWARE( joemont2, 0, 199?, "Sega License", "Joe Montana II Sports Talk Football (World, Rev. A)", 0, 0 )
	SOFTWARE( joemont2a, joemont2, 199?, "Sega License", "Joe Montana II Sports Talk Football (World)", 0, 0 )

	SOFTWARE( madden92, 0, 199?, "Sega License", "John Madden Football '92 (Euro, USA)", 0, 0 )

	SOFTWARE( madden93, 0, 199?, "Sega License", "John Madden Football '93 (Euro, USA)", 0, 0 )

	SOFTWARE( madd93ce, 0, 199?, "Sega License", "John Madden Football '93 - Championship Edition (USA)", 0, 0 )

	SOFTWARE( madden, 0, 199?, "Sega License", "John Madden Football (Euro, USA)", 0, 0 )
	SOFTWARE( maddenj, madden, 199?, "Sega License", "John Madden Football - Pro Football (Jpn)", 0, 0 )

	SOFTWARE( jordanb, 0, 199?, "Sega License", "Jordan Vs Bird (Euro, USA, v1.1)", 0, 0 )
	SOFTWARE( jordanb1, jordan, 199?, "Sega License", "Jordan Vs Bird (Euro, USA)", 0, 0 )
	SOFTWARE( jordanbj, jordan, 199?, "Sega License", "Jordan Vs Bird - One on One (Jpn)", 0, 0 )

	SOFTWARE( joshua, 0, 199?, "Wisdom Tree", "Joshua & The Battle of Jericho (USA)", 0, 0 )

	SOFTWARE( judge, 0, 199?, "Sega License", "Judge Dredd (World)", 0, 0 )
	SOFTWARE( judgeua, judge, 199?, "Sega License", "Judge Dredd (USA, Prototype Alt)", 0, 0 )
	SOFTWARE( judgeub, judge, 199?, "Sega License", "Judge Dredd (USA, Prototype)", 0, 0 )

	SOFTWARE( jujude, 0, 199?, "Sega License", "JuJu Densetsu / Toki - Going Ape Spit (World, Rev. A)", 0, 0 )
	SOFTWARE( jujude1, jujude, 199?, "Sega License", "JuJu Densetsu / Toki - Going Ape Spit (World)", 0, 0 )

	SOFTWARE( junction, 0, 199?, "Sega License", "Junction (Jpn, USA)", 0, 0 )

	SOFTWARE( jungle, 0, 199?, "Sega License", "The Jungle Book (Euro)", 0, 0 )
	SOFTWARE( jungleu, jungle, 199?, "Sega License", "The Jungle Book (USA)", 0, 0 )

	SOFTWARE( jstrike, 0, 199?, "Sega License", "Jungle Strike (Euro, USA)", 0, 0 )
	SOFTWARE( jstrikeb, jstrike, 199?, "Sega License", "Jungle Strike (USA, Prototype)", 0, 0 )
	SOFTWARE( jstrikej, jstrike, 199?, "Sega License", "Jungle Strike - Uketsugareta Kyouki (Jpn)", 0, 0 )

	SOFTWARE( jurass, 0, 199?, "Sega License", "Jurassic Park (Euro)", 0, 0 )
	SOFTWARE( jurassj, jurass, 199?, "Sega License", "Jurassic Park (Jpn)", 0, 0 )
	SOFTWARE( jurassub, jurass, 199?, "Sega License", "Jurassic Park (USA, Prototype)", 0, 0 )
	SOFTWARE( jurassu, jurass, 199?, "Sega License", "Jurassic Park (USA)", 0, 0 )

	SOFTWARE( jprmp, 0, 199?, "Sega License", "Jurassic Park - Rampage Edition (Euro, USA)", 0, 0 )
	SOFTWARE( jprmpa, jprmp, 199?, "Sega License", "Jurassic Park - Rampage Edition (Prototype, 19940708)", 0, 0 )
	SOFTWARE( jprmpb, jprmp, 199?, "Sega License", "Jurassic Park - Rampage Edition (Prototype, 19940713)", 0, 0 )
	SOFTWARE( jprmpc, jprmp, 199?, "Sega License", "Jurassic Park - Rampage Edition (Prototype, 19940714)", 0, 0 )
	SOFTWARE( jprmpd, jprmp, 199?, "Sega License", "Jurassic Park - Rampage Edition (Prototype, 19940715)", 0, 0 )
	SOFTWARE( jprmpe, jprmp, 199?, "Sega License", "Jurassic Park - Rampage Edition (Prototype, 19940717)", 0, 0 )
	SOFTWARE( jprmpf, jprmp, 199?, "Sega License", "Jurassic Park - Rampage Edition (Prototype, 19940718)", 0, 0 )
	SOFTWARE( jprmpg, jprmp, 199?, "Sega License", "Jurassic Park - Rampage Edition (Prototype, 19940620)", 0, 0 )
	SOFTWARE( jprmph, jprmp, 199?, "Sega License", "Jurassic Park - Rampage Edition (Prototype, 19940622)", 0, 0 )
	SOFTWARE( jprmpi, jprmp, 199?, "Sega License", "Jurassic Park - Rampage Edition (Prototype, 19940630)", 0, 0 )

	SOFTWARE( justicel, 0, 199?, "Sega License", "Justice League Task Force (World)", 0, 0 )

	SOFTWARE( kageki, 0, 199?, "Sega License", "Ka-Ge-Ki - Fists of Steel (USA)", 0, 0 )
	SOFTWARE( kagekij, kageki, 199?, "Sega License", "Ka-Ge-Ki (Jpn)", 0, 0 )

	SOFTWARE( kawa, 0, 199?, "Sega License", "Kawasaki Superbike Challenge (Euro, USA)", 0, 0 )
	SOFTWARE( kawab, kawa, 199?, "Sega License", "Kawasaki Superbike Challenge (USA, Prototype)", 0, 0 )

	SOFTWARE( kickoff3, 0, 199?, "Sega License", "Kick Off 3 - European Challenge (Euro)", 0, 0 )

	SOFTWARE( kidcha, 0, 199?, "Sega License", "Kid Chameleon (Euro, USA)", 0, 0 )
	SOFTWARE( chamel, kidcha, 199?, "Sega License", "Chameleon Kid (Jpn)", 0, 0 )

	SOFTWARE( kidouk, 0, 199?, "Sega License", "Kidou Keisatsu Patlabor - 98-Shiki Kidou Seyo! (Jpn)", 0, 0 )

	SOFTWARE( kof98, 0, 199?, "<unlicensed>", "The King of Fighters '98", 0, 0 )	// [!]
	SOFTWARE( kof98a, kof98, 199?, "<unlicensed>", "The King of Fighters '98 (Pirate)", 0, 0 )

	SOFTWARE( kotm, 0, 199?, "Sega License", "King of the Monsters (Euro)", 0, 0 )
	SOFTWARE( kotmu, kotm, 199?, "Sega License", "King of the Monsters (USA)", 0, 0 )

	SOFTWARE( kotm2, 0, 199?, "Sega License", "King of the Monsters 2 (USA)", 0, 0 )

	SOFTWARE( kings, 0, 199?, "Sega License", "King Salmon - The Big Catch (USA)", 0, 0 )
	SOFTWARE( kingsj, kings, 199?, "Sega License", "King Salmon (Jpn)", 0, 0 )

	SOFTWARE( kingsbty, 0, 199?, "Electronic Arts", "King's Bounty - The Conqueror's Quest (Euro, USA)", 0, 0 )

	SOFTWARE( kishid, 0, 199?, "Sega License", "Kishi Densetsu (Jpn)", 0, 0 ) // panzer

	SOFTWARE( kissshot, 0, 199?, "Sega License", "Kiss Shot (Jpn, SegaNet)", 0, 0 )

	SOFTWARE( klax, 0, 199?, "Sega License", "Klax (Euro, USA)", 0, 0 )
	SOFTWARE( klaxj, klax, 199?, "Sega License", "Klax (Jpn)", 0, 0 )

	SOFTWARE( koutet, 0, 199?, "Sega License", "Koutetsu Teikoku (Jpn)", 0, 0 )

	SOFTWARE( krusty, 0, 199?, "Sega License", "Krusty's Super Fun House (Euro, USA, v1.1)", 0, 0 )
	SOFTWARE( krusty1, krusty, 199?, "Sega License", "Krusty's Super Fun House (Euro, USA)", 0, 0 )

	SOFTWARE( kyuuka, 0, 199?, "Namcot", "Kyuukai Douchuuki (Jpn)", 0, 0 )

	SOFTWARE( larussa, 0, 199?, "Sega License", "La Russa Baseball 95 (Oceania, USA)", 0, 0 )

	SOFTWARE( lakers, 0, 199?, "Sega License", "Lakers versus Celtics and the NBA Playoffs (USA)", 0, 0 )

	SOFTWARE( landstlku, landstlk, 199?, "Sega License", "Landstalker (USA)", 0, 0 )
	SOFTWARE( landstlkb, landstlk, 199?, "Sega License", "Landstalker (USA, Prototype)", 0, 0 )
	SOFTWARE( landstlkg, landstlk, 199?, "Sega License", "Landstalker - Die Schatze von Konig Nolo (Germany) (Euro)", 0, 0 )
	SOFTWARE( landstlkj, landstlk, 199?, "Sega License", "Landstalker - Koutei no Zaihou (Jpn)", 0, 0 )
	SOFTWARE( landstlkf, landstlk, 199?, "Sega License", "Landstalker - Le Tresor du Roi Nole (France) (Euro)", 0, 0 )
	SOFTWARE( landstlk, 0, 199?, "Sega License", "Landstalker - The Treasures of King Nole (Euro)", 0, 0 )

	SOFTWARE( langriss, 0, 199?, "Sega License", "Langrisser (Jpn)", 0, 0 )

	SOFTWARE( langris2, 0, 199?, "Sega License", "Langrisser II (Jpn, v1.2)", 0, 0 )
	SOFTWARE( langris2a, langris2, 199?, "Sega License", "Langrisser II (Jpn, v1.1)", 0, 0 )
	SOFTWARE( langris2b, langris2, 199?, "Sega License", "Langrisser II (Jpn)", 0, 0 )

	SOFTWARE( lastact, 0, 199?, "Sega License", "Last Action Hero (Euro, USA)", 0, 0 )

	SOFTWARE( lastbatt, 0, 199?, "Sega License", "Last Battle (Euro, Korea, USA)", 0, 0 )
	SOFTWARE( hokuto, lastbatt, 199?, "Sega License", "Hokuto no Ken - Shin Seikimatsu Kyuuseishu Densetsu (Jpn)", 0, 0 )

	SOFTWARE( lawnmowr, 0, 199?, "Sega License", "The Lawnmower Man (Euro, USA)", 0, 0 )

	SOFTWARE( legend, 0, 199?, "Sega License", "The Legend of Galahad (Euro, USA)", 0, 0 )

	SOFTWARE( lemmings, 0, 199?, "Sega License", "Lemmings (Euro)", 0, 0 )
	SOFTWARE( lemmingsu, lemmings, 199?, "Sega License", "Lemmings (Jpn, Korea, USA, v1.1)", 0, 0 )
	SOFTWARE( lemmingsu1, lemmings, 199?, "Sega License", "Lemmings (Jpn, USA)", 0, 0 )

	SOFTWARE( lemming2, 0, 199?, "Sega License", "Lemmings 2 - The Tribes (Euro)", 0, 0 )
	SOFTWARE( lemming2u, lemming2, 199?, "Sega License", "Lemmings 2 - The Tribes (USA)", 0, 0 )

	SOFTWARE( le, 0, 199?, "Sega License", "Lethal Enforcers (Euro)", 0, 0 )
	SOFTWARE( lej, le, 199?, "Sega License", "Lethal Enforcers (Jpn)", 0, 0 )
	SOFTWARE( leu, le, 199?, "Sega License", "Lethal Enforcers (USA)", 0, 0 )

	SOFTWARE( le2, 0, 199?, "Sega License", "Lethal Enforcers II - Gun Fighters (Euro)", 0, 0 )
	SOFTWARE( le2u, le2, 199?, "Sega License", "Lethal Enforcers II - Gun Fighters (USA)", 0, 0 )

	SOFTWARE( lhx, 0, 199?, "Sega License", "LHX Attack Chopper (Euro, USA)", 0, 0 )
	SOFTWARE( lhxj, lhx, 199?, "Sega License", "LHX Attack Chopper (Jpn)", 0, 0 )

	SOFTWARE( liberty, 0, 199?, "Koei", "Liberty or Death (USA)", 0, 0 )

	SOFTWARE( lightc, 0, 199?, "Sega License", "Light Crusader (Euro)", 0, 0 )
	SOFTWARE( lightcj, lightc, 199?, "Sega License", "Light Crusader (Jpn)", 0, 0 )
	SOFTWARE( lightck, lightc, 199?, "Sega License", "Light Crusader (Korea)", 0, 0 )
	SOFTWARE( lightcp, lightc, 199?, "Sega License", "Light Crusader (Prototype, 19950608)", 0, 0 )
	SOFTWARE( lightcu, lightc, 199?, "Sega License", "Light Crusader (USA)", 0, 0 )

	SOFTWARE( lionkin3, 0, 199?, "<unlicensed>", "Lion King 3", 0, 0 )

	SOFTWARE( lionkin2, 0, 199?, "<unlicensed>", "The Lion King II", 0, 0 )		// [!]
	SOFTWARE( lionkin2a, lionkin2, 199?, "<unlicensed>", "The Lion King II (Pirate)", 0, 0 )

	SOFTWARE( lionking, 0, 199?, "Sega License", "The Lion King (World)", 0, 0 )

	SOFTWARE( lobo, 0, 199?, "Sega License", "Lobo (Prototype) (USA)", 0, 0 )

	SOFTWARE( longch, 0, 199?, "<unlicensed>", "Long (China)", 0, 0 )

	SOFTWARE( lordmo, 0, 199?, "Sega License", "Lord Monarch - Tokoton Sentou Densetsu (Jpn)", 0, 0 )

	SOFTWARE( lostvik, 0, 199?, "Sega License", "The Lost Vikings (Euro)", 0, 0 )
	SOFTWARE( lostvikb, lostvik, 199?, "Sega License", "The Lost Vikings (Euro, Prototype)", 0, 0 )
	SOFTWARE( lostviku, lostvik, 199?, "Sega License", "The Lost Vikings (U.S.A.) (USA)", 0, 0 )

	SOFTWARE( lostwrld, 0, 199?, "Sega License", "The Lost World - Jurassic Park (Euro, USA)", 0, 0 )

	SOFTWARE( lotus2, 0, 199?, "Sega License", "Lotus II (Euro, USA)", 0, 0 )
	SOFTWARE( lotus2b, lot2, 199?, "Sega License", "Lotus II (USA, Prototype)", 0, 0 )

	SOFTWARE( m1abrams, 0, 199?, "Sega License", "M-1 Abrams Battle Tank (Euro, USA)", 0, 0 )

	SOFTWARE( majian, 0, 199?, "<unlicensed>", "Ma Jiang Qing Ren - Ji Ma Jiang Zhi (China)", 0, 0 )

	SOFTWARE( maqiao, 0, 199?, "<unlicensed>", "Ma Qiao E Mo Ta - Devilish Mahjong Tower (China)", 0, 0 )

	SOFTWARE( madden94, 0, 199?, "Sega License", "Madden NFL '94 (Euro, USA)", 0, 0 )

	SOFTWARE( madden95, 0, 199?, "Sega License", "Madden NFL 95 (Euro, USA)", 0, 0 )

	SOFTWARE( madden96, 0, 199?, "Sega License", "Madden NFL 96 (Euro, USA)", 0, 0 )

	SOFTWARE( madden97, 0, 199?, "Sega License", "Madden NFL 97 (Euro, USA)", 0, 0 )

	SOFTWARE( madden98, 0, 199?, "Sega License", "Madden NFL 98 (USA)", 0, 0 )

	SOFTWARE( madoum, 0, 199?, "Sega License", "Madou Monogatari I (Jpn)", 0, 0 )

	SOFTWARE( msb, 0, 199?, "Sega License", "The Magic School Bus (USA)", 0, 0 )
	SOFTWARE( msbpa, msb, 199?, "Sega License", "The Magic School Bus (Prototype, 19950411)", 0, 0 )
	SOFTWARE( msbpb, msb, 199?, "Sega License", "The Magic School Bus (Prototype, 19950421)", 0, 0 )
	SOFTWARE( msbpc, msb, 199?, "Sega License", "The Magic School Bus (Prototype, 19950425)", 0, 0 )
	SOFTWARE( msbpd, msb, 199?, "Sega License", "The Magic School Bus (Prototype, 19950428)", 0, 0 )
	SOFTWARE( msbpe, msb, 199?, "Sega License", "The Magic School Bus (Prototype, 19950202)", 0, 0 )
	SOFTWARE( msbpf, msb, 199?, "Sega License", "The Magic School Bus (Prototype, 19950217)", 0, 0 )
	SOFTWARE( msbpg, msb, 199?, "Sega License", "The Magic School Bus (Prototype, 19950112)", 0, 0 )
	SOFTWARE( msbph, msb, 199?, "Sega License", "The Magic School Bus (Prototype, 19950307)", 0, 0 )
	SOFTWARE( msbpi, msb, 199?, "Sega License", "The Magic School Bus (Prototype, 19950314)", 0, 0 )
	SOFTWARE( msbpj, msb, 199?, "Sega License", "The Magic School Bus (Prototype, 19950327)", 0, 0 )
	SOFTWARE( msbpk, msb, 199?, "Sega License", "The Magic School Bus (Prototype, 19950331)", 0, 0 )
	SOFTWARE( msbpl, msb, 199?, "Sega License", "The Magic School Bus (Prototype, 19950505)", 0, 0 )

	SOFTWARE( mhat, 0, 199?, "Sega License", "Magical Hat no Buttobi Turbo! Daibouken (Jpn)", 0, 0 )

	SOFTWARE( mtaru, 0, 199?, "Sega License", "Magical Taruruuto-kun (Jpn)", 0, 0 )

	SOFTWARE( mahjongc, 0, 199?, "Sega License", "Mahjong Cop Ryuu - Shiro Ookami no Yabou (Jpn)", 0, 0 )

	SOFTWARE( mamono, 0, 199?, "Sega License", "Mamono Hunter Youko - Dai 7 no Keishou (Jpn)", 0, 0 )

	SOFTWARE( manover, 0, 199?, "Sega License", "Man Overboard! (Euro)", 0, 0 )

	SOFTWARE( maoure, 0, 199?, "Sega License", "Maou Renjishi (Jpn)", 0, 0 )

	SOFTWARE( marble, 0, 199?, "Sega License", "Marble Madness (Euro, USA)", 0, 0 )

	SOFTWARE( marioand, 0, 199?, "Sega License", "Mario Andretti Racing (Euro, USA)", 0, 0 )

	SOFTWARE( mariolh, 0, 199?, "Sega License", "Mario Lemieux Hockey (Euro, USA)", 0, 0 )

	SOFTWARE( marko, 0, 199?, "Sega License", "Marko's Magic Football (Euro)", 0, 0 )
	SOFTWARE( markob, marko, 199?, "Sega License", "Marko's Magic Football (Euro, Prototype)", 0, 0 )
	SOFTWARE( markou, marko, 199?, "Sega License", "Marko's Magic Football (USA)", 0, 0 )

	SOFTWARE( marsup, 0, 199?, "Sega License", "Marsupilami (Euro)", 0, 0 )
	SOFTWARE( marsupu, marsup, 199?, "Sega License", "Marsupilami (USA)", 0, 0 )

	SOFTWARE( marysh, 0, 199?, "Sega License", "Mary Shelley's Frankenstein (USA)", 0, 0 )

	SOFTWARE( master, 0, 199?, "Sega License", "Master of Monsters (Jpn)", 0, 0 )
	SOFTWARE( masteru, master, 199?, "Sega License", "Master of Monsters (USA)", 0, 0 )

	SOFTWARE( mweap, 0, 199?, "Sega License", "Master of Weapon (Jpn)", 0, 0 )

	SOFTWARE( maten, 0, 199?, "Sega License", "Maten no Soumetsu (Jpn)", 0, 0 )

	SOFTWARE( mathbl, 0, 199?, "Sega License", "Math Blaster - Episode 1 (USA)", 0, 0 )

	SOFTWARE( mazinw, 0, 199?, "Sega License", "Mazin Wars (Euro)", 0, 0 )
	SOFTWARE( mazins, mazinw, 199?, "Sega License", "Mazin Saga (Asia)", 0, 0 )
	SOFTWARE( mazins1, mazinw, 199?, "Sega License", "Mazin Saga (Jpn, Korea)", 0, 0 )
	SOFTWARE( mazins2, mazinw, 199?, "Sega License", "Mazin Saga Mutant Fighter (USA)", 0, 0 )

	SOFTWARE( mcdonald, 0, 199?, "Sega License", "McDonald's Treasure Land Adventure (Euro)", 0, 0 )
	SOFTWARE( mcdonaldjb, mcdonald, 199?, "Sega License", "McDonald's Treasure Land Adventure (Jpn, Prototype)", 0, 0 )
	SOFTWARE( mcdonaldj, mcdonald, 199?, "Sega License", "McDonald's Treasure Land Adventure (Jpn)", 0, 0 )
	SOFTWARE( mcdonaldu, mcdonald, 199?, "Sega License", "McDonald's Treasure Land Adventure (USA)", 0, 0 )

	SOFTWARE( medalc, 0, 199?, "Sega License", "Medal City (Jpn, SegaNet)", 0, 0 )

	SOFTWARE( megabomb, 0, 199?, "Sega License", "Mega Bomberman (Euro, Korea)", 0, 0 )
	SOFTWARE( megabombu, megabomb, 199?, "Sega License", "Mega Bomberman (USA)", 0, 0 )

	SOFTWARE( megabm8, 0, 199?, "<unlicensed>", "Mega Bomberman - 8 Player Demo", 0, 0 )

	SOFTWARE( megaga, 0, 199?, "Sega License", "Mega Games 10 (Brazil)", 0, 0 )

	SOFTWARE( megaga1, 0, 199?, "Sega License", "Mega Games 2 (Euro)", 0, 0 )

	SOFTWARE( megaga2, 0, 199?, "Sega License", "Mega Games 3 (Euro, Asia)", 0, 0 )

	SOFTWARE( megaga3, 0, 199?, "Sega License", "Mega Games 6 Vol. 1 (Euro)", 0, 0 )

	SOFTWARE( megaga4, 0, 199?, "Sega License", "Mega Games 6 Vol. 2 (Euro)", 0, 0 )

	SOFTWARE( megaga5, 0, 199?, "Sega License", "Mega Games 6 Vol. 3 (Euro)", 0, 0 )

	SOFTWARE( megaga6, 0, 199?, "Sega License", "Mega Games I (Euro)", 0, 0 )

	SOFTWARE( megaman, 0, 199?, "Sega License", "Mega Man - The Wily Wars (Euro)", 0, 0 )
	SOFTWARE( rockman1, megaman, 199?, "Sega License", "Rockman Mega World (Jpn, Alt)", 0, 0 )
	SOFTWARE( rockman, megaman, 199?, "Sega License", "Rockman Mega World (Jpn)", 0, 0 )

	SOFTWARE( megaswiv, 0, 199?, "Sega License", "Mega SWIV (Euro)", 0, 0 )
	SOFTWARE( megaswivb, megaswiv, 199?, "Sega License", "Mega SWIV (pirate) (Euro)", 0, 0 )

	SOFTWARE( megat, 0, 199?, "Sega License", "Mega Turrican (Euro)", 0, 0 )
	SOFTWARE( megatu, megat, 199?, "Sega License", "Mega Turrican (USA)", 0, 0 )

	SOFTWARE( megalo, 0, 199?, "Sega License", "Mega-Lo-Mania (Euro, v1.1)", 0, 0 )
	SOFTWARE( megalo1, megalo, 199?, "Sega License", "Mega-Lo-Mania (Euro)", 0, 0 )
	SOFTWARE( megalof, megalo, 199?, "Sega License", "Mega-Lo-Mania (France) (Euro)", 0, 0 )
	SOFTWARE( megaloj, megalo, 199?, "Sega License", "Mega-Lo-Mania (Jpn)", 0, 0 )
	SOFTWARE( tyrant, megalo, 199?, "Sega License", "Tyrants - Fight through Time (USA)", 0, 0 )

	SOFTWARE( megamind, 0, 199?, "Sega License", "MegaMind (Jpn, SegaNet)", 0, 0 )

	SOFTWARE( megapanl, 0, 199?, "Sega License", "MegaPanel (Jpn)", 0, 0 )

	SOFTWARE( menacer, 0, 199?, "Sega License", "Menacer 6-Game Cartridge (Euro, USA)", 0, 0 )

	SOFTWARE( menghu, 0, 199?, "<unlicensed>", "Meng Huan Shui Guo Pan - 777 Casino (China)", 0, 0 )

	SOFTWARE( metalf, 0, 199?, "Sega License", "Metal Fangs (Jpn)", 0, 0 )

	SOFTWARE( mwalk, 0, 199?, "Sega License", "Michael Jackson's Moonwalker (World, Rev. A)", 0, 0 )
	SOFTWARE( mwalk1, mwalk, 199?, "Sega License", "Michael Jackson's Moonwalker (World)", 0, 0 )

	SOFTWARE( mmania, 0, 199?, "Sega License", "Mickey Mania - The Timeless Adventures of Mickey Mouse (Euro)", 0, 0 )
	SOFTWARE( mmaniaj, mmania, 199?, "Sega License", "Mickey Mania - The Timeless Adventures of Mickey Mouse (Jpn)", 0, 0 )
	SOFTWARE( mmaniaub, mmania, 199?, "Sega License", "Mickey Mania - The Timeless Adventures of Mickey Mouse (USA, Prototype)", 0, 0 )
	SOFTWARE( mmaniau, mmania, 199?, "Sega License", "Mickey Mania - The Timeless Adventures of Mickey Mouse (USA)", 0, 0 )

	SOFTWARE( mickeyuc, 0, 199?, "Sega License", "Mickey's Ultimate Challenge (USA)", 0, 0 )

	SOFTWARE( microm, 0, 199?, "Sega License", "Micro Machines (Euro, USA)", 0, 0 )
	SOFTWARE( micromc, microm, 199?, "Sega License", "Micro Machines (C)", 0, 0 )
	SOFTWARE( microma, microm, 199?, "Sega License", "Micro Machines (Alt) (Euro, USA)", 0, 0 )
	SOFTWARE( micromb, microm, 199?, "Sega License", "Micro Machines (MDMM ACD3) (Euro, USA)", 0, 0 )

	SOFTWARE( micro96, 0, 199?, "Sega License", "Micro Machines Turbo Tournament 96 (Euro, v1.1)", 0, 0 )
	SOFTWARE( micro96a, mic96, 199?, "Sega License", "Micro Machines Turbo Tournament 96 (Euro, J-Cart)", 0, 0 )
	SOFTWARE( micro96b, mic96, 199?, "Sega License", "Micro Machines Turbo Tournament 96 (Euro, v1.1, J-Cart)", 0, 0 )

	SOFTWARE( microm2, 0, 199?, "Sega License", "Micro Machines 2 - Turbo Tournament (Euro, J-Cart)", 0, 0 )
	SOFTWARE( microm2a, microm2, 199?, "Sega License", "Micro Machines 2 - Turbo Tournament (Euro, J-Cart, Alt)", 0, 0 )

	SOFTWARE( micromm, 0, 199?, "Sega License", "Micro Machines Military (Euro, J-Cart)", 0, 0 )
	SOFTWARE( micromma, micromm, 199?, "Sega License", "Micro Machines Military - It's a Blast! (Euro)", 0, 0 )	// [x]

	SOFTWARE( midres, 0, 199?, "Sega License", "Midnight Resistance (USA)", 0, 0 )
	SOFTWARE( midresj, midres, 199?, "Sega License", "Midnight Resistance (Jpn)", 0, 0 )

	SOFTWARE( midway, 0, 199?, "Sega License", "Midway Presents Arcade's Greatest Hits (Euro)", 0, 0 )

	SOFTWARE( mig29fi, 0, 199?, "Sega License", "Mig-29 Fighter Pilot (Euro)", 0, 0 )
	SOFTWARE( mig29f1, mig29fi, 199?, "Sega License", "Mig-29 Fighter Pilot (Jpn)", 0, 0 )
	SOFTWARE( mig29f2, mig29fi, 199?, "Sega License", "Mig-29 Fighter Pilot (USA)", 0, 0 )

	SOFTWARE( mightmag, 0, 199?, "Sega License", "Might and Magic - Gates to Another World (Euro, USA)", 0, 0 )

	SOFTWARE( mmagic3p, 0, 199?, "Sega License", "Might and Magic III - Isles of Terra (Prototype) (USA)", 0, 0 )

	SOFTWARE( mmpr, 0, 199?, "Sega License", "Mighty Morphin Power Rangers (Euro)", 0, 0 )
	SOFTWARE( mmpru, mmpr, 199?, "Sega License", "Mighty Morphin Power Rangers (USA)", 0, 0 )
	SOFTWARE( mmprpa, mmpr, 199?, "Sega License", "Mighty Morphin Power Rangers (Prototype, 19940804)", 0, 0 )
	SOFTWARE( mmprpb, mmpr, 199?, "Sega License", "Mighty Morphin Power Rangers (Prototype, 19940808)", 0, 0 )
	SOFTWARE( mmprpc, mmpr, 199?, "Sega License", "Mighty Morphin Power Rangers (Prototype, 19940809)", 0, 0 )
	SOFTWARE( mmprpd, mmpr, 199?, "Sega License", "Mighty Morphin Power Rangers (Prototype, 19940810)", 0, 0 )
	SOFTWARE( mmprpe, mmpr, 199?, "Sega License", "Mighty Morphin Power Rangers (Prototype, 19940708)", 0, 0 )
	SOFTWARE( mmprpf, mmpr, 199?, "Sega License", "Mighty Morphin Power Rangers (Prototype, 19940718)", 0, 0 )

	SOFTWARE( mmprtm, 0, 199?, "Sega License", "Mighty Morphin Power Rangers - The Movie (Euro)", 0, 0 )
	SOFTWARE( mmprtmu, mmprtm, 199?, "Sega License", "Mighty Morphin Power Rangers - The Movie (USA)", 0, 0 )
	SOFTWARE( mmprtmpa, mmprtm, 199?, "Sega License", "Mighty Morphin Power Rangers - The Movie (Prototype, 19950713)", 0, 0 )
	SOFTWARE( mmprtmpb, mmprtm, 199?, "Sega License", "Mighty Morphin Power Rangers - The Movie (Prototype, 19950717)", 0, 0 )
	SOFTWARE( mmprtmpc, mmprtm, 199?, "Sega License", "Mighty Morphin Power Rangers - The Movie (Prototype, 19950722)", 0, 0 )
	SOFTWARE( mmprtmpd, mmprtm, 199?, "Sega License", "Mighty Morphin Power Rangers - The Movie (Prototype, 19950724)", 0, 0 )

	SOFTWARE( mikedi, 0, 199?, "Sega License", "Mike Ditka Power Football (Euro, USA, Alt)", 0, 0 )
	SOFTWARE( miked1, mikedi, 199?, "Sega License", "Mike Ditka Power Football (Euro, USA)", 0, 0 )

	SOFTWARE( minnesot, 0, 199?, "Sega License", "Minnesota Fats - Pool Legend (USA)", 0, 0 )

	SOFTWARE( miracle, 0, 199?, "Sega License", "Miracle Piano Teaching System (USA)", 0, 0 )

	SOFTWARE( mk5mor, 0, 199?, "<unlicensed>", "MK 5 - Mortal Combat - SubZero", 0, 0 )
	SOFTWARE( mk5mor1, mk5mor, 199?, "<unlicensed>", "MK 5 - Mortal Combat - SubZero (pirate)", 0, 0 )

	SOFTWARE( mlbpab, 0, 199?, "Sega License", "MLBPA Baseball (USA)", 0, 0 )

	SOFTWARE( mono, 0, 199?, "Sega License", "Monopoly (USA)", 0, 0 )
	SOFTWARE( monob, mono, 199?, "Sega License", "Monopoly (USA, Prototype)", 0, 0 )

	SOFTWARE( mworld4, 0, 199?, "Sega License", "Monster World IV (Jpn)", 0, 0 )

	SOFTWARE( mk, 0, 199?, "Sega License", "Mortal Kombat (World, v1.1)", 0, 0 )
	SOFTWARE( mka, mk, 199?, "Sega License", "Mortal Kombat (World)", 0, 0 )

	SOFTWARE( mk2, 0, 199?, "Sega License", "Mortal Kombat II (World)", 0, 0 )

	SOFTWARE( mk3, 0, 199?, "Sega License", "Mortal Kombat 3 (Euro)", 0, 0 )
	SOFTWARE( mk3u, mk3, 199?, "Sega License", "Mortal Kombat 3 (USA)", 0, 0 )

	SOFTWARE( mrnutz, 0, 199?, "Sega License", "Mr. Nutz (Euro)", 0, 0 )

	SOFTWARE( mspacman, 0, 199?, "Sega License", "Ms. Pac-Man (Euro, USA)", 0, 0 )
	SOFTWARE( mspacmanu, mspacman, 199?, "Sega License", "Ms. Pac-Man (pirate) (USA)", 0, 0 )

	SOFTWARE( muhamm, 0, 199?, "Sega License", "Muhammad Ali Heavyweight Boxing (Euro)", 0, 0 )
	SOFTWARE( muhammub, muhamm, 199?, "Sega License", "Muhammad Ali Heavyweight Boxing (USA, Prototype)", 0, 0 )
	SOFTWARE( muhammu, muhamm, 199?, "Sega License", "Muhammad Ali Heavyweight Boxing (USA)", 0, 0 )

	SOFTWARE( musha, 0, 199?, "Sega License", "MUSHA - Metallic Uniframe Super Hybrid Armor (USA)", 0, 0 )
	SOFTWARE( mushaj, musha, 199?, "Sega License", "Musha Aleste - Full Metal Fighter Ellinor (Jpn)", 0, 0 )

	SOFTWARE( mutantlfj, mutantlf, 199?, "Sega License", "Mutant League Football (Jpn)", 0, 0 )
	SOFTWARE( mutantlf, 0, 199?, "Sega License", "Mutant League Football (Euro, USA)", 0, 0 )

	SOFTWARE( mlhockey, 0, 199?, "Sega License", "Mutant League Hockey (Euro, USA)", 0, 0 )

	SOFTWARE( mysticd, 0, 199?, "Sega License", "Mystic Defender (Euro, USA, Rev. A)", 0, 0 )
	SOFTWARE( mysticd1, mysticd, 199?, "Sega License", "Mystic Defender (Euro, USA)", 0, 0 )
	SOFTWARE( kujaku, mysticd, 199?, "Sega License", "Kujaku Ou 2 - Geneijou (Jpn)", 0, 0 )

	SOFTWARE( mysticf, 0, 199?, "Sega License", "Mystical Fighter (USA)", 0, 0 )

	SOFTWARE( nakaf1gp, 0, 199?, "Sega License", "Nakajima Satoru Kanshuu F1 Grand Prix (Jpn)", 0, 0 )

	SOFTWARE( nakaf1he, 0, 199?, "Sega License", "Nakajima Satoru Kanshuu F1 Hero MD (Jpn)", 0, 0 )

	SOFTWARE( nakaf1sl, 0, 199?, "Sega License", "Nakajima Satoru Kanshuu F1 Super License (Jpn)", 0, 0 )

	SOFTWARE( naomichi, 0, 199?, "Sega License", "Naomichi Ozaki no Super Masters (Jpn)", 0, 0 )

	SOFTWARE( nbaa, 0, 199?, "Sega License", "NBA Action (USA)", 0, 0 )
	SOFTWARE( nbaap1, nbaa, 199?, "Sega License", "NBA Action (Prototype, 19940104)", 0, 0 )
	SOFTWARE( nbaap2, nbaa, 199?, "Sega License", "NBA Action (Prototype, 19940116)", 0, 0 )
	SOFTWARE( nbaap3, nbaa, 199?, "Sega License", "NBA Action (Prototype, 19940127, broken - C08 missing)", 0, 0 )

	SOFTWARE( nbaa95, 0, 199?, "Sega License", "NBA Action '95 Starring David Robinson (Euro, USA)", 0, 0 )
	SOFTWARE( nbaa95a, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19941202-B)", 0, 0 )
	SOFTWARE( nbaa95b, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19941209)", 0, 0 )
	SOFTWARE( nbaa95c, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19941215)", 0, 0 )
	SOFTWARE( nbaa95d, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19941222-A)", 0, 0 )
	SOFTWARE( nbaa95e, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19941224-A)", 0, 0 )
	SOFTWARE( nbaa95f, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19941229)", 0, 0 )
	SOFTWARE( nbaa95g, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19941230)", 0, 0 )
	SOFTWARE( nbaa95h, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19941231)", 0, 0 )
	SOFTWARE( nbaa95i, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19950201, Alt)", 0, 0 )
	SOFTWARE( nbaa95j, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19950201)", 0, 0 )	// is the date correct?!?
	SOFTWARE( nbaa95k, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19950202)", 0, 0 )	// is the date correct?!?
	SOFTWARE( nbaa95l, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19950103)", 0, 0 )
	SOFTWARE( nbaa95m, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19950108)", 0, 0 )
	SOFTWARE( nbaa95n, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19950112)", 0, 0 )
	SOFTWARE( nbaa95o, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19950115-A)", 0, 0 )
	SOFTWARE( nbaa95p, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19950121)", 0, 0 )
	SOFTWARE( nbaa95q, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19950122-B)", 0, 0 )
	SOFTWARE( nbaa95r, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19950124-B)", 0, 0 )
	SOFTWARE( nbaa95s, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19950127-A)", 0, 0 )
	SOFTWARE( nbaa95t, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19950127-B)", 0, 0 )
	SOFTWARE( nbaa95u, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19950128-A)", 0, 0 )
	SOFTWARE( nbaa95v, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19950128)", 0, 0 )
	SOFTWARE( nbaa95w, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19950130)", 0, 0 )
	SOFTWARE( nbaa95x, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19941118)", 0, 0 )
	SOFTWARE( nbaa95y, nbaa95, 199?, "Sega License", "NBA Action '95 (Prototype, 19941123-A)", 0, 0 )

	SOFTWARE( nbaallst, 0, 199?, "Sega License", "NBA All-Star Challenge (Euro, USA)", 0, 0 )

	SOFTWARE( nbahang, 0, 199?, "Sega License", "NBA Hang Time (Euro)", 0, 0 )
	SOFTWARE( nbahangu, nbahang, 199?, "Sega License", "NBA Hang Time (USA)", 0, 0 )

	SOFTWARE( nbajam, 0, 199?, "Sega License", "NBA Jam (Euro, USA, v1.1)", 0, 0 )
	SOFTWARE( nbajam1, nbajam, 199?, "Sega License", "NBA Jam (Euro, USA)", 0, 0 )
	SOFTWARE( nbajamj, nbajam, 199?, "Sega License", "NBA Jam (Jpn)", 0, 0 )

	SOFTWARE( nbajamte, 0, 199?, "Sega License", "NBA Jam Tournament Edition (World)", 0, 0 )
	SOFTWARE( nbajamtef, nbajamte, 199?, "Sega License", "NBA Jam Tournament Edition (World, 2002 Fix Release)", 0, 0 )

	SOFTWARE( nbal95, 0, 199?, "Sega License", "NBA Live 95 (Euro, USA)", 0, 0 )
	SOFTWARE( nbal95k, nbal95, 199?, "Sega License", "NBA Live 95 (Korea)", 0, 0 )

	SOFTWARE( nbal96, 0, 199?, "Sega License", "NBA Live 96 (Euro, USA)", 0, 0 )

	SOFTWARE( nbal97, 0, 199?, "Sega License", "NBA Live 97 (Euro, USA)", 0, 0 )

	SOFTWARE( nbal98, 0, 199?, "Sega License", "NBA Live 98 (USA)", 0, 0 )

	SOFTWARE( nbaplay, 0, 199?, "Sega License", "NBA Playoffs - Bulls Vs Blazers (Jpn)", 0, 0 )

	SOFTWARE( nbaplay1, nbaplay, 199?, "Sega License", "NBA Playoffs - Bulls Vs Blazers (Jpn)", 0, 0 )

	SOFTWARE( nbapro, 0, 199?, "Sega License", "NBA Pro Basketball - Bulls Vs Lakers (Jpn)", 0, 0 )

	SOFTWARE( nbapro94, 0, 199?, "Sega License", "NBA Pro Basketball '94 (Jpn)", 0, 0 )

	SOFTWARE( nbashow, 0, 199?, "Sega License", "NBA Showdown '94 (Euro, USA)", 0, 0 )
	SOFTWARE( nbashowb, nbashow, 199?, "Sega License", "NBA Showdown '94 (USA, Prototype)", 0, 0 )

	SOFTWARE( ncaabask, 0, 199?, "Sega License", "NCAA Final Four Basketball (USA)", 0, 0 )

	SOFTWARE( ncaafoot, 0, 199?, "Sega License", "NCAA Football (USA)", 0, 0 )

	SOFTWARE( nekketsu, 0, 199?, "Sega License", "Nekketsu Koukou Dodgeball Bu - Soccer Hen MD (Jpn)", 0, 0 )

	SOFTWARE( n3dgdevi, 0, 199?, "Sega License", "New 3D Golf Simulation Devil's Course (Jpn)", 0, 0 )

	SOFTWARE( n3dgaugu, 0, 199?, "Sega License", "New 3D Golf Simulation Harukanaru Augusta (Jpn)", 0, 0 )

	SOFTWARE( n3dgpebb, 0, 199?, "Sega License", "New 3D Golf Simulation Pebble Beach no Hatou (Jpn)", 0, 0 )

	SOFTWARE( n3dgwaia, 0, 199?, "Sega License", "New 3D Golf Simulation Waialae no Kiseki (Jpn)", 0, 0 )

	SOFTWARE( tnzs, 0, 199?, "Sega License", "The New Zealand Story (Jpn)", 0, 0 )

	SOFTWARE( newman, 0, 199?, "Sega License", "Newman Haas Indy Car Featuring Nigel Mansell (World)", 0, 0 )

	SOFTWARE( nfl95, 0, 199?, "Sega License", "NFL '95 (Euro, USA)", 0, 0 )
	SOFTWARE( nfl95a, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940801)", 0, 0 )
	SOFTWARE( nfl95b, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940805)", 0, 0 )
	SOFTWARE( nfl95c, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940810)", 0, 0 )
	SOFTWARE( nfl95d, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940812)", 0, 0 )
	SOFTWARE( nfl95e, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940812, Alt)", 0, 0 )
	SOFTWARE( nfl95f, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940817-B)", 0, 0 )
	SOFTWARE( nfl95g, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940817)", 0, 0 )
	SOFTWARE( nfl95h, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940822)", 0, 0 )
	SOFTWARE( nfl95i, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940830)", 0, 0 )
	SOFTWARE( nfl95j, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940831)", 0, 0 )
	SOFTWARE( nfl95k, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940901)", 0, 0 )
	SOFTWARE( nfl95l, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940902)", 0, 0 )
	SOFTWARE( nfl95m, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940904)", 0, 0 )
	SOFTWARE( nfl95n, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940905-B)", 0, 0 )
	SOFTWARE( nfl95o, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940905)", 0, 0 )
	SOFTWARE( nfl95p, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940906)", 0, 0 )
	SOFTWARE( nfl95q, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940907)", 0, 0 )
	SOFTWARE( nfl95r, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940908)", 0, 0 )
	SOFTWARE( nfl95s, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940909)", 0, 0 )
	SOFTWARE( nfl95t, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940911-B)", 0, 0 )
	SOFTWARE( nfl95u, nfl95, 199?, "Sega License", "NFL '95 (Prototype, 19940911)", 0, 0 )

	SOFTWARE( nfl98, 0, 199?, "Sega License", "NFL 98 (USA)", 0, 0 )

	SOFTWARE( nfl94, 0, 199?, "Sega License", "NFL Football '94 Starring Joe Montana (USA)", 0, 0 )
	SOFTWARE( nfl94j, nfl94, 199?, "Sega License", "NFL Football '94 (Jpn)", 0, 0 )

	SOFTWARE( nflqua, 0, 199?, "Sega License", "NFL Quarterback Club (World)", 0, 0 )

	SOFTWARE( nflqua96, 0, 199?, "Sega License", "NFL Quarterback Club 96 (Euro, USA)", 0, 0 )

	SOFTWARE( nflsport, 0, 199?, "Sega License", "NFL Sports Talk Football '93 Starring Joe Montana (Euro, USA)", 0, 0 )

	SOFTWARE( nhktaiga, 0, 199?, "Sega License", "NHK Taiga Drama - Taiheiki (Jpn)", 0, 0 )

	SOFTWARE( nhl95, 0, 199?, "Sega License", "NHL 95 (Euro, USA)", 0, 0 )

	SOFTWARE( nhl96, 0, 199?, "Sega License", "NHL 96 (Euro, USA)", 0, 0 )

	SOFTWARE( nhl97, 0, 199?, "Sega License", "NHL 97 (Euro, USA)", 0, 0 )

	SOFTWARE( nhl98, 0, 199?, "Sega License", "NHL 98 (USA)", 0, 0 )

	SOFTWARE( nhlas, 0, 199?, "Sega License", "NHL All-Star Hockey '95 (USA)", 0, 0 )
	SOFTWARE( nhlasa, nhlas, 199?, "Sega License", "NHL All-Star Hockey '95 (Prototype, 19941201-B)", 0, 0 )
	SOFTWARE( nhlasb, nhlas, 199?, "Sega License", "NHL All-Star Hockey '95 (Prototype, 19941201)", 0, 0 )
	SOFTWARE( nhlasc, nhlas, 199?, "Sega License", "NHL All-Star Hockey '95 (Prototype, 19941202)", 0, 0 )
	SOFTWARE( nhlasd, nhlas, 199?, "Sega License", "NHL All-Star Hockey '95 (Prototype, 19941107)", 0, 0 )
	SOFTWARE( nhlase, nhlas, 199?, "Sega License", "NHL All-Star Hockey '95 (Prototype, 19941109)", 0, 0 )
	SOFTWARE( nhlasf, nhlas, 199?, "Sega License", "NHL All-Star Hockey '95 (Prototype, 19941119)", 0, 0 )
	SOFTWARE( nhlasg, nhlas, 199?, "Sega License", "NHL All-Star Hockey '95 (Prototype, 19941121)", 0, 0 )
	SOFTWARE( nhlash, nhlas, 199?, "Sega License", "NHL All-Star Hockey '95 (Prototype, 19941122)", 0, 0 )
	SOFTWARE( nhlasi, nhlas, 199?, "Sega License", "NHL All-Star Hockey '95 (Prototype, 19941123)", 0, 0 )
	SOFTWARE( nhlasj, nhlas, 199?, "Sega License", "NHL All-Star Hockey '95 (Prototype, 19941127)", 0, 0 )
	SOFTWARE( nhlask, nhlas, 199?, "Sega License", "NHL All-Star Hockey '95 (Prototype, 19941128)", 0, 0 )
	SOFTWARE( nhlasl, nhlas, 199?, "Sega License", "NHL All-Star Hockey '95 (Prototype, 19941129)", 0, 0 )
	SOFTWARE( nhlasm, nhlas, 199?, "Sega License", "NHL All-Star Hockey '95 (Prototype, 19941001)", 0, 0 )
	SOFTWARE( nhlasn, nhlas, 199?, "Sega License", "NHL All-Star Hockey '95 (Prototype, 19941021)", 0, 0 )
	SOFTWARE( nhlaso, nhlas, 199?, "Sega License", "NHL All-Star Hockey '95 (Prototype, 19940914)", 0, 0 )
	SOFTWARE( nhlasp, nhlas, 199?, "Sega License", "NHL All-Star Hockey '95 (Prototype, 19940929)", 0, 0 )

	SOFTWARE( nhlhoc, 0, 199?, "Sega License", "NHL Hockey (USA)", 0, 0 )

	SOFTWARE( nhlpah, 0, 199?, "Sega License", "NHLPA Hockey 93 (Euro, USA, v1.1)", 0, 0 )
	SOFTWARE( nhlpah1, nhlpah, 199?, "Sega License", "NHLPA Hockey 93 (Euro, USA)", 0, 0 )

	SOFTWARE( nigelm, 0, 199?, "Sega License", "Nigel Mansell's World Championship Racing (Euro)", 0, 0 )
	SOFTWARE( nigelmu, nigelm, 199?, "Sega License", "Nigel Mansell's World Championship Racing (USA)", 0, 0 )

	SOFTWARE( ncirc, 0, 199?, "Sega License", "Nightmare Circus (Brazil)", 0, 0 )
	SOFTWARE( ncirc1, ncirc, 199?, "Sega License", "Nightmare Circus (Brazil, Alt)", 0, 0 )
	SOFTWARE( ncircp, ncirc, 199?, "Sega License", "Nightmare Circus (Prototype)", 0, 0 )

	SOFTWARE( nikkan, 0, 199?, "Sega License", "Nikkan Sports Pro Yakyuu Van (Jpn)", 0, 0 )

	SOFTWARE( ninjab, 0, 199?, "Sega License", "Ninja Burai Densetsu (Jpn)", 0, 0 )

	SOFTWARE( ninjag, 0, 199?, "Sega License", "Ninja Gaiden (Prototype) (Jpn)", 0, 0 )

	SOFTWARE( noescape, 0, 199?, "Sega License", "No Escape (USA)", 0, 0 )

	SOFTWARE( nobubus, 0, 199?, "Sega License", "Nobunaga no Yabou - Bushou Fuuunroku (Jpn)", 0, 0 )

	SOFTWARE( nobuhao, 0, 199?, "Sega License", "Nobunaga no Yabou - Haouden (Jpn)", 0, 0 )

	SOFTWARE( nobuamb, 0, 199?, "Koei", "Nobunaga's Ambition (USA)", 0, 0 )
	SOFTWARE( nobuzen, nobuamb, 199?, "Koei", "Nobunaga no Yabou - Zenkoku Ban (Jpn)", 0, 0 )

	SOFTWARE( normys, 0, 199?, "Sega License", "Normy's Beach Babe-O-Rama (Euro, USA)", 0, 0 )

	SOFTWARE( olympi, 0, 199?, "Sega License", "Olympic Gold (Euro)", 0, 0 )
	SOFTWARE( olymp1, olympi, 199?, "Sega License", "Olympic Gold (Jpn, Korea)", 0, 0 )
	SOFTWARE( olymp2, olympi, 199?, "Sega License", "Olympic Gold (USA, Alt)", 0, 0 )
	SOFTWARE( olymp3, olympi, 199?, "Sega License", "Olympic Gold (USA)", 0, 0 )

	SOFTWARE( olympsum, 0, 199?, "Sega License", "Olympic Summer Games (Euro, USA)", 0, 0 )

	SOFTWARE( ondalj, 0, 199?, "Sega License", "On Dal Jang Goon (Korea)", 0, 0 )

	SOFTWARE( onslau, 0, 199?, "Sega License", "Onslaught (Euro, USA)", 0, 0 )

	SOFTWARE( ooze, 0, 199?, "Sega License", "The Ooze (Euro)", 0, 0 )
	SOFTWARE( oozeju, ooze, 199?, "Sega License", "The Ooze (Jpn, USA)", 0, 0 )
	SOFTWARE( oozepa, ooze, 199?, "Sega License", "The Ooze (Prototype, 19950728) (Euro)", 0, 0 )
	SOFTWARE( oozepb, ooze, 199?, "Sega License", "The Ooze (Prototype, 19950615)", 0, 0 )
	SOFTWARE( oozepc, ooze, 199?, "Sega License", "The Ooze (Prototype, 19950619)", 0, 0 )
	SOFTWARE( oozepd, ooze, 199?, "Sega License", "The Ooze (Prototype, 19950629-B)", 0, 0 )
	SOFTWARE( oozepe, ooze, 199?, "Sega License", "The Ooze (Prototype, 19950629)", 0, 0 )
	SOFTWARE( oozetf, ooze, 199?, "Sega License", "The Ooze (Prototype 104, 19950622)", 0, 0 )
	SOFTWARE( oozetg, ooze, 199?, "Sega License", "The Ooze (Prototype 112, 19950626)", 0, 0 )

	SOFTWARE( operat, 0, 199?, "Koei", "Operation Europe - Path to Victory 1939-45 (USA)", 0, 0 )
	SOFTWARE( europa, operat, 199?, "Koei", "Europa Sensen (Jpn)", 0, 0 )

	SOFTWARE( osomat, 0, 199?, "Sega License", "Osomatsu-kun Hachamecha Gekijou (Jpn)", 0, 0 )

	SOFTWARE( ottifa, 0, 199?, "Sega License", "The Ottifants (Euro)", 0, 0 )
	SOFTWARE( ottifag, ottifa, 199?, "Sega License", "The Ottifants (Germany) (Euro, Prototype)", 0, 0 )

	SOFTWARE( outlandr, 0, 199?, "Sega License", "Outlander (Euro)", 0, 0 )
	SOFTWARE( outlandru, outlandr, 199?, "Sega License", "Outlander (USA)", 0, 0 )

	SOFTWARE( outrun, 0, 199?, "Sega License", "OutRun (Euro, USA)", 0, 0 )
	SOFTWARE( outrunj, outrun, 199?, "Sega License", "OutRun (Jpn)", 0, 0 )

	SOFTWARE( o2019, 0, 199?, "Sega License", "OutRun 2019 (Euro)", 0, 0 )
	SOFTWARE( o2019j, o2019, 199?, "Sega License", "OutRun 2019 (Jpn)", 0, 0 )
	SOFTWARE( o2019b, o2019, 199?, "Sega License", "OutRun 2019 (USA, Prototype)", 0, 0 )
	SOFTWARE( o2019u, o2019, 199?, "Sega License", "OutRun 2019 (USA)", 0, 0 )
	SOFTWARE( junker, o2019, 199?, "Sega License", "Junker's High (Jpn, Prototype)", 0, 0 )

	SOFTWARE( outrunr, 0, 199?, "Sega License", "OutRunners (USA)", 0, 0 )
	SOFTWARE( outrunrj, outrunr, 199?, "Sega License", "OutRunners (Jpn)", 0, 0 )

	SOFTWARE( pacattck, 0, 199?, "Sega License", "Pac-Attack (USA)", 0, 0 )

	SOFTWARE( pacman, 0, 199?, "Sega License", "Pac-Man 2 - The New Adventures (USA)", 0, 0 )

	SOFTWARE( pacmania, 0, 199?, "Sega License", "Pac-Mania (Euro, USA)", 0, 0 )

	SOFTWARE( pachinko, 0, 199?, "Sega License", "Pachinko Kuunyan (Jpn)", 0, 0 )

	SOFTWARE( pacific, 0, 199?, "Koei", "Pacific Theater of Operations (USA)", 0, 0 )
	SOFTWARE( teitoku, pacific, 199?, "Koei", "Teitoku no Ketsudan (Jpn)", 0, 0 )

	SOFTWARE( paddle, 0, 199?, "Sega License", "Paddle Fighter (Jpn, SegaNet)", 0, 0 )
	SOFTWARE( paddle1, paddle, 199?, "Sega License", "Paddle Fighter (Jpn, Game no Kandume MegaCD Rip)", 0, 0 )

	SOFTWARE( pagemast, 0, 199?, "Sega License", "The Pagemaster (Euro)", 0, 0 )
	SOFTWARE( pagemastub, pagemast, 199?, "Sega License", "The Pagemaster (USA, Prototype)", 0, 0 )
	SOFTWARE( pagemastu, pagemast, 199?, "Sega License", "The Pagemaster (USA)", 0, 0 )

	SOFTWARE( panora, 0, 199?, "Sega License", "Panorama Cotton (Jpn)", 0, 0 )

	SOFTWARE( paperboyj, paperboy, 199?, "Sega License", "Paperboy (Jpn)", 0, 0 )
	SOFTWARE( paperboy, 0, 199?, "Sega License", "Paperboy (Euro, USA)", 0, 0 )

	SOFTWARE( paperby2, 0, 199?, "Sega License", "Paperboy 2 (Euro, USA)", 0, 0 )

	SOFTWARE( partyq, 0, 199?, "Sega License", "Party Quiz Mega Q (Jpn)", 0, 0 )

	SOFTWARE( patriley, 0, 199?, "Sega License", "Pat Riley Basketball (USA)", 0, 0 )

	SOFTWARE( pebble, 0, 199?, "Sega License", "Pebble Beach Golf Links (Euro)", 0, 0 )
	SOFTWARE( pebbleu, pebble, 199?, "Sega License", "Pebble Beach Golf Links (USA)", 0, 0 )

	SOFTWARE( peleii, 0, 199?, "Sega License", "Pele II - World Tournament Soccer (Euro, USA)", 0, 0 )

	SOFTWARE( pele, 0, 199?, "Sega License", "Pele! (Euro, USA)", 0, 0 )

	SOFTWARE( pepenga, 0, 199?, "Sega License", "Pepenga Pengo (Jpn)", 0, 0 )

	SOFTWARE( psampras, 0, 199?, "Sega License", "Pete Sampras Tennis (Euro, USA, J-Cart, MDST6636)", 0, 0 )
	SOFTWARE( psampras1, psampras, 199?, "Sega License", "Pete Sampras Tennis (Euro, USA, J-Cart, MDSTEE 13)", 0, 0 )
	SOFTWARE( psampras2, psampras, 199?, "Sega License", "Pete Sampras Tennis (Euro, USA, J-Cart)", 0, 0 )

	SOFTWARE( pgaeuro, 0, 199?, "Sega License", "PGA European Tour (Euro, USA)", 0, 0 )

	SOFTWARE( pga96, 0, 199?, "Sega License", "PGA Tour 96 (Euro, USA)", 0, 0 )

	SOFTWARE( pga, 0, 199?, "Sega License", "PGA Tour Golf (Euro, USA, v1.2)", 0, 0 )
	SOFTWARE( pgaa, pga, 199?, "Sega License", "PGA Tour Golf (Euro, USA, v1.1)", 0, 0 )

	SOFTWARE( pga2, 0, 199?, "Sega License", "PGA Tour Golf II (Euro, USA, v1.1)", 0, 0 )
	SOFTWARE( pga2a, pga2, 199?, "Sega License", "PGA Tour Golf II (Euro, USA)", 0, 0 )
	SOFTWARE( pga2j, pga2, 199?, "Sega License", "PGA Tour Golf II (Jpn)", 0, 0 )

	SOFTWARE( pga3, 0, 199?, "Sega License", "PGA Tour Golf III (Euro, USA)", 0, 0 )

	SOFTWARE( pstar2, 0, 199?, "Sega License", "Phantasy Star II (Euro, USA, Rev. A)", 0, 0 )
	SOFTWARE( pstar2a, pstar2, 199?, "Sega License", "Phantasy Star II (Euro, USA)", 0, 0 )
	SOFTWARE( pstar2b, pstar2, 199?, "Sega License", "Phantasy Star II (Brazil)", 0, 0 )
	SOFTWARE( pstar2j, pstar2, 199?, "Sega License", "Phantasy Star II - Kaerazaru Toki no Owari ni (Jpn)", 0, 0 )

	SOFTWARE( ps2aa, 0, 199?, "Sega License", "Phantasy Star II - Amia's Adventure (Jpn, SegaNet)", 0, 0 )
	SOFTWARE( ps2ab, 0, 199?, "Sega License", "Phantasy Star II - Anne's Adventure (Jpn, SegaNet)", 0, 0 )
	SOFTWARE( ps2ac, 0, 199?, "Sega License", "Phantasy Star II - Huey's Adventure (Jpn, SegaNet)", 0, 0 )
	SOFTWARE( ps2ad, 0, 199?, "Sega License", "Phantasy Star II - Kinds's Adventure (Jpn, SegaNet)", 0, 0 )
	SOFTWARE( ps2ae, 0, 199?, "Sega License", "Phantasy Star II - Nei's Adventure (Jpn, SegaNet)", 0, 0 )
	SOFTWARE( ps2af, 0, 199?, "Sega License", "Phantasy Star II - Rudger's Adventure (Jpn, SegaNet)", 0, 0 )
	SOFTWARE( ps2ag, 0, 199?, "Sega License", "Phantasy Star II - Shilka's Adventure (Jpn, SegaNet)", 0, 0 )
	SOFTWARE( ps2ah, 0, 199?, "Sega License", "Phantasy Star II - Yushis's Adventure (Jpn, SegaNet)", 0, 0 )

	SOFTWARE( pstar3, 0, 199?, "Sega License", "Phantasy Star III - Generations of Doom (Brazil)", 0, 0 )
	SOFTWARE( pstar3a, pstar3, 199?, "Sega License", "Phantasy Star III - Generations of Doom (Euro, Korea, USA)", 0, 0 )
	SOFTWARE( pstar3b, pstar3, 199?, "Sega License", "Phantasy Star III - Toki no Keishousha (Jpn)", 0, 0 )

	SOFTWARE( pstar4, 0, 199?, "Sega License", "Phantasy Star IV (USA)", 0, 0 )
	SOFTWARE( pstar4a, pstar4, 199?, "Sega License", "Phantasy Star IV (Prototype, 19940815)", 0, 0 )
	SOFTWARE( pstar4b, pstar4, 199?, "Sega License", "Phantasy Star IV (Prototype, 19940608)", 0, 0 )
	SOFTWARE( pstar4c, pstar4, 199?, "Sega License", "Phantasy Star IV (Prototype, 19940530)", 0, 0 )
	SOFTWARE( pstar4d, pstar4, 199?, "Sega License", "Phantasy Star IV (Prototype, 19941107)", 0, 0 )
	SOFTWARE( pstar4e, pstar4, 199?, "Sega License", "Phantasy Star IV (Prototype, 19941027)", 0, 0 )
	SOFTWARE( pstar4j, pstar4, 199?, "Sega License", "Phantasy Star - Sennenki no Owari ni (Jpn)", 0, 0 )

	SOFTWARE( phantom, 0, 199?, "Sega License", "Phantom 2040 (Euro)", 0, 0 )
	SOFTWARE( phantomu, phantom, 199?, "Sega License", "Phantom 2040 (USA)", 0, 0 )

	SOFTWARE( phelios, 0, 199?, "Sega License", "Phelios (Euro)", 0, 0 )
	SOFTWARE( pheliosj, phelios, 199?, "Sega License", "Phelios (Jpn)", 0, 0 )
	SOFTWARE( pheliosu, phelios, 199?, "Sega License", "Phelios (USA)", 0, 0 )

	SOFTWARE( pink, 0, 199?, "Sega License", "Pink Goes to Hollywood (USA)", 0, 0 )
	SOFTWARE( pinkb, pink, 199?, "Sega License", "Pink Goes to Hollywood (USA, Prototype)", 0, 0 )

	SOFTWARE( pinocc, 0, 199?, "Sega License", "Pinocchio (Euro)", 0, 0 )
	SOFTWARE( pinoc1, pinocc, 199?, "Sega License", "Pinocchio (USA)", 0, 0 )

	SOFTWARE( pirate, 0, 199?, "Sega License", "The Pirates of Dark Water (January 1994) (USA)", 0, 0 )
	SOFTWARE( pirate1, pirate, 199?, "Sega License", "The Pirates of Dark Water (May 1994) (Euro, USA)", 0, 0 )

	SOFTWARE( pgold, 0, 199?, "Sega License", "Pirates! Gold (USA)", 0, 0 )
	SOFTWARE( pgoldb, pgold, 199?, "Sega License", "Pirates! Gold (USA, Prototype)", 0, 0 )

	SOFTWARE( pitfight, 0, 199?, "Sega License", "Pit-Fighter (World, Rev. A)", 0, 0 )
	SOFTWARE( pitfight1, pitfight, 199?, "Sega License", "Pit-Fighter (World)", 0, 0 )

	SOFTWARE( pitfall, 0, 199?, "Sega License", "Pitfall - The Mayan Adventure (Euro)", 0, 0 )
	SOFTWARE( pitfallu, pitfall, 199?, "Sega License", "Pitfall - The Mayan Adventure (USA)", 0, 0 )

	SOFTWARE( pocaho, 0, 199?, "Sega License", "Pocahontas (Euro)", 0, 0 )
	SOFTWARE( pocahou, pocaho, 199?, "Sega License", "Pocahontas (USA)", 0, 0 )

	SOFTWARE( pokemon, 0, 199?, "<unlicensed>", "Pocket Monsters", 0, 0 )
	SOFTWARE( pokemona, pokemon, 199?, "<unlicensed>", "Pocket Monsters (Alt)", 0, 0 )

	SOFTWARE( pokemon2, 0, 199?, "<unlicensed>", "Pocket Monsters 2", 0, 0 )

	SOFTWARE( pokecd, 0, 199?, "<unlicensed>", "Pokemon Crazy Drummer", 0, 0 )

	SOFTWARE( pokestad, 0, 199?, "<unlicensed>", "Pokemon Stadium", 0, 0 )

	SOFTWARE( populous, 0, 199?, "Sega License", "Populous (Euro)", 0, 0 )
	SOFTWARE( populousj, populous, 199?, "Sega License", "Populous (Jpn)", 0, 0 )
	SOFTWARE( populousu, populous, 199?, "Sega License", "Populous (USA)", 0, 0 )

	SOFTWARE( powerd, 0, 199?, "Sega License", "Power Drive (Euro)", 0, 0 )

	SOFTWARE( pmonger, 0, 199?, "Sega License", "Power Monger (Euro, USA)", 0, 0 )
	SOFTWARE( pmongerj, pmonger, 199?, "Sega License", "Power Monger (Jpn, Korea)", 0, 0 )

	SOFTWARE( powerb, 0, 199?, "Sega License", "Powerball (USA)", 0, 0 )
	SOFTWARE( wball, powerb, 199?, "Sega License", "Wrestleball (Jpn)", 0, 0 )

	SOFTWARE( predatr2, 0, 199?, "Sega License", "Predator 2 (Euro, USA)", 0, 0 )

	SOFTWARE( premierm, 0, 199?, "Sega License", "Premier Manager (Euro)", 0, 0 )

	SOFTWARE( premrm97, 0, 199?, "Sega License", "Premier Manager 97 (Euro)", 0, 0 )

	SOFTWARE( pretty, 0, 199?, "Sega License", "Pretty Girl Mahjongg (Ch)", 0, 0 )

	SOFTWARE( primal, 0, 199?, "Sega License", "Primal Rage (Euro, USA)", 0, 0 )

	SOFTWARE( primetim, 0, 199?, "Sega License", "Prime Time NFL Starring Deion Sanders (USA)", 0, 0 )

	SOFTWARE( ppersia, 0, 199?, "Sega License", "Prince of Persia (Euro)", 0, 0 )
	SOFTWARE( ppersiaa, ppersia, 199?, "Sega License", "Prince of Persia (Euro, Prototype, Earlier)", 0, 0 )
	SOFTWARE( ppersiab, ppersia, 199?, "Sega License", "Prince of Persia (Euro, Prototype)", 0, 0 )
	SOFTWARE( ppersiau, ppersia, 199?, "Sega License", "Prince of Persia (USA)", 0, 0 )

	SOFTWARE( ppersia2, 0, 199?, "Sega License", "Prince of Persia 2 - The Shadow and the Flame (Prototype) (Euro)", 0, 0 )

	SOFTWARE( proquart, 0, 199?, "Sega License", "Pro Quarterback (USA)", 0, 0 )

	SOFTWARE( proyakyu, 0, 199?, "Sega License", "Pro Yakyuu Super League '91 (Jpn)", 0, 0 )

	SOFTWARE( probot, 0, 199?, "Sega License", "Probotector (Euro)", 0, 0 )
	SOFTWARE( contra, probot, 199?, "Sega License", "Contra - Hard Corps (Korea, USA)", 0, 0 )
	SOFTWARE( contraj, probot, 199?, "Sega License", "Contra - The Hard Corps (Jpn)", 0, 0 )

	SOFTWARE( psyobl, 0, 199?, "Sega License", "Psy-O-Blade (Jpn)", 0, 0 )

	SOFTWARE( psycho, 0, 199?, "Sega License", "Psycho Pinball (Euro, 199410)", 0, 0 )
	SOFTWARE( psycho1, psycho, 199?, "Sega License", "Psycho Pinball (Euro, 199409)", 0, 0 )

	SOFTWARE( puggsy, 0, 199?, "Sega License", "Puggsy (Euro)", 0, 0 )
	SOFTWARE( puggsyb, puggsy, 199?, "Sega License", "Puggsy (Prototype)", 0, 0 )
	SOFTWARE( puggsyu, puggsy, 199?, "Sega License", "Puggsy (USA)", 0, 0 )

	SOFTWARE( pulseman, 0, 199?, "Sega License", "Pulseman (Jpn)", 0, 0 )

	SOFTWARE( punisher, 0, 199?, "Sega License", "The Punisher (Euro)", 0, 0 )
	SOFTWARE( punisheru, punisher, 199?, "Sega License", "The Punisher (USA)", 0, 0 )

	SOFTWARE( putter, 0, 199?, "Sega License", "Putter Golf (Jpn, SegaNet)", 0, 0 )
	SOFTWARE( putter1, putter, 199?, "Sega License", "Putter Golf (Jpn, Game no Kandume MegaCD Rip)", 0, 0 )

	SOFTWARE( puyopuyo, 0, 199?, "Sega License", "Puyo Puyo (Jpn)", 0, 0 )

	SOFTWARE( puyo2, 0, 199?, "Sega License", "Puyo Puyo 2 (Jpn, v1.1)", 0, 0 )
	SOFTWARE( puyo2a, puyo2, 199?, "Sega License", "Puyo Puyo 2 (Jpn)", 0, 0 )

	SOFTWARE( ichir, 0, 199?, "Sega License", "Puzzle & Action - Ichidanto-R (Jpn)", 0, 0 )

	SOFTWARE( tantr, 0, 199?, "Sega License", "Puzzle & Action - Tanto-R (Jpn)", 0, 0 )

	SOFTWARE( pyramid, 0, 199?, "Sega License", "Pyramid Magic (Jpn, SegaNet)", 0, 0 )
	SOFTWARE( pyramida, pyramid, 199?, "Sega License", "Pyramid Magic (Jpn, Game no Kandume MegaCD Rip)", 0, 0 )

	SOFTWARE( pyramid2, 0, 199?, "Sega License", "Pyramid Magic II (Jpn, SegaNet)", 0, 0 )

	SOFTWARE( pyramid3, 0, 199?, "Sega License", "Pyramid Magic III (Jpn, SegaNet)", 0, 0 )

	SOFTWARE( pyramids, 0, 199?, "Sega License", "Pyramid Magic Special (Jpn, SegaNet)", 0, 0 )

	SOFTWARE( quacksht, 0, 199?, "Sega License", "QuackShot Starring Donald Duck / QuackShot - Guruzia Ou no Hihou (World)", 0, 0 )
	SOFTWARE( quacksht1, quacksht, 199?, "Sega License", "QuackShot Starring Donald Duck / QuackShot - Guruzia Ou no Hihou (World, Alt)", 0, 0 )

	SOFTWARE( quadchal, 0, 199?, "Sega License", "Quad Challenge (USA)", 0, 0 )
	SOFTWARE( megatrax, quadchal, 199?, "Sega License", "MegaTrax (Jpn)", 0, 0 )

	SOFTWARE( rbi93, 0, 199?, "Sega License", "R.B.I. Baseball '93 (USA)", 0, 0 )

	SOFTWARE( rbi94, 0, 199?, "Sega License", "R.B.I. Baseball '94 (Euro, USA)", 0, 0 )

	SOFTWARE( rbi3, 0, 199?, "Sega License", "R.B.I. Baseball 3 (USA)", 0, 0 )

	SOFTWARE( rbi4j, rbi4, 199?, "Sega License", "R.B.I. Baseball 4 (Jpn)", 0, 0 )
	SOFTWARE( rbi4, 0, 199?, "Sega License", "R.B.I. Baseball 4 (USA)", 0, 0 )
	SOFTWARE( rbi4b, rbi4, 199?, "Sega License", "R.B.I. Baseball 4 (Prototype)", 0, 0 )

	SOFTWARE( racedriv, 0, 199?, "Sega License", "Race Drivin' (USA)", 0, 0 )

	SOFTWARE( radica, 0, 199?, "Sega License", "Radical Rex (Euro)", 0, 0 )
	SOFTWARE( radica1, radica, 199?, "Sega License", "Radical Rex (USA)", 0, 0 )

	SOFTWARE( raiden, 0, 199?, "Sega License", "Raiden Densetsu / Raiden Trad (Jpn, USA)", 0, 0 )

	SOFTWARE( rainbow, 0, 199?, "Sega License", "Rainbow Islands Extra (Jpn)", 0, 0 )

	SOFTWARE( rambo3, 0, 199?, "Sega License", "Rambo III (World, v1.1)", 0, 0 )
	SOFTWARE( rambo3a, rambo3, 199?, "Sega License", "Rambo III (World)", 0, 0 )

	SOFTWARE( rampart, 0, 199?, "Sega License", "Rampart (USA)", 0, 0 )
	SOFTWARE( rampartj, rampart, 199?, "Sega License", "Rampart (Jpn, Korea)", 0, 0 )

	SOFTWARE( rangerx, 0, 199?, "Sega License", "Ranger-X (Euro)", 0, 0 )
	SOFTWARE( rangerxu, rangerx, 199?, "Sega License", "Ranger-X (USA)", 0, 0 )
	SOFTWARE( exranza, rangerx, 199?, "Sega License", "Ex-Ranza (Jpn)", 0, 0 )
	SOFTWARE( exranzab, rangerx, 199?, "Sega License", "Ex-Ranza (Jpn, Prototype)", 0, 0 )

	SOFTWARE( ransei, 0, 199?, "Sega License", "Ransei no Hasha (Jpn)", 0, 0 )

	SOFTWARE( rastans2, 0, 199?, "Sega License", "Rastan Saga II (USA)", 0, 0 )
	SOFTWARE( rastans2j, rastans2, 199?, "Sega License", "Rastan Saga II (Jpn)", 0, 0 )

	SOFTWARE( redzone, 0, 199?, "Sega License", "Red Zone (Euro, USA)", 0, 0 )

	SOFTWARE( renstim, 0, 199?, "Sega License", "The Ren & Stimpy Show Presents Stimpy's Invention (Euro)", 0, 0 )
	SOFTWARE( renstimu1, renstim, 199?, "Sega License", "The Ren & Stimpy Show Presents Stimpy's Invention (USA, Prototype)", 0, 0 )
	SOFTWARE( renstimu, renstim, 199?, "Sega License", "The Ren & Stimpy Show Presents Stimpy's Invention (USA)", 0, 0 )

	SOFTWARE( renthero, 0, 199?, "Sega License", "Rent a Hero (Jpn)", 0, 0 )

	SOFTWARE( resq, 0, 199?, "Sega License", "Resq (Prototype) (Euro)", 0, 0 )

	SOFTWARE( revshin, 0, 199?, "Sega License", "The Revenge of Shinobi (Euro, USA, Rev. A)", 0, 0 )
	SOFTWARE( revshin1, reveng, 199?, "Sega License", "The Revenge of Shinobi (Euro, USA, Rev. B)", 0, 0 )
	SOFTWARE( revshin2, reveng, 199?, "Sega License", "The Revenge of Shinobi (Euro, USA)", 0, 0 )

	SOFTWARE( revx, 0, 199?, "Sega License", "Revolution X (Euro, USA)", 0, 0 )

	SOFTWARE( rsbt, 0, 199?, "Sega License", "Richard Scarry's BusyTown (USA)", 0, 0 )
	SOFTWARE( rsbtpa, rsbt, 199?, "Sega License", "Richard Scarry's Busytown (Prototype, 19940809)", 0, 0 )
	SOFTWARE( rsbtpb, rsbt, 199?, "Sega License", "Richard Scarry's Busytown (Prototype, 19940815)", 0, 0 )
	SOFTWARE( rsbtpc, rsbt, 199?, "Sega License", "Richard Scarry's Busytown (Prototype, 19940816-B)", 0, 0 )
	SOFTWARE( rsbtpd, rsbt, 199?, "Sega License", "Richard Scarry's Busytown (Prototype, 19940817)", 0, 0 )
	SOFTWARE( rsbtpe, rsbt, 199?, "Sega License", "Richard Scarry's Busytown (Prototype, 19940825)", 0, 0 )
	SOFTWARE( rsbtpf, rsbt, 199?, "Sega License", "Richard Scarry's Busytown (Prototype, 19940826)", 0, 0 )
	SOFTWARE( rsbtpg, rsbt, 199?, "Sega License", "Richard Scarry's Busytown (Prototype, 19940721)", 0, 0 )

	SOFTWARE( riddle, 0, 199?, "Sega License", "Riddle Wired (Jpn, SegaNet)", 0, 0 )

	SOFTWARE( ringspow, 0, 199?, "Sega License", "Rings of Power (Euro, USA)", 0, 0 )

	SOFTWARE( risk, 0, 199?, "Sega License", "Risk (USA)", 0, 0 )

	SOFTWARE( riskyw, 0, 199?, "Electronic Arts", "Risky Woods (Euro, USA)", 0, 0 )
	SOFTWARE( jashin, riskyw, 199?, "Electronic Arts", "Jashin Draxos (Jpn, Korea)", 0, 0 )

	SOFTWARE( ristar, 0, 199?, "Sega License", "Ristar (September 1994) (Euro, USA)", 0, 0 )
	SOFTWARE( ristaro, rist, 199?, "Sega License", "Ristar (August 1994) (Euro, USA)", 0, 0 )
	SOFTWARE( ristarj, rist, 199?, "Sega License", "Ristar - The Shooting Star (Jpn, Korea)", 0, 0 )
	SOFTWARE( ristarpa, rist, 199?, "Sega License", "Ristar (Prototype, 19940812)", 0, 0 )
	SOFTWARE( ristarpb, rist, 199?, "Sega License", "Ristar (Prototype, 19940826)", 0, 0 )
	SOFTWARE( ristarpc, rist, 199?, "Sega License", "Ristar (Prototype, 19940701)", 0, 0 )
	SOFTWARE( ristarpd, rist, 199?, "Sega License", "Ristar (Prototype, 19940718)", 0, 0 )

	SOFTWARE( roadrash, 0, 199?, "Sega License", "Road Rash (Euro, USA)", 0, 0 )

	SOFTWARE( rrash2, 0, 199?, "Sega License", "Road Rash II (Jpn)", 0, 0 )
	SOFTWARE( rrash2a, rrash2, 199?, "Sega License", "Road Rash II (Euro, USA, v1.2)", 0, 0 )
	SOFTWARE( rrash2b, rrash2, 199?, "Sega License", "Road Rash II (Euro, USA)", 0, 0 )

	SOFTWARE( rrash3, 0, 199?, "Sega License", "Road Rash 3 (Euro, USA)", 0, 0 )
	SOFTWARE( rrash3a, rrash3, 199?, "Sega License", "Road Rash 3 (Alpha) (USA)", 0, 0 )

	SOFTWARE( roadb, 0, 199?, "Sega License", "RoadBlasters (USA)", 0, 0 )
	SOFTWARE( roadbj, roadb, 199?, "Sega License", "RoadBlasters (Jpn)", 0, 0 )

	SOFTWARE( robocop, 0, 199?, "Sega License", "RoboCop 3 (Euro, USA)", 0, 0 )

	SOFTWARE( roboterm, 0, 199?, "Sega License", "RoboCop versus The Terminator (Euro)", 0, 0 )
	SOFTWARE( robotermb1, roboterm, 199?, "Sega License", "Robocop Versus The Terminator (Prototype 1)", 0, 0 )
	SOFTWARE( robotermb, roboterm, 199?, "Sega License", "RoboCop versus The Terminator (Euro, Prototype)", 0, 0 )
	SOFTWARE( robotermu, roboterm, 199?, "Sega License", "RoboCop versus The Terminator (USA)", 0, 0 )

	SOFTWARE( robotb, 0, 199?, "Sega License", "Robot Battler (Jpn, SegaNet)", 0, 0 )

	SOFTWARE( robotw, 0, 199?, "Sega License", "Robot Wreckage (Prototype) (USA)", 0, 0 )

	SOFTWARE( rocknr, 0, 199?, "Sega License", "Rock n' Roll Racing (Euro)", 0, 0 )
	SOFTWARE( rocknru, rocknr, 199?, "Sega License", "Rock n' Roll Racing (USA)", 0, 0 )

	SOFTWARE( rocket, 0, 199?, "Sega License", "Rocket Knight Adventures (Euro)", 0, 0 )
	SOFTWARE( rocketj, rocket, 199?, "Sega License", "Rocket Knight Adventures (Jpn)", 0, 0 )
	SOFTWARE( rocketu, rocket, 199?, "Sega License", "Rocket Knight Adventures (USA)", 0, 0 )

	SOFTWARE( rockmnx3, 0, 199?, "<unlicensed>", "Rockman X3", 0, 0 )

	SOFTWARE( rogerc, 0, 199?, "Sega License", "Roger Clements MVP Baseball (USA)", 0, 0 )

	SOFTWARE( rthun2, 0, 199?, "Sega License", "Rolling Thunder 2 (Euro)", 0, 0 )
	SOFTWARE( rthun2j, rthun2, 199?, "Sega License", "Rolling Thunder 2 (Jpn)", 0, 0 )
	SOFTWARE( rthun2u, rthun2, 199?, "Sega License", "Rolling Thunder 2 (USA)", 0, 0 )

	SOFTWARE( rthun3, 0, 199?, "Sega License", "Rolling Thunder 3 (USA)", 0, 0 )

	SOFTWARE( rolo, 0, 199?, "Sega License", "Rolo to the Rescue (Euro, USA)", 0, 0 )

	SOFTWARE( r3k2, 0, 199?, "Sega License", "Romance of the Three Kingdoms II (USA)", 0, 0 )
	SOFTWARE( sangok2, r3k2, 199?, "Sega License", "Sangokushi II (Jpn)", 0, 0 )

	SOFTWARE( r3k3, 0, 199?, "Sega License", "Romance of the Three Kingdoms III - Dragon of Destiny (USA)", 0, 0 )
	SOFTWARE( sangok3, r3k3, 199?, "Sega License", "Sangokushi III (Jpn)", 0, 0 )

	SOFTWARE( rugbyw, 0, 199?, "Sega License", "Rugby World Cup 1995 (Euro, USA)", 0, 0 )

	SOFTWARE( sagaia, 0, 199?, "Sega License", "Sagaia (USA)", 0, 0 )
	SOFTWARE( darius2, sagaia, 199?, "Sega License", "Darius II (Jpn)", 0, 0 )

	SOFTWARE( ssword, sswrdj, 199?, "Sega License", "Saint Sword (USA)", 0, 0 )
	SOFTWARE( sswordj, 0, 199?, "Sega License", "Saint Sword (Jpn)", 0, 0 )

	SOFTWARE( sampra, 0, 199?, "Sega License", "Sampras Tennis 96 (Euro, J-Cart)", 0, 0 )

	SOFTWARE( samsho, 0, 199?, "Sega License", "Samurai Shodown (Euro)", 0, 0 )
	SOFTWARE( samshou, samsho, 199?, "Sega License", "Samurai Shodown (USA)", 0, 0 )
	SOFTWARE( samspir, samsho, 199?, "Sega License", "Samurai Spirits (Jpn)", 0, 0 )

	SOFTWARE( sanguo, 0, 199?, "Sega License", "San Guo Zhi Lie Zhuan - Luan Shi Qun Ying (China)", 0, 0 )

	SOFTWARE( sanguo5, 0, 199?, "<unlicensed>", "San Guo Zhi V (China)", 0, 0 )

	SOFTWARE( sangokr, 0, 199?, "Sega License", "Sangokushi Retsuden - Ransei no Eiyuutachi (Jpn)", 0, 0 )

	SOFTWARE( slammast, 0, 199?, "Sega License", "Saturday Night Slammasters (Euro)", 0, 0 )
	SOFTWARE( slammastu, slammast, 199?, "Sega License", "Saturday Night Slammasters (USA)", 0, 0 )

	SOFTWARE( scooby, 0, 199?, "Sega License", "Scooby Doo Mystery (USA)", 0, 0 )

	SOFTWARE( scrabble, 0, 199?, "Sega License", "Scrabble (Prototype) (Euro)", 0, 0 )

	SOFTWARE( seaquest, 0, 199?, "Sega License", "SeaQuest DSV (Euro)", 0, 0 )
	SOFTWARE( seaquestu, seaquest, 199?, "Sega License", "SeaQuest DSV (USA)", 0, 0 )

	SOFTWARE( second, 0, 199?, "Sega License", "Second Samurai (Euro)", 0, 0 )

	SOFTWARE( segasp, 0, 199?, "Sega License", "Sega Sports 1 (Euro)", 0, 0 )

	SOFTWARE( segatop, 0, 199?, "Sega License", "Sega Top Five (Brazil)", 0, 0 )

	SOFTWARE( mercs, 0, 199?, "Sega License", "Mercs / Senjou no Ookami II (World)", 0, 0 )

	SOFTWARE( sensible, 0, 199?, "Sega License", "Sensible Soccer (Euro)", 0, 0 )
	SOFTWARE( sensibleb, sensible, 199?, "Sega License", "Sensible Soccer (Prototype)", 0, 0 )

	SOFTWARE( sensibie, 0, 199?, "Sega License", "Sensible Soccer - International Edition (Euro)", 0, 0 )

	SOFTWARE( sesame, 0, 199?, "Sega License", "Sesame Street Counting Cafe (USA)", 0, 0 )

	SOFTWARE( shadow, 0, 199?, "Sage Creation", "Shadow Blasters (USA)", 0, 0 )
	SOFTWARE( shiten, shadow, 199?, "Sigma", "Shiten Myouou (Jpn)", 0, 0 )

	SOFTWARE( shdanc, 0, 199?, "Sega License", "Shadow Dancer - The Secret of Shinobi (World)", 0, 0 )

	SOFTWARE( beast, 0, 199?, "Sega License", "Shadow of the Beast (Euro, USA)", 0, 0 )
	SOFTWARE( beastj, beast, 199?, "Sega License", "Shadow of the Beast - Mashou no Okite (Jpn)", 0, 0 )

	SOFTWARE( beast2, 0, 199?, "Sega License", "Shadow of the Beast II (Euro, USA)", 0, 0 )

	SOFTWARE( shdwrun, 0, 199?, "Sega License", "Shadowrun (USA)", 0, 0 )
	SOFTWARE( shdwrunj, shdwrun, 199?, "Sega License", "Shadowrun (Jpn)", 0, 0 )
	SOFTWARE( shdwruna, shdwrun, 199?, "Sega License", "Shadowrun (Prototype, 19931228)", 0, 0 )
	SOFTWARE( shdwrunb, shdwrun, 199?, "Sega License", "Shadowrun (Prototype, 19931231)", 0, 0 )
	SOFTWARE( shdwrunc, shdwrun, 199?, "Sega License", "Shadowrun (Prototype, 19940125-C)", 0, 0 )
	SOFTWARE( shdwrund, shdwrun, 199?, "Sega License", "Shadowrun (Prototype, 19940125)", 0, 0 )

	SOFTWARE( shanew, 0, 199?, "Sega License", "Shane Warne Cricket (Oceania)", 0, 0 )

	SOFTWARE( shai2, 0, 199?, "Sega License", "Shanghai II - Dragon's Eye (USA)", 0, 0 )
	SOFTWARE( shai2a, shai2, 199?, "Sega License", "Shanghai II - Dragon's Eye (USA, Prototype)", 0, 0 )
	SOFTWARE( shai2b, shai2, 199?, "Sega License", "Shanghai II - Dragon's Eye (USA, Prototype Alt)", 0, 0 )

	SOFTWARE( shaqfu, 0, 199?, "Sega License", "Shaq Fu (Euro, USA)", 0, 0 )

	SOFTWARE( shijie, 0, 199?, "<unlicensed>", "Shi Jie Zhi Bang Zheng Ba Zhan - World Pro Baseball 94 (China)", 0, 0 )

	SOFTWARE( shikinjo, 0, 199?, "Sega License", "Shikinjoh (Jpn)", 0, 0 )

	SOFTWARE( shinfrce, 0, 199?, "Sega License", "Shining Force (USA)", 0, 0 )
	SOFTWARE( shinfrceb, shinfrce, 199?, "Sega License", "Shining Force (USA, Prototype)", 0, 0 )
	SOFTWARE( shinfrcej, shinfrce, 199?, "Sega License", "Shining Force - Kamigami no Isan (Jpn)", 0, 0 )

	SOFTWARE( shinfrc2, 0, 199?, "Sega License", "Shining Force II (Euro)", 0, 0 )
	SOFTWARE( shinfrc2pa, shinfrc2, 199?, "Sega License", "Shining Force II (Prototype, 19940404)", 0, 0 )
	SOFTWARE( shinfrc2pb, shinfrc2, 199?, "Sega License", "Shining Force II (Prototype, 19940607)", 0, 0 )
	SOFTWARE( shinfrc2u, shinfrc2, 199?, "Sega License", "Shining Force II (USA)", 0, 0 )
	SOFTWARE( shinfrc2j, shinfrc2, 199?, "Sega License", "Shining Force II - Koe no Fuuin (Jpn)", 0, 0 )

	SOFTWARE( shindark, 0, 199?, "Sega License", "Shining in the Darkness (Euro, USA)", 0, 0 )
	SOFTWARE( shindarkb, shindark, 199?, "Sega License", "Shining in the Darkness (Brazil)", 0, 0 )
	SOFTWARE( shindarkj, shindark, 199?, "Sega License", "Shining and the Darkness (Jpn)", 0, 0 )

	SOFTWARE( shinob3, 0, 199?, "Sega License", "Shinobi III - Return of the Ninja Master (Euro)", 0, 0 )
	SOFTWARE( shinob3u, shinob3, 199?, "Sega License", "Shinobi III - Return of the Ninja Master (USA)", 0, 0 )

	SOFTWARE( ship, 0, 199?, "Sega License", "Ship (Unreleased)", 0, 0 )

	SOFTWARE( shougi, 0, 199?, "Sega License", "Shougi no Hoshi (Jpn)", 0, 0 )

	SOFTWARE( shoveit, 0, 199?, "Sega License", "Shove It! ...The Warehouse Game (USA)", 0, 0 )
	SOFTWARE( shijou, shoveit, 199?, "Sega License", "Shijou Saidai no Soukoban (Jpn)", 0, 0 )

	SOFTWARE( sdm, 0, 199?, "Sega License", "Show do Milhao (Brazil)", 0, 0 )

	SOFTWARE( sdm2, 0, 199?, "Sega License", "Show do Milhao Volume 2 (Brazil)", 0, 0 )
	SOFTWARE( sdm2a, sdm2, 199?, "Sega License", "Show do Milhao Volume 2 (Brazil, Alt)", 0, 0 )

	SOFTWARE( shuihu, 0, 199?, "<unlicensed>", "Shui Hu - Feng Yun Zhuan (China)", 0, 0 )

	SOFTWARE( shuihu1, 0, 199?, "<unlicensed>", "Shui Hu Zhuan (China)", 0, 0 )

	SOFTWARE( shuramon, 0, 199?, "Sega License", "Shura no Mon (Jpn)", 0, 0 )

	SOFTWARE( sidepock, 0, 199?, "Sega License", "Side Pocket (Euro)", 0, 0 )
	SOFTWARE( sidepockj, sidepock, 199?, "Sega License", "Side Pocket (Jpn)", 0, 0 )
	SOFTWARE( sidepocku, sidepock, 199?, "Sega License", "Side Pocket (USA)", 0, 0 )

	SOFTWARE( bartvssm, 0, 199?, "Sega License", "The Simpsons - Bart Vs The Space Mutants (Euro, USA, Rev. A)", 0, 0 )
	SOFTWARE( bartvssm1, bartvssm, 199?, "Sega License", "The Simpsons - Bart Vs The Space Mutants (Euro, USA)", 0, 0 )

	SOFTWARE( bartnigt, 0, 199?, "Sega License", "The Simpsons - Bart's Nightmare (Euro, USA)", 0, 0 )

	SOFTWARE( skelet, 0, 199?, "Sega License", "Skeleton Krew (Euro)", 0, 0 )
	SOFTWARE( skeletu, skelet, 199?, "Sega License", "Skeleton Krew (USA)", 0, 0 )

	SOFTWARE( skitchin, 0, 199?, "Sega License", "Skitchin (Euro, USA)", 0, 0 )

	SOFTWARE( slamshaq, 0, 199?, "Sega License", "Slam - Shaq vs. the Legends (Prototype)", 0, 0 )

	SOFTWARE( slapfigh, 0, 199?, "Sega License", "Slap Fight MD (Jpn)", 0, 0 )

	SOFTWARE( slaughtr, 0, 199?, "Sega License", "Slaughter Sport (USA)", 0, 0 )
	SOFTWARE( fatman, slaughtr, 199?, "Sega License", "Fatman (Jpn)", 0, 0 )

	SOFTWARE( smurfs, 0, 199?, "Sega License", "The Smurfs (Euro, Rev. A)", 0, 0 )

	SOFTWARE( smurfs2, 0, 199?, "Sega License", "The Smurfs Travel the World (Euro)", 0, 0 )

	SOFTWARE( snakernr, 0, 199?, "Sega License", "Snake Rattle n' Roll (Euro)", 0, 0 )

	SOFTWARE( snowbros, 0, 199?, "Sega License", "Snow Bros. - Nick & Tom (Jpn)", 0, 0 )

	SOFTWARE( socket, 0, 199?, "Sega License", "Socket (USA)", 0, 0 )
	SOFTWARE( timedom1, socket, 199?, "Sega License", "Time Dominator 1st (Jpn, Korea)", 0, 0 )

	SOFTWARE( soldeace, 0, 199?, "Sega License", "Sol-Deace (USA)", 0, 0 )

	SOFTWARE( soleil, 0, 199?, "Sega License", "Soleil (Euro)", 0, 0 )
	SOFTWARE( soleilf, soleil, 199?, "Sega License", "Soleil (France)", 0, 0 )
	SOFTWARE( soleilg, soleil, 199?, "Sega License", "Soleil (Germany)", 0, 0 )
	SOFTWARE( soleilj, soleil, 199?, "Sega License", "Soleil (Jpn, Prototype)", 0, 0 )	// shouldn't Ragnacenty be the title?
	SOFTWARE( soleils, soleil, 199?, "Sega License", "Soleil (Spain)", 0, 0 )
	SOFTWARE( crusader, soleil, 199?, "Sega License", "Crusader of Centy (USA)", 0, 0 )
	SOFTWARE( ragnacen, soleil, 199?, "Sega License", "Ragnacenty (Korea)", 0, 0 )
	SOFTWARE( shinso, soleil, 199?, "Sega License", "Shin Souseiki Ragnacenty (Jpn)", 0, 0 )

	SOFTWARE( sk, 0, 199?, "Sega License", "Sonic & Knuckles (World)", 0, 0 )
	SOFTWARE( ska, sk, 199?, "Sega License", "Sonic & Knuckles (Prototype 0525, 19940525, 15.28)", 0, 0 )
	SOFTWARE( skb, sk, 199?, "Sega License", "Sonic & Knuckles (Prototype 0606, 19940606, 10.02)", 0, 0 )
	SOFTWARE( skc, sk, 199?, "Sega License", "Sonic & Knuckles (Prototype 0608, 19940608, 05.03)", 0, 0 )
	SOFTWARE( skd, sk, 199?, "Sega License", "Sonic & Knuckles (Prototype 0610, 19940610, 07.49)", 0, 0 )
	SOFTWARE( ske, sk, 199?, "Sega License", "Sonic & Knuckles (Prototype 0612, 19940612, 18.27)", 0, 0 )
	SOFTWARE( skf, sk, 199?, "Sega License", "Sonic & Knuckles (Prototype 0618, 19940618, 09.15)", 0, 0 )
	SOFTWARE( skg, sk, 199?, "Sega License", "Sonic & Knuckles (Prototype 0619, 19940619, 08.18)", 0, 0 )
	SOFTWARE( skh, sk, 199?, "Sega License", "Sonic & Knuckles (S2K chip) (Prototype 0606, 19940605, 22.25)", 0, 0 )
	SOFTWARE( ski, sk, 199?, "Sega License", "Sonic & Knuckles (S2K chip) (Prototype 0608, 19940608, 03.35)", 0, 0 )
	SOFTWARE( skj, sk, 199?, "Sega License", "Sonic & Knuckles (S2K chip) (Prototype 0610, 19940610, 03.11)", 0, 0 )
	SOFTWARE( skk, sk, 199?, "Sega License", "Sonic & Knuckles (S2K chip) (Prototype 0612, 19940612, 18.18)", 0, 0 )
	SOFTWARE( skl, sk, 199?, "Sega License", "Sonic & Knuckles (S2K chip) (Prototype 0618, 19940618, 9.07)", 0, 0 )

	SOFTWARE( sks1, 0, 199?, "Sega License", "Sonic & Knuckles + Sonic the Hedgehog (World)", 0, 0 )
	SOFTWARE( sks2, 0, 199?, "Sega License", "Sonic & Knuckles + Sonic the Hedgehog 2 (World)", 0, 0 )
	SOFTWARE( knuckl, sks2, 199?, "Sega License", "Knuckles in Sonic 2 (Prototype 0524, 19940527, 10.46)", 0, 0 )

	SOFTWARE( sks3, 0, 199?, "Sega License", "Sonic & Knuckles + Sonic the Hedgehog 3 (World)", 0, 0 )
	SOFTWARE( sonic3c, sks3, 199?, "Sega License", "Sonic 3C (Prototype 0408, 19940408, 17.29)", 0, 0 )
	SOFTWARE( sonic3ca, sks3, 199?, "Sega License", "Sonic 3C (Prototype 0517, 19940517, 17.08)", 0, 0 )

	SOFTWARE( s3d, 0, 199?, "Sega License", "Sonic 3D Blast / Sonic 3D - Flickies' Island (Euro, Korea, USA)", 0, 0 )
	SOFTWARE( s3da, s3d, 199?, "Sega License", "Sonic 3D Blast (Prototype 73, 19960703, 13.58)", 0, 0 )
	SOFTWARE( s3db, s3d, 199?, "Sega License", "Sonic 3D Blast (Prototype 814, 19960815, 07.55)", 0, 0 )
	SOFTWARE( s3dc, s3d, 199?, "Sega License", "Sonic 3D Blast (Prototype 819, 19960819, 19.49)", 0, 0 )
	SOFTWARE( s3dd, s3d, 199?, "Sega License", "Sonic 3D Blast (Prototype 825, 19960826, 15.46)", 0, 0 )
	SOFTWARE( s3de, s3d, 199?, "Sega License", "Sonic 3D Blast (Prototype 830, 19960831, 08.19)", 0, 0 )
	SOFTWARE( s3df, s3d, 199?, "Sega License", "Sonic 3D Blast (Prototype 831, 19960903, 10.07)", 0, 0 )
	SOFTWARE( s3dg, s3d, 199?, "Sega License", "Sonic 3D Blast (Prototype 94, 19960904, 12.01)", 0, 0 )
	SOFTWARE( s3dh, s3d, 199?, "Sega License", "Sonic 3D Blast (USA, Prototype)", 0, 0 )

	SOFTWARE( soniccmp, 0, 199?, "Sega License", "Sonic Compilation / Sonic Classics (Euro, Korea, USA)", 0, 0 )
	SOFTWARE( soniccmp1, soniccmp, 199?, "Sega License", "Sonic Compilation (Euro) (Earlier)", 0, 0 )

	SOFTWARE( scrack, 0, 199?, "Sega License", "Sonic Crackers (Prototype) (Jpn)", 0, 0 )

	SOFTWARE( sonicer, 0, 199?, "Sega License", "Sonic Eraser (Jpn, SegaNet)", 0, 0 )

	SOFTWARE( sonicjam, 0, 199?, "<unlicensed>", "Sonic Jam 6", 0, 0 )
	SOFTWARE( sonicjam1, sonicjam, 199?, "<unlicensed>", "Sonic Jam 6", 0, 0 )

	SOFTWARE( sspin, 0, 199?, "Sega License", "Sonic Spinball (Euro)", 0, 0 )
	SOFTWARE( sspina, sspin, 199?, "Sega License", "Sonic Spinball (Jpn)", 0, 0 )
	SOFTWARE( sspinb, sspin, 199?, "Sega License", "Sonic Spinball (USA, Alt)", 0, 0 )
	SOFTWARE( sspinc, sspin, 199?, "Sega License", "Sonic Spinball (USA, Prototype)", 0, 0 )
	SOFTWARE( sspind, sspin, 199?, "Sega License", "Sonic Spinball (USA)", 0, 0 )

	SOFTWARE( sonic, 0, 199?, "Sega License", "Sonic the Hedgehog (Euro, USA)", 0, 0 )
	SOFTWARE( sonica, sonic, 199?, "Sega License", "Sonic the Hedgehog (Jpn, Korea)", 0, 0 )
	SOFTWARE( sonicb, sonic, 199?, "Sega License", "Sonic the Hedgehog (Pirate?) (World, Rev. 00)", 0, 0 )

	SOFTWARE( sonic2, 0, 199?, "Sega License", "Sonic the Hedgehog 2 (World, Rev. A)", 0, 0 )
	SOFTWARE( sonic2a, sonic2, 199?, "Sega License", "Sonic the Hedgehog 2 (World)", 0, 0 )
	SOFTWARE( sonic2b, sonic2, 199?, "Sega License", "Sonic the Hedgehog 2 (Beta 4, 19920918, 16.26)", 0, 0 )
	SOFTWARE( sonic2c, sonic2, 199?, "Sega License", "Sonic the Hedgehog 2 (Beta 5, 19920921, 12.06)", 0, 0 )
	SOFTWARE( sonic2d, sonic2, 199?, "Sega License", "Sonic the Hedgehog 2 (Beta 6, 19920922, 18.47)", 0, 0 )
	SOFTWARE( sonic2e, sonic2, 199?, "Sega License", "Sonic the Hedgehog 2 (Beta 6, 19920922, 19.42)", 0, 0 )
	SOFTWARE( sonic2f, sonic2, 199?, "Sega License", "Sonic the Hedgehog 2 (Beta 7, 19920924, 09.26)", 0, 0 )
	SOFTWARE( sonic2g, sonic2, 199?, "Sega License", "Sonic the Hedgehog 2 (Beta 8, 19920924, 19.27)", 0, 0 )
	SOFTWARE( sonic2h, sonic2, 199?, "Sega License", "Sonic the Hedgehog 2 (Prototype)", 0, 0 )
	SOFTWARE( sonic2i, sonic2, 199?, "Sega License", "Sonic the Hedgehog 2 (World, Rev. 01a)", 0, 0 )
	SOFTWARE( sonic2j, sonic2, 199?, "Sega License", "Sonic the Hedgehog 2 (World, Rev. SC02)", 0, 0 )
	SOFTWARE( sonic2k, sonic2, 199?, "Sega License", "Sonic the Hedgehog 2 (World, Prototype)", 0, 0 )

	SOFTWARE( sonic3, 0, 199?, "Sega License", "Sonic the Hedgehog 3 (Euro)", 0, 0 )
	SOFTWARE( sonic3a, sonic3, 199?, "Sega License", "Sonic the Hedgehog 3 (Argentinian Pirate)", 0, 0 )
	SOFTWARE( sonic3u, sonic3, 199?, "Sega License", "Sonic the Hedgehog 3 (USA)", 0, 0 )
	SOFTWARE( sonic3j, sonic3, 199?, "Sega License", "Sonic the Hedgehog 3 (Jpn, Korea)", 0, 0 )

	SOFTWARE( sorcerk, 0, 199?, "Sega License", "Sorcerer's Kingdom (USA, v1.1)", 0, 0 )
	SOFTWARE( sorcerk1, sorcerk, 199?, "Sega License", "Sorcerer's Kingdom (USA)", 0, 0 )
	SOFTWARE( sorcerkj, sorcerk, 199?, "Sega License", "Sorcer Kingdom (Jpn)", 0, 0 )

	SOFTWARE( sorcern, 0, 199?, "Sega License", "Sorcerian (Jpn)", 0, 0 )

	SOFTWARE( soulblad, 0, 199?, "<unlicensed>", "Soul Blade", 0, 0 )

	SOFTWARE( sharr, 0, 199?, "Sega License", "Space Harrier II (World)", 0, 0 )
	SOFTWARE( sharrj, sharr, 199?, "Sega License", "Space Harrier II (Launch Cart) (Jpn)", 0, 0 )

	SOFTWARE( sinv91, 0, 199?, "Sega License", "Space Invaders 91 (USA)", 0, 0 )
	SOFTWARE( sinv90, sinv91, 199?, "Sega License", "Space Invaders 90 (Jpn)", 0, 0 )

	SOFTWARE( sparks, 0, 199?, "Sega License", "Sparkster (Euro)", 0, 0 )
	SOFTWARE( sparksu, sparks, 199?, "Sega License", "Sparkster (USA)", 0, 0 )
	SOFTWARE( sparksj, sparks, 199?, "Sega License", "Sparkster - Rocket Knight Adventures 2 (Jpn)", 0, 0 )

	SOFTWARE( speedbl2, 0, 199?, "Sega License", "Speedball 2 (Euro)", 0, 0 )
	SOFTWARE( speedbl2j, speedbl2, 199?, "Sega License", "Speedball 2 (Jpn)", 0, 0 )
	SOFTWARE( speedbl2u, speedbl2, 199?, "Sega License", "Speedball 2 - Brutal Deluxe (USA)", 0, 0 )

	SOFTWARE( spidrman, 0, 199?, "Acclaim Entertainment", "Spider-Man (Acclaim) (Euro, USA)", 0, 0 )
	SOFTWARE( spidrmanb, spidrman, 199?, "Acclaim Entertainment", "Spider-Man (Acclaim) (USA, Prototype, Earlier)", 0, 0 )
	SOFTWARE( spidrmanb1, spidrman, 199?, "Acclaim Entertainment", "Spider-Man (Acclaim) (USA, Prototype)", 0, 0 )

	SOFTWARE( spidsega, 0, 199?, "Sega", "Spider-Man vs. the Kingpin (Sega) (World)", 0, 0 )

	SOFTWARE( sp_mc, 0, 199?, "Sega License", "Spider-Man and Venom - Maximum Carnage (World)", 0, 0 )

	SOFTWARE( sp_sa, 0, 199?, "Sega License", "Spider-Man and Venom - Separation Anxiety (Euro, USA)", 0, 0 )

	SOFTWARE( sp_xm, 0, 199?, "Sega License", "Spider-Man and X-Men - Arcade's Revenge (USA)", 0, 0 )

	SOFTWARE( spirit, 0, 199?, "Wisdom Tree", "Spiritual Warfare (USA)", 0, 0 )

	SOFTWARE( spirou, 0, 199?, "Sega License", "Spirou (Euro)", 0, 0 )

	SOFTWARE( splatt2, 0, 199?, "Sega License", "Splatterhouse 2 (Euro)", 0, 0 )
	SOFTWARE( splatt2u, splatt2, 199?, "Sega License", "Splatterhouse 2 (USA)", 0, 0 )
	SOFTWARE( splatt2j, splatt2, 199?, "Sega License", "Splatterhouse Part 2 (Jpn)", 0, 0 )

	SOFTWARE( splatt3, 0, 199?, "Sega License", "Splatterhouse 3 (USA)", 0, 0 )
	SOFTWARE( splatt3j1, splatt3, 199?, "Sega License", "Splatterhouse Part 3 (Jpn, Alt)", 0, 0 )
	SOFTWARE( splatt3j, splatt3, 199?, "Sega License", "Splatterhouse Part 3 (Jpn, Korea)", 0, 0 )

	SOFTWARE( sportg, 0, 199?, "Sega License", "Sport Games (Brazil)", 0, 0 )

	SOFTWARE( sports, 0, 199?, "Sega License", "Sports Talk Baseball (USA)", 0, 0 )

	SOFTWARE( spotgo, 0, 199?, "Sega License", "Spot Goes to Hollywood (Euro)", 0, 0 )
	SOFTWARE( spotgou, spotgo, 199?, "Sega License", "Spot Goes to Hollywood (USA)", 0, 0 )

	SOFTWARE( squirrel, 0, 199?, "<unlicensed>", "Squirrel King (China)", 0, 0 )

	SOFTWARE( starctrl, 0, 199?, "Ballistic", "Star Control (USA)", 0, 0 )

	SOFTWARE( starcr, 0, 199?, "Sega License", "Star Cruiser (Jpn)", 0, 0 )

	SOFTWARE( stds9, 0, 199?, "Sega License", "Star Trek - Deep Space Nine - Crossroads of Time (Euro)", 0, 0 )
	SOFTWARE( stds9u, stds9, 199?, "Sega License", "Star Trek - Deep Space Nine - Crossroads of Time (USA)", 0, 0 )

	SOFTWARE( sttng, 0, 199?, "Sega License", "Star Trek - The Next Generation - Echoes from the Past (USA, v1.1)", 0, 0 )
	SOFTWARE( sttnga, sttng, 199?, "Sega License", "Star Trek - The Next Generation - Echoes from the Past (USA)", 0, 0 )
	SOFTWARE( sttngb, sttng, 199?, "Sega License", "Star Trek - The Next Generation - Echoes from the Past (Prototype, 19941228)", 0, 0 )
	SOFTWARE( sttngc, sttng, 199?, "Sega License", "Star Trek - The Next Generation - Echoes from the Past (Prototype, 19941229)", 0, 0 )
	SOFTWARE( sttngd, sttng, 199?, "Sega License", "Star Trek - The Next Generation - Echoes from the Past (Prototype, 19940103)", 0, 0 )
	SOFTWARE( sttnge, sttng, 199?, "Sega License", "Star Trek - The Next Generation - Echoes from the Past (Prototype, 19940110)", 0, 0 )
	SOFTWARE( sttngf, sttng, 199?, "Sega License", "Star Trek - The Next Generation - Echoes from the Past (Prototype, 19940118)", 0, 0 )
	SOFTWARE( sttngg, sttng, 199?, "Sega License", "Star Trek - The Next Generation - Echoes from the Past (Prototype, 19940125)", 0, 0 )

	SOFTWARE( starfl, 0, 199?, "Sega License", "Starflight (Euro, USA, v1.1)", 0, 0 )
	SOFTWARE( starfl1, starfl, 199?, "Sega License", "Starflight (Euro, USA)", 0, 0 )

	SOFTWARE( starg, 0, 199?, "Sega License", "Stargate (Euro, USA)", 0, 0 )
	SOFTWARE( stargb, starg, 199?, "Sega License", "Stargate (Euro, Prototype)", 0, 0 )

	SOFTWARE( steeltal, 0, 199?, "Sega License", "Steel Talons (Euro, USA)", 0, 0 )
	SOFTWARE( steeltalj, steeltal, 199?, "Sega License", "Steel Talons (Jpn, Korea)", 0, 0 )
	SOFTWARE( steeltalb, steeltal, 199?, "Sega License", "Steel Talons (USA, Prototype)", 0, 0 )

	SOFTWARE( slord, 0, 199?, "Sega License", "Stormlord (USA)", 0, 0 )
	SOFTWARE( slordj, slord, 199?, "Sega License", "Stormlord (Jpn)", 0, 0 )

	SOFTWARE( storytho, 0, 199?, "Sega License", "The Story of Thor (Euro)", 0, 0 )
	SOFTWARE( storytho1, storytho, 199?, "Sega License", "The Story of Thor (Germany) (Euro)", 0, 0 )
	SOFTWARE( storytho2, storytho, 199?, "Sega License", "The Story of Thor (Jpn, Prototype)", 0, 0 )	// BAD DUMP
	SOFTWARE( storytho3, storytho, 199?, "Sega License", "The Story of Thor  (Korea)", 0, 0 )
	SOFTWARE( storytho4, storytho, 199?, "Sega License", "The Story of Thor (Prototype, 19941101)", 0, 0 )
	SOFTWARE( storytho5, storytho, 199?, "Sega License", "The Story of Thor (Prototype, 19941004)", 0, 0 )
	SOFTWARE( storytho6, storytho, 199?, "Sega License", "The Story of Thor (Prototype, 19941017)", 0, 0 )
	SOFTWARE( storytho7, storytho, 199?, "Sega License", "The Story of Thor (Spain) (Euro)", 0, 0 )
	SOFTWARE( storytho8, storytho, 199?, "Sega License", "The Story of Thor - Hikari o Tsugumono (Jpn)", 0, 0 )
	SOFTWARE( legendtho, storytho, 199?, "Sega License", "La Legende de Thor (France) (Euro)", 0, 0 )
	SOFTWARE( beyoasis, storytho, 199?, "Sega License", "Beyond Oasis (USA)", 0, 0 )
	SOFTWARE( beyoasisp, storytho, 199?, "Sega License", "Beyond Oasis (Prototype, 19941101)", 0, 0 )

	SOFTWARE( sf2, 0, 199?, "Sega License", "Street Fighter II' - Special Champion Edition (Euro)", 0, 0 )
	SOFTWARE( sf2u, sf2, 199?, "Sega License", "Street Fighter II' - Special Champion Edition (USA)", 0, 0 )
	SOFTWARE( sf2p, sf2, 199?, "Sega License", "Street Fighter II' Plus (Jpn, Asia, Korea)", 0, 0 )
	SOFTWARE( sf2tb, sf2, 199?, "Sega License", "Street Fighter II' Turbo (Prototype)", 0, 0 )

	SOFTWARE( sracer, 0, 199?, "Sega License", "Street Racer (Euro)", 0, 0 )

	SOFTWARE( ssmart, 0, 199?, "Sega License", "Street Smart (Jpn, USA)", 0, 0 )

	SOFTWARE( sor, 0, 199?, "Sega License", "Streets of Rage / Bare Knuckle - Ikari no Tetsuken (World, Rev. A)", 0, 0 )
	SOFTWARE( sora, sor, 199?, "Sega License", "Streets of Rage / Bare Knuckle - Ikari no Tetsuken (World)", 0, 0 )

	SOFTWARE( sor2, 0, 199?, "Sega License", "Streets of Rage 2 (USA)", 0, 0 )
	SOFTWARE( bk2, sor2, 199?, "Sega License", "Bare Knuckle II - Shitou heno Chingonka / Streets of Rage II (Euro, Jpn)", 0, 0 )
	SOFTWARE( bk2b, sor2, 199?, "Sega License", "Bare Knuckle II (Jpn, Prototype)", 0, 0 )

	SOFTWARE( sor3, 0, 199?, "Sega License", "Streets of Rage 3 (Euro)", 0, 0 )
	SOFTWARE( sor3u, sor3, 199?, "Sega License", "Streets of Rage 3 (USA)", 0, 0 )
	SOFTWARE( sor3k, sor3, 199?, "Sega License", "Streets of Rage 3 (Korea)", 0, 0 )
	SOFTWARE( sor3pa, sor3, 199?, "Sega License", "Streets of Rage 3 (Prototype, 19940412) (Euro)", 0, 0 )
	SOFTWARE( sor3pb, sor3, 199?, "Sega License", "Streets of Rage 3 (Prototype, 19940413) (Euro)", 0, 0 )
	SOFTWARE( sor3pc, sor3, 199?, "Sega License", "Streets of Rage 3 (Prototype, 19940415) (Euro)", 0, 0 )
	SOFTWARE( sor3pd, sor3, 199?, "Sega License", "Streets of Rage 3 (Prototype, 19940420) (Euro)", 0, 0 )
	SOFTWARE( sor3pe, sor3, 199?, "Sega License", "Streets of Rage 3 (Prototype, 19940425) (Euro)", 0, 0 )
	SOFTWARE( sor3pf, sor3, 199?, "Sega License", "Streets of Rage 3 (Prototype, 19940401)", 0, 0 )
	SOFTWARE( sor3pg, sor3, 199?, "Sega License", "Streets of Rage 3 (Prototype, 19940404)", 0, 0 )
	SOFTWARE( sor3ph, sor3, 199?, "Sega License", "Streets of Rage 3 (Prototype, 19940408)", 0, 0 )
	SOFTWARE( sor3pi, sor3, 199?, "Sega License", "Streets of Rage 3 (Prototype, 19940411)", 0, 0 )
	SOFTWARE( sor3pj, sor3, 199?, "Sega License", "Streets of Rage 3 (Prototype, 19940412)", 0, 0 )
	SOFTWARE( sor3pk, sor3, 199?, "Sega License", "Streets of Rage 3 (Prototype, 19940413)", 0, 0 )
	SOFTWARE( sor3pl, sor3, 199?, "Sega License", "Streets of Rage 3 (Prototype, 19940308)", 0, 0 )
	SOFTWARE( sor3pm, sor3, 199?, "Sega License", "Streets of Rage 3 (Prototype, 19940317)", 0, 0 )
	SOFTWARE( sor3pn, sor3, 199?, "Sega License", "Streets of Rage 3 (Prototype, 19940318)", 0, 0 )
	SOFTWARE( sor3po, sor3, 199?, "Sega License", "Streets of Rage 3 (Prototype, 19940328)", 0, 0 )
	SOFTWARE( bk3, sor3, 199?, "Sega License", "Bare Knuckle III (Jpn)", 0, 0 )
	SOFTWARE( bk3b, sor3, 199?, "Sega License", "Bare Knuckle III (Jpn, Prototype)", 0, 0 )

	SOFTWARE( strider, 0, 199?, "Sega License", "Strider (Euro, USA)", 0, 0 )
	SOFTWARE( striderj, strider, 199?, "Sega License", "Strider Hiryuu (Jpn, Korea)", 0, 0 )

	SOFTWARE( strider2, 0, 199?, "Sega License", "Strider II (Euro)", 0, 0 )
	SOFTWARE( strider2u, strider2, 199?, "Sega License", "Strider Returns - Journey from Darkness (USA)", 0, 0 )

	SOFTWARE( striker, 0, 199?, "Sega License", "Striker (Euro)", 0, 0 )
	SOFTWARE( strikerb, striker, 199?, "Sega License", "Striker (Euro, Prototype)", 0, 0 )

	SOFTWARE( subt, 0, 199?, "Sega License", "SubTerrania (Euro)", 0, 0 )
	SOFTWARE( subtj, subt, 199?, "Sega License", "SubTerrania (Jpn)", 0, 0 )
	SOFTWARE( subtu, subt, 199?, "Sega License", "SubTerrania (USA)", 0, 0 )
	SOFTWARE( subtua, subt, 199?, "Sega License", "SubTerrania (USA, Prototype, Earlier)", 0, 0 )
	SOFTWARE( subtub, subt, 199?, "Sega License", "SubTerrania (USA, Prototype)", 0, 0 )
	SOFTWARE( subtp, subt, 199?, "Sega License", "SubTerrania (Prototype, 19940202)", 0, 0 )

	SOFTWARE( summer, 0, 199?, "Sega License", "Summer Challenge (Euro, USA)", 0, 0 )

	SOFTWARE( sunset, 0, 199?, "Sega License", "Sunset Riders (Euro)", 0, 0 )
	SOFTWARE( sunsetu, sunset, 199?, "Sega License", "Sunset Riders (USA)", 0, 0 )

	SOFTWARE( s15in1, 0, 199?, "Sega License", "Super 15-in-1 (pirate)", 0, 0 )

	SOFTWARE( s19in1, 0, 199?, "Sega License", "Super 19-in-1 (pirate)", 0, 0 )

	SOFTWARE( sb2020, 0, 199?, "Sega License", "Super Baseball 2020 (Euro, USA)", 0, 0 )
	SOFTWARE( 2020to, sb2020, 199?, "Sega License", "2020 Toshi Super Baseball (Jpn)", 0, 0 )

	SOFTWARE( sbship, 0, 199?, "Sega License", "Super Battleship (USA)", 0, 0 )

	SOFTWARE( sbtank, 0, 199?, "Sega License", "Super Battletank - War in the Gulf (USA)", 0, 0 )

	SOFTWARE( supbub, 0, 199?, "<unlicensed>", "Super Bubble Bobble (China)", 0, 0 )

	SOFTWARE( superd, 0, 199?, "Sega License", "Super Daisenryaku (Jpn)", 0, 0 )

	SOFTWARE( sdkong99, 0, 199?, "<unlicensed>", "Super Donkey Kong 99 (Protected)", 0, 0 )
	SOFTWARE( sdkong99a, sdkong99, 199?, "<unlicensed>", "Super Donkey Kong 99 (Unprotected)", 0, 0 )	// [!]
	SOFTWARE( skkong99, sdkong99, 199?, "<unlicensed>", "Super King Kong 99", 0, 0 )	// [!]

	SOFTWARE( sfzone, 0, 199?, "Sega License", "Super Fantasy Zone (Euro)", 0, 0 )
	SOFTWARE( sfzonej, sfzone, 199?, "Sega License", "Super Fantasy Zone (Jpn)", 0, 0 )

	SOFTWARE( superhq, 0, 199?, "Sega License", "Super H.Q. (Jpn)", 0, 0 )

	SOFTWARE( shangon, 0, 199?, "Sega License", "Super Hang-On (World, Rev. A)", 0, 0 )
	SOFTWARE( shangona, shangon, 199?, "Sega License", "Super Hang-On (World)", 0, 0 )

	SOFTWARE( shi, 0, 199?, "Sega License", "Super High Impact (USA)", 0, 0 )

	SOFTWARE( suphy, 0, 199?, "Sega License", "Super Hydlide (Euro)", 0, 0 )
	SOFTWARE( suphyj, suphy, 199?, "Sega License", "Super Hydlide (Jpn)", 0, 0 )
	SOFTWARE( suphyu, suphy, 199?, "Sega License", "Super Hydlide (USA)", 0, 0 )

	SOFTWARE( suprkick, 0, 199?, "Sega License", "Super Kick Off (Euro)", 0, 0 )

	SOFTWARE( suplg, 0, 199?, "Sega License", "Super League (Euro)", 0, 0 )
	SOFTWARE( suplgj, suplg, 199?, "Sega License", "Super League (Jpn)", 0, 0 )

	SOFTWARE( smario2, 0, 199?, "<unlicensed>", "Super Mario 2 1998", 0, 0 )	// [!]

	SOFTWARE( smb, 0, 199?, "<unlicensed>", "Super Mario Bros.", 0, 0 )	// [!]
	SOFTWARE( smw, smb, 199?, "<unlicensed>", "Super Mario World", 0, 0 )	// [!]
	SOFTWARE( smwa, smb, 199?, "<unlicensed>", "Super Mario World (Pirate)", 0, 0 )

	SOFTWARE( smgp, 0, 199?, "Sega License", "Super Monaco GP (USA)", 0, 0 )
	SOFTWARE( smgpj, smgp, 199?, "Sega License", "Super Monaco GP (Jpn)", 0, 0 )
	SOFTWARE( smgpa, smgp, 199?, "Sega License", "Super Monaco GP (Euro, Jpn, Rev. A)", 0, 0 )
	SOFTWARE( smgpb, smgp, 199?, "Sega License", "Super Monaco GP (Euro, Jpn)", 0, 0 )

	SOFTWARE( superoff, 0, 199?, "Sega License", "Super Off Road (Euro, USA)", 0, 0 )

	SOFTWARE( srb, 0, 199?, "Sega License", "Super Real Basketball (Euro)", 0, 0 )
	SOFTWARE( srbj, srb, 199?, "Sega License", "Super Real Basketball (Jpn, Korea)", 0, 0 )

	SOFTWARE( sups2, 0, 199?, "Sega License", "The Super Shinobi II (Jpn, Korea)", 0, 0 )
	SOFTWARE( sups2a, sups2, 199?, "Sega License", "The Super Shinobi II (Jpn, Prototype, Earlier)", 0, 0 )
	SOFTWARE( sups2b, sups2, 199?, "Sega License", "The Super Shinobi II (Jpn, Prototype)", 0, 0 )

	SOFTWARE( supshi, 0, 199?, "Sega License", "The Super Shinobi (Jpn)", 0, 0 )

	SOFTWARE( sskid, 0, 199?, "Sega License", "Super Skidmarks (Euro, J-Cart)", 0, 0 )
	SOFTWARE( sskidb, sskid, 199?, "Sega License", "Super Skidmarks (USA, Prototype)", 0, 0 )

	SOFTWARE( ssmashtv, 0, 199?, "Sega License", "Super Smash TV (Euro, USA)", 0, 0 )

	SOFTWARE( ssf2, 0, 199?, "Sega License", "Super Street Fighter II - The New Challengers (Euro)", 0, 0 )
	SOFTWARE( ssf2j, ssf2, 199?, "Sega License", "Super Street Fighter II - The New Challengers (Jpn)", 0, 0 )
	SOFTWARE( ssf2u, ssf2, 199?, "Sega License", "Super Street Fighter II - The New Challengers (USA)", 0, 0 )

	SOFTWARE( stblad, 0, 199?, "Sega License", "Super Thunder Blade (World)", 0, 0 )
	SOFTWARE( stbladj, stb, 199?, "Sega License", "Super Thunder Blade (Launch Cart) (Jpn)", 0, 0 )

	SOFTWARE( svb, 0, 199?, "Sega License", "Super Volley Ball (USA)", 0, 0 )
	SOFTWARE( svbj, svb, 199?, "Sega License", "Super Volley Ball (Jpn)", 0, 0 )
	SOFTWARE( svb1, svb, 199?, "Sega License", "Super Volley Ball (USA, Alt)", 0, 0 )

	SOFTWARE( supman, 0, 199?, "Sega License", "Superman - The Man of Steel (Euro)", 0, 0 )
	SOFTWARE( supmanu, supman, 199?, "Sega License", "Superman (USA)", 0, 0 )
	SOFTWARE( supmanb, supman, 199?, "Sega License", "Superman (USA, Prototype)", 0, 0 )

	SOFTWARE( surgin, 0, 199?, "Sega License", "Surging Aura (Jpn)", 0, 0 )

	SOFTWARE( swordsod, 0, 199?, "Sega License", "Sword of Sodan (Euro, USA)", 0, 0 )
	SOFTWARE( swordsodj, swordsod, 199?, "Sega License", "Sword of Sodan (Jpn)", 0, 0 )

	SOFTWARE( swordver, 0, 199?, "Sega License", "Sword of Vermilion (Euro, USA)", 0, 0 )

	SOFTWARE( sydofv, 0, 199?, "Sega License", "Syd of Valis (USA)", 0, 0 )
	SOFTWARE( sdvalis, sydofv, 199?, "Sega License", "SD Valis (Jpn)", 0, 0 )

	SOFTWARE( sylves, 0, 199?, "Sega License", "Sylvester & Tweety in Cagey Capers (Euro)", 0, 0 )
	SOFTWARE( sylvesu, sylves, 199?, "Sega License", "Sylvester and Tweety in Cagey Capers (USA)", 0, 0 )

	SOFTWARE( syndic, 0, 199?, "Sega License", "Syndicate (Euro, USA)", 0, 0 )

	SOFTWARE( t2term, 0, 199?, "Sega License", "T2 - Terminator 2 - Judgment Day (Euro, USA)", 0, 0 )

	SOFTWARE( t2ag, 0, 199?, "Sega License", "T2 - The Arcade Game (Euro, USA)", 0, 0 )
	SOFTWARE( t2agb, t2ag, 199?, "Sega License", "T2 - The Arcade Game (USA, Prototype)", 0, 0 )
	SOFTWARE( t2agj, t2ag, 199?, "Sega License", "T2 - The Arcade Game (Jpn)", 0, 0 )

	SOFTWARE( taikou, 0, 199?, "Koei", "Taikou Risshiden (Jpn)", 0, 0 )

	SOFTWARE( taiwan, 0, 199?, "<unlicensed>", "Taiwan Daheng (China)", 0, 0 )

	SOFTWARE( talespin, 0, 199?, "Sega License", "TaleSpin (Euro, USA)", 0, 0 )

	SOFTWARE( talmit, 0, 199?, "Sega License", "Talmit's Adventure (Euro)", 0, 0 )
	SOFTWARE( mvlnd, talmit, 199?, "Sega License", "Marvel Land (USA)", 0, 0 )
	SOFTWARE( mvlndj, talmit, 199?, "Sega License", "Marvel Land (Jpn)", 0, 0 )

	SOFTWARE( target, 0, 199?, "Sega License", "Target Earth (USA)", 0, 0 )
	SOFTWARE( assaul, target, 199?, "Sega License", "Assault Suit Leynos (Jpn)", 0, 0 )

	SOFTWARE( tfh, 0, 199?, "Sega License", "Task Force Harrier EX (USA)", 0, 0 )
	SOFTWARE( tfhj, tfh, 199?, "Sega License", "Task Force Harrier EX (Jpn)", 0, 0 )

	SOFTWARE( tatsujin, 0, 199?, "Sega License", "Tatsujin / Truxton (World)", 0, 0 )

	SOFTWARE( tazmania, 0, 199?, "Sega License", "Taz-Mania (World)", 0, 0 )

	SOFTWARE( teamusa, 0, 199?, "Sega License", "Team USA Basketball (Euro, USA)", 0, 0 )
	SOFTWARE( dreamteam, teamusa, 199?, "Sega License", "Dream Team USA (Jpn)", 0, 0 ) // USA Basketball World Challenge

	SOFTWARE( technocl, 0, 199?, "Sega License", "Techno Clash (Euro, USA)", 0, 0 )

	SOFTWARE( tcop, 0, 199?, "Sega License", "Technocop (USA)", 0, 0 )

	SOFTWARE( tecmoc, 0, 199?, "<pirate>", "Tecmo Cup (pirate) (Jpn)", 0, 0 )
	SOFTWARE( tecmoc1, tecmoc, 199?, "Sega License", "Tecmo Cup (Prototype) (Jpn)", 0, 0 )	// BAD DUMP

	SOFTWARE( tecmos, 0, 199?, "Sega License", "Tecmo Super Baseball (USA)", 0, 0 )

	SOFTWARE( tsb, 0, 199?, "Sega License", "Tecmo Super Bowl (October 1993) (USA)", 0, 0 )
	SOFTWARE( tsbo, tsb, 199?, "Sega License", "Tecmo Super Bowl (September 1993) (USA)", 0, 0 )
	SOFTWARE( tsbj, tsb, 199?, "Sega License", "Tecmo Super Bowl (Jpn)", 0, 0 )

	SOFTWARE( tsb2, 0, 199?, "Sega License", "Tecmo Super Bowl II - Special Edition (USA)", 0, 0 )
	SOFTWARE( tsb2j, tsb2, 199?, "Sega License", "Tecmo Super Bowl II - Special Edition (Jpn)", 0, 0 )

	SOFTWARE( tsb3, 0, 199?, "Sega License", "Tecmo Super Bowl III - Final Edition (USA)", 0, 0 )

	SOFTWARE( tsh, 0, 199?, "Sega License", "Tecmo Super Hockey (USA)", 0, 0 )

	SOFTWARE( tsbb, 0, 199?, "Sega License", "Tecmo Super NBA Basketball (USA)", 0, 0 )
	SOFTWARE( tsbbj, tsbb, 199?, "Sega License", "Tecmo Super NBA Basketball (Jpn)", 0, 0 )

	SOFTWARE( tecmow92, 0, 199?, "Sega License", "Tecmo World Cup '92 (Jpn)", 0, 0 )

	SOFTWARE( tecmow, 0, 199?, "Sega License", "Tecmo World Cup (USA)", 0, 0 )

	SOFTWARE( teddyboy, 0, 199?, "Sega License", "Teddy Boy Blues (Jpn, SegaNet)", 0, 0 )

	SOFTWARE( tmhsh, 0, 199?, "Sega License", "Teenage Mutant Hero Turtles - The Hyperstone Heist (Euro)", 0, 0 )
	SOFTWARE( tmhshj, tmhsh, 199?, "Sega License", "Teenage Mutant Ninja Turtles - Return of the Shredder (Jpn)", 0, 0 )
	SOFTWARE( tmhshu, tmhsh, 199?, "Sega License", "Teenage Mutant Ninja Turtles - The Hyperstone Heist (USA)", 0, 0 )

	SOFTWARE( tmtf, 0, 199?, "Sega License", "Teenage Mutant Hero Turtles - Tournament Fighters (Euro)", 0, 0 )
	SOFTWARE( tmtfj, tmtf, 199?, "Sega License", "Teenage Mutant Ninja Turtles - Tournament Fighters (Jpn)", 0, 0 )
	SOFTWARE( tmtfu, tmtf, 199?, "Sega License", "Teenage Mutant Ninja Turtles - Tournament Fighters (USA)", 0, 0 )

	SOFTWARE( tekk3sp, 0, 199?, "<unlicensed>", "Tekken 3 Special", 0, 0 )

	SOFTWARE( telmah, 0, 199?, "Sega License", "Tel-Tel Mahjong (Jpn)", 0, 0 )

	SOFTWARE( telsta, 0, 199?, "Sega License", "Tel-Tel Stadium (Jpn)", 0, 0 )

	SOFTWARE( telebr, 0, 199?, "Sega License", "Telebradesco Residencia (Brazil)", 0, 0 )

	SOFTWARE( termintr, 0, 199?, "Sega License", "The Terminator (Euro)", 0, 0 )
	SOFTWARE( termintru, termintr, 199?, "Sega License", "The Terminator (USA)", 0, 0 )

	SOFTWARE( testdrv2, 0, 199?, "Sega License", "Test Drive II - The Duel (Euro, USA)", 0, 0 )

	SOFTWARE( tetris, 0, 199?, "Sega License", "Tetris (Jpn)", 0, 0 )

	SOFTWARE( redcliff, 0, 199?, "<unlicensed>", "The Battle of Red Cliffs - Romance of the Three Kingdoms", 0, 0 )

	SOFTWARE( kof99, 0, 199?, "<unlicensed>", "The King of Fighters 99", 0, 0 )

	SOFTWARE( themeprk, 0, 199?, "Sega License", "Theme Park (Euro, USA)", 0, 0 )

	SOFTWARE( thomas, 0, 199?, "Sega License", "Thomas the Tank Engine & Friends (USA)", 0, 0 )

	SOFTWARE( tfii, 0, 199?, "Sega License", "Thunder Force II (Euro, USA)", 0, 0 )
	SOFTWARE( tfiij, tfii, 199?, "Sega License", "Thunder Force II MD (Jpn)", 0, 0 )

	SOFTWARE( tfiii, 0, 199?, "Sega License", "Thunder Force III (Jpn, USA)", 0, 0 )

	SOFTWARE( tfiv, 0, 199?, "Sega License", "Thunder Force IV (Euro)", 0, 0 )
	SOFTWARE( tfivj, tfiv, 199?, "Sega License", "Thunder Force IV (Jpn)", 0, 0 )
	SOFTWARE( lighte, tfiv, 199?, "Sega License", "Lightening Force - Quest for the Darkstar (USA)", 0, 0 )

	SOFTWARE( tfox, 0, 199?, "Sega License", "Thunder Fox (USA)", 0, 0 )
	SOFTWARE( tfoxj, tfox, 199?, "Sega License", "Thunder Fox (Jpn)", 0, 0 )

	SOFTWARE( tpwres, 0, 199?, "Sega License", "Thunder Pro Wrestling Retsuden (Jpn)", 0, 0 )

	SOFTWARE( tick, 0, 199?, "Sega License", "The Tick (USA)", 0, 0 )

	SOFTWARE( timekill, 0, 199?, "Sega License", "Time Killers (Euro)", 0, 0 )
	SOFTWARE( timekillu, timekill, 199?, "Sega License", "Time Killers (USA)", 0, 0 )

	SOFTWARE( tinhead, 0, 199?, "Sega License", "TinHead (USA)", 0, 0 )

	SOFTWARE( tintin, 0, 199?, "Sega License", "Tintin au Tibet (Euro)", 0, 0 )

	SOFTWARE( ttacme, 0, 199?, "Sega License", "Tiny Toon Adventures - Acme All-Stars (Euro)", 0, 0 )
	SOFTWARE( ttacmeu, ttacme, 199?, "Sega License", "Tiny Toon Adventures - Acme All-Stars (Korea, USA)", 0, 0 )

	SOFTWARE( tiny, 0, 199?, "Sega License", "Tiny Toon Adventures - Buster's Hidden Treasure (Euro)", 0, 0 )
	SOFTWARE( tinyu, tiny, 199?, "Sega License", "Tiny Toon Adventures - Buster's Hidden Treasure (USA)", 0, 0 )
	SOFTWARE( tinyk, tiny, 199?, "Sega License", "Tiny Toon Adventures (Korea)", 0, 0 )

	SOFTWARE( tnnbass, 0, 199?, "Sega License", "TNN Bass Tournament of Champions (USA)", 0, 0 )

	SOFTWARE( tnnout, 0, 199?, "Sega License", "TNN Outdoors Bass Tournament '96 (USA)", 0, 0 )

	SOFTWARE( toddslim, 0, 199?, "Sega License", "Todd's Adventures in Slime World (USA)", 0, 0 )
	SOFTWARE( slimew, toddslim, 199?, "Sega License", "Slime World (Jpn)", 0, 0 )

	SOFTWARE( toejam, 0, 199?, "Sega License", "Toe Jam & Earl (World, Rev. A)", 0, 0 )
	SOFTWARE( toejam1, toejam, 199?, "Sega License", "Toe Jam & Earl (World)", 0, 0 )

	SOFTWARE( tje2, 0, 199?, "Sega License", "Toe Jam & Earl in Panic on Funkotron (Euro)", 0, 0 )
	SOFTWARE( tje2g, tje2, 199?, "Sega License", "Toe Jam & Earl in Panic auf Funkotron (Germany) (Euro)", 0, 0 )
	SOFTWARE( tje2j, tje2, 199?, "Sega License", "Toe Jam & Earl in Panic on Funkotron (Jpn)", 0, 0 )
	SOFTWARE( tje2u, tje2, 199?, "Sega License", "Toe Jam & Earl in Panic on Funkotron (USA)", 0, 0 )

	SOFTWARE( tomjerry, 0, 199?, "Sega License", "Tom and Jerry - Frantic Antics (1993) (USA)", 0, 0 )
	SOFTWARE( tomjerry1, tomjerry, 199?, "Sega License", "Tom and Jerry - Frantic Antics (1994) (USA)", 0, 0 )

	SOFTWARE( tommylb, 0, 199?, "Sega License", "Tommy Lasorda Baseball (USA)", 0, 0 )

	SOFTWARE( tonylrb, 0, 199?, "Sega License", "Tony La Russa Baseball (Oceania, USA)", 0, 0 )

	SOFTWARE( topfight, 0, 199?, "<unlicensed>", "Top Fighter 2000 MK VIII", 0, 0 )	//[!]

	SOFTWARE( topgear2, 0, 199?, "Sega License", "Top Gear 2 (USA)", 0, 0 )

	SOFTWARE( tpgolf, 0, 199?, "Sega License", "Top Pro Golf (Jpn)", 0, 0 )

	SOFTWARE( tpgolf2, 0, 199?, "Sega License", "Top Pro Golf 2 (Jpn)", 0, 0 )

	SOFTWARE( totlfoot, 0, 199?, "Sega License", "Total Football (Euro)", 0, 0 )

	SOFTWARE( toughman, 0, 199?, "Sega License", "Toughman Contest (Euro, USA)", 0, 0 )

	SOFTWARE( tougiou, 0, 199?, "Sega License", "Tougiou King Colossus (Jpn)", 0, 0 )

	SOFTWARE( toxicc, 0, 199?, "Sega License", "Toxic Crusaders (USA)", 0, 0 )

	SOFTWARE( toysto, 0, 199?, "Sega License", "Toy Story (Euro)", 0, 0 )
	SOFTWARE( toystou, toysto, 199?, "Sega License", "Toy Story (USA)", 0, 0 )

	SOFTWARE( toys, 0, 199?, "Sega License", "Toys (USA)", 0, 0 )

	SOFTWARE( trampoli, 0, 199?, "Sega License", "Trampoline Terror! (USA)", 0, 0 )

	SOFTWARE( traysia, 0, 199?, "Sega License", "Traysia (USA)", 0, 0 )
	SOFTWARE( minato, traysia, 199?, "Sega License", "Minato no Traysia (Jpn)", 0, 0 )

	SOFTWARE( triple96, 0, 199?, "Sega License", "Triple Play '96 (USA)", 0, 0 )

	SOFTWARE( tpgld, 0, 199?, "Sega License", "Triple Play Gold (USA)", 0, 0 )
	SOFTWARE( tpglda, tpgld, 199?, "Sega License", "Triple Play Gold (USA, Alt)", 0, 0 )

	SOFTWARE( troubsht, 0, 199?, "Sega License", "Trouble Shooter (USA)", 0, 0 )
	SOFTWARE( battlema, troubsht, 199?, "Sega License", "Battle Mania (Jpn)", 0, 0 )

	SOFTWARE( troyaik, 0, 199?, "Sega License", "Troy Aikman NFL Football (USA)", 0, 0 )

	SOFTWARE( truelies, 0, 199?, "Sega License", "True Lies (World)", 0, 0 )

	SOFTWARE( tunshi, 0, 199?, "<unlicensed>", "Tun Shi Tian Di III (China, Simple Chinese)", 0, 0 )
	SOFTWARE( tunshi1, tunshi, 199?, "<unlicensed>", "Tun Shi Tian Di III (China)", 0, 0 )

	SOFTWARE( turboout, 0, 199?, "Sega License", "Turbo OutRun (Euro, Jpn)", 0, 0 )

	SOFTWARE( turrican, 0, 199?, "Sega License", "Turrican (Euro, USA)", 0, 0 )

	SOFTWARE( twincobr, 0, 199?, "Sega License", "Twin Cobra (USA)", 0, 0 )
	SOFTWARE( kyuuky, twincobr, 199?, "Sega License", "Kyuukyoku Tiger (Jpn)", 0, 0 )

	SOFTWARE( twinklet, 0, 199?, "Sega License", "Twinkle Tale (Jpn)", 0, 0 )

	SOFTWARE( twocrude, 0, 199?, "Sega License", "Two Crude Dudes (Euro)", 0, 0 )
	SOFTWARE( twocrude, twocrude, 199?, "Sega License", "Two Crude Dudes (USA)", 0, 0 )
	SOFTWARE( crudeb, twocrude, 199?, "Sega License", "Crude Buster (Jpn)", 0, 0 )

	SOFTWARE( twotribe, 0, 199?, "Sega License", "Two Tribes - Populous II (Euro)", 0, 0 )

	SOFTWARE( uchuusen, 0, 199?, "Sega License", "Uchuu Senkan Gomora (Jpn)", 0, 0 )

	SOFTWARE( umk3, 0, 199?, "Sega License", "Ultimate Mortal Kombat 3 (Euro)", 0, 0 )
	SOFTWARE( umk3u, umk3, 199?, "Sega License", "Ultimate Mortal Kombat 3 (USA)", 0, 0 )

	SOFTWARE( uqix, 0, 199?, "Sega License", "Ultimate Qix (USA)", 0, 0 )

	SOFTWARE( usoccr, 0, 199?, "Sega License", "Ultimate Soccer (Euro)", 0, 0 )

	SOFTWARE( ultraman, 0, 199?, "Sega License", "Ultraman (Jpn)", 0, 0 )

	SOFTWARE( unchw, 0, 199?, "Koei", "Uncharted Waters (USA)", 0, 0 )
	SOFTWARE( daik, unchw, 199?, "Koei", "Daikoukai Jidai (Jpn)", 0, 0 )

	SOFTWARE( unchw2, 0, 199?, "Koei", "Uncharted Waters - New Horizons (USA)", 0, 0 )
	SOFTWARE( daik2, unchw2, 199?, "Koei", "Daikoukai Jidai II (Jpn)", 0, 0 )

	SOFTWARE( undead, 0, 199?, "Sega License", "Undead Line (Jpn)", 0, 0 )

	SOFTWARE( universs, 0, 199?, "Sega License", "Universal Soldier (Euro, USA)", 0, 0 )

	SOFTWARE( unnecess, 0, 199?, "Sega License", "Unnecessary Roughness 95 (USA)", 0, 0 )

	SOFTWARE( urbanstr, 0, 199?, "Sega License", "Urban Strike (Euro, USA)", 0, 0 )

	SOFTWARE( uzukeo, 0, 199?, "Sega License", "Uzu Keobukseon (Korea)", 0, 0 )

	SOFTWARE( valis, 0, 199?, "Sega License", "Valis (USA)", 0, 0 )
	SOFTWARE( mugens, valis, 199?, "Sega License", "Mugen Senshi Valis (Jpn)", 0, 0 )

	SOFTWARE( valis3, 0, 199?, "Sega License", "Valis III (USA)", 0, 0 )
	SOFTWARE( valis3j, valis3, 199?, "Sega License", "Valis III (Jpn, Rev. A)", 0, 0 )

	SOFTWARE( vaportr, 0, 199?, "Sega License", "Vapor Trail (USA)", 0, 0 )
	SOFTWARE( kuuga, vaportr, 199?, "Sega License", "Kuuga - Operation Code 'Vapor Trail' (Jpn)", 0, 0 )

	SOFTWARE( vecman, 0, 199?, "Sega License", "Vectorman (Euro, USA)", 0, 0 )
	SOFTWARE( vecmana, vecman, 199?, "Sega License", "Vectorman (Prototype)", 0, 0 )
	SOFTWARE( vecmanb, vecman, 199?, "Sega License", "Vectorman (Prototype, 19950724)", 0, 0 )
	SOFTWARE( vecmanc, vecman, 199?, "Sega License", "Vectorman (USA, Prototype)", 0, 0 )

	SOFTWARE( vecman2, 0, 199?, "Sega License", "Vectorman 2 (USA)", 0, 0 )
	SOFTWARE( vecman2a, vecman2, 199?, "Sega License", "Vectorman 2 (USA, Prototype)", 0, 0 )
	SOFTWARE( vecman2b, vecman2, 199?, "Sega License", "Vectorman 2 (Prototype, 19960815)", 0, 0 )
	SOFTWARE( vecman2c, vecman2, 199?, "Sega License", "Vectorman 2 (Prototype, 19960816)", 0, 0 )
	SOFTWARE( vecman2d, vecman2, 199?, "Sega License", "Vectorman 2 (Prototype, 19960819)", 0, 0 )
	SOFTWARE( vecman2e, vecman2, 199?, "Sega License", "Vectorman 2 (Prototype, 19960826)", 0, 0 )
	SOFTWARE( vecman2f, vecman2, 199?, "Sega License", "Vectorman 2 (Prototype, 19960827)", 0, 0 )

	SOFTWARE( vermil, 0, 199?, "Sega License", "Vermilion (Jpn)", 0, 0 )

	SOFTWARE( verytex, 0, 199?, "Sega License", "Verytex (Jpn)", 0, 0 )

	SOFTWARE( viewp, 0, 199?, "Sega License", "Viewpoint (USA)", 0, 0 )
	SOFTWARE( viewpb, viewp, 199?, "Sega License", "Viewpoint (USA, Prototype)", 0, 0 )

	SOFTWARE( vf2, 0, 199?, "Sega License", "Virtua Fighter 2 (Euro, USA)", 0, 0 )
	SOFTWARE( vf2a, vf2, 199?, "Sega License", "Virtua Fighter 2 (Korea)", 0, 0 )
	SOFTWARE( vf2b, vf2, 199?, "Sega License", "Virtua Fighter 2 (Prototype, 19960819)", 0, 0 )
	SOFTWARE( vf2c, vf2, 199?, "Sega License", "Virtua Fighter 2 (Prototype, 19960830)", 0, 0 )
	SOFTWARE( vf2d, vf2, 199?, "Sega License", "Virtua Fighter 2 (Prototype, 19960913)", 0, 0 )
	SOFTWARE( vf2e, vf2, 199?, "Sega License", "Virtua Fighter 2 (Prototype, 19960920)", 0, 0 )
	SOFTWARE( vf2f, vf2, 199?, "Sega License", "Virtua Fighter 2 (Prototype, 19960927)", 0, 0 )

	SOFTWARE( vf2tek, 0, 199?, "<unlicensed>", "Virtua Fighter 2 vs Tekken 2", 0, 0 )

	SOFTWARE( vbart, 0, 199?, "Sega License", "Virtual Bart (World)", 0, 0 )

	SOFTWARE( virtpb, 0, 199?, "Sega License", "Virtual Pinball (Euro, USA)", 0, 0 )

	SOFTWARE( vixen357, 0, 199?, "Sega License", "Vixen 357 (Jpn)", 0, 0 )

	SOFTWARE( volfied, 0, 199?, "Sega License", "Volfied (Jpn)", 0, 0 )

	SOFTWARE( vrtroop, 0, 199?, "Sega License", "VR Troopers (Euro, USA)", 0, 0 )

	SOFTWARE( wackyr, 0, 199?, "Sega License", "Wacky Races (Prototype) (USA)", 0, 0 )


	SOFTWARE( ww, 0, 199?, "Sega License", "Wacky Worlds Creativity Studio (USA)", 0, 0 )
	SOFTWARE( wwa, ww, 199?, "Sega License", "Wacky Worlds (Prototype, 19940808)", 0, 0 )
	SOFTWARE( wwb, ww, 199?, "Sega License", "Wacky Worlds (Prototype, 19940817)", 0, 0 )
	SOFTWARE( wwc, ww, 199?, "Sega License", "Wacky Worlds (Prototype, 19940819)", 0, 0 )

	SOFTWARE( waniwani, 0, 199?, "Sega License", "Wani Wani World (Jpn)", 0, 0 )

	SOFTWARE( wardner, 0, 199?, "Sega License", "Wardner (USA)", 0, 0 )
	SOFTWARE( wardnerj, wardner, 199?, "Sega License", "Wardner no Mori Special (Jpn)", 0, 0 )

	SOFTWARE( warlock, 0, 199?, "Sega License", "Warlock (Euro, USA)", 0, 0 )
	SOFTWARE( warlockb, warlock, 199?, "Sega License", "Warlock (USA, Prototype)", 0, 0 )

	SOFTWARE( warpsp, 0, 199?, "Sega License", "Warpspeed (USA)", 0, 0 )

	SOFTWARE( warrior, 0, 199?, "Sega License", "Warrior of Rome (USA)", 0, 0 )

	SOFTWARE( warrior2, 0, 199?, "Sega License", "Warrior of Rome II (USA)", 0, 0 )

	SOFTWARE( warsong, 0, 199?, "Sega License", "Warsong (USA)", 0, 0 )

	SOFTWARE( waterwld, 0, 199?, "Sega License", "Waterworld (Prototype) (Euro)", 0, 0 )

	SOFTWARE( wayneg, 0, 199?, "Sega License", "Wayne Gretzky and the NHLPA All-Stars (Euro, USA)", 0, 0 )

	SOFTWARE( waynewld, 0, 199?, "Sega License", "Wayne's World (USA)", 0, 0 )

	SOFTWARE( weapon, 0, 199?, "Sega License", "Weaponlord (USA)", 0, 0 )

	SOFTWARE( whacacri, 0, 199?, "<unlicensed>", "Whac-a-Critter (USA)", 0, 0 )

	SOFTWARE( wheelfor, 0, 199?, "Sega License", "Wheel of Fortune (USA)", 0, 0 )

	SOFTWARE( wwrld, 0, 199?, "Sega License", "Where in the World Is Carmen Sandiego (Euro, USA)", 0, 0 )
	SOFTWARE( wwrldb, wwrld, 199?, "Sega License", "Where in the World Is Carmen Sandiego (Brazil)", 0, 0 )

	SOFTWARE( wtime, 0, 199?, "Sega License", "Where in Time Is Carmen Sandiego (Euro, USA)", 0, 0 )
	SOFTWARE( wtimeb, wtime, 199?, "Sega License", "Where in Time Is Carmen Sandiego (Brazil)", 0, 0 )

	SOFTWARE( whiprush, 0, 199?, "Sega License", "Whip Rush (USA)", 0, 0 )
	SOFTWARE( whiprushj, whiprush, 199?, "Sega License", "Whip Rush - Wakusei Voltegas no Nazo (Jpn)", 0, 0 )

	SOFTWARE( wildsn, 0, 199?, "Sega License", "Wild Snake (Prototype) (USA)", 0, 0 )

	SOFTWARE( williams, 0, 199?, "Sega License", "Williams Arcade's Greatest Hits (USA)", 0, 0 )

	SOFTWARE( wimbled, 0, 199?, "Sega License", "Wimbledon Championship Tennis (Euro)", 0, 0 )
	SOFTWARE( wimbledj, wimbled, 199?, "Sega License", "Wimbledon Championship Tennis (Jpn)", 0, 0 )
	SOFTWARE( wimbledub, wimbled, 199?, "Sega License", "Wimbledon Championship Tennis (USA, Prototype)", 0, 0 )
	SOFTWARE( wimbledu, wimbled, 199?, "Sega License", "Wimbledon Championship Tennis (USA)", 0, 0 )

	SOFTWARE( wingswor, 0, 199?, "Sega License", "Wings of Wor (USA)", 0, 0 )

	SOFTWARE( wintc, 0, 199?, "Sega License", "Winter Challenge (Euro, USA, Rev. 1)", 0, 0 )
	SOFTWARE( wintca, wintc, 199?, "Sega License", "Winter Challenge (Euro, USA)", 0, 0 )
	SOFTWARE( wintcb, wintc, 199?, "Sega License", "Winter Challenge (Prototype)", 0, 0 )

	SOFTWARE( wintolu, wintol, 199?, "Sega License", "Winter Olympic Games (USA)", 0, 0 )
	SOFTWARE( wintol, 0, 199?, "Sega License", "Winter Olympics (Euro)", 0, 0 )
	SOFTWARE( wintolj, wintol, 199?, "Sega License", "Winter Olympics (Jpn)", 0, 0 )

	SOFTWARE( wiznliz, 0, 199?, "Sega License", "Wiz'n'Liz (USA)", 0, 0 )
	SOFTWARE( wiznliz1, wiznliz, 199?, "Sega License", "Wiz'n'Liz - The Frantic Wabbit Wescue (Euro)", 0, 0 )

	SOFTWARE( wolfch, 0, 199?, "Sega License", "Wolfchild (USA)", 0, 0 )

	SOFTWARE( wolver, 0, 199?, "Sega License", "Wolverine - Adamantium Rage (Euro, USA)", 0, 0 )

	SOFTWARE( wboy3, 0, 199?, "Sega License", "Wonder Boy III - Monster Lair (Euro, Jpn)", 0, 0 )

	SOFTWARE( wbmw, 0, 199?, "Sega License", "Wonder Boy in Monster World (Euro, USA)", 0, 0 )
	SOFTWARE( wboy5, wbmw, 199?, "Sega License", "Wonder Boy V - Monster World III (Jpn, Korea)", 0, 0 )
	SOFTWARE( turmad, wbmw, 199?, "Sega License", "Turma da Monica na Terra dos Monstros (Brazil)", 0, 0 )

	SOFTWARE( wondlib, 0, 199?, "Sega License", "Wonder Library (Jpn)", 0, 0 )

	SOFTWARE( wcs2, 0, 199?, "Sega License", "World Championship Soccer II (USA)", 0, 0 )
	SOFTWARE( wcs2a, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype, 19940223)", 0, 0 )
	SOFTWARE( wcs2b, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype, 19940309)", 0, 0 )
	SOFTWARE( wcs2c, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype, 19940314)", 0, 0 )
	SOFTWARE( wcs2d, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype, 19940323)", 0, 0 )
	SOFTWARE( wcs2e, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype, 19940324)", 0, 0 )
	SOFTWARE( wcs2f, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype, 19940325)", 0, 0 )
	SOFTWARE( wcs2g, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype, 19940326)", 0, 0 )
	SOFTWARE( wcs2h, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype, 19940327)", 0, 0 )
	SOFTWARE( wcs2i, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype, 19940329-B)", 0, 0 )
	SOFTWARE( wcs2j, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype, 19940329)", 0, 0 )
	SOFTWARE( wcs2k, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype, 19940330)", 0, 0 )
	SOFTWARE( wcs2l, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype, 19940523)", 0, 0 )
	SOFTWARE( wcs2m, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype G, 19940222)", 0, 0 )
	SOFTWARE( wcs2n, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype J, 19940228)", 0, 0 )
	SOFTWARE( wcs2o, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype N, 19940303)", 0, 0 )
	SOFTWARE( wcs2p, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype O, 19940303)", 0, 0 )
	SOFTWARE( wcs2q, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype P, 19940304)", 0, 0 )
	SOFTWARE( wcs2r, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype R, 19940309)", 0, 0 )
	SOFTWARE( wcs2s, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype U, 19940314)", 0, 0 )
	SOFTWARE( wcs2t, wcs2, 199?, "Sega License", "World Championship Soccer II (Prototype Y, 19940318)", 0, 0 )
	SOFTWARE( wcs2u, wcs2, 199?, "Sega License", "World Championship Soccer II (USA, Prototype)", 0, 0 )

	SOFTWARE( wclead, 0, 199?, "Sega License", "World Class Leaderboard Golf (Euro)", 0, 0 )
	SOFTWARE( wcleadu, wclead, 199?, "Sega License", "World Class Leaderboard Golf (USA)", 0, 0 )

	SOFTWARE( wc90, 0, 199?, "Sega License", "World Cup Italia '90 (Euro)", 0, 0 )

	SOFTWARE( wcs, 0, 199?, "Sega License", "World Cup Soccer / World Championship Soccer (Jpn, USA, Rev. B)", 0, 0 )
	SOFTWARE( wcsa, wcs, 199?, "Sega License", "World Cup Soccer / World Championship Soccer (Jpn, USA, v1.2)", 0, 0 )
	SOFTWARE( wcsb, wcs, 199?, "Sega License", "World Cup Soccer / World Championship Soccer (Jpn, USA)", 0, 0 )

	SOFTWARE( wcup94, 0, 199?, "Sega License", "World Cup USA 94 (Euro, Korea, USA)", 0, 0 )

	SOFTWARE( whero, 0, 199?, "Sega License", "World Heroes (USA)", 0, 0 )
	SOFTWARE( wheroa, whero, 199?, "Sega License", "World Heroes (Jpn)", 0, 0 )
	SOFTWARE( wherob, whero, 199?, "Sega License", "World Heroes (Prototype, 19940331-B) (Euro)", 0, 0 )
	SOFTWARE( wheroc, whero, 199?, "Sega License", "World Heroes (Prototype, 19940331) (Euro)", 0, 0 )
	SOFTWARE( wherod, whero, 199?, "Sega License", "World Heroes (Prototype, 19940408) (Jpn)", 0, 0 )
	SOFTWARE( wheroe, whero, 199?, "Sega License", "World Heroes (Prototype, 19940415) (Jpn)", 0, 0 )
	SOFTWARE( wherof, whero, 199?, "Sega License", "World Heroes (Prototype, 19940420-B) (Jpn)", 0, 0 )
	SOFTWARE( wherog, whero, 199?, "Sega License", "World Heroes (Prototype, 19940420, broken - C05 missing) (Jpn)", 0, 0 )
	SOFTWARE( wheroh, whero, 199?, "Sega License", "World Heroes (Prototype, 19940330) (Jpn)", 0, 0 )
	SOFTWARE( wheroi, whero, 199?, "Sega License", "World Heroes (Prototype, 19940223) (USA)", 0, 0 )
	SOFTWARE( wheroj, whero, 199?, "Sega License", "World Heroes (Prototype, 19940224) (USA)", 0, 0 )
	SOFTWARE( wherok, whero, 199?, "Sega License", "World Heroes (Prototype, 19940303) (USA)", 0, 0 )
	SOFTWARE( wherol, whero, 199?, "Sega License", "World Heroes (Prototype, 19940307) (USA)", 0, 0 )
	SOFTWARE( wherom, whero, 199?, "Sega License", "World Heroes (Prototype, 19940309) (USA)", 0, 0 )
	SOFTWARE( wheron, whero, 199?, "Sega License", "World Heroes (Prototype, 19940315) (USA)", 0, 0 )
	SOFTWARE( wheroo, whero, 199?, "Sega License", "World Heroes (Prototype, 19940316) (USA)", 0, 0 )
	SOFTWARE( wherop, whero, 199?, "Sega License", "World Heroes (Prototype, 19940318) (USA)", 0, 0 )
	SOFTWARE( wheroq, whero, 199?, "Sega License", "World Heroes (Prototype, 19940322, broken - C07 missing) (USA)", 0, 0 )
	SOFTWARE( wheror, whero, 199?, "Sega License", "World Heroes (Prototype, 19940323) (USA)", 0, 0 )
	SOFTWARE( wheros, whero, 199?, "Sega License", "World Heroes (Prototype, 19940324) (USA)", 0, 0 )
	SOFTWARE( wherot, whero, 199?, "Sega License", "World Heroes (Prototype, 19940330) (USA)", 0, 0 )

	SOFTWARE( worldill, 0, 199?, "Sega License", "World of Illusion Starring Mickey Mouse and Donald Duck (Euro)", 0, 0 )
	SOFTWARE( worldillu, worldill, 199?, "Sega License", "World of Illusion Starring Mickey Mouse and Donald Duck (Korea, USA)", 0, 0 )
	SOFTWARE( worldillj, worldill, 199?, "Sega License", "World of Illusion - Fushigi na Magic Box (Jpn)", 0, 0 )
	SOFTWARE( worldilljb, worldill, 199?, "Sega License", "World of Illusion - Fushigi na Magic Box(Jpn, Prototype)", 0, 0 )

	SOFTWARE( wsb95, 0, 199?, "Sega License", "World Series Baseball '95 (USA)", 0, 0 )
	SOFTWARE( wsb95a, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19941208)", 0, 0 )
	SOFTWARE( wsb95b, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19941214)", 0, 0 )
	SOFTWARE( wsb95c, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19941228-SB)", 0, 0 )
	SOFTWARE( wsb95d, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950202)", 0, 0 )
	SOFTWARE( wsb95e, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950203)", 0, 0 )
	SOFTWARE( wsb95f, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950207)", 0, 0 )
	SOFTWARE( wsb95g, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950209-B)", 0, 0 )
	SOFTWARE( wsb95h, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950209)", 0, 0 )
	SOFTWARE( wsb95i, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950211)", 0, 0 )
	SOFTWARE( wsb95j, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950212)", 0, 0 )
	SOFTWARE( wsb95k, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950213)", 0, 0 )
	SOFTWARE( wsb95l, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950214)", 0, 0 )
	SOFTWARE( wsb95m, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950101-TST)", 0, 0 )
	SOFTWARE( wsb95n, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950103-TST)", 0, 0 )
	SOFTWARE( wsb95o, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950105)", 0, 0 )
	SOFTWARE( wsb95p, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950109-TST)", 0, 0 )
	SOFTWARE( wsb95q, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950110)", 0, 0 )
	SOFTWARE( wsb95r, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950114-RM)", 0, 0 )
	SOFTWARE( wsb95s, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950116)", 0, 0 )
	SOFTWARE( wsb95t, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950118-RM)", 0, 0 )
	SOFTWARE( wsb95u, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950120)", 0, 0 )
	SOFTWARE( wsb95v, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950125)", 0, 0 )
	SOFTWARE( wsb95w, wsb95, 199?, "Sega License", "World Series Baseball '95 (Prototype, 19950130)", 0, 0 )

	SOFTWARE( wsb96, 0, 199?, "Sega License", "World Series Baseball '96 (USA)", 0, 0 )

	SOFTWARE( wsb98, 0, 199?, "Sega License", "World Series Baseball '98 (USA)", 0, 0 )

	SOFTWARE( wsb, 0, 199?, "Sega License", "World Series Baseball (USA)", 0, 0 )
	SOFTWARE( wsba, wsb, 199?, "Sega License", "World Series Baseball (Prototype, 19931222)", 0, 0 )
	SOFTWARE( wsbb, wsb, 199?, "Sega License", "World Series Baseball (Prototype, 19931226)", 0, 0 )
	SOFTWARE( wsbc, wsb, 199?, "Sega License", "World Series Baseball (Prototype, 19931229)", 0, 0 )
	SOFTWARE( wsbd, wsb, 199?, "Sega License", "World Series Baseball (Prototype, 19940218)", 0, 0 )
	SOFTWARE( wsbe, wsb, 199?, "Sega License", "World Series Baseball (Prototype, 19940103)", 0, 0 )
	SOFTWARE( wsbf, wsb, 199?, "Sega License", "World Series Baseball (Prototype, 19940106)", 0, 0 )
	SOFTWARE( wsbg, wsb, 199?, "Sega License", "World Series Baseball (Prototype, 19940116)", 0, 0 )
	SOFTWARE( wsbh, wsb, 199?, "Sega License", "World Series Baseball (Prototype, 19940304)", 0, 0 )
	SOFTWARE( wsbi, wsb, 199?, "Sega License", "World Series Baseball (Prototype, 19940327)", 0, 0 )
	SOFTWARE( wsbj, wsb, 199?, "Sega License", "World Series Baseball (Prototype, 19931001)", 0, 0 )

	SOFTWARE( worldt, 0, 199?, "Sega License", "World Trophy Soccer (USA)", 0, 0 )

	SOFTWARE( worms, 0, 199?, "Sega License", "Worms (Euro)", 0, 0 )
	SOFTWARE( wormsb, worms, 199?, "Sega License", "Worms (Euro, Prototype)", 0, 0 )

	SOFTWARE( wresw, 0, 199?, "Sega License", "Wrestle War (Euro, Jpn)", 0, 0 )
	SOFTWARE( wreswb, wresw, 199?, "Sega License", "Wrestle War (Jpn, Prototype)", 0, 0 )

	SOFTWARE( wukong, 0, 199?, "<unlicensed>", "Wu Kong Wai Zhuan (China)", 0, 0 )

	SOFTWARE( wwfraw, 0, 199?, "Sega License", "WWF Raw (World)", 0, 0 )

	SOFTWARE( wwfroy, 0, 199?, "Sega License", "WWF Royal Rumble (World)", 0, 0 )

	SOFTWARE( wwfsup, 0, 199?, "Sega License", "WWF Super WrestleMania (Euro, USA)", 0, 0 )

	SOFTWARE( wwfag, 0, 199?, "Sega License", "WWF WrestleMania - The Arcade Game (Euro, USA)", 0, 0 )
	SOFTWARE( wwfaga, wwfag, 199?, "Sega License", "WWF WrestleMania - The Arcade Game (Alpha) (USA)", 0, 0 )

	SOFTWARE( xmen, 0, 199?, "Sega License", "X-Men (Euro)", 0, 0 )
	SOFTWARE( xmenus, xmen, 199?, "Sega License", "X-Men (USA)", 0, 0 )

	SOFTWARE( xmen2, 0, 199?, "Sega License", "X-Men 2 - Clone Wars (Euro, USA)", 0, 0 )
	SOFTWARE( xmen2a, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19941202)", 0, 0 )
	SOFTWARE( xmen2b, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19941203)", 0, 0 )
	SOFTWARE( xmen2c, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19941206)", 0, 0 )
	SOFTWARE( xmen2d, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19941207)", 0, 0 )
	SOFTWARE( xmen2e, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19941208)", 0, 0 )
	SOFTWARE( xmen2f, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19941209)", 0, 0 )
	SOFTWARE( xmen2g, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19941210)", 0, 0 )
	SOFTWARE( xmen2h, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19941211-A)", 0, 0 )
	SOFTWARE( xmen2i, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19941211)", 0, 0 )
	SOFTWARE( xmen2j, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19941214)", 0, 0 )
	SOFTWARE( xmen2k, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19941215)", 0, 0 )
	SOFTWARE( xmen2l, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19941216)", 0, 0 )
	SOFTWARE( xmen2m, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19940506)", 0, 0 )
	SOFTWARE( xmen2n, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19940510)", 0, 0 )
	SOFTWARE( xmen2o, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19941117)", 0, 0 )
	SOFTWARE( xmen2p, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19941123)", 0, 0 )
	SOFTWARE( xmen2q, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19941128)", 0, 0 )
	SOFTWARE( xmen2r, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19941130)", 0, 0 )
	SOFTWARE( xmen2s, xmen2, 199?, "Sega License", "X-Men 2 - Clone Wars (Prototype, 19941018)", 0, 0 )

	SOFTWARE( xpert, 0, 199?, "Sega License", "X-Perts (USA)", 0, 0 )
	SOFTWARE( xpertp, xpert, 199?, "Sega License", "X-perts (Prototype)", 0, 0 )

	SOFTWARE( xdrxda, 0, 199?, "Sega License", "XDR - X-Dazedly-Ray (Jpn)", 0, 0 )

	SOFTWARE( xenon2m, 0, 199?, "Sega License", "Xenon 2 Megablast (Euro)", 0, 0 )

	SOFTWARE( xiaomo, 0, 199?, "<unlicensed>", "Xiao Monv - Magic Girl (China)", 0, 0 )

	SOFTWARE( xinqig, 0, 199?, "<unlicensed>", "Xin Qi Gai Wang Zi (China, Alt)", 0, 0 )
	SOFTWARE( xinqi1, xinqig, 199?, "<unlicensed>", "Xin Qi Gai Wang Zi (China)", 0, 0 )

	SOFTWARE( yasech, 0, 199?, "<unlicensed>", "Ya Se Chuan Shuo (China)", 0, 0 )

	SOFTWARE( yangji, 0, 199?, "<unlicensed>", "Yang Jia Jiang - Yang Warrior Family (China)", 0, 0 )

	SOFTWARE( yogibear, 0, 199?, "Sega License", "Yogi Bear's Cartoon Capers (Euro)", 0, 0 )

	SOFTWARE( yindy, 0, 199?, "Sega License", "The Young Indiana Jones Chronicles (Prototype) (USA)", 0, 0 )
	SOFTWARE( yindya, yindy, 199?, "Sega License", "Young Indiana Jones - Instrument of Chaos (Prototype, 19941229)", 0, 0 )
	SOFTWARE( yindyb, yindy, 199?, "Sega License", "Young Indiana Jones - Instrument of Chaos (Prototype, 19941228-A)", 0, 0 )
	SOFTWARE( yindyc, yindy, 199?, "Sega License", "Young Indiana Jones - Instrument of Chaos (Prototype, 19940101)", 0, 0 )
	SOFTWARE( yindyd, yindy, 199?, "Sega License", "Young Indiana Jones - Instrument of Chaos (Prototype, 19940103)", 0, 0 )
	SOFTWARE( yindye, yindy, 199?, "Sega License", "Young Indiana Jones - Instrument of Chaos (Prototype, 19940126)", 0, 0 )
	SOFTWARE( yindyf, yindy, 199?, "Sega License", "Young Indiana Jones - Instrument of Chaos (Prototype, 19940127)", 0, 0 )
	SOFTWARE( yindyg, yindy, 199?, "Sega License", "Young Indiana Jones - Instrument of Chaos (Prototype, 19940923-A)", 0, 0 )
	SOFTWARE( yindyh, yindy, 199?, "Sega License", "Young Indiana Jones - Instrument of Chaos (Prototype, 19931228)", 0, 0 )

	SOFTWARE( ysiiiu, 0, 199?, "Sega License", "Ys III (USA)", 0, 0 )
	SOFTWARE( yswand, ysiiiu, 199?, "Sega License", "Ys - Wanderers from Ys (Jpn)", 0, 0 )

	SOFTWARE( yuusf, 0, 199?, "Sega License", "Yuu Yuu Hakusho - Makyou Toitsusen (Jpn)", 0, 0 )
	SOFTWARE( yuusfb, yuusf, 199?, "Sega License", "Yuu Yuu Hakusho - Sunset Fighters (Brazil)", 0, 0 )

	SOFTWARE( yuugai, 0, 199?, "Sega License", "Yuu Yuu Hakusho Gaiden (Jpn)", 0, 0 )

	SOFTWARE( zanyasha, 0, 199?, "Sega License", "Zan Yasha Enbukyoku (Jpn)", 0, 0 )

	SOFTWARE( zanygolf, 0, 199?, "Sega License", "Zany Golf (Euro, USA, v1.1)", 0, 0 )
	SOFTWARE( zanygolf1, zanygolf, 199?, "Sega License", "Zany Golf (Euro, USA)", 0, 0 )

	SOFTWARE( zerokami, 0, 199?, "Sega License", "Zero the Kamikaze Squirrel (Euro)", 0, 0 )
	SOFTWARE( zerokamiu, zerokami, 199?, "Sega License", "Zero the Kamikaze Squirrel (USA)", 0, 0 )

	SOFTWARE( zerotol, 0, 199?, "Sega License", "Zero Tolerance (Euro, USA)", 0, 0 )

	SOFTWARE( zerowing, 0, 199?, "Sega License", "Zero Wing (Euro)", 0, 0 )
	SOFTWARE( zerowingj, zerowing, 199?, "Sega License", "Zero Wing (Jpn)", 0, 0 )

	SOFTWARE( zhuogu, 0, 199?, "<unlicensed>", "Zhuo Gui Da Shi - Ghost Hunter (China)", 0, 0 )

	SOFTWARE( zhs, 0, 199?, "Sega License", "Zombie High (Prototype) (USA)", 0, 0 )

	SOFTWARE( zombies, 0, 199?, "Sega License", "Zombies (Euro)", 0, 0 )
	SOFTWARE( zombiesu, zombies, 199?, "Sega License", "Zombies Ate My Neighbors (USA)", 0, 0 )

	SOFTWARE( zool, 0, 199?, "Sega License", "Zool - Ninja of the 'Nth' Dimension (Euro)", 0, 0 )
	SOFTWARE( zoolu, zool, 199?, "Sega License", "Zool - Ninja of the 'Nth' Dimension (USA)", 0, 0 )

	SOFTWARE( zoom, 0, 199?, "Sega License", "Zoom! (World)", 0, 0 )

	SOFTWARE( zoop, 0, 199?, "Sega License", "Zoop (Euro)", 0, 0 )
	SOFTWARE( zoopu, zoop, 199?, "Sega License", "Zoop (USA)", 0, 0 )

	SOFTWARE( zouzou, 0, 199?, "Sega License", "Zou! Zou! Zou! Rescue Daisakusen (Jpn)", 0, 0 )

	/* Programs */
	SOFTWARE( fluxeu, 0, 199?, "Sega License", "Flux (Euro)", 0, 0 )	// To be used in conjuction with a MegaCD
	SOFTWARE( gameto, 0, 199?, "Sega License", "Game Toshokan (Jpn, Rev. A)", 0, 0 )	/* Id: G-4503 */
	SOFTWARE( megaan, 0, 199?, "Sega License", "Mega Anser (Jpn)", 0, 0 )	/* Id: G-4502 */
	SOFTWARE( meganet, 0, 199?, "Sega License", "MegaNet (Brazil)", 0, 0 )
	SOFTWARE( segachnl, 0, 199?, "Sega License", "Sega Channel (USA)", 0, 0 )


	/* to sort */
	SOFTWARE( truc96, 0, 1996, "Miky", "Truco '96 (Argentina)", 0, 0 )
	SOFTWARE( tc2000, 0, 199?, "<unlicensed>", "TC 2000 (Argentina)", 0, 0 ) // based on SMGP code
	SOFTWARE( unknown, 0, 199?, "Sega License", "Unknown Chinese Game 1 (Ch)", 0, 0 )
	/* radica */
	SOFTWARE( radicav1, 0, 2004, "Radica Games / Sega", "Radica: Volume 1 (Sonic the Hedgehog, Altered Beast, Golden Axe, Dr. Robotnik's Mean Bean Machine, Kid Chameleon, Flicky) (US)", 0, 0 )
	SOFTWARE( radicasf, 0, 2004, "Radica Games / Capcom", "Radica: Street Fighter Pack (Street Fighter 2' Special Champion Edition, Ghouls and Ghosts) (EURO)", 0, 0 )


	/* Cheat Programs */
	SOFTWARE( actrepl, 0, 199?, "Sega License", "Action Replay (Euro)", 0, 0 )
	SOFTWARE( gamege, 0, 199?, "Sega License", "Game Genie (Euro, USA)", 0, 0 )
	SOFTWARE( gamege1, gamege, 199?, "Sega License", "Game Genie (Euro, USA, Rev. A)", 0, 0 )
	SOFTWARE( proact, 0, 199?, "Sega License", "Pro Action Replay (Euro)", 0, 0 )
	SOFTWARE( proact2, 0, 199?, "Sega License", "Pro Action Replay 2 (Euro)", 0, 0 )
	SOFTWARE( proact2a, proact2, 199?, "Sega License", "Pro Action Replay 2 (Euro, Alt)", 0, 0 )
SOFTWARE_LIST_END


SOFTWARE_LIST_START( megasvp_cart )
	SOFTWARE( vr,  0,  199?, "Sega License", "Virtua Racing (Euro)", 0, 0 )
	SOFTWARE( vra, vr, 199?, "Sega License", "Virtua Racing (Euro, Alt)", 0, 0 )
	SOFTWARE( vrj, vr, 199?, "Sega License", "Virtua Racing (Jpn)", 0, 0 )
	SOFTWARE( vru, vr, 199?, "Sega License", "Virtua Racing (USA)", 0, 0 )
SOFTWARE_LIST_END


SOFTWARE_LIST( megadriv_cart, "Sega MegaDrive / Genesis cartridges" )
SOFTWARE_LIST( megasvp_cart, "Sega MegaDrive / Genesis (w/SVP) cartridges" )



#if 0
MEGADRIVE_ROM_LOAD( debugc, "debug - charles window bug example (pd).bin",                                               0x000000, 0x4000,    CRC(75582ddb) SHA1(bd913dcadba2588d4b9b1aa970038421cba0a1af) )
MEGADRIVE_ROM_LOAD( flavio, "debug flavio's dma test (pd).bin",                                                          0x000000, 0x4000,	  CRC(549cebf4) SHA1(c44f0037a6856bd52a212b26ea416501e92f7645) )
	SOFTWARE( debugc, 0,       199?, "Sega License", "DEBUG - Charles Window Bug Example (PD)", 0, 0 )
	SOFTWARE( flavio, 0,       199?, "Sega License", "DEBUG Flavio's DMA Test (PD)", 0, 0 )
#endif



// UNDUMPED / TO BE REDUMPED LIST
//
// 2IN1 (COSMIC SPACEHEAD FANTASTIC DIZZY)	PAL
// 2IN1 (PSYCHIC PINBALL MICRO MACHINES)	PAL
// AAAHH!!! REAL MONSTERS					PAL			dump check
// ADVANCED DAISENRYAKU						JAPAN		we have Rev 0, Rev A needed
// ARNOLD PALMER TOURNAMENT GOLF			PAL
// AWESOME POSSUM							JAPAN
// BATTLE SQUADRON							PAL
// BEAVIS & BUTTHEAD						PAL
// BUGS BUNNY IN DOUBLE TROUBLE				PAL
// BULLS VS LAKERS							PAL
// CAPTAIN LANG								JAPAN
// CAPTAIN PLANET							ASIA
// CENTURION								USA/PAL		needs proper dump
// DAVIS CUP TENNIS							JAPAN
// DRAGON'S REVENGE							JAPAN
// DYNAMITE HEADDY SAMPLE					JAPAN
// ECCO JR									USA/PAL		graphic corruption on later revision
// ESWAT									PAL			dump check
// EVANDER HOLYFIELD'S BOXING				PAL
// EX-MUTANTS								PAL
// F-22 (JUNE 1992)							USA/PAL		needs proper dump
// FAERY TALE ADVENTURE						PAL
// FATAL FURY 2								PAL			Australia-only PAL release
// FERRARI GRAND PRIX CHALLENGE				PAL			we have Rev A, Rev 0 needed
// FIFA SOCCER								JAPAN
// FIRE SHARK								PAL			dump check
// GAIN GROUND								PAL			revision order check
// GALAXY FORCE II							ALL			we have Rev B, Rev 0 and Rev A needed
// GO NET									JAPAN
// HOOK										PAL
// INTERNATIONAL SUPERSTAR SOCCER			BRAZIL
// JEWEL MASTER								USA/PAL		we have Rev A, Rev 0 needed (does it match Japanese release?)
// KING OF THE MONSTERS						JAPAN
// MARBLE MADNESS							JAPAN
// MIDNIGHT RESISTANCE						PAL
// MOTHERBASE								PAL			32X cart
// MUTANT LEAGUE FOOTBALL					PAL
// NBA PLAYOFFS - BULLS VS BLAZERS			JAPAN		conflict in current info
// NFL PRO FOOTBALL '94						JAPAN
// OSAKA BANK MY LINE						JAPAN
// OSAKA BANK MY LINE DEMO					JAPAN
// PAC-PANIC								PAL
// PAPERBOY 2								PAL
// PGA TOUR GOLF							USA/PAL		we have v1.1 and v1.2, v1.0 needed (does it exist?)
// PHANTASY STAR IV							PAL
// PINK GOES TO HOLLYWOOD					PAL
// ROAD RASH								JAPAN
// ROAD RASH								USA/PAL		needs proper dump
// ROBOCOP VERSUS THE TERMINATOR			JAPAN
// SAN SAN									JAPAN
// SHINING FORCE							PAL
// SIDE POCKET								JAPAN/USA	conflict in current info
// SMURFS									PAL			we have Rev A, Rev 0 needed
// SPIDERMAN AND X-MEN ARCADE'S REVENGE		PAL
// SUMISEI HOME TANMATSU					JAPAN
// SUPER DAISENRYAKU						JAPAN		we have Rev 0, Rev A needed (confirmed by Ototo)
// TOM & JERRY								JAPAN
// VALIS III								JAPAN		we have v1.1 (Rev A?), v1.0 (Rev 0?) needed
// WONDER MIDI								JAPAN
// WORLD CHAMPIONSHIP SOCCER II				PAL
// WORLD CUP SOCCER							JAPAN/USA	we have v1.0 (Rev ?), v1.2 (Rev ?) and v1.3 (Rev B), v1.1 (Rev ?) needed
// WORLD OF ILLUSION						JAPAN		we have v1.0 (Rev 0?), v1.1 (Rev A?) needed (confirmed by Xacrow)
// ZANY GOLF								PAL
// ZOOM!									PAL
