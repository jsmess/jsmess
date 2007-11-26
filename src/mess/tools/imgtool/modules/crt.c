#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "zlib.h"
#include "osdepend.h"
#include "imgtoolx.h"

#ifdef LSB_FIRST
typedef struct { 
	unsigned char high, low;
} biguword;
typedef struct {
	unsigned char highest, high, mid, low;
} bigulong;
#define GET_UWORD(a) (a.low|(a.high<<8))
#define GET_ULONG(a) (a.low|(a.mid<<8)|(a.high<<16)|(a.highest<<24))
#define SET_UWORD(a,v) a.low=(v)&0xff;a.high=((v)>>8)&0xff
#define SET_ULONG(a,v) a.low=(v)&0xff;a.mid=((v)>>8)&0xff;a.high=((v)>>16)&0xff;a.highest=((v)>>24)&0xff
#else
typedef UINT32 bigulong;
typedef UINT16 biguword;
#define GET_UWORD(a) a
#define GET_ULONG(a) a
#define SET_UWORD(a,v) a=v
#define SET_ULONG(a,v) a=v
#endif

/* format description found in the ccs64 emulator

CARTRIDGE FILE FORMAT FOR CCS64 (using fileextension .CRT)

0000 'C64 CARTRIDGE '
0010 ULONG fileheader-length (counted from offset 0000, default=0040)
0014 UWORD Version (=0100)
0016 UWORD Hardware type
0018 UBYTE Exrom line
0019 UBYTE Game line
001A UBYTE[6] For future use...
0020 UBYTE[20] Name (null-terminated string)
0040 Chip Packets 
...

Chip Packets:

0000 'CHIP'
0004 ULONG packetlength (counted from offset 0000)
0008 UWORD chiptype
000A UWORD bank (for normal cartridges = 0)
000C UWORD address
000E UWORD length
0010 UBYTE[] data
...
Hardware Types:

0 - Normal cartridge
1 - Action Replay
2 - KCS Power Cartridge
3 - Final Cartridge III
4 - Simons Basic
5 - Ocean type 1 (256 and 128 Kb)
6 - Expert Cartridge
7 - Fun Play
8 - Super Games
9 - Atomic Power
10 - Epyx Fastload
11 - Westermann


Chip Types:

0 - ROM
1 - RAM, no data field
2 - Flash ROM


All UWORD and ULONG's are in (MSB,LSB) format, i.e. $1234 will be $12,$34 in bytes.

EXAMPLE FOR ACTION REPLAY CARTRIDGE

$0000: 'C64 CARTRIDGE   '
$0010: 00 00 00 40  01 00 00 01  00 00 00 00  00 00 00 00
$0020: 'Action Replay V' 00
$0030: 00 00 ... 00

$0040: 'CHIP'
$0044: 00 00 20 10  00 00 00 00  80 00 20 00
$0050: <data 8192 bytes for bank 0>...

$2050: 'CHIP'
$2054: 00 00 20 10  00 00 00 01  80 00 20 00
$2060: <data 8192 bytes for bank 1>...

$4060: 'CHIP'
$4064: 00 00 20 10  00 00 00 02  80 00 20 00
$4070: <data 8192 bytes for bank 2>...

$6070: 'CHIP'
$6074: 00 00 20 10  00 00 00 03  80 00 20 00
$6080: <data 8192 bytes for bank 3>...

$8080:

 

EXAMPLE FOR KCS POWER CARTRIDGE

$0000: 'C64 CARTRIDGE   '
$0010: 00 00 00 40  01 00 00 02  00 00 00 00  00 00 00 00
$0020: 'KCS Power Cartridge' 00
$0030: 00 00 ... 00

$0040: 'CHIP'
$0044: 00 00 20 10  00 00 00 00  80 00 20 00
$0050: <data 8192 bytes for 8000-9fff>...

$2050: 'CHIP'
$2054: 00 00 20 10  00 00 00 00  A0 00 20 00
$2060: <data 8192 bytes for a000-bfff>...

$4060:

 

EXAMPLE FOR FINAL CARTRIDGE III

$0000: 'C64 CARTRIDGE   '
$0010: 00 00 00 40  01 00 00 03  01 01 00 00  00 00 00 00
$0020: 'Final cartridge' 00
$0030: 00 00 ... 00

$0040: 'CHIP'
$0044: 00 00 40 10  00 00 00 00  80 00 40 00
$0050: <data 16384 bytes for bank 0>...

$4050: 'CHIP'
$4054: 00 00 40 10  00 00 00 01  80 00 40 00
$4060: <data 16384 bytes for bank 1>...

$8060: 'CHIP'
$8064: 00 00 40 10  00 00 00 02  80 00 40 00
$8070: <data 16384 bytes for bank 2>...

$C070: 'CHIP'
$C074: 00 00 40 10  00 00 00 03  80 00 40 00
$C080: <data 16384 bytes for bank 3>...

$10080:

 

EXAMPLE FOR SIMONS BASIC

$0000: 'C64 CARTRIDGE   '
$0010: 00 00 00 40  01 00 00 04  00 01 00 00  00 00 00 00
$0020: 'Simons Basic' 00
$0030: 00 00 ... 00

$0040: 'CHIP'
$0044: 00 00 20 10  00 00 00 00  80 00 20 00
$0050: <data 8192 bytes for 8000-9fff>...

$2050: 'CHIP'
$2054: 00 00 20 10  00 00 00 00  A0 00 20 00
$2060: <data 8192 bytes for a000-bfff>...

$4060:

 

EXAMPLE FOR OCEAN TYPE1

$0000: 'C64 CARTRIDGE   '
$0010: 00 00 00 40  01 00 00 05  00 00 00 00  00 00 00 00
$0020: 'Robocop2' 00
$0030: 00 00 ... 00

$0040: 'CHIP'
$0044: 00 00 20 10  00 00 00 00  80 00 20 00
$0050: <data 8192 bytes for 8000-9fff, bank 0>...

$2050: 'CHIP'
$2054: 00 00 20 10  00 00 00 01  80 00 20 00
$2060: <data 8192 bytes for 8000-9fff, bank 1>...
...
$20140: 'CHIP'
$20144: 00 00 20 10  00 00 00 10  A0 00 20 00
$20150: <data 8192 bytes for a000-bfff, bank 16>...

$22150: 'CHIP'
$22154: 00 00 20 10  00 00 00 11  A0 00 20 00
$22160: <data 8192 bytes for a000-bfff, bank 17>...
...
$40240:

 

EXAMPLE FOR FUN PLAY TYPE

$0000: 'C64 CARTRIDGE '
$0010: 00 00 00 40 01 00 00 07 00 00 00 00 00 00 00 00
$0020: 'FUN PLAY' 00 00 00 00 00 00 00 00
$0030: 00 00 ... 00

$0040: 'CHIP'
$0044: 00 00 20 10 00 00 00 00 80 00 20 00
$0050: <data 8192 bytes for 8000-9fff, bank 0>...

$2050: 'CHIP'
$2054: 00 00 20 10 00 00 00 08 80 00 20 00
$2060: <data 8192 bytes for 8000-9fff, bank 1>...

$4060: 'CHIP'
$2054: 00 00 20 10 00 00 00 10 80 00 20 00
$2060: <data 8192 bytes for 8000-9fff, bank 2>...
...

$1E130: 'CHIP'
$1E134: 00 00 20 10 00 00 00 39 80 00 20 00
$1E140: <data 8192 bytes for 8000-9fff, bank 15>...

$20140:

 

EXAMPLE FOR SUPER GAMES TYPE

$0000: 'C64 CARTRIDGE '
$0010: 00 00 00 40 01 00 00 08 00 00 00 00 00 00 00 00
$0020: 'SUPER GAMES' 00 00 00 00 00
$0030: 00 00 ... 00

$0040: 'CHIP'
$0044: 00 00 40 10 00 00 00 00 80 00 40 00
$0050: <data 16384 bytes for 8000-bfff, bank 0>...

$4050: 'CHIP'
$4054: 00 00 40 10 00 00 00 01 80 00 40 00
$4060: <data 16384 bytes for 8000-bfff, bank 0>...

$8060: 'CHIP'
$8064: 00 00 40 10 00 00 00 02 80 00 40 00
$8070: <data 16384 bytes for 8000-bfff, bank 0>...

$C070: 'CHIP'
$C074: 00 00 40 10 00 00 00 03 80 00 40 00
$C080: <data 16384 bytes for 8000-bfff, bank 0>...

$10080:

 

EXAMPLE FOR ATOMIC POWER CARTRIDGE

$0000: 'C64 CARTRIDGE   '
$0010: 00 00 00 40  01 00 00 09  00 00 00 00  00 00 00 00
$0020: 'Atomic Power' 00
$0030: 00 00 ... 00

$0040: 'CHIP'
$0044: 00 00 20 10  00 00 00 00  80 00 20 00
$0050: <data 8192 bytes for bank 0>...

$2050: 'CHIP'
$2054: 00 00 20 10  00 00 00 01  80 00 20 00
$2060: <data 8192 bytes for bank 1>...

$4060: 'CHIP'
$4064: 00 00 20 10  00 00 00 02  80 00 20 00
$4070: <data 8192 bytes for bank 2>...

$6070: 'CHIP'
$6074: 00 00 20 10  00 00 00 03  80 00 20 00
$6080: <data 8192 bytes for bank 3>...

$8080:

 

EXAMPLE FOR EPYX FASTLOAD TYPE

$0000: 'C64 CARTRIDGE '
$0010: 00 00 00 40 01 00 00 0A 01 01 00 00 00 00 00 00
$0020: 'EPYX FASTLOAD' 00 00 00
$0030: 00 00 ... 00

$0040: 'CHIP'
$0044: 00 00 20 10 00 00 00 00 80 00 20 00
$0050: <data 8192 bytes for 8000-9fff>

$2050:

 

EXAMPLE FOR WESTERMANN TYPE

$0000: 'C64 CARTRIDGE '
$0010: 00 00 00 40 01 00 00 0A 00 01 00 00 00 00 00 00
$0020: 'Westermann' 00 00 00
$0030: 00 00 ... 00

$0040: 'CHIP'
$0044: 00 00 40 10 00 00 00 00 80 00 40 00
$0050: <data 16384 bytes for 8000-bfff>

$4050:
*/
 
