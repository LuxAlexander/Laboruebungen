# Variables
TASK    ?= 03
EXE_PFX = TK

CC      ?= gcc
# CFLAGS  += -Wall
# LDFLAGS += -lao -lm
# pkg-config holt die richtigen Include-Pfade für den Compiler
#CFLAGS  += $(shell pkg-config --cflags ao) -Wall
# pkg-config holt die richtigen Library-Pfade für den Linker
#LDFLAGS += $(shell pkg-config --libs ao) -lm

# PFADE ZUR LOKALEN LIBAV
FFMPEG_DIR = ../libs
FFMPEG_INC = $(FFMPEG_DIR)/include
FFMPEG_LIB = $(FFMPEG_DIR)/lib

# COMPILER-FLAGS ERGÄNZEN
# -I bindet den lokalen Include-Ordner für die Header (<libavutil/avutil.h>) ein
CFLAGS  += -I$(FFMPEG_INC) -Wall

# LINKER-FLAGS ERGÄNZEN
# -L zeigt auf den lokalen Library-Ordner. 
# link statisch die benötigten FFmpeg-Bibliotheken.
# Mathe (-lm), Threads (-lpthread) und zlib (-lz). -lm -lpthread -lz
LDFLAGS += -L$(FFMPEG_LIB) -lavformat -lavcodec -lswresample -lavutil -lpthread -lm

# Automatically find all .c files
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

# Aus a.c wird z.B. AK04a.exe
TARGETS = $(patsubst %.c,$(EXE_PFX)$(TASK)%.exe,$(SRCS))

# Default target
all: Release

# Debug build
Debug: CFLAGS += -g
Debug: $(TARGETS)

# Release build
Release: CFLAGS += -O2
Release: $(TARGETS)

# Rule to build each executable
$(EXE_PFX)$(TASK)%.exe: %.o
	$(CC) -o $@ $< $(LDFLAGS)

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run all programs
run: $(TARGETS)
	@for exe in $(TARGETS); do \
		echo "Running $$exe"; \
		./$$exe; \
	\
	done

# Clean all builds
clean:
	rm -f *.o *.exe
