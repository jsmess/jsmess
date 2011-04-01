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

#include "emu.h"
#include "cpu/mips/mips3.h"
#include "sound/cdda.h"
#include "machine/8530scc.h"
#include "machine/sgi.h"
#include "machine/eeprom.h"
#include "machine/8530scc.h"
#include "machine/wd33c93.h"
#include "imagedev/harddriv.h"
#include "imagedev/chd_cd.h"


class ip20_state : public driver_device
{
public:
	ip20_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_nHPC_MiscStatus;
	UINT32 m_nHPC_ParBufPtr;
	UINT32 m_nHPC_LocalIOReg0Mask;
	UINT32 m_nHPC_LocalIOReg1Mask;
	UINT32 m_nHPC_VMEIntMask0;
	UINT32 m_nHPC_VMEIntMask1;
	UINT8 m_nRTC_RAM[32];
	UINT8 m_nRTC_Temp;
	UINT32 m_nHPC_SCSI0Descriptor;
	UINT32 m_nHPC_SCSI0DMACtrl;
};


#define VERBOSE_LEVEL ( 2 )

INLINE void ATTR_PRINTF(3,4) verboselog(running_machine &machine, int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%08x: %s", cpu_get_pc(machine.device("maincpu")), buf );
	}
}

static VIDEO_START( ip204415 )
{
}

static SCREEN_UPDATE( ip204415 )
{
	return 0;
}

static const eeprom_interface eeprom_interface_93C56 =
{
	7,					// address bits 7
	16,					// data bits    16
	"*110x",			// read         110x aaaaaaaa
	"*101x",			// write        101x aaaaaaaa dddddddd
	"*111x",			// erase        111x aaaaaaaa
	"*10000xxxxxxx",	// lock         100x 00xxxx
	"*10011xxxxxxx",	// unlock       100x 11xxxx
};




#define RTC_DAYOFWEEK	state->m_nRTC_RAM[0x0e]
#define RTC_YEAR		state->m_nRTC_RAM[0x0b]
#define RTC_MONTH		state->m_nRTC_RAM[0x0a]
#define RTC_DAY			state->m_nRTC_RAM[0x09]
#define RTC_HOUR		state->m_nRTC_RAM[0x08]
#define RTC_MINUTE		state->m_nRTC_RAM[0x07]
#define RTC_SECOND		state->m_nRTC_RAM[0x06]
#define RTC_HUNDREDTH	state->m_nRTC_RAM[0x05]

