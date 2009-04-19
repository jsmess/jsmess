UINT8 kay_kbd_c_r( void );
UINT8 kay_kbd_d_r( void );
void kay_kbd_d_w( running_machine *machine, UINT8 data );
INTERRUPT_GEN( kay_kbd_interrupt );
MACHINE_RESET( kay_kbd );
INPUT_PORTS_EXTERN( kay_kbd );
