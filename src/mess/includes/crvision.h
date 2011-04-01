#ifndef __CRVISION__
#define __CRVISION__

#define SCREEN_TAG		"screen"
#define M6502_TAG		"u2"
#define TMS9929_TAG		"u3"
#define PIA6821_TAG		"u21"
#define SN76489_TAG		"u22"
#define CASSETTE_TAG	"cassette"
#define CENTRONICS_TAG	"centronics"

#define BANK_ROM1		"bank1"
#define BANK_ROM2		"bank2"

class crvision_state : public driver_device
{
public:
	crvision_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* keyboard state */
	UINT8 m_keylatch;

	/* joystick state */
	UINT8 m_joylatch;

	/* devices */
	device_t *m_psg;
	device_t *m_cassette;
	device_t *m_centronics;
};

#endif
