/* System E stuff */

#include "driver.h"

extern VIDEO_UPDATE(megatech_bios);
extern DRIVER_INIT( megatech_bios );
extern MACHINE_RESET(megatech_bios);
extern VIDEO_EOF(megatech_bios);

extern READ8_HANDLER( sms_vcounter_r );
extern READ8_HANDLER( sms_vdp_data_r );
extern WRITE8_HANDLER( sms_vdp_data_w );
extern READ8_HANDLER( sms_vdp_ctrl_r );
extern WRITE8_HANDLER( sms_vdp_ctrl_w );
