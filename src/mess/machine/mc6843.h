/**********************************************************************

  Copyright (C) Antoine Mine' 2007

  Motorola 6843 Floppy Disk Controller emulation.

**********************************************************************/

#ifndef MC6843
#define MC6843


/* ---------- configuration ------------ */

typedef struct _mc6843_interface mc6843_interface;
struct _mc6843_interface
{
	void ( * irq_func ) ( int state );
};


/* ---------- functions ------------ */

extern void mc6843_config ( const mc6843_interface *func );

/* reset by external signal */
extern void mc6843_reset ( void );

/* interface to CPU via address/data bus */
extern READ8_HANDLER  ( mc6843_r );
extern WRITE8_HANDLER ( mc6843_w );

/* floppy select */
extern void mc6843_set_drive( int drive );
extern void mc6843_set_side( int side );

#endif
