/***************************************************************************
    vtech2.c

    machine driver
    Juergen Buchmueller <pullmoll@t-online.de> MESS driver, Jan 2000
    Davide Moretti <dave@rimini.com> ROM dump and hardware description

    TODO:
        Add loading images from WAV files.
        Printer and RS232 support.
        Check if the FDC is really the same as in the
        Laser 210/310 (aka VZ200/300) series.

****************************************************************************/

#include "emu.h"
#include "includes/vtech2.h"
#include "imagedev/flopdrv.h"
#include "imagedev/cassette.h"
#include "sound/speaker.h"

/* public */

/* static */

#define TRKSIZE_VZ	0x9a0	/* arbitrary (actually from analyzing format) */

static const UINT8 laser_fdc_wrprot[2] = {0x80, 0x80};

/* write to banked memory (handle memory mapped i/o and videoram) */
static void mwa_bank(running_machine &machine, int bank, int offs, int data);

/* wrappers for bank #1 to #4 */
static WRITE8_HANDLER ( mwa_bank1 ) { mwa_bank(space->machine(), 0,offset,data); }
static WRITE8_HANDLER ( mwa_bank2 ) { mwa_bank(space->machine(), 1,offset,data); }
static WRITE8_HANDLER ( mwa_bank3 ) { mwa_bank(space->machine(), 2,offset,data); }
static WRITE8_HANDLER ( mwa_bank4 ) { mwa_bank(space->machine(), 3,offset,data); }

/* read from banked memory (handle memory mapped i/o) */
static int mra_bank(running_machine &machine, int bank, int offs);

/* wrappers for bank #1 to #4 */
static READ8_HANDLER ( mra_bank1 ) { return mra_bank(space->machine(),0,offset); }
static READ8_HANDLER ( mra_bank2 ) { return mra_bank(space->machine(),1,offset); }
static READ8_HANDLER ( mra_bank3 ) { return mra_bank(space->machine(),2,offset); }
static READ8_HANDLER ( mra_bank4 ) { return mra_bank(space->machine(),3,offset); }

/* read banked memory (handle memory mapped i/o) */
static const read8_space_func mra_bank_soft[4] =
{
    mra_bank1,  /* mapped in 0000-3fff */
    mra_bank2,  /* mapped in 4000-7fff */
    mra_bank3,  /* mapped in 8000-bfff */
    mra_bank4   /* mapped in c000-ffff */
};

/* write banked memory (handle memory mapped i/o and videoram) */
static const write8_space_func mwa_bank_soft[4] =
{
    mwa_bank1,  /* mapped in 0000-3fff */
    mwa_bank2,  /* mapped in 4000-7fff */
    mwa_bank3,  /* mapped in 8000-bfff */
    mwa_bank4   /* mapped in c000-ffff */
};

/* read banked memory (plain ROM/RAM) */
static const char *const mra_bank_hard[4] =
{
    "bank1",  /* mapped in 0000-3fff */
    "bank2",  /* mapped in 4000-7fff */
    "bank3",  /* mapped in 8000-bfff */
    "bank4"   /* mapped in c000-ffff */
};

/* write banked memory (plain ROM/RAM) */
static const char *const mwa_bank_hard[4] =
{
    "bank1",  /* mapped in 0000-3fff */
    "bank2",  /* mapped in 4000-7fff */
    "bank3",  /* mapped in 8000-bfff */
    "bank4"   /* mapped in c000-ffff */
};

DRIVER_INIT(laser)
{
	vtech2_state *state = machine.driver_data<vtech2_state>();
	UINT8 *gfx = machine.region("gfx2")->base();
	int i;

	state->m_laser_track_x2[0] = state->m_laser_track_x2[1] = 80;
	state->m_laser_fdc_bits = 8;
	state->m_laser_drive = -1;

	for (i = 0; i < 256; i++)
		gfx[i] = i;

	state->m_laser_latch = -1;
	state->m_mem = machine.region("maincpu")->base();

	for (i = 0; i < ARRAY_LENGTH(state->m_laser_bank); i++)
		state->m_laser_bank[i] = -1;
}



static void laser_machine_init(running_machine &machine, int bank_mask, int video_mask)
{
	vtech2_state *state = machine.driver_data<vtech2_state>();
    int i;

	state->m_laser_bank_mask = bank_mask;
    state->m_laser_video_bank = video_mask;
	state->m_videoram = state->m_mem + state->m_laser_video_bank * 0x04000;
	logerror("laser_machine_init(): bank mask $%04X, video %d [$%05X]\n", state->m_laser_bank_mask, state->m_laser_video_bank, state->m_laser_video_bank * 0x04000);

	for (i = 0; i < ARRAY_LENGTH(state->m_laser_bank); i++)
		laser_bank_select_w(machine.device("maincpu")->memory().space(AS_PROGRAM), i, 0);
}

