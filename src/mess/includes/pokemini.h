#ifndef _POKEMINI_H_
#define _POKEMINI_H_

struct VDP {
	UINT8	colors_inverted;
	UINT8	background_enabled;
	UINT8	sprites_enabled;
	UINT8	rendering_enabled;
	UINT8	map_size;
};

extern MACHINE_RESET( pokemini );
extern WRITE8_HANDLER( pokemini_hwreg_w );
extern READ8_HANDLER( pokemini_hwreg_r );

DEVICE_INIT( pokemini_cart );
DEVICE_LOAD( pokemini_cart );

#endif /* _POKEMINI_H */
