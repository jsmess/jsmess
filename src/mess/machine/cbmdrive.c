/***************************************************************

    CBM Floppy Drive Simulation

    This code incercepts commands from the CBM machine and
    send/return the appropriate data to/from the disk image

    This code will be eventually replaced by proper emulation
    (see vc1541.c)

***************************************************************/

#include <ctype.h>
#include "driver.h"

#include "includes/cbmserb.h"
#include "includes/cbmieeeb.h"

#include "includes/cbmdrive.h"

#define VERBOSE_LEVEL 0
#define DBG_LOG( MACHINE, N, M, A ) \
	do { \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time(MACHINE)), (char*) M ); \
			logerror A; \
		} \
	} while (0)

/***********************************************

    D64 images handling

***********************************************/

/* tracks 1 to 35
 * sectors number from 0
 * each sector holds 256 data bytes
 * directory and Bitmap Allocation Memory in track 18
 * sector 0:
 * 0: track# of directory begin (this linkage of sector often used)
 * 1: sector# of directory begin
 *
 * BAM entries (one per track)
 * offset 0: # of free sectors
 * offset 1: sector 0 (lsb) free to sector 7
 * offset 2: sector 8 to 15
 * offset 3: sector 16 to whatever the number to sectors in track is
 *
 * directory sector:
 * 0,1: track sector of next directory sector
 * 2, 34, 66, ... : 8 directory entries
 *
 * directory entry:
 * 0: file type
 * (0x = scratched/splat, 8x = alive, Cx = locked
 * where x: 0=DEL, 1=SEQ, 2=PRG, 3=USR, 4=REL)
 * 1,2: track and sector of file
 * 3..18: file name padded with a0
 * 19,20: REL files side sector
 * 21: REL files record length
 * 28,29: number of blocks in file
 * ended with illegal track and sector numbers
 */

#define D64_MAX_TRACKS 35

static const int d64_sectors_per_track[] =
{
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
	19, 19, 19, 19, 19, 19, 19,
	18, 18, 18, 18, 18, 18,
	17, 17, 17, 17, 17
};

static int d64_offset[D64_MAX_TRACKS];		   /* offset of begin of track in d64 file */

/* must be called before other functions */
void cbm_drive_open_helper( void )
{
	int i;

	d64_offset[0] = 0;
	for (i = 1; i < D64_MAX_TRACKS; i++)
		d64_offset[i] = d64_offset[i - 1] + d64_sectors_per_track[i - 1] * 256;
}

/* calculates offset to beginning of d64 file for sector beginning */
static int d64_tracksector2offset( int track, int sector )
{
	return d64_offset[track - 1] + sector * 256;
}

static int cbm_compareNames( unsigned char *left, unsigned char *right )
{
	int i;

	for (i = 0; i < 16; i++)
	{
		if ((left[i] == '*') || (right[i] == '*'))
			return 1;
		if (left[i] == right[i])
			continue;
		if ((left[i] == 0xa0) && (right[i] == 0))
			return 1;
		if ((right[i] == 0xa0) && (left[i] == 0))
			return 1;
		return 0;
	}
	return 1;
}

/* searches program with given name in directory
 * delivers -1 if not found
 * or pos in image of directory node */
static int d64_find( CBM_Drive * drive, unsigned char *name )
{
	int pos, track, sector, i;

	pos = d64_tracksector2offset(18, 0);
	track = drive->image[pos];
	sector = drive->image[pos + 1];

	while ((track >= 1) && (track <= 35))
	{
		pos = d64_tracksector2offset(track, sector);
		for (i = 2; i < 256; i += 32)
		{
			if (drive->image[pos + i] & 0x80)
			{
				if (mame_stricmp((char *) name, (char *) "*") == 0)
					return pos + i;
				if (cbm_compareNames(name, drive->image + pos + i + 3))
					return pos + i;
			}
		}
		track = drive->image[pos];
		sector = drive->image[pos + 1];
	}
	return -1;
}

/* reads file into buffer */
static void d64_readprg( running_machine *machine, CBM_Drive * drive, int pos )
{
	int i;

	for (i = 0; i < 16; i++)
		drive->filename[i] = toupper(drive->image[pos + i + 3]);

	drive->filename[i] = 0;

	pos = d64_tracksector2offset(drive->image[pos + 1], drive->image[pos + 2]);

	i = pos;
	drive->size = 0;
	while (drive->image[i] != 0)
	{
		drive->size += 254;
		i = d64_tracksector2offset(drive->image[i], drive->image[i + 1]);
	}
	drive->size += drive->image[i + 1];

	DBG_LOG(machine, 3, "d64 readprg", ("size %d\n", drive->size));

	drive->buffer = (UINT8*)realloc(drive->buffer, drive->size);
	if (!drive->buffer)
		fatalerror("out of memory %s %d\n", __FILE__, __LINE__);

	drive->size--;

	DBG_LOG(machine, 3, "d64 readprg", ("track: %d sector: %d\n",
								drive->image[pos + 1],
								drive->image[pos + 2]));

	for (i = 0; i < drive->size; i += 254)
	{
		if (i + 254 < drive->size)
		{							   /* not last sector */
			memcpy(drive->buffer + i, drive->image + pos + 2, 254);
			pos = d64_tracksector2offset(drive->image[pos + 0],
									  drive->image[pos + 1]);
			DBG_LOG(machine, 3, "d64 readprg", ("track: %d sector: %d\n",
										drive->image[pos],
										drive->image[pos + 1]));
		}
		else
		{
			memcpy(drive->buffer + i, drive->image + pos + 2, drive->size - i);
		}
	}
}

/* reads sector into buffer */
static void d64_read_sector( CBM_Drive * drive, int track, int sector )
{
	int pos;

	snprintf(drive->filename, sizeof (drive->filename),
			  "track %d sector %d", track, sector);

	pos = d64_tracksector2offset (track, sector);

	drive->buffer = (UINT8*)realloc(drive->buffer,256);
	if (!drive->buffer)
		fatalerror("out of memory %s %d\n", __FILE__, __LINE__);

	logerror("d64 read track %d sector %d\n", track, sector);

	memcpy(drive->buffer, drive->image + pos, 256);
	drive->size = 256;
	drive->pos = 0;
}

