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



    There were a lot of different Fixed Disk Adapter for the IBM pcs.
    Xebec produced a couple of the Fixed Disk Adapter for IBM.

    One Xebec Adapter has these roms on it:
    One MK6xxxx ROM: 62X0822__(M)_AMI_8621MAB__S68B364-P__(C)IBM_CORP_1982,1985__PHILIPPINES.12D.2364.bin
    One 2732 ROM for the Z80: 104839RE__COPYRIGHT__XEBEC_1986.12A.2732.bin


    tandon Fixed Disk Adapter found in an IBM 5150:
    600963-001__TYPE_5.U12.2764.bin

***************************************************************************/

#include "emu.h"
#include "machine/pc_hdc.h"
#include "machine/8237dma.h"
#include "imagedev/harddriv.h"
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
#define CTL_DMA 		0x01

static int idx = 0; 				/* controller * 2 + drive */
static int drv = 0; 							/* 0 master, 1 slave drive */
static int cylinders[MAX_HARD];		/* number of cylinders */
static int rwc[MAX_HARD];			/* reduced write current from cyl */
static int wp[MAX_HARD];			/* write precompensation from cyl */
static int heads[MAX_HARD];			/* heads */
static int ecc[MAX_HARD];			/* ECC bytes */

/* indexes */
static int cylinder[MAX_HARD];			/* current cylinder */
static int head[MAX_HARD];				/* current head */
static int sector[MAX_HARD];			/* current sector */
static int sector_cnt[MAX_HARD];		/* sector count */
static int control[MAX_HARD];			/* control */

static int csb[MAX_BOARD];				/* command status byte */
static int status[MAX_BOARD];			/* drive status */
static int error[MAX_BOARD];			/* error code */
static int dip[MAX_BOARD];				/* dip switches */
static emu_timer *timer[MAX_BOARD];


static int data_cnt = 0;                /* data count */
static UINT8 *buffer;					/* data buffer */
static UINT8 *buffer_ptr = 0;			/* data pointer */
static UINT8 hdc_control;
static void (*hdc_set_irq)(running_machine &,int,int);
static device_t *pc_hdc_dma8237;


static const char *const hdc_command_names[] =
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


static TIMER_CALLBACK(pc_hdc_command);

int pc_hdc_setup(running_machine &machine, void (*hdc_set_irq_func)(running_machine &,int,int))
{
	int i;

	hdc_control = 0;
	hdc_set_irq = hdc_set_irq_func;

	buffer = auto_alloc_array(machine, UINT8, 17*4*512);

	for (i = 0; i < MAX_HARD; i++)
	{
		cylinders[i] = 612;
		rwc[i] = 613;
		wp[i] = 613;
		heads[i] = 4;
		ecc[i] = 11;

		/* indexes */
		cylinder[i] = 0;
		head[i] = 0;
		sector[i] = 0;
		sector_cnt[i] = 0;
		control[i] = 0;
	}

	/* init for all boards */
	for (i = 0; i < MAX_BOARD; i++)
	{
		csb[i] = 0;
		status[i] = 0;
		error[i] = 0;
		dip[i] = 0xff;
		pc_hdc_dma8237 = NULL;
		timer[i] = machine.scheduler().timer_alloc(FUNC(pc_hdc_command));
		if (!timer[i])
			return -1;
	}
	return 0;
}


void pc_hdc_set_dma8237_device( device_t *dma8237 )
{
	pc_hdc_dma8237 = dma8237;
}


static hard_disk_file *pc_hdc_file(running_machine &machine, int id)
{
	device_image_interface *img = NULL;

	switch( id )
	{
	case 0:
		img = dynamic_cast<device_image_interface *>(machine.device("harddisk1"));
		break;
	case 1:
		img = dynamic_cast<device_image_interface *>(machine.device("harddisk2"));
		break;
	case 2:
		img = dynamic_cast<device_image_interface *>(machine.device("harddisk3"));
		break;
	case 3:
		img = dynamic_cast<device_image_interface *>(machine.device("harddisk4"));
		break;
	}

	if ( img == NULL )
		return NULL;

	if (!img->exists())
		return NULL;

	return hd_get_hard_disk_file(&img->device());
}

