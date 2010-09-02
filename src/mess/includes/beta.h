#ifndef __BETA__
#define __BETA__

#define SCREEN_TAG		"screen"
#define M6502_TAG		"m6502"
#define M6532_TAG		"m6532"
#define EPROM_TAG		"eprom"
#define SPEAKER_TAG		"b237"

class beta_state : public driver_device
{
public:
	beta_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* EPROM state */
	int eprom_oe;
	int eprom_ce;
	UINT16 eprom_addr;
	UINT8 eprom_data;
	UINT8 old_data;

	/* display state */
	UINT8 ls145_p;
	UINT8 segment;

	emu_timer *led_refresh_timer;

	/* devices */
	running_device *speaker;
};

#endif