static READ32_HANDLER( hpc_r )
{
	ip20_state *state = space->machine().driver_data<ip20_state>();
	device_t *scc;
	running_machine &machine = space->machine();

	offset <<= 2;
	if( offset >= 0x0e00 && offset <= 0x0e7c )
	{
		verboselog(machine, 2, "RTC RAM[0x%02x] Read: %02x\n", ( offset - 0xe00 ) >> 2, state->m_nRTC_RAM[ ( offset - 0xe00 ) >> 2 ] );
		return state->m_nRTC_RAM[ ( offset - 0xe00 ) >> 2 ];
	}
	switch( offset )
	{
	case 0x05c:
		verboselog(machine, 2, "HPC Unknown Read: %08x (%08x) (returning 0x000000a5 as kludge)\n", 0x1fb80000 + offset, mem_mask );
		return 0x0000a500;
	case 0x00ac:
		verboselog(machine, 2, "HPC Parallel Buffer Pointer Read: %08x (%08x)\n", state->m_nHPC_ParBufPtr, mem_mask );
		return state->m_nHPC_ParBufPtr;
	case 0x00c0:
		verboselog(machine, 2, "HPC Endianness Read: %08x (%08x)\n", 0x0000001f, mem_mask );
		return 0x0000001f;
	case 0x0120:
		if (ACCESSING_BITS_8_15)
		{
			return ( wd33c93_r( space, 0 ) << 8 );
		}
		else
		{
			return 0;
		}
	case 0x0124:
		if (ACCESSING_BITS_8_15)
		{
			return ( wd33c93_r( space, 1 ) << 8 );
		}
		else
		{
			return 0;
		}
	case 0x01b0:
		verboselog(machine, 2, "HPC Misc. Status Read: %08x (%08x)\n", state->m_nHPC_MiscStatus, mem_mask );
		return state->m_nHPC_MiscStatus;
	case 0x01bc:
//      verboselog(machine, 2, "HPC CPU Serial EEPROM Read\n" );
		return ( ( eeprom_read_bit(space->machine().device("eeprom")) << 4 ) );
	case 0x01c4:
		verboselog(machine, 2, "HPC Local IO Register 0 Mask Read: %08x (%08x)\n", state->m_nHPC_LocalIOReg0Mask, mem_mask );
		return state->m_nHPC_LocalIOReg0Mask;
	case 0x01cc:
		verboselog(machine, 2, "HPC Local IO Register 1 Mask Read: %08x (%08x)\n", state->m_nHPC_LocalIOReg0Mask, mem_mask );
		return state->m_nHPC_LocalIOReg1Mask;
	case 0x01d4:
		verboselog(machine, 2, "HPC VME Interrupt Mask 0 Read: %08x (%08x)\n", state->m_nHPC_LocalIOReg0Mask, mem_mask );
		return state->m_nHPC_VMEIntMask0;
	case 0x01d8:
		verboselog(machine, 2, "HPC VME Interrupt Mask 1 Read: %08x (%08x)\n", state->m_nHPC_LocalIOReg0Mask, mem_mask );
		return state->m_nHPC_VMEIntMask1;
	case 0x0d00:
		verboselog(machine, 2, "HPC DUART0 Channel B Control Read\n" );
//      return 0x00000004;
		return 0x7c; //scc8530_r(machine, 0);
	case 0x0d04:
		verboselog(machine, 2, "HPC DUART0 Channel B Data Read\n" );
//      return 0;
		scc = space->machine().device("scc");
		return scc8530_r(scc, 2);
	case 0x0d08:
		verboselog(machine, 2, "HPC DUART0 Channel A Control Read (%08x)\n", mem_mask	 );
//      return 0x40;
		return 0x7c; //scc8530_r(machine, 1);
	case 0x0d0c:
		verboselog(machine, 2, "HPC DUART0 Channel A Data Read\n" );
//      return 0;
		scc = space->machine().device("scc");
		return scc8530_r(scc, 3);
	case 0x0d10:
//      verboselog(machine, 2, "HPC DUART1 Channel B Control Read\n" );
		return 0x00000004;
	case 0x0d14:
		verboselog(machine, 2, "HPC DUART1 Channel B Data Read\n" );
		return 0;
	case 0x0d18:
		verboselog(machine, 2, "HPC DUART1 Channel A Control Read\n" );
		return 0;
	case 0x0d1c:
		verboselog(machine, 2, "HPC DUART1 Channel A Data Read\n" );
		return 0;
	case 0x0d20:
		verboselog(machine, 2, "HPC DUART2 Channel B Control Read\n" );
		return 0x00000004;
	case 0x0d24:
		verboselog(machine, 2, "HPC DUART2 Channel B Data Read\n" );
		return 0;
	case 0x0d28:
		verboselog(machine, 2, "HPC DUART2 Channel A Control Read\n" );
		return 0;
	case 0x0d2c:
		verboselog(machine, 2, "HPC DUART2 Channel A Data Read\n" );
		return 0;
	case 0x0d30:
		verboselog(machine, 2, "HPC DUART3 Channel B Control Read\n" );
		return 0x00000004;
	case 0x0d34:
		verboselog(machine, 2, "HPC DUART3 Channel B Data Read\n" );
		return 0;
	case 0x0d38:
		verboselog(machine, 2, "HPC DUART3 Channel A Control Read\n" );
		return 0;
	case 0x0d3c:
		verboselog(machine, 2, "HPC DUART3 Channel A Data Read\n" );
		return 0;
	}
	verboselog(machine, 0, "Unmapped HPC read: 0x%08x (%08x)\n", 0x1fb80000 + offset, mem_mask );
	return 0;
}

