
#include "driver.h"
#include "includes/gamecom.h"
#include "cpu/sm8500/sm8500.h"
#include "image.h"

typedef struct {
	int enabled;
	int transfer_mode;
	int decrement_y;
	int decrement_x;
	int overwrite_mode;
	int width_x;
	int width_y;
	int width_x_count;
	int width_y_count;
	int source_x;
	int source_x_current;
	int source_y;
	int source_width;
	int dest_x;
	int dest_x_current;
	int dest_y;
	int dest_width;
	int state_count;
	int state_pixel;
	int state_limit;
	UINT8 palette[4];
	UINT8 *source_bank;
	unsigned int source_current;
	unsigned int source_line;
	unsigned int source_mask;
	UINT8 *dest_bank;
	unsigned int dest_current;
	unsigned int dest_line;
	unsigned int dest_mask;
} GAMECOM_DMA;

typedef struct {
	int enabled;
	int state_count;
	int state_limit;
	int counter;
	int check_value;
} GAMECOM_TIMER;

UINT8 *internal_ram;
UINT8 internal_registers[0x80];
UINT8 gamecom_vram[16*1024];
UINT8 *cartridge1 = NULL;
UINT8 *cartridge2 = NULL;
UINT8 *cartridge = NULL;
UINT8 *dummy_bank = NULL;

static mame_timer *gamecom_clock_timer = NULL;
GAMECOM_DMA gamecom_dma;
GAMECOM_TIMER gamecom_timer[2];

//int gamecom_timer_limit[8] = { 2/2, 1024/2, 2048/2, 4096/2, 8192/2, 16384/2, 32768/2, 65536/2 };
int gamecom_timer_limit[8] = { 2, 1024, 2048, 4096, 8192, 16384, 32768, 65536 };

void gamecom_dma_init(void);

static void gamecom_clock_timer_callback(int dummy)
{
	UINT8 val = ( ( internal_registers[SM8521_CLKT] & 0x3F ) + 1 ) & 0x3F;
	internal_registers[SM8521_CLKT] = ( internal_registers[SM8521_CLKT] & 0xC0 ) | val;
	cpunum_set_input_line( 0, CK_INT, HOLD_LINE );
}

MACHINE_RESET( gamecom )
{
	memory_set_bankptr( 1, memory_region(REGION_USER1) );
        memory_set_bankptr( 2, memory_region(REGION_USER1) );
        memory_set_bankptr( 3, memory_region(REGION_USER1) );
        memory_set_bankptr( 4, memory_region(REGION_USER1) );

	/* should possibly go in a DRIVER_INIT piece? */
	if ( gamecom_clock_timer == NULL ) {
		gamecom_clock_timer = timer_alloc( gamecom_clock_timer_callback );
	}
	/* intialize the empty dummy bank */
	if ( dummy_bank == NULL ) {
		dummy_bank = auto_malloc( 8 * 1024 );
	}
	memset( dummy_bank, 0xff, sizeof( dummy_bank ) );
	cartridge = NULL;
	/* disable DMA and timer */
	gamecom_dma.enabled = 0;

	memset( internal_registers, 0x00, sizeof(internal_registers) );
	gamecom_internal_w( SM8521_URTT, 0xFF );
	gamecom_internal_w( SM8521_URTS, 0x42 );
	gamecom_internal_w( SM8521_WDTC, 0x38 );
}

static void gamecom_set_mmu( int mmu, UINT8 data ) {
	if ( data < 32 ) {
		/* select internal ROM bank */
		memory_set_bankptr( mmu, memory_region(REGION_USER1) + (data << 13) );
	} else {
		/* select cartridge bank */
		if ( cartridge == NULL ) {
			memory_set_bankptr( mmu, dummy_bank );
		} else {
			memory_set_bankptr( mmu, cartridge + ( data << 13 ) );
		}
	}
}

