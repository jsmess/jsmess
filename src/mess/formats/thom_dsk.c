/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Thomson 8-bit computers

**********************************************************************/

#include "driver.h"
#include "timer.h"
#include "state.h"
#include "device.h"
#include "formats/thom_dsk.h"
#include "video/thomson.h"

#define VERBOSE 0  /* 0, 1 or 2 */

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


/********************* floppy image / drive *********************/


/* sector flags */
#define THOM_SECT_OK       0
#define THOM_SECT_INVALID  1 /* invalid header */


typedef struct {

  /* disk info */
  thom_floppy_type type;
  UINT8*  data;            /* raw contents */
  UINT8   sides;           /* 1 or 2 */
  DENSITY density;
  UINT16  sector_size;     /* 128 or 256 */
  UINT16  sectors;         /* sectors per track: 16 or 400 (QDD) */
  UINT16  tracks;          /* tracks per side: 40, 80, or 1 */
  UINT8   sect_flag[2560]; /* one flag per sector */

  /* current state */
  UINT8  cur_track;

  /* for write-back */
  mess_image* image;
  UINT8       has_changed;

} thom_floppy_drive;

static thom_floppy_drive thom_floppy_drives[4];

static DENSITY thom_density;


/*********************** floppy access ************************/

/* default interlacing factor for floppies */
static const int thom_floppy_interlace = 7;

/* fixed interlacing map for QDDs */
static int thom_qdd_map[400];

static void thom_qdd_compute_map ( void )
{
  /* this map is hardcoded in the QDD BIOS */
  static const int p[6][4] =
    { { 20,  2, 14,  8 }, { 21, 19, 13,  7 },
      { 22, 18, 12,  6 }, { 23, 17, 11,  5 },
      { 24, 16, 10,  4 }, {  1, 15,  9,  3 } };
  static const int q[4] = { 0, 8, 4, 12 };
  int t, s;
  for ( t = 0; t < 24; t++ )
    for ( s = 0; s < 16; s++ )
      thom_qdd_map[ t*16 + s ] = p[ t/4 ][ s%4 ] * 16 + (s/4) + 4*(t%4);
  for ( s = 0; s < 16; s++ )
    thom_qdd_map[ 24*16 + s ] = q[ s%4 ] + (s/4);
  LOG(( "thom_qdd_compute_map: QDD sector map\n" ));
  for ( s = 0; s < 25; s++ ) {
    for ( t = 0; t < 16; t++ )
      VLOG(( "%03i,", thom_qdd_map[ s*16 + t ] + 1 ));
    VLOG(( "\n" ));
  }
}

static UINT8* thom_floppy_sector_ptr ( thom_floppy_drive* d, 
				       int sector, int track, int side )
{
  if ( track >= d->tracks ) track =  d->tracks - 1;
  if ( track < 0 ) track = 0;
  if ( d->data && 
       sector >= 1 && sector <= d->sectors &&
       side >= 0 && side < d->sides ) {
    int s = track * d->sectors + sector - 1;
    if ( d -> type == THOM_FLOPPY_QDD ) {
      if ( s >= 0 && s < 400 ) s = thom_qdd_map[ s ];
      else return NULL;
    }
    return d->data + ( side * d->tracks * d->sectors + s ) * d->sector_size;
  }
  else
    return NULL;
}

static UINT8* thom_floppy_sector_flag ( thom_floppy_drive* d, 
					int sector, int track, int side )
{
  int s = track * d->sectors + sector - 1;
  if ( d -> type == THOM_FLOPPY_QDD ) s = thom_qdd_map[ s ];
  return d->sect_flag + side * d->tracks * d->sectors + s;
}


