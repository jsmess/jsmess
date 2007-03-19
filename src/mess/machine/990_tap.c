/*
	990_tap.c: emulation of a generic ti990 tape controller, for use with
	TILINE-based TI990 systems (TI990/10, /12, /12LR, /10A, Business system 300
	and 300A).

	This core will emulate the common feature set found in every tape controller.
	Most controllers support additional features, but are still compatible with
	the basic feature set.  I have a little documentation on two specific
	tape controllers (MT3200 and WD800/WD800A), but I have not tried to emulate
	controller-specific features.


	Long description: see 2234398-9701 and 2306140-9701.


	Raphael Nabet 2002
*/
/*
	Image encoding:


	2 bytes: record len - little-endian
	2 bytes: always 0s (length MSBs?)
	len bytes: data
	2 bytes: record len - little-endian
	2 bytes: always 0s (length MSBs?)

	4 0s: EOF mark
*/

#include "driver.h"

#include "990_tap.h"
#include "image.h"

static void update_interrupt(void);

#define MAX_TAPE_UNIT 4

typedef struct tape_unit_t
{
	mess_image *img;		/* image descriptor */
	unsigned int bot : 1;	/* TRUE if we are at the beginning of tape */
	unsigned int eot : 1;	/* TRUE if we are at the end of tape */
	unsigned int wp : 1;	/* TRUE if tape is write-protected */
} tape_unit_t;

typedef struct tpc_t
{
	UINT16 w[8];

	void (*interrupt_callback)(int state);

	tape_unit_t t[MAX_TAPE_UNIT];
} tpc_t;

enum
{
	w0_offline			= 0x8000,
	w0_BOT				= 0x4000,
	w0_EOR				= 0x2000,
	w0_EOF				= 0x1000,
	w0_EOT				= 0x0800,
	w0_write_ring		= 0x0400,
	w0_tape_rewinding	= 0x0200,
	w0_command_timeout	= 0x0100,

	w0_rewind_status	= 0x00f0,
	w0_rewind_mask		= 0x000f,

	w6_unit0_sel		= 0x8000,
	w6_unit1_sel		= 0x4000,
	w6_unit2_sel		= 0x2000,
	w6_unit3_sel		= 0x1000,
	w6_command			= 0x0f00,

	w7_idle				= 0x8000,
	w7_complete			= 0x4000,
	w7_error			= 0x2000,
	w7_int_enable		= 0x1000,
	w7_PE_format		= 0x0200,
	w7_abnormal_completion	= 0x0100,
	w7_interface_parity_err	= 0x0080,
	w7_err_correction_enabled	= 0x0040,
	w7_hard_error			= 0x0020,
	w7_tiline_parity_err	= 0x0010,
	w7_tiline_timing_err	= 0x0008,
	w7_tiline_timeout_err	= 0x0004,
	/*w7_format_error		= 0x0002,*/
	w7_tape_error		= 0x0001
};

static const UINT16 w_mask[8] =
{
	0x000f,		/* Controllers should prevent overwriting of w0 status bits, and I know
				that some controllers do so. */
	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xf3ff		/* Don't overwrite reserved bits */
};

static tpc_t tpc;


DEVICE_INIT( ti990_tape )
{
	tape_unit_t *t;
	int id = image_index_in_device(image);


	if ((id < 0) || (id >= MAX_TAPE_UNIT))
		return INIT_FAIL;

	t = &tpc.t[id];
	memset(t, 0, sizeof(*t));

	t->img = image;
	t->wp = 1;
	t->bot = 0;
	t->eot = 0;

	return INIT_PASS;
}

/*DEVICE_EXIT( ti990_tape )
{
	d->img = NULL;
}*/

/*
	Open a tape image
*/
DEVICE_LOAD( ti990_tape )
{
	tape_unit_t *t;
	int id = image_index_in_device(image);


	if ((id < 0) || (id >= MAX_TAPE_UNIT))
		return INIT_FAIL;

	t = &tpc.t[id];
	memset(t, 0, sizeof(*t));

	/* tell whether the image is writable */
	t->wp = ! image_is_writable(image);

	t->bot = 1;

	return INIT_PASS;
}

/*
	Close a tape image
*/
DEVICE_UNLOAD( ti990_tape )
{
	tape_unit_t *t;
	int id = image_index_in_device(image);

	if ((id < 0) || (id >= MAX_TAPE_UNIT))
		return;

	t = &tpc.t[id];
	t->wp = 1;
	t->bot = 0;
	t->eot = 0;
}

