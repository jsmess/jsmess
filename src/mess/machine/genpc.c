/***************************************************************************

    machine/genpc.c


***************************************************************************/

#include "emu.h"
#include "includes/genpc.h"

#include "machine/i8255a.h"
#include "machine/pic8259.h"
#include "machine/pit8253.h"
#include "machine/8237dma.h"

#include "machine/pit8253.h"
#include "machine/pckeybrd.h"
#include "sound/speaker.h"

#include "machine/kb_keytro.h"
#include "machine/ram.h"

#include "video/pc_vga_mess.h"

#include "memconv.h"

#define VERBOSE_PIO 0	/* PIO (keyboard controller) */

#define PIO_LOG(N,M,A) \
	do { \
		if(VERBOSE_PIO>=N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s",machine->time().as_double(),(char*)M ); \
			logerror A; \
		} \
	} while (0)


/*************************************************************************
 *
 *      PC DMA stuff
 *
 *************************************************************************/

READ8_DEVICE_HANDLER(genpc_page_r)
{
	return 0xFF;
}


WRITE8_DEVICE_HANDLER(genpc_page_w)
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device);
	switch(offset % 4)
	{
	case 1:
		board->dma_offset[2] = data;
		break;
	case 2:
		board->dma_offset[3] = data;
		break;
	case 3:
		board->dma_offset[0] = board->dma_offset[1] = data;
		break;
	}
}


static WRITE_LINE_DEVICE_HANDLER( pc_dma_hrq_changed )
{
	pc_motherboard_device	*board  = downcast<pc_motherboard_device *>(device->owner());
	cpu_set_input_line(board->maincpu, INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	/* Assert HLDA */
	i8237_hlda_w( device, state );
}


static READ8_DEVICE_HANDLER( pc_dma_read_byte )
{	
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());
	address_space *space = board->maincpu->space(AS_PROGRAM);
	offs_t page_offset = (((offs_t) board->dma_offset[board->dma_channel]) << 16) & 0x0F0000;
	return space->read_byte( page_offset + offset);
}


static WRITE8_DEVICE_HANDLER( pc_dma_write_byte )
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());
	address_space *space = board->maincpu->space(AS_PROGRAM);
	offs_t page_offset = (((offs_t) board->dma_offset[board->dma_channel]) << 16) & 0x0F0000;

	space->write_byte( page_offset + offset, data);
}


static READ8_DEVICE_HANDLER( pc_dma8237_1_dack_r )
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());
	return board->isabus->dack_r(1);
}

static READ8_DEVICE_HANDLER( pc_dma8237_2_dack_r )
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());
	return board->isabus->dack_r(2);
}


static READ8_DEVICE_HANDLER( pc_dma8237_3_dack_r )
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());
	return board->isabus->dack_r(3);
}


static WRITE8_DEVICE_HANDLER( pc_dma8237_1_dack_w )
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());
	board->isabus->dack_w(1,data);
}

static WRITE8_DEVICE_HANDLER( pc_dma8237_2_dack_w )
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());
	board->isabus->dack_w(2,data);
}


static WRITE8_DEVICE_HANDLER( pc_dma8237_3_dack_w )
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());
	board->isabus->dack_w(3,data);
}


static WRITE8_DEVICE_HANDLER( pc_dma8237_0_dack_w )
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());
	board->u73_q2 = 0;
	i8237_dreq0_w( board->dma8237, board->u73_q2 );
}


static WRITE_LINE_DEVICE_HANDLER( pc_dma8237_out_eop )
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());	
	return board->isabus->eop_w(state == ASSERT_LINE ? 0 : 1 );
}

static void set_dma_channel(device_t *device, int channel, int state)
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());

	if (!state) board->dma_channel = channel;
}