/* dump some info on the floppy for the user */
static void thom_floppy_show ( thom_floppy_drive* d )
{
  /* format */
  PRINT (( "thom_floppy: drive %li, %s floppy, %s-sided, %i-byte sectors, %s, %i track(s), %i sectors, %i KB\n",
	   (long) (d - thom_floppy_drives),
	   (d->type == THOM_FLOPPY_5_1_4) ? "5\"1/4" : 
	   (d->type == THOM_FLOPPY_3_1_2) ? "3\"1/2" : 
	   (d->type == THOM_FLOPPY_QDD) ? "QDD" : "unknown type",
	   (d->sides == 1) ? "one" : "two",
	   d->sector_size,
	   (d->density == DEN_FM_LO) ? "FM" : "MFM",
	   d->tracks, d->sectors,
	   (d->sides * d->tracks * d->sectors * d->sector_size) / 1024 ));
  LOG (( "thom_floppy: drive %li loaded\n", d - thom_floppy_drives ));

  if ( d->data ) {
    UINT8* dir = d->data + 20 * 16 * d->sector_size;
    char name[] = "01234567";
    int i, j;

    /* BASIC name */
    memcpy( name, dir, 8 );
    for ( i = 0; name[i]; i++ )
      if ( name[i] < ' ' || name[i] >= 127 ) name[i] = '?';
    PRINT (( "thom_floppy: floppy name \"%s\"\n", name ));

    /* BASIC directory */
    dir += d->sector_size*2;
    for ( j = 0; j < d->sector_size * 14 / 32; j++, dir += 32 ) {
      char name[] = "01234567.ABC";
      char comment[] = "01234567";
      if ( *dir == 255 ) break; /* end directory */
      if ( *dir < ' ' || *dir > 127 ) continue; /* empty entry */
      memcpy( name, dir, 8 );
      memcpy( name+9, dir+8, 3 );
      for ( i = 0; name[i]; i++)
	if ( name[i] < ' ' || name[i] >= 127 ) name[i] = '?';
      memcpy( comment, dir + 16, 8 );
      for ( i = 0; comment[i]; i++)
	if ( comment[i] < ' ' || comment[i] >= 127 ) comment[i] = '?';
      PRINT (( "thom_floppy: file \"%s\" type=%s,%s comment=\"%s\"\n", name,
	       (dir[11] == 0) ? "bas" : (dir[11] == 1) ? "dat" : 
	       (dir[11] == 2) ? "bin" : (dir[11] == 3) ? "asm" : "???",
	       (dir[12] == 0) ? "a" : (dir[12] == 0xff) ? "b" : "?", comment ));
    }
  }
}
  
static thom_floppy_drive* thom_floppy_drive_of_image ( mess_image *image )
{
  return thom_floppy_drives + image_index_in_device( image );
}

static void thom_floppy_seek ( mess_image *image, int physical_track )
{
  thom_floppy_drive* d = thom_floppy_drive_of_image( image );
  thom_floppy_active( 0 );
  if ( physical_track < 0 ) physical_track = 0;
  if ( physical_track >= 80 ) physical_track = 79;
  LOG(( "%f thom_floppy_seek: dev=%li track=%i\n", 
	timer_get_time(), d - thom_floppy_drives, physical_track ));
  d->cur_track = physical_track;
}

static int thom_floppy_get_sectors_per_track( mess_image *image, int side )
{
  thom_floppy_drive* d = thom_floppy_drive_of_image( image );
  VLOG(( "thom_floppy_get_sectors_per_track: track=%i/%i dens=%s/%s\n", d->cur_track, d->tracks,  
	 (thom_density == DEN_FM_LO) ? "FM" : "MFM", 
	 (d->density == DEN_FM_LO) ? "FM" : "MFM" ));
  return d->sectors;
}

