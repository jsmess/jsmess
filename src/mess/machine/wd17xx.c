/***************************************************************************

	wd17xx.c

	Implementations of the Western Digitial 17xx and 19xx families of
	floppy disk controllers


	Kevin Thacker
		- Removed disk image code and replaced it with floppy drive functions.
		  Any disc image is now useable with this code.
		- Fixed write protect
				  
	2005-Apr-16 P.Harvey-Smith:
		- Increased the delay in wd179x_timed_read_sector_request and
		  wd179x_timed_write_sector_request, to 40us to more closely match
		  what happens on the real hardware, this has fixed the NitrOS9 boot
		  problems.
  
	TODO:
		- Multiple record read/write
		- What happens if a track is read that doesn't have any id's on it?
	     (e.g. unformatted disc)

***************************************************************************/


#include "driver.h"
#include "machine/wd17xx.h"
#include "devices/flopdrv.h"


/***************************************************************************

	Parameters

***************************************************************************/

#define VERBOSE			0	/* General logging */
#define VERBOSE_DATA	0	/* Logging of each byte during read and write */

#define DELAY_ERROR		3
#define DELAY_NOTREADY	1
#define DELAY_DATADONE	3



/***************************************************************************

	Constants

***************************************************************************/

#define TYPE_I			1
#define TYPE_II 		2
#define TYPE_III		3
#define TYPE_IV 		4

#define FDC_STEP_RATE   0x03    /* Type I additional flags */
#define FDC_STEP_VERIFY 0x04	/* verify track number */
#define FDC_STEP_HDLOAD 0x08	/* load head */
#define FDC_STEP_UPDATE 0x10	/* update track register */

#define FDC_RESTORE 	0x00	/* Type I commands */
#define FDC_SEEK		0x10
#define FDC_STEP		0x20
#define FDC_STEP_IN 	0x40
#define FDC_STEP_OUT	0x60

#define FDC_MASK_TYPE_I 		(FDC_STEP_HDLOAD|FDC_STEP_VERIFY|FDC_STEP_RATE)

/* Type I commands status */
#define STA_1_BUSY		0x01	/* controller is busy */
#define STA_1_IPL		0x02	/* index pulse */
#define STA_1_TRACK0	0x04	/* track 0 detected */
#define STA_1_CRC_ERR	0x08	/* CRC error */
#define STA_1_SEEK_ERR	0x10	/* seek error */
#define STA_1_HD_LOADED 0x20	/* head loaded */
#define STA_1_WRITE_PRO 0x40	/* floppy is write protected */
#define STA_1_NOT_READY 0x80	/* controller not ready */

/* Type II and III additional flags */
#define FDC_DELETED_AM	0x01	/* read/write deleted address mark */
#define FDC_SIDE_CMP_T	0x02	/* side compare track data */
#define FDC_15MS_DELAY	0x04	/* delay 15ms before command */
#define FDC_SIDE_CMP_S	0x08	/* side compare sector data */
#define FDC_MULTI_REC	0x10	/* only for type II commands */

/* Type II commands */
#define FDC_READ_SEC	0x80	/* read sector */
#define FDC_WRITE_SEC	0xA0	/* write sector */

#define FDC_MASK_TYPE_II		(FDC_MULTI_REC|FDC_SIDE_CMP_S|FDC_15MS_DELAY|FDC_SIDE_CMP_T|FDC_DELETED_AM)

/* Type II commands status */
#define STA_2_BUSY		0x01
#define STA_2_DRQ		0x02
#define STA_2_LOST_DAT	0x04
#define STA_2_CRC_ERR	0x08
#define STA_2_REC_N_FND 0x10
#define STA_2_REC_TYPE	0x20
#define STA_2_WRITE_PRO 0x40
#define STA_2_NOT_READY 0x80

#define FDC_MASK_TYPE_III		(FDC_SIDE_CMP_S|FDC_15MS_DELAY|FDC_SIDE_CMP_T|FDC_DELETED_AM)

/* Type III commands */
#define FDC_READ_DAM	0xc0	/* read data address mark */
#define FDC_READ_TRK	0xe0	/* read track */
#define FDC_WRITE_TRK	0xf0	/* write track (format) */

/* Type IV additional flags */
#define FDC_IM0 		0x01	/* interrupt mode 0 */
#define FDC_IM1 		0x02	/* interrupt mode 1 */
#define FDC_IM2 		0x04	/* interrupt mode 2 */
#define FDC_IM3 		0x08	/* interrupt mode 3 */

#define FDC_MASK_TYPE_IV		(FDC_IM3|FDC_IM2|FDC_IM1|FDC_IM0)

/* Type IV commands */
#define FDC_FORCE_INT	0xd0	/* force interrupt */



/***************************************************************************

	Structures

***************************************************************************/

typedef struct _wd179x_info wd179x_info;
struct _wd179x_info
{
	void   (*callback)(int event);   /* callback for IRQ status */
	DENSITY   density;				/* FM/MFM, single / double density */
	wd179x_type_t type;
	UINT8	track_reg;				/* value of track register */
	UINT8	data;					/* value of data register */
	UINT8	command;				/* last command written */
	UINT8	command_type;			/* last command type */
	UINT8	sector; 				/* current sector # */
	UINT8	head;					/* current head # */

	UINT8	read_cmd;				/* last read command issued */
	UINT8	write_cmd;				/* last write command issued */
	INT8	direction;				/* last step direction */

	UINT8	status; 				/* status register */
	UINT8	status_drq; 			/* status register data request bit */
	UINT8	status_ipl; 			/* status register toggle index pulse bit */
	UINT8	busy_count; 			/* how long to keep busy bit set */

