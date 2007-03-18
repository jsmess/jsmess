/*********************************************************************

	uimess.h

	MESS supplement to ui.c.

*********************************************************************/

#ifndef UIMESS_H
#define UIMESS_H

#include "ui.h"

int mess_ui_active(void);
void mess_ui_update(void);
int mess_use_new_ui(void);
int mess_disable_builtin_ui(void);

/* image info screen */
int ui_sprintf_image_info(char *buf);
UINT32 ui_menu_image_info(UINT32 state);

/* file manager */
int filemanager(int selected);

/* tape control */
int tapecontrol(int selected);

#endif /* UIMESS_H */