static const char *hardware_types[]={
	"Normal cartridge",
	"Action Replay",
	"KCS Power Cartridge",
	"Final Cartridge III",
	"Simons Basic",
	"Ocean type 1 (256 and 128 Kb)",
	"Expert Cartridge",
	"Fun Play",
	"Super Games",
	"Atomic Power",
	"Epyx Fastload",
	"Westermann"
};
typedef struct{
	char id[0x10]; // C64 CARTRIDGE
	bigulong length;
	biguword version;
	biguword hardware_type;
	unsigned char exrom_line;
	unsigned char game_line;
	unsigned char reserved[6];
	char name[0x20]; 
} crt_header;
	
static const char *chip_types[]={ "ROM", "RAM", "FLASH" };
typedef struct {
	char id[4]; // CHIP
	bigulong packet_length;
	biguword chip_type;
	biguword bank;
	biguword address;
	biguword length;
} crt_packet;

typedef struct {
	imgtool_image base;
	imgtool_stream *file_handle;
	int size;
	int modified;
	unsigned char *data;
} crt_image;

#define HEADER(image) ((crt_header*)image->data)
#define PACKET(image, pos) ((crt_packet*)(image->data+pos))

typedef struct {
	imgtool_directory base;
	crt_image *image;
	int pos;
	int number;
} crt_iterator;

