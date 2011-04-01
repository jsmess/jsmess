/*****************************************************************************
 *
 * includes/nc.h
 *
 ****************************************************************************/

#ifndef NC_H_
#define NC_H_


#define NC_NUM_COLOURS 4

#define NC_SCREEN_WIDTH        480
#define NC_SCREEN_HEIGHT       64

#define NC200_SCREEN_WIDTH		480
#define NC200_SCREEN_HEIGHT		128

#define NC200_NUM_COLOURS 4

enum
{
	NC_TYPE_1xx, /* nc100/nc150 */
	NC_TYPE_200  /* nc200 */
};


class nc_state : public driver_device
{
public:
	nc_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	emu_timer *m_serial_timer;
	char m_memory_config[4];
	emu_timer *m_keyboard_timer;
	int m_membank_rom_mask;
	int m_membank_internal_ram_mask;
	UINT8 m_poweroff_control;
	int m_card_status;
	unsigned char m_uart_control;
	int m_irq_mask;
	int m_irq_status;
	int m_irq_latch;
	int m_irq_latch_mask;
	int m_sound_channel_periods[2];
	emu_file *m_file;
	int m_previous_inputport_10_state;
	int m_previous_alarm_state;
	UINT8 m_nc200_uart_interrupt_irq;
	unsigned char *m_card_ram;
	int m_membank_card_ram_mask;
	unsigned long m_display_memory_start;
	UINT8 m_type;
	int m_card_size;
	int m_nc200_backlight;
};


/*----------- defined in video/nc.c -----------*/

extern VIDEO_START( nc );
extern SCREEN_UPDATE( nc );
extern PALETTE_INIT( nc );

void nc200_video_set_backlight(running_machine &machine, int state);


/*----------- defined in drivers/nc.c -----------*/

/* pointer to loaded data */
/* mask used to stop access over end of card ram area */


void nc_set_card_present_state(running_machine &machine, int state);


/*----------- defined in machine/nc.c -----------*/

DEVICE_START( nc_pcmcia_card );
DEVICE_IMAGE_LOAD( nc_pcmcia_card );
DEVICE_IMAGE_UNLOAD( nc_pcmcia_card );

#endif /* NC_H_ */
