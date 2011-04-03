/*****************************************************************************
 *
 * includes/apple2gs.h
 *
 * Apple IIgs
 *
 ****************************************************************************/

#ifndef APPLE2GS_H_
#define APPLE2GS_H_

#include "includes/apple2.h"

// IIgs clocks as marked on the schematics
#define APPLE2GS_28M  (XTAL_28_63636MHz) // IIGS master clock
#define APPLE2GS_14M  (APPLE2GS_28M/2)
#define APPLE2GS_7M   (APPLE2GS_28M/4)

// screen dimensions
#define BORDER_LEFT	(32)
#define BORDER_RIGHT	(32)
#define BORDER_TOP	(16)	// (plus bottom)


typedef enum
{
	CLOCKMODE_IDLE,
	CLOCKMODE_TIME,
	CLOCKMODE_INTERNALREGS,
	CLOCKMODE_BRAM1,
	CLOCKMODE_BRAM2
} apple2gs_clock_mode;


typedef enum
{
	ADBSTATE_IDLE,
	ADBSTATE_INCOMMAND,
	ADBSTATE_INRESPONSE
} adbstate_t;


class apple2gs_state : public apple2_state
{
public:
	apple2gs_state(running_machine &machine, const driver_device_config_base &config)
		: apple2_state(machine, config) { }

	UINT8 *m_slowmem;
	UINT8 m_newvideo;
	UINT8 m_docram[64*1024];
	UINT16 m_bordercolor;
	UINT8 m_vgcint;
	UINT8 m_langsel;
	UINT8 m_sltromsel;
	UINT8 m_cyareg;
	UINT8 m_inten;
	UINT8 m_intflag;
	UINT8 m_shadow;
	UINT8 m_pending_irqs;
	UINT8 m_mouse_x;
	UINT8 m_mouse_y;
	INT8 m_mouse_dx;
	INT8 m_mouse_dy;
	device_t *m_cur_slot6_image;
	emu_timer *m_scanline_timer;
	emu_timer *m_clock_timer;
	emu_timer *m_qsecond_timer;
	UINT8 m_clock_data;
	UINT8 m_clock_control;
	UINT8 m_clock_read;
	UINT8 m_clock_reg1;
	apple2gs_clock_mode m_clock_mode;
	UINT32 m_clock_curtime;
	seconds_t m_clock_curtime_interval;
	UINT8 m_clock_bram[256];
	adbstate_t m_adb_state;
	UINT8 m_adb_command;
	UINT8 m_adb_mode;
	UINT8 m_adb_kmstatus;
	UINT8 m_adb_latent_result;
	INT32 m_adb_command_length;
	INT32 m_adb_command_pos;
	UINT8 m_adb_command_bytes[8];
	UINT8 m_adb_response_bytes[8];
	UINT8 m_adb_response_length;
	INT32 m_adb_response_pos;
	UINT8 m_adb_memory[0x100];
	int m_adb_address_keyboard;
	int m_adb_address_mouse;
	UINT8 m_sndglu_ctrl;
	int m_sndglu_addr;
	int m_sndglu_dummy_read;
	bitmap_t *m_legacy_gfx;
    bool m_is_rom3;
    UINT8 m_echo_bank;
};


/*----------- defined in machine/apple2gs.c -----------*/

MACHINE_START( apple2gs );
MACHINE_START( apple2gsr1 );
MACHINE_RESET( apple2gs );

NVRAM_HANDLER( apple2gs );

void apple2gs_doc_irq(device_t *device, int state);


/*----------- defined in video/apple2gs.c -----------*/

VIDEO_START( apple2gs );
SCREEN_UPDATE( apple2gs );


#endif /* APPLE2GS_H_ */
