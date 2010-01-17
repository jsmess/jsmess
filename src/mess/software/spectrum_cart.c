/***************************************************************************

  Sinclair ZX Spectrum cartridges

***************************************************************************/

#include "driver.h"
#include "softlist.h"
#include "devices/cartslot.h"

#define PCB_ZXC1        0x01                              // Paul Farrow's ZXC1 cartridge
#define PCB_ZXC2        0x02                              // Paul Farrow's ZXC2 cartridge
#define PCB_ZXC3        0x04                              // Paul Farrow's ZXC3 cartridge
#define PCB_ZXC23       (PCB_ZXC2 | PCB_ZXC3)             // Some cartridges are supported
#define PCB_ZXCALL      (PCB_ZXC1 | PCB_ZXC2 | PCB_ZXC3)  // by multiple designs
#define PCB_DROY        0x08                              // Droy's 32 Kbytes cartridge
#define PCB_ZXFLASH     0x10                              // Droy's ZX-Flash 512 Kbytes cartridge
#define PCB_HUEHN       0x20                              // Scott-Falk H??hn's 512 Kbytes cartridge
#define PCB_SCARTIF2    0x40                              // Jos?? Leandro Novell??n Mart??nez' Super Cartucho IF2 512 Kbytes cartridge

#define ZX48_ONLY       0x80000000                        // Cartridge cannot run on an unexpanded ZX Spectrum

/* Standard cartridges support up to 16 Kbytes of data */
#define ZXSPECTRUM_ROM_LOAD( set, name, offset, length, hash )  \
SOFTWARE_START( set ) \
    ROM_REGION( 0x4000, CARTRIDGE_REGION_ROM, ROMREGION_ERASEFF ) \
    ROM_LOAD(name, offset, length, hash) \
SOFTWARE_END

/* Droy's cartridges support up to 32 Kbytes of data */
#define ZXSPECTRUM_ROM32K_LOAD( set, name, offset, length, hash )   \
SOFTWARE_START( set ) \
    ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, ROMREGION_ERASEFF ) \
    ROM_LOAD(name, offset, length, hash) \
SOFTWARE_END

/* ZXC1 cartridges support up to 64 Kbytes of data */
#define ZXSPECTRUM_ROM64K_LOAD( set, name, offset, length, hash )   \
SOFTWARE_START( set ) \
    ROM_REGION( 0x10000, CARTRIDGE_REGION_ROM, ROMREGION_ERASEFF ) \
    ROM_LOAD(name, offset, length, hash) \
SOFTWARE_END

/* ZXC2/ZXC3 cartridges support up to 256 Kbytes of data */
#define ZXSPECTRUM_ROM256K_LOAD( set, name, offset, length, hash )  \
SOFTWARE_START( set ) \
    ROM_REGION( 0x40000, CARTRIDGE_REGION_ROM, ROMREGION_ERASEFF ) \
    ROM_LOAD(name, offset, length, hash) \
SOFTWARE_END

/* ZX-FLASH, H??hn's and Super Cartucho IF2 cartridges support up to 512 Kbytes of data */
#define ZXSPECTRUM_ROM512K_LOAD( set, name, offset, length, hash )  \
SOFTWARE_START( set ) \
    ROM_REGION( 0x80000, CARTRIDGE_REGION_ROM, ROMREGION_ERASEFF ) \
    ROM_LOAD(name, offset, length, hash) \
SOFTWARE_END

