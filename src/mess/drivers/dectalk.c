/******************************************************************************
*
*  Dectalk DTC-01 Driver
*  By Jonathan Gevaryahu AKA Lord Nightmare
*  with major help (dumping, tech questions answered, etc)
*  from Kevin 'kevtris' Horton, without whom this driver would
*  have been impossible.
*  Special thanks to Al Kossow for archiving the DTC-01 schematic at bitsavers,
*  which is invaluable for work on this driver.
*
*  TODO:
*  * Attach 2681(M68681 clone?) DUART
*  * Attach m68k interrupts properly
*  * Figure out why status LEDS are flashing 00 FD 00 FD etc... some sort of 68k selftest DUART error?
*    * NICETOHAVE: figure out what all the LED error codes actually mean, as DEC didn't document them anywhere.
*  * Actually store the X2212 nvram's eeprom data to disk rather than throwing it out on exit
*  * force sync between both processors if either one pushes or pulls the fifo
*      or the shared error/fifo flag regs... How can I actually do this? CPU_BOOST_INTERLEAVE?
*      otherwise i need to have the two in some mad insane sync...
*  * Hook the 'output' FIFO up to the 10khz sampling rate generator xtal using SPEAKER
*  * emulate/simulate the MT8060 dtmf decoder as an input device
*  * discuss and figure out how to have an external application send data to the two serial ports to be spoken
*
*******************************************************************************/
/*the 68k memory map is such:
0x000000-0x007fff: E22 low, E8 high
0x008000-0x00ffff: E21 low, E7 high
0x010000-0x017fff: E20 low, E6 high
0x018000-0x01ffff: E19 low, E5 high
0x020000-0x027fff: E18 low, E4 high
0x028000-0x02ffff: E17 low, E3 high
0x030000-0x037fff: E16 low, E2 high
0x038000-0x03ffff: E15 low, E1 high
mirrrored at 0x040000-0x07ffff
ram/nvram/speech mapping:
0x080000-0x083fff: e36 low, e49 high
0x084000-0x087fff: e35 low, e48 high
0x088000-0x08bfff: e34 low, e47 high
0x08c000-0x08ffff: e33 low, e46 high
0x090000-0x093fff: e32 low, e45 high
0x094000-0x097fff: LED/SW/NVR
0x098000-0x09bfff: DUART
0x09c000-0x09ffff: TMS32010 speech (TLC, SPC)
mirrored at 0x0a0000-0x0fffff x3
entire space mirrored at 0x100000-0x7fffff
0x800000-0xffffff is open bus?

interrupts:
68k has an interrupt priority decoder attached to it:
TLC is INT D6
SPC is INT D4
DUART is INT D3
*/



/* Core includes */
#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/tms32010/tms32010.h"
#include "dectalk.lh" // nasty hack to avoid crash on screenless system
#include "machine/68681.h"


/* Components */

/* Devices */
static struct
{
UINT8 data[8]; // ? what is this for?
UINT8 nvram_local[256]; // sram tempspace on the x2214
UINT8 statusLED;
// input fifo, between m68k and tms32010
UINT16 infifo[32]; // technically eight 74LS224 4bit*16stage FIFO chips, arranged as a 32 stage, 16-bit wide fifo
UINT8 infifo_tail_ptr;
UINT8 infifo_head_ptr;
// output fifo, between tms32010 and 10khz sample latch for dac
UINT16 outfifo[16]; // technically three 74LS224 4bit*16stage FIFO chips, arranged as a 16 stage, 12-bit wide fifo
UINT8 outfifo_tail_ptr;
UINT8 outfifo_head_ptr;
UINT8 outfifo_writable_latch; // latch for status of output fifo, d-latch 74ls74 @ E64 'lower half'
UINT8 spc_error_latch; // latch for error status of speech dsp, d-latch 74ls74 @ E64 'upper half'
UINT8 m68k_spcflags_latch; // latch for initializing the speech dsp, d-latch 74ls74 @ E29 'lower half', AND latch for spc irq enable, d-latch 74ls74 @ E29 'upper half'; these are stored in bits 0 and 6 respectively, the rest of the bits stored here MUST be zeroed!
UINT8 m68k_tlcflags_latch; // latch for telephone interface stuff, d-latches 74ls74 @ E93 'upper half' and @ 103 'upper and lower halves'
} dectalk={ {0}};

