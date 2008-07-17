/*********************************************************************

	uimess.h

	MESS supplement to ui.c.

*********************************************************************/

#ifndef UIMESS_H
#define UIMESS_H

#include "ui.h"
#include "uimenu.h"

int mess_ui_active(void);
void mess_ui_update(running_machine *machine);
int mess_use_new_ui(void);
int mess_disable_builtin_ui(running_machine *machine);

/* image info screen */
void ui_menu_image_info(running_machine *machine, ui_menu *menu, void *parameter, void *state);

/* file manager */
void menu_file_manager(running_machine *machine, ui_menu *menu, void *parameter, void *state);

/* tape control */
void menu_tape_control(running_machine *machine, ui_menu *menu, void *parameter, void *state);
astring *tapecontrol_gettime(astring *dest, const device_config *device, int *curpos, int *endpos);

/* paste */
void ui_paste(running_machine *machine);

#endif /* UIMESS_H */
