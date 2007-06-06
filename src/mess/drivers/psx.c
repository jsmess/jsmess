/***************************************************************************

  Sony Playstaion
  ========-------
  Preliminary driver by smf

  todo:
    add memcard support
    tidy up cd controller
    work out why bios schph1000 doesn't get past the first screen
    add cd image support

***************************************************************************/

#include "driver.h"
#include "cpu/mips/psx.h"
#include "devices/snapquik.h"
#include "includes/psx.h"
#include "sound/psx.h"
#include "debugger.h"

struct
{
	UINT8 id[ 8 ];
	UINT32 text;	/* SCE only */
	UINT32 data;	/* SCE only */
	UINT32 pc0;
	UINT32 gp0;		/* SCE only */
	UINT32 t_addr;
	UINT32 t_size;
	UINT32 d_addr;	/* SCE only */
	UINT32 d_size;	/* SCE only */
	UINT32 b_addr;	/* SCE only */
	UINT32 b_size;	/* SCE only */
	UINT32 s_addr;
	UINT32 s_size;
	UINT32 SavedSP;
	UINT32 SavedFP;
	UINT32 SavedGP;
	UINT32 SavedRA;
	UINT32 SavedS0;
	UINT8 dummy[ 0x800 - 76 ];
} m_psxexe_header;

static UINT8 *m_p_psxexe;

static OPBASE_HANDLER( psx_setopbase )
{
	if( address == 0x80030000 )
	{
		UINT8 *p_ram;
		UINT8 *p_psxexe;
		UINT32 n_stack;
		UINT32 n_ram;
		UINT32 n_left;
		UINT32 n_address;

		logerror( "psx_exe_load: pc  %08x\n", m_psxexe_header.pc0 );
		logerror( "psx_exe_load: org %08x\n", m_psxexe_header.t_addr );
		logerror( "psx_exe_load: len %08x\n", m_psxexe_header.t_size );
		logerror( "psx_exe_load: sp  %08x\n", m_psxexe_header.s_addr );
		logerror( "psx_exe_load: len %08x\n", m_psxexe_header.s_size );

		p_ram = (UINT8 *)g_p_n_psxram;
		n_ram = g_n_psxramsize;

		p_psxexe = m_p_psxexe;

		n_address = m_psxexe_header.t_addr;
		n_left = m_psxexe_header.t_size;
		while( n_left != 0 )
		{
			p_ram[ BYTE4_XOR_LE( n_address ) % n_ram ] = *( p_psxexe );
			n_address++;
			p_psxexe++;
			n_left--;
		}

		free( m_p_psxexe );

		activecpu_set_reg( MIPS_PC, m_psxexe_header.pc0 );
		activecpu_set_reg( MIPS_R28, m_psxexe_header.gp0 );
		n_stack = m_psxexe_header.s_addr + m_psxexe_header.s_size;
		if( n_stack != 0 )
		{
			activecpu_set_reg( MIPS_R29, n_stack );
			activecpu_set_reg( MIPS_R30, n_stack );
		}

		memory_set_opbase_handler( 0, NULL );
#ifdef MAME_DEBUG
		DEBUGGER_BREAK;
		CALL_MAME_DEBUG;
#endif
		return ~0;
	}
	return address;
}

static void psxexe_conv32( UINT32 *p_uint32 )
{
	UINT8 *p_uint8;

	p_uint8 = (UINT8 *)p_uint32;

	*( p_uint32 ) = p_uint8[ 0 ] |
		( p_uint8[ 1 ] << 8 ) |
		( p_uint8[ 2 ] << 16 ) |
		( p_uint8[ 3 ] << 24 );
}

static QUICKLOAD_LOAD( psx_exe_load )
{
	if( image_fread( image, &m_psxexe_header, sizeof( m_psxexe_header ) ) != sizeof( m_psxexe_header ) )
	{
		logerror( "psx_exe_load: invalid exe\n" );
		return INIT_FAIL;
	}
	if( memcmp( m_psxexe_header.id, "PS-X EXE", 8 ) != 0 )
	{
		logerror( "psx_exe_load: invalid header id\n" );
		return INIT_FAIL;
	}

	psxexe_conv32( &m_psxexe_header.text );
	psxexe_conv32( &m_psxexe_header.data );
	psxexe_conv32( &m_psxexe_header.pc0 );
	psxexe_conv32( &m_psxexe_header.gp0 );
	psxexe_conv32( &m_psxexe_header.t_addr );
	psxexe_conv32( &m_psxexe_header.t_size );
	psxexe_conv32( &m_psxexe_header.d_addr );
	psxexe_conv32( &m_psxexe_header.d_size );
	psxexe_conv32( &m_psxexe_header.b_addr );
	psxexe_conv32( &m_psxexe_header.b_size );
	psxexe_conv32( &m_psxexe_header.s_addr );
	psxexe_conv32( &m_psxexe_header.s_size );
	psxexe_conv32( &m_psxexe_header.SavedSP );
	psxexe_conv32( &m_psxexe_header.SavedFP );
	psxexe_conv32( &m_psxexe_header.SavedGP );
	psxexe_conv32( &m_psxexe_header.SavedRA );
	psxexe_conv32( &m_psxexe_header.SavedS0 );

	m_p_psxexe = malloc( m_psxexe_header.t_size );
	if( m_p_psxexe == NULL )
	{
		logerror( "psx_exe_load: out of memory\n" );
		return INIT_FAIL;
	}
	image_fread( image, m_p_psxexe, m_psxexe_header.t_size );
	memory_set_opbase_handler( 0, psx_setopbase );
	return INIT_PASS;
}

