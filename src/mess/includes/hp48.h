// in systems/hp48.c
#define KEY_A input_port_0_r(0)&0x80
#define KEY_B input_port_0_r(0)&0x40
#define KEY_C input_port_0_r(0)&0x20
#define KEY_D input_port_0_r(0)&0x10
#define KEY_E input_port_0_r(0)&8
#define KEY_F input_port_0_r(0)&4
#define KEY_G input_port_0_r(0)&2
#define KEY_H input_port_0_r(0)&1
#define KEY_I input_port_1_r(0)&0x80
#define KEY_J input_port_1_r(0)&0x40
#define KEY_K input_port_1_r(0)&0x20
#define KEY_L input_port_1_r(0)&0x10
#define KEY_M input_port_1_r(0)&8
#define KEY_N input_port_1_r(0)&4
#define KEY_O input_port_1_r(0)&2
#define KEY_P input_port_1_r(0)&1
#define KEY_Q input_port_2_r(0)&0x80
#define KEY_R input_port_2_r(0)&0x40
#define KEY_S input_port_2_r(0)&0x20
#define KEY_T input_port_2_r(0)&0x10
#define KEY_U input_port_2_r(0)&8
#define KEY_V input_port_2_r(0)&4
#define KEY_W input_port_2_r(0)&2
#define KEY_X input_port_2_r(0)&1
#define KEY_ENTER input_port_3_r(0)&0x80
#define KEY_Y input_port_3_r(0)&0x40
#define KEY_Z input_port_3_r(0)&0x20
#define KEY_DEL input_port_3_r(0)&0x10
#define KEY_LEFT input_port_3_r(0)&8
#define KEY_ALPHA input_port_3_r(0)&4
#define KEY_7 input_port_3_r(0)&2
#define KEY_8 input_port_3_r(0)&1
#define KEY_9 input_port_4_r(0)&0x80
#define KEY_DIVIDE input_port_4_r(0)&0x40
#define KEY_ORANGE input_port_4_r(0)&0x20
#define KEY_4 input_port_4_r(0)&0x10
#define KEY_5 input_port_4_r(0)&8
#define KEY_6 input_port_4_r(0)&4
#define KEY_MULTIPLY input_port_4_r(0)&2
#define KEY_BLUE input_port_4_r(0)&1
#define KEY_1 input_port_5_r(0)&0x80
#define KEY_2 input_port_5_r(0)&0x40
#define KEY_3 input_port_5_r(0)&0x20
#define KEY_MINUS input_port_5_r(0)&0x10
#define KEY_ON input_port_5_r(0)&8
#define KEY_0 input_port_5_r(0)&4
#define KEY_POINT input_port_5_r(0)&2
#define KEY_SPC input_port_5_r(0)&1
#define KEY_PLUS input_port_6_r(0)&0x80

/* in machine/hp48.c */
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

extern DRIVER_INIT( hp48s );
extern DRIVER_INIT( hp48g );
extern MACHINE_RESET( hp48 );

/* in vidhrdw/hp48.c */
extern unsigned short hp48_colortable[0x20][2];

extern PALETTE_INIT( hp48 );
extern VIDEO_START( hp48 );
extern VIDEO_UPDATE( hp48 );
extern WRITE8_HANDLER( hp48_write );
extern  READ8_HANDLER( hp48_read );
