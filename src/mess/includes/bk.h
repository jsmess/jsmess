/*****************************************************************************
 *
 * includes/bk.h
 *
 ****************************************************************************/

#ifndef BK_H_
#define BK_H_


class bk_state : public driver_device
{
public:
	bk_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT16 scrool;
	UINT16 *bk0010_video_ram;
	UINT16 kbd_state;
	UINT16 key_code;
	UINT16 key_pressed;
	UINT16 key_irq_vector;
	UINT16 drive;
};


/*----------- defined in machine/bk.c -----------*/

extern MACHINE_START( bk0010 );
extern MACHINE_RESET( bk0010 );

extern READ16_HANDLER (bk_key_state_r);
extern WRITE16_HANDLER(bk_key_state_w);
extern READ16_HANDLER (bk_key_code_r);
extern READ16_HANDLER (bk_vid_scrool_r);
extern WRITE16_HANDLER(bk_vid_scrool_w);
extern READ16_HANDLER (bk_key_press_r);
extern WRITE16_HANDLER(bk_key_press_w);

extern READ16_HANDLER (bk_floppy_cmd_r);
extern WRITE16_HANDLER(bk_floppy_cmd_w);
extern READ16_HANDLER (bk_floppy_data_r);
extern WRITE16_HANDLER(bk_floppy_data_w);

/*----------- defined in video/bk.c -----------*/

extern VIDEO_START( bk0010 );
extern VIDEO_UPDATE( bk0010 );

#endif /* BK_H_ */
