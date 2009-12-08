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

    /* 3DO */
    DRIVER( 3do )       /* 3DO consoles                                     */
    DRIVER( 3do_pal )

    /* Atari */
    DRIVER( a2600 )     /* Atari 2600                                       */
    DRIVER( a2600p )    /* Atari 2600 PAL                                   */
    DRIVER( a5200 )     /* Atari 5200                                       */
    DRIVER( a7800 )     /* Atari 7800 NTSC                                  */
    DRIVER( a7800p )    /* Atari 7800 PAL                                   */
    DRIVER( lynx )      /* Atari Lynx Handheld                              */
    DRIVER( lynx2 )     /* Atari Lynx II Handheld redesigned, no additions  */
    DRIVER( jaguar )    /* Atari Jaguar                                     */
    DRIVER( jaguarcd )  /* Atari Jaguar CD                                  */

    /* Nintendo */
    DRIVER( nes )       /* Nintendo Entertainment System                    */
    DRIVER( nespal )    /* Nintendo Entertainment System                    */
    DRIVER( m82 )       /* Nintendo M82 Display Unit                        */
    DRIVER( famicom )
    DRIVER( famitwin )  /* Sharp Famicom Twin System                        */
    DRIVER( drpcjr )    /* Bung Doctor PC Jr                                */
    DRIVER( dendy )     /* Dendy Classic russian famiclone                  */
    DRIVER( gameboy )   /* Nintendo Game Boy Handheld                       */
    DRIVER( supergb )   /* Nintendo Super Game Boy SNES Cartridge           */
    DRIVER( gbpocket )  /* Nintendo Game Boy Pocket Handheld                */
    DRIVER( gblight )   /* Nintendo Game Boy Light Handheld             */
    DRIVER( gbcolor )   /* Nintendo Game Boy Color Handheld                 */
    DRIVER( snes )      /* Nintendo Super Nintendo NTSC                     */
    DRIVER( snessfx )	/* Nintendo Super Nintendo NTSC, SuperFX			*/
    DRIVER( snespal )   /* Nintendo Super Nintendo PAL                      */
    DRIVER( snespsfx )	/* Nintendo Super Nintendo PAL, SuperFX			*/
    DRIVER( sfcbox )    /* Nintendo Super Famicom Box                       */
    DRIVER( n64 )       /* Nintendo N64                                     */
    DRIVER( pokemini )  /* Nintendo Pokemon Mini                            */
    DRIVER( gba )	/* Nintendo Game Boy Advance Handheld               */

    DRIVER( megaduck )  /* Megaduck                                         */

    /* SEGA */
    DRIVER( sg1000 )    /* Sega SG-1000 (Japan)                             */
    DRIVER( sg1000m2 )  /* Sega SG-1000 Mark II (Japan)                     */
    DRIVER( sc3000 )    /* Sega SC-3000 (Japan)                             */
    DRIVER( sc3000h )   /* Sega SC-3000H (Japan)                            */
    DRIVER( sf7000 )    /* Sega SC-3000 w/ SF-7000 (Japan)                  */
//  DRIVER( omv1000 )   /* Tsukuda Original Othello Multivision FG-1000     */
//  DRIVER( omv2000 )   /* Tsukuda Original Othello Multivision FG-2000     */

    DRIVER( gamegear )  /* Sega GameGear                                    */
    DRIVER( gamegeaj )  /* Sega GameGear (Japan)                            */
    DRIVER( sms )       /* Sega Master System II (NTSC)                     */
    DRIVER( sms1 )      /* Sega Master System I (NTSC)                      */
    DRIVER( sms1pal )   /* Sega Master System I (PAL)                       */
    DRIVER( smspal )    /* Sega Master System II (PAL)                      */
    DRIVER( smsj )      /* Sega Master System (Japan) with FM Chip          */
    DRIVER( sg1000m3 )  /* Sega SG-1000 Mark III (Japan)                    */
    DRIVER( sms2kr )    /* Samsung Gam*Boy II (Korea)                       */
    DRIVER( smssdisp )  /* Sega Master System Store Display Unit            */

    DRIVER( megadrij )  /* 1988 Sega Mega Drive (Japan)                     */
    DRIVER( genesis )   /* 1989 Sega Genesis (USA)                          */
    DRIVER( gensvp )    /* 1993 Sega Genesis (USA w/SVP chip)               */
    DRIVER( megadriv )  /* 1990 Sega Mega Drive (Europe)                    */
    DRIVER( pico )      /* 1994 Sega Pico (Europe)                          */
    DRIVER( picou )     /* 1994 Sega Pico (USA)                             */
    DRIVER( picoj )     /* 1993 Sega Pico (Japan)                           */
    DRIVER( segacd )    /* 1992 Sega Sega CD (USA)                          */
    DRIVER( megacd )    /* 1993 Sega Mega-CD (Europe)                       */
    DRIVER( megacdj )   /* 1991 Sega Mega-CD (Japan)                        */
    DRIVER( segacd2 )   /* 1993 Sega Sega CD 2 (USA)                        */
    DRIVER( megacd2 )   /* 1993 Sega Mega-CD 2 (Europe)                     */
    DRIVER( megacd2j )  /* 1993 Sega Mega-CD 2 (Japan)                      */
    DRIVER( laseract )  /* 1993 Pioneer LaserActive (USA)                   */
    DRIVER( laseractj ) /* 1993 Pioneer LaserActive (Japan)                 */
    DRIVER( xeye )      /* 1993 JVC X'eye (USA)                             */
    DRIVER( wmega )     /* 1993 Sega Wondermega (Japan)                     */
    DRIVER( cdx )       /* 1994 Sega CDX (USA)                              */
    DRIVER( multmega )  /* 1994 Sega Multi-Mega (Europe)                    */
    DRIVER( 32x )       /* 1994 Sega 32X (USA)                              */
    DRIVER( 32x_scd )   /* 1994 Sega Sega CD (USA w/32X addon)              */

    DRIVER( saturnjp )  /* 1994 Sega Saturn (Japan)                         */
    DRIVER( saturn )    /* 1995 Sega Saturn (USA)                           */
    DRIVER( saturneu )  /* 1995 Sega Saturn (Europe)                        */
    DRIVER( vsaturn )   /* JVC V-Saturn                                     */
    DRIVER( hisaturn )  /* Hitachi HiSaturn                                 */

    DRIVER( dcjp )      /* 1998 Sega Dreamcast (Japan) */
    DRIVER( dc )        /* 1999 Sega Dreamcast (USA) */
    DRIVER( dceu )      /* 1999 Sega Dreamcast (Europe) */
    DRIVER( dcdev )     /* 1998 Sega HKT-0120 Sega Dreamcast Development Box */

    /* Sony */
    DRIVER( psj )       /* 1994 Sony PlayStation (Japan)                    */
    DRIVER( psu )       /* 1995 Sony PlayStation (USA)                      */
    DRIVER( pse )       /* 1995 Sony PlayStation (Europe)                   */
    DRIVER( psa )       /* 1995 Sony PlayStation (Asia-Pacific)             */
    DRIVER( pockstat )  /* 1999 Sony PocketStation                          */

    /* Bally */
    DRIVER( astrocde )  /* Bally Astrocade                                  */
    DRIVER( astrocdw )  /* Bally Astrocade (white case)                     */

    /* RCA */
    DRIVER( vip )       /* 1977 Cosmac VIP VP-711                           */
    DRIVER( vp111 )     /* 1977 Cosmac VIP VP-111                           */
    DRIVER( studio2 )   /* 1977 Studio II                                   */
    DRIVER( visicom )
//  DRIVER( mpt02s )
//  DRIVER( mpt02h )
//  DRIVER( mtc9016 )   /* 1978 Mustang 9016 Telespiel Computer             */
//  DRIVER( shmc1200 )

    /* Ensoniq */
    DRIVER( esq1 )	/* 1986 ESQ-1 Digital Wave Synthesizer */

    /* Fairchild */
    DRIVER( channelf )  /* Fairchild Channel F VES - 1976                   */
    DRIVER( sabavdpl )  /* SABA Videoplay - 1977 (PAL)                      */
    DRIVER( luxorves )  /* Luxor Video Entertainment System (PAL)           */
    DRIVER( channlf2 )  /* Fairchild Channel F II - 1978                    */
    DRIVER( sabavpl2 )  /* SABA Videoplay 2 (PAL)                           */
    DRIVER( luxorvec )  /* Luxor Video Entertainment Computer (PAL)         */
    DRIVER( itttelma )  /* ITT Tele-Match Processor (PAL)                   */
    DRIVER( ingtelma )  /* Ingelen Tele-Match Processor (PAL)               */

    /* Coleco */
    DRIVER( coleco )    /* ColecoVision (Original BIOS)                     */
    DRIVER( colecoa )   /* ColecoVision (Thick Characters)                  */
    DRIVER( colecob )   /* Spectravideo SVI-603 Coleco Game Adapter         */
    DRIVER( czz50 )     /* Bit Corporation Chuang Zao Zhe 50                */
    DRIVER( dina )      /* Telegames Dina                                   */
    DRIVER( prsarcde )  /* Telegames Personal Arcade                        */
    DRIVER( adam )      /* Coleco Adam                                      */

    /* NEC */
    DRIVER( pce )       /* PC/Engine NEC 1987-1993                          */
    DRIVER( tg16 )      /* Turbo Grafix-16  NEC 1989-1993                   */
    DRIVER( sgx )       /* SuperGrafX NEC 1989                              */
    DRIVER( pcfx )      /* PC-FX NEC 1994                                   */

    DRIVER( arcadia )   /* Emerson Arcadia 2001                             */
    DRIVER( vcg )       /* Palladium Video-Computer-Game                    */

    /* Game Park */
    DRIVER( gp32 )	/* GP32 2001 */
    DRIVER( gp2x )	/* GP2X 2005 */

    /* GCE */
    DRIVER( vectrex )   /* General Consumer Electric Vectrex - 1982-1984    */
                        /* (aka Milton-Bradley Vectrex)                     */
    DRIVER( raaspec )   /* RA+A Spectrum - Modified Vectrex                 */

    /* Mattel */
    DRIVER( intv )      /* Mattel Intellivision - 1979 AKA INTV             */
    DRIVER( intvsrs )   /* Intellivision (Sears License) - 19??             */

    /* Entex */
    DRIVER( advision )  /* Adventurevision                                  */

    /* Capcom */
	DRIVER( sfach )     /* CPS Changer (Street Fighter Alpha)               */
	DRIVER( sfzbch )    /* CPS Changer (Street Fighter ZERO Brazil)         */
	DRIVER( sfzch )     /* CPS Changer (Street Fighter ZERO)                */
	DRIVER( wofch )     /* CPS Changer (Tenchi Wo Kurau II)                 */

    /* Magnavox */
    DRIVER( odyssey2 )  /* Magnavox Odyssey 2 - 1978-1983                   */

    /* Hartung, Watara, ...*/
    DRIVER( gmaster )   /* Hartung Gamemaster                               */

    /* Watara */
    DRIVER( svision )   /* Supervision Handheld                             */
    DRIVER( svisions )
    DRIVER( svisionp )
    DRIVER( svisionn )
    DRIVER( tvlinkp )

    /* INTERTON */
    DRIVER( vc4000 )    /* Interton vc4000                                  */

    /* BANDAI */
    DRIVER( wswan )     /* Bandai WonderSwan Handheld                       */
    DRIVER( wscolor )   /* Bandai WonderSwan Color Handheld                 */

    /* EPOCH */
    DRIVER( gamepock )  /* Epoch Game Pocket Computer                       */

    /* KOEI */
    DRIVER( pasogo )    /* KOEI PasoGo                                      */

    /* SNK */
    DRIVER( ngp )       /* NeoGeo Pocket                                    */
    DRIVER( ngpc )      /* NeoGeo Pocket Color                              */
    DRIVER( aes )       /* NeoGeo AES                                       */
    DRIVER( neocd )     /* NeoGeo CD                                        */
    DRIVER( neocdz )    /* NeoGeo CDZ                                       */

    /* Philips */
    DRIVER( cdimono1 )  /* Philips CD-i (Mono-I board)						*/

