#ifndef __PX8__
#define __PX8__

#define UPD70008_TAG	"4a"
#define UPD7508_TAG		"2e"
#define HD6303_TAG		"13d"
#define SED1320_TAG		"7c"
#define I8251_TAG		"13e"
#define UPD7001_TAG		"1d"
#define CASSETTE_TAG	"cassette"
#define SCREEN_TAG		"screen"

#define PX8_VIDEORAM_MASK	0x17ff

class px8_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, px8_state(machine)); }

	px8_state(running_machine &machine) { }

	/* GAH40M state */
	UINT16 icr;				/* input capture register */
	UINT16 frc;				/* free running counter */
	UINT8 ier;				/* interrupt acknowledge register */
	UINT8 isr;				/* interrupt status register */
	UINT8 sio;				/* serial I/O register */
	int bank0;				/* */

	/* GAH40S state */
	UINT16 cnt;				/* microcassette tape counter */
	int swpr;				/* P-ROM power switch */
	UINT16 pra;				/* P-ROM address */
	UINT8 prd;				/* P-ROM data */

	/* memory state */
	int bk2;				/* */

	/* keyboard state */
	int ksc;				/* keyboard scan column */

	/* video state */
	UINT8 *video_ram;		/* LCD video RAM */

	/* devices */
	running_device *sed1320;
	running_device *cassette;
};

#endif
