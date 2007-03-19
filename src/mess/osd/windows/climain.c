#include <windows.h>

int __declspec(dllimport) mess_cli_main(int argc, char **argv);

int main(int argc, char *argv[])
{
	return mess_cli_main(argc, argv);
}
