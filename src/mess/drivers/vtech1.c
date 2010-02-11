/*****************************************************************************

Video Technology Laser 110-310 computers:

    Video Technology Laser 110
      Sanyo Laser 110
    Video Technology Laser 200
      Salora Fellow
      Texet TX-8000
      Video Technology VZ-200
    Video Technology Laser 210
      Dick Smith Electronics VZ-200
      Sanyo Laser 210
    Video Technology Laser 310
      Dick Smith Electronics VZ-300

System driver:

    Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999
      - everything

    Dirk Best <duke@redump.de>, May 2004
      - clean up
      - fixed parent/clone relationsips and memory size for the Laser 200
      - fixed loading of the DOS ROM
      - added BASIC V2.1
      - added SHA1 checksums

    Dirk Best <duke@redump.de>, March 2006
      - 64KB memory expansion (banked)
      - cartridge support

Thanks go to:

    - Guy Thomason
    - Jason Oakley
    - Bushy Maunder
    - and anybody else on the vzemu list :)
    - Davide Moretti for the detailed description of the colors
    - Leslie Milburn

Memory maps:

    Laser 110/200
        0000-1FFF 8K ROM 0
        2000-3FFF 8K ROM 1
        4000-5FFF 8K DOS ROM or other cartridges (optional)
        6000-67FF 2K reserved for rom cartridges
        6800-6FFF 2K memory mapped I/O
                    R: keyboard
                    W: cassette I/O, speaker, VDP control
        7000-77FF 2K video RAM
        7800-7FFF 2K internal user RAM
        8800-C7FF 16K memory expansion
        8000-BFFF 64K memory expansion, first bank
        C000-FFFF 64K memory expansion, other banks

    Laser 210
        0000-1FFF 8K ROM 0
        2000-3FFF 8K ROM 1
        4000-5FFF 8K DOS ROM or other cartridges (optional)
        6000-67FF 2K reserved for rom cartridges
        6800-6FFF 2K memory mapped I/O
                    R: keyboard
                    W: cassette I/O, speaker, VDP control
        7000-77FF 2K video RAM
        7800-8FFF 6K internal user RAM (3x2K: U2, U3, U4)
        9000-CFFF 16K memory expansion
        8000-BFFF 64K memory expansion, first bank
        C000-FFFF 64K memory expansion, other banks

    Laser 310
        0000-3FFF 16K ROM
        4000-5FFF 8K DOS ROM or other cartridges (optional)
        6000-67FF 2K reserved for rom cartridges
        6800-6FFF 2K memory mapped I/O
                    R: keyboard
                    W: cassette I/O, speaker, VDP control
        7000-77FF 2K video RAM
        7800-B7FF 16K internal user RAM
        B800-F7FF 16K memory expansion
        8000-BFFF 64K memory expansion, first bank
        C000-FFFF 64K memory expansion, other banks


   Memory expansions available for the Laser/VZ computers:

   - a 16kb expansion without banking
   - a 64kb expansion where the first bank is fixed and the other 3 are
     banked in as needed
   - a banked memory expansion similar to the 64kb one, that the user could
     fill themselves with memory up to 4MB total.

   They are externally connected devices. The 16kb extension is different
   between Laser 110/210/310 computers, though it could be relativly
   easily modified to work on another model.


Todo:

    - Figure out which machines were shipped with which ROM version
      where not known (currently only a guess)
    - Lightpen support
    - Rewrite floppy and move to its own file

Notes:

    - The only known dumped cartridge is the DOS ROM:
      CRC(b6ed6084) SHA1(59d1cbcfa6c5e1906a32704fbf0d9670f0d1fd8b)


******************************************************************************/

#include "emu.h"
#include "utils.h"
#include "cpu/z80/z80.h"
#include "video/m6847.h"
#include "machine/ctronics.h"
#include "sound/wave.h"
#include "sound/speaker.h"
#include "devices/cartslot.h"
#include "devices/flopdrv.h"
#include "devices/snapquik.h"
#include "devices/cassette.h"
#include "devices/z80bin.h"
#include "devices/messram.h"
#include "formats/vt_cas.h"


/***************************************************************************
    CONSTANTS & MACROS
***************************************************************************/

#define LOG_VTECH1_LATCH 0
#define LOG_VTECH1_FDC   0

#define VTECH1_CLK        3579500
#define VZ300_XTAL1_CLK   XTAL_17_73447MHz

#define VZ_BASIC 0xf0
#define VZ_MCODE 0xf1

#define TRKSIZE_VZ	0x9b0	/* arbitrary (actually from analyzing format) */
#define TRKSIZE_FM	3172	/* size of a standard FM mode track */

#define PHI0(n) (((n)>>0)&1)
#define PHI1(n) (((n)>>1)&1)
#define PHI2(n) (((n)>>2)&1)
#define PHI3(n) (((n)>>3)&1)


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _vtech1_state vtech1_state;
struct _vtech1_state
{
	/* devices */
	running_device *mc6847;
	running_device *speaker;
	running_device *cassette;
	running_device *printer;

	UINT8 *ram;
	UINT32 ram_size;

	/* floppy */
	int drive;
	UINT8 fdc_track_x2[2];
	UINT8 fdc_wrprot[2];
	UINT8 fdc_status;
	UINT8 fdc_data[TRKSIZE_FM];
	int data;
	int fdc_edge;
	int fdc_bits;
	int fdc_start;
	int fdc_write;
	int fdc_offs;
	int fdc_latch;
};


/***************************************************************************
    SNAPSHOT LOADING
***************************************************************************/

