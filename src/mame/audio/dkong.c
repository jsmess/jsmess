#include "driver.h"
#include "cpu/i8039/i8039.h"
#include "sound/samples.h"
#include "includes/dkong.h"

static UINT8 sh1_state[8];
static UINT8 sh1_count = 0;
static UINT8 walk = 0; /* used to determine if dkongjr is walking or climbing? */
static UINT8 death = 0;
static UINT8 drop = 0;
static UINT8 roar = 0;
static UINT8 land = 0;
static UINT8 climb = 0;
static UINT8 sh_climb_count;
static UINT8 snapjaw = 0;


SOUND_START( dkong )
{
	state_save_register_global_array(sh1_state);
	state_save_register_global(sh1_count);
	state_save_register_global(walk);
	state_save_register_global(death);
	state_save_register_global(drop);
	state_save_register_global(roar);
	state_save_register_global(land);
	state_save_register_global(climb);
	state_save_register_global(sh_climb_count);
	state_save_register_global(snapjaw);

	memset(sh1_state, 0, sizeof(sh1_state));
	sh1_count = 0;
	walk = 0;
	death = 0;
	drop = 0;
	roar = 0;
	land = 0;
	climb = 0;
	sh_climb_count = 0;
	snapjaw = 0;

	return 0;
}



WRITE8_HANDLER( dkong_sh_w )
{
	if (data)
		cpunum_set_input_line(1, 0, ASSERT_LINE);
	else
		cpunum_set_input_line(1, 0, CLEAR_LINE);
}

WRITE8_HANDLER( dkong_sh1_w )
{
	static const int sample_order[7] = {1,2,1,2,0,1,0};

	if (sh1_state[offset] != data)
	{
		if (data)
			{
				if (offset)
					sample_start (offset, offset + 2, 0);
				else
				{
					sample_start (offset, sample_order[sh1_count], 0);
					sh1_count++;
					if (sh1_count == 7)
						sh1_count = 0;
				}
			}
		sh1_state[offset] = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_death_w )
{
	if (death != data)
	{
		if (data)
			sample_stop (7);
		sample_start (6, 6, 0);
		death = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_drop_w )
{
	if (drop != data)
	{
		if (data)
			sample_start (7, 7, 0);
		drop = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_roar_w )
{
	if (roar != data)
	{
		if (data)
			sample_start (7,2,0);
		roar = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_jump_w )
{
	static int jump = 0;

	if (jump != data)
	{
		if (data)
			sample_start (6,0,0);
		jump = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_land_w )
{
	if (land != data)
	{
		if (data)
			sample_stop (7);
		sample_start (4,1,0);
		land = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_climb_w )
{
	static const int sample_order[7] = {1,2,1,2,0,1,0};

	if (climb != data)
	{
		if (data && walk == 0)
		{
			sample_start (3,sample_order[sh_climb_count]+3,0);
			sh_climb_count++;
			if (sh_climb_count == 7) sh_climb_count = 0;
		}
		else if (data && walk == 1)
		{
			sample_start (3,sample_order[sh_climb_count]+8,0);
			sh_climb_count++;
			if (sh_climb_count == 7) sh_climb_count = 0;
		}
		climb = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_snapjaw_w )
{
	if (snapjaw != data)
	{
		if (data)
			sample_stop (7);
		sample_start (4,11,0);
		snapjaw = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_walk_w )
{
	if (walk != data )
	{
		walk = data;
	}
}
