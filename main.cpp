#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"

#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <map>
#include <memory>
#include <sstream>
#include <algorithm>

#include "Lexer.h"
#include "Parser.h"
#include "NFA.h"
#include "DFA.h"
#include "PDA.h"
#include "Visualizer.h"
#include "SubsetConstruction.h"
#include "LexerNFA.h"

// --- BEAUTIFUL GRADIENT THEME ---
void SetupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    style.WindowRounding = 10.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 6);
    
    ImVec4* colors = style.Colors;
    // Deep purple to blue gradient feel
    colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.08f, 0.08f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.10f, 0.10f, 0.15f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.12f, 0.12f, 0.18f, 0.98f);
    colors[ImGuiCol_Border]                 = ImVec4(0.30f, 0.25f, 0.40f, 0.50f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.15f, 0.14f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.22f, 0.20f, 0.32f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.28f, 0.25f, 0.40f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.10f, 0.08f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.20f, 0.15f, 0.35f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.25f, 0.20f, 0.40f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.35f, 0.28f, 0.55f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.45f, 0.35f, 0.70f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.25f, 0.20f, 0.40f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.35f, 0.28f, 0.55f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.45f, 0.35f, 0.70f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.40f, 0.30f, 0.55f, 0.50f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.50f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.65f, 0.50f, 0.90f, 1.00f);
}

// Stage names for the pipeline
const char* STAGE_NAMES[] = { "1. NFA", "2. INPUT", "3. DFA CONVERSION", "4. PDA PARSER" };
const int NUM_STAGES = 4;

