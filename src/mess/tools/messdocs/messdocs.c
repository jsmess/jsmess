#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ctype.h>
#include <tchar.h>
#include <shlwapi.h>
#include <stdio.h>
#include <ctype.h>

#include "pool.h"
#include "sys/stat.h"
#include "osdcore.h"
#include "strconv.h"
#include "winutf8.h"
#include "expat.h"

struct messdocs_state
{
	const char *m_dest_dir;

	const char *m_title;
	const char *m_default_topic;

	object_pool *m_pool;
	XML_Parser m_parser;
	int m_depth;
	int m_error;
	char *m_toc_dir;
	FILE *m_chm_toc;
};


/***************************************************************************
    PROTOTYPES
***************************************************************************/

static void *expat_malloc(size_t size);
static void *expat_realloc(void *ptr, size_t size);
static void expat_free(void *ptr);
static void rtrim(char *buf);


//============================================================
//  win_error_to_emu_file_error
//============================================================

static file_error win_error_to_emu_file_error(DWORD error)
{
	file_error filerr;

	// convert a Windows error to a file_error
	switch (error)
	{
		case ERROR_SUCCESS:
			filerr = FILERR_NONE;
			break;

		case ERROR_OUTOFMEMORY:
			filerr = FILERR_OUT_OF_MEMORY;
			break;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			filerr = FILERR_NOT_FOUND;
			break;

		case ERROR_ACCESS_DENIED:
			filerr = FILERR_ACCESS_DENIED;
			break;

		case ERROR_SHARING_VIOLATION:
			filerr = FILERR_ALREADY_OPEN;
			break;

		default:
			filerr = FILERR_FAILURE;
			break;
	}
	return filerr;
}

//============================================================
//  osd_is_path_separator
//============================================================

static int osd_is_path_separator(char c)
{
	return (c == '/') || (c == '\\');
}

//============================================================
//  osd_mkdir
//============================================================

file_error osd_mkdir(const char *dir)
{
	file_error filerr = FILERR_NONE;

	TCHAR *tempstr = tstring_from_utf8(dir);
	if (!tempstr)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto done;
	}

	if (!CreateDirectory(tempstr, NULL))
	{
		filerr = win_error_to_emu_file_error(GetLastError());
		goto done;
	}

done:
	if (tempstr)
		osd_free(tempstr);
	return filerr;
}


//============================================================
//  osd_rmdir
//============================================================

file_error osd_rmdir(const char *dir)
{
	file_error filerr = FILERR_NONE;

	TCHAR *tempstr = tstring_from_utf8(dir);
	if (!tempstr)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto done;
	}

	if (!RemoveDirectory(tempstr))
	{
		filerr = win_error_to_emu_file_error(GetLastError());
		goto done;
	}

done:
	if (tempstr)
		osd_free(tempstr);
	return filerr;
}


//============================================================
//  osd_copyfile
//============================================================

file_error osd_copyfile(const char *destfile, const char *srcfile)
{
	// wrapper for win_copy_file_utf8()
	if (!win_copy_file_utf8(srcfile, destfile, TRUE))
		return win_error_to_emu_file_error(GetLastError());

	return FILERR_NONE;
}

/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

static void ATTR_PRINTF(3,4) process_error(struct messdocs_state *state, const char *tag, const char *msgfmt, ...)
{
	/*va_list va;*/
	/*char buf[512];*/
	const char *msg;

	if (msgfmt)
	{
		//va_start(va, msgfmt);
		//vsprintf(buf, msgfmt, va);
		//va_end(va);
		//msg = buf;
		msg = "";
	}
	else
	{
		msg = XML_ErrorString(XML_GetErrorCode(state->m_parser));
	}

	fprintf(stderr, "%u:%s:%s\n", (unsigned) XML_GetCurrentLineNumber(state->m_parser), tag ? tag : "", msg);
	state->m_error = 1;
}


static const XML_Char *find_attribute(const XML_Char **attributes, const XML_Char *seek_attribute)
{
	int i;
	for (i = 0; attributes[i] && strcmp(attributes[i], seek_attribute); i += 2)
		;
	return attributes[i] ? attributes[i+1] : NULL;
}



