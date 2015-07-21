#ifndef _GL_H_
#define _GL_H_

#include <GLFW/glfw3.h>

class GL {
public:
	static GLFWwindow * init();
	static void term();
};

#endif // _GL_H_