static void handle_stylus_press( UINT8 column ) {
	static const UINT16 row_data[10] = { 0x3FE, 0x3FD, 0x3FB, 0x3F7, 0x3EF, 0x3DF, 0x3BF, 0x37F, 0x2FF, 0x1FF };
	static UINT32 stylus_x;
	static UINT32 stylus_y;

	if ( column == 0 ) {
		if ( ! ( readinputport(2) & 0x04 ) ) {
			stylus_x = readinputport( 3 ) >> 4;
			stylus_y = readinputport( 4 ) >> 4;
		} else {
			stylus_x = 16;
			stylus_y = 16;
		}
	}

	if ( stylus_x == column ) {
		cpunum_set_reg( 0, SM8500_P0, row_data[stylus_y] & 0xFF );
		cpunum_set_reg( 0, SM8500_P1, ( cpunum_get_reg( 0, SM8500_P1 ) & 0xFC ) | ( ( row_data[stylus_y] >> 8 ) & 0x03 ) );
	} else {
		cpunum_set_reg( 0, SM8500_P0, 0xFF );
		cpunum_set_reg( 0, SM8500_P1, ( cpunum_get_reg( 0, SM8500_P1 ) & 0xFC ) | 0x03 );
	}
}

WRITE8_HANDLER( gamecom_internal_w )
{
	if ( offset >= 0x80 ) {
		internal_ram[offset] = data;
		return;
	}

	switch( offset ) {
	case SM8521_IE0:	cpunum_set_reg( 0, SM8500_IE0, data ); return;
	case SM8521_IE1:	cpunum_set_reg( 0, SM8500_IE1, data ); return;
	case SM8521_IR0:	cpunum_set_reg( 0, SM8500_IR0, data ); return;
	case SM8521_IR1:	cpunum_set_reg( 0, SM8500_IR1, data ); return;
	case SM8521_P0:		cpunum_set_reg( 0, SM8500_P0, data ); return;
	case SM8521_P1:		cpunum_set_reg( 0, SM8500_P1, data ); return;
	case SM8521_P2:		cpunum_set_reg( 0, SM8500_P2, data );
				switch( ( cpunum_get_reg( 0, SM8500_P1 ) << 8 ) | data ) {
				case 0xFBFF:	/* column #0 */
						/* P0 bit 0 cleared => 01 */
						/* P0 bit 1 cleared => 0E */
						/* P0 bit 2 cleared => 1B */
						/* P0 bit 3 cleared => etc */
						/* P0 bit 4 cleared => */
						/* P0 bit 5 cleared => */
						/* P0 bit 6 cleared => */
						/* P0 bit 7 cleared => */
						/* P1 bit 0 cleared => */
						/* P1 bit 1 cleared => */
					handle_stylus_press( 0 );
					break;
				case 0xF7FF:	/* column #1 */
					handle_stylus_press( 1 );
					break;
				case 0xEFFF:	/* column #2 */
					handle_stylus_press( 2 );
					break;
				case 0xDFFF:	/* column #3 */
					handle_stylus_press( 3 );
					break;
				case 0xBFFF:	/* column #4 */
					handle_stylus_press( 4 );
					break;
				case 0x7FFF:	/* column #5 */
					handle_stylus_press( 5 );
					break;
				case 0xFFFE:	/* column #6 */
					handle_stylus_press( 6 );
					break;
				case 0xFFFD:	/* column #7 */
					handle_stylus_press( 7 );
					break;
				case 0xFFFB:	/* column #8 */
					handle_stylus_press( 8 );
					break;
				case 0xFFF7:	/* column #9 */
					handle_stylus_press( 9 );
					break;
				case 0xFFEF:	/* column #10 */
					handle_stylus_press( 10 );
					break;
				case 0xFFDF:	/* column #11 */
					handle_stylus_press( 11 );
					break;
				case 0xFFBF:	/* column #12 */
					handle_stylus_press( 12 );
					break;
				case 0xFF7F:	/* keys #1 */
						/* P0 bit 0 cleared => 83 (up) */
						/* P0 bit 1 cleared => 84 (down) */
						/* P0 bit 2 cleared => 85 (left) */
						/* P0 bit 3 cleared => 86 (right) */
						/* P0 bit 4 cleared => 87 (menu) */
						/* P0 bit 5 cleared => 8A (pause) */
						/* P0 bit 6 cleared => 89 (sound) */
						/* P0 bit 7 cleared => 8B (button A) */
						/* P1 bit 0 cleared => 8C (button B) */
						/* P1 bit 1 cleared => 8D (button C) */
					cpunum_set_reg( 0, SM8500_P0, readinputport(0) );
					cpunum_set_reg( 0, SM8500_P1, ( cpunum_get_reg( 0, SM8500_P1 ) & 0xFC ) | ( readinputport(1) & 0x03 ) );
					break;
				case 0xFFFF:	/* keys #2 */
						/* P0 bit 0 cleared => 88 (power) */
						/* P0 bit 1 cleared => 8E (button D) */
						/* P0 bit 2 cleared => A0 */
						/* P0 bit 3 cleared => A0 */
						/* P0 bit 4 cleared => A0 */
						/* P0 bit 5 cleared => A0 */
						/* P0 bit 6 cleared => A0 */
						/* P0 bit 7 cleared => A0 */
						/* P1 bit 0 cleared => A0 */
						/* P1 bit 1 cleared => A0 */
					cpunum_set_reg( 0, SM8500_P0, ( cpunum_get_reg( 0, SM8500_P0 ) & 0xFC ) | ( readinputport(2) & 0x03 ) );
					cpunum_set_reg( 0, SM8500_P1, 0xFF );
					break;
				}
				return;
	case SM8521_P3:
				/* P3 bit7 clear, bit6 set -> enable cartridge port #0? */
				/* P3 bit6 clear, bit7 set -> enable cartridge port #1? */
				cpunum_set_reg( 0, SM8500_P3, data );
				switch( data & 0xc0 ) {
				case 0x40: cartridge = cartridge1; break;
				case 0x80: cartridge = cartridge2; break;
				default:   cartridge = NULL;       break;
				}
				/* update banks to reflect possible change of cartridge slot */
				gamecom_set_mmu( 1, internal_registers[SM8521_MMU1] );
				gamecom_set_mmu( 2, internal_registers[SM8521_MMU2] );
				gamecom_set_mmu( 3, internal_registers[SM8521_MMU3] );
				gamecom_set_mmu( 4, internal_registers[SM8521_MMU4] );
				return;
	case SM8521_SYS:	cpunum_set_reg( 0, SM8500_SYS, data ); return;
	case SM8521_CKC:	cpunum_set_reg( 0, SM8500_CKC, data ); return;
	case SM8521_SPH:	cpunum_set_reg( 0, SM8500_SPH, data ); return;
	case SM8521_SPL:	cpunum_set_reg( 0, SM8500_SPL, data ); return;
	case SM8521_PS0:	cpunum_set_reg( 0, SM8500_PS0, data ); return;
	case SM8521_PS1:	cpunum_set_reg( 0, SM8500_PS1, data ); return;
	case SM8521_P0C:	cpunum_set_reg( 0, SM8500_P0C, data ); return;
	case SM8521_P1C:	cpunum_set_reg( 0, SM8500_P1C, data ); return;
	case SM8521_P2C:	cpunum_set_reg( 0, SM8500_P2C, data ); return;
	case SM8521_P3C:	cpunum_set_reg( 0, SM8500_P3C, data ); return;
	case SM8521_MMU0:	/* disable bootstrap ROM? most likely not written to on game.com */
		logerror( "Write to MMU0\n" );
		break;
	case SM8521_MMU1:
		gamecom_set_mmu( 1, data );
		break;
	case SM8521_MMU2:
		gamecom_set_mmu( 2, data );
		break;
	case SM8521_MMU3:
		gamecom_set_mmu( 3, data );
		break;
	case SM8521_MMU4:
		gamecom_set_mmu( 4, data );
		break;

	/* Video hardware and DMA */
	case SM8521_LCDC:
//		logerror( "%X: Setting LCDC to %X\n", activecpu_get_pc(), data );
		break;
	case SM8521_LCH:
		break;
	case SM8521_LCV:
		break;
	case SM8521_DMC:
		gamecom_dma.overwrite_mode = data & 0x01;
		gamecom_dma.transfer_mode = data & 0x06;
		gamecom_dma.decrement_x = data & 0x08;
		gamecom_dma.decrement_y = data & 0x10;
		gamecom_dma.enabled = data & 0x80;
		if ( gamecom_dma.enabled ) {
			gamecom_dma_init();
		}
		break;
	case SM8521_DMX1:
		break;
	case SM8521_DMY1:
		break;
	case SM8521_DMDX:
		break;
	case SM8521_DMDY:
		break;
	case SM8521_DMX2:
		break;
	case SM8521_DMY2:
		break;
	case SM8521_DMPL:
		break;
	case SM8521_DMBR:
		break;
	case SM8521_DMVP:
		break;
	case SM8521_TM0D:
		gamecom_timer[0].check_value = data;
		break;
	case SM8521_TM0C:
		gamecom_timer[0].enabled = data & 0x80;
		gamecom_timer[0].state_limit = gamecom_timer_limit[data & 0x07];
		gamecom_timer[0].state_count = 0;
		gamecom_timer[0].counter = 0;
		break;
	case SM8521_TM1D:
		gamecom_timer[1].check_value = data;
		break;
	case SM8521_TM1C:
		gamecom_timer[1].enabled = data & 0x80;
		gamecom_timer[1].state_limit = gamecom_timer_limit[data & 0x07];
		gamecom_timer[1].state_count = 0;
		gamecom_timer[1].counter = 0;
		break;
	case SM8521_CLKT:	/* bit 6-7 */
		if ( data & 0x80 ) {
			/* timer run */
			if ( data & 0x40 ) {
				/* timer resolution 1 minute */
				timer_adjust( gamecom_clock_timer, 1.0, 0, TIME_IN_SEC(60) );
			} else {
				/* TImer resolution 1 second */
				timer_adjust( gamecom_clock_timer, 1.0, 0, TIME_IN_SEC(1) );
			}
		} else {
			/* disable timer reset */
			timer_enable( gamecom_clock_timer, 0 );
			data &= 0xC0;
		}
		break;

	/* Reserved addresses */
	case SM8521_18: case SM8521_1B:
	case SM8521_29: case SM8521_2A: case SM8521_2F:
	case SM8521_33: case SM8521_3E: case SM8521_3F:
	case SM8521_41: case SM8521_43: case SM8521_45: case SM8521_4B:
	case SM8521_4F:
	case SM8521_55: case SM8521_56: case SM8521_57: case SM8521_58:
	case SM8521_59: case SM8521_5A: case SM8521_5B: case SM8521_5C:
	case SM8521_5D:
		logerror( "%X: Write to reserved address (0x%02X). Value written: 0x%02X\n", activecpu_get_pc(), offset, data );
		break;
	}
	internal_registers[offset] = data;
}

