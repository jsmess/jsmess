/*****************************************************************************
 *
 * includes/pc1350.h
 *
 * Pocket Computer 1350
 *
 ****************************************************************************/

#ifndef PC1350_H_
#define PC1350_H_

#define PC1350_CONTRAST (input_port_read(machine, "DSW0") & 0x07)


class pc1350_state : public driver_device
{
public:
	pc1350_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_outa;
	UINT8 m_outb;
	int m_power;
	UINT8 m_reg[0x1000];
};


/*----------- defined in machine/pc1350.c -----------*/

void pc1350_outa(device_t *device, int data);
void pc1350_outb(device_t *device, int data);
void pc1350_outc(device_t *device, int data);

int pc1350_brk(device_t *device);
int pc1350_ina(device_t *device);
int pc1350_inb(device_t *device);

MACHINE_START( pc1350 );
NVRAM_HANDLER( pc1350 );


/*----------- defined in video/pc1350.c -----------*/

READ8_HANDLER(pc1350_lcd_read);
WRITE8_HANDLER(pc1350_lcd_write);
SCREEN_UPDATE( pc1350 );

int pc1350_keyboard_line_r(running_machine &machine);


#endif /* PC1350_H_ */
