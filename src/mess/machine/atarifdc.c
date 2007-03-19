/***************************************************************************

    Atari 400/800

    Floppy Disk Controller code

    Juergen Buchmueller, June 1998

***************************************************************************/

#include <ctype.h>

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "includes/atari.h"
#include "sound/pokey.h"
#include "machine/6821pia.h"
#include "image.h"

#define VERBOSE_SERIAL	0
#define VERBOSE_CHKSUM	0

ATARI_FDC atari_fdc;

typedef struct _atari_drive atari_drive;
struct _atari_drive
{
	UINT8 *image;		/* malloc'd image */
	int type;			/* type of image (XFD, ATR, DSK) */
	int mode;			/* 0 read only, != 0 read/write */
	int density;		/* 0 SD, 1 MD, 2 DD */
	int header_skip;	/* number of bytes in format header */
	int tracks; 		/* number of tracks (35,40,77,80) */
	int heads;			/* number of heads (1,2) */
	int spt;			/* sectors per track (18,26) */
	int seclen; 		/* sector length (128,256) */
	int bseclen;		/* boot sector length (sectors 1..3) */
	int sectors;		/* total sectors, ie. tracks x heads x spt */
};

static atari_drive drv[4];

/*************************************
 *
 *  Disk stuff
 *
 *************************************/

#define FORMAT_XFD	0
#define FORMAT_ATR	1
#define FORMAT_DSK	2

/*****************************************************************************
 * This is the structure I used in my own Atari 800 emulator some years ago
 * Though it's a bit overloaded, I used it as it is the maximum of all
 * supported formats:
 * XFD no header at all
 * ATR 16 bytes header
 * DSK this struct
 * It is used to determine the format of a XFD image by it's size only
 *****************************************************************************/

typedef struct _dsk_format dsk_format;
struct _dsk_format
{
	UINT8 density;
	UINT8 tracks;
	UINT8 door;
	UINT8 sta1;
	UINT8 spt;
	UINT8 doublesided;
	UINT8 highdensity;
	UINT8 seclen_hi;
	UINT8 seclen_lo;
	UINT8 status;
	UINT8 sta2;
	UINT8 sta3;
	UINT8 sta4;
	UINT8 cr;
	UINT8 info[65+1];
};

/* combined with the size the image should have */
typedef struct _xfd_format xfd_format;
struct _xfd_format
{
	int size;
	dsk_format dsk;
};

/* here's a table of known xfd formats */
static const xfd_format xfd_formats[] =
{
	{35 * 18 * 1 * 128, 				{0,35,1,0,18,0,0,0,128,255,0,0,0,13,"35 SS/SD"}},
	{35 * 26 * 1 * 128, 				{1,35,1,0,26,0,4,0,128,255,0,0,0,13,"35 SS/MD"}},
	{(35 * 18 * 1 - 3) * 256 + 3 * 128, {2,35,1,0,18,0,4,1,  0,255,0,0,0,13,"35 SS/DD"}},
	{40 * 18 * 1 * 128, 				{0,40,1,0,18,0,0,0,128,255,0,0,0,13,"40 SS/SD"}},
	{40 * 26 * 1 * 128, 				{1,40,1,0,26,0,4,0,128,255,0,0,0,13,"40 SS/MD"}},
	{(40 * 18 * 1 - 3) * 256 + 3 * 128, {2,40,1,0,18,0,4,1,  0,255,0,0,0,13,"40 SS/DD"}},
	{40 * 18 * 2 * 128, 				{0,40,1,0,18,1,0,0,128,255,0,0,0,13,"40 DS/SD"}},
	{40 * 26 * 2 * 128, 				{1,40,1,0,26,1,4,0,128,255,0,0,0,13,"40 DS/MD"}},
	{(40 * 18 * 2 - 3) * 256 + 3 * 128, {2,40,1,0,18,1,4,1,  0,255,0,0,0,13,"40 DS/DD"}},
	{77 * 18 * 1 * 128, 				{0,77,1,0,18,0,0,0,128,255,0,0,0,13,"77 SS/SD"}},
	{77 * 26 * 1 * 128, 				{1,77,1,0,26,0,4,0,128,255,0,0,0,13,"77 SS/MD"}},
	{(77 * 18 * 1 - 3) * 256 + 3 * 128, {2,77,1,0,18,0,4,1,  0,255,0,0,0,13,"77 SS/DD"}},
	{77 * 18 * 2 * 128, 				{0,77,1,0,18,1,0,0,128,255,0,0,0,13,"77 DS/SD"}},
	{77 * 26 * 2 * 128, 				{1,77,1,0,26,1,4,0,128,255,0,0,0,13,"77 DS/MD"}},
	{(77 * 18 * 2 - 3) * 256 + 3 * 128, {2,77,1,0,18,1,4,1,  0,255,0,0,0,13,"77 DS/DD"}},
	{80 * 18 * 2 * 128, 				{0,80,1,0,18,1,0,0,128,255,0,0,0,13,"80 DS/SD"}},
	{80 * 26 * 2 * 128, 				{1,80,1,0,26,1,4,0,128,255,0,0,0,13,"80 DS/MD"}},
	{(80 * 18 * 2 - 3) * 256 + 3 * 128, {2,80,1,0,18,1,4,1,  0,255,0,0,0,13,"80 DS/DD"}},
	{0, {0,}}
};

