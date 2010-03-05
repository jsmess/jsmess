/*****************************************************************************
 *
 * includes/advision.h
 *
 ****************************************************************************/

#ifndef __ADVISION__
#define __ADVISION__

#define SCREEN_TAG	"screen"
#define I8048_TAG	"i8048"
#define COP411_TAG	"cop411"

class advision_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, advision_state(machine)); }

	advision_state(running_machine &machine) { }

	/* external RAM state */
	UINT8 *extram;
	int rambank;

	/* video state */
	int frame_start;
	int video_enable;
	int video_bank;
	int video_hpos;
	UINT8 led_latch[8];
	UINT8 *display;

	/* sound state */
	int sound_cmd;
	int sound_d;
	int sound_g;
};

/*----------- defined in machine/advision.c -----------*/

MACHINE_START( advision );
MACHINE_RESET( advision );
READ8_HANDLER ( advision_extram_r);
WRITE8_HANDLER( advision_extram_w );

/* Port P1 */
READ8_HANDLER( advision_controller_r );
WRITE8_HANDLER( advision_bankswitch_w );

/* Port P2 */
WRITE8_HANDLER( advision_av_control_w );

READ8_HANDLER ( advision_vsync_r );
READ8_HANDLER ( advision_sound_cmd_r );
WRITE8_HANDLER ( advision_sound_g_w );
WRITE8_HANDLER ( advision_sound_d_w );

/*----------- defined in video/advision.c -----------*/

VIDEO_START( advision );
VIDEO_UPDATE( advision );
PALETTE_INIT( advision );
void advision_vh_write(running_machine *machine, int data);
void advision_vh_update(running_machine *machine, int data);

#endif
