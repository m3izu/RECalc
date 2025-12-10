#pragma once
#include "Lexer.h"
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

struct PDAStep {
    std::string stackState; 
    std::string inputRemaining; 
    std::string action; 
    std::string explanation; // Educational context
    int inputPosition;       // Current token position (for highlighting)
};

class PDAParser {
public:
    std::vector<std::string> stack;
    std::vector<PDAStep> trace;

    void parse(const std::vector<Token>& tokens) {
        trace.clear();
        stack.clear();
        
        stack.push_back("$");
        stack.push_back("E"); 

        int cursor = 0;

        while (!stack.empty()) {
            std::string top = stack.back();
            std::string expl = "";

            std::string inputStr = "";
            for(int i=cursor; i<tokens.size(); ++i) inputStr += tokens[i].value + " ";
            if (cursor >= tokens.size()) inputStr = "[End]";

            std::string stackStr = "";
            for(const auto& s : stack) stackStr += s + " ";

            if (top == "$") {
                // Check if input is exhausted (cursor at end OR on TOK_END token)
                bool inputEmpty = (cursor >= tokens.size()) || 
                                 (cursor < tokens.size() && tokens[cursor].type == TOK_END);
                if (inputEmpty) {
                    trace.push_back({stackStr, inputStr, "Accept", "Stack empty ($) and Input empty. Parse Successful!", cursor});
                    stack.pop_back(); 
                    break;
                } else {
                     trace.push_back({stackStr, inputStr, "Error", "Stack empty ($) but Input remains. Rejecting.", cursor});
                     break;
                }
            }
            
            Token current = (cursor < tokens.size()) ? tokens[cursor] : Token{TOK_END, "", 0};

            // Terminals
            if (top == "+" || top == "-" || top == "*" || top == "/" || top == "(" || top == ")") {
                if (current.value == top) {
                    stack.pop_back();
                    cursor++;
                    trace.push_back({stackStr, inputStr, "Match " + top, "Top of stack is terminal '" + top + "' which matches input. Consumed.", cursor - 1});
                } else {
                    trace.push_back({stackStr, inputStr, "Error", "Mismatch: Expected '" + top + "' but got '" + current.value + "'.", cursor});
                    break;
                }
            } 
            else if (top == "num") {
                if (current.type == TOK_NUMBER) {
                    stack.pop_back();
                    cursor++;
                    trace.push_back({stackStr, inputStr, "Match number (" + current.value + ")", "Matched generic 'num' terminal with value " + current.value + ".", cursor - 1});
                } else {
                    trace.push_back({stackStr, inputStr, "Error", "Expected a Number but got '" + current.value + "'.", cursor});
                    break;
                }
            }
            else if (top == "id") {
                if (current.type == TOK_ID) {
                    stack.pop_back();
                    cursor++;
                    trace.push_back({stackStr, inputStr, "Match id (" + current.value + ")", "Matched identifier '" + current.value + "'.", cursor - 1});
                } else {
                    trace.push_back({stackStr, inputStr, "Error", "Expected an Identifier but got '" + current.value + "'.", cursor});
                    break;
                }
            }
            // Non-Terminals
            else if (top == "E") {
                stack.pop_back();
                stack.push_back("E'");
                stack.push_back("T");
                trace.push_back({stackStr, inputStr, "Expand E -> T E'", "Expression (E) always starts with a Term (T) followed by optional additions (E').", cursor});
            }
            else if (top == "E'") {
                if (current.type == TOK_PLUS) {
                    stack.pop_back();
                    stack.push_back("E'");
                    stack.push_back("T");
                    stack.push_back("+");
                    trace.push_back({stackStr, inputStr, "Expand E' -> + T E'", "Found '+'. Expanding E' to handle addition.", cursor});
                } else if (current.type == TOK_MINUS) {
                    stack.pop_back();
                    stack.push_back("E'");
                    stack.push_back("T");
                    stack.push_back("-");
                    trace.push_back({stackStr, inputStr, "Expand E' -> - T E'", "Found '-'. Expanding E' to handle subtraction.", cursor});
                } else {
                    stack.pop_back();
                    trace.push_back({stackStr, inputStr, "Expand E' -> epsilon", "No additive operator found. E' disappears (epsilon production).", cursor});
                }
            }
            else if (top == "T") {
                stack.pop_back();
                stack.push_back("T'");
                stack.push_back("F");
                trace.push_back({stackStr, inputStr, "Expand T -> F T'", "Term (T) consists of a Factor (F) followed by optional multiplications (T').", cursor});
            }
            else if (top == "T'") {
                if (current.type == TOK_TIMES) {
                    stack.pop_back();
                    stack.push_back("T'");
                    stack.push_back("F");
                    stack.push_back("*");
                    trace.push_back({stackStr, inputStr, "Expand T' -> * F T'", "Found '*'. Expanding T' to handle multiplication.", cursor});
                } else if (current.type == TOK_DIVIDE) {
                    stack.pop_back();
                    stack.push_back("T'");
                    stack.push_back("F");
                    stack.push_back("/");
                    trace.push_back({stackStr, inputStr, "Expand T' -> / F T'", "Found '/'. Expanding T' to handle division.", cursor});
                } else {
                     stack.pop_back();
                    trace.push_back({stackStr, inputStr, "Expand T' -> epsilon", "No multiplicative operator found. T' disappears.", cursor});
                }
            }
            else if (top == "F") {
                if (current.type == TOK_LPAREN) {
                    stack.pop_back();
                    stack.push_back(")");
                    stack.push_back("E");
                    stack.push_back("(");
                    trace.push_back({stackStr, inputStr, "Expand F -> ( E )", "Found '('. Factor is a parenthesized expression.", cursor});
                } else if (current.type == TOK_NUMBER) {
                    stack.pop_back();
                    stack.push_back("num");
                    trace.push_back({stackStr, inputStr, "Expand F -> num", "Found digit. Factor is a number.", cursor});
                } else if (current.type == TOK_ID) {
                    stack.pop_back();
                    stack.push_back("id");
                    trace.push_back({stackStr, inputStr, "Expand F -> id", "Found identifier '" + current.value + "'. Factor is an identifier.", cursor});
                } else {
                    trace.push_back({stackStr, inputStr, "Error", "Invalid Factor start. Expected '(', Number, or Identifier.", cursor});
                    break;
                }
            }
            else {
                 trace.push_back({stackStr, inputStr, "Error", "Unknown symbol on stack: " + top, cursor});
                 break;
            }
        }
    }