/*
	Init the tape controller core
*/
void ti990_tpc_init(void (*interrupt_callback)(int state))
{
	memset(tpc.w, 0, sizeof(tpc.w));
	/* The PE bit is always set for the MT3200 (but not MT1600) */
	/* According to MT3200 manual, w7 bit #4 (reserved) is always set */
	tpc.w[7] = w7_idle /*| w7_PE_format*/ | 0x0800;

	tpc.interrupt_callback = interrupt_callback;

	update_interrupt();
}

/*
	Clean-up the tape controller core
*/
/*void ti990_tpc_exit(void)
{
}*/

/*
	Parse the tape select lines, and return the corresponding tape unit.
	(-1 if none)
*/
static int cur_tape_unit(void)
{
	int reply;


	if (tpc.w[6] & w6_unit0_sel)
		reply = 0;
	else if (tpc.w[6] & w6_unit1_sel)
		reply = 1;
	else if (tpc.w[6] & w6_unit2_sel)
		reply = 2;
	else if (tpc.w[6] & w6_unit3_sel)
		reply = 3;
	else
		reply = -1;

	if (reply >= MAX_TAPE_UNIT)
		reply = -1;

	return reply;
}

/*
	Update interrupt state
*/
static void update_interrupt(void)
{
	if (tpc.interrupt_callback)
		(*tpc.interrupt_callback)((tpc.w[7] & w7_idle)
									&& (((tpc.w[7] & w7_int_enable) && (tpc.w[7] & (w7_complete | w7_error)))
										|| ((tpc.w[0] & ~(tpc.w[0] >> 4)) & w0_rewind_mask)));
}

