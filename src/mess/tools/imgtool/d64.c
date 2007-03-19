#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "osdepend.h"
#include "imgtoolx.h"
#include "d64.h"

#ifdef LSB_FIRST
typedef UINT16 littleuword;
#define GET_UWORD(a) a
#define SET_UWORD(a,v) a=v
#else
typedef struct { 
	unsigned char low, high;
} littleuword;
#define GET_UWORD(a) (a.low|(a.high<<8))
#define SET_UWORD(a,v) a.low=(v)&0xff;a.high=((v)>>8)&0xff
#endif

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

// x64 d64 image with 64 byte header

#define D64_MAX_TRACKS 40

static int d64_sectors_per_track[] =
{
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
	19, 19, 19, 19, 19, 19, 19,
	18, 18, 18, 18, 18, 18,
	17, 17, 17, 17, 17,
	17, 17, 17, 17, 17 // non standard
};

static int d64_offset[D64_MAX_TRACKS+1];		   /* offset of begin of track in d64 file */

typedef struct {
	unsigned char track, sector, // of next directory
		format, doublesided;
	struct { unsigned char free, map[3]; } bam[35];
	unsigned char name[0x10], filler[2], id[2], filler2, version[2], filler3[4];
	unsigned char unused[50]; // zero
	unsigned char free2[35]; //freecount for 1751 tracks 36..70
} D64_HEADER;

typedef struct {
	unsigned char unused[0x6a]; // zero
	struct { unsigned char map2[3]; } bam2[35];
	unsigned char unused2[0x2d]; // zer0
} D71_HEADER;

static D64_HEADER d64_header={ 
	18,1,'A', 
	0, // 1571 0x80
	{ { 0 } },
	{ '\xa0','\xa0','\xa0','\xa0','\xa0','\xa0','\xa0','\xa0',
	  '\xa0','\xa0','\xa0','\xa0','\xa0','\xa0','\xa0','\xa0' }, 
	{ 0xa0, 0xa0 }, { 0xa0, 0xa0 } , 0xa0, { '2', 'A' } , { '\xa0','\xa0','\xa0','\xa0' },
};

typedef struct {
	unsigned char not_usable[2];
	unsigned char type;
#define FILE_TYPE_DEL 0
#define FILE_TYPE_SEQ 1
#define FILE_TYPE_PRG 2
#define FILE_TYPE_USR 3
#define FILE_TYPE_REL 4
#define FILE_TYPE_MASK 0x3f
	unsigned char track, sector, name[0x10];
	unsigned char rel_track, rel_sector, rel_length;
	unsigned char unused[4];
	unsigned char replace_track, replace_sector;
	littleuword blocks;
} D64_ENTRY;

typedef union {
	struct {
		unsigned char track, sector; //linkage to next track=0 for last
	} linkage;
	D64_ENTRY entry[8];
} D64_DIRECTORY;

typedef struct {
	unsigned char track, sector, // of next directory
		format, unused;
	unsigned char name[0x12], id[2], filler2, version[2], filler3[2];
	unsigned char unused1[227]; // zero
} D81_HEADER;

static D81_HEADER d81_header={ 
	40,3,'D', 
	0,
	{ '\xa0','\xa0','\xa0','\xa0','\xa0','\xa0','\xa0','\xa0',
	  '\xa0','\xa0','\xa0','\xa0','\xa0','\xa0','\xa0','\xa0', 0xa0, 0xa0 }, 
	{ 0xa0, 0xa0 } , 0xa0, { '3', 'D' } , { '\xa0','\xa0' },
};

typedef struct {
	unsigned char track, sector;
	unsigned char version, complement;
	unsigned char id[2], iobyte, autoloader;
	unsigned char reserved[8];
	struct { unsigned char free, map[5]; } bam[40];
} D81_BAM;

static D81_BAM d81_bam={
	40, 2,0x44, 0xbb,
};

#define D71_MAX_TRACKS 70

int d71_sectors_per_track[] =
{
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
	19, 19, 19, 19, 19, 19, 19,
	18, 18, 18, 18, 18, 18,
	17, 17, 17, 17, 17,
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
	19, 19, 19, 19, 19, 19, 19,
	18, 18, 18, 18, 18, 18,
	17, 17, 17, 17, 17
};

static int d71_offset[D71_MAX_TRACKS+1];		   /* offset of begin of track in d64 file */

/* must be called before other functions */
void d64_open_helper (void)
{
	int i;

	d64_offset[0] = 0;
	for (i = 1; i <= D64_MAX_TRACKS; i++)
		d64_offset[i] = d64_offset[i - 1] + d64_sectors_per_track[i - 1] * 256;

	d71_offset[0] = 0;
	for (i = 1; i <= D71_MAX_TRACKS; i++)
		d71_offset[i] = d71_offset[i - 1] + d71_sectors_per_track[i - 1] * 256;
}

/* calculates offset to beginning of d64 file for sector beginning */
static int d64_tracksector2offset (int track, int sector)
{
	return d64_offset[track - 1] + sector * 256;
}

/* calculates offset to beginning of d71 file for sector beginning */
int d71_tracksector2offset (int track, int sector)
{
	return d71_offset[track - 1] + sector * 256;
}

