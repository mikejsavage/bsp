all: medfall hm.so pp

OBJS = main.o gl.o work_queue.o
BSPOBJS = bsp.o bsp_renderer.o gl.o
HMOBJS = hm.o heightmap.o terrain_manager.o work_queue.o stb_image.o stb_perlin.o
PPOBJS = pp.o stb_image.o stb_image_write.o

WARNINGS = -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-write-strings
CXXFLAGS += -std=c++11 -O2 $(WARNINGS) -ggdb3 -DGL_GLEXT_PROTOTYPES

# OS detection
ifneq ($(shell uname -s),Darwin)
	LDFLAGS += -lGL -lGLEW -lglfw
	LDFLAGS += -lGLU
else
	# 8)
	CXXFLAGS += -I/usr/local/Cellar/glfw3/3.1.1/include
	CXXFLAGS += -I/usr/local/Cellar/glm/0.9.6.3/include
	CXXFLAGS += -I/usr/local/Cellar/glew/1.12.0/include
	LDFLAGS += -lm -framework Cocoa -framework OpenGL -framework IOKit -framework CoreFoundation -framework CoreVideo
	LDFLAGS += -L/usr/local/Cellar/glfw3/3.1.1/lib -lglfw3
	LDFLAGS += -L/usr/local/Cellar/glew/1.12.0/lib -lGLEW
endif

picky: WARNINGS += -Wunused-parameter -Wunused-function -Wwrite-strings
picky: all

medfall: $(OBJS)
	$(CXX) $^ $(LDFLAGS) -o $@

hm.so: $(HMOBJS)
	$(CXX) $^ $(LDFLAGS) -o $@ -shared

bsp.so: $(BSPOBJS)
	$(CXX) $^ $(LDFLAGS) -o $@ -shared

pp: $(PPOBJS)
	$(CXX) $^ $(LDFLAGS) -o $@

clean:
	rm -f medfall bsp.so hm.so $(OBJS) $(BSPOBJS) $(HMOBJS) $(PPOBJS)

stb_image.o: stb_image.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^ -DSTB_IMAGE_IMPLEMENTATION

stb_image_write.o: stb_image_write.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^ -DSTB_IMAGE_WRITE_IMPLEMENTATION

stb_perlin.o: stb_perlin.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^ -DSTB_PERLIN_IMPLEMENTATION
