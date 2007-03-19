/* 
    vMSX tap format

    Virtual MSX 1.x (being replaced by the MSX driver in mess) had
    it's on format for cassette recordings. The format is similar
    to .cas files; however several recordings could be stored in
    one file. 

    Virtual MSX was never really popular; I haven't seen any of these
    files on the internet. However for completeness sake I've added
    this converter. It's read only; the MSX driver cannot read
    these files, but can read the .cas files (like most other MSX
    emulators). Further use and specially distribution of the .tap files 
    is discouraged as Virtual MSX is obsolete (along with its private
    file formats).

    These files used the M$ mmio functions (similar to .aiff files). There
    is an INFO block which lists all files in the archive. The TAPE block
    contains the actual data.

	Original code in Virtual MSX:

		ftp://ftp.komkon.org/pub/EMUL8/MSX/Emulators/VMSX1SRC.zip
	
	It's in the file TAPE.C.

	RIFF format specs:

		http://www.saettler.com/RIFFMCI/riffmci.html


    This module converts the files to .cas files. Implemented are:

    dir - list all the files in the archive as "%title%-%type%.cas"
    getall - writes all the files to disk
    get - gets one file. If "getall" fails, due to characters in the
          filename your FS/OS doesn't support, use: 

	imgtool get vmsx_tap monmsx.tap monmsx-binary.cas monmsx.cas

    for example.

	Sean Young 
*/

#include "osdepend.h"
#include "imgtoolx.h"
#include "utils.h"

typedef struct
    {
    char        title[32];
    char        type[10];
    char    	chunk[4];
    } TAP_ENTRY;

typedef struct
	{
	imgtool_image 		base;
	imgtool_stream 		*file_handle;
	int 		size;
	UINT8		*data;
	int 		count;
	TAP_ENTRY 	*entries;
	} TAP_IMAGE;

typedef struct
	{
	imgtool_directory 	base;
	TAP_IMAGE	*image;
	int			index;
	} TAP_ITERATOR;

static int vmsx_tap_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg);
static void vmsx_tap_image_exit(imgtool_image *img);
static int vmsx_tap_image_beginenum(imgtool_image *img, imgtool_directory **outenum);
static int vmsx_tap_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent);
static void vmsx_tap_image_closeenum(imgtool_directory *enumeration);
static int vmsx_tap_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf);

IMAGEMODULE(
	vmsx_tap,
	"Virtual MSX Tape archive",		/* human readable name */
	"tap",								/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* eoln */
	0,									/* flags */
	vmsx_tap_image_init,				/* init function */
	vmsx_tap_image_exit,				/* exit function */
	NULL,								/* info function */
	vmsx_tap_image_beginenum,			/* begin enumeration */
	vmsx_tap_image_nextenum,			/* enumerate next */
	vmsx_tap_image_closeenum,			/* close enumeration */
	NULL,								/* free space on image */
	vmsx_tap_image_readfile,			/* read file */
	NULL,/* write file */
	NULL,/* delete file */
	NULL,/* create image */
	NULL,
	NULL,
	NULL,
	NULL
)

static const UINT8 CasHeader[8] = { 0x1F,0xA6,0xDE,0xBA,0xCC,0x13,0x7D,0x74 };


static int vmsx_tap_read_image (TAP_IMAGE *image)
	{
	unsigned char *p, *pmem;
    unsigned int pos, i, size;
	UINT32 offset;

	pmem = image->data;
    size = image->size;
    pos = 0;
    while ( (pos + 8) < size)
		{
		offset = *((UINT32*)(pmem + pos + 4));
		offset = LITTLE_ENDIANIZE_INT32 (offset);
		if (strncmp ((char*)pmem + pos, "INFO", 4) )
			{
			/* not this chunk, skip */
			if (offset & 1) offset++; /* pad byte */
			pos += offset + 8;
			continue;
			}
		/* found it */
		pos += 8;
		p = pmem + pos;
		/* intregity check */
		if ( (offset + pos) > size)
			return IMGTOOLERR_CORRUPTIMAGE;
		if (p[0] != 0 || p[1] != 1)
			return IMGTOOLERR_CORRUPTIMAGE;

        image->count = p[2] + p[3] * 256;
		p += 4;
		if (offset < (image->count * (32 + 10 + 4) + 4) )
			return IMGTOOLERR_CORRUPTIMAGE;

		image->entries = (TAP_ENTRY*)malloc (
			sizeof (TAP_ENTRY) * image->count);
		if (!image->entries)
			return IMGTOOLERR_OUTOFMEMORY;

		for (i=0;i<image->count;i++)
			{
			/* this should really check the data */
			strncpy (image->entries[i].title, (char*)p, 32); p += 32;
			strncpy (image->entries[i].type, (char*)p, 10); p += 10;
			memcpy (image->entries[i].chunk, (char*)p, 4); p += 4;
			}

		return 0;
		}	

	return IMGTOOLERR_MODULENOTFOUND;
	}

