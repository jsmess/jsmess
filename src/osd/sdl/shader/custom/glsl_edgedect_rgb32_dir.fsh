
#pragma optimize (on)
#pragma debug (off)

uniform sampler2D color_texture;
uniform vec2      color_texture_pow2_sz; // pow2 tex size

uniform vec4      vid_attributes;     // gamma, contrast, brightness

// #define DO_GAMMA  1 // 'pow' is very slow on old hardware, i.e. pre R600 and 'slow' in general

#define lutTex2D(v) texture2D(color_texture, (v))

#define  KVAL(v) vec4( v/8.0, v/8.0, v/8.0, 0.0 )

void main()
{
#ifdef DO_GAMMA
	vec4 gamma = vec4(1.0 / vid_attributes.r, 1.0 / vid_attributes.r, 1.0 / vid_attributes.r, 0.0);
#endif
	vec4 sum   = vec4(0.0);

	sum += lutTex2D(gl_TexCoord[0].st + vec2(-1.0, -1.0)/color_texture_pow2_sz) * KVAL( 0.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2( 0.0, -1.0)/color_texture_pow2_sz) * KVAL( 1.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2(+1.0, -1.0)/color_texture_pow2_sz) * KVAL( 0.0);

	sum += lutTex2D(gl_TexCoord[0].st + vec2(-1.0,  0.0)/color_texture_pow2_sz) * KVAL( 1.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2( 0.0,  0.0)/color_texture_pow2_sz) * KVAL(-4.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2(+1.0,  0.0)/color_texture_pow2_sz) * KVAL( 1.0);

	sum += lutTex2D(gl_TexCoord[0].st + vec2(-1.0, +1.0)/color_texture_pow2_sz) * KVAL( 0.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2( 0.0, +1.0)/color_texture_pow2_sz) * KVAL( 1.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2(+1.0, +1.0)/color_texture_pow2_sz) * KVAL( 0.0);

#ifdef DO_GAMMA
	// gamma, contrast, brightness equation from: rendutil.h / apply_brightness_contrast_gamma_fp
	sum = pow( sum, gamma );
#endif

	// contrast/brightness
	gl_FragColor = (sum * vid_attributes.g) + vid_attributes.b - 1.0;
}

