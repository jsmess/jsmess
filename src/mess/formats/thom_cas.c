/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Thomson 8-bit computers

**********************************************************************/

#include <math.h>

#include "timer.h"
#include "state.h"
#include "devices/cassette.h"
#include "formats/cassimg.h"
#include "machine/thomson.h"
#include "formats/thom_cas.h"


/***************************** configuration **************************/

#define K7_SPEED_HACK 1
/* When set to 1, bits extracted from k7 files are not transformed into a
   raw wave. It saves some time & memory but disables the ability to "hear" the
   cassette.
   This does not affect raw wave files.
   Also, we do not do this for k5 files because the the corresponding wave
   files are more compact anyway.
*/

#define VERBOSE 0 /* 0, 1 or 2 */


/***************************** utilities **************************/

#define PRINT(x) mame_printf_info x

#if VERBOSE > 1
#define LOG(x)	logerror x
#define VLOG(x)	logerror x
#elif VERBOSE > 0
#define LOG(x)	logerror x
#define VLOG(x)
#else
#define LOG(x)
#define VLOG(x)
#endif


/*************************** I / O **************************/


INLINE mess_image* thom_cassette_img( void )
{ return image_from_devtype_and_index(IO_CASSETTE, 0); }



/*-------------- TO7 ------------*/


/* On the TO7 & compatible (TO7/70,TO8,TO9, but not MO5,MO6), bits are coded
   in FM format with a 1.1 ms period (900 bauds):
   - 0 is 5 periods at 4.5 KHz
   - 1 is 7 periods at 6.3 KHz
   
   Moreover, a byte is represented using 11 bits:
   - one 0 start bit
   - eight data bits (low bit first)
   - two 1 stop bits

   There are also long (1 s) sequences of 1 bits to re-synchronize the
   cassette at places the motor can be cut off and back on (e.g., between 
   files).

   The computer outputs a modulated wave that is directly put on the cassette.
   However, the input is demodulated by the cassette-reader before being
   sent to the computer: we got 0 when the signal is around 4.5 KHz and 
   1 when the signal is around 6.3 KHz.
*/

#define TO7_BIT_LENGTH 0.001114

#define TO7_FREQ_CASS_0 4500.
#define TO7_FREQ_CASS_1 6300.

#define TO7_PERIOD_CASS_0 ( 1. / TO7_FREQ_CASS_0 )
#define TO7_PERIOD_CASS_1 ( 1. / TO7_FREQ_CASS_1 )

/* buffer storing demodulated bits, only for k7 and with speed hack */
static UINT32 to7_k7_bitsize;
static UINT8* to7_k7_bits;


