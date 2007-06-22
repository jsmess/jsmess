
#pragma optimize (on)
#pragma debug (off)

uniform sampler2D color_texture;
uniform sampler2D colortable_texture;
uniform vec2      colortable_sz; // ct size
uniform vec2      colortable_pow2_sz; // pow2 ct size
uniform vec2      color_texture_pow2_sz; // pow2 tex size

vec4 lutTex2D(in float x, in float y)
{
	vec2 texcoord = vec2(x, y);
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

	return texture2D(colortable_texture, color_map_coord/colortable_pow2_sz);
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
	             mix( lutTex2D(xy.x, xy.y+one.y), lutTex2D(xy.x+one.x, xy.y+one.y), uv_ratio.x), uv_ratio.y );
}

void main()
{
	gl_FragColor = f_bilinear(gl_TexCoord[0].st);
}

