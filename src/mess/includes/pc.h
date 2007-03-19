#include "driver.h"

DRIVER_INIT( pccga );
DRIVER_INIT( pcmda );
DRIVER_INIT( europc );
DRIVER_INIT( bondwell );
DRIVER_INIT( pc200 );
DRIVER_INIT( pc1512 );
DRIVER_INIT( pc1640 );
DRIVER_INIT( pc_vga );
DRIVER_INIT( t1000hx );

MACHINE_RESET( pc_mda );
MACHINE_RESET( pc_cga );
MACHINE_RESET( pc_t1t );
MACHINE_RESET( pc_aga );
MACHINE_RESET( pc_vga );

void pc_cga_frame_interrupt(void);
void pc_mda_frame_interrupt(void);
void tandy1000_frame_interrupt (void);
void pc_aga_frame_interrupt(void);
void pc_vga_frame_interrupt(void);

