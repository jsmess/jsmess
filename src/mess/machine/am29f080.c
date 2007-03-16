/*******************************************/
/* Intel 28F008SA 5 Volt Flash-File Memory */

#include "driver.h"

#include "includes/am29f080.h"

/* commands */

enum
{
	AMD_FLASH_MODE_READ_ARRAY,
	AMD_FLASH_MODE_AUTOSELECT,
	AMD_FLASH_MODE_BYTE_PROGRAM,
	AMD_FLASH_MODE_WAITING_ERASE_TYPE
};


#define AMD_FLASH_COMMAND_RESET				0x0F0
#define AMD_FLASH_COMMAND_ERASE_SUSPEND		0x0B0
#define AMD_FLASH_COMMAND_ERASE_RESUME		0x030
#define AMD_FLASH_COMMAND_AUTOSELECT		0x090
#define AMD_FLASH_COMMAND_ERASE				0x080
#define AMD_FLASH_COMMAND_BYTE_PROGRAM		0x0A0
#define AMD_FLASH_COMMAND_ERASE_CHIP		0x010
#define AMD_FLASH_COMMAND_ERASE_SECTOR		0x030

#define AMD_FLASH_MANUFACTURER_CODE				0x001
#define AMD_FLASH_DEVICE_CODE					0x0d5

struct amd_flash_interface
{
	/* ready/busy# output callback */
	void (*amd_flash_ry_by_output)(int state);
};



typedef struct AMD_FLASH_FILE_MEMORY
{
	unsigned long amd_autoselect_state;
	unsigned long mode;
	unsigned char sector_group_protection;

//	/* internal status register */
//	unsigned long flash_status;
//	/* command */
//	unsigned long flash_command;
//	/* number of command cycles required */
//	unsigned long flash_command_parameters_required;
//
//	/* index of 64k block to erase */
//	unsigned long flash_erase_block;
//
//	/* offset we were last suspended at, or offset the erase procedure started at*/
//	unsigned long flash_offset;

	void	*flash_timer;

	int		flash_erase_status;

	/* address of 1mb data */
	char *base;

	struct amd_flash_interface interface;
} AMD_FLASH_FILE_MEMORY;


#define MAX_FLASH_DEVICES 4

static AMD_FLASH_FILE_MEMORY amd_flash[MAX_FLASH_DEVICES];


#if 0
void	amd_flash_set_interface(int index1, struct amd_flash_interface *interface)
{
	if ((index1<0) || (index1>=MAX_FLASH_DEVICES))
		return;

	memcpy(&amd_flash[index1].interface, interface, sizeof(struct amd_flash_interface));
}

/* set sector group protection status */
void	amd_flash_set_sector_group_protection(int index1, int group_index, int is_protected)
{
	if ((index1<0) || (index1>=MAX_FLASH_DEVICES))
		return;

	if ((group_index<0) || (group_index>=7))
		return;

	amd_flash[index1].sector_group_protection &= ~(1<<group_index);
	
	if (is_protected)
	{
		amd_flash[index1].sector_group_protection |= (1<<group_index);
	}
}
#endif



char *amd_flash_get_base(int index1)
{
	return amd_flash[index1].base;
}


void	amd_flash_reset(int index1)
{
	amd_flash[index1].mode = AMD_FLASH_MODE_READ_ARRAY;
	amd_flash[index1].amd_autoselect_state = 0;
}
	
void amd_flash_init(int index1)
{
	/* all groups are write enabled! */
	amd_flash[index1].sector_group_protection = 0;

	/* 1mb ram */
	amd_flash[index1].base = (char *)malloc(1024*1024);
	if (amd_flash[index1].base!=NULL)
	{
		memset(amd_flash[index1].base, 0x080, 1024*1024);
	}
	amd_flash_reset(index1);
}

void amd_flash_finish(int index1)
{
	if (amd_flash[index1].base!=NULL)
	{
		free(amd_flash[index1].base);
		amd_flash[index1].base = NULL;
	}
}

void amd_flash_store(int index1, const char *flash_name)
{
	file_error filerr;
	mame_file *file;

	if (amd_flash[index1].base!=NULL)
	{
		filerr = mame_fopen(SEARCHPATH_MEMCARD, flash_name, OPEN_FLAG_WRITE, &file);
		if (filerr == FILERR_NONE)
		{
			mame_fwrite(file, amd_flash[index1].base, (1024*1024));
			mame_fclose(file);
		}
	}
}



