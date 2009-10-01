/*********************************************************************

    sgi.h

    Silicon Graphics MC (Memory Controller) code

*********************************************************************/

#ifndef _SGIMC_H
#define _SGIMC_H


void mc_init(running_machine *machine);
void mc_update(void);

READ32_HANDLER(mc_r);
WRITE32_HANDLER(mc_w);


#endif /* _SGIMC_H */
