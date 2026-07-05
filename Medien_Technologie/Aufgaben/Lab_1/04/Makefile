# Variables
TASK    ?= 04
EXE_PFX = AK

CC      ?= gcc
# CFLAGS  += -Wall
# LDFLAGS += -lao -lm
# pkg-config holt die richtigen Include-Pfade für den Compiler
CFLAGS  += $(shell pkg-config --cflags ao) -Wall
# pkg-config holt die richtigen Library-Pfade für den Linker
LDFLAGS += $(shell pkg-config --libs ao) -lm

# Automatically find all .c files
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

# Aus a.c wird AK04a.exe
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
