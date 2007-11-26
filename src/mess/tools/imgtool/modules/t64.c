#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osdepend.h"
#include "imgtoolx.h"

#ifdef LSB_FIRST
typedef UINT16 littleuword;
typedef UINT32 littleulong;
#define GET_UWORD(a) a
#define GET_ULONG(a) a
#define SET_UWORD(a,v) a=v
#define SET_ULONG(a,v) a=v
#else
typedef struct { 
	unsigned char low, high;
} littleuword;
typedef struct { 
	unsigned char low, mid, high, highest;
} littleulong;
#define GET_UWORD(a) (a.low|(a.high<<8))
#define GET_ULONG(a) (a.low|(a.mid<<8)|(a.high<<16)|(a.highest<<24))
#define SET_UWORD(a,v) a.low=(v)&0xff;a.high=((v)>>8)&0xff
#define SET_ULONG(a,v) a.low=(v)&0xff;a.mid=((v)>>8)&0xff;a.high=((v)>>16)&0xff;a.highest=((v)>>24)&0xff
#endif

/*
T64 File Structure was developed by Miha Peternel for use in C64S.
It is easy to use and allows future extensions.

______________________
  T64 file structure

  Offset, size, description
       0, 64    tape record
      64, 32*n  file records for n directory entries
 64+32*n, ???   binary contents of the files

_______________
  Tape Record

  Offset, size, description
       0, 32    DOS tape description + EOF (for type)
      32, 2     tape version ($0101)
      34, 2     number of directory entries
      36, 2     number of used entries (can be 0 in my loader)
      38, 2     free
      40, 24    user description as displayed in tape menu

_______________
  File Record

  Offset, size, description
       0, 1     entry type
                0 = free entry
                1 = normal tape file
                3 = memory snapshot v0.9, uncompressed
                2..255 reserved (for memory snapshots...)
       1, 1     C64 file type
       2, 2     start address
       4, 2     end address
       6, 2     free
       8, 4     offset of file contents start within T64 file
      12, 4     free
      16, 16    C64 file name

*/
 
typedef struct{
	char dostext[0x20]; // dos text
	littleuword version; // packed bcd
	littleuword max_entries;
	littleuword used_entries;
	littleuword reserved;
	char description[24];
} t64_header;
	
typedef struct {
	unsigned char type;
	unsigned char file_type;
	littleuword start_address;
	littleuword end_address;
	littleuword reserved;
	littleulong offset;
	littleulong reserved2;
	char name[0x10];
} t64_entry;

typedef struct {
	imgtool_image base;
	imgtool_stream *file_handle;
	int size;
	int modified;
	unsigned char *data;
} t64_image;

#define HEADER(image) ((t64_header*)image->data)
#define ENTRY(image, index) ((t64_entry*)(image->data+sizeof(t64_header))+index)

typedef struct {
	imgtool_directory base;
	t64_image *image;
	int index;
} t64_iterator;

static int t64_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg);
static void t64_image_exit(imgtool_image *img);
static void t64_image_info(imgtool_image *img, char *string, const int len);
static int t64_image_beginenum(imgtool_image *img, imgtool_directory **outenum);
static int t64_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent);
static void t64_image_closeenum(imgtool_directory *enumeration);
//static size_t t64_image_freespace(imgtool_image *img);
static int t64_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf);
static int t64_image_writefile(imgtool_image *img, const char *fname, imgtool_stream *sourcef, const ResolvedOption *_options);
static int t64_image_deletefile(imgtool_image *img, const char *fname);
static int t64_image_create(const imgtool_module *mod, imgtool_stream *f, const ResolvedOption *_options);

static struct OptionTemplate t64_createopts[] =
{
	{ "entries", NULL, IMGOPTION_FLAG_TYPE_INTEGER,							0,		3,		NULL	},	/* [0] */
	{ "label", NULL, IMGOPTION_FLAG_TYPE_STRING | IMGOPTION_FLAG_HASDEFAULT,	0,		0,		NULL	},	/* [1] */
	{ NULL, NULL, 0, 0, 0, 0 }
};

#define T64_OPTION_ENTRIES	0
#define T64_OPTION_LABEL	1

IMAGEMODULE(
	t64,
	"Commodore 64 Archive for Tapes",	/* human readable name */
	"t64",								/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	EOLN_CR,							/* eoln */
	0,									/* flags */
	t64_image_init,						/* init function */
	t64_image_exit,						/* exit function */
	t64_image_info,						/* info function */
	t64_image_beginenum,				/* begin enumeration */
	t64_image_nextenum,					/* enumerate next */
	t64_image_closeenum,				/* close enumeration */
	NULL, //t64_image_freespace,		/* free space on image */
	t64_image_readfile,					/* read file */
	t64_image_writefile,				/* write file */
	t64_image_deletefile,				/* delete file */
	t64_image_create,					/* create image */
	NULL,								/* read sector */
	NULL,								/* write sector */
	NULL,								/* file _options */
	t64_createopts						/* create options */
)

