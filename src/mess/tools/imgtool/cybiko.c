/*

	Cybiko File System

	(c) 2007 Tim Schuerewegen

*/

#include "imgtool.h"

#include <zlib.h>

#ifndef FALSE
#	 define FALSE 0
#endif
#ifndef TRUE
#	 define TRUE (!FALSE)
#endif

typedef struct _cybiko_file_system cybiko_file_system;
struct _cybiko_file_system
{
	imgtool_stream *stream;
	UINT32 page_count, page_size, block_count;
	UINT16 write_count;
};

typedef struct _cybiko_iter cybiko_iter;
struct _cybiko_iter
{
	UINT16 block;
};

typedef struct _cfs_file cfs_file;
struct _cfs_file
{
	char name[64]; // name of the file
	UINT32 date;   // date/time of the file (seconds since 1900/01/01)
	UINT32 size;   // size of the file
	UINT32 blocks; // number of blocks occupied by the file
};

enum
{
	PAGE_TYPE_BOOT = 1,
	PAGE_TYPE_FILE
};

#define	MAX_PAGE_SIZE (264 * 2)

#define INVALID_FILE_ID  0xFFFF

#define AT45DB041_SIZE  0x084000
#define AT45DB081_SIZE  0x108000
#define AT45DB161_SIZE  0x210000

#define BLOCK_USED(x)      (x[0] & 0x80)
#define BLOCK_FILE_ID(x)   cfs_mem_read_16( x + 2)
#define BLOCK_PART_ID(x)   cfs_mem_read_16( x + 4)
#define BLOCK_FILENAME(x)  (char*)(x + 7)

#define FILE_HEADER_SIZE  0x48

// 2208988800 is the number of seconds between 1900/01/01 and 1970/01/01

static time_t cfs_time_crack( UINT32 cfs_time)
{
	return (time_t)(cfs_time - 2208988800UL);
}

static UINT32 cfs_time_setup( time_t ansi_time)
{
	return (UINT32)(ansi_time + 2208988800UL);
}

static UINT32 cfs_mem_read_32( UINT8 *buffer)
{
	return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | (buffer[3] << 0);
}

static UINT16 cfs_mem_read_16( UINT8 *buffer)
{
	return (buffer[0] << 8) | (buffer[1] << 0);
}

static void cfs_mem_write_32( UINT8 *buffer, UINT32 data)
{
	buffer[0] = (data >> 24) & 0xFF;
	buffer[1] = (data >> 16) & 0xFF;
	buffer[2] = (data >>  8) & 0xFF;
	buffer[3] = (data >>  0) & 0xFF;
}

static void cfs_mem_write_16( UINT8 *buffer, UINT16 data)
{
	buffer[0] = (data >> 8) & 0xFF;
	buffer[1] = (data >> 0) & 0xFF;
}

// page = crc1 (4) + wcnt (2) + crc2 (2) + data (x) + unk (2)

static UINT32 cfs_page_calc_checksum_1( UINT8 *buffer, UINT32 size, int type)
{
	return crc32( 0, buffer + 8, (type == PAGE_TYPE_BOOT) ? 250 : size - 10);
}

static UINT16 cfs_page_calc_checksum_2( UINT8 *buffer)
{
	UINT16 val = 0xAF17;
	val ^= cfs_mem_read_16( buffer + 0);
	val ^= cfs_mem_read_16( buffer + 2);
	val ^= cfs_mem_read_16( buffer + 4);
	return FLIPENDIAN_INT16(val);
}

static bool cfs_page_verify( UINT8 *buffer, UINT32 size, int type)
{
	UINT32 checksum_page, checksum_calc;
	// checksum 1
	checksum_calc = cfs_page_calc_checksum_1( buffer, size, type);
	checksum_page = cfs_mem_read_32( buffer + 0);
	if (checksum_calc != checksum_page) return FALSE;
	// checksum 2
	checksum_calc = cfs_page_calc_checksum_2( buffer);
	checksum_page = cfs_mem_read_16( buffer + 6);
	if (checksum_calc != checksum_page) return FALSE;
	// ok
	return TRUE;
}

