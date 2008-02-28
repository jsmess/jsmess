/******************************************************************************

  messdriv.c

  The list of all available drivers. Drivers have to be included here to be
  recognized by the executable.

  To save some typing, we use a hack here. This file is recursively #included
  twice, with different definitions of the DRIVER() macro. The first one
  declares external references to the drivers; the second one builds an array
  storing all the drivers.

******************************************************************************/

#include "driver.h"


#ifndef DRIVER_RECURSIVE

#define DRIVER_RECURSIVE

/* step 1: declare all external references */
#define DRIVER(NAME) extern const game_driver driver_##NAME;
#define TESTDRIVER(NAME) extern const game_driver driver_##NAME;
#include "messdriv.c"

/* step 2: define the drivers[] array */
#undef DRIVER
#undef TESTDRIVER
#define DRIVER(NAME) &driver_##NAME,
#ifdef MESS_DEBUG
#define TESTDRIVER(NAME) &driver_##NAME,
#else
#define TESTDRIVER(NAME)
#endif
const game_driver * const drivers[] =
{
#include "messdriv.c"
  0             /* end of array */
};

/* step 2: define the test_drivers[] array */
#undef DRIVER
#undef TESTDRIVER
#define DRIVER(NAME)
#define TESTDRIVER(NAME) &driver_##NAME,
const game_driver *const test_drivers[] =
{
#include "messdriv.c"
	0	/* end of array */
};

#else /* DRIVER_RECURSIVE */

/****************CONSOLES****************************************************/

	/* 3DO */
	DRIVER( 3do )		/* 3DO consoles										*/
TESTDRIVER( 3do_pal )

	/* ATARI */
	DRIVER( a2600 )		/* Atari 2600										*/
	DRIVER( a2600p )	/* Atari 2600 PAL									*/
	DRIVER( a5200 )		/* Atari 5200										*/
	DRIVER( a5200a )	/* Atari 5200 alt									*/
	DRIVER( a7800 )		/* Atari 7800 NTSC									*/
	DRIVER( a7800p )	/* Atari 7800 PAL									*/
	DRIVER( lynx )		/* Atari Lynx Handheld								*/
	DRIVER( lynx2 )		/* Atari Lynx II Handheld redesigned, no additions	*/
	DRIVER( jaguar )	/* Atari Jaguar										*/
	DRIVER( atarist )	/* Atari ST											*/
	DRIVER( megast )	/* Atari Mega ST									*/
//	DRIVER( stacy )		/* Atari STacy										*/
	DRIVER( atariste )	/* Atari STe										*/
	DRIVER( megaste )	/* Atari Mega STe									*/
	DRIVER( stbook )	/* Atari STBook										*/
//	DRIVER( tt030 )		/* Atari TT030										*/
//	DRIVER( falcon )	/* Atari Falcon030									*/

	/* NINTENDO */
	DRIVER( nes )		/* Nintendo Entertainment System					*/
	DRIVER( nespal )	/* Nintendo Entertainment System					*/
	DRIVER( famicom )
	DRIVER( famitwin )	/* Sharp Famicom Twin System						*/
	DRIVER( gameboy )	/* Nintendo GameBoy Handheld						*/
	DRIVER( supergb )	/* Nintendo Super GameBoy SNES Cartridge			*/
	DRIVER( gbpocket )	/* Nintendo GameBoy Pocket Handheld					*/
	DRIVER( gbcolor )	/* Nintendo GameBoy Color Handheld					*/
	DRIVER( snes )		/* Nintendo Super Nintendo NTSC						*/
	DRIVER( snespal )	/* Nintendo Super Nintendo PAL						*/
	DRIVER( n64 )		/* Nintendo N64										*/
	DRIVER( pokemini )	/* Nintendo Pokemon Mini							*/

	DRIVER( megaduck )	/* Megaduck											*/

	/* SEGA */
	DRIVER( sg1000 )	/* Sega SG-1000 (Japan)								*/
	DRIVER( sg1000m2 )	/* Sega SG-1000 Mark II (Japan)						*/
	DRIVER( sc3000 )	/* Sega SC-3000 (Japan)								*/
	DRIVER( sf7000 )	/* Sega SC-3000 w/ SF-7000 (Japan)					*/

	DRIVER( gamegear )	/* Sega GameGear									*/
	DRIVER( gamegeaj )	/* Sega GameGear (Japan)							*/
	DRIVER( sms )		/* Sega Master System II (NTSC)						*/
	DRIVER( sms1 )		/* Sega Master System I (NTSC)						*/
	DRIVER( sms1pal )	/* Sega Master System I (PAL)						*/
	DRIVER( smspal )	/* Sega Master System II (PAL)						*/
	DRIVER( smsj )		/* Sega Master System (Japan) with FM Chip			*/
	DRIVER( sg1000m3 )	/* Sega SG-1000 Mark III (Japan)					*/
	DRIVER( sms2kr )	/* Samsung Gam*Boy II (Korea)						*/
	DRIVER( smssdisp )	/* Sega Master System Store Display Unit			*/

	DRIVER( megadrij )	/* 1988 Sega Mega Drive (Japan)						*/
	DRIVER( genesis )	/* 1989 Sega Genesis (USA)							*/
	DRIVER( megadriv )	/* 1990 Sega Mega Drive (Europe)					*/

	DRIVER( saturnjp )	/* 1994 Sega Saturn (Japan)							*/
	DRIVER( saturn )	/* 1995 Sega Saturn (USA)							*/
	DRIVER( saturneu )	/* 1995 Sega Saturn (Europe)						*/
	DRIVER( vsaturn )	/* JVC V-Saturn										*/
	DRIVER( hisaturn )	/* Hitachi HiSaturn									*/

	DRIVER( dcjp )		/* 1998 Sega Dreamcast (Japan) */
	DRIVER( dc )		/* 1999 Sega Dreamcast (USA) */
	DRIVER( dceu )		/* 1999 Sega Dreamcast (Europe) */
	DRIVER( dcdev )		/* 1998 Sega HKT-0120 Sega Dreamcast Development Box 					*/

	/* BALLY */
	DRIVER( astrocde )	/* Bally Astrocade									*/
	DRIVER( astrocdw )	/* Bally Astrocade (white case)						*/

	/* RCA */
	DRIVER( vip )		/* 1977 Cosmac VIP									*/
	DRIVER( studio2 )	/* 1977 Studio II									*/
