.SUFFIXES:
.DEFAULT: all
.PHONY: all run clean header-check

BIN = bin
SRC = src
EXEC = ./app

CXX = g++
CXXWARN += -Wall -Wextra -Wshadow -Wattributes -Wstrict-aliasing
CXXFLAGS += -O2 -g -fno-exceptions -fno-rtti $(CXXWARN) --std=c++20 -I$(SRC) -fmax-errors=1

LD = $(CXX)
LDFLAGS = $(LIBS)
LIBS := -lSDL2 -lGL -lGLEW

RM = rm -rf
MKDIR = mkdir -p
GDB = gdb
VALGRIND = valgrind

FILES-CXX = $(shell find $(SRC) -type f -name "*.cpp" -o -name "*.cc")
FILES-OBJ = $(FILES-CXX:$(SRC)/%=$(BIN)/%.o)
FILES-DEP = $(FILES-OBJ:%.o=%.d)

all: $(EXEC)
	@echo "  >> $(EXEC) is up to date"

$(EXEC): $(FILES-OBJ)
	$(LD) $(LDFLAGS) $^ -o $@
	@echo "  >> $@ linked"

$(BIN)/%.o: $(SRC)/%
	$(MKDIR) "$(@D)"
	$(CXX) $(CXXFLAGS) -c "$<" -o "$@" -MD
	@echo "  >> $@ compiled"

-include $(FILES-DEP)

run: all
	$(EXEC) $(APP-FLAGS)

clean:
	$(RM) $(BIN) $(EXEC)


# header-check: Verify that each header file compiles on its own
# (Not much help against missing includes for things used by template definitions, since
#  those do not get instantiated, but better than nothing.)

HEADER-CHECK-TARGETS :=
define HEADER-CHECK
HEADER-CHECK-TARGETS += header-check-$(1)
header-check-$(1): $(1)
	$(CXX) $(CXXFLAGS) -c $$^ -o /dev/null
endef
FILES-HEADER = $(shell find $(SRC) -type f -name "*.hpp")
$(foreach HEADER,$(FILES-HEADER),$(eval $(call HEADER-CHECK,$(HEADER))))
.PHONY: $(HEADER-CHECK-TARGETS)
header-check: $(HEADER-CHECK-TARGETS)