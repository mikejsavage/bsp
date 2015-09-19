#include "platform_opengl.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "intrinsics.h"
#include "renderer.h"

void renderer_init( Renderer * const renderer ) {
	renderer->num_draw_calls = 0;
}

void renderer_push(
	Renderer * const renderer,
	const GLenum mode, const GLuint vao, const GLuint shader, const bool has_ebo,
	const u32 count, const u32 first, const GLuint texture
) {
	assert( renderer->num_draw_calls < MAX_DRAW_CALLS - 1 );

	DrawCall call = { };
	call.mode = mode;

	call.vao = vao;
	call.shader = shader;
	call.texture = texture;

	call.has_ebo = has_ebo;
	call.first = first;
	call.count = count;

	renderer->draw_calls[ renderer->num_draw_calls ] = call;
	renderer->num_draw_calls++;
}

void renderer_uniform( Renderer * const renderer, const Uniform uniform ) {
	assert( renderer->num_draw_calls > 0 );

	DrawCall * const call = &renderer->draw_calls[ renderer->num_draw_calls - 1 ];
	assert( call->num_uniforms < MAX_UNIFORMS - 1 );

	call->uniforms[ call->num_uniforms ] = uniform;
	call->num_uniforms++;
}

void renderer_uniform_i( Renderer * const renderer, const int i ) {
	Uniform uniform = { };
	uniform.type = UNIFORM_INT;
	uniform.i = i;

	renderer_uniform( renderer, uniform );
}

void renderer_uniform_f( Renderer * const renderer, const float f ) {
	Uniform uniform = { };
	uniform.type = UNIFORM_INT;
	uniform.f = f;

	renderer_uniform( renderer, uniform );
}

void renderer_uniform_v3( Renderer * const renderer, const glm::vec3 v3 ) {
	Uniform uniform = { };
	uniform.type = UNIFORM_INT;
	uniform.v3 = v3;

	renderer_uniform( renderer, uniform );
}

void renderer_uniform_m4( Renderer * const renderer, const glm::mat4 m4 ) {
	Uniform uniform = { };
	uniform.type = UNIFORM_INT;
	uniform.m4 = m4;

	renderer_uniform( renderer, uniform );
}

void renderer_render( const Renderer * const renderer ) {
	// TODO: sort draw calls
	for( u32 i = 0; i < renderer->num_draw_calls; i++ ) {
		const DrawCall * call = &renderer->draw_calls[ i ];

		glUseProgram( call->shader );
		glBindVertexArray( call->vao );

		for( u8 u = 0; u < call->num_uniforms; u++ ) {
			const Uniform * uniform = &call->uniforms[ i ];

			switch( uniform->type ) {
				case UNIFORM_INT: {
					glUniform1i( uniform->location, uniform->i );
				} break;

				case UNIFORM_FLOAT: {
					glUniform1f( uniform->location, uniform->f );
				} break;

				case UNIFORM_VEC3: {
					glUniform3fv( uniform->location, 1, glm::value_ptr( uniform->v3 ) );
				} break;

				case UNIFORM_MAT4: {
					glUniformMatrix4fv( uniform->location, 1, GL_FALSE, glm::value_ptr( uniform->m4 ) );
				} break;
			}
		}

		if( call->has_ebo ) {
			glDrawElements( call->mode, call->count, GL_UNSIGNED_INT, 0 );
		}
		else {
			glDrawArrays( call->mode, call->first, call->count );
		}

		// TODO: remove these if we aren't debugging
		glBindVertexArray( 0 );
		glUseProgram( 0 );
		glterrible();
	}
}
