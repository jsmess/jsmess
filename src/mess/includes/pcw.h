/*****************************************************************************
 *
 * includes/pcw.h
 *
 ****************************************************************************/

#ifndef PCW_H_
#define PCW_H_


#define PCW_BORDER_HEIGHT 8
#define PCW_BORDER_WIDTH 8
#define PCW_NUM_COLOURS 2
#define PCW_DISPLAY_WIDTH 720
#define PCW_DISPLAY_HEIGHT 256

#define PCW_SCREEN_WIDTH	(PCW_DISPLAY_WIDTH + (PCW_BORDER_WIDTH<<1))
#define PCW_SCREEN_HEIGHT	(PCW_DISPLAY_HEIGHT  + (PCW_BORDER_HEIGHT<<1))


class pcw_state : public driver_device
{
public:
	pcw_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	int boot;
	int system_status;
	int fdc_interrupt_code;
	int interrupt_counter;
	UINT8 banks[4];
	unsigned char bank_force;
	UINT8 timer_irq_flag;
	UINT8 nmi_flag;
	UINT8 printer_command;
	UINT8 printer_data;
	UINT8 printer_status;
	UINT8 printer_headpos;
	UINT16 kb_scan_row;
	UINT8 mcu_keyboard_data[16];
	UINT8 mcu_transmit_reset_seq;
	UINT8 mcu_transmit_count;
	UINT8 mcu_selected;
	UINT8 mcu_buffer;
	UINT8 mcu_prev;
	unsigned int roller_ram_addr;
	unsigned short roller_ram_offset;
	unsigned char vdu_video_control_register;
};


/*----------- defined in video/pcw.c -----------*/

extern VIDEO_START( pcw );
extern VIDEO_UPDATE( pcw );
extern PALETTE_INIT( pcw );


#endif /* PCW_H_ */