#define PAD_STATE_IDLE ( 0 )
#define PAD_STATE_LISTEN ( 1 )
#define PAD_STATE_ACTIVE ( 2 )
#define PAD_STATE_READ ( 3 )
#define PAD_STATE_UNLISTEN ( 4 )

#define PAD_TYPE_STANDARD ( 4 )
#define PAD_BYTES_STANDARD ( 2 )

#define PAD_CMD_START ( 0x01 )
#define PAD_CMD_READ  ( 0x42 ) /* B */

#define PAD_DATA_OK   ( 0x5a ) /* Z */
#define PAD_DATA_IDLE ( 0xff )

static struct
{
	int n_shiftin;
	int n_shiftout;
	int n_bits;
	int n_state;
	int n_byte;
	int b_lastclock;
	int b_ack;
} m_pad[ 2 ];

static void psx_pad_ack( int n_port )
{
	if( m_pad[ n_port ].n_state != PAD_STATE_IDLE )
	{
		psx_sio_input( 0, PSX_SIO_IN_DSR, m_pad[ n_port ].b_ack * PSX_SIO_IN_DSR );
		if( !m_pad[ n_port ].b_ack )
		{
			m_pad[ n_port ].b_ack = 1;
			timer_set( TIME_IN_USEC( 2 ), n_port, psx_pad_ack );
		}
	}
}

static void psx_pad( int n_port, int n_data )
{
	int b_sel;
	int b_clock;
	int b_data;
	int b_ack;
	int b_ready;

	b_sel = ( n_data & PSX_SIO_OUT_DTR ) / PSX_SIO_OUT_DTR;
	b_clock = ( n_data & PSX_SIO_OUT_CLOCK ) / PSX_SIO_OUT_CLOCK;
	b_data = ( n_data & PSX_SIO_OUT_DATA ) / PSX_SIO_OUT_DATA;
	b_ready = 0;
	b_ack = 0;

	if( b_sel )
	{
		m_pad[ n_port ].n_state = PAD_STATE_IDLE;
	}

	switch( m_pad[ n_port ].n_state )
	{
	case PAD_STATE_LISTEN:
	case PAD_STATE_ACTIVE:
	case PAD_STATE_READ:
		if( m_pad[ n_port ].b_lastclock && !b_clock )
		{
			psx_sio_input( 0, PSX_SIO_IN_DATA, ( m_pad[ n_port ].n_shiftout & 1 ) * PSX_SIO_IN_DATA );
			m_pad[ n_port ].n_shiftout >>= 1;
		}
		if( !m_pad[ n_port ].b_lastclock && b_clock )
		{
			m_pad[ n_port ].n_shiftin >>= 1;
			m_pad[ n_port ].n_shiftin |= b_data << 7;
			m_pad[ n_port ].n_bits++;

			if( m_pad[ n_port ].n_bits == 8 )
			{
				m_pad[ n_port ].n_bits = 0;
				b_ready = 1;
			}
		}
		break;
	}

	m_pad[ n_port ].b_lastclock = b_clock;

	switch( m_pad[ n_port ].n_state )
	{
	case PAD_STATE_IDLE:
		if( !b_sel )
		{
			m_pad[ n_port ].n_state = PAD_STATE_LISTEN;
			m_pad[ n_port ].n_shiftout = PAD_DATA_IDLE;
			m_pad[ n_port ].n_bits = 0;
		}
		break;
	case PAD_STATE_LISTEN:
		if( b_ready )
		{
			if( m_pad[ n_port ].n_shiftin == PAD_CMD_START )
			{
				m_pad[ n_port ].n_state = PAD_STATE_ACTIVE;
				m_pad[ n_port ].n_shiftout = ( PAD_TYPE_STANDARD << 4 ) | ( PAD_BYTES_STANDARD >> 1 );
				b_ack = 1;
			}
			else
			{
				m_pad[ n_port ].n_state = PAD_STATE_UNLISTEN;
			}
		}
		break;
	case PAD_STATE_ACTIVE:
		if( b_ready )
		{
			if( m_pad[ n_port ].n_shiftin == PAD_CMD_READ )
			{
				m_pad[ n_port ].n_state = PAD_STATE_READ;
				m_pad[ n_port ].n_shiftout = PAD_DATA_OK;
				m_pad[ n_port ].n_byte = 0;
				b_ack = 1;
			}
			else
			{
				logerror( "unknown pad command %02x\n", m_pad[ n_port ].n_shiftin );
				m_pad[ n_port ].n_state = PAD_STATE_UNLISTEN;
			}
		}
		break;
	case PAD_STATE_READ:
		if( b_ready )
		{
			if( m_pad[ n_port ].n_byte < PAD_BYTES_STANDARD )
			{
				m_pad[ n_port ].n_shiftout = readinputport( m_pad[ n_port ].n_byte + ( n_port * PAD_BYTES_STANDARD ) );
				m_pad[ n_port ].n_byte++;
				b_ack = 1;
			}
			else
			{
				m_pad[ n_port ].n_state = PAD_STATE_LISTEN;
			}
		}
		break;
	}

	if( b_ack )
	{
		m_pad[ n_port ].b_ack = 0;
		timer_set( TIME_IN_USEC( 10 ), n_port, psx_pad_ack );
	}
}

