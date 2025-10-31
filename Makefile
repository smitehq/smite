# Compiler and flags
CXX := clang++
MYSYS2_PATH := C:/msys64/mingw64
CXXFLAGS := -g -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -DYAML_CPP_STATIC_DEFINE -static -I. -I$(MYSYS2_PATH)/include
LDFLAGS := -L$(MYSYS2_PATH)/lib -lyaml-cpp -lfmt -mconsole -lstdc++fs -static

# MSYS2 tools path (for Unix commands like mkdir/rm)
MSYS2_TOOLS := C:/msys64/usr/bin

# Use full path for mkdir
MKDIR := $(MSYS2_TOOLS)/mkdir

# Use full path for rm (to avoid clean failing later)
RM := $(MSYS2_TOOLS)/rm

# Source and object files
SRCS := $(wildcard *.cpp) $(wildcard core/*.cpp) $(wildcard modules/**/*.cpp)

# Object files go into build/
OBJS = $(patsubst %.cpp,build/%.o,$(SRCS))

# Executable
TARGET = smite.exe

# Ensure build directory exists
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Compile each .cpp into build/*.o
build/%.o: %.cpp
	$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build folder
clean:
	$(RM) -rf build $(TARGET)