/*************************************************************************

    SNK NeoGeo hardware

*************************************************************************/


#define NEOGEO_MASTER_CLOCK		(24000000)
#define NEOGEO_68K_CLOCK		(NEOGEO_MASTER_CLOCK / 2)
#define NEOGEO_Z80_CLOCK		(NEOGEO_MASTER_CLOCK / 6)
#define NEOGEO_YM2610_CLOCK		(NEOGEO_MASTER_CLOCK / 3)
#define NEOGEO_PIXEL_CLOCK		(NEOGEO_MASTER_CLOCK / 4)
#define NEOGEO_HTOTAL			(0x180)
#define NEOGEO_HBEND			(0x000)		/* ?? */
#define NEOGEO_HBSTART			(0x140)		/* ?? */
#define NEOGEO_VTOTAL			(0x108)
#define NEOGEO_VBEND			(0x010)
#define NEOGEO_VBSTART			(0x0f0)


/*----------- defined in drivers/neogeo.c -----------*/

extern UINT32 neogeo_animation_counter;
extern UINT32 neogeo_has_trackball;

void neogeo_set_cpu1_second_bank(UINT32 bankaddress);
void neogeo_init_cpu2_setbank(void);
void neogeo_register_main_savestate(void);
void neogeo_create_timers(void);
void neogeo_start_timers(void);

/*----------- defined in machine/neogeo.c -----------*/

extern UINT16 *neogeo_ram16;
extern UINT16 *neogeo_sram16;

extern UINT8 *neogeo_memcard;

extern UINT8 *neogeo_game_vectors;

MACHINE_START( neogeo );
MACHINE_RESET( neogeo );
DRIVER_INIT( neogeo );

NVRAM_HANDLER( neogeo );

READ16_HANDLER( neogeo_memcard16_r );
WRITE16_HANDLER( neogeo_memcard16_w );
MEMCARD_HANDLER( neogeo );

WRITE16_HANDLER (neogeo_select_bios_vectors);
WRITE16_HANDLER (neogeo_select_game_vectors);

/*----------- defined in machine/neocrypt.c -----------*/

void kof99_neogeo_gfx_decrypt(int extra_xor);
void kof2000_neogeo_gfx_decrypt(int extra_xor);
void cmc50_neogeo_gfx_decrypt(int extra_xor);
void cmc42_neogeo_gfx_decrypt(int extra_xor);
void kof99_decrypt_68k(void);
void garou_decrypt_68k(void);
void garouo_decrypt_68k(void);
void mslug3_decrypt_68k(void);
void kof2000_decrypt_68k(void);
void kof98_decrypt_68k(void);
void kof2002_decrypt_68k(void);
void matrim_decrypt_68k(void);
void mslug5_decrypt_68k(void);
void svcchaos_px_decrypt(void);
void svcpcb_gfx_decrypt(void);
void svcpcb_s1data_decrypt(void);
void samsho5_decrypt_68k(void);
void kf2k3pcb_gfx_decrypt(void);
void kf2k3pcb_decrypt_68k(void);
void kf2k3pcb_decrypt_s1data(void);
void kof2003_decrypt_68k(void);
void kof2003biosdecode(void);
void samsh5p_decrypt_68k(void);

void neo_pcm2_snk_1999(int value);
void neo_pcm2_swap(int value);

/*----------- defined in machine/neoprot.c -----------*/

extern INT32 neogeo_rng;

WRITE16_HANDLER( neogeo_sram16_lock_w );
WRITE16_HANDLER( neogeo_sram16_unlock_w );
READ16_HANDLER( neogeo_sram16_r );
WRITE16_HANDLER( neogeo_sram16_w );

void fatfury2_install_protection(void);
void mslugx_install_protection(void);
void kof99_install_protection(void);
void garou_install_protection(void);
void garouo_install_protection(void);
void mslug3_install_protection(void);
void kof2000_install_protection(void);
void install_kof98_protection(void);
void install_pvc_protection(void);

/*----------- defined in machine/neoboot.c -----------*/

void kog_px_decrypt(void);
void neogeo_bootleg_cx_decrypt(void);
void install_kof10th_protection(void);
void decrypt_kof10th(void);
void decrypt_kf10thep(void);
void decrypt_kf2k5uni(void);
void neogeo_bootleg_sx_decrypt(int value);
void kf2k2mp_decrypt(void);
void kof2km2_px_decrypt(void);
void decrypt_cthd2003(void);
void patch_cthd2003(void);
void decrypt_ct2k3sp(void);
void decrypt_kof2k4se_68k(void);
void lans2004_decrypt_68k(void);
void lans2004_vx_decrypt(void);
void install_ms5plus_protection(void);
void svcboot_px_decrypt( void );
void svcboot_cx_decrypt( void );
void svcplus_px_decrypt( void );
void svcplus_px_hack( void );
void svcplusa_px_decrypt( void );
void svcsplus_px_decrypt( void );
void svcsplus_px_hack( void );
void kof2003b_px_decrypt( void );
void kof2003b_install_protection(void);
void kof2k3pl_px_decrypt( void );
void kof2k3up_px_decrypt( void );
void kof2k3up_install_protection(void);
void kf2k3pl_install_protection(void);
void samsh5bl_px_decrypt( void );

/*----------- defined in video/neogeo.c -----------*/

extern int neogeo_fix_bank_type;

VIDEO_START( neogeo_mvs );

WRITE16_HANDLER( neogeo_setpalbank0_16_w );
WRITE16_HANDLER( neogeo_setpalbank1_16_w );
READ16_HANDLER( neogeo_paletteram16_r );
WRITE16_HANDLER( neogeo_paletteram16_w );

WRITE16_HANDLER( neogeo_vidram16_offset_w );
READ16_HANDLER( neogeo_vidram16_data_r );
WRITE16_HANDLER( neogeo_vidram16_data_w );
WRITE16_HANDLER( neogeo_vidram16_modulo_w );
READ16_HANDLER( neogeo_vidram16_modulo_r );
WRITE16_HANDLER( neo_board_fix_16_w );
WRITE16_HANDLER( neo_game_fix_16_w );

VIDEO_UPDATE( neogeo );
