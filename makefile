-include /home/seelts/dev/makefile

OPTIMIZATION_FLAGS := $(DEBUG_FLAGS)

INCLUDES_DIR_FLAGS := -I/usr/include/dbus-1.0 -I/usr/lib64/dbus-1.0/include
LIBS_DIR_FLAGS := -L/usr/lib64
LDLIBS := -ldbus-1

###################

all: server client

################### server
SOURCES = $(addprefix $(SRC)/,server.c)
include $(wildcard $(SOURCES:$(SRC)/%.c=$(BUILD_DEP)/%.d))
server: $(BUILD_BIN)/server
$(BUILD_BIN)/server: $(OBJECTS)

################### client
SOURCES = $(addprefix $(SRC)/,client.c)
include $(wildcard $(SOURCES:$(SRC)/%.c=$(BUILD_DEP)/%.d))
client: $(BUILD_BIN)/client
$(BUILD_BIN)/client: $(OBJECTS)
