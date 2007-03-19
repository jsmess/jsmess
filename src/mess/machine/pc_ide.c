/***************************************************************************

	machine/pc_ide.c

	Functions to emulate a IDE hard disk controller
	Not (currently) used, since an XT does not have IDE support in BIOS :(

***************************************************************************/
#include "includes/pc_ide.h"


typedef	struct {
	int lba;				/* 0 CHS mode, 1 LBA mode */
	int cylinder; 		/* current cylinder (lba = 0) */
	int head; 			/* current head (lba = 0) */
	int sector;			/* current sector (or LBA if lba = 1) */
	int sector_cnt;		/* sector count */
	int error;			/* error code */
	int status;			/* drive status */
	
	int data_cnt;                /* data count */
	UINT8 *buffer;				/* data buffer */
	UINT8 *ptr;					/* data pointer */
} DEVICE;

typedef struct {
	int drv; 					/* 0 master, 1 slave drive */
	DEVICE devs[2];
} IDE;
static IDE ide[1]={ // currently only 1 is enough
	{ 0 }
};

static void pc_ide_data_w(DEVICE *This, int data)
{
	if (This->data_cnt) {
		*This->ptr++ = data;
		if (--This->data_cnt == 0) {
		}
	}
}

static int pc_ide_data_r(DEVICE *This)
{
	int data = 0xff;

	if (This->data_cnt) {
		data = *This->ptr++;
		if (--This->data_cnt == 0) {
		}
	}
	return data;
}

static void pc_ide_write_precomp_w(DEVICE *This, int data)
{

}

/*
 * ---diagnostic mode errors---
 * 7   which drive failed (0 = master, 1 = slave)
 * 6-3 reserved
 * 2-0 error code
 *	   001 no error detected
 *	   010 formatter device error
 *	   011 sector buffer error
 *	   100 ECC circuitry error
 *	   101 controlling microprocessor error
 * ---operation mode---
 * 7   bad block detected
 * 6   uncorrectable ECC error
 * 5   reserved
 * 4   ID found
 * 3   reserved
 * 2   command aborted prematurely
 * 1   track 000 not found
 * 0   DAM not found (always 0 for CP-3022)
 */
static int pc_ide_error_r(DEVICE *This)
{
	int data = This->error;
	return data;
}

static void pc_ide_sector_count_w(DEVICE *This, int data)
{
	This->sector_cnt=data;
}

static int pc_ide_sector_count_r(DEVICE *This)
{
	int data = This->sector_cnt;
	return data;
}

static void pc_ide_sector_number_w(DEVICE *This,int data)
{
	if (This->lba)
		This->sector = (This->sector & 0xfffff00) | (data & 0xff);
	else
		This->sector = data;
}

static int pc_ide_sector_number_r(DEVICE *This)
{
	int data = This->sector & 0xff;
	return data;
}

static void pc_ide_cylinder_number_l_w(DEVICE *This, int data)
{
	if (This->lba)
		This->sector = (This->sector & 0xfff00ff) | ((data & 0xff) << 8);
	else
		This->cylinder = (This->cylinder & 0xff00) | (data & 0xff);
}

static int pc_ide_cylinder_number_l_r(DEVICE *This)
{
	int data;
    if (This->lba)
		data = (This->sector >> 8) & 0xff;
	else
        data = This->cylinder & 0xff;
	return data;
}

static void pc_ide_cylinder_number_h_w(DEVICE *This,int data)
{
	if (This->lba)
		This->sector = (This->sector & 0xf00ffff) | ((data & 0xff) << 16);
	else
		This->cylinder = (This->cylinder & 0x00ff) | ((data & 0xff) << 8);
}

static int pc_ide_cylinder_number_h_r(DEVICE *This)
{
	int data;
    if (This->lba)
		data = (This->sector >> 16) & 0xff;
	else
		data = (This->cylinder >> 8) & 0xff;
	return data;
}

static void pc_ide_drive_head_w(IDE *This, int data)
{
	DEVICE *dev;
	This->drv = (data >> 4) & 1;

	dev=This->devs+This->drv;
    dev->lba = (data >> 6) & 1;
	if (dev->lba)
		dev->sector = (dev->sector & 0x0ffffff) | ((data & 0x0f) << 24);
	else
		dev->head = data & 0x0f;
}

