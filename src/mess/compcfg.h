/****************************************************************************

	compcfg.h

****************************************************************************/

#ifndef COMPCFG_H
#define COMPCFG_H

/* RAM parsing options */
UINT32		ram_option(const game_driver *gamedrv, unsigned int i);
int			ram_option_count(const game_driver *gamedrv);
int			ram_is_valid_option(const game_driver *gamedrv, UINT32 ram);
UINT32		ram_default(const game_driver *gamedrv);
UINT32		ram_parse_string(const char *s);
const char *ram_string(char *buffer, UINT32 ram);

UINT8 *memory_install_ram8_handler(running_machine *machine, int cpunum, int spacenum, offs_t start, offs_t end, offs_t ram_offset, int bank);

#endif
