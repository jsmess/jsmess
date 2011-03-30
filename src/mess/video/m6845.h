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

struct m6845_interface
{
	void (*out_MA_func)(running_machine &machine, int offset, int data);
	void (*out_RA_func)(running_machine &machine, int offset, int data);
	void (*out_HS_func)(running_machine &machine, int offset, int data);
	void (*out_VS_func)(running_machine &machine, int offset, int data);
	void (*out_DE_func)(running_machine &machine, int offset, int data);
	void (*out_CR_func)(running_machine &machine, int offset, int data);
	void (*out_CRE_func)(running_machine &machine, int offset, int data);
};

/* set up the local copy of the 6845 external procedure calls */
void m6845_config(const struct m6845_interface *intf);


/* functions to set the 6845 registers */
int m6845_register_r(int offset);
void m6845_address_w(int offset, int data);
void m6845_register_w(int offset, int data);


/* clock the 6845 */
void m6845_clock(running_machine &machine);

/* called every frame to advance the cursor count */
void m6845_frameclock(void);

/* functions to read the 6845 outputs */
int m6845_memory_address_r(int offset);
int m6845_row_address_r(int offset);
int m6845_horizontal_sync_r(int offset);
int m6845_vertical_sync_r(int offset);
int m6845_display_enabled_r(int offset);
int m6845_cursor_enabled_r(int offset);

#if 0
void m6845_recalc(int offset, int cycles);
#endif

void m6845_set_address(int address);

void m6845_reset(void);

void m6845_set_personality(m6845_personality_t personality);

int m6845_get_register(int reg);
int m6845_get_scanline_counter(void);
int m6845_get_row_counter(void);
