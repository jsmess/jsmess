/*********************************************************************

    uimess.c

    MESS supplement to ui.c.

*********************************************************************/

#include "emu.h"
#include "uimenu.h"
#include "uimess.h"
#include "uiinput.h"
#include "input.h"
#include "devices/cassette.h"


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    ui_mess_main_menu_populate - populate MESS-specific menus
-------------------------------------------------*/

void ui_mess_main_menu_populate(running_machine *machine, ui_menu *menu)
{
	/* add tape control menu */
	if (machine->devicelist.first(CASSETTE))
		ui_menu_item_append(menu, "Tape Control", NULL, 0, (void*)ui_mess_menu_tape_control);
}
