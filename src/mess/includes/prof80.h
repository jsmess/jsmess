#pragma once

#ifndef __PROF80__
#define __PROF80__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"
#include "machine/ctronics.h"
#include "machine/i8255a.h"
#include "machine/ram.h"
#include "machine/rescap.h"
#include "machine/terminal.h"
#include "machine/upd1990a.h"
#include "machine/upd765.h"
#include "machine/z80sti.h"
#include "sound/speaker.h"
#include "video/mc6845.h"

#define Z80_TAG					"z1"
#define UPD765_TAG				"z38"
#define UPD1990A_TAG			"z43"

/* ------------------------------------------------------------------------ */

#define SCREEN_TAG				"screen"
#define GRIP_Z80_TAG			"grip_z1"
#define MC6845_TAG				"z30"
#define I8255A_TAG				"z6"
#define Z80STI_TAG				"z9"
#define SPEAKER_TAG				"speaker"
#define CENTRONICS_TAG			"centronics"

#define GRIP_VIDEORAM_SIZE	0x10000
#define GRIP_VIDEORAM_MASK	0xffff

/* ------------------------------------------------------------------------ */

#define UNIO_Z80STI_TAG			"z5"
#define UNIO_Z80SIO_TAG			"z15"
#define UNIO_Z80PIO_TAG			"z13"
#define UNIO_CENTRONICS1_TAG	"n3"
#define UNIO_CENTRONICS2_TAG	"n4"

class prof80_state : public driver_device
{
public:
	prof80_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, Z80_TAG),
		  m_ppi(*this, I8255A_TAG),
		  m_rtc(*this, UPD1990A_TAG),
		  m_fdc(*this, UPD765_TAG),
		  m_ram(*this, RAM_TAG),
		  m_floppy0(*this, FLOPPY_0),
		  m_floppy1(*this, FLOPPY_1),
		  m_terminal(*this, TERMINAL_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	optional_device<device_t> m_ppi;
	required_device<upd1990a_device> m_rtc;
	required_device<device_t> m_fdc;
	required_device<device_t> m_ram;
	required_device<device_t> m_floppy0;
	required_device<device_t> m_floppy1;
	optional_device<device_t> m_terminal;

	virtual void machine_start();
	virtual void machine_reset();

	WRITE8_MEMBER( flr_w );
	READ8_MEMBER( status_r );
	READ8_MEMBER( status2_r );
	WRITE8_MEMBER( par_w );
	READ8_MEMBER( gripc_r );
	READ8_MEMBER( gripd_r );
	WRITE8_MEMBER( gripd_w );
	WRITE8_MEMBER( unio_ctrl_w );

	void bankswitch();
	void ls259_w(int fa, int sa, int fb, int sb);
	void floppy_motor_off();

	/* memory state */
	UINT8 m_mmu[16];			/* MMU block register */
	int m_init;				/* MMU enable */

	/* RTC state */
	int m_c0;
	int m_c1;
	int m_c2;

	/* floppy state */
	int	m_fdc_index;			/* floppy index hole sensor */
	int m_motor;				/* floppy motor */

	/* GRIP state */
	UINT8 m_gripd;			/* GRIP data */
	UINT8 m_gripc;			/* GRIP status */

	/* timers */
	emu_timer	*m_floppy_motor_off_timer;

};

class grip_state : public prof80_state
{
public:
	grip_state(running_machine &machine, const driver_device_config_base &config)
		: prof80_state(machine, config),
		  m_crtc(*this, MC6845_TAG),
		  m_sti(*this, Z80STI_TAG),
		  m_centronics(*this, CENTRONICS_TAG),
		  m_speaker(*this, SPEAKER_TAG)
	{ }

	required_device<device_t> m_crtc;
	required_device<device_t> m_sti;
	required_device<device_t> m_centronics;
	required_device<device_t> m_speaker;

	virtual void machine_start();

	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	WRITE8_MEMBER( vol0_w );
	WRITE8_MEMBER( vol1_w );
	WRITE8_MEMBER( flash_w );
	WRITE8_MEMBER( page_w );
	READ8_MEMBER( stat_r );
	READ8_MEMBER( lrs_r );
	WRITE8_MEMBER( lrs_w );
	READ8_MEMBER( cxstb_r );
	WRITE8_MEMBER( cxstb_w );
	READ8_MEMBER( ppi_pa_r );
	WRITE8_MEMBER( ppi_pa_w );
	READ8_MEMBER( ppi_pb_r );
	WRITE8_MEMBER( ppi_pc_w );
	READ8_MEMBER( sti_gpio_r );
	WRITE_LINE_MEMBER( speaker_w );

	void scan_keyboard();

	/* sound state */
	int m_vol0;
	int m_vol1;

	/* keyboard state */
	UINT8 m_keydata;			/* keyboard data */
	int m_kbf;				/* keyboard buffer full */

	/* video state */
	UINT8 *m_video_ram;		/* video RAM */
	int m_lps;				/* light pen sense */
	int m_page;				/* video page */
	int m_flash;				/* flash */
};

#endif
