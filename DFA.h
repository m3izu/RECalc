#pragma once
#include "Lexer.h"
#include <vector>
#include <string>
#include <map>
#include <cctype>

// DFA States for Tokenization (Supporting Identifiers and Floats)
enum class DFAState {
    START,
    NUMBER,
    POINT,      // Decimal point in float
    FRACTION,   // Digits after decimal point
    IDENTIFIER, // Variable names
    OP_PLUS,
    OP_MINUS,
    OP_TIMES,
    OP_DIVIDE,
    LPAREN,
    RPAREN,
    ERROR
};

struct DFAStep {
    DFAState fromState;
    char inputChar;
    DFAState toState;
    std::string tokenEmitted; 
    std::string explanation;
};

class DFALexer {
public:
    std::vector<Token> tokens;
    std::vector<DFAStep> history;

    static std::string stateToString(DFAState s) {
        switch(s) {
            case DFAState::START: return "START";
            case DFAState::NUMBER: return "NUMBER";
            case DFAState::POINT: return "POINT";
            case DFAState::FRACTION: return "FRACTION";
            case DFAState::IDENTIFIER: return "IDENTIFIER";
            case DFAState::OP_PLUS: return "OP_PLUS";
            case DFAState::OP_MINUS: return "OP_MINUS";
            case DFAState::OP_TIMES: return "OP_TIMES";
            case DFAState::OP_DIVIDE: return "OP_DIVIDE";
            case DFAState::LPAREN: return "LPAREN";
            case DFAState::RPAREN: return "RPAREN";
            case DFAState::ERROR: return "ERROR";
        }
        return "UNKNOWN";
    }