static void combine_path(char *buf, size_t buflen, const char *relpath)
{
	size_t i;

	for (i = 0; buf[i]; i++)
	{
		if (osd_is_path_separator(buf[i]))
			buf[i] = PATH_SEPARATOR[0];
	}

	while(osd_is_path_separator(buf[--i]))
		;
	i++;
	buf[i++] = PATH_SEPARATOR[0];
	buf[i] = '\0';

	while(relpath[0])
	{
		/* skip over './' constructs */
		while ((relpath[0] == '.') && osd_is_path_separator(relpath[1]))
			relpath += 2;

		while((relpath[0] == '.') && (relpath[1] == '.') && osd_is_path_separator(relpath[2]))
		{
			while(i && osd_is_path_separator(buf[--i]))
				buf[i] = '\0';
			while(i && !osd_is_path_separator(buf[i]))
				buf[i--] = '\0';
			relpath += 3;
		}

		snprintf(&buf[i], buflen - i, "%s", relpath);
		relpath += strlen(&buf[i]);
		i += strlen(&buf[i]);
	}
}



static char *make_path(const char *dir, const char *filepath)
{
	char buf[1024];
	char *path;

	snprintf(buf, ARRAY_LENGTH(buf), "%s", dir);
	combine_path(buf, ARRAY_LENGTH(buf), filepath);

	path = (char*)malloc(strlen(buf) + 1);
	if (!path)
		return NULL;
	strcpy(path, buf);
	return path;
}



static void copy_file_to_dest(const char *dest_dir, const char *src_dir, const char *filepath)
{
	char *dest_path = NULL;
	char *src_path = NULL;
	int i;

	/* create dest path */
	dest_path = make_path(dest_dir, filepath);
	if (!dest_path)
		goto done;

	/* create source path */
	src_path = make_path(src_dir, filepath);
	if (!src_path)
		goto done;

	/* create partial directories */
	for (i = strlen(dest_dir) + 1; dest_path[i]; i++)
	{
		if (osd_is_path_separator(dest_path[i]))
		{
			dest_path[i] = '\0';
			osd_mkdir(dest_path);
			dest_path[i] = PATH_SEPARATOR[0];
		}
	}

	osd_copyfile(dest_path, src_path);

done:
	if (dest_path)
		free(dest_path);
	if (src_path)
		free(src_path);
}



static void html_encode(char *buf, size_t buflen)
{
	size_t i, slen;
	size_t offset;
	const char *entity;

	for (i = 0; buf[i]; i++)
	{
		switch(buf[i])
		{
			case '<':	entity = "&lt;";	break;
			case '>':	entity = "&gt;";	break;
			case '&':	entity = "&amp;";	break;
			default:	entity = NULL;		break;
		}

		if (entity)
		{
			slen = strlen(&buf[i]) + 1;
			offset = MIN(slen, buflen - i);
			memmove(&buf[i + strlen(entity) - 1], &buf[i], offset);
			memcpy(&buf[i], entity, MIN(strlen(entity), buflen - i));
		}
	}
}



struct system_info
{
	const char *name;
	const char *desc;
};

static int CLIB_DECL str_compare(const void *p1, const void *p2)
{
	const struct system_info *si1 = (const struct system_info *) p1;
	const struct system_info *si2 = (const struct system_info *) p2;
	return strcoll(si1->desc ? si1->desc : "", si2->desc ? si2->desc : "");
}