	UINT8	buffer[6144];			/* I/O buffer (holds up to a whole track) */
	UINT32	data_offset;			/* offset into I/O buffer */
	INT32	data_count; 			/* transfer count from/into I/O buffer */

	UINT8	*fmt_sector_data[256];	/* pointer to data after formatting a track */

	UINT8	dam_list[256][4];		/* list of data address marks while formatting */
	int 	dam_data[256];			/* offset to data inside buffer while formatting */
	int 	dam_cnt;				/* valid number of entries in the dam_list */
	UINT16	sector_length;			/* sector length (byte) */

	UINT8	ddam;					/* ddam of sector found - used when reading */
	UINT8	sector_data_id;
	mame_timer	*timer, *timer_rs, *timer_ws, *timer_rid;
	int		data_direction;

	UINT8   ipl;					/* index pulse */
};



/***************************************************************************

	Read track stuff

***************************************************************************/

/* structure describing a double density track */
#define TRKSIZE_DD		6144
#if 0
static UINT8 track_DD[][2] = {
	{16, 0x4e}, 	/* 16 * 4E (track lead in)				 */
	{ 8, 0x00}, 	/*	8 * 00 (pre DAM)					 */
	{ 3, 0xf5}, 	/*	3 * F5 (clear CRC)					 */

	{ 1, 0xfe}, 	/* *** sector *** FE (DAM)				 */
	{ 1, 0x80}, 	/*	4 bytes track,head,sector,seclen	 */
	{ 1, 0xf7}, 	/*	1 * F7 (CRC)						 */
	{22, 0x4e}, 	/* 22 * 4E (sector lead in) 			 */
	{12, 0x00}, 	/* 12 * 00 (pre AM) 					 */
	{ 3, 0xf5}, 	/*	3 * F5 (clear CRC)					 */
	{ 1, 0xfb}, 	/*	1 * FB (AM) 						 */
	{ 1, 0x81}, 	/*	x bytes sector data 				 */
	{ 1, 0xf7}, 	/*	1 * F7 (CRC)						 */
	{16, 0x4e}, 	/* 16 * 4E (sector lead out)			 */
	{ 8, 0x00}, 	/*	8 * 00 (post sector)				 */
	{ 0, 0x00}, 	/* end of data							 */
};
#endif
/* structure describing a single density track */
#define TRKSIZE_SD		3172
#if 0
static UINT8 track_SD[][2] = {
	{16, 0xff}, 	/* 16 * FF (track lead in)				 */
	{ 8, 0x00}, 	/*	8 * 00 (pre DAM)					 */
	{ 1, 0xfc}, 	/*	1 * FC (clear CRC)					 */

	{11, 0xff}, 	/* *** sector *** 11 * FF				 */
	{ 6, 0x00}, 	/*	6 * 00 (pre DAM)					 */
	{ 1, 0xfe}, 	/*	1 * FE (DAM)						 */
	{ 1, 0x80}, 	/*	4 bytes track,head,sector,seclen	 */
	{ 1, 0xf7}, 	/*	1 * F7 (CRC)						 */
	{10, 0xff}, 	/* 10 * FF (sector lead in) 			 */
	{ 4, 0x00}, 	/*	4 * 00 (pre AM) 					 */
	{ 1, 0xfb}, 	/*	1 * FB (AM) 						 */
	{ 1, 0x81}, 	/*	x bytes sector data 				 */
	{ 1, 0xf7}, 	/*	1 * F7 (CRC)						 */
	{ 0, 0x00}, 	/* end of data							 */
};
#endif



/***************************************************************************

	Prototypes

***************************************************************************/

static void wd179x_complete_command(wd179x_info *, int delay);
static void wd179x_clear_data_request(void);
static void wd179x_set_data_request(void);
static void wd179x_timed_data_request(void);
static void wd179x_set_irq(wd179x_info *);
static mame_timer *busy_timer = NULL;

/* one wd controlling multiple drives */
static wd179x_info wd;

/* this is the drive currently selected */
static UINT8 current_drive;

/* this is the head currently selected */
static UINT8 hd = 0;

/**************************************************************************/

static mess_image *wd179x_current_image(void)
{
	return image_from_devtype_and_index(IO_FLOPPY, current_drive);
}



/* use this to determine which drive is controlled by WD */
void wd179x_set_drive(UINT8 drive)
{
	current_drive = drive;
}



void wd179x_set_side(UINT8 head)
{
	if (VERBOSE)
	{
		if (head != hd)
			logerror("wd179x_set_side: $%02x\n", head);
	}
	hd = head;
}



void wd179x_set_density(DENSITY density)
{
	wd179x_info *w = &wd;

	if (VERBOSE)
	{
		if (w->density != density)
			logerror("wd179x_set_density: $%02x\n", density);
	}

	w->density = density;
}



static void	wd179x_busy_callback(int dummy)
{
	wd179x_info *w = (wd179x_info *)dummy;

	wd179x_set_irq(w);			
	timer_reset(busy_timer, TIME_NEVER);
}



static void wd179x_set_busy(wd179x_info *w, double milliseconds)
{
	w->status |= STA_1_BUSY;
	timer_adjust(busy_timer, TIME_IN_MSEC(milliseconds), (int)w, 0);
}



/* BUSY COUNT DOESN'T WORK PROPERLY! */

