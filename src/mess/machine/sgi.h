/*********************************************************************

	sgi.h

	Silicon Graphics MC (Memory Controller) code

*********************************************************************/

#ifndef _SGIMC_H
#define _SGIMC_H

#include "mame.h"

void mc_update( int nParam );
void mc_init(void);

READ32_HANDLER(mc_r);
WRITE32_HANDLER(mc_w);

#endif /* _SGIMC_H */