/* calculates offset to beginning of d71 file for sector beginning */
int d81_tracksector2offset (int track, int sector)
{
	return ((track - 1)*20*2 + sector )* 256;
}

static void cbm_filename_to_cstring(unsigned char *cbmstring, int size, unsigned char* cstring)
{
	int i;
	for (i=0; (i<size)&&(cbmstring[i]!=0xa0); i++) cstring[i]=cbmstring[i];
	cstring[i]=0;
}

int cbm_compareNames (const unsigned char *left, const unsigned char *right)
{
	int i;

	for (i = 0; i < 16; i++)
	{
		if ((left[i] == '*') || (right[i] == '*'))
			return 1;

		if (left[i] == right[i]) {
			if (left[i]==0) return 1;
			continue;
		}
		if ((left[i] == 0xa0) && (right[i] == 0))
			return 1;
		if ((right[i] == 0xa0) && (left[i] == 0))
			return 1;
		return 0;
	}
	return 1;
}

typedef struct _d64_image {
	imgtool_image base;
	imgtool_stream *file_handle;
	int d64;
	int x64; // x64 header at the beginning
	int d71; // c128d/1571 diskette
	int d81; // c65/1581/1565 diskette

	int (*get_offset)(int track, int sector);
	void (*alloc_sector)(struct _d64_image *image, int *track, int *sector);
	void (*free_sector)(struct _d64_image *image, int track, int sector);
	struct { int track, sector; } directory;
	int bam_bytes_dir_track;

	int tracks; //35 or 40 tracks
	int crc; // crc data at the end of file
	int size;
	int modified;
	unsigned char *realimage; //inclusive x64 header
	unsigned char *data; // pointer to start of d64 image
} d64_image;

typedef struct {
	imgtool_directory base;
	d64_image *image;
	int track, sector;
	int offset;
} d64_iterator;

static void d64_alloc_sector(struct _d64_image *image, int *track, int *sector)
{
	static int tracks[35];
	int i, j;
	D64_HEADER *header=(D64_HEADER*)
		(image->data+image->get_offset(image->directory.track,image->directory.sector));

	// this contains the tracks for the search strategy for free sectors
	for (i=0,j=17; i<35; i+=2,j--) tracks[i]=j;
	for (i=1,j=19; i<35; i+=2,j++) tracks[i]=j;
	tracks[35-1]=18;

	for (i=0; i<35; i++) {
		*track=tracks[i];
		if (header->bam[*track-1].free) {
			for (j=0,*sector=0;j<3; j++) {
				int mask=1;
				for (; mask<=0x80; mask<<=1, (*sector)++) {
					if (header->bam[*track-1].map[j]&mask) {
						header->bam[*track-1].map[j]&=~mask;
						header->bam[*track-1].free--;
						return;
					}
				}
			}
		}
	}
}

static void d64_free_sector(struct _d64_image *image, int track, int sector)
{
	D64_HEADER *header=(D64_HEADER*)
		(image->data+image->get_offset(image->directory.track,image->directory.sector));
	int i=sector>>3;
	int mask=1<<(sector&7);
	header->bam[track-1].free++;
	header->bam[track-1].map[i]|=mask;
}

static void d71_alloc_sector(struct _d64_image *image, int *track, int *sector)
{
	static int tracks[70];
	int i, j;
	D64_HEADER *header=(D64_HEADER*)
		(image->data+image->get_offset(image->directory.track,image->directory.sector));
	D71_HEADER *header2=(D71_HEADER*)
		(image->data+image->get_offset(image->directory.track+35,image->directory.sector));

	// this contains the tracks for the search strategy for free sectors
	for (i=0,j=17; i<35; i+=4,j--) { tracks[i]=j; tracks[i+1]=j+35; }
	for (i=2,j=19; i<35; i+=4,j++) { tracks[i]=j; tracks[i+1]=j+35; }
	tracks[70-1]=18;

	for (i=0; i<35; i++) {
		*track=tracks[i];
		if (*tracks<=35) {
			if (header->bam[*track-1].free) {
				for (j=0,*sector=0;j<3; j++) {
					int mask=1;
					for (; mask<=0x80; mask<<=1, (*sector)++) {
						if (header->bam[*track-1].map[j]&mask) {
							header->bam[*track-1].map[j]&=~mask;
							header->bam[*track-1].free--;
							return;
						}
					}
				}
			}
		} else {
			if (header->free2[*track-36]) {
				for (j=0,*sector=0;j<3; j++) {
					int mask=1;
					for (; mask<=0x80; mask<<=1, (*sector)++) {
						if (header2->bam2[*track-36].map2[j]&mask) {
							header2->bam2[*track-36].map2[j]&=~mask;
							header->free2[*track-36]--;
							return;
						}
					}
				}
			}
		}
	}
}

