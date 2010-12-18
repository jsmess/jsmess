
/* These are needed here otherwise the "extern const" gives compile errors */
#include "video/mc6845.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "machine/wd17xx.h"
#include "devices/snapquik.h"


class kaypro_state : public driver_device
{
public:
	kaypro_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *videoram;
	running_device *kayproii_z80pio_g;
	running_device *kayproii_z80pio_s;
	running_device *z80sio;
	running_device *kaypro2x_z80sio;
	running_device *printer;
	running_device *fdc;
	UINT8 system_port;
	UINT8 mc6845_cursor[16];
	UINT8 mc6845_reg[32];
	UINT8 mc6845_ind;
	running_device *mc6845;
	const UINT8 *FNT;
	UINT8 speed;
	UINT8 flash;
	UINT8 framecnt;
	UINT16 cursor;
	UINT16 mc6845_video_address;
	struct _kay_kbd_t *kbd;
};


/*----------- defined in machine/kay_kbd.c -----------*/

UINT8 kay_kbd_c_r( running_machine *machine );
UINT8 kay_kbd_d_r( running_machine *machine );
void kay_kbd_d_w( running_machine *machine, UINT8 data );
INTERRUPT_GEN( kay_kbd_interrupt );
MACHINE_RESET( kay_kbd );
INPUT_PORTS_EXTERN( kay_kbd );


/*----------- defined in machine/kaypro.c -----------*/

extern const z80pio_interface kayproii_pio_g_intf;
extern const z80pio_interface kayproii_pio_s_intf;
extern const z80pio_interface kaypro4_pio_s_intf;
extern const z80sio_interface kaypro_sio_intf;
extern const wd17xx_interface kaypro_wd1793_interface;

READ8_DEVICE_HANDLER( kaypro_sio_r );

READ8_HANDLER( kaypro2x_system_port_r );

WRITE8_DEVICE_HANDLER( kaypro_sio_w );

WRITE8_HANDLER( kaypro_baud_a_w );
WRITE8_HANDLER( kayproii_baud_b_w );
WRITE8_HANDLER( kaypro2x_baud_a_w );
WRITE8_HANDLER( kaypro2x_system_port_w );

MACHINE_RESET( kayproii );
MACHINE_START( kayproii );
MACHINE_RESET( kaypro2x );
MACHINE_START( kaypro2x );

QUICKLOAD_LOAD( kayproii );
QUICKLOAD_LOAD( kaypro2x );

/*----------- defined in video/kaypro.c -----------*/

MC6845_UPDATE_ROW( kaypro2x_update_row );
PALETTE_INIT( kaypro );
VIDEO_START( kaypro );
VIDEO_UPDATE( kayproii );
VIDEO_UPDATE( omni2 );
VIDEO_UPDATE( kaypro2x );

READ8_HANDLER( kaypro_videoram_r );
READ8_HANDLER( kaypro2x_status_r );
READ8_HANDLER( kaypro2x_videoram_r );

WRITE8_HANDLER( kaypro_videoram_w );
WRITE8_HANDLER( kaypro2x_index_w );
WRITE8_HANDLER( kaypro2x_register_w );
WRITE8_HANDLER( kaypro2x_videoram_w );