/*****************************************************************************
 *
 * Open a floppy image for drive 'drive' if it is not yet openend
 * and a name was given. Determine the image geometry depending on the
 * type of image and store the results into the global drv[] structure
 *
 *****************************************************************************/

#define MAXSIZE 5760 * 256 + 80
DEVICE_LOAD( a800_floppy )
{
	int size, i;
	const char *ext;
	int id = image_index_in_device(image);

	drv[id].image = image_malloc(image, MAXSIZE);
	if (!drv[id].image)
		return INIT_FAIL;

	/* tell whether the image is writable */
	drv[id].mode = image_is_writable(image);
	/* set up image if it has been created */
	if (image_has_been_created(image))
	{
		int sector;
		char buff[256];
		memset(buff, 0, 256);
		/* default to 720 sectors */
		for( sector = 0; sector < 720; sector++ )
			image_fwrite(image, buff, 256);
		image_fseek(image, 0, SEEK_SET);
	}

	size = image_fread(image, drv[id].image, MAXSIZE);
	if( size <= 0 )
	{
		drv[id].image = NULL;
		return INIT_FAIL;
	}
	/* re allocate the buffer; we don't want to be too lazy ;) */
    drv[id].image = image_realloc(image, drv[id].image, size);

	ext = image_filetype(image);
    /* no extension: assume XFD format (no header) */
    if (!ext)
    {
        drv[id].type = FORMAT_XFD;
        drv[id].header_skip = 0;
    }
    else
    /* XFD extension */
    if( toupper(ext[0])=='X' && toupper(ext[1])=='F' && toupper(ext[2])=='D' )
    {
        drv[id].type = FORMAT_XFD;
        drv[id].header_skip = 0;
    }
    else
    /* ATR extension */
    if( toupper(ext[0])=='A' && toupper(ext[1])=='T' && toupper(ext[2])=='R' )
    {
        drv[id].type = FORMAT_ATR;
        drv[id].header_skip = 16;
    }
    else
    /* DSK extension */
    if( toupper(ext[0])=='D' && toupper(ext[1])=='S' && toupper(ext[2])=='K' )
    {
        drv[id].type = FORMAT_DSK;
        drv[id].header_skip = sizeof(dsk_format);
    }
    else
    {
		drv[id].type = FORMAT_XFD;
        drv[id].header_skip = 0;
    }

	if( drv[id].type == FORMAT_ATR &&
		(drv[id].image[0] != 0x96 || drv[id].image[1] != 0x02) )
	{
		drv[id].type = FORMAT_XFD;
		drv[id].header_skip = 0;
	}

	switch (drv[id].type)
	{
	/* XFD or unknown format: find a matching size from the table */
	case FORMAT_XFD:
		for( i = 0; xfd_formats[i].size; i++ )
		{
			if( size == xfd_formats[i].size )
			{
				drv[id].density = xfd_formats[i].dsk.density;
				drv[id].tracks = xfd_formats[i].dsk.tracks;
				drv[id].spt = xfd_formats[i].dsk.spt;
				drv[id].heads = (xfd_formats[i].dsk.doublesided) ? 2 : 1;
				drv[id].bseclen = 128;
				drv[id].seclen = 256 * xfd_formats[i].dsk.seclen_hi + xfd_formats[i].dsk.seclen_lo;
				drv[id].sectors = drv[id].tracks * drv[id].heads * drv[id].spt;
				break;
			}
		}
		break;
	/* ATR format: find a size including the 16 bytes header */
	case FORMAT_ATR:
		{
			int s;

			drv[id].bseclen = 128;
			/* get sectors from ATR header */
			s = (size - 16) / 128;
			/* 3 + odd number of sectors ? */
			if ( drv[id].image[4] == 128 || (s % 18) == 0 || (s % 26) == 0 || ((s - 3) % 1) != 0 )
			{
				drv[id].sectors = s;
				drv[id].seclen = 128;
				/* sector size 128 or count not evenly dividable by 26 ? */
				if( drv[id].seclen == 128 || (s % 26) != 0 )
				{
					/* yup! single density */
					drv[id].density = 0;
					drv[id].spt = 18;
					drv[id].heads = 1;
					drv[id].tracks = s / 18;
					if( s % 18 != 0 )
						drv[id].tracks += 1;
					if( drv[id].tracks % 2 == 0 && drv[id].tracks > 80 )
					{
						drv[id].heads = 2;
						drv[id].tracks /= 2;
					}
				}
				else
				{
					/* yes: medium density */
					drv[id].density = 0;
					drv[id].spt = 26;
					drv[id].heads = 1;
					drv[id].tracks = s / 26;
					if( s % 26 != 0 )
						drv[id].tracks += 1;
					if( drv[id].tracks % 2 == 0 && drv[id].tracks > 80 )
					{
						drv[id].heads = 2;
						drv[id].tracks /= 2;
					}
				}
			}
			else
			{
				/* it's double density */
				s = (s - 3) / 2 + 3;
				drv[id].sectors = s;
				drv[id].density = 2;
				drv[id].seclen = 256;
				drv[id].spt = 18;
				drv[id].heads = 1;
				drv[id].tracks = s / 18;
				if( s % 18 != 0 )
					drv[id].tracks += 1;
				if( drv[id].tracks % 2 == 0 && drv[id].tracks > 80 )
				{
					drv[id].heads = 2;
					drv[id].tracks /= 2;
				}
			}
		}
		break;
	/* DSK format: it's all in the header */
	case FORMAT_DSK:
		{
			dsk_format *dsk = (dsk_format *) drv[id].image;

			drv[id].tracks = dsk->tracks;
			drv[id].spt = dsk->spt;
			drv[id].heads = (dsk->doublesided) ? 2 : 1;
			drv[id].seclen = 256 * dsk->seclen_hi + dsk->seclen_lo;
			drv[id].bseclen = drv[id].seclen;
			drv[id].sectors = drv[id].tracks * drv[id].heads * drv[id].spt;
		}
		break;
	}
	logerror("atari opened floppy '%s', %d sectors (%d %s%s) %d bytes/sector\n",
			image_filename(image),
			drv[id].sectors,
			drv[id].tracks,
			(drv[id].heads == 1) ? "SS" : "DS",
			(drv[id].density == 0) ? "SD" : (drv[id].density == 1) ? "MD" : "DD",
			drv[id].seclen);
	return INIT_PASS;
}