    void tokenize(const std::string& input) {
        tokens.clear();
        history.clear();
        
        DFAState currentState = DFAState::START;
        std::string currentLexeme;

        auto emit = [&](TokenType type) {
            tokens.push_back({type, currentLexeme, 0});
            currentLexeme.clear();
        };

        for (size_t i = 0; i < input.length(); ++i) {
            char c = input[i];
            DFAState fromState = currentState;
            DFAState nextState = DFAState::ERROR;
            std::string emittedInfo = "";
            std::string expl = "";
            bool consumeChar = true;

            switch (currentState) {
                case DFAState::START:
                    if (isspace(c)) { 
                        nextState = DFAState::START; 
                        expl = "Whitespace. Skipping.";
                    }
                    else if (isdigit(c)) { 
                        nextState = DFAState::NUMBER; 
                        currentLexeme += c; 
                        expl = "Digit detected. Starting NUMBER.";
                    }
                    else if (isalpha(c) || c == '_') {
                        nextState = DFAState::IDENTIFIER;
                        currentLexeme += c;
                        expl = "Letter/Underscore detected. Starting IDENTIFIER.";
                    }
                    else if (c == '+') { nextState = DFAState::OP_PLUS; currentLexeme += c; expl="Plus (+) detected."; }
                    else if (c == '-') { nextState = DFAState::OP_MINUS; currentLexeme += c; expl="Minus (-) detected."; }
                    else if (c == '*') { nextState = DFAState::OP_TIMES; currentLexeme += c; expl="Multiply (*) detected."; }
                    else if (c == '/') { nextState = DFAState::OP_DIVIDE; currentLexeme += c; expl="Divide (/) detected."; }
                    else if (c == '(') { nextState = DFAState::LPAREN; currentLexeme += c; expl="Left Paren detected."; }
                    else if (c == ')') { nextState = DFAState::RPAREN; currentLexeme += c; expl="Right Paren detected."; }
                    else { 
                        nextState = DFAState::ERROR; 
                        expl = "Invalid character. No transition.";
                    }
                    break;

                case DFAState::NUMBER:
                    if (isdigit(c)) {
                        nextState = DFAState::NUMBER;
                        currentLexeme += c;
                        expl = "Digit. Staying in NUMBER.";
                    } 
                    else if (c == '.') {
                        nextState = DFAState::POINT;
                        currentLexeme += c;
                        expl = "Decimal point. Transitioning to POINT.";
                    }
                    else {
                        emit(TOK_NUMBER);
                        emittedInfo = "TOK_NUMBER";
                        expl = "Non-digit. Emitting NUMBER token.";
                        nextState = DFAState::START;
                        consumeChar = false;
                    }
                    break;

                case DFAState::POINT:
                    if (isdigit(c)) {
                        nextState = DFAState::FRACTION;
                        currentLexeme += c;
                        expl = "Digit after point. Transitioning to FRACTION.";
                    } else {
                        nextState = DFAState::ERROR;
                        expl = "Digit expected after decimal point.";
                    }
                    break;
                
                case DFAState::FRACTION:
                    if (isdigit(c)) {
                        nextState = DFAState::FRACTION;
                        currentLexeme += c;
                        expl = "Digit. Staying in FRACTION.";
                    } else {
                        emit(TOK_NUMBER);
                        emittedInfo = "TOK_NUMBER";
                        expl = "End of float. Emitting NUMBER token.";
                        nextState = DFAState::START;
                        consumeChar = false;
                    }
                    break;

                case DFAState::IDENTIFIER:
                    if (isalnum(c) || c == '_') {
                        nextState = DFAState::IDENTIFIER;
                        currentLexeme += c;
                        expl = "Alphanumeric/Underscore. Staying in IDENTIFIER.";
                    } else {
                        // Emit as NUMBER type for now (or you could add TOK_IDENTIFIER)
                        emit(TOK_NUMBER); // Placeholder - ideally TOK_IDENTIFIER
                        emittedInfo = "TOK_IDENTIFIER";
                        expl = "End of identifier. Emitting IDENTIFIER token.";
                        nextState = DFAState::START;
                        consumeChar = false;
                    }
                    break;

                case DFAState::OP_PLUS:   emit(TOK_PLUS);   emittedInfo="TOK_PLUS";   consumeChar=false; nextState=DFAState::START; expl="Emitting PLUS."; break;
                case DFAState::OP_MINUS:  emit(TOK_MINUS);  emittedInfo="TOK_MINUS";  consumeChar=false; nextState=DFAState::START; expl="Emitting MINUS."; break;
                case DFAState::OP_TIMES:  emit(TOK_TIMES);  emittedInfo="TOK_TIMES";  consumeChar=false; nextState=DFAState::START; expl="Emitting TIMES."; break;
                case DFAState::OP_DIVIDE: emit(TOK_DIVIDE); emittedInfo="TOK_DIVIDE"; consumeChar=false; nextState=DFAState::START; expl="Emitting DIVIDE."; break;
                case DFAState::LPAREN:    emit(TOK_LPAREN); emittedInfo="TOK_LPAREN"; consumeChar=false; nextState=DFAState::START; expl="Emitting LPAREN."; break;
                case DFAState::RPAREN:    emit(TOK_RPAREN); emittedInfo="TOK_RPAREN"; consumeChar=false; nextState=DFAState::START; expl="Emitting RPAREN."; break;
                
                case DFAState::ERROR:
                    nextState = DFAState::ERROR;
                    consumeChar = true;
                    break;
            }

            history.push_back({fromState, c, nextState, emittedInfo, expl});
            currentState = nextState;
            if (!consumeChar) i--;
        }
        
        // End of input handling
        if (currentState == DFAState::NUMBER || currentState == DFAState::FRACTION) {
             tokens.push_back({TOK_NUMBER, currentLexeme, 0});
             history.push_back({currentState, '\0', DFAState::START, "TOK_NUMBER", "End of input. Emitting final NUMBER."});
        }
        else if (currentState == DFAState::IDENTIFIER) {
             tokens.push_back({TOK_NUMBER, currentLexeme, 0}); // Placeholder
             history.push_back({currentState, '\0', DFAState::START, "TOK_IDENTIFIER", "End of input. Emitting final IDENTIFIER."});
        }
    }
};
