CXX      := g++
CXXFLAGS := -std=c++20 -pedantic-errors -Wall -Wextra -Werror
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

OFFLINE_LOB_TARGET 	:= offline
OFFLINE_LOB_SRC		:= 					  \
	$(wildcard src/offline/*.cc)		  \
	src/receiver/benchmark.cc			  \
	src/receiver/limit_order_book.cc	  \
	src/receiver/message_parser.cc
OFFLINE_LOB_OBJECTS := $(OFFLINE_LOB_SRC:%.cc=$(OBJ_DIR)/%.o)
OFFLINE_LOB_DEPS    := $(OFFLINE_LOB_OBJECTS:.o=.d)

all: receiver sender offline

$(OBJ_DIR)/%.o: %.cc
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -MMD -o $@

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

# Offline commands
offline-debug:     CXXFLAGS += -DDEBUG -g -O0
offline-debug:     offline

offline-release:   CXXFLAGS += -O3 -march=native -mtune=native -flto -DNDEBUG
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

# Global
clean:
	-@rm -rvf $(OBJ_DIR)/* $(APP_DIR)/*

.PHONY: all clean                                                       \
        receiver receiver-debug receiver-release receiver-info receiver-clean \
        sender   sender-debug   sender-release   sender-info   sender-clean	  \
		offline  offline-debug  offline-release  offline-info  offline-clean