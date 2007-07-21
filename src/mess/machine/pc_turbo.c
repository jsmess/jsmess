/*********************************************************************

    pc_turbo.c

	The PC "turbo" button

**********************************************************************/

#include "driver.h"
#include "pc_turbo.h"


struct pc_turbo_info
{
	int cpunum;
	int port;
	int mask;
	int cur_val;
	double off_speed;
	double on_speed;
};



static TIMER_CALLBACK_PTR(pc_turbo_callback)
{
	struct pc_turbo_info *ti = (struct pc_turbo_info *) param;
	int val;
	
	val = readinputport(ti->port) & ti->mask;

	if (val != ti->cur_val)
	{
		ti->cur_val = val;
		cpunum_set_clockscale(ti->cpunum, val ? ti->on_speed : ti->off_speed);
	}
}



int pc_turbo_setup(int cpunum, int port, int mask, double off_speed, double on_speed)
{
	struct pc_turbo_info *ti;

	ti = auto_malloc(sizeof(struct pc_turbo_info));
	ti->cpunum = cpunum;
	ti->port = port;
	ti->mask = mask;
	ti->cur_val = -1;
	ti->off_speed = off_speed;
	ti->on_speed = on_speed;
	mame_timer_pulse_ptr(MAME_TIME_IN_MSEC(100), ti, pc_turbo_callback);
	return 0;
}
