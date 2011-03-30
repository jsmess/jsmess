/*********************************************************************

    a2cffa.c

    Implementation of Rich Dreher's IDE/CompactFlash card
    for the Apple II.

    http://www.dreher.net/

*********************************************************************/

#include "a2cffa.h"
#include "includes/apple2.h"
#include "machine/idectrl.h"


/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG_A2CFFA	1


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _a2cffa_token a2cffa_token;
struct _a2cffa_token
{
	UINT8 *ROM;		// card ROM
	UINT16 lastdata;	// last data from IDE
};


/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE a2cffa_token *get_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == A2CFFA);
	return (a2cffa_token *)downcast<legacy_device_base *>(device)->token();
}



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/*-------------------------------------------------
    DEVICE_START(a2cffa) - device start
    function
-------------------------------------------------*/

static DEVICE_START(a2cffa)
{
}



/*-------------------------------------------------
    DEVICE_RESET(a2cffa) - device reset
    function
-------------------------------------------------*/

static DEVICE_RESET(a2cffa)
{
	a2cffa_token *token = get_token(device);

	// find card ROM
	token->ROM = (UINT8 *)device->machine().region("cffa")->base();
}



/*-------------------------------------------------
    a2cffa_r - device read function
-------------------------------------------------*/

READ8_DEVICE_HANDLER(a2cffa_r)
{
	a2cffa_token *token = get_token(device);

	if (offset == 0)
	{
		return token->lastdata>>8;
	}
	else if (offset == 8)
	{
		token->lastdata = ide_controller_r(device->machine().m_devicelist.find("ide"), 0x1f0+offset-8, 2);
		return token->lastdata & 0xff;
	}
	else if (offset > 8)
	{
		return ide_controller_r(device->machine().m_devicelist.find("ide"), 0x1f0+offset-8, 1);
	}

	return 0xff;
}



/*-------------------------------------------------
    a2cffa_w - device write function
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER(a2cffa_w)
{
	a2cffa_token *token = get_token(device);

	if (offset == 0)
	{
		token->lastdata &= 0x00ff;
		token->lastdata |= data<<8;
	}
	else if (offset == 8)
	{
		token->lastdata &= 0xff00;
		token->lastdata |= data;
		ide_controller_w(device->machine().m_devicelist.find("ide"), 0x1f0+offset-8, 2, token->lastdata);
	}
	else if (offset > 8)
	{
		ide_controller_w(device->machine().m_devicelist.find("ide"), 0x1f0+offset-8, 1, data);
	}
}


/* slot ext. ROM (C800) read function */
READ8_DEVICE_HANDLER(a2cffa_c800_r)
{
	a2cffa_token *token = get_token(device);

	return token->ROM[offset+0x800];
}

/* slot ext. ROM (C800) write function */
WRITE8_DEVICE_HANDLER(a2cffa_c800_w)
{
}

/* slot ROM (CN00) read function */
READ8_DEVICE_HANDLER(a2cffa_cnxx_r)
{
	a2cffa_token *token = get_token(device);

	return token->ROM[offset+0x700];	// this needs to be = to the slot #, we assume 7 for now
}

/* slot ROM (CN00) write function */
WRITE8_DEVICE_HANDLER(a2cffa_cnxx_w)
{
}


/*-------------------------------------------------
    DEVICE_GET_INFO(a2cffa) - device get info
    function
-------------------------------------------------*/

DEVICE_GET_INFO(a2cffa)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(a2cffa_token);		break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(a2cffa);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(a2cffa);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Apple II CFFA Card (www.dreher.net)");			break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Apple II CFFA Card (www.dreher.net)");			break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
	}
}

DEFINE_LEGACY_DEVICE(A2CFFA, a2cffa);