static int crt_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg);
static void crt_image_exit(imgtool_image *img);
static void crt_image_info(imgtool_image *img, char *string, const int len);
static int crt_image_beginenum(imgtool_image *img, imgtool_directory **outenum);
static int crt_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent);
static void crt_image_closeenum(imgtool_directory *enumeration);
//static size_t crt_image_freespace(imgtool_image *img);
static int crt_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf);
static int crt_image_writefile(imgtool_image *img, const char *fname, imgtool_stream *sourcef, const ResolvedOption *_options);
static int crt_image_deletefile(imgtool_image *img, const char *fname);
static int crt_image_create(const imgtool_module *mod, imgtool_stream *f, const ResolvedOption *_options);

/*
	IMAGE_USES_FTYPE|IMAGE_USES_FADDR|IMAGE_USES_FBANK
	|IMAGE_USES_HARDWARE_TYPE|IMAGE_USES_GAME_LINE
	|IMAGE_USES_EXROM_LINE|IMAGE_USES_LABEL, //flags
*/

static struct OptionTemplate c64crt_fileeopts[] =
{
	{ "ftype",	NULL, IMGOPTION_FLAG_TYPE_INTEGER,	0,		0xffff,	NULL	},	/* [0] */
	{ "faddr",	NULL, IMGOPTION_FLAG_TYPE_INTEGER,	0,		0xffff,	NULL	},	/* [1] */
	{ "fbank",	NULL, IMGOPTION_FLAG_TYPE_INTEGER,	0,		0xffff,	NULL	},	/* [2] */
	{ NULL, NULL, 0, 0, 0, 0 }
};

