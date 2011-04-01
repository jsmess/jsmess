/*****************************************************************************
 *
 * includes/pmd85.h
 *
 ****************************************************************************/

#ifndef PMD85_H_
#define PMD85_H_

#include "machine/serial.h"
#include "machine/i8255a.h"

class pmd85_state : public driver_device
{
public:
	pmd85_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_rom_module_present;
	UINT8 m_ppi_port_outputs[4][3];
	UINT8 m_startup_mem_map;
	UINT8 m_pmd853_memory_mapping;
	int m_previous_level;
	int m_clk_level;
	int m_clk_level_tape;
	UINT8 m_model;
	emu_timer * m_cassette_timer;
	void (*update_memory)(running_machine &);
	serial_connection m_cassette_serial_connection;
};


/*----------- defined in machine/pmd85.c -----------*/

extern const struct pit8253_config pmd85_pit8253_interface;
extern const i8255a_interface pmd85_ppi8255_interface[4];
extern const i8255a_interface alfa_ppi8255_interface[3];
extern const i8255a_interface mato_ppi8255_interface;

 READ8_HANDLER ( pmd85_io_r );
WRITE8_HANDLER ( pmd85_io_w );
 READ8_HANDLER ( mato_io_r );
WRITE8_HANDLER ( mato_io_w );
DRIVER_INIT ( pmd851 );
DRIVER_INIT ( pmd852a );
DRIVER_INIT ( pmd853 );
DRIVER_INIT ( alfa );
DRIVER_INIT ( mato );
DRIVER_INIT ( c2717 );
extern MACHINE_RESET( pmd85 );


/*----------- defined in video/pmd85.c -----------*/

extern VIDEO_START( pmd85 );
extern SCREEN_UPDATE( pmd85 );
extern const unsigned char pmd85_palette[3*3];
extern PALETTE_INIT( pmd85 );


#endif /* PMD85_H_ */
