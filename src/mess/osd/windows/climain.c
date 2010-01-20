#include <windows.h>
#include <tchar.h>

#ifdef main
#undef main
#endif

#ifdef wmain
#undef wmain
#endif

int __declspec(dllimport) mess_cli_main(int argc, TCHAR **argv);

extern "C" int _tmain(int argc, TCHAR*argv[])
{
	return mess_cli_main(argc, argv);
}
