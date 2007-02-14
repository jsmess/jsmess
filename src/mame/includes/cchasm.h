/*************************************************************************

    Cinematronics Cosmic Chasm hardware

*************************************************************************/

#include "sound/custom.h"

/*----------- defined in machine/cchasm.c -----------*/

WRITE16_HANDLER( cchasm_led_w );

/*----------- defined in audio/cchasm.c -----------*/

READ8_HANDLER( cchasm_snd_io_r );
WRITE8_HANDLER( cchasm_snd_io_w );

WRITE16_HANDLER( cchasm_io_w );
READ16_HANDLER( cchasm_io_r );

void *cchasm_sh_start(int clock, const struct CustomSound_interface *config);


/*----------- defined in video/cchasm.c -----------*/

extern UINT16 *cchasm_ram;

WRITE16_HANDLER( cchasm_refresh_control_w );
VIDEO_START( cchasm );

