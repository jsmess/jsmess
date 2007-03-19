/***************************************************************************

	imgtoolx.h

	Internal headers for Imgtool core

***************************************************************************/

#ifndef IMGTOOLX_H
#define IMGTOOLX_H

#include "imgtool.h"

/* ----------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Wave/Cassette calls
 * ---------------------------------------------------------------------------
 */

enum {
	WAVEIMAGE_LSB_FIRST = 0,
	WAVEIMAGE_MSB_FIRST = 1
};

struct WaveExtra
{
	int (*initalt)(imgtool_stream *instream, imgtool_stream **outstream, int *basepos, int *length, int *channels, int *frequency, int *resolution);
	int (*nextfile)(imgtool_image *img, imgtool_dirent *ent);
	int (*readfile)(imgtool_image *img, imgtool_stream *destf);
	int zeropulse;
	int threshpulse;
	int onepulse;
	int waveflags;
	const UINT8 *blockheader;
	int blockheadersize;

};

#define WAVEMODULE(name_,humanname_,ext_,eoln_,flags_,zeropulse,onepulse,threshpulse,waveflags,blockheader,blockheadersize,\
		initalt,nextfile,readfilechunk)	\
	static struct WaveExtra waveextra_##name =								\
	{																		\
		(initalt),															\
		(nextfile),															\
		(readfilechunk),													\
		(zeropulse),														\
		(onepulse),															\
		(threshpulse),														\
		(waveflags),														\
		(blockheader),														\
		(blockheadersize),													\
	};																		\
																			\
	int construct_imgmod_##name_(struct ImageModuleCtorParams *params)		\
	{																		\
		imgtool_module *imgmod = params->imgmod;						\
		memset(imgmod, 0, sizeof(*imgmod));									\
		imgmod->name = #name_;												\
		imgmod->humanname = (humanname_);									\
		imgmod->fileextension = (ext_);										\
		imgmod->eoln = (eoln_);												\
		imgmod->flags = (flags_);											\
		imgmod->init = imgwave_init;										\
		imgmod->exit = imgwave_exit;										\
		imgmod->begin_enum = imgwave_beginenum;								\
		imgmod->next_enum = imgwave_nextenum;								\
		imgmod->close_enum = imgwave_closeenum;								\
		imgmod->read_file = imgwave_readfile;								\
		imgmod->extra = (void *) &waveextra_##name;							\
		return 1;															\
	}

/* These are called internally */
int imgwave_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg);
void imgwave_exit(imgtool_image *img);
int imgwave_beginenum(imgtool_image *img, imgtool_directory **outenum);
int imgwave_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent);
void imgwave_closeenum(imgtool_directory *enumeration);
int imgwave_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf);

/* These are callable from wave modules */
int imgwave_seek(imgtool_image *img, int pos);
int imgwave_forward(imgtool_image *img);
int imgwave_read(imgtool_image *img, UINT8 *buf, int bufsize);


#endif /* IMGTOOLX_H */

