
/************************************************************************
	crct6845

	MESS Driver By:

 	Gordon Jefferyes
 	mess_bbc@gjeffery.dircon.co.uk

 ************************************************************************/

typedef enum
{
	M6845_PERSONALITY_GENUINE,
	M6845_PERSONALITY_PC1512,
	M6845_PERSONALITY_UM6845,
	M6845_PERSONALITY_UM6845R,
	M6845_PERSONALITY_HD6845S,
	M6845_PERSONALITY_AMS40489,
	M6845_PERSONALITY_PREASIC
} m6845_personality_t;

struct crtc6845_interface
{
	void (*out_MA_func)(int offset, int data);
	void (*out_RA_func)(int offset, int data);
	void (*out_HS_func)(int offset, int data);
	void (*out_VS_func)(int offset, int data);
	void (*out_DE_func)(int offset, int data);
	void (*out_CR_func)(int offset, int data);
	void (*out_CRE_func)(int offset, int data);
};

typedef struct crtc6845_state
{
	/* Register Select */
	int address_register;
	/* register data */
	int registers[32];
	/* vertical and horizontal sync widths */
	int vertical_sync_width, horizontal_sync_width;

	int screen_start_address;         /* = R12<<8 + R13 */
	int cursor_address;				  /* = R14<<8 + R15 */
	int light_pen_address;			  /* = R16<<8 + R17 */

	int scan_lines_increment;

	int Horizontal_Counter;
	int Horizontal_Counter_Reset;

	int Scan_Line_Counter;
	int Scan_Line_Counter_Reset;

	int Character_Row_Counter;
	int Character_Row_Counter_Reset;

	int Horizontal_Sync_Width_Counter;
	int Vertical_Sync_Width_Counter;

	int HSYNC;
	int VSYNC;

	int Vertical_Total_Adjust_Active;
	int Vertical_Total_Adjust_Counter;

	int Memory_Address;
	int Memory_Address_of_next_Character_Row;
	int Memory_Address_of_this_Character_Row;

	int Horizontal_Display_Enabled;
	int Vertical_Display_Enabled;
	int Display_Enabled;
	int Display_Delayed_Enabled;

	int Cursor_Delayed_Status;

	int Cursor_Flash_Count;

	int Delay_Flags;
	int Cursor_Start_Delay;
	int Display_Enabled_Delay;
	int Display_Disable_Delay;

	int	Vertical_Adjust_Done;
//	int cycles_to_vsync_start;
//	int cycles_to_vsync_end;
} crtc6845_state;

/* set up the local copy of the 6845 external procedure calls */
void crtc6845_config(const struct crtc6845_interface *intf);


/* functions to set the 6845 registers */
int crtc6845_register_r(int offset);
void crtc6845_address_w(int offset, int data);
void crtc6845_register_w(int offset, int data);


/* clock the 6845 */
void crtc6845_clock(void);

/* called every frame to advance the cursor count */
void crtc6845_frameclock(void);

/* functions to read the 6845 outputs */
int crtc6845_memory_address_r(int offset);
int crtc6845_row_address_r(int offset);
int crtc6845_horizontal_sync_r(int offset);
int crtc6845_vertical_sync_r(int offset);
int crtc6845_display_enabled_r(int offset);
int crtc6845_cursor_enabled_r(int offset);

void crtc6845_recalc(int offset, int cycles);

void crtc6845_set_state(int offset, crtc6845_state *state);
void crtc6845_get_state(int offset, crtc6845_state *state);

void crtc6845_reset(int which);

void crtc6845_start(void);

void crtc6845_set_personality(m6845_personality_t personality);

int crtc6845_get_register(int reg);
int crtc6845_get_scanline_counter(void);
int crtc6845_get_row_counter(void);
