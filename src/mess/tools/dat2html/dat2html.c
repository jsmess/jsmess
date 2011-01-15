/******************************************************************************

  MESS - dat2html.c

  Generates an index and individual system html files from sysinfo.dat

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "emu.h"
#include "osdcore.h"

static void replace_lt_gt(char *line)
{
	char buff[1024] = "", *src = line, *dst = buff;

	if (!strchr(src, '<') && !strchr(src, '>'))
		return;

	while( *src )
	{
		if( *src == '<' )
			dst += sprintf(dst, "&lt;");
		else
		if( *src == '>' )
			dst += sprintf(dst, "&gt;");
		else
			*dst++ = *src;
		src++;
	}
	*dst = '\0';
	strcpy(line, buff);
}

static void a_href_url(char *line)
{
	char buff[1024], c;
	char *url_beg = strstr(line, "http://"), *url_end;
	//int length;
	if (!url_beg)
		return;
	url_end = strchr(url_beg, ' ');
	if (!url_end)
		url_end = url_beg + strlen(url_beg);
	//length = (int) (url_end - url_beg);
	/* insert the a href */
	strcpy(buff, line);
	/* terminate URL */
	c = *url_end;
	*url_end = '\0';
	/* terminate buffer right before the URL */
	buff[(int)(url_beg - line)] = '\0';
	/* add <a href=" to the buffer */
	strcat(buff, "<a href=\"");
	strcat(buff, url_beg);
	strcat(buff, "\">");
	strcat(buff, url_beg);
	strcat(buff, "</a>");
	*url_end = c;
	strcat(buff, url_end);
	strcpy(line, buff);
}


int CLIB_DECL main(int ac, char *av[])
{
	char dat_filename[128] = "sysinfo.dat";
	char html_filename[128] = "sysinfo.htm";
	char html_directory[128] = "sysinfo";
	char system_filename[128] = "";
	char system_name[128] = "";
	int /*systemcount = 0,*/ linecount = 0, emptycount = 0, ulcount = 0;
	FILE *dat, *html, *html_system = NULL;
	time_t tm;

	tm = time(NULL);

	if( ac > 1 )
	{
		strcpy(dat_filename, av[1]);
		if( ac > 2 )
		{
			strcpy(html_filename, av[2]);
			if( ac > 3 )
			{
				strcpy(html_directory, av[3]);
			}
		}
	}

	dat = fopen(dat_filename, "r");
	if( !dat )
	{
		fprintf(stderr, "cannot open input file '%s'.\n", dat_filename);
		return 1;
	}

	html = fopen(html_filename, "w");
	if( !html )
	{
		fclose(dat);
		fprintf(stderr, "cannot create output file '%s'.\n", html_filename);
		return 1;
	}

	//osd_mkdir(html_directory);

	/* Head of the index html file */
	fprintf(html, "<html>\n");
	fprintf(html, "<head>\n");
	fprintf(html, "<title>Contents of %s</title>\n", dat_filename);
	fprintf(html, "</head>\n");
	fprintf(html, "<body leftmargin= 20 rightmargin = 20>\n");
	fprintf(html, "<h1>Contents of %s</h1>\n", dat_filename);
	fprintf(html, "<hr>\n");
	fprintf(html, "<p>\n");

	while( !feof(dat) )
	{
		char line[1024], *eol;

		fgets(line, sizeof(line), dat);
		eol = strchr(line, '\n');
		if( eol )
			*eol = '\0';
		eol = strchr(line, '\r');
		if( eol )
			*eol = '\0';
		if( line[0] != '#' )
		{
			if( line[0] == '$' )
			{
				if( mame_strnicmp(line + 1, "info", 4) == 0 )
				{
					char *eq = strchr(line, '=');
					strcpy(system_name, eq + 1);

					sprintf(system_filename, "%s/%s.htm", html_directory, system_name);
					fprintf(html, "&bull;&nbsp;&nbsp;<a href=\"%s\">%s</a>\n", system_filename, system_name);

					/*systemcount++;*/

					html_system = fopen(system_filename, "w");

					if( !html_system )
					{
						fclose(html);
						fclose(dat);
						fprintf(stderr, "cannot create system_name file '%s'.\n", system_filename);
						return 1;
					}

					/* Head of the system html code */
					fprintf(html_system, "<html>\n");
					fprintf(html_system, "<head>\n");
					fprintf(html_system, "<title>Info for %s</title>\n", system_name);
					fprintf(html_system, "</head>\n");
					fprintf(html_system, "<body leftmargin= 20 rightmargin = 20>\n");
					fprintf(html_system, "<table width=100%%>\n");
					fprintf(html_system, "<tr>\n");
					fprintf(html_system, "<td width=25%%><h4><a href=\"../%s\">Back to index</a></h4></td>\n", html_filename);
					fprintf(html_system, "<td><h1>Info for %s</h1></td>\n", system_name);
					fprintf(html_system, "</tr>\n");
					fprintf(html_system, "</table>\n");
					fprintf(html_system, "<hr>\n");
					linecount = 0;
					emptycount = 0;
				}
				else
				if( mame_strnicmp(line + 1, "bio", 3) == 0 )
				{
					/* that's just fine... */
				}
				else
				if( mame_strnicmp(line + 1, "end", 3) == 0 )
				{
					if (html_system)
					{
						fprintf(html_system, "<hr>\n");
						fprintf(html_system, "<center><font size=-2>Generated on %s</font></center>\n", ctime(&tm));
						fprintf(html_system, "</body>\n");
						fprintf(html_system, "</html>\n");
						fclose(html_system);
						html_system = NULL;
					}
				}
			}
			else
			{
				if( html_system )
				{
					if ( *line == 0 )
					{
						if ( emptycount++ > 1 )
							fprintf(html_system, "<br>\n");
					}
					else
					{
						replace_lt_gt(line);
						a_href_url(line);
						emptycount = 0;
						if ( linecount == 0 )
						{
							if( ulcount )
								fprintf(html_system, "</ul>\n");
							ulcount = 0;
							/* first line is header 4 */
							fprintf(html_system, "<h4>%s</h4>\n", line);
							/* Add description to index file */
							fprintf(html, " - <b>%s</b></br>\n", line);
						}
						else
						{
							int ul = 0;
							while( line[ul] && isspace(line[ul]) )
								ul++;
							/* lines beginning with a dash are lists!? */
							if( ul > 0 && line[ul] == '-' )
							{
								if( ulcount == 0 )
									fprintf(html_system, "<ul>\n");
								fprintf(html_system, "<li>%s\n", line + ul + 1);
								ulcount++;
							}
							else
							{
								if( ulcount )
									fprintf(html_system, "</ul>\n");
								ulcount = 0;
								/* lines ending in a colon are bold */
								if( line[strlen(line)-1] == ':' )
									fprintf(html_system, "<p><b>%s</b><br>\n", line);
								else
									fprintf(html_system, "%s<br>\n", line);
							}
						}
					}
				}
				linecount++;
			}
		}
	}

	fprintf(html, "</p>\n");
	fprintf(html, "<hr>\n");
	fprintf(html, "<center><font size=-2>Generated on %s</font></center>\n", ctime(&tm));
	fprintf(html, "</body>\n");
	fprintf(html, "</html>\n");
	fclose(html);
	fclose(dat);
	return 0;
}