static WRITE_LINE_DEVICE_HANDLER( pc_dack0_w ) { set_dma_channel(device, 0, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack1_w ) { set_dma_channel(device, 1, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack2_w ) { set_dma_channel(device, 2, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack3_w ) { set_dma_channel(device, 3, state); }

I8237_INTERFACE( genpc_dma8237_config )
{
	DEVCB_LINE(pc_dma_hrq_changed),
	DEVCB_LINE(pc_dma8237_out_eop),
	DEVCB_HANDLER(pc_dma_read_byte),
	DEVCB_HANDLER(pc_dma_write_byte),
	{ DEVCB_NULL, 						  DEVCB_HANDLER(pc_dma8237_1_dack_r), DEVCB_HANDLER(pc_dma8237_2_dack_r), DEVCB_HANDLER(pc_dma8237_3_dack_r) },
	{ DEVCB_HANDLER(pc_dma8237_0_dack_w), DEVCB_HANDLER(pc_dma8237_1_dack_w), DEVCB_HANDLER(pc_dma8237_2_dack_w), DEVCB_HANDLER(pc_dma8237_3_dack_w) },
	{ DEVCB_LINE(pc_dack0_w), DEVCB_LINE(pc_dack1_w), DEVCB_LINE(pc_dack2_w), DEVCB_LINE(pc_dack3_w) }
};


/*************************************************************
 *
 * pic8259 configuration
 *
 *************************************************************/
static WRITE_LINE_DEVICE_HANDLER(pc_cpu_line)
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());
	board->maincpu->set_input_line(INPUT_LINE_IRQ0, state);
}
const struct pic8259_interface genpc_pic8259_config =
{
	DEVCB_LINE(pc_cpu_line)
};


/*************************************************************************
 *
 *      PC Speaker related
 *
 *************************************************************************/
static WRITE_LINE_DEVICE_HANDLER(pc_speaker_set_spkrdata)
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());
	board->pc_spkrdata = state ? 1 : 0;
	speaker_level_w( board->speaker, board->pc_spkrdata & board->pc_input );
}


/*************************************************************
 *
 * pit8253 configuration
 *
 *************************************************************/

static WRITE_LINE_DEVICE_HANDLER( genpc_pit8253_out1_changed )
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());
	/* Trigger DMA channel #0 */
	if ( board->out1 == 0 && state == 1 && board->u73_q2 == 0 )
	{
		board->u73_q2 = 1;
		i8237_dreq0_w( board->dma8237, board->u73_q2 );
	}
	board->out1 = state;
}


static WRITE_LINE_DEVICE_HANDLER( genpc_pit8253_out2_changed )
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());
	board->pc_input = state ? 1 : 0;
	speaker_level_w( board->speaker, board->pc_spkrdata & board->pc_input );	
}


const struct pit8253_config genpc_pit8253_config =
{
	{
		{
			XTAL_14_31818MHz/12,				/* heartbeat IRQ */
			DEVCB_NULL,
			DEVCB_DEVICE_LINE("pic8259", pic8259_ir0_w)
		}, {
			XTAL_14_31818MHz/12,				/* dram refresh */
			DEVCB_NULL,
			DEVCB_LINE(genpc_pit8253_out1_changed)
		}, {
			XTAL_14_31818MHz/12,				/* pio port c pin 4, and speaker polling enough */
			DEVCB_NULL,
			DEVCB_LINE(genpc_pit8253_out2_changed)
		}
	}
};

/**********************************************************
 *
 * PPI8255 interface
 *
 *
 * PORT A (input)
 *
 * Directly attached to shift register which stores data
 * received from the keyboard.
 *
 * PORT B (output)
 * 0 - PB0 - TIM2GATESPK - Enable/disable counting on timer 2 of the 8253
 * 1 - PB1 - SPKRDATA    - Speaker data
 * 2 - PB2 -             - Enable receiving data from the keyboard when keyboard is not locked.
 * 3 - PB3 -             - Dipsswitch set selector
 * 4 - PB4 - ENBRAMPCK   - Enable ram parity check
 * 5 - PB5 - ENABLEI/OCK - Enable expansion I/O check
 * 6 - PB6 -             - Connected to keyboard clock signal
 *                         0 = ignore keyboard signals
 *                         1 = accept keyboard signals
 * 7 - PB7 -             - Clear/disable shift register and IRQ1 line
 *                         0 = normal operation
 *                         1 = clear and disable shift register and clear IRQ1 flip flop
 *
 * PORT C
 * 0 - PC0 -         - Dipswitch 0/4 SW1
 * 1 - PC1 -         - Dipswitch 1/5 SW1
 * 2 - PC2 -         - Dipswitch 2/6 SW1
 * 3 - PC3 -         - Dipswitch 3/7 SW1
 * 4 - PC4 - SPK     - Speaker/cassette data
 * 5 - PC5 - I/OCHCK - Expansion I/O check result
 * 6 - PC6 - T/C2OUT - Output of 8253 timer 2
 * 7 - PC7 - PCK     - Parity check result
 *
 * IBM5150 SW1:
 * 0   - OFF - One or more floppy drives
 *       ON  - Diskless operation
 * 1   - OFF - 8087 present
 *       ON  - No 8087 present
 * 2+3 - Used to determine on board memory configuration
 *       OFF OFF - 64KB
 *       ON  OFF - 48KB
 *       OFF ON  - 32KB
 *       ON  ON  - 16KB
 * 4+5 - Used to select display
 *       OFF OFF - Monochrome
 *       ON  OFF - CGA, 80 column
 *       OFF ON  - CGA, 40 column
 *       ON  ON  - EGA/VGA display
 * 6+7 - Used to select number of disk drives
 *       OFF OFF - four disk drives
 *       ON  OFF - three disk drives
 *       OFF ON  - two disk drives
 *       ON  ON  - one disk drive
 *
 **********************************************************/