/* 1-bit cassette input to the computer 
   inside the controller, two frequency filters (adjusted to 6.3 and 4.5 KHz) 
   and a comparator demodulate the raw signal into 0s and 1s.
*/
int to7_get_cassette ( void )
{
  mess_image* img = thom_cassette_img();

  if ( image_exists( img ) ) {
    cassette_image* cass = cassette_get_image( img );
    cassette_state state = cassette_get_state( img );
    double pos = cassette_get_position( img );
    int bitpos = pos / TO7_BIT_LENGTH;
    
    if ( (state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_DISABLED ) return 1;

    if ( K7_SPEED_HACK && to7_k7_bits ) {
      /* hack, feed existing bits */
      if ( bitpos >= to7_k7_bitsize ) bitpos = to7_k7_bitsize -1;
      VLOG (( "$%04x %f to7_get_cassette: state=$%X pos=%f samppos=%i bit=%i\n", 
	      activecpu_get_previouspc(), timer_get_time(), state, pos, bitpos, 
	      to7_k7_bits[ bitpos ] ));
      return to7_k7_bits[ bitpos ];
    }
    else {
      /* demodulate wave signal on-the-fly */
      /* we simply count sign changes... */
      int k, chg;
      INT8 data[40];
      cassette_get_samples( cass, 0, pos, TO7_BIT_LENGTH * 15. / 14., 40, 1, 
			    data, 0 );

      for ( k = 1, chg = 0; k < 40; k++ ) {
	if ( data[ k - 1 ] >= 0 && data[ k ] < 0 ) chg++;
	if ( data[ k - 1 ] <= 0 && data[ k ] > 0 ) chg++;
      }
      k = ( chg >= 13 ) ? 1 : 0;
      VLOG (( "$%04x %f to7_get_cassette: state=$%X pos=%f samppos=%i bit=%i (%i)\n", 
	      activecpu_get_previouspc(), timer_get_time(), state, pos, bitpos, 
	      k, chg ));
      return k;
    }
    
  }

  else return 0;
}

/* 1-bit cassette output */
void to7_set_cassette ( int data )
{
  mess_image* img = thom_cassette_img();
  cassette_output( img, data ? 1. : -1. );
}

WRITE8_HANDLER ( to7_set_cassette_motor )
{
  mess_image* img = thom_cassette_img();
  cassette_state state = cassette_get_state( img );
  double pos = cassette_get_position(img);

  LOG (( "$%04x %f to7_set_cassette_motor: cassette motor %s bitpos=%i\n",
	 activecpu_get_previouspc(), timer_get_time(), data ? "off" : "on",
	 (int) (pos / TO7_BIT_LENGTH) ));

  if ( (state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_DISABLED && 
       !data && pos > 0.3 ) {
    /* rewind a little before starting the motor */
    cassette_seek( img, -0.3, SEEK_CUR );
  }

  cassette_change_state( img, data ? CASSETTE_MOTOR_DISABLED : 
			 CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR );
}


/*-------------- MO5 ------------*/


/* Each byte is represented as 8 bits without start or stop bit (unlike TO7).
   Bits are coded in MFM, and the MFM signal is directly fed to the
   computer which has to decode it in software (unlike TO7).
   A 1 bit is one period at 1200 Hz; a 0 bit is one half-period at 600 Hz.
   Bit-order is most significant bit first (unlike TO7).
   
   Double-density MO6 cassettes follow the exact same mechanism, but with
   at double frequency (perdiods at 2400 Hz, and half-perdios at 1200 Hz).
*/


#define MO5_BIT_LENGTH   0.000833
#define MO5_HBIT_LENGTH (MO5_BIT_LENGTH / 2.)

int mo5_get_cassette ( void )
{
  mess_image* img = thom_cassette_img();

  if ( image_exists( img ) ) {
    cassette_image* cass = cassette_get_image( img );
    cassette_state state = cassette_get_state( img );
    double pos = cassette_get_position( img );
    INT32 hbit;

    if ( (state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_DISABLED ) return 1;

    cassette_get_sample( cass, 0, pos, 0, &hbit );
    hbit = hbit >= 0;
    
    VLOG (( "$%04x %f mo5_get_cassette: state=$%X pos=%f hbitpos=%i hbit=%i\n", 
	    activecpu_get_previouspc(), timer_get_time(), state, pos, 
	    (int) (pos / MO5_HBIT_LENGTH), hbit ));
    return hbit;
  }

  else return 0;
}

void mo5_set_cassette ( int data )
{
  mess_image* img = thom_cassette_img();
  cassette_output( img, data ? 1. : -1. );
}

WRITE8_HANDLER ( mo5_set_cassette_motor )
{
  mess_image* img = thom_cassette_img();
  cassette_state state = cassette_get_state( img );
  double pos = cassette_get_position(img);

  LOG (( "$%04x %f mo5_set_cassette_motor: cassette motor %s hbitpos=%i\n",
	 activecpu_get_previouspc(), timer_get_time(), data ? "off" : "on",
	 (int) (pos / MO5_HBIT_LENGTH) ));

  if ( (state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_DISABLED && 
       !data && pos > 0.3 ) {
    /* rewind a little before starting the motor */
    cassette_seek( img, -0.3, SEEK_CUR );
  }

  cassette_change_state( img, data ? CASSETTE_MOTOR_DISABLED : 
			 CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR );
}



/************************* k7 format *************************/

/* The k7 format is widespread among existing Thomson emulators.
   It represents the stream of bytes returned by the BIOS cassette routine,
   and so, is used by emulators that bypass the BIOS routine.
   It is generally hacked from the raw image more or less by hand
   (in particular, to by-pass special loaders and copy-protection schemes).

   As we run the original BIOS routine, we need to hack back the k7 file
   into the original stream.

   In theory, the byte stream is a sequence of blocks of the form:
     - 2-byte header: 01 3C
     - 1-byte type
           00 = start of file, the block contains the filename
	   01 = chunk of file data
	   FF = end of file
     - 1-byte size
     - size bytes of data
     - one byte of CRC

  These are separated by 0xff fillers.

  There are also a variety of non-standard blocks, and we try to handle them.

  Our main problem is to guess where the places where the motor will be
  cut-off and introduce there sequences of 1 bit for synchronization.

  Note: as a nice effect, if our routine succeeds, it constructs a wave
  file that should be usable on a real computer.
*/

static const struct CassetteModulation to7_k7_modulation = {
  CASSETTE_MODULATION_SQUAREWAVE,
  4000.0,  4500.0, 5000.0,
  5500.0,  6300.0, 7500.0
};

static casserr_t to7_k7_identify ( cassette_image *cass, 
				   struct CassetteOptions *opts ) 
{
  casserr_t e = cassette_modulation_identify( cass, &to7_k7_modulation, opts );
  return e;
}

static casserr_t to7_k7_load( cassette_image *cass ) 
{
#if ! K7_SPEED_HACK
  static const INT8 square_wave[] = { -128, 127 };
  double time = 0.;
#endif
  size_t size = cassette_image_size( cass ), pos = 0;
  int i, sz, sz2, bitmax = 1024, invalid = 0;
  UINT8 in, typ, block[264];

  LOG (( "to7_k7_load: start conversion, size=%li\n", (long)size ));
  PRINT (( "to7_k7_load: open cassette, length: %li bytes\n", (long) size ));

  if ( to7_k7_bits ) { free( to7_k7_bits ); to7_k7_bits = NULL; }
  
  to7_k7_bitsize = 0;
  to7_k7_bits = auto_malloc( bitmax );

/* store one period */
#if K7_SPEED_HACK
#define K7_PUT( PERIOD )
#else
#define K7_PUT( PERIOD ) do {						\
    casserr_t err;							\
    err = cassette_put_samples( cass, 0, time, (PERIOD), 2, 1,		\
				square_wave, CASSETTE_WAVEFORM_8BIT );	\
    if ( err ) return err;						\
    time += (PERIOD);							\
  } while (0)
#endif

/* store one bit */
#define K7_PUT_BIT( BIT ) do {						\
    int b;								\
    if ( BIT ) for ( b = 0; b < 7; b++ ) K7_PUT( TO7_PERIOD_CASS_1 );	\
    else       for ( b = 0; b < 5; b++ ) K7_PUT( TO7_PERIOD_CASS_0 );	\
    if ( to7_k7_bitsize + 1 >= bitmax ) {			\
      UINT8* a = auto_malloc( bitmax * 2);				\
      memcpy ( a, to7_k7_bits, bitmax );				\
      bitmax *= 2;							\
      to7_k7_bits = a;							\
    }									\
    to7_k7_bits[ to7_k7_bitsize++ ] = (BIT);				\
  } while (0)

/* store one byte, with start / stop bits */
#define K7_PUT_BYTE( BYTE ) do {			\
    UINT8 x;						\
    K7_PUT_BIT( 0 );					\
    for ( x = 0; x < 8; x++ )				\
      K7_PUT_BIT( ( (BYTE) >> x ) & 1 );		\
    K7_PUT_BIT( 1 );					\
    K7_PUT_BIT( 1 );					\
  } while (0)
  
#define K7_FILL_1( SIZE ) do {						\
    if ( (SIZE) > 0 ) {							\
      int ii;								\
      LOG (( "to7_k7_load: 1-filler size=%i bitstart=%i\n",		\
	     (SIZE), to7_k7_bitsize ));					\
      for ( ii = 0; ii < (SIZE); ii++ ) K7_PUT_BIT( 1 );		\
    }									\
  } while (0)
  
#define K7_FILL_ff( SIZE ) do {					\
    if ( (SIZE) > 0 ) {						\
      int ii;							\
      LOG (( "to7_k7_load: 0xff filler size=%i bitstart=%i\n",	\
	     (SIZE), to7_k7_bitsize ));				\
      for ( ii = 0; ii < (SIZE); ii++ ) K7_PUT_BYTE( 0xff );	\
    }								\
  } while (0)

  /* check format */
  cassette_image_read( cass, block, 0, 64 );
  for ( i = 3; ; i++ ) {
    if ( ( i >= size ) || ( i >= 64 ) ) {
      /* ? */
      PRINT (( "to7_k7_load: WARNING: this does not look like a MO or TO cassette.\n" ));
      break;
    }
    else if ( ( block[i-3] == 0x01 ) && ( block[i-2] == 0x3c ) && 
	      ( block[i-1] == 0x5a ) && ! block[i] ) {
      /* MO */
      PRINT (( "to7_k7_load: WARNING: this looks like a MO cassette, not a TO one.\n" ));
      break;
    }
    else if ( ( block[i-3] == 0xff ) && ( block[i-2] == 0x01 ) && 
	      ( block[i-1] == 0x3c ) && ! block[i] ) {
      /* TO */
      break;
    }
  }

  /* skip to first 0xff filler */
  for ( sz = 0; pos < size; pos++, sz++ ) {
    cassette_image_read( cass, &in, pos, 1 );
    if ( in == 0xff ) break;
  }
  if ( sz > 0 ) LOG (( "to7_k7_load: skip %i trash bytes\n", sz ));

  /* loop over regular blocks */
  while ( pos < size ) {
    
  rebounce:
    /* skip 0xff filler */
    for ( sz = 0; pos < size; pos++, sz++ ) {
      cassette_image_read( cass, &in, pos, 1 );
      /* actually, we are bit laxist and treat as 0xff bytes with at least
	 5 bits out of 8 set to 1
      */
      for ( i = 0; in; in >>= 1 ) i += (in & 1);
      if ( i < 5 ) break;
    }

    /* get block header */
    if ( pos + 4 > size ) { pos -= sz; break; }
    cassette_image_read( cass, block, pos, 4 );
    typ = block[2];
    sz2 = block[3]+1;
    if ( block[0] != 0x01 || block[1] != 0x3c ||
	 ( typ != 0x00 && typ != 0x01 && typ !=  0xff ) ) { pos -= sz; break; }
    pos += 4;

    /* get block */
    cassette_image_read( cass, block+4, pos, sz2 );
    pos += sz2;

    /* 1-filler and 0xff-filler */
    if ( typ == 0 || typ == 0xff ) K7_FILL_1( 1000 );
    K7_FILL_ff( sz );

    /* put block */
    LOG (( "to7_k7_load: block off=$%x type=$%02X size=%i bitstart=%i\n", 
	   (int) pos-sz2-4, typ, sz2, to7_k7_bitsize ));
    VLOG (( "to7_k7_load: data:" ));
    for ( i = 0; i < sz2 + 4; i ++) {
      VLOG (( " $%02X", block[i] ));
      K7_PUT_BYTE( block[i] );
    }
    VLOG (( "\n" ));

    /* if it is a directory enty, says so to the user */
    if ( typ == 0 ) {
      char name[] = "01234567.ABC";
      UINT8 t = block[15];
      UINT8 u = block[16];
      int p = (to7_k7_bitsize - sz2 - 4 - sz) * TO7_BIT_LENGTH;
      memcpy( name, block+4, 8 );
      memcpy( name+9, block+12, 3 );
      for ( i = 0; name[i]; i++)
	if ( name[i] < ' ' || name[i] >= 127 ) name[i] = '?';
      PRINT (( "to7_k7_load: file \"%s\" type=%s,%s at %imn %is\n", 
	       name, 
	       (t==0) ? "bas" : (t==1) ? "dat" : (t==2) ? "bin" : "???",
	       (u == 0) ? "a" : (u == 0xff) ? "b" : "?",
	       p / 60, p % 60 ));
    }

    /* extra 1-fillers */
    if ( typ == 0 || typ == 0xff ) K7_FILL_1( 1000 );
  }

  /* trailing data with invalid block structure
     => dump it in a raw form, but stay alert for hidden block starts
   */
  if ( pos < size ) {
    invalid++;
    LOG (( "to7_k7_load: trailing raw bytes off=$%x bitstart=%i\n", 
	   (int) pos, to7_k7_bitsize ));

    /* put block */
    for (; pos < size; pos++ ) {
      cassette_image_read( cass, &in, pos, 1 );
      for ( sz = 0; pos < size && in == 0xff; sz++ ) {
	pos++;
	cassette_image_read( cass, &in, pos, 1 ); 
      }
      if ( invalid < 10 && sz > 4 && in == 0x01 && pos + 4 <= size ) {
	UINT8 in1,in2;
	cassette_image_read( cass, &in1,   pos+1, 1 );
	cassette_image_read( cass, &in2, pos+2, 1 );
	if ( (in1 == 0x3c) && ((in2 == 0x00) || (in2 == 0x01) ) ) {
	  /* seems we have a regular block hidden in raw data => rebounce */
	  LOG (( "to7_k7_load: hidden regular block found\n" ));
	  pos -= sz;
	  goto rebounce;
	}
	if ( ( ( in1 == 0x3d ) && ( in2 == 0 ) ) ||
	     ( ( in1 == 0x57 ) && ( in2 == 0x49 ) ) ) {
	  /* special block (Infogrames) => just prepend filler */
	  K7_FILL_1 ( 500 );
	  LOG (( "to7_k7_load: special $%02X $%02X $%02X block found off=$%x bitstart=%i\n", 
		 in, in1, in2, (int) pos, to7_k7_bitsize ));
	}
      }
       for ( i = 0; i < sz; i++ )
	 K7_PUT_BYTE( 0xff );
       K7_PUT_BYTE( in );
    }
  }

  if ( invalid )
    PRINT (( "to7_k7_load: WARNING: the k7 has an unknown structure and may not work\n" ));

  sz = to7_k7_bitsize * TO7_BIT_LENGTH;
  PRINT (( "to7_k7_load: cassette length: %imn %is (%i samples)\n", 
	  sz / 60, sz % 60, to7_k7_bitsize ));

  return CASSETTE_ERROR_SUCCESS;
}  


