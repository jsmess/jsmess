/*****************************************************************************
 *
 * includes/vc20tape.h
 *
 ****************************************************************************/

#ifndef VC20TAPE_H_
#define VC20TAPE_H_


/*----------- defined in machine/vc20tape.c -----------*/

/* put this into your gamedriver */
void vc20tape_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);

/* the function which should be called by change on readline */
extern void vc20_tape_open (write8_machine_func read_callback);
extern void c16_tape_open (void);
extern void vc20_tape_close (void);

/* must be high active keys */
/* set it frequently */
extern void vc20_tape_buttons (int play, int record, int stop);
extern void vc20_tape_config (int on, int noise);

extern int vc20_tape_read (void);
extern void vc20_tape_write (int data);
extern void vc20_tape_motor (int data);

/* pressed wenn cassette is in */
extern int vc20_tape_switch (void);

/* delivers status for displaying */
extern void vc20_tape_status (char *text, int size);


#endif /* VC20TAPE_H_ */