static void wd179x_restore(wd179x_info *w)
{
	UINT8 step_counter;
	
	if (current_drive >= device_count(IO_FLOPPY))
		return;

	step_counter = 255;
		
#if 0
	w->status |= STA_1_BUSY;
#endif

	/* setup step direction */
	w->direction = -1;

	w->command_type = TYPE_I;

	/* reset busy count */
	w->busy_count = 0;

	if (image_slotexists(wd179x_current_image()))
	{
		/* keep stepping until track 0 is received or 255 steps have been done */
		while (!(floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_HEAD_AT_TRACK_0)) && (step_counter!=0))
		{
			/* update time to simulate seek time busy signal */
			w->busy_count++;
			floppy_drive_seek(wd179x_current_image(), w->direction);
			step_counter--;
		}
	}

	/* update track reg */
	w->track_reg = 0;
#if 0
	/* simulate seek time busy signal */
	w->busy_count = 0;	//w->busy_count * ((w->data & FDC_STEP_RATE) + 1);
	
	/* when command completes set irq */
	wd179x_set_irq(w);
#endif
	wd179x_set_busy(w,0.1);
}



static void	wd179x_busy_callback(int dummy);
static void	wd179x_misc_timer_callback(int code);
static void	wd179x_read_sector_callback(int code);
static void	wd179x_write_sector_callback(int code);
static void wd179x_index_pulse_callback(mess_image *img, int state);

void wd179x_reset(void)
{
	int i;
	for (i = 0; i < device_count(IO_FLOPPY); i++)
	{
		mess_image *img = image_from_devtype_and_index(IO_FLOPPY, i);
		floppy_drive_set_index_pulse_callback(img, wd179x_index_pulse_callback);    
		floppy_drive_set_rpm( img, 300.);
	}

	wd179x_restore(&wd);
}



void wd179x_init(wd179x_type_t type, void (*callback)(int))
{
	memset(&wd, 0, sizeof(wd));
	wd.status = STA_1_TRACK0;
	wd.type = type;
	wd.callback = callback;
//	wd.status_ipl = STA_1_IPL;
	wd.density = DEN_MFM_LO;
	busy_timer = mame_timer_alloc(wd179x_busy_callback);
	wd.timer = mame_timer_alloc(wd179x_misc_timer_callback);
	wd.timer_rs = mame_timer_alloc(wd179x_read_sector_callback);
	wd.timer_ws = mame_timer_alloc(wd179x_write_sector_callback);

	wd179x_reset();
}



/* track writing, converted to format commands */
static void write_track(wd179x_info * w)
{
	int i;
	for (i=0;i+4<w->data_offset;)
	{
		if (w->buffer[i]==0xfe)
		{
			/* got address mark */
			int track   = w->buffer[i+1];
			int side    = w->buffer[i+2];
			int sector  = w->buffer[i+3];
			int len     = w->buffer[i+4]; 
			int filler  = 0xe5; /* IBM and Thomson */
			int density = w->density;
			floppy_drive_format_sector(wd179x_current_image(),side,sector,track,
						hd,sector,density?1:0,filler);
			(void)len;
			i += 128; /* at least... */
		}
		else
			i++;
	}
}



/* read an entire track */
static void read_track(wd179x_info * w)
{
#if 0
	UINT8 *psrc;		/* pointer to track format structure */
	UINT8 *pdst;		/* pointer to track buffer */
	int cnt;			/* number of bytes to fill in */
	UINT16 crc; 		/* id or data CRC */
	UINT8 d;			/* data */
	UINT8 t = w->track; /* track of DAM */
	UINT8 h = w->head;	/* head of DAM */
	UINT8 s = w->sector_dam;		/* sector of DAM */
	UINT16 l = w->sector_length;	/* sector length of DAM */
	int i;

	for (i = 0; i < w->sec_per_track; i++)
	{
		w->dam_list[i][0] = t;
		w->dam_list[i][1] = h;
		w->dam_list[i][2] = i;
		w->dam_list[i][3] = l >> 8;
	}

	pdst = w->buffer;

	if (w->density)
	{
		psrc = track_DD[0];    /* double density track format */
		cnt = TRKSIZE_DD;
	}
	else
	{
		psrc = track_SD[0];    /* single density track format */
		cnt = TRKSIZE_SD;
	}

	while (cnt > 0)
	{
		if (psrc[0] == 0)	   /* no more track format info ? */
		{
			if (w->dam_cnt < w->sec_per_track) /* but more DAM info ? */
			{
				if (w->density)/* DD track ? */
					psrc = track_DD[3];
				else
					psrc = track_SD[3];
			}
		}

		if (psrc[0] != 0)	   /* more track format info ? */
		{
			cnt -= psrc[0];    /* subtract size */
			d = psrc[1];

			if (d == 0xf5)	   /* clear CRC ? */
			{
				crc = 0xffff;
				d = 0xa1;	   /* store A1 */
			}

			for (i = 0; i < *psrc; i++)
				*pdst++ = d;   /* fill data */

			if (d == 0xf7)	   /* store CRC ? */
			{
				pdst--; 	   /* go back one byte */
				*pdst++ = crc & 255;	/* put CRC low */
				*pdst++ = crc / 256;	/* put CRC high */
				cnt -= 1;	   /* count one more byte */
			}
			else if (d == 0xfe)/* address mark ? */
			{
				crc = 0xffff;	/* reset CRC */
			}
			else if (d == 0x80)/* sector ID ? */
			{
				pdst--; 	   /* go back one byte */
				t = *pdst++ = w->dam_list[w->dam_cnt][0]; /* track number */
				h = *pdst++ = w->dam_list[w->dam_cnt][1]; /* head number */
				s = *pdst++ = w->dam_list[w->dam_cnt][2]; /* sector number */
				l = *pdst++ = w->dam_list[w->dam_cnt][3]; /* sector length code */
				w->dam_cnt++;
				calc_crc(&crc, t);	/* build CRC */
				calc_crc(&crc, h);	/* build CRC */
				calc_crc(&crc, s);	/* build CRC */
				calc_crc(&crc, l);	/* build CRC */
				l = (l == 0) ? 128 : l << 8;
			}
			else if (d == 0xfb)// data address mark ?
			{
				crc = 0xffff;	// reset CRC
			}
			else if (d == 0x81)// sector DATA ?
			{
				pdst--; 	   /* go back one byte */
				if (seek(w, t, h, s) == 0)
				{
					if (mame_fread(w->image_file, pdst, l) != l)
					{
						w->status = STA_2_CRC_ERR;
						return;
					}
				}
				else
				{
					w->status = STA_2_REC_N_FND;
					return;
				}
				for (i = 0; i < l; i++) // build CRC of all data
					calc_crc(&crc, *pdst++);
				cnt -= l;
			}
			psrc += 2;
		}
		else
		{
			*pdst++ = 0xff;    /* fill track */
			cnt--;			   /* until end */
		}
	}
#endif

	w->data_count = (w->density) ? TRKSIZE_DD : TRKSIZE_SD;

	floppy_drive_read_track_data_info_buffer( wd179x_current_image(), hd, (char *)w->buffer, &(w->data_count) );
	
	w->data_offset = 0;

	wd179x_set_data_request();
	w->status |= STA_2_BUSY;
	w->busy_count = 0;
}



