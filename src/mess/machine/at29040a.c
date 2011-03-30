/*
    Atmel at29c040a flash EEPROM

    512k*8 FEEPROM, organized in pages of 256 bytes.

    References:
    Datasheets were found on Atmel's site (www.atmel.com)

    Raphael Nabet 2003

    Rewritten as device
    Michael Zapf, September 2010
*/

#include "emu.h"
#include "at29040a.h"

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
enum  s_cmd_t
{
	s_cmd_0 = 0x0,
	s_cmd_1 = 0x1,
	s_cmd_2 = 0x2
};

enum  s_pgm_t
{
	s_pgm_0 = 0x0,
	s_pgm_1 = 0x1,
	s_pgm_2 = 0x2,
	s_pgm_3 = 0x3
};


typedef struct _at29c040a_state
{
	UINT8		*memptr;

	int 		s_lower_bbl;		/* set when lower boot block lockout is enabled */
	int 		s_higher_bbl;		/* set when upper boot block lockout is enabled */
	int 		s_sdp;				/* set when in software data protect mode */

	int 		s_id_mode;			/* set when in chip id mode */
	s_cmd_t		s_cmd;				/* command state */
	int 		s_enabling_bbl;		/* set when a boot block lockout command is expecting its parameter */
	int 		s_cmd_0x80_flag;	/* set if 0x80 command has just been executed (some command require this prefix) */
	s_pgm_t 	s_pgm;				/* programming state */
	int 		s_enabling_sdb;		/* set when a sdp enable command is in progress */
	int 		s_disabling_sdb;	/* set when a sdp disable command is in progress */
	int 		s_dirty;			/* set when the memory contents should be set */
	UINT8		toggle_bit;
	UINT8		programming_buffer[SECTOR_SIZE];
	int 		programming_last_offset;
	emu_timer	*programming_timer;
} at29c040a_state;

INLINE at29c040a_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == AT29C040A);

	return (at29c040a_state *)downcast<legacy_device_base *>(device)->token();
}

INLINE const at29c040a_config *get_config(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == AT29C040A);

	return (const at29c040a_config *) downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

/*
    programming timer callback
*/
static TIMER_CALLBACK(at29c040a_programming_timer_callback)
{
	device_t *device = (device_t *)ptr;
	at29c040a_state *feeprom = get_safe_token(device);

	switch (feeprom->s_pgm)
	{
	case s_pgm_1:
		/* programming cycle timeout */
		feeprom->s_pgm = s_pgm_0;
		break;

	case s_pgm_2:
		/* programming cycle start */
		feeprom->s_pgm = s_pgm_3;
		/* max delay 10ms, typical delay 5 to 7 ms */
		feeprom->programming_timer->adjust(attotime::from_msec(5));
		break;

	case s_pgm_3:
		/* programming cycle end */
		memcpy(feeprom->memptr + (feeprom->programming_last_offset & ~0xff), feeprom->programming_buffer, SECTOR_SIZE);

		if (feeprom->s_enabling_sdb)
			feeprom->s_sdp = TRUE;

		if (feeprom->s_disabling_sdb)
			feeprom->s_sdp = FALSE;

		feeprom->s_pgm = s_pgm_0;
		feeprom->s_enabling_sdb = FALSE;
		feeprom->s_disabling_sdb = FALSE;

		feeprom->s_dirty = TRUE;
		break;

	default:
		logerror("internal error in %s %d\n", __FILE__, __LINE__);
		break;
	}
}

int at29c040a_is_dirty(device_t *device)
{
	at29c040a_state *feeprom = get_safe_token(device);
	return feeprom->s_dirty;
}

static void sync_flags(device_t *device)
{
	at29c040a_state *feeprom = get_safe_token(device);
	if (feeprom->s_lower_bbl) feeprom->memptr[1] |= 0x04;
	else feeprom->memptr[1] &= ~0x04;

	if (feeprom->s_higher_bbl) feeprom->memptr[1] |= 0x02;
	else feeprom->memptr[1] &= ~0x02;

	if (feeprom->s_sdp) feeprom->memptr[1] |= 0x01;
	else feeprom->memptr[1] &= ~0x01;
}

