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

#include "formats/hxcmfm_dsk.h"

static floppy_format_type floppy_formats[] = {
	FLOPPY_MFM_FORMAT,
	NULL
};

static void display_usage(void)
{
	fprintf(stderr, "Usage: \n");
	fprintf(stderr, "		floptool.exe identify <inputfile>\n");
}

static void display_formats(void)
{
	fprintf(stderr, "Supported formats:\n\n");
	for(int i = 0; floppy_formats[i]; i++)
	{
		floppy_image_format_t *fif = floppy_formats[i]();
		fprintf(stderr, "%15s - %s\n", fif->name(), fif->description());
		fprintf(stderr, "%20s [%s]\n", "", fif->extensions());
	}
}

int CLIB_DECL main(int argc, char *argv[])
{
	FILE *f;

	if (argc > 1)
	{
		if (!core_stricmp("identify", argv[1]))
		{
			// convert command
			if (argc!=3) {
				fprintf(stderr, "Wrong parameter number.\n\n");
				display_usage();
				return -1;
			} else {
				f = fopen(argv[2], "rb");
				if (!f) {
					fprintf(stderr, "File %s not found.\n",argv[2]);
					return -1;
				}
				floppy_image *image = new floppy_image(f, &stdio_ioprocs_noclose, floppy_formats);
				int best;
				floppy_image_format_t *format = image->identify(&best);
				if (format) {
					fprintf(stderr, "File identified as %s\n",format->description());
					format->load(image);
				} else {
					fprintf(stderr, "Unable to identified file type\n");
				}
				fclose(f);
				goto theend;
			}
		}
	}

	/* Usage */
	fprintf(stderr, "floptool - Generic floppy image manipulation tool for use with MESS\n\n");
	display_usage();
	fprintf(stderr, "\n");
	display_formats();
	fprintf(stderr, "\nExample usage:\n");
	fprintf(stderr, "        floptool.exe identify image.dsk\n\n");

theend :
	return 0;
}