static void thom_floppy_get_id ( mess_image* image, chrn_id* id, 
				 int id_index, int physical_side )
{
  thom_floppy_drive* d = thom_floppy_drive_of_image( image );
  int trk =  d->cur_track;
  if ( trk >= d->tracks ) trk = d->tracks - 1;
  VLOG(( "thom_floppy_get_id: dev=%li track=%i idx=%i phy-side=%i dens=%s/%s\n", 
	 d - thom_floppy_drives, trk, id_index, physical_side,
	 (thom_density == DEN_FM_LO) ? "FM" : "MFM", 
	 (d->density == DEN_FM_LO) ? "FM" : "MFM" ));
  thom_floppy_active( 0 );
  if ( thom_floppy_sector_ptr( d, id_index+1, trk, 0 ) && 
#if 0
       /* fails with bobwinter */
      * thom_floppy_sector_flag( d, id_index+1, trk, 0 ) == 
       THOM_SECT_OK && 
#endif
       thom_density == d->density ) {
    int index = ( id_index * thom_floppy_interlace ) % d->sectors;
    id->C = d->cur_track;
    id->H = physical_side;
    id->R = 1 + index;
    id->N = (d->sector_size == 256) ? 1 : 0;
    id->data_id = 1 + index;
    id->flags = 0;
  }
  else {
    /* error */
    id->C = 0;
    id->H = 0;
    id->R = 0;
    id->N = 0;
    id->data_id = 0;
    id->flags = 0;
    logerror( "thom_floppy_get_id: invalid operation dev=%li track=%i idx=%i phy-side=%i dens=%s/%s\n", 
	      (long) (d - thom_floppy_drives), d->cur_track, id_index, physical_side, 
	      (thom_density == DEN_FM_LO) ? "FM" : "MFM", 
	      (d->density == DEN_FM_LO) ? "FM" : "MFM" );
  }
}

static void
thom_floppy_read_sector_data_into_buffer ( mess_image* image, int side, 
					   int index1, char *ptr, int length )
{
  thom_floppy_drive* d = thom_floppy_drive_of_image( image );
  UINT8* src = thom_floppy_sector_ptr( d, index1, d->cur_track, 0 );
  thom_floppy_active( 0 );
  LOG(( "thom_floppy_read_sector_data_into_buffer: dev=%li track=%i idx=%i side=%i len=%i\n",
	d - thom_floppy_drives, d->cur_track, index1, side, length ));
  if ( length > d->sector_size ) {
    logerror( "thom_floppy_read_sector_data_into_buffer: sector size %i truncated to %i\n", length, d->sector_size );
    length = d->sector_size;
  }
  if ( src )
    memcpy( ptr, src, length );
}

static void 
thom_floppy_write_sector_data_from_buffer ( mess_image *image, int side, 
					    int data_id,  const char *ptr, 
					    int length, int ddam )
{
  thom_floppy_drive* d = thom_floppy_drive_of_image( image );
  UINT8* dst = thom_floppy_sector_ptr( d, data_id, d->cur_track, 0 );
  thom_floppy_active( 1 );
  LOG(( "thom_floppy_write_sector_data_into_buffer: dev=%li track=%i idx=%i side=%i ddam=%i len=%i\n",
	d - thom_floppy_drives, d->cur_track, data_id, side, ddam, length ));
  if ( length > d->sector_size ) {
    logerror( "thom_floppy_write_sector_data_from_buffer: sector size %i truncated to %i\n", length, d->sector_size );
    length = d->sector_size;
  }
  if ( dst ) {
    memcpy( dst, ptr, length );
    d->has_changed = 1;
  }
}

static void 
thom_floppy_format_sector ( mess_image *image, int side, int sector_index,
			    int c, int h, int r, int n, int filler )
{
  thom_floppy_drive* d = thom_floppy_drive_of_image( image );
  int sector_size = 128 << n;
  UINT8* dst;
  thom_floppy_active( 1 );
  LOG(( "thom_floppy_format_sector: dev=%li track=%i/%i side=%i/%i idx=%i/%i c=%i h=%i r=%i n=%i filler=$%02X\n",
	d - thom_floppy_drives, 
	d->cur_track, d->tracks, side, d->sides, sector_index, d->sectors,
	c, h, r, n, filler ));

  if ( d -> type != THOM_FLOPPY_QDD ) {
    if ( sector_size != d->sector_size ||
	 ( d->cur_track >= d->tracks && d->tracks == 40 ) ) {
      /* need to change image params */
      if ( d->cur_track >= 40 ) d->tracks = 80;
      d->sector_size = sector_size; 
      d->density = thom_density;
      d->data = realloc( d->data, 
			 d->sides * d->tracks * d->sectors * d->sector_size );
      LOG (( "thom_floppy_format_sector: image enlarged to sides=%i tracks=%i sectors=%i sector_size=%i\n",d->sides, d->tracks, d->sectors, d->sector_size  ));
      assert( d->data );
    }
  }

  dst = thom_floppy_sector_ptr( d, r, d->cur_track, 0 );
  if ( dst ) {
    memset( dst, filler, d->sector_size );
    * thom_floppy_sector_flag( d, r, d->cur_track, 0 ) = THOM_SECT_OK;
    d->has_changed = 1;
  }
  else logerror( "thom_floppy_format_sector: invalid operation\n" );
}

