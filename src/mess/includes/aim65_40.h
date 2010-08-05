#pragma once

#ifndef __AIM65_40__
#define __AIM65_40__

#define M6502_TAG		"m6502"
#define M6522_0_TAG		"m6522_0"
#define M6522_1_TAG		"m6522_1"
#define M6522_2_TAG		"m6522_2"
#define M6551_TAG		"m6551"
#define SPEAKER_TAG		"speaker"

class aim65_40_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, aim65_40_state(machine)); }

	aim65_40_state(running_machine &machine)
		: driver_data_t(machine) { }

	/* devices */
	running_device *via0;
	running_device *via1;
	running_device *via2;
	running_device *speaker;
};

#endif