static void cfs_page_read( cybiko_file_system *cfs, UINT8 *buffer, UINT32 page)
{
	stream_seek( cfs->stream, page * cfs->page_size, SEEK_SET);
	stream_read( cfs->stream, buffer, cfs->page_size);
}

static void cfs_page_write( cybiko_file_system *cfs, UINT8 *buffer, UINT32 page)
{
	stream_seek( cfs->stream, page * cfs->page_size, SEEK_SET);
	stream_write( cfs->stream, buffer, cfs->page_size);
}

static void cfs_block_read( cybiko_file_system *cfs, UINT8 *buffer, UINT32 block)
{
	UINT8 buffer_page[MAX_PAGE_SIZE];
	cfs_page_read( cfs, buffer_page, block + 5);
	memcpy( buffer, buffer_page + 8, cfs->page_size - 10);
}

static void cfs_block_write( cybiko_file_system *cfs, UINT8 *buffer, UINT32 block)
{
	UINT8 buffer_page[MAX_PAGE_SIZE];
	memcpy( buffer_page + 8, buffer, cfs->page_size - 10);
	cfs_mem_write_32( buffer_page + 0, cfs_page_calc_checksum_1( buffer_page, cfs->page_size, PAGE_TYPE_FILE));
	cfs_mem_write_16( buffer_page + 4, cfs->write_count++);
	cfs_mem_write_16( buffer_page + 6, cfs_page_calc_checksum_2( buffer_page));
	cfs_mem_write_16( buffer_page + cfs->page_size - 2, 0xFFFF);
	cfs_page_write( cfs, buffer_page, block + 5);
}

static void cfs_file_delete( cybiko_file_system *cfs, UINT16 file_id)
{
	UINT8 buffer[MAX_PAGE_SIZE];
	int i;
	for (i=0;i<cfs->block_count;i++)
	{
		cfs_block_read( cfs, buffer, i);
		if (BLOCK_USED(buffer) && (BLOCK_FILE_ID(buffer) == file_id))
		{
			buffer[0] &= ~0x80;
			cfs_block_write( cfs, buffer, i);
		}
	}
}

static bool cfs_file_info( cybiko_file_system *cfs, UINT16 file_id, cfs_file *file)
{
	UINT8 buffer[MAX_PAGE_SIZE];
	int i;
	file->blocks = file->size = 0;
	for (i=0;i<cfs->block_count;i++)
	{
		cfs_block_read( cfs, buffer, i);
		if (BLOCK_USED(buffer) && (BLOCK_FILE_ID(buffer) == file_id))
		{
			if (BLOCK_PART_ID(buffer) == 0)
			{
				strcpy( file->name, BLOCK_FILENAME(buffer));
				file->date = cfs_mem_read_32( buffer + 6 + FILE_HEADER_SIZE - 4);
			}
			file->size += buffer[1];
			file->blocks++;
		}
	}
	return (file->blocks > 0) ? TRUE : FALSE;
}

static bool cfs_file_find( cybiko_file_system *cfs, const char *filename, UINT16 *file_id)
{
	UINT8 buffer[MAX_PAGE_SIZE];
	int i;
	for (i=0;i<cfs->block_count;i++)
	{
		cfs_block_read( cfs, buffer, i);
		if (BLOCK_USED(buffer) && (strncmp( filename, BLOCK_FILENAME(buffer), 40) == 0))
		{
			*file_id = i;
			return TRUE;
		}
	}
	return FALSE;
}

