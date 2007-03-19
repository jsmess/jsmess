#include "driver.h"
#include "cpu/sc61860/sc61860.h"

#include "includes/pocketc.h"
#include "includes/pc1401.h"

/* C-CE while reset, program will not be destroyed! */

/* error codes
1 syntax error
2 calculation error
3 illegal function argument
4 too large a line number
5 next without for
  return without gosub
6 memory overflow
7 print using error
8 i/o device error
9 other errors*/

static UINT8 outa,outb;
UINT8 pc1401_portc;

static int power=1; /* simulates pressed cce when mess is started */

void pc1401_outa(int data)
{
	outa=data;
}

void pc1401_outb(int data)
{
	outb=data;
}

void pc1401_outc(int data)
{
	logerror("%g outc %.2x\n",timer_get_time(), data);
	pc1401_portc=data;
}

int pc1401_ina(void)
{
	int data=outa;
	if (outb&1) {
		if (KEY_SIGN) data|=1;
		if (KEY_8) data|=2;
		if (KEY_2) data|=4;
		if (KEY_5) data|=8;
		if (KEY_CAL) data|=0x10;
		if (KEY_Q) data|=0x20;
		if (KEY_A) data|=0x40;
		if (KEY_Z) data|=0x80;
	}
	if (outb&2) {
		if (KEY_POINT) data|=1;
		if (KEY_9) data|=2;
		if (KEY_3) data|=4;
		if (KEY_6) data|=8;
		if (KEY_BASIC) data|=0x10;
		if (KEY_W) data|=0x20;
		if (KEY_S) data|=0x40;
		if (KEY_X) data|=0x80;
	}
	if (outb&4) {
		if (KEY_PLUS) data|=1;
		if (KEY_DIV) data|=2;
		if (KEY_MINUS) data|=4;
		if (KEY_MUL) data|=8;
		if (KEY_DEF) data|=0x10;
		if (KEY_E) data|=0x20;
		if (KEY_D) data|=0x40;
		if (KEY_C) data|=0x80;
	}
	if (outb&8) {
		if (KEY_BRACE_RIGHT) data|=1;
		if (KEY_BRACE_LEFT) data|=2;
		if (KEY_SQUARE) data|=4;
		if (KEY_ROOT) data|=8;
		if (KEY_POT) data|=0x10;
		if (KEY_EXP) data|=0x20;
		if (KEY_XM) data|=0x40;
		if (KEY_EQUALS) data|=0x80;
	}
	if (outb&0x10) {
		if (KEY_STAT) data|=1;
		if (KEY_1X) data|=2;
		if (KEY_LOG) data|=4;
		if (KEY_LN) data|=8;
		if (KEY_DEG) data|=0x10;
		if (KEY_HEX) data|=0x20;
		if (KEY_MPLUS) data|=0x80;
	}
	if (outb&0x20) {
		if (power||(KEY_CCE)) data|=1;
		if (KEY_FE) data|=2;
		if (KEY_TAN) data|=4;
		if (KEY_COS) data|=8;
		if (KEY_SIN) data|=0x10;
		if (KEY_HYP) data|=0x20;
		if (KEY_SHIFT) data|=0x40;
		if (KEY_RM) data|=0x80;
	}
	if (outa&1) {
		if (KEY_7) data|=2;
		if (KEY_1) data|=4;
		if (KEY_4) data|=8;
		if (KEY_DOWN) data|=0x10;
		if (KEY_R) data|=0x20;
		if (KEY_F) data|=0x40;
		if (KEY_V) data|=0x80;
	}
	if (outa&2) {
		if (KEY_COMMA) data|=4;
		if (KEY_P) data|=8;
		if (KEY_UP) data|=0x10;
		if (KEY_T) data|=0x20;
		if (KEY_G) data|=0x40;
		if (KEY_B) data|=0x80;
	}
	if (outa&4) {
		if (KEY_O) data|=8;
		if (KEY_LEFT) data|=0x10;
		if (KEY_Y) data|=0x20;
		if (KEY_H) data|=0x40;
		if (KEY_N) data|=0x80;
	}
	if (outa&8) {
		if (KEY_RIGHT) data|=0x10;
		if (KEY_U) data|=0x20;
		if (KEY_J) data|=0x40;
		if (KEY_M) data|=0x80;
	}
	if (outa&0x10) {
		if (KEY_I) data|=0x20;
		if (KEY_K) data|=0x40;
		if (KEY_SPC) data|=0x80;
	}
	if (outa&0x20) {
		if (KEY_L) data|=0x40;
		if (KEY_ENTER) data|=0x80;
	}
	if (outa&0x40) {
		if (KEY_0) data|=0x80;
	}
	return data;
}

