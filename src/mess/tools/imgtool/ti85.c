#include "osdepend.h"
#include "imgtoolx.h"
#include "utils.h"
#include "formats/ti85_ser.h"

static const char *var_type[] = {
		"Real number",			//0x00
		"Complex number",		//0x01
		"Real vector",			//0x02
		"Complex vector",		//0x03
		"Real list",			//0x04
		"Complex list",			//0x05
		"Real matrix",			//0x06
		"Complex matrix",		//0x07
		"Real constatnt",		//0x08
		"Complex constant",		//0x09
		"Equation",			//0x0a
		"Range settings",		//0x0b
		"String",			//0x0c
		"Graphics database",		//0x0d
		"Graphics database",		//0x0e
		"Graphics database",		//0x0f
		"Graphics database",		//0x10
		"Picture",			//0x11
		"Program",			//0x12
		"Range settings",		//0x13
		"Unknown",			//0x14
		"Range settings",		//0x15
		"Range settings",		//0x16
		"Range settings",		//0x17
		"Range settings",		//0x18
		"Range settings",		//0x19
		"Range settings",		//0x1a
		"Range settings"};		//0x1b

typedef struct {
	char signature[0x08];
	unsigned char marker[0x03];
	char comment[0x2a];
	unsigned char data_size[0x02];
} ti85_header;

typedef struct {
	imgtool_image base;
	imgtool_stream *file_handle;
	unsigned int size;
	int modified;
	UINT8 *data;
	ti85_entry * entries;
	int number_of_entries;
} ti85_file;

typedef struct {
	imgtool_image base;
	imgtool_stream *file_handle;
	unsigned int size;
	int modified;
	UINT8 *data;
} ti85b_file;

typedef struct {
	imgtool_directory base;
	ti85_file *image;
	int index;
} ti85_iterator;

typedef struct {
	imgtool_directory base;
	ti85b_file *image;
	int index;
} ti85b_iterator;

#define HEADER(file) ((ti85_header*)file->data)

static struct OptionTemplate ti85_fileoptions[] =
{
	{ "ftype", "File type", IMGOPTION_FLAG_TYPE_INTEGER | IMGOPTION_FLAG_HASDEFAULT,0,27,"12"},
	{ NULL, 0, 0, 0, 0 }
};

static struct OptionTemplate ti85_createoptions[] =
{
	{ "comment", "Comment", IMGOPTION_FLAG_TYPE_STRING | IMGOPTION_FLAG_HASDEFAULT,0,0,"TI-85 file created with ImgTool"},
	{ NULL, NULL, 0, 0, 0, 0 }
};

#define TI85_OPTION_FTYPE	0
#define TI85_OPTION_COMMENT	0

static void ti85_file_info(imgtool_image *img, char *string, const int len);
static int ti85_file_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg);
static void ti85_file_exit(imgtool_image *img);
static int ti85_file_beginenum(imgtool_image *img, imgtool_directory **outenum);
static int ti85_file_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent);
static void ti85_file_closeenum(imgtool_directory *enumeration);
static int ti85_file_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf);
static int ti85_file_writefile(imgtool_image *img, const char *fname, imgtool_stream *sourcef, const ResolvedOption *options_);
static int ti85_file_deletefile(imgtool_image *img, const char *fname);
static int ti85_file_create(const imgtool_module *mod, imgtool_stream *f, const ResolvedOption *options_);

static void ti85b_file_info(imgtool_image *img, char *string, const int len);
static int ti85b_file_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg);
static void ti85b_file_exit(imgtool_image *img);
static int ti85b_file_beginenum(imgtool_image *img, imgtool_directory **outenum);
static int ti85b_file_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent);
static void ti85b_file_closeenum(imgtool_directory *enumeration);


#define TI85FILE(name,human_name,extension) \
IMAGEMODULE(			\
	name,			\
	human_name,		\
	extension,		\
	NULL,			\
	NULL,			\
	NULL,			\
	0,			\
	ti85_file_init,		\
	ti85_file_exit,		\
	ti85_file_info,		\
	ti85_file_beginenum,	\
	ti85_file_nextenum,	\
	ti85_file_closeenum,	\
	NULL,			\
	ti85_file_readfile,	\
	ti85_file_writefile,	\
	ti85_file_deletefile,	\
	ti85_file_create,	\
	NULL,			\
	NULL,			\
	ti85_fileoptions,	\
	ti85_createoptions	\
)