/*
    read a byte from FEEPROM
*/
READ8_DEVICE_HANDLER( at29c040a_r )
{
	at29c040a_state *feeprom = get_safe_token(device);
	int reply;

	offset &= ADDRESS_MASK;

	/* reading in the midst of any command sequence cancels it (right???) */
	feeprom->s_cmd = s_cmd_0;
	feeprom->s_cmd_0x80_flag = FALSE;
	feeprom->s_higher_bbl = TRUE;
	sync_flags(device);

	/* reading before the start of a programming cycle cancels it (right???) */
	if (feeprom->s_pgm == s_pgm_1)
	{
		// attempty to access a locked out boot block: cancel programming
		// command if necessary
		feeprom->s_pgm = s_pgm_0;
		feeprom->s_enabling_sdb = FALSE;
		feeprom->s_disabling_sdb = FALSE;
		feeprom->programming_timer->adjust(attotime::never);
	}


	if (feeprom->s_id_mode)
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
			reply = feeprom->s_lower_bbl ? 0xff : 0xfe;
			break;

		case 0x7fff2:
			reply = feeprom->s_higher_bbl ? 0xff : 0xfe;
			break;

		default:
			reply = 0;
			break;
		}
	}
	else if ((feeprom->s_pgm == s_pgm_2) || (feeprom->s_pgm == s_pgm_3))
	{
		if (feeprom->s_pgm == s_pgm_2)
		{	/* data polling starts the programming cycle (right???) */
			feeprom->s_pgm = s_pgm_3;
			/* max delay 10ms, typical delay 5 to 7 ms */
			feeprom->programming_timer->adjust(attotime::from_msec(5));
		}

		reply = feeprom->toggle_bit;
		feeprom->toggle_bit ^= 0x02;
		if ((offset == feeprom->programming_last_offset) && (! (feeprom->programming_buffer[feeprom->programming_last_offset & 0xff] & 0x01)))
			reply |= 0x01;
	}
	else
		reply = feeprom->memptr[offset];

	return reply;
}