READ8_HANDLER( gamecom_internal_r )
{
	if ( offset >= 0x80 ) {
		return internal_ram[offset];
	}

	switch( offset ) {
	case SM8521_R0:  case SM8521_R1:  case SM8521_R2:  case SM8521_R3:
	case SM8521_R4:  case SM8521_R5:  case SM8521_R6:  case SM8521_R7:
	case SM8521_R8:  case SM8521_R9:  case SM8521_R10: case SM8521_R11:
	case SM8521_R12: case SM8521_R13: case SM8521_R14: case SM8521_R15:
		return sm85cpu_mem_readbyte( offset );
	case SM8521_IE0:	return cpunum_get_reg( 0, SM8500_IE0 );
	case SM8521_IE1:	return cpunum_get_reg( 0, SM8500_IE1 );
	case SM8521_IR0:	return cpunum_get_reg( 0, SM8500_IR0 );
	case SM8521_IR1:	return cpunum_get_reg( 0, SM8500_IR1 );
	case SM8521_P0:		/* logerror( "%X: Read from P0\n", activecpu_get_pc() ); */ return cpunum_get_reg( 0, SM8500_P0 );
	case SM8521_P1:		/* logerror( "%X: Read from P1\n", activecpu_get_pc() ); */ return cpunum_get_reg( 0, SM8500_P1 );
	case SM8521_P2:		/* logerror( "%X: Read from P2\n", activecpu_get_pc() ); */ return cpunum_get_reg( 0, SM8500_P2 );
	case SM8521_P3:		return cpunum_get_reg( 0, SM8500_P3 );
	case SM8521_SYS:	return cpunum_get_reg( 0, SM8500_SYS );
	case SM8521_CKC:	return cpunum_get_reg( 0, SM8500_CKC );
	case SM8521_SPH:	return cpunum_get_reg( 0, SM8500_SPH );
	case SM8521_SPL:	return cpunum_get_reg( 0, SM8500_SPL );
	case SM8521_PS0:	return cpunum_get_reg( 0, SM8500_PS0 );
	case SM8521_PS1:	return cpunum_get_reg( 0, SM8500_PS1 );
	case SM8521_P0C:	return cpunum_get_reg( 0, SM8500_P0C );
	case SM8521_P1C:	return cpunum_get_reg( 0, SM8500_P1C );
	case SM8521_P2C:	return cpunum_get_reg( 0, SM8500_P2C );
	case SM8521_P3C:	return cpunum_get_reg( 0, SM8500_P3C );
	case SM8521_TM0D:	return gamecom_timer[0].counter;
	case SM8521_TM1D:	return gamecom_timer[1].counter;
//	case SM8521_CLKT:	/* bit 0-5 read only, 6-7 read/write */
//			return ( internal_registers[offset] & 0xC0 ) | ( clock_timer_val & 0x3F );

	/* Reserved addresses */
	case SM8521_18: case SM8521_1B:
	case SM8521_29: case SM8521_2A: case SM8521_2F:
	case SM8521_33: case SM8521_3E: case SM8521_3F:
	case SM8521_41: case SM8521_43: case SM8521_45: case SM8521_4B:
	case SM8521_4F:
	case SM8521_55: case SM8521_56: case SM8521_57: case SM8521_58:
	case SM8521_59: case SM8521_5A: case SM8521_5B: case SM8521_5C:
	case SM8521_5D:
//		logerror( "%X: Read from reserved address (0x%02X)\n", activecpu_get_pc(), offset );
		break;

	}
	return internal_registers[offset];
}