TI85FILE (ti85p, "TI-85 program file", "85p")
TI85FILE (ti85s, "TI-85 string file (also ZShell programs)", "85s")
TI85FILE (ti85i, "TI-85 picture file (85-image)", "85i")
TI85FILE (ti85n, "TI-85 real number file", "85n")
TI85FILE (ti85c, "TI-85 complex number file", "85c")
TI85FILE (ti85l, "TI-85 list (real or complex) file", "85l")
TI85FILE (ti85k, "TI-85 constant file", "85k")
TI85FILE (ti85m, "TI-85 matrix (real or complex) file", "85m")
TI85FILE (ti85v, "TI-85 vector (real or complex) file", "85v")
TI85FILE (ti85d, "TI-85 graphics database file", "85d")
TI85FILE (ti85e, "TI-85 equation file", "85e")
TI85FILE (ti85r, "TI-85 range settings file", "85r")
TI85FILE (ti85g, "TI-85 grouped file", "85g")
TI85FILE (ti85, "TI-85 file", "85?")
TI85FILE (ti86p, "TI-86 program file", "86p")
TI85FILE (ti86s, "TI-86 string file (also ZShell programs)", "86s")
TI85FILE (ti86i, "TI-86 picture file (85-image)", "86i")
TI85FILE (ti86n, "TI-86 real number file", "86n")
TI85FILE (ti86c, "TI-86 complex number file", "86c")
TI85FILE (ti86l, "TI-86 list (real or complex) file", "86l")
TI85FILE (ti86k, "TI-86 constant file", "86k")
TI85FILE (ti86m, "TI-86 matrix (real or complex) file", "86m")
TI85FILE (ti86v, "TI-86 vector (real or complex) file", "86v")
TI85FILE (ti86d, "TI-86 graphics database file", "86d")
TI85FILE (ti86e, "TI-86 equation file", "86e")
TI85FILE (ti86r, "TI-86 range settings file", "86r")
TI85FILE (ti86g, "TI-86 grouped file", "86g")
TI85FILE (ti86, "TI-86 file", "86?")


IMAGEMODULE(
	ti85b,
	"TI-85 memory backup file",
	"85b",
	NULL,
	NULL,
	NULL,
	0,
	ti85b_file_init,
	ti85b_file_exit,
	ti85b_file_info,
	ti85b_file_beginenum,
	ti85b_file_nextenum,
	ti85b_file_closeenum,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
)

static int ti85_file_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg)
{
	ti85_file *file;

	file=*(ti85_file**)outimg=(ti85_file *) malloc(sizeof(ti85_file));
	if (!file) return IMGTOOLERR_OUTOFMEMORY;

	file->base.module = mod;
	file->size=stream_size(f);
	file->file_handle=f;

	file->data = (unsigned char *) malloc(file->size);
	if (!file->data) return IMGTOOLERR_OUTOFMEMORY;

	if (stream_read(f, file->data, file->size)!=file->size)
	{
		free(file);
		*outimg=NULL;
		return IMGTOOLERR_READERROR;
	}

	if ( strncmp((char *) file->data, (char *) ti85_file_signature, 11) &&
             strncmp((char *) file->data, (char *) ti86_file_signature, 11) )
	{
		free(file);
		return IMGTOOLERR_CORRUPTIMAGE;
	}


	if (file->data[0x3b]==0x1d)
	{
		free(file);
		return IMGTOOLERR_CORRUPTIMAGE;
	}

	file->number_of_entries = ti85_variables_count(file->data, file->size);


	file->entries = (ti85_entry*) malloc (file->number_of_entries*sizeof(ti85_entry));
	if (!file->entries) return IMGTOOLERR_OUTOFMEMORY;


	ti85_variables_read(file->data, file->size, file->entries);

	return 0;
}

static void ti85_file_exit(imgtool_image * img)
{
	ti85_file *file=(ti85_file*)img;
	if (file->modified) {
		stream_clear(file->file_handle);
		stream_write(file->file_handle, file->data, file->size);
	}
	stream_close(file->file_handle);
	free(file->entries);
	free(file->data);
	free(file);
}

static void ti85_file_info(imgtool_image *img, char *string, const int len)
{
	UINT16 file_checksum;
	UINT16 checksum;
	unsigned int data_size;

	ti85_file *file=(ti85_file*)img;
	char comment_with_null[0x2a+1]={ 0 };

	strncpy(comment_with_null, HEADER(file)->comment, 0x2a);

	data_size = HEADER(file)->data_size[1]*256 + HEADER(file)->data_size[0];
	file_checksum = file->data[file->size-1]*256+file->data[file->size-2];
	checksum = ti85_calculate_checksum(file->data+TI85_HEADER_SIZE, file->size-2-TI85_HEADER_SIZE);

	sprintf(string,"\nComment   : %s\nData size : %04xh\nEntries   : %d\nChecksum  : %04xh",
			comment_with_null,
			data_size,
			file->number_of_entries,
			file_checksum);
	if (checksum!=file_checksum) strcat (string, " (BAD)");
}