MACHINE_RESET( laser350 )
{
	/* banks 0 to 3 only, optional ROM extension */
	laser_machine_init(machine, 0xf00f, 3);
}

MACHINE_RESET( laser500 )
{
	/* banks 0 to 2, and 4-7 only , optional ROM extension */
	laser_machine_init(machine, 0xf0f7, 7);
}

MACHINE_RESET( laser700 )
{
	/* all banks except #3 */
	laser_machine_init(machine, 0xfff7, 7);
}


WRITE8_HANDLER( laser_bank_select_w )
{
	vtech2_state *state = space->machine().driver_data<vtech2_state>();
    static const char *const bank_name[16] = {
        "ROM lo","ROM hi","MM I/O","Video RAM lo",
        "RAM #0","RAM #1","RAM #2","RAM #3",
        "RAM #4","RAM #5","RAM #6","RAM #7/Video RAM hi",
        "ext ROM #0","ext ROM #1","ext ROM #2","ext ROM #3"
    };
	char bank[10];
	offset %= 4;
    data &= 15;

	if( data != state->m_laser_bank[offset] )
    {
        state->m_laser_bank[offset] = data;
		logerror("select bank #%d $%02X [$%05X] %s\n", offset+1, data, 0x4000 * (data & 15), bank_name[data]);

        /* memory mapped I/O bank selected? */
		if (data == 2)
		{
			space->machine().device("maincpu")->memory().space(AS_PROGRAM)->install_legacy_read_handler(offset * 0x4000, offset * 0x4000 + 0x3fff, FUNC(mra_bank_soft[offset]));
			space->machine().device("maincpu")->memory().space(AS_PROGRAM)->install_legacy_write_handler(offset * 0x4000, offset * 0x4000 + 0x3fff, FUNC(mwa_bank_soft[offset]));
		}
		else
		{
			sprintf(bank,"bank%d",offset+1);
			memory_set_bankptr(space->machine(), bank, &state->m_mem[0x4000*state->m_laser_bank[offset]]);
			if( state->m_laser_bank_mask & (1 << data) )
			{
				/* video RAM bank selected? */
				if( data == state->m_laser_video_bank )
				{
					logerror("select bank #%d VIDEO!\n", offset+1);
				}
				space->machine().device("maincpu")->memory().space(AS_PROGRAM)->install_read_bank(offset * 0x4000, offset * 0x4000 + 0x3fff, mra_bank_hard[offset]);
				space->machine().device("maincpu")->memory().space(AS_PROGRAM)->install_write_bank(offset * 0x4000, offset * 0x4000 + 0x3fff, mwa_bank_hard[offset]);

			}
			else
			{
				logerror("select bank #%d MASKED!\n", offset+1);
				space->machine().device("maincpu")->memory().space(AS_PROGRAM)->nop_readwrite(offset * 0x4000, offset * 0x4000 + 0x3fff);

			}
		}
    }
}

static device_t *vtech2_cassette_image(running_machine &machine)
{
	return machine.device("cassette");
}

/*************************************************
 * memory mapped I/O read
 * bit  function
 * 7    not assigned
 * 6    column 6
 * 5    column 5
 * 4    column 4
 * 3    column 3
 * 2    column 2
 * 1    column 1
 * 0    column 0
 ************************************************/