/* reads directory into buffer */
static void d64_read_directory( CBM_Drive * drive )
{
	int pos, track, sector, i, j, blocksfree, addr = 0x0101/*0x1001*/;

	drive->buffer = (UINT8*)realloc(drive->buffer, 8 * 18 * 25);
	if (!drive->buffer)
		fatalerror("out of memory %s %d\n", __FILE__, __LINE__);

	drive->size = 0;

	pos = d64_tracksector2offset(18, 0);
	track = drive->image[pos];
	sector = drive->image[pos + 1];

	blocksfree = 0;
	for (j = 1, i = 4; j <= 35; j++, i += 4)
	{
		blocksfree += drive->image[pos + i];
	}
	drive->buffer[drive->size++] = addr & 0xff;
	drive->buffer[drive->size++] = addr >> 8;
	addr += 29;
	drive->buffer[drive->size++] = addr & 0xff;
	drive->buffer[drive->size++] = addr >> 8;
	drive->buffer[drive->size++] = 0;
	drive->buffer[drive->size++] = 0;
	drive->buffer[drive->size++] = '\"';
	for (j = 0; j < 16; j++)
		drive->buffer[drive->size++] = drive->image[pos + 0x90 + j];
/*memcpy(drive->buffer+drive->size,drive->image+pos+0x90, 16);drive->size+=16; */
	drive->buffer[drive->size++] = '\"';
	drive->buffer[drive->size++] = ' ';
	drive->buffer[drive->size++] = drive->image[pos + 162];
	drive->buffer[drive->size++] = drive->image[pos + 163];
	drive->buffer[drive->size++] = ' ';
	drive->buffer[drive->size++] = drive->image[pos + 165];
	drive->buffer[drive->size++] = drive->image[pos + 166];
	drive->buffer[drive->size++] = 0;

	while ((track >= 1) && (track <= 35))
	{
		pos = d64_tracksector2offset(track, sector);
		for (i = 2; i < 256; i += 32)
		{
			if (drive->image[pos + i] & 0x80)
			{
				int len, blocks = drive->image[pos + i + 28]
				+ 256 * drive->image[pos + i + 29];
				char dummy[10];

				sprintf(dummy, "%d", blocks);
				len = strlen (dummy);
				addr += 29 - len;
				drive->buffer[drive->size++] = addr & 0xff;
				drive->buffer[drive->size++] = addr >> 8;
				drive->buffer[drive->size++] = drive->image[pos + i + 28];
				drive->buffer[drive->size++] = drive->image[pos + i + 29];
				for (j = 4; j > len; j--)
					drive->buffer[drive->size++] = ' ';
				drive->buffer[drive->size++] = '\"';
				for (j = 0; j < 16; j++)
					drive->buffer[drive->size++] = drive->image[pos + i + 3 + j];
				drive->buffer[drive->size++] = '\"';
				drive->buffer[drive->size++] = ' ';
				switch (drive->image[pos + i] & 0x3f)
				{
				case 0:
					drive->buffer[drive->size++] = 'D';
					drive->buffer[drive->size++] = 'E';
					drive->buffer[drive->size++] = 'L';
					break;
				case 1:
					drive->buffer[drive->size++] = 'S';
					drive->buffer[drive->size++] = 'E';
					drive->buffer[drive->size++] = 'Q';
					break;
				case 2:
					drive->buffer[drive->size++] = 'P';
					drive->buffer[drive->size++] = 'R';
					drive->buffer[drive->size++] = 'G';
					break;
				case 3:
					drive->buffer[drive->size++] = 'U';
					drive->buffer[drive->size++] = 'S';
					drive->buffer[drive->size++] = 'R';
					break;
				case 4:
					drive->buffer[drive->size++] = 'R';
					drive->buffer[drive->size++] = 'E';
					drive->buffer[drive->size++] = 'L';
					break;
				}
				drive->buffer[drive->size++] = 0;
			}
		}
		track = drive->image[pos];
		sector = drive->image[pos + 1];
	}
	addr += 14;
	drive->buffer[drive->size++] = addr & 0xff;
	drive->buffer[drive->size++] = addr >> 8;
	drive->buffer[drive->size++] = blocksfree & 0xff;
	drive->buffer[drive->size++] = blocksfree >> 8;
	memcpy(drive->buffer + drive->size, "BLOCKS FREE", 11);
	drive->size += 11;
	drive->buffer[drive->size++] = 0;

	strcpy(drive->filename, "$");
}

static int d64_command( running_machine *machine, CBM_Drive * drive, unsigned char *name )
{
	int pos;

	/* name eventuell mit 0xa0 auffuellen */

	if (mame_stricmp((char *) name, (char *) "$") == 0)
	{
		d64_read_directory(drive);
	}
	else
	{
		if ((pos = d64_find (drive, name)) == -1)
		{
			return 1;
		}
		d64_readprg(machine, drive, pos);
	}
	return 0;
}

/**
  7.1 Serial bus

   CBM Serial Bus Control Codes

    20  Talk
    3F  Untalk
    40  Listen
    5F  Unlisten
    60  Open Channel
    70  -
    80  -
    90  -
    A0  -
    B0  -
    C0  -
    D0  -
    E0  Close
    F0  Open



     How the C1541 is called by the C64:

        read (drive 8)
        /28 /f0 filename /3f
        /48 /60 read data /5f
        /28 /e0 /3f

        write (drive 8)
        /28 /f0 filename /3f
        /28 /60 send data /3f
        /28 /e0 /3f

     I used '/' to denote bytes sent under Attention (ATN low).

    28 == LISTEN command + device number 8
    f0 == secondary addres for OPEN file on channel 0

  Note that there's no acknowledge bit, but timeout/EOI handshake for each
  byte. Check the C64 Kernel for exact description...

 computer master

 c16 called
 dload
  20 f0 30 3a name 3f
  40 60 listening 5f
  20 e0 3f

 load
  20 f0 name 3f
 */
