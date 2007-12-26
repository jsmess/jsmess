/***************************************************************************

    Z80 DART (Z8470) implementation

    Copyright (c) 2007, The MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAX_DART			2



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _z80dart_interface z80dart_interface;
struct _z80dart_interface
{
	int baseclock;
	void (*irq_cb)(int state);
	write8_handler dtr_changed_cb;
	write8_handler rts_changed_cb;
	write8_handler break_changed_cb;
	write8_handler transmit_cb;
	int (*receive_poll_cb)(int which);
};



/***************************************************************************
    INITIALIZATION/CONFIGURATION
***************************************************************************/

void z80dart_init(int which, const z80dart_interface *intf);
void z80dart_reset(int which);



/***************************************************************************
    CONTROL REGISTER READ/WRITE
***************************************************************************/

void z80dart_c_w(int which, int ch, UINT8 data);
UINT8 z80dart_c_r(int which, int ch);



/***************************************************************************
    DATA REGISTER READ/WRITE
***************************************************************************/

void z80dart_d_w(int which, int ch, UINT8 data);
UINT8 z80dart_d_r(int which, int ch);



/***************************************************************************
    CONTROL LINE READ/WRITE
***************************************************************************/

int z80dart_get_dtr(int which, int ch);
int z80dart_get_rts(int which, int ch);
void z80dart_set_cts(int which, int ch, int state);
void z80dart_set_dcd(int which, int ch, int state);
void z80dart_set_ri(int which, int state);
void z80dart_receive_data(int which, int ch, UINT8 data);



/***************************************************************************
    DAISY CHAIN INTERFACE
***************************************************************************/

int z80dart_irq_state(int which);
int z80dart_irq_ack(int which);
void z80dart_irq_reti(int which);