/* The manual is not conclusive as to which bit of the DMVP register (offset 0x3D) determines
   which page for source or destination is used */
void gamecom_dma_init(void) {
	if ( gamecom_dma.decrement_x || gamecom_dma.decrement_y ) {
		logerror( "TODO: Decrement-x and decrement-y are not supported yet\n" );
	}
	gamecom_dma.width_x = internal_registers[SM8521_DMDX];
	gamecom_dma.width_x_count = 0;
	gamecom_dma.width_y = internal_registers[SM8521_DMDY];
	gamecom_dma.width_y_count = 0;
	gamecom_dma.source_x = internal_registers[SM8521_DMX1];
	gamecom_dma.source_x_current = gamecom_dma.source_x;
	gamecom_dma.source_y = internal_registers[SM8521_DMY1];
	gamecom_dma.source_width = ( internal_registers[SM8521_LCH] & 0x20 ) ? 50 : 40;
	gamecom_dma.dest_x = internal_registers[SM8521_DMX2];
	gamecom_dma.dest_x_current = gamecom_dma.dest_x;
	gamecom_dma.dest_y = internal_registers[SM8521_DMY2];
	gamecom_dma.dest_width = ( internal_registers[SM8521_LCH] & 0x20 ) ? 50 : 40;
	gamecom_dma.palette[0] = ( internal_registers[SM8521_DMPL] & 0x03 );
	gamecom_dma.palette[1] = ( internal_registers[SM8521_DMPL] & 0x0C ) >> 2;
	gamecom_dma.palette[2] = ( internal_registers[SM8521_DMPL] & 0x30 ) >> 4;
	gamecom_dma.palette[3] = ( internal_registers[SM8521_DMPL] & 0xC0 ) >> 6;
	gamecom_dma.source_mask = 0x1FFF;
	gamecom_dma.dest_mask = 0x1FFF;
//	logerror("DMA: width %Xx%X, source (%X,%X), dest (%X,%X), transfer_mode %X, banks %X \n", gamecom_dma.width_x, gamecom_dma.width_y, gamecom_dma.source_x, gamecom_dma.source_y, gamecom_dma.dest_x, gamecom_dma.dest_y, gamecom_dma.transfer_mode, internal_registers[SM8521_DMVP] );
//	logerror( "   Palette: %d, %d, %d, %d\n", gamecom_dma.palette[0], gamecom_dma.palette[1], gamecom_dma.palette[2], gamecom_dma.palette[3] );
	switch( gamecom_dma.transfer_mode ) {
	case 0x00:
		/* VRAM->VRAM */
		gamecom_dma.source_bank = &gamecom_vram[(internal_registers[SM8521_DMVP] & 0x01) ? 0x2000 : 0x0000];
		gamecom_dma.dest_bank = &gamecom_vram[(internal_registers[SM8521_DMVP] & 0x02) ? 0x2000 : 0x0000];
		break;
	case 0x02:
		/* ROM->VRAM */
//		logerror( "DMA DMBR = %X\n", internal_registers[SM8521_DMBR] );
		gamecom_dma.source_width = 64;
		if ( internal_registers[SM8521_DMBR] < 16 ) {
			gamecom_dma.source_bank = memory_region(REGION_USER1) + (internal_registers[SM8521_DMBR] << 14);
			gamecom_dma.source_mask = 0x3FFF;
		} else {
			logerror( "TODO: Reading from external ROMs not supported yet\n" );
			gamecom_dma.source_bank = memory_region(REGION_USER1);
		}
		gamecom_dma.dest_bank = &gamecom_vram[(internal_registers[SM8521_DMVP] & 0x02) ? 0x2000 : 0x0000];
		break;
	case 0x04:
		/* Extend RAM->VRAM */
		gamecom_dma.source_width = 64;
		gamecom_dma.source_bank = memory_get_read_ptr(0, ADDRESS_SPACE_PROGRAM,  0xE000 );
		gamecom_dma.dest_bank = &gamecom_vram[(internal_registers[SM8521_DMVP] & 0x02) ? 0x2000 : 0x0000];
		break;
	case 0x06:
		/* VRAM->Extend RAM */
		gamecom_dma.source_bank = &gamecom_vram[(internal_registers[SM8521_DMVP] & 0x01) ? 0x2000 : 0x0000];
		gamecom_dma.dest_width = 64;
		gamecom_dma.dest_bank = memory_get_read_ptr(0, ADDRESS_SPACE_PROGRAM, 0xE000 );
		break;
	}
	gamecom_dma.source_current = gamecom_dma.source_width * gamecom_dma.source_y;
	gamecom_dma.source_current += gamecom_dma.source_x >> 2;
	gamecom_dma.dest_current = gamecom_dma.dest_width * gamecom_dma.dest_y;
	gamecom_dma.dest_current += gamecom_dma.dest_x >> 2;
	gamecom_dma.source_line = gamecom_dma.source_current;
	gamecom_dma.dest_line = gamecom_dma.dest_current;
	gamecom_dma.state_count = 0;
}

