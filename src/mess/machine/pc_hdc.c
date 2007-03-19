/***************************************************************************

    machine/pc_hdc.c

	Functions to emulate a WD1004A-WXI hard disk controller

	Information was gathered from various places:
	Linux' /usr/src/linux/drivers/block/xd.c and /usr/include/linux/xd.h
	Original IBM PC-XT technical data book
	The WD1004A-WXI BIOS ROM image

    TODO:
	Still very much :)
	The format of the so called 'POD' (power on drive paramters?) area
	read from the MBR (master boot record) at offset 1AD to 1BD is wrong.

***************************************************************************/

#include <stdio.h>

#include "mscommon.h"
#include "machine/pic8259.h"
#include "machine/8237dma.h"
#include "machine/pc_hdc.h"
#include "devices/harddriv.h"
#include "memconv.h"

#define LOG_HDC_STATUS		0
#define LOG_HDC_CALL		0
#define LOG_HDC_DATA		0


#define MAX_HARD	2
#define MAX_BOARD	2				/* two boards supported */
#define HDC_DMA 	3				/* DMA channel */

#define CMD_TESTREADY   0x00
#define CMD_RECALIBRATE 0x01
#define CMD_SENSE		0x03
#define CMD_FORMATDRV	0x04
#define CMD_VERIFY		0x05
#define CMD_FORMATTRK	0x06
#define CMD_FORMATBAD	0x07
#define CMD_READ		0x08
#define CMD_WRITE		0x0a
#define CMD_SEEK		0x0b

#define CMD_SETPARAM	0x0c
#define CMD_GETECC		0x0d

#define CMD_READSBUFF	0x0e
#define CMD_WRITESBUFF	0x0f

#define CMD_RAMDIAG 	0xe0
#define CMD_DRIVEDIAG   0xe3
#define CMD_INTERNDIAG	0xe4
#define CMD_READLONG	0xe5
#define CMD_WRITELONG	0xe6

/* Bits for command status byte */
#define CSB_ERROR       0x02
#define CSB_LUN 		0x20

/* XT hard disk controller status bits */
#define STA_READY		0x01
#define STA_INPUT		0x02
#define STA_COMMAND 	0x04
#define STA_SELECT		0x08
#define STA_REQUEST 	0x10
#define STA_INTERRUPT	0x20

/* XT hard disk controller control bits */
#define CTL_PIO 		0x00
#define CTL_DMA 		0x03

static int idx = 0; 							/* contoller * 2 + drive */
static int drv = 0; 							/* 0 master, 1 slave drive */
static int cylinders[MAX_HARD] = {612,612,};    /* number of cylinders */
static int rwc[MAX_HARD] = {613,613,};			/* recduced write current from cyl */
static int wp[MAX_HARD] = {613,613,};			/* write precompensation from cyl */
static int heads[MAX_HARD] = {4,4,};			/* heads */
static int ecc[MAX_HARD] = {11,11,};			/* ECC bytes */

/* indexes */
static int cylinder[MAX_HARD] = {0,};			/* current cylinder */
static int head[MAX_HARD] = {0,};				/* current head */
static int sector[MAX_HARD] = {0,}; 			/* current sector */
static int sector_cnt[MAX_HARD] = {0,};         /* sector count */
static int control[MAX_HARD] = {0,};            /* control */

static int csb[MAX_BOARD];				/* command status byte */
static int status[MAX_BOARD];			/* drive status */
static int error[MAX_BOARD]; 			/* error code */
static int dip[MAX_BOARD];				/* dip switches */
static mame_timer *timer[MAX_BOARD];


static int data_cnt = 0;                /* data count */
static UINT8 *buffer;					/* data buffer */
static UINT8 *ptr = 0;					/* data pointer */
static UINT8 hdc_control;

