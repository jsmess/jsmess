/*
	Atmel at29c040a flash EEPROM

	512k*8 FEEPROM, organized in pages of 256 bytes.

	References:
	Datasheets were found on Atmel's site (www.atmel.com)

	Raphael Nabet 2003
*/

#include "driver.h"
#include "at29040.h"

#define MAX_AT29C040A 4

#define FEEPROM_SIZE        0x80000
#define SECTOR_SIZE         0x00100
#define BOOT_BLOCK_SIZE     0x04000

#define ADDRESS_MASK        0x7ffff
#define SECTOR_ADDRESS_MASK 0x7ff00
#define BYTE_ADDRESS_MASK   0x000ff

/*
	at29c40a state

	Command states (s_cmd_0 is the initial state):
	s_cmd_0: default state
	s_cmd_1: state after writing aa to 5555
	s_cmd_2: state after writing 55 to 2aaa

	Programming states (s_programming_0 is the initial state):
	s_programming_0: default state
	s_programming_1: a program and enable/disable lock command  has been
		executed, but programming has not actually started.
	s_programming_2: the programming buffer is being written to
	s_programming_3: the programming buffer is being burnt to flash ROM
*/
static struct
{
	UINT8 *data_ptr;

	/* status bits */
	unsigned int s_lower_bbl     : 1;	/* set when lower boot block lockout is enabled */
	unsigned int s_higher_bbl    : 1;	/* set when upper boot block lockout is enabled */
	unsigned int s_sdp           : 1;	/* set when in software data protect mode */
	unsigned int s_id_mode       : 1;	/* set when in chip id mode */
	enum
	{
		s_cmd_0 = 0x0,
		s_cmd_1 = 0x1,
		s_cmd_2 = 0x2
	} s_cmd;							/* command state */
	unsigned int s_enabling_bbl  : 1;	/* set when a boot block lockout command is expecting its parameter */
	unsigned int s_cmd_0x80_flag : 1;	/* set if 0x80 command has just been executed (some command require this prefix) */
	enum
	{
		s_pgm_0 = 0x0,
		s_pgm_1 = 0x1,
		s_pgm_2 = 0x2,
		s_pgm_3 = 0x3
	} s_pgm;							/* programming state */
	unsigned int s_enabling_sdb  : 1;	/* set when a sdp enable command is in progress */
	unsigned int s_disabling_sdb : 1;	/* set when a sdp disable command is in progress */

	unsigned int s_dirty         : 1;	/* set when the memory contents should be set */

	UINT8 toggle_bit;
	UINT8 programming_buffer[SECTOR_SIZE];
	int programming_last_offset;
	mame_timer *programming_timer;
} at29c040a[MAX_AT29C040A];


/*
	programming timer callback
*/
static void at29c040a_programming_timer_callback(int id)
{
	switch (at29c040a[id].s_pgm)
	{
	case s_pgm_1:
		/* programming cycle timeout */
		at29c040a[id].s_pgm = s_pgm_0;
		break;

	case s_pgm_2:
		/* programming cycle start */
		at29c040a[id].s_pgm = s_pgm_3;
		/* max delay 10ms, typical delay 5 to 7 ms */
		timer_adjust(at29c040a[id].programming_timer, TIME_IN_MSEC(5), id, 0.);
		break;

	case s_pgm_3:
		/* programming cycle end */
		memcpy(at29c040a[id].data_ptr + (at29c040a[id].programming_last_offset & ~0xff), at29c040a[id].programming_buffer, SECTOR_SIZE);

		if (at29c040a[id].s_enabling_sdb)
			at29c040a[id].s_sdp = TRUE;

		if (at29c040a[id].s_disabling_sdb)
			at29c040a[id].s_sdp = FALSE;

		at29c040a[id].s_pgm = s_pgm_0;
		at29c040a[id].s_enabling_sdb = FALSE;
		at29c040a[id].s_disabling_sdb = FALSE;

		at29c040a[id].s_dirty = TRUE;
		break;

	default:
		logerror("internal error in %s %d\n", __FILE__, __LINE__);
		break;
	}
}

/*
	Initialize one FEEPROM chip: may be called at driver init or image load
	time (or machine init time if you don't use MESS image core)
*/
void at29c040a_init_data_ptr(int id, UINT8 *data_ptr)
{
	at29c040a[id].data_ptr = data_ptr;
}

/*
	Initialize one FEEPROM chip: must be called at machine init time
*/
void at29c040a_init(int id)
{
	at29c040a[id].programming_timer = timer_alloc(at29c040a_programming_timer_callback);
}

