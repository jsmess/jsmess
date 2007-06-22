
#pragma optimize (on)
#pragma debug (off)

uniform sampler2D     color_texture;
uniform sampler2D     colortable_texture;
uniform vec2          colortable_sz;      // orig size
uniform vec2          colortable_pow2_sz; // pow2 ct size
uniform vec2          color_texture_pow2_sz; // pow2 tex size

vec4 lutTex2D(in float x, in float y)
{
	vec2 texcoord = vec2(x, y);
	vec4 color_tex;
	vec2 color_map_coord;
	vec4 color0;
	float colortable_scale = (colortable_sz.x/3.0) / colortable_pow2_sz.x;

	// normalized texture coordinates ..
	color_tex         = texture2D(color_texture, texcoord) * ((colortable_sz.x/3.0)-1.0)/colortable_pow2_sz.x;// lookup space 

	color_map_coord.x = color_tex.b;
	color0.b          = texture2D(colortable_texture, color_map_coord).b;

	color_map_coord.x = color_tex.g + colortable_scale;
	color0.g          = texture2D(colortable_texture, color_map_coord).g;

	color_map_coord.x = color_tex.r + 2.0 * colortable_scale;
	color0.r          = texture2D(colortable_texture, color_map_coord).r;

	return color0;
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

