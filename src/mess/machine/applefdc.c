/*********************************************************************

    applefdc.c

    Implementation of various Apple Floppy Disk Controllers, including
    the classic Apple controller and the IWM (Integrated Woz Machine)
    chip

    The IWM chip was used as the floppy disk controller for early Macs and the
    Apple IIgs, and was eventually superceded by the SWIM chp.

    Nate Woods
    Raphael Nabet

    Writing this code would not be possible if it weren't for the work of the
    XGS and KEGS emulators which also contain IWM emulations.

    TODO
      - Implement the unimplemented IWM modes
            - IWM_MODE_CLOCKSPEED
            - IWM_MODE_BITCELLTIME
            - IWM_MODE_HANDSHAKEPROTOCOL
            - IWM_MODE_LATCHMODE
      - Investigate the differences between the IWM and the classic Apple II
        controller more fully.  It is currently unclear what are genuine
        differences and what are effectively hacks that "just seem" to work.
      - Figure out iwm_readenable2handshake() and iwm_enable2(); they are
        hackish at best
      - Support the SWIM chip
      - Proper timing
      - This code was originally IWM specific; we need to clean up IWMisms in
        the code
      - Make it faster?
      - Add sound?

*********************************************************************/

#include "applefdc.h"


/***************************************************************************
    PARAMETERS
***************************************************************************/

/* logging */
#define LOG_APPLEFDC		0
#define LOG_APPLEFDC_EXTRA	0



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* mask for FDC lines */
#define IWM_MOTOR	0x10
#define IWM_DRIVE	0x20
#define IWM_Q6		0x40
#define IWM_Q7		0x80

typedef enum
{
	APPLEFDC_APPLE2,	/* classic Apple II disk controller (pre-IWM) */
	APPLEFDC_IWM,		/* Integrated Woz Machine */
	APPLEFDC_SWIM		/* Steve Woz Integrated Machine (NYI) */
} applefdc_t;



/***************************************************************************
    IWM MODE

    The IWM mode has the following values:

    Bit 7     Reserved
    Bit 6     Reserved
    Bit 5     Reserved
    Bit 4   ! Clock speed
                0=7MHz; used by Apple IIgs
                1=8MHz; used by Mac (I believe)
    Bit 3   ! Bit cell time
                0=4usec/bit (used for 5.25" drives)
                1=2usec/bit (used for 3.5" drives)
    Bit 2     Motor-off delay
                0=leave on for 1 sec after system turns it off
                1=turn off immediately
    Bit 1   ! Handshake protocol
                0=synchronous (software supplies timing for writing data; used for 5.25" drives)
                1=asynchronous (IWM supplies timing; used for 3.5" drives)
    Bit 0   ! Latch mode
                0=read data stays valid for 7usec (used for 5.25" drives)
                1=read data stays valid for full byte time (used for 3.5" drives)

 ***************************************************************************/

enum
{
	IWM_MODE_CLOCKSPEED			= 0x10,
	IWM_MODE_BITCELLTIME		= 0x08,
	IWM_MODE_MOTOROFFDELAY		= 0x04,
	IWM_MODE_HANDSHAKEPROTOCOL	= 0x02,
	IWM_MODE_LATCHMODE			= 0x01
};



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _applefdc_token applefdc_token;
struct _applefdc_token
{
	/* data that is constant for the lifetime of the emulation */
	emu_timer *motor_timer;
	applefdc_t type;

	/* data that changes at emulation time */
	UINT8 write_byte;
	UINT8 lines;					/* flags from IWM_MOTOR - IWM_Q7 */
	UINT8 mode;						/* 0-31; see above */
	UINT8 handshake_hack;			/* not sure what this is for */
};



/***************************************************************************
    PROTOTYPES
***************************************************************************/

static TIMER_CALLBACK(iwm_turnmotor_onoff);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE void assert_is_applefdc(running_device *device)
{
	assert(device != NULL);
	assert((device->type == APPLEFDC) || (device->type == IWM) || (device->type == SWIM));
}



INLINE applefdc_token *get_token(running_device *device)
{
	assert_is_applefdc(device);
	return (applefdc_token *) device->token;
}



INLINE const applefdc_interface *get_interface(running_device *device)
{
	static const applefdc_interface dummy_interface = {0, };

	assert_is_applefdc(device);
	return (device->baseconfig().static_config != NULL)
		? (const applefdc_interface *) device->baseconfig().static_config
		: &dummy_interface;
}



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    applefdc_start - starts up an FDC
-------------------------------------------------*/