static struct CassetteFormat to7_k7 = 
  { "k7\0", to7_k7_identify, to7_k7_load, NULL /* no save */ };



/********************* TO7 WAV format ************************/

static casserr_t to7_wav_identify ( cassette_image *cass, 
				    struct CassetteOptions *opts ) 
{
  casserr_t e = wavfile_format.identify( cass, opts );
  return e;
}

static casserr_t to7_wav_load ( cassette_image *cass )
{
  casserr_t e = wavfile_format.load( cass );
  struct CassetteInfo info;
  double len;

  if ( to7_k7_bits ) { free( to7_k7_bits ); to7_k7_bits = NULL; }

  if ( e != CASSETTE_ERROR_SUCCESS ) return e;
  cassette_get_info( cass, &info );

  len = (double) info.sample_count / info.sample_frequency;
  
  PRINT (( "to7_wav_load: loading cassette, length %imn %is, %i Hz, %i bps, %i bits\n",
	   (int) len / 60, (int) len % 60, 
	   info.sample_frequency, info.bits_per_sample, 
	   (int) (len / TO7_BIT_LENGTH) ));

  return CASSETTE_ERROR_SUCCESS;
}

static casserr_t to7_wav_save ( cassette_image *cass, 
				const struct CassetteInfo *info )
{
  int len = info->sample_count / info->sample_frequency;
  PRINT (( "to7_wav_save: saving cassette, length %imn %is, %i Hz, %i bps\n",
	   len / 60, len % 60, info->sample_frequency, info->bits_per_sample ));
  return wavfile_format.save( cass, info );
}

