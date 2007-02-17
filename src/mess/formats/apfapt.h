#ifndef APFCAS_H
#define APFCAS_H

#include "driver.h"

/* frequency of wave */
#define APF_WAV_FREQUENCY	11050

int apf_cassette_fill_wave(INT16 *buffer, int length, UINT8 *bytes);
int apf_cassette_calculate_size_in_samples(int length, UINT8 *bytes);

#endif /* APFCAS_H */
