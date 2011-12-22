/*
    Apple "Cuda" ADB/system controller MCU
    Emulation by R. Belmont

    Port definitions, primarily from the schematics.

    Port A:

    x-------  O  ADB data line out
    -x------  I  ADB data line in
    --x-----  I  System type.  0 = "secure" power, 1 = "passive" or "soft" power
    ---x----  O  DFAC latch
    ----x---  O  "Fast Reset"
    -----x--  I  Keyboard power switch
    ------x-  I  Chassis switch for "soft" power, pull up for "secure" or "passive"
    -------x  I  Pull up for "passive", "PFW" for "secure" or "soft"

    Port B:

    x-------  O  DFAC bit clock
    -x------  B  DFAC data I/O (used in both directions)
    --x-----  B  VIA shift register data (used in both directions)
    ---x----  O  VIA clock
    ----x---  I  VIA TIP
    -----x--  I  VIA BYTEACK
    ------x-  O  VIA TREQ
    -------x  I  +5v sense

    Port C:
    x---      O  680x0 reset
    -x--      ?  680x0 IPL 2 (used in both directions)
    --x-      ?  IPL 1 for "passive" power, trickle sense for "soft" and "secure"
    ---x	  ?  IPL 0 for "passive" power, pull-up for "soft" power, file server switch for "secure" power
*/

#define ADDRESS_MAP_MODERN

#define CUDA_SUPER_VERBOSE

#include "emu.h"
#include "cuda.h"
#include "cpu/m6805/m6805.h"
#include "machine/6522via.h"
#include "sound/asc.h"
#include "includes/mac.h"

//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define CUDA_CPU_TAG	"cuda"

//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type CUDA = &device_creator<cuda_device>;

ROM_START( cuda )
	ROM_REGION(0x3300, CUDA_CPU_TAG, 0)
    ROM_LOAD( "341s0060.bin", 0x1100, 0x1100, CRC(0f5e7b4a) SHA1(972b3778146d9787b18c3a9874d505cf606b3e15) ) 
    ROM_LOAD( "341s0788.bin", 0x2200, 0x1100, CRC(df6e1b43) SHA1(ec23cc6214c472d61b98964928c40589517a3172) ) 
ROM_END

//-------------------------------------------------
//  ADDRESS_MAP
//-------------------------------------------------

static ADDRESS_MAP_START( cuda_map, AS_PROGRAM, 8, cuda_device )
	AM_RANGE(0x0000, 0x0002) AM_READWRITE(ports_r, ports_w)
	AM_RANGE(0x0004, 0x0006) AM_READWRITE(ddr_r, ddr_w)
	AM_RANGE(0x0007, 0x0007) AM_READWRITE(pll_r, pll_w)
	AM_RANGE(0x0008, 0x0008) AM_READWRITE(timer_ctrl_r, timer_ctrl_w)
	AM_RANGE(0x0009, 0x0009) AM_READWRITE(timer_counter_r, timer_counter_w)
	AM_RANGE(0x0012, 0x0012) AM_READWRITE(onesec_r, onesec_w)
	AM_RANGE(0x0090, 0x00ff) AM_RAM							// work RAM and stack
	AM_RANGE(0x0100, 0x01ff) AM_READWRITE(pram_r, pram_w)
	AM_RANGE(0x0f00, 0x1fff) AM_ROM AM_REGION(CUDA_CPU_TAG, 0)
ADDRESS_MAP_END

//-------------------------------------------------
//  MACHINE_CONFIG
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( cuda )
	MCFG_CPU_ADD(CUDA_CPU_TAG, M68HC05EG, XTAL_32_768kHz*192)	// 32.768 kHz input clock, can be PLL'ed to x128 = 4.1 MHz under s/w control
	MCFG_CPU_PROGRAM_MAP(cuda_map)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor cuda_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( cuda );
}