/* overloaded wav: dump info */
const static struct CassetteFormat to7_wav = 
  { "wav\0", to7_wav_identify, to7_wav_load, to7_wav_save };



/************************* k5 format *************************/

static casserr_t mo5_k5_identify ( cassette_image *cass, 
				   struct CassetteOptions *opts ) 
{
  opts -> bits_per_sample = 8;
  opts -> channels = 1;
  opts -> sample_frequency = 22100;//11050;
  return CASSETTE_ERROR_SUCCESS;
}

/* As the k7 format, the k5 format represents the stream of bytes returned by 
   the BIOS cassette routine and need to be hacked back into the original
   cassette wave.

   The format of blocks is a bit different from that of TO7 cassettes:
     - 2-byte header is: 3C 5A
     - 1-byte type
           00 = start of file, the block contains the filename
	   01 = chunk of file data
	   FF = end of file
     - 1-byte size
     - size - 1 bytes of data (255 bytes of data if size = 0)
     - one byte of CRC: sum from (and including) header up to (and including) 
     CRC byte should be 0 modulo 256

  They are separated by 0x01 filler bytes (generally 10 of them), 
  for synchronisation. There are also extra 0 filler bits between certain 
  kinds of blocks, when the motor is supposed to go off and back on.
*/

static casserr_t mo5_k5_load( cassette_image *cass ) 
{
  size_t size = cassette_image_size( cass ), pos = 0;
  int i, sz, sz2, hbit = 0;
  UINT8 in, in2, in3, typ, block[264], sum;
  int invalid = 0, hbitsize = 0, dcmoto = 0;

  LOG (( "mo5_k5_load: start conversion, size=%li\n", (long)size ));
  PRINT (( "mo5_k5_load: open cassette, length: %li bytes\n", (long) size ));
  
  /* store a half-bit */
#define K5_PUT_HBIT do {						\
    cassette_put_sample ( cass, 0, hbitsize * MO5_HBIT_LENGTH,		\
			  MO5_HBIT_LENGTH, (hbit ? 1 : -1) << 30 );	\
    hbitsize++;								\
  } while ( 0 )
  
  /* store one bit */
