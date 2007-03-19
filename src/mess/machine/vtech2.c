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

#include "driver.h"
#include "video/generic.h"
#include "image.h"
#include "includes/vtech2.h"
#include "devices/cassette.h"
#include "sound/speaker.h"

/* public */
int laser_latch;

/* static */
static UINT8 *mem;
static int laser_bank_mask;	/* up to 16 4K banks supported */
static int laser_bank[4];
static int laser_video_bank;

#define TRKSIZE_VZ	0x9a0	/* arbitrary (actually from analyzing format) */
#define TRKSIZE_FM	3172	/* size of a standard FM mode track */

static UINT8 laser_track_x2[2] = {80, 80};
static UINT8 laser_fdc_wrprot[2] = {0x80, 0x80};
static UINT8 laser_fdc_status = 0;
static UINT8 laser_fdc_data[TRKSIZE_FM];
static int laser_data;
static int laser_fdc_edge = 0;
static int laser_fdc_bits = 8;
static int laser_drive = -1;
static int laser_fdc_start = 0;
static int laser_fdc_write = 0;
static int laser_fdc_offs = 0;
static int laser_fdc_latch = 0;

/* write to banked memory (handle memory mapped i/o and videoram) */
static void mwa_bank(int bank, int offs, int data);

/* wrappers for bank #1 to #4 */
static WRITE8_HANDLER ( mwa_bank1 ) { mwa_bank(0,offset,data); }
static WRITE8_HANDLER ( mwa_bank2 ) { mwa_bank(1,offset,data); }
static WRITE8_HANDLER ( mwa_bank3 ) { mwa_bank(2,offset,data); }
static WRITE8_HANDLER ( mwa_bank4 ) { mwa_bank(3,offset,data); }

/* read from banked memory (handle memory mapped i/o) */
static int mra_bank(int bank, int offs);

/* wrappers for bank #1 to #4 */
static  READ8_HANDLER ( mra_bank1) { return mra_bank(0,offset); }
static  READ8_HANDLER ( mra_bank2) { return mra_bank(1,offset); }
static  READ8_HANDLER ( mra_bank3) { return mra_bank(2,offset); }
static  READ8_HANDLER ( mra_bank4) { return mra_bank(3,offset); }

/* read banked memory (handle memory mapped i/o) */
static read8_handler mra_bank_soft[4] =
{
    mra_bank1,  /* mapped in 0000-3fff */
    mra_bank2,  /* mapped in 4000-7fff */
    mra_bank3,  /* mapped in 8000-bfff */
    mra_bank4   /* mapped in c000-ffff */
};

/* write banked memory (handle memory mapped i/o and videoram) */
static write8_handler mwa_bank_soft[4] =
{
    mwa_bank1,  /* mapped in 0000-3fff */
    mwa_bank2,  /* mapped in 4000-7fff */
    mwa_bank3,  /* mapped in 8000-bfff */
    mwa_bank4   /* mapped in c000-ffff */
};

/* read banked memory (plain ROM/RAM) */
static read8_handler mra_bank_hard[4] =
{
    MRA8_BANK1,  /* mapped in 0000-3fff */
    MRA8_BANK2,  /* mapped in 4000-7fff */
    MRA8_BANK3,  /* mapped in 8000-bfff */
    MRA8_BANK4   /* mapped in c000-ffff */
};

/* write banked memory (plain ROM/RAM) */
static write8_handler mwa_bank_hard[4] =
{
    MWA8_BANK1,  /* mapped in 0000-3fff */
    MWA8_BANK2,  /* mapped in 4000-7fff */
    MWA8_BANK3,  /* mapped in 8000-bfff */
    MWA8_BANK4   /* mapped in c000-ffff */
};

DRIVER_INIT(laser)
{
    UINT8 *gfx = memory_region(REGION_GFX2);
    int i;

	for (i = 0; i < 256; i++)
        gfx[i] = i;

	laser_latch = -1;
    mem = memory_region(REGION_CPU1);

	for (i = 0; i < sizeof(laser_bank) / sizeof(laser_bank[0]); i++)
		laser_bank[i] = -1;
}



static void laser_machine_init(int bank_mask, int video_mask)
{
    int i;

	laser_bank_mask = bank_mask;
    laser_video_bank = video_mask;
	videoram = mem + laser_video_bank * 0x04000;
	logerror("laser_machine_init(): bank mask $%04X, video %d [$%05X]\n", laser_bank_mask, laser_video_bank, laser_video_bank * 0x04000);

	for (i = 0; i < sizeof(laser_bank) / sizeof(laser_bank[0]); i++)
		laser_bank_select_w(i, 0);
}

MACHINE_RESET( laser350 )
{
	/* banks 0 to 3 only, optional ROM extension */
	laser_machine_init(0xf00f, 3);
}

