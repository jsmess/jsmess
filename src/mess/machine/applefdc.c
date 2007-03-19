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
	  -	Implement the unimplemented IWM modes 
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

/* logging */
#define LOG_IWM			0
#define LOG_IWM_EXTRA	0

/* mask for FDC lines */
#define IWM_MOTOR	0x10
#define IWM_DRIVE	0x20
#define IWM_Q6		0x40
#define IWM_Q7		0x80


static UINT8 applefdc_writebyte=0;
static UINT8 applefdc_lines;		/* flags from IWM_MOTOR - IWM_Q7 */

/*
 * IWM mode (iwm_mode):	The IWM mode has the following values:
 *
 * Bit 7	  Reserved
 * Bit 6	  Reserved
 * Bit 5	  Reserved
 * Bit 4	! Clock speed
 *				0=7MHz;	used by Apple IIgs
 *				1=8MHz;	used by Mac (I believe)
 * Bit 3	! Bit cell time
 *				0=4usec/bit	(used for 5.25" drives)
 *				1=2usec/bit (used for 3.5" drives)
 * Bit 2	  Motor-off delay
 *				0=leave on for 1 sec after system turns it off
 *				1=turn off immediately
 * Bit 1	! Handshake protocol
 *				0=synchronous (software supplies timing for writing data; used for 5.25" drives)
 *				1=asynchronous (IWM supplies timing; used for 3.5" drives)
 * Bit 0	! Latch mode
 *				0=read data stays valid for 7usec (used for 5.25" drives)
 *				1=read data stays valid for full byte time (used for 3.5" drives)
 */
enum
{
	IWM_MODE_CLOCKSPEED			= 0x10,
	IWM_MODE_BITCELLTIME		= 0x08,
	IWM_MODE_MOTOROFFDELAY		= 0x04,
	IWM_MODE_HANDSHAKEPROTOCOL	= 0x02,
	IWM_MODE_LATCHMODE			= 0x01
};

static int iwm_mode;		/* 0-31 */
static mame_timer *motor_timer;
static struct applefdc_interface iwm_intf;

static void iwm_turnmotor_onoff(int status);




/***************************************************************************

	IWM code

***************************************************************************/

void applefdc_init(const struct applefdc_interface *intf)
{
	applefdc_lines = 0;
	iwm_mode = 0x1f;	/* default value needed by Lisa 2 - no, I don't know if it is true */
	motor_timer = timer_alloc(iwm_turnmotor_onoff);
	if (intf)
		iwm_intf = *intf;
	else
		memset(&iwm_intf, 0, sizeof(iwm_intf));
}



/* R. Nabet : Next two functions look more like a hack than a real feature of the IWM */
/* I left them there because I had no idea what they intend to do.
They never get called when booting the Mac Plus driver. */
static int iwm_enable2(void)
{
	return (applefdc_lines & APPLEFDC_PH1) && (applefdc_lines & APPLEFDC_PH3);
}



static int iwm_readenable2handshake(void)
{
	static int val = 0;

	if (val++ > 3)
		val = 0;

	return val ? 0xc0 : 0x80;
}



static int applefdc_statusreg_r(void)
{
	/* IWM status:
	 *
	 * Bit 7	Sense input (write protect for 5.25" drive and general status line for 3.5")
	 * Bit 6	Reserved
	 * Bit 5	Drive enable (is 1 if drive is on)
	 * Bits 4-0	Same as IWM mode bits 4-0
	 */

	int result;
	int status;

	status = iwm_enable2() ? 1 : (iwm_intf.read_status ? iwm_intf.read_status() : 0);

	result = (status ? 0x80 : 0x00);

	if (iwm_intf.type != APPLEFDC_APPLE2)
		 result |= (((applefdc_lines & IWM_MOTOR) ? 1 : 0) << 5) | iwm_mode;
	return result;
}



static void iwm_modereg_w(int data)
{
	iwm_mode = data & 0x1f;	/* Write mode register */

	if (LOG_IWM_EXTRA)
		logerror("iwm_modereg_w: iwm_mode=0x%02x\n", (int) iwm_mode);
}



static UINT8 applefdc_read_reg(int lines)
{
	UINT8 result = 0;

	switch(lines)
	{
		case 0:
			/* Read data register */
			if ((iwm_intf.type != APPLEFDC_APPLE2) && (iwm_enable2() || !(applefdc_lines & IWM_MOTOR)))
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
				if (LOG_IWM)
				{
					if ((iwm_mode & IWM_MODE_LATCHMODE) == 0)
						logerror("applefdc_read_reg(): latch mode off not implemented\n");
				}

				result = (iwm_intf.read_data ? iwm_intf.read_data() : 0);
			}
			break;

		case IWM_Q6:
			/* Read status register */
			result = applefdc_statusreg_r();
			break;

		case IWM_Q7:
			/* Classic Apple II: Read status register
			 * IWM: Read handshake register
			 */
			if (iwm_intf.type == APPLEFDC_APPLE2)
				result = applefdc_statusreg_r();
			else
				result = iwm_enable2() ? iwm_readenable2handshake() : 0x80;
			break;
	}
	return result;
}



