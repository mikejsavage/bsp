#include <stdio.h>

#include <string>

#include <GL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "int.h"
#include "heightmap.h"
#include "stb_image.h"
#include "stb_perlin.h"

#define GLSL( shader ) "#version 150\n" #shader

const float SLOPE = 0.3;

const GLchar * const vert_src = GLSL(
	in vec3 position;
	in vec3 normal;
	in float lit;

	out vec3 n;
	out float depth;
	out float l;

	uniform mat4 vp;

	void main() {
		n = normal;
		l = lit;
		gl_Position = vp * vec4( position, 1.0 );
		depth = gl_Position.z;
	}
);

const GLchar * frag_src = GLSL(
	in vec3 n;
	in float depth;
	in float l;

	out vec4 colour;

	uniform vec3 sun;

	void main() {
		vec3 ground;
		if( n.z > 0.9 ) {
			ground = vec3( 0.4, 1.0, 0.4 );
		}
		else {
			ground = vec3( 0.7, 0.7, 0.5 );
		}

		float d = max( 0, -dot( n, sun ) );
		float light = max( 0.2, l * d );

		vec3 fog = vec3( 0.6, 0.6, 0.6 );

		float t = smoothstep( 400, 600, depth );

		colour = vec4( ( 1.0 - t ) * ground * light + t * fog, 1.0 );
	}
);

