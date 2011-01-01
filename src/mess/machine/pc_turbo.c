/*********************************************************************

    pc_turbo.c

    The PC "turbo" button

**********************************************************************/

#include "emu.h"
#include "pc_turbo.h"


struct pc_turbo_info
{
	device_t *cpu;
	const char *port;
	int mask;
	int cur_val;
	double off_speed;
	double on_speed;
};



static TIMER_CALLBACK(pc_turbo_callback)
{
	struct pc_turbo_info *ti = (struct pc_turbo_info *) ptr;
	int val;

	val = input_port_read(machine, ti->port) & ti->mask;

	if (val != ti->cur_val)
	{
		ti->cur_val = val;
		ti->cpu->set_clock_scale(val ? ti->on_speed : ti->off_speed);
	}
}



int pc_turbo_setup(running_machine *machine, device_t *cpu, const char *port, int mask, double off_speed, double on_speed)
{
	struct pc_turbo_info *ti;

	ti = auto_alloc(machine, struct pc_turbo_info);
	ti->cpu = cpu;
	ti->port = port;
	ti->mask = mask;
	ti->cur_val = -1;
	ti->off_speed = off_speed;
	ti->on_speed = on_speed;
	timer_pulse(machine, ATTOTIME_IN_MSEC(100), ti, 0, pc_turbo_callback);
	return 0;
}
