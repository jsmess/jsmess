/***************************************************************************

    CDRDAO TOC parser for CHD compression frontend

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "osdcore.h"
#include "chd.h"
#include "cdrom.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>



/***************************************************************************
    CONSTANTS & DEFINES
***************************************************************************/

#define EATWHITESPACE	\
	while ((linebuffer[i] == ' ' || linebuffer[i] == 0x09 || linebuffer[i] == '\r') && (i < 512))	\
	{	\
		i++;	\
	}

#define EATQUOTE	\
	while ((linebuffer[i] == '"' || linebuffer[i] == '\'') && (i < 512))	\
	{	\
		i++;	\
	}

#define TOKENIZE	\
	j = 0; \
	while (!isspace(linebuffer[i]) && (i < 512) && (j < 128))	\
	{	\
		token[j] = linebuffer[i];	\
		i++;	\
		j++;	\
	}	\
	token[j] = '\0';

#define TOKENIZETOCOLON	\
	j = 0; \
	while ((linebuffer[i] != ':') && (i < 512) && (j < 128))	\
	{	\
		token[j] = linebuffer[i];	\
		i++;	\
		j++;	\
	}	\
	token[j] = '\0';

#define TOKENIZETOCOLONINC	\
	j = 0; \
	while ((linebuffer[i] != ':') && (i < 512) && (j < 128))	\
	{	\
		token[j] = linebuffer[i];	\
		i++;	\
		j++;	\
	}	\
	token[j++] = ':';	\
	token[j] = '\0';



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static char linebuffer[512];



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    get_file_size - get the size of a file
-------------------------------------------------*/

static UINT64 get_file_size(const char *filename)
{
	osd_file *file;
	UINT64 filesize = 0;
	mame_file_error filerr;

	filerr = osd_open(filename, OPEN_FLAG_READ, &file, &filesize);
	if (filerr == FILERR_NONE)
		osd_close(file);
	return filesize;
}


/*-------------------------------------------------
    cdrom_parse_toc - parse a CDRDAO format TOC file
-------------------------------------------------*/

static void show_raw_message(void)
{
	printf("Note: MAME now prefers and can accept RAW format images.\n");
	printf("At least one track of this CDRDAO rip is not either RAW or AUDIO.\n");
}