/*****************************************************************************
 *
 * This is a description of the data flow between Atari (A) and the
 * Floppy (F) for the supported commands.
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *          '1'  'S'  00   00                 get status
 * F->A     ACK  CPL  04   FF   E0   00   CKS
 *                     ^    ^
 *                     |    |
 *                     |    bit 7 : door closed
 *                     |
 *                     bit7  : MD 128 bytes/sector, 26 sectors/track
 *                     bit5  : DD 256 bytes/sector, 18 sectors/track
 *                     else  : SD 128 bytes/sector, 18 sectors/track
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *          '1'  'R'  SECL SECH               read sector
 * F->A     ACK                               command acknowledge
 *               ***                          now read the sector
 * F->A              CPL                      complete: sector read
 * F->A                  128/256 byte CKS
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *          '1'  'W'  SECL SECH               write with verify
 * F->A     ACK                               command acknowledge
 * A->F          128/256 data CKS
 * F->A                            CPL        complete: CKS okay
 *          execute writing the sector
 * F->A                                 CPL   complete: sector written
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *          '1'  'P'  SECL SECH               put sector
 * F->A     ACK                               command acknowledge
 * A->F          128/256 data CKS
 * F->A                            CPL        complete: CKS okay
 *          execute writing the sector
 * F->A                                 CPL   complete: sector written
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *           '1' '!'  xx   xx                 single density format
 * F->A     ACK                               command acknowledge
 *          execute formatting
 * F->A               CPL                     complete: format
 * F->A                    128/256 byte CKS   bad sector table
 *
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *          '1'  '"'  xx   xx                 double density format
 * F->A     ACK                               command acknowledge
 *          execute formatting
 * F->A               CPL                     complete: format
 * F->A                    128/256 byte CKS   bad sector table
 *
 *****************************************************************************/