static void cbm_command( running_machine *machine, CBM_Drive * drive )
{
	unsigned char name[20], type = 'P', mode = 0;
	int channel, head, track, sector;
	int j, i, rc;

	if ((drive->cmdpos == 4)
		&& ((drive->cmdbuffer[0] & 0xf0) == 0x20)
		&& ((drive->cmdbuffer[1] & 0xf0) == 0xf0)
		&& (drive->cmdbuffer[2] == '#')
		&& (drive->cmdbuffer[3] == 0x3f))
	{
		logerror("floppy direct access channel %d opened\n",
					 (unsigned) drive->cmdbuffer[1] & 0xf);
	}
	else if ((drive->cmdpos >= 4)
			 && (((unsigned) drive->cmdbuffer[0] & 0xf0) == 0x20)
			 && (((unsigned) drive->cmdbuffer[1] & 0xf0) == 0xf0)
			 && (drive->cmdbuffer[drive->cmdpos - 1] == 0x3f))
	{
		if (drive->cmdbuffer[3] == ':')
			j = 4;
		else
			j = 2;

		for (i = 0; (j < sizeof (name)) && (drive->cmdbuffer[j] != 0x3f)
			 && (drive->cmdbuffer[j] != ',');
			 i++, j++)
			name[i] = drive->cmdbuffer[j];
		name[i] = 0;

		if (drive->cmdbuffer[j] == ',')
		{
			j++;
			if (j < drive->cmdpos)
			{
				type = drive->cmdbuffer[j];
				j++;
				if ((j < drive->cmdpos) && (drive->cmdbuffer[j] == 'j'))
				{
					j++;
					if (j < drive->cmdpos)
						mode = drive->cmdbuffer[j];
				}
			}
		}
		rc = 1;
		if (drive->drive == D64_IMAGE)
		{
			if ((type == 'P') || (type == 'S'))
				rc = d64_command (machine, drive, name);
		}
		if (!rc)
		{
			drive->state = OPEN;
			drive->pos = 0;
		}
		DBG_LOG(machine, 1, "cbm_open", ("%s %s type:%c %c\n", name,
								 rc ? "failed" : "success", type, mode ? mode : ' '));
	}
	else if ((drive->cmdpos == 1) && (drive->cmdbuffer[0] == 0x5f))
	{
		drive->state = OPEN;
	}
	else if ((drive->cmdpos == 3) && ((drive->cmdbuffer[0] & 0xf0) == 0x20)
			 && ((drive->cmdbuffer[1] & 0xf0) == 0xe0)
			 && (drive->cmdbuffer[2] == 0x3f))
	{
/*    if (drive->buffer) free(drive->buffer);drive->buffer=0; */
		drive->state = 0;
	}
	else if ((drive->cmdpos == 2) && ((drive->cmdbuffer[0] & 0xf0) == 0x40)
			 && ((drive->cmdbuffer[1] & 0xf0) == 0x60))
	{
		if (drive->state == OPEN)
		{
			drive->state = READING;
		}
	}
	else if ((drive->cmdpos == 2) && ((drive->cmdbuffer[0] & 0xf0) == 0x20)
			 && ((drive->cmdbuffer[1] & 0xf0) == 0x60))
	{
		drive->state = WRITING;
	}
	else if ((drive->cmdpos == 1) && (drive->cmdbuffer[0] == 0x3f))
	{
		drive->state = OPEN;
	}
	else if ((drive->drive == D64_IMAGE)
			 && (4 == sscanf ((char *) drive->cmdbuffer, "U1: %d %d %d %d\x0d",
							  &channel, &head, &track, &sector)))
	{
		d64_read_sector (drive, track, sector);
		drive->state = OPEN;
	}
	else
	{

		logerror("unknown floppycommand(size:%d):", drive->cmdpos);
		for (i = 0; i < drive->cmdpos; i++)
			logerror("%.2x", drive->cmdbuffer[i]);
		logerror(" ");
		for (i = 0; i < drive->cmdpos; i++)
			logerror("%c", drive->cmdbuffer[i]);
		logerror("\n");

		drive->state = 0;
	}
	drive->cmdpos = 0;
}


/***********************************************

    Drive state update

***********************************************/


 /*
  * 0x55 begin of command
  *
  * frame
  * selector
  * 0x81 device id
  * 0x82 command
  * 0x83 data
  * 0x84 read byte
  * handshake low
  *
  * byte (like in serial bus!)
  *
  * floppy drive delivers
  * status 3 for file not found
  * or filedata ended with status 3
  */
void c1551_state( running_machine *machine, CBM_Drive * drive )
{
	static int oldstate;

	oldstate = drive->i.iec.state;

	switch (drive->i.iec.state)
	{
	case -1:						   /* currently neccessary for correct init */
		if (drive->i.iec.handshakein)
		{
			drive->i.iec.state++;
		}
		break;
	case 0:
		if (drive->i.iec.datain == 0x55)
		{
			drive->i.iec.status = 0;
			drive->i.iec.state++;
		}
		break;
	case 1:
		if (drive->i.iec.datain != 0x55)
		{
			drive->i.iec.state = 10;
		}
		break;
	case 10:
		if (drive->i.iec.datain != 0)
		{
			drive->i.iec.handshakeout = 0;
			if (drive->i.iec.datain == 0x84)
			{
				drive->i.iec.state = 20;
				if (drive->pos + 1 == drive->size)
					drive->i.iec.status = 3;
			}
			else if (drive->i.iec.datain == 0x83)
			{
				drive->i.iec.state = 40;
			}
			else
			{
				drive->i.iec.status = 0;
				drive->i.iec.state++;
			}
		}
		break;
	case 11:
		if (!drive->i.iec.handshakein)
		{
			drive->i.iec.state++;
			DBG_LOG(machine, 1,"c1551",("taken data %.2x\n",drive->i.iec.datain));
			if (drive->cmdpos < sizeof (drive->cmdbuffer))
				drive->cmdbuffer[drive->cmdpos++] = drive->i.iec.datain;
			if ((drive->i.iec.datain == 0x3f) || (drive->i.iec.datain == 0x5f))
			{
				cbm_command (machine, drive);
				drive->i.iec.state = 30;
			}
			else if (((drive->i.iec.datain & 0xf0) == 0x60))
			{
				cbm_command (machine, drive);
				if (drive->state == READING)
				{
				}
				else if (drive->state == WRITING)
				{
				}
				else
					drive->i.iec.status = 3;
			}
			drive->i.iec.handshakeout = 1;
		}
		break;
	case 12:
		if (drive->i.iec.datain == 0)
		{
			drive->i.iec.state++;
		}
		break;
	case 13:
		if (drive->i.iec.handshakein)
		{
			drive->i.iec.state = 10;
		}
		break;

	case 20:						   /* reading data */
		if (!drive->i.iec.handshakein)
		{
			drive->i.iec.handshakeout = 1;
			if (drive->state == READING)
				drive->i.iec.dataout = drive->buffer[drive->pos++];
			drive->i.iec.state++;
		}
		break;
	case 21:						   /* reading data */
		if (drive->i.iec.handshakein)
		{
			drive->i.iec.handshakeout = 0;
			drive->i.iec.state++;
		}
		break;
	case 22:
		if (drive->i.iec.datain == 0)
		{
			drive->i.iec.state++;
		}
		break;
	case 23:
		if (!drive->i.iec.handshakein)
		{
			drive->i.iec.handshakeout = 1;
			if (drive->state == READING)
				drive->i.iec.state = 10;
			else
				drive->i.iec.state = 0;
		}
		break;

	case 30:						   /* end of command */
		if (drive->i.iec.datain == 0)
		{
			drive->i.iec.state++;
		}
		break;
	case 31:
		if (drive->i.iec.handshakein)
		{
			drive->i.iec.state = 0;
		}
		break;

	case 40:						   /* simple write */
		if (!drive->i.iec.handshakein)
		{
			drive->i.iec.state++;
			if ((drive->state == 0) || (drive->state == OPEN))
			{
				DBG_LOG(machine, 1, "c1551", ("taken data %.2x\n",
									  drive->i.iec.datain));
				if (drive->cmdpos < sizeof (drive->cmdbuffer))
					drive->cmdbuffer[drive->cmdpos++] = drive->i.iec.datain;
			}
			else if (drive->state == WRITING)
			{
				DBG_LOG(machine, 1, "c1551", ("written data %.2x\n", drive->i.iec.datain));
			}
			drive->i.iec.handshakeout = 1;
		}
		break;
	case 41:
		if (drive->i.iec.datain == 0)
		{
			drive->i.iec.state++;
		}
		break;
	case 42:
		if (drive->i.iec.handshakein)
		{
			drive->i.iec.state = 10;
		}
		break;
	}

	if (oldstate != drive->i.iec.state)
		if (VERBOSE_LEVEL >= 1) logerror("state %d->%d %d\n", oldstate, drive->i.iec.state, drive->state);
}