static void applefdc_start(running_device *device, applefdc_t type)
{
	applefdc_token *fdc = get_token(device);

	memset(fdc, 0, sizeof(*fdc));
	fdc->type = type;
	fdc->motor_timer = timer_alloc(device->machine, iwm_turnmotor_onoff, (void *) device);
	fdc->lines = 0x00;
	fdc->mode = 0x1F;	/* default value needed by Lisa 2 - no, I don't know if it is true */

	/* register save states */
	state_save_register_item(device->machine, "applefdc", NULL, 0, fdc->write_byte);
	state_save_register_item(device->machine, "applefdc", NULL, 0, fdc->lines);
	state_save_register_item(device->machine, "applefdc", NULL, 0, fdc->mode);
	state_save_register_item(device->machine, "applefdc", NULL, 0, fdc->handshake_hack);
}



/*-------------------------------------------------
    DEVICE_RESET(applefdc) - resets an FDC
-------------------------------------------------*/

static DEVICE_RESET(applefdc)
{
	applefdc_token *fdc = get_token(device);

	fdc->handshake_hack = 0x00;
	fdc->write_byte = 0x00;
	fdc->lines = 0x00;
	fdc->mode = 0x1F;	/* default value needed by Lisa 2 - no, I don't know if it is true */

	timer_reset(fdc->motor_timer, attotime_never);
}



/*-------------------------------------------------
    iwm_enable2 - hackish function
-------------------------------------------------*/

static int iwm_enable2(running_device *device)
{
	applefdc_token *fdc = get_token(device);

	/* R. Nabet : This function looks more like a hack than a real feature of the IWM; */
	/* it is not called from the Mac Plus driver */
	return (fdc->lines & APPLEFDC_PH1) && (fdc->lines & APPLEFDC_PH3);
}



/*-------------------------------------------------
    iwm_readenable2handshake - hackish function
-------------------------------------------------*/

static UINT8 iwm_readenable2handshake(running_device *device)
{
	applefdc_token *fdc = get_token(device);

	/* R. Nabet : This function looks more like a hack than a real feature of the IWM; */
	/* it is not called from the Mac Plus driver */
	fdc->handshake_hack++;
	fdc->handshake_hack %= 4;
	return (fdc->handshake_hack != 0) ? 0xc0 : 0x80;
}



/*-------------------------------------------------
    applefdc_statusreg_r - reads the status register
-------------------------------------------------*/

static UINT8 applefdc_statusreg_r(running_device *device)
{
	UINT8 result;
	int status;
	applefdc_token *fdc = get_token(device);
	const applefdc_interface *intf = get_interface(device);

	/* IWM status:
     *
     * Bit 7    Sense input (write protect for 5.25" drive and general status line for 3.5")
     * Bit 6    Reserved
     * Bit 5    Drive enable (is 1 if drive is on)
     * Bits 4-0 Same as IWM mode bits 4-0
     */

	status = iwm_enable2(device) ? 1 : (intf->read_status ? intf->read_status(device) : 0);

	result = (status ? 0x80 : 0x00);

	if (fdc->type != APPLEFDC_APPLE2)
		 result |= (((fdc->lines & IWM_MOTOR) ? 1 : 0) << 5) | fdc->mode;
	return result;
}



/*-------------------------------------------------
    iwm_modereg_w - changes the mode register
-------------------------------------------------*/

static void iwm_modereg_w(running_device *device, UINT8 data)
{
	applefdc_token *fdc = get_token(device);

	fdc->mode = data & 0x1f;	/* write mode register */

	if (LOG_APPLEFDC_EXTRA)
		logerror("iwm_modereg_w: iwm_mode=0x%02x\n", (unsigned) fdc->mode);
}



/*-------------------------------------------------
    applefdc_read_reg - reads a register
-------------------------------------------------*/

static UINT8 applefdc_read_reg(running_device *device, int lines)
{
	applefdc_token *fdc = get_token(device);
	const applefdc_interface *intf = get_interface(device);
	UINT8 result = 0;

	switch(lines)
	{
		case 0:
			/* Read data register */
			if ((fdc->type != APPLEFDC_APPLE2) && (iwm_enable2(device) || !(fdc->lines & IWM_MOTOR)))
			{
				result = 0xFF;
			}
			else
			{
				/*
                 * Right now, this function assumes latch mode; which is always used for
                 * 3.5 inch drives.  Eventually we should check to see if latch mode is
                 * off
                 */
				if (LOG_APPLEFDC)
				{
					if ((fdc->mode & IWM_MODE_LATCHMODE) == 0x00)
						logerror("applefdc_read_reg(): latch mode off not implemented\n");
				}

				result = (intf->read_data ? intf->read_data(device) : 0x00);
			}
			break;

		case IWM_Q6:
			/* Read status register */
			result = applefdc_statusreg_r(device);
			break;

		case IWM_Q7:
			/* Classic Apple II: Read status register
             * IWM: Read handshake register
             */
			if (fdc->type == APPLEFDC_APPLE2)
				result = applefdc_statusreg_r(device);
			else
				result = iwm_enable2(device) ? iwm_readenable2handshake(device) : 0x80;
			break;
	}
	return result;
}