static	void make_chksum(UINT8 * chksum, UINT8 data)
{
	UINT8 new;
	new = *chksum + data;
	if (new < *chksum)
		new++;

	if (VERBOSE_CHKSUM)
		logerror("atari chksum old $%02x + data $%02x -> new $%02x\n", *chksum, data, new);

	*chksum = new;
}

static	void clr_serout(int expect_data)
{
	atari_fdc.serout_chksum = 0;
	atari_fdc.serout_offs = 0;
	atari_fdc.serout_count = expect_data + 1;
}

static void add_serout(int expect_data)
{
	atari_fdc.serout_chksum = 0;
	atari_fdc.serout_count = expect_data + 1;
}

static void clr_serin(int ser_delay)
{
	atari_fdc.serin_chksum = 0;
	atari_fdc.serin_offs = 0;
	atari_fdc.serin_count = 0;
	pokey1_serin_ready(ser_delay * 40);
}

static void add_serin(UINT8 data, int with_checksum)
{
	atari_fdc.serin_buff[atari_fdc.serin_count++] = data;
	if (with_checksum)
		make_chksum(&atari_fdc.serin_chksum, data);
}

static void a800_serial_command(void)
{
	int  i, drive, sector, offset;

	if( !atari_fdc.serout_offs )
	{
		if (VERBOSE_SERIAL)
			logerror("atari serout command offset = 0\n");
		return;
	}
	clr_serin(10);

	if (VERBOSE_SERIAL)
	{
		logerror("atari serout command %d: %02X %02X %02X %02X %02X : %02X ",
			atari_fdc.serout_offs,
			atari_fdc.serout_buff[0], atari_fdc.serout_buff[1], atari_fdc.serout_buff[2],
			atari_fdc.serout_buff[3], atari_fdc.serout_buff[4], atari_fdc.serout_chksum);
	}

	if (atari_fdc.serout_chksum == 0)
	{
		if (VERBOSE_SERIAL)
			logerror("OK\n");

		drive = atari_fdc.serout_buff[0] - '1';   /* drive # */
		/* sector # */
		if (drive < 0 || drive > 3) 			/* ignore unknown drives */
		{
			logerror("atari unsupported drive #%d\n", drive+1);
			sprintf(atari_frame_message, "DRIVE #%d not supported", drive+1);
			atari_frame_counter = Machine->screen[0].refresh/2;
			return;
		}

		/* extract sector number from the command buffer */
		sector = atari_fdc.serout_buff[2] + 256 * atari_fdc.serout_buff[3];

		switch (atari_fdc.serout_buff[1]) /* command ? */
		{
			case 'S':   /* status */
				sprintf(atari_frame_message, "DRIVE #%d STATUS", drive+1);
				atari_frame_counter = Machine->screen[0].refresh/2;
				
				if (VERBOSE_SERIAL)
					logerror("atari status\n");

				add_serin('A',0);
				add_serin('C',0);
				if (!drv[drive].mode) /* read only mode ? */
				{
					if (drv[drive].spt == 26)
						add_serin(0x80,1);	/* MD: 0x80 */
					else
					if (drv[drive].seclen == 128)
						add_serin(0x00,1);	/* SD: 0x00 */
					else
						add_serin(0x20,1);	/* DD: 0x20 */
				}
				else
				{
					if (drv[drive].spt == 26)
						add_serin(0x84,1);	/* MD: 0x84 */
					else
					if (drv[drive].seclen == 128)
						add_serin(0x04,1);	/* SD: 0x04 */
					else
						add_serin(0x24,1);	/* DD: 0x24 */
				}
				if (drv[drive].image)
					add_serin(0xff,1);	/* door closed: 0xff */
				else
					add_serin(0x7f,1);	/* door open: 0x7f */
				add_serin(0xe0,1);	/* dunno */
				add_serin(0x00,1);	/* dunno */
				add_serin(atari_fdc.serin_chksum,0);
				break;

			case 'R':   /* read sector */
				if (VERBOSE_SERIAL)
					logerror("atari read sector #%d\n", sector);

				if( sector < 1 || sector > drv[drive].sectors )
				{
					sprintf(atari_frame_message, "DRIVE #%d READ SECTOR #%3d - ERR", drive+1, sector);
					atari_frame_counter = Machine->screen[0].refresh/2;

					if (VERBOSE_SERIAL)
						logerror("atari bad sector #\n");

					add_serin('E',0);
					break;
				}
				add_serin('A',0);   /* acknowledge */
				add_serin('C',0);   /* completed */
				if (sector < 4) 	/* sector 1 .. 3 might be different length */
				{
					sprintf(atari_frame_message, "DRIVE #%d READ SECTOR #%3d - SD", drive+1, sector);
                    atari_frame_counter = Machine->screen[0].refresh/2;
                    offset = (sector - 1) * drv[drive].bseclen + drv[drive].header_skip;
					for (i = 0; i < 128; i++)
						add_serin(drv[drive].image[offset++],1);
				}
				else
				{
					sprintf(atari_frame_message, "DRIVE #%d READ SECTOR #%3d - %cD", drive+1, sector, (drv[drive].seclen == 128) ? 'S' : 'D');
                    atari_frame_counter = Machine->screen[0].refresh/2;
                    offset = (sector - 1) * drv[drive].seclen + drv[drive].header_skip;
					for (i = 0; i < drv[drive].seclen; i++)
						add_serin(drv[drive].image[offset++],1);
				}
				add_serin(atari_fdc.serin_chksum,0);
				break;

			case 'W':   /* write sector with verify */
				if (VERBOSE_SERIAL)
					logerror("atari write sector #%d\n", sector);

				add_serin('A',0);
				if (sector < 4) 	/* sector 1 .. 3 might be different length */
				{
					add_serout(drv[drive].bseclen);
					sprintf(atari_frame_message, "DRIVE #%d WRITE SECTOR #%3d - SD", drive+1, sector);
					atari_frame_counter = Machine->screen[0].refresh/2;
                }
				else
				{
					add_serout(drv[drive].seclen);
					sprintf(atari_frame_message, "DRIVE #%d WRITE SECTOR #%3d - %cD", drive+1, sector, (drv[drive].seclen == 128) ? 'S' : 'D');
                    atari_frame_counter = Machine->screen[0].refresh/2;
                }
				break;

			case 'P':   /* put sector (no verify) */
				if (VERBOSE_SERIAL)
					logerror("atari put sector #%d\n", sector);

				add_serin('A',0);
				if (sector < 4) 	/* sector 1 .. 3 might be different length */
				{
					add_serout(drv[drive].bseclen);
					sprintf(atari_frame_message, "DRIVE #%d PUT SECTOR #%3d - SD", drive+1, sector);
                    atari_frame_counter = Machine->screen[0].refresh/2;
                }
				else
				{
					add_serout(drv[drive].seclen);
					sprintf(atari_frame_message, "DRIVE #%d PUT SECTOR #%3d - %cD", drive+1, sector, (drv[drive].seclen == 128) ? 'S' : 'D');
                    atari_frame_counter = Machine->screen[0].refresh/2;
                }
				break;

			case '!':   /* SD format */
				if (VERBOSE_SERIAL)
					logerror("atari format SD drive #%d\n", drive+1);

				sprintf(atari_frame_message, "DRIVE #%d FORMAT SD", drive+1);
				atari_frame_counter = Machine->screen[0].refresh/2;
                add_serin('A',0);   /* acknowledge */
				add_serin('C',0);   /* completed */
				for (i = 0; i < 128; i++)
					add_serin(0,1);
				add_serin(atari_fdc.serin_chksum,0);
				break;

			case '"':   /* DD format */
				if (VERBOSE_SERIAL)
					logerror("atari format DD drive #%d\n", drive+1);

				sprintf(atari_frame_message, "DRIVE #%d FORMAT DD", drive+1);
                atari_frame_counter = Machine->screen[0].refresh/2;
                add_serin('A',0);   /* acknowledge */
				add_serin('C',0);   /* completed */
				for (i = 0; i < 256; i++)
					add_serin(0,1);
				add_serin(atari_fdc.serin_chksum,0);
				break;

			default:
				if (VERBOSE_SERIAL)
					logerror("atari unknown command #%c\n", atari_fdc.serout_buff[1]);

				sprintf(atari_frame_message, "DRIVE #%d UNKNOWN CMD '%c'", drive+1, atari_fdc.serout_buff[1]);
                atari_frame_counter = Machine->screen[0].refresh/2;
                add_serin('N',0);   /* negative acknowledge */
		}
	}
	else
	{
		sprintf(atari_frame_message, "serial cmd chksum error");
		atari_frame_counter = Machine->screen[0].refresh/2;
		if (VERBOSE_SERIAL)
			logerror("BAD\n");

		add_serin('E',0);
	}
	if (VERBOSE_SERIAL)
		logerror("atari %d bytes to read\n", atari_fdc.serin_count);
}

