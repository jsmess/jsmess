#ifndef __PC8401A__
#define __PC8401A__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define PPI8255_TAG		"ppi8255"
#define UPD1990A_TAG	"upd1990a"
#define AY8910_TAG		"ay8910"
#define SED1330_TAG		"sed1330"

#define PC8401A_VIDEORAM_SIZE	0x2000
#define PC8401A_VIDEORAM_MASK	0x1fff
#define PC8500_VIDEORAM_SIZE	0x4000
#define PC8500_VIDEORAM_MASK	0x3fff

typedef struct _pc8401a_state pc8401a_state;
struct _pc8401a_state
{
	/* clock state */
	int upd1990a_data;		/* RTC data output */

	/* memory state */
	UINT8 mmr;				/* memory mapping register */
	UINT32 io_addr;			/* I/O ROM address counter */

	/* video state */
	UINT8 *video_ram;		/* video RAM */

	const device_config *upd1990a;
	const device_config *sed1330;
};

#endif