/*
	load the contents of one FEEPROM file
*/
int at29c040a_file_load(int id, mame_file *file)
{
	UINT8 buf;

	/* version flag */
	if (mame_fread(file, & buf, 1) != 1)
		return 1;
	if (buf != 0)
		return 1;

	/* chip state: lower boot block lockout, higher boot block lockout,
	software data protect */
	if (mame_fread(file, & buf, 1) != 1)
		return 1;
	at29c040a[id].s_lower_bbl = (buf >> 2) & 1;
	at29c040a[id].s_higher_bbl = (buf >> 1) & 1;
	at29c040a[id].s_sdp = buf & 1;

	/* data */
	if (mame_fread(file, at29c040a[id].data_ptr, FEEPROM_SIZE) != FEEPROM_SIZE)
		return 1;

	at29c040a[id].s_dirty = FALSE;

	return 0;
}

/*
	save the FEEPROM contents to file
*/
int at29c040a_file_save(int id, mame_file *file)
{
	UINT8 buf;

	/* version flag */
	buf = 0;
	if (mame_fwrite(file, & buf, 1) != 1)
		return 1;

	/* chip state: lower boot block lockout, higher boot block lockout,
	software data protect */
	buf = (at29c040a[id].s_lower_bbl << 2) | (at29c040a[id].s_higher_bbl << 1) | at29c040a[id].s_sdp;
	if (mame_fwrite(file, & buf, 1) != 1)
		return 1;

	/* data */
	if (mame_fwrite(file, at29c040a[id].data_ptr, FEEPROM_SIZE) != FEEPROM_SIZE)
		return 1;

	return 0;
}

int at29c040a_get_dirty_flag(int id)
{
	return at29c040a[id].s_dirty;
}

/*
	read a byte from FEEPROM
*/
UINT8 at29c040a_r(int id, offs_t offset)
{
	int reply;

	offset &= ADDRESS_MASK;

	/* reading in the midst of any command sequence cancels it (right???) */
	at29c040a[id].s_cmd = s_cmd_0;
	at29c040a[id].s_cmd_0x80_flag = FALSE;
	at29c040a[id].s_higher_bbl = TRUE;

	/* reading before the start of a programming cycle cancels it (right???) */
	if (at29c040a[id].s_pgm == s_pgm_1)
	{
		/* attempty to access a locked out boot block: cancel programming
		command if necessary */
		at29c040a[id].s_pgm = s_pgm_0;
		at29c040a[id].s_enabling_sdb = FALSE;
		at29c040a[id].s_disabling_sdb = FALSE;
		timer_adjust(at29c040a[id].programming_timer, TIME_NEVER, id, 0.);
	}


	if (at29c040a[id].s_id_mode)
	{
		switch (offset)
		{
		case 0:
			reply = 0x1f;
			break;

		case 1:
			reply = 0xa4;
			break;

		case 2:
			reply = at29c040a[id].s_lower_bbl ? 0xff : 0xfe;
			break;

		case 0x7fff2:
			reply = at29c040a[id].s_higher_bbl ? 0xff : 0xfe;
			break;

		default:
			reply = 0;
			break;
		}
	}
	else if ((at29c040a[id].s_pgm == s_pgm_2) || (at29c040a[id].s_pgm == s_pgm_3))
	{
		if (at29c040a[id].s_pgm == s_pgm_2)
		{	/* data polling starts the programming cycle (right???) */
			at29c040a[id].s_pgm = s_pgm_3;
			/* max delay 10ms, typical delay 5 to 7 ms */
			timer_adjust(at29c040a[id].programming_timer, TIME_IN_MSEC(5), id, 0.);
		}

		reply = at29c040a[id].toggle_bit;
		at29c040a[id].toggle_bit ^= 0x02;
		if ((offset == at29c040a[id].programming_last_offset) && (! (at29c040a[id].programming_buffer[at29c040a[id].programming_last_offset & 0xff] & 0x01)))
			reply |= 0x01;
	}
	else
		reply = at29c040a[id].data_ptr[offset];

	return reply;
}

