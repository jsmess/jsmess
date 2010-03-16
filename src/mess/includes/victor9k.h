#ifndef __VICTOR9K__
#define __VICTOR9K__

#define SCREEN_TAG		"screen"
#define I8088_TAG		"8l"
#define I8048_TAG		"5d"
#define I8022_TAG		"i8022"
#define I8253_TAG		"13h"
#define I8259A_TAG		"7l"
#define UPD7201_TAG		"16e"
#define HD46505S_TAG	"11a"
#define MC6852_TAG		"13b"
#define HC55516_TAG		"1m"
#define M6522_1_TAG		"m6522_1"
#define M6522_2_TAG		"m6522_2"
#define M6522_3_TAG		"m6522_3"
#define M6522_4_TAG		"m6522_4"
#define M6522_5_TAG		"m6522_5"
#define M6522_6_TAG		"m6522_6"
#define SPEAKER_TAG		"speaker"
#define CENTRONICS_TAG	"centronics"
#define IEEE488_TAG		"ieee488"

typedef struct _victor9k_drive_t victor9k_drive_t;
struct _victor9k_drive_t
{
	/* motors */
	int lms;			/* motor speed */
	int st;				/* stepper phase */
	int se;				/* stepper enable */

	/* devices */
	running_device *image;
};

class victor9k_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, victor9k_state(machine)); }

	victor9k_state(running_machine &machine) { }

	/* video state */
	UINT8 *video_ram;
	int vert;
	int brt;
	int cont;

	/* interrupts */
	int via1_irq;
	int via2_irq;
	int via3_irq;
	int via4_irq;
	int via5_irq;
	int via6_irq;
	int ssda_irq;

	/* floppy state */
	victor9k_drive_t floppy[2];			/* drive unit */
	int drive;							/* selected drive */
	int side;							/* selected side */

	/* devices */
	running_device *pic;
	running_device *crt;
	running_device *ssda;
	running_device *cvsd;
	running_device *ieee488;
};

#endif
