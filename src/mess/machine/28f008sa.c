/*******************************************/
/* Intel 28F008SA 5 Volt Flash-File Memory */

#include "driver.h"

#include "includes/28f008sa.h"

/* commands */
#define FLASH_COMMAND_READ_ARRAY_OR_RESET					0x00FF
#define FLASH_COMMAND_INTELLIGENT_IDENTIFIER				0x0090
#define FLASH_COMMAND_READ_STATUS_REGISTER					0x0070
#define FLASH_COMMAND_CLEAR_STATUS_REGISTER					0x0050
#define FLASH_COMMAND_ERASE_SETUP_OR_ERASE_CONFIRM			0x0020
#define FLASH_COMMAND_ERASE_SUSPEND_OR_ERASE_RESUME			0x00b0
#define FLASH_COMMAND_BYTE_WRITE_SETUP_OR_WRITE				0x0040
#define FLASH_COMMAND_ALTERNATIVE_BYTE_WRITE_SETUP_OR_WRITE	0x0010
#define FLASH_COMMAND_ERASE_CONFIRM							0x00d0

/* status bits */
#define FLASH_STATUS_WRITE_STATE_MACHINE_STATUS_READY						0x0080
#define FLASH_STATUS_WRITE_STATE_MACHINE_STATUS_BUSY						0x0000

#define FLASH_STATUS_ERASE_SUSPEND_STATUS_SUSPENDED							0x0040
#define FLASH_STATUS_ERASE_SUSPEND_STATUS_ERASE_IN_PROGRESS_OR_COMPLETED	0x0000

#define FLASH_STATUS_ERASE_STATUS_ERROR_IN_BLOCK							0x0020
#define FLASH_STATUS_ERASE_STATUS_SUCCESSFUL_BLOCK_ERASE					0x0000

#define FLASH_STATUS_BYTE_WRITE_STATUS_ERROR								0x0010
#define FLASH_STATUS_BYTE_WRITE_STATUS_SUCCESSFUL							0x0000

#define FLASH_STATUS_VPP_STATUS_LOW_OPERATION_ABORTED						0x0008
#define FLASH_STATUS_VPP_STATUS_HIGH										0x0000

#define FLASH_STATUS_RESERVED												(0x0004 | 0x0002 | 0x0001)

#define FLASH_MANUFACTURER_CODE				0x089
#define FLASH_DEVICE_CODE					0x0a2

/* setup command has been issued */
#define FLASH_ERASE_STATUS_SETUP 0x001
/* suspended */
#define FLASH_ERASE_STATUS_SUSPENDED 0x002
/* nothing */
#define FLASH_ERASE_STATUS_NONE	0x000
/* erase in progress */
#define FLASH_ERASE_STATUS_ERASING 0x003

/* datasheet states that a block will be erase typically in 1.6 seconds */
#define FLASH_TIME_PER_BLOCK_SECS (1.6f)
#define FLASH_TIME_PER_BYTE_IN_SECS (FLASH_TIME_PER_BLOCK_SECS/65536.0f)

typedef struct FLASH_FILE_MEMORY
{
	/* internal status register */
	unsigned long flash_status;
	/* command */
	unsigned long flash_command;
	/* number of command cycles required */
	unsigned long flash_command_parameters_required;

	/* index of 64k block to erase */
	unsigned long flash_erase_block;

	/* offset we were last suspended at, or offset the erase procedure started at*/
	unsigned long flash_offset;

	void	*flash_timer;

	int		flash_erase_status;

	/* address of 1mb data */
	char *base;
} FLASH_FILE_MEMORY;

#define MAX_FLASH_DEVICES 4

static FLASH_FILE_MEMORY flash[MAX_FLASH_DEVICES];

char *flash_get_base(int index1)
{
	return flash[index1].base;
}

static void flash_check_complete_erase(int index1)
{
	if (flash[index1].flash_offset>=65536)
	{

		flash[index1].flash_status = (FLASH_STATUS_WRITE_STATE_MACHINE_STATUS_READY |
				FLASH_STATUS_ERASE_SUSPEND_STATUS_ERASE_IN_PROGRESS_OR_COMPLETED);

		/* set erase state to none */
		flash[index1].flash_erase_status = FLASH_ERASE_STATUS_NONE;
	}
}


/* callback for flash erase - if it gets to here, the erase procedure is completed */
static void	flash_timer_callback(int index1)
{
	if (flash[index1].flash_offset<65536)
	{
		/* complete erase */
		memset(flash[index1].base + (flash[index1].flash_erase_block<<16) + flash[index1].flash_offset, 0x0ff,65536-flash[index1].flash_offset);
		flash[index1].flash_offset = 65536;
	}

	flash_check_complete_erase(index1);

	/* stop timer from activating again, and do not let
	MAME sub-system remove it from the list */
	mame_timer_reset(flash[index1].flash_timer, time_never);
}