static SNAPSHOT_LOAD( vtech1 )
{
	vtech1_state *vtech1 = (vtech1_state *)image->machine->driver_data;
	const address_space *space = cputag_get_address_space(image->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 i, header[24];
	UINT16 start, end, size;
	char pgmname[18];

	/* get the header */
	image_fread(image, &header, sizeof(header));
	for (i = 0; i < 16; i++) pgmname[i] = header[i+4];
	pgmname[16] = '\0';

	/* get start and end addresses */
	start = pick_integer_le(header, 22, 2);
	end = start + snapshot_size - sizeof(header);
	size = end - start;

	/* check if we have enough ram */
	if (vtech1->ram_size < size)
	{
		char message[256];
		snprintf(message, ARRAY_LENGTH(message), "SNAPLOAD: %s\nInsufficient RAM - need %04X",pgmname,size);
		image_seterror(image, IMAGE_ERROR_INVALIDIMAGE, message);
		image_message(image, "SNAPLOAD: %s\nInsufficient RAM - need %04X",pgmname,size);
		return INIT_FAIL;
	}

	/* write it to ram */
	image_fread(image, &vtech1->ram[start - 0x7800], size);

	/* patch variables depending on snapshot type */
	switch (header[21])
	{
	case VZ_BASIC:		/* 0xF0 */
		memory_write_byte(space, 0x78a4, start % 256); /* start of basic program */
		memory_write_byte(space, 0x78a5, start / 256);
		memory_write_byte(space, 0x78f9, end % 256); /* end of basic program */
		memory_write_byte(space, 0x78fa, end / 256);
		memory_write_byte(space, 0x78fb, end % 256); /* start variable table */
		memory_write_byte(space, 0x78fc, end / 256);
		memory_write_byte(space, 0x78fd, end % 256); /* start free mem, end variable table */
		memory_write_byte(space, 0x78fe, end / 256);
		image_message(image, " %s (B)\nsize=%04X : start=%04X : end=%04X",pgmname,size,start,end);
		break;

	case VZ_MCODE:		/* 0xF1 */
		memory_write_byte(space, 0x788e, start % 256); /* usr subroutine address */
		memory_write_byte(space, 0x788f, start / 256);
		image_message(image, " %s (M)\nsize=%04X : start=%04X : end=%04X",pgmname,size,start,end);
		cpu_set_reg(devtag_get_device(image->machine, "maincpu"), REG_GENPC, start);				/* start program */
		break;

	default:
		image_seterror(image, IMAGE_ERROR_UNSUPPORTED, "Snapshot format not supported.");
		image_message(image, "Snapshot format not supported.");
		return INIT_FAIL;
	}

	return INIT_PASS;
}

static Z80BIN_EXECUTE( vtech1 )
{
	running_device *cpu = devtag_get_device(machine, "maincpu");
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	/* A Microsoft Basic program needs some manipulation before it can be run.
    1. A start address of 7ae9 indicates a basic program which needs its pointers fixed up.
    2. If autorun is turned off, the pointers still need fixing, but then display READY.
    Important addresses:
        7ae9 = start (load) address of a conventional basic program
        791e = custom routine to fix basic pointers */

	memory_write_word_16le(space, 0x791c, end_address + 1);
	memory_write_word_16le(space, 0x781e, execute_address);

	if (start_address == 0x7ae9)
	{
		UINT8 i;
		static const UINT8 data[]={
			0xe5,			// PUSH HL  ;save pcode pointer
			0x2a, 0x1c, 0x79,	// LD HL,(791C) ;get saved end_addr+1
			0x22, 0xf9, 0x78,	// LD (78F9),HL ;move it to correct place
			0x21, 0x39, 0x78,	// LD HL,7839   ;point to control flag
			0xcb, 0xf6,		// SET 6,(HL)   ;turn on autorun (cb b6 = manual run)
			0xcb, 0x9e,		// RES 3,(HL)   ;turn off verify (just in case)
			0xc3, 0xcf, 0x36	// JP 36CF  ;enter bios at autorun point
		};

		for (i = 0; i < ARRAY_LENGTH(data); i++)
			memory_write_byte(space, 0x791e + i, data[i]);

		if (!autorun)
			memory_write_byte(space, 0x7929, 0xb6);	/* turn off autorun */

		cpu_set_reg(cpu, REG_GENPC, 0x791e);
	}
	else
	{
		if (autorun)
			cpu_set_reg(cpu, REG_GENPC, execute_address);
	}
}


/***************************************************************************
    FLOPPY DRIVE
***************************************************************************/
static void vtech1_load_proc(running_device *image)
{
	vtech1_state *vtech1 = (vtech1_state *)image->machine->driver_data;
	int id = floppy_get_drive(image);
	
	if (image_is_writable(image))
		vtech1->fdc_wrprot[id] = 0x00;
	else
		vtech1->fdc_wrprot[id] = 0x80;
}

static void vtech1_get_track(running_machine *machine)
{
	vtech1_state *vtech1 = (vtech1_state *)machine->driver_data;

	/* drive selected or and image file ok? */
	if (vtech1->drive >= 0 && image_exists(floppy_get_device(machine,vtech1->drive)))
	{
		int size, offs;
		size = TRKSIZE_VZ;
		offs = TRKSIZE_VZ * vtech1->fdc_track_x2[vtech1->drive]/2;
		image_fseek(floppy_get_device(machine,vtech1->drive), offs, SEEK_SET);
		size = image_fread(floppy_get_device(machine,vtech1->drive), vtech1->fdc_data, size);
		if (LOG_VTECH1_FDC)
			logerror("get track @$%05x $%04x bytes\n", offs, size);
    }
	vtech1->fdc_offs = 0;
	vtech1->fdc_write = 0;
}

static void vtech1_put_track(running_machine *machine)
{
	vtech1_state *vtech1 = (vtech1_state *)machine->driver_data;

    /* drive selected and image file ok? */
	if (vtech1->drive >= 0 && floppy_get_device(machine,vtech1->drive) != NULL)
	{
		int size, offs;
		offs = TRKSIZE_VZ * vtech1->fdc_track_x2[vtech1->drive]/2;
		image_fseek(floppy_get_device(machine,vtech1->drive), offs + vtech1->fdc_start, SEEK_SET);
		size = image_fwrite(floppy_get_device(machine,vtech1->drive), &vtech1->fdc_data[vtech1->fdc_start], vtech1->fdc_write);
		if (LOG_VTECH1_FDC)
			logerror("put track @$%05X+$%X $%04X/$%04X bytes\n", offs, vtech1->fdc_start, size, vtech1->fdc_write);
    }
}

static READ8_HANDLER( vtech1_fdc_r )
{
	vtech1_state *vtech1 = (vtech1_state *)space->machine->driver_data;
    int data = 0xff;

    switch (offset)
    {
    case 1: /* data (read-only) */
		if (vtech1->fdc_bits > 0)
        {
			if( vtech1->fdc_status & 0x80 )
				vtech1->fdc_bits--;
			data = (vtech1->data >> vtech1->fdc_bits) & 0xff;
			if (LOG_VTECH1_FDC) {
				logerror("vtech1_fdc_r bits %d%d%d%d%d%d%d%d\n",
	                (data>>7)&1,(data>>6)&1,(data>>5)&1,(data>>4)&1,
	                (data>>3)&1,(data>>2)&1,(data>>1)&1,(data>>0)&1 );
			}
        }
		if (vtech1->fdc_bits == 0)
        {
			vtech1->data = vtech1->fdc_data[vtech1->fdc_offs];
			if (LOG_VTECH1_FDC)
				logerror("vtech1_fdc_r %d : data ($%04X) $%02X\n", offset, vtech1->fdc_offs, vtech1->data);
			if(vtech1->fdc_status & 0x80)
            {
				vtech1->fdc_bits = 8;
				vtech1->fdc_offs = (vtech1->fdc_offs + 1) % TRKSIZE_FM;
            }
			vtech1->fdc_status &= ~0x80;
        }
        break;
    case 2: /* polling (read-only) */
        /* fake */
		if (vtech1->drive >= 0)
			vtech1->fdc_status |= 0x80;
		data = vtech1->fdc_status;
        break;
    case 3: /* write protect status (read-only) */
		if (vtech1->drive >= 0)
			data = vtech1->fdc_wrprot[vtech1->drive];
		if (LOG_VTECH1_FDC)
			logerror("vtech1_fdc_r %d : write_protect $%02X\n", offset, data);
        break;
    }
    return data;
}

static WRITE8_HANDLER( vtech1_fdc_w )
{
	vtech1_state *vtech1 = (vtech1_state *)space->machine->driver_data;
	int drive;

    switch (offset)
	{
	case 0: /* latch (write-only) */
		drive = (data & 0x10) ? 0 : (data & 0x80) ? 1 : -1;
		if (drive != vtech1->drive)
		{
			vtech1->drive = drive;
			if (vtech1->drive >= 0)
				vtech1_get_track(space->machine);
        }
		if (vtech1->drive >= 0)
        {
			if ((PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI1(vtech1->fdc_latch)) ||
				(PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI2(vtech1->fdc_latch)) ||
				(PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI3(vtech1->fdc_latch)) ||
				(PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI0(vtech1->fdc_latch)))
            {
				if (vtech1->fdc_track_x2[vtech1->drive] > 0)
					vtech1->fdc_track_x2[vtech1->drive]--;
				if (LOG_VTECH1_FDC)
					logerror("vtech1_fdc_w(%d) $%02X drive %d: stepout track #%2d.%d\n", offset, data, vtech1->drive, vtech1->fdc_track_x2[vtech1->drive]/2,5*(vtech1->fdc_track_x2[vtech1->drive]&1));
				if ((vtech1->fdc_track_x2[vtech1->drive] & 1) == 0)
					vtech1_get_track(space->machine);
            }
            else
			if ((PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI3(vtech1->fdc_latch)) ||
				(PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI0(vtech1->fdc_latch)) ||
				(PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI1(vtech1->fdc_latch)) ||
				(PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI2(vtech1->fdc_latch)))
            {
				if (vtech1->fdc_track_x2[vtech1->drive] < 2*40)
					vtech1->fdc_track_x2[vtech1->drive]++;
				if (LOG_VTECH1_FDC)
					logerror("vtech1_fdc_w(%d) $%02X drive %d: stepin track #%2d.%d\n", offset, data, vtech1->drive, vtech1->fdc_track_x2[vtech1->drive]/2,5*(vtech1->fdc_track_x2[vtech1->drive]&1));
				if ((vtech1->fdc_track_x2[vtech1->drive] & 1) == 0)
					vtech1_get_track(space->machine);
            }
            if ((data & 0x40) == 0)
			{
				vtech1->data <<= 1;
				if ((vtech1->fdc_latch ^ data) & 0x20)
					vtech1->data |= 1;
				if ((vtech1->fdc_edge ^= 1) == 0)
                {
					vtech1->fdc_bits--;

					if (vtech1->fdc_bits == 0)
					{
						UINT8 value = 0;
						vtech1->data &= 0xffff;
						if (vtech1->data & 0x4000 ) value |= 0x80;
						if (vtech1->data & 0x1000 ) value |= 0x40;
						if (vtech1->data & 0x0400 ) value |= 0x20;
						if (vtech1->data & 0x0100 ) value |= 0x10;
						if (vtech1->data & 0x0040 ) value |= 0x08;
						if (vtech1->data & 0x0010 ) value |= 0x04;
						if (vtech1->data & 0x0004 ) value |= 0x02;
						if (vtech1->data & 0x0001 ) value |= 0x01;
						if (LOG_VTECH1_FDC)
							logerror("vtech1_fdc_w(%d) data($%04X) $%02X <- $%02X ($%04X)\n", offset, vtech1->fdc_offs, vtech1->fdc_data[vtech1->fdc_offs], value, vtech1->data);
						vtech1->fdc_data[vtech1->fdc_offs] = value;
						vtech1->fdc_offs = (vtech1->fdc_offs + 1) % TRKSIZE_FM;
						vtech1->fdc_write++;
						vtech1->fdc_bits = 8;
					}
                }
            }
			/* change of write signal? */
			if ((vtech1->fdc_latch ^ data) & 0x40)
            {
                /* falling edge? */
				if (vtech1->fdc_latch & 0x40)
                {
					vtech1->fdc_start = vtech1->fdc_offs;
					vtech1->fdc_edge = 0;
                }
                else
                {
                    /* data written to track before? */
					if (vtech1->fdc_write)
						vtech1_put_track(space->machine);
                }
				vtech1->fdc_bits = 8;
				vtech1->fdc_write = 0;
            }
        }
		vtech1->fdc_latch = data;
		break;
    }
}

static FLOPPY_IDENTIFY( vtech1_dsk_identify )
{
	*vote = 100;
	return FLOPPY_ERROR_SUCCESS;
}


static FLOPPY_CONSTRUCT( vtech1_dsk_construct )
{
	return FLOPPY_ERROR_SUCCESS;
}

FLOPPY_OPTIONS_START( vtech1_only )
	FLOPPY_OPTION(
		vtech1_dsk,
		"dsk",
		"Laser floppy disk image",
		vtech1_dsk_identify,
		vtech1_dsk_construct,
		NULL
	)
FLOPPY_OPTIONS_END0

static const floppy_config vtech1_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(vtech1_only),
	DO_NOT_KEEP_GEOMETRY
};

