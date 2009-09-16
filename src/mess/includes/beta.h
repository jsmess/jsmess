#ifndef __BETA__
#define __BETA__

#define SCREEN_TAG		"screen"
#define M6502_TAG		"m6502"
#define M6532_TAG		"m6532"
#define CASSETTE_TAG	"cassette"
#define SPEAKER_TAG		"b237"

typedef struct _beta_state beta_state;
struct _beta_state
{
	/* EPROM state */
	int ls164_cp;
	UINT16 eprom_addr;

	/* display state */
	UINT8 ls145_p;
	UINT8 segment;

	/* devices */
	const device_config *speaker;
};

#endif
