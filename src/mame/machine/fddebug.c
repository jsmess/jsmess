/***************************************************************************

    fddebug.c

    FD1094 decryption helper routines.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************

    When searching for new keys, here are some common sequences in the
    System 16B games that are useful.

    IRQ4 handler entry points:

        common sequence 1:
            MOVE        SR,(A7)             40D7
            MOVE.B      #$23,(A7)           1EBC 0023
            MOVEM.L     D0-D7/A0-A6,-(A7)   48E7 FFFE

        common sequence 2:
            MOVEM.L     D0-D7/A0-A6,-(A7)   48E7 FFFE

        common sequence 3:
            BRA.W       <previous sequence> 6000 xxxx

    IRQ4 handler exit points:

        common sequence (often appears twice nearby):
            MOVE        (A7)+,D0-D7/A0-A6   4CDF 7FFF
            RTE                             4E73

    Entry points:

        common sequence 1:
            LEA         <stack>.L,A7        4FF9 xxxx xxxx
            MOVE        #$2700,SR           46FC 2700
            CMPI.L      #$00xxffff,D0       0C80 00xx FFFF
            MOVEQ       #0,D0
            MOVE.L      D0,D1               2200
            MOVE.L      D0,D2               2400
            MOVE.L      D0,D3               2600
            MOVE.L      D0,D4               2800
            MOVE.L      D0,D5               2A00
            MOVE.L      D0,D6               2C00
            MOVE.L      D0,D7               2E00

        common sequence 2:
            LEA         <stack>.W,A7        4FF8 xxxx
            MOVE        #$2700,SR           46FC 2700
            CMPI.L      #$00xxffff,D0       0C80 00xx FFFF
            MOVEQ       #0,D0
            MOVE.L      D0,D1               2200
            MOVE.L      D0,D2               2400
            MOVE.L      D0,D3               2600
            MOVE.L      D0,D4               2800
            MOVE.L      D0,D5               2A00
            MOVE.L      D0,D6               2C00
            MOVE.L      D0,D7               2E00

        common sequence 3:
            LEA         <stack>.W,A7        4FF8 xxxx
            MOVE        #$2700,SR           46FC 2700
            MOVEQ       #0,D0
            MOVE.L      D0,D1               2200
            MOVE.L      D0,D2               2400
            MOVE.L      D0,D3               2600
            MOVE.L      D0,D4               2800
            MOVE.L      D0,D5               2A00
            MOVE.L      D0,D6               2C00
            MOVE.L      D0,D7               2E00

        common sequence 4:
            BRA.W       <previous sequence> 6000 xxxx

****************************************************************************

    These constraints worked for finding exctleag's seed:

        fdcset 0410,4ff9
        fdcset 0412,0000
        fdcset 0414,0000
        fdcset 0416,46fc
        fdcset 0418,2700
        fdcset 041a,0c80
        fdcset 041c,0000,ff00
        fdcset 041e,ffff

        //fdcset 0f9e,40d7,ffff,irq
        fdcset 0fa0,1ebc,ffff,irq
        fdcset 0fa2,0023,ffff,irq
        //fdcset 0fa4,48e7,ffff,irq
        fdcset 0fa6,fffe,ffff,irq
        fdcset 0fa8,13f8,ffff,irq
        fdcset 0fac,00c4,ffff,irq
        fdcset 0fae,0001,ffff,irq

        //fdcset 1060,4cdf,ffff,irq
        fdcset 1062,7fff,ffff,irq
        //fdcset 1064,4e73,ffff,irq
        //fdcset 1070,4cdf,ffff,irq
        fdcset 1072,7fff,ffff,irq
        //fdcset 1074,4e73,ffff,irq

***************************************************************************/

#ifdef MAME_DEBUG

#include "driver.h"
#include "machine/fd1094.h"
#include "cpu/m68000/m68k.h"

#include "debug/debugcmd.h"
#include "debug/debugcon.h"
#include "debug/debugcpu.h"
#include "debug/debugvw.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define KEY_SIZE			8192
#define MAX_CONSTRAINTS		100

/* status byte breakdown */
#define STATE_MASK			0xff00
#define HIBITS_MASK			0x00c0
#define STATUS_MASK			0x003f

