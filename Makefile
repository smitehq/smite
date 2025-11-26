# Detect OS
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)

# Compiler
CXX := clang++

# Version
VERSION := 0.1.0

# Platform-specific settings
ifeq ($(findstring Linux,$(UNAME_S)),Linux)
    # Linux
    TARGET := smite
    PLATFORM := linux-amd64
    INCLUDE_PATHS := -I. -I src -I/usr/include
    LIB_PATHS := -L/usr/lib -L/usr/lib/x86_64-linux-gnu
    LIBS := -lyaml-cpp -lfmt -lreadline -lncurses -lstdc++fs
    CXXFLAGS := -g -std=c++20 -DYAML_CPP_STATIC_DEFINE -I. -I src $(INCLUDE_PATHS)
    LDFLAGS := $(LIB_PATHS) $(LIBS)
    MKDIR := mkdir
    RM := rm
    TAR := tar
    EXE_SUFFIX :=
else ifeq ($(findstring Darwin,$(UNAME_S)),Darwin)
    # macOS
    TARGET := smite
    PLATFORM := macos-amd64
    INCLUDE_PATHS := -I. -I src -I/usr/local/include -I/opt/homebrew/include
    LIB_PATHS := -L/usr/local/lib -L/opt/homebrew/lib
    LIBS := -lyaml-cpp -lfmt -lreadline -lncurses
    CXXFLAGS := -g -std=c++20 -DYAML_CPP_STATIC_DEFINE -I. -I src $(INCLUDE_PATHS)
    LDFLAGS := $(LIB_PATHS) $(LIBS)
    MKDIR := mkdir
    RM := rm
    TAR := tar
    EXE_SUFFIX :=
else
    # Windows (MSYS2/MinGW)
    TARGET := smite.exe
    PLATFORM := windows-amd64
    MSYS2_PATH := C:/msys64/mingw64
    MSYS2_TOOLS := C:/msys64/usr/bin
    INCLUDE_PATHS := -I. -I src -I$(MSYS2_PATH)/include
    LIB_PATHS := -L$(MSYS2_PATH)/lib
    LIBS := -lyaml-cpp -lfmt -lreadline -lncurses -lstdc++fs
    CXXFLAGS := -g -std=c++20 -finput-charset=UTF-8 -fexec-charset=UTF-8 -DYAML_CPP_STATIC_DEFINE -DNCURSES_STATIC -static $(INCLUDE_PATHS)
    LDFLAGS := $(LIB_PATHS) $(LIBS) -mconsole -static
    MKDIR := $(MSYS2_TOOLS)/mkdir
    RM := $(MSYS2_TOOLS)/rm
    TAR := $(MSYS2_TOOLS)/tar
    EXE_SUFFIX := .exe
    WINDRES := windres
    RESOURCE_OBJ := build/resources/smite_res.o
endif

# Source files
SRC_SRCS := $(wildcard src/*.cpp) \
            $(wildcard src/core/*.cpp) \
            $(wildcard src/shell/*.cpp) \
            $(wildcard src/state/*.cpp) \
            $(wildcard src/modules/**/*.cpp)

