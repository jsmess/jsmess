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

	emu_timer *serial_timer;
	char memory_config[4];
	emu_timer *keyboard_timer;
	int membank_rom_mask;
	int membank_internal_ram_mask;
	UINT8 poweroff_control;
	int card_status;
	unsigned char uart_control;
	int irq_mask;
	int irq_status;
	int irq_latch;
	int irq_latch_mask;
	int sound_channel_periods[2];
	mame_file *file;
	int previous_inputport_10_state;
	int previous_alarm_state;
	UINT8 nc200_uart_interrupt_irq;
	unsigned char *card_ram;
	int membank_card_ram_mask;
	unsigned long display_memory_start;
	UINT8 type;
	int card_size;
	int nc200_backlight;
};


/*----------- defined in video/nc.c -----------*/

extern VIDEO_START( nc );
extern VIDEO_UPDATE( nc );
extern PALETTE_INIT( nc );

void nc200_video_set_backlight(running_machine *machine, int state);


/*----------- defined in drivers/nc.c -----------*/

/* pointer to loaded data */
/* mask used to stop access over end of card ram area */


void nc_set_card_present_state(running_machine *machine, int state);


/*----------- defined in machine/nc.c -----------*/

DEVICE_START( nc_pcmcia_card );
DEVICE_IMAGE_LOAD( nc_pcmcia_card );
DEVICE_IMAGE_UNLOAD( nc_pcmcia_card );

#endif /* NC_H_ */
