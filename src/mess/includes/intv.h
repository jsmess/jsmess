/*****************************************************************************
 *
 * includes/intv.h
 *
 ****************************************************************************/

#ifndef INTV_H_
#define INTV_H_

typedef struct
{
	int visible;
	int xpos;
	int ypos;
	int coll;
	int collision;
	int doublex;
	int doubley;
	int quady;
	int xflip;
	int yflip;
	int behind_foreground;
	int grom;
	int card;
	int color;
	int doubleyres;
	int dirty;
} intv_sprite_type;

class intv_state : public driver_device
{
public:
	intv_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_videoram;
	intv_sprite_type m_sprite[8];
	UINT8 m_sprite_buffers[8][16][128];
	int m_color_stack_mode;
	int m_collision_registers[8];
	int m_color_stack_offset;
	int m_color_stack[4];
	int m_stic_handshake;
	int m_border_color;
	int m_col_delay;
	int m_row_delay;
	int m_left_edge_inhibit;
	int m_top_edge_inhibit;
	UINT8 m_gramdirty;
	UINT8 m_gram[512];
	UINT8 m_gramdirtybytes[512];
	UINT16 m_ram16[0x160];
	int m_intvkbd_text_blanked;
	UINT16 *m_intvkbd_dualport_ram;
	int m_intvkbd_keyboard_col;
	int m_tape_int_pending;
	int m_sr1_int_pending;
	int m_tape_interrupts_enabled;
	int m_tape_unknown_write[6];
	int m_tape_motor_mode;
	unsigned char m_ram8[256];
	UINT8 m_tms9927_num_rows;
	UINT8 m_tms9927_cursor_col;
	UINT8 m_tms9927_cursor_row;
	UINT8 m_tms9927_last_row;
};


/*----------- defined in video/stic.c -----------*/

READ16_HANDLER( intv_stic_r );
WRITE16_HANDLER( intv_stic_w );


/*----------- defined in video/intv.c -----------*/

extern VIDEO_START( intv );
extern SCREEN_UPDATE( intvkbd );

void intv_stic_screenrefresh(running_machine &machine);

READ8_HANDLER ( intvkbd_tms9927_r );
WRITE8_HANDLER ( intvkbd_tms9927_w );


/*----------- defined in machine/intv.c -----------*/

/*  for the console alone... */

DEVICE_START( intv_cart );
DEVICE_IMAGE_LOAD( intv_cart );

extern MACHINE_RESET( intv );
extern INTERRUPT_GEN( intv_interrupt );

READ16_HANDLER( intv_gram_r );
WRITE16_HANDLER( intv_gram_w );
READ16_HANDLER( intv_ram8_r );
WRITE16_HANDLER( intv_ram8_w );
READ16_HANDLER( intv_ram16_r );
WRITE16_HANDLER( intv_ram16_w );

READ8_HANDLER( intv_right_control_r );
READ8_HANDLER( intv_left_control_r );

/* for the console + keyboard component... */

DEVICE_IMAGE_LOAD( intvkbd_cart );

WRITE16_HANDLER ( intvkbd_dualport16_w );
READ8_HANDLER ( intvkbd_dualport8_lsb_r );
WRITE8_HANDLER ( intvkbd_dualport8_lsb_w );
READ8_HANDLER ( intvkbd_dualport8_msb_r );
WRITE8_HANDLER ( intvkbd_dualport8_msb_w );


/*----------- defined in audio/intv.c -----------*/

READ16_DEVICE_HANDLER( AY8914_directread_port_0_lsb_r );
WRITE16_DEVICE_HANDLER( AY8914_directwrite_port_0_lsb_w );


#endif /* INTV_H_ */
