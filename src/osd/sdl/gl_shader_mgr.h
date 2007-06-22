
#ifndef GL_SHADER_MGR_H
#define GL_SHADER_MGR_H

#include <SDL/SDL.h>

// OpenGL headers
#ifdef SDLMAME_MACOSX
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <SDL/SDL_opengl.h>

#include "gl_shader_tool.h"

// OSD headers
#include "window.h"

// #define GLSL_SOURCE_ON_DISK 1

typedef enum {
	GLSL_SHADER_TYPE_IDX16,
	GLSL_SHADER_TYPE_RGB32_LUT,
	GLSL_SHADER_TYPE_RGB32_DIRECT,
	GLSL_SHADER_TYPE_NUMBER
} GLSL_SHADER_TYPE;

typedef enum {
	GLSL_SHADER_FEAT_PLAIN,		
	GLSL_SHADER_FEAT_BILINEAR,		
	GLSL_SHADER_FEAT_CONV_GAUSSIAN,	
	GLSL_SHADER_FEAT_NUMBER
} GLSL_SHADER_FEATURE;

extern GLhandleARB glsl_programs [GLSL_SHADER_TYPE_NUMBER][GLSL_SHADER_FEAT_NUMBER];

extern GLhandleARB glsl_general_vsh; // one for all

extern GLhandleARB glsl_fsh_shader [GLSL_SHADER_TYPE_NUMBER][GLSL_SHADER_FEAT_NUMBER];

/**
 * returns 0 if ok, otherwise an error value
 */
int glsl_shader_init(sdl_info *sdl);

/**
 * returns 0 if ok, otherwise an error value
 */
int glsl_shader_free(sdl_info *sdl);

#endif // GL_SHADER_MGR_H
