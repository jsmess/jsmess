#include "machine/8255ppi.h"

extern const ppi8255_interface pc_ppi8255_interface;

READ8_HANDLER( pc_rtc_r );
WRITE8_HANDLER( pc_rtc_w );

READ16_HANDLER( pc16le_rtc_r );
WRITE16_HANDLER( pc16le_rtc_w );

void pc_rtc_init(void);

READ8_HANDLER ( pc_EXP_r );
WRITE8_HANDLER ( pc_EXP_w );