static void d71_free_sector(struct _d64_image *image, int track, int sector)
{
	D64_HEADER *header=(D64_HEADER*)
		(image->data+image->get_offset(image->directory.track,image->directory.sector));
	D71_HEADER *header2=(D71_HEADER*)
		(image->data+image->get_offset(image->directory.track+35,image->directory.sector));
	int i=sector>>3;
	int mask=1<<(sector&7);
	if (track<=35) {
		header->bam[track-1].free++;
		header->bam[track-1].map[i]|=mask;
	} else {
		header->free2[track-36]++;
		header2->bam2[track-36].map2[i]|=mask;
	}
}

static void d81_alloc_sector(struct _d64_image *image, int *track, int *sector)
{
	static int tracks[80];
	int i, j;
	D81_BAM *bam[2];
	bam[0] = (D81_BAM*) (image->data + image->get_offset(image->directory.track, image->directory.sector+1)),
	bam[1] = (D81_BAM*) (image->data + image->get_offset(image->directory.track, image->directory.sector+1));
	
	/* this contains the tracks for the search strategy for free sectors */
	for (i=0,j=image->directory.track-1; j>0; i++,j--) tracks[i]=j;
	for (j=image->directory.track+1;j<80;i++,j++) tracks[i]=j;
	tracks[i]=image->directory.track;

	for (i=0; i<80; i++) {
		D81_BAM *b;
		*track=tracks[i];
		if (*track<=40) b=bam[0];
		else b=bam[1];
		if (b->bam[*track-1].free) {
			for (j=0,*sector=0;j<6; j++) {
				int mask=0;
				for (; mask<=0x80; mask<<=1, (*sector)++) {
					if (b->bam[*track-1].map[j]&mask) {
						b->bam[*track-1].map[j]&=~mask;
						b->bam[*track-1].free--;
						return;
					}
				}
			}
		}
	}
}

static void d81_free_sector(struct _d64_image *image, int track, int sector)
{
	D81_BAM *bam;
	int i=sector>>3;
	int mask=1<<(sector&7);

	if (track<=40) {
		bam=(D81_BAM*)
			(image->data
			 +image->get_offset(image->directory.track,
								image->directory.sector+1));
	} else {
		track-=40;
		bam=(D81_BAM*)
			(image->data
			 +image->get_offset(image->directory.track,
								image->directory.sector+2));
	}
	
	bam->bam[track-1].free++;
	bam->bam[track-1].map[i]|=mask;
}

int d64_getcrc(d64_image *image, int track, int sector)
{
	int pos=d64_tracksector2offset(track, sector)>>8;
	if (!image->crc) return 0;
	return image->data[d64_tracksector2offset(image->tracks+1,0)+pos];
}

static D64_ENTRY *d64_get_free_entry(d64_image *image)
{
	D64_HEADER *header;
	D64_DIRECTORY *dir = (D64_DIRECTORY*)
		(image->data+image->get_offset (image->directory.track, image->directory.sector));

	int i, mask, s;

	while ((dir->linkage.track >= 1) && (dir->linkage.track <= image->tracks))
	{
		dir=(D64_DIRECTORY*)
			(image->data+image->get_offset(dir->linkage.track, dir->linkage.sector));
		for (i = 0; i < 8; i ++)
		{
			if (!(dir->entry[i].type & 0x80))
				return dir->entry + i;
		}
	}

	// add new sector to directory
    header = (D64_HEADER*)
		(image->data+image->get_offset (image->directory.track, image->directory.sector));

	if (header->bam[image->directory.track-1].free) {
		for (i=0, s=0; i<image->bam_bytes_dir_track; i++) {
			for (mask=0x80; mask>0; mask>>=1, s++) {
				if (header->bam[image->directory.track-1].map[i]&mask) {
					header->bam[image->directory.track-1].free--;
					header->bam[image->directory.track-1].map[i]&=~mask;
					dir->linkage.track=image->directory.track-1;
					dir->linkage.sector=s;
					return (D64_ENTRY*)(image->data+image->get_offset(image->directory.track, s));
				}
			}
		}
	}
	// no sector in directory track free
	return NULL;
}

/* searches program with given name in directory */
static D64_ENTRY *d64_image_findfile (d64_image *image, const unsigned char *name)
{
	int i;

	D64_DIRECTORY *dir = (D64_DIRECTORY*)
		(image->data+image->get_offset (image->directory.track, image->directory.sector));

	while ((dir->linkage.track >= 1) && (dir->linkage.track <= image->tracks))
	{
		dir=(D64_DIRECTORY*)
			(image->data+image->get_offset(dir->linkage.track, dir->linkage.sector));
		for (i = 0; i < 8; i ++)
		{
			if (dir->entry[i].type & 0x80)
			{
				if (mame_stricmp ((char *) name, (char *) "*") == 0)
					return dir->entry + i;
				if (cbm_compareNames (name, dir->entry[i].name))
					return dir->entry + i;
			}
		}
	}
	return NULL;
}

#if 0
static int d64_filesize(d64_image *image, D64_ENTRY *entry)
{
	int size = 0;
	int i=image->get_offset(entry->track, entry->sector);
	
	while (image->data[i] != 0)
	{
		size += 254;
		i = image->get_offset (image->data[i], image->data[i + 1]);
	}
	size += image->data[i + 1]-1;
	return size;
}
#endif

