#pragma once

#ifndef __VIDBRAIN__
#define __VIDBRAIN__

#define F3850_TAG			"cd34"
#define F3853_TAG			"cd5"
#define SCREEN_TAG			"screen"
#define DISCRETE_TAG		"discrete"
#define TIMER_Y_ODD_TAG		"odd"
#define TIMER_Y_EVEN_TAG	"even"
#define CASSETTE_TAG		"cassette"

class vidbrain_state : public driver_device
{
public:
	vidbrain_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, F3850_TAG),
		  m_discrete(*this, DISCRETE_TAG),
		  m_screen(*this, SCREEN_TAG),
		  m_timer_y_odd(*this, TIMER_Y_ODD_TAG),
		  m_timer_y_even(*this, TIMER_Y_EVEN_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_discrete;
	required_device<screen_device> m_screen;
	required_device<timer_device> m_timer_y_odd;
	required_device<timer_device> m_timer_y_even;

	virtual void machine_start();

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_WRITE8_MEMBER( keyboard_w );
	DECLARE_READ8_MEMBER( keyboard_r );
	DECLARE_WRITE8_MEMBER( sound_w );
	DECLARE_WRITE8_MEMBER( f3853_w );
	DECLARE_READ8_MEMBER( vlsi_r );
	DECLARE_WRITE8_MEMBER( vlsi_w );

	void interrupt_check();
	int get_field_vpos();
	int get_field();
	void set_y_interrupt();
	void do_partial_update();

	// F3853 SMI state
	UINT16 m_vector;
	int m_int_enable;
	int m_ext_int_latch;
	int m_timer_int_latch;

	// keyboard state
	UINT8 m_keylatch;
	int m_joy_enable;

	// video state
	UINT8 m_vlsi_ram[0x90];
	UINT8 m_y_int;
	UINT8 m_fmod;
	UINT8 m_bg;
	UINT8 m_cmd;
	UINT8 m_freeze_x;
	UINT16 m_freeze_y;
	int m_field;

	// sound state
	int m_sound_clk;
};

//----------- defined in video/vidbrain.c -----------

MACHINE_CONFIG_EXTERN( vidbrain_video );

#endif