TESTDRIVER( m9016tc )	/* 1978 Mustang 9016 Telespiel Computer				*/

	/* FAIRCHILD */
	DRIVER( channelf )	/* Fairchild Channel F VES - 1976					*/

	/* COLECO */
	DRIVER( coleco )	/* ColecoVision (Original BIOS)						*/
	DRIVER( colecoa )	/* ColecoVision (Thick Characters)					*/
	DRIVER( colecob )	/* Spectravideo SVI-603 Coleco Game Adapter			*/
	DRIVER( adam )		/* Coleco Adam										*/

	/* NEC */
	DRIVER( pce )		/* PC/Engine NEC 1987-1993							*/
	DRIVER( tg16 )		/* Turbo Grafix-16  NEC 1989-1993					*/
	DRIVER( sgx )		/* SuperGrafX NEC 1989								*/

	DRIVER( arcadia )	/* Emerson Arcadia 2001								*/
TESTDRIVER( vcg )		/* Palladium Video-Computer-Game					*/

	/* GCE */
	DRIVER( vectrex )	/* General Consumer Electric Vectrex - 1982-1984	*/
						/* (aka Milton-Bradley Vectrex)						*/
	DRIVER( raaspec )	/* RA+A Spectrum - Modified Vectrex					*/

	/* MATTEL */
	DRIVER( intv )		/* Mattel Intellivision - 1979 AKA INTV				*/
	DRIVER( intvsrs )	/* Intellivision (Sears License) - 19??				*/

	/* ENTEX */
	DRIVER( advision )	/* Adventurevision									*/

	/* CAPCOM */
	DRIVER( sfzch )		/* CPS Changer (Street Fighter ZERO)				*/

	/* MAGNAVOX */
	DRIVER( odyssey2 )	/* Magnavox Odyssey 2 - 1978-1983					*/

	/* WATARA */
	DRIVER( svision )	/* Supervision Handheld								*/
TESTDRIVER( svisions )

TESTDRIVER( svisionp )
TESTDRIVER( svisionn )
TESTDRIVER( tvlinkp )

	/* INTERTON */
	DRIVER( vc4000 )	/* Interton vc4000									*/

	/* BANDAI */
	DRIVER( wswan )		/* Bandai WonderSwan Handheld						*/
	DRIVER( wscolor )	/* Bandai WonderSwan Color Handheld					*/

	/* EPOCH */
	DRIVER( gamepock )	/* Epoch Game Pocket Computer						*/


/****************COMPUTERS***************************************************/
    /* ACORN */
	DRIVER( acrnsys1 )  /* 1979 Acorn System 1 (Microcomputer Kit)          */
	DRIVER( atom )		/* 1979 Acorn Atom									*/
	DRIVER( atomeb )	/* 1979 Acorn Atom									*/
	DRIVER( bbca )		/* 1981 BBC Micro Model A							*/
	DRIVER( bbcb )		/* 1981 BBC Micro Model B							*/
	DRIVER( bbcbp )		/* 198? BBC Micro Model B+ 64K						*/
	DRIVER( bbcbp128 )	/* 198? BBC Micro Model B+ 128K						*/
	DRIVER( bbcm)		/* 198? BBC Master									*/
	DRIVER( bbcbc )		/* 1985 BBC Bridge Companion						*/
	DRIVER( electron )	/* 1983 Acorn Electron								*/
TESTDRIVER( a310 )		/* 1988 Acorn Archimedes 310						*/

	/* CAMBRIDGE COMPUTERS */
	DRIVER( z88 )		/*													*/

	/* AMSTRAD */
	DRIVER( cpc464 )	/* Amstrad (Schneider in Germany) 1984				*/
	DRIVER( cpc664 )	/* Amstrad (Schneider in Germany) 1985				*/
	DRIVER( cpc6128 )	/* Amstrad (Schneider in Germany) 1985				*/
	DRIVER( cpc6128f )	/* Amstrad (Schneider in Germany) 1985 (AZERTY)		*/
	DRIVER( cpc464p )	/* Amstrad CPC464  Plus - 1990						*/
	DRIVER( cpc6128p )	/* Amstrad CPC6128 Plus - 1990						*/
	DRIVER( gx4000 )    /* Amstrad GX4000 - 1990                            */
	DRIVER( kccomp )	/* VEB KC compact									*/
	DRIVER( al520ex )   /* Patisonic Aleste 520EX (1993)                    */
TESTDRIVER( pcw8256 )	/* 198? PCW8256										*/
TESTDRIVER( pcw8512 )	/* 198? PCW8512										*/
TESTDRIVER( pcw9256 )	/* 198? PCW9256										*/
TESTDRIVER( pcw9512 )	/* 198? PCW9512 (+)									*/
TESTDRIVER( pcw10 ) 	/* 198? PCW10										*/
TESTDRIVER( pcw16 )		/* 1995 PCW16										*/
	DRIVER( nc100 )		/* 19?? NC100										*/
TESTDRIVER( nc200 )		/* 19?? NC200										*/

	/* APPLE */
	DRIVER( apple1 )	/* Jul 1976 Apple 1									*/
	DRIVER( apple2 )	/* Apr 1977 Apple ][								*/
	DRIVER( apple2p )	/* Jun 1979 Apple ][+								*/
	DRIVER( apple2jp )	/* ??? ???? Apple ][j+								*/
	DRIVER( apple2e )	/* Jan 1983 Apple //e								*/
	DRIVER( apple2ee )	/* Mar 1985 Apple //e Enhanced						*/
	DRIVER( apple2ep )	/* Jan 1987 Apple //e Platinum						*/
	DRIVER( apple2c )	/* Apr 1984 Apple //c								*/
	DRIVER( apple2c0 )	/* ??? 1985 Apple //c (3.5 ROM)						*/
	DRIVER( apple2c3 )	/* Sep 1986 Apple //c (Original Mem. Exp.)			*/
	DRIVER( apple2c4 )	/* ??? 198? Apple //c (rev 4)						*/
	DRIVER( apple2cp )	/* Sep 1988 Apple //c+								*/
	DRIVER( apple2g0 )	/* Sep 1986 Apple IIgs ROM00						*/
	DRIVER( apple2g1 )	/* Sep 1987 Apple IIgs ROM01						*/
	DRIVER( apple2gs )	/* Aug 1989 Apple IIgs ROM03						*/
	DRIVER( apple3 )	/* May 1980 Apple ///								*/
						/* Dec 1983 Apple ///+								*/
	DRIVER( ace100 )	/* ??? 1982 Franklin Ace 100						*/
	DRIVER( laser128 )	/* ??? 1987 Laser 128								*/
	DRIVER( las128ex )	/* ??? 1987 Laser 128 EX							*/
