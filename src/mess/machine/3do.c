
#include "driver.h"
#include "includes/3do.h"

READ32_HANDLER( nvarea_r ) {
	logerror( "%08X: NVRAM read offset = %08X, mask = %08X\n", activecpu_get_pc(), offset, mem_mask );
	return 0;
}

WRITE32_HANDLER( nvarea_w ) {
	logerror( "%08X: NVRAM write offset = %08X, data = %08X, mask = %08X\n", activecpu_get_pc(), offset, data, mem_mask );
}

READ32_HANDLER( vram_sport_r ) {
	logerror( "%08X: VRAM SPORT read offset = %08X, mask = %08X\n", activecpu_get_pc(), offset, mem_mask );
	return 0;
}

WRITE32_HANDLER( vram_sport_w ) {
	logerror( "%08X: VRAM SPORT write offset = %08X, data = %08X, mask = %08X\n", activecpu_get_pc(), offset, data, mem_mask );
}

READ32_HANDLER( madam_r ) {
	logerror( "%08X: MADAM read offset = %08X, mask = %08X\n", activecpu_get_pc(), offset, mem_mask );
	return 0;
}

WRITE32_HANDLER( madam_w ) {
	logerror( "%08X: MADAM write offset = %08X, data = %08X, mask = %08X\n", activecpu_get_pc(), offset, data, mem_mask );
}

READ32_HANDLER( clio_r ) {
	logerror( "%08X: CLIO read offset = %08X, mask = %08X\n", activecpu_get_pc(), offset, mem_mask );
	return 0;
}

WRITE32_HANDLER( clio_w ) {
	logerror( "%08X: CLIO write offset = %08X, data = %08X, mask = %08X\n", activecpu_get_pc(), offset, data, mem_mask );
}

