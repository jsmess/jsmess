//============================================================
//
//	menu.h - Win32 MESS menu handling
//
//============================================================

#ifndef MENU_H
#define MENU_H

#include <windows.h>



//============================================================
//	GLOBAL VARIABLES
//============================================================

extern int win_use_natural_keyboard;



//============================================================
//	PROTOTYPES
//============================================================

int win_setup_menus(HMODULE module, HMENU menu_bar);
LRESULT CALLBACK win_mess_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam);
void win_toggle_menubar(void);



#endif /* MENU_H */
