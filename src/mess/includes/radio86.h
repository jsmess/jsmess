/*****************************************************************************
 *
 * includes/radio86.h
 *
 ****************************************************************************/

#ifndef radio86_H_
#define radio86_H_

#include "machine/i8255a.h"
#include "machine/i8257.h"
#include "video/i8275.h"

class radio86_state : public driver_device
{
public:
	radio86_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_tape_value;
	UINT8 m_mikrosha_font_page;
	int m_keyboard_mask;
	UINT8* m_radio_ram_disk;
	UINT8 m_romdisk_lsb;
	UINT8 m_romdisk_msb;
	UINT8 m_disk_sel;
};


/*----------- defined in drivers/radio86.c -----------*/

INPUT_PORTS_EXTERN( radio86 );
INPUT_PORTS_EXTERN( ms7007 );


/*----------- defined in machine/radio86.c -----------*/

extern DRIVER_INIT( radio86 );
extern DRIVER_INIT( radioram );
extern MACHINE_RESET( radio86 );

extern READ8_HANDLER (radio_cpu_state_r );

extern READ8_HANDLER (radio_io_r );
extern WRITE8_HANDLER(radio_io_w );

extern const i8255a_interface radio86_ppi8255_interface_1;
extern const i8255a_interface radio86_ppi8255_interface_2;
extern const i8255a_interface rk7007_ppi8255_interface;

extern const i8255a_interface mikrosha_ppi8255_interface_1;
extern const i8255a_interface mikrosha_ppi8255_interface_2;

extern const i8275_interface radio86_i8275_interface;
extern const i8275_interface partner_i8275_interface;
extern const i8275_interface mikrosha_i8275_interface;
extern const i8275_interface apogee_i8275_interface;

extern WRITE8_HANDLER ( radio86_pagesel );
extern const i8257_interface radio86_dma;


extern void radio86_init_keyboard(running_machine &machine);


/*----------- defined in video/radio86.c -----------*/

extern I8275_DISPLAY_PIXELS(radio86_display_pixels);
extern I8275_DISPLAY_PIXELS(partner_display_pixels);
extern I8275_DISPLAY_PIXELS(mikrosha_display_pixels);
extern I8275_DISPLAY_PIXELS(apogee_display_pixels);

extern SCREEN_UPDATE( radio86 );
extern PALETTE_INIT( radio86 );

#endif /* radio86_H_ */
