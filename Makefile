CC=gcc
CCFLAGS=-g

BUILD_DIR=build
SRC_DIR=src
INCLUDE_DIR=include

OBJECT_FILES=$(addprefix $(BUILD_DIR)/,chunk.o compiler.o debug.o main.o memory.o object.o scanner.o table.o value.o vm.o)
DEPS=$(addprefix $(INCLUDE_DIR)/,chunk.h common.h compiler.h debug.h memory.h object.h scanner.h table.h value.h vm.h)

TARGET=clox

$(TARGET): $(OBJECT_FILES)
	$(CC) $(CCFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	@rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: clean