ZXSPECTRUM_ROM_LOAD( zxtest,   "zxtest.rom",   0x0000, 0x2000, CRC(730bba9e) SHA1(9e162a027e64ab57ccbedb7da86c2afc7ee44749) )
ZXSPECTRUM_ROM_LOAD( spcraid,  "spcraid.rom",  0x0000, 0x4000, CRC(a570bd3d) SHA1(f8359abdf50360261099460e48d440738109af1d) )
ZXSPECTRUM_ROM_LOAD( chess,    "chess.rom",    0x0000, 0x4000, CRC(08c6a8c6) SHA1(bba6d31ff54d3009cc61ae767973aab492bb104c) )
ZXSPECTRUM_ROM_LOAD( plntoids, "plntoids.rom", 0x0000, 0x4000, CRC(06e43fa2) SHA1(da3041aa2f14abda0d2966e44fd2fd144bb3aa72) )
ZXSPECTRUM_ROM_LOAD( hunghora, "hunghora.rom", 0x0000, 0x4000, CRC(bef35699) SHA1(00129d9a5094a91a6e7392245be5121d5d17adf0) )
ZXSPECTRUM_ROM_LOAD( backgamm, "backgamm.rom", 0x0000, 0x4000, CRC(40e78b48) SHA1(e6ede60bb5e08d7ead8343f8078834d2afb49c30) )
ZXSPECTRUM_ROM_LOAD( horaspid, "horaspid.rom", 0x0000, 0x4000, CRC(8ce28ffe) SHA1(805fcdf83c74937adffab97027310d8c8e431c5a) )
ZXSPECTRUM_ROM_LOAD( jetpac,   "jetpac.rom",   0x0000, 0x4000, CRC(1d194e5b) SHA1(0f0ddb79bb6b052846e0419df231df44184fbca6) )
ZXSPECTRUM_ROM_LOAD( pssst,    "pssst.rom",    0x0000, 0x4000, CRC(464cd4f6) SHA1(cebafa93c110eabb4b343e7a34175d77d0fd0797) )
ZXSPECTRUM_ROM_LOAD( tranzam,  "tranzam.rom",  0x0000, 0x4000, CRC(03da9be7) SHA1(caf4441730a7032375d82edf385d5ac7f65c1dcd) )
ZXSPECTRUM_ROM_LOAD( cookie,   "cookie.rom",   0x0000, 0x4000, CRC(7108e9a8) SHA1(fa5753133daa2a6b122948adf604c12ab70e2ce9) )
ZXSPECTRUM_ROM_LOAD( gyruss,   "gyruss.rom",   0x0000, 0x4000, CRC(0dd3bceb) SHA1(9745d1ee713b2c6d319752aee523105905cbf562) )
ZXSPECTRUM_ROM_LOAD( locomotn, "locomotn.rom", 0x0000, 0x4000, CRC(33b8ae50) SHA1(230294b652250b975a84b8618a68a8e60729cc2b) )
ZXSPECTRUM_ROM_LOAD( montezum, "montezum.rom", 0x0000, 0x4000, CRC(ff8d7dc3) SHA1(b700e37f7e24adea6ba12cf0e935307bd006b9c9) )
ZXSPECTRUM_ROM_LOAD( montezua, "montezua.rom", 0x0000, 0x4000, CRC(5d73082e) SHA1(e8f1b6e6b8c8eb6de6a0c6d271351ca7056a39af) )
ZXSPECTRUM_ROM_LOAD( popeye,   "popeye.rom",   0x0000, 0x4000, CRC(85c189c1) SHA1(5c185c7e3eed2c3379bf74bf522f95c32a730278) )
ZXSPECTRUM_ROM_LOAD( qbert,    "qbert.rom",    0x0000, 0x4000, CRC(5de868db) SHA1(777872a80411fd9c9653cc9c6dea1506b833d6b2) )
ZXSPECTRUM_ROM_LOAD( roj,      "roj.rom",      0x0000, 0x4000, CRC(5f5bf622) SHA1(71e55eecb5f38338af8826ae743509d35250f20e) )
ZXSPECTRUM_ROM_LOAD( starwars, "starwars.rom", 0x0000, 0x4000, CRC(732c6f5d) SHA1(0ade16899c2e815ddfea6782618b0cdf760e6d54) )
ZXSPECTRUM_ROM_LOAD( starwara, "starwara.rom", 0x0000, 0x4000, CRC(90b61858) SHA1(29cbda204ea20d42429a723afe2d00b9edd0a04d) )
ZXSPECTRUM_ROM_LOAD( dethstar, "dethstar.rom", 0x0000, 0x4000, CRC(9f74021f) SHA1(852757985b73bca7fee06dcaaa4be5a89c11fbc0) )

