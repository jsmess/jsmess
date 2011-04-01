/***************************************************************************

        Poly-88 machine by Miodrag Milanovic

        18/05/2009 Initial implementation

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "imagedev/cassette.h"
#include "includes/poly88.h"


static TIMER_CALLBACK(poly88_usart_timer_callback)
{
	poly88_state *state = machine.driver_data<poly88_state>();
	state->m_int_vector = 0xe7;
	device_set_input_line(machine.device("maincpu"), 0, HOLD_LINE);
}

WRITE8_HANDLER(poly88_baud_rate_w)
{
	poly88_state *state = space->machine().driver_data<poly88_state>();
	logerror("poly88_baud_rate_w %02x\n",data);
	state->m_usart_timer = space->machine().scheduler().timer_alloc(FUNC(poly88_usart_timer_callback));
	state->m_usart_timer->adjust(attotime::zero, 0, attotime::from_hz(300));

}

static UINT8 row_number(UINT8 code) {
	if BIT(code,0) return 0;
	if BIT(code,1) return 1;
	if BIT(code,2) return 2;
	if BIT(code,3) return 3;
	if BIT(code,4) return 4;
	if BIT(code,5) return 5;
	if BIT(code,6) return 6;
	if BIT(code,7) return 7;
	return 0;
}

static TIMER_CALLBACK(keyboard_callback)
{
	poly88_state *state = machine.driver_data<poly88_state>();
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6" };

	int i;
	UINT8 code;
	UINT8 key_code = 0;
	UINT8 shift = input_port_read(machine, "LINEC") & 0x02 ? 1 : 0;
	UINT8 ctrl =  input_port_read(machine, "LINEC") & 0x01 ? 1 : 0;

	for(i = 0; i < 7; i++)
	{

		code =	input_port_read(machine, keynames[i]);
		if (code != 0)
		{
			if (i==0 && shift==0) {
				key_code = 0x30 + row_number(code) + 8*i; // for numbers and some signs
			}
			if (i==0 && shift==1) {
				key_code = 0x20 + row_number(code) + 8*i; // for shifted numbers
			}
			if (i==1 && shift==0) {
				if (row_number(code) < 4) {
					key_code = 0x30 + row_number(code) + 8*i; // for numbers and some signs
				} else {
					key_code = 0x20 + row_number(code) + 8*i; // for numbers and some signs
				}
			}
			if (i==1 && shift==1) {
				if (row_number(code) < 4) {
					key_code = 0x20 + row_number(code) + 8*i; // for numbers and some signs
				} else {
					key_code = 0x30 + row_number(code) + 8*i; // for numbers and some signs
				}
			}
			if (i>=2 && i<=4 && shift==1 && ctrl==0) {
				key_code = 0x60 + row_number(code) + (i-2)*8; // for small letters
			}
			if (i>=2 && i<=4 && shift==0 && ctrl==0) {
				key_code = 0x40 + row_number(code) + (i-2)*8; // for big letters
			}
			if (i>=2 && i<=4 && ctrl==1) {
				key_code = 0x00 + row_number(code) + (i-2)*8; // for CTRL + letters
			}
			if (i==5 && shift==1 && ctrl==0) {
				if (row_number(code)<7) {
					key_code = 0x60 + row_number(code) + (i-2)*8; // for small letters
				} else {
					key_code = 0x40 + row_number(code) + (i-2)*8; // for signs it is switched
				}
			}
			if (i==5 && shift==0 && ctrl==0) {
				if (row_number(code)<7) {
					key_code = 0x40 + row_number(code) + (i-2)*8; // for small letters
				} else {
					key_code = 0x60 + row_number(code) + (i-2)*8; // for signs it is switched
				}
			}
			if (i==5 && shift==0 && ctrl==1) {
				key_code = 0x00 + row_number(code) + (i-2)*8; // for letters + ctrl
			}
			if (i==6) {
				switch(row_number(code))
				{
					case 0: key_code = 0x11; break;
					case 1: key_code = 0x12; break;
					case 2: key_code = 0x13; break;
					case 3: key_code = 0x14; break;
					case 4: key_code = 0x20; break; // Space
					case 5: key_code = 0x0D; break; // Enter
					case 6: key_code = 0x09; break; // TAB
					case 7: key_code = 0x0A; break; // LF
				}
			}
		}
	}
	if (key_code==0 && state->m_last_code !=0){
		state->m_int_vector = 0xef;
		cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
	} else {
		state->m_last_code = key_code;
	}
}

static IRQ_CALLBACK (poly88_irq_callback)
{
	poly88_state *state = device->machine().driver_data<poly88_state>();
	return state->m_int_vector;
}



static void poly88_cassette_write(running_machine &machine, int id, unsigned long state)
{
	poly88_state *drvstate = machine.driver_data<poly88_state>();
	drvstate->m_cassette_serial_connection.input_state = state;
}

static TIMER_CALLBACK(poly88_cassette_timer_callback)
{
	poly88_state *state = machine.driver_data<poly88_state>();
	int data;
	int current_level;


//  if (!(input_port_read(machine, "DSW0") & 0x02)) /* V.24 / Tape Switch */
	//{
		/* tape reading */
		if (cassette_get_state(machine.device("cassette"))&CASSETTE_PLAY)
		{
					if (state->m_clk_level_tape)
					{
						state->m_previous_level = (cassette_input(machine.device("cassette")) > 0.038) ? 1 : 0;
						state->m_clk_level_tape = 0;
					}
					else
					{
						current_level = (cassette_input(machine.device("cassette")) > 0.038) ? 1 : 0;

						if (state->m_previous_level!=current_level)
						{
							data = (!state->m_previous_level && current_level) ? 1 : 0;
//data = current_level;
							set_out_data_bit(state->m_cassette_serial_connection.State, data);
							serial_connection_out(machine, &state->m_cassette_serial_connection);
							msm8251_receive_clock(machine.device("uart"));

							state->m_clk_level_tape = 1;
						}
					}
		}

		/* tape writing */
		if (cassette_get_state(machine.device("cassette"))&CASSETTE_RECORD)
		{
			data = get_in_data_bit(state->m_cassette_serial_connection.input_state);
			data ^= state->m_clk_level_tape;
			cassette_output(machine.device("cassette"), data&0x01 ? 1 : -1);

			if (!state->m_clk_level_tape)
				msm8251_transmit_clock(machine.device("uart"));

			state->m_clk_level_tape = state->m_clk_level_tape ? 0 : 1;

			return;
		}

		state->m_clk_level_tape = 1;

		if (!state->m_clk_level)
			msm8251_transmit_clock(machine.device("uart"));
		state->m_clk_level = state->m_clk_level ? 0 : 1;