const rom_entry *cuda_device::device_rom_region() const
{
	return ROM_NAME( cuda );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

void cuda_device::send_port(address_space &space, UINT8 offset, UINT8 data)
{
	switch (offset)
	{
		case 0: // port A
/*          printf("ADB:%d DFAC:%d PowerEnable:%d\n",
                (data & 0x80) ? 1 : 0,
                (data & 0x10) ? 1 : 0,
                (data & 0x02) ? 1 : 0);*/

			if ((data & 0x80) != last_adb)
			{
/*              if (data & 0x80)
                {
                    printf("CU ADB: 1->0 time %lld\n", m_maincpu->total_cycles()-last_adb_time);
                }
                else
                {
                    printf("CU ADB: 0->1 time %lld\n", m_maincpu->total_cycles()-last_adb_time);
                }*/
				last_adb = data & 0x80;
				last_adb_time = m_maincpu->total_cycles();
			}
			break;

        case 1: // port B
            {
				if (treq != ((data>>1)&1))
				{
					#ifdef CUDA_SUPER_VERBOSE
                    printf("CU-> TREQ: %d (PC=%x)\n", (data>>1)&1, cpu_get_pc(m_maincpu));
					#endif
					treq = (data>>1) & 1;
				}
                if (via_data != ((data>>5)&1))
                {
					#ifdef CUDA_SUPER_VERBOSE
                    printf("CU-> VIA_DATA: %d (PC=%x)\n", (data>>5)&1, cpu_get_pc(m_maincpu));
					#endif
					via_data = (data>>5) & 1;
                }
                if (via_clock != ((data>>4)&1))
                {
					#ifdef CUDA_SUPER_VERBOSE
                    printf("CU-> VIA_CLOCK: %d (PC=%x)\n", ((data>>4)&1)^1, cpu_get_pc(m_maincpu));
					#endif
					via_clock = (data>>4) & 1;
					via6522_device *via1 = machine().device<via6522_device>("via6522_0");
					via1->write_cb1(via_clock);
                }
            }
            break;

		case 2: // port C
			if ((data & 8) != reset_line)
			{
				#ifdef CUDA_SUPER_VERBOSE
				printf("680x0 reset: %d -> %d\n", (ports[2] & 8)>>3, (data & 8)>>3);
				#endif
				reset_line = (data & 8);

				// falling edge, should reset the machine too
				if ((ports[2] & 8) && !(data&8))
				{
					mac_state *mac = machine().driver_data<mac_state>();

					// force the memory overlay
					mac->set_memory_overlay(0);
					mac->set_memory_overlay(1);

					// if PRAM's waiting to be loaded, transfer it now
					if (!pram_loaded)
					{
						memcpy(pram, disk_pram, 0x100);
						pram_loaded = true;
					}
				}

				cputag_set_input_line(machine(), "maincpu", INPUT_LINE_RESET, (reset_line & 8) ? ASSERT_LINE : CLEAR_LINE);
			}
			break;
	}
}

READ8_MEMBER( cuda_device::ddr_r )
{
	return ddrs[offset];
}

WRITE8_MEMBER( cuda_device::ddr_w )
{
/*  printf("%02x to DDR %c\n", data, 'A' + offset);*/

	send_port(space, offset, ports[offset] & data);

	ddrs[offset] = data;
}

READ8_MEMBER( cuda_device::ports_r )
{
	UINT8 incoming = 0;

	switch (offset)
	{
		case 0: 	// port A
//          incoming |= adb_in ? 0x40 : 0;

			if (cuda_controls_power)
			{
				incoming = 0x20 | 0x03;
			}
			else
			{
				incoming |= 0x23;   // indicate passive power
			}
			break;

        case 1:		// port B
            if (cuda_controls_power)
            {
                incoming |= 0x01;   // 0 for passive power, apparently
            }
            incoming |= byteack<<2;
            incoming |= tip<<3;
            incoming |= via_data<<5;
			incoming |= 0xc0;	// show DFAC lines high
			break;

		case 2:		// port C
			if (cuda_controls_power)
			{
				incoming |= 0x3;
			}
            else
            {
                incoming |= 0x3;
            }
			break;
	}

	// apply data direction registers
	incoming &= (ddrs[offset] ^ 0xff);
	// add in ddr-masked version of port writes
	incoming |= (ports[offset] & ddrs[offset]);

	return incoming;
}

WRITE8_MEMBER( cuda_device::ports_w )
{
	send_port(space, offset, data);

	ports[offset] = data;
}

READ8_MEMBER( cuda_device::pll_r )
{
	return pll_ctrl;
}

WRITE8_MEMBER( cuda_device::pll_w )
{
	#ifdef CUDA_SUPER_VERBOSE
	if (pll_ctrl != data)
	{
		static const int clocks[4] = { 524288, 1048576, 2097152, 4194304 };
		printf("PLL ctrl: clock %d TCS:%d BCS:%d AUTO:%d BWC:%d PLLON:%d\n", clocks[data&3],
			(data & 0x80) ? 1 : 0,
			(data & 0x40) ? 1 : 0,
			(data & 0x20) ? 1 : 0,
			(data & 0x10) ? 1 : 0,
			(data & 0x08) ? 1 : 0);
	}
	#endif
	pll_ctrl = data;
}

READ8_MEMBER( cuda_device::timer_ctrl_r )
{
	return timer_ctrl;
}

WRITE8_MEMBER( cuda_device::timer_ctrl_w )
{
//  printf("%02x to timer control\n", data);
	timer_ctrl = data;
}

READ8_MEMBER( cuda_device::timer_counter_r )
{
	return timer_counter;
}

WRITE8_MEMBER( cuda_device::timer_counter_w )
{
//  printf("%02x to timer/counter\n", data);
	timer_counter = data;
}

READ8_MEMBER( cuda_device::onesec_r )
{
	return onesec;
}

WRITE8_MEMBER( cuda_device::onesec_w )
{
	static const float rates[4] = { 0.5f, 1.0f, 2.0f, 4.0f };

//  printf("%02x to one-second control\n", data);

	m_timer->adjust(attotime::from_hz(rates[data&3]), 0, attotime::from_hz(rates[data&3]));

	if ((onesec & 0x40) && !(data & 0x40))
	{
		device_set_input_line(m_maincpu, M68HC05EG_INT_CPI, CLEAR_LINE);
	}

	onesec = data;
}

READ8_MEMBER( cuda_device::pram_r )
{
	return pram[offset];
}

WRITE8_MEMBER( cuda_device::pram_w )
{
	pram[offset] = data;
}

//-------------------------------------------------
//  cuda_device - constructor
//-------------------------------------------------

cuda_device::cuda_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, CUDA, "Apple Cuda", tag, owner, clock),
	device_nvram_interface(mconfig, *this),
	m_maincpu(*this, CUDA_CPU_TAG)
{
}