static void applefdc_write_reg(UINT8 data)
{
	switch(applefdc_lines & (IWM_Q6 | IWM_Q7))
	{
		case IWM_Q6 | IWM_Q7:
			if (!(applefdc_lines & IWM_MOTOR))
			{
				iwm_modereg_w(data);
			}
			else if (!iwm_enable2())
			{
				/*
				 * Right now, this function assumes latch mode; which is always used for
				 * 3.5 inch drives.  Eventually we should check to see if latch mode is
				 * off
				 */
				if (LOG_IWM)
				{
					if ((iwm_mode & IWM_MODE_LATCHMODE) == 0)
						logerror("applefdc_write_reg(): latch mode off not implemented\n");
				}

				if (iwm_intf.write_data)
					iwm_intf.write_data(data);
			}
			break;
	}
}



static void iwm_turnmotor_onoff(int status)
{
	int enable_lines;

	if (status)
	{
		applefdc_lines |= IWM_MOTOR;
		enable_lines = (applefdc_lines & IWM_DRIVE) ? 2 : 1;
	}
	else
	{
		applefdc_lines &= ~IWM_MOTOR;

		if (iwm_intf.type == APPLEFDC_APPLE2)
			enable_lines = (applefdc_lines & IWM_DRIVE) ? 2 : 1;
		else
			enable_lines = 0;
	}

	/* invoke callback, if present */
	if (iwm_intf.set_enable_lines)
		iwm_intf.set_enable_lines(enable_lines);

	if (LOG_IWM_EXTRA)
		logerror("iwm_turnmotor_onoff(): Turning motor %s\n", status ? "on" : "off");
}



static void iwm_access(int offset)
{
	static const char *lines[] =
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

	if (LOG_IWM_EXTRA)
	{
		logerror("iwm_access(): %s line %s\n",
			(offset & 1) ? "setting" : "clearing", lines[offset >> 1]);
	}

	if (offset & 1)
		applefdc_lines |= (1 << (offset >> 1));
	else
		applefdc_lines &= ~(1 << (offset >> 1));

	if ((offset < 0x08) && iwm_intf.set_lines)
		iwm_intf.set_lines(applefdc_lines & 0x0f);

	switch(offset)
	{
		case 0x08:
			/* Turn off motor */
			timer_adjust(motor_timer,
				(iwm_mode & IWM_MODE_MOTOROFFDELAY) ? 0.0 : TIME_IN_SEC(1),
				0,
				0.0);
			break;

		case 0x09:
			/* Turn on motor */
			timer_adjust(motor_timer, 0.0, 1, 0.0);
			break;

		case 0x0A:
			/* turn off IWM_DRIVE */
			if ((applefdc_lines & IWM_MOTOR) && iwm_intf.set_enable_lines)
				iwm_intf.set_enable_lines(1);
			break;

		case 0x0B:
			/* turn on IWM_DRIVE */
			if ((applefdc_lines & IWM_MOTOR) && iwm_intf.set_enable_lines)
				iwm_intf.set_enable_lines(2);
			break;
	}
}



UINT8 applefdc_r(offs_t offset)
{
	UINT8 result = 0;

	offset &= 15;

	if (LOG_IWM_EXTRA)
		logerror("applefdc_r: offset=%i\n", offset);

	iwm_access(offset);

	switch(iwm_intf.type)
	{
		case APPLEFDC_APPLE2:
			switch(offset)
			{
				case 0x0C:
					if (applefdc_lines & IWM_Q7)
					{
						if (iwm_intf.write_data)
							iwm_intf.write_data(applefdc_writebyte);
						result = 0;
					}
					else
						result = applefdc_read_reg(0);
						
					break;
				case 0x0D:
					result = applefdc_read_reg(IWM_Q6);
					break;
				case 0x0E:
					result = applefdc_read_reg(IWM_Q7);
					break;
				case 0x0F:
					result = applefdc_read_reg(IWM_Q7 | IWM_Q6);
					break;
			}
			break;

		case APPLEFDC_IWM:
			if ((offset & 1) == 0)
				result = applefdc_read_reg(applefdc_lines & (IWM_Q6 | IWM_Q7));
			break;

		case APPLEFDC_SWIM:
			fatalerror("NYI");
			break;
	}
	return result;
}



void applefdc_w(offs_t offset, UINT8 data)
{
	offset &= 15;

	if (LOG_IWM_EXTRA)
		logerror("applefdc_w: offset=%i data=0x%02x\n", offset, data);

	iwm_access(offset);

	switch(iwm_intf.type)
	{
		case APPLEFDC_APPLE2:
			switch(offset)
			{
				case 0x0C:
					if (applefdc_lines & IWM_Q7)
					{
						if (iwm_intf.write_data)
							iwm_intf.write_data(applefdc_writebyte);
					}
					break;

				case 0x0D:
					applefdc_writebyte = data;
					break;
			}
			break;

		case APPLEFDC_IWM:
			if (offset & 1)
				applefdc_write_reg(data);
			break;

		case APPLEFDC_SWIM:
			fatalerror("NYI");
			break;
	}
}



UINT8 applefdc_get_lines(void)
{
	return applefdc_lines & 0x0f;
}
