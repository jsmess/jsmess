#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osdepend.h"
#include "imgtoolx.h"

/* commodore 64 lynx format
   motivation
   stuff several files in one file, for archiving and transmission?
   no packing?

   commodore basic header
   ended with 0 0 0 0x0d
   string count begin of data in 254 byte blocks 0x0d

   string count of entries in directory 0x0d

   count of entries
    name 0x13
	string blocks 0x13
	char type 0x13
	string bytes_in_last_block 0x13

   data for the entries
    filled up to 254 byte blocks

*/

typedef struct {
	char name[21];
	int offset, size;
	char filetype;
} LYNX_ENTRY;

typedef struct {
	imgtool_image base;
	imgtool_stream *file_handle;
	int size;
	unsigned char *data;
	int count;
	LYNX_ENTRY *entries;
} lynx_image;

typedef struct {
	imgtool_directory base;
	lynx_image *image;
	int index;
} lynx_iterator;

static int lynx_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg);
static void lynx_image_exit(imgtool_image *img);
//static void lynx_image_info(imgtool_image *img, char *string, const int len);
static int lynx_image_beginenum(imgtool_image *img, imgtool_directory **outenum);
static int lynx_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent);
static void lynx_image_closeenum(imgtool_directory *enumeration);
static int lynx_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf);

IMAGEMODULE(
	lynx,
	"Commodore 64 Archive",	/* human readable name */
	"lnx",								/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* eoln */
	0,									/* flags */
	lynx_image_init,				/* init function */
	lynx_image_exit,				/* exit function */
	NULL,//lynx_image_info,		/* info function */
	lynx_image_beginenum,			/* begin enumeration */
	lynx_image_nextenum,			/* enumerate next */
	lynx_image_closeenum,			/* close enumeration */
	NULL,/* free space on image */
	lynx_image_readfile,			/* read file */
	NULL,/* write file */
	NULL,/* delete file */
	NULL,/* create image */
	NULL,								/* read sector */
	NULL,								/* write sector */
	NULL,								/* file options */
	NULL								/* create options */
)

static int lynx_read_line(lynx_image *image, int pos)
{
	int i;
	for (i=0; (pos+i<image->size)&&(image->data[pos+i]!=0xd); i++ ) ;
	if (pos+i<image->size) i++;
	return i;
}

static int lynx_read_image(lynx_image *image)
{
	int i=0, j, n;

	n=lynx_read_line(image, i);
	if ( n==0 ) return IMGTOOLERR_CORRUPTIMAGE;
	i+=n;

	n=lynx_read_line(image, i);
	j=atoi((char *)image->data+i); /* begin of data */
	if ((n==0)) return IMGTOOLERR_CORRUPTIMAGE;
	i+=n;

	n=lynx_read_line(image, i);
	image->count=atoi((char *)image->data+i);
	if ((n==0)||(image->count==0)) return IMGTOOLERR_CORRUPTIMAGE;
	i+=n;

	image->entries=malloc(sizeof(LYNX_ENTRY)*image->count);
	if (!image->entries) return IMGTOOLERR_OUTOFMEMORY;

	image->entries[0].offset=j*254;

	for (j=0; j<image->count; j++) {
		int a,b;

		n=lynx_read_line(image, i);
		if ((n==0)) return IMGTOOLERR_CORRUPTIMAGE;
		strncpy(image->entries[j].name,(char *)image->data+i, n-1);
		i+=n;

		n=lynx_read_line(image, i);
		a=atoi((char *)image->data+i);
		if ((n==0)) return IMGTOOLERR_CORRUPTIMAGE;
		i+=n;

		n=lynx_read_line(image, i);
		image->entries[j].filetype=image->data[i];
		if ((n==0)) return IMGTOOLERR_CORRUPTIMAGE;
		i+=n;

		n=lynx_read_line(image, i);
		b=atoi((char *)image->data+i);
		if ((n==0)) return IMGTOOLERR_CORRUPTIMAGE;
		i+=n;

		image->entries[j].size=(a-1)*254+b-1;
		if (j+1<image->count)
			image->entries[j+1].offset=image->entries[j].offset+254*a;
	}

	return 0;
}

static int lynx_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg)
{
	lynx_image *image;
	int rc;

	image=*(lynx_image**)outimg=(lynx_image *) malloc(sizeof(lynx_image));
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(lynx_image));
	image->base.module = mod;
	image->size=stream_size(f);
	image->file_handle=f;

	image->data = (unsigned char *) malloc(image->size);
	if ( (!image->data)
		 ||(stream_read(f, image->data, image->size)!=image->size) ) {
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_OUTOFMEMORY;
	}
	if ( (rc=lynx_read_image(image)) ) {
		if (image->entries) free(image->entries);
		free(image);
		*outimg=NULL;
		return rc;
	}

	return 0;
}

static void lynx_image_exit(imgtool_image *img)
{
	lynx_image *image=(lynx_image*)img;
	stream_close(image->file_handle);
	free(image->entries);
	free(image->data);
	free(image);
}

#if 0
static void lynx_image_info(imgtool_image *img, char *string, const int len)
{
	lynx_image *image=(lynx_image*)img;
	char dostext_with_null[33]= { 0 };
	char name_with_null[25]={ 0 };
	strncpy(dostext_with_null, HEADER(image)->dostext, 32);
	strncpy(name_with_null, HEADER(image)->description, 24);
	sprintf(string,"%s\n%s\nversion:%.4x max entries:%d",
			dostext_with_null,
			name_with_null, 
			GET_UWORD(HEADER(image)->version),
			HEADER(image)->max_entries);
}
#endif

static int lynx_image_beginenum(imgtool_image *img, imgtool_directory **outenum)
{
	lynx_image *image=(lynx_image*)img;
	lynx_iterator *iter;

	iter=*(lynx_iterator**)outenum = (lynx_iterator *) malloc(sizeof(lynx_iterator));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;

	iter->image=image;
	iter->index = 0;
	return 0;
}

static int lynx_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent)
{
	lynx_iterator *iter=(lynx_iterator*)enumeration;
	ent->corrupt=0;
	
	ent->eof=iter->index>=iter->image->count;
	if (!ent->eof) {
		strcpy(ent->fname, iter->image->entries[iter->index].name);
		if (ent->attr) {
			switch (iter->image->entries[iter->index].filetype) {
			case 'P': strcpy(ent->attr,"PRG");break;
			case 'S': strcpy(ent->attr,"SEQ");break;
			case 'R': strcpy(ent->attr,"REL");break;
			case 'U': strcpy(ent->attr,"USR");break;
			default: 
				sprintf(ent->attr,"type:%c",
						iter->image->entries[iter->index].filetype );
			}
		}
		ent->filesize=iter->image->entries[iter->index].size;
		iter->index++;
	}
	return 0;
}

static void lynx_image_closeenum(imgtool_directory *enumeration)
{
	free(enumeration);
}

static LYNX_ENTRY* lynx_image_findfile(lynx_image *image, const char *fname)
{
	int i;

	for (i=0; i<image->count; i++) {
		if (!mame_stricmp(fname, image->entries[i].name) ) return image->entries+i;
	}
	return NULL;
}

static int lynx_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf)
{
	lynx_image *image=(lynx_image*)img;
	LYNX_ENTRY *entry;

	if ((entry=lynx_image_findfile(image, fname))==NULL ) return IMGTOOLERR_MODULENOTFOUND;

	if (stream_write(destf, image->data+entry->offset, entry->size)!=entry->size) {
		return IMGTOOLERR_WRITEERROR;
	}

	return 0;
}

