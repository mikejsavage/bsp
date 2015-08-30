#ifndef _HEIGHTMAP_H_
#define _HEIGHTMAP_H_

#include <string>

#include "platform_opengl.h"
#include <glm/glm.hpp>

#include "intrinsics.h"

class Heightmap {
public:
	u8 * pixels;
	int w, h;

	GLuint vbo_verts = 0;
	GLuint vbo_normals;
	GLuint vbo_lit;
	GLuint ebo;
	GLuint vao;

	~Heightmap();

	void load( const std::string & image, const int ox, const int oy,
		const GLint at_pos, const GLint at_normal, const GLint at_lit );
	void unload();

	glm::vec3 point( int x, int y ) const;
	glm::vec3 point_normal( int x, int y ) const;

	float height( const float x, const float y ) const;

	void render() const;
};

#endif // _HEIGHTMAP_H_
