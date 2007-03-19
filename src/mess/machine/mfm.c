
#define MAX_MFM_DRIVES 4

#define MFM_TRACK_SIZE			6250
#define DISC_ROTATION_SPEED		300

struct mfm_track
{
	/* used to speed up calculations */
	/* these hold the number of id marks on the track
	and the number of data marks */
	int		num_id_marks;
	int		num_data_marks;
	unsigned long *id_mark_offsets;
	unsigned long *data_mark_offsets;

	/* density this track is written in */
	DENSITY	density;
};

struct mfm_drive
{
	/* current track */
	unsigned long current_track;
	/* current side */
	unsigned long current_side;
	/* state of motor */
	int		motor_state;
	/* timer for index pulse */
	void	*index_timer;

	/* when index pulse occurs, execute callback */
	/* an index pulse will occur if drive is connected,
	and motor is on */
	void (*index_pulse_callback)(int drive_id);
};

struct mfm_data
{
	/* setup by mfm_set_density */
	double data_rate_time;
	/* setup by mfm_set_density */
	double track_time;
}

static struct mfm_drive drives[MAX_MFM_DRIVES];

/* get time in cycles for a complete track */
double	mfm_get_track_time(void)
{
	return mfm.track_time;
	MFM_TRACK_SIZE * mfm_time_data_rate();
}



	TIME_IN_US(32);



double	mfm_time_data_rate(void)
{
	return mfm.data_rate_time;
}



/* get pointer to drive info */
static struct mfm_drive *mfm_get_drive(int drive_id)
{
	if ((drive_id<0) || (drive_id>=MAX_MFM_DRIVES))
		return NULL;

	return &drives[drive_id];
}

/* called every 300rpm when index is sensed */
static void	index_timer_callback(int drive_id)
{
	struct mfm_drive *drive = mfm_get_drive(drive_id);
	
	if (drive!=NULL)
	{
		if (drive->index_pulse_callback)
			drive->index_pulse_callback(drive_id);
	}
}

void	mfm_drive_init(int drive_id)
{
	struct mfm_drive *drive = mfm_get_drive(drive_id);

	if (drive!=NULL)
	{
		drive->index_timer = NULL;
		drive->index_pulse_callback = NULL;
	}
}

void	mfm_drive_exit(int drive_id)
{
	struct mfm_drive *drive = mfm_get_drive(drive_id);

	if (drive!=NULL)
	{
		if (drive->index_timer!=NULL)
		{
			timer_reset(drive->index_timer, TIME_NEVER);	/* FIXME - timers should only be allocated once */
			drive->index_timer = NULL;
		}
	}
}

/* set callback function when index pulses */
void	mfm_drive_set_index_callback(int drive_id, void (*callback)(int,int))
{
	struct mfm_drive *drive = mfm_get_drive(drive_id);

	if (drive!=NULL)
	{
		drive->index_pulse_callback = callback;
	}
}


void	mfm_drive_motor(int drive_id, int motor)
{
	struct mfm_drive *drive = mfm_get_drive(drive_id);

	if (drive==NULL)
		return;

	if (motor^drive->motor_state)
	{
		/* motor changed state */

		/* shut off timer */
		if (drive->index_timer!=NULL)
		{
			timer_reset(drive->index_timer, TIME_NEVER);	/* FIXME - timers should only be allocated once */
			drive->index_timer = NULL;
		}

		if (motor!=0)
		{
			/* switched on. start timer */
			/* this is not quite correct. if the motor is stopped in the middle
			of the track, the next index pulse will occur in half a track time, from
			there after it will occur every 300hz */
			drive->index_timer = timer_pulse(TIME_IN_HZ(300), drive_id, index_timer_callback);
		}

		drive->motor_state = motor;
	}
	
}



/* this function is called when a drive/side is selected by the floppy disc controller */
void	mfm_drive_select(int drive_id, int side)
{




}



/* 300rpm -> 5 revolutions per second. */
/* 32us per byte, 31250 bytes per second, 31250/5 = 6250 bytes per revolution */


/* initialise crc */
static void mfm_init_crc(unsigned short *crc)
{
	*crc = 0x0ffff;
}

static void mfm_update_crc(unsigned short *crc, unsigned char byte)
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

static unsigned char mfm_get_byte(int drive_id)
{


}

int		mfm_get_track_size(DENSITY density)
{
	switch (density)
	{
		case DEN_MFM_LO:
		{


		}
		break;
	}
}


unsigned char *mfm_alloc_internal_image(int num_tracks, int num_sides, DENSITY density)
{
	unsigned char *image;



}


/* write byte to buffer without updating crc */
#define mfm_put_byte_to_buffer(buffer_ptr, data) \
	buffer_ptr[0] = data; \
	buffer++; \
	buffer = buffer % MFM_TRACK_SIZE

/* write byte to buffer, update position in buffer, and update crc */
#define mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, data, crc_ptr) \
	buffer_ptr[0] = data; \
	buffer++; \
	buffer = buffer % MFM_TRACK_SIZE; \
	mfm_update_crc(crc_ptr,data)

/* helper function to setup id. crc will be valid for id */
void	mfm_put_id(unsigned char **buffer, unsigned char c, unsigned char h, unsigned char r, unsigned char n)
{
	unsigned short crc;
	unsigned char *buffer_ptr = *buffer;

	/* 12 * 0x00 */
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);

	/* crc includes 3 * 0x0a1 and id mark */
	/* init crc */
	mfm_init_crc(&crc);
	/* 3 bytes a1 */
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x0a1, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x0a1, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x0a1, &crc);
	/* id mark */	
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, MFM_ID_MARK, &crc);
	/* id data */
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, c,&crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, h,&crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, r,&crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, n,&crc);
	/* put crc */
	mfm_put_byte_to_buffer(buffer_ptr, (crc>>8));
	mfm_put_byte_to_buffer(buffer_ptr, (crc & 0x0ff));

	*buffer = buffer_ptr;
}

void mfm_put_data(unsigned char *buffer, unsigned char data_id, unsigned char *data, unsigned long size)
{
	unsigned char *local_data_ptr;
	unsigned short crc;
	unsigned char *buffer_ptr = *buffer;

	/* 12 * 0x00 */
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x00, &crc);

	/* crc includes 3 * 0x0a1 and data mark */
	mfm_init_crc(&crc);

	/* 3 bytes a1 */
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x0a1, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x0a1, &crc);
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, 0x0a1, &crc);
	/* put data mark, either data or deleted data mark */
	mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, data_id, &crc);

	local_data_ptr = data;
	for (i=0; i<size; i++)
	{
		unsigned char data_byte;

		data_byte = local_data_ptr[0];
		local_data_ptr++;

		mfm_put_byte_to_buffer_and_update_crc(buffer_ptr, data_byte, &crc);
	}

	/* put crc */
	mfm_put_byte_to_buffer(buffer_ptr, (crc>>8));
	mfm_put_byte_to_buffer(buffer_ptr, (crc & 0x0ff));

	*buffer = buffer_ptr;
}