static void psx_mcd( int n_port, int n_data )
{
	/* todo */
}

static void psx_sio0( int n_data )
{
	/* todo: raise data & ack when nothing is driving it low */
	psx_pad( 0, n_data );
	psx_mcd( 0, n_data );
	psx_pad( 1, n_data ^ PSX_SIO_OUT_DTR );
	psx_mcd( 1, n_data ^ PSX_SIO_OUT_DTR );
}

/* -----------------------------------------------------------------------
 * cd_stat values
 *
 * NoIntr       0x00 No interrupt
 * DataReady    0x01 Data Ready
 * Acknowledge  0x02 Command Complete
 * Complete     0x03 Acknowledge
 * DataEnd      0x04 End of Data Detected
 * DiskError    0x05 Error Detected
 * ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */

static int cd_param_p;
static int cd_result_p;
static int cd_result_c;
static int cd_result_ready;
static int cd_readed = -1;
static int cd_reset;
static UINT8 cd_stat;
static UINT8 cd_io_status;
static UINT8 cd_param[8];
static UINT8 cd_result[8];

/* ----------------------------------------------------------------------- */

static void psx_cdcmd_sync(void)
{
	/* NYI */
}

static void psx_cdcmd_nop(void)
{
	/* NYI */
}

static void psx_cdcmd_setloc(void)
{
	/* NYI */
}

static void psx_cdcmd_play(void)
{
	/* NYI */
}

static void psx_cdcmd_forward(void)
{
	/* NYI */
}

static void psx_cdcmd_backward(void)
{
	/* NYI */
}

static void psx_cdcmd_readn(void)
{
	/* NYI */
}

static void psx_cdcmd_standby(void)
{
	/* NYI */
}

static void psx_cdcmd_stop(void)
{
	/* NYI */
}

static void psx_cdcmd_pause(void)
{
	/* NYI */
}

static void psx_cdcmd_init(void)
{
	cd_result_p = 0;
	cd_result_c = 1;
	cd_stat = 0x02;
	cd_result[0] = cd_stat;
	/* NYI */
}

static void psx_cdcmd_mute(void)
{
	/* NYI */
}

static void psx_cdcmd_demute(void)
{
	/* NYI */
}

static void psx_cdcmd_setfilter(void)
{
	/* NYI */
}

static void psx_cdcmd_setmode(void)
{
	/* NYI */
}

static void psx_cdcmd_getparam(void)
{
	/* NYI */
}

static void psx_cdcmd_getlocl(void)
{
	/* NYI */
}

static void psx_cdcmd_getlocp(void)
{
	/* NYI */
}

static void psx_cdcmd_gettn(void)
{
	/* NYI */
}

static void psx_cdcmd_gettd(void)
{
	/* NYI */
}

static void psx_cdcmd_seekl(void)
{
	/* NYI */
}

static void psx_cdcmd_seekp(void)
{
	/* NYI */
}

static void psx_cdcmd_test(void)
{
	static const UINT8 test20[] = { 0x98, 0x06, 0x10, 0xC3 };
	static const UINT8 test22[] = { 0x66, 0x6F, 0x72, 0x20, 0x45, 0x75, 0x72, 0x6F };
	static const UINT8 test23[] = { 0x43, 0x58, 0x44, 0x32, 0x39 ,0x34, 0x30, 0x51 };

	switch(cd_param[0]) {
	case 0x04:	/* read SCEx counter (returned in 1st byte?) */
		break;
	case 0x05:	/* reset SCEx counter. */
		break;
	case 0x20:
		memcpy(cd_result, test20, sizeof(test20));
		cd_result_p = 0;
		cd_result_c = sizeof(test20);
		break;
	case 0x22:
		memcpy(cd_result, test22, sizeof(test22));
		cd_result_p = 0;
		cd_result_c = sizeof(test22);
		break;
	case 0x23:
		memcpy(cd_result, test23, sizeof(test23));
		cd_result_p = 0;
		cd_result_c = sizeof(test23);
		break;
	}
	cd_stat = 3;
}

