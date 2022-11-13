.SUFFIXES:
.DEFAULT: all
.PHONY: all run clean gdb valgrind

BIN = bin
SRC = src
EXEC = ./app

CXX = g++
CXXWARN += -Wall -Wextra -Wshadow -Wattributes -Wstrict-aliasing
CXXFLAGS += -O2 -g -fno-exceptions $(CXXWARN) --std=c++20 -I$(SRC) -fmax-errors=1

LD = $(CXX)
LDFLAGS = $(LIBS)
LIBS := -lSDL2 -lGL -lGLEW

RM = rm -rf
MKDIR = mkdir -p
GDB = gdb
VALGRIND = valgrind

FILES-CXX = $(shell find $(SRC) -type f -name "*.cpp" -o -name "*.cc")
FILES-OBJ = $(FILES-CXX:$(SRC)/%=$(BIN)/%.o)
FILES-DEP = $(FILES-CXX:$(SRC)/%=$(BIN)/%.d)

all: $(EXEC)
	@echo "  >> $(EXEC) is up to date"

$(EXEC): $(FILES-OBJ)
	@echo "  >> Linking $@"
	$(LD) $(LDFLAGS) $^ -o $@

$(BIN)/%.o: $(SRC)/%
	$(MKDIR) "$(@D)"
	$(CXX) $(CXXFLAGS) -c "$<" -o "$@" -MD

-include $(FILES-DEP)

run: all
	$(EXEC) $(APP-FLAGS)

clean:
	$(RM) $(BIN) $(EXEC)

gdb: all
	$(GDB) $(EXEC)

valgrind: all
	$(VALGRIND) $(EXEC) --leak-check=full
