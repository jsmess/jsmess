#pragma once

#ifndef __VIDBRAIN__
#define __VIDBRAIN__

#define F3850_TAG		"cd34"
#define F3853_TAG		"cd5"
#define SCREEN_TAG		"screen"
#define DISCRETE_TAG	"discrete"
#define TIMER_Y_INT_TAG	"y_int"
#define TIMER_FIELD_TAG	"field"
#define CASSETTE_TAG	"cassette"

class vidbrain_state : public driver_device
{
public:
	vidbrain_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* F3853 SMI state */
	UINT16 vector;
	int int_enable;
	int ext_int_latch;
	int timer_int_latch;

	/* keyboard state */
	UINT8 keylatch;
	int joy_enable;

	/* video state */
	UINT8 vlsi_ram[0x90];
	UINT8 y_int;
	UINT8 fmod;
	UINT8 bg;
	UINT8 cmd;
	UINT8 freeze_x;
	UINT16 freeze_y;
	int field;

	/* sound state */
	int sound_clk;

	/* devices */
	running_device *discrete;
	screen_device *screen;
	timer_device *timer_y_int;
};

/*----------- defined in driver/vidbrain.c -----------*/

void vidbrain_interrupt_check(running_machine *machine);

/*----------- defined in video/vidbrain.c -----------*/

READ8_HANDLER( vidbrain_vlsi_r );
WRITE8_HANDLER( vidbrain_vlsi_w );

MACHINE_CONFIG_EXTERN(vidbrain_video);

#endif
