#pragma once

#ifndef __VIDBRAIN__
#define __VIDBRAIN__

#define F3850_TAG		"cd34"
#define F3853_TAG		"cd5"
#define SCREEN_TAG		"screen"
#define DISCRETE_TAG	"discrete"
#define CASSETTE_TAG	"cassette"

class vidbrain_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, vidbrain_state(machine)); }

	vidbrain_state(running_machine &machine) { }

	/* keyboard state */
	UINT8 keylatch;
	int joy_enable;
	int uv201_31;

	/* video state */
	UINT8 *video_ram;
	UINT8 bg_color;
	UINT8 vlsi[0x100];

	/* sound state */
	int sound_clk;
	int sound_q[2];

	/* devices */
	running_device *discrete;
};

#endif
