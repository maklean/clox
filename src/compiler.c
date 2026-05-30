#include "../include/common.h"
#include "../include/compiler.h"
#include "../include/scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "../include/debug.h"
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

// can't use UINT24 limit since it segfaults...
#define UINT16_COUNT (UINT16_MAX + 1)

typedef struct {
    Token name;
    int depth;
} Local;

// Local variable state
typedef struct {
    Local locals[UINT16_COUNT];
    int localCount;
    int scopeDepth; // current scope depth
} Compiler;

Parser parser;
Chunk *compilingChunk;
Compiler* current = NULL;

// Initializes the compiler state.
static void initCompiler(Compiler *compiler);

// Advances through the scanner's token stream. Sets the first encountered non-error token to parser.current.
static void advance();

// Attempts to match the next token with the given type, returns true and advances if so.
static bool match(TokenType type);

// Checks whether the next token to consume is of the given type.
static bool check(TokenType type);

// Reports an error at the current token in the parser.
static void errorAtCurrent(const char *message);

// Reports an error at the previous token in the parser.
static void error(const char *message);

// Reports an error at the given token.
static void errorAt(Token *token, const char *message);

// Expects the next token to consume to be of the given type, if not, throws an error.
static void consume(TokenType type, const char *message);

// Writes the given byte into the source code's chunk.
static void emitByte(uint8_t byte);

// Too lazy to use emitByte() twice in a row multiple times, so we make a function that does it for us.
static void emitBytes(uint8_t byte1, uint8_t byte2);

// Emits an instruction with a 3 byte long operand.
static void emitLongBytes(uint8_t instruction, int bytes);

// Returns the current chunk.
static Chunk *currentChunk();

static void endCompiler();

// Parses a declaration
static void declaration();

// Parses a variable declaration.
static void varDeclaration();

// Parses a statement.
static void statement();

// Parses a print statement.
static void printStatement();

// Parses an expression statement.
static void expressionStatement();

// Parses an if statement.
static void ifStatement();

// Parses a while statement.
static void whileStatement();

// Parses a for statement.
static void forStatement();

// Emits a loop instruction with the given loopStart (where to jump back to) as the instruction operand.
static void emitLoop(int loopStart);

// Parses an expression.
static void expression();

// Emits a number instruction.
static void number(bool canAssign);

// Parses grouping.
static void grouping(bool canAssign);

// Parses unary negation
static void unary(bool canAssign);

// Parses a binary expression.
static void binary(bool canAssign);

// Parses a literal (true|false|nil).
static void literal(bool canAssign);

// Parses a string.
static void string(bool canAssign);

// Parses/Reads a variable
static void variable(bool canAssign);

// Parses an 'and' keyword.
static void and_(bool canAssign);

// Parses an 'or' keyword.
static void or_(bool canAssign);

// Parses a block (all declarations between braces)
static void block();

// Increments the global scope depth (called on entering a new scope)
static void beginScope();

// Decrements the global scope depth (called on leaving a scope)
static void endScope();

// Writes the `OP_RETURN` instruction to the current chunk.
static void emitReturn();

// Writes the OP_CONSTANT/OP_CONSTANT_LONG instruction to the current chunk.
static void emitConstant(Value value);

// Emits a jump instruction with an offset placeholder. Returns the offset of where to begin patching.
static int emitJump(uint8_t instruction);

// Resolves/patching the operand of a jump instruction.
static void patchJump(int offset);

// Handles parsing with precedence by calling functions of equal or higher precedence.
static void parsePrecedence(Precedence precedence);

// Consumes a variable identifier and returns the index of the constant in the constant table.
static int parseVariable(const char *errorMessage);

// Emits a OP_GET_GLOBAL/OP_GET_GLOBAL_LONG instruction to the current Chunk.
static void namedVariable(Token name, bool canAssign);

// Emits a OP_DEFINE_GLOBAL/OP_DEFINE_GLOBAL_LONG instruction.
static void defineVariable(int global);