    // Balanced Parentheses Check - Demonstrates CFG: S → (S) | SS | ε
    void checkBalanced(const std::string& input) {
        trace.clear();
        stack.clear();
        stack.push_back("$");
        
        int depth = 0;
        std::string remaining = input;
        
        trace.push_back({"$ ", remaining, "Start", "CFG: S → (S) | SS | ε. Beginning balance check."});
        
        for (size_t i = 0; i < input.length(); ++i) {
            char c = input[i];
            std::string stackStr = "";
            for (int j = 0; j < depth; ++j) stackStr += "( ";
            stackStr += "$ ";
            remaining = input.substr(i);
            
            if (c == '(') {
                depth++;
                stack.push_back("(");
                trace.push_back({stackStr + "( ", remaining, "Push (", "Opening paren. Push onto stack. Depth: " + std::to_string(depth)});
            } else if (c == ')') {
                if (depth == 0) {
                    trace.push_back({stackStr, remaining, "REJECT", "Closing paren without matching open!"});
                    return;
                }
                depth--;
                stack.pop_back();
                trace.push_back({stackStr, remaining, "Pop (", "Matched. Pop stack. Depth: " + std::to_string(depth)});
            }
        }
        
        std::string finalStack = "";
        for (int j = 0; j < depth; ++j) finalStack += "( ";
        finalStack += "$ ";
        
        if (depth == 0) {
            trace.push_back({finalStack, "", "ACCEPT", "All parentheses matched!"});
        } else {
            trace.push_back({finalStack, "", "REJECT", std::to_string(depth) + " unmatched '(' remaining!"});
        }
    }
};