static int d64_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg);
static void d64_image_exit(imgtool_image *img);
static void d64_image_info(imgtool_image *img, char *string, const int len);
static int d64_image_beginenum(imgtool_image *img, imgtool_directory **outenum);
static int d64_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent);
static void d64_image_closeenum(imgtool_directory *enumeration);
static size_t d64_image_freespace(imgtool_image *img);
static int d64_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf);
static int d64_image_writefile(imgtool_image *img, const char *fname, imgtool_stream *sourcef, const ResolvedOption *options_);
static int d64_image_deletefile(imgtool_image *img, const char *fname);
static int d64_image_create(const imgtool_module *mod, imgtool_stream *f, const ResolvedOption *options_);

static int x64_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg);
static int x64_image_create(const imgtool_module *mod, imgtool_stream *f, const ResolvedOption *options_);

static int d71_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg);
static size_t d71_image_freespace(imgtool_image *img);
static int d71_image_create(const imgtool_module *mod, imgtool_stream *f, const ResolvedOption *options_);

static int d81_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg);
static void d81_image_info(imgtool_image *img, char *string, const int len);
static size_t d81_image_freespace(imgtool_image *img);
static int d81_image_create(const imgtool_module *mod, imgtool_stream *f, const ResolvedOption *options_);

static int d64_read_sector(imgtool_image *img, UINT8 head, UINT8 track, UINT8 sector, int offset, void *buffer, int length);
static int d64_write_sector(imgtool_image *img, UINT8 head, UINT8 track, UINT8 sector, int offset, const void *buffer, int length);

static struct OptionTemplate d64_createopts[] =
{
	{ "label",	NULL, IMGOPTION_FLAG_TYPE_STRING | IMGOPTION_FLAG_HASDEFAULT,	0,		0,		NULL	},	/* [3] */
	{ NULL, NULL, 0, 0, 0, 0 }
};

#define D64_CREATEOPTION_LABEL			0

IMAGEMODULE(
	d64,
	"Commodore SX64/VC1541/2031/1551 Diskette",	/* human readable name */
	"d64",								/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	EOLN_CR,							/* eoln */
	0,									/* flags */
	d64_image_init,				/* init function */
	d64_image_exit,				/* exit function */
	d64_image_info,		/* info function */
	d64_image_beginenum,			/* begin enumeration */
	d64_image_nextenum,			/* enumerate next */
	d64_image_closeenum,			/* close enumeration */
	d64_image_freespace,			 /*  free space on image    */
	d64_image_readfile,			/* read file */
	d64_image_writefile,			/* write file */
	d64_image_deletefile,			/* delete file */
	d64_image_create,				/* create image */
	d64_read_sector,
	d64_write_sector,
	NULL,								/* file options */
	d64_createopts						/* create options */
)

IMAGEMODULE(
	x64,
	"Commodore VC1541 Diskette",	/* human readable name */
	"x64",								/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	EOLN_CR,							/* eoln */
	0,									/* flags */
	x64_image_init,				/* init function */
	d64_image_exit,				/* exit function */
	d64_image_info,		/* info function */
	d64_image_beginenum,			/* begin enumeration */
	d64_image_nextenum,			/* enumerate next */
	d64_image_closeenum,			/* close enumeration */
	d64_image_freespace,			 /* free space on image    */
	d64_image_readfile,			/* read file */
	d64_image_writefile,			/* write file */
	d64_image_deletefile,			/* delete file */
	x64_image_create,				/* create image */
	d64_read_sector,
	d64_write_sector,
	NULL,								/* file options */
	d64_createopts						/* create options */
)

IMAGEMODULE(
	d71,
	"Commodore 128D/1571 Diskette",	/* human readable name */
	"d71",								/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	EOLN_CR,							/* eoln */
	0,									/* flags */
	d71_image_init,				/* init function */
	d64_image_exit,				/* exit function */
	d64_image_info,		/* info function */
	d64_image_beginenum,			/* begin enumeration */
	d64_image_nextenum,			/* enumerate next */
	d64_image_closeenum,			/* close enumeration */
	d71_image_freespace,			 /*  free space on image    */
	d64_image_readfile,			/* read file */
	d64_image_writefile,			/* write file */
	d64_image_deletefile,			/* delete file */
	d71_image_create,				/* create image */
	d64_read_sector,
	d64_write_sector,
	NULL,								/* file options */
	d64_createopts						/* create options */
)

IMAGEMODULE(
	d81,
	"Commodore 65/1565/1581 Diskette",	/* human readable name */
	"d81",								/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	EOLN_CR,							/* eoln */
	0,									/* flags */
	d81_image_init,				/* init function */
	d64_image_exit,				/* exit function */
	d81_image_info,		/* info function */
	d64_image_beginenum,			/* begin enumeration */
	d64_image_nextenum,			/* enumerate next */
	d64_image_closeenum,			/* close enumeration */
	d81_image_freespace,			 /*  free space on image    */
	d64_image_readfile,			/* read file */
	d64_image_writefile,			/* write file */
	d64_image_deletefile,			/* delete file */
	d81_image_create,				/* create image */
	d64_read_sector,
	d64_write_sector,
	NULL,								/* file options */
	d64_createopts						/* create options */
)