/* suspend erase operation */
static void flash_suspend_erase(int index1)
{
	/* suspend */
	flash[index1].flash_status = FLASH_STATUS_ERASE_SUSPEND_STATUS_SUSPENDED |
						FLASH_STATUS_WRITE_STATE_MACHINE_STATUS_READY;
	

	if (flash[index1].flash_timer!=0)
	{
		int num_bytes;
		double time_elapsed;

		/* get time passed */
		time_elapsed = timer_timeelapsed(flash[index1].flash_timer);

		/* at this point in time, I don't believe it is necessary to know number of bits that have
		been erased*/

		/* num bytes that have been erased in that time */
		num_bytes = time_elapsed/FLASH_TIME_PER_BYTE_IN_SECS;

		/* this should not happen, but check anyway */
		if (num_bytes>65536)
		{
			num_bytes = 65536;
		}

		if (num_bytes!=0)
		{
			/* back-fill from previous suspend point with erase data (fills up to this point)*/
			memset(flash[index1].base + (flash[index1].flash_erase_block<<16) + flash[index1].flash_offset, 0x0ff, num_bytes);
		}
		/* update offset */
		flash[index1].flash_offset += num_bytes;

		/* remove timer */
		mame_timer_reset(flash[index1].flash_timer, time_never);

		/* set erase state to suspended */
		flash[index1].flash_erase_status = FLASH_ERASE_STATUS_SUSPENDED;
	}

	flash_check_complete_erase(index1);
}

static void flash_resume_erase(int index1)
{
	int num_bytes_remaining;

	flash[index1].flash_status = (FLASH_STATUS_WRITE_STATE_MACHINE_STATUS_BUSY |
					FLASH_STATUS_ERASE_SUSPEND_STATUS_ERASE_IN_PROGRESS_OR_COMPLETED);

	flash[index1].flash_erase_status = FLASH_ERASE_STATUS_ERASING;

	/* calc number of bytes remaining to erase */
	num_bytes_remaining = 65536-flash[index1].flash_offset;

	/* issue a timer for this */
	timer_adjust(flash[index1].flash_timer, TIME_IN_SEC((FLASH_TIME_PER_BYTE_IN_SECS*num_bytes_remaining)), index1, 0);
}


void	flash_init(int index1)
{
	flash[index1].flash_timer = mame_timer_alloc(flash_timer_callback);

	/* 1mb ram */
	flash[index1].base = (char *) auto_malloc(1024*1024);
	memset(flash[index1].base, 0x080, 1024*1024);
	flash_reset(index1);
	/* no erase state */
	flash[index1].flash_erase_status = FLASH_ERASE_STATUS_NONE;
}

void flash_finish(int index1)
{
	if (flash[index1].base!=NULL)
		flash[index1].base = NULL;
}

void flash_store(int index1, const char *flash_name)
{
	file_error filerr;
	mame_file *file;

	if (flash[index1].base!=NULL)
	{
		filerr = mame_fopen(SEARCHPATH_MEMCARD, flash_name, OPEN_FLAG_WRITE, &file);

		if (filerr == FILERR_NONE)
		{
			mame_fwrite(file, flash[index1].base, (1024*1024));
	
			mame_fclose(file);
		}
	}
}



void flash_restore(int index1, const char *flash_name)
{
	file_error filerr;
	mame_file *file;

	if (flash[index1].base!=NULL)
	{
		filerr = mame_fopen(SEARCHPATH_MEMCARD, flash_name, OPEN_FLAG_READ, &file);

		if (filerr == FILERR_NONE)
		{
			mame_fread(file, flash[index1].base, (1024*1024));
	
			mame_fclose(file);
		}
	}
}


void flash_reset(int index1)
{
	flash[index1].flash_status = FLASH_STATUS_WRITE_STATE_MACHINE_STATUS_READY;
	flash[index1].flash_command = FLASH_COMMAND_READ_ARRAY_OR_RESET;
	flash[index1].flash_erase_status = FLASH_ERASE_STATUS_NONE;
	mame_timer_reset(flash[index1].flash_timer, time_never);
}
	

