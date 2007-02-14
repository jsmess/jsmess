/***************************************************************************

    driver.c

    Driver construction helpers.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "driver.h"
#include <ctype.h>



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define DRIVER_LRU_SIZE			10



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static int driver_lru[DRIVER_LRU_SIZE];



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static int penalty_compare(const char *source, const char *target);



/***************************************************************************
    MISCELLANEOUS BITS & PIECES
***************************************************************************/

/*-------------------------------------------------
    expand_machine_driver - construct a machine
    driver from the macroized state
-------------------------------------------------*/

void expand_machine_driver(void (*constructor)(machine_config *), machine_config *output)
{
	/* initialize the tag on the first screen */
	memset(output, 0, sizeof(*output));

	/* keeping this function allows us to pre-init the driver before constructing it */
	(*constructor)(output);

	/* if no screen tagged, tag screen 0 as main */
	if (output->screen[0].tag == NULL && output->screen[0].defstate.format != BITMAP_FORMAT_INVALID)
		output->screen[0].tag = "main";

	/* if no screens, set a dummy refresh for the main screen */
	if (output->screen[0].tag == NULL)
		output->screen[0].defstate.refresh = 60;
}


/*-------------------------------------------------
    driver_add_cpu - add a CPU during machine
    driver expansion
-------------------------------------------------*/

cpu_config *driver_add_cpu(machine_config *machine, const char *tag, int type, int cpuclock)
{
	int cpunum;

	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		if (machine->cpu[cpunum].cpu_type == 0)
		{
			machine->cpu[cpunum].tag = tag;
			machine->cpu[cpunum].cpu_type = type;
			machine->cpu[cpunum].cpu_clock = cpuclock;
			return &machine->cpu[cpunum];
		}

	fatalerror("Out of CPU's!\n");
	return NULL;
}


/*-------------------------------------------------
    driver_find_cpu - find a tagged CPU during
    machine driver expansion
-------------------------------------------------*/

cpu_config *driver_find_cpu(machine_config *machine, const char *tag)
{
	int cpunum;

	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		if (machine->cpu[cpunum].tag && strcmp(machine->cpu[cpunum].tag, tag) == 0)
			return &machine->cpu[cpunum];

	fatalerror("Can't find CPU '%s'!\n", tag);
	return NULL;
}


/*-------------------------------------------------
    driver_remove_cpu - remove a tagged CPU
    during machine driver expansion
-------------------------------------------------*/

void driver_remove_cpu(machine_config *machine, const char *tag)
{
	int cpunum;

	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		if (machine->cpu[cpunum].tag && strcmp(machine->cpu[cpunum].tag, tag) == 0)
		{
			memmove(&machine->cpu[cpunum], &machine->cpu[cpunum + 1], sizeof(machine->cpu[0]) * (MAX_CPU - cpunum - 1));
			memset(&machine->cpu[MAX_CPU - 1], 0, sizeof(machine->cpu[0]));
			return;
		}

	fatalerror("Can't find CPU '%s'!\n", tag);
}


/*-------------------------------------------------
    driver_add_speaker - add a speaker during
    machine driver expansion
-------------------------------------------------*/

speaker_config *driver_add_speaker(machine_config *machine, const char *tag, float x, float y, float z)
{
	int speakernum;

	for (speakernum = 0; speakernum < MAX_SPEAKER; speakernum++)
		if (machine->speaker[speakernum].tag == NULL)
		{
			machine->speaker[speakernum].tag = tag;
			machine->speaker[speakernum].x = x;
			machine->speaker[speakernum].y = y;
			machine->speaker[speakernum].z = z;
			return &machine->speaker[speakernum];
		}

	fatalerror("Out of speakers!\n");
	return NULL;
}


/*-------------------------------------------------
    driver_find_speaker - find a tagged speaker
    system during machine driver expansion
-------------------------------------------------*/

speaker_config *driver_find_speaker(machine_config *machine, const char *tag)
{
	int speakernum;

	for (speakernum = 0; speakernum < MAX_SPEAKER; speakernum++)
		if (machine->speaker[speakernum].tag && strcmp(machine->speaker[speakernum].tag, tag) == 0)
			return &machine->speaker[speakernum];

	fatalerror("Can't find speaker '%s'!\n", tag);
	return NULL;
}


/*-------------------------------------------------
    driver_remove_speaker - remove a tagged speaker
    system during machine driver expansion
-------------------------------------------------*/

