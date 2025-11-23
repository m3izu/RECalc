#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include "Lexer.h"
#include "Parser.h"
#include "NFA.h"

void testArithmetic() {
    std::cout << "Testing Arithmetic..." << std::endl;
    Lexer lexer;
    Parser parser;
    std::vector<std::string> trace;

    // Test 1: 1 + 2
    lexer.setInput("1 + 2");
    parser.setTokens(lexer.tokens);
    auto ast = parser.parseExpression();
    double result = evalAST(ast.get(), trace);
    assert(result == 3.0);
    std::cout << "  1 + 2 = " << result << " [PASS]" << std::endl;

    // Test 2: 3 * (4 - 2)
    lexer.setInput("3 * (4 - 2)");
    parser.setTokens(lexer.tokens);
    ast = parser.parseExpression();
    result = evalAST(ast.get(), trace);
    assert(result == 6.0);
    std::cout << "  3 * (4 - 2) = " << result << " [PASS]" << std::endl;

    // Test 3: 10 / 2 + 5
    lexer.setInput("10 / 2 + 5");
    parser.setTokens(lexer.tokens);
    ast = parser.parseExpression();
    result = evalAST(ast.get(), trace);
    assert(result == 10.0);
    std::cout << "  10 / 2 + 5 = " << result << " [PASS]" << std::endl;
}

void testRegex() {
    std::cout << "Testing Regex..." << std::endl;
    ThompsonNFA nfa;
    std::vector<std::string> trace;

    // Test 1: a|b
    nfa.buildFromRegex("a|b");
    if (!nfa.simulate("a", trace)) {
        std::cerr << "Test 1 failed: a|b should match 'a'" << std::endl;
        for (const auto& s : trace) std::cerr << s << std::endl;
        exit(1);
    }
    if (!nfa.simulate("b", trace)) {
        std::cerr << "Test 1 failed: a|b should match 'b'" << std::endl;
        for (const auto& s : trace) std::cerr << s << std::endl;
        exit(1);
    }
    if (nfa.simulate("c", trace)) {
        std::cerr << "Test 1 failed: a|b should NOT match 'c'" << std::endl;
        for (const auto& s : trace) std::cerr << s << std::endl;
        exit(1);
    }
    std::cout << "  a|b matches 'a', 'b' [PASS]" << std::endl;

    // Test 2: a*
    nfa.buildFromRegex("a*");
    assert(nfa.simulate("", trace) == true);
    assert(nfa.simulate("a", trace) == true);
    assert(nfa.simulate("aaa", trace) == true);
    assert(nfa.simulate("b", trace) == false);
    std::cout << "  a* matches '', 'a', 'aaa' [PASS]" << std::endl;

    // Test 3: (a|b)*c
    nfa.buildFromRegex("(a|b)*c");
    assert(nfa.simulate("c", trace) == true);
    assert(nfa.simulate("ac", trace) == true);
    assert(nfa.simulate("bc", trace) == true);
    assert(nfa.simulate("abac", trace) == true);
    assert(nfa.simulate("aba", trace) == false);
    std::cout << "  (a|b)*c matches 'c', 'ac', 'abac' [PASS]" << std::endl;
}

int main() {
    try {
        testArithmetic();
        testRegex();
        std::cout << "All tests passed!" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
