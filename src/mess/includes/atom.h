#pragma once

#ifndef __ATOM__
#define __ATOM__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/cartslot.h"
#include "imagedev/cassette.h"
#include "imagedev/flopdrv.h"
#include "machine/ram.h"
#include "imagedev/snapquik.h"
#include "formats/atom_atm.h"
#include "formats/atom_tap.h"
#include "formats/basicdsk.h"
#include "formats/uef_cas.h"
#include "machine/ctronics.h"
#include "machine/6522via.h"
#include "machine/i8255.h"
#include "machine/i8271.h"
#include "sound/speaker.h"
#include "video/m6847.h"

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
		: driver_device(machine, config),
		  m_maincpu(*this, SY6502_TAG),
		  m_vdg(*this, MC6847_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_centronics(*this, CENTRONICS_TAG),
		  m_speaker(*this, SPEAKER_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_vdg;
	required_device<device_t> m_cassette;
	required_device<device_t> m_centronics;
	required_device<device_t> m_speaker;

	virtual void machine_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	void bankswitch();
	
	READ8_MEMBER( eprom_r );
	WRITE8_MEMBER( eprom_w );
	WRITE8_MEMBER( ppi_pa_w );
	READ8_MEMBER( ppi_pb_r );
	READ8_MEMBER( ppi_pc_r );
	WRITE8_MEMBER( ppi_pc_w );
	READ8_MEMBER( printer_busy );
	WRITE8_MEMBER( printer_data );
	READ8_MEMBER( vdg_videoram_r );

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
	int m_previous_i8271_int_state;
};

class atomeb_state : public atom_state
{
public:
	atomeb_state(running_machine &machine, const driver_device_config_base &config)
		: atom_state(machine, config)
	{ }

	virtual void machine_start();
};

#endif