static int vc1541_time_greater( running_machine *machine, CBM_Drive * drive, attotime threshold )
{
	return attotime_compare(
		attotime_sub(timer_get_time(machine), drive->i.serial.time),
		threshold) > 0;
}

void vc1541_state( running_machine *machine, CBM_Drive * drive )
{
	int oldstate = drive->i.serial.state;

	switch (drive->i.serial.state)
	{
	case 0:
		if (!cbm_serial.atn[0] && !cbm_serial.clock[0])
		{
			drive->i.serial.data = 0;
			drive->i.serial.state = 2;
			break;
		}
		if (!cbm_serial.clock[0] && drive->i.serial.forme)
		{
			drive->i.serial.data = 0;
			drive->i.serial.state++;
			break;
		}
		break;
	case 1:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state++;
			break;
		}
		if (cbm_serial.clock[0])
		{
			drive->i.serial.broadcast = 0;
			drive->i.serial.data = 1;
			drive->i.serial.state = 100;
			drive->i.serial.last = 0;
			drive->i.serial.value = 0;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
	case 2:
		if (cbm_serial.clock[0])
		{
			drive->i.serial.broadcast = 1;
			drive->i.serial.data = 1;
			drive->i.serial.state = 100;
			drive->i.serial.last = 0;
			drive->i.serial.value = 0;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
		/* bits to byte fitting */
	case 100:
		if (!cbm_serial.clock[0])
		{
			drive->i.serial.state++;
			break;
		}
		break;
	case 102:
	case 104:
	case 106:
	case 108:
	case 110:
	case 112:
	case 114:
		if (!cbm_serial.clock[0])
			drive->i.serial.state++;
		break;
	case 101:
		if (cbm_serial.clock[0])
		{
			drive->i.serial.value |= cbm_serial.data[0] ? 1 : 0;
			drive->i.serial.state++;
		}
		break;
	case 103:
		if (cbm_serial.clock[0])
		{
			drive->i.serial.value |= cbm_serial.data[0] ? 2 : 0;
			drive->i.serial.state++;
		}
		break;
	case 105:
		if (cbm_serial.clock[0])
		{
			drive->i.serial.value |= cbm_serial.data[0] ? 4 : 0;
			drive->i.serial.state++;
		}
		break;
	case 107:
		if (cbm_serial.clock[0])
		{
			drive->i.serial.value |= cbm_serial.data[0] ? 8 : 0;
			drive->i.serial.state++;
		}
		break;
	case 109:
		if (cbm_serial.clock[0])
		{
			drive->i.serial.value |= cbm_serial.data[0] ? 0x10 : 0;
			drive->i.serial.state++;
		}
		break;
	case 111:
		if (cbm_serial.clock[0])
		{
			drive->i.serial.value |= cbm_serial.data[0] ? 0x20 : 0;
			drive->i.serial.state++;
		}
		break;
	case 113:
		if (cbm_serial.clock[0])
		{
			drive->i.serial.value |= cbm_serial.data[0] ? 0x40 : 0;
			drive->i.serial.state++;
		}
		break;
	case 115:
		if (cbm_serial.clock[0])
		{
			drive->i.serial.value |= cbm_serial.data[0] ? 0x80 : 0;
			if (drive->i.serial.broadcast
				&& (((drive->i.serial.value & 0xf0) == 0x20)
					|| ((drive->i.serial.value & 0xf0) == 0x40)))
			{
				drive->i.serial.forme = (drive->i.serial.value & 0xf)
					== drive->i.serial.device;
				if (!drive->i.serial.forme)
				{
					drive->i.serial.state = 160;
					break;
				}
			}
			if (drive->i.serial.forme)
			{
				if (drive->cmdpos < sizeof (drive->cmdbuffer))
					drive->cmdbuffer[drive->cmdpos++] = drive->i.serial.value;
				DBG_LOG(machine, 1, "serial read", ("%s %s %.2x\n",
							drive->i.serial.broadcast ? "broad" : "",
							drive->i.serial.last ? "last" : "",
											drive->i.serial.value));
			}
			drive->i.serial.state++;
		}
		break;
	case 116:
		if (!cbm_serial.clock[0])
		{
			if (drive->i.serial.last)
				drive->i.serial.state = 130;
			else
				drive->i.serial.state++;
			if (drive->i.serial.broadcast &&
				((drive->i.serial.value == 0x3f) || (drive->i.serial.value == 0x5f)
				 || ((drive->i.serial.value & 0xf0) == 0x60)))
			{
				cbm_command(machine, drive);
			}
			drive->i.serial.time = timer_get_time(machine);
			drive->i.serial.data = 0;
			break;
		}
		break;
	case 117:
		if (drive->i.serial.forme && ((drive->i.serial.value & 0xf0) == 0x60)
			&& drive->i.serial.broadcast && cbm_serial.atn[0])
		{
			if (drive->state == READING)
			{
				drive->i.serial.state = 200;
				break;
			}
			else if (drive->state != WRITING)
			{
				drive->i.serial.state = 150;
				break;
			}
		}
		if (((drive->i.serial.value == 0x3f)
			 || (drive->i.serial.value == 0x5f))
			&& drive->i.serial.broadcast && cbm_serial.atn[0])
		{
			drive->i.serial.data = 1;
			drive->i.serial.state = 140;
			break;
		}
		if (cbm_serial.clock[0])
		{
			drive->i.serial.time = timer_get_time(machine);
			drive->i.serial.broadcast = !cbm_serial.atn[0];
			drive->i.serial.data = 1;
			drive->i.serial.value = 0;
			drive->i.serial.state++;
			break;
		}
		break;
		/* if computer lowers clk not in 200micros (last byte following)
         * negativ pulse on data by listener */
	case 118:
		if (!cbm_serial.clock[0])
		{
			drive->i.serial.value = 0;
			drive->i.serial.state = 101;
			drive->i.serial.data = 1;
			break;
		}
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(200)))
		{
			drive->i.serial.data = 0;
			drive->i.serial.last = 1;
			drive->i.serial.state++;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
	case 119:
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(60)))
		{
			drive->i.serial.value = 0;
			drive->i.serial.data = 1;
			drive->i.serial.state = 100;
			break;
		}
		break;

	case 130:						   /* last byte of talk */
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(60)))
		{
			drive->i.serial.data = 1;
			drive->i.serial.state = 0;
			break;
		}
		break;

	case 131:
		if (cbm_serial.clock[0])
		{
			drive->i.serial.state = 0;
		}
		break;

	case 140:						   /* end of talk */
		if (cbm_serial.clock[0])
		{
			drive->i.serial.state = 0;
		}
		break;

	case 150:						   /* file not found */
		if (cbm_serial.atn[0])
		{
			drive->i.serial.state++;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
	case 151:
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(1000)))
		{
			drive->i.serial.state++;
			drive->i.serial.clock = 0;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
	case 152:
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(50)))
		{
			drive->i.serial.state++;
			drive->i.serial.clock = 1;
			break;
		}
		break;
	case 153:
		if (cbm_serial.clock[0])
		{
			drive->i.serial.state = 0;
		}
		break;

	case 160:						   /* not for me */
		if (cbm_serial.atn[0])
		{
			drive->i.serial.state = 0;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;

	case 200:
		if (cbm_serial.clock[0])
		{
			drive->i.serial.state++;
			drive->i.serial.clock = 0;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
	case 201:
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(80)))
		{
			drive->i.serial.clock = 1;
			drive->i.serial.data = 1;
			drive->i.serial.state = 300;
			break;
		}
		break;

	case 300:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state = 330;
			drive->i.serial.data = 1;
			drive->i.serial.clock = 1;
			break;
		}
		if (cbm_serial.data[0])
		{
			drive->i.serial.value = drive->buffer[drive->pos];
			drive->i.serial.clock = 0;
			drive->i.serial.data = (drive->i.serial.value & 1) ? 1 : 0;
			drive->i.serial.state++;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
	case 301:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state = 330;
			drive->i.serial.data = 1;
			drive->i.serial.clock = 1;
			break;
		}
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(40)))
		{
			drive->i.serial.clock = 1;
			drive->i.serial.state++;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
	case 303:
	case 305:
	case 307:
	case 309:
	case 311:
	case 313:
	case 315:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state = 330;
			drive->i.serial.data = 1;
			drive->i.serial.clock = 1;
			break;
		}
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(20)))
		{
			drive->i.serial.clock = 1;
			drive->i.serial.state++;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
	case 302:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state = 330;
			drive->i.serial.data = 1;
			drive->i.serial.clock = 1;
			break;
		}
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(20)))
		{
			drive->i.serial.data = drive->i.serial.value & 2 ? 1 : 0;
			drive->i.serial.clock = 0;
			drive->i.serial.state++;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
	case 304:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state = 330;
			drive->i.serial.data = 1;
			drive->i.serial.clock = 1;
			break;
		}
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(20)))
		{
			drive->i.serial.data = drive->i.serial.value & 4 ? 1 : 0;
			drive->i.serial.clock = 0;
			drive->i.serial.state++;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
	case 306:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state = 330;
			drive->i.serial.data = 1;
			drive->i.serial.clock = 1;
			break;
		}
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(20)))
		{
			drive->i.serial.data = drive->i.serial.value & 8 ? 1 : 0;
			drive->i.serial.clock = 0;
			drive->i.serial.state++;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
	case 308:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state = 330;
			drive->i.serial.data = 1;
			drive->i.serial.clock = 1;
			break;
		}
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(20)))
		{
			drive->i.serial.data = drive->i.serial.value & 0x10 ? 1 : 0;
			drive->i.serial.clock = 0;
			drive->i.serial.state++;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
	case 310:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state = 330;
			drive->i.serial.data = 1;
			drive->i.serial.clock = 1;
			break;
		}
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(20)))
		{
			drive->i.serial.data = drive->i.serial.value & 0x20 ? 1 : 0;
			drive->i.serial.clock = 0;
			drive->i.serial.state++;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
	case 312:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state = 330;
			drive->i.serial.data = 1;
			drive->i.serial.clock = 1;
			break;
		}
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(20)))
		{
			drive->i.serial.data = drive->i.serial.value & 0x40 ? 1 : 0;
			drive->i.serial.clock = 0;
			drive->i.serial.state++;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
	case 314:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state = 330;
			drive->i.serial.data = 1;
			drive->i.serial.clock = 1;
			break;
		}
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(20)))
		{
			drive->i.serial.data = drive->i.serial.value & 0x80 ? 1 : 0;
			drive->i.serial.clock = 0;
			drive->i.serial.state++;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
	case 316:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state = 330;
			drive->i.serial.data = 1;
			drive->i.serial.clock = 1;
			break;
		}
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(20)))
		{
			DBG_LOG(machine, 1, "vc1541", ("%.2x written\n", drive->i.serial.value));
			drive->i.serial.data = 1;
			drive->i.serial.clock = 0;
			drive->i.serial.state++;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
	case 317:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state = 330;
			drive->i.serial.data = 1;
			drive->i.serial.clock = 1;
			break;
		}
		if (!cbm_serial.data[0])
		{
			drive->i.serial.state++;
			drive->i.serial.time = timer_get_time(machine);
			break;
		}
		break;
	case 318:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state = 330;
			drive->i.serial.data = 1;
			drive->i.serial.clock = 1;
			break;
		}
		if (drive->pos + 1 == drive->size)
		{
			drive->i.serial.clock = 1;
			drive->i.serial.state = 0;
			break;
		}
		if (drive->pos + 2 == drive->size)
		{
			drive->pos++;
			drive->i.serial.state = 320;
			break;
		}
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(100)))
		{
			drive->pos++;
			drive->i.serial.clock = 1;
			drive->i.serial.state = 300;
			break;
		}
		break;
	case 320:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state = 330;
			drive->i.serial.data = 1;
			drive->i.serial.clock = 1;
			break;
		}
		if (vc1541_time_greater(machine, drive, ATTOTIME_IN_USEC(100)))
		{
			drive->i.serial.clock = 1;
			drive->i.serial.state++;
			break;
		}
		break;
	case 321:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state = 330;
			drive->i.serial.data = 1;
			drive->i.serial.clock = 1;
			break;
		}
		if (cbm_serial.data[0])
		{
			drive->i.serial.state++;
		}
		break;
	case 322:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state = 330;
			drive->i.serial.data = 1;
			drive->i.serial.clock = 1;
			break;
		}
		if (!cbm_serial.data[0])
		{
			drive->i.serial.state++;
		}
		break;
	case 323:
		if (!cbm_serial.atn[0])
		{
			drive->i.serial.state = 330;
			drive->i.serial.data = 1;
			drive->i.serial.clock = 1;
			break;
		}
		if (cbm_serial.data[0])
		{
			drive->i.serial.state = 300;
		}
		break;
	case 330:						   /* computer breaks receiving */
		drive->i.serial.state = 0;
		break;
	}

	if (oldstate != drive->i.serial.state)
		if (VERBOSE_LEVEL >= 1) logerror("%d state %d->%d %d %s %s %s\n",
				 drive->i.serial.device,
				 oldstate,
				 drive->i.serial.state, drive->state,
				 cbm_serial.atn[0] ? "ATN" : "atn",
				 cbm_serial.clock[0] ? "CLOCK" : "clock",
				 cbm_serial.data[0] ? "DATA" : "data");
}

