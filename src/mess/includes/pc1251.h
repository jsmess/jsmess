/*****************************************************************************
 *
 * includes/pc1251.h
 *
 * Pocket Computer 1251
 *
 ****************************************************************************/

#ifndef PC1251_H_
#define PC1251_H_


#define PC1251_CONTRAST (input_port_read(machine, "DSW0") & 0x07)


class pc1251_state : public driver_device
{
public:
	pc1251_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_outa;
	UINT8 m_outb;
	int m_power;
	UINT8 m_reg[0x100];
};


/*----------- defined in machine/pc1251.c -----------*/

void pc1251_outa(device_t *device, int data);
void pc1251_outb(device_t *device, int data);
void pc1251_outc(device_t *device, int data);

int pc1251_reset(device_t *device);
int pc1251_brk(device_t *device);
int pc1251_ina(device_t *device);
int pc1251_inb(device_t *device);

DRIVER_INIT( pc1251 );
NVRAM_HANDLER( pc1251 );


/*----------- defined in video/pc1251.c -----------*/

READ8_HANDLER(pc1251_lcd_read);
WRITE8_HANDLER(pc1251_lcd_write);
SCREEN_UPDATE( pc1251 );


#endif /* PC1251_H_ */