static int mra_bank(running_machine &machine, int bank, int offs)
{
	vtech2_state *state = machine.driver_data<vtech2_state>();
    int level, data = 0xff;

    /* Laser 500/700 only: keyboard rows A through D */
	if( (offs & 0x00ff) == 0x00ff )
	{
		if( (offs & 0x0300) == 0x0000 ) /* keyboard row A */
		{
			if( input_port_read(machine, "ROWA") != state->m_row_a )
			{
				state->m_row_a = input_port_read(machine, "ROWA");
				data &= state->m_row_a;
			}
		}
		if( (offs & 0x0300) == 0x0100 ) /* keyboard row B */
		{
			if( input_port_read(machine, "ROWB") != state->m_row_b )
			{
				state->m_row_b = input_port_read(machine, "ROWB");
				data &= state->m_row_b;
			}
		}
		if( (offs & 0x0300) == 0x0200 ) /* keyboard row C */
		{
			if( input_port_read(machine, "ROWC") != state->m_row_c )
			{
				state->m_row_c = input_port_read(machine, "ROWC");
				data &= state->m_row_c;
			}
		}
		if( (offs & 0x0300) == 0x0300 ) /* keyboard row D */
		{
			if( input_port_read(machine, "ROWD") != state->m_row_d )
			{
				state->m_row_d = input_port_read(machine, "ROWD");
				data &= state->m_row_d;
			}
		}
	}
	else
	{
		/* All Lasers keyboard rows 0 through 7 */
        if( !(offs & 0x01) )
			data &= input_port_read(machine, "ROW0");
		if( !(offs & 0x02) )
			data &= input_port_read(machine, "ROW1");
		if( !(offs & 0x04) )
			data &= input_port_read(machine, "ROW2");
		if( !(offs & 0x08) )
			data &= input_port_read(machine, "ROW3");
		if( !(offs & 0x10) )
			data &= input_port_read(machine, "ROW4");
		if( !(offs & 0x20) )
			data &= input_port_read(machine, "ROW5");
		if( !(offs & 0x40) )
			data &= input_port_read(machine, "ROW6");
		if( !(offs & 0x80) )
			data &= input_port_read(machine, "ROW7");
	}

    /* what's bit 7 good for? tape input maybe? */
	level = cassette_input(vtech2_cassette_image(machine)) * 65536.0;
	if( level < state->m_level_old - 511 )
		state->m_cassette_bit = 0x00;
	if( level > state->m_level_old + 511 )
		state->m_cassette_bit = 0x80;
	state->m_level_old = level;

	data &= ~state->m_cassette_bit;
    // logerror("bank #%d keyboard_r [$%04X] $%02X\n", bank, offs, data);

    return data;
}

/*************************************************
 * memory mapped I/O write
 * bit  function
 * 7-6  not assigned
 * 5    speaker B ???
 * 4    ???
 * 3    mode: 1 graphics, 0 text
 * 2    cassette out (MSB)
 * 1    cassette out (LSB)
 * 0    speaker A
 ************************************************/
static void mwa_bank(running_machine &machine, int bank, int offs, int data)
{
	vtech2_state *state = machine.driver_data<vtech2_state>();
	device_t *speaker = machine.device("speaker");
	offs += 0x4000 * state->m_laser_bank[bank];
    switch (state->m_laser_bank[bank])
    {
    case  0:    /* ROM lower 16K */
    case  1:    /* ROM upper 16K */
		logerror("bank #%d write to ROM [$%05X] $%02X\n", bank+1, offs, data);
        break;
    case  2:    /* memory mapped output */
        if (data != state->m_laser_latch)
        {
            logerror("bank #%d write to I/O [$%05X] $%02X\n", bank+1, offs, data);
            /* Toggle between graphics and text modes? */
			if ((data ^ state->m_laser_latch) & 0x01)
				speaker_level_w(speaker, data & 1);
            state->m_laser_latch = data;
        }
        break;
    case 12:    /* ext. ROM #1 */
    case 13:    /* ext. ROM #2 */
    case 14:    /* ext. ROM #3 */
    case 15:    /* ext. ROM #4 */
		logerror("bank #%d write to ROM [$%05X] $%02X\n", bank+1, offs, data);
        break;
    default:    /* RAM */
        if( state->m_laser_bank[bank] == state->m_laser_video_bank && state->m_mem[offs] != data )
		{
			logerror("bank #%d write to videoram [$%05X] $%02X\n", bank+1, offs, data);
		}
        state->m_mem[offs] = data;
        break;
    }
}

DEVICE_IMAGE_LOAD( laser_cart )
{
	vtech2_state *state = image.device().machine().driver_data<vtech2_state>();
	int size = 0;

	size = image.fread(&state->m_mem[0x30000], 0x10000);
	state->m_laser_bank_mask &= ~0xf000;
	if( size > 0 )
		state->m_laser_bank_mask |= 0x1000;
	if( size > 0x4000 )
		state->m_laser_bank_mask |= 0x2000;
	if( size > 0x8000 )
		state->m_laser_bank_mask |= 0x4000;
	if( size > 0xc000 )
		state->m_laser_bank_mask |= 0x8000;

	return size > 0 ? IMAGE_INIT_PASS : IMAGE_INIT_FAIL;
}