#define K5_PUT_BIT( BIT ) do {						\
    if ( BIT ) {							\
      K5_PUT_HBIT;							\
      hbit = !hbit;							\
      K5_PUT_HBIT;							\
    }									\
    else {								\
      K5_PUT_HBIT;							\
      K5_PUT_HBIT;							\
    }									\
    hbit = !hbit;							\
  } while (0)
  
  /* store one byte, no start / stop bit, converse order from TO7 */
#define K5_PUT_BYTE( BYTE ) do {			\
    UINT8 b = BYTE;					\
    int x;						\
    for ( x = 0; x < 8; x++ )				\
      K5_PUT_BIT( (b >> (7 - x)) & 1 );			\
  } while (0)
  
  /* store filler */
#define K5_FILL_0( SIZE ) do {						\
    if ( (SIZE) > 0 ) {							\
      int i;								\
      LOG (( "mo5_k5_load: 0-filler size=%i hbitstart=%i\n",		\
	     (SIZE), hbitsize ));					\
      for ( i = 0; i < (SIZE); i++ ) K5_PUT_BIT( 0 );			\
    }									\
  } while (0)
  
#define K5_FILL_01( SIZE )  do {				\
    if ( (SIZE) > 0 ) {						\
      int i;							\
      LOG (( "mo5_k5_load: 0x01 filler size=%i bitstart=%i\n",	\
	     (SIZE), hbitsize ));				\
      for ( i = 0; i < (SIZE); i++ ) K5_PUT_BYTE( 0x01 );	\
    }								\
  } while (0)							\
    
  /* check format */
  cassette_image_read( cass, block, 0, 64 );
  for ( i = 3; ; i++ ) {
    if ( ( i >= size ) || ( i >= 64 ) ) {
      /* ? */
      PRINT (( "to5_k5_load: WARNING: this does not look like a MO or TO cassette.\n" ));
      break;
    }
    else if ( ( block[i-3] == 0x01 ) && ( block[i-2] == 0x3c ) && 
	      ( block[i-1] == 0x5a ) && ! block[i] ) {
      /* MO */
      break;
    }
    else if ( ( block[i-3] == 0xff ) && ( block[i-2] == 0x01 ) && 
	      ( block[i-1] == 0x3c ) && ! block[i] ) {
      /* TO */
      PRINT (( "to5_k5_load: WARNING: this looks like a TO cassette, not a MO one.\n" ));
      break;
    }
  }

  cassette_image_read( cass, block, pos, 6 );
  if ( ! memcmp( block, "DCMOTO", 6 ) ||
       ! memcmp( block, "DCMO5", 5 ) ||
       ! memcmp( block, "DCMO6", 5 ) ) dcmoto = 1;
  
  /* loop over regular blocks */
  while ( pos < size ) {

  rebounce:
    /* skip DCMOTO header*/
    if ( dcmoto ) {
      cassette_image_read( cass, block, pos, 6 );
      if ( ! memcmp( block, "DCMOTO", 6 ) ) {
	LOG (( "mo5_k5_load: DCMOTO signature found at off=$%x\n", pos ));
	pos += 6;
      }
      else if ( ! memcmp( block, "DCMO", 4 ) ) {
	LOG (( "mo5_k5_load: DCMO* signature found at off=$%x\n", pos ));
	pos += 5;
      }
    }
    
    /* skip 0x01 filler */
    for ( sz = 0; pos < size; pos++, sz++ ) {
      cassette_image_read( cass, &in, pos, 1 );
      if ( in != 0x01 ) break;
    }

    /* get block header */
    if ( pos + 4 > size ) { pos -= sz; break; }
    cassette_image_read( cass, block, pos, 4 );
    typ = block[2];
    sz2 = (UINT8) (block[3]-1);
    if ( block[0] != 0x3c || block[1] != 0x5a ||
	 ( typ != 0x00 && typ != 0x01 && typ !=  0xff ) || 
	 pos+sz2 > size ) 
      { pos -= sz; break; }
    pos += 4;

    /* get block */
    cassette_image_read( cass, block+4, pos, sz2 );
    pos += sz2;

    /* 0-fillers and 0x01-fillers */
    if ( typ == 0 || typ == 0xff ) K5_FILL_0( 1200 );
    else K5_FILL_0( 300 ); /* for MO6 */
    K5_FILL_01( sz < 10 ? 10 : sz );

    /* put block */
    LOG (( "mo5_k5_load: block off=$%x type=$%02X size=%i hbitstart=%i\n", 
	   (int) pos-sz2-4, typ, sz2, hbitsize ));
    VLOG (( "mo5_k5_load: data:" ));
    for ( i = 0; i < sz2 + 4; i ++) {
      VLOG (( " $%02X", block[i] ));
      K5_PUT_BYTE( block[i] );
    }
    VLOG (( "\n" ));

    /* checksum */
    for ( i = 0, sum = 0; i < sz2; i++ ) 
      sum += block[i+4];
    if ( sum )
      LOG(( "mo5_k5_load: invalid checksum $%02X (should be 0)\n", sum ));

    /* if it is a directory enty, says so to the user */
    if ( typ == 0 ) {
      char name[] = "01234567.ABC";
      UINT8 t = block[15];
      UINT8 u = block[16];
      int p = (hbitsize - sz2 - 4 - sz) * MO5_HBIT_LENGTH;
      memcpy( name, block+4, 8 );
      memcpy( name+9, block+12, 3 );
      for ( i = 0; name[i]; i++)
	if ( name[i] < ' ' || name[i] >= 127 ) name[i] = '?';
      PRINT (( "mo5_k5_load: file \"%s\" type=%s,%s at %imn %is\n", 
	       name, 
	       (t==0) ? "bas" : (t==1) ? "dat" : (t==2) ? "bin" : "???",
	       (u == 0) ? "a" : (u == 0xff) ? "b" : "?",
	       p / 60, p % 60 ));
    }

    /* 0-fillers */
    if ( typ == 0xff || typ == 0x00 ) K5_FILL_0( 1800 );
  }

  /* dump trailing bytes, but also look for beginings of blocks */
  if ( pos < size ) {
    invalid++;
    K5_FILL_0( 1200 );
    LOG (( "mo5_k5_load: trailing trash off=$%x size=%i hbitstart=%i\n", 
	   (int) pos, (int) size-pos, hbitsize ));
    for ( ; pos < size; pos++ ) {
      cassette_image_read( cass, &in, pos, 1 ); 
      if ( dcmoto && in=='D' ) {  
	/* skip DCMOTO header*/
	cassette_image_read( cass, block, pos, 6 );
	if ( ! memcmp( block, "DCMOTO", 6 ) ) {
	  LOG (( "mo5_k5_load: DCMOTO signature found at off=$%x\n", pos ));
	  pos += 6; 
	  cassette_image_read( cass, &in, pos, 1 ); 
	}
 	else if ( ! memcmp( block, "DCMO", 4 ) ) {
	  LOG (( "mo5_k5_load: DCMO* signature found at off=$%x\n", pos ));
	  pos += 5; 
	  cassette_image_read( cass, &in, pos, 1 ); 
	}
      }
      for ( sz = 0; pos < size && in == 0x01; sz++ ) {
	pos++;
	cassette_image_read( cass, &in, pos, 1 ); 
      }
      if ( sz > 6 ) {
	cassette_image_read( cass, &in2, pos+1, 1 );
	cassette_image_read( cass, &in3, pos+2, 1 );
	if ( invalid < 10 &&  in == 0x3c && in2 == 0x5a &&
	     (in3 == 0x00 || in3 == 0x01 || in3 == 0xff ) ) {
	  /* regular block found */
	  LOG (( "mo5_k5_load: hidden regular block found\n" ));
	  pos -= sz;
	  goto rebounce;
	}
	if ( invalid < 10 && sz > 6 && 
	     ( (in == 0x3c && in2 == 0x5a) ||
	       (in == 0xc3 && in2 == 0x5a) ||
	       (in == 0xc3 && in2 == 0x3c) ||
	       (in == 0x87 && in2 == 0x4a)  ) ) {
	  /* special block found */
	  K5_FILL_0( 1200 );
	  LOG (( "mo5_k5_load: special block $%02X $%02X found off=$%x hbitstart=%i\n",
		 in, in2, (int) pos-sz, hbitsize ));
	}
      }
 
      VLOG (( "mo5_k5_load: special data:" ));
      for ( i = 0; i < sz; i++ ) {
	K5_PUT_BYTE( 0x01 );
	VLOG (( " $01" ));
      }
      K5_PUT_BYTE( in );
      VLOG (( " $%02X\n", in ));
    }
  }

  if ( invalid ) 
    PRINT (( "mo5_k5_load: WARNING: the k5 has an unknown structure and may not work\n" ));

  sz = hbitsize * MO5_HBIT_LENGTH;
  PRINT (( "mo5_k5_load: cassette length: %imn %is (%i half-bits)\n", 
	   sz / 60, sz % 60, hbitsize ));
  
  return CASSETTE_ERROR_SUCCESS;
}

