#ifndef __TMC600__
#define __TMC600__

/* ---------- defined in video/tmc600.c ---------- */

WRITE8_HANDLER( tmc600_vismac_register_w );
WRITE8_DEVICE_HANDLER( tmc600_vismac_data_w );

MACHINE_DRIVER_EXTERN( tmc600_video );

#endif