TESTDRIVER( las3000 )	/* ??? 1983 Laser 3000								*/

/*
 * Lisa 				 January			 1983
 * Lisa 2 				 January			 1984
 * Macintosh XL 		 January			 1985
 */
	DRIVER( lisa2 )		/* 1984 Apple Lisa 2								*/
	DRIVER( lisa210 )	/* 1984 Apple Lisa 2/10								*/
	DRIVER( macxl )		/* 1984 Apple Macintosh XL							*/
/*
 * Macintosh			January				1984
 * Macintosh 512k		July?				1984
 * Macintosh 512ke		April				1986
 * Macintosh Plus		January				1986
 * Macintosh SE			March				1987
 * Macintosh II			March				1987
 */
 	DRIVER( mac128k )	/* 1984 Apple Macintosh								*/
	DRIVER( mac512k )	/* 1985 Apple Macintosh 512k						*/
	DRIVER( mac512ke )	/* 1986 Apple Macintosh 512ke						*/
	DRIVER( macplus )	/* 1986 Apple Macintosh Plus						*/
	DRIVER( macse )		/* 1987 Apple Macintosh SE							*/
	DRIVER( macclasc )	/* 1990 Apple Macintosh Classic						*/

	/* ATARI */
/*
400/800 10kB OS roms
A    NTSC  (?)         (?)         (?)
A    PAL   (?)         0x72b3fed4  CO15199, CO15299, CO12399B
B    NTSC  (?)         0x0e86d61d  CO12499B, CO14599B, 12399B
B    PAL   (?)         (?)         (?)

XL/XE 16kB OS roms
10   1200XL  10/26/1982  0xc5c11546  CO60616A, CO60617A
11   1200XL  12/23/1982  (?)         CO60616B, CO60617B
1    600XL   03/11/1983  0x643bcc98  CO62024
2    XL/XE   05/10/1983  0x1f9cd270  CO61598B
3    800XE   03/01/1985  0x29f133f7  C300717
4    XEGS    05/07/1987  0x1eaf4002  C101687
*/

	DRIVER( a400 )		/* 1979 Atari 400									*/
	DRIVER( a400pal )	/* 1979 Atari 400 PAL								*/
	DRIVER( a800 )		/* 1979 Atari 800									*/
	DRIVER( a800pal )	/* 1979 Atari 800 PAL								*/
	DRIVER( a800xl )	/* 1983 Atari 800 XL								*/

	/* COMMODORE */
	DRIVER( kim1 )		/* Commodore (MOS) KIM-1 1975						*/
	DRIVER( sym1 )		/* Synertek SYM-1									*/
	DRIVER( aim65 )		/* Rockwell AIM65									*/

	DRIVER( pet )		/* PET2001/CBM20xx Series (Basic 1)					*/
	DRIVER( cbm30 )		/* Commodore 30xx (Basic 2)							*/
	DRIVER( cbm30b )	/* Commodore 30xx (Basic 2) (business keyboard)		*/
	DRIVER( cbm40 )		/* Commodore 40xx FAT (CRTC) 60Hz					*/
	DRIVER( cbm40pal )	/* Commodore 40xx FAT (CRTC) 50Hz					*/
	DRIVER( cbm40b )	/* Commodore 40xx THIN (business keyboard)			*/
	DRIVER( cbm80 )		/* Commodore 80xx 60Hz								*/
	DRIVER( cbm80pal )	/* Commodore 80xx 50Hz								*/
	DRIVER( cbm80ger )	/* Commodore 80xx German (50Hz)						*/
	DRIVER( cbm80swe )	/* Commodore 80xx Swedish (50Hz)					*/
	DRIVER( superpet )	/* Commodore SP9000/MMF9000 (50Hz)					*/
TESTDRIVER( mmf9000 )	/* Commodore MMF9000 Swedish						*/

	DRIVER( vic20 )		/* Commodore Vic-20 NTSC							*/
	DRIVER( vic1001 )	/* Commodore VIC-1001 (VIC20 Japan)					*/
	DRIVER( vc20 )		/* Commodore Vic-20 PAL								*/
	DRIVER( vic20swe )	/* Commodore Vic-20 Sweden							*/
TESTDRIVER( vic20v )	/* Commodore Vic-20 NTSC, VC1540					*/
TESTDRIVER( vc20v )		/* Commodore Vic-20 PAL, VC1541						*/
	DRIVER( vic20i )	/* Commodore Vic-20 IEEE488 Interface				*/

	DRIVER( max )		/* Max (Japan)/Ultimax (US)/VC10 (German)			*/
	DRIVER( c64 )		/* Commodore 64 - NTSC								*/
	DRIVER( c64pal )	/* Commodore 64 - PAL								*/
	DRIVER( vic64s )	/* Commodore VIC64S (Swedish)						*/
	DRIVER( cbm4064 )	/* Commodore CBM4064								*/
TESTDRIVER( sx64 )		/* Commodore SX 64 - PAL							*/
TESTDRIVER( vip64 )		/* Commodore VIP64 (SX64, PAL, Swedish)				*/
TESTDRIVER( dx64 )		/* Commodore DX 64 - PROTOTPYE, PAL					*/
	DRIVER( c64gs )		/* Commodore 64 Games System						*/

	DRIVER( cbm500 )	/* Commodore 500/P128-40							*/
	DRIVER( cbm610 )	/* Commodore 610/B128LP								*/
	DRIVER( cbm620 )	/* Commodore 620/B256LP								*/
	DRIVER( cbm620hu )	/* Commodore 620/B256LP Hungarian					*/
	DRIVER( cbm710 )	/* Commodore 710/B128HP								*/
	DRIVER( cbm720 )	/* Commodore 720/B256HP								*/
	DRIVER( cbm720se )	/* Commodore 720/B256HP Swedish/Finnish				*/

	DRIVER( c16 )		/* Commodore 16										*/
	DRIVER( c16hun )	/* Commodore 16 Novotrade (Hungarian Character Set)	*/
	DRIVER( c16c )		/* Commodore 16  c1551								*/
