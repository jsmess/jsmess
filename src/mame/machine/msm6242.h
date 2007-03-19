#ifndef MSM6242_INCLUDE
#define MSM6242_INCLUDE

#include "driver.h"

READ8_HANDLER( msm6242_r );
WRITE8_HANDLER( msm6242_w );

READ16_HANDLER( msm6242_lsb_r );
WRITE16_HANDLER( msm6242_lsb_w );

#endif
