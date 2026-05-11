CC=gcc
CCFLAGS=-Wall

BUILD_DIR=build
SRC_DIR=src
INCLUDE_DIR=include

OBJECT_FILES=$(addprefix $(BUILD_DIR)/,chunk.o debug.o main.o memory.o value.o)
DEPS=$(addprefix $(INCLUDE_DIR)/,chunk.h common.h debug.h memory.h value.h)

TARGET=clox

$(TARGET): $(OBJECT_FILES)
	$(CC) $(CCFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	@rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: clean