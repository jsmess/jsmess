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
#include "messdriv.c"

/* step 2: define the drivers[] array */
#undef DRIVER
#define DRIVER(NAME) &driver_##NAME,
const game_driver * const drivers[] =
{
#include "messdriv.c"
  0             /* end of array */
};

#else /* DRIVER_RECURSIVE */

/****************CONSOLES****************************************************/

// 128u4 NOTE - All drivers that start with "//128u4" were disabled as of that MAME release.
// Any driver with just // was disabled prior to that release. - HT


	/* 3DO */
	DRIVER( 3do )		/* 3DO consoles										*/
	DRIVER( 3do_pal )

	/* ATARI */
//128u4	DRIVER( a2600 )		/* Atari 2600										*/
//128u4	DRIVER( a2600p )	/* Atari 2600 PAL									*/
//128u4	DRIVER( a5200 )		/* Atari 5200										*/
//128u4	DRIVER( a5200a )	/* Atari 5200 alt									*/
//128u4	DRIVER( a7800 )		/* Atari 7800 NTSC									*/
//128u4	DRIVER( a7800p )	/* Atari 7800 PAL									*/
//128u4	DRIVER( lynx )		/* Atari Lynx Handheld								*/
//128u4	DRIVER( lynx2 )		/* Atari Lynx II Handheld redesigned, no additions	*/
//128u4	DRIVER( jaguar )	/* Atari Jaguar										*/
//128u4	DRIVER( atarist )	/* Atari ST											*/
//128u4	DRIVER( megast )	/* Atari Mega ST									*/
//	DRIVER( stacy )		/* Atari STacy										*/
//128u4	DRIVER( atariste )	/* Atari STe										*/
//128u4	DRIVER( stbook )	/* Atari STBook										*/
//128u4	DRIVER( megaste )	/* Atari Mega STe									*/
//	DRIVER( stpad )		/* Atari STPad (prototype)							*/
//	DRIVER( tt030 )		/* Atari TT030										*/
//	DRIVER( fx1 )		/* Atari FX-1 (prototype)							*/
//	DRIVER( falcon )	/* Atari Falcon030									*/
//	DRIVER( falcon40 )	/* Atari Falcon040 (prototype)						*/

	/* NINTENDO */
	DRIVER( nes )		/* Nintendo Entertainment System					*/
	DRIVER( nespal )	/* Nintendo Entertainment System					*/
	DRIVER( famicom )
	DRIVER( famitwin )	/* Sharp Famicom Twin System						*/
	DRIVER( gameboy )	/* Nintendo Game Boy Handheld						*/
	DRIVER( supergb )	/* Nintendo Super Game Boy SNES Cartridge			*/
	DRIVER( gbpocket )	/* Nintendo Game Boy Pocket Handheld				*/
	DRIVER( gbcolor )	/* Nintendo Game Boy Color Handheld					*/
	DRIVER( snes )		/* Nintendo Super Nintendo NTSC						*/
	DRIVER( snespal )	/* Nintendo Super Nintendo PAL						*/
	DRIVER( n64 )		/* Nintendo N64										*/
	DRIVER( pokemini )	/* Nintendo Pokemon Mini							*/

//128u4	DRIVER( megaduck )	/* Megaduck											*/

	/* SEGA */
//128u4	DRIVER( sg1000 )	/* Sega SG-1000 (Japan)								*/
//128u4	DRIVER( sg1000m2 )	/* Sega SG-1000 Mark II (Japan)						*/
//128u4	DRIVER( sc3000 )	/* Sega SC-3000 (Japan)								*/
//128u4	DRIVER( sc3000h )	/* Sega SC-3000H (Japan)							*/
//128u4	DRIVER( sf7000 )	/* Sega SC-3000 w/ SF-7000 (Japan)					*/
//	DRIVER( omv1000 )	/* Tsukuda Original Othello Multivision	FG-1000		*/
//	DRIVER( omv2000 )	/* Tsukuda Original Othello Multivision	FG-2000		*/

//128u4	DRIVER( gamegear )	/* Sega GameGear									*/
//128u4	DRIVER( gamegeaj )	/* Sega GameGear (Japan)							*/
//128u4	DRIVER( sms )		/* Sega Master System II (NTSC)						*/
//128u4	DRIVER( sms1 )		/* Sega Master System I (NTSC)						*/
//128u4	DRIVER( sms1pal )	/* Sega Master System I (PAL)						*/
//128u4	DRIVER( smspal )	/* Sega Master System II (PAL)						*/
//128u4	DRIVER( smsj )		/* Sega Master System (Japan) with FM Chip			*/
//128u4	DRIVER( sg1000m3 )	/* Sega SG-1000 Mark III (Japan)					*/
//128u4	DRIVER( sms2kr )	/* Samsung Gam*Boy II (Korea)						*/
//128u4	DRIVER( smssdisp )	/* Sega Master System Store Display Unit			*/

//128u4	DRIVER( megadrij )	/* 1988 Sega Mega Drive (Japan)						*/
//128u4	DRIVER( genesis )	/* 1989 Sega Genesis (USA)							*/
//128u4	DRIVER( gensvp )	/* 1993 Sega Genesis (USA w/SVP chip)				*/
//128u4	DRIVER( megadriv )	/* 1990 Sega Mega Drive (Europe)					*/
//128u4	DRIVER( pico )		/* 1994 Sega Pico (Europe)							*/
//128u4	DRIVER( picou )		/* 1994 Sega Pico (USA)								*/
//128u4	DRIVER( picoj )		/* 1993 Sega Pico (Japan)							*/

//128u4	DRIVER( saturnjp )	/* 1994 Sega Saturn (Japan)							*/
//128u4	DRIVER( saturn )	/* 1995 Sega Saturn (USA)							*/
//128u4	DRIVER( saturneu )	/* 1995 Sega Saturn (Europe)						*/
//128u4	DRIVER( vsaturn )	/* JVC V-Saturn										*/
//128u4	DRIVER( hisaturn )	/* Hitachi HiSaturn									*/

//128u4	DRIVER( dcjp )		/* 1998 Sega Dreamcast (Japan) */
//128u4	DRIVER( dc )		/* 1999 Sega Dreamcast (USA) */
//128u4	DRIVER( dceu )		/* 1999 Sega Dreamcast (Europe) */
//128u4	DRIVER( dcdev )		/* 1998 Sega HKT-0120 Sega Dreamcast Development Box */

	/* BALLY */
//128u4	DRIVER( astrocde )	/* Bally Astrocade									*/
//128u4	DRIVER( astrocdw )	/* Bally Astrocade (white case)						*/

	/* RCA */
//128u4	DRIVER( vip )		/* 1977 Cosmac VIP VP-711							*/
//128u4	DRIVER( vp111 )		/* 1977 Cosmac VIP VP-111							*/
//128u4	DRIVER( studio2 )	/* 1977 Studio II									*/
//	DRIVER( visicom )
//	DRIVER( mpt02s )
//	DRIVER( mpt02h )
//	DRIVER( mtc9016 )	/* 1978 Mustang 9016 Telespiel Computer				*/ 
//	DRIVER( shmc1200 )

	/* FAIRCHILD */
