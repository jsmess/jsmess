#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "driver.h"

#define VERBOSE_DBG 1				   /* general debug messages */
#include "includes/cbm.h"
#include "includes/cbmserb.h"
#include "includes/cbmieeeb.h"

#include "includes/cbmdrive.h"

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

int d64_sectors_per_track[] =
{
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
	19, 19, 19, 19, 19, 19, 19,
	18, 18, 18, 18, 18, 18,
	17, 17, 17, 17, 17
};

static int d64_offset[D64_MAX_TRACKS];		   /* offset of begin of track in d64 file */

/* must be called before other functions */
void cbm_drive_open_helper (void)
{
	int i;

	d64_offset[0] = 0;
	for (i = 1; i <= 35; i++)
		d64_offset[i] = d64_offset[i - 1] + d64_sectors_per_track[i - 1] * 256;
}

/* calculates offset to beginning of d64 file for sector beginning */
int d64_tracksector2offset (int track, int sector)
{
	return d64_offset[track - 1] + sector * 256;
}

static int cbm_compareNames (unsigned char *left, unsigned char *right)
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
static int d64_find (CBM_Drive * drive, unsigned char *name)
{
	int pos, track, sector, i;

	pos = d64_tracksector2offset (18, 0);
	track = drive->image[pos];
	sector = drive->image[pos + 1];

	while ((track >= 1) && (track <= 35))
	{
		pos = d64_tracksector2offset (track, sector);
		for (i = 2; i < 256; i += 32)
		{
			if (drive->image[pos + i] & 0x80)
			{
				if (mame_stricmp ((char *) name, (char *) "*") == 0)
					return pos + i;
				if (cbm_compareNames (name, drive->image + pos + i + 3))
					return pos + i;
			}
		}
		track = drive->image[pos];
		sector = drive->image[pos + 1];
	}
	return -1;
}

/* reads file into buffer */
static void d64_readprg (CBM_Drive * c1551, int pos)
{
	int i;

	for (i = 0; i < 16; i++)
		c1551->filename[i] = toupper (c1551->image[pos + i + 3]);

	c1551->filename[i] = 0;

	pos = d64_tracksector2offset (c1551->image[pos + 1], c1551->image[pos + 2]);

	i = pos;
	c1551->size = 0;
	while (c1551->image[i] != 0)
	{
		c1551->size += 254;
		i = d64_tracksector2offset (c1551->image[i], c1551->image[i + 1]);
	}
	c1551->size += c1551->image[i + 1];

	DBG_LOG (3, "d64 readprg", ("size %d\n", c1551->size));

	c1551->buffer = (UINT8*)realloc (c1551->buffer, c1551->size);
	if (!c1551->buffer)
		fatalerror("out of memory %s %d\n", __FILE__, __LINE__);

	c1551->size--;

	DBG_LOG (3, "d64 readprg", ("track: %d sector: %d\n",
								c1551->image[pos + 1],
								c1551->image[pos + 2]));

	for (i = 0; i < c1551->size; i += 254)
	{
		if (i + 254 < c1551->size)
		{							   /* not last sector */
			memcpy (c1551->buffer + i, c1551->image + pos + 2, 254);
			pos = d64_tracksector2offset (c1551->image[pos + 0],
									  c1551->image[pos + 1]);
			DBG_LOG (3, "d64 readprg", ("track: %d sector: %d\n",
										c1551->image[pos],
										c1551->image[pos + 1]));
		}
		else
		{
			memcpy (c1551->buffer + i, c1551->image + pos + 2, c1551->size - i);
		}
	}
}

/* reads sector into buffer */
static void d64_read_sector (CBM_Drive * c1551, int track, int sector)
{
	int pos;

	snprintf (c1551->filename, sizeof (c1551->filename),
			  "track %d sector %d", track, sector);

	pos = d64_tracksector2offset (track, sector);

	c1551->buffer = (UINT8*)realloc (c1551->buffer,256);
	if (!c1551->buffer)
		fatalerror("out of memory %s %d\n", __FILE__, __LINE__);

	logerror("d64 read track %d sector %d\n", track, sector);

	memcpy (c1551->buffer, c1551->image + pos, 256);
	c1551->size = 256;
	c1551->pos = 0;
}

