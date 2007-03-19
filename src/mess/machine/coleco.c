/***************************************************************************

  coleco.c

  Machine file to handle emulation of the Colecovision.

  TODO:
	- Extra controller support
***************************************************************************/

#include "driver.h"
#include "video/tms9928a.h"
#include "includes/coleco.h"
#include "devices/cartslot.h"
#include "image.h"

static int JoyMode=0;
int JoyStat[2];

int coleco_cart_verify(const UINT8 *cartdata, size_t size)
{
	int retval = IMAGE_VERIFY_FAIL;

	/* Verify the file is in Colecovision format */
	if ((cartdata[0] == 0xAA) && (cartdata[1] == 0x55)) /* Production Cartridge */
		retval = IMAGE_VERIFY_PASS;
	if ((cartdata[0] == 0x55) && (cartdata[1] == 0xAA)) /* "Test" Cartridge. Some games use this method to skip ColecoVision title screen and delay */
		retval = IMAGE_VERIFY_PASS;

	return retval;
}

READ8_HANDLER ( coleco_paddle_r )
{
	/* Player 1 */
	if ((offset & 0x02)==0)
	{
		/* Keypad and fire 1 (SAC Yellow Button) */
		if (JoyMode==0)
		{
			int inport0,inport1,inport6,data;

			inport0 = input_port_0_r(0);
			inport1 = input_port_1_r(0);
			inport6 = input_port_6_r(0);
			
			/* Numeric pad buttons are not independent on a real ColecoVision, if you push more 
			than one, a real ColecoVision think that it is a third button, so we are going to emulate
			the right behaviour */
			
			data = 0x0F;	/* No key pressed by default */
			if (!(inport6&0x01)) /* If Driving Controller enabled -> no keypad 1*/
			{
			    if (!(inport0 & 0x01)) data &= 0x0A; /* 0 */
			    if (!(inport0 & 0x02)) data &= 0x0D; /* 1 */
			    if (!(inport0 & 0x04)) data &= 0x07; /* 2 */
			    if (!(inport0 & 0x08)) data &= 0x0C; /* 3 */
			    if (!(inport0 & 0x10)) data &= 0x02; /* 4 */
			    if (!(inport0 & 0x20)) data &= 0x03; /* 5 */
			    if (!(inport0 & 0x40)) data &= 0x0E; /* 6 */
			    if (!(inport0 & 0x80)) data &= 0x05; /* 7 */
			    if (!(inport1 & 0x01)) data &= 0x01; /* 8 */
			    if (!(inport1 & 0x02)) data &= 0x0B; /* 9 */
			    if (!(inport1 & 0x04)) data &= 0x06; /* # */
			    if (!(inport1 & 0x08)) data &= 0x09; /* . */
			}
			return (inport1 & 0x70) | (data);
		}
		/* Joystick and fire 2 (SAC Red Button) */
		else
		{
			int data = input_port_2_r(0) & 0xCF;
			int inport6 = input_port_6_r(0);
			
			if (inport6&0x07) /* If Extra Contollers enabled */
			{
			    if (JoyStat[0]==0) data|=0x30; /* Spinner Move Left*/
			    else if (JoyStat[0]==1) data|=0x20; /* Spinner Move Right */
			}  
			return data | 0x80;
		}
	}
	/* Player 2 */
	else
	{
		/* Keypad and fire 1 */
		if (JoyMode==0)
		{
			int inport3,inport4,data;

			inport3 = input_port_3_r(0);
			inport4 = input_port_4_r(0);

			/* Numeric pad buttons are not independent on a real ColecoVision, if you push more 
			than one, a real ColecoVision think that it is a third button, so we are going to emulate
			the right behaviour */
			
			data = 0x0F;	/* No key pressed by default */
			if (!(inport3 & 0x01)) data &= 0x0A; /* 0 */
			if (!(inport3 & 0x02)) data &= 0x0D; /* 1 */
			if (!(inport3 & 0x04)) data &= 0x07; /* 2 */
			if (!(inport3 & 0x08)) data &= 0x0C; /* 3 */
			if (!(inport3 & 0x10)) data &= 0x02; /* 4 */
			if (!(inport3 & 0x20)) data &= 0x03; /* 5 */
			if (!(inport3 & 0x40)) data &= 0x0E; /* 6 */
			if (!(inport3 & 0x80)) data &= 0x05; /* 7 */
			if (!(inport4 & 0x01)) data &= 0x01; /* 8 */
			if (!(inport4 & 0x02)) data &= 0x0B; /* 9 */
			if (!(inport4 & 0x04)) data &= 0x06; /* # */
			if (!(inport4 & 0x08)) data &= 0x09; /* . */

			return (inport4 & 0x70) | (data);
		}
		/* Joystick and fire 2*/
		else
		{
			int data = input_port_5_r(0) & 0xCF;
			int inport6 = input_port_6_r(0);
			
			if (inport6&0x02) /* If Roller Controller enabled */
			{
			    if (JoyStat[1]==0) data|=0x30;
			    else if (JoyStat[1]==1) data|=0x20;
			}  

			return data | 0x80;
		}
	}

}


WRITE8_HANDLER ( coleco_paddle_toggle_off )
{
	JoyMode=0;
    return;
}

WRITE8_HANDLER ( coleco_paddle_toggle_on )
{
	JoyMode=1;
    return;
}

