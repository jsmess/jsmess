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
#define PCW_PRINTER_WIDTH	(80*16)
#define PCW_PRINTER_HEIGHT	(20*16)


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
	INT16 printer_headpos;
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
	UINT8 printer_serial;  // value if shift/store data pin
	UINT8 printer_shift;  // state of shift register
	UINT8 printer_shift_output;  // output presented to the paper feed motor and print head motor
	UINT8 head_motor_state;
	UINT8 linefeed_motor_state;
	UINT16 printer_pins;
	UINT8 printer_p2;  // MCU port P2 state
	UINT32 paper_feed;  // amount of paper fed through printer, by n/360 inches.  One line feed is 61/360in (from the linefeed command in CP/M;s ptr menu)
	bitmap_t* prn_output;
	UINT8 printer_p2_prev;
	emu_timer* prn_stepper;
	emu_timer* prn_pins;
};


/*----------- defined in video/pcw.c -----------*/

extern VIDEO_START( pcw );
extern VIDEO_UPDATE( pcw );
extern PALETTE_INIT( pcw );


#endif /* PCW_H_ */
