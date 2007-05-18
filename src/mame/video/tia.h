
#define TIA_INPUT_PORT_ALWAYS_ON		0
#define TIA_INPUT_PORT_ALWAYS_OFF		0xffff

PALETTE_INIT( tia_NTSC );
PALETTE_INIT( tia_PAL );

VIDEO_START( tia );
VIDEO_UPDATE( tia );

READ8_HANDLER( tia_r );
WRITE8_HANDLER( tia_w );

void tia_init(int (*read_input_port)(int));
void tia_init_pal(int (*read_input_port)(int));
