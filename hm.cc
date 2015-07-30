#include <iostream>
#include <fstream>
#include <math.h>
#include <err.h>

#include <GL/glu.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "int.h"
#include "gl.h"
#include "stb_easy_font.h"
#include "stb_image.h"
#include "stb_perlin.h"

const float EPSILON = 1.0 / 32.0;
const float SLOPE = 0.3;

#define TEXT( x, y, form, ... ) \
	do { \
		static char buffer[ 99999 ]; \
		static char text[ 2048 ]; \
		sprintf( text, form, __VA_ARGS__ ); \
		const int num_quads = stb_easy_font_print( x, y, text, nullptr, buffer, sizeof( buffer ) ); \
		glColor3f(1,1,1); \
		glVertexPointer(2, GL_FLOAT, 16, buffer); \
		glDrawArrays(GL_QUADS, 0, num_quads*4); \
	} while( 0 )

void glterrible() {
	printf( "glterrible\n" );
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

// TODO: bit twiddling?
bool same_sign( const float a, const float b ) {
	return a * b >= 0;
}

float point_plane_distance( const glm::vec3 & point, const glm::vec3 & normal, float d ) {
	return glm::dot( point, normal ) - d;
}

glm::vec3 d2r( const glm::vec3 & degrees ) {
	return degrees * static_cast< float >( M_PI / 180 );
}

glm::vec3 angles_to_vector( const glm::vec3 & angles ) {
	return glm::vec3(
		-sin( angles.y ) * sin( angles.x ),
		-cos( angles.y ) * sin( angles.x ),
		-cos( angles.x )
	);
}

glm::vec3 angles_to_vector_xy( const glm::vec3 & angles ) {
	return glm::vec3( sin( angles.y ), cos( angles.y ), 0 );
}

void print_vec3( const std::string & name, const glm::vec3 & v ) {
	printf( "%s: %.3f %.3f %.3f\n", name.c_str(), v.x, v.y, v.z );
}

glm::vec3 triangle_normal( const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c ) {
	return glm::normalize( glm::cross( b - a, c - a ) );
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

class Heightmap {
public:
	u8 * pixels;
	int w, h;

	float * lit;

	Heightmap( const std::string image ) {
		pixels = stbi_load( image.c_str(), &w, &h, nullptr, 1 );

		printf( "begin lit calculations\n" );
		lit = new float[ w * h ];
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
		printf( "end lit calculations\n" );
	}

	~Heightmap() {
		stbi_image_free( pixels );

		delete lit;
	}

	glm::vec3 point( int x, int y ) const {
		return glm::vec3( x, y, pixels[ y * w + x ] / 8.0f );
	}

	float height( const float x, const float y ) const {
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

	void render() const {
		const glm::vec3 sun = glm::normalize( glm::vec3( 1, 0, -SLOPE ) );
		const glm::vec3 up = glm::vec3( 0, 0, 1 );

		glBegin( GL_TRIANGLES );
		for( int y = 0; y < h - 1; y++ ) {
			for( int x = 0; x < w - 1; x++ ) {
				const float noise = fabsf( stb_perlin_noise3( ( float ) x / w, ( float ) y / h, 0 ) ) / 10;

				const int tri1[ 3 ][ 2 ] = { { x, y }, { x, y + 1 }, { x + 1, y } };
				const int tri2[ 3 ][ 2 ] = { { x + 1, y + 1 }, { x + 1, y }, { x, y + 1 } };

				glm::vec3 points1[ 3 ];
				glm::vec3 points2[ 3 ];
				for( int i = 0; i < 3; i++ ) {
					points1[ i ] = point( tri1[ i ][ 0 ], tri1[ i ][ 1 ] );
					points2[ i ] = point( tri2[ i ][ 0 ], tri2[ i ][ 1 ] );
				}

				const glm::vec3 normal1 = triangle_normal( points1[ 0 ], points1[ 1 ], points1[ 2 ] );
				const glm::vec3 normal2 = triangle_normal( points2[ 0 ], points2[ 1 ], points2[ 2 ] );

				const glm::vec3 tex1 = fabsf( glm::dot( normal1, up ) ) + noise > 0.9 ? glm::vec3( 0.2, 0.6, 0.2 ) : glm::vec3( 0.5, 0.5, 0.3 );
				const glm::vec3 tex2 = fabsf( glm::dot( normal2, up ) ) + noise > 0.9 ? glm::vec3( 0.2, 0.6, 0.2 ) : glm::vec3( 0.5, 0.5, 0.3 );

				for( int i = 0; i < 3; i++ ) {
					const bool islit = lit[ tri1[ i ][ 1 ] * w + tri1[ i ][ 0 ] ] == -1;
					const float light = 0.2 + ( islit ? glm::dot( normal1, sun ) : 0 );

					glColor3fv( &( tex1 * light )[ 0 ] );
					glVertex3fv( &points1[ i ][ 0 ] );
				}

				for( int i = 0; i < 3; i++ ) {
					const bool islit = lit[ tri2[ i ][ 1 ] * w + tri2[ i ][ 0 ] ] == -1;
					const float light = 0.2 + ( islit ? glm::dot( normal2, sun ) : 0 );

					glColor3fv( &( tex2 * light )[ 0 ] );
					glVertex3fv( &points2[ i ][ 0 ] );
				}
			}
		}
		glEnd();
	}
};

int main( int argc, char ** argv ) {
	Heightmap hm( argc == 2 ? argv[ 1 ] : "mountains512.png" );
	GLFWwindow * const window = GL::init();

	glm::vec3 pos( 0, 0, 32 );
	glm::vec3 angles = d2r( glm::vec3( -90, 45, 0 ) );

	float lastFrame = glfwGetTime();

	gluPerspective( 120.0f, 800.0f / 600.0f, 0.1f, 10000.0f );
	glClearColor( 0, 0.5, 0.7, 1 );

	float z = 50;

	while( !glfwWindowShouldClose( window ) ) {
		const float now = glfwGetTime();
		const float dt = now - lastFrame;

		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		const int fb = glfwGetKey( window, 'W' ) - glfwGetKey( window, 'S' );
		const int lr = glfwGetKey( window, 'A' ) - glfwGetKey( window, 'D' );
		const int dz = glfwGetKey( window, GLFW_KEY_SPACE ) - glfwGetKey( window, GLFW_KEY_LEFT_SHIFT );

		const int pitch = glfwGetKey( window, GLFW_KEY_UP ) - glfwGetKey( window, GLFW_KEY_DOWN );
		const int yaw = glfwGetKey( window, GLFW_KEY_RIGHT ) - glfwGetKey( window, GLFW_KEY_LEFT );

		angles.x += pitch * dt * 2;
		angles.y += yaw * dt * 2;

		pos += angles_to_vector_xy( angles ) * 50.0f * dt * ( float ) fb;
		const glm::vec3 sideways = glm::vec3( -cosf( angles.y ), sinf( angles.y ), 0 );
		pos += sideways * 50.0f * dt * ( float ) lr;
		// pos.z = hm.height( pos.x, pos.y ) + 8;
		z += dz * 10.0f * dt;
		pos.z = z;

		// TODO: do matrices like a big boy
		glPushMatrix();

		glRotatef( angles.x * 180 / M_PI, 1.0, 0.0, 0.0 );
		glRotatef( angles.y * 180 / M_PI, 0.0, 0.0, 1.0 );
		glTranslatef( -pos.x, -pos.y, -pos.z );

		hm.render();

		glLoadIdentity();

		glBegin( GL_TRIANGLE_STRIP );
		glColor3f( 0.2, 0.2, 0.2 );
		glVertex2f( -1, 1 );
		glVertex2f( 1, 1 );
		glVertex2f( -1, 0.95 );
		glVertex2f( 1, 0.95 );
		glEnd();

		glOrtho( 0, 640, 480, 0, -1, 1 );
		TEXT( 2, 2, "%d", ( int ) ( 1 / dt ) );

		glPopMatrix();

		glfwSwapBuffers( window );
		glfwPollEvents();

		lastFrame = now;
	}

	GL::term();

	return 0;
}