//128u4	DRIVER( channelf )	/* Fairchild Channel F VES - 1976					*/

	/* COLECO */
	DRIVER( coleco )	/* ColecoVision (Original BIOS)						*/
	DRIVER( colecoa )	/* ColecoVision (Thick Characters)					*/
	DRIVER( colecob )	/* Spectravideo SVI-603 Coleco Game Adapter			*/
	DRIVER( czz50 )		/* Bit Corporation Chuang Zao Zhe 50				*/
	DRIVER( dina )		/* Telegames Dina									*/
	DRIVER( prsarcde )	/* Telegames Personal Arcade						*/
	DRIVER( adam )		/* Coleco Adam										*/

	/* NEC */
//128u4	DRIVER( pce )		/* PC/Engine NEC 1987-1993							*/
//128u4	DRIVER( tg16 )		/* Turbo Grafix-16  NEC 1989-1993					*/
//128u4	DRIVER( sgx )		/* SuperGrafX NEC 1989								*/
//128u4	DRIVER( pcfx)		/* PC-FX NEC 1994									*/

	DRIVER( arcadia )	/* Emerson Arcadia 2001								*/
	DRIVER( vcg )		/* Palladium Video-Computer-Game					*/

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
//128u4	DRIVER( sfzch )		/* CPS Changer (Street Fighter ZERO)				*/

	/* MAGNAVOX */
	DRIVER( odyssey2 )	/* Magnavox Odyssey 2 - 1978-1983					*/

	/* Hartung, Watara, ...*/
//128u4	DRIVER( gmaster )	/* Hartung Gamemaster 								*/

	/* WATARA */
//128u4	DRIVER( svision )	/* Supervision Handheld								*/
//128u4	DRIVER( svisions )
//128u4	DRIVER( svisionp )
//128u4	DRIVER( svisionn )
//128u4	DRIVER( tvlinkp )

	/* INTERTON */
//128u4	DRIVER( vc4000 )	/* Interton vc4000									*/

	/* BANDAI */
//128u4	DRIVER( wswan )		/* Bandai WonderSwan Handheld						*/
//128u4	DRIVER( wscolor )	/* Bandai WonderSwan Color Handheld					*/

	/* EPOCH */
//128u4	DRIVER( gamepock )	/* Epoch Game Pocket Computer						*/

	/* KOEI */
//128u4	DRIVER( pasogo )	/* KOEI PasoGo										*/


/****************COMPUTERS***************************************************/
    /* ACORN */
	DRIVER( acrnsys1 )	/* 1979 Acorn System 1 (Microcomputer Kit)			*/
	DRIVER( atom )		/* 1979 Acorn Atom									*/
	DRIVER( atomeb )	/* 1979 Acorn Atom									*/
	DRIVER( bbca )		/* 1981 BBC Micro Model A							*/
	DRIVER( bbcb )		/* 1981 BBC Micro Model B							*/
	DRIVER( bbcbp )		/* 198? BBC Micro Model B+ 64K						*/
	DRIVER( bbcbp128 )	/* 198? BBC Micro Model B+ 128K						*/
	DRIVER( bbcm)		/* 198? BBC Master									*/
	DRIVER( bbcbc )		/* 1985 BBC Bridge Companion						*/
	DRIVER( electron )	/* 1983 Acorn Electron								*/
//	DRIVER( a310 )		/* 1988 Acorn Archimedes 310						*/

	/* CAMBRIDGE COMPUTERS */
	DRIVER( z88 )		/*													*/

	/* AMSTRAD */
	DRIVER( cpc464 )	/* Amstrad (Schneider in Germany) 1984				*/
	DRIVER( cpc664 )	/* Amstrad (Schneider in Germany) 1985				*/
	DRIVER( cpc6128 )	/* Amstrad (Schneider in Germany) 1985				*/
	DRIVER( cpc6128s )	/* Amstrad (Schneider in Germany) 1985				*/
	DRIVER( cpc6128f )	/* Amstrad (Schneider in Germany) 1985 (AZERTY)		*/
	DRIVER( cpc464p )	/* Amstrad CPC464  Plus - 1990						*/
	DRIVER( cpc6128p )	/* Amstrad CPC6128 Plus - 1990						*/
	DRIVER( gx4000 )	/* Amstrad GX4000 - 1990							*/
	DRIVER( kccomp )	/* VEB KC compact									*/
	DRIVER( al520ex )	/* Patisonic Aleste 520EX (1993)					*/
	DRIVER( pcw8256 )	/* 198? PCW8256										*/
	DRIVER( pcw8512 )	/* 198? PCW8512										*/
	DRIVER( pcw9256 )	/* 198? PCW9256										*/
	DRIVER( pcw9512 )	/* 198? PCW9512 (+)									*/
	DRIVER( pcw10 ) 	/* 198? PCW10										*/
	DRIVER( pcw16 )		/* 1995 PCW16										*/
	DRIVER( nc100 )		/* 19?? NC100										*/
	DRIVER( nc200 )		/* 19?? NC200										*/

	/* APPLE */
	DRIVER( apple1 )	/* Jul 1976 Apple 1									*/
	DRIVER( apple2 )	/* Apr 1977 Apple ][								*/
	DRIVER( apple2p )	/* Jun 1979 Apple ][+								*/
	DRIVER( apple2jp )	/* ??? ???? Apple ][j+								*/
	DRIVER( apple2e )	/* Jan 1983 Apple //128u4e								*/
	DRIVER( apple2ee )	/* Mar 1985 Apple //128u4e Enhanced						*/
	DRIVER( apple2ep )	/* Jan 1987 Apple //128u4e Platinum						*/
	DRIVER( apple2c )	/* Apr 1984 Apple //128u4c								*/
	DRIVER( apple2c0 )	/* ??? 1985 Apple //128u4c (3.5 ROM)						*/
	DRIVER( apple2c3 )	/* Sep 1986 Apple //128u4c (Original Mem. Exp.)			*/
	DRIVER( apple2c4 )	/* ??? 198? Apple //128u4c (rev 4)						*/
	DRIVER( apple2cp )	/* Sep 1988 Apple //128u4c+								*/
	DRIVER( apple2g0 )	/* Sep 1986 Apple IIgs ROM00						*/
	DRIVER( apple2g1 )	/* Sep 1987 Apple IIgs ROM01						*/
	DRIVER( apple2gs )	/* Aug 1989 Apple IIgs ROM03						*/
	DRIVER( apple3 )	/* May 1980 Apple //128u4/								*/
					/* Dec 1983 Apple //128u4/+								*/
	DRIVER( ace100 )	/* ??? 1982 Franklin Ace 100						*/
	DRIVER( laser128 )	/* ??? 1987 Laser 128								*/
	DRIVER( las128ex )	/* ??? 1987 Laser 128 EX							*/
	DRIVER( las3000 )	/* ??? 1983 Laser 3000								*/
	DRIVER( ivelultr )	/* Ivasim Ivel Ultra								*/

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

//128u4	DRIVER( a400 )		/* 1979 Atari 400									*/
//128u4	DRIVER( a400pal )	/* 1979 Atari 400 PAL								*/
//128u4	DRIVER( a800 )		/* 1979 Atari 800									*/
//128u4	DRIVER( a800pal )	/* 1979 Atari 800 PAL								*/
//128u4	DRIVER( a800xl )	/* 1983 Atari 800 XL								*/

	/* COMMODORE */
//128u4	DRIVER( kim1 )		/* Commodore (MOS) KIM-1 1975						*/
//128u4	DRIVER( sym1 )		/* Synertek SYM-1									*/
//128u4	DRIVER( aim65 )		/* Rockwell AIM65									*/

