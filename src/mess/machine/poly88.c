/***************************************************************************

        Poly-88 machine by Miodrag Milanovic

        18/05/2009 Initial implementation

****************************************************************************/

#include "driver.h"
#include "includes/poly88.h"
#include "cpu/i8085/i8085.h"

static UINT8 intr = 0;
static UINT8 last_code = 0;
static UINT8 int_vector = 0;

WRITE8_HANDLER(poly_baud_rate_w)
{
	logerror("poly_baud_rate_w %02x\n",data);
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
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6" };
	
	int i;
	UINT8 code;
	UINT8 key_code = 0;
	UINT8 shift = input_port_read(machine, "LINEC") & 0x02 ? 1 : 0;
	UINT8 ctrl =  input_port_read(machine, "LINEC") & 0x01 ? 1 : 0;
	
	for(i = 0; i < 7; i++) 
	{
		
		code = 	input_port_read(machine, keynames[i]);
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
			if (i==1 && shift==1) {
				key_code = 0x20 + row_number(code) + 8*i; // for shifted numbers
			}
			if (i>=2 && i<=4 && shift==0 && ctrl==0) {
				key_code = 0x60 + row_number(code) + (i-2)*8; // for small letters
			}
			if (i>=2 && i<=4 && shift==1 && ctrl==0) {
				key_code = 0x40 + row_number(code) + (i-2)*8; // for big letters
			}
			if (i>=2 && i<=4 && ctrl==1) {
				key_code = 0x00 + row_number(code) + (i-2)*8; // for CTRL + letters
			}
			if (i==5 && shift==0 && ctrl==0) {
				if (row_number(code) < 3 || row_number(code)==7) {
					key_code = 0x60 + row_number(code) + (i-2)*8; // for small letters
				} else {
					key_code = 0x40 + row_number(code) + (i-2)*8; // for signs it is switched
				}
			}
			if (i==5 && shift==1 && ctrl==0) {
				if (row_number(code) < 3 || row_number(code)==7) {
					key_code = 0x40 + row_number(code) + (i-2)*8; // for small letters
				} else {
					key_code = 0x60 + row_number(code) + (i-2)*8; // for signs it is switched
				}
			}
			if (i==5 && shift==0 && ctrl==1) {
				key_code = 0x00 + row_number(code) + (i-2)*8; // for small letters + ctrl
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
	if (key_code==0 && last_code !=0){		
		int_vector = 0xef;				
		cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);							
	} else {
		last_code = key_code;
	}
}

static IRQ_CALLBACK (poly88_irq_callback)
{	
	return int_vector;
}

MACHINE_RESET(poly88)
{	
	cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"), poly88_irq_callback);
	timer_pulse(machine, ATTOTIME_IN_HZ(2400), NULL, 0, keyboard_callback);
	intr = 0;
	last_code = 0;
}

INTERRUPT_GEN( poly88_interrupt )
{
	int_vector = 0xf7;
	cpu_set_input_line(device, 0, HOLD_LINE);	
}
static void poly88_usart_rxready (const device_config *device, int state)
{
	int_vector = 0xe7;
	//cpu_set_input_line(device, 0, HOLD_LINE);	
}
const msm8251_interface poly88_usart_interface=
{
	NULL,
	NULL,
	poly88_usart_rxready
};

READ8_HANDLER(poly_keyboard_r)
{
	UINT8 retVal = last_code;
	cputag_set_input_line(space->machine, "maincpu", 0, CLEAR_LINE);
	last_code = 0x00;
	return retVal;
}

WRITE8_HANDLER(poly_intr_w)
{
	cputag_set_input_line(space->machine, "maincpu", 0, CLEAR_LINE);
}

SNAPSHOT_LOAD( poly88 )
{
	const address_space *space = cputag_get_address_space(image->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8* data= auto_alloc_array(image->machine, UINT8, snapshot_size);
	UINT16 recordNum;
	UINT16 recordLen;
	UINT16 address;
	UINT8  recordType;
	
	int pos = 0x300;
	char name[9];
	int i = 0;
	int theend = 0;

	image_fread(image, data, snapshot_size);
		
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
					memcpy(memory_get_read_ptr(space,address ), data + pos ,recordLen);
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
    				cpu_set_reg(cputag_get_cpu(image->machine, "maincpu"), I8085_PC, address);
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
	return INIT_PASS;
}
