/*****************************************************************************
 *
 * includes/lynx.h
 *
 ****************************************************************************/

#ifndef LYNX_H_
#define LYNX_H_

#include "imagedev/cartslot.h"


#define LYNX_CART		0
#define LYNX_QUICKLOAD	1


class lynx_state;
typedef struct
{
	UINT8 *mem;
	// global
	UINT16 screen;
	UINT16 colbuf;
	UINT16 colpos; // byte where value of collision is written
	UINT16 xoff, yoff;
	// in command
	int mode;
	UINT16 cmd;
	UINT8 spritenr;
	int x,y;
	UINT16 width, height; // uint16 important for blue lightning
	UINT16 stretch, tilt; // uint16 important
	UINT8 color[16]; // or stored
	void (*line_function)(lynx_state *state, const int y, const int xdir);
	UINT16 bitmap;

	int everon;
	int memory_accesses;
	attotime time;
} BLITTER;

typedef struct
{
	UINT8 serctl;
	UINT8 data_received, data_to_send, buffer;

	int received;
	int sending;
	int buffer_loaded;
} UART;

typedef struct
{
	UINT8 data[0x100];
	int accumulate_overflow;
	UINT8 high;
	int low;
} SUZY;

typedef struct
{
	UINT8 data[0x100];
} MIKEY;

typedef struct
{
	UINT8	bakup;
	UINT8	cntrl1;
	UINT8	cntrl2;
	int		counter;
	emu_timer	*timer;
	int		timer_active;
} LYNX_TIMER;

#define NR_LYNX_TIMERS	8

class lynx_state : public driver_device
{
public:
	lynx_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_mem_0000;
	UINT8 *m_mem_fc00;
	UINT8 *m_mem_fd00;
	UINT8 *m_mem_fe00;
	UINT8 *m_mem_fffa;
	size_t m_mem_fe00_size;
	UINT16 m_granularity;
	int m_line;
	int m_line_y;
	int m_sign_AB;
	int m_sign_CD;
	UINT32 m_palette[0x10];
	int m_rotate0;
	int m_rotate;
	device_t *m_audio;
	SUZY m_suzy;
	BLITTER m_blitter;
	UINT8 m_memory_config;
	UINT8 m_sprite_collide;
	MIKEY m_mikey;
	int m_height;
	int m_width;
	LYNX_TIMER m_timer[NR_LYNX_TIMERS];
	UART m_uart;
};


/*----------- defined in machine/lynx.c -----------*/

MACHINE_START( lynx );


READ8_HANDLER( lynx_memory_config_r );
WRITE8_HANDLER( lynx_memory_config_w );
void lynx_timer_count_down(running_machine &machine, int nr);

INTERRUPT_GEN( lynx_frame_int );

/* These functions are also needed for the Quickload */
int lynx_verify_cart (char *header, int kind);
void lynx_crc_keyword(device_image_interface &image);

MACHINE_CONFIG_EXTERN( lynx_cartslot );


/*----------- defined in audio/lynx.c -----------*/

DECLARE_LEGACY_SOUND_DEVICE(LYNX, lynx_sound);
DECLARE_LEGACY_SOUND_DEVICE(LYNX2, lynx2_sound);

void lynx_audio_write(device_t *device, int offset, UINT8 data);
UINT8 lynx_audio_read(device_t *device, int offset);
void lynx_audio_count_down(device_t *device, int nr);


#endif /* LYNX_H_ */
