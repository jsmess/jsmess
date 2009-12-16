/*********************************************************************

    uimess.h

    MESS supplement to ui.c.

*********************************************************************/

#ifndef __UIMESS_H__
#define __UIMESS_H__

#include "ui.h"
#include "uimenu.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

struct _ui_mess_private;
typedef struct _ui_mess_private ui_mess_private;



/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* initialize the MESS-specific UI */
void ui_mess_init(running_machine *machine);

/* MESS-specific in-"game" UI */
int ui_mess_handler_ingame(running_machine *machine);

/* returns whether the "new UI" is active or not */
int ui_mess_use_new_ui(void);

/* image info screen */
void ui_mess_menu_image_info(running_machine *machine, ui_menu *menu, void *parameter, void *state);

/* file manager */
void ui_mess_menu_file_manager(running_machine *machine, ui_menu *menu, void *parameter, void *state);

/* tape control */
void ui_mess_menu_tape_control(running_machine *machine, ui_menu *menu, void *parameter, void *state);
astring *tapecontrol_gettime(astring *dest, const device_config *device, int *curpos, int *endpos);

/* paste */
void ui_mess_paste(running_machine *machine);

/* returns whether IPT_KEYBOARD input should be disabled */
int ui_mess_keyboard_disabled(running_machine *machine);

/* returns whether the natural keyboard is active */
int ui_mess_get_use_natural_keyboard(running_machine *machine);

/* specifies whether the natural keyboard is active */
void ui_mess_set_use_natural_keyboard(running_machine *machine, int use_natural_keyboard);

/* keyboard mode menu */
void ui_mess_menu_keyboard_mode(running_machine *machine, ui_menu *menu, void *parameter, void *state);

/* populate MESS-specific menus */
void ui_mess_main_menu_populate(running_machine *machine, ui_menu *menu);

#endif /* __UIMESS_H__ */
