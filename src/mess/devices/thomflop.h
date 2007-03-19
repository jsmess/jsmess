/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Thomson 8-bit computers

**********************************************************************/

/* TO7, TO7/70, MO5, MO6 controllers */
extern void to7_floppy_init  ( void* base );
extern void to7_floppy_reset ( void );
extern READ8_HANDLER  ( to7_floppy_r );
extern WRITE8_HANDLER ( to7_floppy_w );
extern UINT8 to7_controller_type; /* set durint init */

/* TO9 internal controller (WD2793) */
extern void to9_floppy_init  ( void );
extern void to9_floppy_reset ( void );
extern READ8_HANDLER  ( to9_floppy_r );
extern WRITE8_HANDLER ( to9_floppy_w );

/* THMFC1 internal controller (Thomson custom Gate-Array, for TO8, TO9+) */
extern void to8_floppy_init  ( void );
extern void to8_floppy_reset ( void );
extern READ8_HANDLER  ( to8_floppy_r );
extern WRITE8_HANDLER ( to8_floppy_w );