/*
	Handle the read binary forward command: read the next record on tape.
*/
static void cmd_read_binary_forward(void)
{
	UINT8 buffer[256];
	int reclen;

	int dma_address;
	int char_count;
	int read_offset;

	int rec_count = 0;
	int chunk_len;
	int bytes_to_read;
	int bytes_read;
	int i;

	int tap_sel = cur_tape_unit();

	if (tap_sel == -1)
	{
		/* No idea what to report... */
		tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
		update_interrupt();
		return;
	}
	else if (! image_exists(tpc.t[tap_sel].img))
	{	/* offline */
		tpc.w[0] |= w0_offline;
		tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
		update_interrupt();
		return;
	}
#if 0
	else if (0)
	{	/* rewind in progress */
		tpc.w[0] |= 0x80 >> tap_sel;
		tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
		update_interrupt();
		return;
	}
#endif

	tpc.t[tap_sel].bot = 0;

	dma_address = ((((int) tpc.w[6]) << 16) | tpc.w[5]) & 0x1ffffe;
	char_count = tpc.w[4];
	read_offset = tpc.w[3];

	bytes_read = image_fread(tpc.t[tap_sel].img, buffer, 4);
	if (bytes_read != 4)
	{
		if (bytes_read == 0)
		{	/* legitimate EOF */
			tpc.t[tap_sel].eot = 1;
			tpc.w[0] |= w0_EOT;	/* or should it be w0_command_timeout? */
			tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
			update_interrupt();
			goto update_registers;
		}
		else
		{	/* illegitimate EOF */
			/* No idea what to report... */
			/* eject tape to avoid catastrophes */
			logerror("Tape error\n");
			image_unload(tpc.t[tap_sel].img);
			tpc.w[0] |= w0_offline;
			tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
			update_interrupt();
			goto update_registers;
		}
	}
	reclen = (((int) buffer[1]) << 8) | buffer[0];
	if (buffer[2] || buffer[3])
	{	/* no idea what these bytes mean */
		logerror("Tape error\n");
		logerror("Tape format looks gooofy\n");
		/* eject tape to avoid catastrophes */
		image_unload(tpc.t[tap_sel].img);
		tpc.w[0] |= w0_offline;
		tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
		update_interrupt();
		goto update_registers;
	}

	/* test for EOF mark */
	if (reclen == 0)
	{
		logerror("read binary forward: found EOF, requested %d\n", char_count);
		tpc.w[0] |= w0_EOF;
		tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
		update_interrupt();
		goto update_registers;
	}

	logerror("read binary forward: rec lenght %d, requested %d\n", reclen, char_count);

	rec_count = reclen;

	/* skip up to read_offset bytes */
	chunk_len = (read_offset > rec_count) ? rec_count : read_offset;

	if (image_fseek(tpc.t[tap_sel].img, chunk_len, SEEK_CUR))
	{	/* eject tape */
		logerror("Tape error\n");
		image_unload(tpc.t[tap_sel].img);
		tpc.w[0] |= w0_offline;
		tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
		update_interrupt();
		goto update_registers;
	}

	rec_count -= chunk_len;
	read_offset -= chunk_len;
	if (read_offset)
	{
		tpc.w[0] |= w0_EOR;
		tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
		update_interrupt();
		goto skip_trailer;
	}

	/* read up to char_count bytes */
	chunk_len = (char_count > rec_count) ? rec_count : char_count;

	for (; chunk_len>0; )
	{
		bytes_to_read = (chunk_len < sizeof(buffer)) ? chunk_len : sizeof(buffer);
		bytes_read = image_fread(tpc.t[tap_sel].img, buffer, bytes_to_read);

		if (bytes_read & 1)
		{
			buffer[bytes_read] = 0xff;
		}

		/* DMA */
		for (i=0; i<bytes_read; i+=2)
		{
			program_write_word_16be(dma_address, (((int) buffer[i]) << 8) | buffer[i+1]);
			dma_address = (dma_address + 2) & 0x1ffffe;
		}

		rec_count -= bytes_read;
		char_count -= bytes_read;
		chunk_len -= bytes_read;

		if (bytes_read != bytes_to_read)
		{	/* eject tape */
			logerror("Tape error\n");
			image_unload(tpc.t[tap_sel].img);
			tpc.w[0] |= w0_offline;
			tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
			update_interrupt();
			goto update_registers;
		}
	}

	if (char_count)
	{
		tpc.w[0] |= w0_EOR;
		tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
		update_interrupt();
		goto skip_trailer;
	}

	if (rec_count)
	{	/* skip end of record */
		if (image_fseek(tpc.t[tap_sel].img, rec_count, SEEK_CUR))
		{	/* eject tape */
			logerror("Tape error\n");
			image_unload(tpc.t[tap_sel].img);
			tpc.w[0] |= w0_offline;
			tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
			update_interrupt();
			goto update_registers;
		}
	}

skip_trailer:
	if (image_fread(tpc.t[tap_sel].img, buffer, 4) != 4)
	{	/* eject tape */
		logerror("Tape error\n");
		image_unload(tpc.t[tap_sel].img);
		tpc.w[0] |= w0_offline;
		tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
		update_interrupt();
		goto update_registers;
	}

	if (reclen != ((((int) buffer[1]) << 8) | buffer[0]))
	{	/* eject tape */
		logerror("Tape error\n");
		image_unload(tpc.t[tap_sel].img);
		tpc.w[0] |= w0_offline;
		tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
		update_interrupt();
		goto update_registers;
	}
	if (buffer[2] || buffer[3])
	{	/* no idea what these bytes mean */
		logerror("Tape error\n");
		logerror("Tape format looks gooofy\n");
		/* eject tape to avoid catastrophes */
		image_unload(tpc.t[tap_sel].img);
		tpc.w[0] |= w0_offline;
		tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
		update_interrupt();
		goto update_registers;
	}

	if (! (tpc.w[7] & w7_error))
	{
		tpc.w[7] |= w7_idle | w7_complete;
		update_interrupt();
	}

update_registers:
	tpc.w[1] = rec_count & 0xffff;
	tpc.w[2] = (rec_count >> 8) & 0xff;
	tpc.w[3] = read_offset;
	tpc.w[4] = char_count;
	tpc.w[5] = (dma_address >> 1) & 0xffff;
	tpc.w[6] = (tpc.w[6] & 0xffe0) | ((dma_address >> 17) & 0xf);
}