static bool cfs_verify( cybiko_file_system *cfs)
{
	UINT8 buffer[MAX_PAGE_SIZE];
	int i;
	for (i=0;i<cfs->page_count;i++)
	{
		cfs_page_read( cfs, buffer, i);
		if (!cfs_page_verify( buffer, cfs->page_size, (i < 5) ? PAGE_TYPE_BOOT : PAGE_TYPE_FILE)) return FALSE;
	}
	return TRUE;
}

static bool cfs_init( cybiko_file_system *cfs, imgtool_stream *stream)
{
	cfs->stream = stream;
	switch (stream_size( stream))
	{
		case AT45DB041_SIZE : cfs->page_count = 2048; cfs->page_size = 264; break;
		case AT45DB081_SIZE : cfs->page_count = 4096; cfs->page_size = 264; break;
		case AT45DB161_SIZE : cfs->page_count = 4096; cfs->page_size = 528; break;
		default             : return FALSE;
	}
	cfs->block_count = cfs->page_count - 5;
	cfs->write_count = 0;
	return TRUE;
}

static UINT16 cfs_calc_free_blocks( cybiko_file_system *cfs)
{
	UINT8 buffer[MAX_PAGE_SIZE];
	int i;
	UINT16 blocks = 0;
	for (i=0;i<cfs->block_count;i++)
	{
		cfs_block_read( cfs, buffer, i);
		if (!BLOCK_USED(buffer)) blocks++;
	}
	return blocks;
}

static UINT32 cfs_calc_free_space( cybiko_file_system *cfs, UINT16 blocks)
{
	UINT32 free_space;
	free_space = blocks * (cfs->page_size - 0x10);
	if (free_space > 0) free_space -= FILE_HEADER_SIZE;
	return free_space;
}

static imgtoolerr_t cybiko_image_open( imgtool_image *image, imgtool_stream *stream)
{
	cybiko_file_system *cfs = (cybiko_file_system*)imgtool_image_extra_bytes( image);
	// init
	if (!cfs_init( cfs, stream)) return IMGTOOLERR_CORRUPTIMAGE;
	// verify
	if (!cfs_verify( cfs)) return IMGTOOLERR_CORRUPTIMAGE;
	// ok
	return IMGTOOLERR_SUCCESS;
}

static void cybiko_image_close( imgtool_image *image)
{
	cybiko_file_system *cfs = (cybiko_file_system*)imgtool_image_extra_bytes( image);
	stream_close( cfs->stream);
}

/*
static imgtoolerr_t cybiko_image_create( imgtool_image *image, imgtool_stream *stream, option_resolution *opts)
{
	return IMGTOOLERR_UNIMPLEMENTED;
}
*/

static imgtoolerr_t cybiko_image_begin_enum( imgtool_directory *enumeration, const char *path)
{
	cybiko_iter *iter = (cybiko_iter*)imgtool_directory_extrabytes( enumeration);
	iter->block = 0;
	return IMGTOOLERR_SUCCESS;
}

static imgtoolerr_t cybiko_image_next_enum( imgtool_directory *enumeration, imgtool_dirent *ent)
{
	imgtool_image *image = imgtool_directory_image( enumeration);
	cybiko_file_system *cfs = (cybiko_file_system*)imgtool_image_extra_bytes( image);
	cybiko_iter *iter = (cybiko_iter*)imgtool_directory_extrabytes( enumeration);
	UINT8 buffer[MAX_PAGE_SIZE];
	UINT16 file_id = INVALID_FILE_ID;
	cfs_file file;
	// find next file
	while (iter->block < cfs->block_count)
	{
		cfs_block_read( cfs, buffer, iter->block++);
		if (BLOCK_USED(buffer) && (BLOCK_PART_ID(buffer) == 0))
		{
			file_id = BLOCK_FILE_ID(buffer);
			break;
		}
	}
	// get file information
	if ((file_id != INVALID_FILE_ID) && cfs_file_info( cfs, file_id, &file))
	{
		strcpy( ent->filename, file.name);
		ent->filesize = file.size;
		ent->lastmodified_time = cfs_time_crack( file.date);
		ent->filesize = file.size;
	}
	else
	{
		ent->eof = 1;
	}
	// ok
	return IMGTOOLERR_SUCCESS;
}