/***************************************************************************
    PRINTER
***************************************************************************/

static READ8_DEVICE_HANDLER( vtech1_printer_r )
{
	UINT8 result = 0xff;

	result &= ~(!centronics_busy_r(device));

	return result;
}

/* TODO: figure out how this really works */
static WRITE8_DEVICE_HANDLER( vtech1_strobe_w )
{
	centronics_strobe_w(device, TRUE);
	centronics_strobe_w(device, FALSE);
}


/***************************************************************************
    RS232 SERIAL
***************************************************************************/

static READ8_HANDLER( vtech1_serial_r )
{
	logerror("vtech1_serial_r offset $%02x\n", offset);
	return 0xff;
}

static WRITE8_HANDLER( vtech1_serial_w )
{
	logerror("vtech1_serial_w $%02x, offset %02x\n", data, offset);
}


/***************************************************************************
    INPUTS
***************************************************************************/

static READ8_HANDLER( vtech1_lightpen_r )
{
	logerror("vtech1_lightpen_r(%d)\n", offset);
	return 0xff;
}

static READ8_HANDLER( vtech1_joystick_r )
{
    int result = 0xff;

	if (!BIT(offset, 0)) result &= input_port_read(space->machine, "joystick_0");
	if (!BIT(offset, 1)) result &= input_port_read(space->machine, "joystick_0_arm");
	if (!BIT(offset, 2)) result &= input_port_read(space->machine, "joystick_1");
	if (!BIT(offset, 3)) result &= input_port_read(space->machine, "joystick_1_arm");

    return result;
}

