
#pragma optimize (on)
#pragma debug (off)

uniform sampler2D     color_texture;
uniform vec2          color_texture_pow2_sz; // pow2 tex size
uniform vec4          vid_attributes;        // gamma, contrast, brightness

// #define DO_GAMMA  1 // 'pow' is very slow on old hardware, i.e. pre R600 and 'slow' in general

vec4 lutTex2D(in float x, in float y)
{
	return texture2D(color_texture, vec2(x, y));
}

vec4 f_bilinear(in vec2 xy)
{
	// mix(x,y,a): x*(1-a) + y*a
	//
	// bilinear filtering includes 2 mix:
	//
	//   pix1 = tex[x0][y0] * ( 1 - u_ratio ) + tex[x1][y0] * u_ratio
	//   pix2 = tex[x0][y1] * ( 1 - u_ratio ) + tex[x1][y1] * u_ratio
	//   fin  =    pix1     * ( 1 - v_ratio ) +     pix2    * v_ratio
	//
	// so we can use the build in mix function for these 2 computations ;-)
	//
	vec2 uv_ratio     = fract(xy*color_texture_pow2_sz); // xy*color_texture_pow2_sz - floor(xy*color_texture_pow2_sz);
	vec2 one          = 1.0/color_texture_pow2_sz;

	return mix ( mix( lutTex2D(xy.x, xy.y      ), lutTex2D(xy.x+one.x, xy.y      ), uv_ratio.x),
	             mix( lutTex2D(xy.x, xy.y+one.y), lutTex2D(xy.x+one.x, xy.y+one.y), uv_ratio.x), uv_ratio.y )
		;
}

void main()
{
#ifdef DO_GAMMA
	vec4 gamma = vec4(1.0 / vid_attributes.r, 1.0 / vid_attributes.r, 1.0 / vid_attributes.r, 0.0);

	// gamma, contrast, brightness equation from: rendutil.h / apply_brightness_contrast_gamma_fp
	vec4 color = pow( f_bilinear(gl_TexCoord[0].st) , gamma);
#else
	vec4 color = f_bilinear(gl_TexCoord[0].st);
#endif

	// contrast/brightness
	gl_FragColor = (color * vid_attributes.g) + vid_attributes.b - 1.0;
}

