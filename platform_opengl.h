#ifndef _PLATFORM_OPENGL_H_
#define _PLATFORM_OPENGL_H_

#ifdef __APPLE__
	#include <OpenGL/gl3.h>
#else
	#include <GL/glew.h>
	#include <GL/gl.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <glm/glm.hpp>

inline void print_vec( const char * const name, const glm::vec3 v ) {
	printf( "%s: %.3f %.3f %.3f\n", name, v.x, v.y, v.z );
}

inline void print_vec( const char * const name, const glm::ivec2 v ) {
	printf( "%s: %d %d\n", name, v.x, v.y );
}

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

#define GLSL( shader ) "#version 150\n" #shader

// this is taken from somewhere online
inline void show_info_log(
	GLuint object,
	PFNGLGETSHADERIVPROC glGet__iv,
	PFNGLGETSHADERINFOLOGPROC glGet__InfoLog
)
{
	GLint log_length;
	char *log;

	glGet__iv(object, GL_INFO_LOG_LENGTH, &log_length);
	log = ( char * ) malloc(log_length);
	glGet__InfoLog(object, log_length, nullptr, log);
	fprintf(stderr, "%s", log);
	free(log);
}

// this is taken from somewhere online
inline void check_compile_status( const GLuint shader ) {
	GLint ok;
	glGetShaderiv( shader, GL_COMPILE_STATUS, &ok );

	if( !ok ) {
		fprintf( stderr, "shit bruh:\n" );
		show_info_log( shader, glGetShaderiv, glGetShaderInfoLog );
		glDeleteShader( shader );
	}
}

// this is taken from somewhere online
inline void check_link_status( const GLuint prog ) {
	GLint ok;
	glGetProgramiv(prog, GL_LINK_STATUS, &ok);
	if (!ok) {
		fprintf(stderr, "Failed to link shader program:\n");
		show_info_log(prog, glGetProgramiv, glGetProgramInfoLog);
		glDeleteProgram(prog);
	}
}

inline GLuint compile_shader( const char * const vert, const char * const frag, const char * const out ) {
	const GLuint vs = glCreateShader( GL_VERTEX_SHADER );
	const GLuint fs = glCreateShader( GL_FRAGMENT_SHADER );

	glShaderSource( vs, 1, &vert, NULL );
	glShaderSource( fs, 1, &frag, NULL );

	glCompileShader( vs );
	check_compile_status( vs );
	glCompileShader( fs );
	check_compile_status( fs );

	const GLuint prog = glCreateProgram();

	glAttachShader( prog, vs );
	glAttachShader( prog, fs );
	glBindFragDataLocation( prog, 0, out );
	glLinkProgram( prog );

	glDeleteShader( vs );
	glDeleteShader( fs );

	check_link_status( prog );

	return prog;
}

#endif // _PLATFORM_OPENGL_H_
