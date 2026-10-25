# Detect OS and CPU. x86_64 keeps the amd64 archive name. Linux reports
# aarch64 and macOS reports arm64; both package as arm64. The Windows
# release stays windows-amd64 (MinGW64 x86_64). There is no windows-arm64.
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)
UNAME_M := $(shell uname -m 2>/dev/null || echo unknown)

ifeq ($(filter aarch64 arm64,$(UNAME_M)),)
    ARCH := amd64
else
    ARCH := arm64
endif

# Compiler
CXX := clang++

# Version
VERSION := 0.2.0
# Pieces of VERSION for the Windows file-version resource (X.Y.Z).
VER_MAJOR := $(word 1,$(subst ., ,$(VERSION)))
VER_MINOR := $(word 2,$(subst ., ,$(VERSION)))
VER_PATCH := $(word 3,$(subst ., ,$(VERSION)))

# Platform-specific settings
ifeq ($(findstring Linux,$(UNAME_S)),Linux)
    # Linux. Debian/Ubuntu keep shared libraries in the multiarch directory.
    TARGET := smite
    PLATFORM := linux-$(ARCH)
    INCLUDE_PATHS := -I. -I src -I/usr/include
    ifeq ($(ARCH),arm64)
        LIB_PATHS := -L/usr/lib -L/usr/lib/aarch64-linux-gnu
    else
        LIB_PATHS := -L/usr/lib -L/usr/lib/x86_64-linux-gnu
    endif
    LIBS := -lyaml-cpp -lfmt -lreadline -lncurses -lstdc++fs
    CXXFLAGS := -g -std=c++20 -DYAML_CPP_STATIC_DEFINE -I. -I src $(INCLUDE_PATHS)
    LDFLAGS := $(LIB_PATHS) $(LIBS)
    MKDIR := mkdir
    RM := rm
    TAR := tar
    EXE_SUFFIX :=
else ifeq ($(findstring Darwin,$(UNAME_S)),Darwin)
    # macOS. Apple Silicon Homebrew is /opt/homebrew. Intel Homebrew is
    # /usr/local. readline and ncurses are keg-only, so headers and libs
    # live under opt/<formula> and are not linked into the prefix include/lib.
    TARGET := smite
    PLATFORM := macos-$(ARCH)
    ifeq ($(ARCH),arm64)
        BREW_PREFIX := /opt/homebrew
        BREW_FALLBACK := /usr/local
    else
        BREW_PREFIX := /usr/local
        BREW_FALLBACK := /opt/homebrew
    endif
    INCLUDE_PATHS := -I. -I src \
        -I$(BREW_PREFIX)/include \
        -I$(BREW_PREFIX)/opt/readline/include \
        -I$(BREW_PREFIX)/opt/ncurses/include \
        -I$(BREW_FALLBACK)/include \
        -I$(BREW_FALLBACK)/opt/readline/include \
        -I$(BREW_FALLBACK)/opt/ncurses/include
    LIB_PATHS := \
        -L$(BREW_PREFIX)/lib \
        -L$(BREW_PREFIX)/opt/readline/lib \
        -L$(BREW_PREFIX)/opt/ncurses/lib \
        -L$(BREW_FALLBACK)/lib \
        -L$(BREW_FALLBACK)/opt/readline/lib \
        -L$(BREW_FALLBACK)/opt/ncurses/lib
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
    INCLUDE_PATHS := -I. -I src -I$(MSYS2_PATH)/include
    LIB_PATHS := -L$(MSYS2_PATH)/lib
    LIBS := -lyaml-cpp -lfmt -lreadline -lncurses -lstdc++fs
    CXXFLAGS := -g -std=c++20 -finput-charset=UTF-8 -fexec-charset=UTF-8 -DYAML_CPP_STATIC_DEFINE -DNCURSES_STATIC -static $(INCLUDE_PATHS)
    LDFLAGS := $(LIB_PATHS) $(LIBS) -mconsole -static
    # MSYS make (the `make` package under `msys2 {0}`) spawns a recipe
    # command itself. A drive-letter path such as C:/msys64/usr/bin/rm is
    # not resolved to rm.exe, so `make dist` dies in `clean` with Error 127.
    # /usr/bin is the MSYS2 prefix and does not depend on the parent PATH.
    MKDIR := /usr/bin/mkdir
    RM := /usr/bin/rm
    TAR := /usr/bin/tar
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

# Windows resource compilation.
# smite.rc carries the same strings as the 0.1.0-alpha executable. @VERSION@
# and @VER_*@ are filled from VERSION so `make dist VERSION=0.2.0` matches the tag.
build/resources/smite_res.o: resources/smite.rc resources/smite.ico
	@$(MKDIR) -p $(dir $@)
	sed -e 's/@VERSION@/$(VERSION)/g' \
	    -e 's/@VER_MAJOR@/$(VER_MAJOR)/g' \
	    -e 's/@VER_MINOR@/$(VER_MINOR)/g' \
	    -e 's/@VER_PATCH@/$(VER_PATCH)/g' \
	    resources/smite.rc > build/resources/smite.rc
	cp resources/smite.ico build/resources/smite.ico
	cd build/resources && $(WINDRES) smite.rc -O coff -o smite_res.o

build/src/%.o: src/%.cpp
	@$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	$(RM) -rf build dist $(TARGET)

# Run all tests
tests: test-k8s-quest test-shell test-quest-browser test-k8s-spec test-k8s-black-friday test-telemetry test-settings test-first-run test-k8s-scale test-k8s-quest-commands

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

# First-run UX: data paths, guided quests, kubectl aliases, hint commands
test-first-run: build/tests/test_first_run.o $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) build/tests/test_first_run.o $(filter-out build/src/smite.o,$(SRC_OBJS)) -o build/tests/test_first_run$(EXE_SUFFIX) $(LDFLAGS)
	./build/tests/test_first_run$(EXE_SUFFIX)

# kubectl scale argument routing
test-k8s-scale: build/tests/test_k8s_scale.o $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) build/tests/test_k8s_scale.o $(filter-out build/src/smite.o,$(SRC_OBJS)) -o build/tests/test_k8s_scale$(EXE_SUFFIX) $(LDFLAGS)
	./build/tests/test_k8s_scale$(EXE_SUFFIX)

# Player-facing kubectl commands referenced by quests
test-k8s-quest-commands: build/tests/test_k8s_quest_commands.o $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) build/tests/test_k8s_quest_commands.o $(filter-out build/src/smite.o,$(SRC_OBJS)) -o build/tests/test_k8s_quest_commands$(EXE_SUFFIX) $(LDFLAGS)
	./build/tests/test_k8s_quest_commands$(EXE_SUFFIX)

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
	@echo "Machine: $(UNAME_M)"
	@echo "Lib paths: $(LIB_PATHS)"

# Debug target to show what files are being compiled
show-sources:
	@echo "Source files:"
	@echo $(SRC_SRCS) | tr ' ' '\n'

.PHONY: clean test tests release dist show-platform show-sources test-k8s-quest test-shell test-quest-browser test-k8s-spec test-k8s-black-friday test-telemetry test-settings test-first-run test-k8s-scale test-k8s-quest-commands
