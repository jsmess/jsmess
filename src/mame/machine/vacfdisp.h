// Any fixes for this driver should be forwarded to AGEMAME HQ (http://www.mameworld.net/agemame/)

#ifndef VACFDISP
#define VACFDISP

#define MAX_VFDS  3	  // max number of vfd displays emulated

#define VFDTYPE_BFMBD1	0 // Bellfruit BD1 display
#define VFDTYPE_MSC1937 1 // OKI MSC1937   display
#define REVERSED 1 // display is reversed

void	vfd_init(  int id, int type, int reversed );		// setup a vfd

void	vfd_reset( int id);					// reset the vfd

void	vfd_shift_data(int id, int data);	// clock in a bit of data

int		vfd_newdata(   int id, int data);	// clock in 8 bits of data (scorpion2 vfd interface)

UINT16	*vfd_get_segments(int id);			// get current segments displayed
UINT16  *vfd_set_outputs(int id);			// convert segments to standard for display
UINT16  *vfd_get_outputs(int id);			// get converted segments

char	*vfd_get_string( int id);			// get current string   displayed (not as accurate)

void draw_16seg(mame_bitmap *bitmap,int vfd, int col_on, int col_off );
void draw_14seg(mame_bitmap *bitmap,int vfd, int col_on, int col_off );
#endif

