/*********************************************************************

    uimess.h

    MESS supplement to ui.c.

*********************************************************************/

#ifndef __UIMESS_H__
#define __UIMESS_H__

#include "ui.h"
#include "uimenu.h"

/***************************************************************************
    PROTOTYPES
***************************************************************************/
/* MESS-specific in-"game" UI */
void ui_mess_handler_ingame(running_machine *machine);

/* image info screen */
void ui_mess_menu_image_info(running_machine *machine, ui_menu *menu, void *parameter, void *state);

/* file manager */
void ui_mess_menu_file_manager(running_machine *machine, ui_menu *menu, void *parameter, void *state);

/* tape control */
void ui_mess_menu_tape_control(running_machine *machine, ui_menu *menu, void *parameter, void *state);
astring *tapecontrol_gettime(astring *dest, running_device *device, int *curpos, int *endpos);

/* populate MESS-specific menus */
void ui_mess_main_menu_populate(running_machine *machine, ui_menu *menu);

#endif /* __UIMESS_H__ */
