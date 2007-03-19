/*
 * xsa.c : decompresses XelaSoft Archives.
 *
 * Sean Young
 */

#include "osdepend.h"
#include "imgtoolx.h"
#include "utils.h"


typedef struct {
	imgtool_image			base;
	char			*file_name;
	imgtool_stream 			*file_handle;
	int 			size;
	} XSA_IMAGE;

typedef struct
	{
	imgtool_directory 	base;
	XSA_IMAGE	*image;
	int			index;
	} XSA_ITERATOR;

static int xsa_extract (imgtool_stream *in, imgtool_stream *out);
static int xsa_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg);
static void xsa_image_exit(imgtool_image *img);
static int xsa_image_beginenum(imgtool_image *img, imgtool_directory **outenum);
static int xsa_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent);
static void xsa_image_closeenum(imgtool_directory *enumeration);
static int xsa_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf);

IMAGEMODULE(
	xsa,
	"XelaSoft Archive",				/* human readable name */
	"xsa\0",						/* file extension */
	NULL,							/* crcfile */
	NULL,							/* crc system name */
	NULL,							/* eoln */
	0,								/* flags */
	xsa_image_init,					/* init function */
	xsa_image_exit,					/* exit function */
	NULL,							/* info function */
	xsa_image_beginenum,			/* begin enumeration */
	xsa_image_nextenum,				/* enumerate next */
	xsa_image_closeenum,			/* close enumeration */
	NULL,							/* free space on image */
	xsa_image_readfile,				/* read file */
	NULL,							/* write file */
	NULL,							/* delete file */
	NULL,							/* create image */
	NULL,							/* read sector */
	NULL,							/* write sector */
	NULL,							/* file options */
	NULL							/* create options */
)

static int xsa_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg)
	{
	XSA_IMAGE *image;
	int pos, len, size;
    UINT8 header[4], byt;
	char *file_name, *file_name_new;

	size = stream_size (f);
	if (size < 5) return IMGTOOLERR_MODULENOTFOUND;
	if (4 != stream_read (f, header, 4) ) return IMGTOOLERR_READERROR;
	
    if (memcmp (header, "PCK\010", 4) )
		return IMGTOOLERR_MODULENOTFOUND;
	
	if (4 != stream_read (f, &size, 4) ) return IMGTOOLERR_READERROR;
	size = LITTLE_ENDIANIZE_INT32 (size);
	if (4 != stream_read (f, header, 4) ) return IMGTOOLERR_READERROR;
	
	/* get name from XSA header, can't be longer than 8.3 really */
	/* but could be, it's zero-terminated */
	file_name = NULL;
	pos = len = 0;
	while (1)
		{
		if (1 != stream_read (f, &byt, 1) )
			{
			if (file_name) free (file_name);
			*outimg=NULL;
			return IMGTOOLERR_READERROR;
			}
		if (len <= pos)
			{
			len += 8; /* why 8? */
			file_name_new = realloc (file_name, len);
			if (!file_name_new)
				{
				if (file_name) free (file_name);
				*outimg=NULL;
				return IMGTOOLERR_OUTOFMEMORY;
				}
			file_name = file_name_new;
			}
		file_name[pos++] = byt;
		if (!byt) break;
		}

	image = (XSA_IMAGE*)malloc (sizeof (XSA_IMAGE) );
	if (!image) return IMGTOOLERR_OUTOFMEMORY;
	*outimg = (imgtool_image*)image;

	memset(image, 0, sizeof(XSA_IMAGE));
	image->base.module = mod;
	image->file_handle = f;
	image->size = size;
	image->file_name = file_name;

	return 0;
	}

static void xsa_image_exit(imgtool_image *img)
	{
	XSA_IMAGE *image=(XSA_IMAGE*)img;
	stream_close(image->file_handle);
	free(image->file_name);
	free(image);
	}

static int xsa_image_beginenum(imgtool_image *img, imgtool_directory **outenum)
	{
	XSA_IMAGE *image=(XSA_IMAGE*)img;
	XSA_ITERATOR *iter;

	iter=*(XSA_ITERATOR**)outenum = (XSA_ITERATOR*) malloc(sizeof(XSA_ITERATOR));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;

	iter->image=image;
	iter->index = 0;
	return 0;
	}

static int xsa_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent)
	{
	XSA_ITERATOR *iter=(XSA_ITERATOR*)enumeration;

	ent->eof = iter->index;
	if (!ent->eof)
		{
		strcpy (ent->fname, iter->image->file_name);

		ent->corrupt=0;
		ent->filesize = iter->image->size;

		iter->index++;
		}

	return 0;
	}

static void xsa_image_closeenum(imgtool_directory *enumeration)
	{
	free(enumeration);
	}

