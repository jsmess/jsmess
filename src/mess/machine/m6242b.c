/* OKI m6242B real time clock/callendar
   
   by tim lindner
   Copyright 2001
*/

#include "driver.h"
#include "m6242b.h"
#include "mscommon.h"

static void update_time_registers( void );

int		m6242_register_table[0x10];

/* Table description:							   Bit3  Bit2  Bit1  Bit0

	0x00		1-second digit register				 S8    S4    S2    S1
	0x01		10-second digit register			  x   S40   S20   S10
	0x02		1-minute digit register				Mi8   Mi4   Mi2   Mi1
	0x03		10-minute digit register			  x  Mi40  Mi20  Mi10
	0x04		1-hour digit register				 H8    H4    H2    H1
	0x05		PM/AM, 10-hour digit register		  x   P/A   H20   H10
	0x06		1-day digit register				 D8    D4    D2    D1
	0x07		10-day digit register				  x     x   D20   D10
	0x08		1-month digit register				Mo8   Mo4   Mo2   Mo1
	0x09		10-month digit register			   Mo80  Mo40  Mo20  Mo10
	0x0A		1-year digit register				 Y8    Y4    Y2    Y1
	0x0B		10-yead digit register				Y80   Y40   Y20   Y10
	0x0C		Week register						  x    W4    W2    W1

													 30
	0x0D		Control register D					Sec   IRQ  Busy  Hold
													adj  Flag
													           ITRP
	0x0E		Control register E					 T1    T0 /STND  Mask
													  
													           
	0x0F		Control register F				   Test 24/12  Stop  Rest
													  
	
	Note: Digits are BDC. Registers only four bits wide.
	X denotes 'not used'
*/

UINT8		m6242_register;

void m6242_init( int time_1224 )
{
	memset( m6242_register_table, 0x00, sizeof( int ) * 0x10 );
	
	if( time_1224 == m6242_24_hour_time )
		m6242_register_table[ 0x0f ] = 0x04; // Set for 24 hour time
}

/* m6242_address_r

   This reads the currently latched register address.
*/

READ8_HANDLER ( m6242_address_r )
{
	return m6242_register;
}

/* m6242_data_r

   This reads the data in the currently selected register table.
*/

 READ8_HANDLER ( m6242_data_r )
{
	return m6242_register_table[ m6242_register & 0x0f ];
}

/* m6242_address_w

   This writes the address to the address latch.
*/

WRITE8_HANDLER ( m6242_address_w )
{
	m6242_register = data & 0x0f;

	/* Update time whenever address register is written to */
	update_time_registers();
}

/* m6242_address_w

   This writes the address to the currently selected data register.
*/

WRITE8_HANDLER ( m6242_data_w )
{
	m6242_register_table[ m6242_register ] = data;
}

static void update_time_registers( void )
{
	mame_system_time systime;
	
	/* get the current date/time from the core */
	mame_get_current_datetime(Machine, &systime);
	
	m6242_register_table[ 0x00 ] = dec_2_bcd(systime.local_time.second) & 0x0f;
	m6242_register_table[ 0x01 ] = dec_2_bcd(systime.local_time.second) >> 4;
	m6242_register_table[ 0x02 ] = dec_2_bcd(systime.local_time.minute) & 0x0f;
	m6242_register_table[ 0x03 ] = dec_2_bcd(systime.local_time.minute) >> 4;
	
	if ( m6242_register_table[ 0x0F ] & 0x04 ) /* should we supply 24 hour time */
	{
		m6242_register_table[ 0x04 ] = dec_2_bcd(systime.local_time.hour) & 0x0f;
		m6242_register_table[ 0x05 ] = dec_2_bcd(systime.local_time.hour) >> 4;
	}
	else /* Nope, 12 hour time is requested */
	{
		int	val = systime.local_time.hour;
		
		if( systime.local_time.hour > 12 )
			val -= 12;
			
		m6242_register_table[ 0x04 ] = dec_2_bcd(val) & 0x0f;
		m6242_register_table[ 0x05 ] = dec_2_bcd(val) >> 4;
		
		if( systime.local_time.hour > 12 )	/* set bit high if PM */
			m6242_register_table[ 0x05 ] += 0x04;
	}
	
	m6242_register_table[ 0x06 ] = dec_2_bcd(systime.local_time.mday) & 0x0f;
	m6242_register_table[ 0x07 ] = dec_2_bcd(systime.local_time.mday) >> 4;
	m6242_register_table[ 0x08 ] = dec_2_bcd(systime.local_time.month + 1) & 0x0f;
	m6242_register_table[ 0x09 ] = dec_2_bcd(systime.local_time.month + 1) >> 4;
	m6242_register_table[ 0x0A ] = dec_2_bcd(systime.local_time.year%100) & 0x0f;
	m6242_register_table[ 0x0B ] = dec_2_bcd(systime.local_time.year%100) >> 4;
	m6242_register_table[ 0x0C ] = dec_2_bcd(systime.local_time.weekday) & 0x0f;
}
