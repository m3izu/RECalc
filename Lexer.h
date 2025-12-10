#pragma once
#include <string>
#include <vector>
#include <cctype>

enum TokenType { TOK_ID, TOK_NUMBER, TOK_PLUS, TOK_MINUS, TOK_TIMES, TOK_DIVIDE, TOK_LPAREN, TOK_RPAREN, TOK_END, TOK_INVALID };

struct Token {
    TokenType type;
    std::string value;
    int pos;
};

struct LexerError {
    std::string message;
    int position;
    std::string severity;  // "ERROR" or "WARNING"
};

class Lexer {
    std::string input;
    size_t pos = 0;
    
    bool isOperator(TokenType t) const {
        return t == TOK_PLUS || t == TOK_MINUS || t == TOK_TIMES || t == TOK_DIVIDE;
    }
    
    bool isOperand(TokenType t) const {
        return t == TOK_NUMBER || t == TOK_ID || t == TOK_RPAREN;
    }
    
    std::string tokenTypeName(TokenType t) const {
        switch(t) {
            case TOK_ID: return "ID";
            case TOK_NUMBER: return "NUMBER";
            case TOK_PLUS: return "PLUS";
            case TOK_MINUS: return "MINUS";
            case TOK_TIMES: return "TIMES";
            case TOK_DIVIDE: return "DIVIDE";
            case TOK_LPAREN: return "LPAREN";
            case TOK_RPAREN: return "RPAREN";
            case TOK_END: return "END";
            default: return "INVALID";
        }
    }
    
public:
    std::vector<std::string> steps;
    std::vector<Token> tokens;
    std::vector<LexerError> errors;
    bool hasErrors = false;

    Lexer() = default;
    explicit Lexer(const std::string &text) { setInput(text); }

    void setInput(const std::string &text) {
        input = text;
        pos = 0;
        steps.clear();
        tokens.clear();
        errors.clear();
        hasErrors = false;
        tokenizeAll();
        validate();
    }

    void tokenizeAll() {
        while (true) {
            Token t = nextTokenInternal();
            tokens.push_back(t);
            if (t.type == TOK_END) break;
        }
    }
    
