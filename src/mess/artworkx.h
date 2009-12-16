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

typedef enum
{
	ARTWORK_CUSTTYPE_JOYSTICK = 0,
	ARTWORK_CUSTTYPE_KEYBOARD,
	ARTWORK_CUSTTYPE_MISC,
	ARTWORK_CUSTTYPE_INVALID
} artwork_cust_type;



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

void artwork_use_device_art(const device_config *img, const char *defaultartfile);

int artwork_get_inputscreen_customizations(png_info *png, artwork_cust_type cust_type,
	const char *section,
	struct inputform_customization *customizations,
	int customizations_length);



#endif /* ARTWORKX_H */
