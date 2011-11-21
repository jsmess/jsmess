/***************************************************************************

  3do_m2.c

  Driver file to handle emulation of the 3DO M2 systems

	Hardware descriptions:

	Central Processing Unit - Dual 66 MHz PowerPC 602s 
		Implements the 32-bit PowerPC RISC instruction set architecture
		PowerPC CPU designed for consumer electronics applications
		1.2 watts power usage each
		32-bit general purpose registers and ALU
		33 MHz 64-bit multiplexed address and data bus
		4 KiB data and instruction caches (Level 1). No Level 2 cache
		1 integer unit, 1 floating point unit, no branch processing unit, 1 load/store unit
		SPECint92 rating of 40 each, approximately 70 MIPS each.
		1 million transistors manufactured on a 0.50 micrometre CMOS process
	Custom ASICs 
		BDA: 
			Memory control, system control, and video/graphic control
			Full triangle renderer including setup engine, MPEG-1 decoder hardware, 
			DSP for audio and various kinds of DMA control and port access
			Random access of frame buffer and z-buffer (actually w-buffer) possible at the same time
		CDE: 
			Power bus connected to BDA and the two CPUs
			"bio-bus" used as a low-speed bus for peripheral hardware
	Renderer capabilities: 
		1 million un-textured triangles/s geometry rate
		100 million pixels/s fill rate
		reportedly 700,000 textured polygons/s *without* gouraud shading or additional effects
		reportedly 300,000 to 500,000 textured polygons/s *with* gouraud shading, lighting and effects
		shading: flat shading and gouraud shading
		texture mapping
		decal, modulation blending, tiling (16K/128K texture buffer built-in)
		hardware z-buffer (16-bit) (actually a block floating point with multiple (4) range w-buffer)
		object-based full-scene anti-aliasing
		alpha channel (4-bit or 7-bit)
		320x240 to 640x480 resolution at 24-bit color
	Sound hardware - 16-bit 32-channel DSP at 66 MHz (within BDA chip)
	Media - Quad-speed CD-ROM drive (600 KB/s)
	RAM - Unified memory subsystem with 8 MB/s 
		64-bit bus resulting in peak 533 MB/s bandwidth
		Average access 400 MB/s
	Full Motion Video - MPEG-1
	Writable Storage - Memory cards from 128 KiB to 32 MiB
	Expansion Capabilities - 1 PCMCIA port (potentially used for Modems, Ethernet NICs, etc.)

***************************************************************************/

#include "emu.h"
#include "cpu/powerpc/ppc.h"

class _3do_m2_state : public driver_device
{
public:
	_3do_m2_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

};

static ADDRESS_MAP_START( 3do_m2_mem, AS_PROGRAM, 64)
	AM_RANGE(0x00000000, 0x000fffff) AM_ROM AM_REGION("user1",0)
	AM_RANGE(0x20000000, 0x200fffff) AM_RAM
	AM_RANGE(0xfff00000, 0xffffffff) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( 3do_m2_slave_mem, AS_PROGRAM, 64)
ADDRESS_MAP_END

static INPUT_PORTS_START( 3do_m2 )
INPUT_PORTS_END


static MACHINE_RESET( 3do_m2 )
{
}

static MACHINE_CONFIG_START( 3do_m2, _3do_m2_state )

	/* Basic machine hardware */
	/* basic machine hardware */
	MCFG_CPU_ADD("ppc1", PPC602, 66000000)	/* 66 MHz */
	MCFG_CPU_PROGRAM_MAP(3do_m2_mem)

	MCFG_CPU_ADD("ppc2", PPC602, 66000000)	/* 66 MHz */
	MCFG_CPU_PROGRAM_MAP(3do_m2_slave_mem)
	MCFG_DEVICE_DISABLE()
	
	MCFG_MACHINE_RESET( 3do_m2 )
MACHINE_CONFIG_END


ROM_START(3do_m2)
	ROM_REGION( 0x100000, "user1", ROMREGION_64BIT | ROMREGION_BE )
	ROM_SYSTEM_BIOS( 0, "panafz35", "Panasonic FZ-35S (3DO M2)" )	
	ROMX_LOAD( "fz35_jpn.bin", 0x000000, 0x100000, CRC(e1c5bfd3) SHA1(0a3e27d672be79eeee1d2dc2da60d82f6eba7934), ROM_BIOS(1) )
ROM_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT   INIT    COMPANY FULLNAME        FLAGS */
CONS( 199?, 3do_m2,     0,      0,      3do_m2,    3do_m2,	0,      "3DO",  "3DO M2",   	GAME_NOT_WORKING | GAME_NO_SOUND )