DEVICE_IMAGE_UNLOAD( laser_cart )
{
	vtech2_state *state = image.device().machine().driver_data<vtech2_state>();
	state->m_laser_bank_mask &= ~0xf000;
	/* wipe out the memory contents to be 100% sure */
	memset(&state->m_mem[0x30000], 0xff, 0x10000);
}

static device_t *laser_file(running_machine &machine)
{
	vtech2_state *state = machine.driver_data<vtech2_state>();
	return machine.device( state->m_laser_drive ? FLOPPY_1 : FLOPPY_0 );
}

static void laser_get_track(running_machine &machine)
{
	vtech2_state *state = machine.driver_data<vtech2_state>();
    sprintf(state->m_laser_frame_message, "#%d get track %02d", state->m_laser_drive, state->m_laser_track_x2[state->m_laser_drive]/2);
    state->m_laser_frame_time = 30;
    /* drive selected or and image file ok? */
    if( state->m_laser_drive >= 0 && laser_file(machine) != NULL )
    {
        int size, offs;
		device_image_interface *image = dynamic_cast<device_image_interface *>(laser_file(machine));
        size = TRKSIZE_VZ;
        offs = TRKSIZE_VZ * state->m_laser_track_x2[state->m_laser_drive]/2;
        image->fseek(offs, SEEK_SET);
        size = image->fread(state->m_laser_fdc_data, size);
        logerror("get track @$%05x $%04x bytes\n", offs, size);
    }
    state->m_laser_fdc_offs = 0;
    state->m_laser_fdc_write = 0;
}

static void laser_put_track(running_machine &machine)
{
	vtech2_state *state = machine.driver_data<vtech2_state>();
	device_image_interface *image = dynamic_cast<device_image_interface *>(laser_file(machine));
    /* drive selected and image file ok? */
    if( state->m_laser_drive >= 0 && laser_file(machine) != NULL )
    {
        int size, offs;
        offs = TRKSIZE_VZ * state->m_laser_track_x2[state->m_laser_drive]/2;
        image->fseek(offs + state->m_laser_fdc_start, SEEK_SET);
        size = image->fwrite(&state->m_laser_fdc_data[state->m_laser_fdc_start], state->m_laser_fdc_write);
        logerror("put track @$%05X+$%X $%04X/$%04X bytes\n", offs, state->m_laser_fdc_start, size, state->m_laser_fdc_write);
    }
}

#define PHI0(n) (((n)>>0)&1)
#define PHI1(n) (((n)>>1)&1)
#define PHI2(n) (((n)>>2)&1)
#define PHI3(n) (((n)>>3)&1)

 READ8_HANDLER( laser_fdc_r )
{
	vtech2_state *state = space->machine().driver_data<vtech2_state>();
    int data = 0xff;
    switch( offset )
    {
    case 1: /* data (read-only) */
        if( state->m_laser_fdc_bits > 0 )
        {
            if( state->m_laser_fdc_status & 0x80 )
                state->m_laser_fdc_bits--;
            data = (state->m_laser_data >> state->m_laser_fdc_bits) & 0xff;
#if 0
            logerror("laser_fdc_r bits %d%d%d%d%d%d%d%d\n",
                (data>>7)&1,(data>>6)&1,(data>>5)&1,(data>>4)&1,
                (data>>3)&1,(data>>2)&1,(data>>1)&1,(data>>0)&1 );
#endif
        }
        if( state->m_laser_fdc_bits == 0 )
        {
            state->m_laser_data = state->m_laser_fdc_data[state->m_laser_fdc_offs];
            logerror("laser_fdc_r %d : data ($%04X) $%02X\n", offset, state->m_laser_fdc_offs, state->m_laser_data);
            if( state->m_laser_fdc_status & 0x80 )
            {
                state->m_laser_fdc_bits = 8;
                state->m_laser_fdc_offs = (state->m_laser_fdc_offs + 1) % TRKSIZE_FM;
            }
            state->m_laser_fdc_status &= ~0x80;
        }
        break;
    case 2: /* polling (read-only) */
        /* fake */
        if( state->m_laser_drive >= 0 )
            state->m_laser_fdc_status |= 0x80;
        data = state->m_laser_fdc_status;
        break;
    case 3: /* write protect status (read-only) */
        if( state->m_laser_drive >= 0 )
            data = laser_fdc_wrprot[state->m_laser_drive];
        logerror("laser_fdc_r %d : write_protect $%02X\n", offset, data);
        break;
    }
    return data;
}

