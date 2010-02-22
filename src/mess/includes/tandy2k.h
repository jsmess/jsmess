#ifndef __TANDY2K__
#define __TANDY2K__

#define SCREEN_TAG		"screen"
#define I80186_TAG		"u76"
#define I8048_TAG		"m1"
#define I8255A_TAG		"u75"
#define I8251A_TAG		"u41"
#define I8253_TAG		"u40"
#define I8259A_0_TAG	"u42"
#define I8259A_1_TAG	"u43"
#define I8272A_TAG		"u121"
#define CRT9007_TAG		"u16"
#define CRT9021_TAG		"u14"
#define WD1010_TAG		"u18"
#define SPEAKER_TAG		"speaker"
#define CENTRONICS_TAG	"centronics"

typedef struct _tandy2k_state tandy2k_state;
struct _tandy2k_state
{
	/* keyboard state */
	int kben;
	UINT16 keylatch;

	/* serial state */
	int extclk;
	int rxrdy;
	int txrdy;

	/* PPI state */
	int pb_sel;

	/* sound state */
	int outspkr;
	int spkrdata;

	/* devices */
	running_device *uart;
	running_device *pit;
	running_device *fdc;
	running_device *pic0;
	running_device *pic1;
	running_device *centronics;
	running_device *speaker;
};

#endif