static int d64_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg)
{
	d64_image *image;

	d64_open_helper();
	image=*(d64_image**)outimg=(d64_image *) malloc(sizeof(d64_image));
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(d64_image));
	image->base.module = mod;
	image->size=stream_size(f);
	image->file_handle=f;
	image->get_offset=d64_tracksector2offset;
	image->alloc_sector=d64_alloc_sector;
	image->free_sector=d64_free_sector;
	image->directory.track=18;image->directory.sector=0;
	image->bam_bytes_dir_track=3;
	image->d64=1;

	switch (image->size) {
	case 174848: image->tracks=35;break;
	case 175531: image->tracks=35;image->crc=1;break;
	case 196608: image->tracks=40;break;
	case 197376: image->tracks=40;image->crc=1;break;
	default:
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_CORRUPTIMAGE;
	}

	image->realimage = (unsigned char *) malloc(image->size);
	image->data = image->realimage;
	if ( (!image->data)
		 ||(stream_read(f, image->data, image->size)!=image->size) ) {
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_OUTOFMEMORY;
	}

	return 0;
}

static int x64_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg)
{
	d64_image *image;

	d64_open_helper();
	image=*(d64_image**)outimg=(d64_image *) malloc(sizeof(d64_image));
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(d64_image));
	image->base.module = mod;
	image->size=stream_size(f);
	image->file_handle=f;
	image->get_offset=d64_tracksector2offset;
	image->alloc_sector=d64_alloc_sector;
	image->free_sector=d64_free_sector;
	image->directory.track=18;image->directory.sector=0;
	image->bam_bytes_dir_track=3;
	image->x64=1;
	switch (image->size) {
	case 174848+0x40: image->tracks=35;break;
	case 175531+0x40: image->tracks=35;image->crc=1;break;
	case 196608+0x40: image->tracks=40;break;
	case 197376+0x40: image->tracks=40;image->crc=1;break;
	default:
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_CORRUPTIMAGE;
	}

	image->realimage = (unsigned char *) malloc(image->size);
	image->data = image->realimage+0x40;
	if ( (!image->realimage)
		 ||(stream_read(f, image->realimage, image->size)!=image->size) ) {
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_OUTOFMEMORY;
	}

	return 0;
}

static int d71_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg)
{
	d64_image *image;

	d64_open_helper();
	image=*(d64_image**)outimg=(d64_image *) malloc(sizeof(d64_image));
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(d64_image));
	image->base.module = mod;
	image->size=stream_size(f);
	image->file_handle=f;
	image->get_offset=d71_tracksector2offset;
	image->alloc_sector=d71_alloc_sector;
	image->free_sector=d71_free_sector;
	image->directory.track=18;image->directory.sector=0;
	image->d71=1;
	image->tracks=70;

	image->realimage = (unsigned char *) malloc(image->size);
	image->data = image->realimage;
	if ( (!image->data)
		 ||(stream_read(f, image->data, image->size)!=image->size) ) {
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_OUTOFMEMORY;
	}

	return 0;
}

static int d81_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg)
{
	d64_image *image;

	d64_open_helper();
	image=*(d64_image**)outimg=(d64_image *) malloc(sizeof(d64_image));
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(d64_image));
	image->base.module = mod;
	image->size=stream_size(f);
	image->get_offset=d81_tracksector2offset;
	image->alloc_sector=d81_alloc_sector;
	image->free_sector=d81_free_sector;
	image->directory.track=40;image->directory.sector=0;
	image->d81=1;
	image->tracks=80;
	image->bam_bytes_dir_track=6;
	image->file_handle=f;

	image->realimage = (unsigned char *) malloc(image->size);
	image->data = image->realimage;
	if ( (!image->data)
		 ||(stream_read(f, image->data, image->size)!=image->size) ) {
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_OUTOFMEMORY;
	}

	return 0;
}

static void d64_image_exit(imgtool_image *img)
{
	d64_image *image=(d64_image*)img;
	if (image->modified) {
		stream_seek(image->file_handle, 0, SEEK_SET);
		stream_write(image->file_handle, image->realimage, image->size);
	}
	stream_close(image->file_handle);
	free(image->realimage);
	free(image);
}

static void d64_image_info(imgtool_image *img, char *string, const int len)
{
	d64_image *image=(d64_image*)img;
	char name[17]={ 0 };

	int pos=image->get_offset(image->directory.track,image->directory.sector);

	cbm_filename_to_cstring((unsigned char *)image->data+pos+0x90, 16,
		(unsigned char*)name);
	if ((image->d64)||(image->x64)) {
		sprintf(string, "%d tracks%s\n%-16s %c%c %c%c",
				image->tracks, image->crc?" with crc":"",
				name,
				image->data[pos + 162], image->data[pos + 163],
				image->data[pos + 165], image->data[pos + 166] );
	} else {
		sprintf(string, "%-16s %c%c %c%c",
				name,
				image->data[pos + 162], image->data[pos + 163],
				image->data[pos + 165], image->data[pos + 166] );
	}
}