void driver_remove_speaker(machine_config *machine, const char *tag)
{
	int speakernum;

	for (speakernum = 0; speakernum < MAX_SPEAKER; speakernum++)
		if (machine->speaker[speakernum].tag && strcmp(machine->speaker[speakernum].tag, tag) == 0)
		{
			memmove(&machine->speaker[speakernum], &machine->speaker[speakernum + 1], sizeof(machine->speaker[0]) * (MAX_SPEAKER - speakernum - 1));
			memset(&machine->speaker[MAX_SPEAKER - 1], 0, sizeof(machine->speaker[0]));
			return;
		}

	fatalerror("Can't find speaker '%s'!\n", tag);
}


/*-------------------------------------------------
    driver_add_sound - add a sound system during
    machine driver expansion
-------------------------------------------------*/

sound_config *driver_add_sound(machine_config *machine, const char *tag, int type, int clock)
{
	int soundnum;

	for (soundnum = 0; soundnum < MAX_SOUND; soundnum++)
		if (machine->sound[soundnum].sound_type == 0)
		{
			machine->sound[soundnum].tag = tag;
			machine->sound[soundnum].sound_type = type;
			machine->sound[soundnum].clock = clock;
			machine->sound[soundnum].config = NULL;
			machine->sound[soundnum].routes = 0;
			return &machine->sound[soundnum];
		}

	fatalerror("Out of sounds!\n");
	return NULL;
}


/*-------------------------------------------------
    driver_find_sound - find a tagged sound
    system during machine driver expansion
-------------------------------------------------*/

sound_config *driver_find_sound(machine_config *machine, const char *tag)
{
	int soundnum;

	for (soundnum = 0; soundnum < MAX_SOUND; soundnum++)
		if (machine->sound[soundnum].tag && strcmp(machine->sound[soundnum].tag, tag) == 0)
			return &machine->sound[soundnum];

	fatalerror("Can't find sound '%s'!\n", tag);
	return NULL;
}


/*-------------------------------------------------
    driver_remove_sound - remove a tagged sound
    system during machine driver expansion
-------------------------------------------------*/

void driver_remove_sound(machine_config *machine, const char *tag)
{
	int soundnum;

	for (soundnum = 0; soundnum < MAX_SOUND; soundnum++)
		if (machine->sound[soundnum].tag && strcmp(machine->sound[soundnum].tag, tag) == 0)
		{
			memmove(&machine->sound[soundnum], &machine->sound[soundnum + 1], sizeof(machine->sound[0]) * (MAX_SOUND - soundnum - 1));
			memset(&machine->sound[MAX_SOUND - 1], 0, sizeof(machine->sound[0]));
			return;
		}

	fatalerror("Can't find sound '%s'!\n", tag);
}


/*-------------------------------------------------
    driver_add_screen - add a screen during
    machine driver expansion
-------------------------------------------------*/

screen_config *driver_add_screen(machine_config *machine, const char *tag, int palbase)
{
	int screennum;

	for (screennum = 0; screennum < MAX_SCREENS; screennum++)
		if (machine->screen[screennum].tag == NULL)
		{
			machine->screen[screennum].tag = tag;
			machine->screen[screennum].palette_base = palbase;
			return &machine->screen[screennum];
		}

	fatalerror("Out of screens!\n");
	return NULL;
}


/*-------------------------------------------------
    driver_find_screen - find a tagged screen
    during machine driver expansion
-------------------------------------------------*/

screen_config *driver_find_screen(machine_config *machine, const char *tag)
{
	int screennum;

	for (screennum = 0; screennum < MAX_SCREENS; screennum++)
		if (machine->screen[screennum].tag && strcmp(machine->screen[screennum].tag, tag) == 0)
			return &machine->screen[screennum];

	fatalerror("Can't find screen '%s'!\n", tag);
	return NULL;
}


/*-------------------------------------------------
    driver_remove_screen - remove a tagged screen
    during machine driver expansion
-------------------------------------------------*/

void driver_remove_screen(machine_config *machine, const char *tag)
{
	int screennum;

	for (screennum = 0; screennum < MAX_SCREENS; screennum++)
		if (machine->screen[screennum].tag && strcmp(machine->screen[screennum].tag, tag) == 0)
		{
			memmove(&machine->screen[screennum], &machine->screen[screennum + 1], sizeof(machine->screen[0]) * (MAX_SCREENS - screennum - 1));
			memset(&machine->screen[MAX_SCREENS - 1], 0, sizeof(machine->screen[0]));
			return;
		}

	fatalerror("Can't find screen '%s'!\n", tag);
}