TESTDRIVER( c16v )		/* Commodore 16  vc1541								*/
	DRIVER( plus4 )		/* Commodore +4  c1551								*/
	DRIVER( plus4c )	/* Commodore +4  vc1541								*/
TESTDRIVER( plus4v )	/* Commodore +4										*/
	DRIVER( c364 )		/* Commodore 364 - Prototype						*/

	DRIVER( c128 )		/* Commodore 128 - NTSC								*/
	DRIVER( c128ger )	/* Commodore 128 - PAL (german)						*/
	DRIVER( c128fra )	/* Commodore 128 - PAL (french)						*/
	DRIVER( c128ita )	/* Commodore 128 - PAL (italian)					*/
	DRIVER( c128swe )	/* Commodore 128 - PAL (swedish)					*/
TESTDRIVER( c128nor )	/* Commodore 128 - PAL (norwegian)					*/
TESTDRIVER( c128d )		/* Commodore 128D - NTSC							*/
TESTDRIVER( c128dita )	/* Commodore 128D - PAL (italian) cost reduced set	*/

	DRIVER( a500n )		/* Commodore Amiga 500 - NTSC						*/
	DRIVER( a500p )		/* Commodore Amiga 500 - PAL						*/
	DRIVER( a1000n )	/* Commodore Amiga 1000 - NTSC						*/
	DRIVER( cdtv )

	DRIVER( c65 )		/* 1991 C65 / C64DX (Prototype, NTSC)				*/
	DRIVER( c64dx )		/* 1991 C65 / C64DX (Prototype, German PAL)			*/

	/* IBM PC & Clones */
	DRIVER( ibmpc )		/* 1982	IBM PC										*/
	DRIVER( ibmpca )	/* 1982 IBM PC										*/
	DRIVER( dgone )		/* 1984 Data General/One */
	DRIVER( pcmda )		/* 1987 PC with MDA (MGA aka Hercules)				*/
	DRIVER( pc )		/* 1987 PC with CGA									*/
TESTDRIVER( bondwell )	/* 1985	Bondwell (CGA)								*/
	DRIVER( europc )	/* 1988	Schneider Euro PC (CGA or Hercules)			*/

	/* pc junior */
TESTDRIVER( ibmpcjr )	/* 1984 IBM PC Jr									*/
	DRIVER( t1000hx )	/* 1987 Tandy 1000HX (similiar to PCJr)				*/
TESTDRIVER( t1000sx )	/* 1987 Tandy 1000SX (similiar to PCJr)				*/

	/* xt */
	DRIVER( ibmxt )		/* 1986	IBM XT										*/
	DRIVER( pc200 )		/* 1988 Sinclair PC200								*/
	DRIVER( pc20 )		/* 1988 Amstrad PC20								*/
TESTDRIVER( ppc512 )	/* 1987 Amstrad PPC512								*/
TESTDRIVER( ppc640 )	/* 1987 Amstrad PPC640								*/
	DRIVER( pc1512 )	/* 1986 Amstrad PC1512 v1 (CGA compatible)			*/
	DRIVER( pc1512v2 )	/* 1986 Amstrad PC1512 v2 (CGA compatible)			*/
	DRIVER( pc1640 )	/* 1987 Amstrad PC1640 (EGA compatible)				*/

	DRIVER( xtvga )		/* 198? PC-XT (VGA, MF2 Keyboard)					*/

	/* at */
	DRIVER( ibmat )		/* 1985	IBM AT										*/
TESTDRIVER( i8530286 )	/* 1988 IBM PS2 Model 30 286 (VGA)					*/
	DRIVER( at )		/* 1987 AMI Bios and Diagnostics					*/
	DRIVER( atvga )		/* 19?? AT VGA										*/
TESTDRIVER( neat )		/* 1989	New Enhanced AT chipset, AMI BIOS			*/
	DRIVER( at386 )		/* 19?? IBM AT 386									*/
	DRIVER( at486 )		/* 19?? IBM AT 486									*/
	DRIVER( at586 )		/* 19?? AT 586										*/

	/* OSBORNE */
	DRIVER( osborne1 )	/* 1981 Osborne-1									*/

	/* SINCLAIR RESEARCH */
	DRIVER( zx80 )		/* 1980 Sinclair ZX-80								*/
	DRIVER( zx81 )		/* 1981 Sinclair ZX-81								*/
	DRIVER( ts1000 )	/* 1982 Timex Sinclair 1000							*/
	DRIVER( aszmic )	/* ASZMIC ZX-81 ROM swap							*/
	DRIVER( pc8300 )	/* Your Computer - PC8300							*/
	DRIVER( pow3000 )	/* Creon Enterprises - Power 3000					*/
	DRIVER( lambda )	/* Lambda 8300										*/