//128u4	DRIVER( pet2001 )	/* PET 2001											*/
//128u4	DRIVER( pet2001n )	/* PET 2001-N										*/
//128u4	DRIVER( pet2001b )	/* PET 2001-B										*/
//128u4	DRIVER( cbm30 )		/* CBM 30xx											*/
//128u4	DRIVER( cbm30b )	/* CBM 30xx (Business keyboard)						*/
//128u4	DRIVER( cbm30nor )	/* CBM 30xx (Norway, Business keyboard)				*/
//128u4	DRIVER( pet40on )	/* PET 40xx (Basic 4, no CRTC, Normal keyboard)		*/
//128u4	DRIVER( pet40ob )	/* PET 40xx (Basic 4, no CRTC, Business keyboard)	*/
//128u4	DRIVER( pet40n )	/* PET 40xx (Basic 4, CRTC 60Hz, 40 columns)		*/
//128u4	DRIVER( pet40b )	/* PET 40xx (Basic 4, CRTC 60Hz, 80 columns)		*/
//128u4	DRIVER( cbm40o )	/* CBM 40xx (Basic 4, no CRTC, Normal keyboard)		*/
//128u4	DRIVER( cbm40ob )	/* CBM 40xx (Basic 4, no CRTC, Business keyboard)	*/
//128u4	DRIVER( cbm40n )	/* CBM 40xx (Basic 4, CRTC 50Hz, 40 columns)		*/
//128u4	DRIVER( cbm40b )	/* CBM 40xx (Basic 4, CRTC 50Hz, 80 columns)		*/
//128u4	DRIVER( pet80 )		/* PET 80xx (Basic 4, CRTC 60Hz, 80 columns)		*/
//128u4	DRIVER( cbm80 )		/* CBM 80xx (Basic 4, CRTC 50Hz, 80 columns)		*/
//128u4	DRIVER( cbm80ger )	/* CBM 80xx (Germany, Basic 4, CRTC 50Hz, 80 cols)	*/
//128u4	DRIVER( cbm80hun )	/* CBM 80xx (Hungary, Basic 4, CRTC 50Hz, 80 cols)	*/
//128u4	DRIVER( cbm80swe )	/* CBM 80xx (Sweden, Basic 4, CRTC 50Hz, 80 cols)	*/
//128u4	DRIVER( cbm8296 )	/* CBM 8296 (Basic 4, CRTC 50Hz, 80 columns)		*/
//128u4	DRIVER( cbm8296d )	/* CBM 8296D										*/
//128u4	DRIVER( superpet )	/* SuperPET											*/
//128u4	DRIVER( sp9000 )	/* CBM SP9000 / MiniMainFrame 9000					*/
//128u4	DRIVER( mmf9000s )	/* MiniMainFrame 9000 (Sweden)						*/

//128u4	DRIVER( vic1001 )	/* Commodore VIC-1001 (Japan)						*/
//128u4	DRIVER( vic20 )		/* Commodore VIC 20 (NTSC)							*/
//128u4	DRIVER( vic20cr )	/* Commodore VIC 20CR (NTSC)						*/
//128u4	DRIVER( vic20i )	/* Commodore VIC 20 (NTSC, IEEE488 Interface)		*/
//128u4	DRIVER( vic20v )	/* Commodore VIC 20 (NTSC, VC1540)					*/
//128u4	DRIVER( vic20pal )	/* Commodore VIC 20 (PAL)							*/
//128u4	DRIVER( vic20crp )	/* Commodore VIC 20CR (PAL)							*/
//128u4	DRIVER( vic20plv )	/* Commodore VIC 20 (PAL, VC1541)					*/
//128u4	DRIVER( vc20 )		/* Commodore VIC 20 (PAL, Germany)					*/
//128u4	DRIVER( vc20v )		/* Commodore VIC 20 (PAL, VC1541)					*/
//128u4	DRIVER( vic20swe )	/* Commodore VIC 20 (Swedish Expanson Kit)			*/

//128u4	DRIVER( max )		/* Commodore Max Machine (Japan)					*/
//128u4	DRIVER( c64 )		/* Commodore 64 (NTSC)								*/
//128u4	DRIVER( c64pal )	/* Commodore 64 (PAL)								*/
//128u4	DRIVER( c64jpn )	/* Commodore 64 (Japan)								*/
//128u4	DRIVER( c64swe )	/* Commodore 64 (Sweden)							*/
//128u4	DRIVER( vic64s )	/* Commodore VIC 64S (Sweden)						*/
//128u4	DRIVER( pet64 )		/* Commodore PET 64									*/
//128u4	DRIVER( cbm4064 )	/* Commodore CBM 4064								*/
//128u4	DRIVER( edu64 )		/* Commodore Educator 64							*/
//128u4	DRIVER( sx64 )		/* Commodore SX-64 Executive Machine (PAL)			*/
//128u4	DRIVER( vip64 )		/* Commodore VIP64 (SX64, PAL, Swedish)				*/
//	DRIVER( dx64 )		/* Commodore DX-64 - Prototype						*/
//128u4	DRIVER( c64c )		/* Commodore 64C (NTSC)								*/
//128u4	DRIVER( c64cpal )	/* Commodore 64C (PAL)								*/
//128u4	DRIVER( c64g )		/* Commodore 64G (PAL)								*/
//128u4	DRIVER( c64gs )		/* Commodore 64 Games System						*/

//128u4	DRIVER( b500 )		/* Commodore B500									*/
//128u4	DRIVER( b128 )		/* Commodore B128									*/
//128u4	DRIVER( b256 )		/* Commodore B256									*/
//128u4	DRIVER( cbm610 )	/* Commodore CBM 610								*/
//128u4	DRIVER( cbm620 )	/* Commodore CBM 620								*/
//128u4	DRIVER( cbm620hu )	/* Commodore CBM 620 (Hungary)						*/
//128u4	DRIVER( b128hp )	/* Commodore B128HP									*/
//128u4	DRIVER( b256hp )	/* Commodore B256HP									*/
//128u4	DRIVER( bx256hp )	/* Commodore BX256HP								*/
//128u4	DRIVER( cbm710 )	/* Commodore CBM 710								*/
//128u4	DRIVER( cbm720 )	/* Commodore CBM 720								*/
//128u4	DRIVER( cbm720se )	/* Commodore CBM 720 (Sweden / Finland)				*/
//128u4	DRIVER( p500 )		/* Commodore P500 (proto, a.k.a. C128-40, PET-II)	*/

//128u4	DRIVER( c16 )		/* Commodore 16										*/
//128u4	DRIVER( c16c )		/* Commodore 16  c1551								*/
//128u4	DRIVER( c16v )		/* Commodore 16  vc1541								*/
//128u4	DRIVER( c16hun )	/* Commodore 16 Novotrade (Hungarian Character Set)	*/
//128u4	DRIVER( c116 )		/* Commodore 116									*/
//128u4	DRIVER( c116c )		/* Commodore 116  c1551								*/
//128u4	DRIVER( c116v )		/* Commodore 116  vc1541							*/
//128u4	DRIVER( plus4 )		/* Commodore +4										*/
//128u4	DRIVER( plus4c )	/* Commodore +4  c1551								*/
//128u4	DRIVER( plus4v )	/* Commodore +4  vc1541								*/
//128u4	DRIVER( c232 )		/* Commodore 232 - Prototype						*/
//128u4	DRIVER( c264 )		/* Commodore 264 - Prototype						*/
//128u4	DRIVER( c364 )		/* Commodore V364 - Prototype						*/