static struct CassetteFormat mo5_k5 = 
  { "k5\0k7\0", mo5_k5_identify, mo5_k5_load, NULL /* no save */ };


/********************* MO5 WAV format ************************/

casserr_t mo5_wav_identify ( cassette_image *cass, 
			     struct CassetteOptions *opts ) 
{
  casserr_t e = wavfile_format.identify( cass, opts );
  return e;
}

casserr_t mo5_wav_load ( cassette_image *cass )
{
  casserr_t e = wavfile_format.load( cass );
  struct CassetteInfo info;
  int len;
  if ( e == CASSETTE_ERROR_SUCCESS ) {
    cassette_get_info( cass, &info );
    len = info.sample_count / info.sample_frequency;
    PRINT (( "mo5_wav_load: loading cassette, length %imn %is, %i Hz, %i bps\n",
	     len / 60, len % 60, info.sample_frequency, info.bits_per_sample ));
  }
  return e;
}

casserr_t mo5_wav_save ( cassette_image *cass,
			 const struct CassetteInfo *info )
{
  int len = info->sample_count / info->sample_frequency;
  PRINT (( "mo5_wav_save: saving cassette, length %imn %is, %i Hz, %i bps\n",
	   len / 60, len % 60, info->sample_frequency, info->bits_per_sample ));
  return wavfile_format.save( cass, info );
}

