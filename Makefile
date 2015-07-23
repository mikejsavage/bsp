all: bsp

SRCS = bsp.cc bsp_renderer.cc gl.cc
OBJS := $(patsubst %.cc,%.o,$(SRCS))

CXXFLAGS += -std=c++11 -O2 -Wall -Wextra -ggdb3
LDFLAGS += -lm -lGL -lglfw
LDFLAGS += -lGLU

bsp: $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o bsp

clean:
	rm -f bsp $(OBJS)
