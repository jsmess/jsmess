#ifndef _VIDEO_TIA_H_
#define _VIDEO_TIA_H_

#define TIA_INPUT_PORT_ALWAYS_ON		0
#define TIA_INPUT_PORT_ALWAYS_OFF		0xffff

struct TIAinterface {
	read16_handler	read_input_port;
	read8_handler	databus_contents;
};

PALETTE_INIT( tia_NTSC );
PALETTE_INIT( tia_PAL );

VIDEO_START( tia );
VIDEO_UPDATE( tia );

READ8_HANDLER( tia_r );
WRITE8_HANDLER( tia_w );

void tia_init(const struct TIAinterface* ti);
void tia_init_pal(const struct TIAinterface* ti);

#endif /* _VIDEO_TIA_H_ */
