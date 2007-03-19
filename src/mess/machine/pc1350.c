#include "driver.h"
#include "cpu/sc61860/sc61860.h"

#include "includes/pocketc.h"
#include "includes/pc1350.h"

static UINT8 outa,outb;

static int power=1; /* simulates pressed cce when mess is started */

void pc1350_outa(int data)
{
	outa=data;
}

void pc1350_outb(int data)
{
	outb=data;
}

void pc1350_outc(int data)
{

}

int pc1350_ina(void)
{
	int data=outa;
	int t=pc1350_keyboard_line_r();
	if (t&1) {
		if (PC1350_KEY_BRACE_CLOSE) data|=1;
		if (PC1350_KEY_COLON) data|=2;
		if (PC1350_KEY_SEMICOLON) data|=4;
		if (PC1350_KEY_COMMA) data|=8;
		if (PC1350_KEY_SML) data|=0x10;
		if (PC1350_KEY_DEF) data|=0x20;
		// not sure if both shifts are wired to the same contacts
		if (PC1350_KEY_LSHIFT||PC1350_KEY_RSHIFT) data|=0x40;
	}
	if (t&2) {
		if (PC1350_KEY_BRACE_OPEN) data|=1;
		if (PC1350_KEY_SLASH) data|=2;
		if (PC1350_KEY_ASTERIX) data|=4;
		if (PC1350_KEY_MINUS) data|=8;
		if (PC1350_KEY_Z) data|=0x10;
		if (PC1350_KEY_A) data|=0x20;
		if (PC1350_KEY_Q) data|=0x40;
	}
	if (t&4) {
		if (PC1350_KEY_9) data|=1;
		if (PC1350_KEY_6) data|=2;
		if (PC1350_KEY_3) data|=4;
		if (PC1350_KEY_PLUS) data|=8;
		if (PC1350_KEY_X) data|=0x10;
		if (PC1350_KEY_S) data|=0x20;
		if (PC1350_KEY_W) data|=0x40;
	}
	if (t&8) {
		if (PC1350_KEY_8) data|=1;
		if (PC1350_KEY_5) data|=2;
		if (PC1350_KEY_2) data|=4;
		if (PC1350_KEY_POINT) data|=8;
		if (PC1350_KEY_C) data|=0x10;
		if (PC1350_KEY_D) data|=0x20;
		if (PC1350_KEY_E) data|=0x40;
	}
	if (t&0x10) {
		if (PC1350_KEY_7) data|=1;
		if (PC1350_KEY_4) data|=2;
		if (PC1350_KEY_1) data|=4;
		if (PC1350_KEY_0) data|=8;
		if (PC1350_KEY_V) data|=0x10;
		if (PC1350_KEY_F) data|=0x20;
		if (PC1350_KEY_R) data|=0x40;
	}
	if (t&0x20) {
		if (PC1350_KEY_UP) data|=1;
		if (PC1350_KEY_DOWN) data|=2;
		if (PC1350_KEY_LEFT) data|=4; 
		if (PC1350_KEY_RIGHT) data|=8;
		if (PC1350_KEY_B) data|=0x10;
		if (PC1350_KEY_G) data|=0x20;
		if (PC1350_KEY_T) data|=0x40;
	}
	if (outa&1) {
//		if (PC1350_KEY_1) data|=2; //?
		if (PC1350_KEY_INS) data|=4;
		if (PC1350_KEY_DEL) data|=8;
		if (PC1350_KEY_N) data|=0x10;
		if (PC1350_KEY_H) data|=0x20;
		if (PC1350_KEY_Y) data|=0x40;
	}
	if (outa&2) {
//		if (PC1350_KEY_2) data|=4; //?
		if (PC1350_KEY_MODE) data|=8;
		if (PC1350_KEY_M) data|=0x10;
		if (PC1350_KEY_J) data|=0x20;
		if (PC1350_KEY_U) data|=0x40;
	}
	if (outa&4) {
		if (power||PC1350_KEY_CLS) data|=8;
		if (PC1350_KEY_SPACE) data|=0x10;
		if (PC1350_KEY_K) data|=0x20;
		if (PC1350_KEY_I) data|=0x40;
	}
	if (outa&8) {
		if (PC1350_KEY_ENTER) data|=0x10;
		if (PC1350_KEY_L) data|=0x20;
		if (PC1350_KEY_O) data|=0x40;
	}
	if (outa&0x10) {
		if (PC1350_KEY_EQUALS) data|=0x20;
		if (PC1350_KEY_P) data|=0x40;
	}
	if (PC1350_KEY_OFF&&(outa&0xc0) ) data|=0xc0;

	// missing lshift
	
	return data;
}

int pc1350_inb(void)
{
	int data=outb;
	return data;
}

bool pc1350_brk(void)
{
	return PC1350_KEY_BRK;
}

/* currently enough to save the external ram */
NVRAM_HANDLER( pc1350 )
{
	UINT8 *ram=memory_region(REGION_CPU1)+0x2000,
		*cpu=sc61860_internal_ram();

	if (read_or_write)
	{
		mame_fwrite(file, cpu, 96);
		mame_fwrite(file, ram, 0x5000);
	}
	else if (file)
	{
		mame_fread(file, cpu, 96);
		mame_fread(file, ram, 0x5000);
	}
	else
	{
		memset(cpu, 0, 96);
		memset(ram, 0, 0x5000);
	}
}

static void pc1350_power_up(int param)
{
	power=0;
}

MACHINE_START( pc1350 )
{
	timer_pulse(1/500.0, 0,sc61860_2ms_tick);
	timer_set(1,0,pc1350_power_up);

	memory_install_read8_handler(0,  ADDRESS_SPACE_PROGRAM, 0x6000, 0x6fff, 0, 0, MRA8_BANK1);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x6fff, 0, 0, MWA8_BANK1);
	memory_set_bankptr(1, &mess_ram[0x0000]);

	if (mess_ram_size >= 0x3000)
	{
		memory_install_read8_handler(0,  ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, MRA8_BANK2);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, MWA8_BANK2);
		memory_set_bankptr(2, &mess_ram[0x1000]);
	}
	else
	{
		memory_install_read8_handler(0,  ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, MRA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, MWA8_NOP);
	}

	if (mess_ram_size >= 0x5000)
	{
		memory_install_read8_handler(0,  ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, MRA8_BANK3);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, MWA8_BANK3);
		memory_set_bankptr(3, &mess_ram[0x3000]);
	}
	else
	{
		memory_install_read8_handler(0,  ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, MRA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, MWA8_NOP);
	}
	return 0;
}
