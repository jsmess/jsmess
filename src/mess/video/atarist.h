#ifndef _VIDEO_ATARIST_H_
#define _VIDEO_ATARIST_H_

READ16_HANDLER( atarist_shifter_r );
WRITE16_HANDLER( atarist_shifter_w );
READ16_HANDLER( atariste_shifter_r );
WRITE16_HANDLER( atariste_shifter_w );

VIDEO_START( atarist );
VIDEO_UPDATE( atarist );

#endif