static floppy_interface thom_floppy_interface = {
  thom_floppy_seek,
  thom_floppy_get_sectors_per_track,
  thom_floppy_get_id,
  thom_floppy_read_sector_data_into_buffer,
  thom_floppy_write_sector_data_from_buffer,
  NULL,
  NULL,
  thom_floppy_format_sector
};


/*********************** .fd format ************************/

static int thom_floppy_fd_load ( mess_image* image )
{
  thom_floppy_drive* d = thom_floppy_drives + image_index_in_device( image );
  int size = image_length( image );

  /* O-sized => create */
  if ( !size ) {
    PRINT(( "new image created in drive %i\n",
	    image_index_in_device( image ) ));
    d->sides = 1;
    d->sectors = 16;
    d->tracks = 40;
    d->sector_size = 128;
    d->density = DEN_FM_LO;
    d->type = THOM_FLOPPY_5_1_4;
    d->data = malloc( d->sector_size * d->sectors * d->tracks * d->sides );
    assert( d->data );
    memset( d->data, 0xff, d->sector_size * d->sectors * d->tracks * d->sides );
    return INIT_PASS;
  }
  
  /* load */
  d->data = malloc( size );
  assert( d->data );
  if ( image_fread( image, d->data, size ) != size ) {
    logerror( "thom_floppy_fd_load: read error\n" );
    goto error;
  }

  /* guess format: TODO two-sided images */
  if ( size == 81920 ) {
    d->type = THOM_FLOPPY_5_1_4;
    d->tracks = 40;
    d->sectors = 16;
    d->sides = 1;
    d->density = DEN_FM_LO;
    d->sector_size = 128;
  }
  else if ( size == 163840 ) {
#if 0
    /* this undocumented case can actually be built by the TO9 */
    d->type = THOM_FLOPPY_3_1_2;
    d->tracks = 80;
    d->sectors = 16;
    d->sides = 1;
    d->density = DEN_FM_LO;
    d->sector_size = 128;
#endif
    d->type = THOM_FLOPPY_5_1_4;
    d->tracks = 40;
    d->sectors = 16;
    d->sides = 1;
    d->density = DEN_MFM_LO;
    d->sector_size = 256;
  }
  else if ( size == 327680 ) {
    d->type = THOM_FLOPPY_3_1_2;
    d->tracks = 80;
    d->sectors = 16;
    d->sides = 1;
    d->density = DEN_MFM_LO;
    d->sector_size = 256;
  }
  else {
    logerror( "thom_floppy_fd_load: disk format not recognized\n" );
    goto error;
  }
  
  memset( d->sect_flag, THOM_SECT_OK, sizeof( d->sect_flag ) );

  thom_floppy_show( d );
  assert( size == d->sides * d->tracks * d->sectors * d->sector_size );

  return INIT_PASS;

 error:
  PRINT(( "could not load floppy image in drive %i\n",
	  image_index_in_device( image ) ));
  if ( d->data ) free( d->data ); 
  d->data = NULL;
  d->type = THOM_FLOPPY_NONE;
  return INIT_FAIL;
}