/****************COMPUTERS***************************************************/

    /* Acorn */
    DRIVER( acrnsys1 )  /* 1979 Acorn System 1 (Microcomputer Kit)          */
    DRIVER( atom )      /* 1979 Acorn Atom                                  */
    DRIVER( atomeb )    /* 1979 Acorn Atom                                  */
    DRIVER( bbca )      /* 1981 BBC Micro Model A                           */
    DRIVER( bbcb )      /* 1981 BBC Micro Model B                           */
    DRIVER( bbcbp )     /* 198? BBC Micro Model B+ 64K                      */
    DRIVER( bbcbp128 )  /* 198? BBC Micro Model B+ 128K                     */
    DRIVER( bbcm)       /* 198? BBC Master                                  */
    DRIVER( bbcbc )     /* 1985 BBC Bridge Companion                        */
    DRIVER( electron )  /* 1983 Acorn Electron                              */
    DRIVER( a310 )      /* 1988 Acorn Archimedes 310                        */
    DRIVER( a6809 )

    /* Cambridge Computers */
    DRIVER( z88 )       /*                                                  */

    /* Amstrad / Schneider */
    DRIVER( cpc464 )    /* Amstrad (Schneider in Germany) 1984              */
    DRIVER( cpc664 )    /* Amstrad (Schneider in Germany) 1985              */
    DRIVER( cpc6128 )   /* Amstrad (Schneider in Germany) 1985              */
    DRIVER( cpc6128s )  /* Amstrad (Schneider in Germany) 1985              */
    DRIVER( cpc6128f )  /* Amstrad (Schneider in Germany) 1985 (AZERTY)     */
    DRIVER( cpc464p )   /* Amstrad CPC464  Plus - 1990                      */
    DRIVER( cpc6128p )  /* Amstrad CPC6128 Plus - 1990                      */
    DRIVER( gx4000 )    /* Amstrad GX4000 - 1990                            */
    DRIVER( kccomp )    /* VEB KC compact                                   */
    DRIVER( al520ex )   /* Patisonic Aleste 520EX (1993)                    */
    DRIVER( pcw8256 )   /* 198? PCW8256                                     */
    DRIVER( pcw8512 )   /* 198? PCW8512                                     */
    DRIVER( pcw9256 )   /* 198? PCW9256                                     */
    DRIVER( pcw9512 )   /* 198? PCW9512 (+)                                 */
    DRIVER( pcw10 )     /* 198? PCW10                                       */
    DRIVER( pcw16 )     /* 1995 PCW16                                       */
    DRIVER( nc100 )     /* 1992 NC100                                       */
    DRIVER( nc150 )     /* 1992 NC150                                       */
    DRIVER( nc200 )     /* 1993 NC200                                       */

    /* Apple */
    DRIVER( apple1 )    /* Jul 1976 Apple 1                                 */
    DRIVER( apple2 )    /* Apr 1977 Apple ][                                */
    DRIVER( apple2p )   /* Jun 1979 Apple ][+                               */
    DRIVER( prav82 )    /* Pravetz 82                                       */
    DRIVER( prav8m )    /* Pravetz 8M                                       */
    DRIVER( apple2jp )  /* ??? ???? Apple ][j+                              */
    DRIVER( apple2e )   /* Jan 1983 Apple //e                               */
    DRIVER( mprof3 )    /* Microprofessor III                               */
    DRIVER( apple2ee )  /* Mar 1985 Apple //e Enhanced                      */
    DRIVER( apple2ep )  /* Jan 1987 Apple //e Platinum                      */
    DRIVER( apple2c )   /* Apr 1984 Apple //c                               */
    DRIVER( prav8c )    /* Pravetz 8C                               */
    DRIVER( apple2c0 )  /* ??? 1985 Apple //c (3.5 ROM)                     */
    DRIVER( apple2c3 )  /* Sep 1986 Apple //c (Original Mem. Exp.)          */
    DRIVER( apple2c4 )  /* ??? 198? Apple //c (rev 4)                       */
    DRIVER( apple2cp )  /* Sep 1988 Apple //c+                              */
    DRIVER( apple2gsr0 )    /* Sep 1986 Apple IIgs ROM00                    */
    DRIVER( apple2gsr1 )    /* Sep 1987 Apple IIgs ROM01                    */
    DRIVER( apple2gs )  /* Aug 1989 Apple IIgs ROM03                        */
    DRIVER( apple2gsr3p )   /* ??? 198? Apple IIgs ROM03 prototype          */
    DRIVER( apple2gsr3lp )  /* ??? 1989 Apple IIgs ROM03 late? prototype    */
    DRIVER( apple3 )    /* May 1980 Apple ///                               */
                    /* Dec 1983 Apple ///+                              */
    DRIVER( ace100 )    /* ??? 1982 Franklin Ace 100                        */
    DRIVER( laser128 )  /* ??? 1987 Laser 128                               */
    DRIVER( las128ex )  /* ??? 1987 Laser 128 EX                            */
    DRIVER( las3000 )   /* ??? 1983 Laser 3000                              */
    DRIVER( ivelultr )  /* Ivasim Ivel Ultra                                */
    DRIVER( agat7 )     /* Agat-7                                           */
    DRIVER( agat9 )     /* Agat-9                                           */

/*
 * Lisa                  January             1983
 * Lisa 2                January             1984
 * Macintosh XL          January             1985
 */
    DRIVER( lisa )      /* 1983 Apple Lisa                                  */
    DRIVER( lisa2 )     /* 1984 Apple Lisa 2                                */
    DRIVER( lisa210 )   /* 1984 Apple Lisa 2/10                             */
    DRIVER( macxl )     /* 1985 Apple Macintosh XL                          */
/*
 * Macintosh            January             1984
 * Macintosh 512k       July?               1984
 * Macintosh 512ke      April               1986
 * Macintosh Plus       January             1986
 * Macintosh SE         March               1987
 * Macintosh II         March               1987
 */
    DRIVER( mac128k )   /* 1984 Apple Macintosh                             */
    DRIVER( mac512k )   /* 1985 Apple Macintosh 512k                        */
    DRIVER( mac512ke )  /* 1986 Apple Macintosh 512ke                       */
    DRIVER( macplus )   /* 1986 Apple Macintosh Plus                        */
    DRIVER( macse )     /* 1987 Apple Macintosh SE                          */
    DRIVER( macii )     /* 1987 Apple Macintosh II */
    DRIVER( macsefd )   /* 1988 Apple Macintosh SE (FDHD) */
    DRIVER( mac2fdhd )	/* 1988 Apple Macintosh II (FDHD) */
    DRIVER( maciix )	/* 1988 Apple Macintosh IIx */
    DRIVER( macse30 )	/* 1989 Apple Macintosh SE/30 */
    DRIVER( maciicx )	/* 1989 Apple Macintosh IIcx */
    DRIVER( maciici )	/* 1989 Apple Macintosh IIci */
    DRIVER( macclasc )  /* 1990 Apple Macintosh Classic                     */
    DRIVER( maclc )	/* 1990 Apple Macintosh LC */
    DRIVER( maciisi )	/* 1990 Apple Macintosh IIsi */
    DRIVER( macclas2 )	/* 1991 Apple Macintosh Classic II */
    DRIVER( maclc2 )	/* 1991 Apple Macintosh LC II */
    DRIVER( maclc3 )	/* 1993 Apple Macintosh LC III */
    DRIVER( pmac6100 )  /* 1994 Apple Power Macintosh 6100 */

    /* Atari */
    DRIVER( a400 )      /* 1979 Atari 400                                   */
    DRIVER( a400pal )   /* 1979 Atari 400 PAL                               */
    DRIVER( a800 )      /* 1979 Atari 800                                   */
    DRIVER( a800pal )   /* 1979 Atari 800 PAL                               */
    DRIVER( a1200xl )   /* 1982 Atari 1200 XL                               */
    DRIVER( a600xl )    /* 1983 Atari 600 XL                                */
    DRIVER( a800xl )    /* 1983 Atari 800 XL                                */
    DRIVER( a800xlp )   /* 1983 Atari 800 XL (PAL)                          */
    DRIVER( a65xe )     /* 1986 Atari 65 XE (XL Extended)                   */
    DRIVER( a65xea )    /* 1986? Atari 65 XE Arabic                         */
    DRIVER( a130xe )    /* 1986 Atari 130 XE                                */
    DRIVER( a800xe )    /* 1986 Atari 800 XE                                */
    DRIVER( xegs )      /* 1987 Atari XE Game System                        */
    DRIVER( atarist )   /* Atari ST                                         */
    DRIVER( megast )    /* Atari Mega ST                                    */
