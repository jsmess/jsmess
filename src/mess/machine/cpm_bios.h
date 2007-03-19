/******************************************************************************
 *
 *	cpm_bios.h
 *
 *	CP/M bios emulation (used for Kaypro 2x)
 *
 *	Juergen Buchmueller, July 1998
 *
 ******************************************************************************/

#ifndef _CPM_BIOS_H
#define _CPM_BIOS_H

/* CCP	load address (console command processor of 62K cpm.sys) */
#define CCP 		0xde00

/* BDOS load address (basic disk operating system of 62K cpm.sys) */
#define BDOS		(CCP + 0x0806)

#define NDSK	4			/* maximum number of disk drives */

#define NFUN    18          /* number of bios functions */
#define DPHL	16			/* length of a DPH structure */
#define DPBL	15			/* length of a DPB structure */
#define RECL	128 		/* CP/M record length */

/* BIOS 'hardware' interface addresses (to be handled by readport/writeport) */
#define BIOS_LSTST  0x60    /* list status */
#define BIOS_LSTIN  0x61    /* list in (w to stuff characters) */
#define BIOS_LSTOUT 0x62    /* list out */

#define BIOS_RDRST  0x64    /* reader status */
#define BIOS_RDRIN  0x65    /* reader in (w to stuff characters) */
#define BIOS_RDROUT 0x66    /* reader out */

#define BIOS_PUNST  0x68    /* punch status */
#define BIOS_PUNIN  0x69    /* punch in (w to stuff characters) */
#define BIOS_PUNOUT 0x6a    /* punch out */

#define BIOS_CONST  0x6c    /* console status */
#define BIOS_CONIN  0x6d    /* console in (w to stuff characters) */
#define BIOS_CONOUT 0x6e    /* console out (r to ) */

#define BIOS_CMD    0x70    /* bios command out */

/* BIOS load address (5,5K after CCP/BDOS) contains a jump table */
#define BIOS        (CCP + 0x1600)

/* BIOS DPHs (disk parameter headers) */
#define DPH0		(BIOS + NFUN * 3)
#define DPH1		(BIOS + NFUN * 3 + 1 * DPHL)
#define DPH2		(BIOS + NFUN * 3 + 2 * DPHL)
#define DPH3		(BIOS + NFUN * 3 + 3 * DPHL)
/* BIOS DPBs (disk parameter blocks) */
#define DPB0        (BIOS + NFUN * 3 + 4 * DPHL)
#define DPB1		(BIOS + NFUN * 3 + 4 * DPHL + 1 * DPBL)
#define DPB2		(BIOS + NFUN * 3 + 4 * DPHL + 2 * DPBL)
#define DPB3		(BIOS + NFUN * 3 + 4 * DPHL + 3 * DPBL)

/* 4 times 128 byte directory buffers */
#define DIRBUF		(BIOS + NFUN * 3 + 4 * DPHL + 4 * DPBL)

/* BIOS DATA (XLATs, CSVs, ALVs) */
#define DATA_START	(BIOS + NFUN * 3 + 4 * DPHL + 4 * DPBL + 4 * RECL)

/* BIOS functions */
#define BIOS_00 	0xffb0
#define BIOS_01 	0xffb4
#define BIOS_02 	0xffb8
#define BIOS_03 	0xffbc
#define BIOS_04 	0xffc0
#define BIOS_05 	0xffc4
#define BIOS_06 	0xffc8
#define BIOS_07 	0xffcc
#define BIOS_08 	0xffd0
#define BIOS_09 	0xffd4
#define BIOS_0A 	0xffd8
#define BIOS_0B 	0xffdc
#define BIOS_0C 	0xffe0
#define BIOS_0D 	0xffe4
#define BIOS_0E 	0xffe8
#define BIOS_0F 	0xffec
#define BIOS_10 	0xfff0
#define BIOS_11 	0xfff4

/* BIOS central function exec via OUT (0),A */
#define BIOS_EXEC	0xfff8	/* code here is: D3 00 C9 */
#define BIOS_END	0xffff

#if 0

typedef enum {
    DEN_FM_LO = 0,
    DEN_FM_HI,
    DEN_MFM_LO,
    DEN_MFM_HI
} DENSITY;

#endif

typedef enum {
    ORD_SIDES = 0,
    ORD_CYLINDERS,
    ORD_EAGLE
} ORDER;

typedef struct {
	UINT16 xlat;				/* sector translation table */
	UINT16 scr1;				/* scratch memory 1 		*/
	UINT16 scr2;				/* scratch memory 2 		*/
	UINT16 scr3;				/* scratch memory 3 		*/
	UINT16 dirbuf;			/* directory buffer 		*/
	UINT16 dpb;				/* drive parameter block	*/
	UINT16 csv;				/* check sector vector		*/
	UINT16 alv;				/* allocation vector		*/
} cpm_dph;

typedef struct {
	UINT16 spt;				/* sectors per track		*/
	UINT8 bsh;				/* block shift				*/
	UINT8 blm;				/* block mask				*/
	UINT8 exm;				/* extent mask				*/
	UINT16 dsm;				/* drive storage max sector */
	UINT16 drm;				/* directory max sector 	*/
	UINT8 al0;				/* allocation bits low		*/
	UINT8 al1;				/* allocation bits high 	*/
	UINT16 cks;				/* check sectors (drm+1)/4 if media is removable */
	UINT16 off;				/* offset (boot sectors)	*/
} cpm_dpb;

typedef struct {
	const char	*id;		/* short name */
	const char	*name;		/* long name */
	const char	*ref;		/* id reference */
	DENSITY density;		/* fdd density */
	UINT16	cylinders;		/* number of cylinders */
	UINT8	sides;			/* number of sides */
	UINT8	spt;			/* sectors per track */
	UINT16	seclen; 		/* sector length */
	UINT8	skew;			/* sector skew */
	UINT8	side1[32];		/* side number, sector numbers */
	UINT8	side2[32];		/* side number, sector numbers */
	ORDER	order;			/* sector ordering */
	const char	*label; 	/* disk label */
	cpm_dpb dpb;			/* associated dpb */
} dsk_fmt;

extern dsk_fmt formats[];

/* these are in cpm_bios.c */
DEVICE_LOAD( cpm_floppy );
extern int cpm_init(int n, const char *ids[]);

extern READ8_HANDLER ( cpm_bios_command_r );
extern WRITE8_HANDLER ( cpm_bios_command_w );

/* these are in cpm_disk.c */
extern int cpm_disk_select_format(int disk, const char *disk_id);
extern void cpm_disk_open_image(void);
extern void cpm_disk_home(void);
extern void cpm_disk_set_track(int t);
extern void cpm_disk_set_sector(int s);
extern void cpm_disk_set_dma(int d);
extern int cpm_disk_read_sector(void);
extern int cpm_disk_write_sector(void);

#endif  /* _CPM_BIOS_H */