void thom_floppy_fd_save ( mess_image* image )
{
  thom_floppy_drive* d = thom_floppy_drives + image_index_in_device( image );
  unsigned size = d->sides * d->tracks * d->sectors * d->sector_size;
  if ( !d->data ) return;
  if ( !d->has_changed ) return;
  PRINT (( "thom_floppy_fd_save: saving floppy %li, %i KB\n",
	   (long) (d - thom_floppy_drives), size / 1024 ));
  /* TODO: truncation */
  image_fseek( image, 0, SEEK_SET );
  image_fwrite( image, d->data, size );
}


/*********************** .qd format ************************/

/* quick-disks (QDD) are Thomson-specific 2"8 disks, with 50 KB capacity:
   - one spiraling physical track, with 400 128-byte sectors
   - all 400 sectors are stepped on each pass
   - the interlacing of tracks / sectors is fixed, it is optimized so that
   many contiguous sectors can be read in one pass
 */

static int thom_floppy_qd_load ( mess_image* image )
{
  thom_floppy_drive* d = thom_floppy_drives + image_index_in_device( image );
  int size = image_length( image );
  
   /* only one format, TODO two-sided images */
  if ( size == 51200 || size == 0 ) {
    d->type = THOM_FLOPPY_QDD;
    d->tracks = 1;
    d->sectors = 400;
    d->sides = 1;
    d->density = DEN_MFM_LO;
    d->sector_size = 128;
  }
  else {
    logerror( "thom_floppy_qd_load: disk format not recognized\n" );
    return INIT_FAIL;
  }

  d->data = malloc( d->sector_size * d->sectors * d->tracks * d->sides );
  assert( d->data );
  memset( d->data, 0xff, d->sector_size * d->sectors * d->tracks * d->sides );
  memset( d->sect_flag, THOM_SECT_OK, sizeof( d->sect_flag ) );

  if  ( !size ) {
    PRINT(( "new image created in drive %i\n",
	    image_index_in_device( image ) ));
    return INIT_PASS;
  }

  if ( image_fread( image, d->data, size ) != size ) {
    logerror( "thom_floppy_qd_load: read error\n" );
    goto error;
  }
  thom_floppy_show( d );
  assert( size == d->sides * d->tracks * d->sectors * d->sector_size );

  return INIT_PASS;

 error:
  PRINT(( "could not load floppy image in drive %i\n",
	  image_index_in_device( image ) ));
  if ( d->data ) free( d->data ); 
  d->data = NULL;
  d->type = THOM_FLOPPY_NONE;
  return INIT_FAIL;
}

void thom_floppy_qd_save ( mess_image* image )
{
  thom_floppy_drive* d = thom_floppy_drives + image_index_in_device( image );
  unsigned size = d->sides * d->tracks * d->sectors * d->sector_size;
  if ( !d->data ) return;
  if ( !d->has_changed ) return;
  PRINT (( "thom_floppy_qd_save: saving floppy %li, %i KB\n",
	   (long) (d - thom_floppy_drives), size / 1024 ));
  /* TODO: truncation */
  image_fseek( image, 0, SEEK_SET );
  image_fwrite( image, d->data, size );
}


/*********************** .sap format ************************/

/* SAP format, Copyright 1998 by Alexandre Pukall */

static const int sap_magic_num = 0xB3; /* simple XOR crypt */

static const char sap_header[]=
  "\001SYSTEME D'ARCHIVAGE PUKALL S.A.P. "
  "(c) Alexandre PUKALL Avril 1998";

static const UINT16 sap_crc[] = {
  0x0000, 0x1081, 0x2102, 0x3183,   0x4204, 0x5285, 0x6306, 0x7387,
  0x8408, 0x9489, 0xa50a, 0xb58b,   0xc60c, 0xd68d, 0xe70e, 0xf78f,
};

static UINT16 thom_sap_crc( UINT8* data, int size )
{  
  int i;
  UINT16 crc = 0xffff, crc2;
  for ( i = 0; i < size; i++ ) {
    crc2 = ( crc >> 4 ) ^ sap_crc[ ( crc ^ data[i] ) & 15 ];
    crc = ( crc2 >> 4 ) ^ sap_crc[ ( crc2 ^ (data[i] >> 4) ) & 15 ];
  }
  return crc;
}

