#ifndef _ZX8302_H
#define _ZX8302_H

typedef struct
{
	int	chip_clock;
	int	rtc_clock;

	int *comdata, *comctl, *baudx4;

	void (*irq_callback)(int state);
} zx8302_interface;

INTERRUPT_GEN( zx8302_int );

READ8_HANDLER( zx8302_rtc_r );
WRITE8_HANDLER( zx8302_rtc_w );
WRITE8_HANDLER( zx8302_control_w );
READ8_HANDLER( zx8302_mdv_track_r );
READ8_HANDLER( zx8302_status_r );
WRITE8_HANDLER( zx8302_ipc_command_w );
WRITE8_HANDLER( zx8302_mdv_control_w );
READ8_HANDLER( zx8302_irq_status_r );
WRITE8_HANDLER( zx8302_irq_acknowledge_w );
WRITE8_HANDLER( zx8302_data_w );

void zx8302_microdrive_getinfo(const device_class *devclass, UINT32 state, union devinfo *info);

void zx8302_config(const zx8302_interface *intf);

#endif