/*
	Handle the record skip forward command: skip a specified number of records.
*/
static void cmd_record_skip_forward(void)
{
	UINT8 buffer[4];
	int reclen;

	int record_count;
	int bytes_read;

	int tap_sel = cur_tape_unit();

	if (tap_sel == -1)
	{
		/* No idea what to report... */
		tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
		update_interrupt();
		return;
	}
	else if (! image_exists(tpc.t[tap_sel].img))
	{	/* offline */
		tpc.w[0] |= w0_offline;
		tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
		update_interrupt();
		return;
	}
#if 0
	else if (0)
	{	/* rewind in progress */
		tpc.w[0] |= 0x80 >> tap_sel;
		tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
		update_interrupt();
		return;
	}
#endif

	record_count = tpc.w[4];

	if (record_count)
		tpc.t[tap_sel].bot = 0;

	while (record_count > 0)
	{
		bytes_read = image_fread(tpc.t[tap_sel].img, buffer, 4);
		if (bytes_read != 4)
		{
			if (bytes_read == 0)
			{	/* legitimate EOF */
				tpc.t[tap_sel].eot = 1;
				tpc.w[0] |= w0_EOT;	/* or should it be w0_command_timeout? */
				tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
				update_interrupt();
				goto update_registers;
			}
			else
			{	/* illegitimate EOF */
				/* No idea what to report... */
				/* eject tape to avoid catastrophes */
				image_unload(tpc.t[tap_sel].img);
				tpc.w[0] |= w0_offline;
				tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
				update_interrupt();
				goto update_registers;
			}
		}
		reclen = (((int) buffer[1]) << 8) | buffer[0];
		if (buffer[2] || buffer[3])
		{	/* no idea what these bytes mean */
			logerror("Tape format looks gooofy\n");
			/* eject tape to avoid catastrophes */
			image_unload(tpc.t[tap_sel].img);
			tpc.w[0] |= w0_offline;
			tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
			update_interrupt();
			goto update_registers;
		}

		/* test for EOF mark */
		if (reclen == 0)
		{
			logerror("record skip forward: found EOF\n");
			tpc.w[0] |= w0_EOF;
			tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
			update_interrupt();
			goto update_registers;
		}

		/* skip record data */
		if (image_fseek(tpc.t[tap_sel].img, reclen, SEEK_CUR))
		{	/* eject tape */
			image_unload(tpc.t[tap_sel].img);
			tpc.w[0] |= w0_offline;
			tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
			update_interrupt();
			goto update_registers;
		}

		if (image_fread(tpc.t[tap_sel].img, buffer, 4) != 4)
		{	/* eject tape */
			image_unload(tpc.t[tap_sel].img);
			tpc.w[0] |= w0_offline;
			tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
			update_interrupt();
			goto update_registers;
		}

		if (reclen != ((((int) buffer[1]) << 8) | buffer[0]))
		{	/* eject tape */
			image_unload(tpc.t[tap_sel].img);
			tpc.w[0] |= w0_offline;
			tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
			update_interrupt();
			goto update_registers;
		}
		if (buffer[2] || buffer[3])
		{	/* no idea what these bytes mean */
			logerror("Tape format looks gooofy\n");
			/* eject tape to avoid catastrophes */
			image_unload(tpc.t[tap_sel].img);
			tpc.w[0] |= w0_offline;
			tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
			update_interrupt();
			goto update_registers;
		}

		record_count--;
	}

	tpc.w[7] |= w7_idle | w7_complete;
	update_interrupt();

update_registers:
	tpc.w[4] = record_count;
}