/*-------------------------------------------------
    applefdc_write_reg - writes a register
-------------------------------------------------*/

static void applefdc_write_reg(running_device *device, UINT8 data)
{
	applefdc_token *fdc = get_token(device);
	const applefdc_interface *intf = get_interface(device);

	switch(fdc->lines & (IWM_Q6 | IWM_Q7))
	{
		case IWM_Q6 | IWM_Q7:
			if (!(fdc->lines & IWM_MOTOR))
			{
				iwm_modereg_w(device, data);
			}
			else if (!iwm_enable2(device))
			{
				/*
                 * Right now, this function assumes latch mode; which is always used for
                 * 3.5 inch drives.  Eventually we should check to see if latch mode is
                 * off
                 */
				if (LOG_APPLEFDC)
				{
					if ((fdc->mode & IWM_MODE_LATCHMODE) == 0)
						logerror("applefdc_write_reg(): latch mode off not implemented\n");
				}

				if (intf->write_data != NULL)
					intf->write_data(device,data);
			}
			break;
	}
}



/*-------------------------------------------------
    TIMER_CALLBACK(iwm_turnmotor_onoff) - timer
    callback for turning motor on or off
-------------------------------------------------*/

static TIMER_CALLBACK(iwm_turnmotor_onoff)
{
	running_device *device = (running_device *) ptr;
	applefdc_token *fdc = get_token(device);
	const applefdc_interface *intf = get_interface(device);
	int status = param;
	int enable_lines;

	if (status != 0)
	{
		fdc->lines |= IWM_MOTOR;
		enable_lines = (fdc->lines & IWM_DRIVE) ? 2 : 1;
	}
	else
	{
		fdc->lines &= ~IWM_MOTOR;

		if (fdc->type == APPLEFDC_APPLE2)
			enable_lines = (fdc->lines & IWM_DRIVE) ? 2 : 1;
		else
			enable_lines = 0;
	}

	/* invoke callback, if present */
	if (intf->set_enable_lines != NULL)
		intf->set_enable_lines(device,enable_lines);

	if (LOG_APPLEFDC_EXTRA)
		logerror("iwm_turnmotor_onoff(): Turning motor %s\n", status ? "on" : "off");
}



/*-------------------------------------------------
    iwm_access
-------------------------------------------------*/

static void iwm_access(running_device *device, int offset)
{
	static const char *const lines[] =
	{
		"PH0",
		"PH1",
		"PH2",
		"PH3",
		"MOTOR",
		"DRIVE",
		"Q6",
		"Q7"
	};

	applefdc_token *fdc = get_token(device);
	const applefdc_interface *intf = get_interface(device);

	if (LOG_APPLEFDC_EXTRA)
	{
		logerror("iwm_access(): %s line %s\n",
			(offset & 1) ? "setting" : "clearing", lines[offset >> 1]);
	}

	if (offset & 1)
		fdc->lines |= (1 << (offset >> 1));
	else
		fdc->lines &= ~(1 << (offset >> 1));

	if ((offset < 0x08) && (intf->set_lines != NULL))
		intf->set_lines(device,fdc->lines & 0x0f);

	switch(offset)
	{
		case 0x08:
			/* turn off motor */
			timer_adjust_oneshot(fdc->motor_timer,
				(fdc->mode & IWM_MODE_MOTOROFFDELAY) ? attotime_zero : ATTOTIME_IN_SEC(1), 0);
			break;

		case 0x09:
			/* turn on motor */
			timer_adjust_oneshot(fdc->motor_timer, attotime_zero, 1);
			break;

		case 0x0A:
			/* turn off IWM_DRIVE */
			if ((fdc->lines & IWM_MOTOR) && (intf->set_enable_lines != NULL))
				intf->set_enable_lines(device,1);
			break;

		case 0x0B:
			/* turn on IWM_DRIVE */
			if ((fdc->lines & IWM_MOTOR) && (intf->set_enable_lines != NULL))
				intf->set_enable_lines(device,2);
			break;
	}
}



/*-------------------------------------------------
    applefdc_r - reads a byte from the FDC
-------------------------------------------------*/

