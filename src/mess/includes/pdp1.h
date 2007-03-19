/* should go into memory.h */

#define READ18_HANDLER(name) READ32_HANDLER(name)
#define WRITE18_HANDLER(name) WRITE32_HANDLER(name)

#define MEMORY_READ_START18(name) MEMORY_READ32_START(name)
#define MEMORY_WRITE_START18(name) MEMORY_WRITE32_START(name)


/* From systems/pdp1.c */
/* defines for input port numbers */
enum
{
	pdp1_spacewar_controllers = 0,
	pdp1_control_switches = 1,		/* various operator control panel switches */
	pdp1_sense_switches = 2,		/* sense switches */
	pdp1_ta_switches = 3,			/* test address switches */
	pdp1_tw_switches_MSB = 4,		/* test word switches */
	pdp1_tw_switches_LSB = 5,
	pdp1_typewriter = 6,			/* typewriter keys */
	pdp1_config = 10,				/* pseudo input port with config */
	pdp1_lightpen_state = 11,		/* pseudo input port with light pen down */
	pdp1_lightpen_x = 12,			/* pseudo input port with light pen x deltas */
	pdp1_lightpen_y = 13			/* pseudo input port with light pen y deltas */
};

/* defines for each bit and mask in input port pdp1_control_switches */
enum
{
	/* bit numbers */
	pdp1_control_bit = 0,

	pdp1_extend_bit		= 1,
	pdp1_start_nobrk_bit= 2,
	pdp1_start_brk_bit	= 3,
	pdp1_stop_bit		= 4,
	pdp1_continue_bit	= 5,
	pdp1_examine_bit	= 6,
	pdp1_deposit_bit	= 7,
	pdp1_read_in_bit	= 8,
	pdp1_reader_bit		= 9,
	pdp1_tape_feed_bit	= 10,
	pdp1_single_step_bit= 11,
	pdp1_single_inst_bit= 12,

	/* masks */
	pdp1_control = (1 << pdp1_control_bit),
	pdp1_extend = (1 << pdp1_extend_bit),
	pdp1_start_nobrk = (1 << pdp1_start_nobrk_bit),
	pdp1_start_brk = (1 << pdp1_start_brk_bit),
	pdp1_stop = (1 << pdp1_stop_bit),
	pdp1_continue = (1 << pdp1_continue_bit),
	pdp1_examine = (1 << pdp1_examine_bit),
	pdp1_deposit = (1 << pdp1_deposit_bit),
	pdp1_read_in = (1 << pdp1_read_in_bit),
	pdp1_reader = (1 << pdp1_reader_bit),
	pdp1_tape_feed = (1 << pdp1_tape_feed_bit),
	pdp1_single_step = (1 << pdp1_single_step_bit),
	pdp1_single_inst = (1 << pdp1_single_inst_bit)
};

/* defines for each bit in input port pdp1_spacewar_controllers*/
#define ROTATE_LEFT_PLAYER1       0x01
#define ROTATE_RIGHT_PLAYER1      0x02
#define THRUST_PLAYER1            0x04
#define FIRE_PLAYER1              0x08
#define ROTATE_LEFT_PLAYER2       0x10
#define ROTATE_RIGHT_PLAYER2      0x20
#define THRUST_PLAYER2            0x40
#define FIRE_PLAYER2              0x80
#define HSPACE_PLAYER1            0x100
#define HSPACE_PLAYER2            0x200

/* defines for each field in input port pdp1_config */
enum
{
	pdp1_config_extend_bit			= 0,
	pdp1_config_extend_mask			= 0x3,	/* 2 bits */
	pdp1_config_hw_mul_div_bit		= 2,
	pdp1_config_hw_mul_div_mask		= 0x1,
	/*pdp1_config_hw_obsolete_bit	= 3,
	pdp1_config_hw_obsolete_mask	= 0x1,*/
	pdp1_config_type_20_sbs_bit		= 4,
	pdp1_config_type_20_sbs_mask	= 0x1,
	pdp1_config_lightpen_bit		= 5,
	pdp1_config_lightpen_mask		= 0x1
};

/* defines for each field in input port pdp1_lightpen_state */
enum
{
	pdp1_lightpen_down_bit			= 0,
	pdp1_lightpen_smaller_bit		= 1,
	pdp1_lightpen_larger_bit		= 2,

	pdp1_lightpen_down				= (1 << pdp1_lightpen_down_bit),
	pdp1_lightpen_smaller			= (1 << pdp1_lightpen_smaller_bit),
	pdp1_lightpen_larger			= (1 << pdp1_lightpen_larger_bit)
};

/* defines for our font */
enum
{
	pdp1_charnum = /*104*/128,	/* ASCII set + 8 special characters */
									/* for whatever reason, 104 breaks some characters */