static void psx_cdcmd_id(void)
{
	/* NYI */
}

static void psx_cdcmd_reads(void)
{
	/* NYI */
}

static void psx_cdcmd_reset(void)
{
	/* NYI */
}

static void psx_cdcmd_readtoc(void)
{
	/* NYI */
}

static void (*psx_cdcmds[])(void) =
{
	psx_cdcmd_sync,
	psx_cdcmd_nop,
	psx_cdcmd_setloc,
	psx_cdcmd_play,
	psx_cdcmd_forward,
	psx_cdcmd_backward,
	psx_cdcmd_readn,
	psx_cdcmd_standby,
	psx_cdcmd_stop,
	psx_cdcmd_pause,
	psx_cdcmd_init,
	psx_cdcmd_mute,
	psx_cdcmd_demute,
	psx_cdcmd_setfilter,
	psx_cdcmd_setmode,
	psx_cdcmd_getparam,
	psx_cdcmd_getlocl,
	psx_cdcmd_getlocp,
	psx_cdcmd_gettn,
	psx_cdcmd_gettd,
	psx_cdcmd_seekl,
	psx_cdcmd_seekp,
	NULL,
	NULL,
	NULL,
	psx_cdcmd_test,
	psx_cdcmd_id,
	psx_cdcmd_reads,
	psx_cdcmd_reset,
	NULL,
	psx_cdcmd_readtoc
};

/* ----------------------------------------------------------------------- */

READ32_HANDLER( psx_cd_r )
{
	UINT32 result = 0;

	if( mem_mask == 0xffffff00 )
	{
		logerror( "%08x cd0 read\n", activecpu_get_pc() );

		if (cd_result_ready && cd_result_c)
			cd_io_status |= 0x20;
		else
			cd_io_status &= ~0x20;

		if (cd_readed == 0)
			result = cd_io_status | 0x40;
		else
			result = cd_io_status;

		/* NPW 21-May-2003 - Seems to expect this on boot */
		result |= 0x0f;
	}
	else if( mem_mask == 0xffff00ff )
	{
		logerror( "%08x cd1 read\n", activecpu_get_pc() );

		if (cd_result_ready && cd_result_c)
			result = ((UINT32) cd_result[cd_result_p]) << 8;

		if (cd_result_ready)
		{
			cd_result_p++;
			if (cd_result_p == cd_result_c)
				cd_result_ready = 0;
		}
	}
	else if( mem_mask == 0xff00ffff )
	{
		logerror( "%08x cd2 read\n", activecpu_get_pc() );
	}
	else if( mem_mask == 0x00ffffff )
	{
		logerror( "%08x cd3 read\n", activecpu_get_pc() );

		result = ((UINT32) cd_stat) << 24;
	}
	else
	{
		logerror( "%08x cd_r( %08x, %08x ) unsupported transfer\n", activecpu_get_pc(), offset, mem_mask );
	}
	return result;
}

WRITE32_HANDLER( psx_cd_w )
{
	void (*psx_cdcmd)(void);

	if( mem_mask == 0xffffff00 )
	{
		/* write to CD register 0 */
		data = (data >> 0) & 0xff;
		logerror( "%08x cd0 write %02x\n", activecpu_get_pc(), data & 0xff );

		cd_param_p = 0;
		if (data == 0x00)
		{
			cd_result_ready = 0;
		}
		else
		{
			if (data & 0x01)
				cd_result_ready = 1;
			if (data == 0x01)
				cd_reset = 1;
		}
	}
	else if( mem_mask == 0xffff00ff )
	{
		/* write to CD register 1 */
		data = (data >> 8) & 0xff;
		logerror( "%08x cd1 write %02x\n", activecpu_get_pc(), data );

		if (data <= sizeof(psx_cdcmds) / sizeof(psx_cdcmds[0]))
			psx_cdcmd = psx_cdcmds[data];
		else
			psx_cdcmd = NULL;

		if (psx_cdcmd)
			psx_cdcmd();

		psx_irq_set(0x0004);
	}
	else if( mem_mask == 0xff00ffff )
	{
		/* write to CD register 2 */
		data = (data >> 16) & 0xff;
		logerror( "%08x cd2 write %02x\n", activecpu_get_pc(), data & 0xff );

		if ((cd_reset == 2) && (data == 0x07))
		{
			/* copied from SOPE; this code is weird */
			cd_param_p = 0;
			cd_result_ready = 1;
			cd_reset = 0;
			cd_stat = 0;
			cd_io_status = 0x00;
			if (cd_result_ready && cd_result_c)
				cd_io_status |= 0x20;
			else
				cd_io_status &= ~0x20;
		}
		else
		{
			/* used for sending arguments */
			cd_reset = 0;
			cd_param[cd_param_p] = data;
			if (cd_param_p < 7)
				cd_param_p++;
		}
	}
	else if( mem_mask == 0x00ffffff )
	{
		/* write to CD register 3 */
		data = (data >> 24) & 0xff;
		logerror( "%08x cd3 write %02x\n", activecpu_get_pc(), data & 0xff );

		if ((data == 0x07) && (cd_reset == 1))
		{
			cd_reset = 2;
/*			psxirq_clear(0x0004); */
		}
		else
		{
			cd_reset = 0;
		}
	}
	else
	{
		logerror( "%08x cd_w( %08x, %08x, %08x ) unsupported transfer\n", activecpu_get_pc(), offset, data, mem_mask );
	}
}

