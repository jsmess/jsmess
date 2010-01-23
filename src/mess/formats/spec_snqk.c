/***************************************************************************

  mess/machine/spectrum.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

  Changes:

  KT 31/1/00 - Added support for .Z80. At the moment only 48k files are supported!
  DJR 8/2/00 - Added checks to avoid trying to load 128K .Z80 files into 48K machine!
  DJR 20/2/00 - Added support for .TAP files.
  -----------------27/02/00 10:54-------------------
  KT 27/2/00 - Added my changes for the WAV support
  --------------------------------------------------
  DJR 14/3/00 - Fixed +3 tape loading and added option to 'rewind' tapes when end reached.
  DJR 21/4/00 - Added support for 128K .SNA and .Z80 files.
  DJR 21/4/00 - Ensure 48K Basic ROM is used when running 48K snapshots on 128K machine.
  DJR 03/5/00 - Fixed bug of not decoding last byte of .Z80 blocks.
  DJR 08/5/00 - Fixed TS2068 .TAP loading.
  DJR 19/5/00 - .TAP files are now classified as cassette files.
  DJR 02/6/00 - Added support for .SCR files (screendumps).

***************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/spectrum.h"
#include "eventlst.h"
#include "video/border.h"
#include "sound/ay8910.h"
#include "formats/spec_snqk.h"
#include "utils.h"


void spectrum_setup_sna(running_machine *machine, unsigned char *pSnapshot, unsigned long SnapshotSize);
void spectrum_setup_z80(running_machine *machine, unsigned char *pSnapshot, unsigned long SnapshotSize);
void spectrum_setup_sp(running_machine *machine, unsigned char *pSnapshot, unsigned long SnapshotSize);

/*******************************************************************
 *
 *      Update the memory and paging of the spectrum being emulated
 *
 *      if port_7ffd_data is -1 then machine is 48K - no paging
 *      if port_1ffd_data is -1 then machine is 128K
 *      if neither port is -1 then machine is +2a/+3
 *
 *      Note: the 128K .SNA and .Z80 file formats do not store the
 *      port 1FFD setting so it is necessary to calculate the appropriate
 *      value for the ROM paging.
 *
 *******************************************************************/
static void spectrum_update_paging(running_machine *machine)
{
    if (spectrum_128_port_7ffd_data == -1)
        return;
    if (spectrum_plus3_port_1ffd_data == -1)
        spectrum_128_update_memory(machine);

    else
    {
        if (spectrum_128_port_7ffd_data & 0x10)
            /* Page in Spec 48K basic ROM */
            spectrum_plus3_port_1ffd_data = 0x04;
        else
            spectrum_plus3_port_1ffd_data = 0;
        spectrum_plus3_update_memory(machine);
    }
}

/* Page in the 48K Basic ROM. Used when running 48K snapshots on a 128K machine. */
static void spectrum_page_basicrom(running_machine *machine)
{
    if (spectrum_128_port_7ffd_data == -1)
        return;
    spectrum_128_port_7ffd_data |= 0x10;
    spectrum_update_paging(machine);
}


SNAPSHOT_LOAD(spectrum)
{
	UINT8 *snapshot_data = NULL;

	snapshot_data = (UINT8*)malloc(snapshot_size);
	if (!snapshot_data)
		goto error;

	image_fread(image, snapshot_data, snapshot_size);

	if (!mame_stricmp(file_type, "sna"))
	{
		if ((snapshot_size != 49179) && (snapshot_size != 131103) && (snapshot_size != 147487))
		{
			logerror("Invalid .SNA file size.\n");
			goto error;
		}
		spectrum_setup_sna(image->machine, snapshot_data, snapshot_size);
	}
	else if (!mame_stricmp(file_type, "sp"))
	{
		if (snapshot_data[0]!='S' && snapshot_data[1]!='P')
		{
			logerror("Invalid .SP signature.\n");
			goto error;
		}
		spectrum_setup_sp(image->machine, snapshot_data, snapshot_size);
	}
	else
	{
		spectrum_setup_z80(image->machine, snapshot_data, snapshot_size);
	}
	free(snapshot_data);
	//logerror("Snapshot loaded - new PC = %04x\n", (unsigned) cpu_get_reg(devtag_get_device(image->machine, "maincpu"), Z80_PC) & 0x0ffff);
	return INIT_PASS;

error:
	if (snapshot_data)
		free(snapshot_data);
	return INIT_FAIL;
}

