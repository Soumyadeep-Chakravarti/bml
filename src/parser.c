#include "internal.h"

#include <stdint.h>
#include <stdlib.h>

typedef enum {
    TOKEN_END,
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_EQUAL,
    TOKEN_APPROACHES,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_CARET,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_COMMA,
    TOKEN_INVALID
} TokenKind;

typedef struct {
    TokenKind kind;
    const char *start;
    size_t length;
    size_t offset;
} Token;

typedef struct {
    const char *input;
    size_t offset;
    Token current;
    BML_Context *ctx;
} Parser;

typedef struct {
    BML_Expr **items;
    size_t count;
    size_t capacity;
} ExprVector;

static int is_ascii_digit(unsigned char ch) {
    return ch >= '0' && ch <= '9';
}

static int is_ascii_letter(unsigned char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

static int is_ascii_space(unsigned char ch) {
    return ch == ' ' || (ch >= '\t' && ch <= '\r');
}

static Token scan_token(Parser *parser) {
    while (is_ascii_space((unsigned char)parser->input[parser->offset])) {
        parser->offset++;
    }

    const size_t offset = parser->offset;
    const char *start = parser->input + offset;
    const unsigned char current = (unsigned char)*start;
    Token token = {TOKEN_END, start, 0, offset};

    if (current == '\0') {
        return token;
    }

    if (is_ascii_digit(current) ||
        (current == '.' && is_ascii_digit((unsigned char)start[1]))) {
        size_t length = 0;
        while (is_ascii_digit((unsigned char)start[length])) {
            length++;
        }
        if (start[length] == '.') {
            length++;
            while (is_ascii_digit((unsigned char)start[length])) {
                length++;
            }
        }
        if (start[length] == 'e' || start[length] == 'E') {
            size_t exponent_length = length + 1;
            if (start[exponent_length] == '+' || start[exponent_length] == '-') {
                exponent_length++;
            }
            const size_t digits_start = exponent_length;
            while (is_ascii_digit((unsigned char)start[exponent_length])) {
                exponent_length++;
            }
            if (exponent_length > digits_start) {
                length = exponent_length;
            }
        }
        parser->offset += length;
        token.kind = TOKEN_NUMBER;
        token.length = length;
        return token;
    }

    if (is_ascii_letter(current) || current == '_') {
        size_t length = 1;
        while (is_ascii_letter((unsigned char)start[length]) ||
               is_ascii_digit((unsigned char)start[length]) ||
               start[length] == '_') {
            length++;
        }
        parser->offset += length;
        token.kind = TOKEN_IDENTIFIER;
        token.length = length;
        return token;
    }

    if (start[0] == '-' && start[1] == '>') {
        parser->offset += 2;
        token.kind = TOKEN_APPROACHES;
        token.length = 2;
        return token;
    }

    parser->offset++;
    token.length = 1;
    switch (*start) {
    case '+': token.kind = TOKEN_PLUS; break;
    case '-': token.kind = TOKEN_MINUS; break;
    case '=': token.kind = TOKEN_EQUAL; break;
    case '*': token.kind = TOKEN_STAR; break;
    case '/': token.kind = TOKEN_SLASH; break;
    case '^': token.kind = TOKEN_CARET; break;
    case '(': token.kind = TOKEN_LEFT_PAREN; break;
    case ')': token.kind = TOKEN_RIGHT_PAREN; break;
    case '{': token.kind = TOKEN_LEFT_BRACE; break;
    case '}': token.kind = TOKEN_RIGHT_BRACE; break;
    case ',': token.kind = TOKEN_COMMA; break;
    default: token.kind = TOKEN_INVALID; break;
    }
    return token;
}

static void advance(Parser *parser) {
    parser->current = scan_token(parser);
    if (parser->current.kind == TOKEN_INVALID) {
        bml_set_error(parser->ctx, BML_ERROR_PARSE, parser->current.offset);
    }
}

static int accept(Parser *parser, TokenKind kind) {
    if (parser->current.kind != kind) {
        return 0;
    }
    advance(parser);
    return 1;
}

static void parse_error(Parser *parser) {
    bml_set_error(parser->ctx, BML_ERROR_PARSE, parser->current.offset);
}

static BML_Expr *parse_expression(Parser *parser);
static BML_Expr *parse_unary(Parser *parser);

static BML_Expr *make_binary(Parser *parser, BML_ExprKind kind,
                             BML_Expr *left, BML_Expr *right) {
    BML_Expr *children[] = {left, right};
    return bml_expr_create(parser->ctx, kind, NULL, NULL, children, 2);
}

static int vector_append(Parser *parser, ExprVector *vector, BML_Expr *expr) {
    if (vector->count == vector->capacity) {
        const size_t capacity = vector->capacity == 0 ? 4 : vector->capacity * 2;
        if (capacity < vector->capacity || capacity > SIZE_MAX / sizeof(*vector->items)) {
            bml_set_error(parser->ctx, BML_ERROR_OUT_OF_MEMORY, parser->current.offset);
            return 0;
        }
        BML_Expr **items = realloc(vector->items, capacity * sizeof(*items));
        if (items == NULL) {
            bml_set_error(parser->ctx, BML_ERROR_OUT_OF_MEMORY, parser->current.offset);
            return 0;
        }
        vector->items = items;
        vector->capacity = capacity;
    }
    vector->items[vector->count++] = expr;
    return 1;
}

static BML_Expr *parse_call(Parser *parser, char *name) {
    ExprVector arguments = {NULL, 0, 0};
    advance(parser); /* '(' */

    if (parser->current.kind != TOKEN_RIGHT_PAREN) {
        for (;;) {
            BML_Expr *argument = parse_expression(parser);
            if (argument == NULL || !vector_append(parser, &arguments, argument)) {
                free(arguments.items);
                return NULL;
            }
            if (accept(parser, TOKEN_COMMA)) {
                continue;
            }
            /* Adjacent braced arguments support forms such as frac({a}{b}). */
            if (parser->current.kind == TOKEN_LEFT_BRACE &&
                arguments.count > 0) {
                continue;
            }
            break;
        }
    }

    if (!accept(parser, TOKEN_RIGHT_PAREN)) {
        parse_error(parser);
        free(arguments.items);
        return NULL;
    }

    BML_Expr *call = bml_expr_create(parser->ctx, BML_EXPR_CALL, name, NULL,
                                      arguments.items, arguments.count);
    free(arguments.items);
    return call;
}

static BML_Expr *parse_primary(Parser *parser) {
    const Token token = parser->current;
    if (token.kind == TOKEN_NUMBER) {
        char *text = bml_arena_copy(parser->ctx, token.start, token.length);
        if (text == NULL) {
            return NULL;
        }
        advance(parser);
        return bml_expr_create(parser->ctx, BML_EXPR_NUMBER, NULL, text, NULL, 0);
    }

    if (token.kind == TOKEN_IDENTIFIER) {
        char *name = bml_arena_copy(parser->ctx, token.start, token.length);
        if (name == NULL) {
            return NULL;
        }
        advance(parser);
        if (parser->current.kind == TOKEN_LEFT_PAREN) {
            return parse_call(parser, name);
        }
        return bml_expr_create(parser->ctx, BML_EXPR_SYMBOL, name, NULL, NULL, 0);
    }

    if (token.kind == TOKEN_LEFT_PAREN || token.kind == TOKEN_LEFT_BRACE) {
        const TokenKind closing = token.kind == TOKEN_LEFT_PAREN
                                      ? TOKEN_RIGHT_PAREN
                                      : TOKEN_RIGHT_BRACE;
        advance(parser);
        BML_Expr *expr = parse_expression(parser);
        if (expr == NULL || !accept(parser, closing)) {
            parse_error(parser);
            return NULL;
        }
        return expr;
    }

    parse_error(parser);
    return NULL;
}

static BML_Expr *parse_power(Parser *parser) {
    BML_Expr *base = parse_primary(parser);
    if (base == NULL || !accept(parser, TOKEN_CARET)) {
        return base;
    }

    BML_Expr *exponent = parse_unary(parser);
    if (exponent == NULL) {
        return NULL;
    }
    return make_binary(parser, BML_EXPR_POWER, base, exponent);
}

static BML_Expr *parse_unary(Parser *parser) {
    if (accept(parser, TOKEN_MINUS)) {
        BML_Expr *child = parse_unary(parser);
        if (child == NULL) {
            return NULL;
        }
        return bml_expr_create(parser->ctx, BML_EXPR_NEGATE, NULL, NULL,
                               &child, 1);
    }
    if (accept(parser, TOKEN_PLUS)) {
        return parse_unary(parser);
    }
    return parse_power(parser);
}

static BML_Expr *parse_multiplicative(Parser *parser) {
    BML_Expr *left = parse_unary(parser);
    while (left != NULL && (parser->current.kind == TOKEN_STAR ||
                            parser->current.kind == TOKEN_SLASH)) {
        const TokenKind operation = parser->current.kind;
        advance(parser);
        BML_Expr *right = parse_unary(parser);
        if (right == NULL) {
            return NULL;
        }
        left = make_binary(parser,
                           operation == TOKEN_STAR ? BML_EXPR_MULTIPLY
                                                   : BML_EXPR_DIVIDE,
                           left, right);
    }
    return left;
}

static BML_Expr *parse_additive(Parser *parser) {
    BML_Expr *left = parse_multiplicative(parser);
    while (left != NULL && (parser->current.kind == TOKEN_PLUS ||
                            parser->current.kind == TOKEN_MINUS)) {
        const TokenKind operation = parser->current.kind;
        advance(parser);
        BML_Expr *right = parse_multiplicative(parser);
        if (right == NULL) {
            return NULL;
        }
        left = make_binary(parser,
                           operation == TOKEN_PLUS ? BML_EXPR_ADD
                                                   : BML_EXPR_SUBTRACT,
                           left, right);
    }
    return left;
}

static BML_Expr *parse_expression(Parser *parser) {
    BML_Expr *left = parse_additive(parser);
    if (left == NULL || (parser->current.kind != TOKEN_EQUAL &&
                         parser->current.kind != TOKEN_APPROACHES)) {
        return left;
    }

    const TokenKind operation = parser->current.kind;
    advance(parser);
    BML_Expr *right = parse_additive(parser);
    if (right == NULL) {
        return NULL;
    }
    return make_binary(parser,
                       operation == TOKEN_EQUAL ? BML_EXPR_EQUAL
                                                : BML_EXPR_APPROACHES,
                       left, right);
}

BML_Expr *bml_parse(BML_Context *ctx, const char *input) {
    if (ctx == NULL) {
        return NULL;
    }
    bml_context_clear_error(ctx);
    if (input == NULL) {
        bml_set_error(ctx, BML_ERROR_INVALID_ARGUMENT, 0);
        return NULL;
    }

    Parser parser = {input, 0, {TOKEN_END, input, 0, 0}, ctx};
    advance(&parser);
    BML_Expr *expr = parse_expression(&parser);
    if (expr == NULL) {
        if (ctx->error.code == BML_ERROR_NONE) {
            parse_error(&parser);
        }
        return NULL;
    }
    if (parser.current.kind != TOKEN_END) {
        parse_error(&parser);
        return NULL;
    }
    return expr;
}