static const char *hdc_command_names[] =
{
	"CMD_TESTREADY",		/* 0x00 */
	"CMD_RECALIBRATE",		/* 0x01 */
	NULL,					/* 0x02 */
	"CMD_SENSE",			/* 0x03 */
	"CMD_FORMATDRV",		/* 0x04 */
	"CMD_VERIFY",			/* 0x05 */
	"CMD_FORMATTRK",		/* 0x06 */
	"CMD_FORMATBAD",		/* 0x07 */
	"CMD_READ",				/* 0x08 */
	NULL,					/* 0x09 */
	"CMD_WRITE",			/* 0x0A */
	"CMD_SEEK",				/* 0x0B */
	"CMD_SETPARAM",			/* 0x0C */
	"CMD_GETECC",			/* 0x0D */
	"CMD_READSBUFF",		/* 0x0E */
	"CMD_WRITESBUFF",		/* 0x0F */

	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x10-0x17 */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x18-0x1F */ 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x20-0x27 */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x28-0x2F */ 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x30-0x37 */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x38-0x3F */ 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x40-0x47 */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x48-0x4F */ 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x50-0x57 */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x58-0x5F */ 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x60-0x67 */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x68-0x6F */ 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x70-0x77 */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x78-0x7F */ 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x80-0x87 */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x88-0x8F */ 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x90-0x97 */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x98-0x9F */ 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0xA0-0xA7 */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0xA8-0xAF */ 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0xB0-0xB7 */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0xB8-0xBF */ 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0xC0-0xC7 */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0xC8-0xCF */ 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0xD0-0xD7 */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0xD8-0xDF */ 

	"CMD_RAMDIAG",			/* 0xE0 */
	NULL,					/* 0xE1 */
	NULL,					/* 0xE2 */
	"CMD_DRIVEDIAG",		/* 0xE3 */
	"CMD_INTERNDIAG",		/* 0xE4 */
	"CMD_READLONG",			/* 0xE5 */
	"CMD_WRITELONG",		/* 0xE6 */
	NULL,					/* 0xE7 */

	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0xE8-0xEF */ 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0xF0-0xF7 */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL  /* 0xF8-0xFF */ 
};


static void pc_hdc_command(int n);

int pc_hdc_setup(void)
{
	int i;

	hdc_control = 0;

	buffer = auto_malloc(17*4*512);

	/* init for all boards */
	for (i = 0; i < MAX_BOARD; i++)
	{
		csb[i] = 0;
		status[i] = 0;
		error[i] = 0;
		dip[i] = 0xff;
		timer[i] = timer_alloc(pc_hdc_command);
		if (!timer[i])
			return -1;
	}
	return 0;
}

static hard_disk_file *pc_hdc_file(int id)
{
	mess_image *img;
	
	img = image_from_devtype_and_index(IO_HARDDISK, id);
	if (!image_exists(img))
		return NULL;

	return mess_hd_get_hard_disk_file(img);
}

static void pc_hdc_result(int n)
{
	int irq;

	/* dip switch selected INT 5 or 2 */
	irq = (dip[n] & 0x40) ? 5 : 2;

	pic8259_set_irq_line(0, irq, 1);
	pic8259_set_irq_line(0, irq, 0);

	if (LOG_HDC_STATUS)
		logerror("pc_hdc_result(): $%02x to $%04x\n", csb[n], data_cnt);

	buffer[data_cnt++] = csb[n];

	if (csb[n] & CSB_ERROR)
	{
        buffer[data_cnt++] = error[n];
		if (error[n] & 0x80)
		{
			buffer[data_cnt++] = (drv << 5) | head[idx];
			buffer[data_cnt++] = ((cylinder[idx] >> 2) & 0xc0) | sector[idx];
			buffer[data_cnt++] = cylinder[idx] & 0xff;

			if (LOG_HDC_STATUS)
			{
				logerror("pc_hdc_result(): result [%02x %02x %02x %02x]\n",
					buffer[data_cnt-4], buffer[data_cnt-3], buffer[data_cnt-2], buffer[data_cnt-1]);
			}
		}
		else
		{
			if (LOG_HDC_STATUS)
				logerror("pc_hdc_result(): result [%02x]\n", buffer[data_cnt-1]);
        }
    }
	status[n] |= STA_INTERRUPT | STA_INPUT | STA_REQUEST | STA_COMMAND | STA_READY;
}



static int no_dma(void)
{
	return (hdc_control & CTL_DMA) == 0;
}



static int get_lbasector(void)
{
	hard_disk_info *info;
	hard_disk_file *file;
	int lbasector;

	file = pc_hdc_file(idx);
	info = hard_disk_get_info(file);
	
	lbasector = cylinder[idx];
	lbasector *= info->heads;
	lbasector += head[idx];
	lbasector *= info->sectors;
	lbasector += sector[idx];
	return lbasector;
}



/********************************************************************
 *
 * Read a number of sectors to the address set up for DMA chan #3
 *
 ********************************************************************/

/* the following crap is an abomination; it is a relic of the old crappy DMA
 * implementation that threw the idea of "emulating the hardware" to the wind
 */
