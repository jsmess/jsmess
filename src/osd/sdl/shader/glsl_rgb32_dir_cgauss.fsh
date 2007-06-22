
#pragma optimize (on)
#pragma debug (off)

uniform sampler2D color_texture;
uniform vec4      vid_attributes;     // gamma, contrast, brightness

// #define DO_GAMMA  1 // 'pow' is very slow on old hardware, i.e. pre R600 and 'slow' in general

#define KERNEL_SIZE 9
const int MaxKernelSize = KERNEL_SIZE;

uniform vec2 Offset[MaxKernelSize];

uniform vec4 KernelValue[MaxKernelSize];

void main()
{
#ifdef DO_GAMMA
	vec4 gamma = vec4(1.0 / vid_attributes.r, 1.0 / vid_attributes.r, 1.0 / vid_attributes.r, 0.0);
#endif
	vec4 sum   = vec4(0.0);
	int i;

	// sum it up ..
	for(i=0; i<KERNEL_SIZE; i++)
	{
		sum += texture2D(color_texture, gl_TexCoord[0].st + Offset[i]) * KernelValue[i];
	}

#ifdef DO_GAMMA
	// gamma, contrast, brightness equation from: rendutil.h / apply_brightness_contrast_gamma_fp
	sum = pow( sum, gamma );
#endif

	// contrast/brightness
	gl_FragColor = (sum * vid_attributes.g) + vid_attributes.b - 1.0;
}