void a800_serial_write(void)
{
	int i, drive, sector, offset;

	if (VERBOSE_SERIAL)
	{
		logerror("atari serout %d bytes written : %02X ",
			atari_fdc.serout_offs, atari_fdc.serout_chksum);
	}

	clr_serin(80);
	if (atari_fdc.serout_chksum == 0)
	{
		if (VERBOSE_SERIAL)
			logerror("OK\n");

		add_serin('C',0);
		/* write the sector */
		drive = atari_fdc.serout_buff[0] - '1';   /* drive # */
		/* not write protected and image available ? */
		if (drv[drive].mode && drv[drive].image)
		{
			/* extract sector number from the command buffer */
			sector = atari_fdc.serout_buff[2] + 256 * atari_fdc.serout_buff[3];
			if (sector < 4) 	/* sector 1 .. 3 might be different length */
			{
				offset = (sector - 1) * drv[drive].bseclen + drv[drive].header_skip;
				
				if (VERBOSE_SERIAL)
					logerror("atari storing 128 byte sector %d at offset 0x%08X", sector, offset );

				for (i = 0; i < 128; i++)
					drv[drive].image[offset++] = atari_fdc.serout_buff[5+i];
				sprintf(atari_frame_message, "DRIVE #%d WROTE SECTOR #%3d - SD", drive+1, sector);
				atari_frame_counter = Machine->screen[0].refresh/2;
            }
			else
			{
				offset = (sector - 1) * drv[drive].seclen + drv[drive].header_skip;

				if (VERBOSE_SERIAL)
					logerror("atari storing %d byte sector %d at offset 0x%08X", drv[drive].seclen, sector, offset );

				for (i = 0; i < drv[drive].seclen; i++)
					drv[drive].image[offset++] = atari_fdc.serout_buff[5+i];
				sprintf(atari_frame_message, "DRIVE #%d WROTE SECTOR #%3d - %cD", drive+1, sector, (drv[drive].seclen == 128) ? 'S' : 'D');
                atari_frame_counter = Machine->screen[0].refresh/2;
            }
			add_serin('C',0);
		}
		else
		{
			add_serin('E',0);
		}
	}
	else
	{
		if (VERBOSE_SERIAL)
			logerror("BAD\n");

		add_serin('E',0);
	}
}

