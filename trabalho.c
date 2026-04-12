#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TOK_EOF = -1,
    TOK_INVALID = 0,
    TOK_PRINCIPAL,
    TOK_IDENTIFICADOR,
    TOK_INT,
    TOK_CAR,
    TOK_LEIA,
    TOK_ESCREVA,
    TOK_NOVALINHA,
    TOK_SE,
    TOK_ENTAO,
    TOK_SENAO,
    TOK_FIMSE,
    TOK_ENQUANTO,
    TOK_CADEIACARACTERES,
    TOK_OU,
    TOK_E,
    TOK_IGUAL,
    TOK_DIFERENTE,
    TOK_MAIORIGUAL,
    TOK_MENORIGUAL,
    TOK_CARCONST,
    TOK_INTCONST,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_COLON,
    TOK_SEMICOLON,
    TOK_COMMA,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_NOT,
    TOK_LT,
    TOK_GT,
    TOK_ASSIGN
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[1024];
    int line;
} Token;

typedef struct {
    long pos;
    int current_char;
    int line;
    TokenType previous_token;
} LexerState;

typedef struct {
    LexerState lexer;
    Token token;
} ParserState;

static FILE *yyin = NULL;
static int yylineno = 1;
static int current_char = EOF;
static TokenType previous_token = TOK_INVALID;
static Token current_token;

static __attribute__((noreturn)) void report_error_and_exit(const char *message, int line) {
    printf("ERRO: %s %d\n", message, line);
    exit(EXIT_FAILURE);
}

static void consume_char(void) {
    if (current_char == '\n') {
        yylineno++;
    }
    current_char = fgetc(yyin);
}

static bool token_can_end_expression(TokenType type) {
    return type == TOK_IDENTIFICADOR || type == TOK_INTCONST || type == TOK_CARCONST || type == TOK_RPAREN;
}

static LexerState save_lexer_state(void) {
    LexerState state;
    state.pos = ftell(yyin);
    state.current_char = current_char;
    state.line = yylineno;
    state.previous_token = previous_token;
    return state;
}

static void restore_lexer_state(LexerState state) {
    fseek(yyin, state.pos, SEEK_SET);
    current_char = state.current_char;
    yylineno = state.line;
    previous_token = state.previous_token;
}

static ParserState save_parser_state(void) {
    ParserState state;
    state.lexer = save_lexer_state();
    state.token = current_token;
    return state;
}

static void restore_parser_state(ParserState state) {
    restore_lexer_state(state.lexer);
    current_token = state.token;
}

static Token make_token(TokenType type, const char *lexeme, int line) {
    Token token;
    token.type = type;
    token.line = line;
    if (lexeme != NULL) {
        strncpy(token.lexeme, lexeme, sizeof(token.lexeme) - 1);
        token.lexeme[sizeof(token.lexeme) - 1] = '\0';
    } else {
        token.lexeme[0] = '\0';
    }
    previous_token = type;
    return token;
}

static void skip_whitespace_and_comments(void) {
    for (;;) {
        while (current_char != EOF && isspace(current_char)) {
            consume_char();
        }

        if (current_char == '/') {
            LexerState state = save_lexer_state();
            consume_char();
            if (current_char == '*') {
                int comment_start_line = yylineno;
                consume_char();

                int prev = 0;
                bool closed = false;
                while (current_char != EOF) {
                    if (prev == '*' && current_char == '/') {
                        consume_char();
                        closed = true;
                        break;
                    }
                    prev = current_char;
                    consume_char();
                }

                if (!closed) {
                    report_error_and_exit("COMENTARIO NAO TERMINA", comment_start_line);
                }
                continue;
            }

            restore_lexer_state(state);
        }

        break;
    }
}

static Token lex_identifier_or_keyword(void) {
    char buffer[1024];
    size_t len = 0;
    int line = yylineno;

    while (current_char != EOF && (isalnum(current_char) || current_char == '_')) {
        if (len + 1 < sizeof(buffer)) {
            buffer[len++] = (char)current_char;
        }
        consume_char();
    }
    buffer[len] = '\0';

    if (strcmp(buffer, "principal") == 0) return make_token(TOK_PRINCIPAL, buffer, line);
    if (strcmp(buffer, "int") == 0) return make_token(TOK_INT, buffer, line);
    if (strcmp(buffer, "car") == 0) return make_token(TOK_CAR, buffer, line);
    if (strcmp(buffer, "leia") == 0) return make_token(TOK_LEIA, buffer, line);
    if (strcmp(buffer, "escreva") == 0) return make_token(TOK_ESCREVA, buffer, line);
    if (strcmp(buffer, "novalinha") == 0) return make_token(TOK_NOVALINHA, buffer, line);
    if (strcmp(buffer, "se") == 0) return make_token(TOK_SE, buffer, line);
    if (strcmp(buffer, "entao") == 0) return make_token(TOK_ENTAO, buffer, line);
    if (strcmp(buffer, "senao") == 0) return make_token(TOK_SENAO, buffer, line);
    if (strcmp(buffer, "fimse") == 0) return make_token(TOK_FIMSE, buffer, line);
    if (strcmp(buffer, "enquanto") == 0) return make_token(TOK_ENQUANTO, buffer, line);

    return make_token(TOK_IDENTIFICADOR, buffer, line);
}