//  DRIVER( stacy )     /* Atari STacy                                      */
    DRIVER( atariste )  /* Atari STe                                        */
    DRIVER( stbook )    /* Atari STBook                                     */
    DRIVER( megaste )   /* Atari Mega STe                                   */
//  DRIVER( stpad )     /* Atari STPad (prototype)                          */
//  DRIVER( tt030 )     /* Atari TT030                                      */
//  DRIVER( fx1 )       /* Atari FX-1 (prototype)                           */
//  DRIVER( falcon )    /* Atari Falcon030                                  */
//  DRIVER( falcon40 )  /* Atari Falcon040 (prototype)                      */

    /* Commodore */
    DRIVER( kim1 )      /* Commodore (MOS) KIM-1 1975                       */
    DRIVER( sym1 )      /* Synertek SYM-1                                   */
    DRIVER( aim65 )     /* Rockwell AIM65                                   */
    DRIVER( aim65_40 )  /* Rockwell AIM65/40                                */

    DRIVER( pet2001 )   /* PET 2001                                         */
    DRIVER( pet2001n )  /* PET 2001-N                                       */
    DRIVER( pet2001b )  /* PET 2001-B                                       */
    DRIVER( cbm30 )     /* CBM 30xx                                         */
    DRIVER( cbm30b )    /* CBM 30xx (Business keyboard)                     */
    DRIVER( cbm30nor )  /* CBM 30xx (Norway, Business keyboard)             */
    DRIVER( pet40on )   /* PET 40xx (Basic 4, no CRTC, Normal keyboard)     */
    DRIVER( pet40ob )   /* PET 40xx (Basic 4, no CRTC, Business keyboard)   */
    DRIVER( pet40n )    /* PET 40xx (Basic 4, CRTC 60Hz, 40 columns)        */
    DRIVER( pet40b )    /* PET 40xx (Basic 4, CRTC 60Hz, 80 columns)        */
    DRIVER( cbm40o )    /* CBM 40xx (Basic 4, no CRTC, Normal keyboard)     */
    DRIVER( cbm40ob )   /* CBM 40xx (Basic 4, no CRTC, Business keyboard)   */
    DRIVER( cbm40n )    /* CBM 40xx (Basic 4, CRTC 50Hz, 40 columns)        */
    DRIVER( cbm40b )    /* CBM 40xx (Basic 4, CRTC 50Hz, 80 columns)        */
    DRIVER( pet80 )     /* PET 80xx (Basic 4, CRTC 60Hz, 80 columns)        */
    DRIVER( cbm80 )     /* CBM 80xx (Basic 4, CRTC 50Hz, 80 columns)        */
    DRIVER( cbm80ger )  /* CBM 80xx (Germany, Basic 4, CRTC 50Hz, 80 cols)  */
    DRIVER( cbm80hun )  /* CBM 80xx (Hungary, Basic 4, CRTC 50Hz, 80 cols)  */
    DRIVER( cbm80swe )  /* CBM 80xx (Sweden, Basic 4, CRTC 50Hz, 80 cols)   */
    DRIVER( cbm8296 )   /* CBM 8296 (Basic 4, CRTC 50Hz, 80 columns)        */
    DRIVER( cbm8296d )  /* CBM 8296D                                        */
    DRIVER( superpet )  /* SuperPET                                         */
    DRIVER( sp9000 )    /* CBM SP9000 / MiniMainFrame 9000                  */
    DRIVER( mmf9000s )  /* MiniMainFrame 9000 (Sweden)                      */

    DRIVER( vic1001 )   /* Commodore VIC-1001 (Japan)                       */
    DRIVER( vic20 )     /* Commodore VIC 20 (NTSC)                          */
    DRIVER( vic20cr )   /* Commodore VIC 20CR (NTSC)                        */
    DRIVER( vic20i )    /* Commodore VIC 20 (NTSC, IEEE488 Interface)       */
    DRIVER( vic20v )    /* Commodore VIC 20 (NTSC, VC1540)                  */
    DRIVER( vic20pal )  /* Commodore VIC 20 (PAL)                           */
    DRIVER( vic20crp )  /* Commodore VIC 20CR (PAL)                         */
    DRIVER( vic20plv )  /* Commodore VIC 20 (PAL, VC1540)                   */
    DRIVER( vc20 )      /* Commodore VIC 20 (PAL, Germany)                  */
    DRIVER( vc20v )     /* Commodore VIC 20 (PAL, VC1540)                   */
    DRIVER( vic20swe )  /* Commodore VIC 20 (Swedish Expanson Kit)          */

    DRIVER( max )       /* Commodore Max Machine (Japan)                    */
    DRIVER( c64 )       /* Commodore 64 (NTSC)                              */
    DRIVER( c64pal )    /* Commodore 64 (PAL)                               */
    DRIVER( c64jpn )    /* Commodore 64 (Japan)                             */
    DRIVER( c64swe )    /* Commodore 64 (Sweden)                            */
    DRIVER( vic64s )    /* Commodore VIC 64S (Sweden)                       */
    DRIVER( pet64 )     /* Commodore PET 64                                 */
    DRIVER( cbm4064 )   /* Commodore CBM 4064                               */
    DRIVER( edu64 )     /* Commodore Educator 64                            */
    DRIVER( sx64 )      /* Commodore SX-64 Executive Machine (PAL)          */
    DRIVER( vip64 )     /* Commodore VIP64 (SX64, PAL, Swedish)             */