// this is taken from somewhere online
static void show_info_log(
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
void check_compile_status( const GLuint shader ) {
	GLint ok;
	glGetShaderiv( shader, GL_COMPILE_STATUS, &ok );

	if( !ok ) {
		fprintf( stderr, "shit bruh:\n" );
		show_info_log( shader, glGetShaderiv, glGetShaderInfoLog );
		glDeleteShader( shader );
	}
}

// this is taken from somewhere online
void check_link_status( const GLuint prog ) {
	GLint ok;
	glGetProgramiv(prog, GL_LINK_STATUS, &ok);
	if (!ok) {
		fprintf(stderr, "Failed to link shader program:\n");
		show_info_log(prog, glGetProgramiv, glGetProgramInfoLog);
		glDeleteProgram(prog);
	}
}

GLuint compile_shader( const char * const vert, const char * const frag, const char * const out ) {
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

float lerp( const float a, const float b, const float t ) {
	return a * ( 1 - t ) + b * t;
}

float bilinear_interpolation(
	const glm::vec3 & v1,
	const glm::vec3 & v2,
	const glm::vec3 & v3,
	const glm::vec3 & v4,
	const glm::vec2 & p
) {
	const float tx = ( p.x - v1.x ) / ( v2.x - v1.x );

	const float mx1 = lerp( v1.z, v2.z, tx );
	const float mx2 = lerp( v3.z, v4.z, tx );

	const float ty = ( p.y - v1.y ) / ( v3.y - v1.y );

	return lerp( mx1, mx2, ty );
}

// CCW winding points towards the camera
glm::vec3 triangle_normal_ccw( const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c ) {
	return glm::normalize( glm::cross( b - a, c - a ) );
}

glm::vec3 triangle_perp_ccw( const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c ) {
	return glm::cross( b - a, c - a );
}

Heightmap::~Heightmap() {
	unload();
	glDeleteProgram( shader );
}

void Heightmap::init() {
	shader = compile_shader( vert_src, frag_src, "colour" );

	at_pos = glGetAttribLocation( shader, "position" );
	at_normal = glGetAttribLocation( shader, "normal" );
	at_lit = glGetAttribLocation( shader, "lit" );

	un_vp = glGetUniformLocation( shader, "vp" );
	un_sun = glGetUniformLocation( shader, "sun" );
}

void Heightmap::load( const std::string & image, const int ox, const int oy ) {
	printf( "%s offset by %dx%d\n", image.c_str(), ox, oy );
	pixels = stbi_load( image.c_str(), &w, &h, nullptr, 1 );

	// printf( "begin lit calculations\n" );

	float * const lit = new GLfloat[ w * h ];
	for( int i = 0; i < w * h; i++ ) {
		lit[ i ] = 0;
	}

	for( int y = 0; y < h; y++ ) {
		lit[ y * w ] = -1;

		for( int x = 1; x < w; x++ ) {
			const float h = point( x, y ).z;
			const float al = lit[ y * w + x - 1 ];
			float dh;

			if( al == -1 ) {
				dh = point( x - 1, y ).z - ( h + SLOPE );
			}
			else {
				dh = al - ( h + SLOPE );
			}

			if( dh > 0 ) {
				lit[ y * w + x ] = h + dh;
			}
			else {
				lit[ y * w + x ] = -1;
			}
		}
	}

	for( int y = 0; y < h; y++ ) {
		for( int x = 0; x < w; x++ ) {
			const int i = y * w + x;

			lit[ i ] = lit[ i ] == -1 ? 1 : 0;
		}
	}

	// printf( "end lit calculations\n" );
        //
	// printf( "sending data to gpu\n" );


	GLfloat * const vertices = new GLfloat[ w * h * 3 ];
	GLfloat * const normals = new GLfloat[ w * h * 3 ];
	GLuint * const indices = new GLuint[ w * h * 6 ];

	for( int y = 0; y < h; y++ ) {
		for( int x = 0; x < w; x++ ) {
			const int base = 3 * ( y * w + x );
			const float height = point( x, y ).z;

			vertices[ base ] = x + ox;
			vertices[ base + 1 ] = y + oy;
			vertices[ base + 2 ] = height;

			const glm::vec3 normal = point_normal( x, y );

			normals[ base ] = normal.x;
			normals[ base + 1 ] = normal.y;
			normals[ base + 2 ] = normal.z;
		}
	}

	for( int y = 0; y < h - 1; y++ ) {
		for( int x = 0; x < w - 1; x++ ) {
			const int base = 6 * ( y * w + x );

			indices[ base + 0 ] = ( y + 0 ) * w + ( x + 0 );
			indices[ base + 1 ] = ( y + 1 ) * w + ( x + 0 );
			indices[ base + 2 ] = ( y + 0 ) * w + ( x + 1 );
			indices[ base + 3 ] = ( y + 1 ) * w + ( x + 1 );
			indices[ base + 4 ] = ( y + 0 ) * w + ( x + 1 );
			indices[ base + 5 ] = ( y + 1 ) * w + ( x + 0 );
		}
	}

	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

	glGenBuffers( 1, &vbo_verts );
	glBindBuffer( GL_ARRAY_BUFFER, vbo_verts );
	glBufferData( GL_ARRAY_BUFFER, w * h * sizeof( GLfloat ) * 3, vertices, GL_STATIC_DRAW );

	glGenBuffers( 1, &vbo_normals );
	glBindBuffer( GL_ARRAY_BUFFER, vbo_normals );
	glBufferData( GL_ARRAY_BUFFER, w * h * sizeof( GLfloat ) * 3, normals, GL_STATIC_DRAW );

	glGenBuffers( 1, &vbo_lit );
	glBindBuffer( GL_ARRAY_BUFFER, vbo_lit );
	glBufferData( GL_ARRAY_BUFFER, w * h * sizeof( GLfloat ), lit, GL_STATIC_DRAW );

	glGenBuffers( 1, &ebo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, w * h * sizeof( GLuint ) * 6, indices, GL_STATIC_DRAW );

	delete vertices;
	delete normals;
	delete indices;
	delete lit;
	stbi_image_free( pixels );

	// printf( "sent\n" );
}

void Heightmap::unload() {
	if( vbo_verts != 0 ) {
		glDeleteBuffers( 1, &vbo_verts );
		glDeleteBuffers( 1, &vbo_normals );
		glDeleteBuffers( 1, &vbo_lit );
		glDeleteBuffers( 1, &ebo );
		glDeleteVertexArrays( 1, &vao );

		vbo_verts = 0;
	}
}

glm::vec3 Heightmap::point( int x, int y ) const {
	return glm::vec3( x, y, pixels[ y * w + x ] / 4.0f );
}

glm::vec3 Heightmap::point_normal( int x, int y ) const {
	glm::vec3 bent_normal = glm::vec3( 0 );

	/*
	 * A vertex looks like this:
	 *
	 *      +y
	 *   \ 6 |
	 *    \  |
	 * 5   \ |  1
	 *      \|
	 * ------X------ +x
	 *       |\
	 *    2  | \   4
	 *       |  \
	 *       | 3 \
	 *
	 * We consider up to six triangles for the bent normal
	 */

	if( x > 0 ) {
		if( y > 0 ) { // bottom left
			const glm::vec3 tri2a = point( x, y );
			const glm::vec3 tri2b = point( x - 1, y );
			const glm::vec3 tri2c = point( x, y - 1 );

			bent_normal += triangle_normal_ccw( tri2a, tri2b, tri2c );
		}
		if( y < h - 1 ) { // top left
			const glm::vec3 tri5a = point( x, y );
			const glm::vec3 tri5b = point( x - 1, y + 1 );
			const glm::vec3 tri5c = point( x - 1, y );

			const glm::vec3 tri6a = point( x, y );
			const glm::vec3 tri6b = point( x, y + 1 );
			const glm::vec3 tri6c = point( x - 1, y + 1 );

			bent_normal += triangle_normal_ccw( tri5a, tri5b, tri5c );
			bent_normal += triangle_normal_ccw( tri6a, tri6b, tri6c );
		}
	}

	if( x < w - 1 ) {
		if( y > 0 ) { // bottom right
			const glm::vec3 tri3a = point( x, y );
			const glm::vec3 tri3b = point( x, y - 1 );
			const glm::vec3 tri3c = point( x + 1, y - 1 );

			const glm::vec3 tri4a = point( x, y );
			const glm::vec3 tri4b = point( x + 1, y - 1 );
			const glm::vec3 tri4c = point( x + 1, y );

			bent_normal += triangle_normal_ccw( tri3a, tri3b, tri3c );
			bent_normal += triangle_normal_ccw( tri4a, tri4b, tri4c );
		}
		if( y < h - 1 ) { // top right
			const glm::vec3 tri1a = point( x, y );
			const glm::vec3 tri1b = point( x + 1, y );
			const glm::vec3 tri1c = point( x, y + 1 );

			bent_normal += triangle_normal_ccw( tri1a, tri1b, tri1c );
		}
	}

	return glm::normalize( bent_normal );
}

float Heightmap::height( const float x, const float y ) const {
	const float ix = floorf( x );
	const float iy = floorf( y );

	return bilinear_interpolation(
		point( ix, iy ),
		point( ix + 1, iy ),
		point( ix, iy + 1 ),
		point( ix + 1, iy + 1 ),
		glm::vec2( x, y )
	);
}

void Heightmap::render( const glm::mat4 & VP ) const {
	if( vbo_verts == 0 ) {
		return;
	}

	const glm::vec3 sun = glm::normalize( glm::vec3( 1, 0, -SLOPE ) );

	glUseProgram( shader );
	glBindVertexArray( vao );

	glUniformMatrix4fv( un_vp, 1, GL_FALSE, glm::value_ptr( VP ) );
	glUniform3fv( un_sun, 1, glm::value_ptr( sun ) );

	glBindBuffer( GL_ARRAY_BUFFER, vbo_verts );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
	glEnableVertexAttribArray( at_pos );
	glVertexAttribPointer( at_pos, 3, GL_FLOAT, GL_FALSE, 0, 0 );

	glBindBuffer( GL_ARRAY_BUFFER, vbo_normals );
	glEnableVertexAttribArray( at_normal );
	glVertexAttribPointer( at_normal, 3, GL_FLOAT, GL_FALSE, 0, 0 );

	glBindBuffer( GL_ARRAY_BUFFER, vbo_lit );
	glEnableVertexAttribArray( at_lit );
	glVertexAttribPointer( at_lit, 1, GL_FLOAT, GL_FALSE, 0, 0 );

	glDrawElements( GL_TRIANGLES, ( w - 1 ) * ( h - 1 ) * 6, GL_UNSIGNED_INT, 0 );
}