#define C64CRT_FILEOPTION_FTYPE		0
#define C64CRT_FILEOPTION_FADDR		1
#define C64CRT_FILEOPTION_FBANK		2

static struct OptionTemplate c64crt_createopts[] =
{
	{ "hardwaretype",	NULL, IMGOPTION_FLAG_TYPE_INTEGER,							0,		0xffff,	NULL	},	/* [0] */
	{ "gameline",		NULL, IMGOPTION_FLAG_TYPE_INTEGER,							0,		0x00ff,	NULL	},	/* [1] */
	{ "exromline",		NULL, IMGOPTION_FLAG_TYPE_INTEGER,							0,		0x00ff,	NULL	},	/* [2] */
	{ "label",			NULL, IMGOPTION_FLAG_TYPE_STRING | IMGOPTION_FLAG_HASDEFAULT,	0,		0,		NULL	},	/* [3] */
	{ NULL, NULL, 0, 0, 0, 0 }
};

#define C64CRT_CREATEOPTION_HARDWARETYPE	0
#define C64CRT_CREATEOPTION_GAMELINE		1
#define C64CRT_CREATEOPTION_EXROMLINE		2
#define C64CRT_CREATEOPTION_LABEL			3

IMAGEMODULE(
	c64crt,
	"Commodore 64 Cartridge",	/* human readable name */
	"crt",								/* file extension */
	"c64.crc",								/* crcfile */
	"c64",								/* crc system name */
	NULL,								/* eoln */
	0,									/* flags */
	crt_image_init,				/* init function */
	crt_image_exit,				/* exit function */
	crt_image_info, /* info function */
	crt_image_beginenum,			/* begin enumeration */
	crt_image_nextenum,			/* enumerate next */
	crt_image_closeenum,			/* close enumeration */
	NULL, //crt_image_freespace,			/* free space on image */
	crt_image_readfile,			/* read file */
	crt_image_writefile,			/* write file */
	crt_image_deletefile,			/* delete file */
	crt_image_create,				/* create image */
	NULL,								/* read sector */
	NULL,								/* write sector */
	c64crt_fileeopts,					/* file _options */
	c64crt_createopts					/* create options */
)

static int crt_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg)
{
	crt_image *image;

	image=*(crt_image**)outimg=(crt_image *) malloc(sizeof(crt_image));
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(crt_image));
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

static void crt_image_exit(imgtool_image *img)
{
	crt_image *image=(crt_image*)img;
	if (image->modified) {
		stream_clear(image->file_handle);
		stream_write(image->file_handle, image->data, image->size);
	}
	stream_close(image->file_handle);
	free(image->data);
	free(image);
}

static void crt_image_info(imgtool_image *img, char *string, const int len)
{
	crt_image *image=(crt_image*)img;
	sprintf(string, "%-32s\nversion:%.4x type:%d:%s exrom:%d game:%d",
			HEADER(image)->name,
			GET_UWORD(HEADER(image)->version),
			GET_UWORD(HEADER(image)->hardware_type),
			hardware_types[GET_UWORD(HEADER(image)->hardware_type)],
			HEADER(image)->exrom_line, HEADER(image)->game_line);
	return;
}

static int crt_image_beginenum(imgtool_image *img, imgtool_directory **outenum)
{
	crt_image *image=(crt_image*)img;
	crt_iterator *iter;

	iter=*(crt_iterator**)outenum = (crt_iterator *) malloc(sizeof(crt_iterator));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;

	iter->image=image;
	iter->pos = GET_ULONG( HEADER(image)->length );
	iter->number = 0;
	return 0;
}

static int crt_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent)
{
	crt_iterator *iter=(crt_iterator*)enumeration;

	ent->corrupt=0;
	
	if (!(ent->eof=(iter->pos>=iter->image->size))) {
		sprintf(ent->fname,"%d", iter->number);
		ent->filesize=GET_UWORD( PACKET(iter->image, iter->pos)->length );
		if (ent->attr) {
		    unsigned crc=crc32(0, iter->image->data+iter->pos+sizeof(crt_packet), ent->filesize);
			sprintf(ent->attr,"%-4s %s bank:%-2d addr:%.4x crc:%8x",
					(char*)PACKET(iter->image, iter->pos),
					chip_types[GET_UWORD(PACKET(iter->image,iter->pos)->chip_type)],
					GET_UWORD( PACKET(iter->image,iter->pos)->bank),
					GET_UWORD( PACKET(iter->image,iter->pos)->address),
				crc);
		}
		iter->number++;

		iter->pos+=GET_ULONG( PACKET(iter->image, iter->pos)->packet_length );
	}
	return 0;
}

