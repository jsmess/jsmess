/*****************************************************************************
 *
 * includes/c16.h
 *
 ****************************************************************************/

#ifndef __C16_H__
#define __C16_H__

class c16_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, c16_state(machine)); }

	c16_state(running_machine &machine)
		: driver_data_t(machine) { }

	/* memory pointers */
	UINT8 *      mem10000;
	UINT8 *      mem14000;
	UINT8 *      mem18000;
	UINT8 *      mem1c000;
	UINT8 *      mem20000;
	UINT8 *      mem24000;
	UINT8 *      mem28000;
	UINT8 *      mem2c000;

	/* misc */
	UINT8        keyline[10];
	UINT8        port6529;
	int          lowrom, highrom;
	int          old_level;

	int          sidcard, pal;

	/* devices */
	legacy_cpu_device *maincpu;
	running_device *ted7360;
	running_device *serbus;
	running_device *cassette;
	running_device *messram;
	running_device *sid;
};


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