static ADDRESS_MAP_START( psx_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM	AM_SHARE(1) AM_BASE(&g_p_n_psxram) AM_SIZE(&g_n_psxramsize) /* ram */
	AM_RANGE(0x1f800000, 0x1f8003ff) AM_RAM /* scratchpad */
	AM_RANGE(0x1f801000, 0x1f801007) AM_WRITENOP
	AM_RANGE(0x1f801008, 0x1f80100b) AM_RAM /* ?? */
	AM_RANGE(0x1f80100c, 0x1f80102f) AM_WRITENOP
	AM_RANGE(0x1f801010, 0x1f801013) AM_READNOP
	AM_RANGE(0x1f801014, 0x1f801017) AM_READ(psx_spu_delay_r)
	AM_RANGE(0x1f801040, 0x1f80105f) AM_READWRITE(psx_sio_r, psx_sio_w)
	AM_RANGE(0x1f801060, 0x1f80106f) AM_WRITENOP
	AM_RANGE(0x1f801070, 0x1f801077) AM_READWRITE(psx_irq_r, psx_irq_w)
	AM_RANGE(0x1f801080, 0x1f8010ff) AM_READWRITE(psx_dma_r, psx_dma_w)
	AM_RANGE(0x1f801100, 0x1f80113f) AM_READWRITE(psx_counter_r, psx_counter_w)
	AM_RANGE(0x1f801800, 0x1f801803) AM_READWRITE(psx_cd_r, psx_cd_w)
	AM_RANGE(0x1f801810, 0x1f801817) AM_READWRITE(psx_gpu_r, psx_gpu_w)
	AM_RANGE(0x1f801820, 0x1f801827) AM_READWRITE(psx_mdec_r, psx_mdec_w)
	AM_RANGE(0x1f801c00, 0x1f801dff) AM_READWRITE(psx_spu_r, psx_spu_w)
	AM_RANGE(0x1f802020, 0x1f802033) AM_RAM /* ?? */
	AM_RANGE(0x1f802040, 0x1f802043) AM_WRITENOP
	AM_RANGE(0x1fc00000, 0x1fc7ffff) AM_ROM AM_SHARE(2) AM_REGION(REGION_USER1, 0) /* bios */
	AM_RANGE(0x80000000, 0x801fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0x80600000, 0x807fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0x9fc00000, 0x9fc7ffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xa0000000, 0xa01fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0xbfc00000, 0xbfc7ffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xfffe0130, 0xfffe0133) AM_WRITENOP
ADDRESS_MAP_END

static MACHINE_RESET( psx )
{
	psx_machine_init();
	psx_sio_install_handler( 0, psx_sio0 );
}

static DRIVER_INIT( psx )
{
	psx_driver_init();
}

INPUT_PORTS_START( psx )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )	PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START )  PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_PLAYER(1)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SELECT ) PORT_PLAYER(1)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("P1 Square")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("P1 Cross")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_NAME("P1 Circle")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) PORT_NAME("P1 Triangle")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1) PORT_NAME("P1 R1")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1) PORT_NAME("P1 L1")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_PLAYER(1) PORT_NAME("P1 R2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_PLAYER(1) PORT_NAME("P1 L2")

	PORT_START_TAG("IN2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )	PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START )  PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_PLAYER(2)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SELECT ) PORT_PLAYER(2)

	PORT_START_TAG("IN3")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 Square")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 Cross")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) PORT_NAME("P2 Circle")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2) PORT_NAME("P2 Triangle")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2) PORT_NAME("P2 R1")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2) PORT_NAME("P2 L1")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_PLAYER(2) PORT_NAME("P2 R2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_PLAYER(2) PORT_NAME("P2 L2")
INPUT_PORTS_END

static struct PSXSPUinterface psxspu_interface =
{
	&g_p_n_psxram,
	psx_irq_set,
	psx_dma_install_read_handler,
	psx_dma_install_write_handler
};

static MACHINE_DRIVER_START( psxntsc )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( psx_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_SCREEN_REFRESH_RATE( 60 )
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC( 0 ))

	MDRV_MACHINE_RESET( psx )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE( 1024, 512 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 639, 0, 479 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD( PSXSPU, 0 )
	MDRV_SOUND_CONFIG( psxspu_interface )
	MDRV_SOUND_ROUTE( 0, "left", 1.00 )
	MDRV_SOUND_ROUTE( 1, "right", 1.00 )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( psxpal )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( psx_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_SCREEN_REFRESH_RATE( 50 )
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC( 0 ))

	MDRV_MACHINE_RESET( psx )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE( 1024, 512 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 639, 0, 511 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD( PSXSPU, 0 )
	MDRV_SOUND_CONFIG( psxspu_interface )
	MDRV_SOUND_ROUTE( 0, "left", 1.00 )
	MDRV_SOUND_ROUTE( 1, "right", 1.00 )
MACHINE_DRIVER_END

SYSTEM_BIOS_START( psxj )
	SYSTEM_BIOS_ADD( 0, "scph1000", "SCPH-1000 (09/22/94)" )
	SYSTEM_BIOS_ADD( 1, "scph3000", "SCPH-3000 (v1.1 01/22/95)" )
	SYSTEM_BIOS_ADD( 2, "scph3500", "SCPH-3500 (v2.1 07/17/95)" )
	SYSTEM_BIOS_ADD( 3, "scph5000", "SCPH-5000 (v2.2 12/04/95)" )
	SYSTEM_BIOS_ADD( 4, "scph5500", "SCPH-5500 (v3.0 09/09/96)" )
	SYSTEM_BIOS_ADD( 5, "scph7000", "SCPH-7000 (v4.0 08/18/97)" )
	SYSTEM_BIOS_ADD( 6, "scph7500", "SCPH-7500" )
	SYSTEM_BIOS_ADD( 7, "scph9000", "SCPH-9000" )
	SYSTEM_BIOS_ADD( 8, "scph100",  "SCPH-100 (PS one)" )
SYSTEM_BIOS_END

SYSTEM_BIOS_START( psxu )
	SYSTEM_BIOS_ADD( 0, "scph1001", "SCPH-1001/DTLH-3001 (v2.2 12/04/95)" )
	SYSTEM_BIOS_ADD( 1, "scph5001", "SCPH-5001" )
	SYSTEM_BIOS_ADD( 2, "scph5501", "SCPH-5501 (v3.0 11/18/96)" )
	SYSTEM_BIOS_ADD( 3, "scph7001", "SCPH-7001 (v4.1 12/16/97)" )
	SYSTEM_BIOS_ADD( 4, "scph7501", "SCPH-7501" )
	SYSTEM_BIOS_ADD( 5, "scph9001", "SCPH-9001" )
	SYSTEM_BIOS_ADD( 6, "scph101",  "SCPH-101 (v4.5 05/25/00) (PS one)" )
SYSTEM_BIOS_END

SYSTEM_BIOS_START( psxe )
	SYSTEM_BIOS_ADD( 0, "scph1002", "SCPH-1002 (v2.0 05/10/95)" )
	SYSTEM_BIOS_ADD( 1, "dtlh3002", "DTLH-3002 (v2.2 12/04/95)" )
	SYSTEM_BIOS_ADD( 2, "scph5002", "SCPH-5002" )
	SYSTEM_BIOS_ADD( 3, "scph5502", "SCPH-5502/SCPH-5552 (v3.0 01/06/97)" )
	SYSTEM_BIOS_ADD( 4, "scph7002", "SCPH-7002" )
	SYSTEM_BIOS_ADD( 5, "scph7502", "SCPH-7502 (v4.1 12/16/97)" )
	SYSTEM_BIOS_ADD( 6, "scph9002", "SCPH-9002" )
	SYSTEM_BIOS_ADD( 7, "scph102",  "SCPH-102 (PS one)" )
SYSTEM_BIOS_END

SYSTEM_BIOS_START( psxa )
	SYSTEM_BIOS_ADD( 0, "scph5003", "SCPH-5003" )
	SYSTEM_BIOS_ADD( 1, "scph5503", "SCPH-5503" )
	SYSTEM_BIOS_ADD( 2, "scph5903", "SCPH-5903" )
	SYSTEM_BIOS_ADD( 3, "scph7003", "SCPH-7003" )
	SYSTEM_BIOS_ADD( 4, "scph7503", "SCPH-7503" )
	SYSTEM_BIOS_ADD( 5, "scph9003", "SCPH-9003" )
	SYSTEM_BIOS_ADD( 6, "scph103",  "SCPH-103 (PS one)" )
SYSTEM_BIOS_END

ROM_START( psxj )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROMX_LOAD( "scph1000.bin", 0x0000000, 0x080000, CRC(3b601fc8) SHA1(343883a7b555646da8cee54aadd2795b6e7dd070), ROM_BIOS(1) )
	ROMX_LOAD( "scph3000.bin", 0x0000000, 0x080000, CRC(3539def6) SHA1(b06f4a861f74270be819aa2a07db8d0563a7cc4e), ROM_BIOS(2) )
	ROMX_LOAD( "scph3500.bin", 0x0000000, 0x080000, CRC(bc190209) SHA1(e38466a4ba8005fba7e9e3c7b9efeba7205bee3f), ROM_BIOS(3) )
	ROMX_LOAD( "scph5000.bin", 0x0000000, 0x080000, CRC(24fc7e17) SHA1(ffa7f9a7fb19d773a0c3985a541c8e5623d2c30d), ROM_BIOS(4) )
	ROMX_LOAD( "scph5500.bin", 0x0000000, 0x080000, CRC(ff3eeb8c) SHA1(b05def971d8ec59f346f2d9ac21fb742e3eb6917), ROM_BIOS(5) )
	ROMX_LOAD( "scph7000.bin", 0x0000000, 0x080000, CRC(ec541cd0) SHA1(77b10118d21ac7ffa9b35f9c4fd814da240eb3e9), ROM_BIOS(6) )
	ROMX_LOAD( "scph7500.bin", 0x0000000, 0x080000, NO_DUMP, ROM_BIOS(7) )
	ROMX_LOAD( "scph9000.bin", 0x0000000, 0x080000, NO_DUMP, ROM_BIOS(8) )
	ROMX_LOAD( "scph100.bin",  0x0000000, 0x080000, NO_DUMP, ROM_BIOS(9) )
ROM_END

ROM_START( psxu )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROMX_LOAD( "scph1001.bin",  0x0000000, 0x080000, CRC(37157331) SHA1(10155d8d6e6e832d6ea66db9bc098321fb5e8ebf), ROM_BIOS(1) )
	ROMX_LOAD( "scph5001.bin",  0x0000000, 0x080000, NO_DUMP, ROM_BIOS(2) )
	ROMX_LOAD( "scph5501.bin",  0x0000000, 0x080000, CRC(8d8cb7e4) SHA1(0555c6fae8906f3f09baf5988f00e55f88e9f30b), ROM_BIOS(3) ) // was 7003
	ROMX_LOAD( "scph7001.bin",  0x0000000, 0x080000, CRC(502224b6) SHA1(14df4f6c1e367ce097c11deae21566b4fe5647a9), ROM_BIOS(4) )
	ROMX_LOAD( "scph7501.bin",  0x0000000, 0x080000, NO_DUMP, ROM_BIOS(5) )
	ROMX_LOAD( "scph9001.bin",  0x0000000, 0x080000, NO_DUMP, ROM_BIOS(6) )
	ROMX_LOAD( "scph101.bin",   0x0000000, 0x080000, CRC(171bdcec) SHA1(dcffe16bd90a723499ad46c641424981338d8378), ROM_BIOS(7) )
ROM_END

ROM_START( psxe )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROMX_LOAD( "scph1002.bin",  0x0000000, 0x080000, CRC(9bb87c4b) SHA1(20b98f3d80f11cbf5a7bfd0779b0e63760ecc62c), ROM_BIOS(1) )
	ROMX_LOAD( "dtlh3002.bin",  0x0000000, 0x080000, CRC(1e26792f) SHA1(b6a11579caef3875504fcf3831b8e3922746df2c), ROM_BIOS(2) )
	ROMX_LOAD( "scph5002.bin",  0x0000000, 0x080000, NO_DUMP, ROM_BIOS(3) )
	ROMX_LOAD( "scph5552.bin",  0x0000000, 0x080000, CRC(d786f0b9) SHA1(f6bc2d1f5eb6593de7d089c425ac681d6fffd3f0), ROM_BIOS(4) )
	ROMX_LOAD( "scph7002.bin",  0x0000000, 0x080000, NO_DUMP, ROM_BIOS(5) )
	ROMX_LOAD( "scph7502.bin",  0x0000000, 0x080000, CRC(318178bf) SHA1(8d5de56a79954f29e9006929ba3fed9b6a418c1d), ROM_BIOS(6) )
	ROMX_LOAD( "scph9002.bin",  0x0000000, 0x080000, NO_DUMP, ROM_BIOS(7) )
	ROMX_LOAD( "scph102.bin",   0x0000000, 0x080000, NO_DUMP, ROM_BIOS(8) )
ROM_END

ROM_START( psxa )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROMX_LOAD( "scph5003.bin",  0x0000000, 0x080000, NO_DUMP, ROM_BIOS(1) )
	ROMX_LOAD( "scph5503.bin",  0x0000000, 0x080000, NO_DUMP, ROM_BIOS(2) )
	ROMX_LOAD( "scph5903.bin",  0x0000000, 0x080000, NO_DUMP, ROM_BIOS(3) )
	ROMX_LOAD( "scph7003.bin",  0x0000000, 0x080000, NO_DUMP, ROM_BIOS(4) )
	ROMX_LOAD( "scph7503.bin",  0x0000000, 0x080000, NO_DUMP, ROM_BIOS(5) )
	ROMX_LOAD( "scph9003.bin",  0x0000000, 0x080000, NO_DUMP, ROM_BIOS(6) )
	ROMX_LOAD( "scph103.bin",   0x0000000, 0x080000, NO_DUMP, ROM_BIOS(7) )
ROM_END

static void psx_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* quickload */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "exe,psx"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_psx_exe_load; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( psx )
	CONFIG_DEVICE(psx_quickload_getinfo)
SYSTEM_CONFIG_END

/*
The version number & release date is stored in ascii text at the end of every bios, except for scph1000.
There is also a BCD encoded date at offset 0x100, but this is set to 041211995 in every version apart
from scph1000 & scph7000 ( where it is 22091994 & 29051997 respectively ).

missing:
 scph5501
 scph7002
 scph7501
 scph9000
 scph9003
 scph100
 scph102

*/

/*     YEAR  NAME    PARENT  BIOS	COMPAT  MACHINE  INPUT  INIT  CONFIG  COMPANY   FULLNAME */
/*
// PU-7/PU-8
CONS( 1994, psx,    0,      0,      psxntsc, psx,   psx,  psx,    "Sony",   "Sony Playstation (scph1000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONS( 1995, psxj11, psx,    0,      psxntsc, psx,   psx,  psx,    "Sony",   "Sony Playstation (scph3000 J v1.1 01/22/95)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONS( 1995, psxe20, psx,    0,      psxpal,  psx,   psx,  psx,    "Sony",   "Sony Playstation (scph1002 E v2.0 05/10/95)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONS( 1995, psxj21, psx,    0,      psxntsc, psx,   psx,  psx,    "Sony",   "Sony Playstation (scph3500 J v2.1 07/17/95)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONS( 1995, psxa22, psx,    0,      psxntsc, psx,   psx,  psx,    "Sony",   "Sony Playstation (scph1001/dtlh3000 A v2.2 12/04/95)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONS( 1995, psxe22, psx,    0,      psxpal,  psx,   psx,  psx,    "Sony",   "Sony Playstation (scph1002/dtlh3002 E v2.2 12/04/95)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
// PU-18
CONS( 1995, psxj22, psx,    0,      psxntsc, psx,   psx,  psx,    "Sony",   "Sony Playstation (scph5000 J v2.2 12/04/95)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONS( 1996, psxj30, psx,    0,      psxntsc, psx,   psx,  psx,    "Sony",   "Sony Playstation (scph5500 J v3.0 09/09/96)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONS( 1997, psxe30, psx,    0,      psxpal,  psx,   psx,  psx,    "Sony",   "Sony Playstation (scph5502/scph5552 E v3.0 01/06/97)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
// PU-20
CONS( 1996, psxa30, psx,    0,      psxpal,  psx,   psx,  psx,    "Sony",   "Sony Playstation (scph7003 A v3.0 11/18/96)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONS( 1997, psxj40, psx,    0,      psxntsc, psx,   psx,  psx,    "Sony",   "Sony Playstation (scph7000 J v4.0 08/18/97)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONS( 1997, psxa41, psx,    0,      psxntsc, psx,   psx,  psx,    "Sony",   "Sony Playstation (scph7001 A v4.1 12/16/97)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
// PU-22
CONS( 1997, psxe41, psx,    0,      psxpal,  psx,   psx,  psx,    "Sony",   "Sony Playstation (scph7502 E v4.1 12/16/97)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
// PU-23
// PM-41
CONS( 2000, psxa45, psx,    0,      psxntsc, psx,   psx,  psx,    "Sony",   "Sony PS one (scph101 A v4.5 05/25/00)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
*/

CONSB( 1994, psxj,   0,      psxj,	0,      psxntsc, psx,   psx,  psx,    "Sony Computer Entertainment", "Sony PlayStation (Japan)",	    GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONSB( 1995, psxe,   psxj,   psxe,	0,      psxpal,  psx,   psx,  psx,    "Sony Computer Entertainment", "Sony PlayStation (Europe)",	    GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONSB( 1995, psxu,   psxj,   psxu,	0,      psxntsc, psx,   psx,  psx,    "Sony Computer Entertainment", "Sony PlayStation (USA)",		    GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONSB( 1995, psxa,   psxj,   psxa,	0,      psxntsc, psx,   psx,  psx,    "Sony Computer Entertainment", "Sony PlayStation (Asia-Pacific)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