/* calculate CRC for data address marks or sector data */
static void calc_crc(UINT16 * crc, UINT8 value)
{
	UINT8 l, h;

	l = value ^ (*crc >> 8);
	*crc = (*crc & 0xff) | (l << 8);
	l >>= 4;
	l ^= (*crc >> 8);
	*crc <<= 8;
	*crc = (*crc & 0xff00) | l;
	l = (l << 4) | (l >> 4);
	h = l;
	l = (l << 2) | (l >> 6);
	l &= 0x1f;
	*crc = *crc ^ (l << 8);
	l = h & 0xf0;
	*crc = *crc ^ (l << 8);
	l = (h << 1) | (h >> 7);
	l &= 0xe0;
	*crc = *crc ^ l;
}



/* read the next data address mark */
static void wd179x_read_id(wd179x_info * w)
{
	chrn_id id;

	w->status &= ~(STA_2_CRC_ERR | STA_2_REC_N_FND);

	/* get next id from disc */
	if (floppy_drive_get_next_id(wd179x_current_image(), hd, &id))
	{
		UINT16 crc = 0xffff;

		w->data_offset = 0;
		w->data_count = 6;

		/* for MFM */
		/* crc includes 3x0x0a1, and 1x0x0fe (id mark) */
		calc_crc(&crc,0x0a1);
		calc_crc(&crc,0x0a1);
		calc_crc(&crc,0x0a1);
		calc_crc(&crc,0x0fe);

		w->buffer[0] = id.C;
		w->buffer[1] = id.H;
		w->buffer[2] = id.R;
		w->buffer[3] = id.N;
		calc_crc(&crc, w->buffer[0]);
		calc_crc(&crc, w->buffer[1]);
		calc_crc(&crc, w->buffer[2]);
		calc_crc(&crc, w->buffer[3]);
		/* crc is stored hi-byte followed by lo-byte */
		w->buffer[4] = crc>>8;
		w->buffer[5] = crc & 255;
		
		w->sector = id.C;

		w->status |= STA_2_BUSY;
		w->busy_count = 0;

		wd179x_complete_command(w, DELAY_DATADONE);

		logerror("read id succeeded.\n");
	}
	else
	{
		/* record not found */
		w->status |= STA_2_REC_N_FND;
		//w->sector = w->track_reg;
		logerror("read id failed\n");

		wd179x_complete_command(w, DELAY_ERROR);
	}
}



static void wd179x_index_pulse_callback(mess_image *img, int state)
{
	wd179x_info *w = &wd;
	if ( img != wd179x_current_image() )
		return;
	w->ipl = state;
}



static int wd179x_has_side_select(void)
{
	return (wd.type == WD_TYPE_1773) || (wd.type == WD_TYPE_1793) || (wd.type == WD_TYPE_2793);
}



static int wd179x_find_sector(wd179x_info *w)
{
	UINT8 revolution_count;
	chrn_id id;

	revolution_count = 0;

	w->status &= ~STA_2_REC_N_FND;

	while (revolution_count!=4)
	{
		if (floppy_drive_get_next_id(wd179x_current_image(), hd, &id))
		{
			/* compare track */
			if (id.C == w->track_reg)
			{
				/* compare head, if we were asked to */
				if (!wd179x_has_side_select() || (id.H == w->head) || (w->head == (UINT8) ~0))
				{
					/* compare id */
					if (id.R == w->sector)
					{
						w->sector_length = 1<<(id.N+7);
						w->sector_data_id = id.data_id;
						/* get ddam status */
						w->ddam = id.flags & ID_FLAG_DELETED_DATA;
						/* got record type here */
						if (VERBOSE)
							logerror("sector found! C:$%02x H:$%02x R:$%02x N:$%02x%s\n", id.C, id.H, id.R, id.N, w->ddam ? " DDAM" : "");
						return 1;
					}
				}
			}
		}

		 /* index set? */
		if (floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_INDEX))
		{
			/* update revolution count */
			revolution_count++;
		}
	}

	/* record not found */
	w->status |= STA_2_REC_N_FND;

	if (VERBOSE)
		logerror("track %d sector %d not found!\n", w->track_reg, w->sector);

	wd179x_complete_command(w, DELAY_ERROR);

	return 0;
}



