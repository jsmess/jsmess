/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Thomson 8-bit micro-computers.

  A 6809E-based French family of personal computers developed in the 80's.
  Despite their rather high price and poor design , they were quite popular 
  in France, probably due to the government plan "Informatique pour tous" 
  (Computer Science for Everyone) to put a network of MO5 computers in every 
  school during 1984-1986. And maybe their very popular light-pen.

  Drivers
  - t9000  Thomson T9000           (early TO7 prototype, oct 1980)
  - to7    Thomson TO7             (nov 1982)
  - to770  Thomson TO7/70          (scarcely enhanced TO7, 1984)
  - mo5    Thomson MO5             (entry-game, TO7-incompatible, jun 1984)
  - to9    Thomson TO9             (high-end TO7/70 successor, sep 1985)
  - mo5e   Thomson MO5E            (export MO5 version, 1986)
  - to8    Thomson TO8             (next generation of TO7/70, sep 1986)
  - to9p   Thomson TO9+            (improved TO9 with TO8 technology, sep 1986)
  - mo6    Thomson MO6             (improved MO5 with TO8 technology, sep 1986)
  - mo5nr  Thomson MO5NR           (network-enhanced MO6, 1986)
  - to8d   Thomson TO8D            (TO8 with integrated floppy, dec 1987)
  - pro128 Olivetti Prodest PC 128 (Italian MO6, built by Thomson, 1986)

  We do not consider here the few 16-bit computers built by Thomson 
  (68000-based Micromega 32, 8088-based Micromega 16 or the TO16: Thomson's own
   8088-based PC).

  You may distinguish three families:
  * TO7,TO7/70,TO8,TO8D (family computers)
  * TO9,TO9+ (professional desktop look with separate keyboard and monitor)
  * MO5,MO5E,MO5NR,MO6 (cheaper, less extensible family)
  Computers in both TO families are compatible. Computers in the MO family are
  compatible. However, the TO and MO families are incompatible
  (different memory-mapping).

  Note that the TO8, TO9+ and MO6 were produced at the same time, using very
  similar technologies, but with different cost/feature trade-offs and
  different compatibility (MO6 is MO5-compatible, TO9+ is TO9-compatible and
  TO8 and TO9+ are TO7-compatible).
  Also note that the MO5NR is actually based on MO6 design, not MO5 
  (although both MO5NR and MO6 are MO5-compatible)

  Also of interest are the Platini and Hinault versions of the MO5
  (plain MO5 inside, but with custom, signed box-case).
  There were several versions of TO7/70 and MO5 with alternate keyboards.

  Thomson stopped producing micro-computers in jan 1990.
  
**********************************************************************/

/* TODO (roughly in decreasing priority order):
   ----

   - test the new game extension with all software
   - test that recent changes to 6821 port A do not break the driver
   - add PORT_CHAR in keyboard
   - support all cartridge bank switching methods
   - THMFC1 support for single density
   - internal TO9 mouse port (untested)
   - better state save
   - support for CD 90-015 floppy controller
   - floppy: 2-sided or 4-sided .fd images
   - printer post-processing => postscript
   - RS232 serial port extensions: CC 90-232, RF 57-932
   - modem, teltel extension: MD 90-120 / MD 90-333 (need controller ROM?)
   - IEEE extension
   - TV overlay (IN 57-001) (@)
   - digitisation extension (DI 90-011) (@)
   - barcode reader (@)

   (@) means MESS is lacking support for this kind of device / feature anyway

*/

#include "driver.h"
#include "device.h"
#include "inputx.h"
#include "machine/6821pia.h"
#include "machine/mc6846.h"
#include "includes/6551.h"
#include "machine/thomson.h"
#include "video/thomson.h"
#include "audio/mea8000.h"
#include "devices/cartslot.h"
#include "devices/printer.h"
#include "devices/cassette.h"
#include "formats/thom_cas.h"
#include "devices/thomflop.h"
#include "formats/thom_dsk.h"
#include "includes/serial.h"


/**************************** common *******************************/

/* layout */
static const char layout_thomson[] = "thomson";

