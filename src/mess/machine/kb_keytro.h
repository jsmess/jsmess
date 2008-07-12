#ifndef __KB_KEYTRO_H_
#define __KB_KEYTRO_H_

WRITE8_HANDLER( kb_keytronic_set_clock_signal );
WRITE8_HANDLER( kb_keytronic_set_data_signal );

/* The code expects kb_keytronic_set_host_interface to be called at init or reset time to
   initialize the internal callbacks.
 */
void kb_keytronic_set_host_interface( write8_machine_func clock_cb, write8_machine_func data_cb );


MACHINE_DRIVER_EXTERN( kb_keytronic );

#endif  /* __KB_KEYTRO_H_ */
