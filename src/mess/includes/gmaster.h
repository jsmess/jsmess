#ifndef __GMASTER_H__
#define __GMASTER_H__

typedef struct
{
	UINT8 data[8];
	int index;
	int x, y;
	/*bool*/int mode; // true read does not increase address
	/*bool*/int delayed;
	UINT8 pixels[8][64/*>=62 sure*/];
} GMASTER_VIDEO;

typedef struct
{
	UINT8 ports[5];
} GMASTER_MACHINE;


class gmaster_state : public driver_device
{
public:
	gmaster_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	GMASTER_VIDEO m_video;
	GMASTER_MACHINE m_machine;
};


/*----------- defined in audio/gmaster.c -----------*/

DECLARE_LEGACY_SOUND_DEVICE(GMASTER, gmaster_sound);

int gmaster_io_callback(device_t *device, int ioline, int state);

#endif /* __GMASTER_H__ */
