#ifndef __KB_KEYTRO_H_
#define __KB_KEYTRO_H_

#define KEYTRONIC_KB3270PC_CPU	"kb_kb3270_pc"

WRITE8_HANDLER( kb_keytronic_set_clock_signal );
WRITE8_HANDLER( kb_keytronic_set_data_signal );

/* The code expects kb_keytronic_set_host_interface to be called at init or reset time to
   initialize the internal callbacks.
 */
void kb_keytronic_set_host_interface( running_machine *machine, write8_space_func clock_cb, write8_space_func data_cb );

INPUT_PORTS_EXTERN( kb_keytronic_pc );
INPUT_PORTS_EXTERN( kb_keytronic_at );
MACHINE_DRIVER_EXTERN( kb_keytronic );

#endif  /* __KB_KEYTRO_H_ */
