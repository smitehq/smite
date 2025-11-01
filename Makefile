# Compiler and flags
CXX := clang++
MYSYS2_PATH := C:/msys64/mingw64
CXXFLAGS := -g -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -DYAML_CPP_STATIC_DEFINE -static -I. -I src -I$(MYSYS2_PATH)/include
LDFLAGS := -L$(MYSYS2_PATH)/lib -lyaml-cpp -lfmt -mconsole -lstdc++fs -static

# MSYS2 tools path (for Unix commands like mkdir/rm)
MSYS2_TOOLS := C:/msys64/usr/bin
MKDIR := $(MSYS2_TOOLS)/mkdir
RM := $(MSYS2_TOOLS)/rm

# Source files
SRC_SRCS := $(wildcard src/*.cpp) $(wildcard src/core/*.cpp) $(wildcard src/modules/**/*.cpp)
TEST_SRCS := $(wildcard test/*.cpp)

# Object files (all under build/)
SRC_OBJS := $(SRC_SRCS:src/%.cpp=build/src/%.o)
TEST_OBJS := $(TEST_SRCS:test/%.cpp=build/test/%.o)

# Executable
TARGET = smite.exe

# Core build
$(TARGET): $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

build/src/%.o: src/%.cpp
	$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	$(RM) -rf build $(TARGET)

# Test (Catch2 header-only)
test: $(TEST_OBJS) $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) $(SRC_OBJS) $(TEST_OBJS) -o test.exe $(LDFLAGS)
	./test.exe

build/test/%.o: test/%.cpp
	$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I src -c $< -o $@

.PHONY: clean test