#define SPC_INITIALIZE dectalk.m68k_spcflags_latch&0x1 // speech initialize flag
#define SPC_IRQ_ENABLED ((dectalk.m68k_spcflags_latch&0x40)>>6) // irq enable flag

static void dectalk_clear_all_fifos( running_machine *machine )
{
	// clear fifos (TODO: memset would work better here...)
	int i;
	for (i=0; i<16; i++) dectalk.outfifo[i] = 0;
	for (i=0; i<32; i++) dectalk.infifo[i] = 0;
	dectalk.outfifo_tail_ptr = dectalk.outfifo_head_ptr = 0;
	dectalk.infifo_tail_ptr = dectalk.infifo_head_ptr = 0;
}

static void dectalk_x2212_store( running_machine *machine )
{
	UINT8 *nvram = memory_region(machine, "nvram");
	int i;
	for (i = 0; i < 256; i++)
	nvram[i] = dectalk.nvram_local[i];
	logerror("nvram store done\n");
}

static void dectalk_x2212_recall( running_machine *machine )
{
	UINT8 *nvram = memory_region(machine, "nvram");
	int i;
	for (i = 0; i < 256; i++)
	dectalk.nvram_local[i] = nvram[i];
	logerror("nvram recall done\n");
}

/* Driver init: stuff that needs setting up which isn't directly affected by reset */
DRIVER_INIT( dectalk )
{
	dectalk_clear_all_fifos(machine);
	dectalk.outfifo_writable_latch = 0;
	dectalk.spc_error_latch = 0;
}

/* Machine reset: stuff that needs setting up which IS directly affected by reset */
MACHINE_RESET( dectalk )
{
// stuff that is affected by the RESET generator on the pcb: the status LED latch, the novram (forced load from eeprom to tempram)
	dectalk.statusLED = 0; // clear status led latch
	dectalk_x2212_recall(machine);
	dectalk.m68k_spcflags_latch = 1; // initial status is speech reset(d0) active and spc int(d6) disabled
	dectalk.m68k_tlcflags_latch = 0; // initial status is tone detect int(d6) off, answer phone(d8) off, ring detect int(d14) off
	dectalk_clear_all_fifos(machine); // which also clears the fifos, though we have to do it explicitly here since we're not actually in the m68k_spcflags_w function.
	/* write me */ // it also forces the CLR line active on the tms32010, we need to do that here as well
}

/* Begin 68k i/o handlers */

/* x2212 nvram/sram chip and led array demunger due to interleaved addresses */
/* 0x400 long: anything at offset&0x1 = 0 is open bus read, write to leds; everything else is nvram*/
READ16_HANDLER( led_sw_nvr_read ) 
{
	UINT16 data = 0xFFFF;
	/*
	if (ACCESSING_BITS_0_7) // LEDS
	{
		return data; // should be open bus; reading the LEDs doesn't work so well
	}
	else if (ACCESSING_BITS_8_15) // X2212 NVRAM chip
	{
		data = dectalk.nvram_local[(offset>>1)&0xff]<<8; // TODO: should this be before or after a possible /RECALL? I'm guessing before.
		if (offset&0x200) // if a9 is set, do a /RECALL
		dectalk_x2212_recall(space->machine);
	}
	*/
	data = dectalk.nvram_local[(offset>>1)&0xff]<<8; // TODO: should this be before or after a possible /RECALL? I'm guessing before.
	if (ACCESSING_BITS_8_15) // X2212 NVRAM chip
	{
		if (offset&0x200) // if a9 is set, do a /RECALL
		dectalk_x2212_recall(space->machine);
	}
	return data;
}

WRITE16_HANDLER( led_sw_nv_write ) 
{
	if (ACCESSING_BITS_0_7) // LEDS
	{
		dectalk.statusLED = data&0xFF;
		popmessage("LED status: %x\n", data&0xFF);
		//popmessage("LED status: %x %x %x %x %x %x %x %x\n", data&0x80, data&0x40, data&0x20, data&0x10, data&0x8, data&0x4, data&0x2, data&0x1);
	}
	else if (ACCESSING_BITS_8_15) // X2212 NVRAM chip
	{
		dectalk.nvram_local[(offset>>1)&0xff] = (UINT8)((data&0xF00)>>8); // TODO: should this be before or after a possible /STORE? I'm guessing before.
		if (offset&0x200) // if a9 is set, do a /STORE
		dectalk_x2212_store(space->machine);
	}
}