/* flash read - offset is offset within flash-file (offset within 1mb)*/
int flash_bank_handler_r(int index1, int offset)
{
	switch (flash[index1].flash_command)
	{
		/* read mode, return flash-rom data */
		case FLASH_COMMAND_READ_ARRAY_OR_RESET:
			return flash[index1].base[offset];

		/* read intelligent identifier */
		case FLASH_COMMAND_INTELLIGENT_IDENTIFIER:
		{
			/* offset/address zero returns manufacturer code */
			if (offset==0)
				return FLASH_MANUFACTURER_CODE;
			/* offset/address one returns device code */
			if (offset==1)
				return FLASH_DEVICE_CODE;
		}
		break;

		/* read status */
		case FLASH_COMMAND_BYTE_WRITE_SETUP_OR_WRITE:
		case FLASH_COMMAND_ALTERNATIVE_BYTE_WRITE_SETUP_OR_WRITE:
		case FLASH_COMMAND_ERASE_SETUP_OR_ERASE_CONFIRM:
		case FLASH_COMMAND_READ_STATUS_REGISTER:
		case FLASH_COMMAND_CLEAR_STATUS_REGISTER:
		case FLASH_COMMAND_ERASE_SUSPEND_OR_ERASE_RESUME:
		case FLASH_COMMAND_ERASE_CONFIRM:
			return flash[index1].flash_status;

	}

	return 0x0ff;
}

/* flash write - offset is offset within flash-file (offset within 1mb)*/
void flash_bank_handler_w(int index1, int offset, int data)
{
	/* for commands requiring more than one cycle */
	if (flash[index1].flash_command_parameters_required!=0)
	{
		switch (flash[index1].flash_command)
		{
			case FLASH_COMMAND_BYTE_WRITE_SETUP_OR_WRITE:
			case FLASH_COMMAND_ALTERNATIVE_BYTE_WRITE_SETUP_OR_WRITE:
			{
				/* ready, with no errors */
				flash[index1].flash_status &= FLASH_STATUS_ERASE_SUSPEND_STATUS_SUSPENDED;
				flash[index1].flash_status |= FLASH_STATUS_WRITE_STATE_MACHINE_STATUS_READY;
				flash[index1].base[offset] = data;
			
				/* no parameters required */
				flash[index1].flash_command_parameters_required = 0;
			}
			return;

			default:
				break;
		}

	}

	
	flash[index1].flash_command_parameters_required = 0;
	/* set command */
	flash[index1].flash_command = data;

	switch (data)
	{
		case FLASH_COMMAND_READ_ARRAY_OR_RESET:
		case FLASH_COMMAND_INTELLIGENT_IDENTIFIER:
		case FLASH_COMMAND_READ_STATUS_REGISTER:
		case FLASH_COMMAND_CLEAR_STATUS_REGISTER:
		{
			/* if clear status register */
			if (data == FLASH_COMMAND_CLEAR_STATUS_REGISTER)
			{
				/* clear it */
				flash[index1].flash_status &=FLASH_STATUS_ERASE_SUSPEND_STATUS_SUSPENDED;
			}
		}
		break;

		case FLASH_COMMAND_BYTE_WRITE_SETUP_OR_WRITE:
		case FLASH_COMMAND_ALTERNATIVE_BYTE_WRITE_SETUP_OR_WRITE:
		{
			/* expecting a parameter */
			flash[index1].flash_command_parameters_required = 1;
		}
		break;

		/* setup an erase */
		case FLASH_COMMAND_ERASE_SETUP_OR_ERASE_CONFIRM:
		{
			flash[index1].flash_erase_status = FLASH_ERASE_STATUS_SETUP;
		}
		break;

		case FLASH_COMMAND_ERASE_SUSPEND_OR_ERASE_RESUME:
		{
			/* is it suspended? */
			if ((flash[index1].flash_status & FLASH_STATUS_ERASE_SUSPEND_STATUS_SUSPENDED)==0)
			{
				/* no */

				/* suspend it */
				flash_suspend_erase(index1);
			}
		}
		break;


		case FLASH_COMMAND_ERASE_CONFIRM:
		{
			switch (flash[index1].flash_erase_status)
			{
				/* suspended? */
				case FLASH_ERASE_STATUS_SUSPENDED:
				{
					/* has the erase procedure completed? */

					/* if suspended.. and not completed */
					/* confirm start erase */

					/* no longer suspended */
					flash_resume_erase(index1);
				}
				break;

				/* setup? */
				case FLASH_ERASE_STATUS_SETUP:
				{
					/* get block we are erasing */
					flash[index1].flash_erase_block = offset>>16;

					/* set initial offset */
					flash[index1].flash_offset = 0;

					/* start the timer for erase */
					flash_resume_erase(index1);
				}
				break;

				default:
					break;
			}
		}
		break;

		default:
			break;

	}

}

/*******************************************/