int pc1401_inb(void)
{
	int data=outb;
	if (KEY_OFF) data|=1;
	return data;
}

bool pc1401_brk(void)
{
	return KEY_BRK;
}

bool pc1401_reset(void)
{
	return KEY_RESET;
}

/* currently enough to save the external ram */
NVRAM_HANDLER( pc1401 )
{
	UINT8 *ram=memory_region(REGION_CPU1)+0x2000,
		*cpu=sc61860_internal_ram();

	if (read_or_write)
	{
		mame_fwrite(file, cpu, 96);
		mame_fwrite(file, ram, 0x2800);
	}
	else if (file)
	{
		mame_fread(file, cpu, 96);
		mame_fread(file, ram, 0x2800);
	}
	else
	{
		memset(cpu, 0, 96);
		memset(ram, 0, 0x2800);
	}
}

static void pc1401_power_up(int param)
{
	power=0;
}

DRIVER_INIT( pc1401 )
{
	int i;
	UINT8 *gfx=memory_region(REGION_GFX1);
#if 0
	char sucker[]={
		/* this routine dump the memory (start 0)
		   in an endless loop,
		   the pc side must be started before this
		   its here to allow verification of the decimal data
		   in mame disassembler
		*/
#if 1
		18,4,/*lip xl */
		2,0,/*lia 0 startaddress low */
		219,/*exam */
		18,5,/*lip xh */
		2,0,/*lia 0 startaddress high */
		219,/*exam */
/*400f x: */
		/* dump internal rom */
		18,5,/*lip 4 */
		89,/*ldm */
		218,/*exab */
		18,4,/*lip 5 */
		89,/*ldm */
		4,/*ix for increasing x */
		0,0,/*lii,0 */
		18,20,/*lip 20 */
		53, /* */
		18,20,/* lip 20 */
		219,/*exam */
#else
		18,4,/*lip xl */
		2,255,/*lia 0 */
		219,/*exam */
		18,5,/*lip xh */
		2,255,/*lia 0 */
		219,/*exam */
/*400f x: */
		/* dump external memory */
		4, /*ix */
		87,/*				 ldd */
#endif
		218,/*exab */



		0,4,/*lii 4 */

		/*a: */
		218,/*				  exab */
		90,/*				  sl */
		218,/*				  exab */
		18,94,/*			lip 94 */
		96,252,/*				  anma 252 */
		2,2, /*lia 2 */
		196,/*				  adcm */
		95,/*				  outf */
		/*b:  */
		204,/*inb */
		102,128,/*tsia 0x80 */
#if 0
		41,4,/*			   jnzm b */
#else
		/* input not working reliable! */
		/* so handshake removed, PC side must run with disabled */
		/* interrupt to not lose data */
		78,20, /*wait 20 */
#endif

		218,/*				  exab */
		90,/*				  sl */
		218,/*				  exab */
		18,94,/*			lip 94 */
		96,252,/*anma 252 */
		2,0,/*				  lia 0 */
		196,/*adcm */
		95,/*				  outf */
		/*c:  */
		204,/*inb */
		102,128,/*tsia 0x80 */
#if 0
		57,4,/*			   jzm c */
#else
		78,20, /*wait 20 */
#endif

		65,/*deci */
		41,34,/*jnzm a */

		41,41,/*jnzm x: */

		55,/*				rtn */
	};

	for (i=0; i<sizeof(sucker);i++) pc1401_mem[0x4000+i]=sucker[i];
	logerror("%d %d\n",i, 0x4000+i);
#endif
	for (i=0; i<128; i++) gfx[i]=i;

	timer_pulse(1/500.0, 0,sc61860_2ms_tick);
	timer_set(1,0,pc1401_power_up);

	if (RAM10K)
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, MWA8_RAM);
	}
	else if (RAM4K)
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x37ff, 0, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM,  0x3800, 0x3fff, 0, 0, MWA8_RAM);
	}
	else
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, MWA8_NOP);
	}
}


