/*
 * msx_dsk.c : converts .ddi/.img/.msx/multidisk disk images to .dsk image
 *
 * Sean Young 2001
 */

/*
 * .msx files are a certain type of disk dumps. They always seem to be
 * double sided 720kB ones. There are no extra headers, but the order of
 * the sectors is all messed up. This little program should put that
 * straight. :)
 *
 * The structure is as follows: the first 9 sectors (=0x1200 bytes) of the
 * first side, then the first 9 sectors of the other side, the next 9
 * sectors of the first site, the next 9 sectors of the other side.. etc.
 *
 * These weird .msx files are found at: ftp://jazz.snu.ac.kr/pub/msx/
 */

/*
 * .ddi DiskDupe images are always double sided 720kB images (at least in the
 * MSX world). There is a header of 0x1200 bytes (in the version known to the
 * MSX world; this is version is 5.12 -- and probably others). This format
 * is used by the CompuJunks MSX2 emulator (MSX2EMUL).
 *
 * The exact format of the header is unknown to me. However, this does not
 * seem to be important; the last 720kB is a complete continuous dump of
 * the disk.
 */

/*
 * .img files have the following format: first a one byte header, which
 * is 1 (01h) for singled sided 360kB images, and 2 (02h) for double sided
 * 720kB images. These files can be found at ftp://ftp.funet.fi/pub/msx/
 */

/*
 * .dsk multidisks are used by fmsx-dos 1.6. These files are always double 
 * sided 3.5" disks (720kB), simply appended to one another.
 */

#include <assert.h>
#include "osdepend.h"
#include "imgtoolx.h"
#include "utils.h"


typedef struct {
	imgtool_image			base; 
	char			*file_name;
	imgtool_stream 			*file_handle;
	int 			size, format, disks;
	} DSK_IMAGE;

typedef struct
	{
	imgtool_directory 	base;
	DSK_IMAGE	*image;
	int			index;
	} DSK_ITERATOR;

static int msx_dsk_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg);
static void msx_dsk_image_exit(imgtool_image *img);
static int msx_dsk_image_beginenum(imgtool_image *img, imgtool_directory **outenum);
static int msx_dsk_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent);
static void msx_dsk_image_closeenum(imgtool_directory *enumeration);
static int msx_dsk_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf);

IMAGEMODULE(
	msx_img,
	"MSX .img diskimage",				/* human readable name */
	"img\0",							/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* eoln */
	0,									/* flags */
	msx_dsk_image_init,					/* init function */
	msx_dsk_image_exit,					/* exit function */
	NULL,								/* info function */
	msx_dsk_image_beginenum,			/* begin enumeration */
	msx_dsk_image_nextenum,				/* enumerate next */
	msx_dsk_image_closeenum,			/* close enumeration */
	NULL,								/* free space on image */
	msx_dsk_image_readfile,				/* read file */
	NULL,								/* write file */
	NULL,								/* delete file */
	NULL,								/* create image */
	NULL,								/* read sector */
	NULL,								/* write sector */
	NULL,								/* file options */
	NULL								/* create options */
)

IMAGEMODULE(
	msx_ddi,
	"MSX DiskDupe .ddi diskimage",		/* human readable name */
	"ddi\0",							/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* eoln */
	0,									/* flags */
	msx_dsk_image_init,					/* init function */
	msx_dsk_image_exit,					/* exit function */
	NULL,								/* info function */
	msx_dsk_image_beginenum,			/* begin enumeration */
	msx_dsk_image_nextenum,				/* enumerate next */
	msx_dsk_image_closeenum,			/* close enumeration */
	NULL,								/* free space on image */
	msx_dsk_image_readfile,				/* read file */
	NULL,								/* write file */
	NULL,								/* delete file */
	NULL,								/* create image */
	NULL,								/* read sector */
	NULL,								/* write sector */
	NULL,								/* file options */
	NULL								/* create options */
)

