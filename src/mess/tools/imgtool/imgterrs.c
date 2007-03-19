/***************************************************************************

	imgterrs.c

	Imgtool errors

***************************************************************************/

#include <assert.h>
#include "imgterrs.h"

static const char *msgs[] =
{
	"Out of memory",
	"Unexpected error",
	"Argument too long",
	"Read error",
	"Write error",
	"Image is read only",
	"Corrupt image",
	"Corrupt file",
	"Corrupt directory",
	"File not found",
	"Unrecognized format",
	"Not implemented",
	"Parameter too small",
	"Parameter too large",
	"Missing parameter not found",
	"Inappropriate parameter",
	"Invalid parameter",
	"Bad file name",
	"Out of space on image",
	"Input past end of file",
	"Cannot specify path",
	"Invalid path",
	"Path not found",
	"Directory not empty",
	"Seek error",
	"File system does not support forks",
	"Fork not found",
	"Invalid partition"
};



const char *imgtool_error(imgtoolerr_t err)
{
	err = ERRORCODE(err) - 1;
	assert(err >= 0);
	assert(err < (sizeof(msgs) / sizeof(msgs[0])));
	return msgs[err];
}
