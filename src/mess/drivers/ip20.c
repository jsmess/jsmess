/*********************************************************************\
*
*   SGI IP20 IRIS Indigo workstation
*
*  Skeleton Driver
*
*  Todo: Everything
*
*  Note: Machine uses R4400, not R4600
*
*  Memory map:
*
*  1fa00000 - 1fa02047      Memory Controller
*  1fb80000 - 1fb9a7ff      HPC1 CHIP0
*  1fc00000 - 1fc7ffff      BIOS
*
\*********************************************************************/

#include "driver.h"
#include "cpu/mips/mips3.h"
#include "machine/8530scc.h"
#include "machine/sgi.h"
#include "machine/eeprom.h"
#include "machine/8530scc.h"
#include "machine/wd33c93.h"
#include "devices/harddriv.h"
#include "devices/chd_cd.h"

#define VERBOSE_LEVEL ( 2 )

INLINE void verboselog( int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%08x: %s", activecpu_get_pc(), buf );
//		mame_printf_info("%08x: %s", activecpu_get_pc(), buf);
	}
}

static VIDEO_START( ip204415 )
{
	return 0;
}

static VIDEO_UPDATE( ip204415 )
{
	return 0;
}

static struct EEPROM_interface eeprom_interface_93C56 =
{
	7,					// address bits	7
	16,					// data bits	16
	"*110x",			// read			110x aaaaaaaa
	"*101x",			// write		101x aaaaaaaa dddddddd
	"*111x",			// erase		111x aaaaaaaa
	"*10000xxxxxxx",	// lock			100x 00xxxx
	"*10011xxxxxxx",	// unlock		100x 11xxxx
};

static NVRAM_HANDLER(93C56)
{
	if (read_or_write)
	{
		EEPROM_save(file);
	}
	else
	{
		EEPROM_init(&eeprom_interface_93C56);
		if (file)
		{
			EEPROM_load(file);
		}
		else
		{
			int length;
			UINT8 *dat;

			dat = EEPROM_get_data_pointer(&length);
			memset(dat, 0, length);
		}
	}
}

static UINT8 nHPC_MiscStatus;
static UINT32 nHPC_ParBufPtr;
static UINT32 nHPC_LocalIOReg0Mask;
static UINT32 nHPC_LocalIOReg1Mask;
static UINT32 nHPC_VMEIntMask0;
static UINT32 nHPC_VMEIntMask1;

static UINT8 nRTC_RAM[32];
static UINT8 nRTC_Temp;

static UINT32 nHPC_SCSI0Descriptor, nHPC_SCSI0DMACtrl;

#define RTC_DAYOFWEEK	nRTC_RAM[0x0e]
#define RTC_YEAR		nRTC_RAM[0x0b]
#define RTC_MONTH		nRTC_RAM[0x0a]
#define RTC_DAY			nRTC_RAM[0x09]
#define RTC_HOUR		nRTC_RAM[0x08]
#define RTC_MINUTE		nRTC_RAM[0x07]
#define RTC_SECOND		nRTC_RAM[0x06]
#define RTC_HUNDREDTH	nRTC_RAM[0x05]

