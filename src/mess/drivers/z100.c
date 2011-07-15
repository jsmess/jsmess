/***************************************************************************
   
        Zenith Z-100

        15/07/2011 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"

class z100_state : public driver_device
{
public:
	z100_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
	{ }
};
/*
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

 */
static ADDRESS_MAP_START(z100_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000,0x1ffff) AM_RAM // 128 KB RAM
	AM_RANGE(0xc0000,0xcffff) AM_RAM // Green
	AM_RANGE(0xd0000,0xdffff) AM_RAM // Red
	AM_RANGE(0xe0000,0xeffff) AM_RAM // Blue 
	AM_RANGE(0xfc000,0xfffff) AM_ROM AM_REGION("ipl", 0)
ADDRESS_MAP_END

static READ8_HANDLER(unk_f5_r)
{
	return 0;
}

static ADDRESS_MAP_START( z100_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE (0xf5, 0xf5) AM_READ(unk_f5_r)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( z100 )
INPUT_PORTS_END


static MACHINE_RESET(z100) 
{	
}

static VIDEO_START( z100 )
{
}

static SCREEN_UPDATE( z100 )
{
    return 0;
}

static MACHINE_CONFIG_START( z100, z100_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",I8088, XTAL_14_31818MHz/3)
    MCFG_CPU_PROGRAM_MAP(z100_mem)
    MCFG_CPU_IO_MAP(z100_io)	

    MCFG_MACHINE_RESET(z100)
	
    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(z100)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(z100)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( z100 )
	ROM_REGION( 0x4000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "intel-d27128-1.bin", 0x0000, 0x4000, CRC(b21f0392) SHA1(69e492891cceb143a685315efe0752981a2d8143))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1982, z100,  	0,       0, 		z100, 	z100, 	 0,  	 "Zenith",   "Z-100",		GAME_NOT_WORKING | GAME_NO_SOUND)