//  DRIVER( dx64 )      /* Commodore DX-64 - Prototype                      */
    DRIVER( c64c )      /* Commodore 64C (NTSC)                             */
    DRIVER( c64cpal )   /* Commodore 64C (PAL)                              */
    DRIVER( c64g )      /* Commodore 64G (PAL)                              */
    DRIVER( c64gs )     /* Commodore 64 Games System                        */

    DRIVER( b500 )      /* Commodore B500                                   */
    DRIVER( b128 )      /* Commodore B128                                   */
    DRIVER( b256 )      /* Commodore B256                                   */
    DRIVER( cbm610 )    /* Commodore CBM 610                                */
    DRIVER( cbm620 )    /* Commodore CBM 620                                */
    DRIVER( cbm620hu )  /* Commodore CBM 620 (Hungary)                      */
    DRIVER( b128hp )    /* Commodore B128HP                                 */
    DRIVER( b256hp )    /* Commodore B256HP                                 */
    DRIVER( bx256hp )   /* Commodore BX256HP                                */
    DRIVER( cbm710 )    /* Commodore CBM 710                                */
    DRIVER( cbm720 )    /* Commodore CBM 720                                */
    DRIVER( cbm720se )  /* Commodore CBM 720 (Sweden / Finland)             */
    DRIVER( p500 )      /* Commodore P500 (proto, a.k.a. C128-40, PET-II)   */

    DRIVER( c16 )       /* Commodore 16                                     */
    DRIVER( c16c )      /* Commodore 16  c1551                              */
    DRIVER( c16v )      /* Commodore 16  vc1541                             */
    DRIVER( c16hun )    /* Commodore 16 Novotrade (Hungarian Character Set) */
    DRIVER( c116 )      /* Commodore 116                                    */
    DRIVER( c116c )     /* Commodore 116  c1551                             */
    DRIVER( c116v )     /* Commodore 116  vc1541                            */
    DRIVER( plus4 )     /* Commodore +4                                     */
    DRIVER( plus4c )    /* Commodore +4  c1551                              */
    DRIVER( plus4v )    /* Commodore +4  vc1541                             */
    DRIVER( c232 )      /* Commodore 232 - Prototype                        */
    DRIVER( c264 )      /* Commodore 264 - Prototype                        */
    DRIVER( c364 )      /* Commodore V364 - Prototype                       */

    DRIVER( c128 )      /* Commodore 128 - NTSC                             */
    DRIVER( c128cr )    /* Commodore 128CR - NTSC (proto?)                  */
    DRIVER( c128sfi )   /* Commodore 128 - PAL (Swedish / Finnish)          */
    DRIVER( c128fino )  /* Commodore 128 - PAL (Finnish, Unconfirmed dumps) */
    DRIVER( c128fra )   /* Commodore 128 - PAL (French)                     */
    DRIVER( c128ger )   /* Commodore 128 - PAL (German)                     */
    DRIVER( c128nor )   /* Commodore 128 - PAL (Norwegian)                  */
    DRIVER( c128d )     /* Commodore 128D - PAL                             */
    DRIVER( c128dpr )   /* Commodore 128D - NTSC (proto)                    */
    DRIVER( c128dcr )   /* Commodore 128DCR - NTSC                          */
    DRIVER( c128drde )  /* Commodore 128DCR - PAL (German)                  */
    DRIVER( c128drit )  /* Commodore 128DCR - PAL (Italian)                 */
    DRIVER( c128drsw )  /* Commodore 128DCR - PAL (Swedish)                 */
    DRIVER( c128d81 )   /* Commodore 128D/81                                */

    DRIVER( a500n )     /* Commodore Amiga 500 - NTSC                       */
    DRIVER( a500p )     /* Commodore Amiga 500 - PAL                        */
    DRIVER( a1000n )    /* Commodore Amiga 1000 - NTSC                      */
    DRIVER( a1000p )    /* Commodore Amiga 1000 - PAL                       */
    DRIVER( cdtv )      /* Commodore CDTV                                   */
    DRIVER( a3000 )     /* Commodore Amiga 3000                             */
    DRIVER( a1200n )    /* Commodore Amiga 1200 - NTSC                      */
    DRIVER( a1200p )    /* Commodore Amiga 1200 - PAL                       */
    DRIVER( cd32 )      /* Commodore Amiga CD32                             */

    DRIVER( c65 )       /* 1991 C65 / C64DX (Prototype, NTSC)               */
    DRIVER( c64dx )     /* 1991 C65 / C64DX (Prototype, German PAL)         */

    /* Epson */
    DRIVER( px4 )       /* 1985 Epson PX-4                                  */
    DRIVER( px4p )      /* 1985 Epson PX-4+                                 */
    DRIVER( px8 )
    DRIVER( qx10 )
	DRIVER( ehx20 )

    /* IBM PC & Clones */
    DRIVER( ibm5150 )   /* 1981 IBM 5150                                    */
    DRIVER( dgone )     /* 1984 Data General/One */
    DRIVER( pcmda )     /* 1987 PC with MDA                                 */
    DRIVER( pcherc )    /* 1987 PC with Hercules (for testing hercules)     */
    DRIVER( pc )        /* 1987 PC with CGA                                 */
    DRIVER( bw230 )		/* 1985 Bondwell (CGA)                              */
    DRIVER( europc )    /* 1988 Schneider Euro PC (CGA or Hercules)         */

    /* PC Junior */
    DRIVER( ibmpcjr )   /* 1984 IBM PC Jr                                   */
    DRIVER( t1000hx )   /* 1987 Tandy 1000HX (similiar to PCJr)             */
    DRIVER( t1000sx )   /* 1987 Tandy 1000SX (similiar to PCJr)             */
	DRIVER( t1000rl )	/* 1989 Tandy 1000RL                                */

    /* XT */
    DRIVER( ibm5160 )   /* 1982 IBM XT 5160                                 */
    DRIVER( ibm5162 )   /* 1986 IBM XT 5162 (XT w/80286)                    */
    DRIVER( pc200 )     /* 1988 Sinclair PC200                              */
    DRIVER( pc20 )      /* 1988 Amstrad PC20                                */
    DRIVER( ppc512 )    /* 1987 Amstrad PPC512                              */
    DRIVER( ppc640 )    /* 1987 Amstrad PPC640                              */
    DRIVER( pc1512 )    /* 1986 Amstrad PC1512 v1 (CGA compatible)          */
    DRIVER( pc1512v2 )  /* 1986 Amstrad PC1512 v2 (CGA compatible)          */
    DRIVER( pc1640 )    /* 1987 Amstrad PC1640 (EGA compatible)             */

    DRIVER( xtvga )     /* 198? PC-XT (VGA, MF2 Keyboard)                   */

    /* AT */
    DRIVER( ibm5170 )   /* 1984 IBM PC/AT 5170, original 6 MHz model        */
    DRIVER( ibm5170a )  /* 1985 IBM PC/AT 5170, enhanced 8 MHz model        */
    DRIVER( i8530286 )  /* 1988 IBM PS2 Model 30 286 (VGA)                  */
    DRIVER( at )        /* 1987 AMI Bios and Diagnostics                    */
    DRIVER( atvga )     /* 19?? AT VGA                                      */
    DRIVER( neat )      /* 1989 New Enhanced AT chipset, AMI BIOS           */
    DRIVER( at386 )     /* 19?? IBM AT 386                                  */
    DRIVER( at486 )     /* 19?? IBM AT 486                                  */
    DRIVER( at586 )     /* 19?? AT 586                                      */

    /* 3Com / Palm / USRobotics */
    DRIVER( pilot1k )   /* Pilot 1000 */
    DRIVER( pilot5k )   /* Pilot 5000 */
    DRIVER( palmpers )  /* Palm Pilot Personal */
    DRIVER( palmpro )   /* Palm Pilot Professional */
    DRIVER( palmiii )   /* Palm III */
    DRIVER( palmiiic)   /* Palm IIIc */
    DRIVER( palmv )     /* Palm V */
    DRIVER( palmvx )    /* Palm Vx */
    DRIVER( palmm100 )  /* Palm m100 */
    DRIVER( palmm130 )  /* Palm m130 */
    DRIVER( palmm505 )  /* Palm m505 */
    DRIVER( palmm515 )  /* Palm m515 */
    DRIVER( visor )     /* Palm Visor Edge */
    DRIVER( spt1500 )   /* Symbol SPT 1500 */
    DRIVER( spt1700 )   /* Symbol SPT 1700 */
    DRIVER( spt1740 )   /* Symbol SPT 1740 */


    /* Osborne */
    DRIVER( osborne1 )  /* 1981 Osborne-1                                   */

    /* Sinclair Research */
    DRIVER( zx80 )      /* 1980 Sinclair ZX-80                              */
    DRIVER( zx81 )      /* 1981 Sinclair ZX-81                              */
    DRIVER( ts1000 )    /* 1982 Timex Sinclair 1000                         */
    DRIVER( ts1500 )    /* Timex Sinclair 1500                              */
    DRIVER( ringo470 )  /* Ringo 470                                        */
    DRIVER( pc8300 )    /* Your Computer - PC8300                           */
    DRIVER( pow3000 )   /* Creon Enterprises - Power 3000                   */
    DRIVER( lambda )    /* Lambda 8300                                      */
    DRIVER( tk85 )      /* Microdigital TK85                                */

    DRIVER( spectrum )/* 1982 ZX Spectrum                                   */
    DRIVER( spec80k ) /* 1987 ZX Spectrum 80k                           */
    DRIVER( specide ) /* 1995 ZX Spectrum IDE                           */
    DRIVER( inves )     /* 1986 Inves Spectrum 48K+                         */
    DRIVER( tk90x )     /* 1985 TK90x Color Computer                        */
    DRIVER( tk95 )      /* 1986 TK95 Color Computer                         */
    DRIVER( tc2048 )    /* 198? TC2048                                      */
    DRIVER( ts2068 )    /* 1983 TS2068                                      */
    DRIVER( uk2086 )    /* 1986 UK2086                                      */

    DRIVER( spec128 )     /* 1986 ZX Spectrum 128                               */
    DRIVER( specpls2 )  /* 1986 ZX Spectrum +2                              */
    DRIVER( specpl2a )  /* 1987 ZX Spectrum +2a                             */
    DRIVER( specpls3 )  /* 1987 ZX Spectrum +3                              */
    DRIVER( specpl3e )  /* 2000 ZX Spectrum +3e                             */
    DRIVER( sp3e8bit )  /* 2002 ZX Spectrum +3e 8bit IDE            */
    DRIVER( sp3ezcf )   /* 2002 ZX Spectrum +3e ZXCF                */
    DRIVER( sp3eata )   /* 2002 ZX Spectrum +3e ZXATASP             */
    DRIVER( scorpion )
    DRIVER( profi )
    DRIVER( kay1024 )
    DRIVER( quorum )
    DRIVER( pentagon )
    DRIVER( pent1024 )
    DRIVER( atm )
    DRIVER( atmtb2 )

    DRIVER( ql )        /* 1984 Sinclair QL (UK)                            */
    DRIVER( ql_us )     /* 1984 Sinclair QL (USA)                           */
    DRIVER( ql_es )     /* 1984 Sinclair QL (Spain)                         */
    DRIVER( ql_fr )     /* 1984 Sinclair QL (France)                        */
    DRIVER( ql_de )     /* 1984 Sinclair QL (Germany)                       */
    DRIVER( ql_it )     /* 1984 Sinclair QL (Italy)                         */
    DRIVER( ql_se )     /* 1984 Sinclair QL (Sweden)                        */
    DRIVER( ql_dk )     /* 1984 Sinclair QL (Denmark)                       */
    DRIVER( ql_gr )     /* 1984 Sinclair QL (Greece)                        */
//  DRIVER( tonto )
//  DRIVER( megaopd )

    /* Sharp */
    DRIVER( pc1245 )    /* Pocket Computer 1245                             */
    DRIVER( pc1250 )    /* Pocket Computer 1250                             */
    DRIVER( pc1251 )    /* Pocket Computer 1251                             */
    DRIVER( pc1255 )    /* Pocket Computer 1255                             */
    DRIVER( trs80pc3 )  /* Tandy TRS80 PC-3                                 */
    DRIVER( pc1260 )    /* Pocket Computer 1260                             */
    DRIVER( pc1261 )    /* Pocket Computer 1261                             */
    DRIVER( pc1401 )    /* Pocket Computer 1401                             */
    DRIVER( pc1402 )    /* Pocket Computer 1402                             */
    DRIVER( pc1350 )    /* Pocket Computer 1350                             */
    DRIVER( pc1403 )    /* Pocket Computer 1403                             */
    DRIVER( pc1403h )   /* Pocket Computer 1403H                            */

    DRIVER( mz700 )     /* 1982 Sharp MZ700                                 */
    DRIVER( mz700j )    /* 1982 Sharp MZ700 Japan                           */
    DRIVER( mz800 )     /* 1984 Sharp MZ800                                 */
    DRIVER( mz1500 )    /* 1984 Sharp MZ1500                                */
    DRIVER( mz2500 )    /* 1985 Sharp MZ2500                                */

    DRIVER( mz80kj )    /* 1979 Sharp MZ80K                                 */
    DRIVER( mz80k )     /* 1979 Sharp MZ80K                                 */
    DRIVER( mz80a )     /* 1982 Sharp MZ80A                                 */
    DRIVER( mz80b )     /* 1981 Sharp MZ80B                                 */

    DRIVER( x1 )        /* 1982 Sharp X1                                    */
    DRIVER( x1ck )      /* 1984 Sharp X1Ck                                  */
    DRIVER( x1turbo )   /* 1984 Sharp X1 Turbo (Model 10)                   */
    DRIVER( x1turbo40 ) /* 1985 Sharp X1 Turbo (Model 40)                   */