/* difference between vic20 and pet (first series)
   pet lowers atn and wants a reaction on ndac */

void c2031_state( running_machine *machine, CBM_Drive *drive )
{
	const device_config *ieeebus = devtag_get_device(machine, "ieee_bus");
	int oldstate = drive->i.ieee.state;
	int data;

	switch (drive->i.ieee.state)
	{
	case 0:
		if (!cbm_ieee_dav_r(ieeebus, 0))
		{
			drive->i.ieee.state = 10;
		}
		else if (!cbm_ieee_atn_r(ieeebus, 0))
		{
			drive->i.ieee.state = 11;
			cbm_ieee_ndac_w(ieeebus, 1, 0);
			logerror("arsch\n");
		}
		break;
	case 1:
		break;
	case 10:
		if (cbm_ieee_dav_r(ieeebus, 0))
		{
			drive->i.ieee.state++;
			cbm_ieee_nrfd_w(ieeebus, 1, 1);
			cbm_ieee_ndac_w(ieeebus, 1, 0);
		}
		break;
	case 11:
		if (!cbm_ieee_dav_r(ieeebus, 0))
		{
			cbm_ieee_nrfd_w(ieeebus, 1, 0);
			data = cbm_ieee_data_r(ieeebus, 0) ^ 0xff;
			cbm_ieee_ndac_w(ieeebus, 1, 1);
			logerror("byte received %.2x\n",data);
			if (!cbm_ieee_atn_r(ieeebus, 0) && ((data & 0x0f) == drive->i.ieee.device))
			{
				if ((data & 0xf0) == 0x40)
					drive->i.ieee.state = 30;
				else
					drive->i.ieee.state = 20;
				if (drive->cmdpos < sizeof (drive->cmdbuffer))
					drive->cmdbuffer[drive->cmdpos++] = data;
			}
			else if ((data&0xf) == 0xf)
			{
				drive->i.ieee.state--;
			}
			else
			{
				drive->i.ieee.state++;
			}
		}
		break;
		/* wait until atn is released */
	case 12:
		if (cbm_ieee_atn_r(ieeebus, 0))
		{
			drive->i.ieee.state++;
			cbm_ieee_nrfd_w(ieeebus, 1, 0);
		}
		break;
	case 13:
		if (!cbm_ieee_atn_r(ieeebus, 0))
		{
			drive->i.ieee.state = 10;
/*          cbm_ieee_nrfd_w(ieeebus, 1, 0); */
		}
		break;

		/* receiving rest of command */
	case 20:
		if (cbm_ieee_dav_r(ieeebus, 0))
		{
			drive->i.ieee.state++;
			cbm_ieee_nrfd_w(ieeebus, 1, 1);
			cbm_ieee_ndac_w(ieeebus, 1, 0);
		}
		break;
	case 21:
		if (!cbm_ieee_dav_r(ieeebus, 0))
		{
			cbm_ieee_nrfd_w(ieeebus, 1, 0);
			data = cbm_ieee_data_r(ieeebus, 0) ^ 0xff;
			logerror("byte received %.2x\n",data);
			if (drive->cmdpos < sizeof (drive->cmdbuffer))
				drive->cmdbuffer[drive->cmdpos++] = data;
			if (!cbm_ieee_atn_r(ieeebus, 0) && ((data & 0xf) == 0xf))
			{
				cbm_command(machine, drive);
				drive->i.ieee.state = 10;
			}
			else
				drive->i.ieee.state = 20;

			cbm_ieee_ndac_w(ieeebus, 1, 1);
		}
		break;

		/* read command */
	case 30:
		if (cbm_ieee_dav_r(ieeebus, 0))
		{
			drive->i.ieee.state++;
			cbm_ieee_nrfd_w(ieeebus, 1, 1);
			cbm_ieee_ndac_w(ieeebus, 1, 0);
		}
		break;
	case 31:
		if (!cbm_ieee_dav_r(ieeebus, 0))
		{
			cbm_ieee_nrfd_w(ieeebus, 1, 0);
			data = cbm_ieee_data_r(ieeebus, 0) ^ 0xff;
			logerror("byte received %.2x\n", data);
			if (drive->cmdpos < sizeof (drive->cmdbuffer))
				drive->cmdbuffer[drive->cmdpos++] = data;
			cbm_command(machine, drive);
			if (drive->state == READING)
				drive->i.ieee.state++;
			else
				drive->i.ieee.state = 10;
			cbm_ieee_ndac_w(ieeebus, 1, 1);
		}
		break;
	case 32:
		if (cbm_ieee_dav_r(ieeebus, 0))
		{
			cbm_ieee_nrfd_w(ieeebus, 1, 1);
			drive->i.ieee.state = 40;
		}
		break;
	case 40:
		if (!cbm_ieee_ndac_r(ieeebus, 0))
		{
			cbm_ieee_data_w(ieeebus, 1, drive->buffer[drive->pos++] ^ 0xff);
			if (drive->pos >= drive->size)
				cbm_ieee_eoi_w(ieeebus, 1, 0);
			cbm_ieee_dav_w(ieeebus, 1, 0);
			drive->i.ieee.state++;
		}
		break;
	case 41:
		if (!cbm_ieee_nrfd_r(ieeebus, 0))
		{
			drive->i.ieee.state++;
		}
		break;
	case 42:
		if (cbm_ieee_ndac_r(ieeebus, 0))
		{
			if (cbm_ieee_eoi_r(ieeebus, 0))
				drive->i.ieee.state = 40;

			else
			{
				cbm_ieee_data_w(ieeebus, 1, 0xff);
				cbm_ieee_ndac_w(ieeebus, 1, 0);
				cbm_ieee_nrfd_w(ieeebus, 1, 0);
				cbm_ieee_eoi_w(ieeebus, 1, 1);
				drive->i.ieee.state = 10;
			}
			cbm_ieee_dav_w(ieeebus, 1, 1);
		}
		break;
	}

	if (oldstate != drive->i.ieee.state)
		if (VERBOSE_LEVEL >= 1) logerror("%d state %d->%d %d\n",
				 drive->i.ieee.device,
				 oldstate,
				 drive->i.ieee.state, drive->state
				 );
}


