/***************************************************************************

	Zenith Z-100

	15/07/2011 Skeleton driver.

	TODO:
	- remove parity check IRQ patch (understand what it really wants there!)

============================================================================

Z207A		EQU 0B0H	; Z-207 disk controller base port
  ; (See DEFZ207 to program controller)
Z217A		EQU 0AEH	; Z-217 disk controller base port
  ; (See DEFZ217 to program controller)
ZGRNSEG		EQU 0E000H	; Segment of green video plane
ZREDSEG		EQU 0D000H	; Segment of red video plane
ZBLUSEG		EQU 0C000H	; Segment of blue video plane
ZVIDEO		EQU 0D8H	; Video 68A21 port
  ; PA0 -> enable red display
  ; PA1 -> enable green display
  ; PA2 -> enable blue display
  ; PA3 -> not flash screen
  ; PA4 -> not write multiple red
  ; PA5 -> not write multiple green
  ; PA6 -> not write multiple blue
  ; PA7 -> disable video RAM
  ; PB7-PB0 -> LA15-LA8
  ; CA1 - not used
  ; CA2 -> clear screen
  ; CB1 - not used
  ; CB2 -> value to write (0 or 1) on clear screen
  ; (see DEF6821 to program the 6821)
ZCRTC		EQU 0DCH	; Video 6845 CRT-C port
  ; (see DEF6845 to program the 6845)
ZLPEN		EQU 0DEH	; Light pen latch
  ZLPEN_BIT	  EQU 00000111B	  ; Bit hit by pen
  ZLPEN_ROW	  EQU 11110000B	  ; Row hit by pen
ZPIA		EQU 0E0H	; Parallel printer plus light pen and
                                ;  video vertical retrace 68A21 port
  ; PA0 -> PDATA1
  ; PA1 -> PDATA2
  ; PA2 -> not STROBE
  ; PA3 -> not INIT
  ; PA4 <- VSYNC
  ; PA5 -> clear VSYNC flip flop
  ; PA6 <- light pen switch
  ; PA7 -> clear light pen flip flop
  ; PB0 <- BUSY
  ; PB1 <- not ERROR
  ; PB2 -> PDATA3
  ; PB3 -> PDATA4
  ; PB4 -> PDATA5
  ; PB5 -> PDATA6
  ; PB6 -> PDATA7
  ; PB7 -> PDATA8
  ; CA1 <- light pen hit (from flip flop)
  ; CA2 <- VSYNC (from flip flop)
  ; CB1 <- not ACKNLG
  ; CB2 <- BUSY
  ; (See DEF6821 to program the PIA)
ZTIMER		EQU 0E4H	; Timer 8253 port
  ZTIMEVAL	  EQU 2500	  ; 100ms divide by N value
  ; (See DEF8253 to program the 8253)
ZTIMERS		EQU 0FBH	; Timer interrupt status port
  ZTIMERS0	  EQU 001H	  ; Timer 0 interrupt
  ZTIMERS2	  EQU 002H	  ; Timer 2 interrupt
ZSERA		EQU 0E8H	; First 2661-2 serial port
ZSERB		EQU 0ECH	; Second 2661-2 serial port
  ; (See DEFEP2 to program 2661-2)
ZM8259A		EQU 0F2H	; Master 8259A interrupt controller port
  ZINTEI	  EQU 0		  ; Parity error or S-100 pin 98 interrupt
  ZINTPS	  EQU 1		  ; Processor swap interrupt
  ZINTTIM	  EQU 2		  ; Timer interrupt
  ZINTSLV	  EQU 3		  ; Slave 8259A interrupt
  ZINTSA	  EQU 4		  ; Serial port A interrupt
  ZINTSB	  EQU 5		  ; Serial port B interrupt
  ZINTKD	  EQU 6		  ; Keyboard, Display, or Light pen interrupt
  ZINTPP	  EQU 7		  ; Parallel port interrupt
  ; (See DEF8259A to program the 8259A)
  ZM8259AI	  EQU 64	    ; Base interrupt number for master
ZS8259A		EQU 0F0H	; Secondary 8259A interrupt controller port
  ZS8259AI	  EQU 72	    ; Base interrupt number for slave
  BIOSAI	  EQU ZS8259AI+8    ; Base of BIOS generated interrupts
ZKEYBRD		EQU 0F4H	; Keyboard port
  ZKEYBRDD	  EQU ZKEYBRD+0	  ; Keyboard data port
  ZKEYBRDC	  EQU ZKEYBRD+1	  ; Keyboard command port
    ZKEYRES	    EQU 0	    ; Reset command
    ZKEYARD	    EQU 1	    ; Autorepeat on command
    ZKEYARF	    EQU 2	    ; Autorepeat off command
    ZKEYKCO	    EQU 3	    ; Key click on command
    ZKEYKCF	    EQU 4	    ; Key click off command
    ZKEYCF	    EQU 5	    ; Clear keyboard FIFO command
    ZKEYCLK	    EQU 6	    ; Generate a click sound command
    ZKEYBEP	    EQU 7	    ; Generate a beep sound command
    ZKEYEK	    EQU 8	    ; Enable keyboard command
    ZKEYDK	    EQU 9	    ; Disable keyboard command
    ZKEYUDM	    EQU 10	    ; Enter UP/DOWN mode command
    ZKEYNSM	    EQU 11	    ; Enter normal scan mode command
    ZKEYEI	    EQU 12	    ; Enable keyboard interrupts command
    ZKEYDI	    EQU 13	    ; Disable keyboard interrupts command
  ZKEYBRDS	  EQU ZKEYBRD+1	  ; Keyboard status port
    ZKEYOBF	    EQU 001H	    ; Output buffer not empty
    ZKEYIBF	    EQU 002H	    ; Input buffer full
ZMCL		EQU 0FCH	; Memory control latch
  ZMCLMS	  EQU 00000011B	  ; Map select mask
    ZSM0	    EQU 0	    ; Map select 0
    ZSM1	    EQU 1	    ; Map select 1
    ZSM2	    EQU 2	    ; Map select 2
    ZSM3	    EQU 3	    ; Map select 3
  ZMCLRM	  EQU 00001100B	  ; Monitor ROM mapping mask
    ZRM0	    EQU 0*4 	    ; Power up mode - ROM everywhere on reads
    ZRM1	    EQU 1*4 	    ; ROM at top of every 64K page
    ZRM2	    EQU 2*4 	    ; ROM at top of 8088's addr space
    ZRM3	    EQU 3*4 	    ; Disable ROM
  ZMCLPZ	  EQU 00010000B	  ; 0=Set Parity to the zero state
  ZMCLPK	  EQU 00100000B	  ; 0=Disable parity checking circuity
  ZMCLPF	  EQU 01000000B	  ; 0=Disable parity error flag
Z205BA		EQU 098H	; Base address for Z-205 boards
  Z205BMC	  EQU 8		  ; Maximum of 8 Z-205 boards installed
ZHAL		EQU 0FDH	; Hi-address latch
  ZHAL85	  EQU 0FFH	  ; 8085 Mask
  ZHAL88	  EQU 0F0H	  ; 8088 Mask
ZPSP		EQU 0FEH	; Processor swap port
  ZPSPPS	  EQU 10000000B   ; Processor select (0=8085, 1=8088)
  ZPSPPS5	  EQU 00000000B	  ; Select 8085
  ZPSPPS8	  EQU 10000000B   ; Select 8088
  ZPSPSI	  EQU 00000010B   ; Generate interrupt on swapping
  ZPSPI8	  EQU 00000001B	  ; 8088 processes all interrupts
ZDIPSW		EQU 0FFH	; Configuration dip switches
  ZDIPSWBOOT	  EQU 00000111B	  ; Boot device field
  ZDIPSWAB	  EQU 00001000B	  ; 1=Auto boot(0=Manual boot)
  ZDIPSWRES	  EQU 01110000B	  ; Reserved
  ZDIPSWHZ	  EQU 10000000B	  ; 1=50Hz(0=60HZ)

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"
#include "video/mc6845.h"
#include "machine/pic8259.h"
#include "machine/6821pia.h"

class z100_state : public driver_device
{
public:
	z100_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
	{ }

	UINT8 *m_gvram;
	UINT8 m_keyb_press;
};

static VIDEO_START( z100 )
{

}

static SCREEN_UPDATE( z100 )
{
	z100_state *state = screen->machine().driver_data<z100_state>();
	int x,y,xi;
	int count;
	int r,g,b;

	count = 0;

	for (y=0;y<200;y++)
	{
		for(x=0;x<1024;x+=8)
		{
			for(xi=0;xi<8;xi++)
			{
				b = ((state->m_gvram[count+0x00000] >> (7-xi)) & 1)<<0;
				r = ((state->m_gvram[count+0x10000] >> (7-xi)) & 1)<<1;
				g = ((state->m_gvram[count+0x20000] >> (7-xi)) & 1)<<2;

				if(y < 200 && x+xi < 640) /* TODO: safety check */
					*BITMAP_ADDR16(bitmap, y, x+xi) = screen->machine().pens[b|r|g];
			}

			count++;
		}
	}

    return 0;
}

