all: hm pp

BSPSRCS = bsp.cc bsp_renderer.cc gl.cc
BSPOBJS := $(patsubst %.cc,%.o,$(BSPSRCS))

HMSRCS = hm.cc gl.cc heightmap.cc terrain_manager.cc stb_image.cc stb_perlin.cc
HMOBJS := $(patsubst %.cc,%.o,$(HMSRCS))

PPSRCS = pp.cc stb_image.cc stb_image_write.cc
PPOBJS := $(patsubst %.cc,%.o,$(PPSRCS))

WARNINGS = -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-write-strings
CXXFLAGS += -std=c++11 -O2 $(WARNINGS) -ggdb3 -DGL_GLEXT_PROTOTYPES

# OS detection
uname := $(shell uname -s)

ifneq ($(uname),Darwin)
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

hm: $(HMOBJS)
	$(CXX) $^ $(LDFLAGS) -o $@

bsp: $(BSPOBJS)
	$(CXX) $^ $(LDFLAGS) -o $@

pp: $(PPOBJS)
	$(CXX) $^ $(LDFLAGS) -o $@

clean:
	rm -f bsp hm $(BSPOBJS) $(HMOBJS) $(PPOBJS)

stb_image.o: stb_image.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^ -DSTB_IMAGE_IMPLEMENTATION

stb_image_write.o: stb_image_write.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^ -DSTB_IMAGE_WRITE_IMPLEMENTATION

stb_perlin.o: stb_perlin.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^ -DSTB_PERLIN_IMPLEMENTATION