//  DRIVER( x1turboz )  /* 1986 Sharp X1 TurboZ                             */

    DRIVER( x68000 )    /* Sharp X68000 (1987)                              */
    DRIVER( x68kxvi )   /* Sharp X68000 XVI (1991)                          */
    DRIVER( x68030 )    /* Sharp X68030 (1993)                              */

    /* Silicon Graphics */
    DRIVER( sgi_ip2 )	/* IP2: IRIS 2x00, 68020, 16MHz						*/
    DRIVER( sgi_ip6 )   /* IP6: 4D/PI, R2000, 20MHz                         */
    DRIVER( ip204415 )  /* IP20: Indigo 1, R4400, 150MHz                    */
    DRIVER( ip225015 )  /* IP22: Indy, R5000, 150MHz						*/
    DRIVER( ip224613 )	/* IP22: Indy, R4600, 133MHz						*/
    DRIVER( ip244415 )	/* IP24: Indigo 2, R4400, 150MHz					*/

    /* Texas Instruments */
    DRIVER( ti990_10 )  /* 1975 TI 990/10                                   */
    DRIVER( ti990_4 )   /* 1976 TI 990/4                                    */
    DRIVER( 990189 )    /* 1978 TM 990/189                                  */
    DRIVER( 990189v )   /* 1980 TM 990/189 with Color Video Board           */

    DRIVER( ti99_224 )  /* 1983 TI 99/2 (24kb ROMs)                         */
    DRIVER( ti99_232 )  /* 1983 TI 99/2 (32kb ROMs)                         */
    DRIVER( ti99_4 )    /* 1979 TI-99/4                                     */
    DRIVER( ti99_4e )   /* 1980 TI-99/4 with 50Hz video                     */
    DRIVER( ti99_4a )   /* 1981 TI-99/4A                                    */
    DRIVER( ti99_4ae )  /* 1981 TI-99/4A with 50Hz video                    */
    DRIVER( ti99_4ev)   /* 1994 TI-99/4A with EVPC video card               */
    DRIVER( ti99_8 )    /* 1983 TI-99/8                                     */
    DRIVER( ti99_8e )   /* 1983 TI-99/8 with 50Hz video                     */

    /* TI 99 clones */
    DRIVER( tutor)      /* 1983? Tomy Tutor                                 */
    DRIVER( geneve )    /* 1987? Myarc Geneve 9640                          */
    DRIVER( genmod )    /* 199?? Myarc Geneve 9640                          */
    DRIVER( ti99_4p )   /* 1996 SNUG 99/4P (a.k.a. SGCPU)                   */

    DRIVER( avigo )     /*                                                  */

    /* Texas Instruments Calculators */
    DRIVER( ti73 )      /* 1990 TI-73                                       */
    DRIVER( ti81 )      /* 1990 TI-81 (Z80 2 MHz)                           */
    DRIVER( ti81v2 )    /* 1990 TI-81 (Z80 2 MHz)                           */
    DRIVER( ti85 )      /* 1992 TI-85 (Z80 6 MHz)                           */
    DRIVER( ti82 )      /* 1993 TI-82 (Z80 6 MHz)                           */
    DRIVER( ti83 )      /* 1996 TI-83 (Z80 6 MHz)                           */
    DRIVER( ti86 )      /* 1997 TI-86 (Z80 6 MHz)                           */
    DRIVER( ti83p )     /* 1999 TI-83 Plus (Z80 6 MHz)                      */
    DRIVER( ti83pse )   /* 2001 TI-83 Plus Silver Edition                   */
//  DRIVER( ti84p )     /* 2004 TI-84 Plus                                  */
    DRIVER( ti84pse )   /* 2004 TI-84 Plus Silver Edition                   */
    DRIVER( ti89 )      /* 1998 TI-89                                       */
    DRIVER( ti92 )      /* 1995 TI-92                                       */
    DRIVER( ti92p )     /* 1999 TI-92 Plus                                  */

    /* NEC */
    DRIVER( pc6001 )
    DRIVER( pc6001a )
    DRIVER( pc6001m2 )
    DRIVER( pc6001sr )
    DRIVER( pc6600 )
    DRIVER( pc6600sr )
    DRIVER( pc8001 )
    DRIVER( pc8001mk2 )
    DRIVER( pc8801 )
    DRIVER( pc8801mk2 )
    DRIVER( pc8801mk2sr )
    DRIVER( pc8801mk2fr )
    DRIVER( pc8801mk2mr )
    DRIVER( pc8801mh )
    DRIVER( pc8801fa )
    DRIVER( pc8801ma )
    DRIVER( pc8801ma2 )
    DRIVER( pc8801mc )
    DRIVER( pc88va )
    DRIVER( pc9801 )
    DRIVER( pc9821 )

    /* Cantab */
    DRIVER( jupiter )   /* Jupiter Ace                                      */

    /* Sord */
    DRIVER( sordm5 )
    DRIVER( srdm5fd5 )

    /* APF Electronics Inc. */
    DRIVER( apfm1000 )
    DRIVER( apfimag )

    /* Tatung */
    DRIVER( einstein )
    DRIVER( einstei2 )
    DRIVER( einst256 )

    /* Intelligent Software */
    DRIVER( ep64 )      /* Enterprise 64                                    */
    DRIVER( ep128 )     /* Enterprise 128                                   */
    DRIVER( phc64 )     /* Hegener & Glaser Mephisto PHC 64                 */

    /* Non-Linear Systems */
    DRIVER( kaypro2x )  /* Kaypro 2 - 2/84 */
    DRIVER( kaypro4a )  /* Kaypro 4 - 4/84 */
    DRIVER( kayproii )  /* Kaypro II - 2/83 */
    DRIVER( kaypro4 )   /* Kaypro 4 - 4/83 */
    DRIVER( kaypro4p88 )   /* Kaypro 4 - 4/83 w/plus88 board installed*/
    DRIVER( kaypro10 )  /* Kaypro 10 */
    DRIVER( omni2 )     /* Omni II */

    /* VEB Mikroelektronik */
    /* KC compact is partial CPC compatible */
    DRIVER( kc85_2 )    /* VEB KC 85/2                                      */
    DRIVER( kc85_3 )    /* VEB KC 85/3                                      */
    DRIVER( kc85_4 )    /* VEB KC 85/4                                      */
    DRIVER( kc85_4d )   /* VEB KC 85/4 with disk interface                  */
    DRIVER( kc85_5 )    /* VEB KC 85/5                                      */
    DRIVER( mc8020 )    /* MC 80.2x                                         */
    DRIVER( mc8030 )    /* MC 80.3x                                         */
    DRIVER( lc80 )
    DRIVER( lc80_2 )
    DRIVER( sc80 )

    /* Microbee Systems */
    DRIVER( mbee )      /* Microbee 16 Standard or Plus                     */
    DRIVER( mbeeic )    /* Microbee 32 IC                                   */
    DRIVER( mbeepc )    /* Microbee 32 PC                                   */
    DRIVER( mbeepc85 )  /* Microbee 32 PC85                                 */
    DRIVER( mbeeppc )   /* Microbee 32 PPC85                                    */
    DRIVER( mbee56 )    /* Microbee 56K (CP/M)                              */
    DRIVER( mbee64 )    /* Microbee 64K (CP/M)                              */

    /* Tandy / Radio Shack */
    DRIVER( trs80 )     /* TRS-80 Model I - Level I BASIC                   */
    DRIVER( trs80l2 )   /* TRS-80 Model I - Level II BASIC                  */
    DRIVER( sys80 )     /* EACA System 80                                   */
    DRIVER( lnw80 )     /* LNW Research LNW-80                              */
    DRIVER( trs80m3 )   /* TRS-80 Model III - Radio Shack/Tandy             */
    DRIVER( trs80m4 )
    DRIVER( trs80m4p )
    DRIVER( ht1080z ) /* Hradstechnika Szvetkezet HT-1080Z */
    DRIVER( ht1080z2 ) /* Hradstechnika Szvetkezet HT-1080Z Series II */
    DRIVER( ht108064 ) /* Hradstechnika Szvetkezet HT-1080Z/64 */
    DRIVER( radionic )  /* Radionic */

    DRIVER( coco )      /* Color Computer                                   */
    DRIVER( cocoe )     /* Color Computer (Extended BASIC 1.0)              */
    DRIVER( coco2 )     /* Color Computer 2                                 */
    DRIVER( coco2b )    /* Color Computer 2B (uses M6847T1 video chip)      */
    DRIVER( coco3 )     /* Color Computer 3 (NTSC)                          */
    DRIVER( coco3p )    /* Color Computer 3 (PAL)                           */
    DRIVER( coco3h )    /* Hacked Color Computer 3 (6309)                   */
    DRIVER( dragon32 )  /* Dragon 32                                        */
    DRIVER( dragon64 )  /* Dragon 64                                        */
    DRIVER( d64plus )   /* Dragon 64 + Compusense Plus addon                */
    DRIVER( dgnalpha )  /* Dragon Alpha                                     */
    DRIVER( dgnbeta )   /* Dragon Beta                                      */
    DRIVER( tanodr64 )  /* Tano Dragon 64 (NTSC)                            */
    DRIVER( cp400 )     /* Prologica CP400                                  */
    DRIVER( mc10 )      /* MC-10                                            */
    DRIVER( alice )     /* Matra & Hachette Ordinateur Alice                */

    /* EACA */
    DRIVER( cgenie )    /* Colour Genie EG2000                              */
    /* system 80 trs80 compatible */

    /* Video Technology  */
    DRIVER( crvision )  /* 1981 creatiVision                                */
    DRIVER( fnvision )  /* 1983 FunVision                                   */
    DRIVER( crvisioj )
    DRIVER( wizzard )
    DRIVER( rameses )
    DRIVER( vz2000 )
    DRIVER( crvisio2 )
    DRIVER( laser110 )  /* 1983 Laser 110                                   */
    DRIVER( las110de )  /* 1983 Sanyo Laser 110 (Germany)                   */
    DRIVER( laser200 )  /* 1983 Laser 200 (color version of 110)            */
