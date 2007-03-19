/* System 16 and friends FD1094 handling */

/*
todo:

support multiple FD1094s (does anything /use/ multiple FD1094s?)
make more configurable (select caches per game?)

*/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "machine/fd1094.h"


#define S16_NUMCACHE 8

static unsigned char *fd1094_key; // the memory region containing key
static UINT16 *fd1094_cpuregion; // the CPU region with encrypted code
static UINT32  fd1094_cpuregionsize; // the size of this region in bytes

static UINT16* fd1094_userregion; // a user region where the current decrypted state is put and executed from
static UINT16* fd1094_cacheregion[S16_NUMCACHE]; // a cache region where S16_NUMCACHE states are stored to improve performance
static int fd1094_cached_states[S16_NUMCACHE]; // array of cached state numbers
static int fd1094_current_cacheposition; // current position in cache array

void *fd1094_get_decrypted_base(void)
{
	if (!fd1094_key)
		return NULL;
	return fd1094_userregion;
}

/* this function checks the cache to see if the current state is cached,
   if it is then it copies the cached data to the user region where code is
   executed from, if its not cached then it gets decrypted to the current
   cache position using the functions in fd1094.c */
void fd1094_setstate_and_decrypt(int state)
{
	int i;
	UINT32 addr;

	cpunum_set_info_int(0, CPUINFO_INT_REGISTER + M68K_PREF_ADDR, 0x0010);	// force a flush of the prefetch cache

	/* set the FD1094 state ready to decrypt.. */
	state = fd1094_set_state(fd1094_key,state);

	/* first check the cache, if its cached we don't need to decrypt it, just copy */
	for (i=0;i<S16_NUMCACHE;i++)
	{
		if (fd1094_cached_states[i] == state)
		{
			/* copy cached state */
			fd1094_userregion=fd1094_cacheregion[i];
			memory_set_decrypted_region(0, 0, fd1094_cpuregionsize - 1, fd1094_userregion);
			m68k_set_encrypted_opcode_range(0,0,fd1094_cpuregionsize);

			return;
		}
	}

// mame_printf_debug("new state %04x\n",state);

	/* mark it as cached (because it will be once we decrypt it) */
	fd1094_cached_states[fd1094_current_cacheposition]=state;

	for (addr=0;addr<fd1094_cpuregionsize/2;addr++)
	{
		UINT16 dat;
		dat = fd1094_decode(addr,fd1094_cpuregion[addr],fd1094_key,0);
		fd1094_cacheregion[fd1094_current_cacheposition][addr]=dat;
	}

#if 0
{
	char filename[100];
	FILE *f;
	int keydex;
	sprintf(filename, "%s-%02X.log", Machine->gamedrv->name, state);
	f = fopen(filename, "w");
	for (keydex = 0; keydex < 0x2000; keydex++)
	{
		int category[3] = { 0 };
		int bit;

		for (addr = keydex; addr < fd1094_cpuregionsize/2; addr += 0x2000)
		{
			UINT16 op = fd1094_cacheregion[fd1094_current_cacheposition][addr];
			if ((op & 0xff80) == 0x4e80) category[0]++;
			else if ((op & 0xf0f8) == 0x50c8) category[1]++;
			else if ((op & 0xf000) == 0x6000) category[2]++;
		}

		bit = (fd1094_key[keydex] >> (keydex < 0x1000 ? 6 : 7)) & 1;
		fprintf(f, "%04X: (%02X) %d%c %c%c%c ->",
					keydex,
					fd1094_key[keydex],
					bit,
					(bit == 0 && category[0] == 0 && category[1] == 0 && category[2] == 0) ? '?' : ' ',
					category[0] ? '0' : '.',
					category[1] ? '1' : '.',
					category[2] ? '2' : '.'
					);
		for (addr = keydex; addr < fd1094_cpuregionsize/2; addr += 0x2000)
		{
			UINT16 op = fd1094_cacheregion[fd1094_current_cacheposition][addr];
			fprintf(f, " %04X%c", op, ((op & 0xff80) == 0x4e80 || (op & 0xf0f8) == 0x50c8 || (op & 0xf000) == 0x6000) ? '*' : ' ');
		}
		fprintf(f, "\n");
	}
	fclose(f);
}
#endif

	/* copy newly decrypted data to user region */
	fd1094_userregion=fd1094_cacheregion[fd1094_current_cacheposition];
	memory_set_decrypted_region(0, 0, fd1094_cpuregionsize - 1, fd1094_userregion);
	m68k_set_encrypted_opcode_range(0,0,fd1094_cpuregionsize);

	fd1094_current_cacheposition++;

	if (fd1094_current_cacheposition>=S16_NUMCACHE)
	{
		mame_printf_debug("out of cache, performance may suffer, incrase S16_NUMCACHE!\n");
		fd1094_current_cacheposition=0;
	}
}

/* Callback for CMP.L instructions (state change) */
void fd1094_cmp_callback(unsigned int val, int reg)
{
	if (reg == 0 && (val & 0x0000ffff) == 0x0000ffff) // ?
	{
		fd1094_setstate_and_decrypt((val & 0xffff0000) >> 16);
	}
}

/* Callback when the FD1094 enters interrupt code */
int fd1094_int_callback (int irq)
{
	fd1094_setstate_and_decrypt(FD1094_STATE_IRQ);
	return (0x60+irq*4)/4; // vector address
}

void fd1094_rte_callback (void)
{
	fd1094_setstate_and_decrypt(FD1094_STATE_RTE);
}


/* KLUDGE, set the initial PC / SP based on table as we can't decrypt them yet */
void fd1094_kludge_reset_values(void)
{
	int i;

	for (i = 0;i < 4;i++)
		fd1094_userregion[i] = fd1094_decode(i,fd1094_cpuregion[i],fd1094_key,1);
}


/* function, to be called from MACHINE_RESET (every reset) */
void fd1094_machine_init(void)
{
	/* punt if no key; this allows us to be called even for non-FD1094 games */
	if (!fd1094_key)
		return;

	fd1094_setstate_and_decrypt(FD1094_STATE_RESET);
	fd1094_kludge_reset_values();

	cpunum_set_info_fct(0, CPUINFO_PTR_M68K_CMPILD_CALLBACK, (genf *)fd1094_cmp_callback);
	cpunum_set_info_fct(0, CPUINFO_PTR_M68K_RTE_CALLBACK, (genf *)fd1094_rte_callback);
	cpunum_set_irq_callback(0, fd1094_int_callback);
}

/* startup function, to be called from DRIVER_INIT (once on startup) */
void fd1094_driver_init(void)
{
	int i;

	fd1094_cpuregion = (UINT16*)memory_region(REGION_CPU1);
	fd1094_cpuregionsize = memory_region_length(REGION_CPU1);
	fd1094_key = memory_region(REGION_USER1);

	/* punt if no key; this allows us to be called even for non-FD1094 games */
	if (!fd1094_key)
		return;

	for (i=0;i<S16_NUMCACHE;i++)
	{
		fd1094_cacheregion[i]=auto_malloc(fd1094_cpuregionsize);
	}

	/* flush the cached state array */
	for (i=0;i<S16_NUMCACHE;i++)
		fd1094_cached_states[i] = -1;

	fd1094_current_cacheposition = 0;
}