//128u4	DRIVER( c128 )		/* Commodore 128 - NTSC								*/
//128u4	DRIVER( c128cr )	/* Commodore 128CR - NTSC (proto?)					*/
//128u4	DRIVER( c128fin )	/* Commodore 128 - PAL (Finnish)					*/
//128u4	DRIVER( c128fra )	/* Commodore 128 - PAL (French)						*/
//128u4	DRIVER( c128ger )	/* Commodore 128 - PAL (German)						*/
//128u4	DRIVER( c128nor )	/* Commodore 128 - PAL (Norwegian)					*/
//128u4	DRIVER( c128d )		/* Commodore 128D - PAL								*/
//128u4	DRIVER( c128dpr )	/* Commodore 128D - NTSC (proto)					*/
//128u4	DRIVER( c128dcr )	/* Commodore 128DCR - NTSC							*/
//128u4	DRIVER( c128drde )	/* Commodore 128DCR - PAL (German)					*/
//128u4	DRIVER( c128drit )	/* Commodore 128DCR - PAL (Italian)					*/
//128u4	DRIVER( c128drsw )	/* Commodore 128DCR - PAL (Swedish)					*/

	DRIVER( a500n )		/* Commodore Amiga 500 - NTSC						*/
	DRIVER( a500p )		/* Commodore Amiga 500 - PAL						*/
	DRIVER( a1000n )	/* Commodore Amiga 1000 - NTSC						*/
	DRIVER( a1000p )	/* Commodore Amiga 1000 - PAL						*/
	DRIVER( cdtv )

//128u4	DRIVER( c65 )		/* 1991 C65 / C64DX (Prototype, NTSC)				*/
//128u4	DRIVER( c64dx )		/* 1991 C65 / C64DX (Prototype, German PAL)			*/

	/* IBM PC & Clones */
//128u4	DRIVER( ibm5150 )	/* 1981	IBM 5150									*/
//128u4	DRIVER( dgone )		/* 1984 Data General/One */
//128u4	DRIVER( pcmda )		/* 1987 PC with MDA									*/
//128u4	DRIVER( pcherc )	/* 1987 PC with Hercules (for testing hercules)		*/
//128u4	DRIVER( pc )		/* 1987 PC with CGA									*/
//128u4	DRIVER( bondwell )	/* 1985	Bondwell (CGA)								*/
//128u4	DRIVER( europc )	/* 1988	Schneider Euro PC (CGA or Hercules)			*/

	/* pc junior */
//128u4	DRIVER( ibmpcjr )	/* 1984 IBM PC Jr									*/
//128u4	DRIVER( t1000hx )	/* 1987 Tandy 1000HX (similiar to PCJr)				*/
//128u4	DRIVER( t1000sx )	/* 1987 Tandy 1000SX (similiar to PCJr)				*/

	/* xt */
//128u4	DRIVER( ibm5160 )	/* 1982	IBM XT 5160									*/
//128u4	DRIVER( ibm5162 )	/* 1986 IBM XT 5162 (XT w/80286)					*/
//128u4	DRIVER( pc200 )		/* 1988 Sinclair PC200								*/
//128u4	DRIVER( pc20 )		/* 1988 Amstrad PC20								*/
//128u4	DRIVER( ppc512 )	/* 1987 Amstrad PPC512								*/
//128u4	DRIVER( ppc640 )	/* 1987 Amstrad PPC640								*/
//128u4	DRIVER( pc1512 )	/* 1986 Amstrad PC1512 v1 (CGA compatible)			*/
//128u4	DRIVER( pc1512v2 )	/* 1986 Amstrad PC1512 v2 (CGA compatible)			*/
//128u4	DRIVER( pc1640 )	/* 1987 Amstrad PC1640 (EGA compatible)				*/

//128u4	DRIVER( xtvga )		/* 198? PC-XT (VGA, MF2 Keyboard)					*/

	/* at */
//128u4	DRIVER( ibm5170 )	/* 1984 IBM PC/AT 5170, original 6 MHz model		*/
//128u4	DRIVER( ibm5170a )	/* 1985	IBM PC/AT 5170, enhanced 8 MHz model		*/
//128u4	DRIVER( i8530286 )	/* 1988 IBM PS2 Model 30 286 (VGA)					*/
//128u4	DRIVER( at )		/* 1987 AMI Bios and Diagnostics					*/
//128u4	DRIVER( atvga )		/* 19?? AT VGA										*/
//128u4	DRIVER( neat )		/* 1989	New Enhanced AT chipset, AMI BIOS			*/
//128u4	DRIVER( at386 )		/* 19?? IBM AT 386									*/
//128u4	DRIVER( at486 )		/* 19?? IBM AT 486									*/
//128u4	DRIVER( at586 )		/* 19?? AT 586										*/

	/* OSBORNE */
	DRIVER( osborne1 )	/* 1981 Osborne-1									*/

	/* SINCLAIR RESEARCH */
	DRIVER( zx80 )		/* 1980 Sinclair ZX-80								*/
	DRIVER( zx81 )		/* 1981 Sinclair ZX-81								*/
	DRIVER( ts1000 )	/* 1982 Timex Sinclair 1000							*/
	DRIVER( pc8300 )	/* Your Computer - PC8300							*/
	DRIVER( pow3000 )	/* Creon Enterprises - Power 3000					*/
	DRIVER( lambda )	/* Lambda 8300										*/

	DRIVER( spectrum )/* 1982 ZX Spectrum									*/
	DRIVER( spec80k ) /* 1987 ZX Spectrum 80k							*/
	DRIVER( specide ) /* 1995 ZX Spectrum IDE							*/
	DRIVER( inves )		/* 1986 Inves Spectrum 48K+							*/
	DRIVER( tk90x )		/* 1985 TK90x Color Computer						*/
	DRIVER( tk95 )		/* 1986 TK95 Color Computer							*/
	DRIVER( tc2048 )	/* 198? TC2048										*/
	DRIVER( ts2068 )	/* 1983 TS2068										*/
	DRIVER( uk2086 )	/* 1986 UK2086										*/  
	
	DRIVER( spec128 )	  /* 1986 ZX Spectrum 128								*/
	DRIVER( specpls2 )	/* 1986 ZX Spectrum +2								*/
	DRIVER( specpl2a )	/* 1987 ZX Spectrum +2a								*/	
	DRIVER( specpls3 )	/* 1987 ZX Spectrum +3								*/
	DRIVER( specpl3e )	/* 2000 ZX Spectrum +3e								*/
	DRIVER( sp3e8bit )	/* 2002 ZX Spectrum +3e	8bit IDE			*/
	DRIVER( sp3ezcf )  	/* 2002 ZX Spectrum +3e	ZXCF   				*/
	DRIVER( sp3eata )  	/* 2002 ZX Spectrum +3e	ZXATASP  			*/	
	DRIVER( scorpion )
	DRIVER( pentagon )

	DRIVER( ql )		/* 1984 Sinclair QL	(UK)							*/
	DRIVER( ql_us )		/* 1984 Sinclair QL	(USA)							*/
	DRIVER( ql_es )		/* 1984 Sinclair QL	(Spain)							*/
	DRIVER( ql_fr )		/* 1984 Sinclair QL	(France)						*/
	DRIVER( ql_de )		/* 1984 Sinclair QL	(Germany)						*/
	DRIVER( ql_it )		/* 1984 Sinclair QL	(Italy)							*/
	DRIVER( ql_se )		/* 1984 Sinclair QL	(Sweden)						*/
	DRIVER( ql_dk )		/* 1984 Sinclair QL	(Denmark)						*/
	DRIVER( ql_gr )		/* 1984 Sinclair QL	(Greece)						*/