//  DRIVER( vz200de )   /* 1983 VZ-200 (Germany)                            */
    DRIVER( fellow )    /* 1983 Salora Fellow (Finland)                     */
    DRIVER( tx8000 )    /* 1983 Texet TX-8000 (U.K.)                        */
    DRIVER( laser210 )  /* 1984 Laser 210 (200 with more memory)            */
    DRIVER( las210de )  /* 1984 Sanyo Laser 210 (Germany)                   */
    DRIVER( vz200 )     /* 1984 Dick Smith Electronics VZ-200               */
    DRIVER( laser310 )  /* 1984 Laser 310 (210 with diff. keyboard and RAM) */
    DRIVER( laser310h ) /* 1984 Laser 310 with SHRG mod                     */
    DRIVER( vz300 )     /* 1984 Dick Smith Electronics VZ-300               */
    DRIVER( laser350 )  /* 1984? Laser 350                                  */
    DRIVER( laser500 )  /* 1984? Laser 500                                  */
    DRIVER( laser700 )  /* 1984? Laser 700                                  */
    DRIVER( socrates )  /* 1988 Socrates Educational Video System */
    DRIVER( socratfc )  /* 1988 Socrates SAITOUT */

    /* Tangerine */
    DRIVER( microtan )  /* 1979 Microtan 65                                 */

    DRIVER( oric1 )     /* 1983 Oric 1                                      */
    DRIVER( orica )     /* 1984 Oric Atmos                                  */
    DRIVER( prav8d )    /* 1985 Pravetz 8D                                  */
    DRIVER( prav8dd )   /* 1989 Pravetz 8D (Disk ROM)                       */
    DRIVER( telstrat )  /* ??? Oric Telestrat/Stratos                       */

    /* Philips */
    DRIVER( p2000t )    /* 1980 P2000T                                      */
    DRIVER( p2000m )    /* 1980 P2000M                                      */
    DRIVER( videopac )  /* 1979 Videopac G7000/C52                          */
    DRIVER( g7400 )     /* 1983 Videopac Plus G7400                         */
    DRIVER( vg5k )      /* 1984 VG-5000                                     */

    /* Compukit */
    DRIVER( uk101 )

    /* Ohio Scientific */
    DRIVER( sb2m600b )
    DRIVER( c1p )
    DRIVER( c1pmf )

    /* ASCII & Microsoft */
    DRIVER( msx )       /* 1983 MSX                                         */
    DRIVER( ax170 )
    DRIVER( canonv10 )
    DRIVER( canonv20 )
    DRIVER( dpc100 )    /* 1984 MSX Korea                                   */
    DRIVER( dpc180 )    /* 1984 MSX Korea                                   */
    DRIVER( dpc200 )    /* 1984 MSX Korea                                   */
    DRIVER( gsfc200 )
    DRIVER( expert10 )  /* 1983 MSX Brazil                                  */
    DRIVER( expert11 )  /* 1984 MSX Brazil                                  */
    DRIVER( expert13 )  /* 1984 MSX Brazil                                  */
    DRIVER( expertdp )  /* 1985 MSX Brazil                                  */
    DRIVER( expertpl )  /* 1984 MSX Brazil                                  */
    DRIVER( jvchc7gb )
    DRIVER( mlf80 )
    DRIVER( mlfx1 )
    DRIVER( cf1200 )    /* 1984 MSX Japan                                   */
    DRIVER( cf2000 )    /* 1983 MSX Japan                                   */
    DRIVER( cf2700 )    /* 1984 MSX Japan                                   */
    DRIVER( cf3000 )    /* 1984 MSX Japan                                   */
    DRIVER( cf3300 )    /* 1985 MSX Japan                                   */
    DRIVER( fs1300 )    /* 1985 MSX Japan                                   */
    DRIVER( fs4000 )    /* 1985 MSX Japan                                   */
    DRIVER( nms801 )
    DRIVER( vg8000 )
    DRIVER( vg8010 )
    DRIVER( vg8010f )
    DRIVER( vg802000 )
    DRIVER( vg802020 )  /* 1985 MSX                                         */
    DRIVER( piopx7 )
    DRIVER( mpc100 )
    DRIVER( hotbit11 )  /* 1985 MSX Brazil                                  */
    DRIVER( hotbit12 )  /* 1985 MSX Brazil                                  */
    DRIVER( hotbi13b )  /* 1985 MSX Brazil                                  */
    DRIVER( hotbi13p )  /* 1985 MSX Brazil                                  */
    DRIVER( hotbit20 )  /* 1986 MSX2 Brazil                                 */
    DRIVER( hb10p )
    DRIVER( hb20p )
    DRIVER( hb201 )     /* 1985 MSX Japan                                   */
    DRIVER( hb201p )    /* 1985 MSX                                         */
    DRIVER( hb501p )    /* 1984 MSX                                         */
    DRIVER( hb55d )     /* 1983 MSX Germany                                 */
    DRIVER( hb55p )     /* 1983 MSX                                         */
    DRIVER( hb75d )     /* 1983 MSX Germany                                 */
    DRIVER( hb75p )     /* 1983 MSX                                         */
    DRIVER( svi728 )    /* 1985 MSX                                         */
    DRIVER( svi738 )    /* 1985 MSX                                         */
    DRIVER( svi738sw )  /* 1985 MSX                                         */
    DRIVER( tadpc200 )
    DRIVER( tadpc20a )
    DRIVER( hx10 )      /* 1984 MSX                                         */
    DRIVER( hx10s )     /* 1984 MSX                                         */
    DRIVER( hx20 )      /* 1984 MSX                                         */
    DRIVER( cx5m )
    DRIVER( cx5m128 )
    DRIVER( cx5m2 )
    DRIVER( yis303 )
    DRIVER( yis503 )
    DRIVER( yis503ii )
    DRIVER( y503iir )
    DRIVER( y503iir2 )
    DRIVER( yis503m )
    DRIVER( yc64 )
    DRIVER( bruc100 )


    DRIVER( msx2 )      /* 1985 MSX2                                        */
    DRIVER( ax350 )
    DRIVER( ax370 )
    DRIVER( hbf9p )     /* 1985 MSX2                                        */
    DRIVER( hbf9s )     /* 1985 MSX2                                        */
    DRIVER( hbf500p )   /* 1985 MSX2                                        */
    DRIVER( hbf700d )   /* 1985 MSX2 Germany                                */
    DRIVER( hbf700f )   /* 1985 MSX2                        */
    DRIVER( hbf700p )   /* 1985 MSX2                                        */
    DRIVER( hbf700s )   /* 1985 MSX2 Spain                                  */
    DRIVER( hbg900ap )  /* 1986 MSX2                                        */
    DRIVER( hbg900p )   /* 1986 MSX2                                        */
    DRIVER( nms8220 )   /* 1986 MSX2                                        */
    DRIVER( nms8220a )  /* 1986 MSX2                                        */
    DRIVER( vg8230 )    /* 1986 MSX2                                        */
    DRIVER( vg8235 )    /* 1986 MSX2                                        */
    DRIVER( vg8235f)    /* 1986 MSX2                                        */
    DRIVER( vg8240) /* 1986 MSX2                                        */
    DRIVER( nms8245 )   /* 1986 MSX2                                        */
    DRIVER( nms8245f)   /* 1986 MSX2                                        */
    DRIVER( nms8250 )   /* 1986 MSX2                                        */
    DRIVER( nms8255 )   /* 1986 MSX2                                        */
    DRIVER( nms8280 )   /* 1986 MSX2                                        */
    DRIVER( nms8280g)   /* 1986 MSX2                                        */
    DRIVER( tpc310)   /* 1986 MSX2                                      */
    DRIVER( hx23)       /* 1986 MSX2                                        */
    DRIVER( hx23f)    /* 1986 MSX2                                      */
    DRIVER( cx7m)       /* 1986 MSX2                                        */
    DRIVER( cx7m128)    /* 1986 MSX2                                        */
    DRIVER( fs5500 )    /* 1985 MSX2 Japan                                  */
    DRIVER( fs4500 )    /* 1986 MSX2 Japan                                  */
    DRIVER( fs4700 )    /* 1986 MSX2 Japan                                  */
    DRIVER( fs5000 )    /* 1986 MSX2 Japan                                  */
    DRIVER( fs4600 )    /* 1986 MSX2 Japan                                  */
    DRIVER( fsa1 )      /* 1986 MSX2 Japan                                  */
    DRIVER( fsa1a )     /* 1986 MSX2 Japan                                  */
    DRIVER( fsa1mk2 )   /* 1987 MSX2 Japan                                  */
    DRIVER( fsa1f )     /* 1987 MSX2 Japan                                  */
    DRIVER( fsa1fm )    /* 1988 MSX2 Japan                                  */
    DRIVER( hbf500 )    /* 1986 MSX2 Japan                                  */
    DRIVER( hbf900 )    /* 1986 MSX2 Japan                                  */
    DRIVER( hbf900a )   /* 1986 MSX2 Japan                                  */
    DRIVER( hbf1 )      /* 1986 MSX2 Japan                                  */
    DRIVER( hbf12 )     /* 1987 MSX2 Japan                                  */
    DRIVER( hbf1xd )    /* 1987 MSX2 Japan                                  */
    DRIVER( hbf1xdm2 )  /* 1988 MSX2 Japan                                  */
    DRIVER( phc23 )     /* 1986 MSX2 Japan                                  */
    DRIVER( cpc300 )    /* 1986 MSX2 Korea                                  */
    DRIVER( cpc300e )   /* 1986 MSX2 Korea                                  */
    DRIVER( cpc400 )    /* 1986 MSX2 Korea                                  */
    DRIVER( cpc400s )   /* 1986 MSX2 Korea                                  */
    DRIVER( expert20 )/* 1986 MSX2 Korea                                    */

    DRIVER( msx2p )     /* 1988 MSX2+ Japan                                 */
    DRIVER( fsa1fx )    /* 1988 MSX2+ Japan                                 */
    DRIVER( fsa1wx )    /* 1988 MSX2+ Japan                                 */
    DRIVER( fsa1wxa )   /* 1988 MSX2+ Japan                                 */
    DRIVER( fsa1wsx )   /* 1989 MSX2+ Japan                                 */
    DRIVER( hbf1xdj )   /* 1988 MSX2+ Japan                                 */
    DRIVER( hbf1xv )    /* 1989 MSX2+ Japan                                 */
    DRIVER( phc70fd )   /* 1988 MSX2+ Japan                                 */
    DRIVER( phc70fd2 )  /* 1988 MSX2+ Japan                                 */
    DRIVER( phc35j )    /* 1989 MSX2+ Japan                                 */


    /* NASCOM Microcomputers */
    DRIVER( nascom1 )   /* 1978 Nascom 1                                    */
    DRIVER( nascom2 )   /* 1979 Nascom 2                                    */

    /* Miles Gordon Technology */
    DRIVER( samcoupe )  /* 1989 Sam Coupe                                   */

    /* Motorola */
    DRIVER( mekd2 )     /* 1977 Motorola Evaluation Kit                     */

    /* DEC */
    DRIVER( pdp1 )      /* 1961 DEC PDP1                                    */
    DRIVER(vt100) /* 1978 Digital Equipment Corporation */
    //DRIVER(vt100wp) /* 1978 Digital Equipment Corporation */
    //DRIVER(vt100stp) /* 1978 Digital Equipment Corporation */
    //DRIVER(vt101) /* 1981 Digital Equipment Corporation */
    //DRIVER(vt102) /* 1981 Digital Equipment Corporation */
    //DRIVER(vt103) /* 1979 Digital Equipment Corporation */
    DRIVER(vt105) /* 1978 Digital Equipment Corporation */
    //DRIVER(vt110) /* 1978 Digital Equipment Corporation */
    //DRIVER(vt125) /* 1981 Digital Equipment Corporation */
    DRIVER(vt131) /* 1981 Digital Equipment Corporation */
    //DRIVER(vt132) /* 1978 Digital Equipment Corporation */
    //DRIVER(vt180) /* 1981 Digital Equipment Corporation */
    DRIVER(vt220) /* 1983 Digital Equipment Corporation */
    DRIVER(vt320) /* 1987 Digital Equipment Corporation */
    DRIVER(vt520) /* 1994 Digital Equipment Corporation */
    DRIVER(vk100) /* 1980 Digital Equipment Corporation */
    DRIVER(dectalk) /* 1982 Digital Equipment Corporation */

    /* Memotech */
    DRIVER( mtx512 )    /* 1983 Memotech MTX 512                            */
    DRIVER( mtx500 )    /* 1983 Memotech MTX 500                            */
    DRIVER( rs128 )     /* 1984 Memotech RS 128                             */

    /* Mattel */
    DRIVER( intvkbd )   /* 1981 - Mattel Intellivision Keyboard Component   */
                        /* (Test marketed, later recalled)                  */
    DRIVER( aquarius )  /* 1983 Aquarius                                    */
    DRIVER( aquarius_qd )  /* 1983 Aquarius w/ Quick Disk                   */
