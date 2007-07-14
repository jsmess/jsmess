
#include "gl_shader_mgr.h"

#define GLSL_VERTEX_SHADER_INT_NUMBER 1 // general
#define GLSL_VERTEX_SHADER_MAX_NUMBER 2 // general + custom
#define GLSL_VERTEX_SHADER_CUSTOM     1 // custom idx

#ifdef GLSL_SOURCE_ON_DISK

static const char * glsl_vsh_files [GLSL_VERTEX_SHADER_INT_NUMBER] =
{
   "/tmp/glsl_general.vsh"								// general
};

static const char * glsl_fsh_files [GLSL_SHADER_TYPE_NUMBER][GLSL_SHADER_FEAT_INT_NUMBER] =
{
 {"/tmp/glsl_plain_idx16_lut.fsh",							// idx16 lut plain
  "/tmp/glsl_bilinear_idx16_lut.fsh"							// idx16 lut bilinear
 },

 {"/tmp/glsl_plain_rgb32_lut.fsh",							// rgb32 lut plain
  "/tmp/glsl_bilinear_rgb32_lut.fsh"							// rgb32 lut bilinear
 },

 {"/tmp/glsl_plain_rgb32_dir.fsh",							// rgb32 dir plain
  "/tmp/glsl_bilinear_rgb32_dir.fsh"							// rgb32 dir bilinear
 }
};

#else // GLSL_SOURCE_ON_DISK

#include "shader/glsl_general.vsh.c"

#include "shader/glsl_plain_idx16_lut.fsh.c"
#include "shader/glsl_bilinear_idx16_lut.fsh.c"

#include "shader/glsl_plain_rgb32_lut.fsh.c"
#include "shader/glsl_bilinear_rgb32_lut.fsh.c"

#include "shader/glsl_plain_rgb32_dir.fsh.c"
#include "shader/glsl_bilinear_rgb32_dir.fsh.c"

static const char * glsl_vsh_sources [GLSL_VERTEX_SHADER_INT_NUMBER] =
{
   glsl_general_vsh_src									// general
};

static const char * glsl_fsh_sources [GLSL_SHADER_TYPE_NUMBER][GLSL_SHADER_FEAT_INT_NUMBER] =
{
 {glsl_plain_idx16_lut_fsh_src,								// idx16 lut plain
  glsl_bilinear_idx16_lut_fsh_src							// idx16 lut bilinear
 },

 {glsl_plain_rgb32_lut_fsh_src,								// rgb32 lut plain
  glsl_bilinear_rgb32_lut_fsh_src							// rgb32 lut bilinear
 },

 {glsl_plain_rgb32_dir_fsh_src,								// rgb32 dir plain
  glsl_bilinear_rgb32_dir_fsh_src							// rgb32 dir bilinear
 }
};

#endif // GLSL_SOURCE_ON_DISK

static const char * glsl_filter_names [GLSL_SHADER_FEAT_MAX_NUMBER] =
{
  "plain",
  "bilinear",
  "custom"
};

static GLhandleARB glsl_programs [GLSL_SHADER_TYPE_NUMBER][GLSL_SHADER_FEAT_MAX_NUMBER] =
{
 {  0, 0, 0 },  /* idx16 lut: plain, bilinear, custom */ 
 {  0, 0, 0 },  /* rgb32 lut: plain, bilinear, custom */ 
 {  0, 0, 0 },  /* rgb32 dir: plain, bilinear, custom */ 
};

/**
 * fragment shader -> vertex shader mapping
 */
static int glsl_fsh2vsh[GLSL_SHADER_FEAT_MAX_NUMBER] =
{
	0,	// plain	-> general
	0,	// bilinear	-> general
	1,      // custom       -> custom
};

static GLhandleARB glsl_vsh_shader[GLSL_VERTEX_SHADER_MAX_NUMBER] = 
{ 0, 0 }; /* general, custom */

static GLhandleARB glsl_fsh_shader [GLSL_SHADER_TYPE_NUMBER][GLSL_SHADER_FEAT_MAX_NUMBER] =
{
 {  0, 0, 0 },  /* idx16 lut: plain, bilinear, custom */ 
 {  0, 0, 0 },  /* rgb32 lut: plain, bilinear, custom */ 
 {  0, 0, 0 },  /* rgb32 dir: plain, bilinear, custom */ 
};

const char * glsl_shader_get_filter_name(int glslShaderFeature)
{
	if ( !(0 <= glslShaderFeature && glslShaderFeature < GLSL_SHADER_FEAT_MAX_NUMBER) )
		return "illegal shader feature";

	return glsl_filter_names[glslShaderFeature];
}

