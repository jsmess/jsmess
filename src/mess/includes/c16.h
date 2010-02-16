/*****************************************************************************
 *
 * includes/c16.h
 *
 ****************************************************************************/

#ifndef __C16_H__
#define __C16_H__

/*----------- defined in machine/c16.c -----------*/

UINT8 c16_m7501_port_read(running_device *device, UINT8 direction);
void c16_m7501_port_write(running_device *device, UINT8 direction, UINT8 data);

extern WRITE8_HANDLER(c16_6551_port_w);
extern READ8_HANDLER(c16_6551_port_r);

extern READ8_HANDLER(c16_fd1x_r);
extern WRITE8_HANDLER(plus4_6529_port_w);
extern READ8_HANDLER(plus4_6529_port_r);

extern WRITE8_HANDLER(c16_6529_port_w);
extern READ8_HANDLER(c16_6529_port_r);

extern WRITE8_HANDLER(c16_select_roms);
extern WRITE8_HANDLER(c16_switch_to_rom);
extern WRITE8_HANDLER(c16_switch_to_ram);

/* ted reads (passed to the device interface) */
extern UINT8 c16_read_keyboard(running_machine *machine, int databus);
extern void c16_interrupt(running_machine *machine, int level);
extern int c16_dma_read(running_machine *machine, int offset);
extern int c16_dma_read_rom(running_machine *machine, int offset);

extern DRIVER_INIT( c16 );
extern DRIVER_INIT( plus4 );
extern DRIVER_INIT( c16sid );
extern DRIVER_INIT( plus4sid );
extern MACHINE_RESET( c16 );
extern INTERRUPT_GEN( c16_frame_interrupt );

MACHINE_DRIVER_EXTERN( c16_cartslot );

#endif /* __C16_H__ */
