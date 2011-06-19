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

#include "machine/ins8250.h"

typedef enum
{
	TYPE_MICROSOFT_MOUSE,
	TYPE_MOUSE_SYSTEMS
} PC_MOUSE_PROTOCOL;


/*----------- defined in machine/pc_mouse.c -----------*/

INS8250_HANDSHAKE_OUT( pc_mouse_handshake_in );

// set base for input port
void pc_mouse_set_serial_port(device_t *ins8250);
void pc_mouse_initialise(running_machine &machine);

INPUT_PORTS_EXTERN( pc_mouse_mousesystems );
INPUT_PORTS_EXTERN( pc_mouse_microsoft );
INPUT_PORTS_EXTERN( pc_mouse_none );


#ifdef __cplusplus
}
#endif

#endif /* PC_MOUSE_H_ */
