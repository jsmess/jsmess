#ifndef __XOR100__
#define __XOR100__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"5b"
#define I8251_A_TAG		"12b"
#define I8251_B_TAG		"14b"
#define I8255A_TAG		"8a"
#define COM5016_TAG		"15c"
#define Z80CTC_TAG		"11b"
#define WD1795_TAG		"wd1795"
#define CASSETTE_TAG	"cassette"
#define CENTRONICS_TAG	"centronics"
#define TERMINAL_TAG	"terminal"

typedef struct _xor100_state xor100_state;
struct _xor100_state
{
	/* memory state */
	int mode;
	int bank;

	/* floppy state */
	int fdc_irq;
	int fdc_drq;
	int fdc_dden;

	/* devices */
	running_device *i8251_a;
	running_device *i8251_b;
	running_device *wd1795;
	running_device *z80ctc;
	running_device *cassette;
	running_device *centronics;
};

#endif
