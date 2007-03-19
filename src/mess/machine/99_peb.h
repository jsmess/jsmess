/*
	header file for machine/99_peb.c
*/

/*
	prototype for CRU handlers in expansion system
*/
typedef int (*cru_read_handler)(int offset);
typedef void (*cru_write_handler)(int offset, int data);

/*
	Descriptor for TI peripheral expansion cards (8-bit bus)
*/
typedef struct ti99_peb_card_handlers_t
{
	cru_read_handler cru_read;		/* card CRU read handler */
	cru_write_handler cru_write;	/* card CRU handler */

	read8_handler mem_read;		/* card mem read handler (8 bits) */
	write8_handler mem_write;	/* card mem write handler (8 bits) */
} ti99_peb_card_handlers_t;

/*
	Descriptor for 16-bit peripheral expansion cards designed for the snug
	sgcpu 99/4p system.  (These cards were not designed by TI, TI always
	regarded the ti-99 as an 8-bit system.)
*/
typedef struct ti99_peb_16bit_card_handlers_t
{
	cru_read_handler cru_read;		/* card CRU read handler */
	cru_write_handler cru_write;	/* card CRU handler */

	read16_handler mem_read;		/* card mem read handler (16 bits) */
	write16_handler mem_write;		/* card mem write handler (16 bits) */
} ti99_peb_16bit_card_handlers_t;

/* masks for ila and ilb */
enum
{
	/* bits from actual ILA register */
	inta_rs232_1_bit = 7-0,
	inta_rs232_2_bit = 7-1,
	inta_rs232_3_bit = 7-4,
	inta_rs232_4_bit = 7-5,

	/* extra "virtual" bits for devices that assert the INTA line without
	setting a bit of the ILA register */
	inta_ide_bit     = 8,
	inta_ide_clk_bit = 9,

	/* bits from actual ILB register */
	intb_fdc_bit     = 7-0,
	intb_ieee488_bit = 7-1
};

void ti99_peb_init(int in_has_16bit_peb, void (*in_inta_callback)(int state), void (*in_intb_callback)(int state));

void ti99_peb_set_card_handlers(int cru_base, const ti99_peb_card_handlers_t *handler);
void ti99_peb_set_16bit_card_handlers(int cru_base, const ti99_peb_16bit_card_handlers_t *handler);
void ti99_peb_set_ila_bit(int bit, int state);
void ti99_peb_set_ilb_bit(int bit, int state);

 READ8_HANDLER ( ti99_4x_peb_cru_r );
WRITE8_HANDLER ( ti99_4x_peb_cru_w );
READ16_HANDLER ( ti99_4x_peb_r );
WRITE16_HANDLER ( ti99_4x_peb_w );

 READ8_HANDLER ( geneve_peb_cru_r );
WRITE8_HANDLER ( geneve_peb_cru_w );
 READ8_HANDLER ( geneve_peb_r );
WRITE8_HANDLER ( geneve_peb_w );

 READ8_HANDLER ( ti99_8_peb_cru_r );
WRITE8_HANDLER ( ti99_8_peb_cru_w );
 READ8_HANDLER ( ti99_8_peb_r );
WRITE8_HANDLER ( ti99_8_peb_w );

 READ8_HANDLER ( ti99_4p_peb_cru_r );
WRITE8_HANDLER ( ti99_4p_peb_cru_w );
READ16_HANDLER ( ti99_4p_peb_r );
WRITE16_HANDLER ( ti99_4p_peb_w );

void ti99_4p_peb_set_senila(int state);
void ti99_4p_peb_set_senilb(int state);