static UINT8 *hdcdma_data;
static UINT8 *hdcdma_src;
static UINT8 *hdcdma_dst;
static int hdcdma_read;
static int hdcdma_write;
static int hdcdma_size;

int pc_hdc_dack_r(void)
{
	UINT8 result;
	hard_disk_info *info;
	hard_disk_file *file;
	
	file = pc_hdc_file(idx);
	if (!file)
		return 0;
	info = hard_disk_get_info(file);

	if (hdcdma_read == 0)
	{
		hard_disk_read(file, get_lbasector(), hdcdma_data);
		hdcdma_read = 512;
		hdcdma_size -= 512;
		hdcdma_src = hdcdma_data;
		sector[idx]++;
	}

	result = *(hdcdma_src++);

	if( --hdcdma_read == 0 )
	{
		/* end of cylinder ? */
		if (sector[idx] >= info->sectors)
		{
			sector[idx] = 0;
			if (++head[idx] >= info->heads)		/* beyond heads? */
			{
				head[idx] = 0;					/* reset head */
				cylinder[idx]++;				/* next cylinder */
            }
        }
	}

	return result;
}



void pc_hdc_dack_w(int data)
{
	hard_disk_info *info;
	hard_disk_file *file;
	
	file = pc_hdc_file(idx);
	if (!file)
		return;
	info = hard_disk_get_info(file);	

	*(hdcdma_dst++) = data;

	if( --hdcdma_write == 0 )
	{
		hard_disk_write(file, get_lbasector(), hdcdma_data);
		hdcdma_write = 512;
		hdcdma_size -= 512;

        /* end of cylinder ? */
		if( ++sector[idx] >= info->sectors )
		{
			sector[idx] = 0;
			if (++head[idx] >= info->heads)		/* beyond heads? */
			{
				head[idx] = 0;					/* reset head */
				cylinder[idx]++;				/* next cylinder */
            }
        }
        hdcdma_dst = hdcdma_data;
    }
}



static void execute_read(void)
{
	hard_disk_file *disk = pc_hdc_file(idx);
	UINT8 data[512], *src = data;
	int size = sector_cnt[idx] * 512;
	int read_ = 0;

	disk = pc_hdc_file(idx);
	if (!disk)
		return;

	hdcdma_data = data;
	hdcdma_src = src;
	hdcdma_read = read_;
	hdcdma_size = size;

	if (no_dma())
	{
		do
		{
			buffer[data_cnt++] = pc_hdc_dack_r();
		} while (hdcdma_read || hdcdma_size);
	}
	else
	{
		dma8237_run_transfer(0, HDC_DMA);
	}
}



static void execute_write(void)
{
	hard_disk_file *disk = pc_hdc_file(idx);
	UINT8 data[512], *dst = data;
	int size = sector_cnt[idx] * 512;
	int write_ = 512;

	disk = pc_hdc_file(idx);
	if (!disk)
		return;

	hdcdma_data = data;
	hdcdma_dst = dst;
	hdcdma_write = write_;
	hdcdma_size = size;

	if (no_dma())
	{
		do
		{
			pc_hdc_dack_w(buffer[data_cnt++]);
		}
		while (hdcdma_write || hdcdma_size);
	}
	else
	{
		dma8237_run_transfer(0, HDC_DMA);
	}
}



static void get_drive(int n)
{
	drv = (buffer[1] >> 5) & 1;
	csb[n] = (drv) ? CSB_LUN : 0x00;
	idx = n * 2 + drv;
}



static void get_chsn(int n)
{
	head[idx] = buffer[1] & 0x1f;
	sector[idx] = buffer[2] & 0x3f;
	cylinder[idx] = (buffer[2] & 0xc0) << 2;
	cylinder[idx] |= buffer[3];
	sector_cnt[idx] = buffer[4];
	control[idx] = buffer[5];   /* 7: no retry, 6: no ecc retry, 210: step rate */

	error[n] = 0x80;	/* a potential error has C/H/S/N info */
}

static int test_ready(int n)
{
	if( !pc_hdc_file(idx) )
	{
		csb[n] |= CSB_ERROR;
		error[n] |= 0x04;	/* drive not ready */
		return 0;
	}
	return 1;
}