/* TODO: protection */
static int thom_floppy_sap_load ( mess_image* image )
{
  int drive = image_index_in_device( image );
  thom_floppy_drive* d = thom_floppy_drives + drive;
  UINT8 buf[262];
  int size = image_length( image );
  int i, format;

  /* O-sized => create */
  if ( !size ) {
    PRINT(( "new image created in drive %i\n",
	    image_index_in_device( image ) ));
    d->sides = 1;
    d->sectors = 16;
    d->tracks = 80;
    d->sector_size = 256;
    d->density = DEN_MFM_LO;
    d->type = THOM_FLOPPY_3_1_2;
    d->data = malloc( d->sector_size * d->sectors * d->tracks * d->sides );
    assert( d->data );
    memset( d->data, 0xff, d->sector_size * d->sectors * d->tracks * d->sides );
    return INIT_PASS;
  }

  /* check header */
  image_fread( image, buf, 66 );
  if ( memcmp( buf+1, sap_header+1, 65 ) ) {
    logerror( "thom_floppy_sap_load: invalid sap header\n" );
    return INIT_FAIL;
  }

  /* guess format */
  image_fread( image, buf, 4 );
  format = buf[0];
  d->sides = 1; /* always one side per sap file */
  d->density = (format==1) ? DEN_FM_LO : DEN_MFM_LO;
  d->sector_size = (format==1) ? 128 : 256;
  d->tracks = 0;
  d->sectors = 16;
  for ( i = 66; i+4 < size; i += d->sector_size + 6 ) {
    int fmt, prot, track, sector;
    image_fseek( image, i, SEEK_SET );
    image_fread( image, buf, 4 );
    fmt    = buf[0];
    prot   = buf[1];
    track  = buf[2];
    sector = buf[3];
    LOG(( "thom_floppy_sap_load: drive %i found sector %i on track %i, fmt=%i, prot=%i\n", drive, sector, track, fmt, prot ));
    if ( sector < 1 || sector > d->sectors ) {
      logerror( "thom_floppy_sap_load: invalid sector %i (must be in 1..16)\n", 
		sector );
      goto error;
    }
    if ( track >= d->tracks ) d->tracks = track+1;
  }
  
  switch ( d->tracks ) {
  case 40: d->type = THOM_FLOPPY_5_1_4; break;
  case 80: d->type = THOM_FLOPPY_3_1_2; break;
  default: 
    logerror( "thom_floppy_sap_load: invalid track number %i\n", d->tracks ); 
    goto error;
  }

  /* load */
  d->data = malloc( d->tracks * d->sectors * d->sector_size );
  assert( d->data );
  for ( i = 0; i < d->tracks * d->sectors; i++ ) {
    UINT16 crc;
    int j, fmt, prot, track, sector;

    /* load */
    image_fseek( image, i * (d->sector_size + 6) + 66, SEEK_SET  );
    image_fread( image, buf, d->sector_size + 6 );
    fmt    = buf[0];
    prot   = buf[1];
    track  = buf[2];
    sector = buf[3];

    /* decrypt */
    for ( j = 0; j < d->sector_size; j++ )
      buf[ j + 4 ] ^= sap_magic_num;

    memcpy( thom_floppy_sector_ptr( d, sector, track, 0 ),
	    buf + 4, d->sector_size );

    * thom_floppy_sector_flag( d, sector, track, 0 ) =
      (fmt == 4) ? THOM_SECT_INVALID : THOM_SECT_OK;

    /* check CRC */
    crc = thom_sap_crc( buf, d->sector_size + 4 );
    if ( ( (crc >> 8)   != buf[ d->sector_size + 4 ] ) ||
	 ( (crc & 0xff) != buf[ d->sector_size + 5 ] ) ) {
      logerror( "thom_floppy_sap_load: CRC error for track %i, sector %i\n", 
		track, sector );
    }

    
  }
  
  thom_floppy_show( d );
 
  return INIT_PASS;

 error:
  PRINT(( "could not load floppy image in drive %i\n",
	  image_index_in_device( image ) ));
  if ( d->data ) free( d->data ); 
  d->data = NULL;
  d->type = THOM_FLOPPY_NONE;
  return INIT_FAIL;
}

