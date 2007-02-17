/* OKI m6242B real time clock/callendar
   
   by tim lindner
   Copyright 2001
*/

enum
{
	m6242_12_hour_time,
	m6242_24_hour_time
};

void m6242_init( int time_1224 );

 READ8_HANDLER ( m6242_address_r );
 READ8_HANDLER ( m6242_data_r );
WRITE8_HANDLER ( m6242_address_w );
WRITE8_HANDLER ( m6242_data_w );