//  DRIVER( aquariu2 )  /* 1984 Aquarius II                                 */

    /* Exidy, Inc. */
    DRIVER( exidy )     /* Sorcerer                                         */
    DRIVER( exidyd )    /* Sorcerer (cassette only)                             */

    /* Galaksija */
    DRIVER( galaxy )
    DRIVER( galaxyp )

    /* Lviv/L'vov */
    DRIVER( lviv )      /* Lviv/L'vov                                       */

    /* Tesla */
    DRIVER( pmd851 )    /* PMD-85.1                                         */
    DRIVER( pmd852 )    /* PMD-85.2                                         */
    DRIVER( pmd852a )   /* PMD-85.2A                                        */
    DRIVER( pmd852b )   /* PMD-85.2B                                        */
    DRIVER( pmd853 )    /* PMD-85.3                                         */

    /* Didaktik */
    DRIVER( alfa )      /* Alfa (PMD-85.1 clone)                            */

    /* Statny */
    DRIVER( mato )      /* Mato (PMD-85.2 clone)                            */

    /* Zbrojovka Brno */
    DRIVER( c2717 )     /* Consul 2717 (PMD-85.2 clone)             		*/
	DRIVER( c2717pmd )  /* Consul 2717 with PMD-32             				*/

    /* Microkey */
    DRIVER( primoa32 )  /* Primo A-32                                       */
    DRIVER( primoa48 )  /* Primo A-48                                       */
    DRIVER( primoa64 )  /* Primo A-64                                       */
    DRIVER( primob32 )  /* Primo B-32                                       */
    DRIVER( primob48 )  /* Primo B-48                                       */
    DRIVER( primob64 )  /* Primo B-64                                       */
    DRIVER( primoc64 )  /* Primo C-64                                       */

    /* Team Concepts */
    /* CPU not known, else should be easy, look into drivers/comquest.c */
    DRIVER( comquest )  /* Comquest Plus German                             */

    /* Hewlett Packard */
    DRIVER( hp38g )
    DRIVER( hp39g )
    DRIVER( hp48s )     /* HP 48S */
    DRIVER( hp48sx )    /* HP 48SX */
    DRIVER( hp48g )     /* HP 48G */
    DRIVER( hp48gx )    /* HP 48GX */
    DRIVER( hp48gp )        /* HP 48G+ */
    DRIVER( hp49g )
    DRIVER( hp16500b )

    /* SpectraVideo */
    DRIVER( svi318 )    /* SVI-318 (PAL)                                    */
    DRIVER( svi318n )   /* SVI-318 (NTSC)                                  */
    DRIVER( svi328 )    /* SVI-328 (PAL)                                    */
    DRIVER( svi328n )   /* SVI-328 (NTSC)                                   */
    DRIVER( sv328p80 )  /* SVI-328 (PAL) + SVI-806 80 column card           */
    DRIVER( sv328n80 )  /* SVI-328 (NTSC) + SVI-806 80 column card          */

    /* Andrew Donald Booth (this is the name of the designer, not a company) */
//  DRIVER( apexc53 )   /* 1951(?) APEXC: All-Purpose Electronic X-ray Computer */
    DRIVER( apexc )     /* 1955(?) APEXC: All-Purpose Electronic X-ray Computer */

    /* Corvus */
    DRIVER(concept)     /* 1982 Corvus Concept                              */

    /* DAI */
    DRIVER(dai)         /* DAI                                              */

    /* Telenova */
    DRIVER(compis)      /* 1985 Telenova Compis                             */
    DRIVER(compis2)     /* 1985 Telenova Compis                             */

    /* Multitech */
    DRIVER(mpf1)        /* 1979 Multitech Micro Professor 1                 */
    DRIVER(mpf1b)       /* 1979 Multitech Micro Professor 1B                */

    /* Telercas Oy */
//  DRIVER(tmc1800)
    DRIVER(tmc2000)
    DRIVER(tmc2000e)
//  DRIVER(tmc600s1)
    DRIVER(tmc600s2)

    /* OSCOM Oy */
//  DRIVER(osc1000b)
    DRIVER(oscnano)

    /* MIT */
    DRIVER( tx0_64kw )  /* April 1956 MIT TX-0 (64kw RAM)                   */
    DRIVER( tx0_8kw )   /* 1962 MIT TX-0 (8kw RAM)                          */

    /* Luxor Datorer AB */
    DRIVER( abc80 )
//  DRIVER( abc80h )
    DRIVER( abc802 )
    DRIVER( abc800m )
    DRIVER( abc800c )
    DRIVER( abc806 )

    /* Be Incorporated */
    DRIVER( bebox )     /* BeBox Dual603-66                                 */
    DRIVER( bebox2 )    /* BeBox Dual603-133                                */

    /* Tiger Electronics */
    DRIVER( gamecom )   /* Tiger Game.com                                   */

    /* Thomson */
    DRIVER( t9000 )     /* 1980 Thomson T9000 (TO7 prototype)               */
    DRIVER( to7 )       /* 1982 Thomson TO7                                 */
    DRIVER( to770 )     /* 1984 Thomson TO7/70                              */
    DRIVER( to770a )    /* 198? Thomson TO7/70 arabic version               */
    DRIVER( mo5 )       /* 1984 Thomson MO5                                 */
    DRIVER( mo5e )      /* 1986 Thomson MO5E (export version)               */
    DRIVER( to9 )       /* 1985 Thomson T09                                 */
    DRIVER( to8 )       /* 1986 Thomson T08                                 */
    DRIVER( to8d )      /* 1987 Thomson T08D                                */
    DRIVER( to9p )      /* 1986 Thomson T09+                                */
    DRIVER( mo6 )       /* 1986 Thomson MO6                                 */
    DRIVER( mo5nr )     /* 1986 Thomson MO5 NR                              */
    DRIVER( pro128 )    /* 1986 Olivetti Prodest PC 128                     */

    /* Cybiko, Inc. */
    DRIVER( cybikov1 )  /* Cybiko Wireless Intertainment System - Classic V1 */
    DRIVER( cybikov2 )  /* Cybiko Wireless Intertainment System - Classic V2 */
    DRIVER( cybikoxt )  /* Cybiko Wireless Intertainment System - Xtreme     */

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
    DRIVER( bk0011m )

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
    DRIVER( mikron2 )
    DRIVER( apogee )
    DRIVER( mikrosha )
    DRIVER( partner )

    /* Homelab */
    DRIVER (homelab2)
    DRIVER (homelab3)
    DRIVER (homelab4)
    DRIVER (brailab4)

    /* Irisha */
    DRIVER (irisha)

    /* PK-8020 */
    DRIVER (korvet)

    /* Vector-06c */
    DRIVER (vector06)

    /* Robotron 1715 */
    DRIVER (rt1715)
    DRIVER (rt1715w)

    /* Z1013 */
    DRIVER (z1013)
    DRIVER (z1013a2)
    DRIVER (z1013k76)
    DRIVER (z1013s60)
    DRIVER (z1013k69)

    /* LLC */
    DRIVER (llc1)
    DRIVER (llc2)

    /* PP-01 */
    DRIVER (pp01)

    /* Ondra */
    DRIVER (ondrat)
    DRIVER (ondrav)

    /* SAPI-1 */
    DRIVER (sapi1)
	DRIVER (sapizps2)
	DRIVER (sapizps3)

  /* Spectrum clones */

  /* ICE-Felix */
    DRIVER( hc85 )    /* 1985 HC-85                                       */
    DRIVER( hc88 )    /* 1988 HC-88                                       */
    DRIVER( hc90 )    /* 1990 HC-90                                       */
    DRIVER( hc91 )    /* 1991 HC-91                                       */
    DRIVER( hc128 )   /* 1991 HC-128                                      */
    DRIVER( hc2000 )  /* 1992 HC-2000                                     */

    DRIVER( cobra )
    DRIVER( cobra80 )

    DRIVER( cip01 )   /* 1987 CIP-01                                      */
    DRIVER( cip03 )   /* 1988 CIP-03                                      */
    DRIVER( jet )     /* 1990 JET                                         */

  /* Didaktik Skalica */
    DRIVER( dgama87)  /* 1987 Didaktik Gama 87          */
    DRIVER( dgama88)  /* 1988 Didaktik Gama 88          */
    DRIVER( dgama89)  /* 1989 Didaktik Gama 89          */
    DRIVER( didakt90) /* 1990 Didaktik Skalica 90       */
    DRIVER( didakm91) /* 1991 Didaktik M 91             */
    DRIVER( didaktk)  /* 1992 Didaktik Kompakt          */
    DRIVER( didakm93) /* 1993 Didaktik M 93             */

    DRIVER( mistrum ) /* 1988 Mistrum                                       */

    /* Russian clones */
    DRIVER( blitz )     /* 1990 Blic                     */
    DRIVER( byte )      /* 1990 Byte                     */
    DRIVER( orizon )    /* 199? Orizon-Micro             */
    DRIVER( quorum48 )  /* 1993 Kvorum 48K               */
    DRIVER( magic6 )    /* 1993 Magic 6                  */
    DRIVER( compani1 )  /* 1990 Kompanion 1              */

    /* Kramer */
    DRIVER(kramermc)    /* 1987 Kramer MC                */

    /* AC1 */
    DRIVER(ac1)     /* 1984 Amateurcomputer AC1             */
    DRIVER(ac1_32)  /* 1984 Amateurcomputer AC1 (32 lines)  */
    DRIVER(ac1scch) /* 1984 Amateurcomputer AC1 SCCH        */

    DRIVER( pcm )   /* PC/M Mugler                          */

    /* Ei Nis */
    DRIVER(pecom64)

    /* Samsung SPC-1000 */
    DRIVER(spc1000)

    /* PolyMorphic Systems */
    DRIVER( poly88 )
    DRIVER( poly8813 )

    /* Bondwell */
    DRIVER( bw2 )

    /* Exeltel */
    DRIVER( exl100 )
	DRIVER( exeltel )

    /* Comx World Operations Ltd */
    DRIVER( comx35p )
    DRIVER( comx35n )
