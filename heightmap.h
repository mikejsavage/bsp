#ifndef _HEIGHTMAP_H_
#define _HEIGHTMAP_H_

#include <string>

#include "opengl.h"
#include <glm/glm.hpp>

#include "intrinsics.h"

class Heightmap {
private:
	u8 * pixels;
	int w, h;

	GLuint vbo_verts = 0;
	GLuint vbo_normals;
	GLuint vbo_lit;
	GLuint ebo;
	GLuint vao;

	GLuint shader;

	GLint at_pos;
	GLint at_normal;
	GLint at_lit;

	GLint un_vp;
	GLint un_sun;

public:
	~Heightmap();

	void init();

	void load( const std::string & image, const int ox, const int oy );
	void unload();

	glm::vec3 point( int x, int y ) const;
	glm::vec3 point_normal( int x, int y ) const;

	float height( const float x, const float y ) const;

	void render( const glm::mat4 & VP ) const;
};

#endif // _HEIGHTMAP_H_
