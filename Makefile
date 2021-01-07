CC = g++
DFLAGS =
CFLAGS = -Wall -Wextra -pthread -std=c++17 -g -rdynamic -pipe $(DFLAGS) #-fsanitize=address
INCLUDES = -Iinclude -Ilib
LIBPATHS =
LIBS =
LFLAGS =

export LD_LIBRARY_PATH := /usr/local/lib64:/usr/lib/gcc4/x64/release/
export PKG_CONFIG_PATH := /usr/local/lib64/pkgconfig

SRCS = $(wildcard src/*.cpp src/arch/m68k/*.cpp src/arch/m68k/md/*.cpp)

OBJS = $(SRCS:.cpp=.o)

MAIN = gs

.PHONY: clean

all:    $(MAIN)
		@echo  GoldScorpion built successfully.

$(MAIN): $(OBJS)
		$(CC) $(CFLAGS) $(INCLUDES) $(LIBPATHS) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)
.cpp.o:
		$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
		$(RM) *.o *~ $(MAIN)
		find src/ -name "*.o" -type f -delete

run:    ${MAIN}
	./gs