/*-------------------------------------------------
    driver_get_index - return the index of a
    driver given its name.
-------------------------------------------------*/

int driver_get_index(const char *name)
{
	int lurnum, drvnum;

	/* scan the LRU list first */
	for (lurnum = 0; lurnum < DRIVER_LRU_SIZE; lurnum++)
		if (mame_stricmp(drivers[driver_lru[lurnum]]->name, name) == 0)
		{
			/* if not first, swap with the head */
			if (lurnum != 0)
			{
				int temp = driver_lru[0];
				driver_lru[0] = driver_lru[lurnum];
				driver_lru[lurnum] = temp;
			}
			return driver_lru[0];
		}

	/* scan for a match in the drivers -- slow! */
	for (drvnum = 0; drivers[drvnum] != NULL; drvnum++)
		if (mame_stricmp(drivers[drvnum]->name, name) == 0)
		{
			memmove((void *)&driver_lru[1], (void *)&driver_lru[0], sizeof(driver_lru[0]) * (DRIVER_LRU_SIZE - 1));
			driver_lru[0] = drvnum;
			return drvnum;
		}

	/* shouldn't happen */
	return -1;
}


/*-------------------------------------------------
    driver_get_clone - return a pointer to the
    clone of a game driver.
-------------------------------------------------*/

const game_driver *driver_get_clone(const game_driver *driver)
{
	int index;

	/* if no clone, easy out */
	if (driver->parent == NULL || (driver->parent[0] == '0' && driver->parent[1] == 0))
		return NULL;

	/* convert the name to an index */
	index = driver_get_index(driver->parent);
	return (index == -1) ? NULL : drivers[index];
}


/*-------------------------------------------------
    driver_get_approx_matches - find the best n
    matches to a driver name.
-------------------------------------------------*/

void driver_get_approx_matches(const char *name, int matches, int *indexes)
{
	int *penalty;
	int drvnum;
	int matchnum;

	/* allocate some temp memory */
	penalty = malloc_or_die(matches * sizeof(*penalty));

	/* initialize everyone's states */
	for (matchnum = 0; matchnum < matches; matchnum++)
	{
		penalty[matchnum] = 9999;
		indexes[matchnum] = -1;
	}

	/* scan the entire drivers array */
	for (drvnum = 0; drivers[drvnum] != NULL; drvnum++)
	{
		int curpenalty, tmp;

		/* skip non-drivers */
		if ((drivers[drvnum]->flags & NOT_A_DRIVER) != 0)
			continue;

		/* pick the best match between driver name and description */
		curpenalty = penalty_compare(name, drivers[drvnum]->description);
		tmp = penalty_compare(name, drivers[drvnum]->name);
		curpenalty = MIN(curpenalty, tmp);

		/* insert into the sorted table of matches */
		for (matchnum = matches - 1; matchnum >= 0; matchnum--)
		{
			/* stop if we're worse than the current entry */
			if (curpenalty >= penalty[matchnum])
				break;

			/* as lng as this isn't the last entry, bump this one down */
			if (matchnum < matches - 1)
			{
				penalty[matchnum + 1] = penalty[matchnum];
				indexes[matchnum + 1] = indexes[matchnum];
			}
			indexes[matchnum] = drvnum;
			penalty[matchnum] = curpenalty;
		}
	}

	/* free our temp memory */
	free(penalty);
}


/*-------------------------------------------------
    penalty_compare - compare two strings for
    closeness and assign a score.
-------------------------------------------------*/

static int penalty_compare(const char *source, const char *target)
{
	int gaps = 0;
	int last = 1;

	/* scan the strings */
	for ( ; *source && *target; target++)
	{
		/* do a case insensitive match */
		int match = (tolower(*source) == tolower(*target));

		/* if we matched, advance the source */
		if (match)
			source++;

		/* if the match state changed, count gaps */
		if (match != last)
		{
			last = match;
			if (!match)
				gaps++;
		}
	}

	/* penalty if short string does not completely fit in */
	for ( ; *source; source++)
		gaps++;

	return gaps;
}


/*-------------------------------------------------
    driver_get_count - returns the amount of
    drivers
-------------------------------------------------*/

int driver_get_count(void)
{
	int i;
	static int count;

	if (count == 0)
	{
		// count the number of drivers
		for (i = 0; drivers[i]; i++)
		{
		}
		count = i;
	}
	return count;
}
