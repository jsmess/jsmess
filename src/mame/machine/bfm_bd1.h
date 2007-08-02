#ifndef BFM_BD1
#define BFM_BD1

#define MAX_BD1  3	  // max number of displays emulated

void	bfm_bd1_init( int id );					// setup a display

void	bfm_bd1_reset( int id);					// reset the alpha

void	bfm_bd1_shift_data(int id, int data);	// clock in a bit of data

int		bfm_bd1_newdata(   int id, int data);	// clock in 8 bits of data

UINT32	*bfm_bd1_get_segments(int id);			// get current segments displayed
UINT32  *bfm_bd1_set_outputs(int id);			// convert segments to standard for display
UINT32  *bfm_bd1_get_outputs(int id);			// get converted segments

char	*bfm_bd1_get_string( int id);			// get current string   displayed (not as accurate)

void	bfm_bd1_draw(int id);
#endif

