/*****************************************************************************
 *
 * includes/pc1403.h
 *
 * Pocket Computer 1403
 *
 ****************************************************************************/

#ifndef PC1403_H_
#define PC1403_H_

#define CONTRAST (input_port_read(machine, "DSW0") & 0x07)


class pc1403_state : public driver_device
{
public:
	pc1403_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_portc;
	UINT8 m_outa;
	int m_power;
	UINT8 m_asic[4];
	int m_DOWN;
	int m_RIGHT;
	UINT8 m_reg[0x100];
};


/*----------- defined in machine/pc1403.c -----------*/

int pc1403_reset(device_t *device);
int pc1403_brk(device_t *device);
void pc1403_outa(device_t *device, int data);
//void pc1403_outb(device_t *device, int data);
void pc1403_outc(device_t *device, int data);
int pc1403_ina(device_t *device);
//int pc1403_inb(device_t *device);

DRIVER_INIT( pc1403 );
NVRAM_HANDLER( pc1403 );

READ8_HANDLER(pc1403_asic_read);
WRITE8_HANDLER(pc1403_asic_write);


/*----------- defined in video/pc1403.c -----------*/

VIDEO_START( pc1403 );
SCREEN_UPDATE( pc1403 );

READ8_HANDLER(pc1403_lcd_read);
WRITE8_HANDLER(pc1403_lcd_write);


#endif /* PC1403_H_ */