//	DRIVER( tonto )
//	DRIVER( megaopd )

	/* SHARP */
	DRIVER( pc1251 )	/* Pocket Computer 1251								*/
	DRIVER( trs80pc3 )	/* Tandy TRS80 PC-3									*/
	DRIVER( pc1401 )	/* Pocket Computer 1401								*/
	DRIVER( pc1402 )	/* Pocket Computer 1402								*/
	DRIVER( pc1350 )	/* Pocket Computer 1350								*/
	DRIVER( pc1403 )	/* Pocket Computer 1403								*/
	DRIVER( pc1403h )	/* Pocket Computer 1403H							*/

	DRIVER( mz700 )		/* 1982 Sharp MZ700									*/
	DRIVER( mz700j )	/* 1982 Sharp MZ700 Japan							*/
	DRIVER( mz800 )		/* 1982 Sharp MZ800									*/
	
	DRIVER( mz80kj )	/* 1979 Sharp MZ80K									*/
	DRIVER( mz80k )		/* 1979 Sharp MZ80K									*/

	DRIVER( x68000 )	/* Sharp X68000 (1987)								*/

	/* SILICON GRAPHICS */
//128u4	DRIVER( ip204415 )	/* IP20 Indigo2										*/
//128u4	DRIVER( ip225015 )
//128u4	DRIVER( ip224613 )
//128u4	DRIVER( ip244415 )

	/* TEXAS INSTRUMENTS */
//128u4	DRIVER( ti990_10 )	/* 1975 TI 990/10									*/
//128u4	DRIVER( ti990_4 )	/* 1976 TI 990/4									*/
//128u4	DRIVER( 990189 )	/* 1978 TM 990/189									*/
//128u4	DRIVER( 990189v )	/* 1980 TM 990/189 with Color Video Board			*/

//128u4	DRIVER( ti99_224 )	/* 1983 TI 99/2 (24kb ROMs)							*/
//128u4	DRIVER( ti99_232 )	/* 1983 TI 99/2 (32kb ROMs)							*/
//128u4	DRIVER( ti99_4 )	/* 1979 TI-99/4										*/
//128u4	DRIVER( ti99_4e )	/* 1980 TI-99/4 with 50Hz video						*/
//128u4	DRIVER( ti99_4a )	/* 1981 TI-99/4A									*/
//128u4	DRIVER( ti99_4ae )	/* 1981 TI-99/4A with 50Hz video					*/
//128u4	DRIVER( ti99_4ev)	/* 1994 TI-99/4A with EVPC video card				*/
//128u4	DRIVER( ti99_8 )	/* 1983 TI-99/8										*/
//128u4	DRIVER( ti99_8e )	/* 1983 TI-99/8 with 50Hz video						*/

	/* TI 99 clones */
//128u4	DRIVER( tutor)		/* 1983? Tomy Tutor									*/
//128u4	DRIVER( geneve )	/* 1987? Myarc Geneve 9640							*/
//128u4	DRIVER( genmod )	/* 199?? Myarc Geneve 9640							*/
//128u4	DRIVER( ti99_4p )	/* 1996 SNUG 99/4P (a.k.a. SGCPU)					*/

	DRIVER( avigo )		/*													*/

	/* TEXAS INSTRUMENTS CALCULATORS */
//128u4	DRIVER( ti81 )		/* 1990 TI-81 (Z80 2 MHz)							*/
//128u4	DRIVER( ti85 )		/* 1992 TI-85 (Z80 6 MHz)							*/
//128u4	DRIVER( ti82 )		/* 1993 TI-82 (Z80 6 MHz)							*/
//128u4	DRIVER( ti83 )		/* 1996 TI-83 (Z80 6 MHz)							*/
//128u4	DRIVER( ti86 )		/* 1997 TI-86 (Z80 6 MHz)							*/
//128u4	DRIVER( ti83p )		/* 1999 TI-83 Plus (Z80 6 MHz)						*/
//	DRIVER( ti83pse )	/* 2001 TI-83 Plus Silver Edition					*/
//	DRIVER( ti84p )		/* 2004 TI-84 Plus									*/
//	DRIVER( ti84pse )	/* 2004 TI-84 Plus Silver Edition					*/

	/* NEC */
//128u4	DRIVER( pc88srl )	/* PC-8801mkIISR(Low res display, VSYNC 15KHz)		*/
//128u4	DRIVER( pc88srh )	/* PC-8801mkIISR(High res display, VSYNC 24KHz)		*/

	/* CANTAB */
	DRIVER( jupiter )	/* Jupiter Ace										*/

	/* SORD */
//128u4	DRIVER( sordm5 )
//128u4	DRIVER( srdm5fd5 )

	/* APF Electronics Inc. */
	DRIVER( apfm1000 )
	DRIVER( apfimag )

	/* Tatung */
//128u4	DRIVER( einstein )
//128u4	DRIVER( einstei2 )

	/* INTELLIGENT SOFTWARE */
	DRIVER( ep128 )		/* Enterprise 128 k									*/

	/* NON LINEAR SYSTEMS */
//128u4	DRIVER( kaypro )	/* Kaypro 2X										*/

	/* VEB MIKROELEKTRONIK */
	/* KC compact is partial CPC compatible */
//128u4	DRIVER( kc85_4 )	/* VEB KC 85/4										*/
//128u4	DRIVER( kc85_3 )	/* VEB KC 85/3										*/
//128u4	DRIVER( kc85_4d )	/* VEB KC 85/4 with disk interface					*/

	/* MICROBEE SYSTEMS */
	DRIVER( mbee )		/* Microbee 16 Standard or Plus						*/
	DRIVER( mbeeic )	/* Microbee 32 IC									*/
	DRIVER( mbeepc )	/* Microbee 32 PC									*/
	DRIVER( mbeepc85 )	/* Microbee 32 PC85									*/
	DRIVER( mbee56 )	/* Microbee 56K (CP/M)								*/

	/* TANDY RADIO SHACK */
	DRIVER( trs80 )		/* TRS-80 Model I - Level I BASIC					*/
	DRIVER( trs80l2 )	/* TRS-80 Model I - Level II BASIC					*/
	DRIVER( sys80 )		/* EACA System 80									*/
	DRIVER( lnw80 )		/* LNW Research LNW-80								*/
	DRIVER( trs80m3 )	/* TRS-80 Model III - Radio Shack/Tandy				*/
	DRIVER( trs80m4 )
	DRIVER( ht1080z ) /* Hradstechnika Szvetkezet HT-1080Z */
	DRIVER( ht1080z2 ) /* Hradstechnika Szvetkezet HT-1080Z Series II */
	DRIVER( ht108064 ) /* Hradstechnika Szvetkezet HT-1080Z/64 */

