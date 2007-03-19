#include "driver.h"
#include "cpu/sc61860/sc61860.h"

#include "includes/pocketc.h"
#include "includes/pc1251.h"

/* C-CE while reset, program will not be destroyed! */

static UINT8 outa,outb;

static int power=1; /* simulates pressed cce when mess is started */

void pc1251_outa(int data)
{
	outa=data;
}

void pc1251_outb(int data)
{
	outb=data;
}

void pc1251_outc(int data)
{

}

int pc1251_ina(void)
{
	int data=outa;

	if (outb&1) {
		if (PC1251_KEY_MINUS) data|=1;
		if (power||PC1251_KEY_CL) data|=2; // problem with the deg lcd
		if (PC1251_KEY_ASTERIX) data|=4;
		if (PC1251_KEY_SLASH) data|=8;
		if (PC1251_KEY_DOWN) data|=0x10;
		if (PC1251_KEY_E) data|=0x20;
		if (PC1251_KEY_D) data|=0x40;
		if (PC1251_KEY_C) data|=0x80;
	}
	if (outb&2) {
		if (PC1251_KEY_PLUS) data|=1;
		if (PC1251_KEY_9) data|=2;
		if (PC1251_KEY_3) data|=4;
		if (PC1251_KEY_6) data|=8;
		if (PC1251_KEY_SHIFT) data|=0x10;
		if (PC1251_KEY_W) data|=0x20;
		if (PC1251_KEY_S) data|=0x40;
		if (PC1251_KEY_X) data|=0x80;
	}
	if (outb&4) {
		if (PC1251_KEY_POINT) data|=1;
		if (PC1251_KEY_8) data|=2;
		if (PC1251_KEY_2) data|=4;
		if (PC1251_KEY_5) data|=8;
		if (PC1251_KEY_DEF) data|=0x10;
		if (PC1251_KEY_Q) data|=0x20;
		if (PC1251_KEY_A) data|=0x40;
		if (PC1251_KEY_Z) data|=0x80;
	}
	if (outa&1) {
		if (PC1251_KEY_7) data|=2;
		if (PC1251_KEY_1) data|=4;
		if (PC1251_KEY_4) data|=8;
		if (PC1251_KEY_UP) data|=0x10;
		if (PC1251_KEY_R) data|=0x20;
		if (PC1251_KEY_F) data|=0x40;
		if (PC1251_KEY_V) data|=0x80;
	}
	if (outa&2) {
		if (PC1251_KEY_EQUALS) data|=4;
		if (PC1251_KEY_P) data|=8;
		if (PC1251_KEY_LEFT) data|=0x10;
		if (PC1251_KEY_T) data|=0x20;
		if (PC1251_KEY_G) data|=0x40;
		if (PC1251_KEY_B) data|=0x80;
	}
	if (outa&4) {
		if (PC1251_KEY_O) data|=8;
		if (PC1251_KEY_RIGHT) data|=0x10;
		if (PC1251_KEY_Y) data|=0x20;
		if (PC1251_KEY_H) data|=0x40;
		if (PC1251_KEY_N) data|=0x80;
	}
	if (outa&8) {
//		if (PC1251_KEY_DOWN) data|=0x10; //?
		if (PC1251_KEY_U) data|=0x20;
		if (PC1251_KEY_J) data|=0x40;
		if (PC1251_KEY_M) data|=0x80;
	}
	if (outa&0x10) {
		if (PC1251_KEY_I) data|=0x20;
		if (PC1251_KEY_K) data|=0x40;
		if (PC1251_KEY_SPACE) data|=0x80;
	}
	if (outa&0x20) {
		if (PC1251_KEY_L) data|=0x40;
		if (PC1251_KEY_ENTER) data|=0x80;
	}
	if (outa&0x40) {
		if (PC1251_KEY_0) data|=0x80;
	}

	return data;
}

int pc1251_inb(void)
{
	int data=outb;
	if (outb&8) data|=PC1251_SWITCH_MODE;
	return data;
}

bool pc1251_brk(void)
{
	return PC1251_KEY_BRK;
}

bool pc1251_reset(void)
{
	return PC1251_KEY_RESET;
}

/* currently enough to save the external ram */
NVRAM_HANDLER( pc1251 )
{
	UINT8 *ram = memory_region(REGION_CPU1)+0x8000,
		*cpu = sc61860_internal_ram();

	if (read_or_write)
	{
		mame_fwrite(file, cpu, 96);
		mame_fwrite(file, ram, 0x4800);
	}
	else if (file)
	{
		mame_fread(file, cpu, 96);
		mame_fread(file, ram, 0x4800);
	}
	else
	{
		memset(cpu, 0, 96);
		memset(ram, 0, 0x4800);
	}
}

static void pc1251_power_up(int param)
{
	power = 0;
}

DRIVER_INIT( pc1251 )
{
	int i;
	UINT8 *gfx = memory_region(REGION_GFX1);
	for (i=0; i<128; i++) gfx[i]=i;

	timer_pulse(1/500.0, 0, sc61860_2ms_tick);
	timer_set(1, 0, pc1251_power_up);

	// c600 b800 b000 a000 8000 tested	
	// 4 kb memory feedback 512 bytes too few???
	// 11 kb ram: program stored at 8000
#if 1
	memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xc7ff, 0, 0, MWA8_BANK1);
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x8000);
#else
	if (PC1251_RAM11K)
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xafff, 0, 0, MWA8_RAM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xc5ff, 0, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc600, 0xc7ff, 0, 0, MWA8_RAM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc800, 0xf7ff, 0, 0, MWA8_RAM);
	}
	else if (PC1251_RAM6K)
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xafff, 0, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xc7ff, 0, 0, MWA8_RAM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc800, 0xcbff, 0, 0, MWA8_RAM);
	}
	else if (PC1251_RAM4K)
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xb7ff, 0, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb800, 0xc7ff, 0, 0, MWA8_RAM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc800, 0xcbff, 0, 0, MWA8_RAM);
	}
	else
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xc5ff, 0, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc600, 0xc9ff, 0, 0, MWA8_RAM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xca00, 0xcbff, 0, 0, MWA8_RAM);
	}
#endif
}