WRITE16_HANDLER( m68k_infifo_w ) // 68k write to the speech input fifo
{
    dectalk.infifo[dectalk.infifo_head_ptr] = data;
    dectalk.infifo_head_ptr++;
    dectalk.infifo_head_ptr&=0x1F;
}

READ16_HANDLER( m68k_spcflags_r ) // 68k read from the speech flags
{
	UINT8 data = 0;
	data |= dectalk.m68k_spcflags_latch; // bits 0 and 6
	data |= dectalk.spc_error_latch<<5; // bit 5
	data |= dectalk.outfifo_writable_latch<<7; // bit 7
	return data;
}

WRITE16_HANDLER( m68k_spcflags_w ) // 68k write to the speech flags (only 3 bits do anything)
{
	dectalk.m68k_spcflags_latch = data&0x41; // ONLY store bits 6 and 0!
	// d0: initialize speech flag (reset tms32010 and clear infifo and outfifo if high)
	if ((data&0x1) == 0x1) // bit 0
	{
		dectalk_clear_all_fifos(space->machine);
		/* write me */ // data&1 also forces the CLR line active on the tms32010, we need to do that here as well
	}
	if ((data&0x2) == 0x2) // bit 1
	{
	// clear the two speech side latches
	dectalk.spc_error_latch = 0;
	dectalk.outfifo_writable_latch = 0;
	}
	if ((data&0x40) == 0x40) // bit 6
	{
		/* write me */ //enable or disable the spc irq, which triggers when fifo writable goes high, if data&40 is set.
	}
}

READ16_HANDLER( m68k_tlcflags_r ) // dtmf flags read
{
	UINT16 data = 0;
	data |= dectalk.m68k_tlcflags_latch; // bits 6, 8, 14;
	//data |= dectalk.tlc_tonedetect<<7; // bit 7 is tone detect
	//data |= dectalk.tlc_ringdetect<<14; // bit 15 is ring detect
	return data;
}

WRITE16_HANDLER( m68k_tlcflags_w ) // dtmf flags write
{
	dectalk.m68k_tlcflags_latch = data&0x4140; // ONLY store bits 6 8 and 14!
	// write me for the ring and tone int enables and answer phone
}

READ16_HANDLER( m68k_tlc_dtmf_r ) // dtmf chip read
{
// write me!
return 0;
}

/* End 68k i/o handlers */

/* Begin tms32010 i/o handlers */
WRITE16_HANDLER( spc_latch_outfifo_error_stats ) // latch 74ls74 @ E64 upper and lower halves with d0 and fifo status respectively
{
    dectalk.outfifo_writable_latch = (dectalk.outfifo_tail_ptr == ((dectalk.outfifo_head_ptr-1)&0xF)?0:1); // may have polarity backwards
    dectalk.spc_error_latch = data&1;
}

READ16_HANDLER( spc_infifo_data_r )
{
 UINT16 data = 0xFFFF;
 data = dectalk.infifo[dectalk.infifo_tail_ptr];
 //logerror("dsp read from infifo, data %x\n", data);
 dectalk.infifo_tail_ptr++;
 dectalk.infifo_tail_ptr&=0x1F;
 return data;
}

WRITE16_HANDLER( spc_outfifo_data_w )
{
 // the low 4 data bits are thrown out on the real unit due to use of a 12 bit dac (and to save use of another 16x4 fifo chip), though technically they're probably valid, and with suitable hacking a dtc-01 could probably output full 16 bit samples at 10khz.
 logerror("dsp write to outfifo, data %x\n", data);
 dectalk.outfifo[dectalk.outfifo_head_ptr] = data;
 dectalk.outfifo_head_ptr++;
 dectalk.outfifo_head_ptr&=0xF;
}

