/*****************************************************************************
 *
 * video/dl1416.c
 *
 * DL1416
 *
 * 4-Digit 16-Segment Alphanumeric Intelligent Display
 * with Memory/Decoder/Driver
 *
 * Notes:
 *   - Currently supports the DL1416T.
 *   - Partial support for DL1416B is available, it just needs the right
 *     character set and MAME core support for its display.
 *   - Cursor support is implemented but not tested, as the AIM65 does not
 *     seem to use it.
 *
 * Todo:
 *   - Is the DL1416A identical to the DL1416T? If not, we need to add
 *     support for it.
 *
 * Changes:
 *   - 2007-07-30: Initial version.  [Dirk Best]
 *   - 2008-02-25: Converted to the new device interface.  [Dirk Best]
 *
 ****************************************************************************/


/*****************************************************************************
 Includes
*****************************************************************************/

#include "driver.h"
#include "dl1416.h"



/*****************************************************************************
 Constants
*****************************************************************************/

#define SEG_UNDEF  (-1)
#define SEG_BLANK  (0)
#define SEG_CURSOR (0xffff)



/*****************************************************************************
 Type definitions
*****************************************************************************/

struct _dl1416_t
{
	const dl1416_interface *intf;

	int write_enable;
	int chip_enable;
	int cursor_enable;

	UINT16 cursor_ram[4];
};



/*****************************************************************************
 Global variables
*****************************************************************************/


/* We use the following order for the segments:
 *
 *   000 111
 *  7D  A  E2
 *  7 D A E 2
 *  7  DAE  2
 *   888 999
 *  6  CBF  3
 *  6 C B F 3
 *  6C  B  F3
 *   555 444
 */

/* Character set DL1416T */
static const int dl1416t_segments[128] = {
	SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, /* undefined */
	SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, /* undefined */
	SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, /* undefined */
	SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, /* undefined */
	SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, /* undefined */
	SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, /* undefined */
	SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, /* undefined */
	SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, /* undefined */
	0x0000,    0x2421,    0x0480,    0x0f3c,    /*   ! " # */
	0x0fbb,    0x5f99,    0xa579,    0x4000,    /* $ % & ' */
	0xc000,    0x3000,    0xff00,    0x0f00,    /* ( ) * + */
	0x1000,    0x0300,    0x0020,    0x5000,    /* , - . / */
	0x0ce1,    0x0c00,    0x0561,    0x0d21,    /* 0 1 2 3 */
	0x0d80,    0x09a1,    0x09e1,    0x0c01,    /* 4 5 6 7 */
	0x0de1,    0x0da1,    0x0021,    0x1001,    /* 8 9 : ; */
	0x5030,    0x0330,    0xa030,    0x0a07,    /* < = > ? */
	0x097f,    0x03cf,    0x0e3f,    0x00f3,    /* @ A B C */
	0x0c3f,    0x01f3,    0x01c3,    0x02fb,    /* D E F G */
	0x03cc,    0x0c33,    0x0c63,    0xc1c0,    /* H I J K */
	0x00f0,    0x60cc,    0xa0cc,    0x00ff,    /* L M N O */
	0x03c7,    0x80ff,    0x83c7,    0x03bb,    /* P Q R S */
	0x0c03,    0x00fc,    0x50c0,    0x90cc,    /* T U V W */
	0xf000,    0x6800,    0x5033,    0x00e1,    /* X Y Z [ */
	0xa000,    0x001e,    0x9000,    0x0030,    /* \ ] ^ _ */
	SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, /* undefined */
	SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, /* undefined */
	SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, /* undefined */
	SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, /* undefined */
	SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, /* undefined */
	SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, /* undefined */
	SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, /* undefined */
	SEG_UNDEF, SEG_UNDEF, SEG_UNDEF, SEG_UNDEF  /* undefined */
};



/*****************************************************************************
 Device interface
*****************************************************************************/