TESTDRIVER( h4th )		/* Sinclair ZX-81 Forth by David Husband			*/
TESTDRIVER( tree4th )	/* Sinclair ZX-81 Tree-Forth by Tree Systems		*/

	DRIVER( spectrum )	/* 1982 ZX Spectrum									*/
	DRIVER( specpls4 )	/* 2000 ZX Spectrum +4								*/
	DRIVER( specbusy )	/* 1994 ZX Spectrum (BusySoft Upgrade v1.18)		*/
	DRIVER( specpsch )	/* 19?? ZX Spectrum (Maly's Psycho Upgrade)			*/
	DRIVER( specgrot )	/* ???? ZX Spectrum (De Groot's Upgrade)			*/
	DRIVER( specimc )	/* 1985 ZX Spectrum (Collier's Upgrade)				*/
	DRIVER( speclec )	/* 1987 ZX Spectrum (LEC Upgrade)					*/
	DRIVER( inves )		/* 1986 Inves Spectrum 48K+							*/
	DRIVER( tk90x )		/* 1985 TK90x Color Computer						*/
	DRIVER( tk95 )		/* 1986 TK95 Color Computer							*/
	DRIVER( tc2048 )	/* 198? TC2048										*/
	DRIVER( ts2068 )	/* 1983 TS2068										*/
	DRIVER( uk2086 )	/* 1986 UK2086										*/

	DRIVER( spec128 )	/* 1986 ZX Spectrum 128								*/
	DRIVER( spec128s )	/* 1985 ZX Spectrum 128 (Spain)						*/
	DRIVER( specpls2 )	/* 1986 ZX Spectrum +2								*/
	DRIVER( specpl2a )	/* 1987 ZX Spectrum +2a								*/
	DRIVER( specpls3 )	/* 1987 ZX Spectrum +3								*/
	DRIVER( specp2fr )	/* 1986 ZX Spectrum +2 (France)						*/
	DRIVER( specp2sp )	/* 1986 ZX Spectrum +2 (Spain)						*/
	DRIVER( specp3sp )	/* 1987 ZX Spectrum +3 (Spain)						*/
	DRIVER( specpl3e )	/* 2000 ZX Spectrum +3e								*/
	DRIVER( specp3es )	/* 2000 ZX Spectrum +3e (Spain)						*/

TESTDRIVER( scorpion )
TESTDRIVER( pentagon )

	DRIVER( ql )		/* 1984 Sinclair QL	(UK)							*/
	DRIVER( ql_us )		/* 1984 Sinclair QL	(USA)							*/
	DRIVER( ql_es )		/* 1984 Sinclair QL	(Spain)							*/
	DRIVER( ql_fr )		/* 1984 Sinclair QL	(France)						*/
	DRIVER( ql_de )		/* 1984 Sinclair QL	(Germany)						*/
	DRIVER( ql_it )		/* 1984 Sinclair QL	(Italy)							*/
	DRIVER( ql_se )		/* 1984 Sinclair QL	(Sweden)						*/
	DRIVER( ql_dk )		/* 1984 Sinclair QL	(Denmark)						*/
	DRIVER( ql_gr )		/* 1984 Sinclair QL	(Greece)						*/

	/* SHARP */
	DRIVER( pc1251 )	/* Pocket Computer 1251								*/
TESTDRIVER( trs80pc3 )	/* Tandy TRS80 PC-3									*/

	DRIVER( pc1401 )	/* Pocket Computer 1401								*/
	DRIVER( pc1402 )	/* Pocket Computer 1402								*/

	DRIVER( pc1350 )	/* Pocket Computer 1350								*/

	DRIVER( pc1403 )	/* Pocket Computer 1403								*/
	DRIVER( pc1403h )	/* Pocket Computer 1403H							*/

	DRIVER( mz700 )		/* 1982 Sharp MZ700									*/
	DRIVER( mz700j )	/* 1982 Sharp MZ700 Japan							*/
TESTDRIVER( mz800  )	/* 1982 Sharp MZ800									*/

	DRIVER( x68000 )    /* Sharp X68000 (1987)								*/

	/* SILICON GRAPHICS */
	DRIVER( ip204415 )	/* IP20 Indigo2										*/
	DRIVER( ip225015 )
	DRIVER( ip224613 )
	DRIVER( ip244415 )

	/* TEXAS INSTRUMENTS */
	DRIVER( ti990_10 )	/* 1975 TI 990/10									*/
	DRIVER( ti990_4 )	/* 1976 TI 990/4									*/
	DRIVER( 990189 )	/* 1978 TM 990/189									*/
	DRIVER( 990189v )	/* 1980 TM 990/189 with Color Video Board			*/

TESTDRIVER( ti99_224 )	/* 1983 TI 99/2 (24kb ROMs)							*/
TESTDRIVER( ti99_232 )	/* 1983 TI 99/2 (32kb ROMs)							*/
	DRIVER( ti99_4 )	/* 1979 TI-99/4										*/
	DRIVER( ti99_4e )	/* 1980 TI-99/4 with 50Hz video						*/
	DRIVER( ti99_4a )	/* 1981 TI-99/4A									*/
	DRIVER( ti99_4ae )	/* 1981 TI-99/4A with 50Hz video					*/
	DRIVER( ti99_4ev)	/* 1994 TI-99/4A with EVPC video card				*/
	DRIVER( ti99_8 )	/* 1983 TI-99/8										*/
	DRIVER( ti99_8e )	/* 1983 TI-99/8 with 50Hz video						*/
	/* TI 99 clones */
	DRIVER( tutor)		/* 1983? Tomy Tutor									*/
	DRIVER( geneve )	/* 1987? Myarc Geneve 9640							*/
TESTDRIVER( genmod )	/* 199?? Myarc Geneve 9640							*/
TESTDRIVER( ti99_4p )	/* 1996 SNUG 99/4P (a.k.a. SGCPU)					*/

	DRIVER( avigo )		/*													*/

	/* TEXAS INSTRUMENTS CALCULATORS */
	DRIVER( ti81 )		/* 1990 TI-81 (Z80 2 MHz)							*/
	DRIVER( ti85 )		/* 1992 TI-85 (Z80 6 MHz)							*/
	DRIVER( ti82 )		/* 1993 TI-82 (Z80 6 MHz)							*/
	DRIVER( ti83 )		/* 1996 TI-83 (Z80 6 MHz)							*/
	DRIVER( ti86 )		/* 1997 TI-86 (Z80 6 MHz)							*/
	DRIVER( ti83p )		/* 1999 TI-83 Plus (Z80 6 MHz)						*/

	/* NEC */
	DRIVER( pc88srl )	/* PC-8801mkIISR(Low res display, VSYNC 15KHz)		*/
	DRIVER( pc88srh )	/* PC-8801mkIISR(High res display, VSYNC 24KHz)		*/

	/* CANTAB */
	DRIVER( jupiter )	/* Jupiter Ace										*/

	/* SORD */
	DRIVER( sordm5 )
TESTDRIVER( srdm5fd5 )

	/* APF Electronics Inc. */
	DRIVER( apfm1000 )
	DRIVER( apfimag )

	/* Tatung */
	DRIVER( einstein )
