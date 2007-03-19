/*
	Code to interface the MESS image code with MAME's harddisk core.

	We do not support diff files as it will involve some changes in the MESS
	image code.

	Raphael Nabet 2003
*/

/*#include "imgtool.h"*/
#include "imgtoolx.h"

#include "harddisk.h"
#include "imghd.h"


/* Encoded mess_image: in order to have hard_disk_open handle mess_image
pointers, we encode the reference as an ASCII string and pass it as a file
name.  mess_hard_disk_open then decodes the file name to get the original
mess_image pointer. */
#define encoded_image_ref_prefix "/:/i/m/g/t/o/o/l//S/T/R/E/A/M//#"
#define encoded_image_ref_len_format "%03d"
#define encoded_image_ref_format encoded_image_ref_prefix encoded_image_ref_len_format "@%p"
enum
{
	ptr_max_len = 100,
	encoded_image_ref_len_offset = sizeof(encoded_image_ref_prefix)-1,
	encoded_image_ref_len_len = 3,
	encoded_image_ref_max_len = sizeof(encoded_image_ref_prefix)-1+encoded_image_ref_len_len+1+ptr_max_len+1
};



/* this should not be necessary, but I'd rather introduce a spectacular rather
 * than a subtle crash */
static void chd_save_interface(chd_interface *intf)
{
	memset(intf, 0, sizeof(*intf));
}



static imgtoolerr_t map_chd_error(chd_error chderr)
{
	imgtoolerr_t err;

	switch(chderr)
	{
		case CHDERR_NONE:
			err = IMGTOOLERR_SUCCESS;
			break;
		case CHDERR_OUT_OF_MEMORY:
			err = IMGTOOLERR_OUTOFMEMORY;
			break;
		case CHDERR_FILE_NOT_WRITEABLE:
			err = IMGTOOLERR_READONLY;
			break;
		case CHDERR_NOT_SUPPORTED:
			err = IMGTOOLERR_UNIMPLEMENTED;
			break;
		default:
			err = IMGTOOLERR_UNEXPECTED;
			break;
	}
	return err;
}



/*
	encode_image_ref()

	Encode an image pointer into an ASCII string that can be passed to
	hard_disk_open as a file name.
*/
static void encode_image_ref(const imgtool_stream *stream, char encoded_image_ref[encoded_image_ref_max_len])
{
	int actual_len;
	char buf[encoded_image_ref_len_len+1];

	/* print, leaving len as 0 */
	actual_len = snprintf(encoded_image_ref, encoded_image_ref_max_len,
		encoded_image_ref_format, 0, (void *) stream);

	/* debug check: has the buffer been filled */
	assert(actual_len < (encoded_image_ref_max_len-1));

	/* print actual lenght */
	sprintf(buf, encoded_image_ref_len_format, actual_len);
	memcpy(encoded_image_ref+encoded_image_ref_len_offset, buf, encoded_image_ref_len_len);
}

/*
	decode_image_ref()

	This function will decode an image pointer, provided one has been encoded
	in the ASCII string.
*/
static imgtool_stream *decode_image_ref(const char *encoded_image_ref)
{
	int expected_len;
	void *ptr;

	/* read original lenght and ptr */
	if (sscanf(encoded_image_ref, encoded_image_ref_format, &expected_len, &ptr) == 2)
	{
		/* only return ptr if lenght match */
		if (expected_len == strlen(encoded_image_ref))
			return (imgtool_stream *) ptr;
	}

	return NULL;
}