/* reads directory into buffer */
static void d64_read_directory (CBM_Drive * c1551)
{
	int pos, track, sector, i, j, blocksfree, addr = 0x0101/*0x1001*/;

	c1551->buffer = (UINT8*)realloc (c1551->buffer, 8 * 18 * 25);
	if (!c1551->buffer)
		fatalerror("out of memory %s %d\n", __FILE__, __LINE__);

	c1551->size = 0;

	pos = d64_tracksector2offset (18, 0);
	track = c1551->image[pos];
	sector = c1551->image[pos + 1];

	blocksfree = 0;
	for (j = 1, i = 4; j <= 35; j++, i += 4)
	{
		blocksfree += c1551->image[pos + i];
	}
	c1551->buffer[c1551->size++] = addr & 0xff;
	c1551->buffer[c1551->size++] = addr >> 8;
	addr += 29;
	c1551->buffer[c1551->size++] = addr & 0xff;
	c1551->buffer[c1551->size++] = addr >> 8;
	c1551->buffer[c1551->size++] = 0;
	c1551->buffer[c1551->size++] = 0;
	c1551->buffer[c1551->size++] = '\"';
	for (j = 0; j < 16; j++)
		c1551->buffer[c1551->size++] = c1551->image[pos + 0x90 + j];
/*memcpy(c1551->buffer+c1551->size,c1551->image+pos+0x90, 16);c1551->size+=16; */
	c1551->buffer[c1551->size++] = '\"';
	c1551->buffer[c1551->size++] = ' ';
	c1551->buffer[c1551->size++] = c1551->image[pos + 162];
	c1551->buffer[c1551->size++] = c1551->image[pos + 163];
	c1551->buffer[c1551->size++] = ' ';
	c1551->buffer[c1551->size++] = c1551->image[pos + 165];
	c1551->buffer[c1551->size++] = c1551->image[pos + 166];
	c1551->buffer[c1551->size++] = 0;

	while ((track >= 1) && (track <= 35))
	{
		pos = d64_tracksector2offset (track, sector);
		for (i = 2; i < 256; i += 32)
		{
			if (c1551->image[pos + i] & 0x80)
			{
				int len, blocks = c1551->image[pos + i + 28]
				+ 256 * c1551->image[pos + i + 29];
				char dummy[10];

				sprintf (dummy, "%d", blocks);
				len = strlen (dummy);
				addr += 29 - len;
				c1551->buffer[c1551->size++] = addr & 0xff;
				c1551->buffer[c1551->size++] = addr >> 8;
				c1551->buffer[c1551->size++] = c1551->image[pos + i + 28];
				c1551->buffer[c1551->size++] = c1551->image[pos + i + 29];
				for (j = 4; j > len; j--)
					c1551->buffer[c1551->size++] = ' ';
				c1551->buffer[c1551->size++] = '\"';
				for (j = 0; j < 16; j++)
					c1551->buffer[c1551->size++] = c1551->image[pos + i + 3 + j];
				c1551->buffer[c1551->size++] = '\"';
				c1551->buffer[c1551->size++] = ' ';
				switch (c1551->image[pos + i] & 0x3f)
				{
				case 0:
					c1551->buffer[c1551->size++] = 'D';
					c1551->buffer[c1551->size++] = 'E';
					c1551->buffer[c1551->size++] = 'L';
					break;
				case 1:
					c1551->buffer[c1551->size++] = 'S';
					c1551->buffer[c1551->size++] = 'E';
					c1551->buffer[c1551->size++] = 'Q';
					break;
				case 2:
					c1551->buffer[c1551->size++] = 'P';
					c1551->buffer[c1551->size++] = 'R';
					c1551->buffer[c1551->size++] = 'G';
					break;
				case 3:
					c1551->buffer[c1551->size++] = 'U';
					c1551->buffer[c1551->size++] = 'S';
					c1551->buffer[c1551->size++] = 'R';
					break;
				case 4:
					c1551->buffer[c1551->size++] = 'R';
					c1551->buffer[c1551->size++] = 'E';
					c1551->buffer[c1551->size++] = 'L';
					break;
				}
				c1551->buffer[c1551->size++] = 0;
			}
		}
		track = c1551->image[pos];
		sector = c1551->image[pos + 1];
	}
	addr += 14;
	c1551->buffer[c1551->size++] = addr & 0xff;
	c1551->buffer[c1551->size++] = addr >> 8;
	c1551->buffer[c1551->size++] = blocksfree & 0xff;
	c1551->buffer[c1551->size++] = blocksfree >> 8;
	memcpy (c1551->buffer + c1551->size, "BLOCKS FREE", 11);
	c1551->size += 11;
	c1551->buffer[c1551->size++] = 0;

	strcpy (c1551->filename, "$");
}

