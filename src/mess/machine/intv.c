#include "emu.h"
#include "includes/intv.h"
#include "cpu/cp1610/cp1610.h"
#include "image.h"
#include "hashfile.h"

#define INTELLIVOICE_MASK	0x02
#define ECS_MASK			0x01



WRITE16_HANDLER ( intvkbd_dualport16_w )
{
	intv_state *state = space->machine().driver_data<intv_state>();
	unsigned char *RAM;

	COMBINE_DATA(&state->m_intvkbd_dualport_ram[offset]);

	/* copy the LSB over to the 6502 OP RAM, in case they are opcodes */
	RAM	 = space->machine().region("keyboard")->base();
	RAM[offset] = (UINT8) (data >> 0);
}

READ8_HANDLER ( intvkbd_dualport8_lsb_r )
{
	intv_state *state = space->machine().driver_data<intv_state>();
	return (UINT8) (state->m_intvkbd_dualport_ram[offset] >> 0);
}

WRITE8_HANDLER ( intvkbd_dualport8_lsb_w )
{
	intv_state *state = space->machine().driver_data<intv_state>();
	unsigned char *RAM;

	state->m_intvkbd_dualport_ram[offset] &= ~0x00FF;
	state->m_intvkbd_dualport_ram[offset] |= ((UINT16) data) << 0;

	/* copy over to the 6502 OP RAM, in case they are opcodes */
	RAM	 = space->machine().region("keyboard")->base();
	RAM[offset] = data;
}



READ8_HANDLER ( intvkbd_dualport8_msb_r )
{
	intv_state *state = space->machine().driver_data<intv_state>();
	unsigned char rv;

	if (offset < 0x100)
	{
		switch (offset)
		{
			case 0x000:
				rv = input_port_read(space->machine(), "TEST") & 0x80;
				logerror("TAPE: Read %02x from 0x40%02x - XOR Data?\n",rv,offset);
				break;
			case 0x001:
				rv = (input_port_read(space->machine(), "TEST") & 0x40) << 1;
				logerror("TAPE: Read %02x from 0x40%02x - Sense 1?\n",rv,offset);
				break;
			case 0x002:
				rv = (input_port_read(space->machine(), "TEST") & 0x20) << 2;
				logerror("TAPE: Read %02x from 0x40%02x - Sense 2?\n",rv,offset);
				break;
			case 0x003:
				rv = (input_port_read(space->machine(), "TEST") & 0x10) << 3;
				logerror("TAPE: Read %02x from 0x40%02x - Tape Present\n",rv,offset);
				break;
			case 0x004:
				rv = (input_port_read(space->machine(), "TEST") & 0x08) << 4;
				logerror("TAPE: Read %02x from 0x40%02x - Comp (339/1)\n",rv,offset);
				break;
			case 0x005:
				rv = (input_port_read(space->machine(), "TEST") & 0x04) << 5;
				logerror("TAPE: Read %02x from 0x40%02x - Clocked Comp (339/13)\n",rv,offset);
				break;
			case 0x006:
				if (state->m_sr1_int_pending)
					rv = 0x00;
				else
					rv = 0x80;
				logerror("TAPE: Read %02x from 0x40%02x - SR1 Int Pending\n",rv,offset);
				break;
			case 0x007:
				if (state->m_tape_int_pending)
					rv = 0x00;
				else
					rv = 0x80;
				logerror("TAPE: Read %02x from 0x40%02x - Tape? Int Pending\n",rv,offset);
				break;
			case 0x060:	/* Keyboard Read */
				rv = 0xff;
				if (state->m_intvkbd_keyboard_col == 0)
					rv = input_port_read(space->machine(), "ROW0");
				if (state->m_intvkbd_keyboard_col == 1)
					rv = input_port_read(space->machine(), "ROW1");
				if (state->m_intvkbd_keyboard_col == 2)
					rv = input_port_read(space->machine(), "ROW2");
				if (state->m_intvkbd_keyboard_col == 3)
					rv = input_port_read(space->machine(), "ROW3");
				if (state->m_intvkbd_keyboard_col == 4)
					rv = input_port_read(space->machine(), "ROW4");
				if (state->m_intvkbd_keyboard_col == 5)
					rv = input_port_read(space->machine(), "ROW5");
				if (state->m_intvkbd_keyboard_col == 6)
					rv = input_port_read(space->machine(), "ROW6");
				if (state->m_intvkbd_keyboard_col == 7)
					rv = input_port_read(space->machine(), "ROW7");
				if (state->m_intvkbd_keyboard_col == 8)
					rv = input_port_read(space->machine(), "ROW8");
				if (state->m_intvkbd_keyboard_col == 9)
					rv = input_port_read(space->machine(), "ROW9");
				break;
			case 0x80:
				rv = 0x00;
				logerror("TAPE: Read %02x from 0x40%02x, clear tape int pending\n",rv,offset);
				state->m_tape_int_pending = 0;
				break;
			case 0xa0:
				rv = 0x00;
				logerror("TAPE: Read %02x from 0x40%02x, clear SR1 int pending\n",rv,offset);
				state->m_sr1_int_pending = 0;
				break;
			case 0xc0:
			case 0xc1:
			case 0xc2:
			case 0xc3:
			case 0xc4:
			case 0xc5:
			case 0xc6:
			case 0xc7:
			case 0xc8:
			case 0xc9:
			case 0xca:
			case 0xcb:
			case 0xcc:
			case 0xcd:
			case 0xce:
			case 0xcf:
				/* TMS9927 regs */
				rv = intvkbd_tms9927_r(space, offset-0xc0);
				break;
			default:
				rv = (state->m_intvkbd_dualport_ram[offset]&0x0300)>>8;
				logerror("Unknown read %02x from 0x40%02x\n",rv,offset);
				break;
		}
		return rv;
	}
	else
		return (state->m_intvkbd_dualport_ram[offset]&0x0300)>>8;
}

