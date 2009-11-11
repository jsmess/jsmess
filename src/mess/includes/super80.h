#include "video/mc6845.h"
#include "machine/z80pio.h"

/*----------- defined in drivers/super80.c -----------*/

extern UINT8 *super80_pcgram;

/*----------- defined in video/super80.c -----------*/

PALETTE_INIT( super80m );
VIDEO_UPDATE( super80 );
VIDEO_UPDATE( super80d );
VIDEO_UPDATE( super80e );
VIDEO_UPDATE( super80m );
VIDEO_START( super80 );
VIDEO_EOF( super80m );

READ8_HANDLER( super80v_low_r );
READ8_HANDLER( super80v_high_r );
WRITE8_HANDLER( super80v_low_w );
WRITE8_HANDLER( super80v_high_w );
WRITE8_HANDLER( super80v_10_w );
WRITE8_HANDLER( super80v_11_w );
WRITE8_HANDLER( super80_f1_w );
VIDEO_START( super80v );
VIDEO_UPDATE( super80v );
MC6845_UPDATE_ROW( super80v_update_row );

/*----------- defined in machine/super80.c -----------*/

READ8_HANDLER( super80_dc_r );
READ8_HANDLER( super80_f2_r );
WRITE8_HANDLER( super80_dc_w );
WRITE8_HANDLER( super80_f0_w );
WRITE8_HANDLER( super80r_f0_w );
READ8_DEVICE_HANDLER( super80_pio_r );
WRITE8_DEVICE_HANDLER( super80_pio_w );
MACHINE_RESET( super80 );
DRIVER_INIT( super80 );
DRIVER_INIT( super80v );

extern UINT8 super80_shared;
extern const z80pio_interface super80_pio_intf;

