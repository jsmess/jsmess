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

	UINT8 portc;
	UINT8 outa;
	int power;
	UINT8 asic[4];
	int DOWN;
	int RIGHT;
	UINT8 reg[0x100];
};


/*----------- defined in machine/pc1403.c -----------*/

int pc1403_reset(running_device *device);
int pc1403_brk(running_device *device);
void pc1403_outa(running_device *device, int data);
//void pc1403_outb(running_device *device, int data);
void pc1403_outc(running_device *device, int data);
int pc1403_ina(running_device *device);
//int pc1403_inb(running_device *device);

DRIVER_INIT( pc1403 );
NVRAM_HANDLER( pc1403 );

READ8_HANDLER(pc1403_asic_read);
WRITE8_HANDLER(pc1403_asic_write);


/*----------- defined in video/pc1403.c -----------*/

VIDEO_START( pc1403 );
VIDEO_UPDATE( pc1403 );

READ8_HANDLER(pc1403_lcd_read);
WRITE8_HANDLER(pc1403_lcd_write);


#endif /* PC1403_H_ */