void amd_flash_restore(int index1, const char *flash_name)
{
	file_error filerr;
	mame_file *file;

	if (amd_flash[index1].base!=NULL)
	{
		filerr = mame_fopen(SEARCHPATH_MEMCARD, flash_name, OPEN_FLAG_READ, &file);
		if (filerr == FILERR_NONE)
		{
			mame_fread(file, amd_flash[index1].base, (1024*1024));
			mame_fclose(file);
		}
	}
}



/* flash read - offset is offset within flash-file (offset within 1mb)*/
int amd_flash_bank_handler_r(int index1, int offset)
{

	switch (amd_flash[index1].mode)
	{
		/* read byte */
		case AMD_FLASH_MODE_READ_ARRAY:
			return amd_flash[index1].base[offset];

		case AMD_FLASH_MODE_AUTOSELECT:
		{
			logerror("amd flash read: offs: %04x\n",offset);
			
			switch (offset & 0x03)
			{
				/* get manufacturer code */
				case 0:
					return AMD_FLASH_MANUFACTURER_CODE;
				/* get device code */
				case 1:
					return AMD_FLASH_DEVICE_CODE;

				/* get sector protection. 01 = write protected, 00 = write enabled */
				/* write protection applies to a group of sectors. Each sector is 64k, and
				each group identifies two consecutive sectors. group 0 contains sector 0 and sector 1 */
				case 2:
				{
					/* A19, A18, A17 define sector group */
					unsigned long sector_group = (offset>>17) & 0x07;

					/* return bit indicating protection status */
					return (amd_flash[index1].sector_group_protection>>sector_group) & 0x01;
				}
				break;
			
				/* 3 - not defined! */
				default:
					break;
			}

		}
		break;
	}

	return 0x0ff;
}


static void	amd_autoselect_sequence_update(int index1, int offset, int data)
{
	switch (amd_flash[index1].amd_autoselect_state)
	{
		case 0:
		{
			int addr = offset & 0x07ff;
			if ((addr==0x0555) && (data==0x0aa))
			{
				logerror("amd flash: recognised address 0x0555 and data 0x0aa\n");
				amd_flash[index1].amd_autoselect_state++;
			}
			else
			{
				amd_flash[index1].amd_autoselect_state = 0;
			}
		}
		break;

		case 1:
		{
			int addr = offset & 0x07ff;
			if ((addr==0x02aa) && (data==0x055))
			{
				logerror("amd flash: recognised address 0x02aa and data 0x055\n");
				amd_flash[index1].amd_autoselect_state++;
				amd_flash[index1].mode = AMD_FLASH_MODE_AUTOSELECT;
			}
			else
			{
				amd_flash[index1].amd_autoselect_state = 0;
			}
		}
		break;

		case 2:
		{
			int addr = offset & 0x07ff;
			amd_flash[index1].amd_autoselect_state = 0;

			if (addr==0x0555)
			{
				logerror("amd flash: recognised address 0x0555, command: %02x\n",data);

				switch (data)
				{
					/* not sure if this is correct for amd flash! */
					case AMD_FLASH_COMMAND_RESET:
					{
						/* return to read array */
						amd_flash[index1].mode = AMD_FLASH_MODE_READ_ARRAY;
						amd_flash[index1].amd_autoselect_state = 0;
					}
					break;

					case AMD_FLASH_COMMAND_AUTOSELECT:
					{
						amd_flash[index1].mode = AMD_FLASH_MODE_AUTOSELECT;
					}
					break;
				
					case AMD_FLASH_COMMAND_BYTE_PROGRAM:
					{
						amd_flash[index1].mode = AMD_FLASH_MODE_BYTE_PROGRAM;
					}
					break;

					case AMD_FLASH_COMMAND_ERASE:
					{
						/* reset state */
						amd_flash[index1].amd_autoselect_state = 0;
						amd_flash[index1].mode = AMD_FLASH_MODE_WAITING_ERASE_TYPE;
					}
					break;
				}
			}
		}
		break;
	}
}


/* flash write - offset is offset within flash-file (offset within 1mb)*/
void amd_flash_bank_handler_w(int index1, int offset, int data)
{
	logerror("amd flash write: offs: %04x data: %02x\n",offset,data);

//	/* reset? */
//	if (data==AMD_FLASH_COMMAND_RESET)
//	{
//		/* return to read array */
//		amd_flash[index1].mode = AMD_FLASH_MODE_READ_ARRAY;
//		amd_flash[index1].amd_autoselect_state = 0;
//		return;
//	}

	amd_autoselect_sequence_update(index1, offset, data);

	switch (amd_flash[index1].mode)
	{
		case AMD_FLASH_MODE_BYTE_PROGRAM:
		{
			/* cannot turn a 0 into 1! */

			/* write the byte */
			amd_flash[index1].base[offset] &=data;
		}
		break;

		default:
			break;
	}

}

/*******************************************/