/*
	Handle the record skip reverse command: skip a specified number of records backwards.
*/
static void cmd_record_skip_reverse(void)
{
	UINT8 buffer[4];
	int reclen;

	int record_count;

	int bytes_read;

	int tap_sel = cur_tape_unit();

	if (tap_sel == -1)
	{
		/* No idea what to report... */
		tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
		update_interrupt();
		return;
	}
	else if (! image_exists(tpc.t[tap_sel].img))
	{	/* offline */
		tpc.w[0] |= w0_offline;
		tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
		update_interrupt();
		return;
	}
#if 0
	else if (0)
	{	/* rewind in progress */
		tpc.w[0] |= 0x80 >> tap_sel;
		tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
		update_interrupt();
		return;
	}
#endif

	record_count = tpc.w[4];

	if (record_count)
		tpc.t[tap_sel].eot = 0;

	while (record_count > 0)
	{
		if (image_ftell(tpc.t[tap_sel].img) == 0)
		{	/* bot */
			tpc.t[tap_sel].bot = 1;
			tpc.w[0] |= w0_BOT;
			tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
			update_interrupt();
			goto update_registers;
		}
		if (image_fseek(tpc.t[tap_sel].img, -4, SEEK_CUR))
		{	/* eject tape */
			image_unload(tpc.t[tap_sel].img);
			tpc.w[0] |= w0_offline;
			tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
			update_interrupt();
			goto update_registers;
		}
		bytes_read = image_fread(tpc.t[tap_sel].img, buffer, 4);
		if (bytes_read != 4)
		{
			/* illegitimate EOF */
			/* No idea what to report... */
			/* eject tape to avoid catastrophes */
			image_unload(tpc.t[tap_sel].img);
			tpc.w[0] |= w0_offline;
			tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
			update_interrupt();
			goto update_registers;
		}
		reclen = (((int) buffer[1]) << 8) | buffer[0];
		if (buffer[2] || buffer[3])
		{	/* no idea what these bytes mean */
			logerror("Tape format looks gooofy\n");
			/* eject tape to avoid catastrophes */
			image_unload(tpc.t[tap_sel].img);
			tpc.w[0] |= w0_offline;
			tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
			update_interrupt();
			goto update_registers;
		}

		/* look for EOF mark */
		if (reclen == 0)
		{
			logerror("record skip reverse: found EOF\n");
			if (image_fseek(tpc.t[tap_sel].img, -4, SEEK_CUR))
			{	/* eject tape */
				image_unload(tpc.t[tap_sel].img);
				tpc.w[0] |= w0_offline;
				tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
				update_interrupt();
				goto update_registers;
			}
			tpc.w[0] |= w0_EOF;
			tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
			update_interrupt();
			goto update_registers;
		}

		if (image_fseek(tpc.t[tap_sel].img, -reclen-8, SEEK_CUR))
		{	/* eject tape */
			image_unload(tpc.t[tap_sel].img);
			tpc.w[0] |= w0_offline;
			tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
			update_interrupt();
			goto update_registers;
		}

		if (image_fread(tpc.t[tap_sel].img, buffer, 4) != 4)
		{	/* eject tape */
			image_unload(tpc.t[tap_sel].img);
			tpc.w[0] |= w0_offline;
			tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
			update_interrupt();
			goto update_registers;
		}
		if (reclen != ((((int) buffer[1]) << 8) | buffer[0]))
		{	/* eject tape */
			image_unload(tpc.t[tap_sel].img);
			tpc.w[0] |= w0_offline;
			tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
			update_interrupt();
			goto update_registers;
		}
		if (buffer[2] || buffer[3])
		{	/* no idea what these bytes mean */
			logerror("Tape format looks gooofy\n");
			/* eject tape to avoid catastrophes */
			image_unload(tpc.t[tap_sel].img);
			tpc.w[0] |= w0_offline;
			tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
			update_interrupt();
			goto update_registers;
		}

		if (image_fseek(tpc.t[tap_sel].img, -4, SEEK_CUR))
		{	/* eject tape */
			image_unload(tpc.t[tap_sel].img);
			tpc.w[0] |= w0_offline;
			tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
			update_interrupt();
			goto update_registers;
		}

		record_count--;
	}

	tpc.w[7] |= w7_idle | w7_complete;
	update_interrupt();

update_registers:
	tpc.w[4] = record_count;
}

/*
	Handle the rewind command: rewind to BOT.
*/
static void cmd_rewind(void)
{
	int tap_sel = cur_tape_unit();

	if (tap_sel == -1)
	{
		/* No idea what to report... */
		tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
		update_interrupt();
		return;
	}
	else if (! image_exists(tpc.t[tap_sel].img))
	{	/* offline */
		tpc.w[0] |= w0_offline;
		tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
		update_interrupt();
		return;
	}
#if 0
	else if (0)
	{	/* rewind in progress */
		tpc.w[0] |= 0x80 >> tap_sel;
		tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
		update_interrupt();
		return;
	}
#endif

	tpc.t[tap_sel].eot = 0;

	if (image_fseek(tpc.t[tap_sel].img, 0, SEEK_SET))
	{	/* eject tape */
		image_unload(tpc.t[tap_sel].img);
		tpc.w[0] |= w0_offline;
		tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
		update_interrupt();
		return;
	}
	tpc.t[tap_sel].bot = 1;

	tpc.w[7] |= w7_idle | w7_complete;
	update_interrupt();
}

/*
	Handle the rewind and offline command: disable the tape unit.
*/
static void cmd_rewind_and_offline(void)
{
	int tap_sel = cur_tape_unit();

	if (tap_sel == -1)
	{
		/* No idea what to report... */
		tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
		update_interrupt();
		return;
	}
	else if (! image_exists(tpc.t[tap_sel].img))
	{	/* offline */
		tpc.w[0] |= w0_offline;
		tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
		update_interrupt();
		return;
	}
#if 0
	else if (0)
	{	/* rewind in progress */
		tpc.w[0] |= 0x80 >> tap_sel;
		tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
		update_interrupt();
		return;
	}
#endif

	/* eject tape */
	image_unload(tpc.t[tap_sel].img);

	tpc.w[7] |= w7_idle | w7_complete;
	update_interrupt();
}