static void pc_hdc_command(int n)
{
	UINT8 cmd;
	const char *command_name;

	csb[n] = 0x00;
	error[n] = 0;

	ptr = buffer;
	cmd = buffer[0];

	if (LOG_HDC_STATUS)
	{
		command_name = hdc_command_names[cmd] ? hdc_command_names[cmd] : "Unknown";
		logerror("pc_hdc_command(): Executing command; pc=0x%08x cmd=0x%02x (%s) drv=%d\n",
			(unsigned) cpunum_get_reg(0, REG_PC), cmd, command_name, drv);
	}

	switch (cmd)
	{
		case CMD_TESTREADY:
		case CMD_SENSE:
			get_drive(n);
			test_ready(n);
            break;
		case CMD_RECALIBRATE:
			get_drive(n);
			get_chsn(n);
/*			test_ready(n); */
            break;

		case CMD_FORMATDRV:
		case CMD_VERIFY:
		case CMD_FORMATTRK:
		case CMD_FORMATBAD:
		case CMD_SEEK:
		case CMD_DRIVEDIAG:
			get_drive(n);
			get_chsn(n);
			test_ready(n);
            break;

		case CMD_READ:
		case CMD_READLONG:
			get_drive(n);
			get_chsn(n);

			if (LOG_HDC_STATUS)
			{
				logerror("hdc read pc=0x%08x INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
					(unsigned) cpunum_get_reg(0, REG_PC), idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]);
			}

			if (test_ready(n))
				execute_read();
			break;

		case CMD_WRITE:
		case CMD_WRITELONG:
			get_drive(n);
			get_chsn(n);

			if (LOG_HDC_STATUS)
			{
				logerror("hdc write pc=0x%08x INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
					(unsigned) cpunum_get_reg(0, REG_PC), idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]);
			}

			if (test_ready(n))
				execute_write();
			break;

		case CMD_SETPARAM:
			get_drive(n);
			get_chsn(n);
			cylinders[idx] = ((buffer[6]&3)<<8) | buffer[7];
			heads[idx] = buffer[8] & 0x1f;
			rwc[idx] = ((buffer[9]&3)<<8) | buffer[10];
			wp[idx] = ((buffer[11]&3)<<8) | buffer[12];
			ecc[idx] = buffer[13];
			break;

		case CMD_GETECC:
			buffer[data_cnt++] = ecc[idx];
			break;

		case CMD_READSBUFF:
		case CMD_WRITESBUFF:
		case CMD_RAMDIAG:
		case CMD_INTERNDIAG:
			break;

	}
	pc_hdc_result(n);
}


/*  Command format
 *	Bits	 Description
 *	7	   0
 *	xxxxxxxx command
 *	dddhhhhh drive / head
 *	ccssssss cylinder h / sector
 *	cccccccc cylinder l
 *	nnnnnnnn count
 *	xxxxxxxx control
 *
 *	Command format extra for set drive characteristics
 *	000000cc cylinders h
 *	cccccccc cylinders l
 *	000hhhhh heads
 *	000000cc reduced write h
 *	cccccccc reduced write l
 *	000000cc write precomp h
 *	cccccccc write precomp l
 *	eeeeeeee ecc
 */
static void pc_hdc_data_w(int n, int data)
{
	if( data_cnt == 0 )
	{
		if (LOG_HDC_DATA)
			logerror("hdc_data_w BOARD #%d $%02x: ", n, data);

        ptr = buffer;
		data_cnt = 6;	/* expect 6 bytes including this one */
		status[n] &= ~STA_READY;
		status[n] &= ~STA_INPUT;
		switch (data)
		{
			case CMD_SETPARAM:
				data_cnt += 8;
				break;

			case CMD_TESTREADY:
			case CMD_RECALIBRATE:
			case CMD_SENSE:
			case CMD_FORMATDRV:
			case CMD_VERIFY:
			case CMD_FORMATTRK:
			case CMD_FORMATBAD:
			case CMD_READ:
			case CMD_WRITE:
			case CMD_SEEK:
			case CMD_GETECC:
            case CMD_READSBUFF:
			case CMD_WRITESBUFF:
			case CMD_RAMDIAG:
            case CMD_DRIVEDIAG:
			case CMD_INTERNDIAG:
			case CMD_READLONG:
			case CMD_WRITELONG:
                break;

			default:
				data_cnt = 0;
				status[n] |= STA_INPUT;
				csb[n] |= CSB_ERROR | 0x20; /* unknown command */
				pc_hdc_result(n);
				break;
		}
		if( data_cnt )
			status[n] |= STA_REQUEST;
	}

	if (data_cnt)
	{
		if (LOG_HDC_DATA)
			logerror("hdc_data_w BOARD #%d $%02x\n", n, data);

		*ptr++ = data;
		status[n] |= STA_READY;
		if (--data_cnt == 0)
		{
			if (LOG_HDC_STATUS)
				logerror("pc_hdc_data_w(): Launching command; pc=0x%08x\n", (unsigned) cpunum_get_reg(0, REG_PC));

            status[n] &= ~STA_COMMAND;
			status[n] &= ~STA_REQUEST;
			status[n] &= ~STA_READY;
			status[n] |= STA_INPUT;
			
			assert(timer[n]);
			timer_adjust(timer[n], 0.001, n, 0);
        }
	}
}



