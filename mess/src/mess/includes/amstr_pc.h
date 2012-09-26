/*****************************************************************************
 *
 * includes/amstr_pc.h
 *
 ****************************************************************************/

#ifndef AMSTR_PC_H_
#define AMSTR_PC_H_


#define PC200_MODE (input_port_read(machine, "DSW0") & 0x30)
#define PC200_MDA 0x30


/*----------- defined in machine/amstr_pc.c -----------*/

READ8_HANDLER( pc1640_port60_r );
WRITE8_HANDLER( pc1640_port60_w );
READ16_HANDLER( pc1640_16le_port60_r );
WRITE16_HANDLER( pc1640_16le_port60_w );

READ16_HANDLER( pc1640_16le_mouse_x_r );
READ16_HANDLER( pc1640_16le_mouse_y_r );
WRITE16_HANDLER( pc1640_16le_mouse_x_w );
WRITE16_HANDLER( pc1640_16le_mouse_y_w );

READ8_HANDLER( pc1640_port3d0_r );
READ8_HANDLER( pc200_port378_r );
READ8_HANDLER( pc1640_port378_r );
READ8_HANDLER( pc1640_port4278_r );
READ8_HANDLER( pc1640_port278_r );

READ16_HANDLER( pc200_16le_port378_r );
READ16_HANDLER( pc200_16le_port278_r );
READ16_HANDLER( pc1640_16le_port378_r );
READ16_HANDLER( pc1640_16le_port3d0_r );
READ16_HANDLER( pc1640_16le_port4278_r );
READ16_HANDLER( pc1640_16le_port278_r );

INPUT_PORTS_EXTERN( amstrad_keyboard );


#endif /* AMSTR_PC_H_ */
