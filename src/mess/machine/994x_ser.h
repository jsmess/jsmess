#ifndef __994X_SER_H__
#define __994X_SER_H__

#include "machine/tms9902.h"

extern const tms9902_interface tms9902_params_0;
extern const tms9902_interface tms9902_params_1;

DEVICE_IMAGE_LOAD( ti99_4_pio );
DEVICE_IMAGE_UNLOAD( ti99_4_pio );

DEVICE_IMAGE_LOAD( ti99_4_rs232 );
DEVICE_IMAGE_UNLOAD( ti99_4_rs232 );

void ti99_rs232_init(running_machine *machine);
void ti99_rs232_reset(running_machine *machine);

#endif /* __994X_SER_H__ */

