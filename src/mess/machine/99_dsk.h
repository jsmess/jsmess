#ifndef TI99_DSK_H_
#define TI99_DSK_H_

#include "machine/wd17xx.h"
#include "formats/flopimg.h"

/* Note: this currently mirrors the flag in ti99_4x.h */
#define HAS_99CCFDC 0

void ti99_floppy_controllers_init_all(running_machine *machine);
void ti99_fdc_reset(running_machine *machine);
#if HAS_99CCFDC
void ti99_ccfdc_reset(running_machine *machine);
#endif
void ti99_bwg_reset(running_machine *machine);
void ti99_hfdc_reset(running_machine *machine);

int ti99_image_in_80_track_drive(void);

extern const wd17xx_interface ti99_wd17xx_interface;
#endif /* TI99_DSK_H_ */
