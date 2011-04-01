/***************************************************************************

        SEL Z80 Trainer (LEHRSYSTEME)

        31/08/2010 Skeleton driver.

        No diagram has been found. The following is guesswork.


****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "selz80.lh"


class selz80_state : public driver_device
{
public:
	selz80_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_digit;
	UINT8 m_segment;
};



static READ8_HANDLER( selz80_00_r )
{
	return 0xff; // keyboard scan code
}

static READ8_HANDLER( selz80_01_r )
{
	return 0; // set to 1-7 to continue.
}

static WRITE8_HANDLER( selz80_01_w )
{
	selz80_state *state = space->machine().driver_data<selz80_state>();
	if ((data & 0xc0)==0x80)
	{
		state->m_digit = data & 7;
		output_set_digit_value(state->m_digit, state->m_segment);
	}
}

static WRITE8_HANDLER( selz80_00_w )
{
	selz80_state *state = space->machine().driver_data<selz80_state>();
	state->m_segment = BITSWAP8(data, 3, 2, 1, 0, 7, 6, 5, 4);
	output_set_digit_value(state->m_digit, state->m_segment);
}

static ADDRESS_MAP_START(selz80_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x1000, 0x1fff) AM_RAM
	AM_RANGE(0xa000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(selz80_io, AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_READWRITE(selz80_00_r,selz80_00_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(selz80_01_r,selz80_01_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( selz80 )
/* 2 x 16-key pads
RS SS RN MM     C(IS)  D(FL)  E(FL') F
EX BP TW TR     8(IX)  9(IY)  A(PC)  B(SP)
RL IN -  +      4(AF') 5(BC') 6(DE') 7(HL')
RG EN SA SD     0(AF)  1(BC)  2(DE)  3(HL)
  */
INPUT_PORTS_END


static MACHINE_RESET(selz80)
{
}

static MACHINE_CONFIG_START( selz80, selz80_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(selz80_mem)
	MCFG_CPU_IO_MAP(selz80_io)

	MCFG_MACHINE_RESET(selz80)

	/* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_selz80)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( selz80 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "z80-trainer.rom", 0x0000, 0x1000, CRC(eed1755f) SHA1(72e6ebfccb0e50034660bc36db1a741932311ce1))
	ROM_LOAD( "term80-a000.bin", 0xa000, 0x2000, CRC(0a58c0a7) SHA1(d1b4b3b2ad0d084175b1ff6966653d8b20025252))
	ROM_LOAD( "term80-e000.bin", 0xe000, 0x2000, CRC(158e08e6) SHA1(f1add43bcf8744a01238fb893ee284872d434db5))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1985, selz80,  0,       0,	selz80, 	selz80, 	0,	 "SEL",   "Z80 Trainer",		GAME_NOT_WORKING | GAME_NO_SOUND)

