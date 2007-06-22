/***************************************************************************

    Capcom CPS-3 Hardware

****************************************************************************/
#include "driver.h"
#include "sound/custom.h"

void *cps3_sh_start(int clock, const struct CustomSound_interface *config);

WRITE32_HANDLER( cps3_sound_w );
READ32_HANDLER( cps3_sound_r );

