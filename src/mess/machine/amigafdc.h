#ifndef AMIGAFDC_H
#define AMIGAFDC_H

WRITE8_DEVICE_HANDLER( amiga_fdc_control_w );
UINT8  amiga_fdc_status_r (running_device *device);
UINT16 amiga_fdc_get_byte (running_device *device);
void amiga_fdc_setup_dma( running_device *device );

/***************************************************************************
    MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(AMIGA_FDC, amiga_fdc);

#define MDRV_AMIGA_FDC_ADD(_tag)	\
	MDRV_DEVICE_ADD((_tag),  AMIGA_FDC, 0)

#endif /* AMIGAFDC_H */
