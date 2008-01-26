/*************************************************************************

    video/crt.h

    CRT video emulation for TX-0 and PDP-1

*************************************************************************/

#ifndef CRT_H_
#define CRT_H_


/*----------- defined in video/crt.c -----------*/

int video_start_crt(int num_levels, int offset_x, int offset_y, int width, int height);
void crt_plot(int x, int y);
VIDEO_EOF( crt );
VIDEO_UPDATE( crt );


#endif /* CRT_H_ */