static READ32_HANDLER( hpc_r )
{
	offset <<= 2;
	if( offset >= 0x0e00 && offset <= 0x0e7c )
	{
		verboselog( 2, "RTC RAM[0x%02x] Read: %02x\n", ( offset - 0xe00 ) >> 2, nRTC_RAM[ ( offset - 0xe00 ) >> 2 ] );
		return nRTC_RAM[ ( offset - 0xe00 ) >> 2 ];
	}
	switch( offset )
	{
	case 0x05c:
		verboselog( 2, "HPC Unknown Read: %08x (%08x) (returning 0x000000a5 as kludge)\n", 0x1fb80000 + offset, mem_mask );
		return 0x0000a500;
		break;
	case 0x00ac:
		verboselog( 2, "HPC Parallel Buffer Pointer Read: %08x (%08x)\n", nHPC_ParBufPtr, mem_mask );
		return nHPC_ParBufPtr;
		break;
	case 0x00c0:
		verboselog( 2, "HPC Endianness Read: %08x (%08x)\n", 0x0000001f, mem_mask );
		return 0x0000001f;
		break;
	case 0x0120:
		if (!(mem_mask & 0x0000ff00))
		{
			return ( wd33c93_r( 0 ) << 8 );
		}
		else
		{
			return 0;
		}
		break;
	case 0x0124:
		if (!(mem_mask & 0x0000ff00))
		{
			return ( wd33c93_r( 1 ) << 8 );
		}
		else
		{
			return 0;
		}
		break;
	case 0x01b0:
		verboselog( 2, "HPC Misc. Status Read: %08x (%08x)\n", nHPC_MiscStatus, mem_mask );
		return nHPC_MiscStatus;
		break;
	case 0x01bc:
//		verboselog( 2, "HPC CPU Serial EEPROM Read\n" );
		return ( ( EEPROM_read_bit() << 4 ) );
		break;
	case 0x01c4:
		verboselog( 2, "HPC Local IO Register 0 Mask Read: %08x (%08x)\n", nHPC_LocalIOReg0Mask, mem_mask );
		return nHPC_LocalIOReg0Mask;
		break;
	case 0x01cc:
		verboselog( 2, "HPC Local IO Register 1 Mask Read: %08x (%08x)\n", nHPC_LocalIOReg0Mask, mem_mask );
		return nHPC_LocalIOReg1Mask;
		break;
	case 0x01d4:
		verboselog( 2, "HPC VME Interrupt Mask 0 Read: %08x (%08x)\n", nHPC_LocalIOReg0Mask, mem_mask );
		return nHPC_VMEIntMask0;
		break;
	case 0x01d8:
		verboselog( 2, "HPC VME Interrupt Mask 1 Read: %08x (%08x)\n", nHPC_LocalIOReg0Mask, mem_mask );
		return nHPC_VMEIntMask1;
		break;
	case 0x0d00:
		verboselog( 2, "HPC DUART0 Channel B Control Read\n" );
//		return 0x00000004;
		return 0x7c; //scc_r(0);
		break;
	case 0x0d04:
		verboselog( 2, "HPC DUART0 Channel B Data Read\n" );
//		return 0;
		return scc_r(2);
		break;
	case 0x0d08:
		verboselog( 2, "HPC DUART0 Channel A Control Read (%08x)\n", mem_mask	 );
//		return 0x40;
		return 0x7c; //scc_r(1);
		break;
	case 0x0d0c:
		verboselog( 2, "HPC DUART0 Channel A Data Read\n" );
//		return 0;
		return scc_r(3);
		break;
	case 0x0d10:
//		verboselog( 2, "HPC DUART1 Channel B Control Read\n" );
		return 0x00000004;
		break;
	case 0x0d14:
		verboselog( 2, "HPC DUART1 Channel B Data Read\n" );
		return 0;
		break;
	case 0x0d18:
		verboselog( 2, "HPC DUART1 Channel A Control Read\n" );
		return 0;
		break;
	case 0x0d1c:
		verboselog( 2, "HPC DUART1 Channel A Data Read\n" );
		return 0;
		break;
	case 0x0d20:
		verboselog( 2, "HPC DUART2 Channel B Control Read\n" );
		return 0x00000004;
		break;
	case 0x0d24:
		verboselog( 2, "HPC DUART2 Channel B Data Read\n" );
		return 0;
		break;
	case 0x0d28:
		verboselog( 2, "HPC DUART2 Channel A Control Read\n" );
		return 0;
		break;
	case 0x0d2c:
		verboselog( 2, "HPC DUART2 Channel A Data Read\n" );
		return 0;
		break;
	case 0x0d30:
		verboselog( 2, "HPC DUART3 Channel B Control Read\n" );
		return 0x00000004;
		break;
	case 0x0d34:
		verboselog( 2, "HPC DUART3 Channel B Data Read\n" );
		return 0;
		break;
	case 0x0d38:
		verboselog( 2, "HPC DUART3 Channel A Control Read\n" );
		return 0;
		break;
	case 0x0d3c:
		verboselog( 2, "HPC DUART3 Channel A Data Read\n" );
		return 0;
		break;
	}
	verboselog( 0, "Unmapped HPC read: 0x%08x (%08x)\n", 0x1fb80000 + offset, mem_mask );
	return 0;
}