TESTDRIVER( einstei2 )

	/* INTELLIGENT SOFTWARE */
	DRIVER( ep128 )		/* Enterprise 128 k									*/
	DRIVER( ep128a )	/* Enterprise 128 k									*/

	/* NON LINEAR SYSTEMS */
	DRIVER( kaypro )	/* Kaypro 2X										*/

	/* VEB MIKROELEKTRONIK */
	/* KC compact is partial CPC compatible */
	DRIVER( kc85_4 )	/* VEB KC 85/4										*/
	DRIVER( kc85_3 )	/* VEB KC 85/3										*/
TESTDRIVER( kc85_4d )	/* VEB KC 85/4 with disk interface					*/

	/* MICROBEE SYSTEMS */
	DRIVER( mbee )		/* Microbee 32 IC									*/
	DRIVER( mbeepc )	/* Microbee 32 PC									*/
	DRIVER( mbeepc85 )	/* Microbee 32 PC85									*/
	DRIVER( mbee56 )	/* Microbee 56K (CP/M)								*/

	/* TANDY RADIO SHACK */
	DRIVER( trs80 )		/* TRS-80 Model I	- Radio Shack Level I BASIC		*/
	DRIVER( trs80l2 )	/* TRS-80 Model I	- Radio Shack Level II BASIC	*/
	DRIVER( trs80l2a )	/* TRS-80 Model I	- R/S L2 BASIC					*/
	DRIVER( sys80 )		/* EACA System 80									*/
	DRIVER( lnw80 )		/* LNW Research LNW-80								*/
TESTDRIVER( trs80m3 )	/* TRS-80 Model III - Radio Shack/Tandy				*/
TESTDRIVER( trs80m4 )

	DRIVER( coco )		/* Color Computer									*/
	DRIVER( cocoe )		/* Color Computer (Extended BASIC 1.0)				*/
	DRIVER( coco2 )		/* Color Computer 2									*/
	DRIVER( coco2b )	/* Color Computer 2B (uses M6847T1 video chip)		*/
	DRIVER( coco3 )		/* Color Computer 3 (NTSC)							*/
	DRIVER( coco3p )	/* Color Computer 3 (PAL)							*/
	DRIVER( coco3h )	/* Hacked Color Computer 3 (6309)					*/
	DRIVER( dragon32 )	/* Dragon 32										*/
	DRIVER( dragon64 )	/* Dragon 64										*/
	DRIVER( d64plus )	/* Dragon 64 + Compusense Plus addon				*/
	DRIVER( dgnalpha )	/* Dragon Alpha										*/
	DRIVER( dgnbeta )	/* Dragon Beta										*/
	DRIVER( tanodr64 )	/* Tano Dragon 64 (NTSC)							*/
	DRIVER( cp400 )		/* Prologica CP400									*/
	DRIVER( mc10 )		/* MC-10											*/
	DRIVER( alice )		/* Matra & Hachette Ordinateur Alice				*/

	/* EACA */
	DRIVER( cgenie )	/* Colour Genie EG2000								*/
	/* system 80 trs80 compatible */

	/* VIDEO TECHNOLOGY */
	DRIVER( crvision )	/* 1981 creatiVision								*/
	DRIVER( fnvision )	/* 1983 FunVision									*/
	DRIVER( laser110 )	/* 1983 Laser 110									*/
	DRIVER( las110de )	/* 1983 Sanyo Laser 110 (Germany)					*/
	DRIVER( laser200 )	/* 1983 Laser 200 (color version of 110)			*/
	DRIVER( vz200de )	/* 1983 VZ-200 (Germany)							*/
	DRIVER( fellow )	/* 1983 Salora Fellow (Finland) 					*/
	DRIVER( tx8000 )	/* 1983 Texet TX-8000 (U.K.)						*/
	DRIVER( laser210 )	/* 1984 Laser 210 (200 with more memory)			*/
	DRIVER( las210de )	/* 1984 Sanyo Laser 210 (Germany)					*/
	DRIVER( vz200 )		/* 1984 Dick Smith Electronics VZ-200				*/
	DRIVER( laser310 )	/* 1984 Laser 310 (210 with diff. keyboard and RAM)	*/
	DRIVER( vz300 )		/* 1984 Dick Smith Electronics VZ-300				*/
	DRIVER( laser350 )	/* 1984? Laser 350									*/
	DRIVER( laser500 )	/* 1984? Laser 500									*/
	DRIVER( laser700 )	/* 1984? Laser 700									*/

	/* TANGERINE */
	DRIVER( microtan )	/* 1979 Microtan 65									*/

	DRIVER( oric1 )		/* 1983 Oric 1										*/
	DRIVER( orica )		/* 1984 Oric Atmos									*/
	DRIVER( prav8d )	/* 1985 Pravetz 8D									*/
	DRIVER( prav8dd )	/* 1989 Pravetz 8D (Disk ROM)						*/
	DRIVER( prav8dda )	/* 1989 Pravetz 8D (Disk ROM, alternate)			*/
	DRIVER( telstrat )	/* ??? Oric Telestrat/Stratos						*/

	/* PHILIPS */
	DRIVER( p2000t )	/* 1980 P2000T										*/
	DRIVER( p2000m )	/* 1980 P2000M										*/
	DRIVER( videopac )	/* 1979 Videopac G7000/C52							*/
