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

	UINT8 *videoram;
	intv_sprite_type sprite[8];
	UINT8 sprite_buffers[8][16][128];
	int color_stack_mode;
	int collision_registers[8];
	int color_stack_offset;
	int color_stack[4];
	int stic_handshake;
	int border_color;
	int col_delay;
	int row_delay;
	int left_edge_inhibit;
	int top_edge_inhibit;
	UINT8 gramdirty;
	UINT8 gram[512];
	UINT8 gramdirtybytes[512];
	UINT16 ram16[0x160];
	int intvkbd_text_blanked;
	UINT16 *intvkbd_dualport_ram;
	int intvkbd_keyboard_col;
	int tape_int_pending;
	int sr1_int_pending;
	int tape_interrupts_enabled;
	int tape_unknown_write[6];
	int tape_motor_mode;
	unsigned char ram8[256];
	UINT8 tms9927_num_rows;
	UINT8 tms9927_cursor_col;
	UINT8 tms9927_cursor_row;
	UINT8 tms9927_last_row;
};


/*----------- defined in video/stic.c -----------*/

READ16_HANDLER( intv_stic_r );
WRITE16_HANDLER( intv_stic_w );


/*----------- defined in video/intv.c -----------*/

extern VIDEO_START( intv );
extern VIDEO_UPDATE( intvkbd );

void intv_stic_screenrefresh(running_machine *machine);

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