WRITE8_HANDLER( laser_fdc_w )
{
	vtech2_state *state = space->machine().driver_data<vtech2_state>();
    int drive;

    switch( offset )
    {
    case 0: /* latch (write-only) */
        drive = (data & 0x10) ? 0 : (data & 0x80) ? 1 : -1;
        if( drive != state->m_laser_drive )
        {
            state->m_laser_drive = drive;
            if( state->m_laser_drive >= 0 )
                laser_get_track(space->machine());
        }
        if( state->m_laser_drive >= 0 )
        {
            if( (PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI1(state->m_laser_fdc_latch)) ||
                (PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI2(state->m_laser_fdc_latch)) ||
                (PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI3(state->m_laser_fdc_latch)) ||
                (PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI0(state->m_laser_fdc_latch)) )
            {
                if( state->m_laser_track_x2[state->m_laser_drive] > 0 )
                    state->m_laser_track_x2[state->m_laser_drive]--;
                logerror("laser_fdc_w(%d) $%02X drive %d: stepout track #%2d.%d\n", offset, data, state->m_laser_drive, state->m_laser_track_x2[state->m_laser_drive]/2,5*(state->m_laser_track_x2[state->m_laser_drive]&1));
                if( (state->m_laser_track_x2[state->m_laser_drive] & 1) == 0 )
                    laser_get_track(space->machine());
            }
            else
            if( (PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI3(state->m_laser_fdc_latch)) ||
                (PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI0(state->m_laser_fdc_latch)) ||
                (PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI1(state->m_laser_fdc_latch)) ||
                (PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI2(state->m_laser_fdc_latch)) )
            {
                if( state->m_laser_track_x2[state->m_laser_drive] < 2*40 )
                    state->m_laser_track_x2[state->m_laser_drive]++;
                logerror("laser_fdc_w(%d) $%02X drive %d: stepin track #%2d.%d\n", offset, data, state->m_laser_drive, state->m_laser_track_x2[state->m_laser_drive]/2,5*(state->m_laser_track_x2[state->m_laser_drive]&1));
                if( (state->m_laser_track_x2[state->m_laser_drive] & 1) == 0 )
                    laser_get_track(space->machine());
            }
            if( (data & 0x40) == 0 )
            {
                state->m_laser_data <<= 1;
                if( (state->m_laser_fdc_latch ^ data) & 0x20 )
                    state->m_laser_data |= 1;
                if( (state->m_laser_fdc_edge ^= 1) == 0 )
                {
                    if( --state->m_laser_fdc_bits == 0 )
                    {
                        UINT8 value = 0;
                        state->m_laser_data &= 0xffff;
                        if( state->m_laser_data & 0x4000 ) value |= 0x80;
                        if( state->m_laser_data & 0x1000 ) value |= 0x40;
                        if( state->m_laser_data & 0x0400 ) value |= 0x20;
                        if( state->m_laser_data & 0x0100 ) value |= 0x10;
                        if( state->m_laser_data & 0x0040 ) value |= 0x08;
                        if( state->m_laser_data & 0x0010 ) value |= 0x04;
                        if( state->m_laser_data & 0x0004 ) value |= 0x02;
                        if( state->m_laser_data & 0x0001 ) value |= 0x01;
                        logerror("laser_fdc_w(%d) data($%04X) $%02X <- $%02X ($%04X)\n", offset, state->m_laser_fdc_offs, state->m_laser_fdc_data[state->m_laser_fdc_offs], value, state->m_laser_data);
                        state->m_laser_fdc_data[state->m_laser_fdc_offs] = value;
                        state->m_laser_fdc_offs = (state->m_laser_fdc_offs + 1) % TRKSIZE_FM;
                        state->m_laser_fdc_write++;
                        state->m_laser_fdc_bits = 8;
                    }
                }
            }
            /* change of write signal? */
            if( (state->m_laser_fdc_latch ^ data) & 0x40 )
            {
                /* falling edge? */
                if ( state->m_laser_fdc_latch & 0x40 )
                {
                    sprintf(state->m_laser_frame_message, "#%d put track %02d", state->m_laser_drive, state->m_laser_track_x2[state->m_laser_drive]/2);
                    state->m_laser_frame_time = 30;
                    state->m_laser_fdc_start = state->m_laser_fdc_offs;
					state->m_laser_fdc_edge = 0;
                }
                else
                {
                    /* data written to track before? */
                    if( state->m_laser_fdc_write )
                        laser_put_track(space->machine());
                }
                state->m_laser_fdc_bits = 8;
                state->m_laser_fdc_write = 0;
            }
        }
        state->m_laser_fdc_latch = data;
        break;
    }
}


