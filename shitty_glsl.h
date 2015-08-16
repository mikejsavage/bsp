#ifndef _SHITTY_H_
#define _SHITTY_H_

#include <stdio.h>
#include "platform_opengl.h"

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

#endif // _SHITTY_H_
