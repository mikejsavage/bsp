all: bsp hm pp

BSPSRCS = bsp.cc bsp_renderer.cc gl.cc
BSPOBJS := $(patsubst %.cc,%.o,$(BSPSRCS))

HMSRCS = hm.cc gl.cc heightmap.cc stb_image.cc stb_perlin.cc
HMOBJS := $(patsubst %.cc,%.o,$(HMSRCS))

PPSRCS = pp.cc stb_image.cc stb_image_write.cc
PPOBJS := $(patsubst %.cc,%.o,$(PPSRCS))

WARNINGS = -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-write-strings
CXXFLAGS += -std=c++11 -O2 $(WARNINGS) -ggdb3 -DGL_GLEXT_PROTOTYPES
LDFLAGS += -lm -lGL -lGLEW -lglfw
LDFLAGS += -lGLU

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
