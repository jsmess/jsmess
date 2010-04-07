#pragma once

#ifndef __VIDBRAIN__
#define __VIDBRAIN__

#define F3850_TAG		"cd34"
#define SCREEN_TAG		"screen"
#define SPEAKER_TAG		"speaker"

class vidbrain_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, vidbrain_state(machine)); }

	vidbrain_state(running_machine &machine) { }

	/* devices */
	running_device *speaker;
};

#endif