/*
	Handle the read transport status command: return the current tape status.
*/
static void read_transport_status(void)
{
	int tap_sel = cur_tape_unit();

	if (tap_sel == -1)
	{
		/* No idea what to report... */
		tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
		update_interrupt();
	}
	else if (! image_exists(tpc.t[tap_sel].img))
	{	/* offline */
		tpc.w[0] |= w0_offline;
		tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
		update_interrupt();
	}
#if 0
	else if (0)
	{	/* rewind in progress */
		tpc.w[0] |= /*...*/;
		tpc.w[7] |= w7_idle | w7_error | w7_tape_error;
		update_interrupt();
	}
#endif
	else
	{	/* no particular error condition */
		if (tpc.t[tap_sel].bot)
			tpc.w[0] |= w0_BOT;
		if (tpc.t[tap_sel].eot)
			tpc.w[0] |= w0_EOT;
		if (tpc.t[tap_sel].wp)
			tpc.w[0] |= w0_write_ring;
		tpc.w[7] |= w7_idle | w7_complete;
		update_interrupt();
	}
}

/*
	Parse command code and execute the command.
*/
static void execute_command(void)
{
	/* hack */
	tpc.w[0] &= 0xff;

	switch ((tpc.w[6] & w6_command) >> 8)
	{
	case 0x00:
	case 0x0C:
	case 0x0E:
		/* NOP */
		logerror("NOP\n");
		tpc.w[7] |= w7_idle | w7_complete;
		update_interrupt();
		break;
	case 0x01:
		/* buffer sync: means nothing under emulation */
		logerror("buffer sync\n");
		tpc.w[7] |= w7_idle | w7_complete;
		update_interrupt();
		break;
	case 0x02:
		/* write EOF - not emulated */
		logerror("write EOF\n");
		/* ... */
		tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
		update_interrupt();
		break;
	case 0x03:
		/* record skip reverse - not fully tested */
		logerror("record skip reverse\n");
		cmd_record_skip_reverse();
		break;
	case 0x04:
		/* read binary forward */
		logerror("read binary forward\n");
		cmd_read_binary_forward();
		break;
	case 0x05:
		/* record skip forward - not tested */
		logerror("record skip forward\n");
		cmd_record_skip_forward();
		break;
	case 0x06:
		/* write binary forward - not emulated */
		logerror("write binary forward\n");
		/* ... */
		tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
		update_interrupt();
		break;
	case 0x07:
		/* erase - not emulated */
		logerror("erase\n");
		/* ... */
		tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
		update_interrupt();
		break;
	case 0x08:
	case 0x09:
		/* read transport status */
		logerror("read transport status\n");
		read_transport_status();
		break;
	case 0x0A:
		/* rewind - not tested */
		logerror("rewind\n");
		cmd_rewind();
		break;
	case 0x0B:
		/* rewind and offline - not tested */
		logerror("rewind and offline\n");
		cmd_rewind_and_offline();
		break;
	case 0x0F:
		/* extended control and status - not emulated */
		logerror("extended control and status\n");
		/* ... */
		tpc.w[7] |= w7_idle | w7_error | w7_hard_error;
		update_interrupt();
		break;
	}
}


/*
	Read one register in TPCS space
*/
READ16_HANDLER(ti990_tpc_r)
{
	if ((offset >= 0) && (offset < 8))
		return tpc.w[offset];
	else
		return 0;
}

/*
	Write one register in TPCS space.  Execute command if w7_idle is cleared.
*/
WRITE16_HANDLER(ti990_tpc_w)
{
	if ((offset >= 0) && (offset < 8))
	{
		/* write protect if a command is in progress */
		if (tpc.w[7] & w7_idle)
		{
			UINT16 old_data = tpc.w[offset];

			/* Only write writable bits AND honor byte accesses (ha!) */
			tpc.w[offset] = (tpc.w[offset] & ((~w_mask[offset]) | mem_mask)) | (data & w_mask[offset] & ~mem_mask);

			if ((offset == 0) || (offset == 7))
				update_interrupt();

			if ((offset == 7) && (old_data & w7_idle) && ! (data & w7_idle))
			{	/* idle has been cleared: start command execution */
				execute_command();
			}
		}
	}
}
