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

unsigned char scc_get_reg_a(int reg);
unsigned char scc_get_reg_b(int reg);
void scc_set_reg_a(int reg, unsigned char data);
void scc_set_reg_b(int reg, unsigned char data);

READ8_HANDLER(scc_r);
WRITE8_HANDLER(scc_w);

#endif /* _8630SCC_H */
