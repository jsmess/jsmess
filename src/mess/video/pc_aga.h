/*
  pc cga/mda combi adapters

  one type hardware switchable between cga and mda/hercules
  another type software switchable between cga and mda/hercules

  some support additional modes like
  commodore pc10 320x200 in 16 colors


	// aga
	// 256 8x8 thick chars
	// 256 8x8 thin chars
	// 256 9x14 in 8x16 chars, line 3 is connected to a10
    ROM_LOAD("aga.chr",     0x00000, 0x02000, CRC(aca81498))
	// hercules font of above
    ROM_LOAD("hercules.chr", 0x00000, 0x1000, CRC(7e8c9d76))

*/


extern const gfx_layout europc_cga_charlayout;
extern const gfx_layout europc_mda_charlayout;
GFXDECODE_EXTERN( europc );
GFXDECODE_EXTERN( aga );

extern PALETTE_INIT( pc_aga );


typedef enum AGA_MODE { AGA_OFF, AGA_COLOR, AGA_MONO } AGA_MODE;
void pc_aga_set_mode(AGA_MODE mode);

extern VIDEO_START( pc_aga );
extern VIDEO_START( pc200 );

extern void pc_aga_timer(void);

READ8_HANDLER( pc_aga_videoram_r );
WRITE8_HANDLER( pc_aga_videoram_w );

READ8_HANDLER( pc200_videoram_r );
WRITE8_HANDLER( pc200_videoram_w );
READ16_HANDLER( pc200_videoram16le_r );
WRITE16_HANDLER( pc200_videoram16le_w );

READ8_HANDLER( pc200_cga_r );
WRITE8_HANDLER( pc200_cga_w );
READ16_HANDLER( pc200_cga16le_r );
WRITE16_HANDLER( pc200_cga16le_w );

