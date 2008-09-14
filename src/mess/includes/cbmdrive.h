/*****************************************************************************
 *
 * includes/cbmdrive.h
 *
 * commodore cbm floppy drives vc1541 c1551
 * synthetic simulation
 *
 * contains state machines and file system accesses
 *
 ****************************************************************************/

#ifndef CBMDRIVE_H_
#define CBMDRIVE_H_


#define IEC 1
#define SERIAL 2
#define IEEE 3


/*----------- defined in machine/cbmdrive.c -----------*/

/* data for one drive */
typedef struct
{
	int interface;
	unsigned char cmdbuffer[32];
	int cmdpos;
#define OPEN 1
#define READING 2
#define WRITING 3
	int state;						   /*0 nothing */
	unsigned char *buffer;
	int size;
	int pos;
	union
	{
		struct
		{
			int handshakein, handshakeout;
			int datain, dataout;
			int status;
			int state;
		}
		iec;
		struct
		{
			int device;
			int data, clock, atn;
			int state, value;
			int forme;				   /* i am selected */
			int last;				   /* last byte to be sent */
			int broadcast;			   /* sent to all */
			attotime time;
		}
		serial;
		struct
		{
			int device;
			int state;
			UINT8 data;
		} ieee;
	}
	i;
#define D64_IMAGE 1
	int drive;
	unsigned char *image;	   /*d64 image */

	char filename[20];
}
CBM_Drive;

extern CBM_Drive cbm_drive[2];


#define D64_MAX_TRACKS 35
extern const int d64_sectors_per_track[D64_MAX_TRACKS];
int d64_tracksector2offset (int track, int sector);
#define D64_TRACK_ID1   (d64_tracksector2offset(18,0)+162)
#define D64_TRACK_ID2   (d64_tracksector2offset(18,0)+163)

typedef struct
{
	int count;
	CBM_Drive *drives[4];
	/* whole + computer + drives */
	int /*reset, request[6], */ data[6], clock[6], atn[6];
}
CBM_Serial;

extern CBM_Serial cbm_serial;


void cbm_drive_open_helper (void);
void c1551_state (CBM_Drive * drive);
void vc1541_state (CBM_Drive * drive);
void c2031_state (CBM_Drive *drive);

void cbm_drive_0_config (int interface, int serialnr);
void cbm_drive_1_config (int interface, int serialnr);


/* IEC interface for c16 with c1551 */

/* To be passed directly to the drivers */
void c1551_0_write_data (int data);
int c1551_0_read_data (void);
void c1551_0_write_handshake (int data);
int c1551_0_read_handshake (void);
int c1551_0_read_status (void);

void c1551_1_write_data (int data);
int c1551_1_read_data (void);
void c1551_1_write_handshake (int data);
int c1551_1_read_handshake (void);
int c1551_1_read_status (void);


/* Serial bus for vic20, c64 & c16 with vc1541 and some printer */

/* To be passed to serial bus emulation */
void drive_reset_write (CBM_Drive * drive, int level);
int vc1541_atn_read (CBM_Drive * drive);
int vc1541_data_read (CBM_Drive * drive);
int vc1541_clock_read (CBM_Drive * drive);
void vc1541_atn_write (CBM_Drive * drive, int level);
void vc1541_data_write (CBM_Drive * drive, int level);
void vc1541_clock_write (CBM_Drive * drive, int level);


void cbmfloppy_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);


#endif /* CBMDRIVE_H_ */
