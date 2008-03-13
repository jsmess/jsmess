/*****************************************************************************
 *
 * machine/ins8154.h
 * 
 * INS8154 N-Channel 128-by-8 Bit RAM Input/Output (RAM I/O)
 *
 ****************************************************************************/

#ifndef INS8154_H_
#define INS8154_H_

#define INS8154  DEVICE_GET_INFO_NAME(ins8154)


/******************* Interface **********************************************/

typedef struct _ins8154_t ins8154_t;
typedef struct _ins8154_interface ins8154_interface;

struct _ins8154_interface
{
	read8_device_func in_a_func;
	read8_device_func in_b_func;
	write8_device_func out_a_func;
	write8_device_func out_b_func;
	void (*irq_func)(int state);	
};

DEVICE_GET_INFO( ins8154 );


/******************* Standard 8-bit CPU interfaces, D0-D7 *******************/

READ8_DEVICE_HANDLER( ins8154_r );
WRITE8_DEVICE_HANDLER( ins8154_w );


/******************* 8-bit A/B port interfaces ******************************/

READ8_DEVICE_HANDLER( ins8154_porta_r );
READ8_DEVICE_HANDLER( ins8154_portb_r );
WRITE8_DEVICE_HANDLER( ins8154_porta_w );
WRITE8_DEVICE_HANDLER( ins8154_portb_w );


#endif /* INS8154_H_ */