// Declares a local variable.
static void declareVariable();

// Adds the given token to the global locals array.
static void addLocal(Token name);

// Marks the variable at the top of the locals array as initialized (ready for usage).
static void markInitialized();

// Returns whether two identifier tokens have the same name.
static bool identifiersEqual(Token *a, Token *b);

// Iterates (in reverse) through the global array of locals, returns the index where the local lives.
static int resolveLocal(Compiler *compiler, Token *name);

// Adds the given token to the constant table (as an ObjString), and returns its index.
static int identifierConstant(Token *name);

// Gets the parse rule for the given token type.
static ParseRule *getRule(TokenType type);

// Tries to exit panic mode by finding the next synchronization point.
static void synchronize();

ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     and_,   PREC_AND},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     or_,    PREC_OR},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static void initCompiler(Compiler *compiler) {
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    current = compiler;
}

bool compile(const char *source, Chunk *chunk) {
    initScanner(source);

    Compiler compiler;
    initCompiler(&compiler);
    
    compilingChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;

    advance();

    while(!match(TOKEN_EOF)) {
        declaration();
    }

    endCompiler();

    return !parser.hadError;
}

static void endCompiler() {
    emitReturn();

    #ifdef DEBUG_PRINT_CODE
        if(!parser.hadError) {
            disassembleChunk(currentChunk(), "code");
        }
    #endif
}