static int c1551_d64_command (CBM_Drive * c1551, unsigned char *name)
{
	int pos;

	/* name eventuell mit 0xa0 auffuellen */

	if (mame_stricmp ((char *) name, (char *) "$") == 0)
	{
		d64_read_directory (c1551);
	}
	else
	{
		if ((pos = d64_find (c1551, name)) == -1)
		{
			return 1;
		}
		d64_readprg (c1551, pos);
	}
	return 0;
}

/**
  7.1 Serial bus

   CBM Serial Bus Control Codes

	20	Talk
	3F	Untalk
	40	Listen
	5F	Unlisten
	60	Open Channel
	70	-
	80	-
	90	-
	A0	-
	B0	-
	C0	-
	D0	-
	E0	Close
	F0	Open



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
static void cbm_command (CBM_Drive * drive)
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
				rc = c1551_d64_command (drive, name);
		}
		if (!rc)
		{
			drive->state = OPEN;
			drive->pos = 0;
		}
		DBG_LOG (1, "cbm_open", ("%s %s type:%c %c\n", name,
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
void c1551_state (CBM_Drive * c1551)
{
#if VERBOSE_DBG
	static int oldstate;

	oldstate = c1551->i.iec.state;
#endif

	switch (c1551->i.iec.state)
	{
	case -1:						   /* currently neccessary for correct init */
		if (c1551->i.iec.handshakein)
		{
			c1551->i.iec.state++;
		}
		break;
	case 0:
		if (c1551->i.iec.datain == 0x55)
		{
			c1551->i.iec.status = 0;
			c1551->i.iec.state++;
		}
		break;
	case 1:
		if (c1551->i.iec.datain != 0x55)
		{
			c1551->i.iec.state = 10;
		}
		break;
	case 10:
		if (c1551->i.iec.datain != 0)
		{
			c1551->i.iec.handshakeout = 0;
			if (c1551->i.iec.datain == 0x84)
			{
				c1551->i.iec.state = 20;
				if (c1551->pos + 1 == c1551->size)
					c1551->i.iec.status = 3;
			}
			else if (c1551->i.iec.datain == 0x83)
			{
				c1551->i.iec.state = 40;
			}
			else
			{
				c1551->i.iec.status = 0;
				c1551->i.iec.state++;
			}
		}
		break;
	case 11:
		if (!c1551->i.iec.handshakein)
		{
			c1551->i.iec.state++;
			DBG_LOG(1,"c1551",("taken data %.2x\n",c1551->i.iec.datain));
			if (c1551->cmdpos < sizeof (c1551->cmdbuffer))
				c1551->cmdbuffer[c1551->cmdpos++] = c1551->i.iec.datain;
			if ((c1551->i.iec.datain == 0x3f) || (c1551->i.iec.datain == 0x5f))
			{
				cbm_command (c1551);
				c1551->i.iec.state = 30;
			}
			else if (((c1551->i.iec.datain & 0xf0) == 0x60))
			{
				cbm_command (c1551);
				if (c1551->state == READING)
				{
				}
				else if (c1551->state == WRITING)
				{
				}
				else
					c1551->i.iec.status = 3;
			}
			c1551->i.iec.handshakeout = 1;
		}
		break;
	case 12:
		if (c1551->i.iec.datain == 0)
		{
			c1551->i.iec.state++;
		}
		break;
	case 13:
		if (c1551->i.iec.handshakein)
		{
			c1551->i.iec.state = 10;
		}
		break;

	case 20:						   /* reading data */
		if (!c1551->i.iec.handshakein)
		{
			c1551->i.iec.handshakeout = 1;
			if (c1551->state == READING)
				c1551->i.iec.dataout = c1551->buffer[c1551->pos++];
			c1551->i.iec.state++;
		}
		break;
	case 21:						   /* reading data */
		if (c1551->i.iec.handshakein)
		{
			c1551->i.iec.handshakeout = 0;
			c1551->i.iec.state++;
		}
		break;
	case 22:
		if (c1551->i.iec.datain == 0)
		{
			c1551->i.iec.state++;
		}
		break;
	case 23:
		if (!c1551->i.iec.handshakein)
		{
			c1551->i.iec.handshakeout = 1;
			if (c1551->state == READING)
				c1551->i.iec.state = 10;
			else
				c1551->i.iec.state = 0;
		}
		break;

	case 30:						   /* end of command */
		if (c1551->i.iec.datain == 0)
		{
			c1551->i.iec.state++;
		}
		break;
	case 31:
		if (c1551->i.iec.handshakein)
		{
			c1551->i.iec.state = 0;
		}
		break;

	case 40:						   /* simple write */
		if (!c1551->i.iec.handshakein)
		{
			c1551->i.iec.state++;
			if ((c1551->state == 0) || (c1551->state == OPEN))
			{
				DBG_LOG (1, "c1551", ("taken data %.2x\n",
									  c1551->i.iec.datain));
				if (c1551->cmdpos < sizeof (c1551->cmdbuffer))
					c1551->cmdbuffer[c1551->cmdpos++] = c1551->i.iec.datain;
			}
			else if (c1551->state == WRITING)
			{
				DBG_LOG (1, "c1551", ("written data %.2x\n", c1551->i.iec.datain));
			}
			c1551->i.iec.handshakeout = 1;
		}
		break;
	case 41:
		if (c1551->i.iec.datain == 0)
		{
			c1551->i.iec.state++;
		}
		break;
	case 42:
		if (c1551->i.iec.handshakein)
		{
			c1551->i.iec.state = 10;
		}
		break;
	}
