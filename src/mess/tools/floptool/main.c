/***************************************************************************

    main.c

    Floptool command line front end

    20/07/2011 Initial version by Miodrag Milanovic

***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "corestr.h"

#include "formats/mfi_dsk.h"
#include "formats/hxcmfm_dsk.h"
#include "formats/ami_dsk.h"
#include "formats/st_dsk.h"

static floppy_format_type floppy_formats[] = {
	FLOPPY_MFI_FORMAT,

	FLOPPY_MFM_FORMAT,
	FLOPPY_ADF_FORMAT,

	FLOPPY_ST_FORMAT,
	FLOPPY_MSA_FORMAT,

	NULL
};

static void display_usage()
{
	fprintf(stderr, "Usage: \n");
	fprintf(stderr, "		floptool.exe identify <inputfile> [<inputfile> ...]\n");
}

static void display_formats()
{
	fprintf(stderr, "Supported formats:\n\n");
	for(int i = 0; floppy_formats[i]; i++)
	{
		floppy_image_format_t *fif = floppy_formats[i]();
		fprintf(stderr, "%15s - %s [%s]\n", fif->name(), fif->description(), fif->extensions());
	}
}

static void display_full_usage()
{
	/* Usage */
	fprintf(stderr, "floptool - Generic floppy image manipulation tool for use with MESS\n\n");
	display_usage();
	fprintf(stderr, "\n");
	display_formats();
	fprintf(stderr, "\nExample usage:\n");
	fprintf(stderr, "        floptool.exe identify image.dsk\n\n");

}

static int identify(int argc, char *argv[])
{
	if (argc<3) {
		fprintf(stderr, "Missing name of file to identify.\n\n");
		display_usage();
		return 1;
	}

	for(int i=2; argv[i]; i++) {
		char msg[4096];
		sprintf(msg, "Error opening %s for reading", argv[i]);
		FILE *f = fopen(argv[i], "rb");
		if (!f) {
			perror(msg);
			return 1;
		}
		io_generic io;
		io.file = f;
		io.procs = &stdio_ioprocs_noclose;
		io.filler = 0xff;

		int best = 0;
		floppy_image_format_t *best_fif = 0;

		for(int j = 0; floppy_formats[j]; j++)
		{
			floppy_image_format_t *fif = floppy_formats[j]();
			int score = fif->identify(&io);
			if(score > best) {
				best = score;
				best_fif = fif;
			}
		}
		if (best_fif)
			printf("%s : %s\n", argv[i], best_fif->description());
		else
			printf("%s : Unknown format\n", argv[i]);
		fclose(f);
	}
	return 0;
}

int CLIB_DECL main(int argc, char *argv[])
{

	if (argc == 1) {
		display_full_usage();
		return 0;
	}

	if (!core_stricmp("identify", argv[1]))
		return identify(argc, argv);
	else {
		fprintf(stderr, "Unknown command '%s'\n\n", argv[1]);
		display_usage();
		return 1;
	}
}
