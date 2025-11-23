#pragma once
#include "Lexer.h"
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include <iostream>

// AST for arithmetic expressions
struct ASTNode {
    virtual ~ASTNode() = default;
};

struct NumberNode : ASTNode {
    double value;
    explicit NumberNode(double v) : value(v) {}
};

struct UnaryNode : ASTNode {
    char op;
    std::unique_ptr<ASTNode> child;
    UnaryNode(char o, std::unique_ptr<ASTNode> c) : op(o), child(std::move(c)) {}
};

struct BinaryNode : ASTNode {
    char op;
    std::unique_ptr<ASTNode> left, right;
    BinaryNode(char o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r) : op(o), left(std::move(l)), right(std::move(r)) {}
};

class Parser {
    const std::vector<Token> *tokens = nullptr;
    size_t pos = 0;
public:
    std::vector<std::string> trace;

    void setTokens(const std::vector<Token> &toks) { tokens = &toks; pos = 0; trace.clear(); }

    const Token &peek() const { static Token e={TOK_END,"",0}; if (!tokens) return e; if (pos < tokens->size()) return (*tokens)[pos]; return (*tokens).back(); }
    Token next() { if (!tokens) return {TOK_END, "", 0}; return (*tokens)[pos++]; }

    std::unique_ptr<ASTNode> parseExpression() { return parseAddSub(); }

    std::unique_ptr<ASTNode> parseAddSub() {
        auto node = parseMulDiv();
        while (true) {
            Token t = peek();
            if (t.type == TOK_PLUS || t.type == TOK_MINUS) {
                next();
                auto rhs = parseMulDiv();
                node = std::make_unique<BinaryNode>(t.value[0], std::move(node), std::move(rhs));
                trace.push_back(std::string("Binary ") + t.value);
            } else break;
        }
        return node;
    }

    std::unique_ptr<ASTNode> parseMulDiv() {
        auto node = parseUnary();
        while (true) {
            Token t = peek();
            if (t.type == TOK_TIMES || t.type == TOK_DIVIDE) {
                next();
                auto rhs = parseUnary();
                node = std::make_unique<BinaryNode>(t.value[0], std::move(node), std::move(rhs));
                trace.push_back(std::string("Binary ") + t.value);
            } else break;
        }
        return node;
    }

    std::unique_ptr<ASTNode> parseUnary() {
        Token t = peek();
        if (t.type == TOK_PLUS || t.type == TOK_MINUS) {
            next();
            auto child = parseUnary();
            trace.push_back(std::string("Unary ") + t.value);
            return std::make_unique<UnaryNode>(t.value[0], std::move(child));
        }
        return parsePrimary();
    }

    std::unique_ptr<ASTNode> parsePrimary() {
        Token t = peek();
        if (t.type == TOK_NUMBER) {
            next();
            double v = std::stod(t.value);
            trace.push_back(std::string("Number ") + t.value);
            return std::make_unique<NumberNode>(v);
        }
        if (t.type == TOK_LPAREN) {
            next();
            auto inside = parseExpression();
            if (peek().type == TOK_RPAREN) next();
            else throw std::runtime_error("Expected )");
            return inside;
        }
        throw std::runtime_error("Unexpected token in primary: " + t.value);
    }
};

// Evaluate AST with trace
inline double evalAST(const ASTNode *node, std::vector<std::string> &trace) {
    if (const NumberNode *n = dynamic_cast<const NumberNode*>(node)) return n->value;
    if (const UnaryNode *u = dynamic_cast<const UnaryNode*>(node)) {
        double v = evalAST(u->child.get(), trace);
        if (u->op == '-') { trace.push_back("Unary -: " + std::to_string(v)); return -v; }
        trace.push_back(std::string("Unary ") + u->op + ": " + std::to_string(v));
        return v;
    }
    if (const BinaryNode *b = dynamic_cast<const BinaryNode*>(node)) {
        double l = evalAST(b->left.get(), trace);
        double r = evalAST(b->right.get(), trace);
        switch (b->op) {
            case '+': trace.push_back("Add: " + std::to_string(l) + " + " + std::to_string(r)); return l + r;
            case '-': trace.push_back("Sub: " + std::to_string(l) + " - " + std::to_string(r)); return l - r;
            case '*': trace.push_back("Mul: " + std::to_string(l) + " * " + std::to_string(r)); return l * r;
            case '/': trace.push_back("Div: " + std::to_string(l) + " / " + std::to_string(r)); return l / r;
        }
    }
    throw std::runtime_error("Unknown AST node");
}

// Render AST as indented text
inline void renderAST(const ASTNode *node, std::ostream &os, int indent=0) {
    std::string pad(indent, ' ');
    if (const NumberNode *n = dynamic_cast<const NumberNode*>(node)) os << pad << "Number(" << n->value << ")\n";
    else if (const UnaryNode *u = dynamic_cast<const UnaryNode*>(node)) {
        os << pad << "Unary(" << u->op << ")\n";
        renderAST(u->child.get(), os, indent+2);
    } else if (const BinaryNode *b = dynamic_cast<const BinaryNode*>(node)) {
        os << pad << "Binary(" << b->op << ")\n";
        renderAST(b->left.get(), os, indent+2);
        renderAST(b->right.get(), os, indent+2);
    }
}