GLhandleARB glsl_shader_get_program(int glslShaderType, int glslShaderFeature)
{
	if ( !(0 <= glslShaderType && glslShaderType < GLSL_SHADER_TYPE_NUMBER) )
		return 0;

	if ( !(0 <= glslShaderFeature && glslShaderFeature < GLSL_SHADER_FEAT_MAX_NUMBER) )
		return 0;

	return glsl_programs[glslShaderType][glslShaderFeature];
}

int glsl_shader_init(sdl_info *sdl)
{
	int i,j, err;

        err = gl_shader_loadExtention((PFNGLGETPROCADDRESSOS)SDL_GL_GetProcAddress);
	if(err) return err;
	
	for (i=0; !err && i<GLSL_VERTEX_SHADER_INT_NUMBER; i++)
	{
	#ifdef GLSL_SOURCE_ON_DISK
		if(glsl_vsh_files[i])
			err = gl_compile_shader_file  ( &glsl_vsh_shader[i], GL_VERTEX_SHADER_ARB, glsl_vsh_files[i], 0);
	#else
		if(glsl_vsh_sources[i])
			err = gl_compile_shader_source( &glsl_vsh_shader[i], GL_VERTEX_SHADER_ARB, glsl_vsh_sources[i], 0);
	#endif
	}

	if(err) return err;

	for (i=0; !err && i<GLSL_SHADER_TYPE_NUMBER; i++)
	{
		for (j=0; !err && j<GLSL_SHADER_FEAT_INT_NUMBER; j++)
		{
		#ifdef GLSL_SOURCE_ON_DISK
			if(glsl_fsh_files[i][j])
				err = gl_compile_shader_files  (&glsl_programs[i][j], 
								&glsl_vsh_shader[glsl_fsh2vsh[j]], &glsl_fsh_shader[i][j],
								NULL /*precompiled*/, glsl_fsh_files[i][j], 0);
		#else
			if(glsl_fsh_sources[i][j])
				err = gl_compile_shader_sources(&glsl_programs[i][j], 
								&glsl_vsh_shader[glsl_fsh2vsh[j]], &glsl_fsh_shader[i][j],
								NULL /*precompiled*/, glsl_fsh_sources[i][j]);
		#endif
		}
	}
	return err;
}

int glsl_shader_free(sdl_info *sdl)
{
	int i,j;

	pfn_glUseProgramObjectARB(0); // back to fixed function pipeline
        glFinish();

	for (i=0; i<GLSL_VERTEX_SHADER_MAX_NUMBER; i++)
	{
		if ( glsl_vsh_shader[i] )
			(void) gl_delete_shader( NULL,  &glsl_vsh_shader[i], NULL); 
	}

	for (i=0; i<GLSL_SHADER_TYPE_NUMBER; i++)
	{
		for (j=0; j<GLSL_SHADER_FEAT_MAX_NUMBER; j++)
		{
			if ( glsl_fsh_shader[i][j] )
				(void) gl_delete_shader( NULL, NULL, &glsl_fsh_shader[i][j]);
		}
	}

	for (i=0; i<GLSL_SHADER_TYPE_NUMBER; i++)
	{
		for (j=0; j<GLSL_SHADER_FEAT_MAX_NUMBER; j++)
		{
			if ( glsl_programs[i][j] )
				(void) gl_delete_shader( &glsl_programs[i][j], NULL, NULL);
		}
	}

	return 0;
}

int glsl_shader_add_custom(sdl_info *sdl, const char * custShaderPrefix)
{
	int i, err;
	static char fname[8192];

	snprintf(fname, 8192, "%s.vsh", custShaderPrefix); fname[8191]=0;

	err = gl_compile_shader_file  ( &glsl_vsh_shader[GLSL_VERTEX_SHADER_CUSTOM], GL_VERTEX_SHADER_ARB, fname, 0);
	if(err) return err;

	for (i=0; !err && i<GLSL_SHADER_TYPE_NUMBER; i++)
	{
		switch(i)
		{
			case GLSL_SHADER_TYPE_IDX16:
				snprintf(fname, 8192, "%s_idx16_lut.fsh", custShaderPrefix); fname[8191]=0;
				break;
			case GLSL_SHADER_TYPE_RGB32_LUT:
				snprintf(fname, 8192, "%s_rgb32_lut.fsh", custShaderPrefix); fname[8191]=0;
				break;
			case GLSL_SHADER_TYPE_RGB32_DIRECT:
				snprintf(fname, 8192, "%s_rgb32_dir.fsh", custShaderPrefix); fname[8191]=0;
				break;
		}

		err = gl_compile_shader_files  (&glsl_programs[i][GLSL_SHADER_FEAT_CUSTOM], 
						&glsl_vsh_shader[GLSL_VERTEX_SHADER_CUSTOM],
						&glsl_fsh_shader[i][GLSL_SHADER_FEAT_CUSTOM],
						NULL /*precompiled*/, fname, 0);
	}
	return err;
}

