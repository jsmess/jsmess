/*****************************************************************************
 *
 * includes/abc80x.h
 *
 ****************************************************************************/

#ifndef __ABC80X__
#define __ABC80X__

#define ABC800_X01	XTAL_12MHz
#define ABC806_X02	XTAL_32_768kHz

#define ABC802_AT0	0x01
#define ABC802_AT1	0x02
#define ABC802_ATD	0x40
#define ABC802_ATE	0x80
#define ABC802_INV	0x80

#define ABC800C_CHAR_RAM_SIZE	0x400
#define ABC800M_CHAR_RAM_SIZE	0x800
#define ABC800_VIDEO_RAM_SIZE	0x4000
#define ABC802_CHAR_RAM_SIZE	0x800
#define ABC806_CHAR_RAM_SIZE	0x800
#define ABC806_ATTR_RAM_SIZE	0x800
#define ABC806_VIDEO_RAM_SIZE	0x20000

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define E0516_TAG		"j13"
#define MC6845_TAG		"b12"
#define Z80CTC_TAG		"z80ctc"
#define Z80SIO_TAG		"z80sio"
#define Z80DART_TAG		"z80dart"
#define CASSETTE_TAG	"cassette"

#define ABC800_CHAR_WIDTH	6
#define ABC800_CCLK			ABC800_X01/ABC800_CHAR_WIDTH

typedef struct _abc800_state abc800_state;
struct _abc800_state
{
	/* cpu state */
	int fetch_charram;			/* opcode fetched from character RAM region (0x7800-0x7fff) */

	/* video state */
	UINT8 *charram;				/* character RAM */
	UINT8 *videoram;			/* HR video RAM */
	const UINT8 *char_rom;		/* character generator ROM */
	const UINT8 *fgctl_prom;	/* foreground control PROM */
	UINT8 hrs;					/* HR picture start scanline */
	UINT8 fgctl;				/* HR foreground control */

	/* sound state */
	int pling;					/* pling */

	/* devices */
	const device_config *z80ctc;
	const device_config *z80dart;
	const device_config *z80sio;
	const device_config *abc77;
	const device_config *mc6845;
	const device_config *cassette;
};

typedef struct _abc802_state abc802_state;
struct _abc802_state
{
	/* cpu state */
	int lrs;				/* low RAM select */

	/* video state */
	UINT8 *charram;			/* character RAM */
	const UINT8 *char_rom;	/* character generator ROM */

	int flshclk_ctr;		/* flash clock counter */
	int flshclk;			/* flash clock */
	int mux80_40;			/* 40/80 column mode */

	/* sound state */
	int pling;					/* pling */

	/* devices */
	const device_config *z80ctc;
	const device_config *z80dart;
	const device_config *z80sio;
	const device_config *abc77;
	const device_config *mc6845;
	const device_config *cassette;
};

typedef struct _abc806_state abc806_state;
struct _abc806_state
{
	/* memory state */
	int keydtr;				/* keyboard DTR */
	int eme;				/* extended memory enable */
	int fetch_charram;		/* opcode fetched from character RAM region (0x7800-0x7fff) */
	UINT8 map[16];			/* memory page register */

	/* video state */
	UINT8 *charram;			/* character RAM */
	UINT8 *colorram;		/* attribute RAM */
	UINT8 *videoram;		/* HR video RAM */
	const UINT8 *rad_prom;	/* line address PROM */
	const UINT8 *hru2_prom;	/* HR palette PROM */
	const UINT8 *char_rom;	/* character generator ROM */

	int txoff;				/* text display enable */
	int _40;				/* 40/80 column mode */
	int flshclk_ctr;		/* flash clock counter */
	int flshclk;			/* flash clock */
	UINT8 attr_data;		/* attribute data latch */
	UINT8 hrs;				/* HR memory mapping */
	UINT8 hrc[16];			/* HR palette */
	UINT8 sync;				/* line synchronization delay */
	UINT8 v50_addr;			/* vertical sync PROM address */
	int hru2_a8;			/* HRU II PROM address line 8 */
	UINT32 vsync_shift;		/* vertical sync shift register */
	int vsync;				/* vertical sync */
	int d_vsync;			/* delayed vertical sync */

	/* devices */
	const device_config *z80ctc;
	const device_config *z80dart;
	const device_config *z80sio;
	const device_config *mc6845;
	const device_config *abc77;
	const device_config *e0516;
	const device_config *cassette;
};

/*----------- defined in video/abc800.c -----------*/

READ8_HANDLER( abc800_charram_r );
WRITE8_HANDLER( abc800_charram_w );

WRITE8_HANDLER( abc800_hrs_w );
WRITE8_HANDLER( abc800_hrc_w );

MACHINE_DRIVER_EXTERN(abc800m_video);
MACHINE_DRIVER_EXTERN(abc800c_video);

/*----------- defined in video/abc802.c -----------*/

READ8_HANDLER( abc802_charram_r );
WRITE8_HANDLER( abc802_charram_w );

MACHINE_DRIVER_EXTERN(abc802_video);

/*----------- defined in video/abc806.c -----------*/

MACHINE_DRIVER_EXTERN(abc806_video);

WRITE8_HANDLER( abc806_hrs_w );
WRITE8_HANDLER( abc806_hrc_w );

READ8_HANDLER( abc806_charram_r );
WRITE8_HANDLER( abc806_charram_w );

READ8_HANDLER( abc806_ami_r );
WRITE8_HANDLER( abc806_amo_w );

READ8_HANDLER( abc806_cli_r );
WRITE8_HANDLER( abc806_sso_w );

READ8_HANDLER( abc806_sti_r );
WRITE8_HANDLER( abc806_sto_w );

#endif /* __ABC80X__ */