/* read a sector */
static void wd179x_read_sector(wd179x_info *w)
{
	w->data_offset = 0;

	/* side compare? */
	if (w->read_cmd & 0x02)
		w->head = (w->read_cmd & 0x08) ? 1 : 0;
	else
		w->head = ~0;

	if (wd179x_find_sector(w))
	{
		w->data_count = w->sector_length;

		/* read data */
		floppy_drive_read_sector_data(wd179x_current_image(), hd, w->sector_data_id, (char *)w->buffer, w->sector_length);

		wd179x_timed_data_request();

		w->status |= STA_2_BUSY;
		w->busy_count = 0;
	}
}



static void	wd179x_set_irq(wd179x_info *w)
{
	w->status &= ~STA_2_BUSY;
	/* generate an IRQ */
	if (w->callback)
		(*w->callback) (WD179X_IRQ_SET);
}



/* 0=command callback; 1=data callback */
enum
{
	MISCCALLBACK_COMMAND,
	MISCCALLBACK_DATA
};



static void wd179x_misc_timer_callback(int callback_type)
{
	wd179x_info *w = &wd;

	switch(callback_type) {
	case MISCCALLBACK_COMMAND:
		/* command callback */
		wd179x_set_irq(w);
		break;

	case MISCCALLBACK_DATA:
		/* data callback */
		/* ok, trigger data request now */
		wd179x_set_data_request();
		break;
	}

	/* stop it, but don't allow it to be free'd */
	timer_reset(w->timer, TIME_NEVER); 
}



/* called on error, or when command is actually completed */
/* KT - I have used a timer for systems that use interrupt driven transfers.
A interrupt occurs after the last byte has been read. If it occurs at the time
when the last byte has been read it causes problems - same byte read again
or bytes missed */
/* TJL - I have add a parameter to allow the emulation to specify the delay
*/
static void wd179x_complete_command(wd179x_info *w, int delay)
{
	int usecs;

	w->data_count = 0;

	/* clear busy bit */
	w->status &= ~STA_2_BUSY;

	usecs = floppy_drive_get_datarate_in_us(w->density);
	usecs *= delay;

	/* set new timer */
	timer_adjust(w->timer, TIME_IN_USEC(usecs), MISCCALLBACK_COMMAND, 0);
}



static void wd179x_write_sector(wd179x_info *w)
{
	/* at this point, the disc is write enabled, and data
	 * has been transfered into our buffer - now write it to
	 * the disc image or to the real disc
	 */

	/* side compare? */
	if (w->write_cmd & 0x02)
		w->head = (w->write_cmd & 0x08) ? 1 : 0;
	else
		w->head = ~0;

	/* find sector */
	if (wd179x_find_sector(w))
	{
		w->data_count = w->sector_length;

		/* write data */
		floppy_drive_write_sector_data(wd179x_current_image(), hd, w->sector_data_id, (char *)w->buffer, w->sector_length,w->write_cmd & 0x01);
	}
}



/* verify the seek operation by looking for a id that has a matching track value */
static void wd179x_verify_seek(wd179x_info *w)
{
	UINT8 revolution_count;
	chrn_id id;

	revolution_count = 0;

	logerror("doing seek verify\n");

	w->status &= ~STA_1_SEEK_ERR;

	/* must be found within 5 revolutions otherwise error */
	while (revolution_count!=5)
	{
		if (floppy_drive_get_next_id(wd179x_current_image(), hd, &id))
		{
			/* compare track */
			if (id.C == w->track_reg)
			{
				logerror("seek verify succeeded!\n");
				return;
			}
		}

		 /* index set? */
		if (floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_INDEX))
		{
			/* update revolution count */
			revolution_count++;
		}
	}

	w->status |= STA_1_SEEK_ERR;

	logerror("failed seek verify!");
}



/* clear a data request */
static void wd179x_clear_data_request(void)
{
	wd179x_info *w = &wd;

//	w->status_drq = 0;
	if (w->callback)
		(*w->callback) (WD179X_DRQ_CLR);
	w->status &= ~STA_2_DRQ;
}



/* set data request */
static void wd179x_set_data_request(void)
{
	wd179x_info *w = &wd;

	if (w->status & STA_2_DRQ)
	{
		w->status |= STA_2_LOST_DAT;
//		return;
	}

	/* set drq */
//	w->status_drq = STA_2_DRQ;
	if (w->callback)
		(*w->callback) (WD179X_DRQ_SET);
	w->status |= STA_2_DRQ;
}



/* callback to initiate read sector */
static void	wd179x_read_sector_callback(int code)
{
	wd179x_info *w = &wd;

	/* ok, start that read! */

	if (VERBOSE)
		logerror("wd179x: Read Sector callback.\n");

	if (!floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_READY))
		wd179x_complete_command(w, DELAY_NOTREADY);
	else
		wd179x_read_sector(w);

	/* stop it, but don't allow it to be free'd */
	timer_reset(w->timer_rs, TIME_NEVER); 
}