static Token lex_intconst(bool signed_number) {
    char buffer[1024];
    size_t len = 0;
    int line = yylineno;

    if (signed_number && (current_char == '+' || current_char == '-')) {
        buffer[len++] = (char)current_char;
        consume_char();
    }

    while (current_char != EOF && isdigit(current_char)) {
        if (len + 1 < sizeof(buffer)) {
            buffer[len++] = (char)current_char;
        }
        consume_char();
    }
    buffer[len] = '\0';

    return make_token(TOK_INTCONST, buffer, line);
}

static Token lex_string(void) {
    char buffer[1024];
    size_t len = 0;
    int start_line = yylineno;

    consume_char();
    while (current_char != EOF && current_char != '"') {
        if (current_char == '\n') {
            report_error_and_exit("CADEIA DE CARACTERES OCUPA MAIS DE UMA LINHA", start_line);
        }

        if (len + 1 < sizeof(buffer)) {
            buffer[len++] = (char)current_char;
        }
        consume_char();
    }

    if (current_char != '"') {
        report_error_and_exit("CADEIA DE CARACTERES OCUPA MAIS DE UMA LINHA", start_line);
    }

    consume_char();
    buffer[len] = '\0';
    return make_token(TOK_CADEIACARACTERES, buffer, start_line);
}

static Token lex_charconst(void) {
    char buffer[16];
    size_t len = 0;
    int line = yylineno;

    buffer[len++] = '\'';
    consume_char();

    if (current_char == EOF || current_char == '\n') {
        report_error_and_exit("CARACTERE INVALIDO", line);
    }

    if (current_char == '\\') {
        buffer[len++] = '\\';
        consume_char();
        if (current_char == EOF || current_char == '\n') {
            report_error_and_exit("CARACTERE INVALIDO", line);
        }
        if (len + 1 < sizeof(buffer)) {
            buffer[len++] = (char)current_char;
        }
        consume_char();
    } else {
        if (len + 1 < sizeof(buffer)) {
            buffer[len++] = (char)current_char;
        }
        consume_char();
    }

    if (current_char != '\'') {
        report_error_and_exit("CARACTERE INVALIDO", line);
    }

    if (len + 1 < sizeof(buffer)) {
        buffer[len++] = '\'';
    }
    consume_char();
    buffer[len] = '\0';

    return make_token(TOK_CARCONST, buffer, line);
}

