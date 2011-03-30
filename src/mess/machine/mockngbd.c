/*********************************************************************

    mockngbd.c

    Implementation of the Apple II Mockingboard

    The Mockingboard comes in different flavours:

    * Sound I:     one AY-3-8910 chip for three audio channels
    * Speech I:    one SC-01 chip
    * Sound II:    two AY-3-8910 chips for six audio channels
    * Sound/Speech I:  one AY-3-8910 and one SC-01

    * Mockingboard A:  two AY-3-8913 chips for six audio channels and two open sockets for SSI-263 speech chips
    * Mockingboard B:  not a soundcard per se, but just two SSI-263 speech chip upgrade for Mockingboard A
    * Mockingboard C:  two AY-3-8913 and one SSI-263 (essentially a Mockingboard A with the upgrade pre-installed, only one speech chip allowed)
    * Mockingboard D:  for Apple IIc only, two AY-3-8913 and one SSI-263, connected to the serial port and its own particular driver
    * Mockingboard M:  bundled with Mindscape's Bank Street Music Writer, with two AY-3-8913 chips and an open socket
                       for one speech chip. This model included a headphone jack and a jumper to permit sound to be played
                       through the Apple's built-in speaker.
    * Mockingboard v1: a clone of the Mockingboard A from ReactiveMicro.com


    TODO - When sound cores and the 6522 VIA become devices, and devices
    can contain other devices, start containing AY8910 and 6522VIA
    implementations

*********************************************************************/

#include "mockngbd.h"
#include "sound/ay8910.h"


/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG_MOCKINGBOARD	0


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _mockingboard_token mockingboard_token;
struct _mockingboard_token
{
	UINT8 flip1;
	UINT8 flip2;
	UINT8 latch0;
	UINT8 latch1;
};


/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE mockingboard_token *get_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == MOCKINGBOARD);
	return (mockingboard_token *) downcast<legacy_device_base *>(device)->token();
}



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/*-------------------------------------------------
    DEVICE_START(mockingboard) - device start
    function
-------------------------------------------------*/

static DEVICE_START(mockingboard)
{
}



/*-------------------------------------------------
    DEVICE_RESET(mockingboard) - device reset
    function
-------------------------------------------------*/

static DEVICE_RESET(mockingboard)
{
	mockingboard_token *token = get_token(device);
	token->flip1 = 0x00;
	token->flip2 = 0x00;
	token->latch0 = 0x00;
	token->latch1 = 0x00;

	/* TODO: fix this */
	/* What follows is pure filth. It abuses the core like an angry pimp on a bad hair day. */

	/* Since we know that the Mockingboard has no code ROM, we'll copy into the slot ROM space
       an image of the onboard ROM so that when an IRQ bankswitches to the onboard ROM, we read
       the proper stuff. Without this, it will choke and try to use the memory handler above, and
       fail miserably. That should really be fixed. I beg you -- if you are reading this comment,
       fix this :) */
//  memcpy (apple2_slotrom(slot), &apple_rom[0x0000 + (slot * 0x100)], 0x100);
}



/*-------------------------------------------------
    mockingboard_r - device read function
-------------------------------------------------*/

READ8_DEVICE_HANDLER(mockingboard_r)
{
	UINT8 result = 0x00;
	mockingboard_token *token = get_token(device);

	switch (offset)
	{
		/* This is used to ID the board */
		case 0x04:
			token->flip1 ^= 0x08;
			result = token->flip1;
			break;

		case 0x84:
			token->flip2 ^= 0x08;
			result = token->flip2;
			break;

		default:
			if (LOG_MOCKINGBOARD)
				logerror("mockingboard_r unmapped, offset: %02x, pc: %s\n", offset, device->machine().describe_context());
			break;
	}
	return 0x00;
}



/*-------------------------------------------------
    mockingboard_w - device write function
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER(mockingboard_w)
{
	device_t *ay8910_1 = device->machine().device("ay8910.1");
	device_t *ay8910_2 = device->machine().device("ay8910.2");
	mockingboard_token *token = get_token(device);

	if (LOG_MOCKINGBOARD)
		logerror("mockingboard_w, $%02x:%02x\n", offset, data);

	/* There is a 6522 in here which interfaces to the 8910s */
	switch (offset)
	{
		case 0x00: /* ORB1 */
			switch (data)
			{
				case 0x00: /* reset */
					ay8910_1->reset();
					break;
				case 0x04: /* make inactive */
					break;
				case 0x06: /* write data */
					ay8910_data_w(ay8910_1, 0, token->latch0);
					break;
				case 0x07: /* set register */
					ay8910_address_w(ay8910_1, 0, token->latch0);
					break;
			}
			break;

		case 0x01: /* ORA1 */
			token->latch0 = data;
			break;

		case 0x02: /* DDRB1 */
		case 0x03: /* DDRA1 */
			break;

		case 0x80: /* ORB2 */
			switch (data)
			{
				case 0x00: /* reset */
					ay8910_2->reset();
					break;
				case 0x04: /* make inactive */
					break;
				case 0x06: /* write data */
					ay8910_data_w(ay8910_2, 0, token->latch1);
					break;
				case 0x07: /* set register */
					ay8910_address_w(ay8910_2, 0, token->latch1);
					break;
			}
			break;

		case 0x81: /* ORA2 */
			token->latch1 = data;
			break;

		case 0x82: /* DDRB2 */
		case 0x83: /* DDRA2 */
			break;
	}
}



/*-------------------------------------------------
    DEVICE_GET_INFO(mockingboard) - device get info
    function
-------------------------------------------------*/

DEVICE_GET_INFO(mockingboard)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(mockingboard_token);		break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(mockingboard);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(mockingboard);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Apple II Mockingboard");			break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Apple II Mockingboard");			break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
	}
}

DEFINE_LEGACY_DEVICE(MOCKINGBOARD, mockingboard);
