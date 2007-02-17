
/* mfm is used to describe a disc with a fm or mfm data structure */
/* disc images are converted to this format at run-time */

/* byte to signal start of id block */
#define MFM_ID_MARK				0x0fe
/* byte to signal start of data block - with data mark */
#define MFM_DATA_MARK			0x0fb
/* byte to signal start of data block - with deleted data mark */
#define MFM_DELETED_DATA_MARK	0x0f8
/* synchronisation byte which appears before id and data mark's */
#define MFM_SYNC_BYTE			0x0a1
/* data byte used to fill gap */
#define MFM_GAP_BYTE			0x04e

typedef enum {
        DEN_FM_LO = 0,
        DEN_FM_HI,
        DEN_MFM_LO,
        DEN_MFM_HI
} DENSITY;


/* the floppy disc controller is simple.
It searches for a id mark which matches the sector specified,
and will then read data from the data block which follows.
In each implementation, if a id cannot be found after x revolutions
of the disc, the command will timeout */

/* get time in cycles for a complete track */
double	mfm_get_track_time(void);
/* time until next id mark */
double  mfm_time_to_next_id_mark(void);
/* time until next data mark */
double  mfm_time_to_next_data_mark(void);
/* time for a single byte at the current density */
double	mfm_time_data_rate(void);

/* helper functions to generate internal mfm image */




/* allocate buffer to hold data for whole disc */
unsigned char *mfm_internal_image_alloc(int num_tracks, int num_sides, DENSITY density);
/* get pointer to track data in internal image */
unsigned char *mfm_internal_image_get_track_ptr(unsigned char *image_data, int track, int side);


/* put a id to the track buffer */
void	mfm_put_id(unsigned char *buffer, unsigned char c, unsigned char h, unsigned char r, unsigned char n);

/* put a block of data to the track buffer */
void mfm_put_data(unsigned char *buffer, unsigned char data_id, unsigned char *data, unsigned long size);