static Token next_token(void) {
    skip_whitespace_and_comments();

    if (current_char == EOF) {
        return make_token(TOK_EOF, "", yylineno);
    }

    if (isalpha(current_char) || current_char == '_') {
        return lex_identifier_or_keyword();
    }

    if (isdigit(current_char)) {
        return lex_intconst(false);
    }

    if (current_char == '+' || current_char == '-') {
        int line = yylineno;
        int lookahead = fgetc(yyin);
        if (lookahead != EOF) {
            ungetc(lookahead, yyin);
        }

        if (lookahead != EOF && isdigit(lookahead) && !token_can_end_expression(previous_token)) {
            return lex_intconst(true);
        }

        if (current_char == '+') {
            consume_char();
            return make_token(TOK_PLUS, "+", line);
        }

        consume_char();
        return make_token(TOK_MINUS, "-", line);
    }

    if (current_char == '"') {
        return lex_string();
    }

    if (current_char == '\'') {
        return lex_charconst();
    }

    int line = yylineno;
    switch (current_char) {
        case '{': consume_char(); return make_token(TOK_LBRACE, "{", line);
        case '}': consume_char(); return make_token(TOK_RBRACE, "}", line);
        case '(': consume_char(); return make_token(TOK_LPAREN, "(", line);
        case ')': consume_char(); return make_token(TOK_RPAREN, ")", line);
        case ':': consume_char(); return make_token(TOK_COLON, ":", line);
        case ';': consume_char(); return make_token(TOK_SEMICOLON, ";", line);
        case ',': consume_char(); return make_token(TOK_COMMA, ",", line);
        case '*': consume_char(); return make_token(TOK_STAR, "*", line);
        case '/': consume_char(); return make_token(TOK_SLASH, "/", line);
        case '<':
            consume_char();
            if (current_char == '=') {
                consume_char();
                return make_token(TOK_MENORIGUAL, "<=", line);
            }
            return make_token(TOK_LT, "<", line);
        case '>':
            consume_char();
            if (current_char == '=') {
                consume_char();
                return make_token(TOK_MAIORIGUAL, ">=", line);
            }
            return make_token(TOK_GT, ">", line);
        case '!':
            consume_char();
            if (current_char == '=') {
                consume_char();
                return make_token(TOK_DIFERENTE, "!=", line);
            }
            return make_token(TOK_NOT, "!", line);
        case '=':
            consume_char();
            if (current_char == '=') {
                consume_char();
                return make_token(TOK_IGUAL, "==", line);
            }
            return make_token(TOK_ASSIGN, "=", line);
        case '|':
            consume_char();
            if (current_char == '|') {
                consume_char();
                return make_token(TOK_OU, "||", line);
            }
            report_error_and_exit("CARACTERE INVALIDO", line);
        case '&':
            consume_char();
            return make_token(TOK_E, "&", line);
        default:
            report_error_and_exit("CARACTERE INVALIDO", line);
    }

    return make_token(TOK_INVALID, "", yylineno);
}

static void advance_token(void) {
    current_token = next_token();
}

static void syntax_error(void) {
    report_error_and_exit("ERRO SINTATICO", current_token.line);
}

static void expect(TokenType type) {
    if (current_token.type != type) {
        syntax_error();
    }
    advance_token();
}

static bool is_command_start(TokenType type) {
    return type == TOK_SEMICOLON || type == TOK_IDENTIFICADOR || type == TOK_LEIA ||
           type == TOK_ESCREVA || type == TOK_NOVALINHA || type == TOK_SE ||
           type == TOK_ENQUANTO || type == TOK_LBRACE;
}

static void parse_expr(void);

static void parse_prim_expr(void) {
    if (current_token.type == TOK_IDENTIFICADOR || current_token.type == TOK_CARCONST ||
        current_token.type == TOK_INTCONST) {
        advance_token();
        return;
    }

    if (current_token.type == TOK_LPAREN) {
        advance_token();
        parse_expr();
        expect(TOK_RPAREN);
        return;
    }

    syntax_error();
}

static void parse_un_expr(void) {
    if (current_token.type == TOK_MINUS || current_token.type == TOK_NOT) {
        advance_token();
        parse_prim_expr();
        return;
    }

    parse_prim_expr();
}

static void parse_mul_expr(void) {
    parse_un_expr();
    while (current_token.type == TOK_STAR || current_token.type == TOK_SLASH) {
        advance_token();
        parse_un_expr();
    }
}

static void parse_add_expr(void) {
    parse_mul_expr();
    while (current_token.type == TOK_PLUS || current_token.type == TOK_MINUS) {
        advance_token();
        parse_mul_expr();
    }
}

static void parse_desig_expr(void) {
    parse_add_expr();
    while (current_token.type == TOK_LT || current_token.type == TOK_GT ||
           current_token.type == TOK_MAIORIGUAL || current_token.type == TOK_MENORIGUAL) {
        advance_token();
        parse_add_expr();
    }
}

static void parse_eq_expr(void) {
    parse_desig_expr();
    while (current_token.type == TOK_IGUAL || current_token.type == TOK_DIFERENTE) {
        advance_token();
        parse_desig_expr();
    }
}

static void parse_and_expr(void) {
    parse_eq_expr();
    while (current_token.type == TOK_E) {
        advance_token();
        parse_eq_expr();
    }
}

static void parse_or_expr(void) {
    parse_and_expr();
    while (current_token.type == TOK_OU) {
        advance_token();
        parse_and_expr();
    }
}

static void parse_expr(void) {
    if (current_token.type == TOK_IDENTIFICADOR) {
        ParserState state = save_parser_state();
        advance_token();
        if (current_token.type == TOK_ASSIGN) {
            restore_parser_state(state);
            expect(TOK_IDENTIFICADOR);
            expect(TOK_ASSIGN);
            parse_expr();
            return;
        }
        restore_parser_state(state);
    }

    parse_or_expr();
}

static void parse_tipo(void) {
    if (current_token.type == TOK_INT || current_token.type == TOK_CAR) {
        advance_token();
        return;
    }
    syntax_error();
}

