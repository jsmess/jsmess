/* ay31015.h

	Written for MESS by Robbbert on May 29th, 2008.

*/

/* control reg */
#define AY31015_NB1	0x01
#define AY31015_NB2	0x02
#define AY31015_TSB	0x04
#define AY31015_EPS	0x08
#define AY31015_NP	0x10

/* status reg */
#define AY31015_TBMT	0x01
#define AY31015_DAV	0x02
#define AY31015_OR	0x04
#define AY31015_FE	0x08
#define AY31015_PE	0x10
#define AY31015_EOC	0x20

#define AY31015_SO	0x40

// Reset the device
void ay51013_init( void );

// Set Control register
void ay51013_cs( UINT8 data );

// Reset the device
void ay31015_init( void );

// Set Control register
void ay31015_cs( UINT8 data );

// Set Receive Clock
void ay31015_set_rx_clock_speed( UINT32 data );

// Read status register
UINT8 ay31015_swe( void );

// Send a bit to the device
void ay31015_si( UINT8 data );

// Read a byte from the device
UINT8 ay31015_rde( void );

// Tell device you have read the data
void ay31015_rdav( void );

// Set Transmit Clock
void ay31015_set_tx_clock_speed( UINT32 data );

// Get serialised output
UINT8 ay31015_so( void );

// Send a byte to the device
void ay31015_ds( UINT8 data );
