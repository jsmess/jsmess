
/************************************************************************
	saa505x

	MESS Driver By:

 	Gordon Jefferyes
 	mess_bbc@gjeffery.dircon.co.uk

 ************************************************************************/


struct saa505x_interface
{
	void (*out_Pixel_func)(int offset, int data);
};

void saa505x_config(const struct saa505x_interface *intf);


void teletext_DEW(void);
void teletext_LOSE_w(int offset, int data);

void teletext_data_w(int offset, int data);
void teletext_F1(void);