static void pc_hdc_reset_w(int n, int data)
{
	cylinder[n] = 0;
	head[n] = 0;
	sector[n] = 0;
	csb[n] = 0;
	status[n] = STA_COMMAND | STA_READY;
	memset(buffer, 0, sizeof(buffer));
	ptr = buffer;
	data_cnt = 0;
}



static void pc_hdc_select_w(int n, int data)
{
	status[n] &= ~STA_INTERRUPT;
	status[n] |= STA_SELECT;
}



static void pc_hdc_control_w(int n, int data)
{
	if (LOG_HDC_STATUS)
		logerror("pc_hdc_control_w(): Control write pc=0x%08x data=%d\n", (unsigned) cpunum_get_reg(0, REG_PC), data);

	hdc_control = data;
}



static UINT8 pc_hdc_data_r(int n)
{
	UINT8 data = 0xff;
	if( data_cnt )
	{
		data = *ptr++;
		status[n] &= ~STA_INTERRUPT;
		if( --data_cnt == 0 )
		{
			status[n] &= ~STA_INPUT;
			status[n] &= ~STA_REQUEST;
			status[n] &= ~STA_SELECT;
			status[n] |= STA_COMMAND;
		}
	}
    return data;
}



static UINT8 pc_hdc_status_r(int n)
{
	int data = status[n];
	return data;
}



static UINT8 pc_hdc_dipswitch_r(int n)
{
	int data = dip[n];
	return data;
}



/*************************************************************************
 *
 *		HDC
 *		hard disk controller
 *
 *************************************************************************/

static UINT8 pc_HDC_r(int chip, offs_t offs)
{
	UINT8 data = 0xff;
	if( !(input_port_3_r(0) & (0x08>>chip)) || !pc_hdc_file(chip<<1) )
		return data;
	switch( offs )
	{
		case 0: data = pc_hdc_data_r(chip); 	 break;
		case 1: data = pc_hdc_status_r(chip);	 break;
		case 2: data = pc_hdc_dipswitch_r(chip); break;
		case 3: break;
	}

	if (LOG_HDC_CALL)
		logerror("pc_HDC_r(): chip=%d offs=%d result=0x%02x\n", chip, offs, data);

	return data;
}



static void pc_HDC_w(int chip, offs_t offs, UINT8 data)
{
	if (LOG_HDC_CALL)
		logerror("pc_HDC_w(): chip=%d offs=%d data=0x%02x\n", chip, offs, data);

	if( !(input_port_3_r(0) & (0x08>>chip)) || !pc_hdc_file(chip<<1) )
		return;

	switch( offs )
	{
		case 0: pc_hdc_data_w(chip,data);	 break;
		case 1: pc_hdc_reset_w(chip,data);	 break;
		case 2: pc_hdc_select_w(chip,data);  break;
		case 3: pc_hdc_control_w(chip,data); break;
	}
}


/*************************************
 *
 *		Port handlers
 *
 *************************************/

READ8_HANDLER ( pc_HDC1_r ) { return pc_HDC_r(0, offset); }
READ8_HANDLER ( pc_HDC2_r ) { return pc_HDC_r(1, offset); }
WRITE8_HANDLER ( pc_HDC1_w ) { pc_HDC_w(0, offset, data); }
WRITE8_HANDLER ( pc_HDC2_w ) { pc_HDC_w(1, offset, data); }

READ32_HANDLER ( pc32_HDC1_r ) { return read32le_with_read8_handler(pc_HDC1_r, offset, mem_mask); }
READ32_HANDLER ( pc32_HDC2_r ) { return read32le_with_read8_handler(pc_HDC2_r, offset, mem_mask); }
WRITE32_HANDLER ( pc32_HDC1_w ) { write32le_with_write8_handler(pc_HDC1_w, offset, data, mem_mask); }
WRITE32_HANDLER ( pc32_HDC2_w ) { write32le_with_write8_handler(pc_HDC2_w, offset, data, mem_mask); }
