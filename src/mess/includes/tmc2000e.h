#pragma once

#ifndef __TMC2000E__
#define __TMC2000E__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/cosmac/cosmac.h"
#include "formats/basicdsk.h"
#include "imagedev/cassette.h"
#include "imagedev/flopdrv.h"
#include "imagedev/printer.h"
#include "machine/rescap.h"
#include "machine/ram.h"
#include "sound/cdp1864.h"

#define SCREEN_TAG		"screen"
#define CDP1802_TAG		"cdp1802"
#define CDP1864_TAG		"cdp1864"
#define CASSETTE_TAG	"cassette"

#define TMC2000E_COLORRAM_SIZE 0x100 // ???

class tmc2000e_state : public driver_device
{
public:
	tmc2000e_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, CDP1802_TAG),
		  m_cti(*this, CDP1864_TAG),
		  m_cassette(*this, CASSETTE_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_cti;
	required_device<device_t> m_cassette;

	virtual void machine_start();
	virtual void machine_reset();

	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
	
	DECLARE_READ8_MEMBER( vismac_r );
	DECLARE_WRITE8_MEMBER( vismac_w );
	DECLARE_READ8_MEMBER( floppy_r );
	DECLARE_WRITE8_MEMBER( floppy_w );
	DECLARE_READ8_MEMBER( ascii_keyboard_r );
	DECLARE_READ8_MEMBER( io_r );
	DECLARE_WRITE8_MEMBER( io_w );
	DECLARE_WRITE8_MEMBER( io_select_w );
	DECLARE_WRITE8_MEMBER( keyboard_latch_w );
	DECLARE_READ_LINE_MEMBER( rdata_r );
	DECLARE_READ_LINE_MEMBER( bdata_r );
	DECLARE_READ_LINE_MEMBER( gdata_r );
	DECLARE_READ_LINE_MEMBER( clear_r );
	DECLARE_READ_LINE_MEMBER( ef2_r );
	DECLARE_READ_LINE_MEMBER( ef3_r );
	DECLARE_WRITE_LINE_MEMBER( q_w );
	DECLARE_WRITE8_MEMBER( dma_w );

	/* video state */
	int m_cdp1864_efx;		/* EFx */
	UINT8 *m_colorram;		/* color memory */
	UINT8 m_color;

	/* keyboard state */
	int m_keylatch;			/* key latch */
	int m_reset;			/* reset activated */
};

#endif
