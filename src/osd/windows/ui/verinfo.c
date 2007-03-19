#include <stdio.h>
#include <ctype.h>

static int read_version_integer(const char *s, int *position)
{
	int value = 0;

	if (isdigit(s[*position]))
	{
		while(isdigit(s[*position]))
		{
			value *= 10;
			value += s[(*position)++] - '0';
		}

		if ((s[*position] == '.') || (s[*position] == 'u'))
			(*position)++;
	}
	return value;
}



int main(int argc, char *argv[])
{
	extern char build_version[];
	int version_major;
	int version_minor;
	int version_build;
	int version_subbuild;
	int year = 2006;
	const char *author;
	const char *comments;
	const char *company_name;
	const char *file_description;
	const char *internal_name;
	const char *product_name;
	const char *legal_trademarks;
	const char *original_filename;
	char legal_copyright[256];
	int position = 0;

	version_major		= read_version_integer(build_version, &position);
	version_minor		= read_version_integer(build_version, &position);
	version_build		= read_version_integer(build_version, &position);
	version_subbuild	= read_version_integer(build_version, &position);

#ifdef MESS
	file_description = "Multiple Emulation Super System for Win32";
	company_name = "MESS Team";
	product_name = "MESS";
	sprintf(legal_copyright, "Copyright 1997-%d Nicola Salmoria and the MAME team", year);
	original_filename = "MESS.exe";
#else
	file_description = "Multiple Arcade Machine Emulator for Win32";
	company_name = "MAME Team";
	product_name = "MAME32";
	sprintf(legal_copyright, "Copyright 1997-%d Nicola Salmoria and the MAME team", year);
	original_filename = "MAME32.exe";
#endif
	author = "Christopher Kirmse and Michael Soderstrom";
	comments = file_description;
	internal_name = product_name;
	legal_trademarks = "";
	
	printf("VS_VERSION_INFO VERSIONINFO\n");
	printf(" FILEVERSION %d,%d,%d,%d\n", version_major, version_minor, version_build, version_subbuild);
	printf(" PRODUCTVERSION %d,%d,%d,%d\n", version_major, version_minor, version_build, version_subbuild);
	printf(" FILEFLAGSMASK 0x3fL\n");
	printf("#ifdef _DEBUG\n");
	printf(" FILEFLAGS 0x1L\n");
	printf("#else\n");
	printf(" FILEFLAGS 0x0L\n");
	printf("#endif\n");
	printf(" FILEOS 0x40004L\n");
	printf(" FILETYPE 0x1L\n");
	printf(" FILESUBTYPE 0x0L\n");
	printf("BEGIN\n");
	printf("    BLOCK \"StringFileInfo\"\n");
	printf("    BEGIN\n");
	printf("        BLOCK \"040904b0\"\n");
	printf("        BEGIN\n");
	printf("            VALUE \"Author\", \"%s\\0\"\n", author);
	printf("            VALUE \"Comments\", \"%s\\0\"\n", comments);
	printf("            VALUE \"CompanyName\", \"%s\\0\"\n", company_name);
	printf("            VALUE \"FileDescription\", \"%s\\0\"\n", file_description);
	printf("            VALUE \"FileVersion\", \"%d, %d, %d, %d\\0\"\n", version_major, version_minor, version_build, version_subbuild);
	printf("            VALUE \"InternalName\", \"%s\\0\"\n", internal_name);
	printf("            VALUE \"LegalCopyright\", \"%s\\0\"\n", legal_copyright);
	printf("            VALUE \"LegalTrademarks\", \"%s\\0\"\n", legal_trademarks);
	printf("            VALUE \"OriginalFilename\", \"%s\\0\"\n", original_filename);
	printf("            VALUE \"ProductName\", \"%s\\0\"\n", product_name);
	printf("            VALUE \"ProductVersion\", \"%d, %d, %d, %d\\0\"\n", version_major, version_minor, version_build, version_subbuild);
	printf("        END\n");
	printf("    END\n");
	printf("    BLOCK \"VarFileInfo\"\n");
	printf("    BEGIN\n");
	printf("        VALUE \"Translation\", 0x409, 1200\n");
	printf("    END\n");
	printf("END\n");

	return 0;
}
