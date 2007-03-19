/*********************************************************************

	8530scc.c

	Zilog 8530 SCC (Serial Control Chip) code

*********************************************************************/

#include "8530scc.h"

#define LOG_SCC	0

static const struct scc8530_interface *scc_intf;
static int scc_mode;
static int scc_reg;
static int scc_status;



void scc_init(const struct scc8530_interface *intf)
{
	scc_intf = intf;
	scc_mode = 0;
	scc_reg = 0;
	scc_status = 0;
}



void scc_set_status(int status)
{
	scc_status = status;
}



static int scc_getareg(void)
{
	/* Not yet implemented */
	return 0;
}



static int scc_getbreg(void)
{

	if (scc_reg == 2)
		return scc_status;

	return 0;
}



static void scc_putareg(int data)
{
	if (scc_reg == 0)
	{
		if (data & 0x10)
		{
			if (scc_intf && scc_intf->acknowledge)
				scc_intf->acknowledge();
		}
	}
}



static void scc_putbreg(int data)
{
	if (scc_reg == 0)
	{
		if (data & 0x10)
		{
			if (scc_intf && scc_intf->acknowledge)
				scc_intf->acknowledge();
		}
	}
}



READ8_HANDLER(scc_r)
{
	UINT8 result = 0;

	offset %= 4;

	if (LOG_SCC)
		logerror("scc_r: offset=%u\n", (unsigned int) offset);

	switch(offset)
	{
		case 0:
			/* Channel B (Printer Port) Control */
			if (scc_mode == 1)
				scc_mode = 0;
			else
				scc_reg = 0;

			result = scc_getbreg();
			break;

		case 1:
			/* Channel A (Modem Port) Control */
			if (scc_mode == 1)
				scc_mode = 0;
			else
				scc_reg = 0;

			result = scc_getareg();
			break;

		case 2:
			/* Channel B (Printer Port) Data */
			/* Not yet implemented */
			break;

		case 3:
			/* Channel A (Modem Port) Data */
			/* Not yet implemented */
			break;
	}
	return result;
}



WRITE8_HANDLER(scc_w)
{
	offset &= 3;

	switch(offset)
	{
		case 0:
			/* Channel B (Printer Port) Control */
			if (scc_mode == 0)
			{
				scc_mode = 1;
				scc_reg = data & 0x0f;
				scc_putbreg(data & 0xf0);
			}
			else
			{
				scc_mode = 0;
				scc_putbreg(data);
			}
			break;

		case 1:
			/* Channel A (Modem Port) Control */
			if (scc_mode == 0)
			{
				scc_mode = 1;
				scc_reg = data & 0x0f;
				scc_putareg(data & 0xf0);
			}
			else
			{
				scc_mode = 0;
				scc_putareg(data);
			}
			break;

		case 2:
			/* Channel B (Printer Port) Data */
			/* Not yet implemented */
			break;

		case 3:
			/* Channel A (Modem Port) Data */
			/* Not yet implemented */
			break;
	}
}

