/*****************************************************************************
 *
 * includes/c16.h
 *
 ****************************************************************************/

#ifndef __C16_H__
#define __C16_H__

class c16_state : public driver_device
{
public:
	c16_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* memory pointers */
	UINT8 *      m_mem10000;
	UINT8 *      m_mem14000;
	UINT8 *      m_mem18000;
	UINT8 *      m_mem1c000;
	UINT8 *      m_mem20000;
	UINT8 *      m_mem24000;
	UINT8 *      m_mem28000;
	UINT8 *      m_mem2c000;

	/* misc */
	UINT8        m_keyline[10];
	UINT8        m_port6529;
	int          m_lowrom, m_highrom;
	int          m_old_level;

	int          m_sidcard, m_pal;

	/* devices */
	legacy_cpu_device *m_maincpu;
	device_t *m_ted7360;
	device_t *m_serbus;
	device_t *m_cassette;
	device_t *m_messram;
	device_t *m_sid;
};


/*----------- defined in machine/c16.c -----------*/

UINT8 c16_m7501_port_read(device_t *device, UINT8 direction);
void c16_m7501_port_write(device_t *device, UINT8 direction, UINT8 data);

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
extern UINT8 c16_read_keyboard(running_machine &machine, int databus);
extern void c16_interrupt(running_machine &machine, int level);
extern int c16_dma_read(running_machine &machine, int offset);
extern int c16_dma_read_rom(running_machine &machine, int offset);

extern DRIVER_INIT( c16 );
extern DRIVER_INIT( plus4 );
extern DRIVER_INIT( c16sid );
extern DRIVER_INIT( plus4sid );
extern MACHINE_RESET( c16 );
extern INTERRUPT_GEN( c16_frame_interrupt );

MACHINE_CONFIG_EXTERN( c16_cartslot );

#endif /* __C16_H__ */