/* callback to initiate write sector */
static void	wd179x_write_sector_callback(int code)
{
	wd179x_info *w = &wd;

	/* ok, start that write! */

	if (VERBOSE)
		logerror("wd179x: Write Sector callback.\n");

	if (!floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_READY))
		wd179x_complete_command(w, DELAY_NOTREADY);
	else
	{

		/* drive write protected? */
		if (floppy_drive_get_flag_state(wd179x_current_image(),FLOPPY_DRIVE_DISK_WRITE_PROTECTED))
		{
			w->status |= STA_2_WRITE_PRO;

			wd179x_complete_command(w, DELAY_ERROR);
		}
		else
		{
			/* side compare? */
			if (w->write_cmd & 0x02)
				w->head = (w->write_cmd & 0x08) ? 1 : 0;
			else
				w->head = ~0;

			/* attempt to find it first before getting data from cpu */
			if (wd179x_find_sector(w))
			{
				/* request data */
				w->data_offset = 0;
				w->data_count = w->sector_length;

				wd179x_set_data_request();

				w->status |= STA_2_BUSY;
				w->busy_count = 0;
			}
		}
	}

	/* stop it, but don't allow it to be free'd */
	timer_reset(w->timer_ws, TIME_NEVER); 
}



/* setup a timed data request - data request will be triggered in a few usecs time */
static void wd179x_timed_data_request(void)
{
	int usecs;
	wd179x_info *w = &wd;

	usecs = floppy_drive_get_datarate_in_us(w->density);

	/* set new timer */
	timer_adjust(w->timer, TIME_IN_USEC(usecs), MISCCALLBACK_DATA, 0);
}



/* setup a timed read sector - read sector will be triggered in a few usecs time */
static void wd179x_timed_read_sector_request(void)
{
	int usecs;
	wd179x_info *w = &wd;

	usecs = 40; /* How long should we wait? How about 40 micro seconds? */

	/* set new timer */
	timer_reset(w->timer_rs, TIME_IN_USEC(usecs));
}



/* setup a timed write sector - write sector will be triggered in a few usecs time */
static void wd179x_timed_write_sector_request(void)
{
	int usecs;
	wd179x_info *w = &wd;

	usecs = 40; /* How long should we wait? How about 40 micro seconds? */

	/* set new timer */
	timer_reset(w->timer_ws, TIME_IN_USEC(usecs));
}



/* read the FDC status register. This clears IRQ line too */
 READ8_HANDLER ( wd179x_status_r )
{
	wd179x_info *w = &wd;
	int result = w->status;

	if (w->callback)
		(*w->callback) (WD179X_IRQ_CLR);
//	if (w->busy_count)
//	{
//		if (!--w->busy_count)
//			w->status &= ~STA_1_BUSY;
//	}

	/* type 1 command or force int command? */
	if ((w->command_type==TYPE_I) || (w->command_type==TYPE_IV))
	{
		/* toggle index pulse */
		result &= ~STA_1_IPL;
		if (w->ipl) result |= STA_1_IPL;

		/* set track 0 state */
		result &=~STA_1_TRACK0;
		if (floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_HEAD_AT_TRACK_0))
			result |= STA_1_TRACK0;

	//	floppy_drive_set_ready_state(wd179x_current_image(), 1,1);
		w->status &= ~STA_1_NOT_READY;
		
		/* TODO: What is this?  We need some more info on this */
		if ((w->type == WD_TYPE_179X) || (w->type == WD_TYPE_1773))
		{
			if (!floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_READY))
				w->status |= STA_1_NOT_READY;
		}
		else
		{
			if (floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_READY))
				w->status |= STA_1_NOT_READY;
		}
	}
	
	/* eventually set data request bit */
//	w->status |= w->status_drq;

	if (VERBOSE)
	{
		if (w->data_count < 4)
			logerror("wd179x_status_r: $%02X (data_count %d)\n", result, w->data_count);
	}

	return result;
}



/* read the FDC track register */
READ8_HANDLER ( wd179x_track_r )
{
	wd179x_info *w = &wd;

	if (VERBOSE)
		logerror("wd179x_track_r: $%02X\n", w->track_reg);

	return w->track_reg;
}



/* read the FDC sector register */
READ8_HANDLER ( wd179x_sector_r )
{
	wd179x_info *w = &wd;

	if (VERBOSE)
		logerror("wd179x_sector_r: $%02X\n", w->sector);

	return w->sector;
}



/* read the FDC data register */
 READ8_HANDLER ( wd179x_data_r )
{
	wd179x_info *w = &wd;

	if (w->data_count >= 1)
	{
		/* clear data request */
		wd179x_clear_data_request();

		/* yes */
		w->data = w->buffer[w->data_offset++];

		if (VERBOSE_DATA)
			logerror("wd179x_data_r: $%02X (data_count %d)\n", w->data, w->data_count);

		/* any bytes remaining? */
		if (--w->data_count < 1)
		{
			/* no */
			w->data_offset = 0;

			/* clear ddam type */
			w->status &=~STA_2_REC_TYPE;
			/* read a sector with ddam set? */
			if (w->command_type == TYPE_II && w->ddam != 0)
			{
				/* set it */
				w->status |= STA_2_REC_TYPE;
			}

			/* not incremented after each sector - only incremented in multi-sector
			operation. If this remained as it was oric software would not run! */
		//	w->sector++;
			/* Delay the INTRQ 3 byte times becuase we need to read two CRC bytes and
			   compare them with a calculated CRC */
			wd179x_complete_command(w, DELAY_DATADONE);

			if (VERBOSE)
				logerror("wd179x_data_r(): data read completed\n");
		}
		else
		{
			/* issue a timed data request */
			wd179x_timed_data_request();		
		}
	}
	else
	{
		logerror("wd179x_data_r: (no new data) $%02X (data_count %d)\n", w->data, w->data_count);
	}
	return w->data;
}



