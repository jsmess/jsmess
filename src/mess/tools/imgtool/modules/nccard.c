#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "osdepend.h"
#include "imgtoolx.h"

/* NC Card image handling code by Kevin Thacker. February 2001 */

static int nc_card_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg);
static void nc_card_image_exit(imgtool_image *img);
static size_t nc_card_image_freespace(imgtool_image *img);
static int nc_card_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf);
static int nc_card_image_writefile(imgtool_image *img, const char *fname, imgtool_stream *sourcef, const ResolvedOption *options_);
static int nc_card_image_deletefile(imgtool_image *img, const char *fname);
static int nc_card_image_create(const imgtool_module *mod, imgtool_stream *f, const ResolvedOption *options_);
static int nc_card_image_beginenum(imgtool_image *img, imgtool_directory **outenum);
static int nc_card_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent);
static void nc_card_image_closeenum(imgtool_directory *enumeration);

IMAGEMODULE(
	nccard,
	"NC100/200 PCMCIA Ram Card Image",			/* human readable name */
	"crd",							/* file extension */
	NULL,							/* crcfile */
	NULL,							/* crc system name */
	EOLN_CRLF,						/* eoln */
	IMGMODULE_FLAG_FILENAMES_PREFERUCASE,	/* flags */
	nc_card_image_init,				/* init function */
	nc_card_image_exit,				/* exit function */
	NULL,							/* info function */
	nc_card_image_beginenum,		/* begin enumeration */
	nc_card_image_nextenum,			/* enumerate next */
	nc_card_image_closeenum,		/* close enumeration */
	nc_card_image_freespace,		/* free space on image    */
	nc_card_image_readfile,			/* read file */
	nc_card_image_writefile,		/* write file */
	nc_card_image_deletefile,		/* delete file */
	nc_card_image_create,			/* create image */
	NULL,
	NULL,
	NULL,							/* file options */
	NULL							/* create options */
)

/*
	NC PCMCIA RAM Card uses a FAT-like filesystem 

	Blocks are fixed at 256 bytes in size. 
	
	Block 0 and Block 1 are reserved for a boot program.

	Block 2 is reserved for root directory.

	Block 3..(3+num_fat_blocks-1) are reserved for fat table.

	Block (3+num_fat_blocks).. are used for data storage.

	The FAT table uses two bytes for each entry.

	The blocks used by a file are stored in the table in a link-list
	structure.

	The index of the first block is stored in the directory entry.
	This entry is looked up in the FAT table. If the entry contains
	another number (other than 0x0ffff and 0x0fffe), this is the next
	block used by the file. If the number is 0x0ffff this means this
	block is the last used by the file.
	
	0x0fffe indicates a reserved block.

	The maximum supported PCMCIA Ram card size is 1mb, and there are
	4096 256-byte blocks on each card. Therefore a FAT entry uses 12-bits
	to store the block index, leaving the top 4-bits as a status??
	
	Maximum size of a file on the PCMCIA Ram card is 65536 bytes.
*/
struct nc_card_dir_entry
{
	/* filename, up to 12 chars, any unused chars are padded with zero's */
	/* if first char is 0, this entry has been deleted */
	unsigned char filename[12];
	/* unused?? */
	unsigned char unused[1];
	/* 5 = basic, 0 = ascii text, 3=directory entry? */
	unsigned char type;
	/* size of file in bytes */
	unsigned char size_low_byte;
	unsigned char size_high_byte;
	/* format unknown */
	unsigned char packed_date[4];
	/* 2-byte block index. first block used by file, other blocks
	found in fat table */
	unsigned char first_block_low;
	unsigned char first_block_high;
	/* unused?? */
	unsigned char pad[32 - 22];
};

#define NC_FAT_END_OF_LIST	0x0ffff
#define NC_FAT_RESERVED		0x0fffe

struct nc_memcard
{
	unsigned char *memcard_data;
	unsigned long memcard_size;
	int fat_num_blocks;
	int max_block_index;
};

/* each block is 256 bytes in size */
#define NC_BLOCK_SIZE	256
/* fat begins at block 3 */
#define NC_FAT_BLOCK 3

#define NC_DIR_ENTRY_TYPE_ASCII_FILE 0
#define NC_DIR_ENTRY_TYPE_BASIC_FILE 5
#define NC_DIR_ENTRY_TYPE_DIR 3