static WRITE32_HANDLER( hpc_w )
{
	ip20_state *state = space->machine().driver_data<ip20_state>();
	device_t *scc;
	device_t *eeprom;
	running_machine &machine = space->machine();

	eeprom = space->machine().device("eeprom");
	offset <<= 2;
	if( offset >= 0x0e00 && offset <= 0x0e7c )
	{
		verboselog(machine, 2, "RTC RAM[0x%02x] Write: %02x\n", ( offset - 0xe00 ) >> 2, data & 0x000000ff );
		state->m_nRTC_RAM[ ( offset - 0xe00 ) >> 2 ] = data & 0x000000ff;
		switch( ( offset - 0xe00 ) >> 2 )
		{
		case 0:
			break;
		case 4:
			if( !( state->m_nRTC_RAM[0x00] & 0x80 ) )
			{
				if( data & 0x80 )
				{
					state->m_nRTC_RAM[0x19] = RTC_SECOND;
					state->m_nRTC_RAM[0x1a] = RTC_MINUTE;
					state->m_nRTC_RAM[0x1b] = RTC_HOUR;
					state->m_nRTC_RAM[0x1c] = RTC_DAY;
					state->m_nRTC_RAM[0x1d] = RTC_MONTH;
				}
			}
			break;
		}
		return;
	}
	switch( offset )
	{
	case 0x0090:	// SCSI0 next descriptor pointer
		state->m_nHPC_SCSI0Descriptor = data;
		break;

	case 0x0094:	// SCSI0 control flags
		state->m_nHPC_SCSI0DMACtrl = data;
		#if 0
		if (data & 0x80)
		{
			UINT32 next;

			mame_printf_info("DMA activated for SCSI0\n");
			mame_printf_info("Descriptor block:\n");
			mame_printf_info("CTL: %08x BUFPTR: %08x DESCPTR %08x\n",
				program_read_dword(state->m_nHPC_SCSI0Descriptor), program_read_dword(state->m_nHPC_SCSI0Descriptor+4),
				program_read_dword(state->m_nHPC_SCSI0Descriptor+8));

			next = program_read_dword(state->m_nHPC_SCSI0Descriptor+8);
			mame_printf_info("CTL: %08x BUFPTR: %08x DESCPTR %08x\n",
				program_read_dword(next), program_read_dword(next+4),
				program_read_dword(next+8));
		}
		#endif
		break;

	case 0x00ac:
		verboselog(machine, 2, "HPC Parallel Buffer Pointer Write: %08x (%08x)\n", data, mem_mask );
		state->m_nHPC_ParBufPtr = data;
		break;
	case 0x0120:
		if (ACCESSING_BITS_8_15)
		{
			verboselog(machine, 2, "HPC SCSI Controller Register Write: %08x\n", ( data >> 8 ) & 0x000000ff );
			wd33c93_w( space, 0, ( data >> 8 ) & 0x000000ff );
		}
		else
		{
			return;
		}
		break;
	case 0x0124:
		if (ACCESSING_BITS_8_15)
		{
			verboselog(machine, 2, "HPC SCSI Controller Data Write: %08x\n", ( data >> 8 ) & 0x000000ff );
			wd33c93_w( space, 1, ( data >> 8 ) & 0x000000ff );
		}
		else
		{
			return;
		}
		break;
	case 0x01b0:
		verboselog(machine, 2, "HPC Misc. Status Write: %08x (%08x)\n", data, mem_mask );
		if( data & 0x00000001 )
		{
			verboselog(machine, 2, "  Force DSP hard reset\n" );
		}
		if( data & 0x00000002 )
		{
			verboselog(machine, 2, "  Force IRQA\n" );
		}
		if( data & 0x00000004 )
		{
			verboselog(machine, 2, "  Set IRQA polarity high\n" );
		}
		else
		{
			verboselog(machine, 2, "  Set IRQA polarity low\n" );
		}
		if( data & 0x00000008 )
		{
			verboselog(machine, 2, "  SRAM size: 32K\n" );
		}
		else
		{
			verboselog(machine, 2, "  SRAM size:  8K\n" );
		}
		state->m_nHPC_MiscStatus = data;
		break;
	case 0x01bc:
//      verboselog(machine, 2, "HPC CPU Serial EEPROM Write: %08x (%08x)\n", data, mem_mask );
		if( data & 0x00000001 )
		{
			verboselog(machine, 2, "    CPU board LED on\n" );
		}
		eeprom_write_bit(eeprom, (data & 0x00000008) ? 1 : 0 );
		eeprom_set_cs_line(eeprom,(data & 0x00000002) ? ASSERT_LINE : CLEAR_LINE );
		eeprom_set_clock_line(eeprom,(data & 0x00000004) ? CLEAR_LINE : ASSERT_LINE );
		break;
	case 0x01c4:
		verboselog(machine, 2, "HPC Local IO Register 0 Mask Write: %08x (%08x)\n", data, mem_mask );
		state->m_nHPC_LocalIOReg0Mask = data;
		break;
	case 0x01cc:
		verboselog(machine, 2, "HPC Local IO Register 1 Mask Write: %08x (%08x)\n", data, mem_mask );
		state->m_nHPC_LocalIOReg1Mask = data;
		break;
	case 0x01d4:
		verboselog(machine, 2, "HPC VME Interrupt Mask 0 Write: %08x (%08x)\n", data, mem_mask );
		state->m_nHPC_VMEIntMask0 = data;
		break;
	case 0x01d8:
		verboselog(machine, 2, "HPC VME Interrupt Mask 1 Write: %08x (%08x)\n", data, mem_mask );
		state->m_nHPC_VMEIntMask1 = data;
		break;
	case 0x0d00:
		verboselog(machine, 2, "HPC DUART0 Channel B Control Write: %08x (%08x)\n", data, mem_mask );
		scc = space->machine().device("scc");
		scc8530_w(scc, 0, data);
		break;
	case 0x0d04:
		verboselog(machine, 2, "HPC DUART0 Channel B Data Write: %08x (%08x)\n", data, mem_mask );
		scc = space->machine().device("scc");
		scc8530_w(scc, 2, data);
		break;
	case 0x0d08:
		verboselog(machine, 2, "HPC DUART0 Channel A Control Write: %08x (%08x)\n", data, mem_mask );
		scc = space->machine().device("scc");
		scc8530_w(scc, 1, data);
		break;
	case 0x0d0c:
		verboselog(machine, 2, "HPC DUART0 Channel A Data Write: %08x (%08x)\n", data, mem_mask );
		scc = space->machine().device("scc");
		scc8530_w(scc, 3, data);
		break;
	case 0x0d10:
		if( ( data & 0x000000ff ) >= 0x00000020 )
		{
//          verboselog(machine, 2, "HPC DUART1 Channel B Control Write: %08x (%08x) %c\n", data, mem_mask, data & 0x000000ff );
			//mame_printf_info( "%c", data & 0x000000ff );
		}
		else
		{
//          verboselog(machine, 2, "HPC DUART1 Channel B Control Write: %08x (%08x)\n", data, mem_mask );
		}
		break;
	case 0x0d14:
		if( ( data & 0x000000ff ) >= 0x00000020 || ( data & 0x000000ff ) == 0x0d || ( data & 0x000000ff ) == 0x0a )
		{
			verboselog(machine, 2, "HPC DUART1 Channel B Data Write: %08x (%08x) %c\n", data, mem_mask, data & 0x000000ff );
			mame_printf_info( "%c", data & 0x000000ff );
		}
		else
		{
			verboselog(machine, 2, "HPC DUART1 Channel B Data Write: %08x (%08x)\n", data, mem_mask );
		}
		break;
	case 0x0d18:
		mame_printf_info("HPC DUART1 Channel A Control Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d1c:
		verboselog(machine, 2, "HPC DUART1 Channel A Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d20:
		mame_printf_info("HPC DUART2 Channel B Control Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d24:
		verboselog(machine, 2, "HPC DUART2 Channel B Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d28:
		mame_printf_info("HPC DUART2 Channel A Control Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d2c:
		verboselog(machine, 2, "HPC DUART2 Channel A Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d30:
		mame_printf_info("HPC DUART3 Channel B Control Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d34:
		verboselog(machine, 2, "HPC DUART3 Channel B Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d38:
		mame_printf_info("HPC DUART3 Channel A Control Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d3c:
		verboselog(machine, 2, "HPC DUART3 Channel A Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	default:
		mame_printf_info("Unmapped HPC write: 0x%08x (%08x): %08x\n", 0x1fb80000 + offset, mem_mask, data);
		break;
	}
}

// INT/INT2/INT3 interrupt controllers
static READ32_HANDLER( int_r )
{
	mame_printf_info("INT: read @ ofs %x (mask %x) (PC=%x)\n", offset, mem_mask, cpu_get_pc(&space->device()));
	return 0;
}

static WRITE32_HANDLER( int_w )
{
	mame_printf_info("INT: write %x to ofs %x (mask %x) (PC=%x)\n", data, offset, mem_mask, cpu_get_pc(&space->device()));
}

static ADDRESS_MAP_START( ip204415_map, AS_PROGRAM, 32 )
	AM_RANGE( 0x00000000, 0x001fffff ) AM_RAM AM_SHARE("share10")
	AM_RANGE( 0x08000000, 0x08ffffff ) AM_RAM AM_SHARE("share5")
	AM_RANGE( 0x09000000, 0x097fffff ) AM_RAM AM_SHARE("share6")
	AM_RANGE( 0x0a000000, 0x0a7fffff ) AM_RAM AM_SHARE("share7")
	AM_RANGE( 0x0c000000, 0x0c7fffff ) AM_RAM AM_SHARE("share8")
	AM_RANGE( 0x10000000, 0x107fffff ) AM_RAM AM_SHARE("share9")
	AM_RANGE( 0x18000000, 0x187fffff ) AM_RAM AM_SHARE("share1")
	AM_RANGE( 0x1fa00000, 0x1fa1ffff ) AM_READWRITE( sgi_mc_r, sgi_mc_w )
	AM_RANGE( 0x1fb80000, 0x1fb8ffff ) AM_READWRITE( hpc_r, hpc_w )
	AM_RANGE( 0x1fbd9000, 0x1fbd903f ) AM_READWRITE( int_r, int_w )
	AM_RANGE( 0x1fc00000, 0x1fc7ffff ) AM_ROM AM_SHARE("share2") AM_REGION( "user1", 0 )
	AM_RANGE( 0x80000000, 0x801fffff ) AM_RAM AM_SHARE("share10")
	AM_RANGE( 0x88000000, 0x88ffffff ) AM_RAM AM_SHARE("share5")
	AM_RANGE( 0xa0000000, 0xa01fffff ) AM_RAM AM_SHARE("share10")
	AM_RANGE( 0xa8000000, 0xa8ffffff ) AM_RAM AM_SHARE("share5")
	AM_RANGE( 0xa9000000, 0xa97fffff ) AM_RAM AM_SHARE("share6")
	AM_RANGE( 0xaa000000, 0xaa7fffff ) AM_RAM AM_SHARE("share7")
	AM_RANGE( 0xac000000, 0xac7fffff ) AM_RAM AM_SHARE("share8")
	AM_RANGE( 0xb0000000, 0xb07fffff ) AM_RAM AM_SHARE("share9")
	AM_RANGE( 0xb8000000, 0xb87fffff ) AM_RAM AM_SHARE("share1")
	AM_RANGE( 0xbfa00000, 0xbfa1ffff ) AM_READWRITE( sgi_mc_r, sgi_mc_w )
	AM_RANGE( 0xbfb80000, 0xbfb8ffff ) AM_READWRITE( hpc_r, hpc_w )
	AM_RANGE( 0xbfbd9000, 0xbfbd903f ) AM_READWRITE( int_r, int_w )
	AM_RANGE( 0xbfc00000, 0xbfc7ffff ) AM_ROM AM_SHARE("share2") /* BIOS Mirror */
ADDRESS_MAP_END

static void scsi_irq(running_machine &machine, int state)
{
}

static const SCSIConfigTable dev_table =
{
        1,                                      /* 1 SCSI device */
        { { SCSI_ID_6, "cdrom", SCSI_DEVICE_CDROM } } /* SCSI ID 6, using CHD 0, and it's a CD-ROM */
};

static const struct WD33C93interface scsi_intf =
{
	&dev_table,		/* SCSI device table */
	&scsi_irq,		/* command completion IRQ */
};

static void ip204415_exit(running_machine &machine)
{
	wd33c93_exit(&scsi_intf);
}

static DRIVER_INIT( ip204415 )
{
	machine.add_notifier(MACHINE_NOTIFY_EXIT, ip204415_exit);
}

// sgi_mc_update wants once every millisecond (1/1000th of a second)
static TIMER_CALLBACK(ip20_timer)
{
	ip20_state *state = machine.driver_data<ip20_state>();
	sgi_mc_update();

	// update RTC every 10 milliseconds
	state->m_nRTC_Temp++;
	if (state->m_nRTC_Temp >= 10)
	{
		state->m_nRTC_Temp = 0;
		RTC_HUNDREDTH++;

		if( ( RTC_HUNDREDTH & 0x0f ) == 0x0a )
		{
			RTC_HUNDREDTH -= 0x0a;
			RTC_HUNDREDTH += 0x10;
			if( ( RTC_HUNDREDTH & 0xa0 ) == 0xa0 )
			{
				RTC_HUNDREDTH = 0;
				RTC_SECOND++;

				if( ( RTC_SECOND & 0x0f ) == 0x0a )
				{
					RTC_SECOND -= 0x0a;
					RTC_SECOND += 0x10;
					if( RTC_SECOND == 0x60 )
					{
						RTC_SECOND = 0;
						RTC_MINUTE++;

						if( ( RTC_MINUTE & 0x0f ) == 0x0a )
						{
							RTC_MINUTE -= 0x0a;
							RTC_MINUTE += 0x10;
							if( RTC_MINUTE == 0x60 )
							{
								RTC_MINUTE = 0;
								RTC_HOUR++;

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
						}
					}
				}
			}
		}
	}

	machine.scheduler().timer_set(attotime::from_msec(1), FUNC(ip20_timer));
}

static MACHINE_START( ip204415 )
{
	ip20_state *state = machine.driver_data<ip20_state>();
	sgi_mc_timer_init(machine);

	wd33c93_init(machine, &scsi_intf);

	sgi_mc_init(machine);

	state->m_nHPC_MiscStatus = 0;
	state->m_nHPC_ParBufPtr = 0;
	state->m_nHPC_LocalIOReg0Mask = 0;
	state->m_nHPC_LocalIOReg1Mask = 0;
	state->m_nHPC_VMEIntMask0 = 0;
	state->m_nHPC_VMEIntMask1 = 0;

	state->m_nRTC_Temp = 0;

	machine.scheduler().timer_set(attotime::from_msec(1), FUNC(ip20_timer));
}

static INPUT_PORTS_START( ip204415 )
	PORT_START("unused")
	PORT_BIT ( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

static const mips3_config config =
{
	32768,	/* code cache size */
	32768	/* data cache size */
};

static MACHINE_CONFIG_START( ip204415, ip20_state )
	MCFG_CPU_ADD( "maincpu", R4600BE, 50000000*3 )
	MCFG_CPU_CONFIG( config )
	MCFG_CPU_PROGRAM_MAP( ip204415_map)

	MCFG_MACHINE_START( ip204415 )

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE( 60 )
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(800, 600)
	MCFG_SCREEN_VISIBLE_AREA(0, 799, 0, 599)
	MCFG_SCREEN_UPDATE( ip204415 )

	MCFG_PALETTE_LENGTH(65536)

	MCFG_VIDEO_START( ip204415 )

	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD( "cdda", CDDA, 0 )
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MCFG_SCC8530_ADD("scc", 7000000)

	MCFG_CDROM_ADD( "cdrom" )

	MCFG_EEPROM_ADD("eeprom", eeprom_interface_93C56)
MACHINE_CONFIG_END

ROM_START( ip204415 )
	ROM_REGION( 0x80000, "user1", 0 )
	ROM_LOAD( "ip204415.bin", 0x000000, 0x080000, CRC(940d960e) SHA1(596aba530b53a147985ff3f6f853471ce48c866c) )
ROM_END

/*    YEAR  NAME      PARENT    COMPAT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
COMP( 1993, ip204415, 0,        0,        ip204415, ip204415, ip204415, "Silicon Graphics Inc", "IRIS Indigo (R4400, 150MHz)", GAME_NOT_WORKING | GAME_NO_SOUND )