/*******************************************************************
 *
 *      Load a .SP file.
 *
 *      There are only 48K .SP files and their format is as follows:
 *      Offset  Size    Description (all registers stored with LSB first)
 *      0       2       "SP" (signature)
 *      2       2       Program length in bytes (normally 49152 for 48K snaps, or 16384 for 16K snaps)
 *      4       2       Program location (normally 16384)
 *      6       8       BC,DE,HL,AF
 *      14      4       IX,IY
 *      18      8       BC',DE',HL',AF'
 *      26      2       R,I
 *      28      4       SP,PC
 *      32      2       0 (reserved for future use)
 *      34      1   Border color
 *      35      1       0 (reserved for future use)
 *      36      2       Status word
 *      38      -       RAM dump
 *
 *  Status word:
 *  Bit Description
 *  15-8    Reserved for future use
 *  7-6 Reserved for internal use (0)
 *  5   Flash: 0=INK/1=PAPER
 *  4   Interrupt pending for execution
 *  3   If 1, IM 0; if 0, bit 1 determines interrupt mode
 *          (Spectrum v 0.99e had this behaviour reversed, and this
 *      bit was not used in versions previous to v 0.99e)
 *  2   IFF2 (internal use)
 *  1   Interrupt Mode (if bit 3 reset): 0=>IM1, 1=>IM2
 *  0   IFF1: 0=DI/1=EI
 *
 *******************************************************************/
void spectrum_setup_sp(running_machine *machine, unsigned char *pSnapshot, unsigned long SnapshotSize)
{
	int i;
	UINT8 lo, hi, data;
	UINT16 offset, size;
	UINT16 status;
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	lo = pSnapshot[2] & 0x0ff;
	hi = pSnapshot[3] & 0x0ff;
	size = (hi << 8) | lo;
	logerror ("Snapshot data size: %d.\n", size);
	lo = pSnapshot[4] & 0x0ff;
	hi = pSnapshot[5] & 0x0ff;
	offset = (hi << 8) | lo;
	logerror ("Snapshot data offset: %d.\n", offset);

	lo = pSnapshot[6] & 0x0ff;
	hi = pSnapshot[7] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_BC, (hi << 8) | lo);
	lo = pSnapshot[8] & 0x0ff;
	hi = pSnapshot[9] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_DE, (hi << 8) | lo);
	lo = pSnapshot[10] & 0x0ff;
	hi = pSnapshot[11] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_HL, (hi << 8) | lo);
	lo = pSnapshot[12] & 0x0ff;
	hi = pSnapshot[13] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_AF, (hi << 8) | lo);

	lo = pSnapshot[14] & 0x0ff;
	hi = pSnapshot[15] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IX, (hi << 8) | lo);
	lo = pSnapshot[16] & 0x0ff;
	hi = pSnapshot[17] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IY, (hi << 8) | lo);

	lo = pSnapshot[18] & 0x0ff;
	hi = pSnapshot[19] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_BC2, (hi << 8) | lo);
	lo = pSnapshot[20] & 0x0ff;
	hi = pSnapshot[21] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_DE2, (hi << 8) | lo);
	lo = pSnapshot[22] & 0x0ff;
	hi = pSnapshot[23] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_HL2, (hi << 8) | lo);
	lo = pSnapshot[24] & 0x0ff;
	hi = pSnapshot[25] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_AF2, (hi << 8) | lo);

	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_R, (pSnapshot[26] & 0x0ff));
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_I, (pSnapshot[27] & 0x0ff));

	lo = pSnapshot[28] & 0x0ff;
	hi = pSnapshot[29] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_SP, (hi << 8) | lo);
	lo = pSnapshot[30] & 0x0ff;
	hi = pSnapshot[31] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_PC, (hi << 8) | lo);

	lo = pSnapshot[32] & 0x0ff;
	hi = pSnapshot[33] & 0x0ff;
	if (((hi << 8) | lo)!=0)
		logerror("Unknown meaning of word on position 32: %04x.\n", (hi << 8) | lo);

	/* Set border colour */
	spectrum_PreviousFE = (spectrum_PreviousFE & 0xf8) | (pSnapshot[34] & 0x07);
	EventList_Reset();
	border_set_last_color(pSnapshot[34] & 0x07);
	border_force_redraw();

	if ((pSnapshot[35]&0x0ff)!=0)
		logerror("Unknown meaning of byte on position 35: %02x.\n", pSnapshot[35]&0x0ff);

        lo = pSnapshot[36] & 0x0ff;
	hi = pSnapshot[37] & 0x0ff;
	status = (hi << 8) | lo;

	data = (status & 0x01);
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IFF1, data);
	data = (status & 0x04) >> 2;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IFF2, data);

	data = (status & 0x08) >> 3;
	if (data)
		cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IM, 0);
	else
	{
		data = (status & 0x02) >> 1;
		if (data)
			cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IM, 2);
		else
			cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IM, 1);
	}

	data = (status & 0x10) >> 4;
	cputag_set_input_line(machine, "maincpu", 0, data);
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, data);
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, 0);

	spectrum_page_basicrom(machine);

	/* memory dump */
	for (i = 0; i < size; i++)
		memory_write_byte(space, i + offset, pSnapshot[38 + i]);
}