TESTDRIVER( g7400 )		/* 1983 Videopac Plus G7400							*/

	/* COMPUKIT */
	DRIVER( uk101 )		/* 1979 UK101										*/

	/* OHIO SCIENTIFIC */
	DRIVER( sb2m600b )	/* 1979 Superboard II								*/

	/* ASCII & MICROSOFT */
	DRIVER( msx )		/* 1983 MSX 										*/
	DRIVER( cf2000 )	/* 1983 MSX Japan									*/
	DRIVER( cf1200 )	/* 1984 MSX Japan									*/
	DRIVER( cf2700 )	/* 1984 MSX Japan									*/
	DRIVER( cf3000 )	/* 1984 MSX Japan									*/
	DRIVER( cf3300 )	/* 1985 MSX Japan									*/
	DRIVER( fs4000 )	/* 1985 MSX Japan									*/
	DRIVER( fs1300 )	/* 1985 MSX Japan									*/
	DRIVER( hb201 )		/* 1985 MSX Japan									*/
	DRIVER( dpc100 )	/* 1984 MSX Korea									*/
	DRIVER( dpc180 )	/* 1984 MSX Korea									*/
	DRIVER( dpc200 )	/* 1984 MSX Korea									*/
	DRIVER( hb75d )		/* 1983 MSX Germany									*/
	DRIVER( hb75p )		/* 1983 MSX											*/
	DRIVER( hb501p )	/* 1984 MSX											*/
	DRIVER( hb201p )	/* 1985 MSX											*/
	DRIVER( svi738 )	/* 1985 MSX											*/
	DRIVER( hotbit11 )	/* 1985 MSX Brazil									*/
	DRIVER( hotbit12 )	/* 1985 MSX	Brazil									*/
	DRIVER( hx10 )		/* 1984 MSX										*/
	DRIVER( vg8020 )	/* 1985 MSX										*/
	DRIVER( expert10 )	/* 1983 MSX Brazil									*/
	DRIVER( expert11 )	/* 1984 MSX Brazil									*/
	DRIVER( expertdp )	/* 1985 MSX Brazil									*/
	DRIVER( msx2 )		/* 1985 MSX2										*/
	DRIVER( hbf9p )		/* 1985 MSX2										*/
	DRIVER( hbf500p )	/* 1985 MSX2										*/
	DRIVER( hbf700d )	/* 1985 MSX2 Germany								*/
	DRIVER( hbf700p )	/* 1985 MSX2										*/
	DRIVER( hbf700s )	/* 1985 MSX2 Spain									*/
	DRIVER( hbg900p )	/* 1986 MSX2										*/
	DRIVER( nms8220 )	/* 1986 MSX2										*/
	DRIVER( nms8220a )	/* 1986 MSX2										*/
	DRIVER( vg8235 )	/* 1986 MSX2										*/
	DRIVER( nms8245 )	/* 1986 MSX2										*/
	DRIVER( nms8250 )	/* 1986 MSX2										*/
	DRIVER( nms8255 )	/* 1986 MSX2										*/
	DRIVER( nms8280 )	/* 1986 MSX2										*/
	DRIVER( fs5500 )	/* 1985 MSX2 Japan									*/
	DRIVER( fs4500 )	/* 1986 MSX2 Japan									*/
	DRIVER( fs4700 )	/* 1986 MSX2 Japan									*/
	DRIVER( fs5000 )	/* 1986 MSX2 Japan									*/
	DRIVER( fs4600 )	/* 1986 MSX2 Japan									*/
	DRIVER( fsa1 )		/* 1986 MSX2 Japan									*/
	DRIVER( fsa1a )		/* 1986 MSX2 Japan									*/
	DRIVER( fsa1mk2 )	/* 1987 MSX2 Japan									*/
	DRIVER( fsa1f )		/* 1987 MSX2 Japan									*/
	DRIVER( fsa1fm )	/* 1988 MSX2 Japan									*/
	DRIVER( hbf500 )	/* 1986 MSX2 Japan									*/
	DRIVER( hbf900 )	/* 1986 MSX2 Japan									*/
	DRIVER( hbf900a )	/* 1986 MSX2 Japan									*/
	DRIVER( hbf1 )		/* 1986 MSX2 Japan									*/
	DRIVER( hbf12 )		/* 1987 MSX2 Japan									*/
	DRIVER( hbf1xd )	/* 1987 MSX2 Japan									*/
	DRIVER( hbf1xdm2 )	/* 1988 MSX2 Japan									*/
	DRIVER( phc23 )		/* 1986 MSX2 Japan									*/
	DRIVER( cpc300 )	/* 1986 MSX2 Korea									*/
	DRIVER( cpc400 )	/* 1986 MSX2 Korea									*/
	DRIVER( cpc400s )	/* 1986 MSX2 Korea									*/
	DRIVER( msx2p )		/* 1988 MSX2+ Japan									*/
	DRIVER( fsa1fx )	/* 1988 MSX2+ Japan									*/
	DRIVER( fsa1wx )	/* 1988 MSX2+ Japan									*/
	DRIVER( fsa1wxa )	/* 1988 MSX2+ Japan									*/
	DRIVER( fsa1wsx )	/* 1989 MSX2+ Japan									*/
	DRIVER( hbf1xdj )	/* 1988 MSX2+ Japan									*/
	DRIVER( hbf1xv )	/* 1989 MSX2+ Japan									*/
	DRIVER( phc70fd )	/* 1988 MSX2+ Japan									*/
	DRIVER( phc70fd2 )	/* 1988 MSX2+ Japan									*/
	DRIVER( phc35j )	/* 1989 MSX2+ Japan									*/


	/* NASCOM MICROCOMPUTERS */
	DRIVER( nascom1 )	/* 1978 Nascom 1									*/
	DRIVER( nascom2 )	/* 1979 Nascom 2									*/


	/* MILES GORDON TECHNOLOGY */
	DRIVER( coupe )		/* 1989 Sam Coupe									*/

	/* MOTOROLA */
TESTDRIVER( mekd2 )		/* 1977 Motorola Evaluation Kit						*/

	/* DEC */
	DRIVER( pdp1 )		/* 1961 DEC PDP1									*/

	/* MEMOTECH */
	DRIVER( mtx512 )	/* 1983 Memotech MTX 512							*/
	DRIVER( mtx500 )    /* 1983 Memotech MTX 500                            */
	DRIVER( rs128 )     /* 1984 Memotech RS 128                             */

	/* MATTEL */
	DRIVER( intvkbd )	/* 1981 - Mattel Intellivision Keyboard Component	*/
						/* (Test marketed, later recalled)					*/
	DRIVER( aquarius )	/* 1983 Aquarius									*/

	/*EXIDY INC */
	DRIVER( exidy )		/* Sorcerer											*/
	DRIVER( exidyd )	/* Sorcerer (diskless)								*/

	/* GALAKSIJA */
	DRIVER( galaxy )

	/* Lviv/L'vov */
	DRIVER( lviv )		/* Lviv/L'vov										*/

	/* Tesla */
	DRIVER( pmd851 )	/* PMD-85.1											*/
	DRIVER( pmd852 )	/* PMD-85.2											*/
	DRIVER( pmd852a )	/* PMD-85.2A										*/
	DRIVER( pmd852b )	/* PMD-85.2B										*/
	DRIVER( pmd853 )	/* PMD-85.3											*/

	/* Didaktik */
	DRIVER( alfa )		/* Alfa (PMD-85.1 clone)							*/

	/* Statny */
	DRIVER( mato )		/* Mato (Basic ROM) (PMD-85.2 clone)				*/
	DRIVER( matoh )		/* Mato (Games ROM) (PMD-85.2 clone)				*/

	/* Microkey */
	DRIVER( primoa32 )	/* Primo A-32										*/
	DRIVER( primoa48 )	/* Primo A-48										*/
	DRIVER( primoa64 )	/* Primo A-64										*/
