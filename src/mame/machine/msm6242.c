/***************************************************************************

    MSM6242 Real Time Clock

***************************************************************************/

#include "machine/msm6242.h"

READ8_HANDLER( msm6242_r )
{
	mame_system_time systime;

	mame_get_current_datetime(Machine, &systime);

	switch(offset)
	{
		case 0x0:	return systime.local_time.second % 10;
		case 0x1:	return systime.local_time.second / 10;
		case 0x2:	return systime.local_time.minute % 10;
		case 0x3:	return systime.local_time.minute / 10;
		case 0x4:	return systime.local_time.hour % 10;
		case 0x5:	return systime.local_time.hour / 10;
		case 0x6:	return systime.local_time.mday % 10;
		case 0x7:	return systime.local_time.mday / 10;
		case 0x8:	return (systime.local_time.month+1) % 10;
		case 0x9:	return (systime.local_time.month+1) / 10;
		case 0xa:	return systime.local_time.year % 10;
		case 0xb:	return (systime.local_time.year % 100) / 10;
		case 0xc:	return systime.local_time.weekday;
		case 0xd:	return 1;
		case 0xe:	return 6;
		case 0xf:	return 4;
	}

	logerror("%04x: MSM6242 unmapped offset %02X read\n",activecpu_get_pc(),offset);
	return 0;
}

WRITE8_HANDLER( msm6242_w )
{
	switch(offset)
	{
		case 0x0:	return;
		case 0x1:	return;
		case 0x2:	return;
		case 0x3:	return;
		case 0x4:	return;
		case 0x5:	return;
		case 0x6:	return;
		case 0x7:	return;
		case 0x8:	return;
		case 0x9:	return;
		case 0xa:	return;
		case 0xb:	return;
		case 0xc:	return;
		case 0xd:	return;
		case 0xe:	return;
		case 0xf:	return;
	}
	logerror("%04x: MSM6242 unmapped offset %02X written with %02X\n",activecpu_get_pc(),offset,data);
}


READ16_HANDLER( msm6242_lsb_r )
{
	return msm6242_r(offset);
}

WRITE16_HANDLER( msm6242_lsb_w )
{
	if (ACCESSING_LSB)
		msm6242_w(offset,data);
}