	pdp1_fontdata_size = 8 * pdp1_charnum
};

extern pdp1_reset_param_t pdp1_reset_param;


/* From machine/pdp1.c */
extern int *pdp1_memory;

MACHINE_START( pdp1 );
READ18_HANDLER ( pdp1_read_mem );
WRITE18_HANDLER ( pdp1_write_mem );

DEVICE_INIT( pdp1_tape );
DEVICE_LOAD( pdp1_tape );
DEVICE_UNLOAD( pdp1_tape );

void pdp1_tape_read_binary(void);
void iot_rpa(int op2, int nac, int mb, int *io, int ac);
void iot_rpb(int op2, int nac, int mb, int *io, int ac);
void iot_rrb(int op2, int nac, int mb, int *io, int ac);
void iot_ppa(int op2, int nac, int mb, int *io, int ac);
void iot_ppb(int op2, int nac, int mb, int *io, int ac);

DEVICE_LOAD(pdp1_typewriter);
DEVICE_UNLOAD(pdp1_typewriter);
void iot_tyo(int op2, int nac, int mb, int *io, int ac);
void iot_tyi(int op2, int nac, int mb, int *io, int ac);

void iot_dpy(int op2, int nac, int mb, int *io, int ac);

DEVICE_LOAD(pdp1_drum);
DEVICE_UNLOAD(pdp1_drum);
void iot_dia(int op2, int nac, int mb, int *io, int ac);
void iot_dba(int op2, int nac, int mb, int *io, int ac);
void iot_dcc(int op2, int nac, int mb, int *io, int ac);
void iot_dra(int op2, int nac, int mb, int *io, int ac);

void iot_011(int op2, int nac, int mb, int *io, int ac);

void iot_cks(int op2, int nac, int mb, int *io, int ac);
void pdp1_io_sc_callback(void);

INTERRUPT_GEN( pdp1_interrupt );

typedef struct lightpen_t
{
	char active;
	char down;
	short x, y;
	short radius;
} lightpen_t;

/* From vidhrdw/pdp1.c */
VIDEO_START( pdp1 );
VIDEO_EOF( pdp1 );
VIDEO_UPDATE( pdp1 );

void pdp1_plot(int x, int y);

void pdp1_typewriter_drawchar(int character);

void pdp1_update_lightpen_state(const lightpen_t *new_state);

enum
{
	/* size and position of crt window */
	crt_window_width = 512,
	crt_window_height = 512,
	crt_window_offset_x = 0,
	crt_window_offset_y = 0,
	/* size and position of operator control panel window */
	panel_window_width = 384,
	panel_window_height = 128,
	panel_window_offset_x = crt_window_width,
	panel_window_offset_y = 0,
	/* size and position of typewriter window */
	typewriter_window_width = 640,
	typewriter_window_height = 160,
	typewriter_window_offset_x = 0,
	typewriter_window_offset_y = crt_window_height
};

enum
{
	total_width = crt_window_width + panel_window_width,
	total_height = crt_window_height + typewriter_window_height,

	/* respect 4:3 aspect ratio to keep pixels square */
	virtual_width_1 = ((total_width+3)/4)*4,
	virtual_height_1 = ((total_height+2)/3)*3,
	virtual_width_2 = virtual_height_1*4/3,
	virtual_height_2 = virtual_width_1*3/4,
	virtual_width = (virtual_width_1 > virtual_width_2) ? virtual_width_1 : virtual_width_2,
	virtual_height = (virtual_height_1 > virtual_height_2) ? virtual_height_1 : virtual_height_2
};

enum
{	/* refresh rate in Hz: can be changed at will */
	refresh_rate = 60
};

/* Color codes */
enum
{
	/* first pen_crt_num_levels colors used for CRT (with remanence) */
	pen_crt_num_levels = 69,
	pen_crt_max_intensity = pen_crt_num_levels-1,

	/* next colors used for control panel and typewriter */
	pen_black = 0,
	pen_white = pen_crt_num_levels,
	pen_green,
	pen_dk_green,
	pen_red,
	pen_lt_gray,

	/* color constants for control panel */
	pen_panel_bg = pen_black,
	pen_panel_caption = pen_white,
	color_panel_caption = 0,
	pen_switch_nut = pen_lt_gray,
	pen_switch_button = pen_white,
	pen_lit_lamp = pen_green,
	pen_unlit_lamp = pen_dk_green,

	/* color constants for typewriter */
	pen_typewriter_bg = pen_white,
	color_typewriter_black = 1,
	color_typewriter_red = 2,

	/* color constants used for light pen */
	pen_lightpen_nonpressed = pen_red,
	pen_lightpen_pressed = pen_green
};

void pdp1_get_open_mode(const struct IODevice *dev, int id,
	unsigned int *readable, unsigned int *writeable, unsigned int *creatable);