static READ8_HANDLER( vtech1_keyboard_r )
{
	vtech1_state *vtech1 = (vtech1_state *)space->machine->driver_data;
	UINT8 result = 0x3f;

	/* bit 0 to 5, keyboard input */
	if (!BIT(offset, 0)) result &= input_port_read(space->machine, "keyboard_0");
	if (!BIT(offset, 1)) result &= input_port_read(space->machine, "keyboard_1");
	if (!BIT(offset, 2)) result &= input_port_read(space->machine, "keyboard_2");
	if (!BIT(offset, 3)) result &= input_port_read(space->machine, "keyboard_3");
	if (!BIT(offset, 4)) result &= input_port_read(space->machine, "keyboard_4");
	if (!BIT(offset, 5)) result &= input_port_read(space->machine, "keyboard_5");
	if (!BIT(offset, 6)) result &= input_port_read(space->machine, "keyboard_6");
	if (!BIT(offset, 7)) result &= input_port_read(space->machine, "keyboard_7");

	/* bit 6, cassette input */
	result |= (cassette_input(vtech1->cassette) > 0 ? 1 : 0) << 6;

	/* bit 7, field sync */
	result |= mc6847_fs_r(vtech1->mc6847) << 7;

    return result;
}


/***************************************************************************
    I/O LATCH
***************************************************************************/

static WRITE8_HANDLER( vtech1_latch_w )
{
	vtech1_state *vtech1 = (vtech1_state *)space->machine->driver_data;

	if (LOG_VTECH1_LATCH)
		logerror("vtech1_latch_w $%02X\n", data);

	/* bit 1, SHRG mod (if installed) */
	if (space->machine->generic.videoram_size == 0x2000)
	{
		mc6847_gm0_w(vtech1->mc6847, BIT(data, 1));
		mc6847_gm2_w(vtech1->mc6847, BIT(data, 1));
	}

	/* bit 2, cassette out */
	cassette_output(vtech1->cassette, BIT(data, 2) ? +1.0 : -1.0);

	/* bit 3 and 4, vdc mode control lines */
	mc6847_ag_w(vtech1->mc6847, BIT(data, 3));
	mc6847_css_w(vtech1->mc6847, BIT(data, 4));

	/* bit 0 and 5, speaker */
	speaker_level_w(vtech1->speaker, (BIT(data, 5) << 1) | BIT(data, 0));
}


/***************************************************************************
    MEMORY BANKING
***************************************************************************/

static WRITE8_HANDLER( vtech1_memory_bank_w )
{
	vtech1_state *vtech1 = (vtech1_state *)space->machine->driver_data;

	logerror("vtech1_memory_bank_w $%02X\n", data);

	if (data >= 1)
		if ((data <= 3 && vtech1->ram_size == 66*1024) || (vtech1->ram_size == 4098*1024))
			memory_set_bank(space->machine, "bank3", data - 1);
}

static WRITE8_HANDLER( vtech1_video_bank_w )
{
	logerror("vtech1_video_bank_w $%02X\n", data);
	memory_set_bank(space->machine, "bank4", data & 0x03);
}


