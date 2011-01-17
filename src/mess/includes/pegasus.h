#include "cpu/m6809/m6809.h"
#include "machine/6821pia.h"
#include "imagedev/cartslot.h"
#include "imagedev/cassette.h"

class pegasus_state : public driver_device
{
public:
	pegasus_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
	m_maincpu(*this, "maincpu"),
	m_cass(*this, "cassette"),
	m_pia_s(*this, "pia_s"),
	m_pia_u(*this, "pia_u")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_cass;
	required_device<device_t> m_pia_s;
	required_device<device_t> m_pia_u;
	DECLARE_READ8_MEMBER( pegasus_keyboard_r );
	DECLARE_READ8_MEMBER( pegasus_protection_r );
	DECLARE_READ8_MEMBER( pegasus_pcg_r );
	DECLARE_WRITE8_MEMBER( pegasus_controls_w );
	DECLARE_WRITE8_MEMBER( pegasus_keyboard_w );
	DECLARE_WRITE8_MEMBER( pegasus_pcg_w );
	DECLARE_READ_LINE_MEMBER( pegasus_keyboard_irq );
	DECLARE_READ_LINE_MEMBER( pegasus_cassette_r );
	DECLARE_WRITE_LINE_MEMBER( pegasus_cassette_w );
	DECLARE_WRITE_LINE_MEMBER( pegasus_firq_clr );
	UINT8 m_kbd_row;
	UINT8 m_kbd_irq;
	UINT8 *m_pcgram;
	UINT8 *m_videoram;
	UINT8 m_control_bits;
};


/*----------- defined in video/pegasus.c -----------*/

VIDEO_UPDATE( pegasus );