static void crt_image_closeenum(imgtool_directory *enumeration)
{
	free(enumeration);
}

#if 0
static size_t crt_image_freespace(imgtool_image *img)
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

static int crt_image_findfile(crt_image *image, const char *fname)
{
	int nr=0;
	char name[6];
	int i=sizeof(crt_header);

	while (i<image->size) {
		sprintf(name, "%d",nr);
		if (!strcmp(fname, name) ) return i;
		i+=GET_ULONG( PACKET(image, i)->packet_length );
		nr++;
	}
	return 0;
}

static int crt_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf)
{
	crt_image *image=(crt_image*)img;
	int size;
	int pos;

	if (!(pos=crt_image_findfile(image, fname)) ) 
		return IMGTOOLERR_MODULENOTFOUND;

	size=GET_UWORD( PACKET(image, pos)->length );
	if (stream_write(destf, image->data+pos+sizeof(crt_packet), size)!=size) {
		return IMGTOOLERR_WRITEERROR;
	}

	return 0;
}

static int crt_image_writefile(imgtool_image *img, const char *fname, imgtool_stream *sourcef, 
							   const ResolvedOption *_options)
{
	crt_image *image=(crt_image*)img;
	int size;
	int pos;

	size=stream_size(sourcef);
	if (!(pos=crt_image_findfile(image, fname)) ) {
		// appending
		pos=image->size;
		if (!(image->data=realloc(image->data, image->size+size+sizeof(crt_packet))) )
			return IMGTOOLERR_OUTOFMEMORY;
		image->size+=size+sizeof(crt_packet);
	} else {
		int oldsize=GET_ULONG(PACKET(image,pos)->packet_length);
		// overwritting
		if (!(image->data=realloc(image->data, image->size+size+sizeof(crt_packet)-oldsize) ) )
			return IMGTOOLERR_OUTOFMEMORY;
		if (image->size-pos-oldsize!=0) {
			memmove(image->data+pos+size+sizeof(crt_packet), image->data+pos+oldsize, 
					image->size-pos-oldsize);
		}
		image->size+=size+sizeof(crt_packet)-oldsize;
	}
	if (stream_read(sourcef, image->data+pos+sizeof(crt_packet), size)!=size) {
		return IMGTOOLERR_READERROR;
	}
	memset(image->data+pos, 0, sizeof(crt_packet));
	memcpy(PACKET(image,pos)->id,"CHIP",4);
	SET_ULONG( PACKET(image, pos)->packet_length, size+sizeof(crt_packet));
	SET_UWORD(PACKET(image, pos)->chip_type, _options[C64CRT_FILEOPTION_FTYPE].i);
	SET_UWORD( PACKET(image, pos)->address, _options[C64CRT_FILEOPTION_FADDR].i);
	SET_UWORD( PACKET(image, pos)->bank, _options[C64CRT_FILEOPTION_FBANK].i);
	SET_UWORD( PACKET(image, pos)->length, size);

	image->modified=1;

	return 0;
}

static int crt_image_deletefile(imgtool_image *img, const char *fname)
{
	crt_image *image=(crt_image*)img;
	int size;
	int pos;

	if (!(pos=crt_image_findfile(image, fname)) ) {
		return IMGTOOLERR_MODULENOTFOUND;
	}
	size=GET_ULONG(PACKET(image, pos)->packet_length);
	if (image->size-pos-size>0)
		memmove(image->data+pos, image->data+pos+size, image->size-pos-size);
	image->size-=size;
	image->modified=1;

	return 0;
}

static int crt_image_create(const imgtool_module *mod, imgtool_stream *f, const ResolvedOption *_options)
{
	crt_header header={ "C64 CARTRIDGE   " };
	SET_ULONG(header.length, sizeof(header));
	SET_UWORD(header.version, 0x100);
	SET_UWORD(header.hardware_type, _options[C64CRT_CREATEOPTION_HARDWARETYPE].i);
	header.game_line=_options[C64CRT_CREATEOPTION_GAMELINE].i;
	header.exrom_line=_options[C64CRT_CREATEOPTION_EXROMLINE].i;
	if (_options[C64CRT_CREATEOPTION_LABEL].s) strcpy(header.name, _options[C64CRT_CREATEOPTION_LABEL].s);
	return (stream_write(f, &header, sizeof(crt_header)) == sizeof(crt_header)) 
		? 0 : IMGTOOLERR_WRITEERROR;
}

