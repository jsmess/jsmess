#ifndef __LC80__
#define __LC80__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "imagedev/cassette.h"
#include "machine/ram.h"
#include "machine/z80pio.h"
#include "machine/z80ctc.h"
#include "sound/speaker.h"

#define SCREEN_TAG		"screen"
#define Z80_TAG			"d201"
#define Z80CTC_TAG		"d208"
#define Z80PIO1_TAG		"d206"
#define Z80PIO2_TAG		"d207"
#define CASSETTE_TAG	"cassette"
#define SPEAKER_TAG		"b237"

class lc80_state : public driver_device
{
public:
	lc80_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, Z80_TAG),
		  m_pio2(*this, Z80PIO2_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_speaker(*this, SPEAKER_TAG),
		  m_ram(*this, RAM_TAG)
	{ }
	
	required_device<cpu_device> m_maincpu;
	required_device<z80pio_device> m_pio2;
	required_device<device_t> m_cassette;
	required_device<device_t> m_speaker;
	required_device<device_t> m_ram;
	
	virtual void machine_start();

	DECLARE_WRITE_LINE_MEMBER( ctc_z0_w );
	DECLARE_WRITE_LINE_MEMBER( ctc_z1_w );
	DECLARE_WRITE_LINE_MEMBER( ctc_z2_w );
	DECLARE_WRITE8_MEMBER( pio1_pa_w );
	DECLARE_READ8_MEMBER( pio1_pb_r );
	DECLARE_WRITE8_MEMBER( pio1_pb_w );
	DECLARE_READ8_MEMBER( pio2_pb_r );

	void update_display();
	
	// display state
	UINT8 m_digit;
	UINT8 m_segment;
};

#endif
