
/* These are needed here otherwise the "extern const" gives compile errors */
#include "video/mc6845.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "machine/wd17xx.h"
#include "devices/snapquik.h"


/*----------- defined in machine/kaypro.c -----------*/

extern const z80pio_interface kayproii_pio_g_intf;
extern const z80pio_interface kayproii_pio_s_intf;
extern const z80sio_interface kaypro_sio_intf;
extern const wd17xx_interface kaypro_wd1793_interface;

READ8_DEVICE_HANDLER( kaypro_sio_r );
READ8_DEVICE_HANDLER( kaypro2x_sio_r );

READ8_HANDLER( kaypro2x_system_port_r );

WRITE8_DEVICE_HANDLER( kaypro_sio_w );
WRITE8_DEVICE_HANDLER( kaypro2x_sio_w );

WRITE8_HANDLER( kaypro_baud_a_w );
WRITE8_HANDLER( kayproii_baud_b_w );
WRITE8_HANDLER( kaypro2x_baud_a_w );
WRITE8_HANDLER( kaypro2x_system_port_w );

MACHINE_RESET( kayproii );
MACHINE_RESET( kaypro2x );

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
