/******************************************************************************
    Acorn Electron driver

    MESS Driver By:

	Wilbert Pol

******************************************************************************/

/* Interrupts */
#define INT_HIGH_TONE		0x40
#define INT_TRANSMIT_EMPTY	0x20
#define INT_RECEIVE_FULL	0x10
#define INT_RTC			0x08
#define INT_DISPLAY_END		0x04
#define INT_SET			0x100
#define INT_CLEAR		0x200

/* ULA context */

typedef struct
{
	UINT8 interrupt_status;
	UINT8 interrupt_control;
	UINT8 rompage;
	UINT16 screen_start;
	UINT16 screen_base;
	int screen_size;
	UINT16 screen_addr;
	UINT8 *vram;
	int current_pal[16];
	int communication_mode;
	int screen_mode;
	int cassette_motor_mode;
	int capslock_mode;
	int scanline;
	/* tape reading related */
	UINT32 tape_value;
	int tape_steps;
	int bit_count;
	int high_tone_set;
	int start_bit;
	int stop_bit;
	int tape_running;
	UINT8 tape_byte;
} ULA;

extern ULA ula;

READ8_HANDLER( electron_jim_r );
WRITE8_HANDLER( electron_jim_w );
READ8_HANDLER( electron_1mhz_r );
WRITE8_HANDLER( electron_1mhz_w );
READ8_HANDLER( electron_ula_r );
WRITE8_HANDLER( electron_ula_w );
MACHINE_START( electron );
void electron_video_init( void );
INTERRUPT_GEN( electron_scanline_interrupt );
void electron_interrupt_handler(int mode, int interrupt);