static int xsa_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf)
	{
	XSA_IMAGE *image=(XSA_IMAGE*)img;

	/*  check file name */
	if (mame_stricmp (fname, image->file_name) ) 
		return IMGTOOLERR_MODULENOTFOUND;

	return xsa_extract (image->file_handle, destf);
	}


/*
 * .xsa decompression. Code stolen from :
 *
 * http://web.inter.nl.net/users/A.P.Wulms/
 */


/****************************************************************/
/* LZ77 data decompression										*/
/* Copyright (c) 1994 by XelaSoft								*/
/* version history:												*/
/*   version 0.9, start date: 11-27-1994						*/
/****************************************************************/

#define SHORT unsigned

#define slwinsnrbits (13)
#define maxstrlen (254)

#define maxhufcnt (127)
#define logtblsize (4)


#define tblsize (1<<logtblsize)

typedef struct huf_node {
  SHORT weight;
  struct huf_node *child1, *child2;
} huf_node;

/****************************************************************/
/* global vars													*/
/****************************************************************/
static unsigned updhufcnt;
static unsigned cpdist[tblsize+1];
static unsigned cpdbmask[tblsize];
static unsigned cpdext[] = { /* Extra bits for distance codes */
          0,  0,  0,  0,  1,  2,  3,  4,
          5,  6,  7,  8,  9, 10, 11, 12};

static SHORT tblsizes[tblsize];
static huf_node huftbl[2*tblsize-1];

/****************************************************************/
/* maak de huffman codeer informatie							*/
/****************************************************************/
static void mkhuftbl( void )
{
  unsigned count;
  huf_node  *hufpos;
  huf_node  *l1pos, *l2pos;
  SHORT tempw;

  /* Initialize the huffman tree */
  for (count=0, hufpos=huftbl; count != tblsize; count++) {
    (hufpos++)->weight=1+(tblsizes[count] >>= 1);
  }
  for (count=tblsize; count != 2*tblsize-1; count++)
    (hufpos++)->weight=-1;

  /* Place the nodes in the correct manner in the tree */
  while (huftbl[2*tblsize-2].weight == -1) {
    for (hufpos=huftbl; !(hufpos->weight); hufpos++)
      ;
    l1pos = hufpos++;
    while (!(hufpos->weight))
      hufpos++;
    if (hufpos->weight < l1pos->weight) {
      l2pos = l1pos;
      l1pos = hufpos++;
    }
    else
      l2pos = hufpos++;
    while ((tempw=(hufpos)->weight) != -1) {
      if (tempw) {
        if (tempw < l1pos->weight) {
          l2pos = l1pos;
          l1pos = hufpos;
        }
        else if (tempw < l2pos->weight)
          l2pos = hufpos;
      }
      hufpos++;
    }
    hufpos->weight = l1pos->weight+l2pos->weight;
    (hufpos->child1 = l1pos)->weight = 0;
    (hufpos->child2 = l2pos)->weight = 0;
  }
  updhufcnt = maxhufcnt;
}

/****************************************************************/
/* initialize the huffman info tables                           */
/****************************************************************/
static void inithufinfo( void )
{
  unsigned offs, count;

  for (offs=1, count=0; count != tblsize; count++) {
    cpdist[count] = offs;
    offs += (cpdbmask[count] = 1<<cpdext[count]);
  }
  cpdist[count] = offs;

  for (count = 0; count != tblsize; count++) {
    tblsizes[count] = 0; /* reset the table counters */
    huftbl[count].child1 = 0;  /* mark the leave nodes */
  }
  mkhuftbl(); /* make the huffman table */
}

/****************************************************************/
/* global vars													*/
/****************************************************************/
static imgtool_stream *in_stream, *out_stream;

static UINT8 *outbuf;       /* the output buffer     */
static UINT8 *outbufpos;    /* pos in output buffer  */
static int outbufcnt;  /* #elements in the output buffer */
static int error_occ;

static UINT8 bitflg;  /* flag with the bits */
static UINT8 bitcnt;  /* #resterende bits   */

#define slwinsize (1 << slwinsnrbits)
#define outbufsize (slwinsize+4096)

/****************************************************************/
/* The function prototypes										*/
/****************************************************************/
static void unlz77( void );       /* perform the real decompression       */
static void charout(UINT8);      /* put a character in the output stream */
static unsigned rdstrlen( void ); /* read string length                   */
static unsigned rdstrpos( void ); /* read string pos                      */
static void flushoutbuf( void );
static UINT8 charin( void );       /* read a char                          */
static UINT8 bitin( void );        /* read a bit                           */