static void d81_image_info(imgtool_image *img, char *string, const int len)
{
	d64_image *image=(d64_image*)img;
	char name[17]={ 0 };

	int pos=image->get_offset(image->directory.track,image->directory.sector);

	cbm_filename_to_cstring((unsigned char *)image->data+pos+4, 16,
		(unsigned char *)name);
	sprintf(string, "%d tracks%s\n%-16s %c%c %c%c",
			image->tracks, image->crc?" with crc":"",
			name,
			image->data[pos + 22], image->data[pos + 23],
			image->data[pos + 25], image->data[pos + 26] );
}

static int d64_image_beginenum(imgtool_image *img, imgtool_directory **outenum)
{
	d64_image *image=(d64_image*)img;
	d64_iterator *iter;
	int pos;

	iter=*(d64_iterator**)outenum = (d64_iterator *) malloc(sizeof(d64_iterator));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;

	iter->image=image;
	pos = image->get_offset (image->directory.track, image->directory.sector);
	iter->track = iter->image->data[pos];
	iter->sector = iter->image->data[pos + 1];
	iter->offset = 2;
	return 0;
}

static int d64_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent)
{
	d64_iterator *iter=(d64_iterator*)enumeration;
	int pos;

	ent->corrupt=0;
	ent->eof=0;
	
	while ((iter->track >= 1) && (iter->track <= iter->image->tracks)) //safer
//	while (iter->track != 0)
	{
		pos = iter->image->get_offset(iter->track, iter->sector);
		for (; iter->offset<256; iter->offset += 32)
		{
			if (iter->image->data[pos + iter->offset] & 0x80)
			{
				cbm_filename_to_cstring(
					(unsigned char*)iter->image->data+pos+iter->offset+3,
					16, (unsigned char *)ent->fname);

				ent->filesize=iter->image->data[pos+iter->offset+28]
					+(iter->image->data[pos+iter->offset+29]<<8); // in blocks

				if (ent->attr) {
					switch (iter->image->data[pos+iter->offset]&FILE_TYPE_MASK) {
					case FILE_TYPE_DEL: sprintf(ent->attr,"DEL");break;
					case FILE_TYPE_SEQ: sprintf(ent->attr,"SEQ");break;
					case FILE_TYPE_PRG: sprintf(ent->attr,"PRG");break;
					case FILE_TYPE_USR: sprintf(ent->attr,"USR");break;
					case FILE_TYPE_REL: sprintf(ent->attr,"REL");break;
					default: sprintf(ent->attr,"unknown!");break;
					}
				}
				iter->offset+=32;
				return 0;
			}
		}
		iter->track = iter->image->data[pos];
		iter->sector = iter->image->data[pos + 1];
		iter->offset = 2;
	}

	ent->eof=1;
	return 0;
}

static void d64_image_closeenum(imgtool_directory *enumeration)
{
	free(enumeration);
}

static size_t d64_image_freespace(imgtool_image *img)
{
	d64_image *image=(d64_image*)img;
	int i, j, pos;
	size_t blocksfree = 0;

	pos = image->get_offset(image->directory.track, image->directory.sector);
	blocksfree = 0;
	for (j = 1, i = 4; j <= 35; j++, i += 4) // no more room for BAM!
	{
		blocksfree += image->data[pos + i];
	}

	return blocksfree;
}

static size_t d71_image_freespace(imgtool_image *img)
{
	d64_image *image=(d64_image*)img;
	int i, j, pos;
	size_t blocksfree = 0;

	pos = image->get_offset(image->directory.track, image->directory.sector);
	blocksfree = 0;
	for (j = 1, i = 4; j <= 35; j++, i += 4)
	{
		blocksfree += image->data[pos + i];
	}
	for (i = 221; j <= 70; j++, i ++)
	{
		blocksfree += image->data[pos + i];
	}

	return blocksfree;
}

static size_t d81_image_freespace(imgtool_image *img)
{
	d64_image *image=(d64_image*)img;
	int j,k;
	size_t blocksfree = 0;

	for (k=1; k<=2; k++) {
		D81_BAM *bam=(D81_BAM*)(image->data+image->get_offset(40,k));
		for (j = 1; j <= 40; j++)
		{
			blocksfree += bam->bam[j-1].free;
		}
	}

	return blocksfree;
}

static int d64_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf)
{
	d64_image *image=(d64_image*)img;
	int pos;
	D64_ENTRY *entry;

	if ((entry=d64_image_findfile(image, (const unsigned char *)fname))==NULL )
		return IMGTOOLERR_MODULENOTFOUND;

	pos = image->get_offset(entry->track, entry->sector);

	while (image->data[pos]!=0) {
		if (stream_write(destf, image->data+pos+2, 254)!=254)
			return IMGTOOLERR_WRITEERROR;
		
		pos = image->get_offset (image->data[pos + 0], image->data[pos + 1]);
	}
	if (image->data[pos+1]-1>0) {
		if (stream_write(destf, image->data+pos+2, image->data[pos+1]-1)!=image->data[pos+1]-1)
			return IMGTOOLERR_WRITEERROR;
	}

	return 0;
}