    void validate() {
        if (tokens.empty()) return;
        
        int parenCount = 0;
        
        for (size_t i = 0; i < tokens.size(); i++) {
            const Token& tok = tokens[i];
            TokenType prev = (i > 0) ? tokens[i-1].type : TOK_END;
            
            // Check for invalid tokens - symbols not in NFA
            if (tok.type == TOK_INVALID) {
                std::string invalidMsg = "Symbol '" + tok.value + "' not recognized by DFA";
                char c = tok.value[0];
                
                // Provide helpful context based on NFA patterns
                if (c == '=' || c == '!' || c == '<' || c == '>') {
                    invalidMsg += " (comparison operators not supported)";
                } else if (c == '&' || c == '|' || c == '^') {
                    invalidMsg += " (logical/bitwise operators not supported)";
                } else if (c == '@' || c == '#' || c == '$' || c == '%') {
                    invalidMsg += " (special symbols not in NFA)";
                } else if (c == '[' || c == ']' || c == '{' || c == '}') {
                    invalidMsg += " (brackets not supported, use parentheses)";
                } else if (c == ';' || c == ':' || c == ',') {
                    invalidMsg += " (punctuation not supported)";
                } else if (c == '\\' || c == '`' || c == '~') {
                    invalidMsg += " (symbol not in lexer alphabet)";
                } else {
                    invalidMsg += " (valid: [a-zA-Z], [0-9], +, -, *, /, (, ))";
                }
                
                errors.push_back({invalidMsg, tok.pos, "ERROR"});
                hasErrors = true;
                steps.push_back("[ERROR] " + invalidMsg + " at position " + std::to_string(tok.pos));
            }
            
            // Check parentheses balance
            if (tok.type == TOK_LPAREN) parenCount++;
            if (tok.type == TOK_RPAREN) {
                parenCount--;
                if (parenCount < 0) {
                    errors.push_back({"Unmatched closing parenthesis ')'", tok.pos, "ERROR"});
                    hasErrors = true;
                    steps.push_back("[ERROR] Unmatched ')' at position " + std::to_string(tok.pos));
                }
            }
            
            // Check for adjacent operators (e.g., "++", "+-", "**")
            if (isOperator(tok.type) && isOperator(prev)) {
                // Exception: unary minus after operator is sometimes allowed, but we'll flag it
                if (tok.type == TOK_MINUS && (prev == TOK_PLUS || prev == TOK_MINUS)) {
                    errors.push_back({"Adjacent operators '" + tokens[i-1].value + tok.value + "' - possible unary minus", tok.pos, "WARNING"});
                    steps.push_back("[WARNING] Adjacent operators at position " + std::to_string(tok.pos));
                } else {
                    errors.push_back({"Invalid adjacent operators '" + tokens[i-1].value + tok.value + "'", tok.pos, "ERROR"});
                    hasErrors = true;
                    steps.push_back("[ERROR] Adjacent operators '" + tokens[i-1].value + tok.value + "' at position " + std::to_string(tok.pos));
                }
            }
            
            // Check for operator at start (except unary minus)
            if (i == 0 && isOperator(tok.type) && tok.type != TOK_MINUS) {
                errors.push_back({"Expression cannot start with operator '" + tok.value + "'", tok.pos, "ERROR"});
                hasErrors = true;
                steps.push_back("[ERROR] Expression cannot start with '" + tok.value + "'");
            }
            
            // Check for operator before closing paren
            if (tok.type == TOK_RPAREN && isOperator(prev)) {
                errors.push_back({"Operator '" + tokens[i-1].value + "' before closing parenthesis", tokens[i-1].pos, "ERROR"});
                hasErrors = true;
                steps.push_back("[ERROR] Operator before ')' at position " + std::to_string(tokens[i-1].pos));
            }
            
            // Check for opening paren after operand (missing operator)
            if (tok.type == TOK_LPAREN && isOperand(prev)) {
                errors.push_back({"Missing operator before '('", tok.pos, "ERROR"});
                hasErrors = true;
                steps.push_back("[ERROR] Missing operator before '(' at position " + std::to_string(tok.pos));
            }
            
            // Check for operand after closing paren (missing operator)
            if (isOperand(tok.type) && prev == TOK_RPAREN) {
                errors.push_back({"Missing operator after ')'", tok.pos, "ERROR"});
                hasErrors = true;
                steps.push_back("[ERROR] Missing operator after ')' at position " + std::to_string(tok.pos));
            }
            
            // Check for empty parens
            if (tok.type == TOK_RPAREN && prev == TOK_LPAREN) {
                errors.push_back({"Empty parentheses '()'", tok.pos, "ERROR"});
                hasErrors = true;
                steps.push_back("[ERROR] Empty parentheses at position " + std::to_string(tok.pos));
            }
        }
        
        // Check for unclosed parentheses
        if (parenCount > 0) {
            errors.push_back({"Unclosed parenthesis - missing " + std::to_string(parenCount) + " ')'", (int)input.length(), "ERROR"});
            hasErrors = true;
            steps.push_back("[ERROR] Unclosed parenthesis - missing " + std::to_string(parenCount) + " ')'");
        }
        
        // Check for operator at end
        if (tokens.size() >= 2) {
            TokenType lastReal = tokens[tokens.size() - 2].type;  // -2 because last is TOK_END
            if (isOperator(lastReal)) {
                errors.push_back({"Expression cannot end with operator '" + tokens[tokens.size() - 2].value + "'", tokens[tokens.size() - 2].pos, "ERROR"});
                hasErrors = true;
                steps.push_back("[ERROR] Expression cannot end with operator");
            }
        }
        
        // Summary
        if (!hasErrors && errors.empty()) {
            steps.push_back("[OK] Tokenization complete - no errors");
        } else if (!hasErrors && !errors.empty()) {
            steps.push_back("[WARNING] Tokenization complete with " + std::to_string(errors.size()) + " warning(s)");
        } else {
            steps.push_back("[FAILED] Tokenization complete with " + std::to_string(errors.size()) + " error(s)");
        }
    }

    Token nextTokenInternal() {
        while (pos < input.size() && std::isspace(static_cast<unsigned char>(input[pos]))) pos++;
        if (pos >= input.size()) return {TOK_END, "", (int)pos};
        char current = input[pos];
        
        // Identifiers: start with letter or underscore
        if (std::isalpha(static_cast<unsigned char>(current)) || current == '_') {
            std::string id;
            int start = (int)pos;
            while (pos < input.size() && (std::isalnum(static_cast<unsigned char>(input[pos])) || input[pos] == '_')) {
                id += input[pos++];
            }
            steps.push_back("[LEXER] ID -> " + id);
            return {TOK_ID, id, start};
        }
        
        // Numbers
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