/***************************************************************************
    VIDEO EMULATION
***************************************************************************/

static READ8_DEVICE_HANDLER( vtech1_mc6847_videoram_r )
{
	mc6847_inv_w(device, BIT(device->machine->generic.videoram.u8[offset], 6));
	mc6847_as_w(device, BIT(device->machine->generic.videoram.u8[offset], 7));

	return device->machine->generic.videoram.u8[offset];
}

static VIDEO_UPDATE( vtech1 )
{
	vtech1_state *vtech1 = (vtech1_state *)screen->machine->driver_data;
	return mc6847_update(vtech1->mc6847, bitmap, cliprect);
}


/***************************************************************************
    DRIVER INIT
***************************************************************************/

static DRIVER_INIT( vtech1 )
{
	vtech1_state *vtech1 = (vtech1_state *)machine->driver_data;
	const address_space *prg = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	int id;

	/* find devices */
	vtech1->mc6847 = devtag_get_device(machine, "mc6847");
	vtech1->speaker = devtag_get_device(machine, "speaker");
	vtech1->cassette = devtag_get_device(machine, "cassette");
	vtech1->printer = devtag_get_device(machine, "printer");

	/* ram */
	vtech1->ram = messram_get_ptr(devtag_get_device(machine, "messram"));
	vtech1->ram_size = messram_get_size(devtag_get_device(machine, "messram"));

	/* setup memory banking */
	memory_set_bankptr(machine, "bank1", vtech1->ram);

	/* 16k memory expansion? */
	if (vtech1->ram_size == 18*1024 || vtech1->ram_size == 22*1024 || vtech1->ram_size == 32*1024)
	{
		offs_t base = 0x7800 + (vtech1->ram_size - 0x4000);
		memory_install_readwrite_bank(prg, base, base + 0x3fff, 0, 0, "bank2");
		memory_set_bankptr(machine, "bank2", vtech1->ram + base - 0x7800);
	}

	/* 64k expansion? */
	if (vtech1->ram_size >= 66*1024)
	{
		/* install fixed first bank */
		memory_install_readwrite_bank(prg, 0x8000, 0xbfff, 0, 0, "bank2");
		memory_set_bankptr(machine, "bank2", vtech1->ram + 0x800);

		/* install the others, dynamically banked in */
		memory_install_readwrite_bank(prg, 0xc000, 0xffff, 0, 0, "bank3");
		memory_configure_bank(machine, "bank3", 0, (vtech1->ram_size - 0x4800) / 0x4000, vtech1->ram + 0x4800, 0x4000);
		memory_set_bank(machine, "bank3", 0);
	}

	/* initialize floppy */
	vtech1->drive = -1;
	vtech1->fdc_track_x2[0] = 80;
	vtech1->fdc_track_x2[1] = 80;
	vtech1->fdc_wrprot[0] = 0x80;
	vtech1->fdc_wrprot[1] = 0x80;
	vtech1->fdc_status = 0;
	vtech1->fdc_edge = 0;
	vtech1->fdc_bits = 8;
	vtech1->fdc_start = 0;
	vtech1->fdc_write = 0;
	vtech1->fdc_offs = 0;
	vtech1->fdc_latch = 0;
	
	for(id=0;id<2;id++) 
	{
		floppy_install_load_proc(floppy_get_device(machine, id), vtech1_load_proc);
	}	
}

static DRIVER_INIT( vtech1h )
{
	const address_space *prg = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);	

	DRIVER_INIT_CALL(vtech1);

	/* the SHRG mod replaces the standard videoram chip with an 8k chip */
	machine->generic.videoram_size = 0x2000;
	machine->generic.videoram.u8 = auto_alloc_array(machine, UINT8, machine->generic.videoram_size);

	memory_install_readwrite_bank(prg, 0x7000, 0x77ff, 0, 0, "bank4");
	memory_configure_bank(machine, "bank4", 0, 4, machine->generic.videoram.u8, 0x800);
	memory_set_bank(machine, "bank4", 0);	
}