static int pc_ide_drive_head_r(IDE *This)
{
	int data;
	DEVICE *dev;
	dev=This->devs+This->drv;

	if (dev->lba)
		data = 0xe0 | (This->drv << 4) | ((dev->sector >> 24) & 0x0f);
	else
		data = 0xa0 | (This->drv << 4) | dev->head;
	return data;
}

static void pc_ide_command_w(DEVICE *This, int data)
{
	switch (data) {
		case 0x00:
			/* nop */
			break;
		case 0x10: case 0x11: case 0x12: case 0x13:
		case 0x14: case 0x15: case 0x16: case 0x17:
		case 0x18: case 0x19: case 0x1a: case 0x1b:
		case 0x1c: case 0x1d: case 0x1e: case 0x1f:
			/* recalibrate */
			This->cylinder = 0;
            break;
		case 0x20: case 0x21:
			/* read sectors (with or w/o retry) */
			This->ptr = This->buffer;
			This->data_cnt = This->sector_cnt * 512;
			break;
		case 0x22: case 0x23:
			/* read long (with or w/o retry) */
            break;
		case 0x30: case 0x31:
            /* write sectors (with or w/o retry) */
			This->ptr = This->buffer;
			This->data_cnt = This->sector_cnt * 512;
			break;
		case 0x32: case 0x33:
			/* write long (with or w/o retry) */
            break;
		case 0x40: case 0x41:
			/* read verify sectors (with or w/o retry) */
            break;
		case 0x50:
			/* format track */
			break;
		case 0x70: case 0x71: case 0x72: case 0x73:
		case 0x74: case 0x75: case 0x76: case 0x77:
		case 0x78: case 0x79: case 0x7a: case 0x7b:
		case 0x7c: case 0x7d: case 0x7e: case 0x7f:
			/* seek */
            break;
		case 0x90:
			/* execute diagnostics */
            break;
		case 0x91:
			/* initialize drive parameters */
            break;
	}
}

/*
 * Bit(s) Description
 * 7	  controller is executing a command
 * 6	  drive is ready
 * 5	  write fault
 * 4	  seek complete
 * 3	  sector buffer requires servicing
 * 2	  disk data read successfully corrected
 * 1	  index - set to 1 each disk revolution
 * 0	  previous command ended in an error
 */
static int pc_ide_status_r(DEVICE *This)
{
	int data = This->status;
	return data;
}


/*************************************************************************
 *
 *		ATHD
 *		AT hard disk
 *
 *************************************************************************/
WRITE8_HANDLER(at_mfm_0_w)
{
	logerror("ide write %.2x %.2x\n", offset, data);
	switch (offset) {
	case 0: pc_ide_data_w(ide->devs+ide->drv, data);				break;
	case 1: pc_ide_write_precomp_w(ide->devs+ide->drv, data);		break;
	case 2: pc_ide_sector_count_w(ide->devs+ide->drv, data);		break;
	case 3: pc_ide_sector_number_w(ide->devs+ide->drv, data);		break;
	case 4: pc_ide_cylinder_number_l_w(ide->devs+ide->drv, data);	break;
	case 5: pc_ide_cylinder_number_h_w(ide->devs+ide->drv, data);	break;
	case 6: pc_ide_drive_head_w(ide, data);			break;
	case 7: pc_ide_command_w(ide->devs+ide->drv, data); 			break;
	}
}

 READ8_HANDLER(at_mfm_0_r)
{
	int data=0;

	switch (offset) {
	case 0: data = pc_ide_data_r(ide->devs+ide->drv); 			break;
	case 1: data = pc_ide_error_r(ide->devs+ide->drv);			break;
	case 2: data = pc_ide_sector_count_r(ide->devs+ide->drv); 	break;
	case 3: data = pc_ide_sector_number_r(ide->devs+ide->drv);	break;
	case 4: data = pc_ide_cylinder_number_l_r(ide->devs+ide->drv);break;
	case 5: data = pc_ide_cylinder_number_h_r(ide->devs+ide->drv);break;
	case 6: data = pc_ide_drive_head_r(ide);		break;
	case 7: data = pc_ide_status_r(ide->devs+ide->drv);			break;
	}
	logerror("ide read %.2x %.2x\n", offset, data);
	return data;
}