/* For now the increment/decrement-x and increment/decrement-y parts are NOT supported.
   Their usage is also not explained properly in the manuals. Guess we'll have to wait
   for them to show up in some rom images...
 */
void gamecom_handle_dma( int cycles ) {
	unsigned y_count, x_count;
	/* If not enabled, ignore */
	if ( ! gamecom_dma.enabled ) {
		return;
	}
	for( y_count = 0; y_count <= gamecom_dma.width_y; y_count++ ) {
		for( x_count = 0; x_count <= gamecom_dma.width_x; x_count++ ) {
			int source_pixel = 0;
			int dest_pixel = 0;
			int src_addr = gamecom_dma.source_current & gamecom_dma.source_mask;
			int dest_addr = gamecom_dma.dest_current & gamecom_dma.dest_mask;
			/* handle DMA for 1 pixel */
			/* Read pixel data */
			switch ( gamecom_dma.source_x_current & 0x03 ) {
			case 0x00: source_pixel = ( gamecom_dma.source_bank[src_addr] & 0xC0 ) >> 6; break;
			case 0x01: source_pixel = ( gamecom_dma.source_bank[src_addr] & 0x30 ) >> 4; break;
			case 0x02: source_pixel = ( gamecom_dma.source_bank[src_addr] & 0x0C ) >> 2; break;
			case 0x03: source_pixel = ( gamecom_dma.source_bank[src_addr] & 0x03 );      break;
			}

			if ( !gamecom_dma.overwrite_mode && source_pixel == 0 ) {
				switch ( gamecom_dma.dest_x_current & 0x03 ) {
				case 0x00: dest_pixel = ( gamecom_dma.dest_bank[dest_addr] & 0xC0 ) >> 6; break;
				case 0x01: dest_pixel = ( gamecom_dma.dest_bank[dest_addr] & 0x30 ) >> 4; break;
				case 0x02: dest_pixel = ( gamecom_dma.dest_bank[dest_addr] & 0x0C ) >> 2; break;
				case 0x03: dest_pixel = ( gamecom_dma.dest_bank[dest_addr] & 0x03 );      break;
				}
				source_pixel = dest_pixel;
			}

			/* Translate pixel data using DMA palette. */
			/* Not sure if this should be done before the compound stuff - WP */
			source_pixel = gamecom_dma.palette[ source_pixel ];
			/* Write pixel data */
			switch( gamecom_dma.dest_x_current & 0x03 ) {
			case 0x00:
				gamecom_dma.dest_bank[dest_addr] = ( gamecom_dma.dest_bank[dest_addr] & 0x3F ) | ( source_pixel << 6 );
				break;
			case 0x01:
				gamecom_dma.dest_bank[dest_addr] = ( gamecom_dma.dest_bank[dest_addr] & 0xCF ) | ( source_pixel << 4 );
				break;
			case 0x02:
				gamecom_dma.dest_bank[dest_addr] = ( gamecom_dma.dest_bank[dest_addr] & 0xF3 ) | ( source_pixel << 2 );
				break;
			case 0x03:
				gamecom_dma.dest_bank[dest_addr] = ( gamecom_dma.dest_bank[dest_addr] & 0xFC ) | source_pixel;
				break;
			}

			/* Advance a pixel */
			if ( gamecom_dma.decrement_x ) {
				gamecom_dma.source_x_current--;
				if ( ( gamecom_dma.source_x_current & 0x03 ) == 0x03 ) {
					gamecom_dma.source_current--;
				}
			} else {
				gamecom_dma.source_x_current++;
				if ( ( gamecom_dma.source_x_current & 0x03 ) == 0x00 ) {
					gamecom_dma.source_current++;
				}
			}
			gamecom_dma.dest_x_current++;
			if ( ( gamecom_dma.dest_x_current & 0x03 ) == 0x00 ) {
				gamecom_dma.dest_current++;
			}
		}

		/* Advance a line */
		gamecom_dma.source_x_current = gamecom_dma.source_x;
		gamecom_dma.dest_x_current = gamecom_dma.dest_x;
		gamecom_dma.source_line += gamecom_dma.source_width;
		gamecom_dma.source_current = gamecom_dma.source_line;
		gamecom_dma.dest_line += gamecom_dma.dest_width;
		gamecom_dma.dest_current = gamecom_dma.dest_line;
	}
	gamecom_dma.enabled = 0;
	cpunum_set_input_line( 0, DMA_INT, HOLD_LINE );
}