TESTDRIVER( primob32 )	/* Primo B-32										*/
TESTDRIVER( primob48 )	/* Primo B-48										*/
	DRIVER( primob64 )	/* Primo B-64										*/

	/* Team Concepts */
	/* CPU not known, else should be easy, look into drivers/comquest.c */
TESTDRIVER( comquest )	/* Comquest Plus German								*/

	/* Hewlett Packard */
TESTDRIVER( hp48s )		/* HP48 S/SX										*/
TESTDRIVER( hp48g )		/* HP48 G/GX										*/

	/* SpectraVideo */
	DRIVER( svi318 )	/* SVI-318 (PAL)									*/
	DRIVER( svi318n )	/* SVI-318 (NTSC)									*/
	DRIVER( svi328 )	/* SVI-328 (PAL)									*/
	DRIVER( svi328n )	/* SVI-328 (NTSC)									*/
	DRIVER( sv328p80 )	/* SVI-328 (PAL) + SVI-806 80 column card			*/
	DRIVER( sv328n80 )	/* SVI-328 (NTSC) + SVI-806 80 column card			*/

	/* Andrew Donald Booth (this is the name of the designer, not a company) */
	DRIVER( apexc )		/* 1951(?) APEXC: All-Purpose Electronic X-ray Computer */

	/* Sony */
	DRIVER( psj )		/* 1994 Sony PlayStation (Japan)					*/
	DRIVER( psu )		/* 1995 Sony PlayStation (USA)						*/
	DRIVER( pse )		/* 1995 Sony PlayStation (Europe)					*/
	DRIVER( psa )		/* 1995 Sony PlayStation (Asia-Pacific)				*/

	/* Corvus */
	DRIVER(concept)		/* 1982 Corvus Concept								*/

	/* DAI */
	DRIVER(dai)			/* DAI												*/

	/* Telenova */
	DRIVER(compis)		/* 1985 Telenova Compis								*/

	/* Multitech */
	DRIVER(mpf1)		/* 1979 Multitech Micro Professor 1					*/
	DRIVER(mpf1b)		/* 1979 Multitech Micro Professor 1B				*/

	/* Telercas Oy */
TESTDRIVER(tmc1800)		/* 1977 Telmac 1800									*/
	DRIVER(tmc2000)		/* 1980 Telmac 2000									*/
TESTDRIVER(tmc2000e)	/* 1980 Telmac 2000E								*/
TESTDRIVER(tmc600s1)	/* 1982 Telmac TMC-600 (Series I)					*/
	DRIVER(tmc600s2)	/* 1982 Telmac TMC-600 (Series II)					*/

	/* MIT */
	DRIVER( tx0_64kw )	/* April 1956 MIT TX-0 (64kw RAM)					*/
	DRIVER( tx0_8kw )	/* 1962 MIT TX-0 (8kw RAM)							*/

	/* Luxor Datorer AB */
	DRIVER( abc80 )
	DRIVER( abc802 )
	DRIVER( abc800m )
	DRIVER( abc800c )
	DRIVER( abc806 )

	/* Be Incorporated */
	DRIVER( bebox )		/* BeBox Dual603-66									*/
	DRIVER( bebox2 )	/* BeBox Dual603-133								*/

	/* Tiger Electronics */
	DRIVER( gamecom )	/* Tiger Game.com									*/

	/* Thomson */
	DRIVER( t9000 )		/* 1980 Thomson T9000 (TO7 prototype)				*/
	DRIVER( to7 )		/* 1982 Thomson TO7									*/
	DRIVER( to770 )		/* 1984 Thomson TO7/70								*/
	DRIVER( to770a )	/* 198? Thomson TO7/70 arabic version				*/
	DRIVER( mo5 )		/* 1984 Thomson MO5									*/
	DRIVER( mo5e )		/* 1986 Thomson MO5E (export version)				*/
	DRIVER( to9 )		/* 1985 Thomson T09									*/
	DRIVER( to8 )		/* 1986 Thomson T08									*/
	DRIVER( to8d )		/* 1987 Thomson T08D								*/
	DRIVER( to9p )		/* 1986 Thomson T09+								*/
	DRIVER( mo6 )		/* 1986 Thomson MO6									*/
	DRIVER( mo5nr )		/* 1986 Thomson MO5 NR								*/
	DRIVER( pro128 )	/* 1986 Oliveti Prodest PC 128						*/

/****************Games*******************************************************/
	/* Computer Electronic */
	DRIVER( mk1 )		/* Chess Champion MK I								*/
	/* Quelle International */
	DRIVER( mk2 )		/* Chess Champion MK II								*/
	/* Hegener & Glaser Munich */
	DRIVER( mm4 )		/* Mephisto 4								*/
	/* Hegener & Glaser Munich */
	DRIVER( mm5 )		/* Mephisto 5.1 ROM								*/
	DRIVER( mm50 )		/* Mephisto 5.0 ROM								*/
	/* Hegener & Glaser Munich */
	DRIVER( rebel5 )		/* Mephisto 5								*/
	DRIVER( glasgow )		/* Glasgow						*/
	DRIVER( amsterd )		/* Amsterdam							*/
	DRIVER( dallas )		/* Dallas							*/
	DRIVER( dallas16 )		/* Dallas							*/
	DRIVER( dallas32 )		/* Dallas							*/
	DRIVER( roma32 )		/* Roma							*/
	/* Novag */
TESTDRIVER( ssystem3 )	/* Chess Champion Super System III / MK III			*/


#endif /* DRIVER_RECURSIVE */
