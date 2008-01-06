/*****************************************************************************
 *
 * machine/upd7002.h
 *
 * uPD7002 Analogue to Digital Converter
 *
 * Driver by Gordon Jefferyes <mess_bbc@gjeffery.dircon.co.uk>
 *
 ****************************************************************************/

#ifndef UPD7002_H_
#define UPD7002_H_


struct uPD7002_interface
{
	int  (*get_analogue_func)(int channel_number);
	void (*EOC_func)(int data);
};


/*----------- defined in machine/upd7002.c -----------*/

void uPD7002_config(const struct uPD7002_interface *intf);

 READ8_HANDLER ( uPD7002_EOC_r );

 READ8_HANDLER ( uPD7002_r );
WRITE8_HANDLER ( uPD7002_w );


#endif /* UPD7002_H_ */
