/******************************************************************************
 *
 *	cpm_bios.c
 *
 *	CP/M bios emulation (used for Kaypro 2x)
 *
 *	Juergen Buchmueller, July 1998
 *	Benjamin C. W. Sittler, July 1998 (minor modifications)
 *
 ******************************************************************************/

#include "driver.h"
#include "memory.h"
#include "cpu/z80/z80.h"
#include "machine/wd17xx.h"
#include "cpm_bios.h"
#include "image.h"

#define VERBOSE 		1
#define VERBOSE_FDD 	1
#define VERBOSE_BIOS	1
#define VERBOSE_CONIO	0

/* buffer for one physical sector */
typedef struct {
	UINT8	 unit;			/* unit number to use for this drive */
	UINT8	 cyl;			/* physical cylinder */
	UINT8	 side;			/* physical side */
	UINT8	 head;			/* logical head number */
	UINT8	 sec;			/* physical sector number */
	UINT8	 dirty; 		/* non zero if the sector buffer is dirty */
	UINT8	 buffer[4096];	/* buffer to hold one physical sector */
}	SECBUF;

static int curdisk = 0; 			/* currently selected disk */
static int num_disks = 0;			/* number of supported disks */
static int fmt[NDSK] = {0,};		/* index of disk formats */
static int mode[NDSK] = {0,};		/* 0 read only, !0 read/write */
static int bdos_trk[NDSK] = {0,};	/* BDOS track number */
static int bdos_sec[NDSK] = {0,};	/* BDOS sector number */
static mess_image *fp[NDSK] = {NULL, };	/* image file pointer */
static int ff[NDSK] = {0, };            /* image filenames specified flags */
static mame_file *lp = NULL; 			/* list file handle (ie. PIP LST:=X:FILE.EXT) */
//static mess_image *pp = NULL;			/* punch file handle (ie. PIP PUN:=X:FILE.EXT) */
//static mess_image *rp = NULL;			/* reader file handle (ie. PIP X:FILE.EXE=RDR:) */
static int dma = 0; 				/* DMA transfer address */

static UINT8 zeropage0[8] =
{
	0xc3,							/* JP  BIOS+3 */
	BIOS & 0xff,
	BIOS >> 8,
	0x80,							/* io byte */
	0x00,							/* usr drive */
	0xc3,							/* JP  BDOS */
	BDOS & 0xff,
	BDOS >> 8
};

cpm_dph dph[NDSK] = {
	{ 0, 0, 0, 0, DIRBUF, DPB0, 0, 0 },
	{ 0, 0, 0, 0, DIRBUF, DPB1, 0, 0 },
	{ 0, 0, 0, 0, DIRBUF, DPB2, 0, 0 },
	{ 0, 0, 0, 0, DIRBUF, DPB3, 0, 0 }
};

static void cpm_exit(running_machine *machine);

#include "cpm_disk.c"

#define CP(n) ((n)<32?'.':(n))


/*****************************************************************************
 *	cpm_jumptable
 *	initialize a jump table with 18 entries for bios functions
 *	initialize bios functions at addresses BIOS_00 .. BIOS_11
 *	and the central bios execute function at BIOS_EXEC
 *****************************************************************************/
static void cpm_jumptable(void)
{
	UINT8 * RAM = memory_get_write_ptr(0, ADDRESS_SPACE_PROGRAM, 0x0000);
	int i;
	int jmp_addr, fun_addr;

	/* reset RST7 vector (RET) */
	RAM[0x0038] = 0xc9;

	/* reset NMI vector (RETI) */
	RAM[0x0066] = 0xed;
	RAM[0x0067] = 0x5d;

	for (i = 0; i < NFUN; i++)
	{
		jmp_addr = BIOS + 3 * i;
		fun_addr = BIOS_00 + 4 * i;

		RAM[jmp_addr + 0] = 0xc3;
		RAM[jmp_addr + 1] = fun_addr & 0xff;
		RAM[jmp_addr + 2] = fun_addr >> 8;

		RAM[fun_addr + 0] = 0x3e;		/* LD	A,i */
		RAM[fun_addr + 1] = i;
		RAM[fun_addr + 2] = 0x18;		/* JR	BIOS_EXEC */
		RAM[fun_addr + 3] = BIOS_EXEC - fun_addr - 4;
	}

	/* initialize bios execute */
	RAM[BIOS_EXEC + 0] = 0xd3;			/* OUT (BIOS_CMD),A */
	RAM[BIOS_EXEC + 1] = BIOS_CMD;
	RAM[BIOS_EXEC + 2] = 0xc9;			/* RET */
}

