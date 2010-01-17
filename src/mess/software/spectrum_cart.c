/***************************************************************************

  Sinclair ZX Spectrum cartridges

***************************************************************************/

#include "driver.h"
#include "softlist.h"
#include "devices/cartslot.h"

#define PCB_ZXC1		0x01   // Paul Farrow's ZXC1 cartridge
#define PCB_ZXC2		0x02   // Paul Farrow's ZXC2 cartridge
#define PCB_ZXC3		0x04   // Paul Farrow's ZXC3 cartridge
#define PCB_DROY		0x08   // Droy's 32 Kbytes cartridge
#define PCB_ZXFLASH		0x10   // Droy's ZX-Flash 512 Kbytes cartridge
#define PCB_HUEHN		0x20   // Scott-Falk H??hn's 512 Kbytes cartridge
#define PCB_SCARTIF2	0x40   // Jos?? Leandro Novell??n Mart??nez' Super Cartucho IF2 512 Kbytes cartridge

#define ZX48_ONLY		0x80000000   // 16 Kbytes configuration not allowed

#define ZXSPECTRUM_ROM_LOAD( set, name, offset, length, hash )	\
SOFTWARE_START( set ) \
	ROM_REGION( 0x4000, CARTRIDGE_REGION_ROM, ROMREGION_ERASEFF ) \
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


SOFTWARE_START( knlrdroy )
	ROM_REGION( 0x8000, CARTRIDGE_REGION_ROM, ROMREGION_ERASEFF )
	ROM_LOAD( "knlrdroy.rom", 0x0000, 0x8000, CRC(7bcd642c) SHA1(0c1aeeabd77b179b343f86e1ccd89f340bfbb1f3) )
SOFTWARE_END


SOFTWARE_START( if2huehn )
	ROM_REGION( 0x80000, CARTRIDGE_REGION_ROM, ROMREGION_ERASEFF )
	ROM_LOAD( "if2huehn.rom", 0x0000, 0x40000, CRC(f49ad814) SHA1(7296a140c02231e8e7562e711d8d1523e6a20a9e) )
SOFTWARE_END


SOFTWARE_START( scartif2 )
	ROM_REGION( 0x80000, CARTRIDGE_REGION_ROM, ROMREGION_ERASEFF )
	ROM_LOAD( "cartucho.rom", 0x0000, 0x40000, CRC(ec348b91) SHA1(843f533ae95a8703ea4a764c60ae148348cde059) )
SOFTWARE_END


SOFTWARE_LIST_START( spectrum_cart )

/* This cartridge was only available to authorised service centres. */
	SOFTWARE( zxtest,	0,	1983,	"Sinclair Research Ltd",	"ZX Spectrum Test Cartridge",	0,	0 )

/* The following 10 cartridges were licensed by Sinclair Research Ltd and officially listed on their catalogue */
	SOFTWARE( spcraid,	0,	1982,	"Psion",			"Space Raiders",	0,	0 ) /* G9/R     5300 */
	SOFTWARE( chess,	0,	1983,	"Psion",			"Chess",		0,	0 ) /* G10/R    5301 */
	SOFTWARE( plntoids,	0,	1982,	"Psion",			"Planetoids",		0,	0 ) /* G12/R    5302 */
	SOFTWARE( hunghora,	0,	1982,	"Psion",			"Hungry Horace",	0,	0 ) /* G13/R    5303 */
	SOFTWARE( backgamm,	0,	1983,	"Psion",			"Backgammon",		0,	0 ) /* G22/R    5304 */
	SOFTWARE( horaspid,	0,	1983,	"Psion",			"Horace & the Spiders",	0,	0 ) /* G24/R    5305 */
	SOFTWARE( jetpac,	0,	1983,	"Ultimate Play The Game",	"Jet Pac",		0,	0 ) /* G27/R    5306 */
	SOFTWARE( pssst,	0,	1983,	"Ultimate Play The Game",	"Pssst",		0,	0 ) /* G28/R    5307 */
	SOFTWARE( tranzam,	0,	1983,	"Ultimate Play The Game",	"Tranz Am",		0,	0 ) /* G29/R    5308 */
	SOFTWARE( cookie,	0,	1983,	"Ultimate Play The Game",	"Cookie",		0,	0 ) /* G30/R    5309 */

/* The following cartridges were created for Parker Brothers but scrapped short before release */
	SOFTWARE( gyruss,	0,		1984,	"Parker Brothers",	"Gyruss (Prototype)",					ZX48_ONLY,	0 )
	SOFTWARE( locomotn,	0,		1984,	"Parker Brothers",	"Loco Motion (Prototype)",				ZX48_ONLY,	0 )
	SOFTWARE( montezum,	0,		1984,	"Parker Brothers",	"Montezuma's Revenge! (Prototype)",			ZX48_ONLY,	0 )
	SOFTWARE( montezua,	montezum,	1984,	"Parker Brothers",	"Montezuma's Revenge! (Prototype, Alternate)",		ZX48_ONLY,	0 )
	SOFTWARE( popeye,	0,		1984,	"Parker Brothers",	"Popeye (Prototype)",					ZX48_ONLY,	0 )
	SOFTWARE( qbert,	0,		1984,	"Parker Brothers",	"Q*Bert (Prototype)",					ZX48_ONLY,	0 )
	SOFTWARE( roj,		0,		1984,	"Parker Brothers",	"Return of the Jedi: Death Star Battle (Prototype)",ZX48_ONLY,	0 )
	SOFTWARE( starwars,	0,		1984,	"Parker Brothers",	"Star Wars (Prototype)",				ZX48_ONLY,	0 )
	SOFTWARE( starwara,	starwars,	1984,	"Parker Brothers",	"Star Wars (Prototype, Alternate)",			ZX48_ONLY,	0 )

/* Homebrews */
	SOFTWARE( dethstar,	0,	1998,	"<Homebrew>",	"Death Star",	ZX48_ONLY,	0 )

/*
   The following conversion demonstrates the bank switching capabilities of Droy's PCBs.
   The 32 Kbytes PCB description and usage are described at http://www.speccy.org/trastero/cosas/droy/cartuchos/cartuchos_s.htm
   and the 512Kb version is described at http://www.speccy.org/trastero/cosas/droy/zxflash/zxflashcart_e.htm
*/
	SOFTWARE( knlrdroy,	0,	2002,	"<Homebrew>",	"Knight Lore (Droy 32Kb PCB)", PCB_DROY,	0 )

/*
   The following conversion demonstrates the bank switching capabilities of Scott-Falk H??hn's PCB.
   The 512 Kbytes PCB description and usage are described at http://s-huehn.de/spectrum/hardware2.htm#if2rom
*/
	SOFTWARE( if2huehn,	0,	2007,	"<Homebrew>",	"512Kb Multicart Demo (H??hn PCB)", PCB_HUEHN,	0 ) // This demo cartridge uses just 16 of the 32 available banks

/*
   The following conversion demonstrates the bank switching capabilities of Jos?? Leandro Novell??n Mart??nez' PCB.
   The 512 Kbytes PCB description and usage are described at http://www.speccy.org/trastero/cosas/JL/if2/IF2-1.html
*/
	SOFTWARE( scartif2,	0,	2004,	"<Homebrew>",	"512Kb Multicart Demo (Super Cartucho IF2 PCB)", PCB_SCARTIF2,	0 ) // This demo cartridge uses just 16 of the 32 available banks

SOFTWARE_LIST_END


SOFTWARE_LIST( spectrum_cart, "Sinclair ZX Spectrum cartridges" )