static const char *const tape_motor_mode_desc[8] =
{
	"IDLE", "IDLE", "IDLE", "IDLE",
	"EJECT", "PLAY/RECORD", "REWIND", "FF"
};

WRITE8_HANDLER ( intvkbd_dualport8_msb_w )
{
	intv_state *state = space->machine().driver_data<intv_state>();
	unsigned int mask;

	if (offset < 0x100)
	{
		switch (offset)
		{
			case 0x020:
				state->m_tape_motor_mode &= 3;
				if (data & 1)
					state->m_tape_motor_mode |= 4;
				logerror("TAPE: Motor Mode: %s\n",tape_motor_mode_desc[state->m_tape_motor_mode]);
				break;
			case 0x021:
				state->m_tape_motor_mode &= 5;
				if (data & 1)
					state->m_tape_motor_mode |= 2;
				logerror("TAPE: Motor Mode: %s\n",tape_motor_mode_desc[state->m_tape_motor_mode]);
				break;
			case 0x022:
				state->m_tape_motor_mode &= 6;
				if (data & 1)
					state->m_tape_motor_mode |= 1;
				logerror("TAPE: Motor Mode: %s\n",tape_motor_mode_desc[state->m_tape_motor_mode]);
				break;
			case 0x023:
			case 0x024:
			case 0x025:
			case 0x026:
			case 0x027:
				state->m_tape_unknown_write[offset - 0x23] = (data & 1);
				break;
			case 0x040:
				state->m_tape_unknown_write[5] = (data & 1);
				break;
			case 0x041:
				if (data & 1)
					logerror("TAPE: Tape Interrupts Enabled\n");
				else
					logerror("TAPE: Tape Interrupts Disabled\n");
				state->m_tape_interrupts_enabled = (data & 1);
				break;
			case 0x042:
				if (data & 1)
					logerror("TAPE: Cart Bus Interrupts Disabled\n");
				else
					logerror("TAPE: Cart Bus Interrupts Enabled\n");
				break;
			case 0x043:
				if (data & 0x01)
					state->m_intvkbd_text_blanked = 0;
				else
					state->m_intvkbd_text_blanked = 1;
				break;
			case 0x044:
				state->m_intvkbd_keyboard_col &= 0x0e;
				state->m_intvkbd_keyboard_col |= (data&0x01);
				break;
			case 0x045:
				state->m_intvkbd_keyboard_col &= 0x0d;
				state->m_intvkbd_keyboard_col |= ((data&0x01)<<1);
				break;
			case 0x046:
				state->m_intvkbd_keyboard_col &= 0x0b;
				state->m_intvkbd_keyboard_col |= ((data&0x01)<<2);
				break;
			case 0x047:
				state->m_intvkbd_keyboard_col &= 0x07;
				state->m_intvkbd_keyboard_col |= ((data&0x01)<<3);
				break;
			case 0x80:
				logerror("TAPE: Write to 0x40%02x, clear tape int pending\n",offset);
				state->m_tape_int_pending = 0;
				break;
			case 0xa0:
				logerror("TAPE: Write to 0x40%02x, clear SR1 int pending\n",offset);
				state->m_sr1_int_pending = 0;
				break;
			case 0xc0:
			case 0xc1:
			case 0xc2:
			case 0xc3:
			case 0xc4:
			case 0xc5:
			case 0xc6:
			case 0xc7:
			case 0xc8:
			case 0xc9:
			case 0xca:
			case 0xcb:
			case 0xcc:
			case 0xcd:
			case 0xce:
			case 0xcf:
				/* TMS9927 regs */
				intvkbd_tms9927_w(space, offset-0xc0, data);
				break;
			default:
				logerror("%04X: Unknown write %02x to 0x40%02x\n",cpu_get_pc(&space->device()),data,offset);
				break;
		}
	}
	else
	{
		mask = state->m_intvkbd_dualport_ram[offset] & 0x00ff;
		state->m_intvkbd_dualport_ram[offset] = mask | ((data<<8)&0x0300);
	}
}

