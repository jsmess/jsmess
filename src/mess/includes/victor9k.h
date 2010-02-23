#ifndef __VICTOR9K__
#define __VICTOR9K__

#define SCREEN_TAG		"screen"
#define I8088_TAG		"i8088"
#define I8253_TAG		"i8253"
#define I8259A_TAG		"i8259"
#define UPD7201_TAG		"upd7201"
#define HD46505S_TAG	"hd46505s"
#define MC6852_TAG		"mc6852"
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

typedef struct _victor9k_state victor9k_state;
struct _victor9k_state
{
	/* video state */
	UINT8 *video_ram;
	int vert;

	/* interrupts */
	int via1_irq;
	int via2_irq;
	int via3_irq;
	int via4_irq;
	int via5_irq;
	int via6_irq;

	/* floppy state */
	victor9k_drive_t floppy[2];			/* drive unit */
	int drive;							/* selected drive */
	int side;							/* selected side */

	/* devices */
	running_device *pic;
	running_device *crt;
	running_device *ssda;
	running_device *ieee488;
};

#endif
