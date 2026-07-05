# Variables with default values (can be overridden from command line)
TASK    ?= 01
FILE    ?= test
EXE_PFX = AK

CC      ?= gcc
CFLAGS  += -Wall
LDFLAGS += -lao -lm

# Dynamic Target and Source
# If TASK=04 and FILE=a, TARGET becomes AK04a.exe
TARGET = $(EXE_PFX)$(TASK)$(FILE).exe
SRC    = $(FILE).c
OBJ    = $(SRC:.c=.o)

all: Release

Debug: CFLAGS += -g
Debug: $(TARGET)

Release: CFLAGS += -O2
Release: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f *.o *.exe