/*
    Write a byte to FEEPROM
*/
WRITE8_DEVICE_HANDLER( at29c040a_w )
{
	at29c040a_state *feeprom = get_safe_token(device);

	offset &= ADDRESS_MASK;

	/* The special CFI commands assume a smaller address space according */
	/* to the specification ("address format A14-A0") */
	offs_t cfi_offset = offset & 0x7fff;

	if (feeprom->s_enabling_bbl)
	{
		feeprom->s_enabling_bbl = FALSE;

		if ((offset == 0x00000) && (data == 0x00))
		{
			feeprom->s_lower_bbl = TRUE;
			feeprom->s_dirty = TRUE;
			sync_flags(device);
			return;
		}
		else if ((offset == 0x7ffff) && (data == 0xff))
		{
			feeprom->s_higher_bbl = TRUE;
			feeprom->s_dirty = TRUE;
			sync_flags(device);
			return;
		}
	}

	switch (feeprom->s_cmd)
	{
	case s_cmd_0:
		if ((cfi_offset == 0x5555) && (data == 0xaa))
			feeprom->s_cmd = s_cmd_1;
		else
		{
			feeprom->s_cmd = s_cmd_0;
			feeprom->s_cmd_0x80_flag = FALSE;
		}
		break;

	case s_cmd_1:
		if ((cfi_offset == 0x2aaa) && (data == 0x55))
			feeprom->s_cmd = s_cmd_2;
		else
		{
			feeprom->s_cmd = s_cmd_0;
			feeprom->s_cmd_0x80_flag = FALSE;
		}
		break;

	case s_cmd_2:
		if (cfi_offset == 0x5555)
		{
			/* exit programming mode */
			feeprom->s_pgm = s_pgm_0;
			feeprom->s_enabling_sdb = FALSE;
			feeprom->s_disabling_sdb = FALSE;
			feeprom->programming_timer->adjust(attotime::never);

			/* process command */
			switch (data)
			{
			case 0x10:
				/*  Software chip erase */
				if (feeprom->s_cmd_0x80_flag)
				{
					if (feeprom->s_lower_bbl || feeprom->s_higher_bbl)
						logerror("If the boot block lockout feature has been enabled, the 6-byte software chip erase algorithm will not function.\n");
					else
					{
						memset(feeprom->memptr, 0xff, 524288);
						feeprom->s_dirty = TRUE;
					}
				}
				break;

			case 0x20:
				/* Software data protection disable */
				if (feeprom->s_cmd_0x80_flag)
				{
					feeprom->s_pgm = s_pgm_1;
					feeprom->s_disabling_sdb = TRUE;
					/* set command timeout (right???) */
					//feeprom->programming_timer->adjust(attotime::from_usec(150), id, 0.);
				}
				break;

			case 0x40:
				/* Boot block lockout enable */
				if (feeprom->s_cmd_0x80_flag)
					feeprom->s_enabling_bbl = TRUE;
				break;

			case 0x90:
				/* Software product identification entry */
				feeprom->s_id_mode = TRUE;
				break;

			case 0xa0:
				/* Software data protection enable */
				feeprom->s_pgm = s_pgm_1;
				feeprom->s_enabling_sdb = TRUE;
				/* set command timeout (right???) */
				//feeprom->programming_timer->adjust(attotime::from_usec(150), id, 0.);
				break;

			case 0xf0:
				/* Software product identification exit */
				feeprom->s_id_mode = FALSE;
				break;
			}
			feeprom->s_cmd = s_cmd_0;
			feeprom->s_cmd_0x80_flag = FALSE;
			if (data == 0x80)
				feeprom->s_cmd_0x80_flag = TRUE;

			/* return, because we don't want to write the EEPROM with the command byte */
			return;
		}
		else
		{
			feeprom->s_cmd = s_cmd_0;
			feeprom->s_cmd_0x80_flag = FALSE;
		}
	}
	if ((feeprom->s_pgm == s_pgm_2)
			&& ((offset & ~0xff) != (feeprom->programming_last_offset & ~0xff)))
	{
		/* cancel current programming cycle */
		feeprom->s_pgm = s_pgm_0;
		feeprom->s_enabling_sdb = FALSE;
		feeprom->s_disabling_sdb = FALSE;
		feeprom->programming_timer->adjust(attotime::never);
	}

	if (((feeprom->s_pgm == s_pgm_0) && !feeprom->s_sdp)
			|| (feeprom->s_pgm == s_pgm_1))
	{
		if (((offset < BOOT_BLOCK_SIZE) && feeprom->s_lower_bbl)
			|| ((offset >= FEEPROM_SIZE-BOOT_BLOCK_SIZE) && feeprom->s_higher_bbl))
		{
			// attempty to access a locked out boot block: cancel programming
			// command if necessary
			feeprom->s_pgm = s_pgm_0;
			feeprom->s_enabling_sdb = FALSE;
			feeprom->s_disabling_sdb = FALSE;
		}
		else
		{	/* enter programming mode */
			memset(feeprom->programming_buffer, 0xff, SECTOR_SIZE);
			feeprom->s_pgm = s_pgm_2;
		}
	}
	if (feeprom->s_pgm == s_pgm_2)
	{
		/* write data to programming buffer */
		feeprom->programming_buffer[offset & 0xff] = data;
		feeprom->programming_last_offset = offset;
		feeprom->programming_timer->adjust(attotime::from_usec(150));
	}
}

static DEVICE_START( at29c040a )
{
	at29c040a_state *feeprom = get_safe_token(device);
	feeprom->programming_timer = device->machine().scheduler().timer_alloc(FUNC(at29c040a_programming_timer_callback), (void *)device);
}

static DEVICE_STOP( at29c040a )
{

}

static DEVICE_RESET( at29c040a )
{
	at29c040a_state *feeprom = get_safe_token(device);
	const at29c040a_config* atconf = (const at29c040a_config*)get_config(device);
	feeprom->memptr = (*atconf->get_memory)(device->owner()) + atconf->offset;

	if (feeprom->memptr[0] != 0)
	{
		logerror("AT29040A: Version mismatch; expected 0 but found %x for %s\n", feeprom->memptr[0], device->tag());
		return;
	}

	feeprom->s_lower_bbl = (feeprom->memptr[1] >> 2) & 1;
	feeprom->s_higher_bbl = (feeprom->memptr[1] >> 1) & 1;
	feeprom->s_sdp = feeprom->memptr[1] & 1;
	feeprom->s_dirty = FALSE;

	feeprom->memptr += 2;
}

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##at29c040a##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_INLINE_CONFIG
#define DEVTEMPLATE_NAME                "ATMEL 29c040a"
#define DEVTEMPLATE_FAMILY              "Flash Memory"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( AT29C040A, at29c040a );
