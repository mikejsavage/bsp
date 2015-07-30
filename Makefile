all: bsp hm

BSPSRCS = bsp.cc bsp_renderer.cc gl.cc
BSPOBJS := $(patsubst %.cc,%.o,$(BSPSRCS))

HMSRCS = hm.cc gl.cc stb_image.cc stb_perlin.cc
HMOBJS := $(patsubst %.cc,%.o,$(HMSRCS))

CXXFLAGS += -std=c++11 -O2 -Wall -Wextra -ggdb3
LDFLAGS += -lm -lGL -lglfw
LDFLAGS += -lGLU

hm: $(HMOBJS)
	$(CXX) $(HMOBJS) $(LDFLAGS) -o hm

bsp: $(BSPOBJS)
	$(CXX) $(BSPOBJS) $(LDFLAGS) -o bsp

clean:
	rm -f bsp hm $(BSPOBJS) $(HMOBJS)

stb_image.o: stb_image.cc
	$(CXX) $(CXXFLAGS) -c -o stb_image.o stb_image.cc -DSTB_IMAGE_IMPLEMENTATION

stb_perlin.o: stb_perlin.cc
	$(CXX) $(CXXFLAGS) -c -o stb_perlin.o stb_perlin.cc -DSTB_PERLIN_IMPLEMENTATION
