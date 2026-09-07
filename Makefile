CXX      := g++
CXXFLAGS := -std=c++20 -pedantic-errors -Wall -Wextra -Wshadow -Werror $(shell pkg-config --cflags absl_flat_hash_map)
LDFLAGS  := -L/usr/lib -lstdc++ -lm -lpthread $(shell pkg-config --libs absl_flat_hash_map)
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/apps
INCLUDE  := -Iinclude/

# Build provenance, stamped into every run document so a result can be traced back to the exact source and flags that produced it. PROVENANCE is recursively expanded on purpose: it is evaluated inside the compile recipe, where the target-specific CXXFLAGS additions (-O3, -flto, ...) are in effect.
GIT_SHA      := $(shell git rev-parse --short=8 HEAD 2>/dev/null || echo unknown)
GIT_DIRTY    := $(shell git status --porcelain 2>/dev/null | head -c 1 | wc -c)
BUILD_CONFIG := dev
PROVENANCE    = -DBENCH_GIT_SHA='"$(GIT_SHA)"' -DBENCH_GIT_DIRTY=$(GIT_DIRTY) -DBENCH_BUILD_CONFIG='"$(BUILD_CONFIG)"' -DBENCH_CXXFLAGS='"$(CXXFLAGS)"'

RECEIVER_TARGET  := receiver
RECEIVER_SRC     :=                       \
   $(wildcard src/receiver/*.cc)          \
   $(wildcard src/shared/*.cc)
RECEIVER_OBJECTS := $(RECEIVER_SRC:%.cc=$(OBJ_DIR)/%.o)
RECEIVER_DEPS    := $(RECEIVER_OBJECTS:.o=.d)

SENDER_TARGET    := sender
SENDER_SRC       :=                       \
   $(wildcard src/sender/*.cc)            \
   $(wildcard src/shared/*.cc)
SENDER_OBJECTS   := $(SENDER_SRC:%.cc=$(OBJ_DIR)/%.o)
SENDER_DEPS      := $(SENDER_OBJECTS:.o=.d)

OFFLINE_LOB_TARGET 	:= offline
OFFLINE_LOB_SRC		:= 					  \
	$(wildcard src/offline/*.cc)		  \
	$(wildcard src/shared/*.cc)
OFFLINE_LOB_OBJECTS := $(OFFLINE_LOB_SRC:%.cc=$(OBJ_DIR)/%.o)
OFFLINE_LOB_DEPS    := $(OFFLINE_LOB_OBJECTS:.o=.d)

all: receiver sender offline

$(OBJ_DIR)/%.o: %.cc
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(PROVENANCE) -c $< -MMD -o $@

# The commit and the flags are compiled into this one object, and make has no way to know they changed, so it is rebuilt unconditionally. It is a single small translation unit.
$(OBJ_DIR)/src/shared/run_result.o: FORCE
FORCE:

receiver: $(APP_DIR)/$(RECEIVER_TARGET)
sender:   $(APP_DIR)/$(SENDER_TARGET)
offline:  $(APP_DIR)/$(OFFLINE_LOB_TARGET)

$(APP_DIR)/$(RECEIVER_TARGET): $(RECEIVER_OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(APP_DIR)/$(SENDER_TARGET): $(SENDER_OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(APP_DIR)/$(OFFLINE_LOB_TARGET): $(OFFLINE_LOB_OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

-include $(RECEIVER_DEPS)
-include $(SENDER_DEPS)
-include $(OFFLINE_LOB_DEPS)

# Receiver commands
receiver-debug:   BUILD_CONFIG := debug
receiver-debug:   CXXFLAGS += -DDEBUG -g -O0
receiver-debug:   receiver

receiver-release: BUILD_CONFIG := release
receiver-release: CXXFLAGS += -O3 -march=native -mtune=native -flto -DNDEBUG
receiver-release: LDFLAGS  += -flto
receiver-release: receiver

receiver-info:
	@echo "[receiver] Target:       $(APP_DIR)/$(RECEIVER_TARGET)"
	@echo "[receiver] Sources:      $(RECEIVER_SRC)"
	@echo "[receiver] Objects:      $(RECEIVER_OBJECTS)"
	@echo "[receiver] Dependencies: $(RECEIVER_DEPS)"

receiver-clean:
	-@rm -f $(RECEIVER_OBJECTS) $(RECEIVER_DEPS)
	-@rm -f $(APP_DIR)/$(RECEIVER_TARGET)

# Sender commands
sender-debug:     BUILD_CONFIG := debug
sender-debug:     CXXFLAGS += -DDEBUG -g -O0
sender-debug:     sender

sender-release:   BUILD_CONFIG := release
sender-release:   CXXFLAGS += -O3 -march=native -mtune=native -flto -DNDEBUG
sender-release:   LDFLAGS  += -flto
sender-release:   sender

sender-info:
	@echo "[sender] Target:       $(APP_DIR)/$(SENDER_TARGET)"
	@echo "[sender] Sources:      $(SENDER_SRC)"
	@echo "[sender] Objects:      $(SENDER_OBJECTS)"
	@echo "[sender] Dependencies: $(SENDER_DEPS)"

sender-clean:
	-@rm -f $(SENDER_OBJECTS) $(SENDER_DEPS)
	-@rm -f $(APP_DIR)/$(SENDER_TARGET)

# Offline commands
offline-debug:     BUILD_CONFIG := debug
offline-debug:     CXXFLAGS += -DDEBUG -g -O0
offline-debug:     offline

offline-release:   BUILD_CONFIG := release
offline-release:   CXXFLAGS += -O2 -march=native -mtune=native -flto -DNDEBUG
offline-release:   LDFLAGS  += -flto
offline-release:   offline

offline-info:
	@echo "[offline] Target:       $(APP_DIR)/$(OFFLINE_LOB_TARGET)"
	@echo "[offline] Sources:      $(OFFLINE_LOB_SRC)"
	@echo "[offline] Objects:      $(OFFLINE_LOB_OBJECTS)"
	@echo "[offline] Dependencies: $(OFFLINE_LOB_DEPS)"

offline-clean:
	-@rm -f $(OFFLINE_LOB_OBJECTS) $(OFFLINE_LOB_DEPS)
	-@rm -f $(APP_DIR)/$(OFFLINE_LOB_TARGET)

# Tests (not part of `all`)
# The ring is header-only, so these compile straight from the test source.
# TSan gets its own binary: it cannot be combined with the release flags.
spsc-test:
	@mkdir -p $(APP_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -O2 -march=native -o $(APP_DIR)/spsc_test tests/spsc_test.cc -lpthread

spsc-test-tsan:
	@mkdir -p $(APP_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -O1 -g -fsanitize=thread -o $(APP_DIR)/spsc_test_tsan tests/spsc_test.cc -lpthread

test: spsc-test spsc-test-tsan
	$(APP_DIR)/spsc_test 5000000
	$(APP_DIR)/spsc_test_tsan 300000

test-clean:
	-@rm -f $(APP_DIR)/spsc_test $(APP_DIR)/spsc_test_tsan

# Global
clean:
	-@rm -rvf $(OBJ_DIR)/* $(APP_DIR)/*

.PHONY: all clean FORCE                                                       \
        receiver receiver-debug receiver-release receiver-info receiver-clean \
        sender   sender-debug   sender-release   sender-info   sender-clean	  \
		offline  offline-debug  offline-release  offline-info  offline-clean	  \
		test     spsc-test      spsc-test-tsan   test-clean