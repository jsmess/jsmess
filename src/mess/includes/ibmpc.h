#include "machine/8255ppi.h"

extern ppi8255_interface pc_ppi8255_interface;

WRITE8_HANDLER( pc_rtc_w );
 READ8_HANDLER( pc_rtc_r );
void pc_rtc_init(void);

extern WRITE8_HANDLER ( pc_EXP_w );
extern  READ8_HANDLER ( pc_EXP_r );


