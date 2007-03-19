
/* DISK IMAGE FORMAT WHICH USED TO BE PART OF WD179X - NOW SEPERATED */

#include "devices/flopdrv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define D88_NUM_TRACK 164

typedef struct {
	char C;
	char H;
	char R;
	char N;
	unsigned long flags;
	unsigned long offset;			/* offset of data */
	DENSITY den;
} d88sect;

typedef struct
{
	mess_image	*image_file;			/* file handle for disc image */
	int 	mode;				/* open mode == 0 read only, != 0 read/write */
	unsigned long image_size;		/* size of image file */

	char disk_name[17];			/* name of disk */
	int write_protected;			/* 1 == protected */
	int disktype;				/* 0 == 2D, 1 == 2DD, 2 == 2HD */

	int num_sects[D88_NUM_TRACK];		/* number of sectors */
	d88sect *sects[D88_NUM_TRACK];	/* sector information */

	UINT8   track;                        /* current track # */
} d88image;


DEVICE_INIT(d88image_floppy);
DEVICE_LOAD(d88image_floppy);

/* check write protect */
void d88image_get_write_protect(int id);

#ifdef __cplusplus
}
#endif