static int	memcard_init(struct nc_memcard *memcard, int size)
{
	/* set size of memcard */
	memcard->memcard_size = size;
	/* allocate memory for it */
	memcard->memcard_data = malloc(size);

	if (memcard->memcard_data==0)
		return 0;

	/* calculate maximum number of blocks for fat to use */
	memcard->max_block_index = (memcard->memcard_size + NC_BLOCK_SIZE-1)/NC_BLOCK_SIZE;

	memcard->fat_num_blocks = ((memcard->max_block_index<<1) + NC_BLOCK_SIZE-1)/NC_BLOCK_SIZE;

	return 1;
}

static void	memcard_free(struct nc_memcard *memcard)
{
	if (memcard->memcard_data!=NULL)
	{
		free(memcard->memcard_data);
		memcard->memcard_data = NULL;
	}
}


/* write to fat */
static void	memcard_fat_write(struct nc_memcard *memcard,int index1, int data)
{
	unsigned long offset;

	offset = (NC_FAT_BLOCK*NC_BLOCK_SIZE) + (index1<<1);

	memcard->memcard_data[offset] = data & 0x0ff;
	memcard->memcard_data[offset+1] = (data>>8);
}

/* read from fat */
static int	memcard_fat_read(struct nc_memcard *memcard,int index1)
{
	int data;
	unsigned long offset;

	offset = (NC_FAT_BLOCK*NC_BLOCK_SIZE) + (index1<<1);

	data = (memcard->memcard_data[offset] & 0x0ff) | ((memcard->memcard_data[offset+1] & 0x0ff)<<8);

	return data;
}

/* get block index stored at the specified block index */
static int memcard_fat_get_next_block(struct nc_memcard *memcard,int index1)
{
	return memcard_fat_read(memcard,index1);
}

/* get index of free block */
static int	memcard_fat_get_free_block(struct nc_memcard *memcard)
{
	int i;

	for (i=0; i<memcard->max_block_index; i++)
	{
		if (memcard_fat_read(memcard,i)==0)
		{
			return i;
		}
	}
	return -1;
}


static unsigned long memcard_block_to_offset(int block)
{
	return block*NC_BLOCK_SIZE;
}


/* returns offset to directory entry */
static unsigned long memcard_get_free_dir_entry(struct nc_memcard *memcard)
{
	/* offset within memcard data of this directory entry inside the parent */
	int offset_in_parent_dir = NC_BLOCK_SIZE*2;
	int previous_block;
	int current_block = 2;
	unsigned long offset;

	while (current_block!=NC_FAT_END_OF_LIST)
	{
		int i;
		offset = memcard_block_to_offset(current_block);

		for (i=0; i<(NC_BLOCK_SIZE/32); i++)
		{
			if (memcard->memcard_data[offset]==0)
			{
				return offset;
			}
		
			offset+=32;
		}

		previous_block = current_block;
		current_block = memcard_fat_get_next_block(memcard,current_block);
		
		if (current_block!=NC_FAT_END_OF_LIST)
		{
			current_block &= 0x0fff;
		}
	}

	/* out of entries within current blocks occupied by directory.
	try to expand directory by one more block */

	current_block = memcard_fat_get_free_block(memcard);

	if (current_block!=-1)
	{
		int size;
		unsigned long block_offset;
		struct nc_card_dir_entry *dir_entry;

		/* update size of directory */
		dir_entry = (struct nc_card_dir_entry *)&memcard->memcard_data[offset_in_parent_dir];

		/* get size of file from directory entry */
		size = ((dir_entry->size_high_byte & 0x0ff)<<8) | (dir_entry->size_low_byte & 0x0ff);

		size+=NC_BLOCK_SIZE;

		/* set size of file in directory entry */
		dir_entry->size_high_byte = (size>>8) & 0x0ff;
		dir_entry->size_low_byte = size & 0x0ff;

		/* got a new block */
		block_offset = memcard_block_to_offset(current_block);

		/* clear block */
		/* only really necessary to clear first byte of each dir entry - but I'll clear
		it all to give decent compressable cards */
		memset(&memcard->memcard_data[block_offset], 0, NC_BLOCK_SIZE);

		/* setup fat */
		memcard_fat_write(memcard,previous_block, current_block|0x02000);
		memcard_fat_write(memcard,current_block, NC_FAT_END_OF_LIST);

		/* return first offset in new block */
		return block_offset;
	}

	return -1;
}

