/*********************************************************************

	artworkx.h

	MESS specific artwork code

*********************************************************************/

#ifndef ARTWORKX_H
#define ARTWORKX_H

#include "fileio.h"
#include "palette.h"
#include "png.h"



/***************************************************************************

	Constants

***************************************************************************/

#define ARTWORK_CUSTTYPE_JOYSTICK	0
#define ARTWORK_CUSTTYPE_KEYBOARD	1
#define ARTWORK_CUSTTYPE_MISC		2
#define ARTWORK_CUSTTYPE_INVALID	3



/***************************************************************************

	Type definitions

***************************************************************************/

struct inputform_customization
{
	UINT32 ipt;
	int x, y;
	int width, height;
};



/***************************************************************************

	Prototypes

***************************************************************************/

void artwork_use_device_art(mess_image *img, const char *defaultartfile);

int artwork_get_inputscreen_customizations(png_info *png, int cust_type,
	const char *section,
	struct inputform_customization *customizations,
	int customizations_length);



#endif /* ARTWORKX_H */
