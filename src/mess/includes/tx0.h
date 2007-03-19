#define READ18_HANDLER(name) READ32_HANDLER(name)
#define WRITE18_HANDLER(name) WRITE32_HANDLER(name)

MACHINE_START( tx0 );
void tx0_tape_get_open_mode(const struct IODevice *dev, int id,
	unsigned int *readable, unsigned int *writeable, unsigned int *creatable);
DEVICE_INIT( tx0_tape );
DEVICE_LOAD( tx0_tape );
DEVICE_UNLOAD( tx0_tape );
void tx0_io_r1l(void);
void tx0_io_r3l(void);
void tx0_io_p6h(void);
void tx0_io_p7h(void);
DEVICE_LOAD(tx0_typewriter);
DEVICE_UNLOAD(tx0_typewriter);
void tx0_io_prt(void);
void tx0_io_dis(void);
DEVICE_INIT( tx0_magtape );
DEVICE_LOAD( tx0_magtape );
DEVICE_UNLOAD( tx0_magtape );
void tx0_sel(void);
void tx0_io_cpy(void);
void tx0_io_reset_callback(void);
INTERRUPT_GEN( tx0_interrupt );

VIDEO_START( tx0 );
void tx0_plot(int x, int y);
VIDEO_UPDATE( tx0 );
void tx0_typewriter_drawchar(int character);

enum
{
	tx0_control_switches = 0,		/* various operator control panel switches */
	tx0_tsr_switches_MSW = 1,		/* toggle switch register switches */
	tx0_tsr_switches_LSW = 2,
	tx0_typewriter = 3				/* typewriter keys */
};

/* defines for each bit and mask in input port tx0_control_switches */
enum
{
	/* bit numbers */
	tx0_control_bit		= 0,

	tx0_stop_c0_bit		= 1,
	tx0_stop_c1_bit		= 2,
	tx0_gbl_cm_sel_bit	= 3,
	tx0_stop_bit		= 4,
	tx0_restart_bit		= 5,
	tx0_read_in_bit		= 6,

	tx0_toggle_dn_bit	= 12,
	tx0_toggle_up_bit	= 13,
	tx0_cm_sel_bit		= 14,
	tx0_lr_sel_bit		= 15,

	/* masks */
	tx0_control = (1 << tx0_control_bit),

	tx0_stop_cyc0 = (1 << tx0_stop_c0_bit),
	tx0_stop_cyc1 = (1 << tx0_stop_c1_bit),
	tx0_gbl_cm_sel = (1 << tx0_gbl_cm_sel_bit),
	tx0_stop = (1 << tx0_stop_bit),
	tx0_restart = (1 << tx0_restart_bit),
	tx0_read_in = (1 << tx0_read_in_bit),

	tx0_toggle_dn = (1 << tx0_toggle_dn_bit),
	tx0_toggle_up = (1 << tx0_toggle_up_bit),
	tx0_cm_sel = (1 << tx0_cm_sel_bit),
	tx0_lr_sel = (1 << tx0_lr_sel_bit)
};

/* defines for our font */
enum
{
	tx0_charnum = /*96*/128,	/* ASCII set + xx special characters */
									/* for whatever reason, 96 breaks some characters */

	tx0_fontdata_size = 8 * tx0_charnum
};

enum
{
	/* size and position of crt window */
	crt_window_width = /*511*/512,
	crt_window_height = /*511*/512,
	crt_window_offset_x = 0,
	crt_window_offset_y = 0,
	/* size and position of operator control panel window */
	panel_window_width = 272,
	panel_window_height = 264,
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