READ8_HANDLER ( atari_serin_r )
{
	int data = 0x00;
	int ser_delay = 0;

	if (atari_fdc.serin_count)
	{
		data = atari_fdc.serin_buff[atari_fdc.serin_offs];
		ser_delay = 2 * 40;
		if (atari_fdc.serin_offs < 3)
		{
			ser_delay = 4 * 40;
			if (atari_fdc.serin_offs < 2)
				ser_delay = 200 * 40;
		}
		atari_fdc.serin_offs++;
		if (--atari_fdc.serin_count == 0)
			atari_fdc.serin_offs = 0;
		else
			pokey1_serin_ready(ser_delay);
	}

	if (VERBOSE_SERIAL)
		logerror("atari serin[$%04x] -> $%02x; delay %d\n", atari_fdc.serin_offs, data, ser_delay);

	return data;
}

WRITE8_HANDLER ( atari_serout_w )
{
	/* ignore serial commands if no floppy image is specified */
	if( !drv[0].image )
		return;
	if (atari_fdc.serout_count)
	{
		/* store data */
		atari_fdc.serout_buff[atari_fdc.serout_offs] = data;

		if (VERBOSE_SERIAL)
			logerror("atari serout[$%04x] <- $%02x; count %d\n", atari_fdc.serout_offs, data, atari_fdc.serout_count);

		atari_fdc.serout_offs++;
		if (--atari_fdc.serout_count == 0)
		{
			/* exclusive or written checksum with calculated */
			atari_fdc.serout_chksum ^= data;
			/* if the attention line is high, this should be data */
			if (pia_get_irq_b(0))
				a800_serial_write();
		}
		else
		{
			make_chksum(&atari_fdc.serout_chksum, data);
		}
	}
}



WRITE8_HANDLER(atari_pia_cb2_w)
{
	if (!data)
	{
		clr_serout(4); /* expect 4 command bytes + checksum */
	}
	else
	{
		atari_fdc.serin_delay = 0;
		a800_serial_command();
	}
}