static void pc_hdc_result(running_machine &machine,int n, int set_error_info)
{
	int irq;

	/* dip switch selected INT 5 or 2 */
	irq = (dip[n] & 0x40) ? 5 : 2;

	if ( ( hdc_control & 0x02 ) && hdc_set_irq ) {
		hdc_set_irq( machine, irq, 1 );
	}

	if (LOG_HDC_STATUS)
		logerror("pc_hdc_result(): $%02x to $%04x\n", csb[n], data_cnt);

	buffer[data_cnt++] = csb[n];

	if (set_error_info && ( csb[n] & CSB_ERROR ) )
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



static int get_lbasector(running_machine &machine)
{
	hard_disk_info *info;
	hard_disk_file *file;
	int lbasector;

	file = pc_hdc_file(machine, idx);
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
static UINT8 hdcdma_data[512];
static UINT8 *hdcdma_src;
static UINT8 *hdcdma_dst;
static int hdcdma_read;
static int hdcdma_write;
static int hdcdma_size;

int pc_hdc_dack_r(running_machine &machine)
{
	UINT8 result;
	hard_disk_info *info;
	hard_disk_file *file;

	file = pc_hdc_file(machine, idx);
	if (!file)
		return 0;
	info = hard_disk_get_info(file);

	if (hdcdma_read == 0)
	{
		hard_disk_read(file, get_lbasector(machine), hdcdma_data);
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

	if ( ! no_dma() )
	{
		assert( pc_hdc_dma8237 != NULL );
		i8237_dreq3_w( pc_hdc_dma8237, ( hdcdma_read || hdcdma_size ) ? 1 : 0 );
	}

	return result;
}



void pc_hdc_dack_w(running_machine &machine, int data)
{
	hard_disk_info *info;
	hard_disk_file *file;

	file = pc_hdc_file(machine, idx);
	if (!file)
		return;
	info = hard_disk_get_info(file);

	*(hdcdma_dst++) = data;

	if( --hdcdma_write == 0 )
	{
		hard_disk_write(file, get_lbasector(machine), hdcdma_data);
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

	if ( ! no_dma() )
	{
		assert( pc_hdc_dma8237 != NULL );
		i8237_dreq3_w( pc_hdc_dma8237, ( hdcdma_write || hdcdma_size ) ? 1 : 0 );
	}
}



static void execute_read(running_machine &machine)
{
	hard_disk_file *disk = pc_hdc_file(machine, idx);
	int size = sector_cnt[idx] * 512;
	int read_ = 0;

	disk = pc_hdc_file(machine, idx);
	if (!disk)
		return;

	hdcdma_src = hdcdma_data;
	hdcdma_read = read_;
	hdcdma_size = size;

	if (no_dma())
	{
		do
		{
			buffer[data_cnt++] = pc_hdc_dack_r( machine );
		} while (hdcdma_read || hdcdma_size);
	}
	else
	{
		assert( pc_hdc_dma8237 != NULL );
		i8237_dreq3_w( pc_hdc_dma8237, 1 );
	}
}



static void execute_write(running_machine &machine)
{
	hard_disk_file *disk = pc_hdc_file(machine, idx);
	int size = sector_cnt[idx] * 512;
	int write_ = 512;

	disk = pc_hdc_file(machine, idx);
	if (!disk)
		return;

	hdcdma_dst = hdcdma_data;
	hdcdma_write = write_;
	hdcdma_size = size;

	if (no_dma())
	{
		do
		{
			pc_hdc_dack_w(machine, buffer[data_cnt++]);
		}
		while (hdcdma_write || hdcdma_size);
	}
	else
	{
		assert( pc_hdc_dma8237 != NULL );
		i8237_dreq3_w( pc_hdc_dma8237, 1 );
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

static int test_ready(running_machine &machine, int n)
{
	if( !pc_hdc_file(machine, idx) )
	{
		csb[n] |= CSB_ERROR;
		error[n] |= 0x04;	/* drive not ready */
		return 0;
	}
	return 1;
}

static TIMER_CALLBACK(pc_hdc_command)
{
	int n = param;
	int set_error_info = 1;
	int old_error = error[n];			/* Previous error data is needed for CMD_SENSE */
	UINT8 cmd;
	const char *command_name;

	csb[n] = 0x00;
	error[n] = 0;

	buffer_ptr = buffer;
	cmd = buffer[0];

	get_drive(n);

	if (LOG_HDC_STATUS)
	{
		command_name = hdc_command_names[cmd] ? hdc_command_names[cmd] : "Unknown";
		logerror("pc_hdc_command(): Executing command; pc=0x%08x cmd=0x%02x (%s) drv=%d\n",
			(unsigned) cpu_get_reg(machine.firstcpu, STATE_GENPC), cmd, command_name, drv);
	}

	switch (cmd)
	{
		case CMD_TESTREADY:
			set_error_info = 0;
			test_ready(machine, n);
            break;
		case CMD_SENSE:
			/* Perform error code translation. This may need to be expanded in the future. */
			buffer[data_cnt++] = ( old_error & 0xC0 ) | ( ( old_error & 0x04 ) ? 0x04 : 0x00 ) ;
			buffer[data_cnt++] = (drv << 5) | head[idx];
			buffer[data_cnt++] = ((cylinder[idx] >> 2) & 0xc0) | sector[idx];
			buffer[data_cnt++] = cylinder[idx] & 0xff;
			set_error_info = 0;
			break;
		case CMD_RECALIBRATE:
			get_chsn(n);
            break;

		case CMD_FORMATDRV:
		case CMD_VERIFY:
		case CMD_FORMATTRK:
		case CMD_FORMATBAD:
		case CMD_SEEK:
		case CMD_DRIVEDIAG:
			get_chsn(n);
			test_ready(machine, n);
            break;

		case CMD_READ:
		case CMD_READLONG:
			get_chsn(n);

			if (LOG_HDC_STATUS)
			{
				logerror("hdc read pc=0x%08x INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
					(unsigned) cpu_get_reg(machine.firstcpu, STATE_GENPC), idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]);
			}

			if (test_ready(machine, n))
				execute_read(machine);
			set_error_info = 0;
			break;

		case CMD_WRITE:
		case CMD_WRITELONG:
			get_chsn(n);

			if (LOG_HDC_STATUS)
			{
				logerror("hdc write pc=0x%08x INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
					(unsigned) cpu_get_reg(machine.firstcpu, STATE_GENPC), idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]);
			}

			if (test_ready(machine, n))
				execute_write(machine);
			break;

		case CMD_SETPARAM:
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
	pc_hdc_result(machine, n, set_error_info);
}


/*  Command format
 *  Bits     Description
 *  7      0
 *  xxxxxxxx command
 *  dddhhhhh drive / head
 *  ccssssss cylinder h / sector
 *  cccccccc cylinder l
 *  nnnnnnnn count
 *  xxxxxxxx control
 *
 *  Command format extra for set drive characteristics
 *  000000cc cylinders h
 *  cccccccc cylinders l
 *  000hhhhh heads
 *  000000cc reduced write h
 *  cccccccc reduced write l
 *  000000cc write precomp h
 *  cccccccc write precomp l
 *  eeeeeeee ecc
 */
static void pc_hdc_data_w(running_machine &machine, int n, int data)
{
	if( data_cnt == 0 )
	{
		if (LOG_HDC_DATA)
			logerror("hdc_data_w BOARD #%d $%02x: ", n, data);

        buffer_ptr = buffer;
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
				pc_hdc_result(machine, n, 1);
				break;
		}
		if( data_cnt )
			status[n] |= STA_REQUEST;
	}

	if (data_cnt)
	{
		if (LOG_HDC_DATA)
			logerror("hdc_data_w BOARD #%d $%02x\n", n, data);

		*buffer_ptr++ = data;
		status[n] |= STA_READY;
		if (--data_cnt == 0)
		{
			if (LOG_HDC_STATUS)
				logerror("pc_hdc_data_w(): Launching command; pc=0x%08x\n", (unsigned) cpu_get_reg(machine.firstcpu, STATE_GENPC));

            status[n] &= ~STA_COMMAND;
			status[n] &= ~STA_REQUEST;
			status[n] &= ~STA_READY;
			status[n] |= STA_INPUT;

			assert(timer[n]);
			timer[n]->adjust(attotime::from_msec(1), n);
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
	memset(buffer, 0, 17*4*512);
	buffer_ptr = buffer;
	data_cnt = 0;
}



static void pc_hdc_select_w(int n, int data)
{
	status[n] &= ~STA_INTERRUPT;
	status[n] |= STA_SELECT;
}



static void pc_hdc_control_w(running_machine &machine, int n, int data)
{
	int irq = (dip[n] & 0x40) ? 5 : 2;

	if (LOG_HDC_STATUS)
		logerror("pc_hdc_control_w(): Control write pc=0x%08x data=%d\n", (unsigned) cpu_get_reg(machine.firstcpu, STATE_GENPC), data);

	hdc_control = data;

	if ( ! ( hdc_control & 0x02 ) && hdc_set_irq ) {
		hdc_set_irq( machine, irq, 0 );
	}
}



static UINT8 pc_hdc_data_r(int n)
{
	UINT8 data = 0xff;
	if( data_cnt )
	{
		data = *buffer_ptr++;
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



/*
    Dipswitch configuration


    Tandon/Western Digital Fixed Disk Controller
    bit0-1 : Determine disk size(?)
        Causes geometry data to be read from c8043, c8053, c8063, c8073 (?)
        00 - 40 Mbytes
        01 - 30 Mbytes
        10 - 10 Mbytes
        11 - 20 Mbytes
    bit2-7 : unknown

 */

static UINT8 pc_hdc_dipswitch_r(int n)
{
	int data = dip[n];
	return data;
}



/*************************************************************************
 *
 *      HDC
 *      hard disk controller
 *
 *************************************************************************/

static UINT8 pc_HDC_r(running_machine &machine, int chip, offs_t offs)
{
	UINT8 data = 0xff;

	switch( offs )
	{
		case 0: data = pc_hdc_data_r(chip); 	 break;
		case 1: data = pc_hdc_status_r(chip);	 break;
		case 2: data = pc_hdc_dipswitch_r(chip); break;
		case 3: break;
	}

	if (LOG_HDC_CALL)
		logerror("pc_HDC_r(): pc=%06X chip=%d offs=%d result=0x%02x\n", cpu_get_pc(machine.firstcpu), chip, offs, data);

	return data;
}



static void pc_HDC_w(running_machine &machine, int chip, offs_t offs, UINT8 data)
{
	if (LOG_HDC_CALL)
		logerror("pc_HDC_w(): pc=%06X chip=%d offs=%d data=0x%02x\n", cpu_get_pc(machine.firstcpu), chip, offs, data);

	switch( offs )
	{
		case 0: pc_hdc_data_w(machine, chip,data);	 break;
		case 1: pc_hdc_reset_w(chip,data);	 break;
		case 2: pc_hdc_select_w(chip,data);  break;
		case 3: pc_hdc_control_w(machine, chip, data); break;
	}
}


/*************************************
 *
 *      Port handlers
 *
 *************************************/

READ8_HANDLER ( pc_HDC1_r ) { return pc_HDC_r(space->machine(), 0, offset); }
READ8_HANDLER ( pc_HDC2_r ) { return pc_HDC_r(space->machine(), 1, offset); }
WRITE8_HANDLER ( pc_HDC1_w ) { pc_HDC_w(space->machine(), 0, offset, data); }
WRITE8_HANDLER ( pc_HDC2_w ) { pc_HDC_w(space->machine(), 1, offset, data); }

READ16_HANDLER ( pc16le_HDC1_r ) { return read16le_with_read8_handler(pc_HDC1_r, space, offset, mem_mask); }
READ16_HANDLER ( pc16le_HDC2_r ) { return read16le_with_read8_handler(pc_HDC2_r, space, offset, mem_mask); }
WRITE16_HANDLER ( pc16le_HDC1_w ) { write16le_with_write8_handler(pc_HDC1_w, space, offset, data, mem_mask); }
WRITE16_HANDLER ( pc16le_HDC2_w ) { write16le_with_write8_handler(pc_HDC2_w, space, offset, data, mem_mask); }

READ32_HANDLER ( pc32le_HDC1_r ) { return read32le_with_read8_handler(pc_HDC1_r, space, offset, mem_mask); }
READ32_HANDLER ( pc32le_HDC2_r ) { return read32le_with_read8_handler(pc_HDC2_r, space, offset, mem_mask); }
WRITE32_HANDLER ( pc32le_HDC1_w ) { write32le_with_write8_handler(pc_HDC1_w, space, offset, data, mem_mask); }
WRITE32_HANDLER ( pc32le_HDC2_w ) { write32le_with_write8_handler(pc_HDC2_w, space, offset, data, mem_mask); }


MACHINE_CONFIG_FRAGMENT( pc_hdc )
	/* harddisk */
	MCFG_HARDDISK_ADD("harddisk1")
	MCFG_HARDDISK_ADD("harddisk2")
	MCFG_HARDDISK_ADD("harddisk3")
	MCFG_HARDDISK_ADD("harddisk4")
MACHINE_CONFIG_END


