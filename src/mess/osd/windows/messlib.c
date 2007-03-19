//============================================================
//
//  messlib.c - Entry points for messlib.dll
//
//============================================================

// We want to use the main() in src/windows/main.c, but we have
// to export the entry point with dllexport.  So we'll do a trick
// to wrap it
#define main mame_main
#include "windows/main.c"
#undef main

//============================================================
//  mess_cli_main - main entry proc for CLI MESS
//============================================================

int __declspec(dllexport) mess_cli_main(int argc, char **argv)
{
	return mame_main(argc, argv);
}



//============================================================
//  mess_gui_main - main entry proc for GUI MESS
//============================================================

int __declspec(dllexport) mess_gui_main(
	HINSTANCE    hInstance,
	HINSTANCE    hPrevInstance,
	LPTSTR       lpCmdLine,
	int          nCmdShow)
{
	extern int Mame32Main(HINSTANCE, LPTSTR, int);
	return Mame32Main(hInstance, lpCmdLine, nCmdShow);
}
