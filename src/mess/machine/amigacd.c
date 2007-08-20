/***************************************************************************

	Amiga cdrom controller emulation
	
Notes:
Many thanks to Toni Willem for all the help and information about the
DMAC controller.

***************************************************************************/

#include "mame.h"
#include "driver.h"
#include "amiga.h"
#include "amigacd.h"
#include "machine/6526cia.h"
#include "machine/wd33c93.h"
#include "tpi6525.h"
#include "matsucd.h"

#define VERBOSE_DMAC 0

#if VERBOSE_DMAC
#define LOG logerror
#else
INLINE void dummy_log( const char *a, ... ) {}
#define LOG dummy_log
#endif

/* constants */
#define CD_SECTOR_TIME		(1000/((300*1024)/2048))	/* 2X CDROM sector time in msec (300KBps) */

/***************************************************************************

	DMAC
	
***************************************************************************/

/*
 * value to go into DAWR
 */
#define DAWR_ATZSC      3       /* according to A3000T service-manual */

/*
 * bits defined for CNTR
 */
#define CNTR_TCEN       (1<<7)  /* Terminal Count Enable */
#define CNTR_PREST      (1<<6)  /* Perp Reset (not implemented :-((( ) */
#define CNTR_PDMD       (1<<5)  /* Perp Device Mode Select (1=SCSI,0=XT/AT) */
#define CNTR_INTEN      (1<<4)  /* Interrupt Enable */
#define CNTR_DDIR       (1<<3)  /* Device Direction. 1==rd host, wr to perp */

/*
 * bits defined for ISTR
 */
#define ISTR_INTX       (1<<8)  /* XT/AT Interrupt pending */
#define ISTR_INT_F      (1<<7)  /* Interrupt Follow */
#define ISTR_INTS       (1<<6)  /* SCSI Peripheral Interrupt */
#define ISTR_E_INT      (1<<5)  /* End-Of-Process Interrupt */
#define ISTR_INT_P      (1<<4)  /* Interrupt Pending */
#define ISTR_UE_INT     (1<<3)  /* Under-Run FIFO Error Interrupt */
#define ISTR_OE_INT     (1<<2)  /* Over-Run FIFO Error Interrupt */
#define ISTR_FF_FLG     (1<<1)  /* FIFO-Full Flag */
#define ISTR_FE_FLG     (1<<0)  /* FIFO-Empty Flag */

typedef struct
{
	UINT16		istr;		/* Interrupt Status Register (R) */
 	UINT16		cntr;		/* Control Register (RW) */
	UINT32		wtc;		/* Word Transfer Count Register (RW) */
	UINT32		acr;		/* Address Count Register (RW) */
	UINT16		dawr;		/* DACK Width Register (W) */
	mame_timer *dma_timer;
} _dmac_data;

static _dmac_data dmac_data;

static void check_interrupts( void )
{
	/* if interrupts are disabled, bail */
	if ( (dmac_data.cntr & CNTR_INTEN) == 0 )
		return;
	
	/* if no interrupts are pending, bail */
	if ( (dmac_data.istr & ISTR_INT_P) == 0 )
		return;
	
	/* otherwise, generate the IRQ */
	amiga_custom_w(REG_INTREQ, 0x8000 | INTENA_PORTS, 0);
}

static TIMER_CALLBACK(dmac_dma_proc)
{
	while( dmac_data.wtc > 0 )
	{
		UINT16	dat16;
		UINT8	dat8;
		
		if ( matsucd_get_next_byte( &dat8 ) < 0 )
			break;
		
		dat16 = dat8;
					
		if ( matsucd_get_next_byte( &dat8 ) < 0 )
			break;
		
		dat16 <<= 8;
		dat16 |= dat8;
		
		amiga_chip_ram_w(dmac_data.acr, dat16);
		
		dmac_data.acr += 2;
		dmac_data.wtc--;
	}
	
	if ( dmac_data.wtc > 0 )
	{
		matsucd_read_next_block();
		mame_timer_adjust( dmac_data.dma_timer, MAME_TIME_IN_MSEC( CD_SECTOR_TIME ), 0, time_zero );
	}
	else
	{
		dmac_data.istr |= ISTR_INT_P | ISTR_E_INT;
		check_interrupts();
	}
}

