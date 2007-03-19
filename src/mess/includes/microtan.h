/******************************************************************************
 *	Microtan 65
 *
 *	variables and function prototypes
 *
 *	Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *	Thanks go to Geoff Macdonald <mail@geoff.org.uk>
 *	for his site http:://www.geo255.redhotant.com
 *	and to Fabrice Frances <frances@ensica.fr>
 *	for his site http://www.ifrance.com/oric/microtan.html
 *
 ******************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "cpu/m6502/m6502.h"
#include "machine/6522via.h"
#include "sound/ay8910.h"
#include "includes/6551.h"
#include "devices/snapquik.h"

/* from src/mess/vidhrdw/microtan.c */

extern UINT8 microtan_chunky_graphics;
extern UINT8 *microtan_chunky_buffer;

/* from mess/machine/microtan.c */
DRIVER_INIT( microtan );
MACHINE_RESET( microtan );

SNAPSHOT_LOAD( microtan );
QUICKLOAD_LOAD( microtan_hexfile );

INTERRUPT_GEN( microtan_interrupt );

 READ8_HANDLER ( microtan_via_0_r );
 READ8_HANDLER ( microtan_via_1_r );
 READ8_HANDLER ( microtan_bffx_r );
 READ8_HANDLER ( microtan_sound_r );
 READ8_HANDLER ( microtan_sio_r );

WRITE8_HANDLER ( microtan_via_0_w );
WRITE8_HANDLER ( microtan_via_1_w );
WRITE8_HANDLER ( microtan_bffx_w );
WRITE8_HANDLER ( microtan_sound_w );
WRITE8_HANDLER ( microtan_sio_w );

/* from src/mess/vidhrdw/microtan.c */
extern char microtan_frame_message[64+1];
extern int microtan_frame_time;

extern WRITE8_HANDLER ( microtan_videoram_w );

extern PALETTE_INIT( microtan );
extern VIDEO_START( microtan );
extern VIDEO_UPDATE( microtan );
