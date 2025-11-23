#include <SFML/Graphics.hpp> // <-- MUST BE FIRST
#include "imgui.h"           // NEW: Explicitly include core ImGui definitions
#include "imgui-SFML.h"      // <-- This includes the SFML backend

#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <map>
#include <memory>
#include <sstream>

#include "Lexer.h"
#include "Parser.h"
#include "NFA.h"

// ---------- MAIN GUI ----------
int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "RECalc GUI");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Failed to initialize ImGui-SFML\n"; // Qualified with std::
        return 1;
    }

    std::string mode = "None";
    // Buffers for UI
    char exprBuf[512] = "";           // arithmetic input
    char regexBuf[512] = "";          // regex pattern
    char regexTestBuf[512] = "";      // regex test string

    std::vector<std::string> output;    // generic output area

    Lexer lexer;
    Parser parser;
    std::unique_ptr<ASTNode> ast;
    ThompsonNFA nfa;

    sf::Clock deltaClock;

    while (window.isOpen()) {
        auto maybeEvent = window.pollEvent();
        while (maybeEvent) {
            sf::Event event = *maybeEvent;
            ImGui::SFML::ProcessEvent(window, event);

            if (event.is<sf::Event::Closed>()) {
                window.close();
            }

            maybeEvent = window.pollEvent();
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        // ImGui window
        ImGui::Begin("RECalc");

        if (ImGui::Button("Arithmetic")) { mode = "Arithmetic"; output.clear(); }
        ImGui::SameLine();
        if (ImGui::Button("Regex")) { mode = "Regex"; output.clear(); }

        ImGui::Text("Mode: %s", mode.c_str());
        ImGui::Separator();

        if (mode == "Arithmetic") {
            ImGui::InputText("Expression", exprBuf, sizeof(exprBuf));
            ImGui::SameLine();
            if (ImGui::Button("Tokenize")) {
                lexer.setInput(std::string(exprBuf));
                output = lexer.steps;
            }
            ImGui::SameLine();
            if (ImGui::Button("Parse")) {
                try {
                    lexer.setInput(std::string(exprBuf));
                    parser.setTokens(lexer.tokens);
                    ast = parser.parseExpression();
                    output = lexer.steps;
                    output.insert(output.end(), parser.trace.begin(), parser.trace.end());
                    std::ostringstream oss; renderAST(ast.get(), oss);
                    output.push_back("-- AST --");
                    std::string s = oss.str();
                    std::istringstream iss(s);
                    std::string line;
                    while (std::getline(iss, line)) output.push_back(line);
                } catch (std::exception &ex) {
                    output.clear(); output.push_back(std::string("Parse error: ") + ex.what());
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Evaluate")) {
                try {
                    lexer.setInput(std::string(exprBuf));
                    parser.setTokens(lexer.tokens);
                    ast = parser.parseExpression();
                    std::vector<std::string> evalTrace;
                    double result = evalAST(ast.get(), evalTrace);
                    output = lexer.steps;
                    output.insert(output.end(), parser.trace.begin(), parser.trace.end());
                    output.push_back("-- Eval --");
                    output.push_back("Result = " + std::to_string(result));
                    output.insert(output.end(), evalTrace.begin(), evalTrace.end());
                } catch (std::exception &ex) {
                    output.clear(); output.push_back(std::string("Eval error: ") + ex.what());
                }
            }
        } else if (mode == "Regex") {
            ImGui::InputText("Pattern", regexBuf, sizeof(regexBuf));
            ImGui::InputText("Test String", regexTestBuf, sizeof(regexTestBuf));
            ImGui::SameLine();
            if (ImGui::Button("Build NFA")) {
                try {
                    nfa.buildFromRegex(std::string(regexBuf));
                    output = nfa.trace;
                } catch (std::exception &ex) {
                    output.clear(); output.push_back(std::string("NFA build error: ") + ex.what());
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Simulate")) {
                try {
                    std::vector<std::string> sim;
                    bool ok = nfa.simulate(std::string(regexTestBuf), sim);
                    output = nfa.trace;
                    output.push_back("-- Simulation --");
                    output.insert(output.end(), sim.begin(), sim.end());
                    output.push_back(ok?"Final: Accepted":"Final: Rejected");
                } catch (std::exception &ex) {
                    output.clear(); output.push_back(std::string("Simulation error: ") + ex.what());
                }
            }
        }

        ImGui::Separator();
        ImGui::BeginChild("Output", ImVec2(0,300), true);
        for (const auto &line : output)
            ImGui::TextWrapped("%s", line.c_str());
        ImGui::EndChild();

        ImGui::End();

        window.clear(sf::Color(50, 50, 50));
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}