/*
    mbc55x.h
    Includes for the Sanyo MBC-550, MBC-555.

    Phill Harvey-Smith
    2011-01-29.
*/

#include "machine/ram.h"
#include "machine/wd17xx.h"
#include "machine/msm8251.h"
#include "machine/pit8253.h"
#include "machine/i8255a.h"
#include "machine/pic8259.h"
#include "video/mc6845.h"

#define MAINCPU_TAG "maincpu"

#define SCREEN_TAG				"screen"
#define SCREEN_WIDTH_PIXELS     640
#define SCREEN_HEIGHT_LINES     250
#define SCREEN_NO_COLOURS       8

#define VIDEO_MEM_SIZE			(32*1024)
#define VID_MC6845_NAME			"mc6845"

// Red and blue colour planes sit at a fixed loaction, green
// is in main memory.

#define COLOUR_PLANE_SIZE		0x4000

#define	RED_PLANE_OFFSET		(0*COLOUR_PLANE_SIZE)
#define BLUE_PLANE_OFFSET		(1*COLOUR_PLANE_SIZE)

#define COLOUR_PLANE_MEMBASE	0xF0000
#define RED_PLANE_MEMBASE		(COLOUR_PLANE_MEMBASE+RED_PLANE_OFFSET)
#define BLUE_PLANE_MEMBASE		(COLOUR_PLANE_MEMBASE+BLUE_PLANE_OFFSET)

#define RED_PLANE_TAG			"red"
#define BLUE_PLANE_TAG			"blue"

// Keyboard

#define MBC55X_KEYROWS      	7
#define KEYBOARD_QUEUE_SIZE 	32

#define KB_BITMASK				0x1000
#define KB_SHIFTS				12

#define KEY_SPECIAL_TAG			"KEY_SPECIAL"
#define KEY_BIT_LSHIFT			0x01
#define KEY_BIT_RSHIFT			0x02
#define KEY_BIT_CTRL			0x04
#define KEY_BIT_GRAPH			0x08


typedef struct
{
	UINT8       keyrows[MBC55X_KEYROWS];
	emu_timer   *keyscan_timer;

	UINT8		key_special;
}  keyboard_t;


class mbc55x_state : public driver_device
{
public:
	mbc55x_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	device_t	*m_maincpu;
	device_t	*m_pic8259;
	device_t	*m_pit8253;

	UINT32		m_debug_machine;
	UINT32		m_debug_video;
	UINT8		m_video_mem[VIDEO_MEM_SIZE];
	UINT8		m_vram_page;

	keyboard_t	m_keyboard;
};

/* IO chips */

#define PPI8255_TAG	"ppi8255"

READ8_HANDLER(ppi8255_r);
WRITE8_HANDLER(ppi8255_w);

#define PIC8259_TAG	"pic8259"

READ8_HANDLER(pic8259_r);
WRITE8_HANDLER(pic8259_w);

// From tech manual clock c1 is fed from c0, but it approx 100Hz
#define PIT8253_TAG		"pit8253"
#define PIT_C0_CLOCK	78600
#define PIT_C1_CLOCK	100
#define PIT_C2_CLOCK	1789770

READ8_HANDLER(pit8253_r);
WRITE8_HANDLER(pit8253_w);

/* Vram page register */
READ8_HANDLER(vram_page_r);
WRITE8_HANDLER(vram_page_w);

READ8_HANDLER(mbc55x_video_io_r);
WRITE8_HANDLER(mbc55x_video_io_w);

#define	SPEAKER_TAG				"speaker"
#define MONO_TAG                "mono"

#define MSM8251A_KB_TAG			"msm8251a_kb"

typedef struct
{
} msm_rx_t;

/*----------- defined in drivers/mbc55x.c -----------*/

extern const unsigned char mbc55x_palette[SCREEN_NO_COLOURS][3];


/*----------- defined in machine/mbc55x.c -----------*/

extern const struct pit8253_config mbc55x_pit8253_config;
extern const struct pic8259_interface mbc55x_pic8259_config;
extern const i8255a_interface mbc55x_ppi8255_interface;
extern const msm8251_interface mbc55x_msm8251a_interface;
extern const msm8251_interface mbc55x_msm8251b_interface;

DRIVER_INIT(mbc55x);
MACHINE_RESET(mbc55x);
MACHINE_START(mbc55x);


/* mbc55x specific */
READ16_HANDLER (mbc55x_io_r);
WRITE16_HANDLER (mbc55x_io_w);

/* Memory controler */
#define RAM_BANK00_TAG  "bank0"
#define RAM_BANK01_TAG  "bank1"
#define RAM_BANK02_TAG  "bank2"
#define RAM_BANK03_TAG  "bank3"
#define RAM_BANK04_TAG  "bank4"
#define RAM_BANK05_TAG  "bank5"
#define RAM_BANK06_TAG  "bank6"
#define RAM_BANK07_TAG  "bank7"
#define RAM_BANK08_TAG  "bank8"
#define RAM_BANK09_TAG  "bank9"
#define RAM_BANK0A_TAG  "banka"
#define RAM_BANK0B_TAG  "bankb"
#define RAM_BANK0C_TAG  "bankc"
#define RAM_BANK0D_TAG  "bankd"
#define RAM_BANK0E_TAG  "banke"

#define RAM_BANK_SIZE	(64*1024)
#define RAM_BANK_COUNT	15

/* Floppy drive interface */

#define FDC_TAG                 "wd1793"
#define FDC_PAUSE				10000

extern const wd17xx_interface mbc55x_wd17xx_interface;

READ8_HANDLER( mbc55x_disk_r );
WRITE8_HANDLER( mbc55x_disk_w );
READ8_HANDLER(mbc55x_usart_r);
WRITE8_HANDLER(mbc55x_usart_w);
READ8_HANDLER(mbc55x_kb_usart_r);
WRITE8_HANDLER(mbc55x_kb_usart_w);



/*----------- defined in video/mbc55x.c -----------*/

extern const mc6845_interface mb55x_mc6845_intf;

READ16_HANDLER (mbc55x_video_io_r);
WRITE16_HANDLER (mbc55x_video_io_w);

VIDEO_START( mbc55x );
SCREEN_EOF( mbc55x );
SCREEN_UPDATE( mbc55x );
VIDEO_RESET( mbc55x );

#define RED                     0
#define GREEN                   1
#define BLUE                    2

#define LINEAR_ADDR(seg,ofs)    ((seg<<4)+ofs)

#define OUTPUT_SEGOFS(mess,seg,ofs)  logerror("%s=%04X:%04X [%08X]\n",mess,seg,ofs,((seg<<4)+ofs))
