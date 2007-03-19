/***************************************************************************

	includes/bebox.h

	BeBox

***************************************************************************/

#ifndef BEBOX_H
#define BEBOX_H

#include "driver.h"

MACHINE_RESET( bebox );
DRIVER_INIT( bebox );
NVRAM_HANDLER( bebox );

READ64_HANDLER( bebox_cpu0_imask_r );
READ64_HANDLER( bebox_cpu1_imask_r );
READ64_HANDLER( bebox_interrupt_sources_r );
READ64_HANDLER( bebox_crossproc_interrupts_r );
READ64_HANDLER( bebox_800001F0_r );
READ64_HANDLER( bebox_800003F0_r );
READ64_HANDLER( bebox_interrupt_ack_r );
READ64_HANDLER( bebox_page_r );
READ64_HANDLER( bebox_80000480_r );
READ64_HANDLER( bebox_flash_r );

WRITE64_HANDLER( bebox_cpu0_imask_w );
WRITE64_HANDLER( bebox_cpu1_imask_w );
WRITE64_HANDLER( bebox_crossproc_interrupts_w );
WRITE64_HANDLER( bebox_processor_resets_w );
WRITE64_HANDLER( bebox_800001F0_w );
WRITE64_HANDLER( bebox_800003F0_w );
WRITE64_HANDLER( bebox_page_w );
WRITE64_HANDLER( bebox_80000480_w );
WRITE64_HANDLER( bebox_flash_w );



#endif /* BEBOX_H */
