/*  CAT702 ZN security chip */
#include "osdcore.h"

void znsec_init(int chip, const UINT8 *transform);
void znsec_start(int chip);
UINT8 znsec_step(int chip, UINT8 input);