static ADDRESS_MAP_START(z100_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000,0x1ffff) AM_RAM // 128 KB RAM
//	AM_RANGE(0xb0000,0xbffff) AM_ROM // expansion ROM
	AM_RANGE(0xc0000,0xeffff) AM_RAM AM_BASE_MEMBER(z100_state,m_gvram) // Blue / Red / Green
//	AM_RANGE(0xfbffa,0xfbffb) // expansion ROM check ID 0x4550
	AM_RANGE(0xfc000,0xfffff) AM_ROM AM_REGION("ipl", 0)
ADDRESS_MAP_END

static READ8_HANDLER(keyb_data_r)
{
	z100_state *state = space->machine().driver_data<z100_state>();

	return state->m_keyb_press;
}

static READ8_HANDLER(keyb_status_r)
{
	return 1;
}

static WRITE8_HANDLER(keyb_command_w)
{
	// ...
}

static ADDRESS_MAP_START( z100_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
//	AM_RANGE (0x98, 0x98) Z-205 max number
//	AM_RANGE (0xae, 0xaf) Z-217 disk controller
//	AM_RANGE (0xb0, 0xb1) Z-207 disk controller
	AM_RANGE (0xd8, 0xdb) AM_DEVREADWRITE_MODERN("pia0", pia6821_device, read, write)
	AM_RANGE (0xdc, 0xdc) AM_DEVWRITE_MODERN("crtc", mc6845_device, address_w)
	AM_RANGE (0xdd, 0xdd) AM_DEVWRITE_MODERN("crtc", mc6845_device, register_w)
//	AM_RANGE (0xde, 0xde) light pen
	AM_RANGE (0xe0, 0xe3) AM_DEVREADWRITE_MODERN("pia1", pia6821_device, read, write)
//	AM_RANGE (0xe4, 0xe7) 8253 PIT
//	AM_RANGE (0xe8, 0xeb) First 2661-2 serial port
//	AM_RANGE (0xec, 0xef) Second 2661-2 serial port
	AM_RANGE (0xf0, 0xf1) AM_DEVREADWRITE("pic8259_slave", pic8259_r, pic8259_w)
	AM_RANGE (0xf2, 0xf3) AM_DEVREADWRITE("pic8259_master", pic8259_r, pic8259_w)
	AM_RANGE (0xf4, 0xf4) AM_READ(keyb_data_r)
	AM_RANGE (0xf5, 0xf5) AM_READWRITE(keyb_status_r,keyb_command_w)
//	AM_RANGE (0xf6, 0xf6) expansion ROM is present (bit 0, active low)
//	AM_RANGE (0xfb, 0xfb) timer irq status
//	AM_RANGE (0xfc, 0xfc) memory latch
//	AM_RANGE (0xfd, 0xfd) Hi-address latch
//	AM_RANGE (0xfe, 0xfe) Processor swap port
	AM_RANGE (0xff, 0xff) AM_READ_PORT("DSW")
ADDRESS_MAP_END

static INPUT_CHANGED( key_stroke )
{
	z100_state *state = field.machine().driver_data<z100_state>();

	if(newval && !oldval)
	{
		state->m_keyb_press = (UINT8)(FPTR)(param) & 0x7f;
		//pic8259_ir1_w(field.machine().device("pic8259_master"), 1);
	}

	if(oldval && !newval)
		state->m_keyb_press = 0;
}

/* Input ports */
INPUT_PORTS_START( z100 )
	PORT_START("KEY0") // 0x00 - 0x07
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_UNUSED)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x01)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x02)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x03)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x04)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x05)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x06)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x07)

	PORT_START("KEY1") // 0x08 - 0x0f
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x08)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x09)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x0a)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x0b)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x0c)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x0d)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x0e)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x0f)

	PORT_START("KEY2") // 0x10 - 0x17
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x10)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x11)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x12)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x13)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x14)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x15)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x16)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x17)

	PORT_START("KEY3") // 0x18 - 0x1f
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x18)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x19)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x1a)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x1b)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x1c)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x1d)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x1e)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x1f)

	PORT_START("KEY4") // 0x20 - 0x27
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x20)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x21)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x22)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x23)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x24)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x25)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x26)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x27)

	PORT_START("KEY5") // 0x28 - 0x2f
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x28)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x29)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x2a)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x2b)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x2c)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x2d)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x2e)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x2f)

	PORT_START("KEY6") // 0x30 - 0x37
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x30)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x31)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x32)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x33)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x34)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x35)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x36)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x37)

	PORT_START("KEY7") // 0x38 - 0x3f
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x38)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x39)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x3a)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x3b)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x3c)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x3d)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x3e)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x3f)

	PORT_START("KEY8") // 0x40 - 0x47
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("8-0") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x60)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x61)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x62)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x63)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x64)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x65)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x66)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x67)

	PORT_START("KEY9") // 0x48 - 0x4f
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x68)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x69)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x6a)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x6b)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x6c)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x6d)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x6e)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x6f)

	PORT_START("KEYA") // 0x50 - 0x57
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x70)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x71)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x72)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x73)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x74)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x75)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x76)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x77)

	PORT_START("KEYB") // 0x58 - 0x5f
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x78)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x79)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x7a)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("B-4") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x7b)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("B-5") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x7c)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("B-6") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x7d)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("B-7") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x7e)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("B-8") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x7f)

	PORT_START("DSW")
	PORT_DIPNAME( 0x07, 0x00, "Default Auto-boot Device" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x07, "7" )
	PORT_DIPNAME( 0x08, 0x08, "Auto-boot" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) // Reserved
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) ) // Reserved
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) ) // Reserved
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Monitor" )
	PORT_DIPSETTING(    0x80, "PAL 50 Hz" )
	PORT_DIPSETTING(    0x00, "NTSC 60 Hz" )

	PORT_START("CONFIG")
	PORT_CONFNAME( 0x01, 0x01, "Video Board" )
	PORT_CONFSETTING( 0x00, "Monochrome" )
	PORT_CONFSETTING( 0x01, "Color" )