/**************************************

    C1551 handlers

**************************************/


static void c1551_write_data( running_machine *machine, CBM_Drive * drive, int data )
{
	drive->i.iec.datain = data;
	c1551_state (machine, drive);
}

static int c1551_read_data( running_machine *machine, CBM_Drive * drive )
{
	c1551_state (machine, drive);
	return drive->i.iec.dataout;
}

static void c1551_write_handshake( running_machine *machine, CBM_Drive * drive, int data )
{
	drive->i.iec.handshakein = data&0x40?1:0;
	c1551_state (machine, drive);
}

static int c1551_read_handshake( running_machine *machine, CBM_Drive * drive )
{
	c1551_state (machine, drive);
	return drive->i.iec.handshakeout?0x80:0;
}

static int c1551_read_status( running_machine *machine, CBM_Drive * drive )
{
	c1551_state (machine, drive);
	return drive->i.iec.status;
}


WRITE8_DEVICE_HANDLER( c1551_0_write_data )
{
	c1551_write_data(device->machine, cbm_drive, data);
}

READ8_DEVICE_HANDLER( c1551_0_read_data )
{
	return c1551_read_data(device->machine, cbm_drive);
}

WRITE8_DEVICE_HANDLER( c1551_0_write_handshake )
{
	c1551_write_handshake(device->machine, cbm_drive, data);
}

