#include <stdlib.h>
#include <err.h>
#include <GLFW/glfw3.h>

#ifndef __APPLE__
#include <GL/glew.h>
#endif

#include "gl.h"

static const int WIDTH = 640;
static const int HEIGHT = 480;

#define RESET "\e[0m"
#define RED "\e[1;31m"
#define YELLOW "\e[1;32m"
#define GREEN "\e[1;33m"

#ifndef __APPLE__
char * type_string( const GLenum type ) {
	switch (type) {
		case GL_DEBUG_TYPE_ERROR:
			return "error";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			return "deprecated";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			return "undefined";
		case GL_DEBUG_TYPE_PORTABILITY:
			return "nonportable";
		case GL_DEBUG_TYPE_PERFORMANCE:
			return "performance";
		case GL_DEBUG_TYPE_OTHER:
			return "other";
		default:
			return "idk";
	}
}

char * severity_string( const GLenum severity ) {
	switch( severity ) {
		case GL_DEBUG_SEVERITY_LOW:
			return GREEN "low" RESET;
		case GL_DEBUG_SEVERITY_MEDIUM:
			return YELLOW "medium" RESET;
		case GL_DEBUG_SEVERITY_HIGH:
			return RED "high" RESET;
		default:
			return "idk";
	}
}

void gl_error_printer(
	const GLenum source, const GLenum type, const GLuint id,
	const GLenum severity, const GLsizei length, const GLchar * const message, const void * _
) {
	warnx( "GL [%s - %s]: %s",
		type_string( type ),
		severity_string( severity ),
		message );

	if( severity == GL_DEBUG_SEVERITY_HIGH ) {
		exit( 1 );
	}
}
#endif

void glfw_error_printer( const int code, const char * const message ) {
	warnx( "GLFW error %d: %s", code, message );
}

GLFWwindow * GL::init() {
	glfwSetErrorCallback( glfw_error_printer );

	if( !glfwInit() ) {
		errx( 1, "glfwInit" );
	}

	glfwWindowHint( GLFW_RESIZABLE, 0 );

	glfwWindowHint( GLFW_CLIENT_API, GLFW_OPENGL_API );
	glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
	glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 2 );

	GLFWwindow * const window = glfwCreateWindow( WIDTH, HEIGHT, "bsp", nullptr, nullptr );
	if( !window ) {
		errx( 1, "glfwCreateWindow" );
	}

	glfwMakeContextCurrent( window );

	#ifndef __APPLE__
		glewExperimental = true;
		GLenum glew_error = glewInit();
		if( glew_error != GLEW_OK ) {
			errx( 1, "glewInit: %s", glewGetErrorString( glew_error ) );
		}

		glEnable( GL_DEBUG_OUTPUT );
		glDebugMessageCallback( gl_error_printer, nullptr );
	#endif

	warnx( "Version %s", glGetString( GL_VERSION ) );
	warnx( "Vendor %s", glGetString( GL_VENDOR ) );

	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );

	glEnable( GL_CULL_FACE );
	glCullFace( GL_FRONT );

	return window;
}

void GL::term() {
	glfwTerminate();
}
