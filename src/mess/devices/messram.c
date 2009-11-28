/*************************************************************************

    MESS RAM device

    Provides a configurable amount of RAM to drivers

**************************************************************************/

#include <stdio.h>
#include <ctype.h>

#include "driver.h"
#include "messram.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define RAM_STRING_BUFLEN	16
#define MAX_RAM_OPTIONS		16


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _messram_state messram_state;
struct _messram_state
{
	UINT32 size; /* total amount of ram configured */
	UINT8 *ram;  /* pointer to the start of ram */
};


/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/

INLINE messram_state *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == MESSRAM);

	return (messram_state *)device->token;
}


/*****************************************************************************
    HELPER FUNCTIONS
*****************************************************************************/

/*-------------------------------------------------
    messram_parse_string - convert a ram string to an
    integer value
-------------------------------------------------*/

UINT32 messram_parse_string(const char *s)
{
	UINT32 ram;
	char suffix = '\0';

	s += sscanf(s, "%u%c", &ram, &suffix);

	switch(tolower(suffix))
	{
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

#ifdef UNUSED_FUNCTION
const char *messram_string(char *buffer, UINT32 ram)
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
#endif


/*****************************************************************************
    DEVICE INTERFACE
*****************************************************************************/

static DEVICE_START( messram )
{
	messram_state *messram = get_safe_token(device);
	ram_config *config = device->inline_config;

	/* the device named 'messram' can get ram options from command line */
	if (strcmp(device->tag, "messram") == 0)
	{
		const char *ramsize_string = options_get_string(mame_options(), OPTION_RAMSIZE);

		if ((ramsize_string != NULL) && (ramsize_string[0] != '\0'))
			messram->size = messram_parse_string(ramsize_string);
	}

	/* if we didn't get a size yet, use the default */
	if (messram->size == 0)
		messram->size = messram_parse_string(config->default_size);

	/* allocate space for the ram */
	messram->ram = auto_alloc_array(device->machine, UINT8, messram->size);

	/* reset ram to the default value */
	memset(messram->ram, config->default_value, messram->size);

	/* register for state saving */
	state_save_register_device_item(device, 0, messram->size);
	state_save_register_device_item_pointer(device, 0, messram->ram, messram->size);
}

static DEVICE_VALIDITY_CHECK( messram )
{
	ram_config *config = device->inline_config;
	const char *ramsize_string = NULL;
	int is_valid = FALSE;
	UINT32 specified_ram = 0;
	int error = FALSE;
	const char *gamename_option = NULL;

	/* verify default ram value */
	if (config!=NULL && messram_parse_string(config->default_size) == 0)
	{
		mame_printf_error("%s: '%s' has an invalid default RAM option: %s\n", driver->source_file, driver->name, config->default_size);
		error = TRUE;
	}
	
	/* command line options are only parsed for the device named "messram" */
	if (device->tag!=NULL && strcmp(device->tag, "messram") == 0)
	{
		if (mame_options()==NULL) return FALSE;
		/* verify command line ram option */
		ramsize_string = options_get_string(mame_options(), OPTION_RAMSIZE);
		gamename_option = options_get_string(mame_options(), OPTION_GAMENAME);

		if ((ramsize_string != NULL) && (ramsize_string[0] != '\0'))
		{
			specified_ram = messram_parse_string(ramsize_string);

			if (specified_ram == 0)
			{
				mame_printf_error("%s: '%s' cannot recognize the RAM option %s\n", driver->source_file, driver->name, ramsize_string);
				error = TRUE;
			}
			if (gamename_option!=NULL && strlen(gamename_option) > 0 && strcmp(gamename_option, driver->name) == 0)
			{							
				/* compare command line option to default value */
				if (messram_parse_string(config->default_size) == specified_ram)
					is_valid = TRUE;

				/* verify extra ram options */
				if (config->extra_options != NULL)
				{
					const char *s;

					astring *buffer = astring_alloc();
					astring_cpyc(buffer, config->extra_options);
					astring_replacechr(buffer, ',', 0);

					s = astring_c(buffer);

					/* try to parse each option */
					while(*s != '\0')
					{
						UINT32 option_ram_size = messram_parse_string(s);

						if (option_ram_size == 0)
						{
							mame_printf_error("%s: '%s' has an invalid RAM option: %s\n", driver->source_file, driver->name, s);
							error = TRUE;
						}

						if (option_ram_size == specified_ram)
							is_valid = TRUE;

						s += strlen(s) + 1;
					}

					astring_free(buffer);
				}

			} else {
				/* if not for this driver then return ok */
				is_valid = TRUE;
			}
		}
		else
		{
			/* not specifying the ramsize on the command line is valid as well */
			is_valid = TRUE;
		}
	}
	else
		is_valid = TRUE;
				
	if (!is_valid)
	{
		mame_printf_error("%s: '%s' cannot recognize the RAM option %s", driver->source_file, driver->name, ramsize_string);
		mame_printf_error(" (valid options are %s", config->default_size);

		if (config->extra_options != NULL)
			mame_printf_error(",%s).\n", config->extra_options);
		else
			mame_printf_error(").\n");

		error = TRUE;
	}

	return error;
	
}


DEVICE_GET_INFO( messram )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(messram_state);			break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(ram_config);				break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_OTHER;				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(messram);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;
		case DEVINFO_FCT_VALIDITY_CHECK:				info->validity_check = DEVICE_VALIDITY_CHECK_NAME(messram); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "MESS RAM");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "RAM");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.00");					break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");		break;
	}
}


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

UINT32 messram_get_size(const device_config *device)
{
	messram_state *messram = get_safe_token(device);
	return messram->size;
}


UINT8 *messram_get_ptr(const device_config *device)
{
	messram_state *messram = get_safe_token(device);
	return messram->ram;
}


#ifdef UNUSED_FUNCTION
void messram_dump(const device_config *device, const char *filename)
{
	messram_state *messram = get_safe_token(device);
	file_error filerr;
	mame_file *file;

	/* use a default filename */
	if (!filename)
		filename = "ram.bin";

	/* open the file */
	filerr = mame_fopen(NULL, filename, OPEN_FLAG_WRITE, &file);
	if (filerr == FILERR_NONE)
	{
		/* write the data */
		mame_fwrite(file, messram->ram, messram->size);

		/* close file */
		mame_fclose(file);
	}
}
#endif
