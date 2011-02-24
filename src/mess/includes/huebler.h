#ifndef __HUEBLER__
#define __HUEBLER__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define Z80CTC_TAG		"z80ctc"
#define Z80SIO_TAG		"z80sio"
#define Z80PIO1_TAG		"z80pio1"
#define Z80PIO2_TAG		"z80pio2"
#define CASSETTE_TAG	"cassette"

class amu880_state : public driver_device
{
public:
	amu880_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_cassette(*this, CASSETTE_TAG),
		  m_key_d6(0),
		  m_key_d7(0),
		  m_key_a8(1)
	{ }

	required_device<device_t> m_cassette;

	virtual void machine_start();

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_READ8_MEMBER( keyboard_r );

	void scan_keyboard();

	// keyboard state
	const UINT8 *m_kb_rom;
	int m_key_d6;
	int m_key_d7;
	int m_key_a4;
	int m_key_a5;
	int m_key_a8;

	// video state
	UINT8 *m_video_ram;
	const UINT8 *m_char_rom;
};

#endif
