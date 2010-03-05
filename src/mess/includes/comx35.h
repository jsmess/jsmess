#ifndef __COMX35__
#define __COMX35__

#include "devices/snapquik.h"
#include "machine/wd17xx.h"

#define SCREEN_TAG			"screen"
#define MC6845_SCREEN_TAG	"screen80"

#define CDP1870_TAG			"u1"
#define CDP1869_TAG			"u2"
#define CDP1802_TAG			"u3"
#define CDP1871_TAG			"u4"
#define MC6845_TAG			"mc6845"
#define WD1770_TAG			"wd1770"

#define CASSETTE_TAG		"cassette"

#define COMX35_PAGERAM_SIZE 0x400
#define COMX35_CHARRAM_SIZE 0x800
#define COMX35_VIDEORAM_SIZE 0x800

#define COMX35_PAGERAM_MASK 0x3ff
#define COMX35_CHARRAM_MASK 0x7ff
#define COMX35_VIDEORAM_MASK 0x7ff

enum
{
	BANK_NONE = 0,
	BANK_FLOPPY,
	BANK_PRINTER_PARALLEL,
	BANK_PRINTER_PARALLEL_FM,
	BANK_PRINTER_SERIAL,
	BANK_PRINTER_THERMAL,
	BANK_JOYCARD,
	BANK_80_COLUMNS,
	BANK_RAMCARD
};

class comx35_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, comx35_state(machine)); }

	comx35_state(running_machine &machine) { }

	/* processor state */
	int cdp1802_mode;		/* CPU mode */
	int cdp1802_q;			/* Q flag */
	int cdp1802_ef4;		/* EF4 flag */
	int iden;				/* interrupt/DMA enable */
	int slot;				/* selected slot */
	int bank;				/* selected device bank */
	int rambank;			/* selected RAM bank */
	int dma;				/* memory refresh DMA */

	/* video state */
	int pal_ntsc;			/* PAL/NTSC */
	int cdp1869_prd;		/* CDP1869 predisplay */

	UINT8 *pageram;			/* page memory */
	UINT8 *charram;			/* character memory */
	UINT8 *videoram;		/* 80 column video memory */

	/* keyboard state */
	int cdp1871_efxa;		/* keyboard data available */
	int cdp1871_efxb;		/* keyboard repeat */

	/* floppy state */
	int fdc_addr;			/* FDC address */
	int fdc_irq;			/* interrupt request */
	int fdc_drq_enable;		/* EF4 enabled */

	/* timers */
	emu_timer *reset_timer;	/* power on reset timer */

	/* devices */
	running_device *cassette;
};

/* ---------- defined in machine/comx35.c ---------- */

extern const wd17xx_interface comx35_wd17xx_interface;

WRITE8_HANDLER( comx35_bank_select_w );
READ8_HANDLER( comx35_io_r );
READ8_HANDLER( comx35_io2_r );
WRITE8_HANDLER( comx35_io_w );

MACHINE_START( comx35p );
MACHINE_START( comx35n );
MACHINE_RESET( comx35 );
INPUT_CHANGED( comx35_reset );

QUICKLOAD_LOAD( comx35 );

/* ---------- defined in video/comx35.c ---------- */

WRITE8_HANDLER( comx35_videoram_w );
READ8_HANDLER( comx35_videoram_r );

MACHINE_DRIVER_EXTERN( comx35_pal_video );
MACHINE_DRIVER_EXTERN( comx35_ntsc_video );

#endif