static READ16_HANDLER( amiga_dmac_r )
{
	offset &= 0xff;

	switch( offset )
	{
		case 0x20:
		{
			UINT8	v = dmac_data.istr;
			LOG( "DMAC: PC=%08x - ISTR Read(%04x)\n", activecpu_get_pc(), dmac_data.istr );
			
			dmac_data.istr &= ~0x0f;
			return v;
		}
		break;
		
		case 0x21:
		{
			LOG( "DMAC: PC=%08x - CNTR Read(%04x)\n", activecpu_get_pc(), dmac_data.cntr );
			return dmac_data.cntr;
		}
		break;
		
		case 0x40:	/* wtc hi */
		{
			LOG( "DMAC: PC=%08x - WTC HI Read\n", activecpu_get_pc() );
			return (dmac_data.wtc >> 16);
		}
		break;
			
		case 0x41:	/* wtc lo */
		{
			LOG( "DMAC: PC=%08x - WTC LO Read\n", activecpu_get_pc() );
			return dmac_data.wtc;
		}
		break;
		
		case 0x42:	/* acr hi */
		{
			LOG( "DMAC: PC=%08x - ACR HI Read\n", activecpu_get_pc() );
			return (dmac_data.acr >> 16);
		}
		break;
			
		case 0x43:	/* acr lo */
		{
			LOG( "DMAC: PC=%08x - ACR LO Read\n", activecpu_get_pc() );
			return dmac_data.acr;
		}
		break;

		case 0x48:	/* wd33c93 SCSI expansion */
		case 0x49:
		{
			LOG( "DMAC: PC=%08x - WD33C93 Read(%d)\n", activecpu_get_pc(), offset & 1 );
			return 0x00;	/* Not available without SCSI expansion */
		}
		break;
		
		case 0x50:
		{
			LOG( "DMAC: PC=%08x - CDROM RESP Read\n", activecpu_get_pc() );
			return matsucd_response_r();
		}
		break;
		
		case 0x51:	/* XT IO */
		case 0x52:
		case 0x53:
		{
			LOG( "DMAC: PC=%08x - XT IO Read(%d)\n", activecpu_get_pc(), (offset & 3)-1 );
			return 0xff;
		}
		break;
		
		case 0x58:	/* TPI6525 */
		case 0x59:
		case 0x5A:
		case 0x5B:
		case 0x5C:
		case 0x5D:
		case 0x5E:
		case 0x5F:
		case 0x60:
		case 0x61:
		case 0x62:
		case 0x63:
		case 0x64:
		case 0x65:
		case 0x66:
		case 0x67:
		{
			LOG( "DMAC: PC=%08x - TPI6525 Read(%d)\n", activecpu_get_pc(), (offset - 0x58) );
			return tpi6525_0_port_r(offset - 0x58);
		}
		break;
		
		case 0x70:	/* DMA start strobe */
		{
			LOG( "DMAC: PC=%08x - DMA Start Strobe\n", activecpu_get_pc() );
			mame_timer_adjust( dmac_data.dma_timer, MAME_TIME_IN_MSEC( CD_SECTOR_TIME ), 0, time_zero );
		}
		break;
		
		case 0x71:	/* DMA stop strobe */
		{
			LOG( "DMAC: PC=%08x - DMA Stop Strobe\n", activecpu_get_pc() );
			mame_timer_reset( dmac_data.dma_timer, time_never );
		}
		break;
		
		case 0x72:	/* Clear IRQ strobe */
		{
			LOG( "DMAC: PC=%08x - IRQ Clear Strobe\n", activecpu_get_pc() );
			dmac_data.istr &= ~ISTR_INT_P;
		}
		break;
		
		case 0x74:	/* Flush strobe */
		{
			LOG( "DMAC: PC=%08x - Flush Strobe\n", activecpu_get_pc() );
			dmac_data.istr |= ISTR_FE_FLG;
		}
		break;
				
		default:
			logerror( "DMAC-READ: PC=%08x, offset = %02x\n", activecpu_get_pc(), offset );
		break;
	}
	
	return 0;
}