const char *nc_memcard_executable_id = "NC100PRG";

/* if memory card has "NC100PRG" at 0x0200 then the card is executable! */
static int memcard_is_executable(struct nc_memcard *memcard)
{
	if (memcmp(&memcard->memcard_data[0x0200], nc_memcard_executable_id, 8)==0)
	{
		return 1;
	}

	return 0;
}


/* format a blank card */
static void	memcard_format(struct nc_memcard *memcard)
{
	int i;

	/* initialise all to zero */
	/* real NC100 does not do this */
	memset(memcard->memcard_data, 0, memcard->memcard_size);

	/* not executable */
	/* real NC100 does not do this */
	memcard->memcard_data[0] = 0x0c9;

	/* mark boot block as used */
	memcard_fat_write(memcard,0, NC_FAT_END_OF_LIST);
	memcard_fat_write(memcard,1, NC_FAT_END_OF_LIST);
	/* mark directory block as used */
	memcard_fat_write(memcard,2, NC_FAT_END_OF_LIST);

	/* mark fat blocks as used */
	for (i=0; i<memcard->fat_num_blocks; i++)
	{
		memcard_fat_write(memcard,3+i, NC_FAT_END_OF_LIST);
	}

	/* reserved at end of fat table - required?? */
	memcard_fat_write(memcard,memcard->max_block_index-1, NC_FAT_RESERVED);

	{
		/* format for NC100 */
		unsigned long dir_entry_offset = memcard_get_free_dir_entry(memcard);
		int dir_size = NC_BLOCK_SIZE;
		struct nc_card_dir_entry *dir_entry;

		dir_entry = (struct nc_card_dir_entry *)&memcard->memcard_data[dir_entry_offset];

		strcpy((char *)dir_entry->filename,(char *)"NC100");
		dir_entry->size_high_byte = (dir_size>>8) & 0x0ff;
		dir_entry->size_low_byte = (dir_size & 0x0ff);
		dir_entry->first_block_high = 0x020;
		dir_entry->first_block_low = 0x02;
		dir_entry->type = NC_DIR_ENTRY_TYPE_DIR;
	}
}


static unsigned long memcard_locate_dir_entry(struct nc_memcard *memcard,char *pNCFilename)
{
	int current_block = 2;
	unsigned long offset;

	while (current_block!=NC_FAT_END_OF_LIST)
	{
		int i;

		offset = memcard_block_to_offset(current_block);

		for (i=0; i<(NC_BLOCK_SIZE/32); i++)
		{
			if (memcard->memcard_data[offset]!=0)
			{
				if (memcmp(&memcard->memcard_data[offset], pNCFilename,12)==0)
				{
					return offset;
				}
			}
		
			offset+=32;
		}

		current_block = memcard_fat_get_next_block(memcard,current_block);
		
		if (current_block!=NC_FAT_END_OF_LIST)
		{
			current_block = current_block & 0x0fff;
		}
	}

	return -1;
}

static int	memcard_del(struct nc_memcard *memcard,char *pNCFilename)
{
	unsigned long dir_entry_offset = memcard_locate_dir_entry(memcard,pNCFilename);
	int success_code = 0;

	if (dir_entry_offset!=-1)
	{
		int current_block;
		struct nc_card_dir_entry *dir_entry;

		dir_entry = (struct nc_card_dir_entry *)&memcard->memcard_data[dir_entry_offset];

		/* get first block index from directory entry */
		current_block = ((dir_entry->first_block_high & 0x0f)<<8) | dir_entry->first_block_low;

		while (current_block!=0)
		{
			int next_block;
		
			/* get next block in list */
			next_block = memcard_fat_get_next_block(memcard,current_block);
			/* zero out this block */
			memcard_fat_write(memcard,current_block,0);
			
			/* go to next block */
			current_block = next_block;

			if (current_block!=NC_FAT_END_OF_LIST)
			{
				current_block &= 0x0fff;
			}
		}

		/* clear first char of filename */
		dir_entry->filename[0] = '\0';
		success_code = 1;
	}

	return success_code;
}