//128u4	DRIVER( coco )		/* Color Computer									*/
//128u4	DRIVER( cocoe )		/* Color Computer (Extended BASIC 1.0)				*/
//128u4	DRIVER( coco2 )		/* Color Computer 2									*/
//128u4	DRIVER( coco2b )	/* Color Computer 2B (uses M6847T1 video chip)		*/
//128u4	DRIVER( coco3 )		/* Color Computer 3 (NTSC)							*/
//128u4	DRIVER( coco3p )	/* Color Computer 3 (PAL)							*/
//128u4	DRIVER( coco3h )	/* Hacked Color Computer 3 (6309)					*/
//128u4	DRIVER( dragon32 )	/* Dragon 32										*/
//128u4	DRIVER( dragon64 )	/* Dragon 64										*/
//128u4	DRIVER( d64plus )	/* Dragon 64 + Compusense Plus addon				*/
//128u4	DRIVER( dgnalpha )	/* Dragon Alpha										*/
//128u4	DRIVER( dgnbeta )	/* Dragon Beta										*/
//128u4	DRIVER( tanodr64 )	/* Tano Dragon 64 (NTSC)							*/
//128u4	DRIVER( cp400 )		/* Prologica CP400									*/
	DRIVER( mc10 )		/* MC-10											*/
//128u4	DRIVER( alice )		/* Matra & Hachette Ordinateur Alice				*/

	/* EACA */
//128u4	DRIVER( cgenie )	/* Colour Genie EG2000								*/
	/* system 80 trs80 compatible */

	/* VIDEO TECHNOLOGY */
	DRIVER( crvision )	/* 1981 creatiVision								*/
	DRIVER( fnvision )	/* 1983 FunVision									*/
	DRIVER( laser110 )	/* 1983 Laser 110									*/
	DRIVER( las110de )	/* 1983 Sanyo Laser 110 (Germany)					*/
	DRIVER( laser200 )	/* 1983 Laser 200 (color version of 110)			*/
//	DRIVER( vz200de )	/* 1983 VZ-200 (Germany)							*/
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
//128u4	DRIVER( microtan )	/* 1979 Microtan 65									*/

//128u4	DRIVER( oric1 )		/* 1983 Oric 1										*/
//128u4	DRIVER( orica )		/* 1984 Oric Atmos									*/
//128u4	DRIVER( prav8d )	/* 1985 Pravetz 8D									*/
//128u4	DRIVER( prav8dd )	/* 1989 Pravetz 8D (Disk ROM)						*/
//128u4	DRIVER( telstrat )	/* ??? Oric Telestrat/Stratos						*/

	/* PHILIPS */
	DRIVER( p2000t )	/* 1980 P2000T										*/
	DRIVER( p2000m )	/* 1980 P2000M										*/
	DRIVER( videopac )	/* 1979 Videopac G7000/C52							*/
	DRIVER( g7400 )		/* 1983 Videopac Plus G7400							*/

	/* COMPUKIT */
//128u4	DRIVER( uk101 )		/* 1979 UK101										*/

	/* OHIO SCIENTIFIC */
//128u4	DRIVER( sb2m600b )	/* 1979 Superboard II								*/

	/* ASCII & MICROSOFT */
	DRIVER( msx )		/* 1983 MSX 										*/
	DRIVER( ax170 )
	DRIVER( canonv10 )
	DRIVER( canonv20 )
	DRIVER( dpc100 )	/* 1984 MSX Korea									*/
	DRIVER( dpc180 )	/* 1984 MSX Korea									*/
	DRIVER( dpc200 )	/* 1984 MSX Korea									*/
	DRIVER( gsfc200 )
	DRIVER( expert10 )	/* 1983 MSX Brazil									*/
	DRIVER( expert11 )	/* 1984 MSX Brazil									*/
	DRIVER( expert13 )	/* 1984 MSX Brazil									*/
	DRIVER( expertdp )	/* 1985 MSX Brazil									*/
	DRIVER( expertpl )	/* 1984 MSX Brazil									*/
	DRIVER( jvchc7gb )
	DRIVER( mlf80 )
	DRIVER( mlfx1 )
	DRIVER( cf1200 )	/* 1984 MSX Japan									*/
	DRIVER( cf2000 )	/* 1983 MSX Japan									*/
	DRIVER( cf2700 )	/* 1984 MSX Japan									*/
	DRIVER( cf3000 )	/* 1984 MSX Japan									*/
	DRIVER( cf3300 )	/* 1985 MSX Japan									*/
	DRIVER( fs1300 )	/* 1985 MSX Japan									*/
	DRIVER( fs4000 )	/* 1985 MSX Japan									*/
	DRIVER( nms801 )
	DRIVER( vg802000 )
	DRIVER( vg802020 )	/* 1985 MSX											*/
	DRIVER( piopx7 )	
	DRIVER( mpc100 )
	DRIVER( hotbit11 )	/* 1985 MSX Brazil									*/
	DRIVER( hotbit12 )	/* 1985 MSX	Brazil									*/
	DRIVER( hotbi13b )	/* 1985 MSX	Brazil									*/
	DRIVER( hotbi13p )	/* 1985 MSX	Brazil									*/
	DRIVER( hotbit20 )	/* 1986 MSX2 Brazil									*/
	DRIVER( hb201 )		/* 1985 MSX Japan									*/
	DRIVER( hb201p )	/* 1985 MSX											*/
	DRIVER( hb501p )	/* 1984 MSX											*/
	DRIVER( hb55d )		/* 1983 MSX Germany									*/
	DRIVER( hb55p )		/* 1983 MSX											*/
	DRIVER( hb75d )		/* 1983 MSX Germany									*/
	DRIVER( hb75p )		/* 1983 MSX											*/
	DRIVER( svi728 )	/* 1985 MSX											*/
	DRIVER( svi738 )	/* 1985 MSX											*/
	DRIVER( svi738sw )	/* 1985 MSX											*/
	DRIVER( tadpc200 )
	DRIVER( tadpc20a )
	DRIVER( hx10 )		/* 1984 MSX											*/
	DRIVER( hx10s )		/* 1984 MSX											*/
	DRIVER( hx20 )		/* 1984 MSX											*/
	DRIVER( cx5m )
	DRIVER( cx5m128 )
	DRIVER( cx5m2 )	
	DRIVER( yis303 )
	DRIVER( yis503 )
	DRIVER( yis503ii )
	DRIVER( yis503m )
	DRIVER( yc64 )
	
	
	DRIVER( msx2 )		/* 1985 MSX2										*/
	DRIVER( ax350 )
	DRIVER( ax370 )
	DRIVER( hbf9p )		/* 1985 MSX2										*/
	DRIVER( hbf9s )		/* 1985 MSX2										*/
	DRIVER( hbf500p )	/* 1985 MSX2										*/
	DRIVER( hbf700d )	/* 1985 MSX2 Germany								*/
	DRIVER( hbf700f )	/* 1985 MSX2 						*/
	DRIVER( hbf700p )	/* 1985 MSX2										*/
	DRIVER( hbf700s )	/* 1985 MSX2 Spain									*/
	DRIVER( hbg900ap )	/* 1986 MSX2										*/
	DRIVER( hbg900p )	/* 1986 MSX2										*/
	DRIVER( nms8220 )	/* 1986 MSX2										*/
	DRIVER( nms8220a )	/* 1986 MSX2										*/
	DRIVER( vg8230 )	/* 1986 MSX2										*/
	DRIVER( vg8235 )	/* 1986 MSX2										*/
	DRIVER( vg8235f)	/* 1986 MSX2										*/
	DRIVER( vg8240)	/* 1986 MSX2										*/
	DRIVER( nms8245 )	/* 1986 MSX2										*/
	DRIVER( nms8245f)	/* 1986 MSX2										*/
	DRIVER( nms8250 )	/* 1986 MSX2										*/
	DRIVER( nms8255 )	/* 1986 MSX2										*/
	DRIVER( nms8280 )	/* 1986 MSX2										*/
	DRIVER( nms8280g)	/* 1986 MSX2										*/
	DRIVER( tpc310)	  /* 1986 MSX2										*/
	DRIVER( hx23)	    /* 1986 MSX2										*/
	DRIVER( hx23f)	  /* 1986 MSX2										*/
	DRIVER( cx7m)	    /* 1986 MSX2										*/
	DRIVER( cx7m128)	/* 1986 MSX2										*/
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
	DRIVER( cpc300e )	/* 1986 MSX2 Korea									*/
	DRIVER( cpc400 )	/* 1986 MSX2 Korea									*/
	DRIVER( cpc400s )	/* 1986 MSX2 Korea									*/
	DRIVER( expert20 )/* 1986 MSX2 Korea									*/

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
//128u4	DRIVER( nascom1 )	/* 1978 Nascom 1									*/
//128u4	DRIVER( nascom2 )	/* 1979 Nascom 2									*/

	/* MILES GORDON TECHNOLOGY */