ZXSPECTRUM_ROM32K_LOAD( knlrdroy, "knlrdroy.rom", 0x0000, 0x8000, CRC(7bcd642c) SHA1(0c1aeeabd77b179b343f86e1ccd89f340bfbb1f3) )

ZXSPECTRUM_ROM512K_LOAD( if2huehn, "if2huehn.rom", 0x0000, 0x40000, CRC(f49ad814) SHA1(7296a140c02231e8e7562e711d8d1523e6a20a9e) )

ZXSPECTRUM_ROM512K_LOAD( scartif2, "cartucho.rom", 0x0000, 0x40000, CRC(ec348b91) SHA1(843f533ae95a8703ea4a764c60ae148348cde059) )

ZXSPECTRUM_ROM_LOAD( copier,  "Copier.rom",     0x0000, 0x2000, CRC(d8c5429a) SHA1(f44abcc7d582b85de3938956067c9461c1d9d296) )
ZXSPECTRUM_ROM_LOAD( ramtest, "RAM_Tester.rom", 0x0000, 0x2000, CRC(5d2b7ad6) SHA1(3c762943e472f1fd8c40d8b34cbebcf9c9c5da72) )
ZXSPECTRUM_ROM_LOAD( romtest, "ROM_Tester.rom", 0x0000, 0x2000, CRC(d8511d55) SHA1(8f3549f8cc57da2962de06e5344e5accef8e84b8) )

ZXSPECTRUM_ROM256K_LOAD( 128,        "128.rom",           0x0000, 0x8000,  CRC(ab78b08a) SHA1(5096bd50a2f5ff0f1ec618e80ac0dde4f7adf25c) )
ZXSPECTRUM_ROM256K_LOAD( 128p,       "128+.rom",          0x0000, 0x8000,  CRC(023e670a) SHA1(a897b33fe5286cf028db73920fa3575d45c616ea) )
ZXSPECTRUM_ROM256K_LOAD( 128_i1e1,   "128_IF1_ED1.rom",   0x0000, 0x10000, CRC(5c7e989d) SHA1(eab62941d6215580b650b5b6c3d555149af0d8ce) )
ZXSPECTRUM_ROM256K_LOAD( 128p_i1e1,  "128+_IF1_ED1.rom",  0x0000, 0x10000, CRC(230daa69) SHA1(ddb4d7e676dadddb3226ba343761e72d364d2703) )
ZXSPECTRUM_ROM256K_LOAD( 128_i1e2,   "128_IF1_ED2.rom",   0x0000, 0x10000, CRC(2f7753c0) SHA1(ae31859fec059adaf9a78b48e1001eb4dcc9200c) )
ZXSPECTRUM_ROM256K_LOAD( 128p_i1e2,  "128+_IF1_ED2.rom",  0x0000, 0x10000, CRC(50046134) SHA1(4d0018503c68156748f3b8f683da851edbbd7083) )
ZXSPECTRUM_ROM256K_LOAD( 128m_i1e1,  "128M_IF1_ED1.rom",  0x0000, 0x10000, CRC(d829d56b) SHA1(53256fc0a05bde81f1947d9cf0bac3a6bcc11a2e) )
ZXSPECTRUM_ROM256K_LOAD( 128mp_i1e1, "128M+_IF1_ED1.rom", 0x0000, 0x10000, CRC(a75ae79f) SHA1(95b784ccaa05d9b682425f56b491bd862f1aa930) )
ZXSPECTRUM_ROM256K_LOAD( 128m_i1e2,  "128M_IF1_ED2.rom",  0x0000, 0x10000, CRC(ab201e36) SHA1(f4afcb0c9281ef9dc5c675a67a6ca239d4eddf52) )
ZXSPECTRUM_ROM256K_LOAD( 128mp_i1e2, "128M+_IF1_ED2.rom", 0x0000, 0x10000, CRC(d4532cc2) SHA1(a52ca6dab0bfc997d273f9db1046dcc59e6c703a) )