/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START( laser110_mem, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0x3fff) AM_ROM	/* basic rom */
    AM_RANGE(0x4000, 0x5fff) AM_ROM	/* dos rom or other catridges */
    AM_RANGE(0x6000, 0x67ff) AM_ROM	/* reserved for cartridges */
    AM_RANGE(0x6800, 0x6fff) AM_READWRITE(vtech1_keyboard_r, vtech1_latch_w)
    AM_RANGE(0x7000, 0x77ff) AM_RAM AM_BASE_SIZE_GENERIC(videoram) /* (6847) */
    AM_RANGE(0x7800, 0x7fff) AM_RAMBANK("bank1") /* 2k user ram */
    AM_RANGE(0x8000, 0xbfff) AM_NOP /* 16k ram expansion */
    AM_RANGE(0xc000, 0xffff) AM_NOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( laser210_mem, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0x3fff) AM_ROM	/* basic rom */
    AM_RANGE(0x4000, 0x5fff) AM_ROM	/* dos rom or other catridges */
    AM_RANGE(0x6000, 0x67ff) AM_ROM	/* reserved for cartridges */
    AM_RANGE(0x6800, 0x6fff) AM_READWRITE(vtech1_keyboard_r, vtech1_latch_w)
    AM_RANGE(0x7000, 0x77ff) AM_RAM AM_BASE_SIZE_GENERIC(videoram) /* U7 (6847) */
    AM_RANGE(0x7800, 0x8fff) AM_RAMBANK("bank1") /* 6k user ram */
    AM_RANGE(0x9000, 0xcfff) AM_NOP /* 16k ram expansion */
    AM_RANGE(0xd000, 0xffff) AM_NOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( laser310_mem, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0x3fff) AM_ROM	/* basic rom */
    AM_RANGE(0x4000, 0x5fff) AM_ROM	/* dos rom or other catridges */
    AM_RANGE(0x6000, 0x67ff) AM_ROM	/* reserved for cartridges */
    AM_RANGE(0x6800, 0x6fff) AM_READWRITE(vtech1_keyboard_r, vtech1_latch_w)
    AM_RANGE(0x7000, 0x77ff) AM_RAM AM_BASE_SIZE_GENERIC(videoram) /* (6847) */
    AM_RANGE(0x7800, 0xb7ff) AM_RAMBANK("bank1") /* 16k user ram */
    AM_RANGE(0xb800, 0xf7ff) AM_NOP /* 16k ram expansion */
    AM_RANGE(0xf8ff, 0xffff) AM_NOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( vtech1_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_DEVREAD("centronics", vtech1_printer_r)
	AM_RANGE(0x0d, 0x0d) AM_DEVWRITE("centronics", vtech1_strobe_w)
	AM_RANGE(0x0e, 0x0e) AM_DEVWRITE("centronics", centronics_data_w)
	AM_RANGE(0x10, 0x1f) AM_READWRITE(vtech1_fdc_r, vtech1_fdc_w)
	AM_RANGE(0x20, 0x2f) AM_READ(vtech1_joystick_r)
	AM_RANGE(0x30, 0x3f) AM_READWRITE(vtech1_serial_r, vtech1_serial_w)
	AM_RANGE(0x40, 0x4f) AM_READ(vtech1_lightpen_r)
	AM_RANGE(0x70, 0x7f) AM_WRITE(vtech1_memory_bank_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( vtech1_shrg_io, ADDRESS_SPACE_IO, 8 )
	AM_IMPORT_FROM(vtech1_io)
	AM_RANGE(0xd0, 0xdf) AM_WRITE(vtech1_video_bank_w)
ADDRESS_MAP_END


/***************************************************************************
    INPUT PORTS
***************************************************************************/

static INPUT_PORTS_START(vtech1)
	PORT_START("keyboard_0")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R       RETURN  LEFT$")   PORT_CODE(KEYCODE_R)     PORT_CHAR('R')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q       FOR     CHR$")    PORT_CODE(KEYCODE_Q)     PORT_CHAR('Q')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E       NEXT    LEN(")    PORT_CODE(KEYCODE_E)     PORT_CHAR('E')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W       TO      VAL(")    PORT_CODE(KEYCODE_W)     PORT_CHAR('W')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T       THEN    MID$")    PORT_CODE(KEYCODE_T)     PORT_CHAR('T')

	PORT_START("keyboard_1")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F       GOSUB   RND(")    PORT_CODE(KEYCODE_F)     PORT_CHAR('F')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A       MODE(   ASC(")    PORT_CODE(KEYCODE_A)     PORT_CHAR('A')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D       DIM     RESTORE") PORT_CODE(KEYCODE_D)     PORT_CHAR('D')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL")                    PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S       STEP    STR$(")   PORT_CODE(KEYCODE_S)     PORT_CHAR('S')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G       GOTO    STOP")    PORT_CODE(KEYCODE_G)     PORT_CHAR('G')

	PORT_START("keyboard_2")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V       LPRINT  USR")     PORT_CODE(KEYCODE_V)     PORT_CHAR('V')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z       PEEK(   INP")     PORT_CODE(KEYCODE_Z)     PORT_CHAR('Z')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C       CONT    COPY")    PORT_CODE(KEYCODE_C)     PORT_CHAR('C')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT")                   PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X       POKE    OUT")     PORT_CODE(KEYCODE_X)     PORT_CHAR('X')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B       LLIST   SOUND")   PORT_CODE(KEYCODE_B)     PORT_CHAR('B')

	PORT_START("keyboard_3")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4  $    VERIFY  ATN(")    PORT_CODE(KEYCODE_4)     PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1  !    CSAVE   SIN(")    PORT_CODE(KEYCODE_1)     PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3  #    CRUN    TAN(")    PORT_CODE(KEYCODE_3)     PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2  \"    CLOAD   COS(")   PORT_CODE(KEYCODE_2)     PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5  %    LIST    LOG(")    PORT_CODE(KEYCODE_5)     PORT_CHAR('5') PORT_CHAR('%')

	PORT_START("keyboard_4")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M  \\    \xE2\x86\x90")   PORT_CODE(KEYCODE_M)     PORT_CHAR('M') PORT_CHAR('\\') PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE   \xE2\x86\x93")    PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ') PORT_CHAR('~')  PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",  <    \xE2\x86\x92")    PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')  PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".  >    \xE2\x86\x91")    PORT_CODE(KEYCODE_STOP)  PORT_CHAR('.') PORT_CHAR('>')  PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N  ^    COLOR   USING")   PORT_CODE(KEYCODE_N)     PORT_CHAR('N') PORT_CHAR('^')

	PORT_START("keyboard_5")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7  '    END     SGN(")    PORT_CODE(KEYCODE_7)     PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0  @    DATA    INT(")    PORT_CODE(KEYCODE_0)     PORT_CHAR('0') PORT_CHAR('@')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8  (    NEW     SQR(")    PORT_CODE(KEYCODE_8)     PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-  =    [Break]")         PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')  PORT_CHAR(UCHAR_MAMEKEY(CANCEL))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9  )    READ    ABS(")    PORT_CODE(KEYCODE_9)     PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6  &    RUN     EXP(")    PORT_CODE(KEYCODE_6)     PORT_CHAR('6') PORT_CHAR('&')

	PORT_START("keyboard_6")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U       IF      INKEY$")  PORT_CODE(KEYCODE_U)     PORT_CHAR('U')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P  ]    PRINT   NOT")     PORT_CODE(KEYCODE_P)     PORT_CHAR('P') PORT_CHAR(']')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I       INPUT   AND")     PORT_CODE(KEYCODE_I)     PORT_CHAR('I')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RETURN  [Function]")      PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O  [    LET     OR")      PORT_CODE(KEYCODE_O)     PORT_CHAR('O') PORT_CHAR('[')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y       ELSE    RIGHT$(") PORT_CODE(KEYCODE_Y)     PORT_CHAR('Y')

	PORT_START("keyboard_7")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J       REM     RESET")   PORT_CODE(KEYCODE_J)     PORT_CHAR('J')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";  +    [Rubout]")        PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')  PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K  /    TAB(    POINT")   PORT_CODE(KEYCODE_K)     PORT_CHAR('K') PORT_CHAR('/')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":  *    [Inverse]")       PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')  PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L  ?    [Insert]")        PORT_CODE(KEYCODE_L)     PORT_CHAR('L') PORT_CHAR('?')  PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H       CLS     SET")     PORT_CODE(KEYCODE_H)     PORT_CHAR('H')

	PORT_START("joystick_0")
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON1)        PORT_PLAYER(1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)  PORT_PLAYER(1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  PORT_PLAYER(1)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    PORT_PLAYER(1)

	PORT_START("joystick_0_arm")
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON2)        PORT_PLAYER(1)
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("joystick_1")
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON1)        PORT_PLAYER(2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)  PORT_PLAYER(2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  PORT_PLAYER(2)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    PORT_PLAYER(2)

	PORT_START("joystick_1_arm")
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON2)        PORT_PLAYER(2)
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED)

	/* Enhanced options not available on real hardware */
	PORT_START("CONFIG")
	PORT_CONFNAME( 0x01, 0x01, "Autorun on Quickload")
	PORT_CONFSETTING(    0x00, DEF_STR(No))
	PORT_CONFSETTING(    0x01, DEF_STR(Yes))