/* possible status values */
#define STATUS_UNVISITED	0x00
#define STATUS_LOCKED		0x01
#define STATUS_NOCHANGE		0x02
#define STATUS_GUESS		0x03



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* a single possible instruction decoding */
typedef struct _fd1094_possibility fd1094_possibility;
struct _fd1094_possibility
{
	offs_t		basepc;				/* starting PC of the possibility */
	int			length;				/* number of words */
	UINT8		instrbuffer[10];	/* instruction data for disassembler */
	UINT8		keybuffer[10];		/* array of key values to produce the instruction data */
	char		dasm[256];			/* disassembly */
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

/* array of PCs not to stop at */
static UINT8 *				ignorepc;

/* array of instruction lengths per opcode */
static UINT8 *				oplength;

/* buffer for undoing operations */
static UINT8 *				undobuff;

/* array of possible instruction decodings */
static fd1094_possibility	posslist[4*4*4*4*4];
static int 					posscount;

/* array of possible seeds */
static UINT32 *				possible_seed;

/* array of constraints */
static fd1094_constraint	constraints[MAX_CONSTRAINTS];
static int					constcount;

/* current key generation parameters */
static UINT32				fd1094_global;
static UINT32				fd1094_seed;
static UINT8				keydirty;

/* pointers to our data */
static UINT16 *				coderegion;
static UINT32				coderegion_words;
static UINT8 *				keyregion;
static UINT16 *				keystatus;
static UINT32				keystatus_words;

/* key changed callback */
static void					(*key_changed)(void);



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void set_default_key_params(void);
static void load_overlay_file(void);
static void save_overlay_file(void);
void fd1094_regenerate_key(void);

static int instruction_hook(offs_t curpc);

static void execute_fdsave(int ref, int params, const char **param);
static void execute_fdseed(int ref, int params, const char **param);
static void execute_fdlockguess(int ref, int params, const char **param);
static void execute_fdunlock(int ref, int params, const char **param);
static void execute_fdignore(int ref, int params, const char **param);
static void execute_fdundo(int ref, int params, const char **param);
static void execute_fdstatus(int ref, int params, const char **param);
static void execute_fdcset(int ref, int params, const char **param);
static void execute_fdclist(int ref, int params, const char **param);
static void execute_fdcsearch(int ref, int params, const char **param);

static fd1094_possibility *try_all_possibilities(int basepc, int offset, int length, UINT8 *instrbuffer, UINT8 *keybuffer, fd1094_possibility *possdata);
static void tag_possibility(fd1094_possibility *possdata, UINT8 status);

static void perform_constrained_search(void);
static UINT32 find_global_key_matches(UINT32 startwith, UINT16 *output);
static int find_constraint_sequence(UINT32 global, int quick);
static int does_key_work_for_constraints(const UINT16 *base, UINT8 *key);
static UINT32 reconstruct_base_seed(int keybaseaddr, UINT32 startseed);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-----------------------------------------------
    addr_to_keyaddr - given an address,
    return the address in the key that will be
    used to decrypt it
-----------------------------------------------*/

INLINE int addr_to_keyaddr(offs_t address)
{
	/* for address xx0000-xx0006 (but only if >= 000008), use key xx2000-xx2006 */
	if ((address & 0x0ffc) == 0 && address >= 4)
		return (address & 0x1fff) | 0x1000;
	else
		return address & 0x1fff;
}


/*-----------------------------------------------
    mask_for_keyaddr - given a key address,
    return a mask indicating which bits should
    always be 1
-----------------------------------------------*/

INLINE UINT8 mask_for_keyaddr(offs_t address)
{
	/* the first half of the key always has bit 0x80 set; the second half 0x40 */
	/* however, the values at 0000-0003 and 1000-1003 don't follow this rule */
	if ((address & 0x0ffc) == 0)
		return 0x00;
	else if ((address & 0x1000) == 0)
		return 0x80;
	else
		return 0x40;
}


/*-----------------------------------------------
    advance_seed - advance the PRNG seed by
    the specified number of steps
-----------------------------------------------*/

INLINE UINT32 advance_seed(UINT32 seed, int count)
{
	/* iterate over the seed for 'count' reps */
	while (count--)
	{
		seed = seed * 0x29;
		seed += seed << 16;
	}
	return seed;
}


/*-----------------------------------------------
    key_value_from_seed - extract the key value
    from a seed and apply the given mask
-----------------------------------------------*/

INLINE UINT8 key_value_from_seed(UINT32 seed, UINT8 mask)
{
	/* put bits 16-21 of the seed in the low 6 bits and OR with the mask */
	return ((~seed >> 16) & 0x3f) | mask;
}


/*-----------------------------------------------
    generate_key_bytes - generate a sequence of
    consecutive key bytes, starting with the
    given seed
-----------------------------------------------*/

INLINE void generate_key_bytes(UINT8 *dest, UINT32 keyoffs, UINT32 count, UINT32 seed)
{
	int bytenum;

	/* generate 'count' bytes of a key */
	for (bytenum = 0; bytenum < count; bytenum++)
	{
		UINT32 keyaddr = (keyoffs + bytenum) & 0x1fff;
		UINT8 mask = mask_for_keyaddr(keyaddr);

		/* advance the seed first, then store the derived value */
		seed = advance_seed(seed, 1);
        dest[keyaddr] = key_value_from_seed(seed, mask);
    }
}


/*-----------------------------------------------
    get_opcode_length - return the length of
    an opcode based on the opcode
-----------------------------------------------*/

INLINE UINT8 get_opcode_length(UINT16 opcode)
{
	/* if we don't know the length yet, figure it out */
	if (oplength[opcode] == 0)
	{
		char dummybuffer[256];
		UINT8 instrbuffer[10];

		/* build up the first word of an instruction */
		instrbuffer[0] = opcode >> 8;
		instrbuffer[1] = opcode;

		/* disassembling that gives us the length */
		oplength[opcode] = (m68k_disassemble_raw(dummybuffer, 0, instrbuffer, instrbuffer, M68K_CPU_TYPE_68000) & 0xff) / 2;
	}

	/* return the length from the table */
	return oplength[opcode];
}


/*-----------------------------------------------
    set_constraint - set the values of a
    constraint
-----------------------------------------------*/

INLINE void set_constraint(fd1094_constraint *constraint, UINT32 pc, UINT16 state, UINT16 value, UINT16 mask)
{
	constraint->pc = pc;
	constraint->state = state;
	constraint->value = value & mask;
	constraint->mask = mask;
}



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

/*-----------------------------------------------
    fd1094_init_debugging - set up debugging
-----------------------------------------------*/

void fd1094_init_debugging(int cpureg, int keyreg, int statreg, void (*changed)(void))
{
	/* set the key changed callback */
	key_changed = changed;

	/* set up the regions */
	coderegion = (UINT16 *)memory_region(cpureg);
	coderegion_words = memory_region_length(cpureg) / 2;
	keyregion = (UINT8 *)memory_region(keyreg);
	keystatus = (UINT16 *)memory_region(statreg);
	keystatus_words = memory_region_length(statreg) / 2;
	assert(coderegion_words == keystatus_words);

	/* allocate memory for the opcode length table */
	oplength = auto_malloc(65536);
	memset(oplength, 0, 65536);

	/* allocate memory for the ignore table */
	ignorepc = auto_malloc(1 << 23);
	memset(ignorepc, 0, 1 << 23);

	/* allocate memory for the undo buffer */
	undobuff = auto_malloc(keystatus_words * 2);
	memcpy(undobuff, keystatus, keystatus_words * 2);

	/* allocate memory for the possible seeds array */
	possible_seed = auto_malloc(65536 * sizeof(possible_seed[0]));

	/* set up default constraints */
	constcount = 0;
	set_constraint(&constraints[constcount++], 0x000000, FD1094_STATE_RESET, 0x0000, 0xffff);
	set_constraint(&constraints[constcount++], 0x000002, FD1094_STATE_RESET, 0x0000, 0xffff);
	set_constraint(&constraints[constcount++], 0x000004, FD1094_STATE_RESET, 0x0000, 0xffff);
	set_constraint(&constraints[constcount++], 0x000006, FD1094_STATE_RESET, 0x0000, 0xc001);

	/* determine the key parameters */
	set_default_key_params();

	/* read the key overlay file */
	load_overlay_file();

	/* add some commands */
	debug_console_register_command("fdsave", CMDFLAG_NONE, 0, 0, 0, execute_fdsave);
	debug_console_register_command("fdseed", CMDFLAG_NONE, 0, 2, 2, execute_fdseed);
	debug_console_register_command("fdguess", CMDFLAG_NONE, STATUS_GUESS, 1, 1, execute_fdlockguess);
	debug_console_register_command("fdlock", CMDFLAG_NONE, STATUS_LOCKED, 1, 1, execute_fdlockguess);
	debug_console_register_command("fdunlock", CMDFLAG_NONE, 0, 1, 1, execute_fdunlock);
	debug_console_register_command("fdignore", CMDFLAG_NONE, 0, 0, 1, execute_fdignore);
	debug_console_register_command("fdundo", CMDFLAG_NONE, 0, 0, 0, execute_fdundo);
	debug_console_register_command("fdstatus", CMDFLAG_NONE, 0, 0, 0, execute_fdstatus);
	debug_console_register_command("fdcset", CMDFLAG_NONE, 0, 2, 4, execute_fdcset);
	debug_console_register_command("fdclist", CMDFLAG_NONE, 0, 0, 0, execute_fdclist);
	debug_console_register_command("fdcsearch", CMDFLAG_NONE, 0, 0, 0, execute_fdcsearch);

	/* set up the instruction hook */
	debug_set_instruction_hook(0, instruction_hook);

	/* regenerate the key */
	if (keydirty)
		fd1094_regenerate_key();
}


/*-----------------------------------------------
    set_default_key_params - based on the game
    name, set some defaults
-----------------------------------------------*/

static void set_default_key_params(void)
{
	static const struct
	{
		const char *	gamename;
		UINT32			global;
		UINT32			seed;
	} default_keys[] =
	{
		{ "altbeaj1", 0xFCAFF9F9, 0x177AC6 },
		{ "bullet",   0x12A8F8E5, 0x0AD691 },
		{ "exctleag", 0x98AFF6FA, 0x31DDC6 },
		{ "suprleag", 0x3CFEFFD8, 0x127114 },
//      { "suprleag", 0x2CFEFFF9, 0x027114 },
	};
	int keynum;

	/* look for a matching game and set the key appropriately */
	for (keynum = 0; keynum < ARRAY_LENGTH(default_keys); keynum++)
		if (strcmp(Machine->gamedrv->name, default_keys[keynum].gamename) == 0)
		{
			fd1094_global = default_keys[keynum].global;
			fd1094_seed = default_keys[keynum].seed;
			keydirty = TRUE;
			break;
		}
}


/*-----------------------------------------------
    load_overlay_file - load the key overlay
    file
-----------------------------------------------*/

static void load_overlay_file(void)
{
	char filename[20];
	file_error filerr;
	mame_file *file;
	int pcaddr;

	/* determin the filename and open the file */
	sprintf(filename, "%s.kov", Machine->gamedrv->name);
	filerr = mame_fopen(SEARCHPATH_RAW, filename, OPEN_FLAG_READ, &file);
	if (filerr == FILERR_NONE)
	{
		int bytes = mame_fread(file, keystatus, keystatus_words * 2);
		mame_fclose(file);

		/* temporary hack */
		if (bytes == keystatus_words)
		{
			int reps = keystatus_words / KEY_SIZE, repnum;
			UINT8 *src = malloc_or_die(keystatus_words);
			memcpy(src, keystatus, keystatus_words);
			for (pcaddr = 0; pcaddr < KEY_SIZE; pcaddr++)
				for (repnum = 0; repnum < reps; repnum++)
				{
					UINT8 srcdata = src[repnum * KEY_SIZE + pcaddr];
					keystatus[repnum * KEY_SIZE + pcaddr] = srcdata & STATUS_MASK;
					keystatus[pcaddr] |= srcdata & HIBITS_MASK;
				}
			free(src);
		}
		else
		{

		/* convert from big-endian */
		for (pcaddr = 0; pcaddr < keystatus_words; pcaddr++)
			keystatus[pcaddr] = BIG_ENDIANIZE_INT16(keystatus[pcaddr]);
		}
	}

	/* mark the key dirty */
	keydirty = TRUE;
}


/*-----------------------------------------------
    save_overlay_file - save the key overlay
    file
-----------------------------------------------*/

static void save_overlay_file(void)
{
	char filename[20];
	file_error filerr;
	mame_file *file;
	int pcaddr;

	/* determin the filename and open the file */
	sprintf(filename, "%s.kov", Machine->gamedrv->name);
	filerr = mame_fopen(SEARCHPATH_RAW, filename, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE, &file);
	if (filerr == FILERR_NONE)
	{
		/* convert to big-endian */
		for (pcaddr = 0; pcaddr < keystatus_words; pcaddr++)
			keystatus[pcaddr] = BIG_ENDIANIZE_INT16(keystatus[pcaddr]);

		/* write the data */
		mame_fwrite(file, keystatus, keystatus_words * 2);
		mame_fclose(file);

		/* convert from big-endian */
		for (pcaddr = 0; pcaddr < keystatus_words; pcaddr++)
			keystatus[pcaddr] = BIG_ENDIANIZE_INT16(keystatus[pcaddr]);
	}
}


/*-----------------------------------------------
    fd1094_regenerate_key - regenerate the key
    based on the raw parameters and the overlay
    data
-----------------------------------------------*/

void fd1094_regenerate_key(void)
{
	int reps = keystatus_words / KEY_SIZE;
	int keyaddr, repnum;

	/* store the global key in the first 4 bytes */
	keyregion[0] = fd1094_global >> 24;
	keyregion[1] = fd1094_global >> 16;
	keyregion[2] = fd1094_global >> 8;
	keyregion[3] = fd1094_global >> 0;

	/* then generate the remaining 8188 bytes */
	generate_key_bytes(keyregion, 4, 8192 - 4, fd1094_seed);

	/* apply the overlay */
	for (keyaddr = 0; keyaddr < KEY_SIZE; keyaddr++)
	{
		keyregion[keyaddr] |= keystatus[keyaddr] & HIBITS_MASK;

		/* if we're locked, propogate that info to all our reps */
		if ((keystatus[keyaddr] & STATUS_MASK) == STATUS_LOCKED)
			for (repnum = 1; repnum < reps; repnum++)
				keystatus[repnum * KEY_SIZE + keyaddr] = (keystatus[repnum * KEY_SIZE + keyaddr] & ~STATUS_MASK) | STATUS_LOCKED;
	}

	/* update the key with the current fd1094 manager */
	if (key_changed != NULL)
		(*key_changed)();

	/* force all memory and disassembly views to update */
	debug_view_update_type(DVT_MEMORY);
	debug_disasm_update_all();

	/* reset keydirty */
	keydirty = FALSE;
}


/*-----------------------------------------------
    instruction_hook - per-instruction hook
-----------------------------------------------*/

static int instruction_hook(offs_t curpc)
{
	int curfdstate = fd1094_set_state(keyregion, -1);
	UINT8 instrbuffer[10], keybuffer[5];
	int i, keystat;

	/* quick exit if we're ignoring */
	if (ignorepc != NULL && ignorepc[curpc/2])
		return 0;

	/* quick exit if we're already locked */
	keystat = keystatus[curpc/2] & STATUS_MASK;
	keystatus[curpc/2] = (keystatus[curpc/2] & ~STATE_MASK) | (curfdstate << 8);
	if (keystat == STATUS_LOCKED || keystat == STATUS_NOCHANGE)
	{
		UINT16 opcode = fd1094_decode(curpc/2, coderegion[curpc/2], keyregion, 0);
		int length = get_opcode_length(opcode);
		for (i = 1; i < length; i++)
		{
			keystat = keystatus[curpc/2 + i] & STATUS_MASK;
			if (keystat != STATUS_LOCKED && keystat != STATUS_NOCHANGE)
				break;
		}
		if (i == length)
		{
			for (i = 1; i < length; i++)
				keystatus[curpc/2 + i] = (keystatus[curpc/2 + i] & ~STATE_MASK) | (curfdstate << 8);
			return 0;
		}
	}

	/* try all possible decodings at the current pc */
	posscount = try_all_possibilities(curpc, 0, 0, instrbuffer, keybuffer, posslist) - posslist;
	if (keydirty)
		fd1094_regenerate_key();

	/* if we only ended up with one possibility, mark that one as good */
	if (posscount == 1)
	{
		tag_possibility(&posslist[0], STATUS_LOCKED);
		fd1094_regenerate_key();
		return 0;
	}

	/* print possibilities and break */
	debug_console_printf("Possibilities:\n");
	for (i = 0; i < posscount; i++)
		debug_console_printf("%2x: %s\n", i, posslist[i].dasm);

	return 1;
}


/*-----------------------------------------------
    execute_fdsave - handle the 'fdsave' command
-----------------------------------------------*/

static void execute_fdsave(int ref, int params, const char **param)
{
	save_overlay_file();
	debug_console_printf("File saved\n");
}


/*-----------------------------------------------
    execute_fdseed - handle the 'fdseed' command
-----------------------------------------------*/

static void execute_fdseed(int ref, int params, const char **param)
{
	UINT64 num1, num2;

	/* extract the parameters */
	if (!debug_command_parameter_number(param[0], &num1))
		return;
	if (!debug_command_parameter_number(param[1], &num2))
		return;

	/* set the global and seed, and then regenerate the key */
	fd1094_global = num1;
	fd1094_seed = num2;
	fd1094_regenerate_key();
}


/*-----------------------------------------------
    execute_fdlockguess - handle the 'fdlock'
    and 'fdguess' commands
-----------------------------------------------*/

static void execute_fdlockguess(int ref, int params, const char **param)
{
	UINT64 num1;

	/* extract the parameter */
	if (!debug_command_parameter_number(param[0], &num1))
		return;

	/* make sure it is within range of our recent possibilities */
	if (num1 >= posscount)
	{
		debug_console_printf("Possibility of out range (%x max)\n", posscount);
		return;
	}

	/* create an undo buffer */
	memcpy(undobuff, keystatus, keystatus_words * 2);

	/* tag this possibility as indicated by the ref parameter, and then regenerate the key */
	tag_possibility(&posslist[num1], ref);
	fd1094_regenerate_key();

	/* tell the user what happened */
	if (ref == STATUS_LOCKED)
		debug_console_printf("Locked possibility %x\n", (int)num1);
	else
		debug_console_printf("Guessing possibility %x\n", (int)num1);
}


/*-----------------------------------------------
    execute_fdunlock - handle the 'fdunlock'
    command
-----------------------------------------------*/

static void execute_fdunlock(int ref, int params, const char **param)
{
	int reps = keystatus_words / KEY_SIZE;
	int keyaddr, repnum;
	UINT64 offset;

	/* support 0 or 1 parameters */
	if (params != 1 || !debug_command_parameter_number(param[0], &offset))
 		offset = activecpu_get_pc();
 	keyaddr = addr_to_keyaddr(offset / 2);

	/* toggle the ignore PC status */
	debug_console_printf("Unlocking PC %06X\n", (int)offset);

	/* iterate over all reps and unlock them */
	for (repnum = 0; repnum < reps; repnum++)
	{
		UINT16 *dest = &keystatus[repnum * KEY_SIZE + keyaddr];
		if ((*dest & STATUS_MASK) == STATUS_LOCKED)
			*dest &= STATE_MASK;
	}
}


/*-----------------------------------------------
    execute_fdignore - handle the 'fdignore'
    command
-----------------------------------------------*/

static void execute_fdignore(int ref, int params, const char **param)
{
	UINT64 offset;

	/* support 0 or 1 parameters */
	if (params != 1 || !debug_command_parameter_number(param[0], &offset))
 		offset = activecpu_get_pc();
 	offset /= 2;

	/* toggle the ignore PC status */
	ignorepc[offset] = !ignorepc[offset];
	if (ignorepc[offset])
		debug_console_printf("Ignoring address %06X\n", (int)offset * 2);
	else
		debug_console_printf("No longer ignoring address %06X\n", (int)offset * 2);

	/* if no parameter given, implicitly run as well */
	if (params == 0)
		debug_cpu_go(~0);
}


/*-----------------------------------------------
    execute_fdundo - handle the 'fdundo'
    command
-----------------------------------------------*/

static void execute_fdundo(int ref, int params, const char **param)
{
	/* copy the undobuffer back and regenerate the key */
	memcpy(keystatus, undobuff, keystatus_words * 2);
	fd1094_regenerate_key();
	debug_console_printf("Undid last change\n");
}


/*-----------------------------------------------
    execute_fdstatus - handle the 'fdstatus'
    command
-----------------------------------------------*/

static void execute_fdstatus(int ref, int params, const char **param)
{
	int locked = 4;
	int keyaddr;

	/* count how many locked keys we have */
	for (keyaddr = 4; keyaddr < KEY_SIZE; keyaddr++)
		if ((keystatus[keyaddr] & STATUS_MASK) == STATUS_LOCKED)
			locked++;
	debug_console_printf("%d/%d keys locked (%d%%)\n", locked, KEY_SIZE, locked * 100 / KEY_SIZE);
}


/*-----------------------------------------------
    execute_fdcset - handle the 'fdcset'
    command
-----------------------------------------------*/

static void execute_fdcset(int ref, int params, const char **param)
{
	UINT64 pc, value, mask = 0xffff, state = FD1094_STATE_RESET;
	int cnum;

	/* extract the parameters */
	if (!debug_command_parameter_number(param[0], &pc))
		return;
	if (!debug_command_parameter_number(param[1], &value))
		return;
	if (params >= 3 && !debug_command_parameter_number(param[2], &mask))
		return;
	if (params >= 4)
	{
		if (strcmp(param[3], "irq") == 0)
			state = FD1094_STATE_IRQ;
		else if (!debug_command_parameter_number(param[3], &state))
			return;
	}

	/* validate parameters */
	if ((pc & 1) != 0 || pc > 0xffffff)
	{
		debug_console_printf("Invalid PC specified (%08X)\n", (UINT32)pc);
		return;
	}

	/* look for a match and remove any matching constraints */
	for (cnum = 0; cnum < constcount; cnum++)
	{
		/* insert ahead of later constraints */
		if (constraints[cnum].pc > pc)
		{
			memmove(&constraints[cnum + 1], &constraints[cnum], (constcount - cnum) * sizeof(constraints[0]));
			break;
		}

		/* replace matching constraints */
		else if (constraints[cnum].pc == pc)
			break;
	}

	/* set the new constraint and increase the count */
	if (cnum >= constcount || constraints[cnum].pc != pc)
		constcount++;
	set_constraint(&constraints[cnum], pc, state, value, mask);

	/* explain what we did */
	debug_console_printf("Set new constraint at PC=%06X, state=%03X: decrypted & %04X == %04X\n",
			(int)pc, (int)state, (int)mask, (int)value);
}


/*-----------------------------------------------
    execute_fdclist - handle the 'fdclist'
    command
-----------------------------------------------*/

static void execute_fdclist(int ref, int params, const char **param)
{
	int cnum;

	/* loop over constraints and print them */
	for (cnum = 0; cnum < constcount; cnum++)
	{
		fd1094_constraint *constraint = &constraints[cnum];
		debug_console_printf("  PC=%06X, state=%03X: decrypted & %04X == %04X\n",
				constraint->pc, constraint->state, constraint->mask, constraint->value);
	}
}


/*-----------------------------------------------
    execute_fdcsearch - handle the 'fdcsearch'
    command
-----------------------------------------------*/

static void execute_fdcsearch(int ref, int params, const char **param)
{
//  debug_console_printf("Searching for possible global keys....\n");
	perform_constrained_search();
}


/*-----------------------------------------------
    try_all_possibilities - recursively try
    all possible values of the high bits of the
    key at the given address for the specified
    length
-----------------------------------------------*/

static fd1094_possibility *try_all_possibilities(int basepc, int offset, int length, UINT8 *instrbuffer, UINT8 *keybuffer, fd1094_possibility *possdata)
{
	UINT8 keymask, keystat;
	UINT16 possvalue[4];
	UINT8 posskey[4];
	int numposs = 0;
	int decoded;
	int keyaddr;
	int pcaddr;
	int hibit;
	int i;

	/* get the key address and mask */
	pcaddr = basepc/2 + offset;
	keyaddr = addr_to_keyaddr(pcaddr);
	keymask = mask_for_keyaddr(keyaddr);
	keystat = keystatus[pcaddr] & STATUS_MASK;

	/* if the status is 1 (locked) or 2 (doesn't matter), just take the current value */
	if (keystat == STATUS_LOCKED || keystat == STATUS_NOCHANGE)
	{
		posskey[numposs] = keyregion[keyaddr];
		possvalue[numposs++] = fd1094_decode(pcaddr, coderegion[pcaddr], keyregion, 0);
	}

	/* otherwise, iterate over high bits */
	else
	{
		/* remember the original key and iterate over high bits */
		UINT8 origkey = keyregion[keyaddr];
		for (hibit = 0x00; hibit < 0x100; hibit += 0x40)
			if ((hibit & keymask) == keymask)
			{
				/* set the key and decode this word */
				keyregion[keyaddr] = (origkey & STATUS_MASK) | hibit;
				decoded = fd1094_decode(pcaddr, coderegion[pcaddr], keyregion, 0);

				/* see if we already got that value */
				for (i = 0; i < numposs; i++)
					if ((UINT16)decoded == possvalue[i])
						break;

				/* if not, add it to the list */
				if (i == numposs)
				{
					posskey[numposs] = keyregion[keyaddr];
					possvalue[numposs++] = decoded;
				}
			}

		/* restore the original key */
		keyregion[keyaddr] = origkey;

		/* if there was only one possibility, then mark it as "doesn't matter" */
		if (numposs == 1)
		{
			keystatus[pcaddr] = (keystatus[pcaddr] & ~STATUS_MASK) | STATUS_NOCHANGE;
			keydirty = TRUE;
		}
	}

	/* now iterate over our possible values */
	for (i = 0; i < numposs; i++)
	{
		/* set the instruction buffer */
		instrbuffer[offset*2 + 0] = possvalue[i] >> 8;
		instrbuffer[offset*2 + 1] = possvalue[i];
		keybuffer[offset] = posskey[i];

		/* if our length is 0, we need to do a quick dasm to see how long our length is */
		if (offset == 0)
		{
			/* first make sure we are a valid instruction */
			if ((possvalue[i] & 0xf000) == 0xa000 || (possvalue[i] & 0xf000) == 0xf000)
				continue;
			if (!m68k_is_valid_instruction(possvalue[i], M68K_CPU_TYPE_68000))
				continue;

			/* determine the length by disassembling the incomplete instruction */
			length = get_opcode_length(possvalue[i]);
		}

		/* if our offset is 1, and the previous instruction is a word-sized branch, make sure */
		/* we're not odd */
//      if (offset == 1 &&

		/* if we're not at our target length, recursively call ourselves */
		if (offset < length - 1)
			possdata = try_all_possibilities(basepc, offset + 1, length, instrbuffer, keybuffer, possdata);

		/* otherwise, output what we have */
		else
		{
			possdata->basepc = basepc;
			possdata->length = length;
			m68k_disassemble_raw(possdata->dasm, basepc, instrbuffer, instrbuffer, M68K_CPU_TYPE_68000);
			memcpy(possdata->instrbuffer, instrbuffer, sizeof(possdata->instrbuffer));
			memcpy(possdata->keybuffer, keybuffer, sizeof(possdata->keybuffer));
			possdata++;
		}
	}

	return possdata;
}


/*-----------------------------------------------
    tag_possibility - tag a given possibility
    with the specified status
-----------------------------------------------*/

static void tag_possibility(fd1094_possibility *possdata, UINT8 status)
{
	int curfdstate = fd1094_set_state(keyregion, -1);
	int pcoffs;

	/* iterate over words in the opcode */
	for (pcoffs = 0; pcoffs < possdata->length; pcoffs++)
	{
		int pcaddr = possdata->basepc/2 + pcoffs;
		int keyaddr = addr_to_keyaddr(pcaddr);
		int keystat = keystatus[pcaddr] & STATUS_MASK;

		/* if the status doesn't match and isn't "no change", then set the status */
		if (keystat != STATUS_NOCHANGE && keystat != status)
		{
			keystatus[keyaddr] = (keystatus[keyaddr] & ~HIBITS_MASK) | (possdata->keybuffer[pcoffs] & HIBITS_MASK);
			keystatus[pcaddr] = (curfdstate << 8) | (keystatus[pcaddr] & HIBITS_MASK) | status;
			keydirty = TRUE;
		}
		else
			keystatus[pcaddr] = (curfdstate << 8) | (keystatus[pcaddr] & ~STATE_MASK);
	}
}


/*-----------------------------------------------
    perform_constrained_search - look for
    the next global key that will match the
    given sequence/mask pair
-----------------------------------------------*/

static void perform_constrained_search(void)
{
	UINT32 global;

	/* ensure our first 4 constraints are what we expect */
	assert(constraints[0].pc == 0x000000);
	assert(constraints[1].pc == 0x000002);
	assert(constraints[2].pc == 0x000004);
	assert(constraints[3].pc == 0x000006);

	/* start with a 0 global key and brute force from there */
	global = 0;

	/* loop until we run out of possibilities */
	while (1)
	{
		UINT16 output[4];
		int numseeds;

		/* look for the next global key match */
		global = find_global_key_matches(global + 1, output);
		if (global == 0)
			break;
//      debug_console_printf("Checking global key %08X (PC=%06X)....\n", global, (output[2] << 16) | output[3]);

		/* use the IRQ handler to find more possibilities */
		numseeds = find_constraint_sequence(global, FALSE);
		if (numseeds > 0)
		{
			int i;
			for (i = 0; i < numseeds; i++)
				debug_console_printf("  Possible: global=%08X seed=%06X pc=%04X\n", global, possible_seed[i], output[3]);
		}
	}
}


/*-----------------------------------------------
    find_global_key_matches - look for
    the next global key that will match the
    given sequence/mask pair
-----------------------------------------------*/

static UINT32 find_global_key_matches(UINT32 startwith, UINT16 *output)
{
	int key0, key1, key2, key3;
	UINT8 key[4];

	/* iterate over the first key byte, allowing all possible values */
	for (key0 = (startwith >> 24) & 0xff; key0 < 256; key0++)
	{
		/* set the key and reset the fd1094 */
		key[0] = key0;
		startwith &= 0x00ffffff;
		fd1094_set_state(key, FD1094_STATE_RESET);

		/* if we match, iterate over the second key byte */
		output[0] = fd1094_decode(0x000000, coderegion[0], key, TRUE);
		if ((output[0] & constraints[0].mask) == constraints[0].value)

			/* iterate over the second key byte, limiting the scope to known valid keys */
			for (key1 = (startwith >> 16) & 0xff; key1 < 256; key1++)
				if ((key1 & 0xf8) == 0xa8 || (key1 & 0xf8) == 0xf8)
				{
					/* set the key and reset the fd1094 */
					key[1] = key1;
					startwith &= 0x0000ffff;
					fd1094_set_state(key, FD1094_STATE_RESET);

					/* if we match, iterate over the third key byte */
					output[1] = fd1094_decode(0x000001, coderegion[1], key, TRUE);
					if ((output[1] & constraints[1].mask) == constraints[1].value)

						/* iterate over the third key byte, limiting the scope to known valid keys */
						for (key2 = (startwith >> 8) & 0xff; key2 < 256; key2++)
							if ((key2 & 0xc0) == 0xc0)
							{
								/* set the key and reset the fd1094 */
								key[2] = key2;
								startwith &= 0x000000ff;
								fd1094_set_state(key, FD1094_STATE_RESET);

								/* if we match, iterate over the fourth key byte */
								output[2] = fd1094_decode(0x000002, coderegion[2], key, TRUE);
								if ((output[2] & constraints[2].mask) == constraints[2].value)

									/* iterate over the fourth key byte, limiting the scope to known valid keys */
									for (key3 = (startwith >> 0) & 0xff; key3 < 256; key3++)
										if ((key3 & 0xc0) == 0xc0)
										{
											/* set the key and reset the fd1094 */
											key[3] = key3;
											startwith = 0;
											fd1094_set_state(key, FD1094_STATE_RESET);

											/* if we match, return the value */
											output[3] = fd1094_decode(0x000003, coderegion[3], key, TRUE);
											if ((output[3] & constraints[3].mask) == constraints[3].value)
												return (key0 << 24) | (key1 << 16) | (key2 << 8) | key3;
										}
							}
				}
	}
	return 0;
}


/*-----------------------------------------------
    find_constraint_sequence - look for a
    sequence of decoded words at the given
    address, and optionally verify that there
    are valid PRNG keys that could generate the
    results
-----------------------------------------------*/

static int find_constraint_sequence(UINT32 global, int quick)
{
	const fd1094_constraint *minkeyaddr = &constraints[4];
	const fd1094_constraint *maxkeyaddr = &constraints[4];
	const fd1094_constraint *curr;
	int keyvalue, keyaddr, keysneeded;
	int seedcount = 0;
	UINT16 decrypted;
	UINT8 key[8192];
	UINT8 keymask;
	offs_t pcaddr;

	/* if we don't have any extra constraints, we're good */
	if (constcount <= 4)
		return -1;

	/* set the global key */
	key[0] = global >> 24;
	key[1] = global >> 16;
	key[2] = global >> 8;
	key[3] = global >> 0;
	fd1094_set_state(key, -1);

	/* first see if it is even possible, regardless of PRNG */
	for (curr = &constraints[4]; curr < &constraints[constcount]; curr++)
	{
		/* get the key address and value for this offset */
		pcaddr = curr->pc / 2;
		keyaddr = addr_to_keyaddr(pcaddr);
		keymask = mask_for_keyaddr(keyaddr);

		/* track the minumum and maximum key addresses, but only for interesting combinations */
		if ((coderegion[pcaddr] & 0xe000) != 0x0000)
		{
			if (keyaddr < addr_to_keyaddr(minkeyaddr->pc / 2))
				minkeyaddr = curr;
			if (keyaddr > addr_to_keyaddr(maxkeyaddr->pc / 2))
				maxkeyaddr = curr;
		}

		/* set the state */
		fd1094_set_state(key, curr->state);

		/* brute force search this byte */
		for (keyvalue = 0; keyvalue < 256; keyvalue++)
			if ((keyvalue & keymask) == keymask)
			{
				/* see if this works */
				key[keyaddr] = keyvalue;
				decrypted = fd1094_decode(pcaddr, coderegion[pcaddr], key, FALSE);

				/* if we got a match, stop; we're done */
				if ((decrypted & curr->mask) == curr->value)
					break;
			}

		/* if we failed, we're done */
		if (keyvalue == 256)
			return 0;
	}

	/* if we're quick, that's all the checking we do */
	if (quick)
		return -1;

	/* determine how many keys we need to cover our whole range */
	keysneeded = addr_to_keyaddr(maxkeyaddr->pc / 2) + 1 - addr_to_keyaddr(minkeyaddr->pc / 2);

	/* now do the more thorough search */
	pcaddr = minkeyaddr->pc / 2;
	keyaddr = addr_to_keyaddr(pcaddr);
	keymask = mask_for_keyaddr(keyaddr);

	/* set the state */
	fd1094_set_state(key, minkeyaddr->state);

	/* brute force search the first byte key of the key */
	for (keyvalue = 0; keyvalue < 256; keyvalue++)
		if ((keyvalue & keymask) == keymask)
		{
			/* see if this works */
			key[keyaddr] = keyvalue;
			decrypted = fd1094_decode(pcaddr, coderegion[pcaddr], key, FALSE);

			/* if we got a match, then iterate over all possible PRNG sequences starting with this */
			if ((decrypted & minkeyaddr->mask) == minkeyaddr->value)
			{
				UINT32 seedlow;

//              debug_console_printf("Global %08X ... Looking for keys that generate a keyvalue of %02X at %04X\n",
//                      global, keyvalue, keyaddr);

				/* iterate over seed possibilities */
				for (seedlow = 0; seedlow < (1 << 16); seedlow++)
				{
					/* start with the known upper bits together with the 16 guessed lower bits */
					UINT32 seedstart = (~keyvalue << 16) | seedlow;

					/* generate data starting with this seed into the key */
					generate_key_bytes(key, keyaddr + 1, keysneeded - 1, seedstart);

					/* if the whole thing matched, record the match */
					if (does_key_work_for_constraints(coderegion, key))
					{
						seedstart = reconstruct_base_seed(keyaddr, seedstart);
						if ((seedstart & 0x3fffff) != 0)
							possible_seed[seedcount++] = seedstart;
					}
				}
			}
		}

	return seedcount;
}


/*-----------------------------------------------
    does_key_work_for_constraints - return true
    if the given key might work for a given set
    of constraints
-----------------------------------------------*/

static int does_key_work_for_constraints(const UINT16 *base, UINT8 *key)
{
	const fd1094_constraint *curr;
	UINT16 decrypted;

	/* iterate over the sequence */
	for (curr = &constraints[4]; curr < &constraints[constcount]; curr++)
	{
		offs_t pcaddr = curr->pc / 2;
		int keyaddr = addr_to_keyaddr(pcaddr);
		UINT8 keymask = mask_for_keyaddr(keyaddr);
		int hibits;

		/* set the state */
		fd1094_set_state(key, curr->state);

		/* iterate over high bits (1 per byte) */
		for (hibits = 0; hibits < 0x100; hibits += 0x40)
			if ((hibits & keymask) == keymask)
			{
				/* update the key bits */
				key[keyaddr] = (key[keyaddr] & ~0xc0) | hibits;

				/* decrypt using this key; stop if we get a match */
				decrypted = fd1094_decode(pcaddr, base[pcaddr], key, FALSE);
				if ((decrypted & curr->mask) == curr->value)
					break;
			}

		/* if we failed to match, we're done */
		if (hibits >= 0x100)
			return FALSE;
	}

	/* got a match on all entries */
	return TRUE;
}


/*-----------------------------------------------
    reconstruct_base_seed - given the seed
    value at a particular key address, return
    the seed that would be used to generate the
    first key value (at offset 4)
-----------------------------------------------*/

static UINT32 reconstruct_base_seed(int keybaseaddr, UINT32 startseed)
{
	UINT32 seed = startseed;
	UINT32 window[8192];
	int index = 0;

	/* keep generating, starting from the start seed until we re-generate the start seed */
	/* note that some sequences are smaller than the window, so we also have to ensure */
	/* that we generate at least one full window's worth of data */
	do
	{
        seed = seed * 0x29;
        seed += seed << 16;
        window[index++ % ARRAY_LENGTH(window)] = seed;
    } while (((startseed ^ seed) & 0x3fffff) != 0 || index < ARRAY_LENGTH(window));

    /* when we break, we have overshot */
    index--;

    /* back up to where we would have been at address 3 */
    index -= keybaseaddr - 3;
    if (index < 0)
    	index += ARRAY_LENGTH(window);

    /* return the value from the window at that location */
    return window[index % ARRAY_LENGTH(window)] & 0x3fffff;
}

#endif
