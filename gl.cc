#include <err.h>
#include <GLFW/glfw3.h>

#include "gl.h"

static const int WIDTH = 640;
static const int HEIGHT = 480;

static const int BITS_RED = 8;
static const int BITS_GREEN = 8;
static const int BITS_BLUE = 8;
static const int BITS_ALPHA = 8;
static const int BITS_DEPTH = 24;
static const int BITS_STENCIL = 8;

void error_handler( const int code, const char * const message ) {
	warnx( "GLFW error %d: %s", code, message );
}

GLFWwindow * GL::init() {
	glfwSetErrorCallback( error_handler );
	
	if( !glfwInit() ) {
		errx( 1, "glfwInit" );
	}

	glfwWindowHint( GLFW_RESIZABLE, 0 );

	GLFWwindow * const window = glfwCreateWindow( WIDTH, HEIGHT, "bsp", nullptr, nullptr );
	if( !window ) {
		errx( 1, "glfwCreateWindow" );
	}

	glfwMakeContextCurrent( window );

	// TODO: do matrices like a big boy
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	// TODO: do matrices like a big boy
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );

	glEnable( GL_CULL_FACE );
	glCullFace( GL_FRONT );

	glEnableClientState( GL_VERTEX_ARRAY );

	return window;
}

void GL::term() {
	glfwTerminate();
}