/*******************************************************************
 *
 *      Load a 48K or 128K .SNA file.
 *
 *      48K Format as follows:
 *      Offset  Size    Description (all registers stored with LSB first)
 *      0       1       I
 *      1       18      HL',DE',BC',AF',HL,DE,BC,IY,IX
 *      19      1       Interrupt (bit 2 contains IFF2 1=EI/0=DI
 *      20      1       R
 *      21      4       AF,SP
 *      25      1       Interrupt Mode (0=IM0/1=IM1/2=IM2)
 *      26      1       Border Colour (0..7)
 *      27      48K     RAM dump 0x4000-0xffff
 *      PC is stored on stack.
 *
 *      128K Format as follows:
 *      Offset  Size    Description
 *      0       27      Header as 48K
 *      27      16K     RAM bank 5 (0x4000-0x7fff)
 *      16411   16K     RAM bank 2 (0x8000-0xbfff)
 *      32795   16K     RAM bank n (0xc000-0xffff - currently paged bank)
 *      49179   2       PC
 *      49181   1       port 7FFD setting
 *      49182   1       TR-DOS rom paged (1=yes)
 *      49183   16K     remaining RAM banks in ascending order
 *
 *      The bank in 0xc000 is always included even if it is page 2 or 5
 *      in which case it is included twice.
 *
 *******************************************************************/
