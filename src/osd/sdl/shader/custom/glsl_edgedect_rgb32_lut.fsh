
#pragma optimize (on)
#pragma debug (off)

uniform sampler2D color_texture;
uniform vec2      color_texture_pow2_sz; // pow2 tex size

uniform sampler2D colortable_texture;
uniform vec2      colortable_sz;         // orig size for full bgr
uniform vec2      colortable_pow2_sz;    // orig size for full bgr

#define  KVAL(v) vec4( v/8.0, v/8.0, v/8.0, 0.0 )

vec4 lutTex2D(in vec2 texcoord)
{
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

void main()
{
	vec4 sum = vec4(0.0);
	
	sum += lutTex2D(gl_TexCoord[0].st + vec2(-1.0, -1.0)/color_texture_pow2_sz) * KVAL( 0.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2( 0.0, -1.0)/color_texture_pow2_sz) * KVAL( 1.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2(+1.0, -1.0)/color_texture_pow2_sz) * KVAL( 0.0);

	sum += lutTex2D(gl_TexCoord[0].st + vec2(-1.0,  0.0)/color_texture_pow2_sz) * KVAL( 1.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2( 0.0,  0.0)/color_texture_pow2_sz) * KVAL(-4.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2(+1.0,  0.0)/color_texture_pow2_sz) * KVAL( 1.0);

	sum += lutTex2D(gl_TexCoord[0].st + vec2(-1.0, +1.0)/color_texture_pow2_sz) * KVAL( 0.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2( 0.0, +1.0)/color_texture_pow2_sz) * KVAL( 1.0);
	sum += lutTex2D(gl_TexCoord[0].st + vec2(+1.0, +1.0)/color_texture_pow2_sz) * KVAL( 0.0);

	gl_FragColor = sum;
}