ZXSPECTRUM_ROM256K_LOAD( 128sp,        "SP128.rom",           0x0000, 0x8000,  CRC(8e0f002d) SHA1(0440f87fdf9addf5bdb8e6964b4f7dcc296c0f95) )
ZXSPECTRUM_ROM256K_LOAD( 128spp,       "SP128+.rom",          0x0000, 0x8000,  CRC(d72a3fb6) SHA1(2e8fd35adac92e1fc48e605c452cddb160d4ceba) )
ZXSPECTRUM_ROM256K_LOAD( 128sp_i1e1,   "SP128_IF1_ED1.rom",   0x0000, 0x10000, CRC(795333f5) SHA1(5924541f77870ca2ddf84de6b57957f157e8d5c3) )
ZXSPECTRUM_ROM256K_LOAD( 128spp_i1e1,  "SP128+_IF1_ED1.rom",  0x0000, 0x10000, CRC(7256796a) SHA1(1465e55d5794ca45438ceb1aede4f602e6bc8e60) )
ZXSPECTRUM_ROM256K_LOAD( 128sp_i1e2,   "SP128_IF1_ED2.rom",   0x0000, 0x10000, CRC(0a5af8a8) SHA1(65ac3c502fe4cbdd07af4aa4bf3142beff58eb0a) )
ZXSPECTRUM_ROM256K_LOAD( 128spp_i1e2,  "SP128+_IF1_ED2.rom",  0x0000, 0x10000, CRC(015fb237) SHA1(aabbee34f523327bb5057d42b4ff4a6129edefbd) )
ZXSPECTRUM_ROM256K_LOAD( 128spm_i1e1,  "SP128M_IF1_ED1.rom",  0x0000, 0x10000, CRC(fd047e03) SHA1(b0d1cb1e506c7fb7af5ecaecaa0cb0d6620a7327) )
ZXSPECTRUM_ROM256K_LOAD( 128spmp_i1e1, "SP128M+_IF1_ED1.rom", 0x0000, 0x10000, CRC(f601349c) SHA1(a0a2db9bdb454f7fcb07a0de26f7529e06d026c4) )
ZXSPECTRUM_ROM256K_LOAD( 128spm_i1e2,  "SP128M_IF1_ED2.rom",  0x0000, 0x10000, CRC(8e0db55e) SHA1(6ec5d0fbb6d743fa6fda9820ca41e939e246ef8e) )
ZXSPECTRUM_ROM256K_LOAD( 128spmp_i1e2, "SP128M+_IF1_ED2.rom", 0x0000, 0x10000, CRC(8508ffc1) SHA1(b622238fd34866b489a41536466f961df4d771c6) )

ZXSPECTRUM_ROM256K_LOAD( 48_i1e1,    "48_IF1_ED1.rom",    0x0000, 0x8000,  CRC(d7e990ac) SHA1(e67e3def822a5b32775fcca02e46a0726c2d41f8) )
ZXSPECTRUM_ROM256K_LOAD( 48_i1e2,    "48_IF1_ED2.rom",    0x0000, 0x8000,  CRC(ea535630) SHA1(9b6257b0f33590bd554fb41297e555f618f7fa0b) )

ZXSPECTRUM_ROM256K_LOAD( ace,        "Ace.rom",           0x0000, 0x4000,  CRC(c5fa5365) SHA1(8efa1aab8a26d36893cd1b51fa3f82ad7a019324) )

SOFTWARE_LIST_START( spectrum_cart )

