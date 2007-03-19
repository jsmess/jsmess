/*********************************************************************

	flopimg.h

	Floppy disk image abstraction code

*********************************************************************/

#ifndef FLOPIMG_H
#define FLOPIMG_H

#include "formats/ioprocs.h"
#include "opresolv.h"


/***************************************************************************

	Constants

***************************************************************************/

#define FLOPPY_FLAGS_READWRITE		0
#define FLOPPY_FLAGS_READONLY		1



/***************************************************************************

	Type definitions

***************************************************************************/

typedef enum
{
	FLOPPY_ERROR_SUCCESS,			/* no error */
	FLOPPY_ERROR_INTERNAL,			/* fatal internal error */
	FLOPPY_ERROR_UNSUPPORTED,		/* this operation is unsupported */
	FLOPPY_ERROR_OUTOFMEMORY,		/* ran out of memory */
	FLOPPY_ERROR_SEEKERROR,			/* attempted to seek to nonexistant location */
	FLOPPY_ERROR_INVALIDIMAGE,		/* this image in invalid */
	FLOPPY_ERROR_READONLY,			/* attempt to write to read-only image */
	FLOPPY_ERROR_NOSPACE,
	FLOPPY_ERROR_PARAMOUTOFRANGE,
	FLOPPY_ERROR_PARAMNOTSPECIFIED
}
floperr_t;

typedef struct _floppy_image floppy_image;

struct FloppyCallbacks
{
	floperr_t (*read_sector)(floppy_image *floppy, int head, int track, int sector, void *buffer, size_t buflen);
	floperr_t (*write_sector)(floppy_image *floppy, int head, int track, int sector, const void *buffer, size_t buflen);
	floperr_t (*read_indexed_sector)(floppy_image *floppy, int head, int track, int sector_index, void *buffer, size_t buflen);
	floperr_t (*write_indexed_sector)(floppy_image *floppy, int head, int track, int sector_index, const void *buffer, size_t buflen);
	floperr_t (*read_track)(floppy_image *floppy, int head, int track, UINT64 offset, void *buffer, size_t buflen);
	floperr_t (*write_track)(floppy_image *floppy, int head, int track, UINT64 offset, const void *buffer, size_t buflen);
	floperr_t (*format_track)(floppy_image *floppy, int head, int track, option_resolution *params);
	floperr_t (*post_format)(floppy_image *floppy, option_resolution *params);
	int (*get_heads_per_disk)(floppy_image *floppy);
	int (*get_tracks_per_disk)(floppy_image *floppy);
	int (*get_sectors_per_track)(floppy_image *floppy, int head, int track);
	UINT32 (*get_track_size)(floppy_image *floppy, int head, int track);
	floperr_t (*get_sector_length)(floppy_image *floppy, int head, int track, int sector, UINT32 *sector_length);
	floperr_t (*get_indexed_sector_info)(floppy_image *floppy, int head, int track, int sector_index, int *cylinder, int *side, int *sector, UINT32 *sector_length);
	floperr_t (*get_track_data_offset)(floppy_image *floppy, int head, int track, UINT64 *offset);
};



struct FloppyFormat
{
	const char *name;
	const char *extensions;
	const char *description;
	floperr_t (*identify)(floppy_image *floppy, const struct FloppyFormat *format, int *vote);
	floperr_t (*construct)(floppy_image *floppy, const struct FloppyFormat *format, option_resolution *params);
	const char *param_guidelines;
};



#define FLOPPY_OPTIONS_START(name)												\
	struct FloppyFormat floppyoptions_##name[] =								\
	{																			\

#define FLOPPY_OPTIONS_END														\
		{ NULL }																\
	};

#define FLOPPY_OPTIONS_EXTERN(name)												\
	extern struct FloppyFormat floppyoptions_##name[]							\

