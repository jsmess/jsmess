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
	avigo_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	UINT8 m_key_line;
	UINT8 m_irq;
	UINT8 m_speaker_data;
	unsigned long m_ram_bank_l;
	unsigned long m_ram_bank_h;
	unsigned long m_rom_bank_l;
	unsigned long m_rom_bank_h;
	unsigned long m_ad_control_status;
	intelfsh8_device *m_flashes[3];
	int m_flash_at_0x4000;
	int m_flash_at_0x8000;
	void *m_banked_opbase[4];
	int m_previous_input_port_data[4];
	int m_ox;
	int m_oy;
	unsigned int m_ad_value;
	UINT8 *m_video_memory;
	UINT8 m_screen_column;
};


/*----------- defined in video/avigo.c -----------*/

extern  READ8_HANDLER(avigo_vid_memory_r);
extern WRITE8_HANDLER(avigo_vid_memory_w);

extern VIDEO_START( avigo );
extern SCREEN_UPDATE( avigo );
extern PALETTE_INIT( avigo );


#endif /* AVIGO_H_ */
