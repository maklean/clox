#include "../include/common.h"
#include "../include/scanner.h"

#include <string.h>

typedef struct {
    const char *start;
    const char *current;
    int line;
} Scanner;

Scanner scanner;

// returns whether the scanner is at the end of the source.
static bool isAtEnd();

// Creates a token with the given type (uses scanner's current position)
static Token makeToken(TokenType type);

// Creates an error token with the given message.
static Token errorToken(const char *message);

// Consumes the currrent character and advances
static char advance();

// Consumes the current character if it matches the expected character.
static bool match(char expected);

// Skips all the whitespace-like characters.
static void skipWhitespace();

// Returns the current character.
static char peek();

// Returns the character after the current character.
static char peekNext();

// Scans a string literal
static Token string();

// Scans a number literal
static Token number();

// Scans an identifier
static Token identifier();

// Returns the type of the scanned identifier.
static TokenType identifierType();

// Returns whether the current identifier matches the given reserved identifier type.
static TokenType checkKeyword(int start, int length, const char* rest, TokenType type);

// Returns whether a character is an alphabetical character (also allows '_' for identifiers).
static bool isAlpha(char c);

// Returns whether a character is a digit.
static bool isDigit(char c);

void initScanner(const char *source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

Token scanToken() {
    skipWhitespace();
    
    scanner.start = scanner.current;

    if(isAtEnd()) return makeToken(TOKEN_EOF);

    char c = advance();

    if(isAlpha(c)) return identifier();
    if(isDigit(c)) return number();

    switch(c) {
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case '/': return makeToken(TOKEN_SLASH);
        case '*': {
            if(match('*')) return makeToken(TOKEN_STAR_STAR);
            return makeToken(TOKEN_STAR);
        }
        case '%': return makeToken(TOKEN_PERCENT);
        case '!':
            return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"': return string();
    }

    return errorToken("Unexpected character.");
}

static bool isAtEnd() {
    return *scanner.current == '\0';
}

static Token makeToken(TokenType type) {
    Token token;

    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    
    return token;
}

static Token errorToken(const char *message) {
    Token token;

    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;

    return token;
}

static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

static bool match(char expected) {
    if(isAtEnd() || peek() != expected) return false;
    scanner.current++;
    return true;
}

static void skipWhitespace() {
    for(;;) {
        char c = peek();

        switch(c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '/':
                if(peekNext() == '/') {
                    // skip entire line if it's a comment
                    while(peek() != '\n' && !isAtEnd()) advance();
                } else if(peekNext() == '*') {
                    // multi-line comment
                    advance();
                    advance();

                    while(!isAtEnd() && !(peek() == '*' && peekNext() == '/')) {
                        if(peek() == '\n') scanner.line++;
                        advance();
                    }

                    if(isAtEnd()) {
                        errorToken("Unterminated block comment.");
                        return;
                    }

                    // consume '*' and '/'
                    advance();
                    advance();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static char peek() {
    return *scanner.current;
}

static char peekNext() {
    if(isAtEnd()) return '\0';
    return scanner.current[1];
}

static Token string() {
    while(peek() != '"' && !isAtEnd()) {
        if(peek() == '\n') scanner.line++;
        advance();
    }

    if(isAtEnd()) return errorToken("Unterminated string.");

    advance(); // consume closing quote
    return makeToken(TOKEN_STRING);
}

static Token number() {
    while(isDigit(peek())) advance();

    // look for fractional part
    if(peek() == '.' && isDigit(peekNext())) {
        advance(); // consume .

        while(isDigit(peek())) advance();
    }

    return makeToken(TOKEN_NUMBER);
}

static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

static Token identifier() {
    while(isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}

static TokenType identifierType() {
    switch(scanner.start[0]) {
        case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'b': return checkKeyword(1, 4, "reak", TOKEN_BREAK);
        case 'c':
            if(scanner.current - scanner.start > 1) {
                switch(scanner.start[1]) {
                    case 'l': return checkKeyword(2, 3, "ass", TOKEN_CLASS);
                    case 'o': return checkKeyword(2, 6, "ntinue", TOKEN_CONTINUE);
                }
            }
            break;
        case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
        case 'f':
            if(scanner.current - scanner.start > 1) {
                // try to match false, for, or fun
                switch(scanner.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
        case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
        case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
        case 't':
            if(scanner.current - scanner.start > 1) {
                // try to match this, or true
                switch(scanner.start[1]) {
                    case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static TokenType checkKeyword(int start, int length, const char *rest, TokenType type) {
    // try to match length and characters
    if(
        scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0
    ) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}