static void start_handler(void *data, const XML_Char *tagname, const XML_Char **attributes)
{
	const char *title;
	const char *name;
	const char *filepath;
	const char *srcpath;
	const char *destpath;
	const char *datfile_foldername;
	struct system_info *sysinfo_array;
	int sys_count;
	char buf[512];
	const char *sysname;
	char sysfilename[512];
	char *datfile_path = NULL;
	char *s;
	FILE *datfile = NULL;
	FILE *sysfile = NULL;
	int lineno = 0;
	int i;
	int ul = FALSE;

	struct messdocs_state *state = (struct messdocs_state *) data;

	if (state->m_depth == 0)
	{
		/* help tag */
		if (strcmp(tagname, "help"))
		{
			process_error(state, NULL, "Expected tag 'help'");
			return;
		}

		title = find_attribute(attributes, "title");
		if (title)
			state->m_title = pool_strdup_lib(state->m_pool, title);
	}
	else if (!strcmp(tagname, "topic"))
	{
		/* topic tag */
		name = find_attribute(attributes, "text");
		filepath = find_attribute(attributes, "filepath");

		/* output TOC info */
		fprintf(state->m_chm_toc, "\t<LI> <OBJECT type=\"text/sitemap\">\n");
		fprintf(state->m_chm_toc, "\t\t<param name=\"Name\"  value=\"%s\">\n", name);
		fprintf(state->m_chm_toc, "\t\t<param name=\"Local\" value=\"%s\">\n", filepath);
		fprintf(state->m_chm_toc, "\t\t</OBJECT>\n");

		/* copy file */
		copy_file_to_dest(state->m_dest_dir, state->m_toc_dir, filepath);

		if (!state->m_default_topic)
			state->m_default_topic = pool_strdup_lib(state->m_pool, filepath);
	}
	else if (!strcmp(tagname, "folder"))
	{
		/* folder tag */
		name = find_attribute(attributes, "text");
		fprintf(state->m_chm_toc, "\t<LI> <OBJECT type=\"text/sitemap\">\n");
		fprintf(state->m_chm_toc, "\t\t<param name=\"Name\" value=\"%s\">\n", name);
		fprintf(state->m_chm_toc, "\t\t</OBJECT>\n");
		fprintf(state->m_chm_toc, "\t\t<UL>\n");
	}
	else if (!strcmp(tagname, "file"))
	{
		/* file tag */
		filepath = find_attribute(attributes, "filepath");
		copy_file_to_dest(state->m_dest_dir, state->m_toc_dir, filepath);
	}
	else if (!strcmp(tagname, "datfile"))
	{
		/* datfile tag */
		srcpath = find_attribute(attributes, "srcpath");
		destpath = find_attribute(attributes, "destpath");
		datfile_foldername = find_attribute(attributes, "text");

		datfile_path = make_path(state->m_toc_dir, srcpath);
		datfile = fopen(datfile_path, "r");
		if (!datfile)
		{
			process_error(state, NULL, "Cannot open datfile '%s'\n", datfile_path);
			return;
		}

		snprintf(buf, ARRAY_LENGTH(buf), "%s", state->m_dest_dir);
		combine_path(buf, ARRAY_LENGTH(buf), destpath);
		osd_mkdir(buf);

		sysinfo_array = NULL;
		sys_count = 0;
		sysname = NULL;

		while(!feof(datfile))
		{
			fgets(buf, ARRAY_LENGTH(buf), datfile);
			s = strchr(buf, '\n');
			if (s)
				*s = '\0';
			rtrim(buf);

			switch(buf[0]) {
			case '$':
				if (!strncmp(buf, "$info", 5))
				{
					/* $info */
					s = strchr(buf, '=');
					s = s ? s + 1 : &buf[strlen(buf)];

					sysinfo_array = (system_info*)pool_realloc_lib(state->m_pool, sysinfo_array, sizeof(*sysinfo_array) * (sys_count + 1));
					if (!sysinfo_array)
						goto outofmemory;
					sysinfo_array[sys_count].name = pool_strdup_lib(state->m_pool, s);
					sysinfo_array[sys_count].desc = NULL;

					sysname = sysinfo_array[sys_count].name;
					sys_count++;

					snprintf(sysfilename, sizeof(sysfilename), "%s%s%s%s%s.htm", state->m_dest_dir, PATH_SEPARATOR, destpath, PATH_SEPARATOR, s);

					if (sysfile)
						fclose(sysfile);
					sysfile = fopen(sysfilename, "w");
					lineno = 0;
				}
				else if (!strncmp(buf, "$bio", 4))
				{
					/* $bio */
				}
				else if (!strncmp(buf, "$end", 4))
				{
					/* $end */
				}
				break;
			case '#':
			case '\0':
				/* comments */
				break;
			default:
				/* normal line */
				if (!sysfile)
					break;

				html_encode(buf, ARRAY_LENGTH(buf));

				if (!strncmp(buf, "======", 6) && lineno == 0)
				{
					char *heading = (char*)malloc(strlen(buf) + 1);
					memset(heading, 0, strlen(buf) + 1);
					strncpy(heading, buf + 6, strlen(buf) - 12);
					fprintf(sysfile, "<h1>%s</h1>\n", heading);
					fprintf(sysfile, "<p><i>(directory: %s)</i></p>\n", sysname);

					if (!sysinfo_array[sys_count-1].desc)
						sysinfo_array[sys_count-1].desc = pool_strdup_lib(state->m_pool, heading);

					free(heading);
				}
				else if (buf[0] == '=')
				{
					int count;
					char *heading = (char*)malloc(strlen(buf) + 1);
					memset(heading, 0, strlen(buf) + 1);

					if (ul)
					{
						fprintf(sysfile, "</ul>");
						ul = FALSE;
					}

					for (count = 0; buf[count] == '='; count++);

					strncpy(heading, buf + count, strlen(buf) - count*2);
					fprintf(sysfile, "<h%d>%s</h%d>\n", 7-count, heading, 7-count);
					free(heading);
				}
				else if (!strncmp(buf, "  * ", 4))
				{
					if (!ul)
					{
						fprintf(sysfile, "<ul>");
					}

					ul = TRUE;

					fprintf(sysfile, "<li>%s</li>", buf + 4);
				}
				else
				{
					if (ul)
					{
						fprintf(sysfile, "</ul>");
						ul = FALSE;
					}
					fprintf(sysfile, "%s\n", buf);
				}
				lineno++;
				break;
			}
		}

		/* now write out all toc */
		qsort(sysinfo_array, sys_count, sizeof(*sysinfo_array), str_compare);

		fprintf(state->m_chm_toc, "\t<LI> <OBJECT type=\"text/sitemap\">\n");
		fprintf(state->m_chm_toc, "\t\t<param name=\"Name\" value=\"%s\">\n", datfile_foldername);
		fprintf(state->m_chm_toc, "\t\t</OBJECT>\n");
		fprintf(state->m_chm_toc, "\t\t<UL>\n");

		for (i = 0; i < sys_count; i++)
		{
			fprintf(state->m_chm_toc, "\t<LI> <OBJECT type=\"text/sitemap\">\n");
			fprintf(state->m_chm_toc, "\t\t<param name=\"Name\" value=\"%s\">\n", sysinfo_array[i].desc);
			fprintf(state->m_chm_toc, "\t\t<param name=\"Local\" value=\"%s%s%s.htm\">\n", destpath, PATH_SEPARATOR, sysinfo_array[i].name);
			fprintf(state->m_chm_toc, "\t\t</OBJECT>\n");
		}

		fprintf(state->m_chm_toc, "\t\t</UL>\n");
	}

	state->m_depth++;

outofmemory:
	if (datfile_path)
		free(datfile_path);
	if (datfile)
		fclose(datfile);
	if (sysfile)
		fclose(sysfile);
}