void spectrum_setup_sna(running_machine *machine, unsigned char *pSnapshot, unsigned long SnapshotSize)
{
	int i, j, usedbanks[8];
	long bank_offset;
	unsigned char lo, hi, data;
	unsigned short addr;
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	if ((SnapshotSize != 49179) && (spectrum_128_port_7ffd_data == -1))
	{
		logerror("Can't load 128K .SNA file into 48K machine\n");
		return;
	}

	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_I, (pSnapshot[0] & 0x0ff));
	lo = pSnapshot[1] & 0x0ff;
	hi = pSnapshot[2] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_HL2, (hi << 8) | lo);
	lo = pSnapshot[3] & 0x0ff;
	hi = pSnapshot[4] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_DE2, (hi << 8) | lo);
	lo = pSnapshot[5] & 0x0ff;
	hi = pSnapshot[6] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_BC2, (hi << 8) | lo);
	lo = pSnapshot[7] & 0x0ff;
	hi = pSnapshot[8] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_AF2, (hi << 8) | lo);
	lo = pSnapshot[9] & 0x0ff;
	hi = pSnapshot[10] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_HL, (hi << 8) | lo);
	lo = pSnapshot[11] & 0x0ff;
	hi = pSnapshot[12] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_DE, (hi << 8) | lo);
	lo = pSnapshot[13] & 0x0ff;
	hi = pSnapshot[14] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_BC, (hi << 8) | lo);
	lo = pSnapshot[15] & 0x0ff;
	hi = pSnapshot[16] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IY, (hi << 8) | lo);
	lo = pSnapshot[17] & 0x0ff;
	hi = pSnapshot[18] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IX, (hi << 8) | lo);
	data = (pSnapshot[19] & 0x04) >> 2;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IFF2, data);
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IFF1, data);
	data = (pSnapshot[20] & 0x0ff);
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_R, data);
	lo = pSnapshot[21] & 0x0ff;
	hi = pSnapshot[22] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_AF, (hi << 8) | lo);
	lo = pSnapshot[23] & 0x0ff;
	hi = pSnapshot[24] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_SP, (hi << 8) | lo);
	data = (pSnapshot[25] & 0x0ff);
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IM, data);

	/* Set border colour */
	spectrum_PreviousFE = (spectrum_PreviousFE & 0xf8) | (pSnapshot[26] & 0x07);
	EventList_Reset();
	border_set_last_color(pSnapshot[26] & 0x07);
	border_force_redraw();

	cputag_set_input_line(machine, "maincpu", 0, data);
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, data);
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, 0);

	if (SnapshotSize == 49179)
		/* 48K Snapshot */
		spectrum_page_basicrom(machine);
	else
	{
		/* 128K Snapshot */
		spectrum_128_port_7ffd_data = (pSnapshot[49181] & 0x0ff);
		spectrum_update_paging(machine);
	}

	/* memory dump */
	for (i = 0; i < 49152; i++)
	{
		memory_write_byte(space,i + 16384, pSnapshot[27 + i]);
	}

	if (SnapshotSize == 49179)
	{
		/* get pc from stack */
		addr = cpu_get_reg(devtag_get_device(machine, "maincpu"), Z80_SP) & 0xFFFF;

		lo = memory_read_byte(space,addr + 0);
		hi = memory_read_byte(space,addr + 1);
		cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_PC, (hi << 8) | lo);
		memory_write_byte(space, addr + 0, 0);
		memory_write_byte(space, addr + 1, 0);

		memory_write_byte(space, 0x5CB0, 0xff); // Set NMIADD system variable to make game run
												//  when directly started with cart mounted
		addr += 2;
		cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_SP, (addr & 0x0ffff));
	}
	else
	{
		/* Set up other RAM banks */
		bank_offset = 49183;
		for (i = 0; i < 8; i++)
			usedbanks[i] = 0;

		usedbanks[5] = 1;				/* 0x4000-0x7fff */
		usedbanks[2] = 1;				/* 0x8000-0xbfff */
		usedbanks[spectrum_128_port_7ffd_data & 0x07] = 1;	/* Banked memory */

		for (i = 0; i < 8; i++)
		{
			if (!usedbanks[i])
			{
				logerror("Loading bank %d from offset %ld\n", i, bank_offset);
				spectrum_128_port_7ffd_data &= 0xf8;
				spectrum_128_port_7ffd_data += i;
				spectrum_update_paging(machine);
				for (j = 0; j < 16384; j++)
					memory_write_byte(space,j + 49152, pSnapshot[bank_offset + j]);
				bank_offset += 16384;
			}
		}

		/* Reset paging */
		spectrum_128_port_7ffd_data = (pSnapshot[49181] & 0x0ff);
		spectrum_update_paging(machine);

		/* program counter */
		lo = pSnapshot[49179] & 0x0ff;
		hi = pSnapshot[49180] & 0x0ff;
		cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_PC, (hi << 8) | lo);
	}
}