READ16_HANDLER( intv_gram_r )
{
	intv_state *state = space->machine().driver_data<intv_state>();
	//logerror("read: %d = GRAM(%d)\n",state->m_gram[offset],offset);
	return (int)state->m_gram[offset];
}

WRITE16_HANDLER( intv_gram_w )
{
	intv_state *state = space->machine().driver_data<intv_state>();
    data &= 0xFF;

	state->m_gram[offset] = data;
	state->m_gramdirtybytes[offset] = 1;
    state->m_gramdirty = 1;
}


READ16_HANDLER( intv_ram8_r )
{
	intv_state *state = space->machine().driver_data<intv_state>();
	//logerror("%x = ram8_r(%x)\n",state->m_ram8[offset],offset);
	return (int)state->m_ram8[offset];
}

WRITE16_HANDLER( intv_ram8_w )
{
	intv_state *state = space->machine().driver_data<intv_state>();
	//logerror("ram8_w(%x) = %x\n",offset,data);
	state->m_ram8[offset] = data&0xff;
}


READ16_HANDLER( intv_ram16_r )
{
	intv_state *state = space->machine().driver_data<intv_state>();
	//logerror("%x = ram16_r(%x)\n",state->m_ram16[offset],offset);
	return (int)state->m_ram16[offset];
}

WRITE16_HANDLER( intv_ram16_w )
{
	intv_state *state = space->machine().driver_data<intv_state>();
	//logerror("%g: WRITING TO GRAM offset = %d\n",machine.time(),offset);
	//logerror("ram16_w(%x) = %x\n",offset,data);
	state->m_ram16[offset] = data&0xffff;
}