//128u4	DRIVER( samcoupe )	/* 1989 Sam Coupe									*/

	/* MOTOROLA */
	DRIVER( mekd2 )		/* 1977 Motorola Evaluation Kit						*/

	/* DEC */
//128u4	DRIVER( pdp1 )		/* 1961 DEC PDP1									*/

	/* MEMOTECH */
//128u4	DRIVER( mtx512 )	/* 1983 Memotech MTX 512							*/
//128u4	DRIVER( mtx500 )	/* 1983 Memotech MTX 500							*/
//128u4	DRIVER( rs128 )		/* 1984 Memotech RS 128								*/

	/* MATTEL */
//128u4	DRIVER( intvkbd )	/* 1981 - Mattel Intellivision Keyboard Component	*/
						/* (Test marketed, later recalled)					*/
	DRIVER( aquarius )	/* 1983 Aquarius									*/
//	DRIVER( aquariu2 )	/* 1984 Aquarius II									*/

	/*EXIDY INC */
	DRIVER( exidy )		/* Sorcerer											*/
	DRIVER( exidyd )	/* Sorcerer (cassette only)								*/

	/* GALAKSIJA */
	DRIVER( galaxy )
	DRIVER( galaxyp )

	/* Lviv/L'vov */
	DRIVER( lviv )		/* Lviv/L'vov										*/

	/* Tesla */
//128u4	DRIVER( pmd851 )	/* PMD-85.1											*/
//128u4	DRIVER( pmd852 )	/* PMD-85.2											*/
//128u4	DRIVER( pmd852a )	/* PMD-85.2A										*/
//128u4	DRIVER( pmd852b )	/* PMD-85.2B										*/
//128u4	DRIVER( pmd853 )	/* PMD-85.3											*/

	/* Didaktik */
//128u4	DRIVER( alfa )		/* Alfa (PMD-85.1 clone)							*/

	/* Statny */
//128u4	DRIVER( mato )		/* Mato (PMD-85.2 clone)							*/
	
	/* Zbrojovka Brno */
//128u4	DRIVER( c2717 )		/* Consul 2717 (PMD-85.2 clone)				*/
	
	/* Microkey */
//128u4	DRIVER( primoa32 )	/* Primo A-32										*/
//128u4	DRIVER( primoa48 )	/* Primo A-48										*/
//128u4	DRIVER( primoa64 )	/* Primo A-64										*/
//128u4	DRIVER( primob32 )	/* Primo B-32										*/
//128u4	DRIVER( primob48 )	/* Primo B-48										*/
//128u4	DRIVER( primob64 )	/* Primo B-64										*/
//128u4	DRIVER( primoc64 )	/* Primo C-64										*/

	/* Team Concepts */
	/* CPU not known, else should be easy, look into drivers/comquest.c */
//128u4	DRIVER( comquest )	/* Comquest Plus German								*/

	/* Hewlett Packard */
//128u4	DRIVER( hp48s )		/* HP48 S */
//128u4	DRIVER( hp48sx )	/* HP48 SX */
//128u4	DRIVER( hp48g )		/* HP48 G */
//128u4	DRIVER( hp48gx )	/* HP48 GX */

	/* SpectraVideo */
//128u4	DRIVER( svi318 )	/* SVI-318 (PAL)									*/
//128u4	DRIVER( svi318n )	/* SVI-318 (NTSC)									*/
//128u4	DRIVER( svi328 )	/* SVI-328 (PAL)									*/
//128u4	DRIVER( svi328n )	/* SVI-328 (NTSC)									*/
//128u4	DRIVER( sv328p80 )	/* SVI-328 (PAL) + SVI-806 80 column card			*/
//128u4	DRIVER( sv328n80 )	/* SVI-328 (NTSC) + SVI-806 80 column card			*/

	/* Andrew Donald Booth (this is the name of the designer, not a company) */
//	DRIVER( apexc53 )	/* 1951(?) APEXC: All-Purpose Electronic X-ray Computer */
	DRIVER( apexc )		/* 1955(?) APEXC: All-Purpose Electronic X-ray Computer */

	/* Sony */
	DRIVER( psj )		/* 1994 Sony PlayStation (Japan)					*/
	DRIVER( psu )		/* 1995 Sony PlayStation (USA)						*/
	DRIVER( pse )		/* 1995 Sony PlayStation (Europe)					*/
	DRIVER( psa )		/* 1995 Sony PlayStation (Asia-Pacific)				*/

	/* Corvus */
//128u4	DRIVER(concept)		/* 1982 Corvus Concept								*/

	/* DAI */
	DRIVER(dai)			/* DAI												*/

	/* Telenova */
//128u4	DRIVER(compis)		/* 1985 Telenova Compis								*/

	/* Multitech */
//128u4	DRIVER(mpf1)		/* 1979 Multitech Micro Professor 1					*/
//128u4	DRIVER(mpf1b)		/* 1979 Multitech Micro Professor 1B				*/

	/* Telercas Oy */
//	DRIVER(tmc1800)
//128u4	DRIVER(tmc2000)
//128u4	DRIVER(tmc2000e)
//	DRIVER(tmc600s1)
//128u4	DRIVER(tmc600s2)

	/* OSCOM Oy */
//	DRIVER(osc1000b)
//128u4	DRIVER(oscnano)

	/* MIT */
//128u4	DRIVER( tx0_64kw )	/* April 1956 MIT TX-0 (64kw RAM)					*/
//128u4	DRIVER( tx0_8kw )	/* 1962 MIT TX-0 (8kw RAM)							*/

	/* Luxor Datorer AB */
	DRIVER( abc80 )
	DRIVER( abc802 )
	DRIVER( abc800m )
	DRIVER( abc800c )
	DRIVER( abc806 )

	/* Be Incorporated */
//128u4	DRIVER( bebox )		/* BeBox Dual603-66									*/
//128u4	DRIVER( bebox2 )	/* BeBox Dual603-133								*/

	/* Tiger Electronics */
//128u4	DRIVER( gamecom )	/* Tiger Game.com									*/

	/* Thomson */
