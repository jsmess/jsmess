#ifndef AMIGAFDC_H
#define AMIGAFDC_H

void amiga_fdc_control_w( const device_config *device, UINT8 data );
int amiga_fdc_status_r( void );
unsigned short amiga_fdc_get_byte( void );
void amiga_fdc_setup_dma( running_machine *machine );

void amiga_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);

#endif /* AMIGAFDC_H */