static WRITE32_HANDLER( hpc_w )
{
	offset <<= 2;
	if( offset >= 0x0e00 && offset <= 0x0e7c )
	{
		verboselog( 2, "RTC RAM[0x%02x] Write: %02x\n", ( offset - 0xe00 ) >> 2, data & 0x000000ff );
		nRTC_RAM[ ( offset - 0xe00 ) >> 2 ] = data & 0x000000ff;
		switch( ( offset - 0xe00 ) >> 2 )
		{
		case 0:
			break;
		case 4:
			if( !( nRTC_RAM[0x00] & 0x80 ) )
			{
				if( data & 0x80 )
				{
					nRTC_RAM[0x19] = RTC_SECOND;
					nRTC_RAM[0x1a] = RTC_MINUTE;
					nRTC_RAM[0x1b] = RTC_HOUR;
					nRTC_RAM[0x1c] = RTC_DAY;
					nRTC_RAM[0x1d] = RTC_MONTH;
				}
			}
			break;
		}
		return;
	}
	switch( offset )
	{
	case 0x0090:	// SCSI0 next descriptor pointer
		nHPC_SCSI0Descriptor = data;
		break;

	case 0x0094:	// SCSI0 control flags
		nHPC_SCSI0DMACtrl = data;
		#if 0
		if (data & 0x80)
		{
			UINT32 next;

			mame_printf_info("DMA activated for SCSI0\n");
			mame_printf_info("Descriptor block:\n");
			mame_printf_info("CTL: %08x BUFPTR: %08x DESCPTR %08x\n",
				program_read_dword(nHPC_SCSI0Descriptor), program_read_dword(nHPC_SCSI0Descriptor+4),
				program_read_dword(nHPC_SCSI0Descriptor+8));

			next = program_read_dword(nHPC_SCSI0Descriptor+8);
			mame_printf_info("CTL: %08x BUFPTR: %08x DESCPTR %08x\n",
				program_read_dword(next), program_read_dword(next+4),
				program_read_dword(next+8));
		}
		#endif
		break;

	case 0x00ac:
		verboselog( 2, "HPC Parallel Buffer Pointer Write: %08x (%08x)\n", data, mem_mask );
		nHPC_ParBufPtr = data;
		break;
	case 0x0120:
		if (!(mem_mask & 0x0000ff00))
		{
			verboselog( 2, "HPC SCSI Controller Register Write: %08x\n", ( data >> 8 ) & 0x000000ff );
			wd33c93_w( 0, ( data >> 8 ) & 0x000000ff );
		}
		else
		{
			return;
		}
		break;
	case 0x0124:
		if (!(mem_mask & 0x0000ff00))
		{
			verboselog( 2, "HPC SCSI Controller Data Write: %08x\n", ( data >> 8 ) & 0x000000ff );
			wd33c93_w( 1, ( data >> 8 ) & 0x000000ff );
		}
		else
		{
			return;
		}
		break;
	case 0x01b0:
		verboselog( 2, "HPC Misc. Status Write: %08x (%08x)\n", data, mem_mask );
		if( data & 0x00000001 )
		{
			verboselog( 2, "  Force DSP hard reset\n" );
		}
		if( data & 0x00000002 )
		{
			verboselog( 2, "  Force IRQA\n" );
		}
		if( data & 0x00000004 )
		{
			verboselog( 2, "  Set IRQA polarity high\n" );
		}
		else
		{
			verboselog( 2, "  Set IRQA polarity low\n" );
		}
		if( data & 0x00000008 )
		{
			verboselog( 2, "  SRAM size: 32K\n" );
		}
		else
		{
			verboselog( 2, "  SRAM size:  8K\n" );
		}
		nHPC_MiscStatus = data;
		break;
	case 0x01bc:
//		verboselog( 2, "HPC CPU Serial EEPROM Write: %08x (%08x)\n", data, mem_mask );
		if( data & 0x00000001 )
		{
			verboselog( 2, "    CPU board LED on\n" );
		}
		EEPROM_write_bit( (data & 0x00000008) ? 1 : 0 );
		EEPROM_set_cs_line( (data & 0x00000002) ? ASSERT_LINE : CLEAR_LINE );
		EEPROM_set_clock_line( (data & 0x00000004) ? CLEAR_LINE : ASSERT_LINE );
		break;
	case 0x01c4:
		verboselog( 2, "HPC Local IO Register 0 Mask Write: %08x (%08x)\n", data, mem_mask );
		nHPC_LocalIOReg0Mask = data;
		break;
	case 0x01cc:
		verboselog( 2, "HPC Local IO Register 1 Mask Write: %08x (%08x)\n", data, mem_mask );
		nHPC_LocalIOReg1Mask = data;
		break;
	case 0x01d4:
		verboselog( 2, "HPC VME Interrupt Mask 0 Write: %08x (%08x)\n", data, mem_mask );
		nHPC_VMEIntMask0 = data;
		break;
	case 0x01d8:
		verboselog( 2, "HPC VME Interrupt Mask 1 Write: %08x (%08x)\n", data, mem_mask );
		nHPC_VMEIntMask1 = data;
		break;
	case 0x0d00:
		verboselog( 2, "HPC DUART0 Channel B Control Write: %08x (%08x)\n", data, mem_mask );
		scc_w(0, data);
		break;
	case 0x0d04:
		verboselog( 2, "HPC DUART0 Channel B Data Write: %08x (%08x)\n", data, mem_mask );
		scc_w(2, data);
		break;
	case 0x0d08:
		verboselog( 2, "HPC DUART0 Channel A Control Write: %08x (%08x)\n", data, mem_mask );
		scc_w(1, data);
		break;
	case 0x0d0c:
		verboselog( 2, "HPC DUART0 Channel A Data Write: %08x (%08x)\n", data, mem_mask );
		scc_w(3, data);
		break;
	case 0x0d10:
		if( ( data & 0x000000ff ) >= 0x00000020 )
		{
//			verboselog( 2, "HPC DUART1 Channel B Control Write: %08x (%08x) %c\n", data, mem_mask, data & 0x000000ff );
			//mame_printf_info( "%c", data & 0x000000ff );
		}
		else
		{
//			verboselog( 2, "HPC DUART1 Channel B Control Write: %08x (%08x)\n", data, mem_mask );
		}
		break;
	case 0x0d14:
		if( ( data & 0x000000ff ) >= 0x00000020 || ( data & 0x000000ff ) == 0x0d || ( data & 0x000000ff ) == 0x0a )
		{
			verboselog( 2, "HPC DUART1 Channel B Data Write: %08x (%08x) %c\n", data, mem_mask, data & 0x000000ff );
			mame_printf_info( "%c", data & 0x000000ff );
		}
		else
		{
			verboselog( 2, "HPC DUART1 Channel B Data Write: %08x (%08x)\n", data, mem_mask );
		}
		break;
	case 0x0d18:
		mame_printf_info("HPC DUART1 Channel A Control Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d1c:
		verboselog( 2, "HPC DUART1 Channel A Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d20:
		mame_printf_info("HPC DUART2 Channel B Control Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d24:
		verboselog( 2, "HPC DUART2 Channel B Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d28:
		mame_printf_info("HPC DUART2 Channel A Control Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d2c:
		verboselog( 2, "HPC DUART2 Channel A Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d30:
		mame_printf_info("HPC DUART3 Channel B Control Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d34:
		verboselog( 2, "HPC DUART3 Channel B Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d38:
		mame_printf_info("HPC DUART3 Channel A Control Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d3c:
		verboselog( 2, "HPC DUART3 Channel A Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	default:
		mame_printf_info("Unmapped HPC write: 0x%08x (%08x): %08x\n", 0x1fb80000 + offset, mem_mask, data);
		break;
	}
}

// INT/INT2/INT3 interrupt controllers
static READ32_HANDLER( int_r )
{
	mame_printf_info("INT: read @ ofs %x (mask %x) (PC=%x)\n", offset, mem_mask, activecpu_get_pc());
	return 0;
}

static WRITE32_HANDLER( int_w )
{
	mame_printf_info("INT: write %x to ofs %x (mask %x) (PC=%x)\n", data, offset, mem_mask, activecpu_get_pc());
}

static INTERRUPT_GEN( ip20_update_chips )
{
	mc_update(0);
	nRTC_Temp++;
	if( nRTC_Temp == 100 )
	{
		nRTC_Temp = 0;
		RTC_HUNDREDTH++;
	}
	if( ( RTC_HUNDREDTH & 0x0f ) == 0x0a )
	{
		RTC_HUNDREDTH -= 0x0a;
		RTC_HUNDREDTH += 0x10;
		if( ( RTC_HUNDREDTH & 0xa0 ) == 0xa0 )
		{
			RTC_HUNDREDTH = 0;
			RTC_SECOND++;
		}
	}
	if( ( RTC_SECOND & 0x0f ) == 0x0a )
	{
		RTC_SECOND -= 0x0a;
		RTC_SECOND += 0x10;
		if( RTC_SECOND == 0x60 )
		{
			RTC_SECOND = 0;
			RTC_MINUTE++;
		}
	}
	if( ( RTC_MINUTE & 0x0f ) == 0x0a )
	{
		RTC_MINUTE -= 0x0a;
		RTC_MINUTE += 0x10;
		if( RTC_MINUTE == 0x60 )
		{
			RTC_MINUTE = 0;
			RTC_HOUR++;
		}
	}
	if( ( RTC_HOUR & 0x0f ) == 0x0a )
	{
		RTC_HOUR -= 0x0a;
		RTC_HOUR += 0x10;
		if( RTC_HOUR == 0x24 )
		{
			RTC_HOUR = 0;
			RTC_DAY++;
		}
	}
}

static ADDRESS_MAP_START( ip204415_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE( 0x00000000, 0x001fffff ) AM_RAM AM_SHARE(10)
	AM_RANGE( 0x08000000, 0x08ffffff ) AM_RAM AM_SHARE(5)
	AM_RANGE( 0x09000000, 0x097fffff ) AM_RAM AM_SHARE(6)
	AM_RANGE( 0x0a000000, 0x0a7fffff ) AM_RAM AM_SHARE(7)
	AM_RANGE( 0x0c000000, 0x0c7fffff ) AM_RAM AM_SHARE(8)
	AM_RANGE( 0x10000000, 0x107fffff ) AM_RAM AM_SHARE(9)
	AM_RANGE( 0x18000000, 0x187fffff ) AM_RAM AM_SHARE(1)
	AM_RANGE( 0x1fa00000, 0x1fa1ffff ) AM_READWRITE( mc_r, mc_w )
	AM_RANGE( 0x1fb80000, 0x1fb8ffff ) AM_READWRITE( hpc_r, hpc_w )
	AM_RANGE( 0x1fbd9000, 0x1fbd903f ) AM_READWRITE( int_r, int_w )
	AM_RANGE( 0x1fc00000, 0x1fc7ffff ) AM_ROM AM_SHARE(2) AM_REGION( REGION_USER1, 0 )
	AM_RANGE( 0x80000000, 0x801fffff ) AM_RAM AM_SHARE(10)
	AM_RANGE( 0x88000000, 0x88ffffff ) AM_RAM AM_SHARE(5)
	AM_RANGE( 0xa0000000, 0xa01fffff ) AM_RAM AM_SHARE(10)
	AM_RANGE( 0xa8000000, 0xa8ffffff ) AM_RAM AM_SHARE(5)
	AM_RANGE( 0xa9000000, 0xa97fffff ) AM_RAM AM_SHARE(6)
	AM_RANGE( 0xaa000000, 0xaa7fffff ) AM_RAM AM_SHARE(7)
	AM_RANGE( 0xac000000, 0xac7fffff ) AM_RAM AM_SHARE(8)
	AM_RANGE( 0xb0000000, 0xb07fffff ) AM_RAM AM_SHARE(9)
	AM_RANGE( 0xb8000000, 0xb87fffff ) AM_RAM AM_SHARE(1)
	AM_RANGE( 0xbfa00000, 0xbfa1ffff ) AM_READWRITE( mc_r, mc_w )
	AM_RANGE( 0xbfb80000, 0xbfb8ffff ) AM_READWRITE( hpc_r, hpc_w )
	AM_RANGE( 0xbfbd9000, 0xbfbd903f ) AM_READWRITE( int_r, int_w )
	AM_RANGE( 0xbfc00000, 0xbfc7ffff ) AM_ROM AM_SHARE(2) /* BIOS Mirror */
ADDRESS_MAP_END

static MACHINE_RESET( ip204415 )
{
	mc_init();

	nHPC_MiscStatus = 0;
	nHPC_ParBufPtr = 0;
	nHPC_LocalIOReg0Mask = 0;
	nHPC_LocalIOReg1Mask = 0;
	nHPC_VMEIntMask0 = 0;
	nHPC_VMEIntMask1 = 0;

	nRTC_Temp = 0;
}

static void scsi_irq(int state)
{
}

#if 0
static SCSIConfigTable dev_table =
{
        1,                                      /* 1 SCSI device */
        { { SCSI_ID_6, 0, SCSI_DEVICE_CDROM } } /* SCSI ID 6, using CHD 0, and it's a CD-ROM */
};

static struct WD33C93interface scsi_intf =
{
	&dev_table,		/* SCSI device table */
	&scsi_irq,		/* command completion IRQ */
};
#endif

static DRIVER_INIT( ip204415 )
{
	scc_init(NULL);
}

INPUT_PORTS_START( ip204415 )
	PORT_START
	PORT_BIT ( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

static void ip20_chdcd_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* CHD CD-ROM */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 4; break;

		default: cdrom_device_getinfo(devclass, state, info);
	}
}

static struct mips3_config config =
{
	32768,	/* code cache size */
	32768	/* data cache size */
};

MACHINE_DRIVER_START( ip204415 )
	MDRV_CPU_ADD_TAG( "main", R4600BE, 50000000*3 )
	MDRV_CPU_CONFIG( config )
	MDRV_CPU_PROGRAM_MAP( ip204415_map, 0 )
	MDRV_CPU_VBLANK_INT( ip20_update_chips, 10000 )

	MDRV_SCREEN_REFRESH_RATE( 60 )
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET( ip204415 )
	MDRV_NVRAM_HANDLER(93C56)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(800, 600)
	MDRV_SCREEN_VISIBLE_AREA(0, 799, 0, 599)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START( ip204415 )
	MDRV_VIDEO_UPDATE( ip204415 )

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD( CDDA, 0 )
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( ip204415 )
	ROM_REGION( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "ip204415.bin", 0x000000, 0x080000, CRC(940d960e) SHA1(596aba530b53a147985ff3f6f853471ce48c866c) )
ROM_END

SYSTEM_CONFIG_START( ip204415 )
	CONFIG_DEVICE(ip20_chdcd_getinfo)
SYSTEM_CONFIG_END

/*     YEAR  NAME      PARENT    COMPAT    MACHINE   INPUT     INIT      CONFIG    COMPANY   FULLNAME */
COMP( 1993, ip204415, 0,        0,        ip204415, ip204415, ip204415, ip204415, "Silicon Graphics, Inc", "IRIS Indigo (R4400, 150MHz)", GAME_NOT_WORKING | GAME_NO_SOUND )
