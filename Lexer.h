#pragma once
#include <string>
#include <vector>
#include <cctype>

enum TokenType { TOK_NUMBER, TOK_PLUS, TOK_MINUS, TOK_TIMES, TOK_DIVIDE, TOK_LPAREN, TOK_RPAREN, TOK_END, TOK_INVALID };

struct Token {
    TokenType type;
    std::string value;
    int pos;
};

class Lexer {
    std::string input;
    size_t pos = 0;
public:
    std::vector<std::string> steps;
    std::vector<Token> tokens; // full token stream

    Lexer() = default;
    explicit Lexer(const std::string &text) { setInput(text); }

    void setInput(const std::string &text) {
        input = text;
        pos = 0;
        steps.clear();
        tokens.clear();
        tokenizeAll();
    }

    void tokenizeAll() {
        while (true) {
            Token t = nextTokenInternal();
            tokens.push_back(t);
            if (t.type == TOK_END) break;
        }
    }

    Token nextTokenInternal() {
        while (pos < input.size() && std::isspace(static_cast<unsigned char>(input[pos]))) pos++;
        if (pos >= input.size()) return {TOK_END, "", (int)pos};
        char current = input[pos];
        if (std::isdigit(static_cast<unsigned char>(current)) || current == '.') {
            std::string num;
            int start = (int)pos;
            bool seenDot = false;
            while (pos < input.size() && (std::isdigit(static_cast<unsigned char>(input[pos])) || (!seenDot && input[pos] == '.'))) {
                if (input[pos] == '.') seenDot = true;
                num += input[pos++];
            }
            steps.push_back("[LEXER] NUMBER -> " + num);
            return {TOK_NUMBER, num, start};
        }
        pos++;
        std::string sym(1, current);
        switch (current) {
            case '+': steps.push_back("[LEXER] PLUS -> +"); return {TOK_PLUS, sym, (int)pos-1};
            case '-': steps.push_back("[LEXER] MINUS -> -"); return {TOK_MINUS, sym, (int)pos-1};
            case '*': steps.push_back("[LEXER] TIMES -> *"); return {TOK_TIMES, sym, (int)pos-1};
            case '/': steps.push_back("[LEXER] DIVIDE -> /"); return {TOK_DIVIDE, sym, (int)pos-1};
            case '(' : steps.push_back("[LEXER] LPAREN -> ("); return {TOK_LPAREN, sym, (int)pos-1};
            case ')' : steps.push_back("[LEXER] RPAREN -> )"); return {TOK_RPAREN, sym, (int)pos-1};
            default: steps.push_back("[LEXER] INVALID -> " + sym); return {TOK_INVALID, sym, (int)pos-1};
        }
    }
};