static void end_handler(void *data, const XML_Char *name)
{
	struct messdocs_state *state = (struct messdocs_state *) data;

	state->m_depth--;

	if (!strcmp(name, "folder"))
	{
		fprintf(state->m_chm_toc, "\t</UL>\n");
	}
}



static void data_handler(void *data, const XML_Char *s, int len)
{
}



static int rmdir_recursive(const char *dir_path)
{
	osd_directory *dir;
	const osd_directory_entry *ent;
	osd_directory_entry *ent2;
	char *newpath;

	dir = osd_opendir(dir_path);
	if (dir)
	{
		while((ent = osd_readdir(dir)) != NULL)
		{
			if (strcmp(ent->name, ".") && strcmp(ent->name, ".."))
			{
				newpath = (char*)malloc(strlen(dir_path) + 1 + strlen(ent->name) + 1);
				if (!newpath)
					return -1;

				strcpy(newpath, dir_path);
				strcat(newpath, PATH_SEPARATOR);
				strcat(newpath, ent->name);

				ent2 = osd_stat(newpath);
				if (ent2)
				{
					if (ent2->type == ENTTYPE_DIR)
						rmdir_recursive(newpath);
					else
						osd_rmfile(newpath);
					free(ent2);
				}
				free(newpath);
			}
		}
		osd_closedir(dir);
	}
	osd_rmdir(dir_path);
	return 0;
}



