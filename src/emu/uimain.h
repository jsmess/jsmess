/***************************************************************************

    uimain.h

    Internal MAME menus for the user interface.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __UIMAIN_H__
#define __UIMAIN_H__

/* slider handler */
UINT32 ui_slider_ui_handler(running_machine &machine, render_container *container, UINT32 state);

/* force game select menu */
void ui_menu_force_game_select(running_machine &machine, render_container *container);

/* handle the main menu */
void menu_main(running_machine &machine, ui_menu *menu, void *parameter, void *state);

#endif	/* __UIMAIN_H__ */