static int ti85_file_beginenum(imgtool_image *img, imgtool_directory **outenum)
{
	ti85_file *file=(ti85_file*)img;
	ti85_iterator *iter;

	iter=*(ti85_iterator**)outenum = (ti85_iterator *) malloc(sizeof(ti85_iterator));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;

	iter->image=file;
	iter->index = 0;
	return 0;
}

static int ti85_file_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent)
{
	ti85_iterator *iter=(ti85_iterator*)enumeration;

	ent->eof=iter->index>=iter->image->number_of_entries;
	if (!ent->eof)
	{
		memset(ent->fname,0,iter->image->entries[iter->index].name_size+1);
		strncpy (ent->fname,
			 (char*)iter->image->data+iter->image->entries[iter->index].offset+0x06,
			 iter->image->entries[iter->index].name_size);
		if (ent->attr)
			sprintf(ent->attr,"%s (%02xh)",
				var_type[iter->image->entries[iter->index].type],
				iter->image->entries[iter->index].type);
		ent->filesize = iter->image->entries[iter->index].data_size;
		ent->corrupt=0;
		iter->index++;
	}

	return 0;
}

static void ti85_file_closeenum(imgtool_directory *enumeration)
{
	free(enumeration);
}

static int ti85_findfile (ti85_file * file, const char * fname)
{
	int i;

	for (i = 0; i < file->number_of_entries; i++)
		if (!strnicmp(fname, (char *) file->data+file->entries[i].offset+0x06, file->entries[i].name_size) )
			return i;		

	return -1;
}


static int ti85_file_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf)
{
	ti85_file *file=(ti85_file*)img;
	int start, size;
	int ind;

	if ((ind=ti85_findfile(file, fname))==-1) return IMGTOOLERR_MODULENOTFOUND;

	size = file->entries[ind].data_size;
	start = file->entries[ind].offset+file->entries[ind].head_size+0x04;

	if (stream_write(destf, file->data+start, size) != size)
        		return IMGTOOLERR_WRITEERROR;
	return 0;
}

static int ti85_file_writefile(imgtool_image *img, const char *fname, imgtool_stream *sourcef, const ResolvedOption *options_)
{
	ti85_file *file=(ti85_file*)img;
	int ind;

	unsigned char name_size = strlen (fname);
	UINT16 head_size = 0x04 + name_size;
	UINT16 data_size = stream_size(sourcef);
	unsigned char type = options_[TI85_OPTION_FTYPE].i;
	unsigned int offset = file->size-2;

	if (!(file->data=realloc(file->data, file->size+data_size+head_size+4)) )
		return IMGTOOLERR_OUTOFMEMORY;

	file->size+=data_size+head_size+4;

	if (!(file->entries=realloc(file->entries, sizeof(ti85_entry)*(file->number_of_entries+1))) )
		return IMGTOOLERR_OUTOFMEMORY;
	file->number_of_entries++;

	ind = file->number_of_entries-1;

	file->entries[ind].head_size = head_size;
	file->entries[ind].data_size = data_size;
	file->entries[ind].type = type;
	file->entries[ind].name_size = name_size;
	file->entries[ind].offset = offset;

	HEADER(file)->data_size[0] = (file->size-0x36)&0x00ff;
	HEADER(file)->data_size[1] = ((file->size-0x36)&0xff00)>>8;

	file->data[offset] = head_size&0x00ff;
	file->data[offset+0x01] = (head_size&0xff00)>>8;
	file->data[offset+0x02] = data_size&0x00ff;
	file->data[offset+0x03] = (data_size&0xff00)>>8;
	file->data[offset+0x04] = type;
	file->data[offset+0x05] = name_size;

	strncpy ((char*) file->data+offset+0x06, fname, name_size);

	file->data[offset+head_size+0x02] = data_size%0x00ff;
	file->data[offset+head_size+0x03] = (data_size%0xff00)>>8;

	if (stream_read(sourcef, file->data+offset+head_size+0x04, data_size)!=data_size)
		return IMGTOOLERR_READERROR;
	
	file->data[file->size-2] = ti85_calculate_checksum(file->data+TI85_HEADER_SIZE, file->size-2-TI85_HEADER_SIZE)&0x00ff;
	file->data[file->size-1] = (ti85_calculate_checksum(file->data+TI85_HEADER_SIZE, file->size-2-TI85_HEADER_SIZE)&0xff00)>>8;

	file->modified=1;

	return 0;
}

