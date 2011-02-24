#pragma once

#ifndef __PORTFOLIO__
#define __PORTFOLIO__

#include "video/hd61830.h"

#define SCREEN_TAG		"screen"
#define M80C88A_TAG		"u1"
#define M82C55A_TAG		"hpc101_u1"
#define M82C50A_TAG		"hpc102_u1"
#define HD61830_TAG		"hd61830"
#define CENTRONICS_TAG	"centronics"
#define SPEAKER_TAG		"speaker"
#define TIMER_TICK_TAG	"tick"

class portfolio_state : public driver_device
{
public:
	portfolio_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, M80C88A_TAG),
		  m_lcdc(*this, HD61830_TAG),
		  m_ppi(*this, M82C55A_TAG),
		  m_uart(*this, M82C50A_TAG),
		  m_speaker(*this, SPEAKER_TAG),
		  m_timer_tick(*this, TIMER_TICK_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<hd61830_device> m_lcdc;
	required_device<device_t> m_ppi;
	required_device<device_t> m_uart;
	required_device<device_t> m_speaker;
	required_device<timer_device> m_timer_tick;

	virtual void machine_start();
	virtual void machine_reset();

	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect) { m_lcdc->update_screen(&bitmap, &cliprect); return 0; }

	void check_interrupt();
	void trigger_interrupt(int level);
	void scan_keyboard();

	DECLARE_READ8_MEMBER( irq_status_r );
	DECLARE_READ8_MEMBER( keyboard_r );
	DECLARE_READ8_MEMBER( battery_r );
	DECLARE_READ8_MEMBER( counter_r );
	DECLARE_READ8_MEMBER( pid_r );

	DECLARE_WRITE8_MEMBER( irq_mask_w );
	DECLARE_WRITE8_MEMBER( sivr_w );
	DECLARE_WRITE8_MEMBER( speaker_w );
	DECLARE_WRITE8_MEMBER( power_w );
	DECLARE_WRITE8_MEMBER( unknown_w );
	DECLARE_WRITE8_MEMBER( counter_w );
	DECLARE_WRITE8_MEMBER( ncc1_w );

	/* interrupt state */
	UINT8 m_ip;							/* interrupt pending */
	UINT8 m_ie;							/* interrupt enable */
	UINT8 m_sivr;						/* serial interrupt vector register */

	/* counter state */
	UINT16 m_counter;

	/* keyboard state */
	UINT8 m_keylatch;

	/* video state */
	UINT8 m_contrast;

	/* peripheral state */
	UINT8 m_pid;						/* peripheral identification */
};

#endif