INPUT_PORTS_END

static IRQ_CALLBACK(z100_irq_callback)
{
	return pic8259_acknowledge( device->machine().device( "pic8259_master" ) );
}

static WRITE_LINE_DEVICE_HANDLER( z100_pic_irq )
{
	cputag_set_input_line(device->machine(), "maincpu", 0, state ? HOLD_LINE : CLEAR_LINE);
//  logerror("PIC#1: set IRQ line to %i\n",interrupt);
}

static READ8_DEVICE_HANDLER( get_slave_ack )
{
	if (offset==3) { // IRQ = 7
		return pic8259_acknowledge(device->machine().device( "pic8259_slave"));
	}
	return 0x00;
}

static const struct pic8259_interface z100_pic8259_master_config =
{
	DEVCB_LINE(z100_pic_irq),
	DEVCB_LINE_VCC,
	DEVCB_HANDLER(get_slave_ack)
};

static const struct pic8259_interface z100_pic8259_slave_config =
{
	DEVCB_DEVICE_LINE("pic8259_master", pic8259_ir7_w),
	DEVCB_LINE_GND,
	DEVCB_NULL
};

static const mc6845_interface mc6845_intf =
{
	"screen",	/* screen we are acting on */
	8,			/* number of pixels per video memory address */
	NULL,		/* before pixel update callback */
	NULL,		/* row update callback */
	NULL,		/* after pixel update callback */
	DEVCB_NULL,	/* callback for display state changes */
	DEVCB_NULL,	/* callback for cursor state changes */
	DEVCB_NULL,	/* HSYNC callback */
	DEVCB_NULL,	/* VSYNC callback */
	NULL		/* update address callback */
};