static int ti85_file_deletefile(imgtool_image *img, const char *fname)
{
	ti85_file *file=(ti85_file*)img;
	int ind,i;
	int size, start;

	if ((ind=ti85_findfile(file, fname))==-1 )
		return IMGTOOLERR_MODULENOTFOUND;

	size = file->entries[ind].data_size+file->entries[ind].head_size+0x04;
	start = file->entries[ind].offset;

	memmove(file->data+start, file->data+start+size, file->size-start-size);
	memmove(file->entries+ind*sizeof(ti85_entry),
		file->entries+(ind+1)*sizeof(ti85_entry),
		(file->number_of_entries-ind)*sizeof(ti85_entry));

	for (i = ind; i < file->number_of_entries; i++)
		file->entries[i].offset-=size;

	file->size-=size;
	file->data[file->size-2] = ti85_calculate_checksum(file->data+TI85_HEADER_SIZE, file->size-2-TI85_HEADER_SIZE)&0x00ff;
	file->data[file->size-1] = (ti85_calculate_checksum(file->data+TI85_HEADER_SIZE, file->size-2-TI85_HEADER_SIZE)&0xff00)>>8;
	file->modified=1;

	return 0;
}

static int ti85_file_create(const imgtool_module *mod, imgtool_stream *f, const ResolvedOption *options_)
{

	ti85_header header = {	{'*','*','T','I','8','5','*','*'},
				{0x1a, 0x0c, 0x00},
				"",
				{0x00, 0x00} };

	char checksum[] = {0x00, 0x00};

	strncpy(header.comment, options_[TI85_OPTION_COMMENT].s,0x2a);
	if (stream_write(f, &header, sizeof(ti85_header)) != sizeof(ti85_header)) 
		return  IMGTOOLERR_WRITEERROR;
	if (stream_write(f, &checksum, 2) != 2) 
		return  IMGTOOLERR_WRITEERROR;

	return 0;
}

static int ti85b_file_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg)
{
	ti85b_file *file;

	file=*(ti85b_file**)outimg=(ti85b_file *) malloc(sizeof(ti85b_file));
	if (!file) return IMGTOOLERR_OUTOFMEMORY;

	file->base.module = mod;
	file->size=stream_size(f);
	file->file_handle=f;

	file->data = (unsigned char *) malloc(file->size);
	if (!file->data) return IMGTOOLERR_OUTOFMEMORY;

	if (stream_read(f, file->data, file->size)!=file->size)
	{
		free(file);
		*outimg=NULL;
		return IMGTOOLERR_READERROR;
	}

	if ( strncmp((char *) file->data, (char *) ti85_file_signature, 11))
	{
		free(file);
		return IMGTOOLERR_CORRUPTIMAGE;
	}

	if (file->data[0x3b]!=0x1d)
	{
		free(file);
		return IMGTOOLERR_CORRUPTIMAGE;
	}

	return 0;
}

static void ti85b_file_exit(imgtool_image * img)
{
	ti85b_file *file=(ti85b_file*)img;
	stream_close(file->file_handle);
	free(file->data);
	free(file);
}

static void ti85b_file_info(imgtool_image *img, char *string, const int len)
{
	UINT16 file_checksum;
	UINT16 checksum;
	unsigned int data_size;

	ti85b_file *file=(ti85b_file*)img;
	char comment_with_null[0x2a+1]={ 0 };

	strncpy(comment_with_null, HEADER(file)->comment, 0x2a);

	data_size = HEADER(file)->data_size[1]*256 + HEADER(file)->data_size[0] - 17;
	file_checksum = file->data[file->size-1]*256+file->data[file->size-2];
	checksum = ti85_calculate_checksum(file->data+TI85_HEADER_SIZE, file->size-2-TI85_HEADER_SIZE);

	sprintf(string,"\nComment   : %s\nData size : %04xh\nChecksum  : %04xh",
			comment_with_null,
			data_size,
			file_checksum);
	if (checksum!=file_checksum) strcat (string, " (BAD)");
}

static int ti85b_file_beginenum(imgtool_image *img, imgtool_directory **outenum)
{
	ti85b_file *file=(ti85b_file*)img;
	ti85b_iterator *iter;

	iter=*(ti85b_iterator**)outenum = (ti85b_iterator *) malloc(sizeof(ti85b_iterator));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;

	iter->image=file;
	iter->index = 0;
	return 0;
}

static int ti85b_file_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent)
{
	ti85b_iterator *iter=(ti85b_iterator*)enumeration;

	ent->eof=iter->index>=3;
	if (!ent->eof)
	{
		switch (iter->index) {
			case 0 :
				strcpy (ent->fname, "TI-85 Memory");
				ent->filesize = iter->image->data[0x3a]*256 +iter->image->data[0x39];
				break;
			case 1 :
				strcpy (ent->fname, "User Memory");
				ent->filesize = iter->image->data[0x3d]*256 +iter->image->data[0x3c];
				break;
			case 2 :
				strcpy (ent->fname, "VAT");
				ent->filesize = iter->image->data[0x3f]*256 +iter->image->data[0x3e];
				break;
		}

		
		ent->corrupt=0;
		iter->index++;
	}

	return 0;
}

static void ti85b_file_closeenum(imgtool_directory *enumeration)
{
	free(enumeration);
}