/* write the FDC command register */
WRITE8_HANDLER ( wd179x_command_w )
{
	wd179x_info *w = &wd;
	
	floppy_drive_set_motor_state(wd179x_current_image(), 1);
	floppy_drive_set_ready_state(wd179x_current_image(), 1,0);
	/* also cleared by writing command */
	if (w->callback)
		(*w->callback) (WD179X_IRQ_CLR);

	/* clear write protected. On read sector, read track and read dam, write protected bit is clear */
	w->status &= ~((1<<6) | (1<<5) | (1<<4));

	if ((data & ~FDC_MASK_TYPE_IV) == FDC_FORCE_INT)
	{
		if (VERBOSE)
			logerror("wd179x_command_w $%02X FORCE_INT (data_count %d)\n", data, w->data_count);

		w->data_count = 0;
		w->data_offset = 0;
		w->status &= ~(STA_2_BUSY);
		
		wd179x_clear_data_request();

		if (data & 0x0f)
		{



		}


//		w->status_ipl = STA_1_IPL;
/*		w->status_ipl = 0; */
		
		w->busy_count = 0;
		w->command_type = TYPE_IV;
        return;
	}

	if (data & 0x80)
	{
		/*w->status_ipl = 0;*/

		if ((data & ~FDC_MASK_TYPE_II) == FDC_READ_SEC)
		{
			if (VERBOSE)
				logerror("wd179x_command_w $%02X READ_SEC\n", data);

			w->read_cmd = data;
			w->command = data & ~FDC_MASK_TYPE_II;
			w->command_type = TYPE_II;
			w->status &= ~STA_2_LOST_DAT;
			wd179x_clear_data_request();

			wd179x_timed_read_sector_request();

			return;
		}

		if ((data & ~FDC_MASK_TYPE_II) == FDC_WRITE_SEC)
		{
			if (VERBOSE)
				logerror("wd179x_command_w $%02X WRITE_SEC\n", data);

			w->write_cmd = data;
			w->command = data & ~FDC_MASK_TYPE_II;
			w->command_type = TYPE_II;
			w->status &= ~STA_2_LOST_DAT;
			wd179x_clear_data_request();

			wd179x_timed_write_sector_request();

			return;
		}

		if ((data & ~FDC_MASK_TYPE_III) == FDC_READ_TRK)
		{
			if (VERBOSE)
				logerror("wd179x_command_w $%02X READ_TRK\n", data);

			w->command = data & ~FDC_MASK_TYPE_III;
			w->command_type = TYPE_III;
			w->status &= ~STA_2_LOST_DAT;
			wd179x_clear_data_request();
#if 1
//			w->status = seek(w, w->track, w->head, w->sector);
			if (w->status == 0)
				read_track(w);
#endif
			return;
		}

		if ((data & ~FDC_MASK_TYPE_III) == FDC_WRITE_TRK)
		{
			if (VERBOSE)
				logerror("wd179x_command_w $%02X WRITE_TRK\n", data);

			w->command_type = TYPE_III;
			w->status &= ~STA_2_LOST_DAT;
			wd179x_clear_data_request();

			if (!floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_READY))
            {
				wd179x_complete_command(w, DELAY_NOTREADY);
            }
            else
            {
    
                /* drive write protected? */
                if (floppy_drive_get_flag_state(wd179x_current_image(),FLOPPY_DRIVE_DISK_WRITE_PROTECTED))
                {
                    /* yes */
                    w->status |= STA_2_WRITE_PRO;
                    /* quit command */
                    wd179x_complete_command(w, DELAY_ERROR);
                }
                else
                {
                    w->command = data & ~FDC_MASK_TYPE_III;
                    w->data_offset = 0;
                    w->data_count = (w->density) ? TRKSIZE_DD : TRKSIZE_SD;
                    wd179x_set_data_request();

                    w->status |= STA_2_BUSY;
                    w->busy_count = 0;
                }
            }
            return;
		}

		if ((data & ~FDC_MASK_TYPE_III) == FDC_READ_DAM)
		{
			if (VERBOSE)
				logerror("wd179x_command_w $%02X READ_DAM\n", data);

			w->command_type = TYPE_III;
			w->status &= ~STA_2_LOST_DAT;
  			wd179x_clear_data_request();

			if (!floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_READY))
            {
				wd179x_complete_command(w, DELAY_NOTREADY);
            }
            else
            {
                wd179x_read_id(w);
            }
			return;
		}

		if (VERBOSE)
			logerror("wd179x_command_w $%02X unknown\n", data);

		return;
	}

	w->status |= STA_1_BUSY;
	
	/* clear CRC error */
	w->status &=~STA_1_CRC_ERR;

	if ((data & ~FDC_MASK_TYPE_I) == FDC_RESTORE)
	{
		if (VERBOSE)
			logerror("wd179x_command_w $%02X RESTORE\n", data);

		wd179x_restore(w);
	}

	if ((data & ~FDC_MASK_TYPE_I) == FDC_SEEK)
	{
		UINT8 newtrack;

		if (VERBOSE)
			logerror("old track: $%02x new track: $%02x\n", w->track_reg, w->data);
		w->command_type = TYPE_I;

		/* setup step direction */
		if (w->track_reg < w->data)
		{
			if (VERBOSE)
				logerror("direction: +1\n");

			w->direction = 1;
		}
		else if (w->track_reg > w->data)
        {
			if (VERBOSE)
				logerror("direction: -1\n");

			w->direction = -1;
		}

		newtrack = w->data;
		if (VERBOSE)
			logerror("wd179x_command_w $%02X SEEK (data_reg is $%02X)\n", data, newtrack);

		/* reset busy count */
		w->busy_count = 0;

		/* keep stepping until reached track programmed */
		while (w->track_reg != newtrack)
		{
			/* update time to simulate seek time busy signal */
			w->busy_count++;

			/* update track reg */
			w->track_reg += w->direction;

			floppy_drive_seek(wd179x_current_image(), w->direction);
		}

		/* simulate seek time busy signal */
		w->busy_count = 0;	//w->busy_count * ((data & FDC_STEP_RATE) + 1);
#if 0
		wd179x_set_irq(w);
#endif
		wd179x_set_busy(w,0.1);

	}

	if ((data & ~(FDC_STEP_UPDATE | FDC_MASK_TYPE_I)) == FDC_STEP)
	{
		if (VERBOSE)
			logerror("wd179x_command_w $%02X STEP dir %+d\n", data, w->direction);

		w->command_type = TYPE_I;
        /* if it is a real floppy, issue a step command */
		/* simulate seek time busy signal */
		w->busy_count = 0;	//((data & FDC_STEP_RATE) + 1);

		floppy_drive_seek(wd179x_current_image(), w->direction);

		if (data & FDC_STEP_UPDATE)
			w->track_reg += w->direction;

#if 0
		wd179x_set_irq(w);
#endif
		wd179x_set_busy(w,0.1);


	}

	if ((data & ~(FDC_STEP_UPDATE | FDC_MASK_TYPE_I)) == FDC_STEP_IN)
	{
		if (VERBOSE)
			logerror("wd179x_command_w $%02X STEP_IN\n", data);

		w->command_type = TYPE_I;
        w->direction = +1;
		/* simulate seek time busy signal */
		w->busy_count = 0;	//((data & FDC_STEP_RATE) + 1);

		floppy_drive_seek(wd179x_current_image(), w->direction);

		if (data & FDC_STEP_UPDATE)
			w->track_reg += w->direction;
#if 0
		wd179x_set_irq(w);
#endif
		wd179x_set_busy(w,0.1);

	}

	if ((data & ~(FDC_STEP_UPDATE | FDC_MASK_TYPE_I)) == FDC_STEP_OUT)
	{
		if (VERBOSE)
			logerror("wd179x_command_w $%02X STEP_OUT\n", data);

		w->command_type = TYPE_I;
        w->direction = -1;
		/* simulate seek time busy signal */
		w->busy_count = 0;	//((data & FDC_STEP_RATE) + 1);

		/* for now only allows a single drive to be selected */
		floppy_drive_seek(wd179x_current_image(), w->direction);

		if (data & FDC_STEP_UPDATE)
			w->track_reg += w->direction;

#if 0
		wd179x_set_irq(w);
#endif
		wd179x_set_busy(w,0.1);
	}

