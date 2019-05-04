# Compile settings
CC=gcc
CFLAGS=-Wall -std=c11
LFLAGS=

# Sources
SRC:=src
OBJ:=obj
BIN:=bin

SOURCES=$(wildcard $(SRC)/*.c)
HEADERS=$(wildcard $(SRC)/*.h)
OBJECTS=$(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SOURCES))

BINARY=$(BIN)/echoloop

# Specific variables
debug:		CFLAGS +=-DDEBUG -g
release:    CFLAGS +=-O2

all: debug

debug release:  $(BINARY)

$(BINARY): $(OBJECTS) 
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

$(OBJ)/%.o: $(SRC)/%.c
	mkdir -p $(BIN) $(OBJ)
	$(CC) $(CFLAGS) -c $^ -o $@


# Additional targets
.PHONY: clean

clean:
	rm -rf $(BIN)
	rm -rf $(OBJ)



