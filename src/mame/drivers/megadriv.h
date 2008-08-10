extern DRIVER_INIT( megadrie );
extern DRIVER_INIT( megadriv );
extern DRIVER_INIT( megadrij );
extern DRIVER_INIT( megadsvp );
extern DRIVER_INIT( picoe );
extern DRIVER_INIT( picou );
extern DRIVER_INIT( picoj );

INPUT_PORTS_EXTERN(megadriv);
INPUT_PORTS_EXTERN(aladbl);
INPUT_PORTS_EXTERN(megadri6);
INPUT_PORTS_EXTERN(ssf2ghw);
INPUT_PORTS_EXTERN(megdsvp);
INPUT_PORTS_EXTERN(pico);

MACHINE_DRIVER_EXTERN(megadriv);
MACHINE_DRIVER_EXTERN(megadpal);
MACHINE_DRIVER_EXTERN(_32x);
MACHINE_DRIVER_EXTERN(megdsvp);
MACHINE_DRIVER_EXTERN(pico);

extern UINT16* megadriv_backupram;
extern int megadriv_backupram_length;
extern UINT16* megadrive_ram;
extern int megadrive_region_export;
extern int megadrive_region_pal;
extern UINT8 megatech_bios_port_cc_dc_r(int offset, int ctrl);
extern void megadriv_stop_scanline_timer(void);

extern READ16_HANDLER ( megadriv_vdp_r );
extern WRITE16_HANDLER ( megadriv_vdp_w );

void megatech_set_megadrive_z80_as_megadrive_z80(running_machine *machine);

extern READ8_HANDLER (megatech_sms_ioport_dc_r);
extern READ8_HANDLER (megatech_sms_ioport_dd_r);

MACHINE_RESET( megadriv );
VIDEO_START( megadriv );
VIDEO_UPDATE( megadriv );
VIDEO_EOF( megadriv );

/* the same on all systems? */
#define MASTER_CLOCK		53693100