DEVICE_LOAD( cpm_floppy )
{
	int id = image_index_in_device(image);

	ff[id] = TRUE;

	/* now try to open the image if a filename is given */
	fp[id] = image;
	mode[id] = image_is_writable(image);

	return INIT_PASS;
}

/*TODO: implement DEVICE_UNLOAD that clears ff[id]*/

/*****************************************************************************
 *	cpm_init
 *	Initialize the BIOS simulation for 'n' drives of types stored in 'ids'
 *****************************************************************************/
int cpm_init(int n, const char *ids[])
{
	UINT8 * RAM = memory_get_write_ptr(0, ADDRESS_SPACE_PROGRAM, 0x0000);
	dsk_fmt *f;
	int i, d;


	/* fill memory with HALT insn */
	memset(RAM + 0x100, 0x76, 0xff00);

	/* Reset jump */
	zeropage0[0] = 0xc3;
	zeropage0[1] = BIOS & 0xff;
	zeropage0[2] = BIOS >> 8;

	/* Copy substantial zero page data */
	for (i = 0; i < 8; i++)
		RAM[i] = zeropage0[i];

	if (VERBOSE)
	{
		logerror("CPM CCP     %04X\n", CCP);
		logerror("CPM BDOS    %04X\n", BDOS);
		logerror("CPM BIOS    %04X\n", BIOS);
	}

	/* Start allocating CSV & ALV */
	i = DATA_START;

	num_disks = n;

	for (d = 0; d < num_disks; d++)
	{
		if (ids[d])
		{
			/* select a format by name */
			if( cpm_disk_select_format(d, ids[d]) )
			{
				if (VERBOSE)
					logerror("Error drive #%d id '%s' is unknown\n", d, ids[d]);
				return 1;
			}

			f = &formats[fmt[d]];

			/* transfer DPB to memory */
			RAM[DPB0 + d * DPBL +  0] = f->dpb.spt & 0xff;
			RAM[DPB0 + d * DPBL +  1] = f->dpb.spt >> 8;
			RAM[DPB0 + d * DPBL +  2] = f->dpb.bsh;
			RAM[DPB0 + d * DPBL +  3] = f->dpb.blm;
			RAM[DPB0 + d * DPBL +  4] = f->dpb.exm;
			RAM[DPB0 + d * DPBL +  5] = f->dpb.dsm & 0xff;
			RAM[DPB0 + d * DPBL +  6] = f->dpb.dsm >> 8;
			RAM[DPB0 + d * DPBL +  7] = f->dpb.drm & 0xff;
			RAM[DPB0 + d * DPBL +  8] = f->dpb.drm >> 8;
			RAM[DPB0 + d * DPBL +  9] = f->dpb.al0;
			RAM[DPB0 + d * DPBL + 10] = f->dpb.al1;
			RAM[DPB0 + d * DPBL + 11] = f->dpb.cks & 0xff;
			RAM[DPB0 + d * DPBL + 12] = f->dpb.cks >> 8;
			RAM[DPB0 + d * DPBL + 13] = f->dpb.off & 0xff;
			RAM[DPB0 + d * DPBL + 14] = f->dpb.off >> 8;

			if (VERBOSE)
				logerror("CPM DPB%d    %04X\n", d, DPB0 + d * DPBL);

			/* TODO: generate sector translation table and
			   allocate space for it; for now it is always 0 */
			dph[d].xlat = 0;
			if (VERBOSE)
				logerror("CPM XLAT%d   %04X\n", d, dph[d].xlat);

			/* memory address for dirbuf */
			dph[d].dirbuf = DIRBUF + d * RECL;
			if (VERBOSE)
				logerror("CPM DIRBUF%d %04X\n", d, dph[d].dirbuf);

			/* memory address for CSV */
			dph[d].csv = i;
			if (VERBOSE)
				logerror("CPM CSV%d    %04X\n", d, dph[d].csv);

			/* length is CKS bytes */
			i += f->dpb.cks;

			/* memory address for ALV */
			dph[d].alv = i;
			if (VERBOSE)
				logerror("CPM ALV%d    %04X\n", d, dph[d].alv);

			/* length is DSM+1 bits */
			i += (f->dpb.dsm + 1 + 7) >> 3;

			if (i >= BIOS_00)
			{
				if (VERBOSE)
					logerror("Error configuring BIOS tables: out of memory!\n");
				return 1;
			}
		}
		else
		{
			memset(&dph[d], 0, DPHL);
		}

		logerror("CPM DPH%d    %04X\n", d, DPH0 + d * DPHL);

		/* transfer DPH to memory */
		RAM[DPH0 + d * DPHL +  0] = dph[d].xlat & 0xff;
		RAM[DPH0 + d * DPHL +  1] = dph[d].xlat >> 8;
		RAM[DPH0 + d * DPHL +  2] = dph[d].scr1 & 0xff;
		RAM[DPH0 + d * DPHL +  3] = dph[d].scr1 >> 8;
		RAM[DPH0 + d * DPHL +  4] = dph[d].scr2 & 0xff;
		RAM[DPH0 + d * DPHL +  5] = dph[d].scr2 >> 8;
		RAM[DPH0 + d * DPHL +  6] = dph[d].scr3 & 0xff;
		RAM[DPH0 + d * DPHL +  7] = dph[d].scr3 >> 8;
		RAM[DPH0 + d * DPHL +  8] = dph[d].dirbuf & 0xff;
		RAM[DPH0 + d * DPHL +  9] = dph[d].dirbuf >> 8;
		RAM[DPH0 + d * DPHL + 10] = dph[d].dpb & 0xff;
		RAM[DPH0 + d * DPHL + 11] = dph[d].dpb >> 8;
		RAM[DPH0 + d * DPHL + 12] = dph[d].csv & 0xff;
		RAM[DPH0 + d * DPHL + 13] = dph[d].csv >> 8;
		RAM[DPH0 + d * DPHL + 14] = dph[d].alv & 0xff;
		RAM[DPH0 + d * DPHL + 15] = dph[d].alv >> 8;
	}

	/* create a file to receive list output (ie. PIP LST:=FILE.EXT) */
	mame_fopen(SEARCHPATH_IMAGE, "cpm.lst", OPEN_FLAG_WRITE, &lp);

	cpm_jumptable();

	add_exit_callback(Machine, cpm_exit);
	return 0;
}