static void advance() {
    parser.previous = parser.current;

    for(;;) {
        parser.current = scanToken();
        if(parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

static bool match(TokenType type) {
    if(!check(type)) return false;
    advance();
    return true;
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

static void consume(TokenType type, const char *message) {
    if(parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitLongBytes(uint8_t instruction, int bytes) {
    emitByte(instruction);
    
    emitByte((uint8_t)((bytes >> 16) & 0xFF));
    emitByte((uint8_t)((bytes >> 8) & 0xFF));
    emitByte((uint8_t)(bytes & 0xFF));
}

static void emitReturn() {
    emitByte(OP_RETURN);
}

static void emitConstant(Value value) {
    // Will emit either OP_CONSTANT or OP_CONSTANT_LONG to the current chunk.
    if(!writeConstant(currentChunk(), value, parser.previous.line)) {
        error("Too many constants in one chunk.");
        return;
    }
}

static int emitJump(uint8_t instruction) {
    emitByte(instruction);
    
    // placeholder (allows for max. uint16_t jumps)
    emitBytes(0xFF, 0xFF);

    return currentChunk()->count-2;
}

static void patchJump(int offset) {
    // calculate how many bytes to go forward from the operands if the condition if false
    int jump = currentChunk()->count - offset - 2;

    if(jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xFF;
    currentChunk()->code[offset+1] = jump & 0xFF;
}

static Chunk *currentChunk() {
    return compilingChunk;
}

static void errorAtCurrent(const char *message) {
    errorAt(&parser.current, message);
}

static void error(const char* message) {
    errorAt(&parser.previous, message);
}

static void errorAt(Token *token, const char *message) {
    if(parser.panicMode) return;

    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if(token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if(token->type == TOKEN_ERROR) {

    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void declaration() {
    if(match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        // if it doesn't match any of the tokens above, it's a statement
        statement();
    }

    // Find synchronization point if we're in panic mode
    if(parser.panicMode) synchronize();
}

static void varDeclaration() {
    int global = parseVariable("Expected variable/identifier name.");

    // Emit the value instruction before the var. instruction
    if(match(TOKEN_EQUAL)) {
        expression();
    } else {
        // use nil as the initializer expression if none was provided
        emitByte(OP_NIL);
    }

    consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");
    defineVariable(global);
}

static void statement() {
    if(match(TOKEN_PRINT)) {
        printStatement();
    } else if(match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else if(match(TOKEN_IF)) {
        ifStatement();
    } else if(match(TOKEN_WHILE)) {
        whileStatement();
    } else if(match(TOKEN_FOR)) {
        forStatement();
    } else {
        // if we couldn't match with any token, it's an expression statement.
        expressionStatement();
    }
}

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after value in print statement.");
    emitByte(OP_PRINT);
}

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after an expression.");
    emitByte(OP_POP);
}

static void ifStatement() {
    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'if' keyword.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after if-statement expression.");

    // jump if the condition if false
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // pop condition (when the condition is truthy)
    statement();

    int elseJump = emitJump(OP_JUMP);

    // backpatch for the thenJump (should jump to the else block, or the code after the then block)
    patchJump(thenJump);
    emitByte(OP_POP); // pop condition (when the condition is falsey)

    if(match(TOKEN_ELSE)) statement();

    // backpatch for the elseJump (should jump over the elseBlock )
    patchJump(elseJump);
}

static void whileStatement() {
    int loopStart = currentChunk()->count; // the offset for the condition (so we can jump back)

    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after 'while' condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    emitLoop(loopStart); // emit a loop instruction if the condition is true

    patchJump(exitJump);
    emitByte(OP_POP); // pop condition and exit while-loop if condition is false.
}

static void forStatement() {
    beginScope(); // all variables should be scoped to the for block

    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'for'.");

    // initializer clause
    if(match(TOKEN_SEMICOLON)) {
        // no initializer clause
    } else if(match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        expressionStatement(); // parses the expression + consumes the semicolon and the expression on the VM stack
    }

    int loopStart = currentChunk()->count;
    int exitJump = -1;

    // condition clause
    if(!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expected ';' after loop condition.");

        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
    }

    // increment clause
    if(!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OP_JUMP); // jump over the increment expression into the body

        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expected ')' after increment clause.");

        emitLoop(loopStart); // this should loop back to the condition clause
        loopStart = incrementStart; // this is to make the increment expression run after the body
        patchJump(bodyJump);
    }

    statement();
    emitLoop(loopStart); // this should loop back to the increment expression

    if(exitJump != -1) {
        patchJump(exitJump); // jump out of the for-loop if the 
        emitByte(OP_POP); // pop condition out
    }

    endScope();
}

static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if(offset > UINT16_MAX) error("Loop body is too large.");

    emitByte((offset >> 8) & 0xFF);
    emitByte(offset & 0xFF);
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

static void number(bool canAssign) {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    // compile the rest of the operand
    parsePrecedence(PREC_UNARY);

    // emit the corresponding instruction
    switch(operatorType) {
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        case TOKEN_BANG: emitByte(OP_NOT); break;
        default: return;
    }
}

static void binary(bool canAssign) {
    TokenType operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_PLUS: emitByte(OP_ADD); break;
        case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
        case TOKEN_BANG_EQUAL: emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL: emitByte(OP_EQUAL); break;
        case TOKEN_GREATER: emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break; // a >= b <=> !(a < b)
        case TOKEN_LESS: emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL: emitBytes(OP_GREATER, OP_NOT); break; // a <= b <=> !(a > b)
        default: return; // Unreachable.
    }
}

static void literal(bool canAssign) {
    switch(parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        default: return; // Unreachable.
    }
}

static void string(bool canAssign) {
    emitConstant(OBJ_VAL(copyString(parser.previous.start+1, parser.previous.length-2)));
}

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static void and_(bool canAssign) {
    // emit jump to short-circuit if the condition is false
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP); // remove condition from stack (if condition is truthy)
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

static void or_(bool canAssign) {
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump); // jump to RHS expression if LHS is false
    emitByte(OP_POP); // pop LHS expression

    parsePrecedence(PREC_OR);
    patchJump(endJump); // jump outside of expression if the LHS expression is true
}

static void block() {
    // parse all declarations between the braces
    while(!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expected '}' to close block statement.");
}

static void beginScope() {
    current->scopeDepth++;
}

static void endScope() {
    current->scopeDepth--;

    // remove all local variables in the scope (can optimize with an OP_POPN)
    while(current->localCount > 0 && current->locals[current->localCount-1].depth > current->scopeDepth) {
        emitByte(OP_POP);
        current->localCount--;
    }
}

static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;

    // syntax error if there's no prefix rule
    if(prefixRule == NULL) {
        error("Expected expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    // call infix rules that are at least the current precedence
    while(precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    // if equal is the top token, we couldn't use the infix rule properly, so they tried to assign to an invalid target.
    if(canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static int parseVariable(const char *errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if(current->scopeDepth > 0) return 0; // exit and return a dummy index if we're in a local scope

    return identifierConstant(&parser.previous);
}

static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);

    if(arg != -1) {
        getOp = arg <= 255 ? OP_GET_LOCAL : OP_GET_LOCAL_LONG;
        setOp = arg <= 255 ? OP_SET_LOCAL : OP_SET_LOCAL_LONG;
    } else {
        // it's a global variable
        arg = identifierConstant(&name);

        getOp = arg <= 255 ? OP_GET_GLOBAL : OP_GET_GLOBAL_LONG;
        setOp = arg <= 255 ? OP_SET_GLOBAL : OP_SET_GLOBAL_LONG;
    }

    // todo: clean this
    if(canAssign && match(TOKEN_EQUAL)) {
        // If there's an equal after the variable, it's an assignment. Operand is the index in the constant table (if global), or locals array (if local)
        expression();  
        (arg <= 255) ? emitBytes(setOp, (uint8_t)arg) : emitLongBytes(setOp, arg);
    } else {
        (arg <= 255) ? emitBytes(getOp, (uint8_t)arg) : emitLongBytes(getOp, arg);
    }
}

static void defineVariable(int global) {
    if(current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    if(global <= 255) {
        emitBytes(OP_DEFINE_GLOBAL, (uint8_t)global);
    } else {
        emitLongBytes(OP_DEFINE_GLOBAL_LONG, global);
    }
}

static void declareVariable() {
    // if it's in the global scope, don't declare as a local
    if(current->scopeDepth == 0) return;

    Token *name = &parser.previous;
    for(int i = current->scopeDepth-1; i > -1; i--) {
        Local *local = &current->locals[i];

        // we've passed all variables in the current scope, any shadowing is allowed from here.
        if(local->depth < current->scopeDepth) {
            break;
        }

        // if it's the same scope and identifer name, the user is redeclaring the variable
        if(identifiersEqual(name, &local->name)) {
            error("There's already a variable with this name in the current scope.");
        }
    }

    addLocal(*name);
}

static void addLocal(Token name) {
    if(current->localCount == UINT16_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    Local *local = &current->locals[current->localCount++];

    local->name = name;
    local->depth = -1;
}

static void markInitialized() {
    current->locals[current->localCount-1].depth = current->scopeDepth;
}

static bool identifiersEqual(Token *a, Token *b) {
    if(a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler *compiler, Token *name) {
    // try to find the local's index
    for(int i = compiler->localCount-1; i > -1; i--) {
        Local *local = &compiler->locals[i];

        if(identifiersEqual(&local->name, name)) {
            // local is uninitialized if its depth is still -1
            if(local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static int identifierConstant(Token *name) {
    writeValueArray(&currentChunk()->constants, OBJ_VAL(copyString(name->start, name->length)));

    // this should work...
    return currentChunk()->constants.count-1;
}

static ParseRule *getRule(TokenType type) {
    return &rules[type];
}

static void synchronize() {
    parser.panicMode = false;

    while(parser.current.type != TOKEN_EOF) {
        if(parser.previous.type == TOKEN_SEMICOLON) return;

        // if the next token is any of these, stop here.
        switch(parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default: ; // nothing
        }

        advance();
    }
}