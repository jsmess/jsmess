/*****************************************************************************
 *
 * includes/pc1401.h
 *
 * Pocket Computer 1401
 *
 ****************************************************************************/

#ifndef PC1401_H_
#define PC1401_H_

#define CONTRAST (input_port_read(machine, "DSW0") & 0x07)


class pc1401_state : public driver_device
{
public:
	pc1401_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_portc;
	UINT8 m_outa;
	UINT8 m_outb;
	int m_power;
	UINT8 m_reg[0x100];
};


/*----------- defined in machine/pc1401.c -----------*/

int pc1401_reset(device_t *device);
int pc1401_brk(device_t *device);
void pc1401_outa(device_t *device, int data);
void pc1401_outb(device_t *device, int data);
void pc1401_outc(device_t *device, int data);
int pc1401_ina(device_t *device);
int pc1401_inb(device_t *device);

DRIVER_INIT( pc1401 );
NVRAM_HANDLER( pc1401 );


/*----------- defined in video/pc1401.c -----------*/

READ8_HANDLER(pc1401_lcd_read);
WRITE8_HANDLER(pc1401_lcd_write);
SCREEN_UPDATE( pc1401 );


#endif /* PC1401_H_ */
