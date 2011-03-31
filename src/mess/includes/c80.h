#ifndef __C80__
#define __C80__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/z80pio.h"
#include "imagedev/cassette.h"
#include "machine/ram.h"

#define SCREEN_TAG		"screen"
#define Z80_TAG			"d2"
#define Z80PIO1_TAG		"d11"
#define Z80PIO2_TAG		"d12"
#define CASSETTE_TAG	"cassette"

class c80_state : public driver_device
{
public:
	c80_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, Z80_TAG),
		  m_pio1(*this, Z80PIO1_TAG),
		  m_cassette(*this, CASSETTE_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_pio1;
	required_device<device_t> m_cassette;

	virtual void machine_start();

	DECLARE_READ8_MEMBER( pio1_pa_r );
	DECLARE_WRITE8_MEMBER( pio1_pa_w );
	DECLARE_WRITE8_MEMBER( pio1_pb_w );
	DECLARE_WRITE_LINE_MEMBER( pio1_brdy_w );
	
	/* keyboard state */
	int m_keylatch;

	/* display state */
	int m_digit;
	int m_pio1_a5;
	int m_pio1_brdy;
};

#endif
