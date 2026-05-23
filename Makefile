CXX      := g++
CXXFLAGS := -std=c++17 -pedantic-errors -Wall -Wextra -Werror
LDFLAGS  := -L/usr/lib -lstdc++ -lm
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/apps
INCLUDE  := -Iinclude/

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

all: receiver sender

$(OBJ_DIR)/%.o: %.cc
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -MMD -o $@

receiver: $(APP_DIR)/$(RECEIVER_TARGET)
sender:   $(APP_DIR)/$(SENDER_TARGET)

$(APP_DIR)/$(RECEIVER_TARGET): $(RECEIVER_OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(APP_DIR)/$(SENDER_TARGET): $(SENDER_OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

-include $(RECEIVER_DEPS)
-include $(SENDER_DEPS)

# Receiver commands
receiver-debug:   CXXFLAGS += -DDEBUG -g -O0
receiver-debug:   receiver

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
sender-debug:     CXXFLAGS += -DDEBUG -g -O0
sender-debug:     sender

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

# Global
clean:
	-@rm -rvf $(OBJ_DIR)/* $(APP_DIR)/*

.PHONY: all clean                                                       \
        receiver receiver-debug receiver-release receiver-info receiver-clean \
        sender   sender-debug   sender-release   sender-info   sender-clean