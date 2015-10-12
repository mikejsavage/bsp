all: medfall bsp.so hm.so pp btt.so

MAINOBJS = main.o gl.o memory_arena.o

COMMONOBJS = memory_arena.o work_queue.o immediate.o stb_truetype.o stb_image.o
BSPOBJS = bsp.o bsp_renderer.o
HMOBJS = hm.o heightmap.o terrain_manager.o stb_perlin.o
BTTOBJS = btt.o heightmap.o gpubtt.o

PPOBJS = pp.o stb_image.o stb_image_write.o

WARNINGS = -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-write-strings -Wno-char-subscripts
CXXFLAGS += -std=c++11 -O2 -pthread -ggdb3 $(WARNINGS) -DGL_GLEXT_PROTOTYPES -DGLFW_INCLUDE_NONE
LDFLAGS += -lm

# OS detection
ifneq ($(shell uname -s),Darwin)
	CXXFLAGS += -fPIC
	LDFLAGS += -lGL -lGLEW -lglfw -ldl
else
	LDFLAGS += -framework OpenGL -lglfw3
endif

picky: WARNINGS += -Wunused-parameter -Wunused-function -Wwrite-strings -Wconversion
picky: all

medfall: $(MAINOBJS)
	$(CXX) $^ $(LDFLAGS) -o $@

hm.so: $(COMMONOBJS) $(HMOBJS)
	$(CXX) $^ $(LDFLAGS) -o $@ -shared

bsp.so: $(COMMONOBJS) $(BSPOBJS)
	$(CXX) $^ $(LDFLAGS) -o $@ -shared

btt.so: $(COMMONOBJS) $(BTTOBJS)
	$(CXX) $^ $(LDFLAGS) -o $@ -shared

pp: $(PPOBJS)
	$(CXX) $^ $(LDFLAGS) -o $@

clean:
	rm -f medfall bsp.so hm.so pp *.o

stb_image.o: stb_image.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^ -DSTB_IMAGE_IMPLEMENTATION

stb_image_write.o: stb_image_write.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^ -DSTB_IMAGE_WRITE_IMPLEMENTATION

stb_perlin.o: stb_perlin.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^ -DSTB_PERLIN_IMPLEMENTATION

stb_truetype.o: stb_truetype.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^ -DSTB_TRUETYPE_IMPLEMENTATION