/* overloaded wav: dump info */
const static struct CassetteFormat mo5_wav = 
  { "wav\0", mo5_wav_identify, mo5_wav_load, mo5_wav_save };


/********************* formats ************************/

const struct CassetteFormat *to7_cassette_formats[] = 
  { &to7_wav, &to7_k7, NULL };


const struct CassetteFormat *mo5_cassette_formats[] = 
  { &mo5_wav, &mo5_k5, NULL };


/********************* devices ************************/

void to7_cassette_getinfo( const device_class *devclass, 
			   UINT32 state, union devinfo *info )
{
  switch ( state ) {
  case DEVINFO_INT_COUNT:        
    info->i = 1;
    break;
  case DEVINFO_PTR_CASSETTE_FORMATS:
    info->p = (void *) to7_cassette_formats; 
    break;
  case DEVINFO_INT_CASSETTE_DEFAULT_STATE:  
    info->i = 
      CASSETTE_PLAY | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED; 
    break;
  case DEVINFO_STR_DEV_FILE:
    strcpy( info->s = device_temp_str(), __FILE__ );
    break;
  default: 
    cassette_device_getinfo( devclass, state, info );
  }
}

void mo5_cassette_getinfo( const device_class *devclass, 
			   UINT32 state, union devinfo *info )
{
  switch ( state ) {
  case DEVINFO_INT_COUNT:        
    info->i = 1;
    break;
  case DEVINFO_PTR_CASSETTE_FORMATS:
    info->p = (void *) mo5_cassette_formats; 
    break;
  case DEVINFO_INT_CASSETTE_DEFAULT_STATE:  
    info->i = 
      CASSETTE_PLAY | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED; 
    break;
  case DEVINFO_STR_DEV_FILE:
    strcpy( info->s = device_temp_str(), __FILE__ );
    break;
  default: 
    cassette_device_getinfo( devclass, state, info );
  }
}