static int t64_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg)
{
	t64_image *image;

	image=*(t64_image**)outimg=(t64_image *) malloc(sizeof(t64_image));
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(t64_image));
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

	return 0;
}

static void t64_image_exit(imgtool_image *img)
{
	t64_image *image=(t64_image*)img;
	if (image->modified) {
		stream_clear(image->file_handle);
		stream_write(image->file_handle, image->data, image->size);
	}
	stream_close(image->file_handle);
	free(image->data);
	free(image);
}

static void t64_image_info(imgtool_image *img, char *string, const int len)
{
	t64_image *image=(t64_image*)img;
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

static int t64_image_beginenum(imgtool_image *img, imgtool_directory **outenum)
{
	t64_image *image=(t64_image*)img;
	t64_iterator *iter;

	iter=*(t64_iterator**)outenum = (t64_iterator *) malloc(sizeof(t64_iterator));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;

	iter->image=image;
	iter->index = 0;
	return 0;
}

static int t64_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent)
{
	t64_iterator *iter=(t64_iterator*)enumeration;
	ent->corrupt=0;
	
	for (;!(ent->eof=(iter->index>=GET_UWORD(HEADER(iter->image)->max_entries)));iter->index++ ){
		if (ENTRY(iter->image, iter->index)->type==0) continue;
		memset(ent->fname,0,17);
		strncpy(ent->fname, ENTRY(iter->image, iter->index)->name, 16);
		if (ent->attr)
			sprintf(ent->attr,"start:%.4x end:%.4x type:%d file:%d",
					GET_UWORD( ENTRY(iter->image,iter->index)->start_address),
					GET_UWORD( ENTRY(iter->image,iter->index)->end_address),
					ENTRY(iter->image,iter->index)->type,
					ENTRY(iter->image,iter->index)->file_type );
		ent->filesize=GET_UWORD( ENTRY(iter->image, iter->index)->end_address )
			-GET_UWORD( ENTRY(iter->image, iter->index)->start_address );
		iter->index++;
		break;
	}
	return 0;
}

static void t64_image_closeenum(imgtool_directory *enumeration)
{
	free(enumeration);
}

#if 0
static size_t t64_image_freespace(imgtool_image *img)
{
	int i;
	rsdos_diskimage *rsimg = (rsdos_diskimage *) img;
	size_t s = 0;

	for (i = 0; i < GRANULE_COUNT; i++)
		if (rsimg->granulemap[i] == 0xff)
			s += (9 * 256);
	return s;
}
#endif

static int t64_image_findfile(t64_image *image, const char *fname)
{
	int i=0;

	for (i=0; i<GET_UWORD(HEADER(image)->max_entries); i++) {
		if (ENTRY(image, i)->type==0) continue;
		if (!strnicmp(fname, ENTRY(image,i)->name, strlen(fname)) ) return i;
	}
	return -1;
}

static int t64_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf)
{
	t64_image *image=(t64_image*)img;
	int size;
	int ind;

	if ((ind=t64_image_findfile(image, fname))==-1 ) return IMGTOOLERR_MODULENOTFOUND;

	size=GET_UWORD( ENTRY(image, ind)->end_address )-GET_UWORD( ENTRY(image, ind)->start_address );

	if (stream_write(destf, &ENTRY(image, ind)->start_address, 2)!=2) {
		return IMGTOOLERR_WRITEERROR;
	}
	if (stream_write(destf, image->data+GET_ULONG(ENTRY(image, ind)->offset), size)!=size) {
		return IMGTOOLERR_WRITEERROR;
	}

	return 0;
}