struct nc_card_image
{
	imgtool_image base;
	imgtool_stream *file_handle;
	struct nc_memcard memcard;
};

static int nc_card_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg)
{
	struct nc_card_image *nc_card;

	*outimg = NULL;

	nc_card = (struct nc_card_image *)malloc(sizeof(struct nc_card_image));

	if (!nc_card)
	{
		return IMGTOOLERR_OUTOFMEMORY;
	}

	nc_card->file_handle = f;
	nc_card->base.module = mod;

	/* allocate a big chunk of memory for memcard data - max size will be 1mb! */
	if (!memcard_init(&nc_card->memcard, stream_size(f)))
	{
		return IMGTOOLERR_OUTOFMEMORY;
	}

	/* read data into memcard */
	if (stream_read(f, nc_card->memcard.memcard_data, nc_card->memcard.memcard_size)!=nc_card->memcard.memcard_size)
	{
		memcard_free(&nc_card->memcard);

		return IMGTOOLERR_READERROR;
	}

	*outimg = (imgtool_image *)nc_card;
	return 0;
}

static void nc_card_image_exit(imgtool_image *img)
{
	struct nc_card_image *nc_card = (struct nc_card_image *)img;

	stream_seek(nc_card->file_handle, 0, SEEK_SET);
	stream_write(nc_card->file_handle, nc_card->memcard.memcard_data, nc_card->memcard.memcard_size);
	stream_close(nc_card->file_handle);

	memcard_free(&nc_card->memcard);
}


/* count number of free blocks and calculate freespace */
static size_t nc_card_image_freespace(imgtool_image *img)
{
	struct nc_card_image *nc_card = (struct nc_card_image *)img;
	int i;
	int count;

	count = 0;
	for (i=0; i<nc_card->memcard.max_block_index; i++)
	{
		if (memcard_fat_read(&nc_card->memcard,i)==0)
		{
			count++;
		}
	}
	return count*NC_BLOCK_SIZE;
}

/* create a empty image */
/* in this case a formatted image */
static int nc_card_image_create(const imgtool_module *mod, imgtool_stream *f, const ResolvedOption *options_)
{
	int code;

	struct nc_memcard memcard;
	
	/* allocate a big chunk of memory for memcard data - max size will be 1mb! */
	if (!memcard_init(&memcard, 1024*1024))
	{
		return IMGTOOLERR_OUTOFMEMORY;
	}

	/* format the card */
	memcard_format(&memcard);

	/* write out */
	code = 0;
	if (stream_write(f, memcard.memcard_data, memcard.memcard_size) != memcard.memcard_size)
	{
		/* write error! */
		code = IMGTOOLERR_WRITEERROR;
	}

	/* free memcard data */
	memcard_free(&memcard);

	/* return success or failure code */
	return code;
}

static void setup_nc_filename(char *nc_filename, const char *fname)
{
	char ch;
	int offset;

	memset(nc_filename, 0, 12);

	offset = 0;
	do
	{
		/* get char */
		ch = fname[offset];

		if (ch!=0)
		{
			/* convert to upper case */
			ch = toupper(ch);
			nc_filename[offset] = ch;
			offset++;
		}
	}
	while ((ch!=0) && (offset<12));
}

static int nc_card_image_deletefile(imgtool_image *img, const char *fname)
{
	struct nc_card_image *nc_card = (struct nc_card_image *)img;
	char nc_filename[12];

	setup_nc_filename(nc_filename,fname);

	if (memcard_del(&nc_card->memcard,nc_filename))
	{
		return IMGTOOLERR_MODULENOTFOUND;		
	}

	return 0;
}