//  DRIVER( comxpl80 )

    /* Grundy Business Systems Ltd */
    DRIVER( newbrain )
    DRIVER( newbraie )
    DRIVER( newbraia )
    DRIVER( newbraim )

    /* Nokia Data */
    DRIVER( mm1m6 )
    DRIVER( mm1m7 )

    /* Nuova Elettronica */
    DRIVER( z80ne )         /* 1980 - Z80NE */
    DRIVER( z80net )        /* 1980 - Z80NE + LX388 Video Interface */
    DRIVER( z80netb )       /* 1980 - Z80NE + LX388 Video Interface + 16Kb BASIC */
    DRIVER( z80netf )       /* 1980 - Z80NE + LX.388 Video Interface + LX.390 FD Controller */

    /* Talking Electronics Magazine */
    DRIVER( tec1 )          /* Talking Electronics Computer */

    /* Kyocera (and clones) */
    DRIVER( kc85 )
    DRIVER( m10 )
//  DRIVER( m10m )
    DRIVER( trsm100 )
    DRIVER( tandy102 )
    DRIVER( tandy200 )
//  DRIVER( pc8201 )
    DRIVER( pc8201a )
//  DRIVER( npc8300 )
//  DRIVER( pc8401a )
    DRIVER( pc8500 )

    /* Nakajima manufactured eletronic typewriters */
    DRIVER( drwrt200 )  /* 199? NTS DreamWriter T200                         */
    DRIVER( drwrt400 )  /* 1996 NTS DreamWriter T400                         */
    DRIVER( wales210 )  /* 199? Walther ES-210 (German)                      */
    DRIVER( dator3k )   /* 199? Dator 3000 (Spanish)                         */

    /* Fujitsu */
    DRIVER( fm7 )
    DRIVER( fm77av )
    DRIVER( fm7740sx )
    DRIVER( fmtowns )   /* 1989 Fujitsu FM-Towns */
    DRIVER( fmtownsa )
    DRIVER( fmtmarty )  /* 1993 Fujitsu FM-Towns Marty */
    DRIVER( carmarty )  /* Fujitsu FM-Towns Car Marty */

    /* Camputers */
    DRIVER( lynx48k )
    DRIVER( lynx96k )
    DRIVER( lynx128k )

    /* Votrax */
    DRIVER( votrtnt ) /* 1980 Votrax Type-'N-Talk */
    DRIVER( votrpss ) /* 1982 Votrax Personal Speech System */

	/* Conitec Datensysteme */
    DRIVER( prof80 )
    DRIVER( prof80g21 )
    DRIVER( prof80g25 )
    DRIVER( prof80g26 )
    DRIVER( prof80g31 )
    DRIVER( prof80g562 )
    DRIVER( prof180x )

/****************Games*******************************************************/
    /* Computer Electronic */
    DRIVER( mk1 )       /* Chess Champion MK I                              */
    /* Quelle International */
    DRIVER( mk2 )       /* Chess Champion MK II                             */

    /* Novag */
    DRIVER( ssystem3 )  /* Chess Champion Super System III / MK III         */

    /* Hegener & Glaser Munich */
//  DRIVER( mephisto )  /* Mephisto                                         */
    DRIVER( mm4 )       /* Mephisto 4                                       */
    DRIVER( mm5 )       /* Mephisto 5.1 ROM                                 */
    DRIVER( mm50 )      /* Mephisto 5.0 ROM                                 */
    DRIVER( rebel5 )        /* Mephisto 5                                   */
    DRIVER( glasgow )       /* Glasgow                                      */
    DRIVER( amsterd )       /* Amsterdam                                    */
    DRIVER( dallas )        /* Dallas                                       */
    DRIVER( dallas16 )      /* Dallas                                       */
    DRIVER( dallas32 )      /* Dallas                                       */
    DRIVER( roma )          /* Roma                                         */
    DRIVER( roma32 )        /* Roma                                         */

/*********** Misc ***********************************************************/

    DRIVER( ex800 ) /* Epson EX-800 printer */
    DRIVER( lx800 ) /* Epson LX-800 printer */
    DRIVER( ssem ) /* Manchester Small-Scale Experimental Machine, "Baby" */
    DRIVER( craft ) /* Craft, by [lft] */

/*********** To sort (mostly skeleton drivers) ******************************/

    DRIVER( pc2000 ) /* VTech PreComputer 2000 */
    DRIVER( a5105 )
    DRIVER( bcs3 )
    DRIVER( bcs3a )
    DRIVER( bcs3b )
    DRIVER( bcs3c )
    DRIVER( bob85 )
    DRIVER( c80 )
    DRIVER( mc1000 )
    DRIVER( d6809 )
    DRIVER( mk85 )
    DRIVER( mk90 )
    DRIVER( elwro800 )
    DRIVER( fk1 )
    DRIVER( et3400 )
    DRIVER( amu880 )
    DRIVER( interact )
    DRIVER( jr200 )
    DRIVER( h8 )
    DRIVER( h19 )
    DRIVER( h89 )
    DRIVER( hec2hrp )
    DRIVER( hec2hr )
    DRIVER( hec2hrx )
    DRIVER( hec2mx80 )
    DRIVER( hec2mx40 )
    DRIVER( hector1 )
    DRIVER( victor )
	DRIVER( poly880 )
    DRIVER( sc1 )
    DRIVER( sc2 )
    DRIVER( chessmst )
    DRIVER( sys2900 )
    DRIVER( pmi80 )
    DRIVER( kontiki )
    DRIVER( tiki100 )
    DRIVER( vcs80 )
    DRIVER( v1050 )
    DRIVER( xerox820 )
    DRIVER( xor100 )
    DRIVER( iq151 )
    DRIVER( pyl601 )
    DRIVER( pyl601a )
    DRIVER( m20 )
    DRIVER( m40 )
    DRIVER( nanos )
    DRIVER( a5120 )
    DRIVER( a5130 )
    DRIVER( beehive )
    DRIVER( uts20 )
    DRIVER( uknc )
    DRIVER( zx97)
    DRIVER( x07)
    DRIVER( vesta)
    DRIVER( hobby)
    DRIVER( unior)
    DRIVER( vec1200)
    DRIVER( pk6128c)
    DRIVER( tvc64 )
    DRIVER( tvc64p )
    DRIVER( tvc64pru )
    DRIVER( bw12 )
    DRIVER( bw14 )
    DRIVER( sdk86 )
    DRIVER( vboy )
    DRIVER( zrt80 )
    DRIVER( exp85 )
    DRIVER( z9001 )
    DRIVER( kc85_111 )
    DRIVER( kc87_10 )
    DRIVER( kc87_11 )
    DRIVER( kc87_20 )
    DRIVER( kc87_21 )
    DRIVER( cat )
    DRIVER( swyft )
    DRIVER( mmd1 )
    DRIVER( mmd2 )
    DRIVER( mpf1p )
    DRIVER( stopthie )
    DRIVER( aprixi )
    DRIVER( aprif1 )
    DRIVER( aprif10 )
    DRIVER( aprifp )
    DRIVER( amico2k )
    DRIVER( jtc )
    DRIVER( jtces88 )
    DRIVER( jtces23 )
    DRIVER( jtces40 )
    DRIVER( ec65 )
    DRIVER( ec65k )
    DRIVER( junior )
    DRIVER( beta )
    DRIVER( elf2 )
    DRIVER( pippin )
    DRIVER( sol20 )
    DRIVER( 4004clk )
    DRIVER( busicom )
    DRIVER( p8000 )
    DRIVER( p8000_16 )
	DRIVER( cosmicos )
	DRIVER( a7150 )
	DRIVER( next )
	DRIVER( nextnt )
	DRIVER( nexttrb )
	DRIVER( pda600 )
	DRIVER( pce220 )
	DRIVER( mod8 )
	DRIVER( k1003 )
	DRIVER( mk14 )
	DRIVER( elekscmp )
	DRIVER( ht68k )
	DRIVER( mits680b )
	DRIVER( basic52 )
	DRIVER( basic31 )	
	DRIVER( al8800bt )
	DRIVER( sun1 )
	DRIVER( micronic )
	DRIVER( plan80 )
	DRIVER( pro80 )
	DRIVER( pimps )
	DRIVER( sage2 )
	DRIVER( zexall ) /* zexall z80 test suite with kevtris' preloader/serial interface at 0000-00ff */
	DRIVER( horizon )
	DRIVER( horizsd )
	DRIVER( vector1 )
	DRIVER( tricep )
	DRIVER( indiana )
	DRIVER( vector4 )
	
#endif /* DRIVER_RECURSIVE */