/*****************************************************************************
 *	cpm_exit
 *	Shut down the BIOS simulation; close image files; exit osd_fdc
 *****************************************************************************/
static void cpm_exit(running_machine *machine)
{
	int d;

	/* if a list file is still open close it now */
	if (lp)
	{
		mame_fclose(lp);
		lp = NULL;
	}

	/* close all opened disk images */
	for (d = 0; d < NDSK; d++)
	{
		if (fp[d])
		{
			fp[d] = NULL;
		}
	}
}

/*****************************************************************************
 * cpm_conout_chr
 * send a character to the console
 *****************************************************************************/
static void cpm_conout_chr(int data)
{
	io_write_byte_8(BIOS_CONOUT, data);
}

/*****************************************************************************
 * cpm_conout_str
 * send a zero terminated string to the console
 *****************************************************************************/
static void cpm_conout_str(const char *src)
{
	while (*src)
		cpm_conout_chr(*src++);
}

/*****************************************************************************
 * cpm_conout_str
 * select a disk format for drive 'd' by searching formats for 'id'
 *****************************************************************************/
int cpm_disk_select_format(int d, const char *id)
{
	int i, j;

	if (!id)
		return 1;

	if (VERBOSE)
		logerror("CPM search format '%s'\n", id);

	for (i = 0; formats[i].id && mame_stricmp(formats[i].id, id); i++)
		;

	if (!formats[i].id)
	{
		if (VERBOSE)
			logerror("CPM format '%s' not supported\n", id);
		return 1;
	}

	/* references another type id? */
	if (formats[i].ref)
	{
		if (VERBOSE)
			logerror("CPM search format '%s' for '%s'\n", formats[i].ref, id);

		/* search for the referred id */
		for (j = 0; formats[j].id && mame_stricmp(formats[j].id, formats[i].ref); j++)
			;
		if (!formats[j].id)
		{
			if (VERBOSE)
				logerror("CPM format '%s' not supported\n", formats[i].ref);
			return 1;
		}
		/* set current format */
		fmt[d] = j;
	}
	else
	{
		/* set current format */
		fmt[d] = i;
	}

	if (VERBOSE)
	{
		dsk_fmt *f = &formats[fmt[d]];

		logerror("CPM '%s' is '%s'\n", id, f->name);
		logerror("CPM #%d %dK (%d cylinders, %d sides, %d %d bytes sectors/track)\n",
			d,
			f->cylinders * f->sides * f->spt * f->seclen / 1024,
			f->cylinders, f->sides, f->spt, f->seclen);
	}

	return 0;
}


