all: bsp

SRCS = bsp.cc gl.cc
OBJS := $(patsubst %.cc,%.o,$(SRCS))

CXXFLAGS += -std=c++11 -O2 -Wall -Wextra
LDFLAGS += -lm -lGL -lglfw
LDFLAGS += -lGLU

bsp: $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o bsp

clean:
	rm -f bsp $(OBJS)
