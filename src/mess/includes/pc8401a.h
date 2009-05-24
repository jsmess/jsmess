#ifndef __PC8401A__
#define __PC8401A__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define PPI8255_TAG		"ppi8255"

#define PC8401A_VIDEORAM_SIZE	0x2000

typedef struct _pc8401a_state pc8401a_state;
struct _pc8401a_state
{
	/* memory state */
	UINT8 mmr;				/* memory mapping register */

	/* video state */
	UINT8 *video_ram;		/* video RAM */
};

#endif