/*****************************************************************************
 * cpm_disk_image_seek
 * seek to an image offset for the currently selected disk 'curdisk'
 * the track 'bdos_trk[curdisk]' and sector 'bdos_sec[curdisk]' are used
 * to calculate the offset.
 *****************************************************************************/
static void cpm_disk_image_seek(void)
{
	dsk_fmt *f = &formats[fmt[curdisk]];
	int offs, o, r, s, h;

	switch (f->order)
	{
	/* TRACK  0   1   2 	   n/2-1	   n/2	   n/2+1	 n/2+2	  n-1 */
	/* F/B	 f0, f1, f2 ... f(n/2)-1, b(n/2)-1, b(n/2)-2, b(n/2)-3 ... b0 */
	case ORD_CYLINDERS:
		offs = (bdos_trk[curdisk] % f->cylinders) * 2;
		offs *= f->dpb.spt * RECL;
		o = bdos_sec[curdisk] * RECL;
		r = (o % f->seclen) / RECL;
		s = o / f->seclen;
		h = (s >= f->spt) ? f->side2[0] : f->side1[0];
		s = (s >= f->spt) ? f->side2[s+1] : f->side1[s+1];
		s -= f->side1[1];		/* subtract first sector number */
		offs += (h * f->spt + s) * f->seclen + r * RECL;
		if (bdos_trk[curdisk] >= f->cylinders)
			offs += (int)f->cylinders * f->sides * f->spt * f->seclen / 2;

		if (VERBOSE_FDD)
			logerror("CPM image #%d ord_cylinders C:%2d H:%d S:%2d REC:%d -> 0x%08X\n", curdisk, bdos_trk[curdisk], h, s, r, offs * RECL);
		break;

	/* TRACK  0   1   2 	   n/2-1 n/2  n/2+1  n/2+3	   n-1		*/
	/* F/B	 f0, f1, f2 ... f(n/2)-1, b0,	 b1,	b2 ... b(n/2)-1 */
	case ORD_EAGLE:
		offs = (bdos_trk[curdisk] % f->cylinders) * 2;
		if (bdos_trk[curdisk] >= f->cylinders)
			offs = f->cylinders * 2 - 1 - offs;
		offs *= f->dpb.spt * RECL;
		o = bdos_sec[curdisk] * RECL;
		r = (o % f->seclen) / RECL;
		s = o / f->seclen;
		h = (s >= f->spt) ? f->side2[0] : f->side1[0];
		s = (s >= f->spt) ? f->side2[s+1] : f->side1[s+1];
		s -= f->side1[1];		/* subtract first sector number */
		offs += (h * f->spt + s) * f->seclen + r * RECL;

		if (VERBOSE_FDD)
			logerror("CPM image #%d ord_eagle     C:%2d H:%d S:%2d REC:%d -> 0x%08X\n", curdisk, bdos_trk[curdisk], h, s, r, offs);
		break;

	/* TRACK	 0		1	  2 		  n-1 */
	/* F/B	 f0/b0, f1/b1 f2/b2 ... fn-1/bn-1 */
	case ORD_SIDES:
	default:
		offs = bdos_trk[curdisk];
		offs *= f->dpb.spt * RECL;
		o = bdos_sec[curdisk] * RECL;
		r = (o % f->seclen) / RECL;
		s = o / f->seclen;
		h = (s >= f->spt) ? f->side2[0] : f->side1[0];
		s = (s >= f->spt) ? f->side2[s+1] : f->side1[s+1];
		s -= f->side1[1];		/* subtract first sector number */
		offs += (h * f->spt + s) * f->seclen + r * RECL;

		if (VERBOSE_FDD)
			logerror("CPM image #%d ord_sides     C:%2d H:%d S:%2d REC:%d -> 0x%08X\n", curdisk, bdos_trk[curdisk], h, s, r, offs);
		break;

	}
	image_fseek(fp[curdisk], offs, SEEK_SET);
}


