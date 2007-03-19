/*********************************************************************

	8530scc.h

	Zilog 8530 SCC (Serial Control Chip) code

*********************************************************************/

#ifndef _8630SCC_H
#define _8630SCC_H

#include "mame.h"

struct scc8530_interface
{
	void (*acknowledge)(void);
};



void scc_init(const struct scc8530_interface *intf);
void scc_set_status(int status);

READ8_HANDLER(scc_r);
WRITE8_HANDLER(scc_w);

#endif /* _8630SCC_H */
