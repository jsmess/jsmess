/*****************************************************************************
 *
 * includes/channelf.h
 *
 ****************************************************************************/

#ifndef CHANNELF_H_
#define CHANNELF_H_



/*----------- defined in video/channelf.c -----------*/

extern UINT8 channelf_val_reg;
extern UINT8 channelf_row_reg;
extern UINT8 channelf_col_reg;

extern PALETTE_INIT( channelf );
extern VIDEO_START( channelf );
extern VIDEO_UPDATE( channelf );


/*----------- defined in audio/channelf.c -----------*/

DECLARE_LEGACY_SOUND_DEVICE(CHANNELF, channelf_sound);

void channelf_sound_w(running_machine *machine, int mode);


#endif /* CHANNELF_H_ */
