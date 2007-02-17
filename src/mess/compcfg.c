#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include "mess.h"

#define MAX_RAM_OPTIONS	16

static int get_ram_options(const game_driver *gamedrv, UINT32 *ram_options, int max_options, int *default_option)
{
	struct SystemConfigurationParamBlock params;

	memset(&params, 0, sizeof(params));
	params.max_ram_options = max_options;
	params.ram_options = ram_options;

	if (gamedrv->sysconfig_ctor)
		gamedrv->sysconfig_ctor(&params);

	if (default_option)
		*default_option = params.default_ram_option;
	return params.actual_ram_options;
}



UINT32 ram_option(const game_driver *gamedrv, unsigned int i)
{
	UINT32 ram_options[MAX_RAM_OPTIONS];
	int ram_optcount;

	ram_optcount = get_ram_options(gamedrv, ram_options, sizeof(ram_options) / sizeof(ram_options[0]), NULL);
	return i >= ram_optcount ? 0 : ram_options[i];
}



UINT32 ram_default(const game_driver *gamedrv)
{
	UINT32 ram_options[MAX_RAM_OPTIONS];
	int ram_optcount;
	int default_ram_option;

	ram_optcount = get_ram_options(gamedrv, ram_options, sizeof(ram_options) / sizeof(ram_options[0]), &default_ram_option);
	return default_ram_option >= ram_optcount ? 0 : ram_options[default_ram_option];
}



int ram_option_count(const game_driver *gamedrv)
{
	UINT32 ram_options[MAX_RAM_OPTIONS];
	return get_ram_options(gamedrv, ram_options, sizeof(ram_options) / sizeof(ram_options[0]), NULL);
}



int ram_is_valid_option(const game_driver *gamedrv, UINT32 ram)
{
	UINT32 ram_options[MAX_RAM_OPTIONS];
	int ram_optcount;
	int i;

	ram_optcount = get_ram_options(gamedrv, ram_options, sizeof(ram_options) / sizeof(ram_options[0]), NULL);

	for(i = 0; i < ram_optcount; i++)
		if (ram_options[i] == ram)
			return 1;
	return 0;
}



UINT32 ram_parse_string(const char *s)
{
	UINT32 ram;
	char suffix = '\0';

	s += sscanf(s, "%u%c", &ram, &suffix);
	switch(tolower(suffix)) {
	case 'k':
		/* kilobytes */
		ram *= 1024;
		break;

	case 'm':
		/* megabytes */
		ram *= 1024*1024;
		break;

	case '\0':
		/* no suffix */
		break;

	default:
		/* parse failure */
		ram = 0;
		break;
	}
	return ram;
}



const char *ram_string(char *buffer, UINT32 ram)
{
	const char *suffix;

	if ((ram % (1024*1024)) == 0)
	{
		ram /= 1024*1024;
		suffix = "m";
	}
	else if ((ram % 1024) == 0)
	{
		ram /= 1024;
		suffix = "k";
	}
	else
	{
		suffix = "";
	}
	sprintf(buffer, "%u%s", ram, suffix);
	return buffer;
}



/* ----------------------------------------------------------------------- */

UINT8 *memory_install_ram8_handler(int cpunum, int spacenum, offs_t start, offs_t end, offs_t ram_offset, int bank)
{
	read8_handler read_bank = (read8_handler) (STATIC_BANK1 + bank - 1);
	write8_handler write_bank = (write8_handler) (STATIC_BANK1 + bank - 1);
	offs_t bank_size = end - start + 1;

	memory_set_bankptr(bank, mess_ram + ram_offset);

	memory_install_read8_handler(cpunum, spacenum, start,
		MIN(end, start - ram_offset + mess_ram_size - 1), 0, 0, read_bank);
	memory_install_write8_handler(cpunum, spacenum, start,
		MIN(end, start - ram_offset + mess_ram_size - 1), 0, 0, write_bank);

	if (bank_size > (mess_ram_size - ram_offset))
	{
		memory_install_read8_handler(cpunum, spacenum, start - ram_offset + mess_ram_size, end, 0, 0, MRA8_ROM);
		memory_install_write8_handler(cpunum, spacenum, start - ram_offset + mess_ram_size, end, 0, 0, MWA8_ROM);
	}
	return mess_ram + ram_offset;
}