int messdocs(const char *toc_filename, const char *dest_dir, const char *help_project_filename,
	const char *help_contents_filename, const char *help_filename)
{
	char buf[4096];
	struct messdocs_state state;
	int len;
	int done;
	FILE *in;
	FILE *chm_hhp;
	int i;
	char *s;
	XML_Memory_Handling_Suite memcallbacks;

	memset(&state, 0, sizeof(state));
	state.m_pool = pool_alloc_lib(NULL);

	/* open the DOC */
	in = fopen(toc_filename, "r");
	if (!in)
	{
		fprintf(stderr, "Cannot open TOC file '%s'\n", toc_filename);
		goto error;
	}

	/* figure out the TOC directory */
	state.m_toc_dir = pool_strdup_lib(state.m_pool, toc_filename);
	if (!state.m_toc_dir)
		goto outofmemory;
	for (i = strlen(state.m_toc_dir) - 1; (i > 0) && !osd_is_path_separator(state.m_toc_dir[i]); i--)
		state.m_toc_dir[i] = '\0';

	/* clean the target directory */
	rmdir_recursive(dest_dir);
	osd_mkdir(dest_dir);

	/* create the help contents file */
	s = (char*)pool_malloc_lib(state.m_pool, strlen(dest_dir) + 1 + strlen(help_project_filename) + 1);
	if (!s)
		goto outofmemory;
	strcpy(s, dest_dir);
	strcat(s, PATH_SEPARATOR);
	strcat(s, help_contents_filename);
	state.m_chm_toc = fopen(s, "w");
	state.m_dest_dir = dest_dir;
	if (!state.m_chm_toc)
	{
		fprintf(stderr, "Cannot open file %s\n", s);
		goto error;
	}

	fprintf(state.m_chm_toc, "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//EN\"\n");
	fprintf(state.m_chm_toc, "<HTML>\n");
	fprintf(state.m_chm_toc, "<HEAD>\n");
	fprintf(state.m_chm_toc, "</HEAD>\n");
	fprintf(state.m_chm_toc, "<BODY>\n");
	fprintf(state.m_chm_toc, "<OBJECT type=\"text/site properties\">\n");
	fprintf(state.m_chm_toc, "\t<param name=\"Window Styles\" value=\"0x800625\">\n");
	fprintf(state.m_chm_toc, "\t<param name=\"ImageType\" value=\"Folder\">\n");
	fprintf(state.m_chm_toc, "\t<param name=\"Font\" value=\"Arial,8,0\">\n");
	fprintf(state.m_chm_toc, "</OBJECT>\n");
	fprintf(state.m_chm_toc, "<UL>\n");

	/* create the XML parser */
	memcallbacks.malloc_fcn = expat_malloc;
	memcallbacks.realloc_fcn = expat_realloc;
	memcallbacks.free_fcn = expat_free;
	state.m_parser = XML_ParserCreate_MM(NULL, &memcallbacks, NULL);
	if (!state.m_parser)
		goto outofmemory;

	XML_SetUserData(state.m_parser, &state);
	XML_SetElementHandler(state.m_parser, start_handler, end_handler);
	XML_SetCharacterDataHandler(state.m_parser, data_handler);

	do
	{
		len = (int) fread(buf, 1, sizeof(buf), in);
		done = feof(in);

		if (XML_Parse(state.m_parser, buf, len, done) == XML_STATUS_ERROR)
		{
			process_error(&state, NULL, NULL);
			break;
		}
	}
	while(!done);

	fprintf(state.m_chm_toc, "</UL>\n");
	fprintf(state.m_chm_toc, "</BODY></HTML>");
	fclose(state.m_chm_toc);

	/* create the help project file */
	s = (char*)pool_malloc_lib(state.m_pool, strlen(dest_dir) + 1 + strlen(help_project_filename) + 1);
	if (!s)
		goto outofmemory;
	strcpy(s, dest_dir);
	strcat(s, PATH_SEPARATOR);
	strcat(s, help_project_filename);
	chm_hhp = fopen(s, "w");
	if (!chm_hhp)
	{
		fprintf(stderr, "Cannot open file %s\n", s);
		goto error;
	}

	fprintf(chm_hhp, "[OPTIONS]\n");
	fprintf(chm_hhp, "Compiled file=%s\n", help_filename);
	fprintf(chm_hhp, "Contents file=%s\n", help_contents_filename);
	fprintf(chm_hhp, "Default topic=%s\n", state.m_default_topic);
	fprintf(chm_hhp, "Language=0x409 English (United States)\n");
	fprintf(chm_hhp, "Title=%s\n", state.m_title);
	fprintf(chm_hhp, "\n");
	fclose(chm_hhp);

	/* finish up */
	XML_ParserFree(state.m_parser);
	fclose(in);
	pool_free_lib(state.m_pool);
	return state.m_error ? -1 : 0;

outofmemory:
	fprintf(stderr, "Out of memory");
error:
	if (state.m_chm_toc) fclose(state.m_chm_toc);
	if (in)	fclose(in);
	return -1;
}



int CLIB_DECL main(int argc, char **argv)
{
	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s [tocxml] [destdir]\n", argv[0]);
		return -1;
	}

	return messdocs(argv[1], argv[2], "mess.hhp", "mess.hhc", "mess.chm");
}



void CLIB_DECL logerror(const char *text,...)
{
}



static void rtrim(char *buf)
{
	size_t buflen;
	char *s;

	buflen = strlen(buf);
	if (buflen)
	{
		for (s = &buf[buflen-1]; s >= buf && (*s >= '\0') && isspace(*s); s--)
			*s = '\0';
	}
}



/***************************************************************************
    EXPAT INTERFACES
***************************************************************************/

/*-------------------------------------------------
    expat_malloc/expat_realloc/expat_free -
    wrappers for memory allocation functions so
    that they pass through out memory tracking
    systems
-------------------------------------------------*/

static void *expat_malloc(size_t size)
{
	return malloc(size);
}

static void *expat_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

static void expat_free(void *ptr)
{
	free(ptr);
}
