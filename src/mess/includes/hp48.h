/*****************************************************************************
 *
 * includes/hp48.h
 *
 ****************************************************************************/

#ifndef HP48_H_
#define HP48_H_


#define KEY_A readinputport(0)&0x80
#define KEY_B readinputport(0)&0x40
#define KEY_C readinputport(0)&0x20
#define KEY_D readinputport(0)&0x10
#define KEY_E readinputport(0)&8
#define KEY_F readinputport(0)&4
#define KEY_G readinputport(0)&2
#define KEY_H readinputport(0)&1
#define KEY_I readinputport(1)&0x80
#define KEY_J readinputport(1)&0x40
#define KEY_K readinputport(1)&0x20
#define KEY_L readinputport(1)&0x10
#define KEY_M readinputport(1)&8
#define KEY_N readinputport(1)&4
#define KEY_O readinputport(1)&2
#define KEY_P readinputport(1)&1
#define KEY_Q readinputport(2)&0x80
#define KEY_R readinputport(2)&0x40
#define KEY_S readinputport(2)&0x20
#define KEY_T readinputport(2)&0x10
#define KEY_U readinputport(2)&8
#define KEY_V readinputport(2)&4
#define KEY_W readinputport(2)&2
#define KEY_X readinputport(2)&1
#define KEY_ENTER readinputport(3)&0x80
#define KEY_Y readinputport(3)&0x40
#define KEY_Z readinputport(3)&0x20
#define KEY_DEL readinputport(3)&0x10
#define KEY_LEFT readinputport(3)&8
#define KEY_ALPHA readinputport(3)&4
#define KEY_7 readinputport(3)&2
#define KEY_8 readinputport(3)&1
#define KEY_9 readinputport(4)&0x80
#define KEY_DIVIDE readinputport(4)&0x40
#define KEY_ORANGE readinputport(4)&0x20
#define KEY_4 readinputport(4)&0x10
#define KEY_5 readinputport(4)&8
#define KEY_6 readinputport(4)&4
#define KEY_MULTIPLY readinputport(4)&2
#define KEY_BLUE readinputport(4)&1
#define KEY_1 readinputport(5)&0x80
#define KEY_2 readinputport(5)&0x40
#define KEY_3 readinputport(5)&0x20
#define KEY_MINUS readinputport(5)&0x10
#define KEY_ON readinputport(5)&8
#define KEY_0 readinputport(5)&4
#define KEY_POINT readinputport(5)&2
#define KEY_SPC readinputport(5)&1
#define KEY_PLUS readinputport(6)&0x80


/*----------- defined in machine/hp48.c -----------*/

typedef struct {
	int data[0x40];
	UINT16 crc;
	UINT32 timer2;
} HP48_HARDWARE;
extern HP48_HARDWARE hp48_hardware;

extern UINT8 *hp48_ram, *hp48_card1, *hp48_card2;
void hp48_mem_reset(void);
void hp48_mem_config(int v);
void hp48_mem_unconfig(int v);
int hp48_mem_id(void);
void hp48_crc(int adr, int data);
int hp48_in(void);
void hp48_out(int v);

DRIVER_INIT( hp48s );
DRIVER_INIT( hp48g );
MACHINE_RESET( hp48 );

WRITE8_HANDLER( hp48_write );
READ8_HANDLER( hp48_read );

WRITE8_HANDLER( hp48_mem_w );

/*----------- defined in video/hp48.c -----------*/

extern const unsigned short hp48_colortable[0x20][2];

extern PALETTE_INIT( hp48 );
extern VIDEO_START( hp48 );
extern VIDEO_UPDATE( hp48 );


#endif /* HP48_H_ */
