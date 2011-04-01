#ifndef __VCS80__
#define __VCS80__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define Z80PIO_TAG		"z80pio"

class vcs80_state : public driver_device
{
public:
	vcs80_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* keyboard state */
	int m_keylatch;
	int m_keyclk;

	/* devices */
	device_t *m_z80pio;
};

#endif