static void cybiko_image_close_enum( imgtool_directory *enumeration)
{
	// nothing
}

static imgtoolerr_t cybiko_image_free_space( imgtool_partition *partition, UINT64 *size)
{
	imgtool_image *image = imgtool_partition_image( partition);
	cybiko_file_system *cfs = (cybiko_file_system*)imgtool_image_extra_bytes( image);
	if (size) *size = cfs_calc_free_space( cfs, cfs_calc_free_blocks( cfs));
	return IMGTOOLERR_SUCCESS;
}

static imgtoolerr_t cybiko_image_read_file( imgtool_partition *partition, const char *filename, const char *fork, imgtool_stream *destf)
{
	imgtool_image *image = imgtool_partition_image( partition);
	cybiko_file_system *cfs = (cybiko_file_system*)imgtool_image_extra_bytes( image);
	UINT8 buffer[MAX_PAGE_SIZE];
	UINT16 file_id, part_id = 0, old_part_id;
	int i;
	// find file
	if (!cfs_file_find( cfs, filename, &file_id)) return IMGTOOLERR_FILENOTFOUND;
	// read file
	do
	{
		old_part_id = part_id;
		for (i=0;i<cfs->block_count;i++)
		{
			cfs_block_read( cfs, buffer, i);
			if (BLOCK_USED(buffer) && (BLOCK_FILE_ID(buffer) == file_id) && (BLOCK_PART_ID(buffer) == part_id))
			{
				stream_write( destf, buffer + 6 + ((part_id == 0) ? FILE_HEADER_SIZE : 0), buffer[1]);
				part_id++;
			}
		}
	} while (old_part_id != part_id);
	// ok
	return IMGTOOLERR_SUCCESS;
}

static imgtoolerr_t cybiko_image_write_file( imgtool_partition *partition, const char *filename, const char *fork, imgtool_stream *sourcef, option_resolution *opts)
{
	imgtool_image *image = imgtool_partition_image( partition);
	cybiko_file_system *cfs = (cybiko_file_system*)imgtool_image_extra_bytes( image);
	UINT8 buffer[MAX_PAGE_SIZE];
	UINT16 file_id, part_id = 0, free_blocks;
	UINT64 bytes_left;
	cfs_file file;
	int i;
	// find file
	if (!cfs_file_find( cfs, filename, &file_id)) file_id = INVALID_FILE_ID;
	// check free space
	free_blocks = cfs_calc_free_blocks( cfs);
	if ((file_id != INVALID_FILE_ID) && cfs_file_info( cfs, file_id, &file)) free_blocks += file.blocks;
	if (cfs_calc_free_space( cfs, free_blocks) < stream_size( sourcef)) return IMGTOOLERR_NOSPACE;
	// delete file
	if (file_id != INVALID_FILE_ID) cfs_file_delete( cfs, file_id);
	// create/write destination file
	bytes_left = stream_size( sourcef);
	i = 0;
	while ((bytes_left > 0) && (i < cfs->block_count))
	{
		cfs_block_read( cfs, buffer, i);
		if (!BLOCK_USED(buffer))
		{
			if (part_id == 0) file_id = i;
			memset( buffer, 0xFF, cfs->page_size - 0x10);
			buffer[0] = 0x80;
			buffer[1] = cfs->page_size - 0x10 - ((part_id == 0) ? FILE_HEADER_SIZE : 0);
			if (bytes_left < buffer[1]) buffer[1] = bytes_left;
			cfs_mem_write_16( buffer + 2, file_id);
			cfs_mem_write_16( buffer + 4, part_id);
			if (part_id == 0)
			{
				buffer[6] = 0;
				strcpy( BLOCK_FILENAME(buffer), filename);
				cfs_mem_write_32( buffer + 6 + FILE_HEADER_SIZE - 4, cfs_time_setup( time( NULL)));
				stream_read( sourcef, buffer + 6 + FILE_HEADER_SIZE, buffer[1]);
			}
			else
			{
				stream_read( sourcef, buffer + 6, buffer[1]);
			}
			cfs_block_write( cfs, buffer, i);
			bytes_left -= buffer[1];
			part_id++;
		}
		i++;
	}
	// ok
	return IMGTOOLERR_SUCCESS;
}

