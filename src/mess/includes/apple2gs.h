/*****************************************************************************
 *
 * includes/apple2gs.h
 *
 * Apple IIgs
 *
 ****************************************************************************/

#ifndef APPLE2GS_H_
#define APPLE2GS_H_


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


class apple2gs_state : public driver_device
{
public:
	apple2gs_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *slowmem;
	UINT8 newvideo;
	UINT8 docram[64*1024];
	UINT16 bordercolor;
	UINT8 vgcint;
	UINT8 langsel;
	UINT8 sltromsel;
	UINT8 cyareg;
	UINT8 inten;
	UINT8 intflag;
	UINT8 shadow;
	UINT8 pending_irqs;
	UINT8 mouse_x;
	UINT8 mouse_y;
	INT8 mouse_dx;
	INT8 mouse_dy;
	device_t *cur_slot6_image;
	emu_timer *scanline_timer;
	emu_timer *clock_timer;
	emu_timer *qsecond_timer;
	UINT8 clock_data;
	UINT8 clock_control;
	UINT8 clock_read;
	UINT8 clock_reg1;
	apple2gs_clock_mode clock_mode;
	UINT32 clock_curtime;
	seconds_t clock_curtime_interval;
	UINT8 clock_bram[256];
	adbstate_t adb_state;
	UINT8 adb_command;
	UINT8 adb_mode;
	UINT8 adb_kmstatus;
	UINT8 adb_latent_result;
	INT32 adb_command_length;
	INT32 adb_command_pos;
	UINT8 adb_command_bytes[8];
	UINT8 adb_response_bytes[8];
	UINT8 adb_response_length;
	INT32 adb_response_pos;
	UINT8 adb_memory[0x100];
	int adb_address_keyboard;
	int adb_address_mouse;
	UINT8 sndglu_ctrl;
	int sndglu_addr;
	int sndglu_dummy_read;
	bitmap_t *legacy_gfx;
};


/*----------- defined in machine/apple2gs.c -----------*/

MACHINE_START( apple2gs );
MACHINE_RESET( apple2gs );

NVRAM_HANDLER( apple2gs );

void apple2gs_doc_irq(device_t *device, int state);


/*----------- defined in video/apple2gs.c -----------*/

VIDEO_START( apple2gs );
VIDEO_UPDATE( apple2gs );


#endif /* APPLE2GS_H_ */