static void spectrum_z80_decompress_block(running_machine *machine,unsigned char *pSource, int Dest, int size)
{
	unsigned char ch;
	int i;
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	do
	{
		/* get byte */
		ch = pSource[0];

		/* either start 0f 0x0ed, 0x0ed, xx yy or
         * single 0x0ed */
		if (ch == (unsigned char) 0x0ed)
		{
			if (pSource[1] == (unsigned char) 0x0ed)
			{

				/* 0x0ed, 0x0ed, xx yy */
				/* repetition */

				int count;
				int data;

				count = (pSource[2] & 0x0ff);

				if (count == 0)
					return;

				data = (pSource[3] & 0x0ff);

				pSource += 4;

				if (count > size)
					count = size;

				size -= count;

				for (i = 0; i < count; i++)
				{
					memory_write_byte(space,Dest, data);
					Dest++;
				}
			}
			else
			{
				/* single 0x0ed */
				memory_write_byte(space,Dest, ch);
				Dest++;
				pSource++;
				size--;

			}
		}
		else
		{
			/* not 0x0ed */
			memory_write_byte(space,Dest, ch);
			Dest++;
			pSource++;
			size--;
		}

	}
	while (size > 0);
}

static SPECTRUM_Z80_SNAPSHOT_TYPE spectrum_identify_z80 (unsigned char *pSnapshot, unsigned long SnapshotSize)
{
	unsigned char lo, hi, data;

	if (SnapshotSize < 30)
		return SPECTRUM_Z80_SNAPSHOT_INVALID;	/* Invalid file */

	lo = pSnapshot[6] & 0x0ff;
	hi = pSnapshot[7] & 0x0ff;
	if (hi || lo)
		return SPECTRUM_Z80_SNAPSHOT_48K_OLD;	/* V1.45 - 48K only */

	lo = pSnapshot[30] & 0x0ff;
	hi = pSnapshot[31] & 0x0ff;
	data = pSnapshot[34] & 0x0ff;			/* Hardware mode */

	if ((hi == 0) && (lo == 23))
	{						/* V2.01 */							/* V2.01 format */
		switch (data)
		{
			case 0:
			case 1:	return SPECTRUM_Z80_SNAPSHOT_48K;
			case 2:	return SPECTRUM_Z80_SNAPSHOT_SAMRAM;
			case 3:
			case 4:	return SPECTRUM_Z80_SNAPSHOT_128K;
			case 128: return SPECTRUM_Z80_SNAPSHOT_TS2068;
		}
	}

	if ((hi == 0) && (lo == 54))
	{						/* V3.0x */							/* V2.01 format */
		switch (data)
		{
			case 0:
			case 1:
			case 3:	return SPECTRUM_Z80_SNAPSHOT_48K;
			case 2:	return SPECTRUM_Z80_SNAPSHOT_SAMRAM;
			case 4:
			case 5:
			case 6:	return SPECTRUM_Z80_SNAPSHOT_128K;
			case 128: return SPECTRUM_Z80_SNAPSHOT_TS2068;
		}
	}

	return SPECTRUM_Z80_SNAPSHOT_INVALID;
}

