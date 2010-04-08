#pragma once

#ifndef __VIDBRAIN__
#define __VIDBRAIN__

#define F3850_TAG		"cd34"
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

	/* devices */
	running_device *discrete;
};

#endif