static void thom_floppy_sap_save ( mess_image* image )
{
  thom_floppy_drive* d = thom_floppy_drives + image_index_in_device( image );
  unsigned size = d->sides * d->tracks * d->sectors * d->sector_size;
  int format, sector, track, i;
  UINT8 buf[262];
  if ( !d->data ) return;
  if ( !d->has_changed ) return;
  PRINT (( "thom_floppy_sap_save: saving floppy %li, %i KB\n",
	   (long) (d - thom_floppy_drives), size / 1024 ));

  /* TODO: truncation */

  /* write header*/
  image_fseek( image, 0, SEEK_SET );
  image_fwrite( image, sap_header, 66 );

  /* get format */
  format = (d->sector_size == 128) ? 1 : 0;

  /* write sectors */
  for ( track = 0; track < d->tracks; track++ )
    for ( sector = 1; sector <= d->sectors; sector++ ) {
      UINT8 *p = thom_floppy_sector_ptr( d, sector, track, 0 );
      UINT16 crc;

      /* sector header */
      buf[ 0 ] = format;
      buf[ 1 ] = 0;
      buf[ 2 ] = track;
      buf[ 3 ] = sector;

      memcpy( buf + 4, p, d->sector_size );

      /* CRC */
      crc = thom_sap_crc( buf, d->sector_size + 4 );
      buf[ d->sector_size + 4 ] = crc >> 8;
      buf[ d->sector_size + 5 ] = crc & 0xff;

      /* crypt data */
      for ( i = 0; i < d->sector_size; i++ ) 
	buf[ i + 4 ] ^= sap_magic_num;

      image_fwrite( image, buf, d->sector_size + 6 );
    }
}


/*********************** device ** ************************/

static void thom_floppy_reset ( thom_floppy_drive* d )
{
  if ( d->data ) free( d->data );
  d->data = NULL;
  d->cur_track = 0;
  d->tracks = 0;
  d->type = THOM_FLOPPY_NONE;
  memset( d->sect_flag, THOM_SECT_INVALID, sizeof( d->sect_flag ) );
}

int thom_floppy_init ( mess_image *image )
{
  thom_qdd_compute_map();
  thom_floppy_reset( thom_floppy_drive_of_image( image  ) );
  return floppy_drive_init( image, &thom_floppy_interface );
}

int thom_floppy_load ( mess_image* image )
{
  const char* typ = image_filetype( image );
  thom_floppy_drive* d = thom_floppy_drive_of_image( image );
  thom_floppy_reset( d );

  d->image = image;
  d->has_changed = 0;
  d->cur_track = 0;

  /* dispatch according to extension */
  if ( typ ) {
    if ( !mame_stricmp( typ, "sap" ) ) 
      return thom_floppy_sap_load( image );
    else if ( !mame_stricmp( typ, "fd" ) ) 
      return thom_floppy_fd_load( image );
    else if ( !mame_stricmp( typ, "qd" ) ) 
      return thom_floppy_qd_load( image );
  }

  return INIT_FAIL;
}

void thom_floppy_unload ( mess_image *image )
{
  thom_floppy_drive* d = thom_floppy_drive_of_image( image );
  const char* typ = image_filetype( image );
  if ( typ ) {
    if ( !mame_stricmp( typ, "sap" ) )     thom_floppy_sap_save( image );
    else if ( !mame_stricmp( typ, "fd" ) ) thom_floppy_fd_save( image );
    else if ( !mame_stricmp( typ, "qd" ) ) thom_floppy_qd_save( image );
  }
  thom_floppy_reset( d );
}

