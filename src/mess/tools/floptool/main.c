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

static FLOPPY_OPTIONS_START( supported )
	FLOPPY_OPTION( mfm, "mfm", "HxCFloppyEmulator floppy disk image", FLOPPY_MFM_FORMAT, NULL )
FLOPPY_OPTIONS_END

static void display_usage(void)
{
	fprintf(stderr, "Usage: \n");
	fprintf(stderr, "		floptool.exe identify <inputfile>\n");
}

static void display_formats(void)
{
	int i;
	fprintf(stderr, "Supported formats:\n\n");
	for (i = 0; floppyoptions_supported[i].name; i++) {
		fprintf(stderr, "%15s - %s\n",floppyoptions_supported[i].name,floppyoptions_supported[i].description);
		fprintf(stderr, "%20s [%s]\n","",floppyoptions_supported[i].extensions);
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
				floppy_image *image = new floppy_image(f, &stdio_ioprocs_noclose, FLOPPY_OPTIONS_NAME(supported));
				int best;
				const struct floppy_format_def *format = image->identify(&best);
				if (format) {
					fprintf(stderr, "File identified as %s\n",format->description);
					image->load(best);
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
