#include <windows.h>
#include <tchar.h>

#ifdef main
#undef main
#endif

#ifdef wmain
#undef wmain
#endif

#ifdef __GNUC__
int __declspec(dllimport) mess_cli_main(int argc, char **argv);

int main(int argc, char *argv[])
{
	return mess_cli_main(argc, argv);
}
#else
int __declspec(dllimport) mess_cli_main(int argc, TCHAR **argv);

int _tmain(int argc, TCHAR*argv[])
{
	return mess_cli_main(argc, argv);
}
#endif
