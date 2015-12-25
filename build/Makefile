INCLUDEDIR =../include
CC=g++-4.9
CFLAGS=-I$(INCLUDEDIR) -std=gnu++11 -O0 -g `sdl-config --cflags`
OBJDIR=obj
LDIR =../lib
LIBS=-lSDL_image `sdl-config --libs`

_DEPS = fileobserver.hpp
DEPS = $(patsubst %,$(INCLUDEDIR)/%,$(_DEPS))

_OBJ = fileobserver.o spriteview.o 
OBJ = $(patsubst %,$(OBJDIR)/%,$(_OBJ))

$(OBJDIR)/%.o: ../%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

spriteview: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJDIR)/*.o ./spriteview