/* get a file from the memcard */
static int nc_card_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf)
{
	struct nc_card_image *nc_card = (struct nc_card_image *)img;
	char nc_filename[12];
	unsigned long dir_entry_offset;
	
	setup_nc_filename(nc_filename, fname);

	dir_entry_offset = memcard_locate_dir_entry(&nc_card->memcard,(char *)nc_filename);

	if (dir_entry_offset!=-1)
	{
		struct nc_card_dir_entry *dir_entry;
		int current_block;
		int length_remaining;

		dir_entry = (struct nc_card_dir_entry *)&nc_card->memcard.memcard_data[dir_entry_offset];

		/* get size of file from directory entry */
		length_remaining = ((dir_entry->size_high_byte & 0x0ff)<<8) | (dir_entry->size_low_byte & 0x0ff);
	
		/* get first block index from directory entry */
		current_block = ((dir_entry->first_block_high & 0x0f)<<8) | dir_entry->first_block_low;

		/* keep going until we have written all data or we have got to end of block list */
		while ((length_remaining!=0) && (current_block!=NC_FAT_END_OF_LIST))
		{
			unsigned long block_offset;
			int copy_length;

			/* get offset of block data in memcard data */
			block_offset = memcard_block_to_offset(current_block);
		
			/* calc amount of data to copy from block */
			if (length_remaining<NC_BLOCK_SIZE)
			{
				copy_length = length_remaining;
			}
			else
			{
				copy_length = NC_BLOCK_SIZE;
			}

			/* output to file */
			if (stream_write(destf, &nc_card->memcard.memcard_data[block_offset], copy_length)!=copy_length)
				return IMGTOOLERR_WRITEERROR;
		
			/* update length of data remaining to write */
			length_remaining -=copy_length;	

			/* get next block in list */
			current_block = memcard_fat_get_next_block(&nc_card->memcard,current_block);
			
			if (current_block!=NC_FAT_END_OF_LIST)
			{
				current_block &= 0x0fff;
			}
		}
	}

	return 0;
}

/* put a file to the memcard */
static int nc_card_image_writefile(imgtool_image *img, const char *fname, imgtool_stream *sourcef, const ResolvedOption *options_)
{
	struct nc_card_image *nc_card = (struct nc_card_image *)img;
	unsigned long length;
	unsigned long offset;

	char nc_filename[12];

	length = stream_size(sourcef);

	/* locate a free directory entry */
	offset = memcard_get_free_dir_entry(&nc_card->memcard);

	setup_nc_filename(nc_filename, fname);

	if (offset!=-1)
	{
		/* got a directory entry */

		int	current_block;
		int previous_block;
		unsigned long block_offset;
		unsigned long file_offset = 0;
		unsigned long length_remaining = length;
		struct nc_card_dir_entry *dir_entry;
		int copy_length;

		/* do not transfer more then 65536 bytes */
		/* this is max size of file on card */
		/* file size specified with 2 bytes only! */
		if (length_remaining>65536)
		{
			length_remaining = 65536;
		}

		/* locate the first free block */
		current_block = memcard_fat_get_free_block(&nc_card->memcard);

		if (current_block!=-1)
		{
			/* initialise directory entry - blank all entries */
			memset(&nc_card->memcard.memcard_data[offset], 0, 32);

			dir_entry = (struct nc_card_dir_entry *)&nc_card->memcard.memcard_data[offset];
			/* set size of file in directory entry */
			dir_entry->size_high_byte = (length>>8) & 0x0ff;
			dir_entry->size_low_byte = length & 0x0ff;
			/* set first block in directory entry */
			dir_entry->first_block_high = ((current_block>>8) & 0x0f) | 0x020;
			dir_entry->first_block_low = (current_block & 0x0ff);
			/* setup filename */
			strncpy((char *)dir_entry->filename,(char *)nc_filename, 12);
			
			while (length_remaining!=0)
			{
				/* get offset of block data */
				block_offset = memcard_block_to_offset(current_block);
			
				/* calc amount of data to copy to block */
				if (length_remaining<NC_BLOCK_SIZE)
				{
					copy_length = length_remaining;
				}
				else
				{
					copy_length = NC_BLOCK_SIZE;
				}

				/* copy data to memcard */
				if (stream_read(sourcef, &nc_card->memcard.memcard_data[block_offset], copy_length)!=copy_length)
					return IMGTOOLERR_WRITEERROR;

				length_remaining -=copy_length;	
				file_offset+=copy_length;

				/* mark block as used */
				memcard_fat_write(&nc_card->memcard,current_block, NC_FAT_END_OF_LIST);

				previous_block = current_block;
				current_block = memcard_fat_get_free_block(&nc_card->memcard);
		
				if (current_block==-1)
					break;
			
				/* link up block chain */
				memcard_fat_write(&nc_card->memcard,previous_block, current_block|0x02000);
			}
		
			/* end block chain */
			memcard_fat_write(&nc_card->memcard,current_block, NC_FAT_END_OF_LIST);
		}
	}


	return 0;
}