//	if (w->busy_count==0)
//		w->status &= ~STA_1_BUSY;

//	/* toggle index pulse at read */
//	w->status_ipl = STA_1_IPL;

	/* 0 is enable spin up sequence, 1 is disable spin up sequence */
	if ((data & FDC_STEP_HDLOAD)==0)
		w->status |= STA_1_HD_LOADED;

	if (data & FDC_STEP_VERIFY)
	{
		/* verify seek */
		wd179x_verify_seek(w);
	}
}



/* write the FDC track register */
WRITE8_HANDLER ( wd179x_track_w )
{
	wd179x_info *w = &wd;
	w->track_reg = data;

	if (VERBOSE)
		logerror("wd179x_track_w $%02X\n", data);
}



/* write the FDC sector register */
WRITE8_HANDLER ( wd179x_sector_w )
{
	wd179x_info *w = &wd;
	w->sector = data;
	if (VERBOSE)
		logerror("wd179x_sector_w $%02X\n", data);
}



/* write the FDC data register */
WRITE8_HANDLER ( wd179x_data_w )
{
	wd179x_info *w = &wd;

	if (w->data_count > 0)
	{
		/* clear data request */
		wd179x_clear_data_request();

		/* put byte into buffer */
		if (VERBOSE_DATA)
			logerror("wd179x_info buffered data: $%02X at offset %d.\n", data, w->data_offset);
	
		w->buffer[w->data_offset++] = data;
		
		if (--w->data_count < 1)
		{
			if (w->command == FDC_WRITE_TRK)
				write_track(w);
			else
				wd179x_write_sector(w);

			w->data_offset = 0;

			wd179x_complete_command(w, DELAY_DATADONE);
		}
		else
		{
			/* yes... setup a timed data request */
			wd179x_timed_data_request();
		}

	}
	else
	{
		if (VERBOSE)
			logerror("wd179x_data_w $%02X\n", data);
	}
	w->data = data;
}



 READ8_HANDLER( wd179x_r )
{
	UINT8 result = 0;

	switch(offset % 4) {
	case 0: 
		result = wd179x_status_r(0);
		break;
	case 1: 
		result = wd179x_track_r(0);
		break;
	case 2: 
		result = wd179x_sector_r(0);
		break;
	case 3: 
		result = wd179x_data_r(0);
		break;
	}
	return result;
}



WRITE8_HANDLER( wd179x_w )
{
	switch(offset % 4) {
	case 0:
		wd179x_command_w(0, data);
		break;
	case 1:
		wd179x_track_w(0, data);
		break;
	case 2:
		wd179x_sector_w(0, data);
		break;
	case 3:
		wd179x_data_w(0, data);
		break;
	}
}

