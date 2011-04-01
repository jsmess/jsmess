#pragma once

#ifndef __ATOM__
#define __ATOM__

#include "emu.h"

#define SY6502_TAG		"ic22"
#define INS8255_TAG		"ic25"
#define MC6847_TAG		"ic31"
#define R6522_TAG		"ic1"
#define I8271_TAG		"ic13"
#define MC6854_TAG		"econet_ic1"
#define SCREEN_TAG		"screen"
#define CENTRONICS_TAG	"centronics"
#define CASSETTE_TAG	"cassette"
#define SPEAKER_TAG		"speaker"

#define X1	XTAL_3_579545MHz	// MC6847 Clock
#define X2	XTAL_4MHz			// CPU Clock - a divider reduces it to 1MHz

class atom_state : public driver_device
{
public:
	atom_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* eprom state */
	int m_eprom;

	/* video state */
	UINT8 *m_video_ram;

	/* keyboard state */
	int m_keylatch;

	/* cassette state */
	int m_hz2400;
	int m_pc0;
	int m_pc1;

	/* devices */
	device_t *m_mc6847;
	device_t *m_cassette;
	int m_previous_i8271_int_state;
};

#endif