READ16_HANDLER( spc_outfifo_writable_r ) // Return state of d-latch 74ls74 @ E64 'lower half' in d0 which indicates whether outfifo is writable
{
 return dectalk.outfifo_writable_latch;
}
/* end tms32010 i/o handlers */


/******************************************************************************
 Address Maps
******************************************************************************/
/*
Address maps (x = ignored; * = selects address within this range)
68k address map:
a23	a22	a21	a20	a19	a18	a17	a16	a15	a14	a13	a12	a11	a10	a9	a8	a7	a6	a5	a4	a3	a2	a1	(a0 via UDS/LDS)
0	x	x	x	0	x	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*		R	ROM
0	x	x	x	1	x	x	0	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*		RW	RAM (first 4 chip pairs)
0	x	x	x	1	x	x	1	0	0	*	*	*	*	*	*	*	*	*	*	*	*	*	*		RW	RAM (last chip pair)
0	x	x	x	1	x	x	1	0	1	x	x	x	x	x	x	x	x	x	x	x	x	x	0		W	Status LED <d7-d0>
0	x	x	x	1	x	x	1	0	1	x	x	x	x	0	*	*	*	*	*	*	*	*	1		RW	NVRAM (read/write volatile ram, does not store to eeprom)
0	x	x	x	1	x	x	1	0	1	x	x	x	x	1	*	*	*	*	*	*	*	*	1		RW	NVRAM (all reads do /recall from eeprom, all writes do /store to eeprom)
0	x	x	x	1	x	x	1	1	0	x	x	x	x	x	x	x	x	x	*	*	*	*	x		RW	DUART (keep in mind that a0 is not connected)
0	x	x	x	1	x	x	1	1	1	x	x	x	x	x	x	x	x	x	x	x	0	0	*		RW	SPC flags: fifo writable (readonly, d7), spc irq suppress (readwrite, d6), fifo error status (readonly, d5), 'fifo release'/clear-tms-fifo-error-status-bits (writeonly, d1), speech initialize/clear (readwrite, d0) [see schematic sheet 4]
0	x	x	x	1	x	x	1	1	1	x	x	x	x	x	x	x	x	x	x	x	0	1	0?		W	SPC fifo write (clocks fifo)

0	x	x	x	1	x	x	1	1	1	x	x	x	x	x	x	x	x	x	x	x	1	0	*		RW	TLC flags: ring detect (readonly, d15), ring detected irq enable (readwrite, d14), answer phone (readwrite, d8), tone detected (readonly, d7), tone detected irq enable (readwrite, d6) [see schematic sheet 6]
0	x	x	x	1	x	x	1	1	1	x	x	x	x	x	x	x	x	x	x	x	1	1	*		R	TLC tone chip read, reads on bits d0-d7 only, d4-d7 are tied low; d15-d8 are probably open bus
			  |				  |				  |				  |				  |
*/

static ADDRESS_MAP_START(m68k_mem, ADDRESS_SPACE_PROGRAM, 16)
    ADDRESS_MAP_UNMAP_HIGH
    AM_RANGE(0x000000, 0x03ffff) AM_ROM AM_MIRROR(0x740000) /* ROM */
    AM_RANGE(0x080000, 0x093fff) AM_RAM AM_MIRROR(0x760000) /* RAM */
    AM_RANGE(0x094000, 0x0943ff) AM_READWRITE(led_sw_nvr_read, led_sw_nv_write) AM_MIRROR(0x763C00) /* LED array and Xicor X2212 NVRAM */
    //AM_RANGE(0x094001, 0x094001) AM_READWRITE(x2212_read, x2212_write) AM_MIRROR(0x763C00) /* Xicor X2212 NVRAM (SRAM) */
    //AM_RANGE(0x094201, 0x094201) AM_READWRITE(x2212_recall, x2212_store) AM_MIRROR(0x763C00)  /* Xicor X2212 NVRAM (EEPROM commands) */
    //AM_RANGE(0x098000, 0x09bfff) AM_READWRITE(1) AM_MIRROR(0x760000) /* DUART */
    AM_RANGE(0x09C000, 0x09C001) AM_READWRITE(m68k_spcflags_r, m68k_spcflags_w) AM_MIRROR(0x761FF8) /* SPC flags reg */
    AM_RANGE(0x09C002, 0x09C003) AM_WRITE(m68k_infifo_w) AM_MIRROR(0x761FF8) /* SPC fifo reg */
    AM_RANGE(0x09C004, 0x09C005) AM_READWRITE(m68k_tlcflags_r, m68k_tlcflags_w) AM_MIRROR(0x761FF8) /* telephone status flags */
    AM_RANGE(0x09C006, 0x09C007) AM_READ(m68k_tlc_dtmf_r) AM_MIRROR(0x761FF8) /* telephone dtmf read */