//  }
}


static TIMER_CALLBACK( setup_machine_state )
{
	poly88_state *state = machine.driver_data<poly88_state>();
	msm8251_connect(machine.device("uart"), &state->m_cassette_serial_connection);
}

DRIVER_INIT ( poly88 )
{
	poly88_state *state = machine.driver_data<poly88_state>();
	state->m_previous_level = 0;;
	state->m_clk_level = state->m_clk_level_tape = 1;
	state->m_cassette_timer = machine.scheduler().timer_alloc(FUNC(poly88_cassette_timer_callback));
	state->m_cassette_timer->adjust(attotime::zero, 0, attotime::from_hz(600));

	serial_connection_init(machine, &state->m_cassette_serial_connection);
	serial_connection_set_in_callback(machine, &state->m_cassette_serial_connection, poly88_cassette_write);

	machine.scheduler().timer_pulse(attotime::from_hz(24000), FUNC(keyboard_callback));
}

MACHINE_RESET(poly88)
{
	poly88_state *state = machine.driver_data<poly88_state>();
	device_set_irq_callback(machine.device("maincpu"), poly88_irq_callback);
	state->m_intr = 0;
	state->m_last_code = 0;

	machine.scheduler().timer_set(attotime::zero, FUNC(setup_machine_state));
}

INTERRUPT_GEN( poly88_interrupt )
{
	poly88_state *state = device->machine().driver_data<poly88_state>();
	state->m_int_vector = 0xf7;
	device_set_input_line(device, 0, HOLD_LINE);
}

static WRITE_LINE_DEVICE_HANDLER( poly88_usart_rxready )
{
	//poly88_state *drvstate = device->machine().driver_data<poly88_state>();
	//drvstate->m_int_vector = 0xe7;
	//device_set_input_line(device, 0, HOLD_LINE);
}

const msm8251_interface poly88_usart_interface=
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(poly88_usart_rxready),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

READ8_HANDLER(poly88_keyboard_r)
{
	poly88_state *state = space->machine().driver_data<poly88_state>();
	UINT8 retVal = state->m_last_code;
	cputag_set_input_line(space->machine(), "maincpu", 0, CLEAR_LINE);
	state->m_last_code = 0x00;
	return retVal;
}

WRITE8_HANDLER(poly88_intr_w)
{
	cputag_set_input_line(space->machine(), "maincpu", 0, CLEAR_LINE);
}

SNAPSHOT_LOAD( poly88 )
{
	address_space *space = image.device().machine().device("maincpu")->memory().space(AS_PROGRAM);
	UINT8* data= auto_alloc_array(image.device().machine(), UINT8, snapshot_size);
	UINT16 recordNum;
	UINT16 recordLen;
	UINT16 address;
	UINT8  recordType;

	int pos = 0x300;
	char name[9];
	int i = 0;
	int theend = 0;

	image.fread( data, snapshot_size);

	while (pos<snapshot_size) {
		for(i=0;i<9;i++) {
			name[i] = (char) data[pos + i];
		}
		pos+=8;
		name[8] = 0;


		recordNum = data[pos]+ data[pos+1]*256; pos+=2;
		recordLen = data[pos]; pos++;
		if (recordLen==0) recordLen=0x100;
		address = data[pos] + data[pos+1]*256; pos+=2;
		recordType = data[pos]; pos++;

		logerror("Block :%s number:%d length: %d address=%04x type:%d\n",name,recordNum,recordLen,address, recordType);
		switch(recordType) {
			case 0 :
					/* 00 Absolute */
					memcpy(space->get_read_ptr(address ), data + pos ,recordLen);
					break;
			case 1 :
					/* 01 Comment */
					break;
			case 2 :
					/* 02 End */
					theend = 1;
					break;
			case 3 :
    				/* 03 Auto Start @ Address */
    				cpu_set_reg(image.device().machine().device("maincpu"), I8085_PC, address);
    				theend = 1;
    				break;
    		case 4 :
    				/* 04 Data ( used by Assembler ) */
    				logerror("ASM load unsupported\n");
    				theend = 1;
    				break;
    		case 5 :
    				/* 05 BASIC program file */
    				logerror("BASIC load unsupported\n");
    				theend = 1;
    				break;
    		case 6 :
    				/* 06 End ( used by Assembler? ) */
    				theend = 1;
					break;
			default: break;
		}

		if (theend) {
			break;
		}
		pos+=recordLen;
	}
	image.device().machine().device("uart")->reset();
	return IMAGE_INIT_PASS;
}