#define KEY(pos,name,key)				\
  PORT_BIT  ( 1<<(pos), IP_ACTIVE_LOW, IPT_KEYBOARD )	\
  PORT_NAME ( name )					\
  PORT_CODE ( KEYCODE_##key )
 
#define PAD(mask,player,name,port,dir,key)				\
  PORT_BIT    ( mask, IP_ACTIVE_LOW, IPT_##port )			\
  PORT_NAME   ( "P" #player " " name )					\
  PORT_CODE( JOYCODE_##player##_##dir )					\
  PORT_CODE( KEYCODE_##key )						\
  PORT_PLAYER ( player )


/* ------------- game port ------------- */

/*
  Two generations of game port extensions were developped

  - CM 90-112 (model 1)
    connect up to two 8-position 1-button game pads

  - SX 90-018 (model 2)
    connect either two 8-position 2-button game pads
    or a 2-button mouse (not both at the same time!)

  We emulate the SX 90-018 as it is fully compatible with the CM 90-112.  

  Notes:
  * all extensions are compatible with all Thomson computers.
  * the SX 90-018 extension is integrated within the TO8(D)
  * the TO9 has its own, different mouse port
  * all extensions are based on a Motorola 6821 PIA
  * all extensions include a 6-bit sound DAC
  * most pre-TO8 software (including TO9) do not recognise the mouse nor the 
  second button of each pad
  * the mouse cannot be used at the same time as the pads: they use the same
  6821 input ports & physical port; we use a config switch to tell MESS 
  whether pads or a mouse is connected
  * the mouse should not be used at the same time as the sound DAC: they use
  the same 6821 ports, either as input or output; starting from the TO8,
  there is a 'mute' signal to cut the DAC output and avoid producing an
  audible buzz whenever the mouse is moved; unfortunately, mute is not 
  available on the TO7(/70), TO9 and MO5.
*/

INPUT_PORTS_START( thom_game_port )

/* joysticks, common to CM 90-112 & SX 90-018 */
     PORT_START_TAG ( "game_port_directions" )
     PAD ( 0x01, 1, "Up",    JOYSTICK_UP, UP, UP)
     PAD ( 0x02, 1, "Down",  JOYSTICK_DOWN, DOWN, DOWN )
     PAD ( 0x04, 1, "Left",  JOYSTICK_LEFT, LEFT, LEFT )
     PAD ( 0x08, 1, "Right", JOYSTICK_RIGHT, RIGHT, RIGHT )
     PAD ( 0x10, 2, "Up",    JOYSTICK_UP, UP, 8_PAD )
     PAD ( 0x20, 2, "Down",  JOYSTICK_DOWN, DOWN, 2_PAD )
     PAD ( 0x40, 2, "Left",  JOYSTICK_LEFT, LEFT, 4_PAD )
     PAD ( 0x80, 2, "Right", JOYSTICK_RIGHT, RIGHT, 6_PAD )

     PORT_START_TAG ( "game_port_buttons" )
     PAD ( 0x40, 1, "Action A", BUTTON1, BUTTON1, LCONTROL )
     PAD ( 0x80, 2, "Action A", BUTTON1, BUTTON1, RCONTROL )
 
/* joysticks, SX 90-018 specific */
     PAD ( 0x04, 1, "Action B", BUTTON2, BUTTON2, LALT )
     PAD ( 0x08, 2, "Action B", BUTTON2, BUTTON2, RALT )
     PORT_BIT  ( 0x30, IP_ACTIVE_LOW, IPT_UNUSED )
     PORT_BIT  ( 0x03, IP_ACTIVE_LOW, IPT_UNUSED ) /* ? */

/* mouse, SX 90-018 specific */
     PORT_START_TAG ( "mouse_x" )
     PORT_BIT ( 0xffff, 0x00, IPT_MOUSE_X ) 
     PORT_NAME ( "Mouse X" )
     PORT_SENSITIVITY ( 150 )
     PORT_CODE( MOUSECODE_1_ANALOG_X )
     PORT_PLAYER (1)
  
     PORT_START_TAG ( "mouse_y" )
     PORT_BIT ( 0xffff, 0x00, IPT_MOUSE_Y )
     PORT_NAME ( "Mouse Y" )
     PORT_SENSITIVITY ( 150 ) 
     PORT_CODE( MOUSECODE_1_ANALOG_Y )
     PORT_PLAYER (1)

     PORT_START_TAG ( "mouse_button" )
     PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
     PORT_NAME ( "Left Mouse Button" )
     PORT_CODE( MOUSECODE_1_BUTTON1 )
     PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
     PORT_NAME ( "Right Mouse Button" )
     PORT_CODE( MOUSECODE_1_BUTTON3 )

INPUT_PORTS_END


/* ------------ lightpen ------------ */

INPUT_PORTS_START( thom_lightpen )

     PORT_START_TAG ( "lightpen_x" )
     PORT_BIT ( 0xffff, THOM_TOTAL_WIDTH/2, IPT_LIGHTGUN_X )
     PORT_NAME ( "Lightpen X" )
     PORT_MINMAX( 0, THOM_TOTAL_WIDTH )
     PORT_SENSITIVITY( 50 )
     PORT_CODE( MOUSECODE_1_ANALOG_X )
     PORT_CROSSHAIR(X, 1.0, 0.0, 0)

     PORT_START_TAG ( "lightpen_y" )
     PORT_BIT ( 0xffff, THOM_TOTAL_HEIGHT/2, IPT_LIGHTGUN_Y )
     PORT_NAME ( "Lightpen Y" )
     PORT_MINMAX ( 0, THOM_TOTAL_HEIGHT )
     PORT_SENSITIVITY( 50 )
     PORT_CODE( MOUSECODE_1_ANALOG_Y )
     PORT_CROSSHAIR(Y, 1.0, 0.0, 0)

     PORT_START_TAG ( "lightpen_button" )
     PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
     PORT_NAME ( "Lightpen Button" )
     PORT_CODE( MOUSECODE_1_BUTTON1 )

INPUT_PORTS_END


/* ------------ cartridge ------------ */

static void to7_cartridge_getinfo( const device_class *devclass, 
				   UINT32 state, union devinfo *info )
{
  switch ( state ) {
  case DEVINFO_INT_COUNT:
    info->i = 1;
    break;
  case DEVINFO_PTR_LOAD:
    info->load = to7_cartridge_load;
    break;
  case DEVINFO_STR_FILE_EXTENSIONS:  
    strcpy( info->s = device_temp_str(), "m7,rom" );
    break;
  default: 
    cartslot_device_getinfo( devclass, state, info );
  }  
}

static void mo5_cartridge_getinfo( const device_class *devclass, 
				   UINT32 state, union devinfo *info )
{
  switch ( state ) {
  case DEVINFO_INT_COUNT:
    info->i = 1;
    break;
  case DEVINFO_PTR_LOAD:
    info->load = mo5_cartridge_load;
    break;
  case DEVINFO_STR_FILE_EXTENSIONS:  
    strcpy( info->s = device_temp_str(), "m5,rom" );
    break;
  default: 
    cartslot_device_getinfo( devclass, state, info );
  }  
}


/* ------------ printer ------------ */

static void thom_printer_getinfo ( const device_class *devclass, 
				   UINT32 state, union devinfo *info )
{
  switch ( state ) {
  case DEVINFO_INT_COUNT:
    info->i = 1; 
    break;
  default:
    printer_device_getinfo( devclass, state, info );
  }
}


/* ------------ serial ------------ */

static const char* thom_serial_names[3][3]=
  { { "cc90232", "c232", "RS232 (older, CC 90-232)" },
    { "rf57932", "r232", "RS232 (newer, RF 57-932)" },
    { "modem", "modm", "Modem" } }; 

static void thom_serial_getinfo ( const device_class *devclass, 
				   UINT32 state, union devinfo *info )
{
  switch ( state ) {
  case DEVINFO_INT_COUNT:
    info->i = 3; 
    break;
  case DEVINFO_INT_TYPE:
    info->i = IO_SERIAL; 
    break;
  case DEVINFO_INT_READABLE:
    info->i = 1; 
    break;
  case DEVINFO_INT_WRITEABLE:
    info->i = 1; 
    break;
  case DEVINFO_INT_CREATABLE:
    info->i = 1; 
    break;

  case DEVINFO_PTR_INIT:
    info->init = thom_serial_init;
    break;
  case DEVINFO_PTR_LOAD:
    info->load = thom_serial_load; 
    break;
  case DEVINFO_PTR_UNLOAD:
    info->unload = thom_serial_unload;
    break;

  case DEVINFO_STR_NAME+0:
  case DEVINFO_STR_NAME+1:
  case DEVINFO_STR_NAME+2:
    strcpy( info->s = device_temp_str(), 
	    thom_serial_names[ state - DEVINFO_STR_NAME ][ 0 ] );
    return;
  case DEVINFO_STR_SHORT_NAME+0:
  case DEVINFO_STR_SHORT_NAME+1:
  case DEVINFO_STR_SHORT_NAME+2:
    strcpy( info->s = device_temp_str(), 
	    thom_serial_names[ state - DEVINFO_STR_SHORT_NAME ][ 1 ] );
    return;
  case DEVINFO_STR_DESCRIPTION+0:
  case DEVINFO_STR_DESCRIPTION+1:
  case DEVINFO_STR_DESCRIPTION+2:
    strcpy( info->s = device_temp_str(), 
	    thom_serial_names[ state - DEVINFO_STR_DESCRIPTION ][ 2 ] );
    return;

  case DEVINFO_STR_DEV_FILE:
    strcpy( info->s = device_temp_str(), "serial" );
    break;
  case DEVINFO_STR_FILE_EXTENSIONS:
    strcpy( info->s = device_temp_str(), "txt" ); 
    break;
  }
}



/************************** T9000 / TO7 *******************************

TO7 (1982)
---

First computer by Thomson.
Note that the computer comes with only a minimal BIOS and requires an
external cartridge to be usable.
Most software are distributed on cassette and require the BASIC 1.0 cartridge
to be present (-cart basic.m7), as only it provides the necessary OS 
capabilities (e.g., a cassette loader).
To use disks, you will need both a BASIC 1.0 cartridge and a BASIC DOS 
boot floppy.

* chips:
  - 1 MHz Motorola 6809E CPU
  - 1 Motorola 6821 PIA (+3 for I/O, game, and modem extensions)
  - 1 Motorola 6846 timer, I/O, ROM

* memory:
  - 8 KB base user RAM
    + 16 KB extended user RAM (EM 90-016) = 24 KB total user RAM emulated
  - 6 KB BIOS ROM
  - 6-bit x 8 K color RAM + 8-bit x 8 K point RAM, bank switched
  - 2 to 8 KB ROM comes with the floppy drive / network controller

* video:
    320x200 pixels with color constraints (2 colors per horizontal
    8-pixel span), 8-color pixel palette,
    50 Hz (tweaked SECAM)

* devices:
  - AZERTY keyboard, 58-keys, French with accents
  - cartridge 16 KB (up to 64 KB using bank-switching),
    the MESS cartridge device is named -cart
  - cassette 900 bauds (frequency signals: 0=4.5Khz, 1=6.3KHz)
    the MESS cassette device is named -cass
  - 1-bit internal buzzer
  - lightpen, with 8-pixel horizontal resolution, 1-pixel vertical
  - SX 90-018 game & music extension
    . 6-bit DAC sound
    . two 8-position 2-button game pads
    . 2-button mouse
    . based on a Motorola 6821 PIA
  - CC 90-232 I/O extension: 
    . CENTRONICS (printer) 
    . RS232 (unemulated)
    . based on a Motorola 6821 PIA
    . NOTE: you cannot use the CENTRONICS and RS232 at the same time
  - RF 57-932: RS232 extension, based on a SY 6551 ACIA (unemulated)
  - MD 90-120: MODEM, TELETEL extension (unemulated)
    . 1 Motorola 6850 ACIA
    . 1 Motorola 6821 PIA
    . 1 EFB 7513 MODEM FSK V13, full duplex
    . PTT-, VELI7Y-, and V23-compatible MODEM (up to 1200 bauds)
    . seems to come with an extra ROM
  - 5"1/2 floppy drive extension
    . CD 90-640 floppy controller, based on a Western Digital 2793
    . DD 90-320 double-density double-sided 5"1/4 floppy
      (2 drives considered as 4 simple-face drives: 0/1 for the first drive, 
       2/3 for the second drive, 1 and 3 for upper sides, 0 and 2 for lower 
       sides)
    . floppies are 40 tracks/side, 16 sectors/track, 128 or 256 bytes/sector
      = from 80 KB one-sided single-density, to 320 KB two-sided double-density
    . MESS floppy devices are named -flop0 to -flop3
  - alternate 5"1/2 floppy drive extension (unemulated)
    . CD 90-015 floppy controller (no information on this)
    . UD 90-070 5"1/4 single-sided single density floppy drive
  - alternate 3"1/2 floppy drive extension
    . CD 90-351 floppy controller, based on a custom Thomson gate-array
    . DD 90-352 3"1/2 floppy drives
  - alternate QDD floppy drive extension
    . CQ 90-028 floppy controller, based on a Motorola 6852 SSDA
    . QD 90-028 quickdrive 2"8 (QDD), only one drive, signe side
  - speech synthesis extension: based on a Philips / Signetics MEA 8000
    (cannot be used with the MODEM)
  - MIDIPAK MIDI extension, uses a EF 6850 ACIA
  - NR 07-005: network extension, MC 6854 based, 2 KB ROM & 64 KB RAM
    (build by the French Leanord company)


T9000 (1980)
-----

Early TO7 prototype.
The hardware seems to be the exactly same. Only the BIOS is different. 
It has some bug that were corrected later for the TO7.
Actually, the two computers are undistinguishable, except for the different
startup screen, and a couple BIOS addresses.
They can run the same software and accept the same devices and extensions.


**********************************************************************/


/* ------------ address maps ------------ */

static ADDRESS_MAP_START ( to7, ADDRESS_SPACE_PROGRAM, 8 )

     AM_RANGE ( 0x0000, 0x3fff ) AM_ROMBANK   ( THOM_CART_BANK ) /* 4 * 16 KB */
                                 AM_WRITE     ( to7_cartridge_w )
     AM_RANGE ( 0x4000, 0x5fff ) AM_RAMBANK   ( THOM_VRAM_BANK )
                                 AM_WRITE     ( to7_vram_w ) 
     AM_RANGE ( 0x6000, 0x7fff ) AM_RAM       /* base user memory 8 KB */
     AM_RANGE ( 0x8000, 0xbfff ) AM_RAM       /* extension user memory 16 KB */
     AM_RANGE ( 0xc000, 0xdfff ) AM_NOP       /* unused 8 KB*/
     AM_RANGE ( 0xe000, 0xe7bf ) AM_ROMBANK   ( THOM_FLOP_BANK )
     AM_RANGE ( 0xe7c0, 0xe7c7 ) AM_READWRITE ( mc6846_r, mc6846_w )
     AM_RANGE ( 0xe7c8, 0xe7cb ) AM_READWRITE ( pia_0_r,  pia_0_w )
     AM_RANGE ( 0xe7cc, 0xe7cf ) AM_READWRITE ( pia_1_r,  pia_1_w )
     AM_RANGE ( 0xe7d0, 0xe7df ) AM_READWRITE ( to7_floppy_r, to7_floppy_w )
     AM_RANGE ( 0xe7e0, 0xe7e3 ) AM_READWRITE ( pia_2_r, pia_2_w )
     AM_RANGE ( 0xe7e8, 0xe7eb ) AM_READWRITE ( acia_6551_r, acia_6551_w )
     AM_RANGE ( 0xe7f2, 0xe7f3 ) AM_READWRITE ( to7_midi_r, to7_midi_w )
     AM_RANGE ( 0xe7f8, 0xe7fb ) AM_READWRITE ( pia_3_r, pia_3_w )
     AM_RANGE ( 0xe7fe, 0xe7ff ) AM_READWRITE ( to7_modem_mea8000_r, 
						to7_modem_mea8000_w )
     AM_RANGE ( 0xe800, 0xffff ) AM_ROM       /* system bios  */

/* 0x10000 - 0x1ffff: 64 KB external ROM cartridge */
/* 0x20000 - 0x23fff: 16 KB video RAM (actually 8 K x 8 bits + 8 K x 6 bits) */
/* 0x24000 - 0x27fff: 16 KB floppy ROM controllers */

ADDRESS_MAP_END



/* ------------ ROMS ------------ */

/* external floppy controllers */
#define ROM_FLOPPY( base )						\
     /* no controller */						\
     ROM_FILL( base, 0x800, 0x39 )					\
     /* CD 90-640 (5"1/4) */						\
     ROM_LOAD ( "cd90-640.rom", base+0x800, 0x7c0,			\
		CRC(5114c0a5)						\
		MD5(e5b609dd90b26c9974a46cd54eab4216) )			\
     /* CD 90-351 (3"1/2) */						\
     ROM_LOAD ( "cd-351-0.rom", base+0x1000, 0x7c0,			\
		CRC(2c0159fd)						\
		MD5(74b61b2905e355419c497b5520706aa0) )			\
     ROM_LOAD ( "cd-351-1.rom", base+0x1800, 0x7c0,			\
		CRC(8e58d159)						\
		MD5(e2a2cd1be213bc340bb8e9ebbed95d0d) )			\
     ROM_LOAD ( "cd-351-2.rom", base+0x2000, 0x7c0,			\
		CRC(c9228b60)						\
		MD5(087e52f007dd24a385adea39f80682a3) )			\
     ROM_LOAD ( "cd-351-3.rom", base+0x2800, 0x7c0,			\
		CRC(3ca8e5dc)						\
		MD5(e01dceb2295fa548a4f74cf0487c3f6d) )			\
     /* CQ 90-028 (2"8, aka QDD) */					\
     ROM_LOAD ( "cq90-028.rom", base+0x3000, 0x7c0,			\
		CRC(ca4dba3d)						\
		MD5(0f58e167bf6ebcd2cbba946be2084fbe) )			
     
/* external floppy / network controller: 8 banks */     
#define ROM_FLOPPY5( base )				\
  ROM_FLOPPY( base )					\
  ROM_LOAD ( "nano5.rom", base+0x3800, 0x7c0,	\
	     CRC(2f756868)				\
	     MD5(06ff309276d4fc656e99a8ad1ca67899) )

#define ROM_FLOPPY7( base )				\
  ROM_FLOPPY( base )					\
  ROM_LOAD ( "nano7.rom", base+0x3800, 0x7c0,	\
	     CRC(42a1d1a6)				\
	     MD5(77da8cfc9e0a14ef2ed7034f5941b542) )
     

ROM_START ( to7 )
     ROM_REGION ( 0x28000, REGION_CPU1, 0 )
     ROM_LOAD ( "to7.rom", 0xe800, 0x1800,
		CRC(0e7826da)
		MD5(5bf18521bf35293de942645f690b2845) )
     ROM_FILL ( 0x10000, 0x10000, 0x39 )
     ROM_FLOPPY7 ( 0x24000 )
ROM_END

ROM_START ( t9000 )
     ROM_REGION ( 0x28000, REGION_CPU1, 0 )
     ROM_LOAD ( "t9000.rom", 0xe800, 0x1800,
		CRC(daa8cfbf)
		MD5(b3007f26e7b621c1a4f0fd2c287f80b9) )
     ROM_FILL ( 0x10000, 0x10000, 0x39 )
     ROM_FLOPPY7 ( 0x24000 )
ROM_END


/* ------------ inputs   ------------ */

INPUT_PORTS_START ( to7_config )
     PORT_START_TAG ( "config" )

     PORT_CONFNAME ( 0x01, 0x00, "Game Port" )
     PORT_CONFSETTING ( 0x00, DEF_STR( Joystick ) )
     PORT_CONFSETTING ( 0x01, "Mouse" )

INPUT_PORTS_END

INPUT_PORTS_START ( to7_vconfig )
     PORT_START_TAG ( "vconfig" )

     PORT_CONFNAME ( 0x03, 0x00, "Border" )
     PORT_CONFSETTING ( 0x00, "Normal (56x47)" )
     PORT_CONFSETTING ( 0x01, "Small (16x16)" )
     PORT_CONFSETTING ( 0x02, DEF_STR ( None ) )
  
     PORT_CONFNAME ( 0x0c, 0x08, "Resolution" )
     PORT_CONFSETTING ( 0x00, DEF_STR ( Low ) )
     PORT_CONFSETTING ( 0x04, DEF_STR ( High  ) )
     PORT_CONFSETTING ( 0x08, "Auto"  )

INPUT_PORTS_END

INPUT_PORTS_START ( to7_mconfig )
     PORT_START_TAG ( "mconfig" )

     PORT_CONFNAME ( 0x01, 0x01, "E7FE-F port" )
     PORT_CONFSETTING ( 0x00, "Modem (unemulated)" )
     PORT_CONFSETTING ( 0x01, "Speech" )

INPUT_PORTS_END

INPUT_PORTS_START ( to7_fconfig )
     PORT_START_TAG ( "fconfig" )

     PORT_CONFNAME ( 0x07, 0x02, "Floppy (reset)" )
     PORT_CONFSETTING ( 0x00, DEF_STR ( None ) )
     PORT_CONFSETTING ( 0x01, "CD 90-640 (5\"1/4)" )
     PORT_CONFSETTING ( 0x02, "CD 90-351 (3\"1/2)" )
     PORT_CONFSETTING ( 0x03, "CQ 90-028 (2\"8 QDD)" )
     PORT_CONFSETTING ( 0x04, "Network" )

     PORT_CONFNAME ( 0xf8, 0x08, "Network ID" )
     PORT_CONFSETTING ( 0x00, "0 (Master)" )
     PORT_CONFSETTING ( 0x08, "1" )
     PORT_CONFSETTING ( 0x10, "2" )
     PORT_CONFSETTING ( 0x18, "3" )
     PORT_CONFSETTING ( 0x20, "4" )
     PORT_CONFSETTING ( 0x28, "5" )
     PORT_CONFSETTING ( 0x30, "6" )
     PORT_CONFSETTING ( 0x38, "7" )
     PORT_CONFSETTING ( 0x40, "8" )
     PORT_CONFSETTING ( 0x48, "9" )
     PORT_CONFSETTING ( 0x50, "10" )
     PORT_CONFSETTING ( 0x58, "11" )
     PORT_CONFSETTING ( 0x60, "12" )
     PORT_CONFSETTING ( 0x68, "13" )
     PORT_CONFSETTING ( 0x70, "14" )
     PORT_CONFSETTING ( 0x78, "15" )
     PORT_CONFSETTING ( 0x80, "16" )
     PORT_CONFSETTING ( 0x88, "17" )
     PORT_CONFSETTING ( 0x90, "18" )
     PORT_CONFSETTING ( 0x98, "19" )
     PORT_CONFSETTING ( 0xa0, "20" )
     PORT_CONFSETTING ( 0xa8, "21" )
     PORT_CONFSETTING ( 0xb0, "22" )
     PORT_CONFSETTING ( 0xb8, "23" )
     PORT_CONFSETTING ( 0xc0, "24" )
     PORT_CONFSETTING ( 0xc8, "25" )
     PORT_CONFSETTING ( 0xd0, "26" )
     PORT_CONFSETTING ( 0xd8, "27" )
     PORT_CONFSETTING ( 0xe0, "28" )
     PORT_CONFSETTING ( 0xe8, "29" )
     PORT_CONFSETTING ( 0xf0, "30" )
     PORT_CONFSETTING ( 0xf8, "31" )

INPUT_PORTS_END


INPUT_PORTS_START ( to7_keyboard )
     PORT_START_TAG ( "keyboard_0" )
     KEY ( 0, "SHIFT", LSHIFT ) PORT_CODE ( KEYCODE_RSHIFT )
     PORT_BIT  ( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )
     PORT_START_TAG ( "keyboard_1" )
     KEY ( 0, "W", W )
     KEY ( 1, "UP", UP )
     KEY ( 2, "C \303\247", C )
     KEY ( 3, "CLEAR", ESC )
     KEY ( 4, "ENTER", ENTER )
     KEY ( 5, "CONTROL", LCONTROL )
     KEY ( 6, "ACCENT", END )
     KEY ( 7, "STOP", TAB )
     PORT_START_TAG ( "keyboard_2" )
     KEY ( 0, "X", X )
     KEY ( 1, "LEFT", LEFT )
     KEY ( 2, "V", V )
     KEY ( 3, "Q", Q )
     KEY ( 4, "* :", QUOTE )
     KEY ( 5, "A", A )
     KEY ( 6, "+ ;", EQUALS )
     KEY ( 7, "1 !", 1 )
     PORT_START_TAG ( "keyboard_3" )
     KEY ( 0, "SPACE CAPS-LOCK", SPACE )
     KEY ( 1, "DOWN", DOWN )
     KEY ( 2, "B", B )
     KEY ( 3, "S", S )
     KEY ( 4, "/ ?", SLASH )
     KEY ( 5, "Z \305\223", Z)
     KEY ( 6, "- =", MINUS )
     KEY ( 7, "2 \" \302\250", 2 )
     PORT_START_TAG ( "keyboard_4" )
     KEY ( 0, "@ \342\206\221", TILDE )
     KEY ( 1, "RIGHT", RIGHT )
     KEY ( 2, "M", M )
     KEY ( 3, "D", D )
     KEY ( 4, "P", P )
     KEY ( 5, "E", E )
     KEY ( 6, "0 \140", 0 )
     KEY ( 7, "3 #", 3 )
     PORT_START_TAG ( "keyboard_5" )
     KEY ( 0, ". >", STOP )
     KEY ( 1, "HOME", HOME )
     KEY ( 2, "L", L )
     KEY ( 3, "F", F )
     KEY ( 4, "O", O )
     KEY ( 5, "R", R )
     KEY ( 6, "9 )", 9 )
     KEY ( 7, "4 $", 4 )
     PORT_START_TAG ( "keyboard_6" )
     KEY ( 0, ", <", COMMA )
     KEY ( 1, "INSERT", INSERT )
     KEY ( 2, "K", K )
     KEY ( 3, "G", G )
     KEY ( 4, "I", I )
     KEY ( 5, "T", T )
     KEY ( 6, "8 (", 8 )
     KEY ( 7, "5 %", 5 )
     PORT_START_TAG ( "keyboard_7" )
     KEY ( 0, "N", N )
     KEY ( 1, "DELETE", DEL )
     KEY ( 2, "J \305\222", J )
     KEY ( 3, "H \302\250", H )
     KEY ( 4, "U", U )
     KEY ( 5, "Y", Y )
     KEY ( 6, "7 ' \302\264", 7 )
     KEY ( 7, "6 &", 6 )

     /* unused */
     PORT_START_TAG ( "keyboard_8" )
     PORT_START_TAG ( "keyboard_9" )

INPUT_PORTS_END

INPUT_PORTS_START ( to7 )
     PORT_INCLUDE ( thom_lightpen )
     PORT_INCLUDE ( thom_game_port )
     PORT_INCLUDE ( to7_keyboard )
     PORT_INCLUDE ( to7_config )
     PORT_INCLUDE ( to7_fconfig )
     PORT_INCLUDE ( to7_vconfig )
     PORT_INCLUDE ( to7_mconfig )
INPUT_PORTS_END

INPUT_PORTS_START ( t9000 )
     PORT_INCLUDE ( to7 )
INPUT_PORTS_END


/* ------------ config ------------ */

SYSTEM_CONFIG_START ( to7 )
     CONFIG_DEVICE ( to7_cartridge_getinfo )
     CONFIG_DEVICE ( to7_cassette_getinfo )
     CONFIG_DEVICE ( thom_floppy_getinfo )
     CONFIG_DEVICE ( thom_printer_getinfo )
     CONFIG_DEVICE ( thom_serial_getinfo )
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START ( t9000 )
     CONFIG_IMPORT_FROM ( to7 )
SYSTEM_CONFIG_END


/* ------------ driver ------------ */

static MACHINE_DRIVER_START ( to7 )

     MDRV_MACHINE_START ( to7 )     
     MDRV_MACHINE_RESET ( to7 )     
  
/* cpu */
     MDRV_CPU_ADD_TAG ( "main", M6809, 1000000 )
     MDRV_CPU_PROGRAM_MAP ( to7, 0 )
  
/* video */
/* Finally, I figured it out. The video hardware overclocks the SECAM 
   framerate from 50 Hz to 1/0.019968 Hz to get 312 64us lines per frame,
   i.e., 19.968 ms per frame, not 20 ms
*/
     MDRV_SCREEN_REFRESH_RATE ( /*50*/ 1./0.019968 )
     MDRV_INTERLEAVE ( 0 )
     MDRV_VIDEO_ATTRIBUTES ( VIDEO_TYPE_RASTER )
     MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
     MDRV_SCREEN_SIZE ( THOM_TOTAL_WIDTH * 2, THOM_TOTAL_HEIGHT )
     MDRV_SCREEN_VISIBLE_AREA ( 0, THOM_TOTAL_WIDTH * 2 - 1, 
				0, THOM_TOTAL_HEIGHT - 1 )
     MDRV_PALETTE_LENGTH ( 4097 ) /* 12-bit color + transparency */
     MDRV_PALETTE_INIT ( thom )
     MDRV_VIDEO_START ( thom )
     MDRV_VIDEO_UPDATE ( thom )
     MDRV_VIDEO_EOF ( thom )
     MDRV_DEFAULT_LAYOUT( layout_thomson )

/* sound */
     MDRV_SPEAKER_STANDARD_MONO("mono")
     MDRV_SOUND_ADD ( DAC, 0 )
     MDRV_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.) /* 1-bit buzzer */
     MDRV_SOUND_ADD ( DAC, 0 )
     MDRV_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.) /* 6-bit game extention DAC */
     MDRV_SOUND_ADD ( DAC, 0 )
     MDRV_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.) /* speech synthesis */

MACHINE_DRIVER_END

static MACHINE_DRIVER_START ( t9000 )
     MDRV_IMPORT_FROM ( to7 )
MACHINE_DRIVER_END


COMP ( 1982, to7, 0, 0, to7, to7, NULL, to7, "Thomson", "TO7", 
       0 /*GAME_SUPPORTS_SAVE*/ )

COMP ( 1980, t9000, to7, 0, t9000, t9000, NULL, t9000, "Thomson", "T9000", 
       0 /*GAME_SUPPORTS_SAVE*/ )


/***************************** TO7/70 *********************************

TO7/70 ( 1984)
------

Enhanced TO7.
The TO7/70 supports virtually all TO7 software and most TO7 devices and
extensions (floppy, game, communucation, etc.).
As the TO7, it is only usable with a cartridge, and most software require
the BASIC 1.0 cartridge to be present.
Though, you may also use the more advanced BASIC 128 (-cart basic128.m7): 
it allows BASIC programs to access all the memory and the video capabilities, 
includes its own DOS (no need for a boot disk), but may not be compatible 
with all games.

It has the following modifications:

* chips:
  - custom logics for video, lightpen, address map has been replaced with an 
    integrated Gate-Array (Motorola MC 1300 ALS)

* memory:
  - 48 KB user base RAM (16 KB unswitchable + 2 switchable banks of 16 KB)
    + 64 KB user extended RAM (EM 97-064, as 4 extra 16 KB banks) 
    = 112 KB total user RAM emulated
  - now 8-bit x 8 K color RAM (instead of 6-bit x 8 K)

* video:
  - 16-color fixed palette instead of 8-color (but same constraints)
  - IN 57-001: TV overlay extension, not implemented
    (black becomes transparent and shows the TV image)

* devices:
  - lightpen management has changed, it now has 1-pixel horizontal resolution
  - keyboard management has changed (but the keys are the same)


TO7/70 arabic (198?)
-------------

TO7/70 with an alternate ROM.
Together with a special (64 KB) BASIC 128 cartridge (-cart basic128a.m7),
it allows typing in arabic.
Use Ctrl+W to switch to arabic, and Ctrl+F to switch back to latin.
In latin mode, Ctrl+U / Ctrl+X to start / stop typing in-line arabic.
In arabic mode, Ctrl+E / Ctrl+X to start / stop typing in-line latin.

**********************************************************************/

static ADDRESS_MAP_START ( to770, ADDRESS_SPACE_PROGRAM, 8 )

     AM_RANGE ( 0x0000, 0x3fff ) AM_ROMBANK   ( THOM_CART_BANK ) /* 4 * 16 KB */
                                 AM_WRITE     ( to7_cartridge_w )
     AM_RANGE ( 0x4000, 0x5fff ) AM_RAMBANK   ( THOM_VRAM_BANK )
                                 AM_WRITE     ( to770_vram_w ) 
     AM_RANGE ( 0x6000, 0x9fff ) AM_RAM       /* base user memory 16 KB */
     AM_RANGE ( 0xa000, 0xdfff ) AM_RAMBANK   ( THOM_RAM_BANK )/* 6 * 16 KB */
     AM_RANGE ( 0xe000, 0xe7bf ) AM_ROMBANK   ( THOM_FLOP_BANK )
     AM_RANGE ( 0xe7c0, 0xe7c7 ) AM_READWRITE ( mc6846_r, mc6846_w )
     AM_RANGE ( 0xe7c8, 0xe7cb ) AM_READWRITE ( pia_0_r,  pia_0_w )
     AM_RANGE ( 0xe7cc, 0xe7cf ) AM_READWRITE ( pia_1_r,  pia_1_w )
     AM_RANGE ( 0xe7d0, 0xe7df ) AM_READWRITE ( to7_floppy_r, to7_floppy_w )
     AM_RANGE ( 0xe7e0, 0xe7e3 ) AM_READWRITE ( pia_2_r, pia_2_w )
     AM_RANGE ( 0xe7e4, 0xe7e7 ) AM_READWRITE ( to770_gatearray_r, 
                                                to770_gatearray_w )
     AM_RANGE ( 0xe7e8, 0xe7eb ) AM_READWRITE ( acia_6551_r, acia_6551_w )
     AM_RANGE ( 0xe7f2, 0xe7f3 ) AM_READWRITE ( to7_midi_r, to7_midi_w )
     AM_RANGE ( 0xe7f8, 0xe7fb ) AM_READWRITE ( pia_3_r, pia_3_w )
     AM_RANGE ( 0xe7fe, 0xe7ff ) AM_READWRITE ( to7_modem_mea8000_r, 
						to7_modem_mea8000_w )
     AM_RANGE ( 0xe800, 0xffff ) AM_ROM       /* system bios  */

/* 0x10000 - 0x1ffff: 64 KB external ROM cartridge */
/* 0x20000 - 0x37fff: 96 KB banked RAM */
/* 0x38000 - 0x3bfff: 16 KB video RAM  */
/* 0x3c000 - 0x3ffff: 16 KB floppy ROM controllers */

ADDRESS_MAP_END



/* ------------ ROMS ------------ */

ROM_START ( to770 )
     ROM_REGION ( 0x40000, REGION_CPU1, 0 )
     ROM_LOAD ( "to770.rom", 0xe800, 0x1800, /* BIOS */
		CRC(89518862)
		MD5(61402c35b75faeb4b74b815f323fff3d) )
     ROM_FLOPPY7 ( 0x3c000 )
     ROM_FILL ( 0x10000, 0x10000, 0x39 )
ROM_END

ROM_START ( to770a )
     ROM_REGION ( 0x40000, REGION_CPU1, 0 )
     ROM_LOAD ( "to770a.rom", 0xe800, 0x1800,
		CRC(378ea808)
		MD5(6b63aa135107beee243967a2da0e5453) )
     ROM_FLOPPY7 ( 0x3c000 )
     ROM_FILL ( 0x10000, 0x10000, 0x39 )
ROM_END


/* ------------ inputs   ------------ */

INPUT_PORTS_START ( to770 )
     PORT_INCLUDE ( to7 )

     PORT_MODIFY ( "keyboard_1" )
     KEY ( 2, "C \302\250 \303\247", C )
     PORT_MODIFY ( "keyboard_4" )
     KEY ( 6, "0 \303\240 \140", 0 )
     PORT_MODIFY ( "keyboard_5" )
     KEY ( 6, "9 ) \303\247", 9 )
     PORT_MODIFY ( "keyboard_6" )
     KEY ( 6, "8 ( \303\271", 8 )
     PORT_MODIFY ( "keyboard_7" )
     KEY ( 6, "7 ' \303\250 \302\264", 7 )
     KEY ( 7, "6 & \303\251", 6 )

INPUT_PORTS_END

/* arabic version (QWERTY keyboard) */
INPUT_PORTS_START ( to770a )
     PORT_INCLUDE ( to770 )

     PORT_MODIFY ( "keyboard_1" )
     KEY ( 0, "Z", Z )
     PORT_MODIFY ( "keyboard_2" )
     KEY ( 3, "A", A )
     KEY ( 5, "Q", Q )
     PORT_MODIFY ( "keyboard_3" )
     KEY ( 5, "W", W)
     PORT_MODIFY ( "keyboard_4" )
     KEY ( 0, ". >", STOP )
     KEY ( 2, "@ \342\206\221", TILDE )
     KEY ( 6, "0 \302\243 \302\260 \140", 0 )
     PORT_MODIFY ( "keyboard_5" )
     KEY ( 0, ", <", COMMA )
     KEY ( 6, "9 ) \303\261", 9 )
     PORT_MODIFY ( "keyboard_6" )
     KEY ( 0, "M", M )
     KEY ( 6, "8 ( \303\274", 8 )
     PORT_START_TAG ( "keyboard_7" )
     KEY ( 6, "7 ' \303\266 \302\264", 7 )
     KEY ( 7, "6 & \303\244", 6 )

INPUT_PORTS_END


/* ------------ config ------------ */

SYSTEM_CONFIG_START ( to770 )
     CONFIG_IMPORT_FROM ( to7 )
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START ( to770a )
     CONFIG_IMPORT_FROM ( to770 )
SYSTEM_CONFIG_END


/* ------------ driver ------------ */

static MACHINE_DRIVER_START ( to770 )
     MDRV_IMPORT_FROM ( to7 )
     MDRV_MACHINE_START ( to770 )
     MDRV_MACHINE_RESET ( to770 )
     MDRV_CPU_MODIFY( "main" )
     MDRV_CPU_PROGRAM_MAP ( to770, 0 )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START ( to770a )
     MDRV_IMPORT_FROM ( to770 )
MACHINE_DRIVER_END

COMP ( 1984, to770, 0, 0, to770, to770, NULL, to770, "Thomson", "TO7/70", 
       0 /*GAME_SUPPORTS_SAVE*/ )

COMP ( 1984, to770a, to770, 0, to770a, to770a, NULL, to770a, "Thomson", 
       "TO7/70 arabic", 
       0 /*GAME_SUPPORTS_SAVE*/ )


/************************* MO5 / MO5E *********************************

MO5 (1984)
---

The MO5 is Thomson's attempt to provide a less costly micro-computer, using
the same technology as the TO7/70.
It has less memory and is less expandable. The MC6846 timer has disapeared.
The BIOS has been throughly rewritten and uses a more compact call scheme.
This, and the fact that the address map has changed, makes the MO5 completely
TO7 software incompatible (except for pure BASIC programs). 
Moreover, the MO5 has incompatible cassette and cartridge formats.
Unlike the TO7, the BASIC 1.0 is integrated and the MO5 can be used "as-is".

* chips:
  - 1 MHz Motorola 6809E CPU
  - 1 Motorola 6821 PIA (+3 for I/O, game, and modem extensions)
  - Motorola 1300 ALS Gate-Array

* memory:
  - 32 KB base user RAM
  - 64 KB extended user RAM (4 x 16 KB banks) with the network extension
  - 16 KB combined BASIC and BIOS ROM
  - 8 KB color RAM + 8 KB point RAM, bank switched
  - 2 to 8 KB floppy ROM comes with the floppy drive / network extension

* video: 
  - as the TO7/70 but with different color encoding,
    320x200 pixels with color constraints, 16-color fixed palette
  - IN 57-001: TV overlay extension (not implemented)

* devices:
  - AZERTY keyboard, 58-keys, slightlty different from the TO7
    . the right SHIFT key has been replaced with a BASIC key
    . no caps-lock led
  - the famous lightpen is optional
  - cassette 1200 bauds (frequency signals: 0=4.5Khz, 1=6.3KHz),
    TO7-incompatible
  - optional cartridge, up to 64 KB, incompatible with TO7,
    masks the integrated BASIC ROM
  - game & music, I/O, floppy, network extensions: identical to TO7
  - speech synthesis extension: identical to TO7
  - MIDIPAK MIDI extension: identical to TO7

MO5E (1986)
----

This is a special MO5 version for the export market (mainly Germany).
Although coming in a different (nicer) case, it is internally similar to
the MO5 and is fully compatible.
Differences include:
 - much better keyboard; some are QWERTY instead of AZERTY (we emulate QWERTY)
 - a different BIOS and integrated BASIC
 - the game extension is integrated


**********************************************************************/

static ADDRESS_MAP_START ( mo5, ADDRESS_SPACE_PROGRAM, 8 )

     AM_RANGE ( 0x0000, 0x1fff ) AM_RAMBANK   ( THOM_VRAM_BANK )
                                 AM_WRITE     ( to770_vram_w ) 
     AM_RANGE ( 0x2000, 0x9fff ) AM_RAM       /* base user memory 32 KB */
     AM_RANGE ( 0xa000, 0xa7bf ) AM_ROMBANK   ( THOM_FLOP_BANK )
     AM_RANGE ( 0xa7c0, 0xa7c3 ) AM_READWRITE ( pia_0_r,  pia_0_w )
     AM_RANGE ( 0xa7cb, 0xa7cb ) AM_WRITE     ( mo5_ext_w )
     AM_RANGE ( 0xa7cc, 0xa7cf ) AM_READWRITE ( pia_1_r,  pia_1_w )
     AM_RANGE ( 0xa7d0, 0xa7df ) AM_READWRITE ( to7_floppy_r, to7_floppy_w )
     AM_RANGE ( 0xa7e0, 0xa7e3 ) AM_READWRITE ( pia_2_r, pia_2_w )
     AM_RANGE ( 0xa7e4, 0xa7e7 ) AM_READWRITE ( mo5_gatearray_r,
						mo5_gatearray_w )
     AM_RANGE ( 0xa7e8, 0xa7eb ) AM_READWRITE ( acia_6551_r, acia_6551_w )
     AM_RANGE ( 0xa7f2, 0xa7f3 ) AM_READWRITE ( to7_midi_r, to7_midi_w )
     AM_RANGE ( 0xa7fe, 0xa7ff ) AM_READWRITE ( mea8000_r, mea8000_w )
     AM_RANGE ( 0xb000, 0xefff ) AM_ROMBANK   ( THOM_CART_BANK )
                                 AM_WRITE     ( mo5_cartridge_w )
     AM_RANGE ( 0xf000, 0xffff ) AM_ROM       /* system bios */

/* 0x10000 - 0x1ffff: 16 KB integrated BASIC / 64 KB external cartridge */
/* 0x20000 - 0x2ffff: 64 KB banked RAM extension */
/* 0x30000 - 0x33fff: 16 KB video RAM  */
/* 0x34000 - 0x3ffff: 16 KB floppy ROM controllers */

ADDRESS_MAP_END



/* ------------ ROMS ------------ */

ROM_START ( mo5 )
     ROM_REGION ( 0x38000, REGION_CPU1, 0 )
     ROM_LOAD ( "mo5.rom", 0xf000, 0x1000,
		CRC(f0ea9140)
		MD5(ab3533a7132f90933acce80e256ae459) )
     ROM_LOAD ( "basic5.rom", 0x11000, 0x3000,
		CRC(c2c11b9d)
		MD5(f992a912093d3e8f165f225f74345b57) )
     ROM_FLOPPY5 ( 0x34000 )
ROM_END

ROM_START ( mo5e )
     ROM_REGION ( 0x38000, REGION_CPU1, 0 )
     ROM_LOAD ( "mo5e.rom", 0xf000, 0x1000,
		CRC(6520213a)
		MD5(434c42b96c31a341e13085048cdc8eae) )
     ROM_LOAD ( "basic5e.rom", 0x11000, 0x3000,
		CRC(934a72b2)
		MD5(6404a7f49ec28937decd905d2a3cbb28) )
     ROM_FLOPPY5 ( 0x34000 )
ROM_END


/* ------------ inputs  ------------ */

INPUT_PORTS_START ( mo5 )
     PORT_INCLUDE ( to770 )

     PORT_MODIFY ( "keyboard_0" )
     KEY ( 1, "BASIC", RCONTROL)
     PORT_BIT  ( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

INPUT_PORTS_END

/* QWERTY version */
INPUT_PORTS_START ( mo5e )
     PORT_INCLUDE ( to770a )

     PORT_MODIFY ( "keyboard_0" )
     KEY ( 1, "BASIC", RCONTROL)
     PORT_BIT  ( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

INPUT_PORTS_END


/* ------------ config ------------ */

SYSTEM_CONFIG_START ( mo5 )
     CONFIG_DEVICE ( mo5_cartridge_getinfo )
     CONFIG_DEVICE ( mo5_cassette_getinfo )
     CONFIG_DEVICE ( thom_floppy_getinfo )
     CONFIG_DEVICE ( thom_printer_getinfo )
     CONFIG_DEVICE ( thom_serial_getinfo )
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START ( mo5e )
     CONFIG_IMPORT_FROM ( mo5 )
SYSTEM_CONFIG_END


/* ------------ driver ------------ */

static MACHINE_DRIVER_START ( mo5 )
     MDRV_IMPORT_FROM ( to7 )
     MDRV_MACHINE_START ( mo5 )
     MDRV_MACHINE_RESET ( mo5 )
     MDRV_CPU_MODIFY( "main" )
     MDRV_CPU_PROGRAM_MAP ( mo5, 0 )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START ( mo5e )
     MDRV_IMPORT_FROM ( mo5 )
MACHINE_DRIVER_END


COMP ( 1984, mo5, 0, 0, mo5, mo5, NULL, mo5, "Thomson", "MO5", 
       0 /*GAME_SUPPORTS_SAVE*/ )

COMP ( 1986, mo5e, mo5, 0, mo5e, mo5e, NULL, mo5e, "Thomson", "MO5E",
       0 /*GAME_SUPPORTS_SAVE*/ )


/********************************* TO9 *******************************

TO9 (1985)
---

The TO9 is the successor of the TO7/70.
It is a high-end product: it integrates 96 KB of base RAM, 128 KB of
software in ROM, a floppy drive. It has improved graphics capabilities
(several video modes, a palette of 4096 colors, thanks to the use of
a dedicated video gate-array).
The ROM contains the old BASIC 1.0 for compatibility and the newer BASIC 128.
It has a more professional, desktop look, with a separate keyboard, and an
optional mouse.
It is also quite compatible with the TO7 and TO7/70 (but not the MO5).
However, it also has many problems. The integrated BASIC 128 can only access
128 KB of memory, which forces the 64 KB extension to be managed as a virtual 
disk. The early versions of the software ROM has many bugs. The integrated 
floppy drive is one-sided.
It was replaced quickly with the improved TO9+.

* chips:
  - 1 MHz Motorola 6809E CPU
  - 1 Motorola 6821 PIA (+2 for game, modem extensions)
  - 1 Motorola 6846 timer, PIA
  - 1 Motorola 6805 + 1 Motorola 6850 (keyboard & mouse control)
  - 1 Western Digital 2793 (disk controller)
  - 3 gate-arrays (address decoding, system, video)

* memory:
  - 112 KB base RAM
  - 64 KB extension RAM (as virtual disk)
  - 6 KB BIOS ROM + 2 KB floppy BIOS
  - 128 KB software ROM (BASIC 1, BASIC 128, extended BIOS, 
    DOS and configuration GUI, two software: "Paragraphe" and
    "Fiches et dossiers")
  - 16 KB video RAM

* video:
  - 8 video modes:
    o 320x200, 16 colors with constraints (TO7/70 compatible)
    o 320x200, 4 colors without constraints
    o 160x200, 16 colors without constraints
    o 640x200, 2 colors
    o 320x200, 2 colors, two pages
       . page one
       . page two
       . pages overlaid
    o 160x200, 2 colors, four pages overlaid
  - palette: 16 colors can be chosen among 4096

* devices:
  - AZERTY keyboard, 81-keys, French with accents, keypad & function keys
  - cartridge, up to 64 KB, TO7 compatible
  - two-button mouse connected to the keyboard (not working yet)
  - lightpen, with 1-pixel vertical and horizontal resolution
  - 1-bit internal buzzer
  - cassette 900 bauds, TO7 compatible
  - integrated parallel CENTRONICS (printer emulated)
  - SX 90-018: game extension (identical to the TO7)
  - RF 57-932: RS232 extension (identical to the TO7)
  - MD 90-120: MODEM extension (identical to the TO7)
  - IEEE extension ? (unemulated)
  - floppy:
    . integrated floppy controller, based on WD2793
    . integrated one-sided double-density 3"1/2
    . external two-sided double-density 3"1/2, 5"1/4 or QDD (extension)
    . floppies are TO7 and MO5 compatible
  - speech synthesis extension: identical to TO7
  - MIDIPAK MIDI extension: identical to TO7

**********************************************************************/

static ADDRESS_MAP_START ( to9, ADDRESS_SPACE_PROGRAM, 8 )

     AM_RANGE ( 0x0000, 0x3fff ) AM_ROMBANK   ( THOM_CART_BANK )/* 12 * 16 KB */
                                 AM_WRITE     ( to9_cartridge_w )
     AM_RANGE ( 0x4000, 0x5fff ) AM_RAMBANK   ( THOM_VRAM_BANK )
                                 AM_WRITE     ( to770_vram_w ) 
     AM_RANGE ( 0x6000, 0x9fff ) AM_RAM       /* unbanked memory 16 KB */
     AM_RANGE ( 0xa000, 0xdfff ) AM_RAMBANK   ( THOM_RAM_BANK )/* 10 * 16 KB */
     AM_RANGE ( 0xe000, 0xe7af ) AM_ROM       /* floppy BIOS */
     AM_RANGE ( 0xe7c0, 0xe7c7 ) AM_READWRITE ( mc6846_r, mc6846_w )
     AM_RANGE ( 0xe7c8, 0xe7cb ) AM_READWRITE ( pia_0_r,  pia_0_w )
     AM_RANGE ( 0xe7cc, 0xe7cf ) AM_READWRITE ( pia_1_r,  pia_1_w )
     AM_RANGE ( 0xe7d0, 0xe7d9 ) AM_READWRITE ( to9_floppy_r, to9_floppy_w )
     AM_RANGE ( 0xe7da, 0xe7dd ) AM_READWRITE ( to9_vreg_r, to9_vreg_w )
     AM_RANGE ( 0xe7de, 0xe7df ) AM_READWRITE ( to9_kbd_r, to9_kbd_w )
     AM_RANGE ( 0xe7e4, 0xe7e7 ) AM_READWRITE ( to9_gatearray_r, 
						to9_gatearray_w )
     AM_RANGE ( 0xe7e8, 0xe7eb ) AM_READWRITE ( acia_6551_r, acia_6551_w )
/*   AM_RANGE ( 0xe7f0, 0xe7f7 ) AM_READWRITE ( to9_ieee_r, to9_ieee_w ) */
     AM_RANGE ( 0xe7f2, 0xe7f3 ) AM_READWRITE ( to7_midi_r, to7_midi_w )
     AM_RANGE ( 0xe7f8, 0xe7fb ) AM_READWRITE ( pia_3_r, pia_3_w )
     AM_RANGE ( 0xe7fe, 0xe7ff ) AM_READWRITE ( to7_modem_mea8000_r, 
						to7_modem_mea8000_w )
     AM_RANGE ( 0xe800, 0xffff ) AM_ROM       /* system bios  */

/* 0x10000 - 0x1ffff:  64 KB external ROM cartridge */
/* 0x20000 - 0x3ffff: 128 KB internal software ROM */
/* 0x40000 - 0x67fff: 160 KB banked RAM  */
/* 0x68000 - 0x6bfff:  16 KB video RAM  */

ADDRESS_MAP_END



/* ------------ ROMS ------------ */

/* NOT WORKING
   these bios seem heavily patched (probably to work with specific emulators 
   that trap some bios calls)
 */

ROM_START ( to9 )
     ROM_REGION ( 0x6c000, REGION_CPU1, 0 )
     ROM_LOAD ( "to9.rom", 0xe000, 0x2000, /* BIOS & floppy controller */
		CRC(f9278bf7)
		MD5(507f0c482462b70b816fd23cf6791179) )

     /* BASIC & software */
     ROM_LOAD ( "basic9-0.rom", 0x20000, 0x4000,
		CRC(c7bac620)
		MD5(095d0f94ab506ab5da1e73ae550ee2bc) )
     ROM_LOAD ( "basic9-1.rom", 0x24000, 0x4000,
		CRC(ea5f3e43)
		MD5(ce29a894c31db083acf6bdf015a02b5e) )
     ROM_LOAD ( "basic9-2.rom", 0x28000, 0x4000,
		CRC(0f5581b3)
		MD5(02279e22b8bfe29e750fb62671e3951d) )
     ROM_LOAD ( "basic9-3.rom", 0x2c000, 0x4000,
		CRC(6b5b19e3)
		MD5(dac4b33a8cbebf64916b35515e0693c8) )
     ROM_LOAD ( "soft9-0a.rom", 0x30000, 0x4000,
		CRC(8cee157e)
		MD5(3dab712e800351b4f639790595b12101) )
     ROM_LOAD ( "soft9-1a.rom", 0x34000, 0x4000,
		CRC(cf39ac93)
		MD5(bcd744cdbd022cd9e2651e87792cd742) )
     ROM_LOAD ( "soft9-0b.rom", 0x38000, 0x4000,
		CRC(033aee3f)
		MD5(1a4d3549c78328479361e867d240f285) )
     ROM_LOAD ( "soft9-1b.rom", 0x3c000, 0x4000,
		CRC(214fe527)
		MD5(3dc2820fbe7b81a3936f731b44f23cbe) )

     ROM_FILL( 0x10000, 0x10000, 0x39 )
ROM_END


/* ------------ inputs   ------------ */

INPUT_PORTS_START ( to9_keyboard )
     PORT_START_TAG ( "keyboard_0" )
     KEY ( 0, "F2 F7", F2 )
     KEY ( 1, "_ 6", 6 )
     KEY ( 2, "Y", Y )
     KEY ( 3, "H \302\250", H )
     KEY ( 4, "UP", UP )
     KEY ( 5, "RIGHT", RIGHT )
     KEY ( 6, "HOME CLEAR", HOME )
     KEY ( 7, "N", N )
     PORT_START_TAG ( "keyboard_1" )
     KEY ( 0, "F3 F8", F3 )
     KEY ( 1, "( 5", 5 )
     KEY ( 2, "T", T )
     KEY ( 3, "G", G )
     KEY ( 4, "= +", EQUALS )
     KEY ( 5, "LEFT", LEFT )
     KEY ( 6, "INS", INSERT )
     KEY ( 7, "B \302\264", B )
     PORT_START_TAG ( "keyboard_2" )
     KEY ( 0, "F4 F9", F4 )
     KEY ( 1, "' 4", 4 )
     KEY ( 2, "R", R )
     KEY ( 3, "F", F )
     KEY ( 4, "ACCENT", END )
     KEY ( 5, "1-PAD", 1_PAD )
     KEY ( 6, "DELETE BACKSPACE", DEL )
     KEY ( 7, "V", V )
     PORT_START_TAG ( "keyboard_3" )
     KEY ( 0, "F5 F10", F5 )
     KEY ( 1, "\" 3", 3 )
     KEY ( 2, "E", E )
     KEY ( 3, "D", D )
     KEY ( 4, "7-PAD", 7_PAD )
     KEY ( 5, "4-PAD", 4_PAD )
     KEY ( 6, "0-PAD", 0_PAD )
     KEY ( 7, "C \136", C )
     PORT_START_TAG ( "keyboard_4" )
     KEY ( 0, "F1 F6", F1 )
     KEY ( 1, "\303\251 2", 2 )
     KEY ( 2, "Z", Z )
     KEY ( 3, "S", S )
     KEY ( 4, "8-PAD", 8_PAD )
     KEY ( 5, "2-PAD", 2_PAD )
     KEY ( 6, ".-PAD", DEL_PAD )
     KEY ( 7, "X", X )
     PORT_START_TAG ( "keyboard_5" )
     KEY ( 0, "# @", TILDE )
     KEY ( 1, "* 1", 1 )
     KEY ( 2, "A \140", A )
     KEY ( 3, "Q", Q )
     KEY ( 4, "[ {", QUOTE )
     KEY ( 5, "5-PAD", 5_PAD )
     KEY ( 6, "6-PAD", 6_PAD )
     KEY ( 7, "W", W )
     PORT_START_TAG ( "keyboard_6" )
     KEY ( 0, "STOP", TAB )
     KEY ( 1, "\303\250 7", 7 )
     KEY ( 2, "U", U )
     KEY ( 3, "J", J )
     KEY ( 4, "SPACE", SPACE )
     KEY ( 5, "9-PAD", 9_PAD )
     KEY ( 6, "ENTER-PAD", ENTER_PAD )
     KEY ( 7, ", ?", COMMA )
     PORT_START_TAG ( "keyboard_7" )
     KEY ( 0, "CONTROL", LCONTROL )
     KEY ( 1, "! 8", 8 )
     KEY ( 2, "I", I )
     KEY ( 3, "K", K )
     KEY ( 4, "$ &", CLOSEBRACE )
     KEY ( 5, "DOWN", DOWN )
     KEY ( 6, "] }", BACKSLASH )
     KEY ( 7, "; .", STOP )
     PORT_START_TAG ( "keyboard_8" )
     KEY ( 0, "CAPS-LOCK", CAPSLOCK )
     KEY ( 1, "\303\247 9", 9 )
     KEY ( 2, "O", O )
     KEY ( 3, "L", L )
     KEY ( 4, "- \\", BACKSPACE )
     KEY ( 5, "\303\271 %", COLON )
     KEY ( 6, "ENTER", ENTER )
     KEY ( 7, ": /", SLASH )
     PORT_START_TAG ( "keyboard_9" )
     KEY ( 0, "SHIFT", LSHIFT )  PORT_CODE ( KEYCODE_RSHIFT )
     KEY ( 1, "\303\240 0", 0 )
     KEY ( 2, "P", P )
     KEY ( 3, "M", M )
     KEY ( 4, ") \302\260", MINUS )
     KEY ( 5, "\342\206\221 \302\250", OPENBRACE )
     KEY ( 6, "3-PAD", 3_PAD )
     KEY ( 7, "> <", BACKSLASH2 )
INPUT_PORTS_END

INPUT_PORTS_START ( to9_fconfig )
     PORT_START_TAG ( "fconfig" )
INPUT_PORTS_END

INPUT_PORTS_START ( to9 )
     PORT_INCLUDE ( thom_lightpen )
     PORT_INCLUDE ( thom_game_port )
     PORT_INCLUDE ( to9_keyboard )
     PORT_INCLUDE ( to7_config )
     PORT_INCLUDE ( to9_fconfig )
     PORT_INCLUDE ( to7_vconfig )
     PORT_INCLUDE ( to7_mconfig )
INPUT_PORTS_END


/* ------------ config ------------ */

SYSTEM_CONFIG_START ( to9 )
     CONFIG_IMPORT_FROM ( to7 )
SYSTEM_CONFIG_END


/* ------------ driver ------------ */

static MACHINE_DRIVER_START ( to9 )
     MDRV_IMPORT_FROM ( to7 )
     MDRV_MACHINE_START ( to9 )
     MDRV_MACHINE_RESET ( to9 )
     MDRV_CPU_MODIFY( "main" )
     MDRV_CPU_PROGRAM_MAP ( to9, 0 )
MACHINE_DRIVER_END


COMP ( 1985, to9, 0, 0, to9, to9, NULL, to9, "Thomson", "TO9", 
       0 /*GAME_SUPPORTS_SAVE*/ )


/******************************** TO8 ********************************

TO8 (1986)
---

The TO8 was meant to replace the TO7/70 as a home-computer.
It includes and improves on the technology from the TO9 (improved video,
256 KB of RAM fully managed by the new BASIC 512, more integrated gate-array).
It has a more compact Amiga-like look, no separate keyboard, no integrated 
floppy drive (although the controller is integrated), no software in ROM,
less extension slots. Also, the game & music extension is now integrated.
It is quite compatible with the TO7 and TO7/70, and with the TO9 to some
extent.
The TO8 was quite popular and became the de-facto gamming computer in the
Thomson family.

* chips:
  - 1 MHz Motorola 6809E CPU
  - 2 Motorola 6821 PIAs (system, game, +1 in modem extension)
  - 1 Motorola 6846 timer, PIA
  - 1 Motorola 6804 (keyboard)
  - 2 gate-arrays (system & video, floppy controller)

* memory:
  - 256 KB base RAM 
    + 256 KB extended RAM (EM 88-256) = 512 KB total RAM emulated
  - 16 KB BIOS ROM
  - 64 KB software ROM (BASIC 1, BASIC 512, extended BIOS)
  - unified memory view via improved bank switching

* video:
  improved wrt TO9: a 9-th video mode, 4 video pages (shared with main RAM)
  border color has its 4-th index bit inverted

* devices:
  - same keyboard as T09: AZERTY 81-keys 
    (but no 6850 controller, the 6804 is directly connected to the 6821 & 6846)
  - cartridge, up to 64 KB, TO7 compatible
  - two-button serial mouse (TO9-incompatible)
  - lightpen, with 1-pixel vertical and horizontal resolution
  - two 8-position 2-button game pads (SX 90-018 extension integrated)
  - 6-bit DAC sound (NOTE: 1-bit buzzer is gone)
  - cassette 900 bauds, TO7 compatible
  - integrated parallel CENTRONICS (printer emulated)
  - RF 57-932: RS232 extension (identical to the TO7)
  - MD 90-120: MODEM extension (identical to the TO7?)
  - IEEE extension ?
  - floppy:
   . integrated floppy controller, based on custom Thomson gate-array
   . no integrated drive
   . up to two external two-sided double-density 3"1/2, 5"1/4 or QDD drives
   . floppies are TO7 and MO5 compatible
  - speech synthesis extension: identical to TO7
  - MIDIPAK MIDI extension: identical to TO7

TO8D (1987)
----

The TO8D is simply a TO8 with an integrated 3"1/2 floppy drive.

**********************************************************************/


static ADDRESS_MAP_START ( to8, ADDRESS_SPACE_PROGRAM, 8 )

     AM_RANGE ( 0x0000, 0x3fff ) AM_RAMBANK   ( THOM_CART_BANK ) /* 8 * 16 KB */
                                 AM_WRITE     ( to8_cartridge_w )
     AM_RANGE ( 0x4000, 0x5fff ) AM_RAMBANK   ( THOM_VRAM_BANK )
                                 AM_WRITE     ( to770_vram_w ) 
     AM_RANGE ( 0x6000, 0x7fff ) AM_RAMBANK   ( TO8_SYS_LO )
                                 AM_WRITE     ( to8_sys_lo_w )
     AM_RANGE ( 0x8000, 0x9fff ) AM_RAMBANK   ( TO8_SYS_HI )
                                 AM_WRITE     ( to8_sys_hi_w )
     AM_RANGE ( 0xa000, 0xbfff ) AM_RAMBANK   ( TO8_DATA_LO )
                                 AM_WRITE     ( to8_data_lo_w )
     AM_RANGE ( 0xc000, 0xdfff ) AM_RAMBANK   ( TO8_DATA_HI )
                                 AM_WRITE     ( to8_data_hi_w )
     AM_RANGE ( 0xe000, 0xe7af ) AM_ROMBANK   ( TO8_FLOP_BANK ) /* 2 * 2 KB */
     AM_RANGE ( 0xe7c0, 0xe7c7 ) AM_READWRITE ( mc6846_r, mc6846_w )
     AM_RANGE ( 0xe7c8, 0xe7cb ) AM_READWRITE ( pia_0_r,  pia_0_w )
     AM_RANGE ( 0xe7cc, 0xe7cf ) AM_READWRITE ( pia_1_r,  pia_1_w )
     AM_RANGE ( 0xe7d0, 0xe7d9 ) AM_READWRITE ( to8_floppy_r, to8_floppy_w )
     AM_RANGE ( 0xe7da, 0xe7dd ) AM_READWRITE ( to8_vreg_r, to8_vreg_w )
     AM_RANGE ( 0xe7e4, 0xe7e7 ) AM_READWRITE ( to8_gatearray_r, 
						to8_gatearray_w )
     AM_RANGE ( 0xe7e8, 0xe7eb ) AM_READWRITE ( acia_6551_r, acia_6551_w )
/*   AM_RANGE ( 0xe7f0, 0xe7f7 ) AM_READWRITE ( to9_ieee_r, to9_ieee_w ) */
     AM_RANGE ( 0xe7f2, 0xe7f3 ) AM_READWRITE ( to7_midi_r, to7_midi_w )
     AM_RANGE ( 0xe7f8, 0xe7fb ) AM_READWRITE ( pia_3_r, pia_3_w )
     AM_RANGE ( 0xe7fe, 0xe7ff ) AM_READWRITE ( to7_modem_mea8000_r, 
						to7_modem_mea8000_w )
     AM_RANGE ( 0xe800, 0xffff ) AM_ROMBANK   ( TO8_BIOS_BANK ) /* 2 * 6 KB */

/* 0x10000 - 0x1ffff:  64 KB external ROM cartridge */
/* 0x20000 - 0x2ffff:  64 KB internal software ROM */
/* 0x30000 - 0xaffff: 512 KB total RAM (including video) */
/* 0xb0000 - 0xb3fff:  16 KB BIOS ROM */

ADDRESS_MAP_END


/* ------------ ROMS ------------ */

ROM_START ( to8 )
     ROM_REGION ( 0xb4000, REGION_CPU1, 0 )

     /* BIOS & floppy */
     ROM_LOAD ( "to8-0.rom", 0xb0000, 0x2000,
		CRC(3c4a640a)
		MD5(97c9b803305031daf6bffa671f7667c2) )
     ROM_LOAD ( "to8-1.rom", 0xb2000, 0x2000,
		CRC(cb9bae2d)
		MD5(6ca0ebe022f9c433373eaed403aaf0ce) )

     /* BASIC */
     ROM_LOAD ( "basic8-0.rom", 0x20000, 0x4000,
		CRC(e5a00fb3)
		MD5(6c33e3cd79bd6c675634b172594bbfec) )
     ROM_LOAD ( "basic8-1.rom", 0x24000, 0x4000,
		CRC(4b241e63)
		MD5(360cbf42078bf8c80a283828bf6ee4e6) )
     ROM_LOAD ( "basic8-2.rom", 0x28000, 0x4000,
		CRC(0f5581b3)
		MD5(02279e22b8bfe29e750fb62671e3951d) )
     ROM_LOAD ( "basic8-3.rom", 0x2c000, 0x4000,
		CRC(f552e7e3)
		MD5(56c1e929ef4be6771cac7157d169132f) )

     ROM_FILL( 0x10000, 0x10000, 0x39 )
ROM_END

ROM_START ( to8d )
     ROM_REGION ( 0xb4000, REGION_CPU1, 0 )

     /* BIOS & floppy */
     ROM_LOAD ( "to8d-0.rom", 0xb0000, 0x2000,
		CRC(30ea4950)
		MD5(7704c1148b8761e9ce650ef1aa7ded69) )
     ROM_LOAD ( "to8d-1.rom", 0xb2000, 0x2000,
		CRC(926cf0ca)
		MD5(3e202c3914e16c5ede32ab2bd2a61a5e) )

     /* BASIC */
     ROM_LOAD ( "basic8-0.rom", 0x20000, 0x4000,
		CRC(e5a00fb3)
		MD5(6c33e3cd79bd6c675634b172594bbfec) )
     ROM_LOAD ( "basic8-1.rom", 0x24000, 0x4000,
		CRC(4b241e63)
		MD5(360cbf42078bf8c80a283828bf6ee4e6) )
     ROM_LOAD ( "basic8-2.rom", 0x28000, 0x4000,
		CRC(0f5581b3)
		MD5(02279e22b8bfe29e750fb62671e3951d) )
     ROM_LOAD ( "basic8-3.rom", 0x2c000, 0x4000,
		CRC(f552e7e3)
		MD5(56c1e929ef4be6771cac7157d169132f) )

     ROM_FILL( 0x10000, 0x10000, 0x39 )
ROM_END


/* ------------ inputs   ------------ */

INPUT_PORTS_START ( to8_config )
     PORT_START_TAG ( "config" )

     PORT_CONFNAME ( 0x01, 0x00, "Game Port" )
     PORT_CONFSETTING ( 0x00, DEF_STR( Joystick ) )
     PORT_CONFSETTING ( 0x01, "Mouse" )

     PORT_CONFNAME ( 0x02, 0x00, "Keyboard" )
     PORT_CONFSETTING ( 0x00, "Enabled" )
     PORT_CONFSETTING ( 0x02, "Disabled" )

INPUT_PORTS_END


INPUT_PORTS_START ( to8 )
     PORT_INCLUDE ( thom_lightpen )
     PORT_INCLUDE ( thom_game_port )
     PORT_INCLUDE ( to9_keyboard )
     PORT_INCLUDE ( to8_config )
     PORT_INCLUDE ( to9_fconfig )
     PORT_INCLUDE ( to7_vconfig )
     PORT_INCLUDE ( to7_mconfig )
INPUT_PORTS_END


INPUT_PORTS_START ( to8d )
     PORT_INCLUDE ( to8 )
INPUT_PORTS_END


/* ------------ config ------------ */

SYSTEM_CONFIG_START ( to8 )
     CONFIG_IMPORT_FROM ( to7 )
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START ( to8d )
     CONFIG_IMPORT_FROM ( to8 )
SYSTEM_CONFIG_END


/* ------------ driver ------------ */

static MACHINE_DRIVER_START ( to8 )
     MDRV_IMPORT_FROM ( to7 )
     MDRV_MACHINE_START ( to8 )
     MDRV_MACHINE_RESET ( to8 )
     MDRV_CPU_MODIFY( "main" )
     MDRV_CPU_PROGRAM_MAP ( to8, 0 )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START ( to8d )
     MDRV_IMPORT_FROM ( to8 )
MACHINE_DRIVER_END


COMP ( 1986, to8, 0, 0, to8, to8, NULL, to8, "Thomson", "TO8", 
       0 /*GAME_SUPPORTS_SAVE*/ )

COMP ( 1987, to8d, to8, 0, to8d, to8d, NULL, to8d, "Thomson", "TO8D",
       0 /*GAME_SUPPORTS_SAVE*/ )


/******************************** TO9+ *******************************

TO9+ (1986)
----

The TO9+ is the direct successor of the T09 as Thomson's high-end
product: desktop look, 512 KB of RAM, integrated floppy drive and
modem. Some software integrated in ROM on the TO9 are now supplied on 
floppies.
Internally, the TO9+ is based more on TO8 technology than T09 
(same gate-arrays).
It has enhanced communication capabilities by integrating either the 
MODEM or the RS232 extension.
It should be compatible with the TO9 and, to some extent, with the TO7, TO7/70
and TO8.
It uses the same video gate-array and floppy controller.

The differences with the TO8 are:

* chips:
  - 1 Motorola 6805 + 1 Motorola 6850 (keyboard)
  - 3 Motorola 6821 PIAs (system, game, modem)

* memory:
  - 512 KB RAM (not extendable)

* devices:
  - same keyboard as T08/TO9 (AZERTY 81-keys) but different controller
  - RF 57-932: RS232 (identical to the TO7) sometimes integrated
  - MD 90-120: MODEM (identical to the TO7?) sometimes integrated
  - IEEE extension ?
  - floppy: one two-sided double-density 3"1/2 floppy drive is integrated
  - RS 52-932 RS232 extension ?
  - digitisation extension

**********************************************************************/

static ADDRESS_MAP_START ( to9p, ADDRESS_SPACE_PROGRAM, 8 )

     AM_RANGE ( 0x0000, 0x3fff ) AM_RAMBANK   ( THOM_CART_BANK ) /* 8 * 16 KB */
                                 AM_WRITE     ( to8_cartridge_w )
     AM_RANGE ( 0x4000, 0x5fff ) AM_RAMBANK   ( THOM_VRAM_BANK )
                                 AM_WRITE     ( to770_vram_w ) 
     AM_RANGE ( 0x6000, 0x7fff ) AM_RAMBANK   ( TO8_SYS_LO )
                                 AM_WRITE     ( to8_sys_lo_w )
     AM_RANGE ( 0x8000, 0x9fff ) AM_RAMBANK   ( TO8_SYS_HI )
                                 AM_WRITE     ( to8_sys_hi_w )
     AM_RANGE ( 0xa000, 0xbfff ) AM_RAMBANK   ( TO8_DATA_LO )
                                 AM_WRITE     ( to8_data_lo_w )
     AM_RANGE ( 0xc000, 0xdfff ) AM_RAMBANK   ( TO8_DATA_HI )
                                 AM_WRITE     ( to8_data_hi_w )
     AM_RANGE ( 0xe000, 0xe7af ) AM_ROMBANK   ( TO8_FLOP_BANK ) /* 2 * 2 KB */
     AM_RANGE ( 0xe7c0, 0xe7c7 ) AM_READWRITE ( mc6846_r, mc6846_w )
     AM_RANGE ( 0xe7c8, 0xe7cb ) AM_READWRITE ( pia_0_r,  pia_0_w )
     AM_RANGE ( 0xe7cc, 0xe7cf ) AM_READWRITE ( pia_1_r,  pia_1_w )
     AM_RANGE ( 0xe7d0, 0xe7d9 ) AM_READWRITE ( to8_floppy_r, to8_floppy_w )
     AM_RANGE ( 0xe7da, 0xe7dd ) AM_READWRITE ( to8_vreg_r, to8_vreg_w )
     AM_RANGE ( 0xe7de, 0xe7df ) AM_READWRITE ( to9_kbd_r, to9_kbd_w )
     AM_RANGE ( 0xe7e4, 0xe7e7 ) AM_READWRITE ( to8_gatearray_r, 
						to8_gatearray_w )
     AM_RANGE ( 0xe7e8, 0xe7eb ) AM_READWRITE ( acia_6551_r, acia_6551_w )
/*   AM_RANGE ( 0xe7f0, 0xe7f7 ) AM_READWRITE ( to9_ieee_r, to9_ieee_w ) */
     AM_RANGE ( 0xe7f2, 0xe7f3 ) AM_READWRITE ( to7_midi_r, to7_midi_w )
     AM_RANGE ( 0xe7f8, 0xe7fb ) AM_READWRITE ( pia_3_r, pia_3_w )
     AM_RANGE ( 0xe7fe, 0xe7ff ) AM_READWRITE ( to7_modem_mea8000_r, 
						to7_modem_mea8000_w )
     AM_RANGE ( 0xe800, 0xffff ) AM_ROMBANK   ( TO8_BIOS_BANK ) /* 2 * 6 KB */

/* 0x10000 - 0x1ffff:  64 KB external ROM cartridge */
/* 0x20000 - 0x2ffff:  64 KB internal software ROM */
/* 0x30000 - 0xaffff: 512 KB total RAM (including video) */
/* 0xb0000 - 0xb3fff:  16 KB BIOS ROM */

ADDRESS_MAP_END


/* ------------ ROMS ------------ */

ROM_START ( to9p )
     ROM_REGION ( 0xb4000, REGION_CPU1, 0 )

     /* BIOS & floppy */
     ROM_LOAD ( "to9p-0.rom", 0xb0000, 0x2000,
		CRC(a2731296)
		MD5(dfb6bfe5ae6142395fe4d1b854b7f6ff) )
     ROM_LOAD ( "to9p-1.rom", 0xb2000, 0x2000,
		CRC(c52ce315)
		MD5(66bba93895d27c788b25a8b48c8d3799) )

     /* BASIC */
     ROM_LOAD ( "basicp-0.rom", 0x20000, 0x4000,
		CRC(e5a00fb3)
		MD5(6c33e3cd79bd6c675634b172594bbfec) )
     ROM_LOAD ( "basicp-1.rom", 0x24000, 0x4000,
		CRC(4b241e63)
		MD5(360cbf42078bf8c80a283828bf6ee4e6) )
     ROM_LOAD ( "basicp-2.rom", 0x28000, 0x4000,
		CRC(0f5581b3)
		MD5(02279e22b8bfe29e750fb62671e3951d) )
     ROM_LOAD ( "basicp-3.rom", 0x2c000, 0x4000,
		CRC(ebe9c8d9)
		MD5(67e7f2de15bcd2ee9dac7974da478901) )

      ROM_FILL( 0x10000, 0x10000, 0x39 )
ROM_END


/* ------------ inputs   ------------ */

INPUT_PORTS_START ( to9p )
     PORT_INCLUDE ( to8 )
INPUT_PORTS_END


/* ------------ config ------------ */

SYSTEM_CONFIG_START ( to9p )
     CONFIG_IMPORT_FROM ( to8 )
SYSTEM_CONFIG_END


/* ------------ driver ------------ */

static MACHINE_DRIVER_START ( to9p )
     MDRV_IMPORT_FROM ( to7 )
     MDRV_MACHINE_START ( to9p )
     MDRV_MACHINE_RESET ( to9p )
     MDRV_CPU_MODIFY( "main" )
     MDRV_CPU_PROGRAM_MAP ( to9p, 0 )
MACHINE_DRIVER_END

COMP ( 1986, to9p, 0, 0, to9p, to9p, NULL, to9p, "Thomson", "TO9+", 
       0 /*GAME_SUPPORTS_SAVE*/ )



/******************************** MO6 ********************************

MO6 (1986)
---

The MO6 is the (long awaited) successor to the MO5.
It is based on TO8 technology (same system & video gate-array).
However, it is lower-end and cheaper: less memory (128 KB RAM, not
extensible), no floppy controller but an integrated cassette recorder.
The MO6 is MO5 compatible, but not compatible with the TO family.

* chips:
  - 1 MHz Motorola 6809E CPU
  - 2 Motorola 6821 PIAs (system, game)
  - 1 gate-array (system & video, identical to the TO8)

* memory:
  - 128 KB RAM (not extendable)
  - 8 KB BIOS ROM
  - 24 KB BASIC 1 ROM
  - 32 KB BASIC 128 & extended BIOS ROM

* video:
  all modes from the TO8, but the TO7/70-compatible mode is replaced with
  an MO5-compatible one

* devices:
  - AZERTY keyboard, 69 keys, no keyboard controller (scanning is done 
    periodically by the 6809)
  - MO5-compatible cartridge
  - two-button mouse (TO8-like)
  - optional lightpen
  - integrated game port (similar to SX 90-018)
    . 6-bit DAC sound
    . two 8-position 2-button game pads
    . two-button mouse
  - integrated cassette reader 1200 bauds (MO5 compatible) and 2400 bauds
  - parallel CENTRONICS (printer emulated)
  - RF 57-932: RS232 extension (identical to the TO7), or RF 90-932 (???)
  - IEEE extension ?
  - no integrated controller, but external TO7 floppy controllers & drives
    are possible
  - speech synthesis extension: identical to TO7 ?
  - MIDIPAK MIDI extension: identical to TO7 ?


Olivetti Prodest PC 128 (1986)
-----------------------

Italian version of the MO6, built by Thomson and sold by Olivetti.
Except from the ROM, it is very similar to the MO6.
Do not confuse with the Olivetti Prodest PC 128 Systema (or 128s) which is 
based on the Acorn BBC Master Compact. Or with the Olivetti PC 1, which is
a PC XT.


**********************************************************************/

static ADDRESS_MAP_START ( mo6, ADDRESS_SPACE_PROGRAM, 8 )

     AM_RANGE ( 0x0000, 0x1fff ) AM_RAMBANK   ( THOM_VRAM_BANK )
                                 AM_WRITE     ( to770_vram_w ) 
     AM_RANGE ( 0x2000, 0x3fff ) AM_RAMBANK   ( TO8_SYS_LO )
                                 AM_WRITE     ( to8_sys_lo_w )
     AM_RANGE ( 0x4000, 0x5fff ) AM_RAMBANK   ( TO8_SYS_HI )
                                 AM_WRITE     ( to8_sys_hi_w )
     AM_RANGE ( 0x6000, 0x7fff ) AM_RAMBANK   ( TO8_DATA_LO )
                                 AM_WRITE     ( to8_data_lo_w )
     AM_RANGE ( 0x8000, 0x9fff ) AM_RAMBANK   ( TO8_DATA_HI )
                                 AM_WRITE     ( to8_data_hi_w )
     AM_RANGE ( 0xa000, 0xa7bf ) AM_ROMBANK   ( THOM_FLOP_BANK )
     AM_RANGE ( 0xa7c0, 0xa7c3 ) AM_READWRITE ( pia_0_r,  pia_0_w )
     AM_RANGE ( 0xa7cb, 0xa7cb ) AM_WRITE     ( mo6_ext_w )
     AM_RANGE ( 0xa7cc, 0xa7cf ) AM_READWRITE ( pia_1_r,  pia_1_w )
     AM_RANGE ( 0xa7d0, 0xa7d9 ) AM_READWRITE ( to7_floppy_r, to7_floppy_w )
     AM_RANGE ( 0xa7da, 0xa7dd ) AM_READWRITE ( mo6_vreg_r, mo6_vreg_w )
     AM_RANGE ( 0xa7e4, 0xa7e7 ) AM_READWRITE ( mo6_gatearray_r, 
						mo6_gatearray_w )
     AM_RANGE ( 0xa7e8, 0xa7eb ) AM_READWRITE ( acia_6551_r, acia_6551_w )
/*   AM_RANGE ( 0xa7f0, 0xa7f7 ) AM_READWRITE ( to9_ieee_r, to9_ieee_w )*/
     AM_RANGE ( 0xa7f2, 0xa7f3 ) AM_READWRITE ( to7_midi_r, to7_midi_w )
     AM_RANGE ( 0xa7fe, 0xa7ff ) AM_READWRITE ( mea8000_r, mea8000_w )
     AM_RANGE ( 0xb000, 0xefff ) AM_ROMBANK   ( THOM_CART_BANK )
                                 AM_WRITE     ( mo6_cartridge_w )
     AM_RANGE ( 0xf000, 0xffff ) AM_ROMBANK   ( TO8_BIOS_BANK )

/* 0x10000 - 0x1ffff:  64 KB external cartridge */
/* 0x20000 - 0x2ffff:  64 KB BIOS ROM */
/* 0x30000 - 0x4ffff: 128 KB RAM */
/* 0x50000 - 0x53fff:  16 KB floppy ROM controllers */

ADDRESS_MAP_END


/* ------------ ROMS ------------ */

ROM_START ( mo6 )
     ROM_REGION ( 0x54000, REGION_CPU1, 0 )

     /* BIOS */
     ROM_LOAD ( "mo6-0.rom", 0x23000, 0x1000,
		CRC(0446eef6)
		MD5(192c44506f347b02030eb3ec862b850f) )
     ROM_LOAD ( "mo6-1.rom", 0x27000, 0x1000,
		CRC(eb6df8d4)
		MD5(7ff810d9b7e6900d8904336b6114ebd7) )

     /* BASIC */
     ROM_LOAD ( "basic6-0.rom", 0x20000, 0x3000,
		CRC(18789833)
		MD5(08edc429b44e581b3c826ac81b06cd2b) )
     ROM_LOAD ( "basic6-1.rom", 0x24000, 0x3000,
		CRC(c9b4d6f4)
		MD5(03f82e6d26547dd23fca7c420b6952c4) )
     ROM_LOAD ( "basic6-2.rom", 0x28000, 0x4000,
		CRC(08eac9bb)
		MD5(4f5d0bb8dea45d85d8551c09888423d1) )
     ROM_LOAD ( "basic6-3.rom", 0x2c000, 0x4000,
		CRC(19d66dc4)
		MD5(c2c4dab28d42adf4ea264270ea889c4f) )

     ROM_FLOPPY5 ( 0x50000 )
     ROM_FILL ( 0x10000, 0x10000, 0x39 )
ROM_END

ROM_START ( pro128 )
     ROM_REGION ( 0x54000, REGION_CPU1, 0 )

     /* BIOS */
     ROM_LOAD ( "pro128-0.rom", 0x23000, 0x1000,
		CRC(a8aef291)
		MD5(b7d937421754b79587050208b8c3beb2) )
     ROM_LOAD ( "pro128-1.rom", 0x27000, 0x1000,
		CRC(5b3340ec)
		MD5(9b7f4505195d12a5efceaac26bcb4079) )

     /* BASIC */
     ROM_LOAD ( "basico-0.rom", 0x20000, 0x3000,
		CRC(98b10d5e)
		MD5(96ce523fe27cd3935195f4218deb6277) )
     ROM_LOAD ( "basico-1.rom", 0x24000, 0x3000,
		CRC(721d2124)
		MD5(73cfdcd1638932ef8ebc639cbc6e2dd0) )
     ROM_LOAD ( "basico-2.rom", 0x28000, 0x4000,
		CRC(135438ab)
		MD5(d155a93aa1a2b1b5102bb5e0acdb3391) )
     ROM_LOAD ( "basico-3.rom", 0x2c000, 0x4000,
		CRC(2c2befa6)
		MD5(5e31d779961ed1ae7fc800489277f96a) )

     ROM_FLOPPY5 ( 0x50000 )
     ROM_FILL ( 0x10000, 0x10000, 0x39 )
ROM_END


/* ------------ inputs   ------------ */

INPUT_PORTS_START ( mo6_keyboard )

     PORT_START_TAG ( "keyboard_0" )
     KEY ( 0, "N", N )
     KEY ( 1, ", ?", COMMA )
     KEY ( 2, "; .", STOP )
     KEY ( 3, "# @", TILDE )
     KEY ( 4, "SPACE", SPACE )
     KEY ( 5, "X", X )
     KEY ( 6, "W", W )
     KEY ( 7, "SHIFT", LSHIFT ) PORT_CODE ( KEYCODE_RSHIFT )
     PORT_START_TAG ( "keyboard_1" )
     KEY ( 0, "DELETE BACKSPACE", DEL )
     KEY ( 1, "INS", INSERT )
     KEY ( 2, "> <", BACKSLASH2 )
     KEY ( 3, "RIGHT", RIGHT )
     KEY ( 4, "DOWN", DOWN )
     KEY ( 5, "LEFT", LEFT )
     KEY ( 6, "UP", UP )
     KEY ( 7, "BASIC", RCONTROL )
     PORT_START_TAG ( "keyboard_2" )
     KEY ( 0, "J", J )
     KEY ( 1, "K", K )
     KEY ( 2, "L", L )
     KEY ( 3, "M", M )
     KEY ( 4, "B \302\264", B )
     KEY ( 5, "V", V )
     KEY ( 6, "C \136", C )
     KEY ( 7, "CAPS-LOCK", CAPSLOCK )
     PORT_START_TAG ( "keyboard_3" )
     KEY ( 0, "H \302\250", H )
     KEY ( 1, "G", G )
     KEY ( 2, "F", F )
     KEY ( 3, "D", D )
     KEY ( 4, "S", S )
     KEY ( 5, "Q", Q )
     KEY ( 6, "HOME CLEAR", HOME )
     KEY ( 7, "F1 F6", F1 )
     PORT_START_TAG ( "keyboard_4" )
     KEY ( 0, "U", U )
     KEY ( 1, "I", I )
     KEY ( 2, "O", O )
     KEY ( 3, "P", P )
     KEY ( 4, ": /", SLASH )
     KEY ( 5, "$ &", CLOSEBRACE )
     KEY ( 6, "ENTER", ENTER )
     KEY ( 7, "F2 F7", F2 )
     PORT_START_TAG ( "keyboard_5" )
     KEY ( 0, "Y", Y )
     KEY ( 1, "T", T )
     KEY ( 2, "R", R )
     KEY ( 3, "E", E )
     KEY ( 4, "Z", Z )
     KEY ( 5, "A \140", A )
     KEY ( 6, "CONTROL", LCONTROL  )
     KEY ( 7, "F3 F8", F3 )
     PORT_START_TAG ( "keyboard_6" )
     KEY ( 0, "7 \303\250", 7 )
     KEY ( 1, "8 !", 8 )
     KEY ( 2, "9 \303\247", 9 )
     KEY ( 3, "0 \303\240", 0 )
     KEY ( 4, "- \\", BACKSPACE )
     KEY ( 5, "= +", EQUALS )
     KEY ( 6, "ACCENT", END )
     KEY ( 7, "F4 F9", F4 )
     PORT_START_TAG ( "keyboard_7" )
     KEY ( 0, "6 _", 6 )
     KEY ( 1, "5 (", 5 )
     KEY ( 2, "4 '", 4 )
     KEY ( 3, "3 \"", 3 )
     KEY ( 4, "2 \303\251", 2 )
     KEY ( 5, "1 *", 1 )
     KEY ( 6, "STOP", TAB )
     KEY ( 7, "F5 F10", F5 )
     PORT_START_TAG ( "keyboard_8" )
     KEY ( 0, "[ {", QUOTE )
     KEY ( 1, "] }", BACKSLASH )
     KEY ( 2, ") \302\260", MINUS )
     KEY ( 3, "\342\206\221 \302\250", OPENBRACE )
     KEY ( 4, "\303\271 %", COLON )
     PORT_BIT  ( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

     /* unused */
     PORT_START_TAG ( "keyboard_9" )

INPUT_PORTS_END

/* QWERTY version */
INPUT_PORTS_START ( pro128_keyboard )
     PORT_INCLUDE ( mo6_keyboard )

     PORT_MODIFY ( "keyboard_0" )
     KEY ( 1, "M", M )
     KEY ( 2, ", ;", COMMA )
     KEY ( 3, "[ {", QUOTE  )
     KEY ( 6, "Z", Z )
     KEY ( 7, "SHIFT", LSHIFT ) PORT_CODE ( KEYCODE_RSHIFT )
     PORT_MODIFY ( "keyboard_1" )
     KEY ( 2, "- _", MINUS )
     PORT_MODIFY ( "keyboard_2" )
     KEY ( 3, "\303\221", TILDE )
     PORT_MODIFY ( "keyboard_3" )
     KEY ( 5, "A \140", A )
     PORT_MODIFY ( "keyboard_4" )
     KEY ( 4, ". :", STOP )
     KEY ( 5, "+ *", BACKSPACE )
     PORT_MODIFY ( "keyboard_5" )
     KEY ( 4, "W", W )
     KEY ( 5, "Q", Q )
     PORT_MODIFY ( "keyboard_6" )
     KEY ( 0, "7 /", 7 )
     KEY ( 1, "8 (", 8 )
     KEY ( 2, "9 )", 9 )
     KEY ( 3, "0 =", 0 )
     KEY ( 4, "' \302\243", CLOSEBRACE )
     KEY ( 5, "] }", BACKSLASH )
     PORT_MODIFY ( "keyboard_7" )
     KEY ( 0, "6 &", 6 )
     KEY ( 1, "5 %", 5 )
     KEY ( 2, "4 $", 4 )
     KEY ( 3, "3 \302\247", 3 )
     KEY ( 4, "2 \"", 2 )
     KEY ( 5, "1 !", 1 )
     PORT_MODIFY ( "keyboard_8" )
     KEY ( 0, "> <", BACKSLASH2 )
     KEY ( 1, "# \342\206\221", EQUALS )
     KEY ( 2, "\303\247 ?", COLON )
     KEY ( 3, "\302\277 @", SLASH )
     KEY ( 4, "\302\241 \302\250", OPENBRACE )

INPUT_PORTS_END


INPUT_PORTS_START ( mo6 )
     PORT_INCLUDE ( thom_lightpen )
     PORT_INCLUDE ( thom_game_port )
     PORT_INCLUDE ( mo6_keyboard )
     PORT_INCLUDE ( to7_config )
     PORT_INCLUDE ( to7_fconfig )
     PORT_INCLUDE ( to7_vconfig )
INPUT_PORTS_END

INPUT_PORTS_START ( pro128 )
     PORT_INCLUDE ( thom_lightpen )
     PORT_INCLUDE ( thom_game_port )
     PORT_INCLUDE ( pro128_keyboard )
     PORT_INCLUDE ( to7_config )
     PORT_INCLUDE ( to7_fconfig )
     PORT_INCLUDE ( to7_vconfig )
INPUT_PORTS_END


/* ------------ config ------------ */

SYSTEM_CONFIG_START ( mo6 )
     CONFIG_IMPORT_FROM ( mo5 )
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START ( pro128 )
     CONFIG_IMPORT_FROM ( mo6 )
SYSTEM_CONFIG_END


/* ------------ driver ------------ */

static MACHINE_DRIVER_START ( mo6 )
     MDRV_IMPORT_FROM ( to7 )
     MDRV_MACHINE_START ( mo6 )
     MDRV_MACHINE_RESET ( mo6 )
     MDRV_CPU_MODIFY( "main" )
     MDRV_CPU_PROGRAM_MAP ( mo6, 0 )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START ( pro128 )
     MDRV_IMPORT_FROM ( mo6 )
MACHINE_DRIVER_END

COMP ( 1986, mo6, 0, 0, mo6, mo6, NULL, mo6, "Thomson", "MO6", 
       0 /*GAME_SUPPORTS_SAVE*/ )

COMP ( 1986, pro128, mo6, 0, pro128, pro128, NULL, pro128, "Olivetti / Thomson", "Prodest PC 128", 
       0 /*GAME_SUPPORTS_SAVE*/ )




/****************************** MO5NR ********************************

MO5 NR (1986)
------

Despite its name, the MO5 NR is much more related to the MO6 than to the MO5.
It can be though as the network-enhanced version of the MO6.
It is both MO5 and MO6 compatible (but not TO-compatible).

Here are the differences between the MO6 and MO5NR:

* chips:
  - integrated MC 6854 network controller

* memory: 
  - extra 2 KB ROM for the integrated network controller,
    can be masked by the ROM from the external floppy controller

* video: identical

* devices:
  - AZERTY keyboard has only 58 keys, and no caps-lock led
  - CENTRONICS printer handled differently
  - MO5-compatible network (probably identical to NR 07-005 extension)
  - extern floppy controller & drive possible, masks the network

**********************************************************************/

static ADDRESS_MAP_START ( mo5nr, ADDRESS_SPACE_PROGRAM, 8 )

     AM_RANGE ( 0x0000, 0x1fff ) AM_RAMBANK   ( THOM_VRAM_BANK )
                                 AM_WRITE     ( to770_vram_w ) 
     AM_RANGE ( 0x2000, 0x3fff ) AM_RAMBANK   ( TO8_SYS_LO )
                                 AM_WRITE     ( to8_sys_lo_w )
     AM_RANGE ( 0x4000, 0x5fff ) AM_RAMBANK   ( TO8_SYS_HI )
                                 AM_WRITE     ( to8_sys_hi_w )
     AM_RANGE ( 0x6000, 0x7fff ) AM_RAMBANK   ( TO8_DATA_LO )
                                 AM_WRITE     ( to8_data_lo_w )
     AM_RANGE ( 0x8000, 0x9fff ) AM_RAMBANK   ( TO8_DATA_HI )
                                 AM_WRITE     ( to8_data_hi_w )
     AM_RANGE ( 0xa000, 0xa7bf ) AM_ROMBANK   ( THOM_FLOP_BANK )
     AM_RANGE ( 0xa7c0, 0xa7c3 ) AM_READWRITE ( pia_0_r,  pia_0_w )
     AM_RANGE ( 0xa7cb, 0xa7cb ) AM_WRITE     ( mo6_ext_w )
     AM_RANGE ( 0xa7cc, 0xa7cf ) AM_READWRITE ( pia_1_r,  pia_1_w )
     AM_RANGE ( 0xa7d0, 0xa7d9 ) AM_READWRITE ( mo5nr_net_r, mo5nr_net_w )
     AM_RANGE ( 0xa7da, 0xa7dd ) AM_READWRITE ( mo6_vreg_r, mo6_vreg_w )
     AM_RANGE ( 0xa7e0, 0xa7e3 ) AM_READWRITE ( mo5nr_prn_r, mo5nr_prn_w )
     AM_RANGE ( 0xa7e4, 0xa7e7 ) AM_READWRITE ( mo6_gatearray_r, 
						mo6_gatearray_w )
     AM_RANGE ( 0xa7e8, 0xa7eb ) AM_READWRITE ( acia_6551_r, acia_6551_w )
/*   AM_RANGE ( 0xa7f0, 0xa7f7 ) AM_READWRITE ( to9_ieee_r, to9_ieee_w ) */
     AM_RANGE ( 0xa7f2, 0xa7f3 ) AM_READWRITE ( to7_midi_r, to7_midi_w )
     AM_RANGE ( 0xa7f8, 0xa7fb ) AM_READWRITE ( pia_3_r, pia_3_w )
     AM_RANGE ( 0xa7fe, 0xa7ff ) AM_READWRITE ( mea8000_r, mea8000_w )
     AM_RANGE ( 0xb000, 0xefff ) AM_RAMBANK   ( THOM_CART_BANK ) /* 8 * 16 KB */
                                 AM_WRITE     ( mo6_cartridge_w )
     AM_RANGE ( 0xf000, 0xffff ) AM_ROMBANK   ( TO8_BIOS_BANK )

/* 0x10000 - 0x1ffff:  64 KB external cartridge */
/* 0x20000 - 0x2ffff:  64 KB BIOS ROM */
/* 0x30000 - 0x4ffff: 128 KB RAM */
/* 0x50000 - 0x53fff:  16 KB floppy ROM controllers */

ADDRESS_MAP_END


/* ------------ ROMS ------------ */

ROM_START ( mo5nr )
     ROM_REGION ( 0x54000, REGION_CPU1, 0 )

     /* BIOS */
     ROM_LOAD ( "mo5nr-0.rom", 0x23000, 0x1000,
		CRC(06e31115)
		MD5(5826a0190d7d0eb9a03c327072198a5d) )
     ROM_LOAD ( "mo5nr-1.rom", 0x27000, 0x1000,
		CRC(7cda17c9)
		MD5(ede3dd012cd15ad5066581522ce724b5) )

     /* BASIC */
     ROM_LOAD ( "basicn-0.rom", 0x20000, 0x3000,
		CRC(fae9e691)
		MD5(03c7d856a17b34c7b295b22038c89c8f) )
     ROM_LOAD ( "basicn-1.rom", 0x24000, 0x3000,
		CRC(cf134dd7)
		MD5(e8069cd0857735e856ac0125b473b696) )
     ROM_LOAD ( "basicn-2.rom", 0x28000, 0x4000,
		CRC(b69d2e0d)
		MD5(cd11f0244c0cd30fd315c14e83b21381) )
     ROM_LOAD ( "basicn-3.rom", 0x2c000, 0x4000,
		CRC(7785610f)
		MD5(c67c65ac66c5f82bea3fcb83c2308a51) )

     ROM_FLOPPY5 ( 0x50000 )
     ROM_FILL ( 0x10000, 0x10000, 0x39 ) /* TODO: network ROM */
ROM_END




/* ------------ inputs   ------------ */

INPUT_PORTS_START ( mo5nr_keyboard )

     PORT_START_TAG ( "keyboard_0" )
     KEY ( 0, "N", N )
     KEY ( 1, ", <", COMMA )
     KEY ( 2, ". >", STOP )
     KEY ( 3, "@ \342\206\221", TILDE )
     KEY ( 4, "SPACE CAPS-LOCK", SPACE )
     KEY ( 5, "X", X )
     KEY ( 6, "W", W )
     KEY ( 7, "SHIFT", LSHIFT ) PORT_CODE ( KEYCODE_RSHIFT )
     PORT_START_TAG ( "keyboard_1" )
     KEY ( 0, "DELETE", DEL )
     KEY ( 1, "INS", INSERT )
     KEY ( 2, "HOME", HOME )
     KEY ( 3, "RIGHT", RIGHT )
     KEY ( 4, "DOWN", DOWN )
     KEY ( 5, "LEFT", LEFT )
     KEY ( 6, "UP", UP )
     KEY ( 7, "BASIC", RCONTROL )
     PORT_START_TAG ( "keyboard_2" )
     KEY ( 0, "J", J )
     KEY ( 1, "K", K )
     KEY ( 2, "L", L )
     KEY ( 3, "M", M )
     KEY ( 4, "B \140", B )
     KEY ( 5, "V", V )
     KEY ( 6, "C \136", C )
     PORT_BIT  ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
     PORT_START_TAG ( "keyboard_3" )
     KEY ( 0, "H \302\250", H )
     KEY ( 1, "G", G )
     KEY ( 2, "F", F )
     KEY ( 3, "D", D )
     KEY ( 4, "S", S )
     KEY ( 5, "Q", Q )
     KEY ( 6, "CLEAR", ESC )
     PORT_BIT  ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
     PORT_START_TAG ( "keyboard_4" )
     KEY ( 0, "U", U )
     KEY ( 1, "I", I )
     KEY ( 2, "O", O )
     KEY ( 3, "P", P )
     KEY ( 4, "/ ?", SLASH )
     KEY ( 5, "* :", QUOTE )
     KEY ( 6, "ENTER", ENTER )
     PORT_BIT  ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
     PORT_START_TAG ( "keyboard_5" )
     KEY ( 0, "Y", Y )
     KEY ( 1, "T", T )
     KEY ( 2, "R", R )
     KEY ( 3, "E", E )
     KEY ( 4, "Z", Z )
     KEY ( 5, "A \140", A )
     KEY ( 6, "CONTROL", LCONTROL  )
     PORT_BIT  ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
     PORT_START_TAG ( "keyboard_6" )
     KEY ( 0, "7 ' \303\250", 7 )
     KEY ( 1, "8 ( \303\271", 8 )
     KEY ( 2, "9 ) \303\247", 9 )
     KEY ( 3, "0 \303\240", 0 )
     KEY ( 4, "- =", MINUS )
     KEY ( 5, "+ ;", EQUALS )
     KEY ( 6, "ACCENT", END )
     PORT_BIT  ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
     PORT_START_TAG ( "keyboard_7" )
     KEY ( 0, "6 & \303\251", 6 )
     KEY ( 1, "5 %", 5 )
     KEY ( 2, "4 $", 4 )
     KEY ( 3, "3 #", 3 )
     KEY ( 4, "2 \"", 2 )
     KEY ( 5, "1 !", 1 )
     KEY ( 6, "STOP", TAB )
     PORT_BIT  ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

     /* unused */
     PORT_START_TAG ( "keyboard_8" )
     PORT_START_TAG ( "keyboard_9" )

INPUT_PORTS_END

INPUT_PORTS_START ( mo5nr )
     PORT_INCLUDE ( thom_lightpen )
     PORT_INCLUDE ( thom_game_port )
     PORT_INCLUDE ( mo5nr_keyboard )
     PORT_INCLUDE ( to7_config )
     PORT_INCLUDE ( to7_fconfig )
     PORT_INCLUDE ( to7_vconfig )
INPUT_PORTS_END


/* ------------ config ------------ */

SYSTEM_CONFIG_START ( mo5nr )
     CONFIG_IMPORT_FROM ( mo6 )
SYSTEM_CONFIG_END


/* ------------ driver ------------ */

static MACHINE_DRIVER_START ( mo5nr )
     MDRV_IMPORT_FROM ( to7 )
     MDRV_MACHINE_START ( mo5nr )
     MDRV_MACHINE_RESET ( mo5nr )
     MDRV_CPU_MODIFY( "main" )
     MDRV_CPU_PROGRAM_MAP ( mo5nr, 0 )
MACHINE_DRIVER_END

COMP ( 1986, mo5nr, 0, 0, mo5nr, mo5nr, NULL, mo5nr, "Thomson", "MO5 NR", 
       0 /*GAME_SUPPORTS_SAVE*/ )
