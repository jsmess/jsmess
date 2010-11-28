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
#include "devices/cartslot.h"

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

	int old_level;
	int old_data;
	int old_exrom;
	int old_game;
	UINT8 vicirq;
	emu_timer *datasette_timer;
	UINT8 *colorram;
	UINT8 *basic;
	UINT8 *kernal;
	UINT8 *chargen;
	UINT8 *memory;
	int pal;
	int tape_on;
	UINT8 *c64_roml;
	UINT8 *c64_romh;
	UINT8 *vicaddr;
	UINT8 *c128_vicaddr;
	UINT8 game;
	UINT8 exrom;
	UINT8 *io_mirror;
	UINT8 port_data;
	UINT8 *roml;
	UINT8 *romh;
	int ultimax;
	int cia1_on;
	int io_enabled;
	int is_sx64;
	UINT8 *io_ram_w_ptr;
	UINT8 *io_ram_r_ptr;
	c64_cart_t cart;
};


/*----------- defined in machine/c64.c -----------*/

/* private area */

UINT8 c64_m6510_port_read(running_device *device, UINT8 direction);
void c64_m6510_port_write(running_device *device, UINT8 direction, UINT8 data);

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
int c64_paddle_read (running_device *device, int which);
void c64_vic_interrupt (running_machine *machine, int level);

extern const mos6526_interface c64_ntsc_cia0, c64_pal_cia0;
extern const mos6526_interface c64_ntsc_cia1, c64_pal_cia1;

MACHINE_CONFIG_EXTERN( c64_cartslot );
MACHINE_CONFIG_EXTERN( ultimax_cartslot );

#endif /* C64_H_ */
