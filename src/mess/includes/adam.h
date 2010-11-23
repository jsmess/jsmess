/*****************************************************************************
 *
 * includes/adam.h
 *
 ****************************************************************************/

#ifndef ADAM_H_
#define ADAM_H_


class adam_state : public driver_device
{
public:
	adam_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	int last_state;
	int lower_memory;
	int upper_memory;
	int joy_stat[2];
	int net_data;
	int pcb;
	int joy_mode;
	int KeyboardBuffer[20];
	UINT8 KbRepeatTable[256];
};


/*----------- defined in drivers/adam.c -----------*/

void adam_set_memory_banks(running_machine *machine);
void adam_reset_pcb(running_machine *machine);


/*----------- defined in machine/adam.c -----------*/

//int adam_cart_verify(const UINT8 *buf, size_t size);

void adam_clear_keyboard_buffer(running_machine *machine);
void adam_explore_keyboard(running_machine *machine);

READ8_HANDLER  ( adamnet_r );
WRITE8_HANDLER ( adamnet_w );
READ8_HANDLER  ( adam_paddle_r );
WRITE8_HANDLER ( adam_paddle_toggle_off );
WRITE8_HANDLER ( adam_paddle_toggle_on );
WRITE8_HANDLER ( adam_memory_map_controller_w );
READ8_HANDLER ( adam_memory_map_controller_r );
READ8_HANDLER ( adam_video_r );
WRITE8_HANDLER ( adam_video_w );
WRITE8_HANDLER ( adam_common_writes_w );


#endif /* ADAM_H_ */
