/*****************************************************************************
 *
 * includes/ace.h
 *
 ****************************************************************************/

#ifndef ACE_H_
#define ACE_H_

#include "machine/ram.h"
#define Z80_TAG			"z0"
#define AY8910_TAG		"ay8910"
#define SPEAKER_TAG		"speaker"
#define CASSETTE_TAG	"cassette"
#define SCREEN_TAG		"screen"

class ace_state : public driver_device
{
public:
	ace_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, Z80_TAG),
		  m_speaker(*this, SPEAKER_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_ram(*this, RAM_TAG)
	 { }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_speaker;
	required_device<device_t> m_cassette;
	required_device<device_t> m_ram;

	virtual void machine_start();

	bool video_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_READ8_MEMBER( io_r );
	DECLARE_WRITE8_MEMBER( io_w );

	UINT8 *m_video_ram;
	UINT8 *m_char_ram;
};


/*----------- defined in machine/ace.c -----------*/


SNAPSHOT_LOAD(ace);


/*----------- defined in video/ace.c -----------*/

#endif /* ACE_H_ */