static int intv_load_rom_file(device_image_interface &image)
{
    int i,j;

	UINT8 temp;
	UINT8 num_segments;
	UINT8 start_seg;
	UINT8 end_seg;

	UINT32 current_address;
	UINT32 end_address;

	UINT8 high_byte;
	UINT8 low_byte;

	UINT8 *memory = image.device().machine().region("maincpu")->base();
	const char *filetype = image.filetype();

	/* if it is in .rom format, we enter here */
	if (!mame_stricmp (filetype, "rom"))
	{
		image.fread( &temp, 1);			/* header */
		if (temp != 0xa8)
		{
			return IMAGE_INIT_FAIL;
		}

		image.fread( &num_segments, 1);

		image.fread( &temp, 1);
		if (temp != (num_segments ^ 0xff))
		{
			return IMAGE_INIT_FAIL;
		}

		for (i = 0; i < num_segments; i++)
		{
			image.fread( &start_seg, 1);
			current_address = start_seg * 0x100;

			image.fread( &end_seg, 1);
			end_address = end_seg * 0x100 + 0xff;

			while (current_address <= end_address)
			{
				image.fread( &low_byte, 1);
				memory[(current_address << 1) + 1] = low_byte;
				image.fread( &high_byte, 1);
				memory[current_address << 1] = high_byte;
				current_address++;
			}

			/* Here we should calculate and compare the CRC16... */
			image.fread( &temp, 1);
			image.fread( &temp, 1);
		}

		/* Access tables and fine address restriction tables are not supported ATM */
		for (i = 0; i < (16 + 32 + 2); i++)
		{
			image.fread( &temp, 1);
		}
		return IMAGE_INIT_PASS;
	}
	/* otherwise, we load it as a .bin file, using extrainfo from intv.hsi in place of .cfg */
	else
	{
		/* This code is a blatant hack, due to impossibility to load a separate .cfg file in MESS. */
		/* It shall be eventually replaced by the .xml loading */

		/* extrainfo format */
		// 1. mapper number (to deal with bankswitch). no bankswitch is mapper 0 (most games).
		// 2.->5. current images have at most 4 chunks of data. we store here block size and location to load
		//  (value & 0xf0) >> 4 is the location / 0x1000
		//  (value & 0x0f) is the size / 0x800
		// 6. some images have a ram chunk. as above we store location and size in 8 bits
		// 7. extra = 1 ECS, 2 Intellivoice
		int start, size;
		int mapper, rom[5], ram, extra;
		const char *extrainfo = hashfile_extrainfo(image);
		if (!extrainfo)
		{
			/* If no extrainfo, we assume a single 0x2000 chunk at 0x5000 */
			for (i = 0; i < 0x2000; i++ )
			{
				image.fread( &low_byte, 1);
				memory[((0x5000 + i) << 1) + 1] = low_byte;
				image.fread( &high_byte, 1);
				memory[(0x5000 + i) << 1] = high_byte;
			}
		}
		else
		{
			sscanf(extrainfo,"%d %d %d %d %d %d %d", &mapper, &rom[0], &rom[1], &rom[2],
																&rom[3], &ram, &extra);

//          logerror("extrainfo: %d %d %d %d %d %d %d \n", mapper, rom[0], rom[1], rom[2],
//                                                              rom[3], ram, extra);

			if (mapper)
			{
				logerror("Bankswitch not yet implemented! \n");
			}

			if (ram)
			{
				logerror("RAM banks not yet implemented! \n");
			}

			if (extra & INTELLIVOICE_MASK)
			{
				logerror("Intellivoice support not yet implemented! \n");
			}

			if (extra & ECS_MASK)
			{
				logerror("ECS support is only partial\n");
				logerror("(even with the intvkbd driver)! \n");
			}

			for (j = 0; j < 4; j++)
			{
				start = (( rom[j] & 0xf0 ) >> 4) * 0x1000;
				size = ( rom[j] & 0x0f ) * 0x800;

				/* some cart has to be loaded to 0x4800, but none goes to 0x4000. Hence, we use */
				/* 0x04 << 4 in extrainfo (to reduce the stored values) and fix the value here. */
				if (start == 0x4000) start += 0x800;

//              logerror("step %d: %d %d \n", j, start / 0x1000, size / 0x1000);

				for (i = 0; i < size; i++ )
				{
					image.fread( &low_byte, 1);
					memory[((start + i) << 1) + 1] = low_byte;
					image.fread( &high_byte, 1);
					memory[(start + i) << 1] = high_byte;
				}
			}
		}

		return IMAGE_INIT_PASS;
	}
}