/*
	write a byte to FEEPROM
*/
void at29c040a_w(int id, offs_t offset, UINT8 data)
{
	offset &= ADDRESS_MASK;

	if (at29c040a[id].s_enabling_bbl)
	{
		at29c040a[id].s_enabling_bbl = FALSE;

		if ((offset == 0x00000) && (data == 0x00))
		{
			at29c040a[id].s_lower_bbl = TRUE;
			at29c040a[id].s_dirty = TRUE;
			return;
		}
		else if ((offset == 0x7ffff) && (data == 0xff))
		{
			at29c040a[id].s_higher_bbl = TRUE;
			at29c040a[id].s_dirty = TRUE;
			return;
		}
	}

	switch (at29c040a[id].s_cmd)
	{
	case s_cmd_0:
		if ((offset == 0x5555) && (data == 0xaa))
			at29c040a[id].s_cmd = s_cmd_1;
		else
		{
			at29c040a[id].s_cmd = s_cmd_0;
			at29c040a[id].s_cmd_0x80_flag = FALSE;
		}
		break;

	case s_cmd_1:
		if ((offset == 0x2aaa) && (data == 0x55))
			at29c040a[id].s_cmd = s_cmd_2;
		else
		{
			at29c040a[id].s_cmd = s_cmd_0;
			at29c040a[id].s_cmd_0x80_flag = FALSE;
		}
		break;

	case s_cmd_2:
		if (offset == 0x5555)
		{
			/* exit programming mode */
			at29c040a[id].s_pgm = s_pgm_0;
			at29c040a[id].s_enabling_sdb = FALSE;
			at29c040a[id].s_disabling_sdb = FALSE;
			timer_adjust(at29c040a[id].programming_timer, TIME_NEVER, id, 0.);

			/* process command */
			switch (data)
			{
			case 0x10:
				/*  Software chip erase */
				if (at29c040a[id].s_cmd_0x80_flag)
				{
					if (at29c040a[id].s_lower_bbl || at29c040a[id].s_higher_bbl)
						logerror("\"If the boot block lockout feature has been enabled, the 6-byte software chip erase algorithm will not function.\"\n");
					else
					{
						memset(at29c040a[id].data_ptr, 0xff, 524288);
						at29c040a[id].s_dirty = TRUE;
					}
				}
				break;

			case 0x20:
				/* Software data protection disable */
				if (at29c040a[id].s_cmd_0x80_flag)
				{
					at29c040a[id].s_pgm = s_pgm_1;
					at29c040a[id].s_disabling_sdb = TRUE;
					/* set command timeout (right???) */
					//timer_adjust(at29c040a[id].programming_timer, TIME_IN_USEC(150), id, 0.);
				}
				break;

			case 0x40:
				/* Boot block lockout enable */
				if (at29c040a[id].s_cmd_0x80_flag)
					at29c040a[id].s_enabling_bbl = TRUE;
				break;

			case 0x90:
				/* Software product identification entry */
				at29c040a[id].s_id_mode = TRUE;
				break;

			case 0xa0:
				/* Software data protection enable */
				at29c040a[id].s_pgm = s_pgm_1;
				at29c040a[id].s_enabling_sdb = TRUE;
				/* set command timeout (right???) */
				//timer_adjust(at29c040a[id].programming_timer, TIME_IN_USEC(150), id, 0.);
				break;

			case 0xf0:
				/* Software product identification exit */
				at29c040a[id].s_id_mode = FALSE;
				break;
			}
			at29c040a[id].s_cmd = s_cmd_0;
			at29c040a[id].s_cmd_0x80_flag = FALSE;
			if (data == 0x80)
				at29c040a[id].s_cmd_0x80_flag = TRUE;

			/* return, because we don't want to write the EEPROM with the command byte */
			return;
		}
		else
		{
			at29c040a[id].s_cmd = s_cmd_0;
			at29c040a[id].s_cmd_0x80_flag = FALSE;
		}
	}
	if ((at29c040a[id].s_pgm == s_pgm_2)
			&& ((offset & ~0xff) != (at29c040a[id].programming_last_offset & ~0xff)))
	{
		/* cancel current programming cycle */
		at29c040a[id].s_pgm = s_pgm_0;
		at29c040a[id].s_enabling_sdb = FALSE;
		at29c040a[id].s_disabling_sdb = FALSE;
		timer_adjust(at29c040a[id].programming_timer, TIME_NEVER, id, 0.);
	}

	if (((at29c040a[id].s_pgm == s_pgm_0) && ! at29c040a[id].s_sdp)
			|| (at29c040a[id].s_pgm == s_pgm_1))
	{
		if (((offset < BOOT_BLOCK_SIZE) && at29c040a[id].s_lower_bbl)
			|| ((offset >= FEEPROM_SIZE-BOOT_BLOCK_SIZE) && at29c040a[id].s_higher_bbl))
		{
			/* attempty to access a locked out boot block: cancel programming
			command if necessary */
			at29c040a[id].s_pgm = s_pgm_0;
			at29c040a[id].s_enabling_sdb = FALSE;
			at29c040a[id].s_disabling_sdb = FALSE;
		}
		else
		{	/* enter programming mode */
			memset(at29c040a[id].programming_buffer, 0xff, SECTOR_SIZE);
			at29c040a[id].s_pgm = s_pgm_2;
		}
	}
	if (at29c040a[id].s_pgm == s_pgm_2)
	{
		/* write data to programming buffer */
		at29c040a[id].programming_buffer[offset & 0xff] = data;
		at29c040a[id].programming_last_offset = offset;
		timer_adjust(at29c040a[id].programming_timer, TIME_IN_USEC(150), id, 0.);
	}
}