//-------------------------------------------------
//  static_set_type - configuration helper to set
//  the chip type
//-------------------------------------------------

void cuda_device::static_set_type(device_t &device, int type)
{
	cuda_device &cuda = downcast<cuda_device &>(device);
	cuda.rom_offset = type;
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void cuda_device::device_start()
{
	m_timer = timer_alloc(0, NULL);
	save_item(NAME(ddrs[0]));
	save_item(NAME(ddrs[1]));
	save_item(NAME(ddrs[2]));
	save_item(NAME(ports[0]));
	save_item(NAME(ports[1]));
	save_item(NAME(ports[2]));
	save_item(NAME(pll_ctrl));
	save_item(NAME(timer_ctrl));
	save_item(NAME(timer_counter));
	save_item(NAME(onesec));
	save_item(NAME(treq));
	save_item(NAME(byteack));
	save_item(NAME(tip));
	save_item(NAME(via_data));
	save_item(NAME(via_clock));
	save_item(NAME(adb_in));
	save_item(NAME(reset_line));
	save_item(NAME(pram_loaded));
	save_item(NAME(pram));
	save_item(NAME(disk_pram));

	astring tempstring;
	UINT8 *rom = device().machine().region(device().subtag(tempstring, CUDA_CPU_TAG))->base();

    if (rom)                
    {
        memcpy(rom, rom+rom_offset, 0x1100);
    }
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void cuda_device::device_reset()
{
	ddrs[0] = ddrs[1] = ddrs[2] = 0;
	ports[0] = ports[1] = ports[2] = 0;

	m_timer->adjust(attotime::never);

	cuda_controls_power = false;	// set to hard power control
	adb_in = true;	// line is pulled up to +5v, so nothing plugged in would read as "1"
	reset_line = 0;
    tip = 0;
    treq = 0;
    byteack = 0;
    via_data = 0;
    via_clock = 0;
    pll_ctrl = 0;
    timer_ctrl = 0;
    timer_counter = 0;
	last_adb_time = m_maincpu->total_cycles();
}

void cuda_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	onesec |= 0x40;

	if (onesec & 0x10)
	{
		device_set_input_line(m_maincpu, M68HC05EG_INT_CPI, ASSERT_LINE);
	}
}

// the 6805 program clears PRAM on startup (on h/w it's always running once a battery is inserted)
// we deal with that by loading pram from disk to a secondary buffer and then slapping it into "live"
// once the cuda reboots the 68k
void cuda_device::nvram_default()
{
	memset(pram, 0, 0x100);
	memset(disk_pram, 0, 0x100);

	pram[0x1] = 0x80;
	pram[0x2] = 0x4f;
	pram[0x3] = 0x48;
	pram[0x8] = 0x13;
	pram[0x9] = 0x88;
	pram[0xb] = 0x4c;
	pram[0xc] = 0x4e;
	pram[0xd] = 0x75;
	pram[0xe] = 0x4d;
	pram[0xf] = 0x63;
	pram[0x10] = 0xa8;
	pram[0x14] = 0xcc;
	pram[0x15] = 0x0a;
	pram[0x16] = 0xcc;
	pram[0x17] = 0x0a;
	pram[0x1d] = 0x02;
	pram[0x17] = 0x63;
	pram[0x6f] = 0x28;
	pram[0x70] = 0x83;
	pram[0x71] = 0x26;
	pram[0x77] = 0x01;
	pram[0x78] = 0xff;
	pram[0x79] = 0xff;
	pram[0x7a] = 0xff;
	pram[0x7b] = 0xdf;
	pram[0x7d] = 0x09;
	pram[0xf3] = 0x12;
	pram[0xf9] = 0x01;
	pram[0xf3] = 0x12;
	pram[0xfb] = 0x8d;
	pram_loaded = false;
}

void cuda_device::nvram_read(emu_file &file)
{
	file.read(disk_pram, 0x100);
	pram_loaded = false;
}

void cuda_device::nvram_write(emu_file &file)
{
	file.write(pram, 0x100);
}
