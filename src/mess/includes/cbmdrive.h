/* 
 * private area for the
 * commodore cbm floppy drives vc1541 c1551
 * synthetic simulation
 * 
 * contains state machines and file system accesses
 * 
 */
#ifndef __CBMDRIVE_H_
#define __CBMDRIVE_H_

#if 0
#define IEC 1
#define SERIAL 2
#define IEEE 3
#endif

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
			double time;
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

#define D64_MAX_TRACKS 35
extern int d64_sectors_per_track[D64_MAX_TRACKS];
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
void c1551_state (CBM_Drive * c1551);
void vc1541_state (CBM_Drive * vc1541);
void c2031_state(CBM_Drive *drive);

#if 0
void c1551_write_data (CBM_Drive * drive, int data);
int c1551_read_data (CBM_Drive * drive);
void c1551_write_handshake (CBM_Drive * drive, int data);
int c1551_read_handshake (CBM_Drive * drive);
int c1551_read_status (CBM_Drive * drive);

#endif

#endif
