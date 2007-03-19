
extern unsigned char *astrocade_videoram;

DEVICE_LOAD(astrocade_rom);

extern PALETTE_INIT( astrocade );
extern  READ8_HANDLER( astrocade_intercept_r );
extern WRITE8_HANDLER( astrocade_videoram_w );
extern WRITE8_HANDLER( astrocade_magic_expand_color_w );
extern WRITE8_HANDLER( astrocade_magic_control_w );
extern WRITE8_HANDLER( astrocade_magicram_w );

extern  READ8_HANDLER( astrocade_video_retrace_r );
extern WRITE8_HANDLER( astrocade_vertical_blank_w );
extern WRITE8_HANDLER( astrocade_interrupt_enable_w );
extern WRITE8_HANDLER( astrocade_interrupt_w );
extern INTERRUPT_GEN( astrocade_interrupt );

WRITE8_HANDLER ( astrocade_mode_w );
WRITE8_HANDLER ( astrocade_interrupt_vector_w );

WRITE8_HANDLER ( astrocade_colour_register_w );
WRITE8_HANDLER ( astrocade_colour_block_w );
WRITE8_HANDLER ( astrocade_colour_split_w );

void astrocade_copy_line(int line);