int  thom_floppy_create ( mess_image *image, 
			  int create_format, option_resolution *args )
{
  thom_floppy_drive* d = thom_floppy_drive_of_image( image );
  const char* typ = image_filetype( image );
  thom_floppy_reset( d );

  d->image = image;
  d->cur_track = 0;
  if ( typ && !mame_stricmp( typ, "qd" ) )  {
    d->sides = 1;
    d->sectors = 400;
    d->tracks = 1;
    d->sector_size = 128;
    d->density = DEN_MFM_LO;
    d->type = THOM_FLOPPY_QDD;
  }
  else if ( typ && !mame_stricmp( typ, "fd" ) )  {
    d->sides = 1;
    d->sectors = 16;
    d->tracks = 40;
    d->sector_size = 128;
    d->density = DEN_FM_LO;
    d->type = THOM_FLOPPY_5_1_4;
  }
  else {
    d->sides = 1;
    d->sectors = 16;
    d->tracks = 40;
    d->sector_size = 256;
    d->density = DEN_MFM_LO;
    d->type = THOM_FLOPPY_5_1_4;
  }
  d->data = malloc( d->sector_size * d->sectors * d->tracks * d->sides );
  assert( d->data );
  memset( d->data, 0xff, d->sector_size * d->sectors * d->tracks * d->sides );

  PRINT(( "new image created in drive %i\n", image_index_in_device( image ) ));

  /* not much to do:
     - the image is created in memory upon formatting
     - it is stored to the file-system upon unload
   */

  return INIT_PASS;
}

void thom_floppy_set_density ( DENSITY density )
{
  thom_density = density;
}

DENSITY thom_floppy_get_density ( void )
{
  return thom_density;
}

thom_floppy_type thom_floppy_get_type ( int drive )
{
  if ( drive < 0 || drive >= 4 ) return THOM_FLOPPY_NONE;
  return thom_floppy_drives[ drive ].type;
}

void thom_floppy_getinfo( const device_class *devclass, 
			  UINT32 state, union devinfo *info )
{
  switch ( state ) {
  case DEVINFO_INT_TYPE:
    info->i = IO_FLOPPY; 
    break;
  case DEVINFO_INT_READABLE:
    info->i = 1; 
    break;
  case DEVINFO_INT_WRITEABLE:
    info->i = 1;
    break;
  case DEVINFO_INT_CREATABLE:
    info->i = 1;
    break;
  case DEVINFO_PTR_INIT:
    info->init = thom_floppy_init;
    break;
  case DEVINFO_PTR_LOAD:
    info->load = thom_floppy_load;
    break;
  case DEVINFO_PTR_UNLOAD:
    info->unload = thom_floppy_unload;
    break;
  case DEVINFO_PTR_CREATE:
    info->create = thom_floppy_create;
    break;
  case DEVINFO_INT_COUNT:
    info->i = 4;
    break;
  case DEVINFO_STR_NAME+0:
  case DEVINFO_STR_NAME+1:
  case DEVINFO_STR_NAME+2:
  case DEVINFO_STR_NAME+3:
    strcpy( info->s = device_temp_str(), "floppydisk0" );
    info->s[ strlen( info->s ) - 1 ] += state - DEVINFO_STR_NAME;
    break;
  case DEVINFO_STR_SHORT_NAME+0:
  case DEVINFO_STR_SHORT_NAME+1:
  case DEVINFO_STR_SHORT_NAME+2:
  case DEVINFO_STR_SHORT_NAME+3:
    strcpy( info->s = device_temp_str(), "flop0");
    info->s[ strlen( info->s ) - 1 ] += state - DEVINFO_STR_SHORT_NAME;
    break;
  case DEVINFO_STR_DESCRIPTION+0:
  case DEVINFO_STR_DESCRIPTION+1:
  case DEVINFO_STR_DESCRIPTION+2:
  case DEVINFO_STR_DESCRIPTION+3:
    strcpy( info->s = device_temp_str(), "Floppy #0" );
    info->s[ strlen( info->s ) - 1 ] += state - DEVINFO_STR_DESCRIPTION;
    break;
  case DEVINFO_STR_DEV_FILE:
    strcpy( info->s = device_temp_str(), __FILE__ );
    break;
  case DEVINFO_STR_FILE_EXTENSIONS:
    strcpy( info->s = device_temp_str(), "fd,qd,sap" ); 
    break;
  }
}