int main() {
    // Get desktop resolution for fullscreen-like experience
    auto desktop = sf::VideoMode::getDesktopMode();
    unsigned int width = std::min(desktop.size.x - 100, 1800u);
    unsigned int height = std::min(desktop.size.y - 100, 1000u);
    
    sf::RenderWindow window(sf::VideoMode({width, height}), "RECalc: Compiler Front-End Educational Pipeline", sf::Style::Default);
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) return 1;
    SetupImGuiStyle();

    // Pipeline State
    int stage = 0; // 0=NFA, 1=INPUT, 2=DFA, 3=PDA
    char inputBuf[512] = "3 + (4 * 5)";
    
    // Automata State
    FullNFA lexerNFA;
    std::vector<LexerDFAState> lexerDFA;
    PDAParser pdaParser;
    Lexer lexer;
    
    // Animation State
    int dfaStep = 0;
    int dfaTotalSteps = 0;
    int pdaStep = 0;
    int tokenStep = 0;  // Current token animation step
    int tokenTotalSteps = 0;
    bool isPlaying = false;
    float playTimer = 0.0f;
    float playSpeed = 0.5f;
    
    // Cached lexer for animation
    Lexer cachedLexer;
    
    // Log messages
    std::vector<std::string> logs;
    std::vector<std::pair<std::string, std::string>> tokens; // (type, value)
    
    // Build NFA on startup
    lexerNFA = buildCombinedNFA();
    logs.push_back("✓ Thompson's NFA constructed with " + std::to_string(lexerNFA.states.size()) + " states");
    logs.push_back("• Patterns: ID, NUMBER, +, -, *, /, (, ), whitespace");

    sf::Clock deltaClock;

    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>()) window.close();
        }

        sf::Time dt = deltaClock.restart();
        ImGui::SFML::Update(window, dt);

        // Animation tick
        if (isPlaying) {
            playTimer += dt.asSeconds();
            if (playTimer >= playSpeed) {
                playTimer = 0;
                if (stage == 2 && tokenStep < tokenTotalSteps - 1) tokenStep++;
                else if (stage == 3 && pdaStep < (int)pdaParser.trace.size() - 1) pdaStep++;
                else isPlaying = false;
            }
        }

        // Main Window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(1400, 850));
        ImGui::Begin("Pipeline", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

        // ============ STAGE INDICATOR BAR ============
        {
            ImGui::BeginChild("StageBar", ImVec2(0, 60), true);
            ImDrawList* draw = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();
            
            // Draw gradient background
            draw->AddRectFilledMultiColor(
                ImVec2(p.x - 10, p.y - 10), 
                ImVec2(p.x + 1380, p.y + 50),
                IM_COL32(60, 40, 100, 255),  // Purple
                IM_COL32(40, 60, 120, 255),  // Blue
                IM_COL32(30, 80, 100, 255),  // Teal
                IM_COL32(50, 50, 90, 255)    // Purple
            );
            
            float stageWidth = 330.0f;
            float startX = p.x + 30;
            
            for (int i = 0; i < NUM_STAGES; i++) {
                float x = startX + i * stageWidth;
                float y = p.y + 15;
                
                // Stage circle
                ImU32 circleCol = (i < stage) ? IM_COL32(100, 220, 100, 255) :
                                  (i == stage) ? IM_COL32(255, 200, 80, 255) :
                                  IM_COL32(100, 100, 120, 255);
                draw->AddCircleFilled(ImVec2(x, y + 10), 12, circleCol);
                
                // Stage number
                char num[2] = { (char)('1' + i), 0 };
                draw->AddText(ImVec2(x - 4, y + 3), IM_COL32(0, 0, 0, 255), num);
                
                // Stage name
                ImU32 textCol = (i == stage) ? IM_COL32(255, 220, 100, 255) : IM_COL32(200, 200, 210, 255);
                draw->AddText(ImVec2(x + 20, y + 3), textCol, STAGE_NAMES[i]);
                
                // Connecting line
                if (i < NUM_STAGES - 1) {
                    draw->AddLine(ImVec2(x + 12, y + 10), ImVec2(x + stageWidth - 12, y + 10),
                        (i < stage) ? IM_COL32(100, 220, 100, 200) : IM_COL32(80, 80, 100, 200), 2.0f);
                }
            }
            
            ImGui::EndChild();
        }

        // ============ TWO COLUMN LAYOUT ============
        ImGui::Columns(2, "MainCols", false);
        ImGui::SetColumnWidth(0, 900);

        // LEFT: Visualization
        ImGui::BeginChild("VizPanel", ImVec2(0, 580), true);
        {
            // Title based on stage
            const char* titles[] = {
                "STAGE 1: THOMPSON'S NFA - Token Patterns",
                "STAGE 2: INPUT - Your Expression",
                "STAGE 3: DFA TOKENIZATION - Scanning Your Input",
                "STAGE 4: PDA PARSER - Syntax Analysis"
            };
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.4f, 1.0f), "%s", titles[stage]);
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 5));
            
            if (stage == 0) {
                // NFA Visualization
                Viz::DrawLexerNFA(lexerNFA);
            }
            else if (stage == 1) {
                // Input & Tokenization
                ImGui::TextWrapped("The NFA above recognizes these token patterns. Now enter an expression to tokenize:");
                ImGui::Dummy(ImVec2(0, 10));
                ImGui::Text("Your Expression:");
                ImGui::SetNextItemWidth(600);
                if (ImGui::InputText("##input", inputBuf, sizeof(inputBuf))) {
                    // Input changed - will be tokenized in next stage
                }
                ImGui::Dummy(ImVec2(0, 15));
                
                // Live token preview
                if (strlen(inputBuf) > 0) {
                    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Live Token Preview:");
                    ImGui::Separator();
                    
                    // Simple tokenization display
                    Lexer tempLex(inputBuf);
                    ImGui::BeginChild("TokenList", ImVec2(0, 200), true);
                    ImDrawList* draw = ImGui::GetWindowDrawList();
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    float x = pos.x + 10;
                    float y = pos.y + 10;
                    
                    for (size_t i = 0; i < tempLex.tokens.size(); i++) {
                        auto& tok = tempLex.tokens[i];
                        if (tok.type == TOK_END) continue;
                        std::string label = tok.value;
                        ImVec2 sz = ImGui::CalcTextSize(label.c_str());
                        
                        // Color by type
                        ImU32 bg, fg;
                        std::string typeName;
                        if (tok.type == TOK_ID) {
                            bg = IM_COL32(40, 80, 140, 255); fg = IM_COL32(180, 200, 255, 255);
                            typeName = "IDENTIFIER";
                        }
                        else if (tok.type == TOK_NUMBER) { 
                            bg = IM_COL32(40, 120, 80, 255); fg = IM_COL32(180, 255, 180, 255); 
                            typeName = "NUMBER";
                        }
                        else if (tok.type == TOK_PLUS || tok.type == TOK_MINUS || tok.type == TOK_TIMES || tok.type == TOK_DIVIDE) {
                            bg = IM_COL32(140, 100, 40, 255); fg = IM_COL32(255, 220, 150, 255);
                            typeName = "OPERATOR";
                        }
                        else if (tok.type == TOK_LPAREN || tok.type == TOK_RPAREN) {
                            bg = IM_COL32(100, 60, 120, 255); fg = IM_COL32(220, 180, 255, 255);
                            typeName = (tok.type == TOK_LPAREN) ? "LPAREN" : "RPAREN";
                        }
                        else if (tok.type == TOK_INVALID) { 
                            bg = IM_COL32(120, 40, 40, 255); fg = IM_COL32(255, 150, 150, 255); 
                            typeName = "INVALID";
                        }
                        else { 
                            bg = IM_COL32(60, 60, 70, 255); fg = IM_COL32(200, 200, 200, 255); 
                            typeName = "OTHER";
                        }
                        
                        // Token box with good readable spacing
                        float pad = 12;         // Generous padding inside box
                        float spacing = 20;     // Comfortable space between tokens
                        float rowHeight = 65;   // Height per row for type label
                        
                        // Token value box
                        float boxW = sz.x + pad * 2;
                        float boxH = sz.y + pad + 4;
                        draw->AddRectFilled(ImVec2(x, y), ImVec2(x + boxW, y + boxH), bg, 6);
                        draw->AddRect(ImVec2(x, y), ImVec2(x + boxW, y + boxH), IM_COL32(255, 255, 255, 80), 6);
                        draw->AddText(ImVec2(x + pad, y + (boxH - sz.y) / 2), fg, label.c_str());
                        
                        // Type label below (centered)
                        ImVec2 typeSize = ImGui::CalcTextSize(typeName.c_str());
                        float typeX = x + (boxW - typeSize.x) / 2;
                        draw->AddText(ImVec2(typeX, y + boxH + 5), IM_COL32(160, 170, 190, 255), typeName.c_str());
                        
                        x += boxW + spacing;
                        if (x > pos.x + 620) { x = pos.x + 10; y += rowHeight; }
                    }
                    ImGui::EndChild();
                }
            }
            else if (stage == 2) {
                // DFA Tokenization of user input - ANIMATED with scroll
                ImGui::BeginChild("DFAScroll", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                
                // Header section
                ImGui::TextColored(ImVec4(0.7f, 0.85f, 1.0f, 1.0f), "The DFA is scanning your input:");
                ImGui::Dummy(ImVec2(0, 5));
                
                // Input display with styled background
                {
                    ImDrawList* drawBg = ImGui::GetWindowDrawList();
                    ImVec2 bgPos = ImGui::GetCursorScreenPos();
                    drawBg->AddRectFilled(bgPos, ImVec2(bgPos.x + 700, bgPos.y + 32), 
                        IM_COL32(40, 45, 60, 255), 6.0f);
                    drawBg->AddRect(bgPos, ImVec2(bgPos.x + 700, bgPos.y + 32), 
                        IM_COL32(80, 90, 120, 200), 6.0f);
                    drawBg->AddText(ImVec2(bgPos.x + 12, bgPos.y + 7), 
                        IM_COL32(180, 230, 180, 255), inputBuf);
                    ImGui::Dummy(ImVec2(0, 38));
                }
                
                if (lexerDFA.empty()) {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1), "Building DFA...");
                } else {
                    // Validation status indicator
                    if (cachedLexer.hasErrors) {
                        ImDrawList* errDraw = ImGui::GetWindowDrawList();
                        ImVec2 errPos = ImGui::GetCursorScreenPos();
                        errDraw->AddRectFilled(errPos, ImVec2(errPos.x + 500, errPos.y + 28), 
                            IM_COL32(120, 40, 40, 255), 6.0f);
                        errDraw->AddRect(errPos, ImVec2(errPos.x + 500, errPos.y + 28), 
                            IM_COL32(200, 80, 80, 255), 6.0f);
                        std::string errMsg = "✗ ERRORS DETECTED: " + std::to_string(cachedLexer.errors.size()) + " issue(s) found";
                        errDraw->AddText(ImVec2(errPos.x + 10, errPos.y + 6), 
                            IM_COL32(255, 180, 180, 255), errMsg.c_str());
                        ImGui::Dummy(ImVec2(0, 35));
                        
                        // Show first error
                        if (!cachedLexer.errors.empty()) {
                            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1), "  %s", cachedLexer.errors[0].message.c_str());
                        }
                    } else if (!cachedLexer.errors.empty()) {
                        ImDrawList* warnDraw = ImGui::GetWindowDrawList();
                        ImVec2 warnPos = ImGui::GetCursorScreenPos();
                        warnDraw->AddRectFilled(warnPos, ImVec2(warnPos.x + 500, warnPos.y + 28), 
                            IM_COL32(100, 90, 40, 255), 6.0f);
                        warnDraw->AddText(ImVec2(warnPos.x + 10, warnPos.y + 6), 
                            IM_COL32(255, 230, 150, 255), "⚡ WARNINGS - check logs for details");
                        ImGui::Dummy(ImVec2(0, 35));
                    } else {
                        ImDrawList* okDraw = ImGui::GetWindowDrawList();
                        ImVec2 okPos = ImGui::GetCursorScreenPos();
                        okDraw->AddRectFilled(okPos, ImVec2(okPos.x + 300, okPos.y + 25), 
                            IM_COL32(40, 100, 60, 255), 6.0f);
                        okDraw->AddText(ImVec2(okPos.x + 10, okPos.y + 5), 
                            IM_COL32(150, 255, 180, 255), "✓ Tokenization valid");
                        ImGui::Dummy(ImVec2(0, 32));
                    }
                    
                    // Only show animation controls if no errors
                    if (!cachedLexer.hasErrors) {
                        // Playback controls with better styling
                        ImGui::Dummy(ImVec2(0, 5));
                        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.6f, 1), "ANIMATION CONTROLS");
                        ImGui::Separator();
                        ImGui::Dummy(ImVec2(0, 5));
                        
                        ImGui::Text("Step: %d / %d", tokenStep + 1, tokenTotalSteps);
                        ImGui::SameLine(150);
                        
                        if (ImGui::Button(isPlaying ? "II PAUSE" : "> PLAY", ImVec2(90, 28))) {
                            isPlaying = !isPlaying;
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(">> STEP", ImVec2(90, 28))) {
                            if (tokenStep < tokenTotalSteps - 1) tokenStep++;
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("RESET", ImVec2(70, 28))) {
                            tokenStep = 0;
                            isPlaying = false;
                        }
                        
                        // Animation speed control
                        ImGui::Text("Speed:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(150);
                        ImGui::SliderFloat("##speed2", &playSpeed, 0.1f, 2.0f, "%.1fs");
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.6f, 0.7f, 0.8f, 1), "(delay per step)");
                        
                        ImGui::SetNextItemWidth(350);
                        ImGui::SliderInt("##tokstep", &tokenStep, 0, tokenTotalSteps > 0 ? tokenTotalSteps - 1 : 0);
                        ImGui::Dummy(ImVec2(0, 15));
                    
                    // Current action with styled box
                    if (tokenStep < (int)cachedLexer.steps.size()) {
                        ImDrawList* drawAction = ImGui::GetWindowDrawList();
                        ImVec2 actPos = ImGui::GetCursorScreenPos();
                        drawAction->AddRectFilled(actPos, ImVec2(actPos.x + 500, actPos.y + 35), 
                            IM_COL32(60, 80, 50, 255), 6.0f);
                        drawAction->AddRect(actPos, ImVec2(actPos.x + 500, actPos.y + 35), 
                            IM_COL32(100, 180, 100, 200), 6.0f);
                        
                        std::string actionText = "ACTION: " + cachedLexer.steps[tokenStep];
                        drawAction->AddText(ImVec2(actPos.x + 10, actPos.y + 9), 
                            IM_COL32(180, 255, 180, 255), actionText.c_str());
                        ImGui::Dummy(ImVec2(0, 42));
                    }
                    
                    // Tokens discovered section
                    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1), "TOKENS DISCOVERED:");
                    ImGui::Dummy(ImVec2(0, 5));
                    
                    int tokensShown = 0;
                    for (size_t i = 0; i < cachedLexer.tokens.size() && tokensShown <= tokenStep; i++) {
                        auto& tok = cachedLexer.tokens[i];
                        if (tok.type == TOK_END) continue;
                        const char* typeName = 
                            tok.type == TOK_ID ? "IDENTIFIER" :
                            tok.type == TOK_NUMBER ? "NUMBER" :
                            tok.type == TOK_PLUS ? "OPERATOR" :
                            tok.type == TOK_MINUS ? "OPERATOR" :
                            tok.type == TOK_TIMES ? "OPERATOR" :
                            tok.type == TOK_DIVIDE ? "OPERATOR" :
                            tok.type == TOK_LPAREN ? "LPAREN" :
                            tok.type == TOK_RPAREN ? "RPAREN" : "OTHER";
                        
                        if (tokensShown == tokenStep) {
                            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1), ">> %s: '%s'", typeName, tok.value.c_str());
                        } else {
                            ImGui::TextColored(ImVec4(0.6f, 0.7f, 0.8f, 1), "   %s: '%s'", typeName, tok.value.c_str());
                        }
                        tokensShown++;
                    }
                    ImGui::Dummy(ImVec2(0, 20));
                    
                    // Scanning progress visualization
                    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1, 1), "SCANNING PROGRESS:");
                    std::string inputStr(inputBuf);
                    char currentChar = 0;
                    if (tokenStep < (int)cachedLexer.tokens.size()) {
                        int pos = cachedLexer.tokens[tokenStep].pos;
                        int len = (int)cachedLexer.tokens[tokenStep].value.length();
                        if (pos < (int)inputStr.length()) {
                            currentChar = inputStr[pos];
                        }
                        
                        ImDrawList* draw = ImGui::GetWindowDrawList();
                        ImVec2 p = ImGui::GetCursorScreenPos();
                        
                        // Background bar
                        draw->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + 650, p.y + 35), 
                            IM_COL32(25, 28, 38, 255), 6.0f);
                        draw->AddRect(ImVec2(p.x, p.y), ImVec2(p.x + 650, p.y + 35), 
                            IM_COL32(60, 70, 100, 200), 6.0f);
                        
                        // Characters with highlighting
                        float charWidth = 14;
                        for (int c = 0; c < (int)inputStr.length(); c++) {
                            ImU32 col;
                            if (c >= pos && c < pos + len) {
                                // Highlight background for current token
                                draw->AddRectFilled(
                                    ImVec2(p.x + 8 + c * charWidth - 2, p.y + 4),
                                    ImVec2(p.x + 8 + c * charWidth + charWidth - 2, p.y + 31),
                                    IM_COL32(80, 200, 80, 100), 3.0f);
                                col = IM_COL32(100, 255, 100, 255);
                            } else if (c < pos) {
                                col = IM_COL32(130, 140, 160, 255);
                            } else {
                                col = IM_COL32(220, 225, 240, 255);
                            }
                            char ch[2] = { inputStr[c], 0 };
                            draw->AddText(ImVec2(p.x + 10 + c * charWidth, p.y + 9), col, ch);
                        }
                        ImGui::Dummy(ImVec2(0, 45));
                    }
                    
                    // DFA Diagram
                    ImGui::Dummy(ImVec2(0, 15));
                    int dfaHighlightState = 0;
                    if (tokenStep < (int)cachedLexer.tokens.size()) {
                        auto& tok = cachedLexer.tokens[tokenStep];
                        if (tok.type == TOK_NUMBER) dfaHighlightState = 2;
                        else if (tok.type == TOK_ID) dfaHighlightState = 1;
                        else if (tok.type == TOK_PLUS || tok.type == TOK_MINUS || 
                                 tok.type == TOK_TIMES || tok.type == TOK_DIVIDE ||
                                 tok.type == TOK_LPAREN || tok.type == TOK_RPAREN) {
                            dfaHighlightState = 3;
                        }
                    }
                    Viz::DrawLexerDFAAnimated(lexerDFA, dfaHighlightState, currentChar);
                    
                    } // End of if (!cachedLexer.hasErrors)
                }
                
                ImGui::EndChild();
            }
            else if (stage == 3) {
                // PDA Parsing - Comprehensive View with scroll
                ImGui::BeginChild("PDAScroll", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                
                if (pdaParser.trace.empty()) {
                    ImGui::Dummy(ImVec2(0, 50));
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1), "Click 'PARSE' to analyze syntax...");
                } else {
                    ImDrawList* sectionDraw = ImGui::GetWindowDrawList();
                    
                    // ═══════════════════════════════════════════════════════
                    // SECTION 0: TOKENIZED INPUT STREAM
                    // ═══════════════════════════════════════════════════════
                    {
                        ImVec2 hdrPos = ImGui::GetCursorScreenPos();
                        sectionDraw->AddRectFilled(hdrPos, ImVec2(hdrPos.x + 520, hdrPos.y + 26), 
                            IM_COL32(70, 50, 90, 255), 4.0f);
                        sectionDraw->AddText(ImVec2(hdrPos.x + 10, hdrPos.y + 5), 
                            IM_COL32(220, 180, 255, 255), "0. TOKENIZED INPUT STREAM");
                        ImGui::Dummy(ImVec2(0, 32));
                    }
                    
                    // Display tokens with types - highlights current token
                    {
                        ImDrawList* tokDraw = ImGui::GetWindowDrawList();
                        ImVec2 tokPos = ImGui::GetCursorScreenPos();
                        tokDraw->AddRectFilled(tokPos, ImVec2(tokPos.x + 560, tokPos.y + 55), 
                            IM_COL32(35, 35, 50, 255), 6.0f);
                        tokDraw->AddRect(tokPos, ImVec2(tokPos.x + 560, tokPos.y + 55), 
                            IM_COL32(100, 80, 140, 200), 6.0f);
                        
                        float tx = tokPos.x + 10;
                        float ty = tokPos.y + 8;
                        
                        // Get current input position from pdaStep
                        int currentInputPos = -1;
                        if (pdaStep >= 0 && pdaStep < (int)pdaParser.trace.size()) {
                            currentInputPos = pdaParser.trace[pdaStep].inputPosition;
                        }
                        
                        tokDraw->AddText(ImVec2(tx, ty), IM_COL32(200, 255, 200, 255), "Token Stream:");
                        
                        // Draw each token individually with highlighting
                        float tokenX = tx;
                        float tokenY = ty + 20;
                        for (int i = 0; i < (int)cachedLexer.tokens.size(); i++) {
                            const auto& tok = cachedLexer.tokens[i];
                            std::string tokDesc;
                            switch(tok.type) {
                                case TOK_NUMBER: tokDesc = "NUM"; break;
                                case TOK_ID: tokDesc = "ID"; break;
                                case TOK_PLUS: tokDesc = "+"; break;
                                case TOK_MINUS: tokDesc = "-"; break;
                                case TOK_TIMES: tokDesc = "*"; break;
                                case TOK_DIVIDE: tokDesc = "/"; break;
                                case TOK_LPAREN: tokDesc = "("; break;
                                case TOK_RPAREN: tokDesc = ")"; break;
                                case TOK_END: tokDesc = "$"; break;
                                default: tokDesc = "?"; break;
                            }
                            
                            ImVec2 textSize = ImGui::CalcTextSize(tokDesc.c_str());
                            float boxW = textSize.x + 8;
                            float boxH = 18;
                            
                            // Highlight current token
                            bool isCurrent = (i == currentInputPos);
                            bool isConsumed = (i < currentInputPos);
                            
                            if (isCurrent) {
                                // Bright highlight for current token
                                tokDraw->AddRectFilled(ImVec2(tokenX, tokenY), 
                                    ImVec2(tokenX + boxW, tokenY + boxH), IM_COL32(255, 200, 50, 255), 3.0f);
                                tokDraw->AddText(ImVec2(tokenX + 4, tokenY + 1), IM_COL32(0, 0, 0, 255), tokDesc.c_str());
                            } else if (isConsumed) {
                                // Gray/strikethrough for consumed tokens
                                tokDraw->AddRectFilled(ImVec2(tokenX, tokenY), 
                                    ImVec2(tokenX + boxW, tokenY + boxH), IM_COL32(60, 60, 70, 255), 2.0f);
                                tokDraw->AddText(ImVec2(tokenX + 4, tokenY + 1), IM_COL32(120, 120, 120, 255), tokDesc.c_str());
                            } else {
                                // Normal for future tokens
                                tokDraw->AddRectFilled(ImVec2(tokenX, tokenY), 
                                    ImVec2(tokenX + boxW, tokenY + boxH), IM_COL32(50, 60, 80, 255), 2.0f);
                                tokDraw->AddText(ImVec2(tokenX + 4, tokenY + 1), IM_COL32(200, 210, 230, 255), tokDesc.c_str());
                            }
                            
                            tokenX += boxW + 5;
                        }
                        
                        ImGui::Dummy(ImVec2(0, 65));
                    }
                    
                    // ═══════════════════════════════════════════════════════
                    // SECTION 1: CONTEXT-FREE GRAMMAR
                    // ═══════════════════════════════════════════════════════
                    {
                        ImVec2 hdrPos = ImGui::GetCursorScreenPos();
                        sectionDraw->AddRectFilled(hdrPos, ImVec2(hdrPos.x + 520, hdrPos.y + 26), 
                            IM_COL32(60, 70, 100, 255), 4.0f);
                        sectionDraw->AddText(ImVec2(hdrPos.x + 10, hdrPos.y + 5), 
                            IM_COL32(255, 230, 150, 255), "1. CONTEXT-FREE GRAMMAR (LL(1))");
                        ImGui::Dummy(ImVec2(0, 32));
                    }
                    
                    // CFG Display with styled background
                    {
                        ImDrawList* cfgDraw = ImGui::GetWindowDrawList();
                        ImVec2 cfgPos = ImGui::GetCursorScreenPos();
                        cfgDraw->AddRectFilled(cfgPos, ImVec2(cfgPos.x + 350, cfgPos.y + 130), 
                            IM_COL32(35, 40, 55, 255), 6.0f);
                        cfgDraw->AddRect(cfgPos, ImVec2(cfgPos.x + 350, cfgPos.y + 130), 
                            IM_COL32(80, 100, 140, 200), 6.0f);
                        
                        float ty = cfgPos.y + 8;
                        cfgDraw->AddText(ImVec2(cfgPos.x + 10, ty), IM_COL32(150, 200, 255, 255), "E  -> T E'");
                        cfgDraw->AddText(ImVec2(cfgPos.x + 10, ty + 18), IM_COL32(150, 200, 255, 255), "E' -> + T E' | - T E' | e");
                        cfgDraw->AddText(ImVec2(cfgPos.x + 10, ty + 36), IM_COL32(150, 255, 200, 255), "T  -> F T'");
                        cfgDraw->AddText(ImVec2(cfgPos.x + 10, ty + 54), IM_COL32(150, 255, 200, 255), "T' -> * F T' | / F T' | e");
                        cfgDraw->AddText(ImVec2(cfgPos.x + 10, ty + 72), IM_COL32(255, 200, 150, 255), "F  -> ( E ) | num | id");
                        cfgDraw->AddText(ImVec2(cfgPos.x + 10, ty + 95), IM_COL32(180, 180, 180, 255), "e = epsilon (empty)");
                        ImGui::Dummy(ImVec2(0, 140));
                    }
                    
                    // ═══════════════════════════════════════════════════════
                    // SECTION 2: PARSING TABLE
                    // ═══════════════════════════════════════════════════════
                    ImGui::Dummy(ImVec2(0, 15));
                    {
                        ImVec2 hdrPos = ImGui::GetCursorScreenPos();
                        sectionDraw->AddRectFilled(hdrPos, ImVec2(hdrPos.x + 520, hdrPos.y + 26), 
                            IM_COL32(50, 80, 70, 255), 4.0f);
                        sectionDraw->AddText(ImVec2(hdrPos.x + 10, hdrPos.y + 5), 
                            IM_COL32(150, 255, 200, 255), "2. LL(1) PARSING TABLE");
                        ImGui::Dummy(ImVec2(0, 32));
                    }
                    
                    // Parsing table
                    {
                        ImDrawList* tblDraw = ImGui::GetWindowDrawList();
                        ImVec2 tblPos = ImGui::GetCursorScreenPos();
                        float cellW = 60, cellH = 22;
                        int cols = 10, rows = 6;  // Added id and $ columns
                        
                        // Header background
                        tblDraw->AddRectFilled(tblPos, ImVec2(tblPos.x + cellW * cols, tblPos.y + cellH), 
                            IM_COL32(60, 70, 100, 255));
                        
                        // Column headers: empty, num, id, +, -, *, /, (, ), $
                        const char* headers[] = {"", "num", "id", "+", "-", "*", "/", "(", ")", "$"};
                        for (int c = 0; c < cols; c++) {
                            tblDraw->AddText(ImVec2(tblPos.x + c * cellW + 3, tblPos.y + 4), 
                                IM_COL32(200, 220, 255, 255), headers[c]);
                        }
                        
                        // LL(1) Parsing Table - Correct entries
                        // Row: E, E', T, T', F
                        // Cols: num, id, +, -, *, /, (, ), $
                        const char* rowLabels[] = {"E", "E'", "T", "T'", "F"};
                        const char* tableData[5][10] = {
                            //        num      id       +        -        *        /        (        )        $
                            {"E",    "TE'",   "TE'",   "-",     "-",     "-",     "-",     "TE'",   "-",     "-"},
                            {"E'",   "-",     "-",     "+TE'",  "-TE'",  "-",     "-",     "-",     "e",     "e"},
                            {"T",    "FT'",   "FT'",   "-",     "-",     "-",     "-",     "FT'",   "-",     "-"},
                            {"T'",   "-",     "-",     "e",     "e",     "*FT'",  "/FT'",  "-",     "e",     "e"},
                            {"F",    "num",   "id",    "-",     "-",     "-",     "-",     "(E)",   "-",     "-"}
                        };
                        
                        for (int r = 0; r < 5; r++) {
                            float rowY = tblPos.y + (r + 1) * cellH;
                            // Row label background
                            tblDraw->AddRectFilled(ImVec2(tblPos.x, rowY), 
                                ImVec2(tblPos.x + cellW, rowY + cellH), IM_COL32(50, 55, 70, 255));
                            
                            for (int c = 0; c < cols; c++) {
                                // Cell border
                                tblDraw->AddRect(ImVec2(tblPos.x + c * cellW, rowY), 
                                    ImVec2(tblPos.x + (c + 1) * cellW, rowY + cellH), 
                                    IM_COL32(80, 90, 110, 200));
                                
                                ImU32 textCol = (c == 0) ? IM_COL32(200, 200, 255, 255) : 
                                               (tableData[r][c][0] == '-') ? IM_COL32(100, 100, 100, 255) :
                                               (tableData[r][c][0] == 'e') ? IM_COL32(180, 180, 120, 255) :
                                               IM_COL32(180, 220, 180, 255);
                                tblDraw->AddText(ImVec2(tblPos.x + c * cellW + 3, rowY + 4), 
                                    textCol, tableData[r][c]);
                            }
                        }
                        ImGui::Dummy(ImVec2(0, (rows + 1) * cellH + 10));
                    }
                    
                    // ═══════════════════════════════════════════════════════
                    // SECTION 3: ANIMATION CONTROLS
                    // ═══════════════════════════════════════════════════════
                    ImGui::Dummy(ImVec2(0, 15));
                    {
                        ImVec2 hdrPos = ImGui::GetCursorScreenPos();
                        sectionDraw->AddRectFilled(hdrPos, ImVec2(hdrPos.x + 520, hdrPos.y + 26), 
                            IM_COL32(80, 70, 50, 255), 4.0f);
                        sectionDraw->AddText(ImVec2(hdrPos.x + 10, hdrPos.y + 5), 
                            IM_COL32(255, 220, 150, 255), "3. STEP-BY-STEP PARSING");
                        ImGui::Dummy(ImVec2(0, 32));
                    }
                    
                    ImGui::Text("Step: %d / %d", pdaStep + 1, (int)pdaParser.trace.size());
                    ImGui::SameLine(150);
                    
                    if (ImGui::Button(isPlaying ? "II PAUSE" : "> PLAY", ImVec2(90, 28))) {
                        isPlaying = !isPlaying;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(">> STEP", ImVec2(90, 28))) {
                        if (pdaStep < (int)pdaParser.trace.size() - 1) pdaStep++;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("RESET", ImVec2(70, 28))) {
                        pdaStep = 0;
                        isPlaying = false;
                    }
                    
                    // Animation speed control
                    ImGui::Text("Speed:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(150);
                    ImGui::SliderFloat("##speed", &playSpeed, 0.1f, 2.0f, "%.1fs");
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.6f, 0.7f, 0.8f, 1), "(delay per step)");
                    
                    ImGui::SetNextItemWidth(350);
                    ImGui::SliderInt("##pdastep", &pdaStep, 0, (int)pdaParser.trace.size() > 0 ? (int)pdaParser.trace.size() - 1 : 0);
                    
                    // ═══════════════════════════════════════════════════════
                    // SECTION 4: CURRENT STEP
                    // ═══════════════════════════════════════════════════════
                    ImGui::Dummy(ImVec2(0, 20));
                    if (pdaStep < (int)pdaParser.trace.size()) {
                        auto& step = pdaParser.trace[pdaStep];
                        
                        bool isError = step.action.find("Error") != std::string::npos;
                        bool isAccept = step.action.find("Accept") != std::string::npos;
                        
                        // Section header
                        ImVec2 hdrPos = ImGui::GetCursorScreenPos();
                        ImU32 hdrBg = isError ? IM_COL32(100, 50, 50, 255) : 
                                     isAccept ? IM_COL32(50, 90, 60, 255) : 
                                     IM_COL32(60, 60, 80, 255);
                        sectionDraw->AddRectFilled(hdrPos, ImVec2(hdrPos.x + 520, hdrPos.y + 26), hdrBg, 4.0f);
                        std::string sec4Title = "4. CURRENT: " + step.action;
                        sectionDraw->AddText(ImVec2(hdrPos.x + 10, hdrPos.y + 5), 
                            IM_COL32(255, 255, 255, 255), sec4Title.c_str());
                        ImGui::Dummy(ImVec2(0, 32));
                        
                        // Action box
                        ImDrawList* actDraw = ImGui::GetWindowDrawList();
                        ImVec2 actPos = ImGui::GetCursorScreenPos();
                        
                        ImU32 actBg = isError ? IM_COL32(100, 50, 50, 255) : 
                                     isAccept ? IM_COL32(50, 100, 60, 255) : 
                                     IM_COL32(50, 60, 80, 255);
                        ImU32 actBorder = isError ? IM_COL32(200, 100, 100, 255) : 
                                         isAccept ? IM_COL32(100, 200, 120, 255) : 
                                         IM_COL32(100, 120, 180, 255);
                        
                        actDraw->AddRectFilled(actPos, ImVec2(actPos.x + 500, actPos.y + 35), actBg, 6.0f);
                        actDraw->AddRect(actPos, ImVec2(actPos.x + 500, actPos.y + 35), actBorder, 6.0f);
                        
                        std::string actionLabel = "ACTION: " + step.action;
                        actDraw->AddText(ImVec2(actPos.x + 10, actPos.y + 9), IM_COL32(255, 255, 255, 255), actionLabel.c_str());
                        ImGui::Dummy(ImVec2(0, 42));
                        
                        // Stack visualization
                        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1, 1), "STACK:");
                        std::stringstream ss(step.stackState);
                        std::string seg;
                        std::vector<std::string> stackVec;
                        while (std::getline(ss, seg, ' ')) if (!seg.empty()) stackVec.push_back(seg);
                        Viz::DrawStack(stackVec);
                        
                        // Input remaining
                        ImGui::Dummy(ImVec2(0, 10));
                        ImGui::TextColored(ImVec4(0.8f, 1, 0.6f, 1), "INPUT REMAINING:");
                        ImGui::TextColored(ImVec4(1, 1, 1, 1), "  %s", step.inputRemaining.c_str());
                        
                        // Explanation
                        ImGui::Dummy(ImVec2(0, 10));
                        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.6f, 1), "EXPLANATION:");
                        ImGui::TextWrapped("  %s", step.explanation.c_str());
                        
                        // ═══════════════════════════════════════════════════════
                        // TOKEN STREAM ANIMATION (above PDA Diagram)
                        // ═══════════════════════════════════════════════════════
                        ImGui::Dummy(ImVec2(0, 15));
                        {
                            ImDrawList* tokDraw = ImGui::GetWindowDrawList();
                            ImVec2 tokPos = ImGui::GetCursorScreenPos();
                            
                            tokDraw->AddRectFilled(tokPos, ImVec2(tokPos.x + 520, tokPos.y + 50), 
                                IM_COL32(30, 35, 50, 255), 4.0f);
                            tokDraw->AddRect(tokPos, ImVec2(tokPos.x + 520, tokPos.y + 50), 
                                IM_COL32(80, 100, 140, 200), 4.0f);
                            
                            float tx = tokPos.x + 10;
                            float ty = tokPos.y + 6;
                            
                            // Get current input position
                            int currentInputPos = step.inputPosition;
                            
                            tokDraw->AddText(ImVec2(tx, ty), IM_COL32(180, 200, 230, 255), "INPUT:");
                            
                            // Draw tokens with highlighting
                            float tokenX = tx + 55;
                            float tokenY = ty - 2;
                            for (int ti = 0; ti < (int)cachedLexer.tokens.size(); ti++) {
                                const auto& tok = cachedLexer.tokens[ti];
                                std::string tokLabel;
                                switch(tok.type) {
                                    case TOK_NUMBER: tokLabel = tok.value; break;
                                    case TOK_ID: tokLabel = tok.value; break;
                                    case TOK_PLUS: tokLabel = "+"; break;
                                    case TOK_MINUS: tokLabel = "-"; break;
                                    case TOK_TIMES: tokLabel = "*"; break;
                                    case TOK_DIVIDE: tokLabel = "/"; break;
                                    case TOK_LPAREN: tokLabel = "("; break;
                                    case TOK_RPAREN: tokLabel = ")"; break;
                                    case TOK_END: tokLabel = "$"; break;
                                    default: tokLabel = "?"; break;
                                }
                                
                                ImVec2 textSize = ImGui::CalcTextSize(tokLabel.c_str());
                                float boxW = textSize.x + 10;
                                float boxH = 22;
                                
                                bool isCurrent = (ti == currentInputPos);
                                bool isConsumed = (ti < currentInputPos);
                                
                                ImU32 bgCol, textCol;
                                if (isCurrent) {
                                    bgCol = IM_COL32(255, 200, 50, 255);
                                    textCol = IM_COL32(0, 0, 0, 255);
                                } else if (isConsumed) {
                                    bgCol = IM_COL32(50, 55, 65, 255);
                                    textCol = IM_COL32(100, 100, 110, 255);
                                } else {
                                    bgCol = IM_COL32(60, 80, 120, 255);
                                    textCol = IM_COL32(220, 230, 255, 255);
                                }
                                
                                tokDraw->AddRectFilled(ImVec2(tokenX, tokenY), 
                                    ImVec2(tokenX + boxW, tokenY + boxH), bgCol, 3.0f);
                                tokDraw->AddText(ImVec2(tokenX + 5, tokenY + 3), textCol, tokLabel.c_str());
                                
                                tokenX += boxW + 4;
                            }
                            
                            // Legend below
                            float ly = tokPos.y + 30;
                            tokDraw->AddRectFilled(ImVec2(tx, ly), ImVec2(tx + 10, ly + 10), IM_COL32(255, 200, 50, 255), 2.0f);
                            tokDraw->AddText(ImVec2(tx + 14, ly - 2), IM_COL32(150, 160, 180, 255), "Current");
                            tokDraw->AddRectFilled(ImVec2(tx + 75, ly), ImVec2(tx + 85, ly + 10), IM_COL32(50, 55, 65, 255), 2.0f);
                            tokDraw->AddText(ImVec2(tx + 89, ly - 2), IM_COL32(150, 160, 180, 255), "Consumed");
                            
                            ImGui::Dummy(ImVec2(0, 55));
                        }
                        
                        // ═══════════════════════════════════════════════════════
                        // SECTION 5: PDA DIAGRAM
                        // ═══════════════════════════════════════════════════════
                        ImGui::Dummy(ImVec2(0, 10));
                        {
                            ImVec2 hdrPos = ImGui::GetCursorScreenPos();
                            sectionDraw->AddRectFilled(hdrPos, ImVec2(hdrPos.x + 520, hdrPos.y + 26), 
                                IM_COL32(70, 60, 90, 255), 4.0f);
                            sectionDraw->AddText(ImVec2(hdrPos.x + 10, hdrPos.y + 5), 
                                IM_COL32(200, 180, 255, 255), "5. PDA STATE DIAGRAM");
                            ImGui::Dummy(ImVec2(0, 32));
                        }
                        std::string stackTop = "";
                        if (!stackVec.empty()) stackTop = stackVec.back();
                        Viz::DrawPDAAnimated(step.action, stackTop);
                    }
                    
                    // ═══════════════════════════════════════════════════════
                    // SECTION 6: PARSE TRACE  
                    // ═══════════════════════════════════════════════════════
                    ImGui::Dummy(ImVec2(0, 20));
                    {
                        ImVec2 hdrPos = ImGui::GetCursorScreenPos();
                        sectionDraw->AddRectFilled(hdrPos, ImVec2(hdrPos.x + 520, hdrPos.y + 26), 
                            IM_COL32(80, 60, 100, 255), 4.0f);
                        sectionDraw->AddText(ImVec2(hdrPos.x + 10, hdrPos.y + 5), 
                            IM_COL32(220, 180, 255, 255), "6. COMPLETE PARSE TRACE");
                        ImGui::Dummy(ImVec2(0, 32));
                    }
                    ImGui::Separator();
                    
                    ImGui::BeginChild("TraceList", ImVec2(0, 150), true);
                    for (size_t i = 0; i < pdaParser.trace.size(); i++) {
                        auto& t = pdaParser.trace[i];
                        bool isCurrent = ((int)i == pdaStep);
                        
                        ImVec4 col = isCurrent ? ImVec4(0.4f, 1.0f, 0.4f, 1) :
                                    (t.action.find("Error") != std::string::npos) ? ImVec4(1.0f, 0.5f, 0.5f, 1) :
                                    (t.action.find("Accept") != std::string::npos) ? ImVec4(0.5f, 1.0f, 0.6f, 1) :
                                    ImVec4(0.7f, 0.7f, 0.8f, 1);
                        
                        if (isCurrent) ImGui::TextColored(col, ">> ");
                        else ImGui::TextColored(col, "   ");
                        ImGui::SameLine();
                        ImGui::TextColored(col, "%d: %s", (int)i + 1, t.action.c_str());
                    }
                    ImGui::EndChild();
                }
                
                ImGui::EndChild();
            }
        }
        ImGui::EndChild();
        
        // Navigation buttons
        ImGui::Dummy(ImVec2(0, 10));
        if (stage > 0) {
            if (ImGui::Button("< BACK", ImVec2(100, 35))) {
                stage--;
                isPlaying = false;
            }
            ImGui::SameLine();
        }
        
        // Stage-specific action button
        if (stage == 0) {
            if (ImGui::Button("NEXT: Enter Expression >", ImVec2(250, 35))) {
                stage = 1;
            }
        }
        else if (stage == 1) {
            if (ImGui::Button("TOKENIZE WITH DFA >>", ImVec2(250, 35))) {
                lexerDFA = subsetConstruct(lexerNFA);
                dfaTotalSteps = (int)lexerDFA.size();
                dfaStep = 0;
                
                // Initialize tokenization animation
                cachedLexer.setInput(inputBuf);
                tokenTotalSteps = (int)cachedLexer.steps.size();
                tokenStep = 0;
                isPlaying = false;
                
                logs.push_back("✓ NFA→DFA conversion complete: " + std::to_string(lexerDFA.size()) + " states");
                logs.push_back("✓ Tokenizing input: \"" + std::string(inputBuf) + "\"");
                logs.push_back("✓ Found " + std::to_string(cachedLexer.tokens.size() - 1) + " tokens");
                
                // Log any validation errors
                if (cachedLexer.hasErrors) {
                    logs.push_back("⚠ VALIDATION ERRORS DETECTED:");
                    for (const auto& err : cachedLexer.errors) {
                        if (err.severity == "ERROR") {
                            logs.push_back("  ✗ " + err.message + " (pos " + std::to_string(err.position) + ")");
                        } else {
                            logs.push_back("  ⚡ " + err.message + " (pos " + std::to_string(err.position) + ")");
                        }
                    }
                } else if (!cachedLexer.errors.empty()) {
                    logs.push_back("⚡ WARNINGS:");
                    for (const auto& err : cachedLexer.errors) {
                        logs.push_back("  ⚡ " + err.message);
                    }
                } else {
                    logs.push_back("✓ Validation passed - no errors");
                }
                
                stage = 2;
            }
        }
        else if (stage == 2) {
            if (cachedLexer.hasErrors) {
                // Show disabled button with error message
                ImGui::BeginDisabled();
                ImGui::Button("PARSE WITH PDA >> (FIX ERRORS)", ImVec2(280, 35));
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Fix errors first!");
            } else {
                if (ImGui::Button("PARSE WITH PDA >>", ImVec2(250, 35))) {
                    Lexer tempLex(inputBuf);
                    pdaParser.parse(tempLex.tokens);
                    pdaStep = 0;
                    logs.push_back("✓ Tokens passed to PDA parser");
                    logs.push_back("✓ PDA analysis: " + std::to_string(pdaParser.trace.size()) + " steps");
                    stage = 3;
                }
            }
        }
        else if (stage == 3) {
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "✓ Pipeline Complete!");
        }
        
        ImGui::NextColumn();

        // RIGHT: Education & Logs
        ImGui::BeginChild("EduPanel", ImVec2(0, 700), true);
        {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "EDUCATIONAL GUIDE");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 5));
            
            if (stage == 0) {
                ImGui::TextColored(ImVec4(1, 0.9f, 0.5f, 1), "Thompson's Construction");
                ImGui::TextWrapped("This NFA recognizes ALL token types:");
                ImGui::BulletText("ID: letter(alnum|_)*");
                ImGui::BulletText("NUMBER: digit+(.digit+)?");
                ImGui::BulletText("Operators: + - * /");
                ImGui::BulletText("Parentheses: ( )");
                ImGui::BulletText("Whitespace: (space|tab)+");
                ImGui::Dummy(ImVec2(0, 10));
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1, 1), "Key Concepts:");
                ImGui::TextWrapped("• q0 is the super-start state");
                ImGui::TextWrapped("• Epsilon (ε) transitions allow non-determinism");
                ImGui::TextWrapped("• Green nodes are accept states with token labels");
            }
            else if (stage == 1) {
                ImGui::TextColored(ImVec4(1, 0.9f, 0.5f, 1), "Lexical Analysis");
                ImGui::TextWrapped("The lexer breaks input into tokens:");
                ImGui::Dummy(ImVec2(0, 5));
                ImGui::TextColored(ImVec4(0.7f, 1, 0.7f, 1), "NUMBER");
                ImGui::TextWrapped("  Matches: 0-9, decimals");
                ImGui::TextColored(ImVec4(0.7f, 0.8f, 1, 1), "ID");
                ImGui::TextWrapped("  Matches: variables, keywords");
                ImGui::TextColored(ImVec4(1, 0.9f, 0.6f, 1), "OPERATORS");
                ImGui::TextWrapped("  Matches: + - * /");
            }
            else if (stage == 2) {
                ImGui::TextColored(ImVec4(1, 0.9f, 0.5f, 1), "Subset Construction");
                ImGui::TextWrapped("Converting NFA to DFA:");
                ImGui::BulletText("Each DFA state = set of NFA states");
                ImGui::BulletText("ε-closure computed at each step");
                ImGui::BulletText("Deterministic: one transition per input");
                ImGui::Dummy(ImVec2(0, 10));
                ImGui::Text("DFA States: %d", (int)lexerDFA.size());
            }
            else if (stage == 3) {
                ImGui::TextColored(ImVec4(1, 0.9f, 0.5f, 1), "PDA Parsing");
                ImGui::TextWrapped("Syntax analysis using a pushdown automaton:");
                ImGui::BulletText("Stack tracks parse state");
                ImGui::BulletText("Grammar rules applied");
                ImGui::BulletText("Shift/Reduce actions");
                ImGui::Dummy(ImVec2(0, 10));
                
                // Playback for PDA
                if (!pdaParser.trace.empty()) {
                    ImGui::Text("Step %d / %d", pdaStep + 1, (int)pdaParser.trace.size());
                    if (ImGui::Button(isPlaying ? "PAUSE" : "PLAY", ImVec2(80, 25))) {
                        isPlaying = !isPlaying;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("STEP", ImVec2(80, 25))) {
                        if (pdaStep < (int)pdaParser.trace.size() - 1) pdaStep++;
                    }
                    ImGui::SliderInt("##pdastep", &pdaStep, 0, (int)pdaParser.trace.size() - 1);
                    
                    if (pdaStep < (int)pdaParser.trace.size()) {
                        auto& s = pdaParser.trace[pdaStep];
                        ImGui::Dummy(ImVec2(0, 5));
                        ImGui::TextColored(ImVec4(1, 1, 0.5f, 1), "Action: %s", s.action.c_str());
                        ImGui::TextWrapped("%s", s.explanation.c_str());
                    }
                }
            }
            
            // Log section
            ImGui::Dummy(ImVec2(0, 20));
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1, 1), "SYSTEM LOG");
            ImGui::Separator();
            ImGui::BeginChild("LogScroll", ImVec2(0, 150), true);
            for (auto& log : logs) {
                ImGui::TextWrapped("%s", log.c_str());
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();
        
        ImGui::Columns(1);
        ImGui::End();

        window.clear(sf::Color(15, 15, 20));
        ImGui::SFML::Render(window);
        window.display();
    }
    
    ImGui::SFML::Shutdown();
    return 0;
}