static WRITE16_HANDLER( amiga_dmac_w )
{
	offset &= 0xff;

	switch( offset )
	{
		case 0x21:	/* control write */
		{
			LOG( "DMAC: PC=%08x - CNTR Write(%04x)\n", activecpu_get_pc(), data );
			dmac_data.cntr = data;
			check_interrupts();
		}
		break;
		
		case 0x40:	/* wtc hi */
		{
			LOG( "DMAC: PC=%08x - WTC HI Write - data = %04x\n", activecpu_get_pc(), data );
			dmac_data.wtc &= 0x0000ffff;
			dmac_data.wtc |= ((UINT32)data) << 16;
		}
		break;
			
		case 0x41:	/* wtc lo */
		{
			LOG( "DMAC: PC=%08x - WTC LO Write - data = %04x\n", activecpu_get_pc(), data );
			dmac_data.wtc &= 0xffff0000;
			dmac_data.wtc |= data;
		}
		break;
		
		case 0x42:	/* acr hi */
		{
			LOG( "DMAC: PC=%08x - ACR HI Write - data = %04x\n", activecpu_get_pc(), data );
			dmac_data.acr &= 0x0000ffff;
			dmac_data.acr |= ((UINT32)data) << 16;
		}
		break;
			
		case 0x43:	/* acr lo */
		{
			LOG( "DMAC: PC=%08x - ACR LO Write - data = %04x\n", activecpu_get_pc(), data );
			dmac_data.acr &= 0xffff0000;
			dmac_data.acr |= data;
		}
		break;
		
		case 0x47:	/* dawr */
		{
			LOG( "DMAC: PC=%08x - DAWR Write - data = %04x\n", activecpu_get_pc(), data );
			dmac_data.dawr = data;
		}
		break;
		
		case 0x48:	/* wd33c93 SCSI expansion */
		case 0x49:
		{
			LOG( "DMAC: PC=%08x - WD33C93 Write(%d) - data = %04x\n", activecpu_get_pc(), offset & 1, data );
			/* Not available without SCSI expansion */
		}
		break;
		
		case 0x50:
		{
			LOG( "DMAC: PC=%08x - CDROM CMD Write - data = %04x\n", activecpu_get_pc(), data );
			matsucd_command_w( data );
		}
		break;
		
		case 0x58:	/* TPI6525 */
		case 0x59:
		case 0x5A:
		case 0x5B:
		case 0x5C:
		case 0x5D:
		case 0x5E:
		case 0x5F:
		case 0x60:
		case 0x61:
		case 0x62:
		case 0x63:
		case 0x64:
		case 0x65:
		case 0x66:
		case 0x67:
		{
			LOG( "DMAC: PC=%08x - TPI6525 Write(%d) - data = %04x\n", activecpu_get_pc(), (offset - 0x58), data );
			tpi6525_0_port_w(offset - 0x58, data);
		}
		break;
		
		case 0x70:	/* DMA start strobe */
		{
			LOG( "DMAC: PC=%08x - DMA Start Strobe\n", activecpu_get_pc() );
			mame_timer_adjust( dmac_data.dma_timer, MAME_TIME_IN_MSEC( CD_SECTOR_TIME ), 0, time_zero );
		}
		break;
		
		case 0x71:	/* DMA stop strobe */
		{
			LOG( "DMAC: PC=%08x - DMA Stop Strobe\n", activecpu_get_pc() );
			mame_timer_reset( dmac_data.dma_timer, time_never );
		}
		break;
		
		case 0x72:	/* Clear IRQ strobe */
		{
			LOG( "DMAC: PC=%08x - IRQ Clear Strobe\n", activecpu_get_pc() );
			dmac_data.istr &= ~ISTR_INT_P;
		}
		break;
		
		case 0x74:	/* Flush Strobe */
		{
			LOG( "DMAC: PC=%08x - Flush Strobe\n", activecpu_get_pc() );
			dmac_data.istr |= ISTR_FE_FLG;
		}
		break;
		
		default:
			logerror( "DMAC-WRITE: PC=%08x, offset = %02x, data = %04x\n", activecpu_get_pc(), offset, data );
		break;
	}
}

/***************************************************************************

	Autoconfig
	
***************************************************************************/

static void	dmac_install(offs_t base)
{
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, base, base + 0xFFFF, 0, 0, amiga_dmac_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, base, base + 0xFFFF, 0, 0, amiga_dmac_w);
}

static void	dmac_uninstall(offs_t base)
{
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, base, base + 0xFFFF, 0, 0, MRA16_UNMAP);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, base, base + 0xFFFF, 0, 0, MWA16_UNMAP);
}