IMAGEMODULE(
	msx_msx,
	"MSX .msx diskimage",		/* human readable name */
	"ddi\0",							/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* eoln */
	0,									/* flags */
	msx_dsk_image_init,					/* init function */
	msx_dsk_image_exit,					/* exit function */
	NULL,								/* info function */
	msx_dsk_image_beginenum,			/* begin enumeration */
	msx_dsk_image_nextenum,				/* enumerate next */
	msx_dsk_image_closeenum,			/* close enumeration */
	NULL,								/* free space on image */
	msx_dsk_image_readfile,				/* read file */
	NULL,								/* write file */
	NULL,								/* delete file */
	NULL,								/* create image */
	NULL,								/* read sector */
	NULL,								/* write sector */
	NULL,								/* file options */
	NULL								/* create options */
)

IMAGEMODULE(
	msx_mul,
	"MSX multi disk image",				/* human readable name */
	"dsk\0",							/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* eoln */
	0,									/* flags */
	msx_dsk_image_init,					/* init function */
	msx_dsk_image_exit,					/* exit function */
	NULL,								/* info function */
	msx_dsk_image_beginenum,			/* begin enumeration */
	msx_dsk_image_nextenum,				/* enumerate next */
	msx_dsk_image_closeenum,			/* close enumeration */
	NULL,								/* free space on image */
	msx_dsk_image_readfile,				/* read file */
	NULL,								/* write file */
	NULL,								/* delete file */
	NULL,								/* create image */
	NULL,								/* read sector */
	NULL,								/* write sector */
	NULL,								/* file options */
	NULL								/* create options */
)

#define 	FORMAT_IMG		(0)
#define 	FORMAT_DDI		(1)
#define 	FORMAT_MSX		(2)
#define 	FORMAT_MULTI	(3)

static int msx_dsk_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg)
{
	DSK_IMAGE *image;
	int size, disks, correct, format;
    UINT8 header;

	size = stream_size (f);
	if (size < (360*1024) ) 
		return IMGTOOLERR_MODULENOTFOUND;
	
	disks = 1;

	correct = 0;

	if (!strcmp(mod->name, "msx_img"))
	{
		format = FORMAT_IMG;
		if (1 != stream_read (f, &header, 1) ) 
			return IMGTOOLERR_READERROR;

		if ( (size == (720*1024+1) ) && (header == 2) )
			{
			correct = 1;
			size--;
			}
		if ( (size == (360*1024+1) ) && (header == 1) )
			{
			correct = 1;
			size--;
			}
	}
	else if (!strcmp(mod->name, "msx_msx"))
	{
		format = FORMAT_MSX;
		if (size == (720*1024) )
			correct = 1;
	}
	else if (!strcmp(mod->name, "msx_ddi"))
	{
		format = FORMAT_DDI;
		if (size == (720*1024+0x1200) )
			{
			size -= 0x1200;
			correct = 1;
			}
	}
	else if (!strcmp(mod->name, "msx_mul"))
	{
		format = FORMAT_MULTI;
		if ( (size > 720*1024) && !(size % (720*1024) ) )
			{
			disks = size / (720*1024);
			correct = 1;
			size = 720*1024;
			}
	}
	else
	{
		assert(0);
		return IMGTOOLERR_UNEXPECTED;
	}
	
	if (!correct) return IMGTOOLERR_MODULENOTFOUND;

	image = (DSK_IMAGE*)malloc (sizeof (DSK_IMAGE) );
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	*outimg = (imgtool_image*)image;

	memset(image, 0, sizeof(DSK_IMAGE));
	image->base.module = mod;
	image->file_handle = f;
	image->size = size;
	image->format = format;
	image->file_name = NULL;
	image->disks = disks;

	return 0;
	}

static void msx_dsk_image_exit(imgtool_image *img)
	{
	DSK_IMAGE *image=(DSK_IMAGE*)img;
	stream_close(image->file_handle);
	if (image->file_name) free(image->file_name);
	free(image);
	}