static imgtoolerr_t cybiko_image_delete_file( imgtool_partition *partition, const char *filename)
{
	imgtool_image *image = imgtool_partition_image( partition);
	cybiko_file_system *cfs = (cybiko_file_system*)imgtool_image_extra_bytes( image);
	UINT16 file_id;
	// find file
	if (!cfs_file_find( cfs, filename, &file_id)) return IMGTOOLERR_FILENOTFOUND;
	// delete file
	cfs_file_delete( cfs, file_id);
	// ok
	return IMGTOOLERR_SUCCESS;
}

void cybiko_get_info( const imgtool_class *imgclass, UINT32 state, union imgtoolinfo *info)
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case IMGTOOLINFO_INT_IMAGE_EXTRA_BYTES          : info->i = sizeof( cybiko_file_system); break;
		case IMGTOOLINFO_INT_DIRECTORY_EXTRA_BYTES      : info->i = sizeof( cybiko_iter); break;
//		case IMGTOOLINFO_INT_SUPPORTS_CREATION_TIME     : info->i = 1; break;
		case IMGTOOLINFO_INT_SUPPORTS_LASTMODIFIED_TIME : info->i = 1; break;
		case IMGTOOLINFO_INT_BLOCK_SIZE                 : info->i = 264; break;
		// --- the following bits of info are returned as pointers to data or functions ---
		case IMGTOOLINFO_PTR_OPEN        : info->open        = cybiko_image_open; break;
//		case IMGTOOLINFO_PTR_CREATE      : info->create      = cybiko_image_create; break;
		case IMGTOOLINFO_PTR_CLOSE       : info->close       = cybiko_image_close; break;
		case IMGTOOLINFO_PTR_BEGIN_ENUM  : info->begin_enum  = cybiko_image_begin_enum; break;
		case IMGTOOLINFO_PTR_NEXT_ENUM   : info->next_enum   = cybiko_image_next_enum; break;
		case IMGTOOLINFO_PTR_CLOSE_ENUM  : info->close_enum  = cybiko_image_close_enum; break;
		case IMGTOOLINFO_PTR_FREE_SPACE  : info->free_space  = cybiko_image_free_space; break;
		case IMGTOOLINFO_PTR_READ_FILE   : info->read_file   = cybiko_image_read_file; break;
		case IMGTOOLINFO_PTR_WRITE_FILE  : info->write_file  = cybiko_image_write_file; break;
		case IMGTOOLINFO_PTR_DELETE_FILE : info->delete_file = cybiko_image_delete_file; break;
		// --- the following bits of info are returned as NULL-terminated strings ---
		case IMGTOOLINFO_STR_NAME            : strcpy( info->s = imgtool_temp_str(), "cybiko"); break;
		case IMGTOOLINFO_STR_DESCRIPTION     : strcpy( info->s = imgtool_temp_str(), "Cybiko File System"); break;
		case IMGTOOLINFO_STR_FILE            : strcpy( info->s = imgtool_temp_str(), __FILE__); break;
		case IMGTOOLINFO_STR_FILE_EXTENSIONS : strcpy( info->s = imgtool_temp_str(), "bin"); break;
		case IMGTOOLINFO_STR_EOLN            : strcpy( info->s = imgtool_temp_str(), "\r\n"); break;
	}
}
