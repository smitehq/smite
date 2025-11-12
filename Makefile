# Compiler and flags
CXX := clang++
MYSYS2_PATH := C:/msys64/mingw64
CXXFLAGS := -g -std=c++20 -finput-charset=UTF-8 -fexec-charset=UTF-8 -DYAML_CPP_STATIC_DEFINE -DNCURSES_STATIC -static -I. -I src -I$(MYSYS2_PATH)/include
LIBS = -lyaml-cpp -lfmt -lreadline -lncurses -lstdc++fs
LDFLAGS := -L$(MYSYS2_PATH)/lib $(LIBS) -mconsole -static

# MSYS2 tools path (for Unix commands like mkdir/rm)
MSYS2_TOOLS := C:/msys64/usr/bin
MKDIR := $(MSYS2_TOOLS)/mkdir
RM := $(MSYS2_TOOLS)/rm
TAR := $(MSYS2_TOOLS)/tar

# Version
VERSION := 0.1.0

# Source files
SRC_SRCS := $(wildcard src/*.cpp) \
            $(wildcard src/core/*.cpp) \
            $(wildcard src/shell/*.cpp) \
            $(wildcard src/state/*.cpp) \
            $(wildcard src/modules/**/*.cpp)

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
	$(RM) -rf build dist $(TARGET)

# K8s Quest Test (specific standalone test)
test-k8s-quest: build/tests/test_k8s_quest.o $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) build/tests/test_k8s_quest.o $(filter-out build/src/smite.o,$(SRC_OBJS)) -o build/tests/test_k8s_quest.exe $(LDFLAGS)
	./build/tests/test_k8s_quest.exe

# Shell module (specific standalone test)
test-shell: build/tests/test_shell.o $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) build/tests/test_shell.o $(filter-out build/src/smite.o,$(SRC_OBJS)) -o build/tests/test_shell.exe $(LDFLAGS)
	./build/tests/test_shell.exe

# Quest Browser Test (specific standalone test)
test-quest-browser: build/tests/test_quest_browser.o $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) build/tests/test_quest_browser.o $(filter-out build/src/smite.o,$(SRC_OBJS)) -o build/tests/test_quest_browser.exe $(LDFLAGS)
	./build/tests/test_quest_browser.exe

# K8s Spec Tracking Quests Test
test-k8s-spec: build/tests/test_k8s_spec_quests.o $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) build/tests/test_k8s_spec_quests.o $(filter-out build/src/smite.o,$(SRC_OBJS)) -o build/tests/test_k8s_spec_quests.exe $(LDFLAGS)
	./build/tests/test_k8s_spec_quests.exe

# K8s Black Friday Quest Test
test-k8s-black-friday: build/tests/test_k8s_black_friday.o $(SRC_OBJS)
	$(CXX) $(CXXFLAGS) build/tests/test_k8s_black_friday.o $(filter-out build/src/smite.o,$(SRC_OBJS)) -o build/tests/test_k8s_black_friday.exe $(LDFLAGS)
	./build/tests/test_k8s_black_friday.exe

build/tests/%.o: tests/%.cpp
	$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Dist (create binary tarball: exe + README + optional YAMLs)
dist: $(TARGET)
	$(MKDIR) -p dist/modules
	cp $(TARGET) README.md dist/
	yaml_files := $(shell find src/modules -name '*.yaml' -type f 2>/dev/null)
	if [ -n "$(yaml_files)" ]; then \
		for file in $(yaml_files); do \
			dir=$$(dirname $$file | sed 's|^src/modules/||'); \
			$(MKDIR) -p "dist/modules/$$dir"; \
			cp $$file "dist/modules/$$dir/"; \
		done; \
	else \
		echo "No YAMLs found—skipping"; \
	fi
	$(TAR) -czf dist/smite-$(VERSION).tar.gz -C dist .
	$(RM) -rf dist

# Distcheck (test binary dist: extract, run exe, verify)
distcheck: dist
	$(TAR) -xzf dist/smite-$(VERSION).tar.gz
	cd smite-$(VERSION) && ./$(TARGET) --version
	cd smite-$(VERSION) && make test
	$(RM) -rf smite-$(VERSION)
	$(RM) dist/smite-$(VERSION).tar.gz

# Debug target to show what files are being compiled
show-sources:
	@echo "Source files:"
	@echo $(SRC_SRCS) | tr ' ' '\n'

.PHONY: clean test distcheck dist show-sources test-k8s-quest test-shell test-quest-browser test-k8s-spec test-k8s-black-friday