READ8_DEVICE_HANDLER( applefdc_r )
{
	applefdc_token *fdc = get_token(device);
	const applefdc_interface *intf = get_interface(device);
	UINT8 result = 0;

	/* normalize offset */
	offset &= 15;

	if (LOG_APPLEFDC_EXTRA)
		logerror("applefdc_r: offset=%i\n", offset);

	iwm_access(device, offset);

	switch(fdc->type)
	{
		case APPLEFDC_APPLE2:
			switch(offset)
			{
				case 0x0C:
					if (fdc->lines & IWM_Q7)
					{
						if (intf->write_data != NULL)
							intf->write_data(device,fdc->write_byte);
						result = 0;
					}
					else
						result = applefdc_read_reg(device, 0);

					break;
				case 0x0D:
					result = applefdc_read_reg(device, IWM_Q6);
					break;
				case 0x0E:
					result = applefdc_read_reg(device, IWM_Q7);
					break;
				case 0x0F:
					result = applefdc_read_reg(device, IWM_Q7 | IWM_Q6);
					break;
			}
			break;

		case APPLEFDC_IWM:
			if ((offset & 1) == 0)
				result = applefdc_read_reg(device, fdc->lines & (IWM_Q6 | IWM_Q7));
			break;

		case APPLEFDC_SWIM:
			fatalerror("NYI");
			break;
	}
	return result;
}



/*-------------------------------------------------
    applefdc_w - writes a byte to the FDC
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( applefdc_w )
{
	applefdc_token *fdc = get_token(device);
	const applefdc_interface *intf = get_interface(device);

	/* normalize offset */
	offset &= 15;

	if (LOG_APPLEFDC_EXTRA)
		logerror("applefdc_w: offset=%i data=0x%02x\n", offset, data);

	iwm_access(device, offset);

	switch(fdc->type)
	{
		case APPLEFDC_APPLE2:
			switch(offset)
			{
				case 0x0C:
					if (fdc->lines & IWM_Q7)
					{
						if (intf->write_data != NULL)
							intf->write_data(device,fdc->write_byte);
					}
					break;

				case 0x0D:
					fdc->write_byte = data;
					break;
			}
			break;

		case APPLEFDC_IWM:
			if (offset & 1)
				applefdc_write_reg(device, data);
			break;

		case APPLEFDC_SWIM:
			fatalerror("NYI");
			break;
	}
}



/*-------------------------------------------------
    applefdc_w - writes a byte to the FDC
-------------------------------------------------*/

UINT8 applefdc_get_lines(running_device *device)
{
	applefdc_token *fdc = get_token(device);
	return fdc->lines & 0x0f;
}



/***************************************************************************
    INTERFACE
***************************************************************************/

/*-------------------------------------------------
    DEVICE_GET_INFO(applefdc_base) - device get info
    callback
-------------------------------------------------*/

static DEVICE_GET_INFO(applefdc_base)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(applefdc_token);			break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							/* Nothing */								break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(applefdc);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Apple FDC");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
	}
}



/*-------------------------------------------------
    DEVICE_START(oldfdc) - device start
    callback
-------------------------------------------------*/

static DEVICE_START(oldfdc)
{
	applefdc_start(device, APPLEFDC_APPLE2);
}



/*-------------------------------------------------
    DEVICE_GET_INFO(applefdc) - device get info
    callback
-------------------------------------------------*/

DEVICE_GET_INFO(applefdc)
{
	switch (state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Apple FDC");				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(oldfdc);	break;

		default: 										DEVICE_GET_INFO_CALL(applefdc_base);	break;
	}
}



/*-------------------------------------------------
    DEVICE_START(iwm) - device start
    callback
-------------------------------------------------*/

static DEVICE_START(iwm)
{
	applefdc_start(device, APPLEFDC_IWM);
}



/*-------------------------------------------------
    DEVICE_GET_INFO(iwm) - device get info
    callback
-------------------------------------------------*/

DEVICE_GET_INFO(iwm)
{
	switch (state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Apple IWM (Integrated Woz Machine)");				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(iwm);	break;

		default: 										DEVICE_GET_INFO_CALL(applefdc_base);	break;
	}
}



/*-------------------------------------------------
    DEVICE_START(iwm) - device start
    callback
-------------------------------------------------*/

static DEVICE_START(swim)
{
	applefdc_start(device, APPLEFDC_SWIM);
}



/*-------------------------------------------------
    DEVICE_GET_INFO(swim) - device get info
    callback
-------------------------------------------------*/

DEVICE_GET_INFO(swim)
{
	switch (state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Apple SWIM (Steve Woz Integrated Machine)");				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(swim);	break;

		default: 										DEVICE_GET_INFO_CALL(applefdc_base);	break;
	}
}