WRITE8_DEVICE_HANDLER( genpc_kb_set_clock_signal )
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device);
	device_t *keyboard = device->machine->device("keyboard");

	if ( board->ppi_clock_signal != data )
	{
		if ( board->ppi_keyb_clock && board->ppi_shift_enable )
		{
			board->ppi_clock_signal = data;
			if ( ! board->ppi_keyboard_clear )
			{
				/* Data is clocked in on a high->low transition */
				if ( ! data )
				{
					UINT8	trigger_irq = board->ppi_shift_register & 0x01;

					board->ppi_shift_register = ( board->ppi_shift_register >> 1 ) | ( board->ppi_data_signal << 7 );
					if ( trigger_irq )
					{
						pic8259_ir1_w(board->pic8259, 1);
						board->ppi_shift_enable = 0;
						board->ppi_clock_signal = 0;
						kb_keytronic_clock_w(keyboard, board->ppi_clock_signal);
					}
				}
			}
		}
	}

	kb_keytronic_clock_w(keyboard, board->ppi_clock_signal);
}


WRITE8_DEVICE_HANDLER( genpc_kb_set_data_signal )
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device);
	device_t *keyboard = device->machine->device("keyboard");

	board->ppi_data_signal = data;

	kb_keytronic_data_w(keyboard, board->ppi_data_signal);
}

static READ8_DEVICE_HANDLER (genpc_ppi_porta_r)
{
	int data = 0xFF;
	running_machine *machine = device->machine;
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());

	/* KB port A */
	if (board->ppi_keyboard_clear)
	{
		/*   0  0 - no floppy drives
         *   1  Not used
         * 2-3  The number of memory banks on the system board
         * 4-5  Display mode
         *      11 = monochrome
         *      10 - color 80x25
         *      01 - color 40x25
         * 6-7  The number of floppy disk drives
         */
		data = input_port_read(board, "DSW0");
	}
	else
	{
		data = board->ppi_shift_register;
	}
    PIO_LOG(1,"PIO_A_r",("$%02x\n", data));
    return data;
}


static READ8_DEVICE_HANDLER ( genpc_ppi_portc_r )
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());
	int timer2_output = pit8253_get_output( board->pit8253, 2 );
	int data=0xff;
	running_machine *machine = device->machine;

	data&=~0x80; // no parity error
	data&=~0x40; // no error on expansion board
	/* KB port C: equipment flags */
	if (board->ppi_portc_switch_high)
	{
		/* read hi nibble of S2 */
		data = (data & 0xf0) | ((input_port_read(board, "DSW0") >> 4) & 0x0f);
		PIO_LOG(1,"PIO_C_r (hi)",("$%02x\n", data));
	}
	else
	{
		/* read lo nibble of S2 */
		data = (data & 0xf0) | (input_port_read(board, "DSW0") & 0x0f);
		PIO_LOG(1,"PIO_C_r (lo)",("$%02x\n", data));
	}

	if ( board->ppi_portb & 0x01 )
	{
		data = ( data & ~0x10 ) | ( timer2_output ? 0x10 : 0x00 );
	}
	data = ( data & ~0x20 ) | ( timer2_output ? 0x20 : 0x00 );

	return data;
}


