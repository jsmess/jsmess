/*****************************************************************************
 *
 * includes/channelf.h
 *
 ****************************************************************************/

#ifndef CHANNELF_H_
#define CHANNELF_H_

#include "sound/custom.h"


/*----------- defined in video/channelf.c -----------*/

extern UINT8 channelf_val_reg;
extern UINT8 channelf_row_reg;
extern UINT8 channelf_col_reg;

extern PALETTE_INIT( channelf );
extern VIDEO_START( channelf );
extern VIDEO_UPDATE( channelf );


/*----------- defined in audio/channelf.c -----------*/

void channelf_sound_w(int);
CUSTOM_START( channelf_sh_custom_start );


#endif /* CHANNELF_H_ */