/* now supports 48k & 128k .Z80 files */
void spectrum_setup_z80(running_machine *machine, unsigned char *pSnapshot, unsigned long SnapshotSize)
{
	int i;
	unsigned char lo, hi, data;
	SPECTRUM_Z80_SNAPSHOT_TYPE z80_type;
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	z80_type = spectrum_identify_z80(pSnapshot, SnapshotSize);

	switch (z80_type)
	{
		case SPECTRUM_Z80_SNAPSHOT_INVALID:
				logerror("Invalid .Z80 file\n");
				return;
		case SPECTRUM_Z80_SNAPSHOT_48K_OLD:
		case SPECTRUM_Z80_SNAPSHOT_48K:
				logerror("48K .Z80 file\n");
				if (!strcmp(machine->gamedrv->name,"ts2068"))
					logerror("48K .Z80 file in TS2068\n");
				break;
		case SPECTRUM_Z80_SNAPSHOT_128K:
				logerror("128K .Z80 file\n");
				if (spectrum_128_port_7ffd_data == -1)
				{
					logerror("Not a 48K .Z80 file\n");
					return;
				}
				if (!strcmp(machine->gamedrv->name,"ts2068"))
				{
					logerror("Not a TS2068 .Z80 file\n");
					return;
				}
				break;
		case SPECTRUM_Z80_SNAPSHOT_TS2068:
				logerror("TS2068 .Z80 file\n");
				if (strcmp(machine->gamedrv->name,"ts2068"))
					logerror("Not a TS2068 machine\n");
				break;
		case SPECTRUM_Z80_SNAPSHOT_SAMRAM:
				logerror("Hardware not supported - .Z80 file\n");
				return;
	}

	/* AF */
	hi = pSnapshot[0] & 0x0ff;
	lo = pSnapshot[1] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_AF, (hi << 8) | lo);
	/* BC */
	lo = pSnapshot[2] & 0x0ff;
	hi = pSnapshot[3] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_BC, (hi << 8) | lo);
	/* HL */
	lo = pSnapshot[4] & 0x0ff;
	hi = pSnapshot[5] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_HL, (hi << 8) | lo);

	/* SP */
	lo = pSnapshot[8] & 0x0ff;
	hi = pSnapshot[9] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_SP, (hi << 8) | lo);

	/* I */
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_I, (pSnapshot[10] & 0x0ff));

	/* R */
	data = (pSnapshot[11] & 0x07f) | ((pSnapshot[12] & 0x01) << 7);
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_R, data);

	/* Set border colour */
	spectrum_PreviousFE = (spectrum_PreviousFE & 0xf8) | ((pSnapshot[12] & 0x0e) >> 1);
	EventList_Reset();
	border_set_last_color((pSnapshot[12] & 0x0e) >> 1);
	border_force_redraw();

	lo = pSnapshot[13] & 0x0ff;
	hi = pSnapshot[14] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_DE, (hi << 8) | lo);

	lo = pSnapshot[15] & 0x0ff;
	hi = pSnapshot[16] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_BC2, (hi << 8) | lo);

	lo = pSnapshot[17] & 0x0ff;
	hi = pSnapshot[18] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_DE2, (hi << 8) | lo);

	lo = pSnapshot[19] & 0x0ff;
	hi = pSnapshot[20] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_HL2, (hi << 8) | lo);

	hi = pSnapshot[21] & 0x0ff;
	lo = pSnapshot[22] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_AF2, (hi << 8) | lo);

	lo = pSnapshot[23] & 0x0ff;
	hi = pSnapshot[24] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IY, (hi << 8) | lo);

	lo = pSnapshot[25] & 0x0ff;
	hi = pSnapshot[26] & 0x0ff;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IX, (hi << 8) | lo);

	/* Interrupt Flip/Flop */
	if (pSnapshot[27] == 0)
	{
		cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IFF1, 0);
		/* cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IRQ_STATE, 0); */
	}
	else
	{
		cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IFF1, 1);
		/* cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IRQ_STATE, 1); */
	}

	cputag_set_input_line(machine, "maincpu", 0, data);
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, data);
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, 0);

	/* IFF2 */
	if (pSnapshot[28] != 0)
	{
		data = 1;
	}
	else
	{
		data = 0;
	}
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IFF2, data);

	/* Interrupt Mode */
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_IM, (pSnapshot[29] & 0x03));

	if (z80_type == SPECTRUM_Z80_SNAPSHOT_48K_OLD)
	{
		lo = pSnapshot[6] & 0x0ff;
		hi = pSnapshot[7] & 0x0ff;
		cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_PC, (hi << 8) | lo);

		spectrum_page_basicrom(machine);

		if ((pSnapshot[12] & 0x020) == 0)
		{
			logerror("Not compressed\n");	/* not compressed */
			for (i = 0; i < 49152; i++)
				memory_write_byte(space,i + 16384, pSnapshot[30 + i]);
		}
		else
		{
			logerror("Compressed\n");	/* compressed */
			spectrum_z80_decompress_block(machine, pSnapshot + 30, 16384, 49152);
		}
	}
	else
	{
		unsigned char *pSource;
		int header_size;

		header_size = 30 + 2 + ((pSnapshot[30] & 0x0ff) | ((pSnapshot[31] & 0x0ff) << 8));

		lo = pSnapshot[32] & 0x0ff;
		hi = pSnapshot[33] & 0x0ff;
		cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_PC, (hi << 8) | lo);

		if ((z80_type == SPECTRUM_Z80_SNAPSHOT_128K) || ((z80_type == SPECTRUM_Z80_SNAPSHOT_TS2068) && !strcmp(machine->gamedrv->name,"ts2068")))
		{
			const device_config *ay8912 = devtag_get_device(machine, "ay8912");

			/* Only set up sound registers for 128K machine or TS2068! */
			for (i = 0; i < 16; i++)
			{
				ay8910_address_w(ay8912, 0, i);
				ay8910_data_w(ay8912, 0, pSnapshot[39 + i]);
			}
			ay8910_address_w(ay8912, 0, pSnapshot[38]);
		}

		pSource = pSnapshot + header_size;

		if (z80_type == SPECTRUM_Z80_SNAPSHOT_48K)
			/* Ensure 48K Basic ROM is used */
			spectrum_page_basicrom(machine);

		do
		{
			unsigned short length;
			unsigned char page;
			int Dest = 0;

			length = (pSource[0] & 0x0ff) | ((pSource[1] & 0x0ff) << 8);
			page = pSource[2];

			if (z80_type == SPECTRUM_Z80_SNAPSHOT_48K || z80_type == SPECTRUM_Z80_SNAPSHOT_TS2068)
			{
				switch (page)
				{
					case 4:	Dest = 0x08000;	break;
					case 5:	Dest = 0x0c000;	break;
					case 8:	Dest = 0x04000;	break;
					default: Dest = 0; break;
				}
			}
			else
			{
				/* 3 = bank 0, 4 = bank 1 ... 10 = bank 7 */
				if ((page >= 3) && (page <= 10))
				{
					/* Page the appropriate bank into 0xc000 - 0xfff */
					spectrum_128_port_7ffd_data = page - 3;
					spectrum_update_paging(machine);
					Dest = 0x0c000;
				}
				else
					/* Other values correspond to ROM pages */
					Dest = 0x0;
			}

			if (Dest != 0)
			{
				if (length == 0x0ffff)
				{
					/* block is uncompressed */
					logerror("Not compressed\n");

					/* not compressed */
					for (i = 0; i < 16384; i++)
						memory_write_byte(space,i + Dest, pSource[i]);
				}
				else
				{
					logerror("Compressed\n");

					/* block is compressed */
					spectrum_z80_decompress_block(machine,&pSource[3], Dest, 16384);
				}
			}

			/* go to next block */
			pSource += (3 + length);
		}
		while ((pSource - pSnapshot) < SnapshotSize);

		if ((spectrum_128_port_7ffd_data != -1) && (z80_type != SPECTRUM_Z80_SNAPSHOT_48K))
		{
			/* Set up paging */
			spectrum_128_port_7ffd_data = (pSnapshot[35] & 0x0ff);
			spectrum_update_paging(machine);
		}
		if ((z80_type == SPECTRUM_Z80_SNAPSHOT_48K) && !strcmp(machine->gamedrv->name,"ts2068"))
		{
			ts2068_port_f4_data = 0x03;
			ts2068_port_ff_data = 0x00;
			ts2068_update_memory(machine);
		}
		if (z80_type == SPECTRUM_Z80_SNAPSHOT_TS2068 && !strcmp(machine->gamedrv->name,"ts2068"))
		{
			ts2068_port_f4_data = pSnapshot[35];
			ts2068_port_ff_data = pSnapshot[36];
			ts2068_update_memory(machine);
		}
	}
}

QUICKLOAD_LOAD(spectrum)
{
	const address_space *space = cputag_get_address_space(image->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	int i;
	int quick_addr = 0x4000;
	int quick_length;
	UINT8 *quick_data;
	int read_;

	quick_length = image_length(image);
	quick_data = (UINT8 *)malloc(quick_length);
	if (!quick_data)
		return INIT_FAIL;

	read_ = image_fread(image, quick_data, quick_length);
	if (read_ != quick_length)
		return INIT_FAIL;

	for (i = 0; i < quick_length; i++)
		memory_write_byte(space,i + quick_addr, quick_data[i]);

	logerror("quick loading at %.4x size:%.4x\n", quick_addr, quick_length);
	return INIT_PASS;
}