MACHINE_RESET( laser500 )
{
	/* banks 0 to 2, and 4-7 only , optional ROM extension */
	laser_machine_init(0xf0f7, 7);
}

MACHINE_RESET( laser700 )
{
	/* all banks except #3 */
	laser_machine_init(0xfff7, 7);
}


WRITE8_HANDLER( laser_bank_select_w )
{
    static const char *bank_name[16] = {
        "ROM lo","ROM hi","MM I/O","Video RAM lo",
        "RAM #0","RAM #1","RAM #2","RAM #3",
        "RAM #4","RAM #5","RAM #6","RAM #7/Video RAM hi",
        "ext ROM #0","ext ROM #1","ext ROM #2","ext ROM #3"};
	read8_handler read_handler;
	write8_handler write_handler;

	offset %= 4;
    data &= 15;

	if( data != laser_bank[offset] )
    {
        laser_bank[offset] = data;
		logerror("select bank #%d $%02X [$%05X] %s\n", offset+1, data, 0x4000 * (data & 15), bank_name[data]);

        /* memory mapped I/O bank selected? */
		if (data == 2)
		{
			read_handler = mra_bank_soft[offset];
			write_handler = mwa_bank_soft[offset];
		}
		else
		{
			memory_set_bankptr(offset+1, &mem[0x4000*laser_bank[offset]]);
			if( laser_bank_mask & (1 << data) )
			{
				read_handler = mra_bank_hard[offset];
				write_handler = mwa_bank_hard[offset];

				/* video RAM bank selected? */
				if( data == laser_video_bank )
				{
					logerror("select bank #%d VIDEO!\n", offset+1);
				}
			}
			else
			{
				logerror("select bank #%d MASKED!\n", offset+1);
				read_handler = return8_FF;
				write_handler = MWA8_NOP;
			}
		}
		memory_install_read8_handler(0,  ADDRESS_SPACE_PROGRAM, offset * 0x4000, offset * 0x4000 + 0x3fff, 0, 0, read_handler);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, offset * 0x4000, offset * 0x4000 + 0x3fff, 0, 0, write_handler);
    }
}

static mess_image *vtech2_cassette_image(void)
{
	return image_from_devtype_and_index(IO_CASSETTE, 0);
}

/*************************************************
 * memory mapped I/O read
 * bit  function
 * 7	not assigned
 * 6	column 6
 * 5	column 5
 * 4	column 4
 * 3	column 3
 * 2	column 2
 * 1	column 1
 * 0	column 0
 ************************************************/
