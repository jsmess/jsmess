/*
	strata.h: header file for strata.c
*/

int strataflash_init(int id);
int strataflash_load(int id, mame_file *f);
int strataflash_save(int id, mame_file *file);
UINT8 strataflash_8_r(int id, UINT32 address);
void strataflash_8_w(int id, UINT32 address, UINT8 data);
UINT16 strataflash_16_r(int id, offs_t offset);
void strataflash_16_w(int id, offs_t offset, UINT16 data);
