#ifndef __EXP85__
#define __EXP85__

#define SCREEN_TAG		"screen"
#define I8085A_TAG		"u100"
#define I8155_TAG		"u106"
#define I8355_TAG		"u105"
#define CASSETTE_TAG	"cassette"
#define SPEAKER_TAG		"speaker"

class exp85_state : public driver_device
{
public:
	exp85_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, I8085A_TAG),
		  m_terminal(*this, TERMINAL_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_speaker(*this, SPEAKER_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_terminal;
	required_device<device_t> m_cassette;
	required_device<device_t> m_speaker;

	virtual void machine_start();

	DECLARE_READ8_MEMBER( i8355_a_r );
	DECLARE_WRITE8_MEMBER( i8355_a_w );
	DECLARE_READ_LINE_MEMBER( sid_r );
	DECLARE_WRITE_LINE_MEMBER( sod_w );

	/* cassette state */
	int m_tape_control;
};

#endif
