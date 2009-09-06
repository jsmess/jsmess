/*****************************************************************************
 *
 * includes/adam.h
 *
 ****************************************************************************/

#ifndef ADAM_H_
#define ADAM_H_


/*----------- defined in drivers/adam.c -----------*/

void adam_set_memory_banks(running_machine *machine);
void adam_reset_pcb(running_machine *machine);


/*----------- defined in machine/adam.c -----------*/

extern int adam_lower_memory; /* Lower 32k memory Z80 address configuration */
extern int adam_upper_memory; /* Upper 32k memory Z80 address configuration */
extern int adam_joy_stat[2];
extern int adam_net_data; /* Data on AdamNet Bus */
extern int adam_pcb;

int adam_cart_verify(const UINT8 *buf, size_t size);

void adam_clear_keyboard_buffer(void);
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