static int d64_image_writefile(imgtool_image *img, const char *fname, imgtool_stream *sourcef, const ResolvedOption *options_)
{
	d64_image *image=(d64_image*)img;
	int fsize, pos, i, b, freespace;
	int track, sector;
	D64_ENTRY *entry;

	fsize=stream_size(sourcef)+1;

	imgtool_partition_get_free_space(img, &freespace);

	if ((entry=d64_image_findfile(image, (const unsigned char *)fname))!=NULL ) {
		/* overriding */
		if ((freespace + GET_UWORD(entry->blocks))*254<fsize) 
			return IMGTOOLERR_NOSPACE;
		track=entry->track;
		sector=entry->sector;
		
		while (track!=0) {
			image->free_sector(image, track, sector);
			pos = image->get_offset(track, sector);
			track=image->data[pos];
			sector=image->data[pos+1];
		}
	} else {
		if (freespace*254<fsize) return IMGTOOLERR_NOSPACE;
		// search free entry
		entry=d64_get_free_entry(image);
	}

	entry->type=FILE_TYPE_PRG|0x80;
	memset(entry->name, 0xa0, sizeof(entry->name));
	memcpy(entry->name, fname, strlen(fname));

	image->alloc_sector(image, &track, &sector);
	entry->track=track; 
	entry->sector=sector;
	pos = image->get_offset(track, sector);

	for (i = 0, b=0; i + 254 < fsize; i += 254, b++)
	{
		if (stream_read(sourcef, image->data+pos+2, 254)!=254)
			return IMGTOOLERR_READERROR;
		
		image->alloc_sector(image, &track, &sector);
		image->data[pos]=track; 
		image->data[pos+1]=sector;
		pos = image->get_offset (track, sector);
	}
	b++;
	image->data[pos]=0;
	image->data[pos+1]=fsize-i;
	if (fsize-i-1>0) {
		if (stream_read(sourcef, image->data+pos+2, fsize-i-1)!=fsize-i-1)
			return IMGTOOLERR_READERROR;
	}
	image->data[pos+2+fsize-i]=0;
	
	SET_UWORD(entry->blocks, b);
	image->modified=1;

	return 0;
}

static int d64_image_deletefile(imgtool_image *img, const char *fname)
{
	d64_image *image=(d64_image*)img;
	int pos;
	int sector, track;
	D64_ENTRY *entry;

	if ((entry=d64_image_findfile(image, (const unsigned char *)fname))==NULL )
		return IMGTOOLERR_MODULENOTFOUND;

	track=entry->track;
	sector=entry->sector;

	while (track!=0) {
		image->free_sector(image, track, sector);
		pos = image->get_offset(track, sector);
		track=image->data[pos];
		sector=image->data[pos+1];
	}

	image->modified=1;
	return 0;
}

static int d64_read_sector(imgtool_image *img, UINT8 head, UINT8 track, UINT8 sector, int offset, void *buffer, int length)
{
	/* not yet implemented */
	assert(0);
	return IMGTOOLERR_UNEXPECTED;
}

static int d64_write_sector(imgtool_image *img, UINT8 head, UINT8 track, UINT8 sector, int offset, const void *buffer, int length)
{
	/* not yet implemented */
	assert(0);
	return IMGTOOLERR_UNEXPECTED;
}

static int d64_image_create(const imgtool_module *mod, imgtool_stream *f, const ResolvedOption *options_)
{
	unsigned char sector[0x100]={0};
	int tracks=35;
	int crc=0;
	int t,s,i;

	for (t=1; t<=tracks; t++) {
		for (s=0; s<d64_sectors_per_track[t-1]; s++) {
			if ((t==18)&&(s==0)) {
				for (i=1; i<=35; i++) {
					d64_header.bam[i-1].free=d64_sectors_per_track[i-1];
					d64_header.bam[i-1].map[0]=0xff;
					d64_header.bam[i-1].map[1]=0xff;
					switch (d64_header.bam[i-1].free) {
					case 17: d64_header.bam[i-1].map[2]=1;break;
					case 18: d64_header.bam[i-1].map[2]=3;break;
					case 19: d64_header.bam[i-1].map[2]=7;break;
					case 21: d64_header.bam[i-1].map[2]=0x1f;break;
					}
					if (i==18) { // allocate the 2 directory sectors
						d64_header.bam[i-1].free-=2;
						d64_header.bam[i-1].map[0]&=~3;
					}
				}
				//d64_header.name=0xa0;
				//d64_header.id=0xa0;
				if (stream_write(f, &d64_header, sizeof(d64_header)) != sizeof(d64_header)) 
					return  IMGTOOLERR_WRITEERROR;
			} else {
				if (stream_write(f, &sector, sizeof(sector)) != sizeof(sector)) 
					return  IMGTOOLERR_WRITEERROR;
			}
		}
	}
	if (crc) {
		for (t=1; t<=tracks; t++) {
			if (stream_write(f, &d64_header, d64_sectors_per_track[t-1]) 
				!= d64_sectors_per_track[t-1]) 
				return  IMGTOOLERR_WRITEERROR;
			}
	}
	return 0;
}