DEVICE_IMAGE_LOAD( intv_cart )
{
	return intv_load_rom_file(image);
}

#ifdef UNUSED_FUNCTION
DRIVER_INIT( intv )
{
}
#endif

/* Set Reset and INTR/INTRM Vector */
MACHINE_RESET( intv )
{
	device_set_input_line_vector(machine.device("maincpu"), CP1610_RESET, 0x1000);

	/* These are actually the same vector, and INTR is unused */
	device_set_input_line_vector(machine.device("maincpu"), CP1610_INT_INTRM, 0x1004);
	device_set_input_line_vector(machine.device("maincpu"), CP1610_INT_INTR,  0x1004);

	/* Set initial PC */
	cpu_set_reg(machine.device("maincpu"), CP1610_R7, 0x1000);

	return;
}


static TIMER_CALLBACK(intv_interrupt_complete)
{
	cputag_set_input_line(machine, "maincpu", CP1610_INT_INTRM, CLEAR_LINE);
}

INTERRUPT_GEN( intv_interrupt )
{
	intv_state *state = device->machine().driver_data<intv_state>();
	cputag_set_input_line(device->machine(), "maincpu", CP1610_INT_INTRM, ASSERT_LINE);
	state->m_sr1_int_pending = 1;
	device->machine().scheduler().timer_set(device->machine().device<cpu_device>("maincpu")->cycles_to_attotime(3791), FUNC(intv_interrupt_complete));
	intv_stic_screenrefresh(device->machine());
}

static const UINT8 controller_table[] =
{
	0x81, 0x41, 0x21, 0x82, 0x42, 0x22, 0x84, 0x44,
	0x24, 0x88, 0x48, 0x28, 0xa0, 0x60, 0xc0, 0x00,
	0x04, 0x16, 0x02, 0x13, 0x01, 0x19, 0x08, 0x1c
};

READ8_HANDLER( intv_right_control_r )
{
	UINT8 rv = 0x00;

	int bit, byte;
	int value=0;

	for(byte=0; byte<3; byte++)
	{
		switch(byte)
		{
			case 0:
				value = input_port_read(space->machine(), "IN0");
				break;
			case 1:
				value = input_port_read(space->machine(), "IN1");
				break;
			case 2:
				value = input_port_read(space->machine(), "IN2");
				break;
		}
		for(bit=7; bit>=0; bit--)
		{
			if (value & (1<<bit))
			{
				rv |= controller_table[byte*8+(7-bit)];
			}
		}
	}
	return rv ^ 0xff;
}

READ8_HANDLER( intv_left_control_r )
{
	return intv_right_control_r(space, 0 ); //0xff; /* small patch to allow Frogger to be played */
}

/* Intellivision console + keyboard component */

DEVICE_IMAGE_LOAD( intvkbd_cart )
{
	if (strcmp(image.device().tag(),"cart1") == 0) /* Legacy cartridge slot */
	{
		/* First, initialize these as empty so that the intellivision
         * will think that the playcable is not attached */
		UINT8 *memory = image.device().machine().region("maincpu")->base();

		/* assume playcable is absent */
		memory[0x4800 << 1] = 0xff;
		memory[(0x4800 << 1) + 1] = 0xff;

		intv_load_rom_file(image);
	}

	if (strcmp(image.device().tag(),"cart2") == 0) /* Keyboard component cartridge slot */
	{
		UINT8 *memory = image.device().machine().region("keyboard")->base();

		/* Assume an 8K cart, like BASIC */
		image.fread( &memory[0xe000], 0x2000);
	}

	return IMAGE_INIT_PASS;

}