static int vmsx_tap_image_read_data (TAP_IMAGE *image, char *chunk, unsigned char **pcas, int *psize)
	{
	unsigned int caspos, pos = 0, found = 0, offset, size, tappos;
	UINT32 tapblock;
	unsigned char *p, *pmem;

	size = image->size;
	pmem = image->data;

    while ( (pos + 8) < size)
		{
		offset = *((UINT32*)(pmem + pos + 4));
		offset = LITTLE_ENDIANIZE_INT32 (offset);
		if (memcmp (pmem + pos, "LIST", 4) )
			{
			if (offset & 1) offset++;
			pos += offset + 8;
			continue;
			}

		if (memcmp (pmem + pos + 8, "TAPE", 4) )
			return IMGTOOLERR_CORRUPTIMAGE;

		pos += 12;
		offset -= 8;
		if ( (pos + offset) > size)
			return IMGTOOLERR_CORRUPTIMAGE;

		pmem += pos;
		size = offset;
		found = 1;
		break;
		}

    if (!found)
		return IMGTOOLERR_CORRUPTIMAGE;

	/* OK we've got the right data chunk. Now we can start looking for
		the blocks */
	pos = 0; 
	while ( (pos + 8) < size)
		{
		offset = *((UINT32*)(pmem + pos + 4));
		offset = LITTLE_ENDIANIZE_INT32 (offset);
		if (memcmp (pmem + pos, chunk, 4) )
			{
			if (offset & 1) offset++;
			pos += offset + 8;
			continue;
			}

		pos += 8;
		caspos = tappos = 0;
		p = malloc (offset * 2);
		if (!p) return IMGTOOLERR_OUTOFMEMORY;

		while ( (tappos + 4) <= offset)
			{
			tapblock = *((UINT32*)(pmem + pos + tappos));
			tappos += 4;
			tapblock = LITTLE_ENDIANIZE_INT32 (tapblock);
			if (tapblock == 0xffffffff) break;
			memcpy (p + caspos, CasHeader, 8); 
			caspos += 8;
			memcpy (p + caspos, pmem + pos + tappos, tapblock); 
			caspos += tapblock;
			tappos += tapblock;
			}


		*psize = caspos;
		*pcas = p;
	
		return 0;
		}

	return IMGTOOLERR_CORRUPTIMAGE;
	}

static int vmsx_tap_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg)
{
	TAP_IMAGE *image;
	int rc;

	image = (TAP_IMAGE*)malloc (sizeof (TAP_IMAGE) );
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	*outimg = (imgtool_image*)image;

	memset(image, 0, sizeof(TAP_IMAGE));
	image->base.module = mod;
	image->size=stream_size(f);
	image->file_handle=f;

	image->data = (UINT8*) malloc(image->size);
	if (!image->data)
	{
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_OUTOFMEMORY;
	}

	if (stream_read(f, image->data, image->size)!=image->size) 
	{
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_READERROR;
	}

	if ( (rc=vmsx_tap_read_image(image)) ) 	
	{
		if (image->entries) free(image->entries);
		free(image);
		*outimg=NULL;
		return rc;
	}

	return 0;
}

static void vmsx_tap_image_exit(imgtool_image *img)
{
	TAP_IMAGE *image=(TAP_IMAGE*)img;
	stream_close(image->file_handle);
	free(image->entries);
	free(image->data);
	free(image);
}

static int vmsx_tap_image_beginenum(imgtool_image *img, imgtool_directory **outenum)
{
	TAP_IMAGE *image=(TAP_IMAGE*)img;
	TAP_ITERATOR *iter;

	iter=*(TAP_ITERATOR**)outenum = (TAP_ITERATOR*) malloc(sizeof(TAP_ITERATOR));
	if (!iter)
		return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;

	iter->image=image;
	iter->index = 0;
	return 0;
}

static int vmsx_tap_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent)
	{
	TAP_ITERATOR *iter=(TAP_ITERATOR*)enumeration;
	unsigned char *p;
	int size, rc;
	
	ent->eof=iter->index>=iter->image->count;
	if (!ent->eof) 
		{
		sprintf (ent->fname, "%s-%s.cas", 
			iter->image->entries[iter->index].title,
			iter->image->entries[iter->index].type);
		size = 0;
		p = NULL;
		rc = vmsx_tap_image_read_data (iter->image, 
			iter->image->entries[iter->index].chunk, &p, &size); 

		ent->corrupt=rc;
		if (!rc) ent->filesize = size;
		iter->index++;

		if (p) free (p);
		}

	return 0;
	}

static void vmsx_tap_image_closeenum(imgtool_directory *enumeration)
	{
	free (enumeration);
	}

static TAP_ENTRY* vmsx_tap_image_findfile(TAP_IMAGE *image, const char *fname)
	{
	int i;
	char filename[64]; /* that's always enough! */

	for (i=0; i<image->count; i++)
		{
		sprintf (filename, "%s-%s.cas", 
			image->entries[i].title, image->entries[i].type);
		if (!strcmp(fname, filename) ) return image->entries+i;
		}
	return NULL;
	}

static int vmsx_tap_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf)
	{
	TAP_IMAGE *image=(TAP_IMAGE*)img;
	TAP_ENTRY *entry;
	unsigned char* p;
	int size, rc;

	if ((entry=vmsx_tap_image_findfile(image, fname))==NULL ) 
		return IMGTOOLERR_MODULENOTFOUND;

	rc = vmsx_tap_image_read_data (image, entry->chunk, &p, &size);
	if (!rc)
		{
		if (stream_write(destf, p, size)!=size)
			{
			free (p);
			return IMGTOOLERR_WRITEERROR;
			}
		}

    free (p);

	return rc;
	}

