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

unsigned char scc_reg_val_a[16];
unsigned char scc_reg_val_b[16];


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
	logerror("SCC: port A reg %i read 0x%02x\n",scc_reg,scc_reg_val_a[scc_reg]);
	return scc_reg_val_a[scc_reg];
}



static int scc_getbreg(void)
{
	logerror("SCC: port A reg %i read 0x%02x\n",scc_reg,scc_reg_val_b[scc_reg]);

	if (scc_reg == 2)
		return scc_status;

	return scc_reg_val_b[scc_reg];
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
	scc_reg_val_a[scc_reg] = data;
	logerror("SCC: port A reg %i write 0x%02x\n",scc_reg,data);
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
	scc_reg_val_b[scc_reg] = data;
	logerror("SCC: port B reg %i write 0x%02x\n",scc_reg,data);
}

unsigned char scc_get_reg_a(int reg)
{
	return scc_reg_val_a[reg];
}

unsigned char scc_get_reg_b(int reg)
{
	return scc_reg_val_b[reg];
}

void scc_set_reg_a(int reg, unsigned char data)
{
	scc_reg_val_a[reg] = data;
}

void scc_set_reg_b(int reg, unsigned char data)
{
	scc_reg_val_b[reg] = data;
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
				if((data & 0xf0) == 0)  // not a reset command
				{
					scc_mode = 1;
					scc_reg = data & 0x0f;
					logerror("SCC: Port B Reg select - %i\n",scc_reg);
//					scc_putbreg(data & 0xf0);
				}
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
				if((data & 0xf0) == 0)  // not a reset command
				{
					scc_mode = 1;
					scc_reg = data & 0x0f;
					logerror("SCC: Port A Reg select - %i\n",scc_reg);
//					scc_putareg(data & 0xf0);
				}
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