void gamecom_update_timers( int cycles ) {
	if ( gamecom_timer[0].enabled ) {
		gamecom_timer[0].state_count += cycles;
		while ( gamecom_timer[0].state_count >= gamecom_timer[0].state_limit ) {
			gamecom_timer[0].state_count -= gamecom_timer[0].state_limit;
			gamecom_timer[0].counter++;
			if ( gamecom_timer[0].counter == gamecom_timer[0].check_value ) {
				gamecom_timer[0].counter = 0;
				cpunum_set_input_line( 0, TIM0_INT, HOLD_LINE );
			}
		}
	}
	if ( gamecom_timer[1].enabled ) {
		gamecom_timer[1].state_count += cycles;
		while ( gamecom_timer[1].state_count >= gamecom_timer[1].state_limit ) {
			gamecom_timer[1].state_count -= gamecom_timer[1].state_limit;
			gamecom_timer[1].counter++;
			if ( gamecom_timer[1].counter == gamecom_timer[1].check_value ) {
				gamecom_timer[1].counter = 0;
				cpunum_set_input_line( 0, TIM1_INT, HOLD_LINE );
			}
		}
	}
}

WRITE8_HANDLER( gamecom_vram_w ) {
	gamecom_vram[offset] = data;
}

READ8_HANDLER( gamecom_vram_r ) {
	return gamecom_vram[offset];
}