static int msx_dsk_image_beginenum(imgtool_image *img, imgtool_directory **outenum)
	{
	DSK_IMAGE *image=(DSK_IMAGE*)img;
	DSK_ITERATOR *iter;

	iter=*(DSK_ITERATOR**)outenum = (DSK_ITERATOR*) malloc(sizeof(DSK_ITERATOR));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = image->base.module; 
	iter->image=image;
	iter->index = 1;

	return 0;
	}

static int msx_dsk_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent)
	{
	DSK_ITERATOR *iter=(DSK_ITERATOR*)enumeration;

	ent->eof = (iter->index > iter->image->disks);
	if (!ent->eof)
		{
		if (iter->image->file_name)
			strcpy (ent->fname, iter->image->file_name);
		else
			{
			if (iter->image->disks ==  1)
				strcpy (ent->fname, "msx.dsk");
			else
				sprintf (ent->fname, "msx-%d.dsk", iter->index);
			}

		ent->corrupt=0;
		ent->filesize = iter->image->size;

		iter->index++;
		}

	return 0;
	}

static void msx_dsk_image_closeenum(imgtool_directory *enumeration)
	{
	free(enumeration);
	}

static int msx_dsk_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf)
	{
	DSK_IMAGE *image=(DSK_IMAGE*)img;
	UINT8	buf[0x1200];
	int 	i, offset, n1, n2, disks = 0;

	/*  check file name */
	switch (image->format)
		{
		case FORMAT_IMG: 
		case FORMAT_DDI: 
		case FORMAT_MSX: 
			if (strcmpi (fname, "msx.dsk") ) 
				return IMGTOOLERR_MODULENOTFOUND;
			break;
		case FORMAT_MULTI:
			if (strncmpi (fname, "msx-", 4) )
				return IMGTOOLERR_MODULENOTFOUND;
			offset = 4; disks = 0;
			while ( (fname[offset] >= '0') || (fname[offset] <= '9') )
				disks = disks * 10 + (fname[offset++] - '0');

			if (mame_stricmp (fname + offset, ".dsk") )
				return IMGTOOLERR_MODULENOTFOUND;

			if ( (disks < 1) || (disks > image->disks) )
				return IMGTOOLERR_MODULENOTFOUND;

			break;
		}

	/* copy the file */
	switch (image->format)
		{
		case FORMAT_MSX:
			i = 80; n1 = 0; n2 = 80;
			while (i--)
				{
				stream_seek (image->file_handle, n1++ * 0x1200, SEEK_SET);
				if (0x1200 != stream_read (image->file_handle, buf, 0x1200) )
					return IMGTOOLERR_READERROR;

				if (0x1200 != stream_write (destf, buf, 0x1200) )
					return IMGTOOLERR_WRITEERROR;

				stream_seek (image->file_handle, n2++ * 0x1200, SEEK_SET);
				if (0x1200 != stream_read (image->file_handle, buf, 0x1200) )
					return IMGTOOLERR_READERROR;

				if (0x1200 != stream_write (destf, buf, 0x1200) )
					return IMGTOOLERR_WRITEERROR;
				}

			return 0;
		case FORMAT_IMG: 
			offset = 1; i = (image->size / 0x1200); 
			break;
		case FORMAT_DDI: 
			offset = 0x1200; i = 160; 
			break;
		case FORMAT_MULTI:	/* multi disk */
			i = 160; offset = 720*1024 * (disks - 1);
			break;
		default:
			return IMGTOOLERR_UNEXPECTED;
		}

	stream_seek (image->file_handle, offset, SEEK_SET);

	while (i--)
		{
		if (0x1200 != stream_read (image->file_handle, buf, 0x1200) )
			return IMGTOOLERR_READERROR;

		if (0x1200 != stream_write (destf, buf, 0x1200) )
			return IMGTOOLERR_WRITEERROR;
		}

	return 0;
	}

