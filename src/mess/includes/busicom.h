/*****************************************************************************
 *
 * includes/busicom.h
 *
 ****************************************************************************/

#ifndef BUSICOM_H_
#define BUSICOM_H_


/*----------- defined in video/busicom.c -----------*/

extern UINT8 busicom_printer_line[11][17];
extern UINT8 busicom_printer_line_color[11];

extern PALETTE_INIT( busicom );
extern VIDEO_START( busicom );
extern VIDEO_UPDATE( busicom );

#endif /* BUSICOM_H_ */
