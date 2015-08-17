#ifndef _PLATFORM_OPENGL_H_
#define _PLATFORM_OPENGL_H_

#ifdef __APPLE__
	#include <OpenGL/gl3.h>
#else
	#include <GL/glew.h>
	#include <GL/gl.h>
#endif

#include <stdio.h>

inline void glterrible() {
	GLenum err = glGetError();
	const char * error;

	switch(err) {
		case GL_INVALID_OPERATION:      error="INVALID_OPERATION";      break;
		case GL_INVALID_ENUM:           error="INVALID_ENUM";           break;
		case GL_INVALID_VALUE:          error="INVALID_VALUE";          break;
		case GL_OUT_OF_MEMORY:          error="OUT_OF_MEMORY";          break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:  error="INVALID_FRAMEBUFFER_OPERATION";  break;
		default: error = "shit bro"; break;
	}

	printf( "GL error: %s\n", error );
}

#endif // _PLATFORM_OPENGL_H_