static int x64_image_create(const imgtool_module *mod, imgtool_stream *f, const ResolvedOption *options_)
{
	struct { 
		unsigned char data[0x40];
	} x64_header={ 
		{ 0x43, 0x15, 0x41, 0x64,
		  0x01, 0x02, 0x01, 0x23, 
		  0x01
		}
	};
	if (stream_write(f, &x64_header, sizeof(x64_header)) != sizeof(x64_header)) 
		return  IMGTOOLERR_WRITEERROR;

	d64_image_create(mod, f, options_);
	return 0;
}

static int d71_image_create(const imgtool_module *mod, imgtool_stream *f, const ResolvedOption *options_)
{
	D71_HEADER d71_header= { { 0 } };
	unsigned char sector[0x100]={0};
	int t,s,i;

	for (t=1; t<=70; t++) {
		for (s=0; s<d71_sectors_per_track[t-1]; s++) {
			if ((t==18)&&(s==0)) {
				d64_header.doublesided=0x80;
				for (i=1; i<=35; i++) {
					d64_header.bam[i-1].free=d71_sectors_per_track[i-1];
					d64_header.bam[i-1].map[0]=0xff;
					d64_header.bam[i-1].map[1]=0xff;
					switch (d64_header.bam[i-1].free) {
					case 17: d64_header.bam[i-1].map[2]=1;break;
					case 18: d64_header.bam[i-1].map[2]=3;break;
					case 19: d64_header.bam[i-1].map[2]=7;break;
					case 21: d64_header.bam[i-1].map[2]=0x1f;break;
					}
					if (i==18) { // allocate the 2 directory sectors
						d64_header.bam[i-1].free-=2;
						d64_header.bam[i-1].map[0]&=~3;
					}
				}
				for (;i<=70;i++) {
					if (i!=18+35)
						d64_header.free2[i-36]=d71_sectors_per_track[i-1];
					else
						d64_header.free2[i-36]=0;
				}
				//d64_header.name=0xa0;
				//d64_header.id=0xa0;
				if (stream_write(f, &d64_header, sizeof(d64_header)) != sizeof(d64_header)) 
					return  IMGTOOLERR_WRITEERROR;
			} else if ((t==35+18)&&(s==0)) {
				for (i=36;i<=70;i++) {
					if (i!=18+35) {
						d71_header.bam2[i-36].map2[0]=0xff;
						d71_header.bam2[i-36].map2[1]=0xff;
						switch (d64_header.free2[i-36]) {
						case 17: d71_header.bam2[i-36].map2[2]=1;break;
						case 18: d71_header.bam2[i-36].map2[2]=3;break;
						case 19: d71_header.bam2[i-36].map2[2]=7;break;
						case 21: d71_header.bam2[i-36].map2[2]=0x1f;break;
						}
					} else {
						d71_header.bam2[i-36].map2[0]=0;
						d71_header.bam2[i-36].map2[1]=0;
						d71_header.bam2[i-36].map2[2]=0;
					}
				}
				if (stream_write(f, &d71_header, sizeof(d71_header)) 
					!= sizeof(d71_header)) 
					return  IMGTOOLERR_WRITEERROR;
			} else {
				if (stream_write(f, &sector, sizeof(sector)) != sizeof(sector)) 
					return  IMGTOOLERR_WRITEERROR;
			}
		}
	}
	return 0;
}

static int d81_image_create(const imgtool_module *mod, imgtool_stream *f, const ResolvedOption *options_)
{
	unsigned char sector[0x100]={0};
	unsigned char id[2]={'1','2'};
	int t,s,i;

	for (t=1; t<=80; t++) {
		for (s=0; s<40; s++) {
			if ((t==40)&&(s==0)) {
				//d81_header.name=0xa0;
				d81_header.id[0]=id[0];
				d81_header.id[0]=id[1];
				if (stream_write(f, &d81_header, sizeof(d81_header)) != sizeof(d81_header)) 
					return  IMGTOOLERR_WRITEERROR;
			} else if ((t==40)&&(s==1)){
				d81_bam.id[0]=id[0];
				d81_bam.id[0]=id[1];
				for (i=1; i<=40; i++) {
					d81_bam.bam[i-1].free=40;
					d81_bam.bam[i-1].map[0]=0xff;
					d81_bam.bam[i-1].map[1]=0xff;
					d81_bam.bam[i-1].map[2]=0xff;
					d81_bam.bam[i-1].map[3]=0xff;
					d81_bam.bam[i-1].map[4]=0xff;
				}
				d81_bam.bam[40-1].free=36;
				d81_bam.bam[40-1].map[0]=0xf0;
				if (stream_write(f, &d81_bam, sizeof(d81_bam)) != sizeof(d81_bam)) 
					return  IMGTOOLERR_WRITEERROR;
				d81_bam.bam[40-1].free=40;
				d81_bam.bam[40-1].map[0]=0xff;
			} else if ((t==40)&&(s==2)){
				d81_bam.track=0;
				if (stream_write(f, &d81_bam, sizeof(d81_bam)) != sizeof(d81_bam)) 
					return  IMGTOOLERR_WRITEERROR;
			} else {
				if (stream_write(f, &sector, sizeof(sector)) != sizeof(sector)) 
					return  IMGTOOLERR_WRITEERROR;
			}
		}
	}
	return 0;
}