//128u4	DRIVER( t9000 )		/* 1980 Thomson T9000 (TO7 prototype)				*/
//128u4	DRIVER( to7 )		/* 1982 Thomson TO7									*/
//128u4	DRIVER( to770 )		/* 1984 Thomson TO7/70								*/
//128u4	DRIVER( to770a )	/* 198? Thomson TO7/70 arabic version				*/
//128u4	DRIVER( mo5 )		/* 1984 Thomson MO5									*/
//128u4	DRIVER( mo5e )		/* 1986 Thomson MO5E (export version)				*/
//128u4	DRIVER( to9 )		/* 1985 Thomson T09									*/
//128u4	DRIVER( to8 )		/* 1986 Thomson T08									*/
//128u4	DRIVER( to8d )		/* 1987 Thomson T08D								*/
//128u4	DRIVER( to9p )		/* 1986 Thomson T09+								*/
//128u4	DRIVER( mo6 )		/* 1986 Thomson MO6									*/
//128u4	DRIVER( mo5nr )		/* 1986 Thomson MO5 NR								*/
//128u4	DRIVER( pro128 )	/* 1986 Olivetti Prodest PC 128						*/

	/* Cybiko, Inc. */
//128u4	DRIVER( cybikov1 )	/* Cybiko Wireless Intertainment System - Classic V1 */
//128u4	DRIVER( cybikov2 )	/* Cybiko Wireless Intertainment System - Classic V2 */
//128u4	DRIVER( cybikoxt )	/* Cybiko Wireless Intertainment System - Xtreme     */

	/* Dick Smith */
	DRIVER( super80 )
	DRIVER( super80d )
	DRIVER( super80e )
	DRIVER( super80m )
	DRIVER( super80r )
	DRIVER( super80v )

	/* Galeb */
	DRIVER( galeb )

	/* Orao */
	DRIVER( orao )
	DRIVER( orao103 )

	/* UT-88 */
	DRIVER( ut88 )
	DRIVER( ut88mini )

	/* Mikro-80 */
	DRIVER( mikro80 )
	DRIVER( radio99 )

	/* Specialist */
	DRIVER( special )
	DRIVER( specialp )
	DRIVER( lik )
	DRIVER( erik )
	DRIVER( specimx )

	/* Orion */
	DRIVER( orion128 )
	DRIVER( orionms )
	DRIVER( orionz80 )
	DRIVER( orionide )
	DRIVER( orionzms )
	DRIVER( orionidm )
	DRIVER( orionpro )

	/* BK */
	DRIVER( bk0010 )
	DRIVER( bk001001 )
	DRIVER( bk0010fd )

	/* Bashkiria-2M */
	DRIVER( b2m )
	DRIVER( b2mrom )

	/* Radio-86RK */
	DRIVER( radio86 )
	DRIVER( radio16 )
	DRIVER( radio4k )
	DRIVER( rk7007 )
	DRIVER( rk700716 )
	DRIVER( radiorom )
	DRIVER( radioram )
	DRIVER( spektr01 )
	DRIVER( apogee )
	DRIVER( mikrosha )
	DRIVER( partner )

  	/* Homelab */  
	DRIVER (homelab2)
	DRIVER (homelab3)
	DRIVER (homelab4)
	
	/* Irisha */
	DRIVER (irisha)
	
	/* PK-8020 */
	DRIVER (korvet)

	/* Vector-06c */
	DRIVER (vector06)

	/* Robotron 1715 */
	DRIVER (rt1715)
	DRIVER (rt1715w)

	/* PP-01 */
	DRIVER (pp01)

	/* Ondra */
	DRIVER (ondrat)
	DRIVER (ondrav)

	/* SAPI-1 */
	DRIVER (sapi1)

  /* Spectrum clones */
  	
  /* ICE-Felix */
	DRIVER( hc85 )    /* 1985 HC-85										  */
	DRIVER( hc90 )    /* 1990 HC-90										  */
	DRIVER( hc91 )    /* 1991 HC-91										  */
	DRIVER( hc128 )   /* 1991 HC-128									  */
	DRIVER( hc2000 )  /* 1992 HC-2000									  */

	DRIVER( cip03 )   /* 1988 CIP-03										  */
	DRIVER( jet )     /* 1990 JET										  */
  
  /* Didaktik Skalica */
	DRIVER( dgama87)  /* 1987 Didaktik Gama 87          */
	DRIVER( dgama88)  /* 1988 Didaktik Gama 88          */
	DRIVER( dgama89)  /* 1989 Didaktik Gama 89          */
	DRIVER( didakt90) /* 1990 Didaktik Skalica 90       */
	DRIVER( didakm91) /* 1991 Didaktik M 91             */
	DRIVER( didaktk)  /* 1992 Didaktik Kompakt          */
	DRIVER( didakm93) /* 1993 Didaktik M 93             */
	
	DRIVER( mistrum ) /* 1988 Mistrum										*/
	
	/* Russian clones */
	DRIVER( blitz )     /* 1990 Blic                     */
	DRIVER( byte )      /* 1990 Byte                     */
	DRIVER( orizon )    /* 199? Orizon-Micro             */
	DRIVER( quorum48 )  /* 1993 Kvorum 48K               */
	DRIVER( magic6 )    /* 1993 Magic 6                  */
	DRIVER( compani1 )  /* 1990 Kompanion 1              */

	/* Kramer */
	DRIVER(kramermc) /* 1987 Kramer MC				*/

	/* Ei Nis */
	DRIVER(pecom64)
		
	/* Bondwell */
//128u4	DRIVER( bw2 )

	/* Exeltel */
//128u4	DRIVER( exl100 )
//	DRIVER( exeltel )

	/* Comx World Operations Ltd */
//128u4	DRIVER( comx35p )
//128u4	DRIVER( comx35n )
//	DRIVER( comxpl80 )

	/* Grundy Business Systems Ltd */
//128u4	DRIVER( newbrain )
//128u4	DRIVER( newbraim )
//128u4	DRIVER( newbraia )
//128u4	DRIVER( newbraiv )

	/* Votrax */
//	DRIVER( votrtnt ) /* Votrax Type-N-Talk */
	DRIVER( votrpss ) /* Votrax Personal Speech System */

/****************Games*******************************************************/
	/* Computer Electronic */
//128u4	DRIVER( mk1 )		/* Chess Champion MK I								*/
	/* Quelle International */
	DRIVER( mk2 )		/* Chess Champion MK II								*/

	/* Novag */
//128u4	DRIVER( ssystem3 )	/* Chess Champion Super System III / MK III			*/

	/* Hegener & Glaser Munich */
//	DRIVER( mephisto )	/* Mephisto											*/
//128u4	DRIVER( mm4 )		/* Mephisto 4										*/
//128u4	DRIVER( mm5 )		/* Mephisto 5.1 ROM									*/
//128u4	DRIVER( mm50 )		/* Mephisto 5.0 ROM									*/
//128u4	DRIVER( rebel5 )		/* Mephisto 5									*/
//128u4	DRIVER( glasgow )		/* Glasgow										*/
//128u4	DRIVER( amsterd )		/* Amsterdam									*/
//128u4	DRIVER( dallas )		/* Dallas										*/
//128u4	DRIVER( dallas16 )		/* Dallas										*/
//128u4	DRIVER( dallas32 )		/* Dallas										*/
//128u4	DRIVER( roma )			/* Roma											*/
//128u4	DRIVER( roma32 )		/* Roma											*/

/*********** Misc ***********************************************************/
//128u4	DRIVER( ex800 )


#endif /* DRIVER_RECURSIVE */
