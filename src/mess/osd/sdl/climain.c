#include <windows.h>

int __declspec(dllimport) __cdecl main_(int argc, char **argv);

int main(int argc, char *argv[])
{
	return main_(argc, argv);
}