TEST_SRCS := $(wildcard tests/*.cpp)

# Object files (all under build/)
SRC_OBJS := $(SRC_SRCS:src/%.cpp=build/src/%.o)
TEST_OBJS := $(TEST_SRCS:tests/%.cpp=build/tests/%.o)

# Core build
ifneq ($(findstring MINGW,$(UNAME_S))$(findstring Windows,$(UNAME_S)),)
$(TARGET): $(SRC_OBJS) $(RESOURCE_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
else
$(TARGET): $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
endif

# Windows resource compilation
build/resources/smite_res.o: resources/smite.rc resources/smite.ico
	@$(MKDIR) -p $(dir $@)
	cd resources && $(WINDRES) smite.rc -O coff -o ../build/resources/smite_res.o

build/src/%.o: src/%.cpp
	@$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	$(RM) -rf build dist $(TARGET)

# Run all tests
tests: test-k8s-quest test-shell test-quest-browser test-k8s-spec test-k8s-black-friday test-telemetry test-settings

# K8s Quest Test
test-k8s-quest: build/tests/test_k8s_quest.o $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) build/tests/test_k8s_quest.o $(filter-out build/src/smite.o,$(SRC_OBJS)) -o build/tests/test_k8s_quest$(EXE_SUFFIX) $(LDFLAGS)
	./build/tests/test_k8s_quest$(EXE_SUFFIX)

# Shell module test
test-shell: build/tests/test_shell.o $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) build/tests/test_shell.o $(filter-out build/src/smite.o,$(SRC_OBJS)) -o build/tests/test_shell$(EXE_SUFFIX) $(LDFLAGS)
	./build/tests/test_shell$(EXE_SUFFIX)

# Quest Browser Test
test-quest-browser: build/tests/test_quest_browser.o $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) build/tests/test_quest_browser.o $(filter-out build/src/smite.o,$(SRC_OBJS)) -o build/tests/test_quest_browser$(EXE_SUFFIX) $(LDFLAGS)
	./build/tests/test_quest_browser$(EXE_SUFFIX)

# K8s Spec Tracking Quests Test
test-k8s-spec: build/tests/test_k8s_spec_quests.o $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) build/tests/test_k8s_spec_quests.o $(filter-out build/src/smite.o,$(SRC_OBJS)) -o build/tests/test_k8s_spec_quests$(EXE_SUFFIX) $(LDFLAGS)
	./build/tests/test_k8s_spec_quests$(EXE_SUFFIX)

# K8s Black Friday Quest Test
test-k8s-black-friday: build/tests/test_k8s_black_friday.o $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) build/tests/test_k8s_black_friday.o $(filter-out build/src/smite.o,$(SRC_OBJS)) -o build/tests/test_k8s_black_friday$(EXE_SUFFIX) $(LDFLAGS)
	./build/tests/test_k8s_black_friday$(EXE_SUFFIX)

# Telemetry Test
test-telemetry: build/tests/test_telemetry.o $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) build/tests/test_telemetry.o $(filter-out build/src/smite.o,$(SRC_OBJS)) -o build/tests/test_telemetry$(EXE_SUFFIX) $(LDFLAGS)
	./build/tests/test_telemetry$(EXE_SUFFIX)

# Settings Test
test-settings: build/tests/test_settings.o $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) build/tests/test_settings.o $(filter-out build/src/smite.o,$(SRC_OBJS)) -o build/tests/test_settings$(EXE_SUFFIX) $(LDFLAGS)
	./build/tests/test_settings$(EXE_SUFFIX)

build/tests/%.o: tests/%.cpp
	@$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Release build (stripped binary)
release: CXXFLAGS += -O3 -DNDEBUG
release: LDFLAGS += -s
release: clean $(TARGET)
	@echo "Built release binary: $(TARGET)"

# Create distribution package
dist: release
	@$(MKDIR) -p dist/smite-$(VERSION)-$(PLATFORM)
	cp $(TARGET) LICENSE README.md dist/smite-$(VERSION)-$(PLATFORM)/
	@$(MKDIR) -p dist/smite-$(VERSION)-$(PLATFORM)/src/modules/kubernetes/quests
	@$(MKDIR) -p dist/smite-$(VERSION)-$(PLATFORM)/src/modules/kubernetes/state
	cp src/modules/kubernetes/quests/*.yaml dist/smite-$(VERSION)-$(PLATFORM)/src/modules/kubernetes/quests/ 2>/dev/null || true
	cp src/modules/kubernetes/state/*.yaml dist/smite-$(VERSION)-$(PLATFORM)/src/modules/kubernetes/state/ 2>/dev/null || true
	cd dist && $(TAR) -czf smite-$(VERSION)-$(PLATFORM).tar.gz smite-$(VERSION)-$(PLATFORM)
	@echo "Created dist/smite-$(VERSION)-$(PLATFORM).tar.gz"

# Debug target to show platform info
show-platform:
	@echo "Platform: $(PLATFORM)"
	@echo "Target: $(TARGET)"
	@echo "CXX: $(CXX)"
	@echo "OS: $(UNAME_S)"

# Debug target to show what files are being compiled
show-sources:
	@echo "Source files:"
	@echo $(SRC_SRCS) | tr ' ' '\n'

.PHONY: clean test release dist show-platform show-sources test-k8s-quest test-shell test-quest-browser test-k8s-spec test-k8s-black-friday test-telemetry test-settings
