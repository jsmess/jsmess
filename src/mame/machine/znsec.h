/*  CAT702 ZN security chip */


void znsec_init(int chip, const unsigned char *transform);
void znsec_start(int chip);
unsigned char znsec_step(int chip, unsigned char input);
