/*
	smc92x4.h: header file for smc92x4.c

	Raphael Nabet
*/

/* this enum matches the possible values for bits 3-2 of the select register */
typedef enum select_mode_t
{
	sm_undefined = -1,		/* value after reset and drive deselect(right?) */
	sm_at_harddisk = 0,		/* hard disk in PC-AT compatible harddisk format */
	sm_harddisk,			/* hard disk */
	sm_floppy_slow,			/* 8" drive (and 5.25" and 3.5" HD disks?) */
	sm_floppy_fast			/* 5.25" and 3.5" drive */
} select_mode_t;

typedef struct smc92x4_intf
{
	/* select_callback serves 2 purposes:
	a) the emulated system may use other lines than the normal smc92x4 select
	  lines
	b) if there is more than one smc92x4 controller, we need to have different
	  drive numbers for each controller
	which: smc92x4 controller concerned
	select_mode: any of sm_harddisk, sm_floppy_slow or sm_floppy_fast
	select_line: state smc92x4 select lines (0: no line active, 1, 2, 3: first,
	  second or third line active)
	gpos: state of 4 general prurpose outputs that are used as floppy select
	  lines by the (4 LSBits of smc99x4 retry count register)
	return -1 if no drive is selected, or index of image in MESS IO_FLOPPY
	  IO_HARDDISK device array. */
	int (*select_callback)(int which, select_mode_t select_mode, int select_line, int gpos);
	/* dma_read_callback will be called by the smc99x4 core to read the sector
	buffer */
	UINT8 (*dma_read_callback)(int which, offs_t offset);
	/* dma_read_callback will be called by the smc99x4 core to write the sector
	buffer */
	void (*dma_write_callback)(int which, offs_t offset, UINT8 data);
	/* int_callback will be called by the smc99x4 core whenever the state of
	the interrupt lien changes */
	void (*int_callback)(int which, int state);
} smc92x4_intf;

int smc92x4_hd_load(mess_image *image, int disk_unit);
void smc92x4_hd_unload(mess_image *image, int disk_unit);

void smc92x4_init(int which, const smc92x4_intf *intf);
void smc92x4_reset(int which);
int smc92x4_r(int which, int offset);
void smc92x4_w(int which, int offset, int data);
 READ8_HANDLER(smc92x4_0_r);
WRITE8_HANDLER(smc92x4_0_w);