#if VERBOSE_DBG
	if (oldstate != c1551->i.iec.state)
		logerror ("state %d->%d %d\n", oldstate, c1551->i.iec.state, c1551->state);
#endif
}

void vc1541_state (CBM_Drive * vc1541)
{
#if VERBOSE_DBG
	int oldstate = vc1541->i.serial.state;

#endif

	switch (vc1541->i.serial.state)
	{
	case 0:
		if (!cbm_serial.atn[0] && !cbm_serial.clock[0])
		{
			vc1541->i.serial.data = 0;
			vc1541->i.serial.state = 2;
			break;
		}
		if (!cbm_serial.clock[0] && vc1541->i.serial.forme)
		{
			vc1541->i.serial.data = 0;
			vc1541->i.serial.state++;
			break;
		}
		break;
	case 1:
		if (!cbm_serial.atn[0])
		{
			vc1541->i.serial.state++;
			break;
		}
		if (cbm_serial.clock[0])
		{
			vc1541->i.serial.broadcast = 0;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.state = 100;
			vc1541->i.serial.last = 0;
			vc1541->i.serial.value = 0;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;
	case 2:
		if (cbm_serial.clock[0])
		{
			vc1541->i.serial.broadcast = 1;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.state = 100;
			vc1541->i.serial.last = 0;
			vc1541->i.serial.value = 0;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;
		/* bits to byte fitting */
	case 100:
		if (!cbm_serial.clock[0])
		{
			vc1541->i.serial.state++;
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
			vc1541->i.serial.state++;
		break;
	case 101:
		if (cbm_serial.clock[0])
		{
			vc1541->i.serial.value |= cbm_serial.data[0] ? 1 : 0;
			vc1541->i.serial.state++;
		}
		break;
	case 103:
		if (cbm_serial.clock[0])
		{
			vc1541->i.serial.value |= cbm_serial.data[0] ? 2 : 0;
			vc1541->i.serial.state++;
		}
		break;
	case 105:
		if (cbm_serial.clock[0])
		{
			vc1541->i.serial.value |= cbm_serial.data[0] ? 4 : 0;
			vc1541->i.serial.state++;
		}
		break;
	case 107:
		if (cbm_serial.clock[0])
		{
			vc1541->i.serial.value |= cbm_serial.data[0] ? 8 : 0;
			vc1541->i.serial.state++;
		}
		break;
	case 109:
		if (cbm_serial.clock[0])
		{
			vc1541->i.serial.value |= cbm_serial.data[0] ? 0x10 : 0;
			vc1541->i.serial.state++;
		}
		break;
	case 111:
		if (cbm_serial.clock[0])
		{
			vc1541->i.serial.value |= cbm_serial.data[0] ? 0x20 : 0;
			vc1541->i.serial.state++;
		}
		break;
	case 113:
		if (cbm_serial.clock[0])
		{
			vc1541->i.serial.value |= cbm_serial.data[0] ? 0x40 : 0;
			vc1541->i.serial.state++;
		}
		break;
	case 115:
		if (cbm_serial.clock[0])
		{
			vc1541->i.serial.value |= cbm_serial.data[0] ? 0x80 : 0;
			if (vc1541->i.serial.broadcast
				&& (((vc1541->i.serial.value & 0xf0) == 0x20)
					|| ((vc1541->i.serial.value & 0xf0) == 0x40)))
			{
				vc1541->i.serial.forme = (vc1541->i.serial.value & 0xf)
					== vc1541->i.serial.device;
				if (!vc1541->i.serial.forme)
				{
					vc1541->i.serial.state = 160;
					break;
				}
			}
			if (vc1541->i.serial.forme)
			{
				if (vc1541->cmdpos < sizeof (vc1541->cmdbuffer))
					vc1541->cmdbuffer[vc1541->cmdpos++] = vc1541->i.serial.value;
				DBG_LOG (1, "serial read", ("%s %s %.2x\n",
							vc1541->i.serial.broadcast ? "broad" : "",
							vc1541->i.serial.last ? "last" : "",
											vc1541->i.serial.value));
			}
			vc1541->i.serial.state++;
		}
		break;
	case 116:
		if (!cbm_serial.clock[0])
		{
			if (vc1541->i.serial.last)
				vc1541->i.serial.state = 130;
			else
				vc1541->i.serial.state++;
			if (vc1541->i.serial.broadcast &&
				((vc1541->i.serial.value == 0x3f) || (vc1541->i.serial.value == 0x5f)
				 || ((vc1541->i.serial.value & 0xf0) == 0x60)))
			{
				cbm_command (vc1541);
			}
			vc1541->i.serial.time = timer_get_time ();
			vc1541->i.serial.data = 0;
			break;
		}
		break;
	case 117:
		if (vc1541->i.serial.forme && ((vc1541->i.serial.value & 0xf0) == 0x60)
			&& vc1541->i.serial.broadcast && cbm_serial.atn[0])
		{
			if (vc1541->state == READING)
			{
				vc1541->i.serial.state = 200;
				break;
			}
			else if (vc1541->state != WRITING)
			{
				vc1541->i.serial.state = 150;
				break;
			}
		}
		if (((vc1541->i.serial.value == 0x3f)
			 || (vc1541->i.serial.value == 0x5f))
			&& vc1541->i.serial.broadcast && cbm_serial.atn[0])
		{
			vc1541->i.serial.data = 1;
			vc1541->i.serial.state = 140;
			break;
		}
		if (cbm_serial.clock[0])
		{
			vc1541->i.serial.time = timer_get_time ();
			vc1541->i.serial.broadcast = !cbm_serial.atn[0];
			vc1541->i.serial.data = 1;
			vc1541->i.serial.value = 0;
			vc1541->i.serial.state++;
			break;
		}
		break;
		/* if computer lowers clk not in 200micros (last byte following)
		 * negativ pulse on data by listener */
	case 118:
		if (!cbm_serial.clock[0])
		{
			vc1541->i.serial.value = 0;
			vc1541->i.serial.state = 101;
			vc1541->i.serial.data = 1;
			break;
		}
		if (timer_get_time () - vc1541->i.serial.time >= 200e-6)
		{
			vc1541->i.serial.data = 0;
			vc1541->i.serial.last = 1;
			vc1541->i.serial.state++;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;
	case 119:
		if (timer_get_time () - vc1541->i.serial.time >= 60e-6)
		{
			vc1541->i.serial.value = 0;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.state = 100;
			break;
		}
		break;

	case 130:						   /* last byte of talk */
		if (timer_get_time () - vc1541->i.serial.time >= 60e-6)
		{
			vc1541->i.serial.data = 1;
			vc1541->i.serial.state = 0;
			break;
		}
		break;

	case 131:
		if (cbm_serial.clock[0])
		{
			vc1541->i.serial.state = 0;
		}
		break;

	case 140:						   /* end of talk */
		if (cbm_serial.clock[0])
		{
			vc1541->i.serial.state = 0;
		}
		break;

	case 150:						   /* file not found */
		if (cbm_serial.atn[0])
		{
			vc1541->i.serial.state++;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;
	case 151:
		if (timer_get_time () - vc1541->i.serial.time > 1000e-6)
		{
			vc1541->i.serial.state++;
			vc1541->i.serial.clock = 0;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;
	case 152:
		if (timer_get_time () - vc1541->i.serial.time > 50e-6)
		{
			vc1541->i.serial.state++;
			vc1541->i.serial.clock = 1;
			break;
		}
		break;
	case 153:
		if (cbm_serial.clock[0])
		{
			vc1541->i.serial.state = 0;
		}
		break;

	case 160:						   /* not for me */
		if (cbm_serial.atn[0])
		{
			vc1541->i.serial.state = 0;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;

	case 200:
		if (cbm_serial.clock[0])
		{
			vc1541->i.serial.state++;
			vc1541->i.serial.clock = 0;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;
	case 201:
		if (timer_get_time () - vc1541->i.serial.time > 80e-6)
		{
			vc1541->i.serial.clock = 1;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.state = 300;
			break;
		}
		break;

	case 300:
		if (!cbm_serial.atn[0])
		{
			vc1541->i.serial.state = 330;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 1;
			break;
		}
		if (cbm_serial.data[0])
		{
			vc1541->i.serial.value = vc1541->buffer[vc1541->pos];
			vc1541->i.serial.clock = 0;
			vc1541->i.serial.data = (vc1541->i.serial.value & 1) ? 1 : 0;
			vc1541->i.serial.state++;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;
	case 301:
		if (!cbm_serial.atn[0])
		{
			vc1541->i.serial.state = 330;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 1;
			break;
		}
		if (timer_get_time () - vc1541->i.serial.time > 40e-6)
		{
			vc1541->i.serial.clock = 1;
			vc1541->i.serial.state++;
			vc1541->i.serial.time = timer_get_time ();
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
			vc1541->i.serial.state = 330;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 1;
			break;
		}
		if (timer_get_time () - vc1541->i.serial.time > 20e-6)
		{
			vc1541->i.serial.clock = 1;
			vc1541->i.serial.state++;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;
	case 302:
		if (!cbm_serial.atn[0])
		{
			vc1541->i.serial.state = 330;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 1;
			break;
		}
		if (timer_get_time () - vc1541->i.serial.time > 20e-6)
		{
			vc1541->i.serial.data = vc1541->i.serial.value & 2 ? 1 : 0;
			vc1541->i.serial.clock = 0;
			vc1541->i.serial.state++;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;
	case 304:
		if (!cbm_serial.atn[0])
		{
			vc1541->i.serial.state = 330;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 1;
			break;
		}
		if (timer_get_time () - vc1541->i.serial.time > 20e-6)
		{
			vc1541->i.serial.data = vc1541->i.serial.value & 4 ? 1 : 0;
			vc1541->i.serial.clock = 0;
			vc1541->i.serial.state++;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;
	case 306:
		if (!cbm_serial.atn[0])
		{
			vc1541->i.serial.state = 330;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 1;
			break;
		}
		if (timer_get_time () - vc1541->i.serial.time > 20e-6)
		{
			vc1541->i.serial.data = vc1541->i.serial.value & 8 ? 1 : 0;
			vc1541->i.serial.clock = 0;
			vc1541->i.serial.state++;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;
	case 308:
		if (!cbm_serial.atn[0])
		{
			vc1541->i.serial.state = 330;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 1;
			break;
		}
		if (timer_get_time () - vc1541->i.serial.time > 20e-6)
		{
			vc1541->i.serial.data = vc1541->i.serial.value & 0x10 ? 1 : 0;
			vc1541->i.serial.clock = 0;
			vc1541->i.serial.state++;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;
	case 310:
		if (!cbm_serial.atn[0])
		{
			vc1541->i.serial.state = 330;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 1;
			break;
		}
		if (timer_get_time () - vc1541->i.serial.time > 20e-6)
		{
			vc1541->i.serial.data = vc1541->i.serial.value & 0x20 ? 1 : 0;
			vc1541->i.serial.clock = 0;
			vc1541->i.serial.state++;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;
	case 312:
		if (!cbm_serial.atn[0])
		{
			vc1541->i.serial.state = 330;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 1;
			break;
		}
		if (timer_get_time () - vc1541->i.serial.time > 20e-6)
		{
			vc1541->i.serial.data = vc1541->i.serial.value & 0x40 ? 1 : 0;
			vc1541->i.serial.clock = 0;
			vc1541->i.serial.state++;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;
	case 314:
		if (!cbm_serial.atn[0])
		{
			vc1541->i.serial.state = 330;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 1;
			break;
		}
		if (timer_get_time () - vc1541->i.serial.time > 20e-6)
		{
			vc1541->i.serial.data = vc1541->i.serial.value & 0x80 ? 1 : 0;
			vc1541->i.serial.clock = 0;
			vc1541->i.serial.state++;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;
	case 316:
		if (!cbm_serial.atn[0])
		{
			vc1541->i.serial.state = 330;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 1;
			break;
		}
		if (timer_get_time () - vc1541->i.serial.time > 20e-6)
		{
			DBG_LOG (1, "vc1541", ("%.2x written\n", vc1541->i.serial.value));
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 0;
			vc1541->i.serial.state++;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;
	case 317:
		if (!cbm_serial.atn[0])
		{
			vc1541->i.serial.state = 330;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 1;
			break;
		}
		if (!cbm_serial.data[0])
		{
			vc1541->i.serial.state++;
			vc1541->i.serial.time = timer_get_time ();
			break;
		}
		break;
	case 318:
		if (!cbm_serial.atn[0])
		{
			vc1541->i.serial.state = 330;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 1;
			break;
		}
		if (vc1541->pos + 1 == vc1541->size)
		{
			vc1541->i.serial.clock = 1;
			vc1541->i.serial.state = 0;
			break;
		}
		if (vc1541->pos + 2 == vc1541->size)
		{
			vc1541->pos++;
			vc1541->i.serial.state = 320;
			break;
		}
		if (timer_get_time () - vc1541->i.serial.time > 100e-6)
		{
			vc1541->pos++;
			vc1541->i.serial.clock = 1;
			vc1541->i.serial.state = 300;
			break;
		}
		break;
	case 320:
		if (!cbm_serial.atn[0])
		{
			vc1541->i.serial.state = 330;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 1;
			break;
		}
		if (timer_get_time () - vc1541->i.serial.time > 100e-6)
		{
			vc1541->i.serial.clock = 1;
			vc1541->i.serial.state++;
			break;
		}
		break;
	case 321:
		if (!cbm_serial.atn[0])
		{
			vc1541->i.serial.state = 330;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 1;
			break;
		}
		if (cbm_serial.data[0])
		{
			vc1541->i.serial.state++;
		}
		break;
	case 322:
		if (!cbm_serial.atn[0])
		{
			vc1541->i.serial.state = 330;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 1;
			break;
		}
		if (!cbm_serial.data[0])
		{
			vc1541->i.serial.state++;
		}
		break;
	case 323:
		if (!cbm_serial.atn[0])
		{
			vc1541->i.serial.state = 330;
			vc1541->i.serial.data = 1;
			vc1541->i.serial.clock = 1;
			break;
		}
		if (cbm_serial.data[0])
		{
			vc1541->i.serial.state = 300;
		}
		break;
	case 330:						   /* computer breaks receiving */
		vc1541->i.serial.state = 0;
		break;
	}
#if VERBOSE_DBG
	if (oldstate != vc1541->i.serial.state)
		logerror ("%d state %d->%d %d %s %s %s\n",
				 vc1541->i.serial.device,
				 oldstate,
				 vc1541->i.serial.state, vc1541->state,
				 cbm_serial.atn[0] ? "ATN" : "atn",
				 cbm_serial.clock[0] ? "CLOCK" : "clock",
				 cbm_serial.data[0] ? "DATA" : "data");
#endif
}

/* difference between vic20 and pet (first series)
   pet lowers atn and wants a reaction on ndac */

void c2031_state(CBM_Drive *drive)
{
#if VERBOSE_DBG
	int oldstate = drive->i.ieee.state;
#endif
	int data;

	switch (drive->i.ieee.state)
	{
	case 0:
		if (!cbm_ieee_dav_r()) {
			drive->i.ieee.state=10;
		} else if (!cbm_ieee_atn_r()) {
			drive->i.ieee.state=11;
			cbm_ieee_ndac_w(1,0);
			logerror("arsch\n");
		}
		break;
	case 1:
		break;
	case 10:
		if (cbm_ieee_dav_r()) {
			drive->i.ieee.state++;
			cbm_ieee_nrfd_w(1,1);
			cbm_ieee_ndac_w(1,0);
		}
		break;
	case 11:
		if (!cbm_ieee_dav_r()) {
			cbm_ieee_nrfd_w(1,0);
			data=cbm_ieee_data_r()^0xff;
			cbm_ieee_ndac_w(1,1);
			logerror("byte received %.2x\n",data);
			if (!cbm_ieee_atn_r()&&((data&0x0f)==drive->i.ieee.device) ) {
				if ((data&0xf0)==0x40)
					drive->i.ieee.state=30;
				else
					drive->i.ieee.state=20;
				if (drive->cmdpos < sizeof (drive->cmdbuffer))
					drive->cmdbuffer[drive->cmdpos++] = data;
			} else if ((data&0xf)==0xf) {
				drive->i.ieee.state--;
			} else {
				drive->i.ieee.state++;
			}
		}
		break;
		/* wait until atn is released */
	case 12:
		if (cbm_ieee_atn_r()) {
			drive->i.ieee.state++;
			cbm_ieee_nrfd_w(1,0);
		}
		break;
	case 13:
		if (!cbm_ieee_atn_r()) {
			drive->i.ieee.state=10;
/*			cbm_ieee_nrfd_w(1,0); */
		}
		break;

		/* receiving rest of command */
	case 20:
		if (cbm_ieee_dav_r()) {
			drive->i.ieee.state++;
			cbm_ieee_nrfd_w(1,1);
			cbm_ieee_ndac_w(1,0);
		}
		break;
	case 21:
		if (!cbm_ieee_dav_r()) {
			cbm_ieee_nrfd_w(1,0);
			data=cbm_ieee_data_r()^0xff;
			logerror("byte received %.2x\n",data);
			if (drive->cmdpos < sizeof (drive->cmdbuffer))
				drive->cmdbuffer[drive->cmdpos++] = data;
			if (!cbm_ieee_atn_r()&&((data&0xf)==0xf)) {
				cbm_command(drive);
				drive->i.ieee.state=10;
			} else
				drive->i.ieee.state=20;
			cbm_ieee_ndac_w(1,1);
		}
		break;

		/* read command */
	case 30:
		if (cbm_ieee_dav_r()) {
			drive->i.ieee.state++;
			cbm_ieee_nrfd_w(1,1);
			cbm_ieee_ndac_w(1,0);
		}
		break;
	case 31:
		if (!cbm_ieee_dav_r()) {
			cbm_ieee_nrfd_w(1,0);
			data=cbm_ieee_data_r()^0xff;
			logerror("byte received %.2x\n",data);
			if (drive->cmdpos < sizeof (drive->cmdbuffer))
				drive->cmdbuffer[drive->cmdpos++] = data;
			cbm_command(drive);
			if (drive->state==READING)
				drive->i.ieee.state++;
			else
				drive->i.ieee.state=10;
			cbm_ieee_ndac_w(1,1);
		}
		break;
	case 32:
		if (cbm_ieee_dav_r()) {
			cbm_ieee_nrfd_w(1,1);
			drive->i.ieee.state=40;
		}
		break;
	case 40:
		if (!cbm_ieee_ndac_r()) {
			cbm_ieee_data_w(1,drive->buffer[drive->pos++]^0xff);
			if (drive->pos>=drive->size)
				cbm_ieee_eoi_w(1,0);
			cbm_ieee_dav_w(1,0);
			drive->i.ieee.state++;
		}
		break;
	case 41:
		if (!cbm_ieee_nrfd_r()) {
			drive->i.ieee.state++;
		}
		break;
	case 42:
		if (cbm_ieee_ndac_r()) {
			if (cbm_ieee_eoi_r())
				drive->i.ieee.state=40;
			else {
				cbm_ieee_data_w(1,0xff);
				cbm_ieee_ndac_w(1,0);
				cbm_ieee_nrfd_w(1,0);
				cbm_ieee_eoi_w(1,1);
				drive->i.ieee.state=10;
			}
			cbm_ieee_dav_w(1,1);
		}
		break;
	}

#if VERBOSE_DBG
	if (oldstate != drive->i.ieee.state)
		logerror("%d state %d->%d %d\n",
				 drive->i.ieee.device,
				 oldstate,
				 drive->i.ieee.state, drive->state
				 );
#endif
}