static void *dl1416_start(running_machine *machine, const char *tag,
	const void *static_config, const void *inline_config)
{
	dl1416_t *dl1416;
	char unique_tag[30];

	/* validate arguments */
	assert(tag != NULL);
	assert(strlen(tag) < 20);

	/* allocate the object that holds the state */
	dl1416 = auto_malloc(sizeof(*dl1416));
	memset(dl1416, 0, sizeof(*dl1416));

	dl1416->intf = static_config;

	/* register for state saving */
	state_save_combine_module_and_tag(unique_tag, "dl1416", tag);

	state_save_register_item(unique_tag, 0, dl1416->chip_enable);
	state_save_register_item(unique_tag, 0, dl1416->cursor_enable);
	state_save_register_item(unique_tag, 0, dl1416->write_enable);
	state_save_register_item_array(unique_tag, 0, dl1416->cursor_ram);

	return dl1416;
}


static void dl1416_reset(running_machine *machine, void *token)
{
	int i;
	dl1416_t *chip = token;

	/* Disable all lines */
	chip->chip_enable = FALSE;
	chip->write_enable = FALSE;
	chip->cursor_enable = FALSE;

	/* Randomize cursor memory */
	for (i = 0; i < 4; i++)
		chip->cursor_ram[i] = mame_rand(machine);
}


static void dl1416_set_info(running_machine *machine, void *token, UINT32 state, const deviceinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void dl1416_get_info(running_machine *machine, void *token, UINT32 state, deviceinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;							break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = dl1416_set_info;		break;
		case DEVINFO_FCT_START:							info->start = dl1416_start;				break;
		case DEVINFO_FCT_STOP:							/* Nothing */							break;
		case DEVINFO_FCT_RESET:							info->reset = dl1416_reset;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "DL1416";						break;
		case DEVINFO_STR_FAMILY:						info->s = "DL1416";						break;
		case DEVINFO_STR_VERSION:						info->s = "1.0";						break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__;						break;
		case DEVINFO_STR_CREDITS:						info->s = "Copyright MESS Team";		break;
	}
}



/*****************************************************************************
 Implementation
*****************************************************************************/

/* Write enable, active low */
void dl1416_set_input_w(dl1416_t *chip, int data)
{
	chip->write_enable = !data;
}


/* Chip enable, active low */
void dl1416_set_input_ce(dl1416_t *chip, int data)
{
	chip->chip_enable = !data;
}


/* Cursor enable, active low */
void dl1416_set_input_cu(dl1416_t *chip, int data)
{
	chip->cursor_enable = !data;
}


/* Data */
void dl1416_write(dl1416_t *chip, offs_t offset, UINT8 data)
{
	int i, digit;

	offset &= 0x03; /* A0-A1 */
	data &= 0x7f;   /* D0-D6 */

	/* Only try to update the data if we are enabled and write is enabled */
	if (chip->chip_enable && chip->write_enable)
	{
		if (chip->cursor_enable)
		{
			switch (chip->intf->type)
			{

			case DL1416B:

				/* The cursor will be set if D0 is high and the original */
				/* character restored otherwise */
				digit = data & 1 ? SEG_CURSOR : chip->cursor_ram[offset];

				/* Call update function */
				chip->intf->digit_changed(offset, digit);

				break;

			case DL1416T:

				for (i = 0; i < 4; i++)
				{
					/* Save old digit */
					int previous_digit = chip->cursor_ram[i];

					/* Either set the cursor or restore the original character */
					digit = data & i ? SEG_CURSOR : chip->cursor_ram[i];

					/* Call update function if we changed something */
					if (previous_digit != digit)
						chip->intf->digit_changed(i, digit);
				}
				break;

			}
		}
		else
		{
			/* On the DL1416T, a digit can only be changed if there is no */
			/* previously stored cursor, or overriden by an undefined */
			/* character (blank) */
			if ((chip->intf->type != DL1416T) || (
				(chip->cursor_ram[offset] != SEG_CURSOR) ||
				(dl1416t_segments[data] == SEG_UNDEF)))
			{
				/* Load data */
				digit = dl1416t_segments[data];

				/* Undefined characters are replaced by blanks */
				if (digit == SEG_UNDEF)
					digit = SEG_BLANK;

				/* Save value */
				chip->cursor_ram[offset] = digit;

				/* Call update function */
				chip->intf->digit_changed(offset, digit);
			}
		}
	}
}
