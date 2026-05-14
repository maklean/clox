#include "../include/common.h"
#include "../include/chunk.h"
#include "../include/debug.h"
#include "../include/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

// Runs the REPL.
static void repl();

// Runs the file at the given path.
static void runFile(const char *path);

// Returns the contents of the file at 'path'.
static char *readFile(const char *path);

int main(int argc, const char **argv) {
    initVM();

    if(argc == 1) {
        repl();
    } else if(argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(EX_USAGE);
    }

    freeVM();

    return 0;
}

static void repl() {
    char line[1024];

    for(;;) {
        printf("> ");

        if(!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret(line);
    }
}

static void runFile(const char *path) {
    char *source = readFile(path);

    InterpretResult result = interpret(source);
    free(source);

    if(result == INTERPRET_COMPILE_ERROR) exit(EX_DATAERR);
    if(result == INTERPRET_RUNTIME_ERROR) exit(EX_SOFTWARE);
}

static char *readFile(const char *path) {
    if(path == NULL) exit(EX_USAGE);

    FILE *f = fopen(path, "rb");
    if(f == NULL) {
        fprintf(stderr, "Failed to open file at \"%s\".\n", path);
        exit(EX_IOERR);
    }

    fseek(f, 0, SEEK_END);
    size_t length = ftell(f);
    rewind(f);

    // copy entire file content into 'buffer'
    char *buffer = malloc(length+1);
    if(buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(EX_IOERR);
    }

    size_t read = fread(buffer, 1, length, f);
    if(read < length) {
        fprintf(stderr, "Could not read \"%s\".\n", path);
        exit(EX_IOERR);
    }

    buffer[read] = '\0';

    fclose(f);
    return buffer;
}