/*****************************************************************************
 * cpm_disk_select
 * select a disk drive and return it's DPH (disk parameter header)
 * offset in Z80s memory range.
 *****************************************************************************/
static int cpm_disk_select(int d)
{
	int return_dph = 0;

	curdisk = d;
	switch (curdisk)
	{
		case 0:
			if (num_disks > 0)
				return_dph = DPH0;
			break;
		case 1:
			if (num_disks > 1)
				return_dph	= DPH1;
			break;
		case 2:
			if (num_disks > 2)
				return_dph	= DPH2;
			break;
		case 3:
			if (num_disks > 3)
				return_dph	= DPH3;
			break;
	}
	return return_dph;

}

void cpm_disk_set_track(int t)
{
	if (curdisk >= 0 && curdisk < num_disks)
		bdos_trk[curdisk] = t;
}

void cpm_disk_set_sector(int s)
{
	if (curdisk >= 0 && curdisk < num_disks)
		bdos_sec[curdisk] = s;
}

void cpm_disk_home(void)
{
	cpm_disk_set_track(0);
}

void cpm_disk_set_dma(int d)
{
	dma = d;
}

static void dump_sector(void)
{
	UINT8 *DMA = ((UINT8 *) memory_get_write_ptr(0, ADDRESS_SPACE_PROGRAM, 0x0000)) + dma;

	{
		int i;
		for (i = 0x00; i < 0x80; i += 0x10)
		{
			logerror("%02X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
				i,
				DMA[i+0x00],DMA[i+0x01],DMA[i+0x02],DMA[i+0x03],
				DMA[i+0x04],DMA[i+0x05],DMA[i+0x06],DMA[i+0x07],
				DMA[i+0x08],DMA[i+0x09],DMA[i+0x0a],DMA[i+0x0b],
				DMA[i+0x0c],DMA[i+0x0d],DMA[i+0x0e],DMA[i+0x0f],
				CP(DMA[i+0x00]),CP(DMA[i+0x01]),CP(DMA[i+0x02]),CP(DMA[i+0x03]),
				CP(DMA[i+0x04]),CP(DMA[i+0x05]),CP(DMA[i+0x06]),CP(DMA[i+0x07]),
				CP(DMA[i+0x08]),CP(DMA[i+0x09]),CP(DMA[i+0x0a]),CP(DMA[i+0x0b]),
				CP(DMA[i+0x0c]),CP(DMA[i+0x0d]),CP(DMA[i+0x0e]),CP(DMA[i+0x0f]));
		}
	}
}

int cpm_disk_read_sector(void)
{
	int result = -1;

	/* TODO: remove this */
	unsigned char *RAM = memory_get_write_ptr(0, ADDRESS_SPACE_PROGRAM, 0x0000);

	if (curdisk >= 0 &&
		curdisk < num_disks &&
		fmt[curdisk] != -1 &&
		bdos_trk[curdisk] >= 0 &&
		bdos_trk[curdisk] < formats[fmt[curdisk]].sides * formats[fmt[curdisk]].cylinders)
	{
		{
			if (fp[curdisk])
			{
				cpm_disk_image_seek();
				if (image_fread(fp[curdisk], &RAM[dma], RECL) == RECL)
					result = 0;
			}
		}
	}
	if (result)
	{
		logerror("BDOS Err on %c: Select\n", curdisk + 'A');
	}
		dump_sector();
	return result;
}