READ8_DEVICE_HANDLER( c1551_0_read_handshake )
{
	return c1551_read_handshake(device->machine, cbm_drive);
}

READ8_DEVICE_HANDLER( c1551_0_read_status )
{
	return c1551_read_status(device->machine, cbm_drive);
}

WRITE8_DEVICE_HANDLER( c1551_1_write_data )
{
	c1551_write_data(device->machine, cbm_drive + 1, data);
}

READ8_DEVICE_HANDLER( c1551_1_read_data )
{
	return c1551_read_data(device->machine, cbm_drive + 1);
}

WRITE8_DEVICE_HANDLER( c1551_1_write_handshake )
{
	c1551_write_handshake(device->machine, cbm_drive + 1, data);
}

READ8_DEVICE_HANDLER( c1551_1_read_handshake )
{
	return c1551_read_handshake(device->machine, cbm_drive + 1);
}

READ8_DEVICE_HANDLER( c1551_1_read_status )
{
	return c1551_read_status(device->machine, cbm_drive + 1);
}


/**************************************

    VC1541 Serial Bus Device Interface

**************************************/

static UINT8 vc1541_atn_read( running_machine *machine, CBM_Drive * drive )
{
	vc1541_state(machine, drive);
	return drive->i.serial.atn;
}

static UINT8 vc1541_data_read( running_machine *machine, CBM_Drive * drive )
{
	vc1541_state(machine, drive);
	return drive->i.serial.data;
}

static UINT8 vc1541_clock_read( running_machine *machine, CBM_Drive * drive )
{
	vc1541_state(machine, drive);
	return drive->i.serial.clock;
}

static void vc1541_data_write( running_machine *machine, CBM_Drive * drive, int level )
{
	vc1541_state(machine, drive);
}

static void vc1541_clock_write( running_machine *machine, CBM_Drive * drive, int level )
{
	vc1541_state(machine, drive);
}

static void vc1541_atn_write( running_machine *machine, CBM_Drive * drive, int level )
{
	vc1541_state(machine, drive);
}

static void drive_reset_write( CBM_Drive * drive, int level )
{
	if (level == 0)
	{
		drive->i.serial.data =
			drive->i.serial.clock =
			drive->i.serial.atn = 1;
		drive->i.serial.state = 0;
	}
}

static READ8_DEVICE_HANDLER( sim_drive_atn_read )
{
	int i;

	cbm_serial.atn[0] = cbm_serial.atn[1];
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.atn[0] &= cbm_serial.atn[i + 2] =
			vc1541_atn_read(device->machine, cbm_serial.drives[i]);
	return cbm_serial.atn[0];
}

static READ8_DEVICE_HANDLER( sim_drive_clock_read )
{
	int i;

	cbm_serial.clock[0] = cbm_serial.clock[1];
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.clock[0] &= cbm_serial.clock[i + 2] =
			vc1541_clock_read(device->machine, cbm_serial.drives[i]);
	return cbm_serial.clock[0];
}

static READ8_DEVICE_HANDLER( sim_drive_data_read )
{
	int i;

	cbm_serial.data[0] = cbm_serial.data[1];
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.data[0] &= cbm_serial.data[i + 2] =
			vc1541_data_read(device->machine, cbm_serial.drives[i]);
	return cbm_serial.data[0];
}

static READ8_DEVICE_HANDLER( sim_drive_request_read )
{
	/* in c16 not connected */
	return 1;
}

static WRITE8_DEVICE_HANDLER( sim_drive_atn_write )
{
	int i;

	cbm_serial.atn[0] =
		cbm_serial.atn[1] = data;
	/* update line */
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.atn[0] &= cbm_serial.atn[i + 2];
	/* inform drives */
	for (i = 0; i < cbm_serial.count; i++)
		vc1541_atn_write(device->machine, cbm_serial.drives[i], cbm_serial.atn[0]);
}

