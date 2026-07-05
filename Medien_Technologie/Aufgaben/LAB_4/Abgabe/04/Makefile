# Variables
TASK    ?= 04
EXE_PFX = MTM

CC      ?= gcc

# CURDIR liefert den absoluten Pfad des aktuellen Makefile-Verzeichnisses auf dem Server
ifneq ($(wildcard $(CURDIR)/libs/),)
    LOCAL_LIBS_DIR = $(CURDIR)/libs
else
    LOCAL_LIBS_DIR = $(CURDIR)/../libs
endif

# WICHTIG: Wir erlauben pkg-config, auch in den Systempfaden des FH-Servers zu suchen!
PKG_CONFIG_PATH := $(LOCAL_LIBS_DIR)/lib/pkgconfig:$(PKG_CONFIG_PATH):/usr/lib/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig

export PKG_CONFIG_PATH

# CFLAGS: Wir binden deine lokalen Header ein
CFLAGS  += -I$(LOCAL_LIBS_DIR)/include -Wall -DX264_API_IMPORTS -O2

# LDFLAGS: Wir zwingen den Linker, zuerst lokal nach x264 zu suchen (-L und -lx264).
# Danach holen wir uns libavformat, libavcodec und libavutil flexibel über pkg-config vom Server!
LDFLAGS += -L$(LOCAL_LIBS_DIR)/lib -lx264 $(shell pkg-config --libs libavformat libavcodec libavutil 2>/dev/null || echo "-lavformat -lavcodec -lavutil") -lm -lpthread

# Standard-Suchpfade für alle Fälle
LDFLAGS += -Wl,-rpath,'$$ORIGIN/../libs/lib':'$$ORIGIN/libs/lib'

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