int cpm_disk_write_sector(void)
{
	int result = -1;

	/* TODO: remove this */
	unsigned char *RAM = memory_get_write_ptr(0, ADDRESS_SPACE_PROGRAM, 0x0000);

	if (curdisk >= 0 &&
		curdisk < num_disks &&
		fmt[curdisk] != -1 &&
		bdos_trk[curdisk] >= 0 &&
		bdos_trk[curdisk] < formats[fmt[curdisk]].sides * formats[fmt[curdisk]].cylinders)
	{
		{
			if (fp[curdisk])
			{
				cpm_disk_image_seek();
				if (image_fwrite(fp[curdisk], &RAM[dma], RECL) == RECL)
					result = 0;
			}
		}
	}
	if (result)
		logerror("BDOS Err on %c: Select\n", curdisk + 'A');
	return result;
}

 READ8_HANDLER (	cpm_bios_command_r )
{
	return 0xff;
}

WRITE8_HANDLER ( cpm_bios_command_w )
{
	dsk_fmt *f;
	char buff[256];
	UINT8 * RAM = memory_get_write_ptr(0, ADDRESS_SPACE_PROGRAM, 0x0000);
	UINT16 tmp, af, bc, de, hl;
	int i;

	/* Get the values of the basic Z80 registers */
	af = activecpu_get_reg( Z80_AF );
	bc = activecpu_get_reg( Z80_BC );
	de = activecpu_get_reg( Z80_DE );
	hl = activecpu_get_reg( Z80_HL );

	switch( data )
	{
	case 0x00: /* CBOOT */
		if (VERBOSE)
			logerror("BIOS 00 cold boot\n");

		cpm_conout_str("MESS CP/M 2.2 Emulator\r\n\n");
		for (i = 0; i < NDSK; i++)
		{
			if (fmt[i] != -1)
			{
				f = &formats[fmt[i]];
				sprintf(buff, "Drive %c: '%s' %5dK (%d %s%s %2d,%4d) %d %dK blocks\r\n",
					'A'+i,
					f->id,
					f->cylinders * f->sides * f->spt * f->seclen / 1024,
					f->cylinders,
					(f->sides == 2) ? "DS" : "SS",
					(f->density == DEN_FM_LO || f->density == DEN_FM_HI) ? "SD" : "DD",
					f->spt, f->seclen,
					f->dpb.dsm + 1, (RECL << f->dpb.bsh) / 1024);
				cpm_conout_str(buff);
			}
		}
		/* change vector at 0000 to warm start */
		zeropage0[0] = 0xc3;
		zeropage0[1] = (BIOS + 3) & 0xff;
		zeropage0[2] = (BIOS + 3) >> 8;
		/* fall through */

	case 0x01: /* WBOOT */
		if (VERBOSE)
			logerror("BIOS 01 warm boot\n");

		zeropage0[4] = curdisk;
		/* copy substantial zero page data */
		for (i = 0; i < 8; i++)
			RAM[0x0000 + i] = zeropage0[i];

		/* copy the CP/M 2.2 image to Z80 memory space */
		for (i = 0; i < 0x1600; i++)
			RAM[CCP + i] = memory_region(2)[i];

		/* build the bios jump table */
		cpm_jumptable();
		bc = curdisk;
		activecpu_set_reg( Z80_BC, bc );
		activecpu_set_reg( Z80_SP, 0x0080  );
		activecpu_set_reg( Z80_PC, CCP + 3 );
		break;

	case 0x02: /* CSTAT */
		tmp = io_read_byte_8(BIOS_CONST) & 0xff;
		af = (af & 0xff) | (tmp << 8);

		if (VERBOSE_CONIO)
			logerror("BIOS 02 console status      A:%02X\n", tmp);
		break;

	case 0x03: /* CONIN */
		if ( io_read_byte_8(BIOS_CONST) == 0 )
		{
			activecpu_set_reg( Z80_PC, activecpu_get_reg(Z80_PC) - 2 );
			break;
		}
		tmp = io_read_byte_8(BIOS_CONIN) & 0xff;
		af = (af & 0xff) | (tmp << 8);
		if (VERBOSE_CONIO)
			logerror("BIOS 03 console input       A:%02X\n", tmp);
		break;

	case 0x04: /* CONOUT */
		tmp = bc & 0xff;
		if (VERBOSE_CONIO)
			logerror("BIOS 04 console output      C:%02X\n", tmp);
		cpm_conout_chr( tmp );
		break;

	case 0x05: /* LSTOUT */
		tmp = bc & 0xff;
		if (VERBOSE_BIOS)
			logerror("BIOS 05 list output         C:%02X\n", tmp);
		/* If the line printer file is created */
		if (lp)
			mame_fwrite(lp, &tmp, 1);
		break;


	case 0x06: /* PUNOUT */
		tmp = bc & 0xff;
		if (VERBOSE_BIOS)
			logerror("BIOS 06 punch output        C:%02X\n", tmp);

		/* We have no punch in MESS (yet ;-) */
		break;


	case 0x07: /* RDRIN */
		/* We have no reader in MESS (yet ;-) */
		tmp = 0x00;
		af = (af & 0xff) | (tmp << 8);

		if (VERBOSE_BIOS)
			logerror("BIOS 07 reader input        A:%02X\n", tmp);
		break;


	case 0x08: /* HOME */
		if (VERBOSE_BIOS)
			logerror("BIOS 08 home\n");

		cpm_disk_home();
		break;


	case 0x09: /* SELDSK */
		tmp = bc & 0xff;
		if (VERBOSE_BIOS)
			logerror("BIOS 09 select disk         C:%02X\n", tmp);

		hl = cpm_disk_select(tmp);
		break;

	case 0x0a: /* SETTRK */
		tmp = bc;

		if (VERBOSE_BIOS)
			logerror("BIOS 0A set bdos_trk        BC:%04X\n", tmp);

		cpm_disk_set_track(tmp);
		break;


	case 0x0b: /* SETSEC */
		tmp = bc & 0xff;

		if (VERBOSE_BIOS)
			logerror("BIOS 0B set bdos_sec        C:%02X\n", tmp);

		cpm_disk_set_sector(tmp);
		break;


	case 0x0c: /* SETDMA */
		tmp = bc;

		if (VERBOSE_BIOS)
			logerror("BIOS 0C set DMA             BC:%04X\n", tmp);
		cpm_disk_set_dma(tmp);
		break;


	case 0x0d: /* RDSEC */
		tmp = cpm_disk_read_sector();
		af = (af & 0xff) | (tmp << 8);

		if (VERBOSE_BIOS)
			logerror("BIOS 0D read sector         A:%02X\n", tmp);
		break;


	case 0x0e: /* WRSEC */
		tmp = cpm_disk_write_sector();
		af = (af & 0xff) | (tmp << 8);

		if (VERBOSE_BIOS)
			logerror("BIOS 0E write sector        A:%02X\n", tmp);
		break;

	case 0x0f: /* LSTSTA */
		tmp = 0xff;    /* always ready */
		af = (af & 0xff) | (tmp << 8);

		if (VERBOSE_BIOS)
			logerror("BIOS 0F list status         A:%02X\n", tmp);
		break;

	case 0x10: /* SECTRA */
		bc = bc & 0x00ff;	/* mvi b,0 */
		if( de )			/* translation table ? */
		{
			/* XCHG    */
			tmp = hl;
			hl = de;
			de = tmp;
			/* DAD B   */
			hl += bc;
			/* MOV A,M */
			af = (af & 0xff) | ( program_read_byte(hl) << 8);
			/* MOV L,A */
			hl = (hl & 0xff00) | (af >> 8);
		}
		else
		{
			/* MOV L,C */
			/* MOV H,B */
			hl = bc;
		}
		if( curdisk >= 0 && curdisk < num_disks )
			bdos_sec[curdisk] = hl & 0xff;

		if (VERBOSE_BIOS)
			logerror("BIOS 10 sector trans        BC:%04X DE:%04X HL:%04X A:%02X\n", bc, de, hl, af>>8 );
		break;


	case 0x11: /* TRAP */
		cpm_conout_str(buff);
		break;
	}

	/* Write back the possibly modified Z80 basic registers */
	activecpu_set_reg( Z80_AF, af );
	activecpu_set_reg( Z80_BC, bc );
	activecpu_set_reg( Z80_DE, de );
	activecpu_set_reg( Z80_HL, hl );
}

