CC = g++
DFLAGS =
CFLAGS = -Wall -Wextra -pthread -std=c11 -g -rdynamic -pipe $(DFLAGS) #-fsanitize=address
INCLUDES = -Iinclude
LIBPATHS =
LIBS =
LFLAGS =

export LD_LIBRARY_PATH := /usr/local/lib64:/usr/lib/gcc4/x64/release/
export PKG_CONFIG_PATH := /usr/local/lib64/pkgconfig

SRCS = $(wildcard src/*.cpp)

OBJS = $(SRCS:.cpp=.o)

MAIN = gs

.PHONY: clean

all:    $(MAIN)
		@echo  GoldScorpion built successfully.

$(MAIN): $(OBJS)
		$(CC) $(CFLAGS) $(INCLUDES) $(LIBPATHS) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)
.c.o:
		$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
		$(RM) *.o *~ $(MAIN)
		find src/ -name "*.o" -type f -delete

run:    ${MAIN}
	./gs