#define FLOPPY_OPTION(name, extensions_, description_, identify_, construct_, ranges_)\
	{ #name, extensions_, description_, identify_, construct_, ranges_ },				\


#define PARAM_END				'\0'
#define PARAM_HEADS				'H'
#define PARAM_TRACKS			'T'
#define PARAM_SECTORS			'S'
#define PARAM_SECTOR_LENGTH		'L'
#define PARAM_INTERLEAVE		'I'
#define PARAM_FIRST_SECTOR_ID	'F'

#define HEADS(range)			"H" #range
#define TRACKS(range)			"T" #range
#define SECTORS(range)			"S" #range
#define SECTOR_LENGTH(range)	"L" #range
#define INTERLEAVE(range)		"I" #range
#define FIRST_SECTOR_ID(range)	"F" #range

#define FLOPPY_IDENTIFY(name)	floperr_t name(floppy_image *floppy, const struct FloppyFormat *format, int *vote)
#define FLOPPY_CONSTRUCT(name)	floperr_t name(floppy_image *floppy, const struct FloppyFormat *format, option_resolution *params)

/***************************************************************************

	Prototypes

***************************************************************************/

OPTION_GUIDE_EXTERN(floppy_option_guide);

/* opening, closing and creating of floppy images */
floperr_t floppy_open(void *fp, const struct io_procs *procs, const char *extension, const struct FloppyFormat *format, int flags, floppy_image **outfloppy);
floperr_t floppy_open_choices(void *fp, const struct io_procs *procs, const char *extension, const struct FloppyFormat *formats, int flags, floppy_image **outfloppy);
floperr_t floppy_create(void *fp, const struct io_procs *procs, const struct FloppyFormat *format, option_resolution *parameters, floppy_image **outfloppy);
void floppy_close(floppy_image *floppy);

/* useful for identifying a floppy image */
floperr_t floppy_identify(void *fp, const struct io_procs *procs, const char *extension,
	const struct FloppyFormat *formats, int *identified_format);

/* functions useful within format constructors */
void *floppy_tag(floppy_image *floppy, const char *tagname);
void *floppy_create_tag(floppy_image *floppy, const char *tagname, size_t tagsize);
struct FloppyCallbacks *floppy_callbacks(floppy_image *floppy);
UINT8 floppy_get_filler(floppy_image *floppy);
void floppy_set_filler(floppy_image *floppy, UINT8 filler);

/* calls for accessing disk image data */
floperr_t floppy_read_sector(floppy_image *floppy, int head, int track, int sector, int offset, void *buffer, size_t buffer_len);
floperr_t floppy_write_sector(floppy_image *floppy, int head, int track, int sector, int offset, const void *buffer, size_t buffer_len);
floperr_t floppy_read_indexed_sector(floppy_image *floppy, int head, int track, int sector_index, int offset, void *buffer, size_t buffer_len);
floperr_t floppy_write_indexed_sector(floppy_image *floppy, int head, int track, int sector_index, int offset, const void *buffer, size_t buffer_len);
floperr_t floppy_clear_sector(floppy_image *floppy, int head, int track, int sector, UINT8 data);
floperr_t floppy_read_track(floppy_image *floppy, int head, int track, void *buffer, size_t buffer_len);
floperr_t floppy_write_track(floppy_image *floppy, int head, int track, const void *buffer, size_t buffer_len);
floperr_t floppy_read_track_data(floppy_image *floppy, int head, int track, void *buffer, size_t buffer_len);
floperr_t floppy_write_track_data(floppy_image *floppy, int head, int track, const void *buffer, size_t buffer_len);
floperr_t floppy_format_track(floppy_image *floppy, int head, int track, option_resolution *params);
int floppy_get_tracks_per_disk(floppy_image *floppy);
int floppy_get_heads_per_disk(floppy_image *floppy);
UINT32 floppy_get_track_size(floppy_image *floppy, int head, int track);
floperr_t floppy_get_sector_length(floppy_image *floppy, int head, int track, int sector, UINT32 *sector_length);
floperr_t floppy_get_indexed_sector_info(floppy_image *floppy, int head, int track, int sector_index, int *cylinder, int *side, int *sector, UINT32 *sector_length);
floperr_t floppy_get_sector_count(floppy_image *floppy, int head, int track, int *sector_count);
floperr_t floppy_load_track(floppy_image *floppy, int head, int track, int dirtify, void **track_data, size_t *track_length);
int floppy_is_read_only(floppy_image *floppy);

/* accessors for meta information about the image */
const char *floppy_format_description(floppy_image *floppy);

/* calls for accessing the raw disk image */
void floppy_image_read(floppy_image *floppy, void *buffer, UINT64 offset, size_t length);
void floppy_image_write(floppy_image *floppy, const void *buffer, UINT64 offset, size_t length);
void floppy_image_write_filler(floppy_image *floppy, UINT8 filler, UINT64 offset, size_t length);
UINT64 floppy_image_size(floppy_image *floppy);

/* misc */
const char *floppy_error(floperr_t err);

/* debugging calls */
floperr_t floppy_test_format(const struct FloppyFormat *format, int (*printerror)(const char *format, ...));



#endif /* FLOPIMG_H */

