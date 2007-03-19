//============================================================
//
//	guimain.c - Win32 GUI entry point
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int __declspec(dllimport) mess_gui_main(HINSTANCE    hInstance,
                   HINSTANCE    hPrevInstance,
                   LPSTR        lpCmdLine,
                   int          nCmdShow);

int WINAPI WinMain(HINSTANCE    hInstance,
                   HINSTANCE    hPrevInstance,
                   LPSTR        lpCmdLine,
                   int          nCmdShow)
{
	return mess_gui_main(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
