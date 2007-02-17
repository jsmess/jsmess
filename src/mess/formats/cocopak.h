#ifndef COCOPAK_H
#define COCOPAK_H

#include "driver.h"

typedef struct {
	UINT16 length;
	UINT16 start;
} pak_header;

typedef struct {
	UINT16 reg_pc;
	UINT16 reg_x;
	UINT16 reg_y;
	UINT16 reg_u;
	UINT16 reg_s;
	UINT8 reg_dp;
	UINT8 reg_b;
	UINT8 reg_a;
	UINT8 reg_cc;
	UINT8 pia[8];
	UINT16 sam;
} pak_decodedtrailer;

/* This function takes a set of bytes, interprets a PAK trailer and
 * extracts the interesting information
 */
int pak_decode_trailer(UINT8 *rawtrailer, int rawtrailerlen, pak_decodedtrailer *trailer);

#endif /* COCOPAK_H */
