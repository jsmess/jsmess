#ifndef PC_HDC_H
#define PC_HDC_H

#include "driver.h"

int pc_hdc_setup(void);

READ8_HANDLER  ( pc_HDC1_r );
WRITE8_HANDLER ( pc_HDC1_w );
READ8_HANDLER  ( pc_HDC2_r );
WRITE8_HANDLER ( pc_HDC2_w );

READ32_HANDLER  ( pc32_HDC1_r );
WRITE32_HANDLER ( pc32_HDC1_w );
READ32_HANDLER  ( pc32_HDC2_r );
WRITE32_HANDLER ( pc32_HDC2_w );

int pc_hdc_dack_r(void);
void pc_hdc_dack_w(int data);

#endif /* PC_HDC_H */
