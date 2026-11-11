# Factory Equipment Preventive Maintenance Tracker
# Targets: all, run, test, clean

# ---------- Tools and flags ----------
CC       := gcc
CXX      := g++
CFLAGS   := -std=c11 -Wall -Wextra -Wpedantic -g -Iinclude
CXXFLAGS := -std=c++17 -Wall -Wextra -g -Iinclude
TEST_LIBS := -lgtest -lgtest_main -lpthread

# ---------- Directories and output names ----------
SRC_DIR   := src
TEST_DIR  := tests
BUILD_DIR := build

ifeq ($(OS),Windows_NT)
    EXE_EXT := .exe
endif

APP      := $(BUILD_DIR)/maintenance_tracker$(EXE_EXT)
TEST_BIN := $(BUILD_DIR)/run_tests$(EXE_EXT)

# ---------- Source and object lists ----------
SRCS      := $(wildcard $(SRC_DIR)/*.c)
OBJS      := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
# The tests link every module except main.o (gtest supplies its own main).
LIB_OBJS  := $(filter-out $(BUILD_DIR)/main.o,$(OBJS))

TEST_SRCS := $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJS := $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/tests/%.o,$(TEST_SRCS))

.PHONY: all run test clean

# ---------- Main targets ----------
all: $(APP)

run: $(APP)
	./$(APP)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR)

# ---------- Linking ----------
$(APP): $(OBJS)
	$(CC) $(OBJS) -o $@

$(TEST_BIN): $(TEST_OBJS) $(LIB_OBJS)
	$(CXX) $(TEST_OBJS) $(LIB_OBJS) -o $@ $(TEST_LIBS)

# ---------- Compiling (one object file per source file) ----------
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/tests/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# ---------- Header dependency tracking ----------
-include $(OBJS:.o=.d) $(TEST_OBJS:.o=.d)
