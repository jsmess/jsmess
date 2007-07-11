
#include "gl_shader_mgr.h"

#define GLSL_SHADER_FEAT_INT_NUMBER 3 // plain, bilinear, conv3x3 

#ifdef GLSL_SOURCE_ON_DISK

static const char * glsl_fsh_files [GLSL_SHADER_TYPE_NUMBER][GLSL_SHADER_FEAT_INT_NUMBER] =
{
 {"/tmp/glsl_idx16_lut.fsh",								// idx16 lut plain
  "/tmp/glsl_idx16_lut_bilinear.fsh",							// idx16 lut bilinear
  "/tmp/glsl_idx16_lut_conv3x3.fsh" },		                                        // idx16 lut conv3x3

 {"/tmp/glsl_rgb32_lut.fsh",								// rgb32 lut plain
  "/tmp/glsl_rgb32_lut_bilinear.fsh",							// rgb32 lut bilinear
  "/tmp/glsl_rgb32_lut_conv3x3.fsh" },							// rgb32 lut conv3x3

 {"/tmp/glsl_rgb32_dir.fsh",								// rgb32 dir plain
  "/tmp/glsl_rgb32_dir_bilinear.fsh",							// rgb32 dir bilinear
  "/tmp/glsl_rgb32_dir_conv3x3.fsh" }							// rgb32 dir conv3x3
};

#else // GLSL_SOURCE_ON_DISK

#include "shader/glsl_general.vsh.c"

#include "shader/glsl_idx16_lut.fsh.c"
#include "shader/glsl_idx16_lut_bilinear.fsh.c"
#include "shader/glsl_idx16_lut_conv3x3.fsh.c"

#include "shader/glsl_rgb32_lut.fsh.c"
#include "shader/glsl_rgb32_lut_bilinear.fsh.c"
#include "shader/glsl_rgb32_lut_conv3x3.fsh.c"

#include "shader/glsl_rgb32_dir.fsh.c"
#include "shader/glsl_rgb32_dir_bilinear.fsh.c"
#include "shader/glsl_rgb32_dir_conv3x3.fsh.c"

static const char * glsl_fsh_sources [GLSL_SHADER_TYPE_NUMBER][GLSL_SHADER_FEAT_INT_NUMBER] =
{
 {glsl_idx16_lut_fsh_src,								// idx16 lut plain
  glsl_idx16_lut_bilinear_fsh_src,							// idx16 lut bilinear
  glsl_idx16_lut_conv3x3_fsh_src },							// idx16 lut conv3x3

 {glsl_rgb32_lut_fsh_src,								// rgb32 lut plain
  glsl_rgb32_lut_bilinear_fsh_src,							// rgb32 lut bilinear
  glsl_rgb32_lut_conv3x3_fsh_src },							// rgb32 lut conv3x3

 {glsl_rgb32_dir_fsh_src,								// rgb32 dir plain
  glsl_rgb32_dir_bilinear_fsh_src,							// rgb32 dir bilinear
  glsl_rgb32_dir_conv3x3_fsh_src },							// rgb32 dir conv3x3
};

#endif // GLSL_SOURCE_ON_DISK

static GLhandleARB glsl_programs [GLSL_SHADER_TYPE_NUMBER][GLSL_SHADER_FEAT_INT_NUMBER] =
{
 {  0, 0, 0 },  /* idx16 lut: plain, bilinear, conv3x3 */ 
 {  0, 0, 0 },  /* rgb32 lut: plain, bilinear, conv3x3 */ 
 {  0, 0, 0 },  /* rgb32 dir: plain, bilinear, conv3x3 */ 
};

static GLhandleARB glsl_general_vsh = 0; // one for all

static GLhandleARB glsl_fsh_shader [GLSL_SHADER_TYPE_NUMBER][GLSL_SHADER_FEAT_INT_NUMBER] =
{
 {  0, 0, 0 },  /* idx16 lut: plain, bilinear, conv3x3 */ 
 {  0, 0, 0 },  /* rgb32 lut: plain, bilinear, conv3x3 */ 
 {  0, 0, 0 },  /* rgb32 dir: plain, bilinear, conv3x3 */ 
};

GLhandleARB glsl_shader_get_program(int glslShaderType, int glslShaderFeature)
{
	GLhandleARB prog = 0;

	if ( glslShaderType < 0 || glslShaderType>=GLSL_SHADER_TYPE_NUMBER)
		return 0;

	switch(glslShaderFeature)
	{
		case GLSL_SHADER_FEAT_PLAIN:
			prog = glsl_programs[glslShaderType][0];
			break;
		case GLSL_SHADER_FEAT_BILINEAR:
			prog = glsl_programs[glslShaderType][1];
			break;
		case GLSL_SHADER_FEAT_CONV_GAUSSIAN:
		case GLSL_SHADER_FEAT_CONV_EDGE:
			prog = glsl_programs[glslShaderType][2];
			break;
	}
	return prog;
}

int glsl_shader_init(sdl_info *sdl)
{
	int i,j, err;

        err = gl_shader_loadExtention((PFNGLGETPROCADDRESSOS)SDL_GL_GetProcAddress);
	if(err) return err;
	
	#ifdef GLSL_SOURCE_ON_DISK
	err = gl_compile_shader_file  ( &glsl_general_vsh, GL_VERTEX_SHADER_ARB, "/tmp/glsl_general.vsh", 0 /* verbose */ );
	#else
	err = gl_compile_shader_source( &glsl_general_vsh, GL_VERTEX_SHADER_ARB, glsl_general_vsh_src, 0 /* verbose */ );
	#endif

	if(err) return err;

	for (i=0; !err && i<GLSL_SHADER_TYPE_NUMBER; i++)
	{
		for (j=0; !err && j<GLSL_SHADER_FEAT_INT_NUMBER; j++)
		{
		#ifdef GLSL_SOURCE_ON_DISK
			if(glsl_fsh_files[i][j])
				err = gl_compile_shader_files  (&glsl_programs[i][j], &glsl_general_vsh, &glsl_fsh_shader[i][j],
								NULL /*precompiled*/, glsl_fsh_files[i][j], 0);
		#else
			if(glsl_fsh_sources[i][j])
				err = gl_compile_shader_sources(&glsl_programs[i][j], &glsl_general_vsh, &glsl_fsh_shader[i][j],
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

	(void) gl_delete_shader( NULL,  &glsl_general_vsh, NULL); 

	for (i=0; i<GLSL_SHADER_TYPE_NUMBER; i++)
	{
		for (j=0; j<GLSL_SHADER_FEAT_INT_NUMBER; j++)
		{
			(void) gl_delete_shader( NULL, NULL, &glsl_fsh_shader[i][j]);
		}
	}

	for (i=0; i<GLSL_SHADER_TYPE_NUMBER; i++)
	{
		for (j=0; j<GLSL_SHADER_FEAT_INT_NUMBER; j++)
		{
			(void) gl_delete_shader( &glsl_programs[i][j], NULL, NULL);
		}
	}

	return 0;
}

