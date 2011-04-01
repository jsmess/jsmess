/*****************************************************************************
 *
 * includes/c64.h
 *
 * Commodore C64 Home Computer
 *
 * peter.trauner@jk.uni-linz.ac.at
 *
 * Documentation: www.funet.fi
 *
 ****************************************************************************/

#ifndef C64_H_
#define C64_H_

#include "machine/6526cia.h"
#include "imagedev/cartslot.h"

#define C64_MAX_ROMBANK 64 // .crt files contain multiple 'CHIPs', i.e. rom banks (of variable size) with headers. Known carts have at most 64 'CHIPs'.

typedef struct {
	int addr, size, index, start;
} C64_ROM;

typedef struct _c64_cart_t c64_cart_t;
struct _c64_cart_t {
	C64_ROM     bank[C64_MAX_ROMBANK];
	INT8        game;
	INT8        exrom;
	UINT8       mapper;
	UINT8       n_banks;
};

class c64_state : public driver_device
{
public:
	c64_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	int m_old_level;
	int m_old_data;
	int m_old_exrom;
	int m_old_game;
	UINT8 m_vicirq;
	emu_timer *m_datasette_timer;
	UINT8 *m_colorram;
	UINT8 *m_basic;
	UINT8 *m_kernal;
	UINT8 *m_chargen;
	UINT8 *m_memory;
	int m_pal;
	int m_tape_on;
	UINT8 *m_c64_roml;
	UINT8 *m_c64_romh;
	UINT8 *m_vicaddr;
	UINT8 *m_c128_vicaddr;
	UINT8 m_game;
	UINT8 m_exrom;
	UINT8 *m_io_mirror;
	UINT8 m_port_data;
	UINT8 *m_roml;
	UINT8 *m_romh;
	int m_ultimax;
	int m_cia1_on;
	int m_io_enabled;
	int m_is_sx64;
	UINT8 *m_io_ram_w_ptr;
	UINT8 *m_io_ram_r_ptr;
	c64_cart_t m_cart;
	int m_nmilevel;
};


/*----------- defined in machine/c64.c -----------*/

/* private area */

UINT8 c64_m6510_port_read(device_t *device, UINT8 direction);
void c64_m6510_port_write(device_t *device, UINT8 direction, UINT8 data);

READ8_HANDLER ( c64_colorram_read );
WRITE8_HANDLER ( c64_colorram_write );

DRIVER_INIT( c64 );
DRIVER_INIT( c64pal );
DRIVER_INIT( ultimax );
DRIVER_INIT( c64gs );
DRIVER_INIT( sx64 );

MACHINE_START( c64 );
MACHINE_RESET( c64 );
INTERRUPT_GEN( c64_frame_interrupt );
TIMER_CALLBACK( c64_tape_timer );

/* private area */
READ8_HANDLER(c64_ioarea_r);
WRITE8_HANDLER(c64_ioarea_w);

WRITE8_HANDLER ( c64_write_io );
READ8_HANDLER ( c64_read_io );
int c64_paddle_read (device_t *device, int which);
void c64_vic_interrupt (running_machine &machine, int level);

extern const mos6526_interface c64_ntsc_cia0, c64_pal_cia0;
extern const mos6526_interface c64_ntsc_cia1, c64_pal_cia1;

MACHINE_CONFIG_EXTERN( c64_cartslot );
MACHINE_CONFIG_EXTERN( ultimax_cartslot );

#endif /* C64_H_ */