static void parse_decl_var_tail(void) {
    while (current_token.type == TOK_COMMA) {
        advance_token();
        expect(TOK_IDENTIFICADOR);
    }
}

static void parse_single_decl(void) {
    expect(TOK_IDENTIFICADOR);
    parse_decl_var_tail();
    expect(TOK_COLON);
    parse_tipo();
    expect(TOK_SEMICOLON);
}

static void parse_lista_decl_var(void) {
    parse_single_decl();
    while (current_token.type == TOK_IDENTIFICADOR) {
        parse_single_decl();
    }
}

static bool looks_like_var_section_after_first_lbrace(void) {
    if (current_token.type != TOK_IDENTIFICADOR) {
        return false;
    }

    ParserState state = save_parser_state();
    bool is_var_section = true;

    if (current_token.type != TOK_IDENTIFICADOR) {
        is_var_section = false;
    }

    while (is_var_section) {
        if (current_token.type != TOK_IDENTIFICADOR) {
            is_var_section = false;
            break;
        }
        advance_token();

        while (current_token.type == TOK_COMMA) {
            advance_token();
            if (current_token.type != TOK_IDENTIFICADOR) {
                is_var_section = false;
                break;
            }
            advance_token();
        }
        if (!is_var_section) {
            break;
        }

        if (current_token.type != TOK_COLON) {
            is_var_section = false;
            break;
        }
        advance_token();

        if (current_token.type != TOK_INT && current_token.type != TOK_CAR) {
            is_var_section = false;
            break;
        }
        advance_token();

        if (current_token.type != TOK_SEMICOLON) {
            is_var_section = false;
            break;
        }
        advance_token();

        if (current_token.type != TOK_IDENTIFICADOR) {
            break;
        }
    }

    if (is_var_section && current_token.type == TOK_RBRACE) {
        advance_token();
        is_var_section = (current_token.type == TOK_LBRACE);
    } else {
        is_var_section = false;
    }

    restore_parser_state(state);
    return is_var_section;
}

static void parse_comando(void);

static void parse_lista_comando(void) {
    if (!is_command_start(current_token.type)) {
        syntax_error();
    }

    do {
        parse_comando();
    } while (is_command_start(current_token.type));
}

static void parse_bloco(void) {
    expect(TOK_LBRACE);

    if (looks_like_var_section_after_first_lbrace()) {
        parse_lista_decl_var();
        expect(TOK_RBRACE);
        expect(TOK_LBRACE);
        parse_lista_comando();
        expect(TOK_RBRACE);
        return;
    }

    parse_lista_comando();
    expect(TOK_RBRACE);
}

static void parse_comando(void) {
    switch (current_token.type) {
        case TOK_SEMICOLON:
            advance_token();
            break;
        case TOK_IDENTIFICADOR:
            parse_expr();
            expect(TOK_SEMICOLON);
            break;
        case TOK_LEIA:
            advance_token();
            expect(TOK_IDENTIFICADOR);
            expect(TOK_SEMICOLON);
            break;
        case TOK_ESCREVA:
            advance_token();
            if (current_token.type == TOK_CADEIACARACTERES) {
                advance_token();
            } else {
                parse_expr();
            }
            expect(TOK_SEMICOLON);
            break;
        case TOK_NOVALINHA:
            advance_token();
            expect(TOK_SEMICOLON);
            break;
        case TOK_SE:
            advance_token();
            expect(TOK_LPAREN);
            parse_expr();
            expect(TOK_RPAREN);
            expect(TOK_ENTAO);
            parse_comando();
            if (current_token.type == TOK_SENAO) {
                advance_token();
                parse_comando();
            }
            expect(TOK_FIMSE);
            break;
        case TOK_ENQUANTO:
            advance_token();
            expect(TOK_LPAREN);
            parse_expr();
            expect(TOK_RPAREN);
            parse_comando();
            break;
        case TOK_LBRACE:
            parse_bloco();
            break;
        default:
            syntax_error();
    }
}

static void parse_programa(void) {
    expect(TOK_PRINCIPAL);
    parse_bloco();
    expect(TOK_EOF);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo.g>\n", argv[0]);
        return EXIT_FAILURE;
    }

    yyin = fopen(argv[1], "r");
    if (yyin == NULL) {
        perror("Erro ao abrir arquivo");
        return EXIT_FAILURE;
    }

    current_char = fgetc(yyin);
    advance_token();
    parse_programa();

    fclose(yyin);
    return EXIT_SUCCESS;
}
