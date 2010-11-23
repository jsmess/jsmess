/*****************************************************************************
 *
 * includes/avigo.h
 *
 ****************************************************************************/

#ifndef AVIGO_H_
#define AVIGO_H_

#include "machine/intelfsh.h"

#define AVIGO_NUM_COLOURS 2

#define AVIGO_SCREEN_WIDTH        160
#define AVIGO_SCREEN_HEIGHT       240


class avigo_state : public driver_device
{
public:
	avigo_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 key_line;
	UINT8 irq;
	UINT8 speaker_data;
	unsigned long ram_bank_l;
	unsigned long ram_bank_h;
	unsigned long rom_bank_l;
	unsigned long rom_bank_h;
	unsigned long ad_control_status;
	intelfsh8_device *flashes[3];
	int flash_at_0x4000;
	int flash_at_0x8000;
	void *banked_opbase[4];
	int previous_input_port_data[4];
	int stylus_marker_x;
	int stylus_marker_y;
	int stylus_press_x;
	int stylus_press_y;
	int ox;
	int oy;
	unsigned int ad_value;
	UINT8 *video_memory;
	UINT8 screen_column;
	int stylus_x;
	int stylus_y;
	gfx_element *stylus_pointer;
};


/*----------- defined in video/avigo.c -----------*/

extern  READ8_HANDLER(avigo_vid_memory_r);
extern WRITE8_HANDLER(avigo_vid_memory_w);

extern VIDEO_START( avigo );
extern VIDEO_UPDATE( avigo );
extern PALETTE_INIT( avigo );

void avigo_vh_set_stylus_marker_position(running_machine *machine, int x, int y);


#endif /* AVIGO_H_ */