static WRITE8_DEVICE_HANDLER( video_pia_porta_w )
{
 /*
 	x--- ---- -> disable video RAM
	-x-- ---- -> not write multiple blue
    --x- ---- -> not write multiple green
    ---x ---- -> not write multiple red
    ---- x--- -> not flash screen
    ---- -x-- -> enable blue display
  	---- --x- -> enable green display
 	---- ---x -> enable red display
    */
}

static const pia6821_interface pia0_intf =
{
	DEVCB_NULL,		/* port A in */
	DEVCB_NULL,		/* port B in */
	DEVCB_NULL,		/* line CA1 in */
	DEVCB_NULL,		/* line CB1 in */
	DEVCB_NULL,		/* line CA2 in */
	DEVCB_NULL,		/* line CB2 in */
	DEVCB_HANDLER(video_pia_porta_w),		/* port A out */
	DEVCB_NULL,		/* port B out */
	DEVCB_NULL,		/* line CA2 out */
	DEVCB_NULL,		/* port CB2 out */
	DEVCB_NULL,		/* IRQA */
	DEVCB_NULL		/* IRQB */
};

static const pia6821_interface pia1_intf =
{
	DEVCB_NULL,		/* port A in */
	DEVCB_NULL,		/* port B in */
	DEVCB_NULL,		/* line CA1 in */
	DEVCB_NULL,		/* line CB1 in */
	DEVCB_NULL,		/* line CA2 in */
	DEVCB_NULL,		/* line CB2 in */
	DEVCB_NULL,		/* port A out */
	DEVCB_NULL,		/* port B out */
	DEVCB_NULL,		/* line CA2 out */
	DEVCB_NULL,		/* port CB2 out */
	DEVCB_NULL,		/* IRQA */
	DEVCB_NULL		/* IRQB */
};