static int t64_image_writefile(imgtool_image *img, const char *fname, imgtool_stream *sourcef, const ResolvedOption *_options)
{
	t64_image *image=(t64_image*)img;
	int size, fsize;
	int ind;
	int pos, i, t;

	fsize=stream_size(sourcef);
	if ((ind=t64_image_findfile(image, fname))==-1 ) {
		// appending
		for (ind=0; ind<GET_UWORD(HEADER(image)->max_entries)&&(ENTRY(image,ind)->type!=0); ind++) ;
		if (ind>=GET_UWORD(HEADER(image)->max_entries)) return IMGTOOLERR_NOSPACE;
		pos=image->size;
		if (!(image->data=realloc(image->data, image->size+fsize-2)) )
			return IMGTOOLERR_OUTOFMEMORY;
		image->size+=fsize-2;
#ifdef LSB_FIRST
		HEADER(image)->used_entries++;
#else
		if (++HEADER(image)->used_entries.low == 0)
			HEADER(image)->used_entries.high++;
#endif
	} else {
		pos=GET_ULONG(ENTRY(image,ind)->offset);
		// find the size of the data in this area
		for (size=image->size-pos, i=0; i<GET_UWORD(HEADER(image)->max_entries); i++) {
			if (ENTRY(image,i)->type==0) continue;
			if (GET_UWORD(ENTRY(image,i)->offset)<pos) continue;
			if (GET_ULONG(ENTRY(image,i)->offset)-pos>=size) continue;
			size=GET_ULONG(ENTRY(image, i)->offset)-pos;
		}
		if ((size!=0)&&(image->size-pos-size!=0)) 
			memmove(image->data+pos, image->data+pos+size, image->size-pos-size);
		// correct offset positions in other entries
		for (i=0;i<GET_UWORD(HEADER(image)->max_entries); i++) {
			if (i==ind) continue;
			if (ENTRY(image,i)->type==0) continue;
			if (GET_ULONG(ENTRY(image,i)->offset)<pos) continue;
			t=GET_ULONG(ENTRY(image,i)->offset)-size;
			SET_ULONG( ENTRY(image,i)->offset, t);
		}
		// overwritting
		if (!(image->data=realloc(image->data, image->size+fsize-2-size)))
			return IMGTOOLERR_OUTOFMEMORY;
		pos=image->size;
		image->size=image->size+fsize-2-size;
	}
	memset(ENTRY(image,ind), 0, sizeof(t64_entry));
	if (stream_read(sourcef, &ENTRY(image,ind)->start_address, 2)!=2) {
		return IMGTOOLERR_READERROR;
	}
	if (stream_read(sourcef, image->data+pos, fsize-2)!=fsize-2) {
		return IMGTOOLERR_READERROR;
	}
	SET_UWORD( ENTRY(image, ind)->end_address,
			   GET_UWORD( ENTRY(image, ind)->start_address)+fsize-2);
	SET_ULONG( ENTRY(image, ind)->offset, pos);
	strncpy(ENTRY(image, ind)->name, fname, 16);
	ENTRY(image,ind)->type=1; // normal file
	ENTRY(image,ind)->file_type=1; // prg
	image->modified=1;

	return 0;
}

static int t64_image_deletefile(imgtool_image *img, const char *fname)
{
	t64_image *image=(t64_image*)img;
	int pos, size;
	int ind, i;

	if ((ind=t64_image_findfile(image, fname))==-1 ) {
		return IMGTOOLERR_MODULENOTFOUND;
	}
	pos=GET_ULONG(ENTRY(image,ind)->offset);
	// find the size of the data in this area
	for (size=image->size-pos, i=0; i<GET_UWORD(HEADER(image)->max_entries); i++) {
		if (ENTRY(image,i)->type==0) continue;
		if (GET_UWORD(ENTRY(image,i)->offset)<pos) continue;
		if (GET_ULONG(ENTRY(image,i)->offset)-pos>=size) continue;
		size=GET_ULONG(ENTRY(image, i)->offset)-pos;
	}
	if ((size!=0)&&(image->size-pos-size!=0)) 
		memmove(image->data+pos, image->data+pos+size, image->size-pos-size);
	// correct offset positions in other entries
	for (i=0;i<GET_UWORD(HEADER(image)->max_entries); i++) {
		if (i==ind) continue;
		if (ENTRY(image,i)->type==0) continue;
		if (GET_ULONG(ENTRY(image,i)->offset)<pos) continue;
		SET_ULONG( ENTRY(image,i)->offset, GET_ULONG(ENTRY(image,i)->offset)-size );
	}
	image->size-=size;
	ENTRY(image,ind)->type=0; // normal file
	image->modified=1;
#ifdef LSB_FIRST
	HEADER(image)->used_entries--;
#else
		if (HEADER(image)->used_entries.low-- == 0)
			HEADER(image)->used_entries.high--;
#endif

	return 0;
}

static int t64_image_create(const imgtool_module *mod, imgtool_stream *f, const ResolvedOption *_options)
{
	int entries;
	t64_header header={ "T64 Tape archiv created by MESS\x1a" };
	t64_entry entry= { 0 };
	int i;

	entries = _options[T64_OPTION_ENTRIES].i;
	if (entries==0) entries=10;
	SET_UWORD(header.version, 0x0101);
	SET_UWORD(header.max_entries, entries);
	if (_options[T64_OPTION_LABEL].s) strcpy(header.description, _options[T64_OPTION_LABEL].s);
	if (stream_write(f, &header, sizeof(t64_header)) != sizeof(t64_header)) 
		return  IMGTOOLERR_WRITEERROR;
	for (i=0; i<entries; i++) {
	if (stream_write(f, &entry, sizeof(t64_entry)) != sizeof(t64_entry)) 
		return  IMGTOOLERR_WRITEERROR;
	}

	return 0;
}