ADDRESS_MAP_END

// do we even need this below?
static ADDRESS_MAP_START(m68k_io, ADDRESS_SPACE_IO, 16)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

static ADDRESS_MAP_START(tms32010_mem, ADDRESS_SPACE_PROGRAM, 16)
    AM_RANGE(0x000, 0x7ff) AM_ROM /* ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START(tms32010_io, ADDRESS_SPACE_IO, 16)
    AM_RANGE(0, 0) AM_WRITE(spc_latch_outfifo_error_stats) // latch fifo status to be read by outfifo_status_r, and also latch the error bit at D0.
    AM_RANGE(1, 1) AM_READWRITE(spc_infifo_data_r, spc_outfifo_data_w) //read from input fifo, write to sound fifo
    AM_RANGE(TMS32010_BIO, TMS32010_BIO) AM_READ(spc_outfifo_writable_r) //read output fifo writable status
ADDRESS_MAP_END

/******************************************************************************
 Input Ports
******************************************************************************/



/******************************************************************************
 Machine Drivers
******************************************************************************/

static MACHINE_DRIVER_START(dectalk)
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", M68000, XTAL_20MHz/2)
    MDRV_CPU_PROGRAM_MAP(m68k_mem)
    MDRV_CPU_IO_MAP(m68k_io)
    MDRV_QUANTUM_TIME(HZ(10000))
    MDRV_MACHINE_RESET(dectalk)

    MDRV_CPU_ADD("dsp", TMS32010, XTAL_20MHz)
    MDRV_CPU_PROGRAM_MAP(tms32010_mem)
    MDRV_CPU_IO_MAP(tms32010_io)
    MDRV_QUANTUM_TIME(HZ(10000))

    /* video hardware */
	MDRV_DEFAULT_LAYOUT(layout_dectalk) // hack to avoid screenless system crash

    /* sound hardware */
	//MDRV_SPEAKER_STANDARD_MONO("mono")
	//MDRV_SOUND_ADD("dectalk_snd", SPEAKER, XTAL_10Khz)
	//MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)


MACHINE_DRIVER_END



/******************************************************************************
 ROM Definitions
******************************************************************************/