chd_error cdrom_parse_toc(const char *tocfname, cdrom_toc *outtoc, cdrom_track_input_info *outinfo)
{
	FILE *infile;
	int i, j, k, trknum, m, s, f, foundcolon;
	static char token[128];

	infile = fopen(tocfname, "rt");

	if (infile == (FILE *)NULL)
	{
		return CHDERR_FILE_NOT_FOUND;
	}

	/* clear structures */
	memset(outtoc, 0, sizeof(cdrom_toc));
	memset(outinfo, 0, sizeof(cdrom_track_input_info));

	trknum = -1;

	while (!feof(infile))
	{
		/* get the next line */
		fgets(linebuffer, 511, infile);

		/* if EOF didn't hit, keep going */
		if (!feof(infile))
		{
			i = 0;
			EATWHITESPACE
			TOKENIZE

			if ((!strcmp(token, "DATAFILE")) || (!strcmp(token, "FILE")))
			{
				/* found the data file for a track */
				EATWHITESPACE
				EATQUOTE
				TOKENIZE

				/* remove trailing quote if any */
				if (token[strlen(token)-1] == '"')
				{
					token[strlen(token)-1] = '\0';
				}

				/* keep the filename */
				strncpy(&outinfo->fname[trknum][0], token, strlen(token));

				/* get either the offset or the length */
				EATWHITESPACE

				if (linebuffer[i] == '#')
				{
					/* it's a decimal offset, use it */
					TOKENIZE
					outinfo->offset[trknum] = strtoul(&token[1], NULL, 10);

					/* we're using this token, go on */
					EATWHITESPACE
					TOKENIZETOCOLONINC
				}
				else
				{
					/* no offset, just M:S:F */
					outinfo->offset[trknum] = 0;
					TOKENIZETOCOLONINC
				}

				/*
                   This is tricky: the next number can be either a raw
                   number or an M:S:F number.  Check which it is.
                   If a space or LF/CR/terminator occurs before a colon in the token,
                   it's a raw number.
                */

trycolonagain:
				foundcolon = 0;
				for (k = 0; k < strlen(token); k++)
				{
					if ((token[k] <= ' ') && (token[k] != ':'))
					{
						break;
					}

					if (token[k] == ':')
					{
						foundcolon = 1;
						break;
					}
				}

				if (!foundcolon)
				{
					// rewind to the start of the real MSF
					while (linebuffer[i] != ' ')
					{
						i--;
					}

					// check for spurious offset included by newer CDRDAOs
					if ((token[0] == '0') && (token[1] == ' '))
					{
						i++;
						EATWHITESPACE
						TOKENIZETOCOLONINC
						goto trycolonagain;
					}

					i++;
					TOKENIZE


					f = strtoul(token, NULL, 10);
				}
				else
				{
					/* now get the MSF format length (might be MSF offset too) */
					m = strtoul(token, NULL, 10);
					i++;	/* skip the colon */
					TOKENIZETOCOLON
					s = strtoul(token, NULL, 10);
					i++;	/* skip the colon */
					TOKENIZE
					f = strtoul(token, NULL, 10);

					/* convert to just frames */
					s += (m * 60);
					f += (s * 75);
				}

				EATWHITESPACE
				if (isdigit(linebuffer[i]))
				{
					f *= outtoc->tracks[trknum].datasize;

					outinfo->offset[trknum] += f;

					EATWHITESPACE
					TOKENIZETOCOLON

					m = strtoul(token, NULL, 10);
					i++;	/* skip the colon */
					TOKENIZETOCOLON
					s = strtoul(token, NULL, 10);
					i++;	/* skip the colon */
					TOKENIZE
					f = strtoul(token, NULL, 10);

					/* convert to just frames */
					s += (m * 60);
					f += (s * 75);
				}

				if (f)
				{
					outtoc->tracks[trknum].frames = f;
				}
				else	/* track can't be zero length, guesstimate it */
				{
					UINT64 tlen;

					printf("Warning: Estimating length of track %d.  If this is not the final or only track\n on the disc, the estimate may be wrong.\n", trknum+1);

					tlen = get_file_size(outinfo->fname[trknum]);

					tlen /= (outtoc->tracks[trknum].datasize + outtoc->tracks[trknum].subsize);

					outtoc->tracks[trknum].frames = tlen;
				}
			}
			else if (!strcmp(token, "TRACK"))
			{
				/* found a new track */
				trknum++;

				/* next token on the line is the track type */
				EATWHITESPACE
				TOKENIZE

				outtoc->tracks[trknum].trktype = CD_TRACK_MODE1;
				outtoc->tracks[trknum].datasize = 0;
				outtoc->tracks[trknum].subtype = CD_SUB_NONE;
				outtoc->tracks[trknum].subsize = 0;

				cdrom_convert_type_string_to_track_info(token, &outtoc->tracks[trknum]);
				if (outtoc->tracks[trknum].datasize == 0)
					printf("ERROR: Unknown track type [%s].  Contact MAMEDEV.\n", token);
				else if (outtoc->tracks[trknum].trktype != CD_TRACK_MODE1_RAW &&
						 outtoc->tracks[trknum].trktype != CD_TRACK_MODE2_RAW &&
						 outtoc->tracks[trknum].trktype != CD_TRACK_AUDIO)
					show_raw_message();

				/* next (optional) token on the line is the subcode type */
				EATWHITESPACE
				TOKENIZE

				cdrom_convert_subtype_string_to_track_info(token, &outtoc->tracks[trknum]);
			}
		}
	}

	/* close the input TOC */
	fclose(infile);

	/* store the number of tracks found */
	outtoc->numtrks = trknum + 1;

	return CHDERR_NONE;
}
