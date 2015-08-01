#ifndef _HEIGHTMAP_H_
#define _HEIGHTMAP_H_

#include <GL/gl.h>
#include <glm/glm.hpp>

#include "int.h"

class Heightmap {
private:
	u8 * pixels;
	int w, h;

	float * lit;

	GLuint vbo = 0;
	GLuint vbo_normals;
	GLuint vbo_lit;
	GLuint vao;
	GLuint ebo;

	GLuint shader;
	GLint at_pos;
	GLint at_normal;
	GLint at_lit;
	GLint un_vp;
	GLint un_sun;

public:
	void init( const std::string image );

	~Heightmap();

	glm::vec3 point( int x, int y ) const;

	glm::vec3 point_normal( int x, int y ) const;

	float height( const float x, const float y ) const;

	void render( const glm::mat4 & VP ) const;
};

#endif // _HEIGHTMAP_H_
