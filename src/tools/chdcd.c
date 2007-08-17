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
	file_error filerr;

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
	int i, j, trknum, m, s, f;
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

			if ((!strcmp(token, "DATAFILE")) || (!strcmp(token, "AUDIOFILE")) || (!strcmp(token, "FILE")))
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
				TOKENIZE

				m = 0;
				s = 0;
				f = 0;

				if (token[0] == '#')
				{
					/* it's a decimal offset, use it */
					f = strtoul(&token[1], NULL, 10);
				}
				else if (isdigit(token[0]))
				{
					sscanf( token, "%d:%d:%d", &m, &s, &f );

					/* convert to just frames */
					s += (m * 60);
					f += (s * 75);

					f *= (outtoc->tracks[trknum].datasize + outtoc->tracks[trknum].subsize);
				}

				outinfo->offset[trknum] = f;

				EATWHITESPACE
				TOKENIZE

				m = 0;
				s = 0;
				f = 0;

				if (isdigit(token[0]))
				{
					if( sscanf( token, "%d:%d:%d", &m, &s, &f ) == 1 )
					{
						f = m;
					}
					else
					{
						/* convert to just frames */
						s += (m * 60);
						f += (s * 75);
					}
				}
				else if( trknum == 0 && outinfo->offset[trknum] != 0 )
				{
					/* the 1st track might have a length with no offset */
					f = outinfo->offset[trknum] / (outtoc->tracks[trknum].datasize + outtoc->tracks[trknum].subsize);
					outinfo->offset[trknum] = 0;
				}
				else
				{
					/* guesstimate the track length */
					UINT64 tlen;
					printf("Warning: Estimating length of track %d.  If this is not the final or only track\n on the disc, the estimate may be wrong.\n", trknum+1);

					tlen = get_file_size(outinfo->fname[trknum]) - outinfo->offset[trknum];

					tlen /= (outtoc->tracks[trknum].datasize + outtoc->tracks[trknum].subsize);

					f = tlen;
				}

				outtoc->tracks[trknum].frames = f;
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
