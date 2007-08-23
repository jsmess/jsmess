//============================================================
//
//  verinfo.c - Version resource emitter code
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#include <ctype.h>
#include <stdio.h>

#include "osdcore.h"


extern char build_version[];



//============================================================
//  TYPE DEFINITIONS
//============================================================

typedef struct _version_info version_info;
struct _version_info
{
	int version_major;
	int version_minor;
	int version_build;
	int version_subbuild;
	const char *author;
	const char *comments;
	const char *company_name;
	const char *file_description;
	const char *internal_name;
	const char *legal_copyright;
	const char *original_filename;
	const char *product_name;
};



//============================================================
//  emit_version_info
//============================================================

static void emit_version_info(const version_info *v)
{
	printf("VS_VERSION_INFO VERSIONINFO\n");
	printf("\tFILEVERSION %d,%d,%d,%d\n", v->version_major, v->version_minor, v->version_build, v->version_subbuild);
	printf("\tPRODUCTVERSION %d,%d,%d,%d\n", v->version_major, v->version_minor, v->version_build, v->version_subbuild);
	printf("\tFILEFLAGSMASK 0x3fL\n");
#ifdef MAME_DEBUG
	if (v->version_build == 0)
		printf("\tFILEFLAGS VS_FF_DEBUG\n");
	else
		printf("\tFILEFLAGS VS_FF_PRERELEASE | VS_FF_DEBUG\n");
#else
	if (v->version_build == 0)
		printf("\tFILEFLAGS 0x0L\n");
	else
		printf("\tFILEFLAGS VS_FF_PRERELEASE\n");
#endif
	printf("\tFILEOS VOS_NT_WINDOWS32\n");
	printf("\tFILETYPE VFT_APP\n");
	printf("\tFILESUBTYPE VFT2_UNKNOWN\n");
	printf("BEGIN\n");
	printf("\tBLOCK \"StringFileInfo\"\n");
	printf("\tBEGIN\n");
	printf("#ifdef UNICODE\n");
	printf("\t\tBLOCK \"040904b0\"\n");
	printf("#else\n");
	printf("\t\tBLOCK \"040904E4\"\n");
	printf("#endif\n");
	printf("\t\tBEGIN\n");
	if (v->author != NULL)
		printf("\t\t\tVALUE \"Author\", \"%s\\0\"\n", v->author);
	if (v->comments != NULL)
		printf("\t\t\tVALUE \"Comments\", \"%s\\0\"\n", v->comments);
	if (v->company_name != NULL)
		printf("\t\t\tVALUE \"CompanyName\", \"%s\\0\"\n", v->company_name);
	if (v->file_description != NULL)
		printf("\t\t\tVALUE \"FileDescription\", \"%s\\0\"\n", v->file_description);
	printf("\t\t\tVALUE \"FileVersion\", \"%d, %d, %d, %d\\0\"\n", v->version_major, v->version_minor, v->version_build, v->version_subbuild);
	if (v->internal_name != NULL)
		printf("\t\t\tVALUE \"InternalName\", \"%s\\0\"\n", v->internal_name);
	if (v->legal_copyright != NULL)
		printf("\t\t\tVALUE \"LegalCopyright\", \"%s\\0\"\n", v->legal_copyright);
	if (v->original_filename != NULL)
		printf("\t\t\tVALUE \"OriginalFilename\", \"%s\\0\"\n", v->original_filename);
	if (v->product_name != NULL)
		printf("\t\t\tVALUE \"ProductName\", \"%s\\0\"\n", v->product_name);
	printf("\t\t\tVALUE \"ProductVersion\", \"%s\\0\"\n", build_version);
	printf("\t\tEND\n");
	printf("\tEND\n");
	printf("\tBLOCK \"VarFileInfo\"\n");
	printf("\tBEGIN\n");
	printf("#ifdef UNICODE\n");
	printf("\t\tVALUE \"Translation\", 0x409, 1200\n");
	printf("#else\n");
	printf("\t\tVALUE \"Translation\", 0x409, 1252\n");
	printf("#endif\n");
	printf("\tEND\n");
	printf("END\n");
}



//============================================================
//  parse_version_digit
//============================================================

static int parse_version_digit(const char *str, int *position)
{
	int value = 0;

	while(str[*position] && !isspace(str[*position]) && !isdigit(str[*position]))
	{
		(*position)++;
	}
	if (str[*position] && isdigit(str[*position]))
	{
		sscanf(&str[*position], "%d", &value);
		while(isdigit(str[*position]))
			(*position)++;
	}
	return value;
}



//============================================================
//  parse_version
//============================================================

static void parse_version(const char *str, int *version_major, int *version_minor,
	int *version_micro, int *year)
{
	int position = 0;
	int day = 0;
	char month[3];

	*version_major = parse_version_digit(str, &position);
	*version_minor = parse_version_digit(str, &position);
	*version_micro = parse_version_digit(str, &position);

	*year = 0;
	sscanf(&str[position], " (%c%c%c %d %d)", &month[0], &month[1], &month[2], &day, year);
}



//============================================================
//  main
//============================================================

int CLIB_DECL main(int argc, char **argv)
{
	version_info v;
	int begin_year, current_year;
	char legal_copyright[512];

	memset(&v, 0, sizeof(v));

	// parse out version string
	parse_version(build_version, &v.version_major, &v.version_minor, &v.version_build, &current_year);

#ifdef MESS
	// MESS
	v.author = "MESS Team";
	v.comments = "Multi Emulation Super System";
	v.company_name = "MESS Team";
	v.file_description = "Multi Emulation Super System";
	v.internal_name = "MESS";
	v.original_filename = "MESS";
	v.product_name = "MESS";
	begin_year = 1998;
#elif defined(WINUI)
	// MAME32
	v.author = "Christopher Kirmse and Michael Soderstrom";
	v.comments = "Multiple Arcade Machine Emulator for Win32";
	v.company_name = "MAME Team";
	v.file_description = "Multiple Arcade Machine Emulator for Win32";
	v.internal_name = "MAME32";
	v.original_filename = "MAME32";
	v.product_name = "MAME32";
	begin_year = 1997;
#else
	// MAME
	v.author = "Nicola Salmoria and the MAME Team";
	v.comments = "Multiple Arcade Machine Emulator";
	v.company_name = "MAME Team";
	v.file_description = "Multiple Arcade Machine Emulator";
	v.internal_name = "MAME";
	v.original_filename = "MAME";
	v.product_name = "MAME";
	begin_year = 1996;
#endif

	// build legal_copyright string
	v.legal_copyright = legal_copyright;
	snprintf(legal_copyright, ARRAY_LENGTH(legal_copyright), "Copyright %d-%d Nicola Salmoria and the MAME team", begin_year, current_year);

	// emit the info
	emit_version_info(&v);
	return 0;
}