static WRITE8_DEVICE_HANDLER( sim_drive_clock_write )
{
	int i;

	cbm_serial.clock[0] =
		cbm_serial.clock[1] = data;
	/* update line */
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.clock[0] &= cbm_serial.clock[i + 2];
	/* inform drives */
	for (i = 0; i < cbm_serial.count; i++)
		vc1541_clock_write(device->machine, cbm_serial.drives[i], cbm_serial.clock[0]);
}

static WRITE8_DEVICE_HANDLER( sim_drive_data_write )
{
	int i;

	cbm_serial.data[0] =
		cbm_serial.data[1] = data;
	/* update line */
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.data[0] &= cbm_serial.data[i + 2];
	/* inform drives */
	for (i = 0; i < cbm_serial.count; i++)
		vc1541_data_write(device->machine, cbm_serial.drives[i], cbm_serial.data[0]);
}

static WRITE8_DEVICE_HANDLER( sim_drive_request_write )
{
}

static WRITE8_DEVICE_HANDLER( sim_drive_reset_write )
{
	int i;

	for (i = 0; i < cbm_serial.count; i++)
		drive_reset_write(cbm_serial.drives[i], data);
	/* init bus signals */
}

static const cbm_serial_bus_interface cbm_sim_drive_interface =
{
	DEVCB_HANDLER(sim_drive_atn_read),
	DEVCB_HANDLER(sim_drive_clock_read),
	DEVCB_HANDLER(sim_drive_data_read),
	DEVCB_HANDLER(sim_drive_request_read),
	DEVCB_HANDLER(sim_drive_atn_write),
	DEVCB_HANDLER(sim_drive_clock_write),
	DEVCB_HANDLER(sim_drive_data_write),
	DEVCB_HANDLER(sim_drive_request_write),

	DEVCB_HANDLER(sim_drive_reset_write)
};


MACHINE_DRIVER_START( simulated_drive )
	MDRV_CBM_SERBUS_ADD("serial_bus", cbm_sim_drive_interface)
MACHINE_DRIVER_END



/***********************************************

    Drive handling

***********************************************/

CBM_Drive cbm_drive[2];

CBM_Serial cbm_serial;

/* must be called before other functions */
static void cbm_drive_open( void )
{
	int i;

	memset(cbm_drive, 0, sizeof(cbm_drive));
	memset(&cbm_serial, 0, sizeof(cbm_serial));

	cbm_drive_open_helper();

	cbm_serial.count = 0;
	for (i = 0; i < ARRAY_LENGTH(cbm_serial.atn); i++)

	{
		cbm_serial.atn[i] =
			cbm_serial.data[i] =
			cbm_serial.clock[i] = 1;
	}
}

static void cbm_drive_close( void )
{
	int i;

	cbm_serial.count = 0;
	for (i = 0; i < ARRAY_LENGTH(cbm_drive); i++)
	{
		cbm_drive[i].interface = 0;

		if( cbm_drive[i].buffer )
		{
			free(cbm_drive[i].buffer);
			cbm_drive[i].buffer = NULL;
		}

		if (cbm_drive[i].drive == D64_IMAGE)
			cbm_drive[i].image = NULL;
		cbm_drive[i].drive = 0;
	}
}



static void cbm_drive_config( CBM_Drive * drive, int interface, int serialnr )
{
	int i;

	if (interface == SERIAL)
		drive->i.serial.device = serialnr;

	if (interface == IEEE)
		drive->i.ieee.device = serialnr;

	if (drive->interface == interface)
		return;

	if (drive->interface == SERIAL)
	{
		for (i = 0; (i < cbm_serial.count) && (cbm_serial.drives[i] != drive); i++);
		for (; i + 1 < cbm_serial.count; i++)
			cbm_serial.drives[i] = cbm_serial.drives[i + 1];
		cbm_serial.count--;
	}

	drive->interface = interface;

	if (drive->interface == IEC)
	{
		drive->i.iec.handshakein =
			drive->i.iec.handshakeout = 0;
		drive->i.iec.status = 0;
		drive->i.iec.dataout = drive->i.iec.datain = 0xff;
		drive->i.iec.state = 0;
	}
	else if (drive->interface == SERIAL)
	{
		cbm_serial.drives[cbm_serial.count++] = drive;
		drive_reset_write(drive, 0);
	}
}

void cbm_drive_0_config( int interface, int serialnr )
{
	cbm_drive_config(cbm_drive, interface, serialnr);
}

void cbm_drive_1_config( int interface, int serialnr )
{
	cbm_drive_config(cbm_drive + 1, interface, serialnr);
}


/**************************************

    Floppy device configurations

**************************************/


static DEVICE_START( cbm_drive )
{
	int id = image_index_in_device(device);
	if (id == 0)
		cbm_drive_open();	/* boy these C64 drivers are butt ugly */
}

static DEVICE_STOP( cbm_drive )
{
	int id = image_index_in_device(device);
	if (id == 0)
		cbm_drive_close();	/* boy these C64 drivers are butt ugly */
}



/* open an d64 image */
static DEVICE_IMAGE_LOAD( cbm_drive )
{
	int id = image_index_in_device(image);

	cbm_drive[id].drive = 0;
	cbm_drive[id].image = NULL;

	cbm_drive[id].image = image_ptr(image);
	if (!cbm_drive[id].image)
		return INIT_FAIL;

	cbm_drive[id].drive = D64_IMAGE;
	return 0;
}

static DEVICE_IMAGE_UNLOAD( cbm_drive )
{
	int id = image_index_in_device(image);

	cbm_drive[id].drive = 0;
	cbm_drive[id].image = NULL;
}


void cbmfloppy_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:							info->i = IO_FLOPPY; break;
		case MESS_DEVINFO_INT_READABLE:						info->i = 1; break;
		case MESS_DEVINFO_INT_WRITEABLE:					info->i = 0; break;
		case MESS_DEVINFO_INT_CREATABLE:					info->i = 0; break;
		case MESS_DEVINFO_INT_COUNT:						info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:						info->start = DEVICE_START_NAME(cbm_drive); break;
		case MESS_DEVINFO_PTR_STOP:							info->stop = DEVICE_STOP_NAME(cbm_drive); break;
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(cbm_drive); break;
		case MESS_DEVINFO_PTR_UNLOAD:						info->unload = DEVICE_IMAGE_UNLOAD_NAME(cbm_drive); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "d64"); break;
	}
}
