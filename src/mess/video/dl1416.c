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
 *
 * **************************************************************************/


/*****************************************************************************
 Includes
*****************************************************************************/


#include "dl1416.h"



/*****************************************************************************
 Macros
*****************************************************************************/


#define MAX_DL1416 (5)

#define SEG_UNDEF  (-1)
#define SEG_BLANK  (0)
#define SEG_CURSOR (0xffff)



/*****************************************************************************
 Type definitions
*****************************************************************************/

/* DL1416 chip state */
typedef struct _dl1416_state dl1416_state;

struct _dl1416_state
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


static dl1416_state dl1416[MAX_DL1416];



/*****************************************************************************
 Implementation
*****************************************************************************/

/* Config */
void dl1416_config(int which, const dl1416_interface *intf)
{
	assert_always(mame_get_phase(Machine) == MAME_PHASE_INIT, "Can only call dl1416_config at init time!");
	assert_always(which < MAX_DL1416, "'which' exceeds maximum number of configured DL1416s!");

	dl1416[which].intf = intf;

	state_save_register_item("dl1416", which, dl1416[which].chip_enable);
	state_save_register_item("dl1416", which, dl1416[which].cursor_enable);
	state_save_register_item("dl1416", which, dl1416[which].write_enable);
	state_save_register_item_array("dl1416", which, dl1416[which].cursor_ram);
}


/* Reset */
void dl1416_reset(int which)
{
	int i;

	assert_always(which < MAX_DL1416, "'which' exceeds maximum number of configured DL1416s!");

	/* Disable all lines */
	dl1416[which].chip_enable = FALSE;
	dl1416[which].write_enable = FALSE;
	dl1416[which].cursor_enable = FALSE;

	/* Randomize cursor memory */
	for (i = 0; i < 4; i++)
		dl1416[which].cursor_ram[i] = mame_rand(Machine);
}


/* Write enable, active low */
void dl1416_set_input_w(int which, int data)
{
	assert_always(which < MAX_DL1416, "'which' exceeds maximum number of configured DL1416s!");
	dl1416[which].write_enable = !data;
}


/* Chip enable, active low */
void dl1416_set_input_ce(int which, int data)
{
	assert_always(which < MAX_DL1416, "'which' exceeds maximum number of configured DL1416s!");
	dl1416[which].chip_enable = !data;
}


/* Cursor enable, active low */
void dl1416_set_input_cu(int which, int data)
{
	assert_always(which < MAX_DL1416, "'which' exceeds maximum number of configured DL1416s!");
	dl1416[which].cursor_enable = !data;
}


/* Data */
void dl1416_write(int which, offs_t offset, UINT8 data)
{
	int i, digit;

	assert_always(which < MAX_DL1416, "'which' exceeds maximum number of configured DL1416s!");

	offset &= 0x03; /* A0-A1 */
	data &= 0x7f;   /* D0-D6 */

	/* Only try to update the data if we are enabled and write is enabled */
	if (dl1416[which].chip_enable && dl1416[which].write_enable)
	{
		if (dl1416[which].cursor_enable)
		{
			switch (dl1416[which].intf->type)
			{

			case DL1416B:

				/* The cursor will be set if D0 is high and the original */
				/* character restored otherwise */
				digit = data & 1 ? SEG_CURSOR : dl1416[which].cursor_ram[offset];

				/* Call update function */
				dl1416[which].intf->digit_changed(offset, digit);

				break;

			case DL1416T:

				for (i = 0; i < 4; i++)
				{
					/* Save old digit */
					int previous_digit = dl1416[which].cursor_ram[i];

					/* Either set the cursor or restore the original character */
					digit = data & i ? SEG_CURSOR : dl1416[which].cursor_ram[i];

					/* Call update function if we changed something */
					if (previous_digit != digit)
						dl1416[which].intf->digit_changed(i, digit);
				}
				break;

			}
		}
		else
		{
			/* On the DL1416T, a digit can only be changed if there is no */
			/* previously stored cursor, or overriden by an undefined */
			/* character (blank) */
			if ((dl1416[which].intf->type != DL1416T) || (
				(dl1416[which].cursor_ram[offset] != SEG_CURSOR) ||
				(dl1416t_segments[data] == SEG_UNDEF)))
			{
				/* Load data */
				digit = dl1416t_segments[data];

				/* Undefined characters are replaced by blanks */
				if (digit == SEG_UNDEF)
					digit = SEG_BLANK;

				/* Save value */
				dl1416[which].cursor_ram[offset] = digit;

				/* Call update function */
				dl1416[which].intf->digit_changed(offset, digit);
			}
		}
	}
}


/* Standard handlers */
WRITE8_HANDLER( dl1416_0_w ) { dl1416_write(0, offset, data); }
WRITE8_HANDLER( dl1416_1_w ) { dl1416_write(1, offset, data); }
WRITE8_HANDLER( dl1416_2_w ) { dl1416_write(2, offset, data); }
WRITE8_HANDLER( dl1416_3_w ) { dl1416_write(3, offset, data); }
WRITE8_HANDLER( dl1416_4_w ) { dl1416_write(4, offset, data); }
