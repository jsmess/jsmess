#include "mame.h"
#include "sound/custom.h"

/* in vidhrdw/channelf.c */
extern UINT8 channelf_val_reg;
extern UINT8 channelf_row_reg;
extern UINT8 channelf_col_reg;

extern PALETTE_INIT( channelf );
extern VIDEO_START( channelf );
extern VIDEO_UPDATE( channelf );

/* in sndhrdw/channelf.c */
void channelf_sound_w(int);

void *channelf_sh_custom_start(int clock, const struct CustomSound_interface *config);

