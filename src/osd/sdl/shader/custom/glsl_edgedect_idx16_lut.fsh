
#pragma optimize (on)
#pragma debug (off)

uniform sampler2D color_texture;
uniform vec2      color_texture_pow2_sz; // pow2 tex size

uniform sampler2D colortable_texture;
uniform vec2      colortable_sz;      // ct size
uniform vec2      colortable_pow2_sz; // pow2 ct size

#define  KVAL(v) vec4( v, v, v, 0.0 )

#if 0
#define KERNEL_SIZE 9


vec4 KernelValue[KERNEL_SIZE] = {
		KVAL( 0.0), KVAL(  1.0), KVAL( 0.0),
		KVAL( 1.0), KVAL( -4.0), KVAL( 1.0),
		KVAL( 0.0), KVAL(  1.0), KVAL( 0.0)
};

vec2 Offset[KERNEL_SIZE] = {
	vec2( -1.0, -1.0 ), vec2( +0.0, -1.0 ), vec2( +1.0, -1.0 ),
	vec2( -1.0, +0.0 ), vec2( +0.0, +0.0 ), vec2( +1.0, +0.0 ),
	vec2( -1.0, +1.0 ), vec2( +0.0, +1.0 ), vec2( +1.0, +1.0 )
};
#endif
				

vec4 lutTex2D(in vec2 texcoord)
{
	vec4 color_tex;
	vec2 color_map_coord;

	// normalized texture coordinates ..
	color_tex = texture2D(color_texture, texcoord);

	// GL_UNSIGNED_SHORT GL_ALPHA in ALPHA16 conversion:
	// general: f = c / ((2*N)-1), c color bitfield, N number of bits
	// ushort:  c = ((2**16)-1)*f;
	color_map_coord.x  = 65536.0 * color_tex.a;

	// map it to the 2D lut table
	color_map_coord.y = floor(color_map_coord.x/colortable_sz.x);
	color_map_coord.x =   mod(color_map_coord.x,colortable_sz.x);

	return texture2D(colortable_texture, color_map_coord/(colortable_pow2_sz-1.0));
}

void main()
{
	vec4 sum = vec4(0.0);

#if 0
	int i;

	// sum it up ..
	for(i=0; i<KERNEL_SIZE; i++)
	{
		sum += lutTex2D(gl_TexCoord[0].st + Offset[i]/color_texture_pow2_sz) * KernelValue[i];
	}
#else
	sum += lutTex2D(gl_TexCoord[0].st + vec2(-1.0, -1.0)/color_texture_pow2_sz) * KVAL( 0.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2( 0.0, -1.0)/color_texture_pow2_sz) * KVAL( 1.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2(+1.0, -1.0)/color_texture_pow2_sz) * KVAL( 0.0);

	sum += lutTex2D(gl_TexCoord[0].st + vec2(-1.0,  0.0)/color_texture_pow2_sz) * KVAL( 1.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2( 0.0,  0.0)/color_texture_pow2_sz) * KVAL(-4.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2(+1.0,  0.0)/color_texture_pow2_sz) * KVAL( 1.0);

	sum += lutTex2D(gl_TexCoord[0].st + vec2(-1.0, +1.0)/color_texture_pow2_sz) * KVAL( 0.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2( 0.0, +1.0)/color_texture_pow2_sz) * KVAL( 1.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2(+1.0, +1.0)/color_texture_pow2_sz) * KVAL( 0.0);
#endif
	
	gl_FragColor = sum;
}

