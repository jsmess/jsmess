/*****************************************************************************
 *
 * includes/pcw16.h
 *
 ****************************************************************************/

#ifndef PCW16_H_
#define PCW16_H_


#define PCW16_BORDER_HEIGHT 8
#define PCW16_BORDER_WIDTH 8
#define PCW16_NUM_COLOURS 32
#define PCW16_DISPLAY_WIDTH 640
#define PCW16_DISPLAY_HEIGHT 480

#define PCW16_SCREEN_WIDTH	(PCW16_DISPLAY_WIDTH + (PCW16_BORDER_WIDTH<<1))
#define PCW16_SCREEN_HEIGHT	(PCW16_DISPLAY_HEIGHT  + (PCW16_BORDER_HEIGHT<<1))


class pcw16_state : public driver_device
{
public:
	pcw16_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	unsigned long m_interrupt_counter;
	int m_banks[4];
	int m_4_bit_port;
	int m_fdc_int_code;
	int m_system_status;
	char *m_mem_ptr[4];
	unsigned char m_keyboard_data_shift;
	int m_keyboard_parity_table[256];
	int m_keyboard_bits;
	int m_keyboard_bits_output;
	int m_keyboard_state;
	int m_keyboard_previous_state;
	unsigned char m_rtc_seconds;
	unsigned char m_rtc_minutes;
	unsigned char m_rtc_hours;
	unsigned char m_rtc_days_max;
	unsigned char m_rtc_days;
	unsigned char m_rtc_months;
	unsigned char m_rtc_years;
	unsigned char m_rtc_control;
	unsigned char m_rtc_256ths_seconds;
	int m_previous_fdc_int_state;
	int m_colour_palette[16];
	int m_video_control;
};


/*----------- defined in video/pcw16.c -----------*/

extern PALETTE_INIT( pcw16 );
extern VIDEO_START( pcw16 );
extern SCREEN_UPDATE( pcw16 );


#endif /* PCW16_H_ */
