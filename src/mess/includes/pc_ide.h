/* 
   ibm at mfm hard disk 
   interface still used in ide devices!
*/

#include "driver.h"

 READ8_HANDLER(at_mfm_0_r);
WRITE8_HANDLER(at_mfm_0_w);

#if 0
void pc_ide_data_w(UINT8 data);
void pc_ide_write_precomp_w(UINT8 data);
void pc_ide_sector_count_w(UINT8 data);
void pc_ide_sector_number_w(UINT8 data);
void pc_ide_cylinder_number_l_w(UINT8 data);
void pc_ide_cylinder_number_h_w(UINT8 data);
void pc_ide_drive_head_w(UINT8 data);
void pc_ide_command_w(UINT8 data);
UINT8 pc_ide_data_r(void);
UINT8 pc_ide_error_r(void);
UINT8 pc_ide_sector_count_r(void);
UINT8 pc_ide_sector_number_r(void);
UINT8 pc_ide_cylinder_number_l_r(void);
UINT8 pc_ide_cylinder_number_h_r(void);
UINT8 pc_ide_drive_head_r(void);
UINT8 pc_ide_status_r(void);
#endif