ROM_START( dectalk )
	ROM_REGION16_BE(0x40000,"maincpu", 0)
	// dectalk dtc-01 firmware v1.2? (first half: 23Jul84 tag; second half: 02Jul84 tag), all roms are 27128 eproms
	ROM_LOAD16_BYTE("dtc01_e5.123", 0x00000, 0x4000, CRC(03e1eefa) SHA1(e586de03e113683c2534fca1f3f40ba391193044)) // Label: "SP8510123E5" @ E8
	ROM_LOAD16_BYTE("dtc01_e5.119", 0x00001, 0x4000, CRC(af20411f) SHA1(7954bb56b7591f8954403a22d34de31c7d5441ac)) // Label: "SP8510119E5" @ E22
	ROM_LOAD16_BYTE("dtc01_e5.124", 0x08000, 0x4000, CRC(9edeafcb) SHA1(7724babf4ae5d77c0b4200f608d599058d04b25c)) // Label: "SP8510124E5" @ E7
	ROM_LOAD16_BYTE("dtc01_e5.120", 0x08001, 0x4000, CRC(f2a346a6) SHA1(af5e4ea0b3631f7d6f16c22e86a33fa2cb520ee0)) // Label: "SP8510120E5" @ E21
	ROM_LOAD16_BYTE("dtc01_e5.125", 0x10000, 0x4000, CRC(1c0100d1) SHA1(1b60cd71dfa83408b17e13f683b6bf3198c905cc)) // Label: "SP8510125E5" @ E6
	ROM_LOAD16_BYTE("dtc01_e5.121", 0x10001, 0x4000, CRC(4cb081bd) SHA1(4ad0b00628a90085cd7c78a354256c39fd14db6c)) // Label: "SP8510121E5" @ E20
	ROM_LOAD16_BYTE("dtc01_e5.126", 0x18000, 0x4000, CRC(7823dedb) SHA1(e2b2415eec838ddd46094f2fea93fd289dd0caa2)) // Label: "SP8510126E5" @ E5
	ROM_LOAD16_BYTE("dtc01_e5.122", 0x18001, 0x4000, CRC(b86370e6) SHA1(92ab22a24484ad0d0f5c8a07347105509999f3ee)) // Label: "SP8510122E5" @ E19
	ROM_LOAD16_BYTE("dtc01_e5.103", 0x20000, 0x4000, CRC(35aac6b9) SHA1(b5aec0bf37a176ff4d66d6a10357715957662ebd)) // Label: "SP8510103E5" @ E4
	ROM_LOAD16_BYTE("dtc01_e5.095", 0x20001, 0x4000, CRC(2296fe39) SHA1(891f3a3b4ce75ef14001257bc8f1f60463a9a7cb)) // Label: "SP8510095E5" @ E18
	ROM_LOAD16_BYTE("dtc01_e5.104", 0x28000, 0x4000, CRC(9658b43c) SHA1(4d6808f67cbdd316df23adc8ddf701df57aa854a)) // Label: "SP8510104E5" @ E3
	ROM_LOAD16_BYTE("dtc01_e5.096", 0x28001, 0x4000, CRC(cf236077) SHA1(496c69e52cfa013173f7b9c500ce544a03ad01f7)) // Label: "SP8510096E5" @ E17
	ROM_LOAD16_BYTE("dtc01_e5.105", 0x30000, 0x4000, CRC(09cddd28) SHA1(de0c25687bab3ff0c88c98622092e0b58331aa16)) // Label: "SP8510105E5" @ E2
	ROM_LOAD16_BYTE("dtc01_e5.097", 0x30001, 0x4000, CRC(49434da1) SHA1(c450abae0ccf372d7eb87370b8a8c97a45e164d3)) // Label: "SP8510097E5" @ E16
	ROM_LOAD16_BYTE("dtc01_e5.106", 0x38000, 0x4000, CRC(a389ab31) SHA1(355348bfc96a04193136cdde3418366e6476c3ca)) // Label: "SP8510106E5" @ E1
	ROM_LOAD16_BYTE("dtc01_e5.098", 0x38001, 0x4000, CRC(3d8910e7) SHA1(01921e77b46c2d4845023605239c45ffa4a35872)) // Label: "SP8510098E5" @ E15

	ROM_REGION(0x2000,"dsp", 0)
	// dectalk dtc-01 'klsyn' tms32010 firmware v1.2?, both proms are 82s191 equivalent
	ROM_LOAD16_BYTE("dtc01_f4.205", 0x000, 0x800, CRC(ed76a3ad) SHA1(3136bae243ef48721e21c66fde70dab5fc3c21d0)) // Label: "LM8506205F4 // M1-76161-5" @ E70
	ROM_LOAD16_BYTE("dtc01_f4.204", 0x001, 0x800, CRC(79bb54ff) SHA1(9409f90f7a397b041e4440341f2d7934cb479285)) // Label: "LM8504204F4 // 78S191" @ E69

	ROM_REGION(0x100,"nvram", 0)
	ROM_FILL(0x00, 0x100, 0xa5) /* fill with crap for now */

ROM_END

/******************************************************************************
 System Config
******************************************************************************/
static SYSTEM_CONFIG_START(dectalk)
// write me!
SYSTEM_CONFIG_END


/******************************************************************************
 Drivers
******************************************************************************/

/*    YEAR	NAME		PARENT	COMPAT	MACHINE		INPUT	INIT		CONFIG		COMPANY		FULLNAME			FLAGS */
COMP( 1984, dectalk,	0,		0,		dectalk,	0,		dectalk,	dectalk,	"DEC",		"DECTalk DTC-01",	GAME_NOT_WORKING )