static amiga_autoconfig_device dmac_device =
{
	0,				/* link into free memory list */
	0,				/* ROM vector offset valid */
	0,				/* multiple devices on card */
	1,				/* number of 64k pages */
	3,				/* product number (DMAC) */
	0,				/* prefer 8MB address space */
	0,				/* can be shut up */
	0x0202,			/* manufacturers number (Commodore) */
	0,				/* serial number */
	0,				/* ROM vector offset */
	NULL,			/* interrupt control read */
	NULL,			/* interrupt control write */
	dmac_install,	/* memory installation */
	dmac_uninstall	/* memory uninstallation */
};

/***************************************************************************

	TPI6525
	
***************************************************************************/

static int tp6525_portc_r( void )
{
	int	ret = 0;
	
	if ( (tpi6525[0].c.ddr & 0x04) == 0 ) /* if pin 2 is set to input */
	{
		ret |= matsucd_stch_r() ? 0x00 : 0x04;	/* read status change signal */
	}
	
	if ( (tpi6525[0].c.ddr & 0x08) == 0 ) /* if pin 3 is set to input */
		ret |= matsucd_sten_r() ? 0x08 : 0x00;	/* read enable signal */
	
	return ret;
}

static void tp6525_portb_w( int data )
{
	if ( tpi6525[0].b.ddr & 0x01 ) /* if pin 0 is set to output */
		matsucd_cmd_w( data & 1 ); /* write to the /CMD signal */
	
	if ( tpi6525[0].b.ddr & 0x02 ) /* if pin 1 is set to output */
		matsucd_enable_w( data & 2 ); /* write to the /ENABLE signal */
}

static mame_timer *tp6525_delayed_timer;

static TIMER_CALLBACK(tp6525_delayed_irq)
{
	(void)param;
	
	if ( (CUSTOM_REG(REG_INTREQ) & INTENA_PORTS) == 0 )
	{
		amiga_custom_w(REG_INTREQ, 0x8000 | INTENA_PORTS, 0);
	}
	else
	{
		mame_timer_adjust( tp6525_delayed_timer, MAME_TIME_IN_MSEC(1), 0, time_zero );
	}
}

static void tp6525_irq( int level )
{
	LOG( "TPI6525 Interrupt: level = %d\n", level);
	
	if ( level )
	{
		if ( (CUSTOM_REG(REG_INTREQ) & INTENA_PORTS) == 0 )
		{
			amiga_custom_w(REG_INTREQ, 0x8000 | INTENA_PORTS, 0);
		}
		else
		{
			/* we *have to* deliver the irq, so if we can't, delay it and try again later */
			mame_timer_adjust( tp6525_delayed_timer, MAME_TIME_IN_MSEC(1), 0, time_zero );
		}
	}
}

static void cdrom_status_enabled( int level )
{
	/* PC3 on the 6525 */
	tpi6525_0_irq3_level( level );
}

static void cdrom_status_change( int level )
{
	/* invert */
	level = level ? 0 : 1;
	
	/* PC2 on the 6525 */
	tpi6525_0_irq2_level( level );
}

static void cdrom_subcode_ready( int level )
{
	/* PC1 on the 6525 */
	tpi6525_0_irq1_level( level );
}

void amigacd_init( void )
{
	/* initialize the dmac */
	memset( &dmac_data, 0, sizeof( dmac_data ) );
	
	dmac_data.dma_timer = mame_timer_alloc(dmac_dma_proc);
	tp6525_delayed_timer = mame_timer_alloc(tp6525_delayed_irq);
	
	/* initialize the 6525 TPI */
	tpi6525_0_reset();
	
	tpi6525[0].a.read = NULL;
	tpi6525[0].b.read = NULL;
	tpi6525[0].c.read = tp6525_portc_r;
	
	tpi6525[0].a.output = NULL;
	tpi6525[0].b.output = tp6525_portb_w;
	tpi6525[0].c.output = NULL;
	
	tpi6525[0].interrupt.output = tp6525_irq;
	tpi6525[0].ca.output = NULL;
	tpi6525[0].cb.output = NULL;
	
	/* initialize the cdrom */
	matsucd_init();
	matsucd_set_status_enabled_callback( cdrom_status_enabled );
	matsucd_set_status_changed_callback( cdrom_status_change );
	matsucd_set_subcode_ready_callback( cdrom_subcode_ready );
	
	/* set up DMAC with autoconfig */
	amiga_add_autoconfig( &dmac_device );
}