static PALETTE_INIT(z100)
{
	// ...
}

static MACHINE_START(z100)
{
//	z100_state *state = machine.driver_data<z100_state>();

	device_set_irq_callback(machine.device("maincpu"), z100_irq_callback);
}

static MACHINE_RESET(z100)
{
	int i;

	if(input_port_read(machine,"CONFIG") & 1)
	{
		for(i=0;i<8;i++)
			palette_set_color_rgb(machine, i,pal1bit(i >> 1),pal1bit(i >> 2),pal1bit(i >> 0));
	}
	else
	{
		for(i=0;i<8;i++)
			palette_set_color_rgb(machine, i,pal3bit(i),pal3bit(i),pal3bit(i));
	}
}

static MACHINE_CONFIG_START( z100, z100_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",I8088, XTAL_14_31818MHz/3)
    MCFG_CPU_PROGRAM_MAP(z100_mem)
    MCFG_CPU_IO_MAP(z100_io)

    MCFG_MACHINE_START(z100)
    MCFG_MACHINE_RESET(z100)

	MCFG_PIC8259_ADD( "pic8259_master", z100_pic8259_master_config )
	MCFG_PIC8259_ADD( "pic8259_slave", z100_pic8259_slave_config )

	MCFG_PIA6821_ADD("pia0", pia0_intf)
	MCFG_PIA6821_ADD("pia1", pia1_intf)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(z100)

	MCFG_MC6845_ADD("crtc", MC6845, XTAL_14_31818MHz/8, mc6845_intf)	/* unknown clock, hand tuned to get ~50/~60 fps */

    MCFG_PALETTE_LENGTH(8)
	MCFG_PALETTE_INIT(z100)

    MCFG_VIDEO_START(z100)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( z100 )
	ROM_REGION( 0x4000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "intel-d27128-1.bin", 0x0000, 0x4000, CRC(b21f0392) SHA1(69e492891cceb143a685315efe0752981a2d8143))

	ROM_REGION( 0x1000, "mcu", ROMREGION_ERASEFF )
	ROM_LOAD( "mcu", 0x0000, 0x1000, NO_DUMP )
ROM_END

static DRIVER_INIT( z100 )
{
	UINT8 *ROM = machine.region("ipl")->base();

	ROM[0xfc116 & 0x3fff] = 0x90; // patch parity IRQ check
	ROM[0xfc117 & 0x3fff] = 0x90;

	ROM[0xfc0d4 & 0x3fff] = 0x75; // patch checksum
}


/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1982, z100,  	0,       0, 		z100, 	z100, 	 z100,  	 "Zenith",   "Z-100",		GAME_NOT_WORKING | GAME_NO_SOUND)

