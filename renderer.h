#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "platform_opengl.h"
#include <glm/glm.hpp>

#include "intrinsics.h"

#define MAX_DRAW_CALLS 65536
#define MAX_UNIFORMS 8

enum UniformType {
	UNIFORM_INT,
	UNIFORM_FLOAT,
	UNIFORM_VEC3,
	UNIFORM_MAT4,
};

struct Uniform {
	UniformType type;
	GLint location;
	union {
		GLint i;
		GLfloat f;
		glm::vec3 v3;
		glm::mat4 m4;
	};
};

// TODO: badly packed
struct DrawCall {
	GLenum mode;

	GLuint vao;
	GLuint shader;
	GLuint texture;

	u8 num_uniforms;
	Uniform uniforms[ MAX_UNIFORMS ];

	bool has_ebo;
	u32 first;
	u32 count;
};

struct Renderer {
	u32 num_draw_calls;
	DrawCall draw_calls[ MAX_DRAW_CALLS ];
};

void renderer_init( Renderer * const renderer );

void renderer_push( Renderer * const renderer,
	const GLenum mode, const GLuint vao, const GLuint shader, const bool has_ebo,
	const u32 count, const u32 first = 0,
	const GLuint texture = 0
);

void renderer_uniform( Renderer * const renderer, const Uniform uniform );
void renderer_uniform_i( Renderer * const renderer, const int i );
void renderer_uniform_f( Renderer * const renderer, const float f );
void renderer_uniform_v3( Renderer * const renderer, const glm::vec3 v3 );
void renderer_uniform_m4( Renderer * const renderer, const glm::mat4 m4 );

void renderer_render( const Renderer * const renderer );

#endif // _RENDERER_H_
