/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Thomson 8-bit computers

**********************************************************************/


/*************************** common ********************************/

/* input ports (first port number of each class) */
#define THOM_INPUT_LIGHTPEN  0 /* 3 ports: analog X, analog Y, and button */
#define THOM_INPUT_GAME      3 /* 2-5 ports: joystick, mouse */
#define THOM_INPUT_KEYBOARD  8 /* 8-10 lines */
#define THOM_INPUT_CONFIG   18 /* machine-specific options */
#define THOM_INPUT_FCONFIG  19 /* floppy / network options */
#define THOM_INPUT_VCONFIG  20 /* video options */
#define THOM_INPUT_MCONFIG  21 /* modem / speech options */

/* 6821 PIAs */
#define THOM_PIA_SYS    0  /* system PIA */
#define THOM_PIA_GAME   1  /* music & game PIA (joypad + sound) */
#define THOM_PIA_IO     2  /* CC 90-232 I/O extension (parallel & RS-232) */
#define THOM_PIA_MODEM  3  /* MD 90-120 MODEM extension */

/* sound ports */
#define THOM_SOUND_BUZ    0 /* 1-bit buzzer */
#define THOM_SOUND_GAME   1 /* 6-bit game port DAC */
#define THOM_SOUND_SPEECH 2 /* speach synthesis */

/* serial devices */
#define THOM_SERIAL_CC90323  0 /* RS232 port in I/O extension */
#define THOM_SERIAL_RF57232  1 /* RS232 extension */
#define THOM_SERIAL_MODEM    2 /* modem extension */


/* bank-switching */
#define THOM_CART_BANK  2
#define THOM_RAM_BANK   3
#define THOM_FLOP_BANK  4

/* serial */
extern int  thom_serial_init   ( mess_image* image );
extern int  thom_serial_load   ( mess_image* image );
extern void thom_serial_unload ( mess_image* image );


/***************************** TO7 / T9000 *************************/

/* cartridge bank-switching */
extern int to7_cartridge_load ( mess_image* image );
extern WRITE8_HANDLER ( to7_cartridge_w );

/* dispatch MODEM or speech synthesis extension */
extern READ8_HANDLER ( to7_modem_mea8000_r );
extern WRITE8_HANDLER ( to7_modem_mea8000_w );

/* MIDI extension (actually an 6850 ACIA) */
extern READ8_HANDLER  ( to7_midi_r );
extern WRITE8_HANDLER ( to7_midi_w );

extern MACHINE_START ( to7 );
extern MACHINE_RESET ( to7 );


/***************************** TO7/70 ******************************/

/* gate-array */
extern READ8_HANDLER  ( to770_gatearray_r );
extern WRITE8_HANDLER ( to770_gatearray_w );

extern MACHINE_START ( to770 );
extern MACHINE_RESET ( to770 );


/***************************** MO5 ******************************/

/* gate-array */
extern READ8_HANDLER  ( mo5_gatearray_r );
extern WRITE8_HANDLER ( mo5_gatearray_w );

/* cartridge / extended RAM bank-switching */
extern int mo5_cartridge_load ( mess_image* image );
extern WRITE8_HANDLER ( mo5_ext_w );
extern WRITE8_HANDLER ( mo5_cartridge_w );

extern MACHINE_START ( mo5 );
extern MACHINE_RESET ( mo5 );


/***************************** TO9 ******************************/

/* IEEE extension */
extern WRITE8_HANDLER ( to9_ieee_w );
extern READ8_HANDLER  ( to9_ieee_r );

/* ROM bank-switching */
extern WRITE8_HANDLER ( to9_cartridge_w );

/* system gate-array */
extern READ8_HANDLER  ( to9_gatearray_r );
extern WRITE8_HANDLER ( to9_gatearray_w );

/* video gate-array */
extern READ8_HANDLER  ( to9_vreg_r );
extern WRITE8_HANDLER ( to9_vreg_w );

/* keyboard */
extern READ8_HANDLER  ( to9_kbd_r );
extern WRITE8_HANDLER ( to9_kbd_w );

extern MACHINE_START ( to9 );
extern MACHINE_RESET ( to9 );


/***************************** TO8 ******************************/

/* bank-switching */
#define TO8_SYS_LO      5
#define TO8_SYS_HI      6
#define TO8_DATA_LO     7
#define TO8_DATA_HI     8
#define TO8_FLOP_BANK   4
#define TO8_BIOS_BANK   9

extern UINT8 to8_data_vpage;
extern UINT8 to8_cart_vpage;

extern WRITE8_HANDLER ( to8_cartridge_w );

/* system gate-array */
extern READ8_HANDLER  ( to8_gatearray_r );
extern WRITE8_HANDLER ( to8_gatearray_w );

/* video gate-array */
extern READ8_HANDLER  ( to8_vreg_r );
extern WRITE8_HANDLER ( to8_vreg_w );

extern MACHINE_START ( to8 );
extern MACHINE_RESET ( to8 );


/***************************** TO9+ ******************************/

extern MACHINE_START ( to9p );
extern MACHINE_RESET ( to9p );


/***************************** MO6 ******************************/

extern WRITE8_HANDLER ( mo6_cartridge_w );
extern WRITE8_HANDLER ( mo6_ext_w );

/* system gate-array */
extern READ8_HANDLER  ( mo6_gatearray_r );
extern WRITE8_HANDLER ( mo6_gatearray_w );

/* video gate-array */
extern READ8_HANDLER  ( mo6_vreg_r );
extern WRITE8_HANDLER ( mo6_vreg_w );

extern MACHINE_START ( mo6 );
extern MACHINE_RESET ( mo6 );


/***************************** MO5 NR ******************************/

/* network */
extern READ8_HANDLER  ( mo5nr_net_r );
extern WRITE8_HANDLER ( mo5nr_net_w );

/* printer */
extern READ8_HANDLER  ( mo5nr_prn_r );
extern WRITE8_HANDLER ( mo5nr_prn_w );

extern MACHINE_START ( mo5nr );
extern MACHINE_RESET ( mo5nr );