static int mra_bank(int bank, int offs)
{
	static int level_old = 0, cassette_bit = 0;
    int level, data = 0xff;

    /* Laser 500/700 only: keyboard rows A through D */
	if( (offs & 0x00ff) == 0x00ff )
	{
		static int row_a,row_b,row_c,row_d;
		if( (offs & 0x0300) == 0x0000 ) /* keyboard row A */
		{
			if( readinputport( 8) != row_a )
			{
				row_a = readinputport(8);
				data &= row_a;
			}
		}
		if( (offs & 0x0300) == 0x0100 ) /* keyboard row B */
		{
			if( readinputport( 9) != row_b )
			{
				row_b = readinputport( 9);
				data &= row_b;
			}
		}
		if( (offs & 0x0300) == 0x0200 ) /* keyboard row C */
		{
			if( readinputport(10) != row_c )
			{
				row_c = readinputport(10);
				data &= row_c;
			}
		}
		if( (offs & 0x0300) == 0x0300 ) /* keyboard row D */
		{
			if( readinputport(11) != row_d )
			{
				row_d = readinputport(11);
				data &= row_d;
			}
		}
	}
	else
	{
		/* All Lasers keyboard rows 0 through 7 */
        if( !(offs & 0x01) )
			data &= readinputport( 0);
		if( !(offs & 0x02) )
			data &= readinputport( 1);
		if( !(offs & 0x04) )
			data &= readinputport( 2);
		if( !(offs & 0x08) )
			data &= readinputport( 3);
		if( !(offs & 0x10) )
			data &= readinputport( 4);
		if( !(offs & 0x20) )
			data &= readinputport( 5);
		if( !(offs & 0x40) )
			data &= readinputport( 6);
		if( !(offs & 0x80) )
			data &= readinputport( 7);
	}

    /* what's bit 7 good for? tape input maybe? */
	level = cassette_input(vtech2_cassette_image()) * 65536.0;
	if( level < level_old - 511 )
		cassette_bit = 0x00;
	if( level > level_old + 511 )
		cassette_bit = 0x80;
	level_old = level;

	data &= ~cassette_bit;
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
static void mwa_bank(int bank, int offs, int data)
{
	offs += 0x4000 * laser_bank[bank];
    switch (laser_bank[bank])
    {
    case  0:    /* ROM lower 16K */
    case  1:    /* ROM upper 16K */
		logerror("bank #%d write to ROM [$%05X] $%02X\n", bank+1, offs, data);
        break;
    case  2:    /* memory mapped output */
        if (data != laser_latch)
        {
            logerror("bank #%d write to I/O [$%05X] $%02X\n", bank+1, offs, data);
            /* Toggle between graphics and text modes? */
			if ((data ^ laser_latch) & 0x01)
				speaker_level_w(0, data & 1);
            laser_latch = data;
        }
        break;
    case 12:    /* ext. ROM #1 */
    case 13:    /* ext. ROM #2 */
    case 14:    /* ext. ROM #3 */
    case 15:    /* ext. ROM #4 */
		logerror("bank #%d write to ROM [$%05X] $%02X\n", bank+1, offs, data);
        break;
    default:    /* RAM */
        if( laser_bank[bank] == laser_video_bank && mem[offs] != data )
		{
			logerror("bank #%d write to videoram [$%05X] $%02X\n", bank+1, offs, data);
            dirtybuffer[offs&0x3fff] = 1;
		}
        mem[offs] = data;
        break;
    }
}

DEVICE_LOAD( laser_cart )
{
	int size = 0;

	size = image_fread(image, &mem[0x30000], 0x10000);
	laser_bank_mask &= ~0xf000;
	if( size > 0 )
		laser_bank_mask |= 0x1000;
	if( size > 0x4000 )
		laser_bank_mask |= 0x2000;
	if( size > 0x8000 )
		laser_bank_mask |= 0x4000;
	if( size > 0xc000 )
		laser_bank_mask |= 0x8000;

	return size > 0 ? INIT_PASS : INIT_FAIL;
}

DEVICE_UNLOAD( laser_cart )
{
	laser_bank_mask &= ~0xf000;
	/* wipe out the memory contents to be 100% sure */
	memset(&mem[0x30000], 0xff, 0x10000);
}

DEVICE_LOAD( laser_floppy )
{
	UINT8 buff[32];

	image_fread(image, buff, sizeof(buff));
	if (memcmp(buff, "\x80\x80\x80\x80\x80\x80\x00\xfe\0xe7\0x18\0xc3\x00\x00\x00\x80\x80", 16))
		return INIT_FAIL;

	return INIT_PASS;
}

static mess_image *laser_file(void)
{
	return image_from_devtype_and_index(IO_FLOPPY, laser_drive);
}

static void laser_get_track(void)
{
    sprintf(laser_frame_message, "#%d get track %02d", laser_drive, laser_track_x2[laser_drive]/2);
    laser_frame_time = 30;
    /* drive selected or and image file ok? */
    if( laser_drive >= 0 && laser_file() != NULL )
    {
        int size, offs;
        size = TRKSIZE_VZ;
        offs = TRKSIZE_VZ * laser_track_x2[laser_drive]/2;
        image_fseek(laser_file(), offs, SEEK_SET);
        size = image_fread(laser_file(), laser_fdc_data, size);
        logerror("get track @$%05x $%04x bytes\n", offs, size);
    }
    laser_fdc_offs = 0;
    laser_fdc_write = 0;
}

static void laser_put_track(void)
{
    /* drive selected and image file ok? */
    if( laser_drive >= 0 && laser_file() != NULL )
    {
        int size, offs;
        offs = TRKSIZE_VZ * laser_track_x2[laser_drive]/2;
        image_fseek(laser_file(), offs + laser_fdc_start, SEEK_SET);
        size = image_fwrite(laser_file(), &laser_fdc_data[laser_fdc_start], laser_fdc_write);
        logerror("put track @$%05X+$%X $%04X/$%04X bytes\n", offs, laser_fdc_start, size, laser_fdc_write);
    }
}

#define PHI0(n) (((n)>>0)&1)
#define PHI1(n) (((n)>>1)&1)
#define PHI2(n) (((n)>>2)&1)
#define PHI3(n) (((n)>>3)&1)

 READ8_HANDLER( laser_fdc_r )
{
    int data = 0xff;
    switch( offset )
    {
    case 1: /* data (read-only) */
        if( laser_fdc_bits > 0 )
        {
            if( laser_fdc_status & 0x80 )
                laser_fdc_bits--;
            data = (laser_data >> laser_fdc_bits) & 0xff;
#if 0
            logerror("laser_fdc_r bits %d%d%d%d%d%d%d%d\n",
                (data>>7)&1,(data>>6)&1,(data>>5)&1,(data>>4)&1,
                (data>>3)&1,(data>>2)&1,(data>>1)&1,(data>>0)&1 );
#endif
        }
        if( laser_fdc_bits == 0 )
        {
            laser_data = laser_fdc_data[laser_fdc_offs];
            logerror("laser_fdc_r %d : data ($%04X) $%02X\n", offset, laser_fdc_offs, laser_data);
            if( laser_fdc_status & 0x80 )
            {
                laser_fdc_bits = 8;
                laser_fdc_offs = (laser_fdc_offs + 1) % TRKSIZE_FM;
            }
            laser_fdc_status &= ~0x80;
        }
        break;
    case 2: /* polling (read-only) */
        /* fake */
        if( laser_drive >= 0 )
            laser_fdc_status |= 0x80;
        data = laser_fdc_status;
        break;
    case 3: /* write protect status (read-only) */
        if( laser_drive >= 0 )
            data = laser_fdc_wrprot[laser_drive];
        logerror("laser_fdc_r %d : write_protect $%02X\n", offset, data);
        break;
    }
    return data;
}

WRITE8_HANDLER( laser_fdc_w )
{
    int drive;

    switch( offset )
    {
    case 0: /* latch (write-only) */
        drive = (data & 0x10) ? 0 : (data & 0x80) ? 1 : -1;
        if( drive != laser_drive )
        {
            laser_drive = drive;
            if( laser_drive >= 0 )
                laser_get_track();
        }
        if( laser_drive >= 0 )
        {
            if( (PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI1(laser_fdc_latch)) ||
                (PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI2(laser_fdc_latch)) ||
                (PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI3(laser_fdc_latch)) ||
                (PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI0(laser_fdc_latch)) )
            {
                if( laser_track_x2[laser_drive] > 0 )
                    laser_track_x2[laser_drive]--;
                logerror("laser_fdc_w(%d) $%02X drive %d: stepout track #%2d.%d\n", offset, data, laser_drive, laser_track_x2[laser_drive]/2,5*(laser_track_x2[laser_drive]&1));
                if( (laser_track_x2[laser_drive] & 1) == 0 )
                    laser_get_track();
            }
            else
            if( (PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI3(laser_fdc_latch)) ||
                (PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI0(laser_fdc_latch)) ||
                (PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI1(laser_fdc_latch)) ||
                (PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI2(laser_fdc_latch)) )
            {
                if( laser_track_x2[laser_drive] < 2*40 )
                    laser_track_x2[laser_drive]++;
                logerror("laser_fdc_w(%d) $%02X drive %d: stepin track #%2d.%d\n", offset, data, laser_drive, laser_track_x2[laser_drive]/2,5*(laser_track_x2[laser_drive]&1));
                if( (laser_track_x2[laser_drive] & 1) == 0 )
                    laser_get_track();
            }
            if( (data & 0x40) == 0 )
            {
                laser_data <<= 1;
                if( (laser_fdc_latch ^ data) & 0x20 )
                    laser_data |= 1;
                if( (laser_fdc_edge ^= 1) == 0 )
                {
                    if( --laser_fdc_bits == 0 )
                    {
                        UINT8 value = 0;
                        laser_data &= 0xffff;
                        if( laser_data & 0x4000 ) value |= 0x80;
                        if( laser_data & 0x1000 ) value |= 0x40;
                        if( laser_data & 0x0400 ) value |= 0x20;
                        if( laser_data & 0x0100 ) value |= 0x10;
                        if( laser_data & 0x0040 ) value |= 0x08;
                        if( laser_data & 0x0010 ) value |= 0x04;
                        if( laser_data & 0x0004 ) value |= 0x02;
                        if( laser_data & 0x0001 ) value |= 0x01;
                        logerror("laser_fdc_w(%d) data($%04X) $%02X <- $%02X ($%04X)\n", offset, laser_fdc_offs, laser_fdc_data[laser_fdc_offs], value, laser_data);
                        laser_fdc_data[laser_fdc_offs] = value;
                        laser_fdc_offs = (laser_fdc_offs + 1) % TRKSIZE_FM;
                        laser_fdc_write++;
                        laser_fdc_bits = 8;
                    }
                }
            }
            /* change of write signal? */
            if( (laser_fdc_latch ^ data) & 0x40 )
            {
                /* falling edge? */
                if ( laser_fdc_latch & 0x40 )
                {
                    sprintf(laser_frame_message, "#%d put track %02d", laser_drive, laser_track_x2[laser_drive]/2);
                    laser_frame_time = 30;
                    laser_fdc_start = laser_fdc_offs;
					laser_fdc_edge = 0;
                }
                else
                {
                    /* data written to track before? */
                    if( laser_fdc_write )
                        laser_put_track();
                }
                laser_fdc_bits = 8;
                laser_fdc_write = 0;
            }
        }
        laser_fdc_latch = data;
        break;
    }
}


