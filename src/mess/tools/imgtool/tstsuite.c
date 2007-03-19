#include <ctype.h>
#include "imgtool.h"
#include "main.h"
#include "utils.h"
#include "snprintf.h"

#ifdef MAME_DEBUG

int cmd_testsuite(struct command *c, int argc, char *argv[])
{
	const char *testsuitefile;
	FILE *inifile;
	const imgtool_module *module = NULL;
	char buffer[1024];
	char buffer2[1024];
	char filename[1024];
	char *filename_base;
	size_t filename_len;
	char *s;
	char *s2;
	const char *directive;
	const char *directive_value;
	imgtool_image *img = NULL;
	imgtool_directory *imgenum;
	imgtool_dirent imgdirent;
	int err = -1, i;

	testsuitefile = argv[0];

	/* open the testsuite file */
	inifile = fopen(testsuitefile, "r");
	if (!inifile)
	{
		fprintf(stderr, "*** cannot open testsuite file '%s'\n", testsuitefile);
		goto error;
	}

	/* change to the current directory of the test suite */
	strncpyz(filename, testsuitefile, sizeof(filename));
	s = filename + strlen(filename) - 1;
	while((*s != '/') && (*s != '\\'))
		*(s--) = '\0';
	filename_base = s+1;
	filename_len = filename_base - filename;

	while(!feof(inifile))
	{
		buffer[0] = '\0';
		fgets(buffer, sizeof(buffer) / sizeof(buffer[0]), inifile);

		if (buffer[0] != '\0')
		{
			s = buffer + strlen(buffer) -1;
			while(isspace(*s))
				*(s--) = '\0';
		}
		s = buffer;
		while(isspace(*s))
			s++;

		if (*s == '[')
		{
			s2 = strchr(s, ']');
			if (!s2)
				goto syntaxerror;
			*s2 = '\0';
			module = findimagemodule(s+1);
			if (!module)
			{
				fprintf(stderr, "*** unrecognized imagemodule '%s'\n", s+1);
				goto error;
			}
			imgtool_test(module);
		}
		else if (isalpha(*s))
		{
			directive = s;
			while(isalpha(*s))
				s++;
			while(isspace(*s))
				*(s++) = '\0';

			if (*s != '=')
			{
				fprintf(stderr, "*** expected '='\n");
				goto error;
			}
			*(s++) = '\0';
			while(isspace(*s))
				s++;
			directive_value = s;

			if (!mame_stricmp(directive, "imagefile"))
			{
				/* imagefile directive */
				if (img)
					imgtool_image_close(img);
				strncpyz(filename_base, directive_value, filename_len);
				err = imgtool_image_open(module, filename, OSD_FOPEN_READ, &img);
				if (err)
					goto error;

				fprintf(stdout, "%s\n", filename);
			}
			else if (!mame_stricmp(directive, "files"))
			{
				/* files directive */
				if (!img)
					goto needimgmodule;
				err = imgtool_directory_open(img, &imgenum);
				if (err)
					goto error;

				memset(&imgdirent, 0, sizeof(imgdirent));
				imgdirent.fname = buffer2;
				imgdirent.fname_len = sizeof(buffer2);
				while(((err = imgtool_directory_get_next(imgenum, &imgdirent)) == 0) && imgdirent.fname[0])
				{
					i = strlen(buffer2);
					buffer2[i++] = ',';
					imgdirent.fname = &buffer2[i];
					imgdirent.fname_len = sizeof(buffer2) - i;
				}
				imgtool_directory_close(imgenum);
				if (err)
					goto error;
				i = strlen(buffer2);
				if (i > 0)
					buffer2[i-1] = '\0';

				if (strcmp(directive_value, buffer2))
				{
					fprintf(stderr, "*** expected files '%s', but instead got '%s'", directive_value, buffer2);
					goto error;
				}
			}
			else
			{
				fprintf(stderr, "*** unrecognized directive '%s'\n", directive);
				goto done;
			}
		}
	}
	err = 0;
	goto done;

needimgmodule:
	fprintf(stderr, "*** need [format] declaration before any directives\n");
	goto error;

syntaxerror:
	fprintf(stderr, "*** syntax error: %s\n", buffer);
	goto error;

error:
	if (err && (err != -1))
		reporterror(err, c, module ? module->name : NULL, filename, NULL, NULL, NULL);

done:
	if (inifile)
		fclose(inifile);
	if (img)
		imgtool_image_close(img);
	return err;
}

#endif /* MAME_DEBUG */