static WRITE8_DEVICE_HANDLER( genpc_ppi_portb_w )
{
	pc_motherboard_device *board  = downcast<pc_motherboard_device *>(device->owner());
	device_t *keyboard = device->machine->device("keyboard");

	/* PPI controller port B*/
	board->ppi_portb = data;
	board->ppi_portc_switch_high = data & 0x08;
	board->ppi_keyboard_clear = data & 0x80;
	board->ppi_keyb_clock = data & 0x40;
	pit8253_gate2_w(board->pit8253, BIT(data, 0));
	pc_speaker_set_spkrdata( device, data & 0x02 );

	board->ppi_clock_signal = ( board->ppi_keyb_clock ) ? 1 : 0;
	kb_keytronic_clock_w(keyboard, board->ppi_clock_signal);

	/* If PB7 is set clear the shift register and reset the IRQ line */
	if ( board->ppi_keyboard_clear )
	{
		pic8259_ir1_w(board->pic8259, 0);
		board->ppi_shift_register = 0;
		board->ppi_shift_enable = 1;
	}
}


I8255A_INTERFACE( genpc_ppi8255_interface )
{
	DEVCB_HANDLER(genpc_ppi_porta_r),
	DEVCB_NULL,
	DEVCB_HANDLER(genpc_ppi_portc_r),
	DEVCB_NULL,
	DEVCB_HANDLER(genpc_ppi_portb_w),
	DEVCB_NULL
};

static const isabus_interface isabus_intf =
{
	// interrupts
	DEVCB_DEVICE_LINE("pic8259", pic8259_ir2_w),
	DEVCB_DEVICE_LINE("pic8259", pic8259_ir3_w),
	DEVCB_DEVICE_LINE("pic8259", pic8259_ir4_w),
	DEVCB_DEVICE_LINE("pic8259", pic8259_ir5_w),
	DEVCB_DEVICE_LINE("pic8259", pic8259_ir6_w),
	DEVCB_DEVICE_LINE("pic8259", pic8259_ir7_w),

	// dma request
	DEVCB_DEVICE_LINE("dma8237", i8237_dreq1_w),
	DEVCB_DEVICE_LINE("dma8237", i8237_dreq2_w),
	DEVCB_DEVICE_LINE("dma8237", i8237_dreq3_w)
};

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************
 
const device_type PC_MOTHERBOARD = pc_motherboard_device_config::static_alloc_device_config;
 
//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

static MACHINE_CONFIG_FRAGMENT( pc_motherboard_config )
	MCFG_PIT8253_ADD( "pit8253", genpc_pit8253_config )

	MCFG_I8237_ADD( "dma8237", XTAL_14_31818MHz/3, genpc_dma8237_config )

	MCFG_PIC8259_ADD( "pic8259", genpc_pic8259_config )

	MCFG_I8255A_ADD( "ppi8255", genpc_ppi8255_interface )
		
	MCFG_ISA8_BUS_ADD("isa", "maincpu", isabus_intf)
	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mb:mono", 1.00)
MACHINE_CONFIG_END

 
//-------------------------------------------------
//  pc_motherboard_device_config - constructor
//-------------------------------------------------
 
pc_motherboard_device_config::pc_motherboard_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
        : device_config(mconfig, static_alloc_device_config, "PC_MOTHERBOARD", tag, owner, clock)
{
}
 
//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------
 
device_config *pc_motherboard_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
	return global_alloc(pc_motherboard_device_config(mconfig, tag, owner, clock));
}
 
//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------
 
device_t *pc_motherboard_device_config::alloc_device(running_machine &machine) const
{
	return auto_alloc(&machine, pc_motherboard_device(machine, *this));
}

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor pc_motherboard_device_config::machine_config_additions() const
{
	return MACHINE_CONFIG_NAME( pc_motherboard_config );
}


static INPUT_PORTS_START( pc_motherboard )
	PORT_START("DSW0") /* IN1 */
	PORT_DIPNAME( 0xc0, 0x40, "Number of floppy drives")
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_DIPNAME( 0x30, 0x30, "Graphics adapter")
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_DIPNAME( 0x0c, 0x0c, "RAM banks")
	PORT_DIPSETTING(	0x00, "1 - 16/ 64/256K" )
	PORT_DIPSETTING(	0x04, "2 - 32/128/512K" )
	PORT_DIPSETTING(	0x08, "3 - 48/192/576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64/256/640K" )
	PORT_DIPNAME( 0x02, 0x00, "8087 installed")
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x02, DEF_STR(Yes) )
	PORT_DIPNAME( 0x01, 0x01, "Any floppy drive installed")
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x01, DEF_STR(Yes) )
INPUT_PORTS_END
//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

