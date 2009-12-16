//============================================================
//
//  menu.h - Win32 MESS menu handling
//
//============================================================

#ifndef MENU_H
#define MENU_H

#include <windows.h>



//============================================================
//  PROTOTYPES
//============================================================

int win_setup_menus(running_machine *machine, HMODULE module, HMENU menu_bar);
LRESULT CALLBACK win_mess_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam);
void win_toggle_menubar(void);
#ifdef HAS_WINDOW_MENU
int win_create_menu(running_machine *machine, HMENU *menus);
#endif


#endif /* MENU_H */