static chd_interface_file *imgtool_chd_open(const char *filename, const char *mode);
static void imgtool_chd_close(chd_interface_file *file);
static UINT32 imgtool_chd_read(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 imgtool_chd_write(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer);
static UINT64 imgtool_chd_length(chd_interface_file *file);

static chd_interface imgtool_chd_interface =
{
	imgtool_chd_open,
	imgtool_chd_close,
	imgtool_chd_read,
	imgtool_chd_write,
	imgtool_chd_length
};

/*
	MAME hard disk core interface
*/

/*
	imgtool_chd_open - interface for opening a hard disk image
*/
static chd_interface_file *imgtool_chd_open(const char *filename, const char *mode)
{
	imgtool_stream *img = decode_image_ref(filename);

	/* invalid "file name"? */
	if (img == NULL)
		return NULL;

	/* read-only fp? */
	if ((stream_isreadonly(img)) && ! (mode[0] == 'r' && !strchr(mode, '+')))
		return NULL;

	/* otherwise return file pointer */
	return (chd_interface_file *) img;
}

/*
	imgtool_chd_close - interface for closing a hard disk image
*/
static void imgtool_chd_close(chd_interface_file *file)
{
}

/*
	imgtool_chd_read - interface for reading from a hard disk image
*/
static UINT32 imgtool_chd_read(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	stream_seek((imgtool_stream *)file, offset, SEEK_SET);
	return stream_read((imgtool_stream *)file, buffer, count);
}

/*
	imgtool_chd_write - interface for writing to a hard disk image
*/
static UINT32 imgtool_chd_write(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	stream_seek((imgtool_stream *)file, offset, SEEK_SET);
	return stream_write((imgtool_stream *)file, buffer, count);
}

/*
	imgtool_chd_write - interface for determining the length of a hard disk image
*/
static UINT64 imgtool_chd_length(chd_interface_file *file)
{
	return stream_size((imgtool_stream *)file);
}


/*
	imghd_create()

	Create a MAME HD image
*/
imgtoolerr_t imghd_create(imgtool_stream *stream, UINT32 hunksize, UINT32 cylinders, UINT32 heads, UINT32 sectors, UINT32 seclen)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	char encoded_image_ref[encoded_image_ref_max_len];
	chd_interface interface_save;
	UINT8 *cache = NULL;
	chd_file *chd = NULL;
	int rc;
	UINT64 logicalbytes;
	int hunknum, totalhunks;
	char metadata[256];

	/* jump through the hoops as required by the CHD system */
	encode_image_ref(stream, encoded_image_ref);
	chd_save_interface(&interface_save);
	chd_set_interface(&imgtool_chd_interface);

	/* sanity check args */
	if (hunksize >= 2048)
	{
		err = IMGTOOLERR_PARAMCORRUPT;
		goto done;
	}
	if (hunksize <= 0)
		hunksize = 1024;	/* default value */

	/* bail if we are read only */
	if (stream_isreadonly(stream))
	{
		err = IMGTOOLERR_READONLY;
		goto done;
	}

	/* calculations */
	logicalbytes = (UINT64)cylinders * heads * sectors * seclen;

	/* create the new hard drive */
	rc = chd_create(encoded_image_ref, logicalbytes, hunksize, CHDCOMPRESSION_NONE, NULL);
	if (rc != CHDERR_NONE)
	{
		err = map_chd_error(rc);
		goto done;
	}

	/* open the new hard drive */
	rc = chd_open(encoded_image_ref, CHD_OPEN_READWRITE, NULL, &chd);
	if (rc != CHDERR_NONE)
	{
		err = map_chd_error(rc);
		goto done;
	}

	/* write the metadata */
	sprintf(metadata, HARD_DISK_METADATA_FORMAT, cylinders, heads, sectors, seclen);
	err = chd_set_metadata(chd, HARD_DISK_METADATA_TAG, 0, metadata, strlen(metadata) + 1);
	if (rc != CHDERR_NONE)
	{
		err = map_chd_error(rc);
		goto done;
	}

	/* alloc and zero buffer */
	cache = malloc(hunksize);
	if (!cache)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	memset(cache, '\0', hunksize);

	/* zero out every hunk */
	totalhunks = (logicalbytes + hunksize - 1) / hunksize;
	for (hunknum = 0; hunknum < totalhunks; hunknum++)
	{
		rc = chd_write(chd, hunknum, cache);
		if (rc)
		{
			err = IMGTOOLERR_WRITEERROR;
			goto done;
		}
	}

	
done:
	if (cache)
		free(cache);
	if (chd)
		chd_close(chd);
	chd_set_interface(&interface_save);
	return err;
}



/*
	imghd_open()

	Open stream as a MAME HD image
*/
imgtoolerr_t imghd_open(imgtool_stream *stream, struct mess_hard_disk_file *hard_disk)
{
	chd_error chderr;
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	char encoded_image_ref[encoded_image_ref_max_len];
	chd_interface interface_save;

	hard_disk->hard_disk = NULL;
	hard_disk->chd = NULL;
	encode_image_ref(stream, encoded_image_ref);

	/* use our CHD interface */
	chd_save_interface(&interface_save);
	chd_set_interface(&imgtool_chd_interface);

	chderr = chd_open(encoded_image_ref, stream_isreadonly(stream) ? CHD_OPEN_READ : CHD_OPEN_READWRITE, NULL, &hard_disk->chd);
	if (chderr)
	{
		err = map_chd_error(chderr);
		goto done;
	}

	hard_disk->hard_disk = hard_disk_open(hard_disk->chd);
	if (!hard_disk->hard_disk)
	{
		err = IMGTOOLERR_UNEXPECTED;
		goto done;
	}
	hard_disk->stream = stream;

done:
	if (err)
		imghd_close(hard_disk);
	chd_set_interface(&interface_save);
	return err;
}



