/*********************************************************************

	mess.h

	Core MESS headers

*********************************************************************/

#ifndef MESS_H
#define MESS_H

#include <stdarg.h>

struct SystemConfigurationParamBlock;

#include "device.h"
#include "image.h"
#include "artworkx.h"
#include "memory.h"



/***************************************************************************

	Constants

***************************************************************************/

#define LCD_FRAMES_PER_SECOND	30



/**************************************************************************/

/* MESS_DEBUG is a debug switch (for developers only) for
   debug code, which should not be found in distributions, like testdrivers,...
   contrary to MAME_DEBUG, NDEBUG it should not be found in the makefiles of distributions
   use it in your private root makefile */
/* #define MESS_DEBUG */

/* Win32 defines this for vararg functions */
#ifndef DECL_SPEC
#define DECL_SPEC
#endif

/***************************************************************************/

void mess_options_init(core_options *opts);

extern char mess_disclaimer[];

UINT32 hash_data_extract_crc32(const char *d);



/***************************************************************************/

#if HAS_WAVE
int tapecontrol(int selected);
void tapecontrol_gettime(char *timepos, size_t timepos_size, mess_image *img, int *curpos, int *endpos);
#endif

/* IODevice Initialisation return values.  Use these to determine if */
/* the emulation can continue if IODevice initialisation fails */
#define INIT_PASS 0
#define INIT_FAIL 1
#define IMAGE_VERIFY_PASS 0
#define IMAGE_VERIFY_FAIL 1

/* runs checks to see if device code is proper */
int mess_validitychecks(void);

/* these are called from mame.c */
void devices_init(running_machine *machine);

void mess_config_init(running_machine *machine);

enum
{
	OSD_FOPEN_READ,
	OSD_FOPEN_WRITE,
	OSD_FOPEN_RW,
	OSD_FOPEN_RW_CREATE
};

/* --------------------------------------------------------------------------------------------- */

/* This call is used to return the next compatible driver with respect to
 * software images.  It is usable both internally and from front ends
 */
const game_driver *mess_next_compatible_driver(const game_driver *drv);
int mess_count_compatible_drivers(const game_driver *drv);

/* --------------------------------------------------------------------------------------------- */

/* RAM configuration calls */
extern UINT32 mess_ram_size;
extern UINT8 *mess_ram;
extern UINT8 mess_ram_default_value;

/* RAM parsing options */
#define RAM_STRING_BUFLEN 16
UINT32		ram_option(const game_driver *gamedrv, unsigned int i);
int			ram_option_count(const game_driver *gamedrv);
int			ram_is_valid_option(const game_driver *gamedrv, UINT32 ram);
UINT32		ram_default(const game_driver *gamedrv);
UINT32		ram_parse_string(const char *s);
const char *ram_string(char *buffer, UINT32 ram);
int			ram_validate_option(void);
void		ram_dump(const char *filename);

UINT8 *memory_install_ram8_handler(int cpunum, int spacenum, offs_t start, offs_t end, offs_t ram_offset, int bank);

/* --------------------------------------------------------------------------------------------- */

/* dummy read handlers */
READ8_HANDLER(return8_FE);
READ8_HANDLER(return8_FF);
READ16_HANDLER(return16_FFFF);

#endif