//  PORT_CONFNAME( 0x08, 0x08, "Cassette Speaker")
//  PORT_CONFSETTING(    0x08, DEF_STR(On))
//  PORT_CONFSETTING(    0x00, DEF_STR(Off))
INPUT_PORTS_END


/***************************************************************************
    PALETTE
***************************************************************************/

static const UINT32 vtech1_palette_mono[] =
{
	MAKE_RGB(131, 131, 131),
	MAKE_RGB(211, 211, 211),
	MAKE_RGB(29, 29, 29),
	MAKE_RGB(76, 76, 76),
	MAKE_RGB(213, 213, 213),
	MAKE_RGB(167, 167, 167),
	MAKE_RGB(105, 105, 105),
	MAKE_RGB(136, 136, 136),
	MAKE_RGB(0, 0, 0),
	MAKE_RGB(131, 131, 131),
	MAKE_RGB(0, 0, 0),
	MAKE_RGB(213, 213, 213),
	MAKE_RGB(37, 37, 37),
	MAKE_RGB(133, 133, 133),
	MAKE_RGB(28, 28, 28),
	MAKE_RGB(193, 193, 193)
};


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static const INT16 speaker_levels[] = {-32768, 0, 32767, 0};

static const speaker_interface vtech1_speaker_interface =
{
	4,
	speaker_levels
};

static const cassette_config laser_cassette_config =
{
	vtech1_cassette_formats,
	NULL,
	(cassette_state)(CASSETTE_PLAY)
};

static const mc6847_interface vtech1_mc6847_intf =
{
	DEVCB_HANDLER(vtech1_mc6847_videoram_r),
	DEVCB_LINE_GND,
	DEVCB_LINE_VCC,
	DEVCB_LINE_GND,
	DEVCB_LINE_GND,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_CPU_INPUT_LINE("maincpu", 0),
	DEVCB_NULL,
	DEVCB_NULL,
};

static const mc6847_interface vtech1_shrg_mc6847_intf =
{
	DEVCB_HANDLER(vtech1_mc6847_videoram_r),
	DEVCB_NULL,
	DEVCB_LINE_VCC,
	DEVCB_NULL,
	DEVCB_LINE_GND,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_CPU_INPUT_LINE("maincpu", 0),
	DEVCB_NULL,
	DEVCB_NULL,
};

static MACHINE_DRIVER_START( laser110 )
	MDRV_DRIVER_DATA(vtech1_state)

    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", Z80, VTECH1_CLK)  /* 3.57950 MHz */
    MDRV_CPU_PROGRAM_MAP(laser110_mem)
    MDRV_CPU_IO_MAP(vtech1_io)

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(M6847_PAL_FRAMES_PER_SECOND)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(320, 25+192+26)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 1, 239)

	MDRV_VIDEO_UPDATE(vtech1)

	MDRV_MC6847_ADD("mc6847", vtech1_mc6847_intf)
	MDRV_MC6847_TYPE(M6847_VERSION_ORIGINAL_PAL)
	MDRV_MC6847_PALETTE(vtech1_palette_mono)

    /* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_CONFIG(vtech1_speaker_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)

	/* printer */
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)

	/* snapshot/quickload */
	MDRV_SNAPSHOT_ADD("snapshot", vtech1, "vz", 1.5)
	MDRV_Z80BIN_QUICKLOAD_ADD("quickload", vtech1, 1.5)

	MDRV_CASSETTE_ADD( "cassette", laser_cassette_config )

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom")

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("66K")
	MDRV_RAM_EXTRA_OPTIONS("2K,18K,4098K")
	
	MDRV_FLOPPY_2_DRIVES_ADD(vtech1_floppy_config)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( laser200 )
    MDRV_IMPORT_FROM(laser110)

    MDRV_DEVICE_MODIFY("mc6847")
    MDRV_MC6847_PALETTE(NULL)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( laser210 )
    MDRV_IMPORT_FROM(laser200)
    MDRV_CPU_MODIFY("maincpu")
    MDRV_CPU_PROGRAM_MAP(laser210_mem)

	/* internal ram */
	MDRV_RAM_MODIFY("messram")
	MDRV_RAM_DEFAULT_SIZE("66K")
	MDRV_RAM_EXTRA_OPTIONS("6K,22K,4098K")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( laser310 )
    MDRV_IMPORT_FROM(laser200)
    MDRV_CPU_REPLACE("maincpu", Z80, VZ300_XTAL1_CLK / 5)  /* 3.546894 MHz */
    MDRV_CPU_PROGRAM_MAP(laser310_mem)

	/* internal ram */
	MDRV_RAM_MODIFY("messram")
	MDRV_RAM_DEFAULT_SIZE("66K")
	MDRV_RAM_EXTRA_OPTIONS("16K,32K,4098K")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( laser310h )
    MDRV_IMPORT_FROM(laser310)

    MDRV_CPU_MODIFY("maincpu")
    MDRV_CPU_IO_MAP(vtech1_shrg_io)

    MDRV_DEVICE_REMOVE("mc6847")
    MDRV_MC6847_ADD("mc6847", vtech1_shrg_mc6847_intf)
	MDRV_MC6847_TYPE(M6847_VERSION_ORIGINAL_PAL)
