/*****************************************************************************
 *
 * includes/pc_mouse.h
 *
 ****************************************************************************/

#ifndef PC_MOUSE_H_
#define PC_MOUSE_H_

#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
	TYPE_MICROSOFT_MOUSE,
	TYPE_MOUSE_SYSTEMS
} PC_MOUSE_PROTOCOL;


/*----------- defined in machine/pc_mouse.c -----------*/

void pc_mouse_handshake_in(int n, int data);

// set base for input port
void pc_mouse_set_serial_port(int uart_index);
void pc_mouse_initialise(void);

INPUT_PORTS_EXTERN( pc_mouse_mousesystems );
INPUT_PORTS_EXTERN( pc_mouse_microsoft );
INPUT_PORTS_EXTERN( pc_mouse_none );


#ifdef __cplusplus
}
#endif

#endif /* PC_MOUSE_H_ */
