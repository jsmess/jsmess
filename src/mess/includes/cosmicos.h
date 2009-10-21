#ifndef __COSMICOS__
#define __COSMICOS__

#define CDP1802_TAG		"ic19"
#define CDP1864_TAG		"ic3"
#define DM9368_TAG		"ic10"
#define CASSETTE_TAG	"cassette"
#define SCREEN_TAG		"screen"

typedef struct _cosmicos_state cosmicos_state;
struct _cosmicos_state
{
	/* memory state */
	int boot;

	/* keyboard state */
	UINT8 keylatch;

	/* display state */
	UINT8 segment;
	int counter;
	int q;
	int dmaout;
	int efx;
	int video_on;

	/* devices */
	const device_config *dm9368;
	const device_config *cdp1864;
	const device_config *cassette;
};

#endif
