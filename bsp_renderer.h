#ifndef _BSP_RENDERER_H_
#define _BSP_RENDERER_H_

#include <glm/glm.hpp>

#include "bsp.h"

class BSP_Renderer {
private:
	static void pick_color( const BSP & bsp, const BSP_Face & face );
	static void render_leaf( const BSP & bsp, const BSP_Leaf & leaf );

public:
	static void render( const BSP & bsp, const glm::vec3 & camera );
};

#endif // _BSP_RENDERER_H_