/*
	imghd_close()

	Close MAME HD image
*/
void imghd_close(struct mess_hard_disk_file *disk)
{
	chd_interface interface_save;

	chd_save_interface(&interface_save);
	chd_set_interface(&imgtool_chd_interface);

	if (disk->hard_disk)
	{
		hard_disk_close(disk->hard_disk);
		disk->hard_disk = NULL;
	}
	if (disk->chd)
	{
		chd_close(disk->chd);
		disk->chd = NULL;
	}
	if (disk->stream)
		stream_close(disk->stream);

	chd_set_interface(&interface_save);
}



/*
	imghd_read()

	Read sector(s) from MAME HD image
*/
imgtoolerr_t imghd_read(struct mess_hard_disk_file *disk, UINT32 lbasector, void *buffer)
{
	chd_interface interface_save;
	UINT32 reply;

	chd_save_interface(&interface_save);
	chd_set_interface(&imgtool_chd_interface);
	reply = hard_disk_read(disk->hard_disk, lbasector, buffer);
	chd_set_interface(&interface_save);

	return reply ? IMGTOOLERR_SUCCESS : map_chd_error(reply);
}



/*
	imghd_write()

	Write sector(s) from MAME HD image
*/
imgtoolerr_t imghd_write(struct mess_hard_disk_file *disk, UINT32 lbasector, const void *buffer)
{
	chd_interface interface_save;
	UINT32 reply;

	chd_save_interface(&interface_save);
	chd_set_interface(&imgtool_chd_interface);
	reply = hard_disk_write(disk->hard_disk, lbasector, buffer);
	chd_set_interface(&interface_save);

	return reply ? IMGTOOLERR_SUCCESS : map_chd_error(reply);
}



/*
	imghd_get_header()

	Return pointer to the header of MAME HD image
*/
const hard_disk_info *imghd_get_header(struct mess_hard_disk_file *disk)
{
	chd_interface interface_save;
	const hard_disk_info *reply;

	chd_save_interface(&interface_save);
	chd_set_interface(&imgtool_chd_interface);
	reply = hard_disk_get_info(disk->hard_disk);
	chd_set_interface(&interface_save);

	return reply;
}


static imgtoolerr_t mess_hd_image_create(imgtool_image *image, imgtool_stream *f, option_resolution *createoptions);

enum
{
	mess_hd_createopts_blocksize = 'B',
	mess_hd_createopts_cylinders = 'C',
	mess_hd_createopts_heads     = 'D',
	mess_hd_createopts_sectors   = 'E',
	mess_hd_createopts_seclen    = 'F'
};

OPTION_GUIDE_START( mess_hd_create_optionguide )
	OPTION_INT(mess_hd_createopts_blocksize, "blocksize", "Sectors Per Block" )
	OPTION_INT(mess_hd_createopts_cylinders, "cylinders", "Cylinders" )
	OPTION_INT(mess_hd_createopts_heads, "heads",	"Heads" )
	OPTION_INT(mess_hd_createopts_sectors, "sectors", "Total Sectors" )
	OPTION_INT(mess_hd_createopts_seclen, "seclen", "Sector Bytes" )
OPTION_GUIDE_END

#define mess_hd_create_optionspecs "B[1]-2048;C1-[32]-65536;D1-[8]-64;E1-[128]-4096;F128/256/[512]/1024/2048/4096/8192/16384/32768/65536"


void mess_hd_get_info(const imgtool_class *imgclass, UINT32 state, union imgtoolinfo *info)
{
	switch(state)
	{
		case IMGTOOLINFO_STR_NAME:							strcpy(info->s = imgtool_temp_str(), "mess_hd"); break;
		case IMGTOOLINFO_STR_DESCRIPTION:					strcpy(info->s = imgtool_temp_str(), "MESS hard disk image"); break;
		case IMGTOOLINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = imgtool_temp_str(), "hd"); break;

		case IMGTOOLINFO_PTR_CREATE:						info->create = mess_hd_image_create; break;

		case IMGTOOLINFO_PTR_CREATEIMAGE_OPTGUIDE:			info->createimage_optguide = mess_hd_create_optionguide; break;
		case IMGTOOLINFO_STR_CREATEIMAGE_OPTSPEC:			strcpy(info->s = imgtool_temp_str(), mess_hd_create_optionspecs); break;
	}
}



static imgtoolerr_t mess_hd_image_create(imgtool_image *image, imgtool_stream *f, option_resolution *createoptions)
{
	UINT32  blocksize, cylinders, heads, sectors, seclen;

	/* read options */
	blocksize = option_resolution_lookup_int(createoptions, mess_hd_createopts_blocksize);
	cylinders = option_resolution_lookup_int(createoptions, mess_hd_createopts_cylinders);
	heads = option_resolution_lookup_int(createoptions, mess_hd_createopts_heads);
	sectors = option_resolution_lookup_int(createoptions, mess_hd_createopts_sectors);
	seclen = option_resolution_lookup_int(createoptions, mess_hd_createopts_seclen);

	return imghd_create(f, blocksize, cylinders, heads, sectors, seclen);
}