DEVICE_INIT( gamecom_cart )
{
	internal_ram = sm8500_internal_ram();
	return INIT_PASS;
}

DEVICE_LOAD( gamecom_cart )
{
	int filesize;
	int load_offset = 0;

	/* allocate memory on first load of a cartridge */
	if ( cartridge1 == NULL ) {
		cartridge1 = auto_malloc( 2048 * 1024 );
	}
	filesize = image_length( image );
	switch( filesize ) {
	case 0x008000: load_offset = 0;        break;  /* 32 KB */
	case 0x040000: load_offset = 0;        break;  /* 256KB */
	case 0x080000: load_offset = 0;        break;  /* 512KB */
	case 0x100000: load_offset = 0;        break;  /* 1  MB */
	case 0x1c0000: load_offset = 0x040000; break;  /* 1.8MB */
	case 0x200000: load_offset = 0;        break;  /* 2  MB */
	default:                                       /* otherwise */
		logerror( "Error loading cartridge: Invalid file size.\n" );
		return INIT_FAIL;
	}
	if ( image_fread( image, cartridge1 + load_offset, filesize ) != filesize ) {
		logerror( "Error loading cartridge: Unable to read from file: %s.\n", image_filename(image) );
		return INIT_FAIL;
	}
	if ( filesize < 0x010000 ) { memcpy( cartridge1 + 0x008000, cartridge1, 0x008000 ); } /* ->64KB */
	if ( filesize < 0x020000 ) { memcpy( cartridge1 + 0x010000, cartridge1, 0x010000 ); } /* ->128KB */
	if ( filesize < 0x040000 ) { memcpy( cartridge1 + 0x020000, cartridge1, 0x020000 ); } /* ->256KB */
	if ( filesize < 0x080000 ) { memcpy( cartridge1 + 0x040000, cartridge1, 0x040000 ); } /* ->512KB */
	if ( filesize < 0x100000 ) { memcpy( cartridge1 + 0x080000, cartridge1, 0x080000 ); } /* ->1MB */
	if ( filesize < 0x1c0000 ) { memcpy( cartridge1 + 0x100000, cartridge1, 0x100000 ); } /* -> >=1.8MB */
	return INIT_PASS;
}