/* This cartridge was only available to authorised service centres. */
    SOFTWARE( zxtest,   0,        1983, "Sinclair Research Ltd",    "ZX Spectrum Test Cartridge",   0,  0 )

/* The following 10 cartridges were licensed by Sinclair Research Ltd and officially listed on their catalogue */
    SOFTWARE( spcraid,  0,        1982, "Psion",                  "Space Raiders",        0, 0 ) /* G9/R     5300 */
    SOFTWARE( chess,    0,        1983, "Psion",                  "Chess",                0, 0 ) /* G10/R    5301 */
    SOFTWARE( plntoids, 0,        1982, "Psion",                  "Planetoids",           0, 0 ) /* G12/R    5302 */
    SOFTWARE( hunghora, 0,        1982, "Psion",                  "Hungry Horace",        0, 0 ) /* G13/R    5303 */
    SOFTWARE( backgamm, 0,        1983, "Psion",                  "Backgammon",           0, 0 ) /* G22/R    5304 */
    SOFTWARE( horaspid, 0,        1983, "Psion",                  "Horace & the Spiders", 0, 0 ) /* G24/R    5305 */
    SOFTWARE( jetpac,   0,        1983, "Ultimate Play The Game", "Jet Pac",              0, 0 ) /* G27/R    5306 */
    SOFTWARE( pssst,    0,        1983, "Ultimate Play The Game", "Pssst",                0, 0 ) /* G28/R    5307 */
    SOFTWARE( tranzam,  0,        1983, "Ultimate Play The Game", "Tranz Am",             0, 0 ) /* G29/R    5308 */
    SOFTWARE( cookie,   0,        1983, "Ultimate Play The Game", "Cookie",               0, 0 ) /* G30/R    5309 */

/* The following cartridges were created for Parker Brothers to be released officially but were scrapped short before launch */
    SOFTWARE( gyruss,   0,        1984, "Parker Brothers",        "Gyruss (Prototype)",                                ZX48_ONLY, 0 )
    SOFTWARE( locomotn, 0,        1984, "Parker Brothers",        "Loco Motion (Prototype)",                           ZX48_ONLY, 0 )
    SOFTWARE( montezum, 0,        1984, "Parker Brothers",        "Montezuma's Revenge! (Prototype)",                  ZX48_ONLY, 0 )
    SOFTWARE( montezua, montezum, 1984, "Parker Brothers",        "Montezuma's Revenge! (Prototype, Alternate)",       ZX48_ONLY, 0 )
    SOFTWARE( popeye,   0,        1984, "Parker Brothers",        "Popeye (Prototype)",                                ZX48_ONLY, 0 )
    SOFTWARE( qbert,    0,        1984, "Parker Brothers",        "Q*Bert (Prototype)",                                ZX48_ONLY, 0 )
    SOFTWARE( roj,      0,        1984, "Parker Brothers",        "Return of the Jedi: Death Star Battle (Prototype)", ZX48_ONLY, 0 )
    SOFTWARE( starwars, 0,        1984, "Parker Brothers",        "Star Wars (Prototype)",                             ZX48_ONLY, 0 )
    SOFTWARE( starwara, starwars, 1984, "Parker Brothers",        "Star Wars (Prototype, Alternate)",                  ZX48_ONLY, 0 )

/* Homebrews */
    SOFTWARE( dethstar, 0, 1998, "<Homebrew>", "Death Star", ZX48_ONLY,  0 )

/*
   The following conversion demonstrates the bank switching capabilities of Droy's PCBs.
   The 32 Kbytes PCB description and usage are described at http://www.speccy.org/trastero/cosas/droy/cartuchos/cartuchos_s.htm
   and the 512Kb version is described at http://www.speccy.org/trastero/cosas/droy/zxflash/zxflashcart_e.htm
*/
    SOFTWARE( knlrdroy, 0, 2002, "<Homebrew>", "Knight Lore (Droy 32Kb PCB)", PCB_DROY, 0 )