/* directory fills 1 or more blocks. 
current_block holds index of current block we are accessing,
offset_in_block holds offset within block referenced by current_block.
eod is 1 if end of directory (no more blocks) or 0 if not end of directory */

struct nc_card_direnum 
{
	imgtool_directory base;
	/* card image */
	struct nc_card_image *img;
	/* directory block index */
	int current_block;
	/* index of entry within block */
	int offset_in_block;
	/* true if reached last entry in last directory block */
	int eod;
};


static int nc_card_image_beginenum(imgtool_image *img, imgtool_directory **outenum)
{
	struct nc_card_image *nc_card = (struct nc_card_image *)img;
	struct nc_card_direnum *card_enum;

	if (memcard_is_executable(&nc_card->memcard))
	{
		return IMGTOOLERR_MODULENOTFOUND;
	}	

	card_enum = (struct nc_card_direnum *) malloc(sizeof(struct nc_card_direnum));
	if (!card_enum)
		return IMGTOOLERR_OUTOFMEMORY;

	card_enum->base.module = img->module;
	card_enum->img = (struct nc_card_image *) img;
	/* directory always starts on block 2 */
	card_enum->current_block = 2;
	card_enum->offset_in_block = 0;
	card_enum->eod = 0;
	*outenum = &card_enum->base;
	return 0;
}

static int nc_card_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent)
{
	struct nc_card_direnum *card_enum = (struct nc_card_direnum *) enumeration;

	/* got to end of directory? */
	if (!card_enum->eod)
	{
		int quit = 0;
		/* no */

		/* loop until we get a used directory entry */
		while (quit==0)
		{
			int dir_block_offset;
			struct nc_card_dir_entry *dir_entry;

			/* gone over end of block? */
			if (card_enum->offset_in_block==NC_BLOCK_SIZE)
			{
				/* yes */

				/* reset offset */
				card_enum->offset_in_block = 0;
				/* get next block */
				card_enum->current_block = memcard_fat_get_next_block(&card_enum->img->memcard,card_enum->current_block);

				/* block index = end of list? */
				if (card_enum->current_block==NC_FAT_END_OF_LIST)
				{
					/* yes */

					/* no more directory blocks, end of directory reached */
					card_enum->eod = 1;
		
					/* no more blocks - so quit */
					quit = 1;
				}
				else
				{
					/* strip off upper 4 bits */
					card_enum->current_block &=0x0fff;
				}
			}

			if (quit==0)
			{
				dir_block_offset = memcard_block_to_offset(card_enum->current_block);

				dir_entry = (struct nc_card_dir_entry *)&card_enum->img->memcard.memcard_data[dir_block_offset + card_enum->offset_in_block];

				if (dir_entry->filename[0]!=0)
				{	
					/* got a used directory entry */

					if (ent->fname_len<13)
						return IMGTOOLERR_BUFFERTOOSMALL;

					memcpy(ent->fname, dir_entry->filename,12);
					dir_entry->filename[12] = '\0';

					/* set file-size -> from directory */
					ent->filesize = ((dir_entry->size_high_byte & 0x0ff)<<8) | (dir_entry->size_low_byte & 0x0ff);

					/* not last entry */
					ent->eof = 0;
					/* not corrupt */
					ent->corrupt = 0;

					if (ent->attr_len)
					{	
						switch (dir_entry->type)
						{
							case 0:
							{
								sprintf(ent->attr,"ASCII FILE");
							}
							break;

							case 5:
							{
								sprintf(ent->attr,"BASIC FILE");
							}
							break;

							case 3:
							{
								sprintf(ent->attr,"DIRECTORY");
							}
							break;
						}
					}

					quit = 1;
				}
			}

			/* update offset for next enum */
			card_enum->offset_in_block+=32;
		}
	}

	/* got to end of directory? */
	if (card_enum->eod)
	{
		/* yes */
		ent->filesize = 0;
		ent->corrupt = 0;
		ent->eof = 1;
		if (ent->fname_len > 0)
			ent->fname[0] = '\0';
	}

	return 0;
}

static void nc_card_image_closeenum(imgtool_directory *enumeration)
{
	free(enumeration);
}
