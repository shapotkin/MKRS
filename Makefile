CC     = gcc
CFLAGS = -iquote $(IDIR) -g -Wall -Wconversion -Wextra
IDIR   = inc
SOURCE = src
ODIR   = obj
LDIR   = /usr/local/lib
LIBS   = -lwiringPi -lwiringPiDev -lpthread -lm
BUILDDIR = build
_DEPS  = dp.h 
DEPS   = $(patsubst %,$(IDIR)/%,$(_DEPS))
_OBJ   = dp.o main.o
OBJ    = $(patsubst %,$(ODIR)/%,$(_OBJ))

all: clean prepare build

build: $(BUILDDIR)/demo

$(ODIR)/%.o: $(SOURCE)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/demo: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
	size $(BUILDDIR)/demo

.PHONY: clean prepare

clean:
	rm -f $(ODIR)/*.o $(BUILDDIR)/*~ core $(INCDIR)/*~

prepare:
	mkdir -p $(ODIR)
	mkdir -p $(BUILDDIR)