/****************************************************************/
/* de hoofdlus													*/
/****************************************************************/
static int xsa_extract (imgtool_stream *in, imgtool_stream *out)
    {
    UINT8 byt;

	/* setup the in-stream */
	stream_seek (in, 12, SEEK_SET);
    do  {
		if (1 != stream_read (in, &byt, 1) )
			return IMGTOOLERR_READERROR;
		} while (byt);
	in_stream = in;
    bitcnt = 0;         /* nog geen bits gelezen               */

	/* setup the out buffer */
    outbuf = malloc (outbufsize);
    if (!outbuf) return IMGTOOLERR_OUTOFMEMORY;
    outbufcnt = 0;
    outbufpos = outbuf; /* dadelijk vooraan in laden           */
	out_stream = out;

	/* let's do it .. */
	inithufinfo(); /* initialize the cpdist tables */

    error_occ = 0;
    unlz77();         /* decrunch de data echt               */

	free (outbuf);

	return error_occ;
    }

/****************************************************************/
/* the actual decompression algorithm itself					*/
/****************************************************************/
static void unlz77( void )
{
  UINT8 strl = 0;
  unsigned strpos;

  do {
    switch (bitin()) {
      case 0 : charout(charin()); break;
      case 1 : strl = rdstrlen();
	       if (strl == (maxstrlen+1))
		 break;
	       strpos = rdstrpos();
	       while (strl--)
		 charout(*(outbufpos-strpos));
	       strl = 0;
	       break;
    }
  }
  while (strl != (maxstrlen+1)&&!error_occ);
  flushoutbuf ();
}

/****************************************************************/
/* Put the next character in the input buffer.                  */
/* Take care that minimal 'slwinsize' chars are before the      */
/* current position.                                            */
/****************************************************************/
static void charout(UINT8 ch)
{
  if (error_occ) return;

  if ((outbufcnt++) == outbufsize) {
    if ( (outbufsize-slwinsize) != stream_write (out_stream, outbuf, outbufsize-slwinsize) )
	 	error_occ = IMGTOOLERR_WRITEERROR;

    memcpy(outbuf, outbuf+outbufsize-slwinsize, slwinsize);
    outbufpos = outbuf+slwinsize;
    outbufcnt = slwinsize+1;
  }
  *(outbufpos++) = ch;
}

/****************************************************************/
/* flush the output buffer                                      */
/****************************************************************/
static void flushoutbuf( void )
{
  if (!error_occ && outbufcnt) {
    if (outbufcnt != stream_write (out_stream, outbuf, outbufcnt) )
		error_occ = IMGTOOLERR_WRITEERROR;

    outbufcnt = 0;
  }
}

/****************************************************************/
/* read string length											*/
/****************************************************************/
static unsigned rdstrlen( void )
{
  UINT8 len;
  UINT8 nrbits;

  if (!bitin())
    return 2;
  if (!bitin())
    return 3;
  if (!bitin())
    return 4;

  for (nrbits = 2; (nrbits != 7) && bitin(); nrbits++)
    ;

  len = 1;
  while (nrbits--)
    len = (len << 1) | bitin();

  return (unsigned)(len+1);
}

/****************************************************************/
/* read string pos												*/
/****************************************************************/
static unsigned rdstrpos( void )
{
  UINT8 nrbits;
  UINT8 cpdindex;
  unsigned strpos;
  huf_node *hufpos;

  hufpos = huftbl+2*tblsize-2;

  while (hufpos->child1)
    if (bitin()) {
      hufpos = hufpos->child2;
    }
    else {
      hufpos = hufpos->child1;
    }
  cpdindex = hufpos-huftbl;
  tblsizes[cpdindex]++;

  if (cpdbmask[cpdindex] >= 256) {
    UINT8 strposmsb, strposlsb;

    strposlsb = (unsigned)charin();  /* lees LSB van strpos */
    strposmsb = 0;
    for (nrbits = cpdext[cpdindex]-8; nrbits--; strposmsb |= bitin())
      strposmsb <<= 1;
    strpos = (unsigned)strposlsb | (((unsigned)strposmsb)<<8);
  }
  else {
    strpos=0;
    for (nrbits = cpdext[cpdindex]; nrbits--; strpos |= bitin())
      strpos <<= 1;
  }
  if (!(updhufcnt--))
    mkhuftbl(); /* make the huffman table */

  return strpos+cpdist[cpdindex];
}

/****************************************************************/
/* read a bit from the input file								*/
/****************************************************************/
static UINT8 bitin( void )
{
  UINT8 temp;

  if (!bitcnt) {
    bitflg = charin();  /* lees de bitflg in */
    bitcnt = 8;         /* nog 8 bits te verwerken */
  }
  temp = bitflg & 1;
  bitcnt--;  /* weer 1 bit verwerkt */
  bitflg >>= 1;

  return temp;
}

/****************************************************************/
/* Get the next character from the input buffer.                */
/****************************************************************/
static UINT8 charin( void )
	{
	UINT8 byte;

	if (error_occ)
		return 0;
	else
        {
		if (1 != stream_read (in_stream, &byte, 1) )
			{
			error_occ = IMGTOOLERR_READERROR;
			return 0;
			}
		else
			return byte;
		}
	}