/*
   The following conversion demonstrates the bank switching capabilities of Scott-Falk H??hn's PCB.
   The 512 Kbytes PCB description and usage are described at http://s-huehn.de/spectrum/hardware2.htm#if2rom
*/
    SOFTWARE( if2huehn, 0, 2007, "<Homebrew>", "512Kb Multicart Demo (H??hn PCB)", PCB_HUEHN, 0 ) // This demo cartridge uses just 16 of the 32 available banks

/*
   The following conversion demonstrates the bank switching capabilities of Jos?? Leandro Novell??n Mart??nez' PCB.
   The 512 Kbytes PCB description and usage are described at http://www.speccy.org/trastero/cosas/JL/if2/IF2-1.html
*/
    SOFTWARE( scartif2, 0, 2004, "<Homebrew>", "512Kb Multicart Demo (Super Cartucho IF2 PCB)", PCB_SCARTIF2, 0 ) // This demo cartridge uses just 16 of the 32 available banks

/*
   The following cartridges demonstrate the bank switching capabilities of Paul Farrow's ZXCx PCBs.
   PCBs descriptions and usage are described at http://www.fruitcake.plus.com/Sinclair/Interface2/Interface2_RC_Custom.htm
*/
    SOFTWARE( copier,       0,       1988, "<Homebrew>", "Microdrive/Cassette Copier v1.04",                                                   PCB_ZXC23,            0)
    SOFTWARE( ramtest,      0,       2008, "<Homebrew>", "ZX Spectrum 48/128/+2 RAM Tester v2.05",                                             PCB_ZXCALL,           0)
    SOFTWARE( romtest,      0,       2009, "<Homebrew>", "ZX Spectrum 48/128/+2 ROM Tester v1.03",                                             PCB_ZXC23,            0)

    SOFTWARE( 128,          0,       2008, "<Homebrew>", "ZX Spectrum 128 Emulator v1.11A",                                                    ZX48_ONLY|PCB_ZXC23,  0)
    SOFTWARE( 128p,         128,     2008, "<Homebrew>", "Bug Free ZX Spectrum 128 Emulator v1.11a",                                           ZX48_ONLY|PCB_ZXC23,  0)

    SOFTWARE( 128_i1e1,     128,     2008, "<Homebrew>", "ZX Spectrum 128 + ZX Interface 1 (Ed. 1) Emulator v1.11B",                           ZX48_ONLY|PCB_ZXC23,  0)
    SOFTWARE( 128p_i1e1,    128,     2008, "<Homebrew>", "Bug Free ZX Spectrum 128 + ZX Interface 1 (Ed. 1) Emulator v1.11b",                  ZX48_ONLY|PCB_ZXC23,  0)

    SOFTWARE( 128_i1e2,     128,     2008, "<Homebrew>", "ZX Spectrum 128 + ZX Interface 1 (Ed. 2) Emulator v1.11C",                           ZX48_ONLY|PCB_ZXC23,  0)
    SOFTWARE( 128p_i1e2,    128,     2008, "<Homebrew>", "Bug Free ZX Spectrum 128 + ZX Interface 1 (Ed. 2) Emulator v1.11c",                  ZX48_ONLY|PCB_ZXC23,  0)

    SOFTWARE( 128m_i1e1,    128,     2008, "<Homebrew>", "Modified ZX Spectrum 128 + ZX Interface 1 (Ed. 1) Emulator v1.11N",                  ZX48_ONLY|PCB_ZXC23,  0)
    SOFTWARE( 128mp_i1e1,   128,     2008, "<Homebrew>", "Modified Bug Free ZX Spectrum 128 + ZX Interface 1 (Ed. 1) Emulator v1.11n",         ZX48_ONLY|PCB_ZXC23,  0)

    SOFTWARE( 128m_i1e2,    128,     2008, "<Homebrew>", "Modified ZX Spectrum 128 + ZX Interface 1 (Ed. 2) Emulator v1.11O",                  ZX48_ONLY|PCB_ZXC23,  0)
    SOFTWARE( 128mp_i1e2,   128,     2008, "<Homebrew>", "Modified Bug Free ZX Spectrum 128 + ZX Interface 1 (Ed. 2) Emulator v1.11o",         ZX48_ONLY|PCB_ZXC23,  0)

    SOFTWARE( 128sp,        0,       2008, "<Homebrew>", "ZX Spectrum 128 (Spain) Emulator v1.04A",                                            ZX48_ONLY|PCB_ZXC23,  0)
    SOFTWARE( 128spp,       128sp,   2008, "<Homebrew>", "Bug Free ZX Spectrum 128 (Spain) Emulator v1.04a",                                   ZX48_ONLY|PCB_ZXC23,  0)

    SOFTWARE( 128sp_i1e1,   128sp,   2008, "<Homebrew>", "ZX Spectrum 128 (Spain) + ZX Interface 1 (Ed. 1) Emulator v1.04B",                   ZX48_ONLY|PCB_ZXC23,  0)
    SOFTWARE( 128spp_i1e1,  128sp,   2008, "<Homebrew>", "Bug Free ZX Spectrum 128 (Spain) + ZX Interface 1 (Ed. 1) Emulator v1.04b",          ZX48_ONLY|PCB_ZXC23,  0)

    SOFTWARE( 128sp_i1e2,   128sp,   2008, "<Homebrew>", "ZX Spectrum 128 (Spain) + ZX Interface 1 (Ed. 2) Emulator v1.04C",                   ZX48_ONLY|PCB_ZXC23,  0)
    SOFTWARE( 128spp_i1e2,  128sp,   2008, "<Homebrew>", "Bug Free ZX Spectrum 128 (Spain) + ZX Interface 1 (Ed. 2) Emulator v1.04c",          ZX48_ONLY|PCB_ZXC23,  0)

    SOFTWARE( 128spm_i1e1,  128sp,   2008, "<Homebrew>", "Modified ZX Spectrum 128 (Spain) + ZX Interface 1 (Ed. 1) Emulator v1.04N",          ZX48_ONLY|PCB_ZXC23,  0)
    SOFTWARE( 128spmp_i1e1, 128sp,   2008, "<Homebrew>", "Modified Bug Free ZX Spectrum 128 (Spain) + ZX Interface 1 (Ed. 1) Emulator v1.04n", ZX48_ONLY|PCB_ZXC23,  0)

    SOFTWARE( 128spm_i1e2,  128sp,   2008, "<Homebrew>", "Modified ZX Spectrum 128 (Spain) + ZX Interface 1 (Ed. 2) Emulator v1.04O",          ZX48_ONLY|PCB_ZXC23,  0)
    SOFTWARE( 128spmp_i1e2, 128sp,   2008, "<Homebrew>", "Modified Bug Free ZX Spectrum 128 (Spain) + ZX Interface 1 (Ed. 2) Emulator v1.04o", ZX48_ONLY|PCB_ZXC23,  0)

    SOFTWARE( 48_i1e1,      0,       2008, "<Homebrew>", "ZX Spectrum + ZX Interface 1 (Ed. 1) v1.01",                                         PCB_ZXC23,            0)
    SOFTWARE( 48_i1e2,      48_i1e1, 2008, "<Homebrew>", "ZX Spectrum + ZX Interface 1 (Ed. 2) v1.01",                                         PCB_ZXC23,            0)

    SOFTWARE( ace,          0,       2005, "<Homebrew>", "Jupiter ACE Emulator v1.07",                                                         ZX48_ONLY|PCB_ZXCALL, 0)

SOFTWARE_LIST_END


SOFTWARE_LIST( spectrum_cart, "Sinclair ZX Spectrum cartridges" )

