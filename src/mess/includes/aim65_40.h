#pragma once

#ifndef __AIM65_40__
#define __AIM65_40__

#define M6502_TAG		"m6502"
#define M6522_0_TAG		"m6522_0"
#define M6522_1_TAG		"m6522_1"
#define M6522_2_TAG		"m6522_2"
#define M6551_TAG		"m6551"
#define SPEAKER_TAG		"speaker"

class aim65_40_state : public driver_device
{
public:
	aim65_40_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* devices */
	device_t *m_via0;
	device_t *m_via1;
	device_t *m_via2;
	device_t *m_speaker;
};

#endif