const input_port_token *pc_motherboard_device_config::input_ports() const
{
	return INPUT_PORTS_NAME( pc_motherboard );
}
	

void pc_motherboard_device_config::static_set_cputag(device_config *device, const char *tag)
{
	pc_motherboard_device_config *isa = downcast<pc_motherboard_device_config *>(device);
	isa->m_cputag = tag;
}
 
//**************************************************************************
//  LIVE DEVICE
//**************************************************************************
 
//-------------------------------------------------
//  pc_motherboard_device - constructor
//-------------------------------------------------
 
pc_motherboard_device::pc_motherboard_device(running_machine &_machine, const pc_motherboard_device_config &config) :
        device_t(_machine, config),
        m_config(config),
		maincpu(*(owner()), config.m_cputag),
		pic8259(*this, "pic8259"),
		dma8237(*this, "dma8237"),
		pit8253(*this, "pit8253"),
		ppi8255(*this, "ppi8255"),
		speaker(*this, "speaker"),
		isabus(*this, "isa")
{
}
 
 #define memory_install_readwrite8_device_handler_mask(space, device, start, end, mask, mirror, rhandler, whandler, unitmask) \
	const_cast<address_space *>(space)->install_legacy_handler(*(device), start, end, mask, mirror, rhandler, #rhandler, whandler, #whandler, unitmask)

void pc_motherboard_device::install_device(device_t *dev, offs_t start, offs_t end, offs_t mask, offs_t mirror, read8_device_func rhandler, write8_device_func whandler)
{	
	int buswidth = device_memory(maincpu)->space_config(ADDRESS_SPACE_IO)->m_databus_width;
	switch(buswidth)
	{
		case 8:
			memory_install_readwrite8_device_handler_mask(cpu_get_address_space(maincpu, ADDRESS_SPACE_IO), dev, start, end, mask, mirror, rhandler, whandler, 0);			
			break;
		case 16:
			memory_install_readwrite8_device_handler_mask(cpu_get_address_space(maincpu, ADDRESS_SPACE_IO), dev, start, end, mask, mirror, rhandler, whandler, 0xffff);			
			break;
		default:
			fatalerror("PC_MOTHERBOARD: Bus width %d not supported", buswidth);
			break;
	}
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------
 
void pc_motherboard_device::device_start()
{
	install_device(dma8237, 0x0000, 0x000f, 0, 0, i8237_r, i8237_w );	
	install_device(pic8259, 0x0020, 0x0021, 0, 0, pic8259_r, pic8259_w );	
	install_device(pit8253, 0x0040, 0x0043, 0, 0, pit8253_r, pit8253_w );	
	install_device(ppi8255, 0x0060, 0x0063, 0, 0, i8255a_r, i8255a_w );	
	install_device(this,    0x0080, 0x0087, 0, 0, genpc_page_r, genpc_page_w );	
}
 
IRQ_CALLBACK(pc_motherboard_device::pc_irq_callback)
{
	device_t *pic = device->machine->device("mb:pic8259");
	return pic8259_acknowledge( pic );	
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------
  
void pc_motherboard_device::device_reset()
{
	cpu_set_irq_callback(maincpu, pc_irq_callback);

	u73_q2 = 0;
	out1 = 0;
	pc_spkrdata = 0;
	pc_input = 0;
	dma_channel = 0;
	memset(dma_offset,0,sizeof(dma_offset));
	ppi_portc_switch_high = 0;
	ppi_speaker = 0;
	ppi_keyboard_clear = 0;
	ppi_keyb_clock = 0;
	ppi_portb = 0;
	ppi_clock_signal = 0;
	ppi_data_signal = 0;
	ppi_shift_register = 0;
	ppi_shift_enable = 0;
	
	speaker_level_w( speaker, 0 );
}

/********************************************************************************************/

DRIVER_INIT( genpc )
{
	/* MESS managed RAM */
	if ( ram_get_ptr(machine->device(RAM_TAG)) )
		memory_set_bankptr( machine, "bank10", ram_get_ptr(machine->device(RAM_TAG)) );
}

static READ8_HANDLER( input_port_0_r ) { return 0x08; }

static const struct pc_vga_interface vga_interface =
{
	NULL,
	NULL,

	input_port_0_r,

	ADDRESS_SPACE_IO,
	0x0000
};

DRIVER_INIT( genpcvga )
{
	DRIVER_INIT_CALL(genpc);
	pc_vga_init(machine, &vga_interface, NULL);
}
