/******************************************************************************
    uPD7002 Analogue to Digital Converter

    MESS Driver By:

	Gordon Jefferyes
	mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

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


