/*****************************************************************************
 *
 * machine/ins8154.h
 * 
 * INS8154 N-Channel 128-by-8 Bit RAM Input/Output (RAM I/O)
 *
 ****************************************************************************/

#ifndef INS8154_H_
#define INS8154_H_


/******************* Interface **********************************************/

typedef struct _ins8154_interface ins8154_interface;
struct _ins8154_interface
{
	read8_handler in_a_func;
	read8_handler in_b_func;
	write8_handler out_a_func;
	write8_handler out_b_func;
	void (*irq_func)(int state);	
};


/******************* Configuration ******************************************/

void ins8154_config(int which, const ins8154_interface *intf);
void ins8154_reset(int which);


/******************* Standard 8-bit CPU interfaces, D0-D7 *******************/

READ8_HANDLER( ins8154_0_r );
READ8_HANDLER( ins8154_1_r );
WRITE8_HANDLER( ins8154_0_w );
WRITE8_HANDLER( ins8154_1_w );


/******************* 8-bit A/B port interfaces ******************************/

READ8_HANDLER( ins8154_0_porta_r );
READ8_HANDLER( ins8154_0_portb_r );
READ8_HANDLER( ins8154_1_porta_r );
READ8_HANDLER( ins8154_1_portb_r );
WRITE8_HANDLER( ins8154_0_porta_w );
WRITE8_HANDLER( ins8154_0_portb_w );
WRITE8_HANDLER( ins8154_1_porta_w );
WRITE8_HANDLER( ins8154_1_portb_w );


#endif /*INS8154_H_*/