MACHINE_DRIVER_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

ROM_START( laser110 )
    ROM_REGION(0x6800, "maincpu", 0)
    ROM_LOAD("vtechv12.u09",   0x0000, 0x2000, CRC(99412d43) SHA1(6aed8872a0818be8e1b08ecdfd92acbe57a3c96d))
    ROM_LOAD("vtechv12.u10",   0x2000, 0x2000, CRC(e4c24e8b) SHA1(9d8fb3d24f3d4175b485cf081a2d5b98158ab2fb))
    ROM_CART_LOAD("cart",  0x4000, 0x27ff, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

/* The VZ-200 sold in Germany and the Netherlands came with BASIC V1.1, which
   is currently not dumped. */
ROM_START( vz200de )
    ROM_REGION(0x6800, "maincpu", 0)
    ROM_LOAD("vtechv11.u09",   0x0000, 0x2000, NO_DUMP)
    ROM_LOAD("vtechv11.u10",   0x2000, 0x2000, NO_DUMP)
    ROM_CART_LOAD("cart",  0x4000, 0x27ff, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

#define rom_las110de    rom_laser110
#define rom_laser200    rom_laser110
#define rom_fellow      rom_laser110

/* It's possible that the Texet TX8000 came with BASIC V1.0, but this
   needs to be verified */
#define rom_tx8000      rom_laser110

ROM_START( laser210 )
    ROM_REGION(0x6800, "maincpu", 0)
    ROM_LOAD("vtechv20.u09",   0x0000, 0x2000, CRC(cc854fe9) SHA1(6e66a309b8e6dc4f5b0b44e1ba5f680467353d66))
    ROM_LOAD("vtechv20.u10",   0x2000, 0x2000, CRC(7060f91a) SHA1(8f3c8f24f97ebb98f3c88d4e4ba1f91ffd563440))
    ROM_CART_LOAD("cart",  0x4000, 0x27ff, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

#define rom_las210de    rom_laser210
#define rom_vz200       rom_laser210

ROM_START( laser310 )
    ROM_REGION(0x6800, "maincpu", 0)
	ROM_SYSTEM_BIOS(0, "basic20", "BASIC V2.0")
    ROMX_LOAD("vtechv20.u12", 0x0000, 0x4000, CRC(613de12c) SHA1(f216c266bc09b0dbdbad720796e5ea9bc7d91e53), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "basic21", "BASIC V2.1 (hack)")
    ROMX_LOAD("vtechv21.u12", 0x0000, 0x4000, CRC(f7df980f) SHA1(5ba14a7a2eedca331b033901080fa5d205e245ea), ROM_BIOS(2))
    ROM_CART_LOAD("cart", 0x4000, 0x27ff, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

#define rom_vz300       rom_laser310
#define rom_laser310h   rom_laser310

/***************************************************************************
    GAME DRIVERS
***************************************************************************/

/*    YEAR  NAME       PARENT    COMPAT  MACHINE    INPUT   INIT     COMPANY                   FULLNAME                          FLAGS */
COMP( 1983, laser110,  0,        0,      laser110,  vtech1, vtech1,  "Video Technology",       "Laser 110",                      GAME_SHARE_ROMS )
COMP( 1983, las110de,  laser110, 0,      laser110,  vtech1, vtech1,  "Sanyo",                  "Laser 110 (Germany)",            GAME_SHARE_ROMS )
COMP( 1983, laser200,  0,        0,      laser200,  vtech1, vtech1,  "Video Technology",       "Laser 200",                      GAME_SHARE_ROMS )
COMP( 1983, vz200de,   laser200, 0,      laser200,  vtech1, vtech1,  "Video Technology",       "VZ-200 (Germany & Netherlands)", GAME_SHARE_ROMS )
COMP( 1983, fellow,    laser200, 0,      laser200,  vtech1, vtech1,  "Salora",                 "Fellow (Finland)",               GAME_SHARE_ROMS )
COMP( 1983, tx8000,    laser200, 0,      laser200,  vtech1, vtech1,  "Texet",                  "TX-8000 (UK)",                   GAME_SHARE_ROMS )
COMP( 1984, laser210,  0,        0,      laser210,  vtech1, vtech1,  "Video Technology",       "Laser 210",                      GAME_SHARE_ROMS )
COMP( 1984, vz200,     laser210, 0,      laser210,  vtech1, vtech1,  "Dick Smith Electronics", "VZ-200 (Oceania)",               GAME_SHARE_ROMS )
COMP( 1984, las210de,  laser210, 0,      laser210,  vtech1, vtech1,  "Sanyo",                  "Laser 210 (Germany)",            GAME_SHARE_ROMS )
COMP( 1984, laser310,  0,        0,      laser310,  vtech1, vtech1,  "Video Technology",       "Laser 310",                      GAME_SHARE_ROMS )
COMP( 1984, vz300,     laser310, 0,      laser310,  vtech1, vtech1,  "Dick Smith Electronics", "VZ-300 (Oceania)",               GAME_SHARE_ROMS )
COMP( 1984, laser310h, laser310, 0,      laser310h, vtech1, vtech1h, "Video Technology",       "Laser 310 (SHRG)",               GAME_UNOFFICIAL | GAME_SHARE_ROMS)
