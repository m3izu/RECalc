#pragma once
#include "imgui.h"
#include "DFA.h"
#include "PDA.h"
#include "NFA.h"
#include "SubsetConstruction.h"
#include "LexerNFA.h"
#include <string>
#include <cmath>
#include <vector>
#include <sstream>

namespace Viz {

    // Helper: Linear Interpolation for smooth colors
    inline ImU32 LerpColor(ImU32 c1, ImU32 c2, float t) {
        int r1 = (c1 >> 0) & 0xFF; int g1 = (c1 >> 8) & 0xFF; int b1 = (c1 >> 16) & 0xFF; int a1 = (c1 >> 24) & 0xFF;
        int r2 = (c2 >> 0) & 0xFF; int g2 = (c2 >> 8) & 0xFF; int b2 = (c2 >> 16) & 0xFF; int a2 = (c2 >> 24) & 0xFF;
        int r = (int)(r1 + (r2 - r1) * t);
        int g = (int)(g1 + (g2 - g1) * t);
        int b = (int)(b1 + (b2 - b1) * t);
        int a = (int)(a1 + (a2 - a1) * t);
        return IM_COL32(r, g, b, a);
    }

    inline void DrawArrow(ImDrawList* draw_list, ImVec2 p1, ImVec2 p2, ImU32 col, float thickness=2.5f) {
        // Shorten the arrow so it doesn't overlap the node circles
        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        float dist = sqrt(dx*dx + dy*dy);
        if (dist > 50.0f) { // Only shorten if reasonably long
            float margin = 28.0f; // Radius of node + padding
            float ratio = (dist - margin) / dist;
            p2.x = p1.x + dx * ratio;
            p2.y = p1.y + dy * ratio;
            
            float startRatio = margin / dist;
            p1.x = p1.x + dx * startRatio;
            p1.y = p1.y + dy * startRatio;
        }

        draw_list->AddLine(p1, p2, col, thickness);
        
        float angle = atan2f(dy, dx);
        float arrowSize = 12.0f;
        
        // Arrow head
        ImVec2 pA = ImVec2(p2.x - arrowSize * cosf(angle - 0.5f), p2.y - arrowSize * sinf(angle - 0.5f));
        ImVec2 pB = ImVec2(p2.x - arrowSize * cosf(angle + 0.5f), p2.y - arrowSize * sinf(angle + 0.5f));
        draw_list->AddTriangleFilled(p2, pA, pB, col);
    }

    inline void DrawNode(ImDrawList* draw, ImVec2 pos, const char* label, bool active, bool isDouble = false) {
        ImU32 fillCol = active ? IM_COL32(46, 204, 113, 255) : IM_COL32(52, 73, 94, 255);
        ImU32 borderCol = active ? IM_COL32(236, 240, 241, 255) : IM_COL32(149, 165, 166, 255);
        ImU32 textCol = IM_COL32(255, 255, 255, 255);

        float radius = 24.0f;
        draw->AddCircleFilled(pos, radius, fillCol);
        draw->AddCircle(pos, radius, borderCol, 0, 3.0f);
        if (isDouble) draw->AddCircle(pos, radius - 4.0f, borderCol, 0, 2.0f);
        
        ImVec2 textSize = ImGui::CalcTextSize(label);
        draw->AddText(ImVec2(pos.x - textSize.x * 0.5f, pos.y - textSize.y * 0.5f), textCol, label);
    }

    inline void DrawCurvedArrow(ImDrawList* draw, ImVec2 p1, ImVec2 p2, ImU32 col, float thickness=2.0f, float offset=30.0f) {
        ImVec2 mid = ImVec2((p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f + offset);
        draw->AddBezierQuadratic(p1, mid, p2, col, thickness);
        
        // Arrow head at p2 (approximate direction from mid)
        float angle = atan2f(p2.y - mid.y, p2.x - mid.x);
        float arrowSize = 12.0f;
        draw->AddTriangleFilled(
            p2,
            ImVec2(p2.x - arrowSize * cosf(angle - 0.5f), p2.y - arrowSize * sinf(angle - 0.5f)),
            ImVec2(p2.x - arrowSize * cosf(angle + 0.5f), p2.y - arrowSize * sinf(angle + 0.5f)),
            col
        );
    }

    inline void DrawDFA(const DFALexer& dfa, int stepIndex) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 origin = ImGui::GetCursorScreenPos(); // Relative origin
        
        // Layout
        // Start roughly center-left
        ImVec2 pStart  = ImVec2(origin.x + 100, origin.y + 250);
        
        // Identifiers (Top path)
        ImVec2 pId     = ImVec2(origin.x + 300, origin.y + 100);
        
        // Numbers (Middle path)
        ImVec2 pNum    = ImVec2(origin.x + 250, origin.y + 250);
        ImVec2 pPoint  = ImVec2(origin.x + 400, origin.y + 250);
        ImVec2 pFrac   = ImVec2(origin.x + 550, origin.y + 250);
        
        // Operators (Bottom path)
        ImVec2 pOp     = ImVec2(origin.x + 250, origin.y + 400);
        ImVec2 pParen  = ImVec2(origin.x + 350, origin.y + 400);
        
        ImVec2 pError  = ImVec2(origin.x + 100, origin.y + 400);

        // Determine Activity
        DFAState currentTo = DFAState::START;
        DFAState currentFrom = DFAState::START;
        bool hasStep = (stepIndex >= 0 && stepIndex < dfa.history.size());
        
        if (hasStep) {
            currentTo = dfa.history[stepIndex].toState;
            currentFrom = dfa.history[stepIndex].fromState;
        }

        ImU32 colInactive = IM_COL32(189, 195, 199, 255); // Silver
        ImU32 colActive = IM_COL32(241, 196, 15, 255);    // Gold

        // Helper
        auto isActiveStats = [&](DFAState f, DFAState t) {
            if (!hasStep) return false;
            // Simplify OP states etc
            bool fromOp = (currentFrom >= DFAState::OP_PLUS && currentFrom <= DFAState::OP_DIVIDE);
            bool toOp = (currentTo >= DFAState::OP_PLUS && currentTo <= DFAState::OP_DIVIDE);
            bool fromParen = (currentFrom == DFAState::LPAREN || currentFrom == DFAState::RPAREN);
            bool toParen = (currentTo == DFAState::LPAREN || currentTo == DFAState::RPAREN);
            bool fMatch = (f == DFAState::OP_PLUS) ? fromOp : ((f == DFAState::LPAREN) ? fromParen : currentFrom == f);
            bool tMatch = (t == DFAState::OP_PLUS) ? toOp : ((t == DFAState::LPAREN) ? toParen : currentTo == t);
            return fMatch && tMatch;
        };

        // --- Edges ---
        
        // ID PATH
        draw->AddText(ImVec2(origin.x+200, origin.y+80), colInactive, "[a-z]");
        DrawArrow(draw, pStart, pId, isActiveStats(DFAState::START, DFAState::IDENTIFIER) ? colActive : colInactive);
        DrawCurvedArrow(draw, pId, pStart, isActiveStats(DFAState::IDENTIFIER, DFAState::START) ? colActive : colInactive, 2.0f, -40.0f);
        // ID loop
        {
             ImVec2 center(pId.x, pId.y - 35);
             ImU32 col = isActiveStats(DFAState::IDENTIFIER, DFAState::IDENTIFIER) ? colActive : colInactive;
             draw->PathArcTo(center, 15.0f, 0.0f, 6.28f);
             draw->PathStroke(col, 0, 2.5f);
        }

        // NUMBER PATH
        draw->AddText(ImVec2(origin.x+180, origin.y+230), colInactive, "[0-9]");
        DrawArrow(draw, pStart, pNum, isActiveStats(DFAState::START, DFAState::NUMBER) ? colActive : colInactive);
        DrawCurvedArrow(draw, pNum, pStart, isActiveStats(DFAState::NUMBER, DFAState::START) ? colActive : colInactive, 2.0f, -40.0f);
        // Num loop
        {
             ImVec2 center(pNum.x, pNum.y - 35);
             ImU32 col = isActiveStats(DFAState::NUMBER, DFAState::NUMBER) ? colActive : colInactive;
             draw->PathArcTo(center, 15.0f, 0.0f, 6.28f);
             draw->PathStroke(col, 0, 2.5f);
        }
        // Num -> Point
        DrawArrow(draw, pNum, pPoint, isActiveStats(DFAState::NUMBER, DFAState::POINT) ? colActive : colInactive);
        // Point -> Frac
        DrawArrow(draw, pPoint, pFrac, isActiveStats(DFAState::POINT, DFAState::FRACTION) ? colActive : colInactive);
        // Frac Loop
         {
             ImVec2 center(pFrac.x, pFrac.y - 35);
             ImU32 col = isActiveStats(DFAState::FRACTION, DFAState::FRACTION) ? colActive : colInactive;
             draw->PathArcTo(center, 15.0f, 0.0f, 6.28f);
             draw->PathStroke(col, 0, 2.5f);
        }
        // Frac -> Start
        DrawCurvedArrow(draw, pFrac, pStart, isActiveStats(DFAState::FRACTION, DFAState::START) ? colActive : colInactive, 2.0f, 100.0f);

        // OP PATH
        DrawArrow(draw, pStart, pOp, isActiveStats(DFAState::START, DFAState::OP_PLUS) ? colActive : colInactive);
        DrawCurvedArrow(draw, pOp, pStart, isActiveStats(DFAState::OP_PLUS, DFAState::START) ? colActive : colInactive, 2.0f, 40.0f);

        // PAREN PATH
        DrawArrow(draw, pStart, pParen, isActiveStats(DFAState::START, DFAState::LPAREN) ? colActive : colInactive);
        DrawCurvedArrow(draw, pParen, pStart, isActiveStats(DFAState::LPAREN, DFAState::START) ? colActive : colInactive, 2.0f, 50.0f);
        

        // --- Nodes ---
        DrawNode(draw, pStart, "START", currentTo == DFAState::START);
        
        DrawNode(draw, pId, "ID", currentTo == DFAState::IDENTIFIER, true);
        
        DrawNode(draw, pNum, "INT", currentTo == DFAState::NUMBER, true);
        DrawNode(draw, pPoint, ".", currentTo == DFAState::POINT, false);
        DrawNode(draw, pFrac, "FRAC", currentTo == DFAState::FRACTION, true);
        
        bool activeOp = (currentTo >= DFAState::OP_PLUS && currentTo <= DFAState::OP_DIVIDE);
        DrawNode(draw, pOp, "OP", activeOp, true);
        
        bool activeParen = (currentTo == DFAState::LPAREN || currentTo == DFAState::RPAREN);
        DrawNode(draw, pParen, "( )", activeParen, true);
        
        DrawNode(draw, pError, "ERR", currentTo == DFAState::ERROR);
    }

    inline void DrawNFA(const ThompsonNFA& nfa, const std::string& activeStateDesc) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 origin = ImGui::GetCursorScreenPos();
        
        if (nfa.owned.empty()) {
            draw->AddText(origin, IM_COL32(150,150,150,255), "No NFA built yet. Enter a regex and click BUILD.");
            return;
        }

        int numStates = (int)nfa.owned.size();
        
        // Horizontal tree-like layout (left to right flow)
        // Compute depth for each state using BFS from start
        std::map<int, int> depth;
        std::map<int, int> row;
        if (nfa.start) {
            std::queue<int> q;
            q.push(nfa.start->id);
            depth[nfa.start->id] = 0;
            row[nfa.start->id] = 0;
            
            std::map<int, int> rowCount; // count per depth
            rowCount[0] = 1;
            
            while (!q.empty()) {
                int curr = q.front(); q.pop();
                int currDepth = depth[curr];
                
                for (const auto& uState : nfa.owned) {
                    if (uState->id == curr) {
                        for (const auto& [key, dests] : uState->trans) {
                            for (auto* vState : dests) {
                                if (depth.find(vState->id) == depth.end()) {
                                    depth[vState->id] = currDepth + 1;
                                    if (rowCount.find(currDepth + 1) == rowCount.end()) rowCount[currDepth + 1] = 0;
                                    row[vState->id] = rowCount[currDepth + 1]++;
                                    q.push(vState->id);
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
        
        // Assign unvisited states
        int maxDepth = 0;
        for (auto& [id, d] : depth) maxDepth = std::max(maxDepth, d);
        for (const auto& uState : nfa.owned) {
            if (depth.find(uState->id) == depth.end()) {
                depth[uState->id] = maxDepth + 1;
                row[uState->id] = 0;
            }
        }
        
        // Compute positions
        float spacingX = 80.0f;
        float spacingY = 50.0f;
        std::map<int, int> depthRowCount;
        for (auto& [id, r] : row) depthRowCount[depth[id]] = std::max(depthRowCount[depth[id]], r + 1);
        
        std::vector<ImVec2> positions(numStates);
        for (const auto& uState : nfa.owned) {
            int id = uState->id;
            int d = depth[id];
            int r = row[id];
            int totalRows = depthRowCount[d];
            float offsetY = (r - totalRows * 0.5f + 0.5f) * spacingY;
            positions[id] = ImVec2(origin.x + 60 + d * spacingX, origin.y + 200 + offsetY);
        }

        ImU32 lineCol = IM_COL32(80, 80, 80, 255);
        ImU32 epsCol = IM_COL32(100, 100, 100, 255);
        ImU32 charCol = IM_COL32(80, 80, 80, 255);

        // Draw edges first
        for (const auto& uState : nfa.owned) {
            int uId = uState->id;
            ImVec2 p1 = positions[uId];
            
            for (auto const& [key, dests] : uState->trans) {
                for (auto* vState : dests) {
                    int vId = vState->id;
                    ImVec2 p2 = positions[vId];
                    
                    std::string label = (key == 0) ? "ε" : std::string(1, key);
                    
                    float dx = p2.x - p1.x;
                    float dy = p2.y - p1.y;
                    float dist = sqrtf(dx*dx + dy*dy);
                    if (dist < 1.0f) dist = 1.0f;
                    
                    float margin = 18.0f;
                    ImVec2 start(p1.x + dx/dist * margin, p1.y + dy/dist * margin);
                    ImVec2 end(p2.x - dx/dist * margin, p2.y - dy/dist * margin);
                    
                    // Slight curve for overlapping edges
                    float perpX = -dy / dist * 8.0f;
                    float perpY = dx / dist * 8.0f;
                    ImVec2 ctrl((start.x + end.x)*0.5f + perpX, (start.y + end.y)*0.5f + perpY);
                    
                    draw->AddBezierQuadratic(start, ctrl, end, lineCol, 1.5f);
                    
                    // Arrow head
                    float angle = atan2f(end.y - ctrl.y, end.x - ctrl.x);
                    float as = 8.0f;
                    draw->AddTriangleFilled(
                        end,
                        ImVec2(end.x - as * cosf(angle - 0.4f), end.y - as * sinf(angle - 0.4f)),
                        ImVec2(end.x - as * cosf(angle + 0.4f), end.y - as * sinf(angle + 0.4f)),
                        lineCol
                    );
                    
                    // Edge label
                    ImVec2 labelPos(ctrl.x - 4, ctrl.y - 12);
                    draw->AddText(labelPos, IM_COL32(60, 60, 60, 255), label.c_str());
                }
            }
        }

        // Draw Nodes
        for (const auto& uState : nfa.owned) {
            int id = uState->id;
            ImVec2 pos = positions[id];
            
            bool isAccept = uState->accept;
            bool isStart = (nfa.start == uState.get());
            
            float nodeRadius = 16.0f;
            
            // White fill for regular, green for accept
            ImU32 fillCol = isAccept ? IM_COL32(144, 238, 144, 255) : IM_COL32(255, 255, 255, 255);
            ImU32 borderCol = IM_COL32(60, 60, 60, 255);
            
            draw->AddCircleFilled(pos, nodeRadius, fillCol);
            draw->AddCircle(pos, nodeRadius, borderCol, 0, 1.5f);
            if (isAccept) draw->AddCircle(pos, nodeRadius - 3.0f, borderCol, 0, 1.0f);
            
            // Start arrow
            if (isStart) {
                ImVec2 arrowStart(pos.x - 35, pos.y);
                ImVec2 arrowEnd(pos.x - nodeRadius - 2, pos.y);
                draw->AddLine(arrowStart, arrowEnd, IM_COL32(0, 0, 200, 255), 2.0f);
                draw->AddTriangleFilled(
                    arrowEnd,
                    ImVec2(arrowEnd.x - 6, arrowEnd.y - 4),
                    ImVec2(arrowEnd.x - 6, arrowEnd.y + 4),
                    IM_COL32(0, 0, 200, 255)
                );
            }
            
            // Node label
            std::string label = "q" + std::to_string(id);
            ImVec2 ts = ImGui::CalcTextSize(label.c_str());
            draw->AddText(ImVec2(pos.x - ts.x*0.5f, pos.y - ts.y*0.5f), IM_COL32(0,0,0,255), label.c_str());
        }
        
        // Legend
        ImVec2 lp(origin.x + 10, origin.y + 380);
        draw->AddCircleFilled(ImVec2(lp.x + 10, lp.y), 8, IM_COL32(255,255,255,255));
        draw->AddCircle(ImVec2(lp.x + 10, lp.y), 8, IM_COL32(60,60,60,255));
        draw->AddText(ImVec2(lp.x + 25, lp.y - 7), IM_COL32(150,150,150,255), "State");
        draw->AddCircleFilled(ImVec2(lp.x + 80, lp.y), 8, IM_COL32(144,238,144,255));
        draw->AddCircle(ImVec2(lp.x + 80, lp.y), 8, IM_COL32(60,60,60,255));
        draw->AddCircle(ImVec2(lp.x + 80, lp.y), 5, IM_COL32(60,60,60,255));
        draw->AddText(ImVec2(lp.x + 95, lp.y - 7), IM_COL32(150,150,150,255), "Accept");
        draw->AddText(ImVec2(lp.x + 160, lp.y - 7), IM_COL32(100,100,100,255), "ε = epsilon");
    }

    inline void DrawStack(const std::vector<std::string>& stack) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 origin = ImGui::GetCursorScreenPos();
        float boxWidth = 50.0f;
        float boxHeight = 26.0f;
        float spacing = 3.0f;
        
        // Draw stack HORIZONTAL (left to right, bottom to top of stack)
        float startX = origin.x + 10;
        float y = origin.y + 5;
        
        // Label first
        draw->AddText(ImVec2(startX, y + 4), IM_COL32(150,150,150,255), "[$");
        startX += 25;
        
        for (size_t i = 0; i < stack.size(); ++i) {
            // Adjust box width based on content
            ImVec2 textSize = ImGui::CalcTextSize(stack[i].c_str());
            float thisBoxW = std::max(boxWidth, textSize.x + 16);
            
            ImVec2 boxMin(startX, y);
            ImVec2 boxMax(boxMin.x + thisBoxW, boxMin.y + boxHeight);
            
            // Top of stack is orange, rest blue
            bool isTop = (i == stack.size() - 1);
            ImU32 fillCol = isTop ? IM_COL32(230, 126, 34, 255) : IM_COL32(52, 152, 219, 255);

            draw->AddRectFilled(boxMin, boxMax, fillCol, 4.0f);
            draw->AddRect(boxMin, boxMax, IM_COL32(255,255,255,200), 4.0f);
            
            draw->AddText(ImVec2(boxMin.x + (thisBoxW - textSize.x)*0.5f, boxMin.y + (boxHeight - textSize.y)*0.5f), 
                          IM_COL32(255,255,255,255), stack[i].c_str());
            
            startX += thisBoxW + spacing;
        }
        
        // End bracket
        draw->AddText(ImVec2(startX, y + 4), IM_COL32(150,150,150,255), "] <- TOP");
        
        // Reserve space
        ImGui::Dummy(ImVec2(startX + 60, boxHeight + 12));
    }

    // Visualize the DFA produced by Subset Construction
    inline void DrawSubsetDFA(const SubsetConstructionDFA& dfa) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 origin = ImGui::GetCursorScreenPos();
        
        if (dfa.states.empty()) {
            draw->AddText(origin, IM_COL32(150,150,150,255), "No DFA built yet. Enter regex, build NFA, then convert.");
            return;
        }

        int numStates = (int)dfa.states.size();
        
        // Use radial layout
        ImVec2 center(origin.x + 300, origin.y + 200);
        float radius = 120.0f;
        if (numStates > 6) radius = 160.0f;
        if (numStates > 10) radius = 200.0f;
        
        std::vector<ImVec2> positions(numStates);
        for (int i = 0; i < numStates; ++i) {
            float angle = (2.0f * 3.14159f * i) / numStates - 3.14159f / 2.0f;
            positions[i] = ImVec2(center.x + radius * cosf(angle), center.y + radius * sinf(angle));
        }

        ImU32 edgeCol = IM_COL32(52, 152, 219, 255);  // Blue

        // Draw transitions
        for (const auto& trans : dfa.transitions) {
            ImVec2 p1 = positions[trans.fromState];
            ImVec2 p2 = positions[trans.toState];
            
            std::string label(1, trans.symbol);
            
            if (trans.fromState == trans.toState) {
                // Self loop
                ImVec2 loopCenter(p1.x, p1.y - 35);
                draw->AddCircle(loopCenter, 12, edgeCol, 0, 2.0f);
                draw->AddText(ImVec2(loopCenter.x - 4, loopCenter.y - 22), edgeCol, label.c_str());
            } else {
                float dx = p2.x - p1.x;
                float dy = p2.y - p1.y;
                float dist = sqrtf(dx*dx + dy*dy);
                
                // Slight curve
                float perpX = -dy / dist * 12.0f;
                float perpY = dx / dist * 12.0f;
                ImVec2 ctrl((p1.x + p2.x) * 0.5f + perpX, (p1.y + p2.y) * 0.5f + perpY);
                
                float margin = 28.0f;
                float startRatio = margin / dist;
                float endRatio = (dist - margin) / dist;
                ImVec2 start(p1.x + dx * startRatio, p1.y + dy * startRatio);
                ImVec2 end(p1.x + dx * endRatio, p1.y + dy * endRatio);
                
                draw->AddBezierQuadratic(start, ctrl, end, edgeCol, 2.0f);
                
                // Arrow head
                float angle = atan2f(end.y - ctrl.y, end.x - ctrl.x);
                float as = 10.0f;
                draw->AddTriangleFilled(
                    end,
                    ImVec2(end.x - as * cosf(angle - 0.5f), end.y - as * sinf(angle - 0.5f)),
                    ImVec2(end.x - as * cosf(angle + 0.5f), end.y - as * sinf(angle + 0.5f)),
                    edgeCol
                );
                
                // Label
                draw->AddText(ImVec2(ctrl.x - 4, ctrl.y - 10), edgeCol, label.c_str());
            }
        }

        // Draw nodes
        for (const auto& state : dfa.states) {
            ImVec2 pos = positions[state.dfaId];
            
            bool isStart = (state.dfaId == dfa.startStateId);
            bool isAccept = state.isAccept;
            
            ImU32 fillCol = IM_COL32(52, 73, 94, 255);
            if (isStart && isAccept) fillCol = IM_COL32(142, 68, 173, 255); // Purple
            else if (isStart) fillCol = IM_COL32(52, 152, 219, 255); // Blue
            else if (isAccept) fillCol = IM_COL32(46, 204, 113, 255); // Green

            float nodeRadius = 26.0f;
            draw->AddCircleFilled(pos, nodeRadius, fillCol);
            draw->AddCircle(pos, nodeRadius, IM_COL32(236, 240, 241, 255), 0, 2.5f);
            if (isAccept) draw->AddCircle(pos, nodeRadius - 4.0f, IM_COL32(236, 240, 241, 255), 0, 2.0f);
            
            // Start arrow
            if (isStart) {
                ImVec2 arrowStart(pos.x - 50, pos.y);
                ImVec2 arrowEnd(pos.x - nodeRadius - 2, pos.y);
                draw->AddLine(arrowStart, arrowEnd, IM_COL32(236, 240, 241, 255), 2.5f);
                draw->AddTriangleFilled(
                    arrowEnd,
                    ImVec2(arrowEnd.x - 8, arrowEnd.y - 4),
                    ImVec2(arrowEnd.x - 8, arrowEnd.y + 4),
                    IM_COL32(236, 240, 241, 255)
                );
            }
            
            // State label (show set of NFA states)
            std::string label = state.label();
            // Truncate if too long
            if (label.length() > 12) label = "D" + std::to_string(state.dfaId);
            ImVec2 ts = ImGui::CalcTextSize(label.c_str());
            draw->AddText(ImVec2(pos.x - ts.x*0.5f, pos.y - ts.y*0.5f), IM_COL32(255,255,255,255), label.c_str());
        }
        
        // Title
        draw->AddText(ImVec2(origin.x + 10, origin.y + 10), IM_COL32(241, 196, 15, 255), "DFA (from Subset Construction)");
        
        // Legend
        ImVec2 lp(origin.x + 10, origin.y + 400);
        draw->AddCircleFilled(ImVec2(lp.x + 10, lp.y), 8, IM_COL32(52, 152, 219, 255));
        draw->AddText(ImVec2(lp.x + 25, lp.y - 7), IM_COL32(200,200,200,255), "Start");
        draw->AddCircleFilled(ImVec2(lp.x + 80, lp.y), 8, IM_COL32(46, 204, 113, 255));
        draw->AddCircle(ImVec2(lp.x + 80, lp.y), 8, IM_COL32(255,255,255,255));
        draw->AddCircle(ImVec2(lp.x + 80, lp.y), 5, IM_COL32(255,255,255,255));
        draw->AddText(ImVec2(lp.x + 95, lp.y - 7), IM_COL32(200,200,200,255), "Accept");
        draw->AddText(ImVec2(lp.x + 160, lp.y - 7), IM_COL32(200,200,200,255), "Labels = NFA state sets");
    }


    // Draw Complete Lexer NFA (Thompson's Construction) with new FullNFA structure
    inline void DrawLexerNFA(const FullNFA& nfa) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 origin = ImGui::GetCursorScreenPos();
        
        if (nfa.states.empty()) {
            draw->AddText(origin, IM_COL32(150,150,150,255), "Click 'BUILD LEXER NFA' to generate.");
            return;
        }

        int numStates = (int)nfa.states.size();
        
        // BFS layout from start state
        std::map<int, int> depth;
        std::map<int, int> row;
        std::map<int, int> rowCount;
        
        if (nfa.start >= 0) {
            std::queue<int> q;
            q.push(nfa.start);
            depth[nfa.start] = 0;
            row[nfa.start] = 0;
            rowCount[0] = 1;
            
            while (!q.empty()) {
                int curr = q.front(); q.pop();
                int currDepth = depth[curr];
                
                for (const auto& t : nfa.states[curr].trans) {
                    if (depth.find(t.to) == depth.end()) {
                        depth[t.to] = currDepth + 1;
                        if (rowCount.find(currDepth + 1) == rowCount.end()) 
                            rowCount[currDepth + 1] = 0;
                        row[t.to] = rowCount[currDepth + 1]++;
                        q.push(t.to);
                    }
                }
            }
        }
        
        // Assign unvisited states
        int maxDepth = 0;
        for (auto& [id, d] : depth) maxDepth = std::max(maxDepth, d);
        for (int i = 0; i < numStates; i++) {
            if (depth.find(i) == depth.end()) {
                depth[i] = maxDepth + 1;
                row[i] = 0;
            }
        }
        
        // Compute positions
        float spacingX = 55.0f;
        float spacingY = 32.0f;
        std::map<int, int> depthRowCount;
        for (auto& [id, r] : row) {
            int d = depth[id];
            depthRowCount[d] = std::max(depthRowCount[d], r + 1);
        }
        
        std::vector<ImVec2> positions(numStates);
        for (int i = 0; i < numStates; i++) {
            int d = depth[i];
            int r = row[i];
            int totalRows = depthRowCount[d];
            float centerY = origin.y + 200;
            float offsetY = (r - totalRows * 0.5f + 0.5f) * spacingY;
            positions[i] = ImVec2(origin.x + 45 + d * spacingX, centerY + offsetY);
        }

        ImU32 lineCol = IM_COL32(100, 100, 100, 255);
        ImU32 labelCol = IM_COL32(0, 100, 200, 255);

        // Draw edges
        for (int sId = 0; sId < numStates; sId++) {
            ImVec2 p1 = positions[sId];
            
            for (const auto& t : nfa.states[sId].trans) {
                int vId = t.to;
                if (vId >= numStates) continue;
                ImVec2 p2 = positions[vId];
                
                std::string label = labelKindStr(t.kind, t.ch);
                
                float dx = p2.x - p1.x;
                float dy = p2.y - p1.y;
                float dist = sqrtf(dx*dx + dy*dy);
                if (dist < 1.0f) dist = 1.0f;
                
                float margin = 11.0f;
                ImVec2 start(p1.x + dx/dist * margin, p1.y + dy/dist * margin);
                ImVec2 end(p2.x - dx/dist * margin, p2.y - dy/dist * margin);
                
                float perpX = -dy / dist * 5.0f;
                float perpY = dx / dist * 5.0f;
                ImVec2 ctrl((start.x + end.x)*0.5f + perpX, (start.y + end.y)*0.5f + perpY);
                
                draw->AddBezierQuadratic(start, ctrl, end, lineCol, 1.0f);
                
                float angle = atan2f(end.y - ctrl.y, end.x - ctrl.x);
                float as = 5.0f;
                draw->AddTriangleFilled(
                    end,
                    ImVec2(end.x - as * cosf(angle - 0.4f), end.y - as * sinf(angle - 0.4f)),
                    ImVec2(end.x - as * cosf(angle + 0.4f), end.y - as * sinf(angle + 0.4f)),
                    lineCol
                );
                
                ImVec2 labelPos(ctrl.x - 6, ctrl.y - 9);
                draw->AddText(labelPos, labelCol, label.c_str());
            }
        }

        // Draw nodes
        float nodeRadius = 10.0f;
        for (int i = 0; i < numStates; i++) {
            ImVec2 pos = positions[i];
            
            bool isAccept = nfa.acceptToken.count(i) > 0;
            bool isStart = (i == nfa.start);
            
            ImU32 fillCol = isAccept ? IM_COL32(144, 238, 144, 255) : IM_COL32(255, 255, 255, 255);
            ImU32 borderCol = IM_COL32(80, 80, 80, 255);
            
            draw->AddCircleFilled(pos, nodeRadius, fillCol);
            draw->AddCircle(pos, nodeRadius, borderCol, 0, 1.0f);
            
            if (isAccept) {
                draw->AddCircle(pos, nodeRadius - 2.5f, borderCol, 0, 0.8f);
            }
            
            if (isStart) {
                ImVec2 arrowStart(pos.x - 25, pos.y);
                ImVec2 arrowEnd(pos.x - nodeRadius - 1, pos.y);
                draw->AddLine(arrowStart, arrowEnd, IM_COL32(0, 0, 180, 255), 1.5f);
                draw->AddTriangleFilled(
                    arrowEnd,
                    ImVec2(arrowEnd.x - 5, arrowEnd.y - 3),
                    ImVec2(arrowEnd.x - 5, arrowEnd.y + 3),
                    IM_COL32(0, 0, 180, 255)
                );
            }
            
            std::string label = "q" + std::to_string(i);
            ImVec2 ts = ImGui::CalcTextSize(label.c_str());
            draw->AddText(ImVec2(pos.x - ts.x*0.5f, pos.y - ts.y*0.5f), IM_COL32(0,0,0,255), label.c_str());
            
            if (isAccept) {
                std::string tkLabel = tokenName(nfa.acceptToken.at(i));
                ImVec2 tokenPos(pos.x - 12, pos.y + nodeRadius + 1);
                draw->AddText(tokenPos, IM_COL32(0, 100, 0, 255), tkLabel.c_str());
            }
        }
        
        draw->AddText(ImVec2(origin.x + 5, origin.y + 5), IM_COL32(80,80,80,255), 
            "This diagram shows the COMPLETE NFA built using Thompson's Construction.");
        draw->AddText(ImVec2(origin.x + 5, origin.y + 18), IM_COL32(80,80,80,255), 
            "All token types (ID, NUMBER, PLUS, MINUS, STAR, SLASH, LPAREN, RPAREN, WHITESPACE)");
        draw->AddText(ImVec2(origin.x + 5, origin.y + 31), IM_COL32(80,80,80,255), 
            "are in a single automaton with consistent state numbering (q0, q1, q2, ...).");
    }

    // Draw Lexer DFA with token type labels
    inline void DrawLexerDFA(const std::vector<LexerDFAState>& dfa) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 origin = ImGui::GetCursorScreenPos();
        
        if (dfa.empty()) {
            draw->AddText(origin, IM_COL32(150,150,150,255), "Click 'CONVERT TO DFA' to generate.");
            return;
        }

        int numStates = (int)dfa.size();
        
        // Radial layout
        ImVec2 center(origin.x + 280, origin.y + 180);
        float radius = 100.0f + numStates * 8.0f;
        if (radius > 180.0f) radius = 180.0f;
        
        std::vector<ImVec2> positions(numStates);
        for (int i = 0; i < numStates; ++i) {
            float angle = (2.0f * 3.14159f * i) / numStates - 3.14159f / 2.0f;
            positions[i] = ImVec2(center.x + radius * cosf(angle), center.y + radius * sinf(angle));
        }

        ImU32 edgeCol = IM_COL32(80, 80, 80, 255);

        // Draw transitions (grouped by from->to, showing first char only for brevity)
        for (size_t i = 0; i < dfa.size(); i++) {
            const auto& state = dfa[i];
            std::map<int, std::string> transToLabels;
            for (const auto& [c, to] : state.trans) {
                if (transToLabels[to].empty()) {
                    transToLabels[to] = std::string(1, c);
                }
            }
            
            for (const auto& [to, label] : transToLabels) {
                ImVec2 p1 = positions[i];
                ImVec2 p2 = positions[to];
                
                float dx = p2.x - p1.x;
                float dy = p2.y - p1.y;
                float dist = sqrtf(dx*dx + dy*dy);
                if (dist < 1.0f) continue;
                
                float margin = 22.0f;
                ImVec2 start(p1.x + dx/dist * margin, p1.y + dy/dist * margin);
                ImVec2 end(p2.x - dx/dist * margin, p2.y - dy/dist * margin);
                
                float perpX = -dy / dist * 10.0f;
                float perpY = dx / dist * 10.0f;
                ImVec2 ctrl((start.x + end.x)*0.5f + perpX, (start.y + end.y)*0.5f + perpY);
                
                draw->AddBezierQuadratic(start, ctrl, end, edgeCol, 1.5f);
                
                float angle = atan2f(end.y - ctrl.y, end.x - ctrl.x);
                float as = 8.0f;
                draw->AddTriangleFilled(
                    end,
                    ImVec2(end.x - as * cosf(angle - 0.4f), end.y - as * sinf(angle - 0.4f)),
                    ImVec2(end.x - as * cosf(angle + 0.4f), end.y - as * sinf(angle + 0.4f)),
                    edgeCol
                );
            }
        }

        // Draw nodes
        for (size_t i = 0; i < dfa.size(); i++) {
            const auto& state = dfa[i];
            ImVec2 pos = positions[i];
            
            bool isStart = (i == 0);
            bool isAccept = state.accept;
            
            float nodeRadius = 20.0f;
            
            ImU32 fillCol = isAccept ? IM_COL32(144, 238, 144, 255) : IM_COL32(255, 255, 255, 255);
            ImU32 borderCol = IM_COL32(60, 60, 60, 255);
            
            draw->AddCircleFilled(pos, nodeRadius, fillCol);
            draw->AddCircle(pos, nodeRadius, borderCol, 0, 1.5f);
            if (isAccept) draw->AddCircle(pos, nodeRadius - 4.0f, borderCol, 0, 1.0f);
            
            if (isStart) {
                ImVec2 arrowStart(pos.x - 40, pos.y);
                ImVec2 arrowEnd(pos.x - nodeRadius - 2, pos.y);
                draw->AddLine(arrowStart, arrowEnd, IM_COL32(0, 0, 180, 255), 2.0f);
                draw->AddTriangleFilled(
                    arrowEnd,
                    ImVec2(arrowEnd.x - 6, arrowEnd.y - 4),
                    ImVec2(arrowEnd.x - 6, arrowEnd.y + 4),
                    IM_COL32(0, 0, 180, 255)
                );
            }
            
            std::string label = state.label();
            ImVec2 ts = ImGui::CalcTextSize(label.c_str());
            draw->AddText(ImVec2(pos.x - ts.x*0.5f, pos.y - ts.y*0.5f), IM_COL32(0,0,0,255), label.c_str());
            
            if (isAccept) {
                std::string tkLabel = state.tokenLabel();
                ImVec2 tokenPos(pos.x - 20, pos.y + nodeRadius + 3);
                draw->AddText(tokenPos, IM_COL32(0, 128, 0, 255), tkLabel.c_str());
            }
        }
        
        draw->AddText(ImVec2(origin.x + 5, origin.y + 5), IM_COL32(100,100,100,255), "Lexer DFA (Subset Construction)");
    }

    // Animated DFA with token-type states matching NFA patterns
    // Simplified educational view: START → ID/NUMBER/OPERATOR accept states
    inline void DrawLexerDFAAnimated(const std::vector<LexerDFAState>& dfa, int highlightState = -1, char currentChar = 0) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 origin = ImGui::GetCursorScreenPos();
        
        // Background panel
        ImVec2 panelTL(origin.x, origin.y);
        ImVec2 panelBR(origin.x + 580, origin.y + 300);
        draw->AddRectFilledMultiColor(panelTL, panelBR, 
            IM_COL32(22, 25, 35, 255), IM_COL32(28, 32, 45, 255),
            IM_COL32(20, 22, 30, 255), IM_COL32(30, 35, 50, 255));
        draw->AddRect(panelTL, panelBR, IM_COL32(60, 70, 100, 150), 8.0f, 0, 1.5f);
        
        // Title
        draw->AddText(ImVec2(origin.x + 15, origin.y + 10), IM_COL32(180, 190, 220, 255), 
            "DFA TOKENIZATION - Token Type States");
        
        // Define 5 simplified states matching NFA pattern:
        // 0: START (q0)
        // 1: ID Accept (q1 - letters → ID)
        // 2: NUMBER Accept (q2 - digits → NUMBER)
        // 3: OPERATOR Accept (q3 - +,-,*,/,(,))
        
        struct SimpleState {
            const char* name;
            const char* label;      // State number (q0, q1, etc.)
            const char* typeName;   // Token type name
            ImVec2 pos;
            ImU32 color;
            ImU32 borderColor;
            bool isAccept;
        };
        
        // Position states in a tree layout
        float centerX = origin.x + 290;
        float startY = origin.y + 70;
        float row2Y = origin.y + 180;
        float spacing = 130;
        
        SimpleState states[] = {
            {"START", "q0", "Start", ImVec2(centerX, startY), IM_COL32(200, 210, 240, 255), IM_COL32(80, 90, 140, 255), false},
            {"ID", "q1", "ID", ImVec2(centerX - spacing, row2Y), IM_COL32(100, 180, 255, 255), IM_COL32(50, 130, 200, 255), true},
            {"NUMBER", "q2", "NUM", ImVec2(centerX, row2Y), IM_COL32(100, 220, 130, 255), IM_COL32(50, 160, 80, 255), true},
            {"OPERATOR", "q3", "OP", ImVec2(centerX + spacing, row2Y), IM_COL32(255, 180, 100, 255), IM_COL32(200, 130, 50, 255), true}
        };
        
        float nodeRadius = 28.0f;
        
        // Define transitions with labels
        struct TransInfo {
            int from, to;
            const char* label;
        };
        TransInfo transitions[] = {
            {0, 1, "[a-zA-Z]"},
            {0, 2, "[0-9]"},
            {0, 3, "+−*/()"},
            {1, 1, "[alnum_]"},  // Self-loop
            {2, 2, "[0-9.]"}     // Self-loop
        };
        
        // Map highlight state to our simplified states
        int activeState = -1;
        if (highlightState == 1) activeState = 1;       // ID
        else if (highlightState == 2) activeState = 2;  // NUMBER
        else if (highlightState == 3) activeState = 3;  // OPERATOR
        else if (highlightState == 0) activeState = 0;  // START
        
        // Draw transitions
        for (const auto& t : transitions) {
            ImVec2 p1 = states[t.from].pos;
            ImVec2 p2 = states[t.to].pos;
            
            bool isActive = (t.from == 0 && t.to == activeState && activeState > 0);
            ImU32 edgeCol = isActive ? IM_COL32(80, 255, 120, 255) : IM_COL32(100, 110, 140, 180);
            float thick = isActive ? 3.0f : 1.5f;
            
            if (t.from == t.to) {
                // Self-loop - position based on state
                float loopRadius = 22.0f;
                ImVec2 loopCenter;
                float startAngle, endAngle;
                
                // Place loops on different sides for each state
                if (t.from == 1) {  // q1 (ID) - loop on left side
                    loopCenter = ImVec2(p1.x - nodeRadius - loopRadius + 8, p1.y);
                    startAngle = 3.14159f * 0.5f;   // Start at bottom
                    endAngle = 3.14159f * 2.5f;     // End at bottom (full loop)
                } else {  // q2 (NUMBER) - loop on right side
                    loopCenter = ImVec2(p1.x + nodeRadius + loopRadius - 8, p1.y);
                    startAngle = -3.14159f * 0.5f;  // Start at top
                    endAngle = 3.14159f * 1.5f;     // End at top (full loop)
                }
                
                // Draw arc
                draw->PathClear();
                for (int i = 0; i <= 20; i++) {
                    float angle = startAngle + (1.6f * 3.14159f) * i / 20.0f;
                    if (t.from == 1) angle = 3.14159f * 1.2f + (1.6f * 3.14159f) * i / 20.0f;
                    else angle = -3.14159f * 0.2f + (1.6f * 3.14159f) * i / 20.0f;
                    draw->PathLineTo(ImVec2(loopCenter.x + loopRadius * cosf(angle), 
                                            loopCenter.y + loopRadius * sinf(angle)));
                }
                
                bool selfActive = (activeState == t.from && activeState > 0);
                ImU32 selfCol = selfActive ? IM_COL32(80, 255, 120, 255) : IM_COL32(100, 120, 160, 200);
                
                // Glow when active
                if (selfActive) {
                    draw->PathStroke(IM_COL32(80, 255, 120, 60), 0, 6.0f);
                    draw->PathClear();
                    for (int i = 0; i <= 20; i++) {
                        float angle = (t.from == 1) ? 
                            (3.14159f * 1.2f + (1.6f * 3.14159f) * i / 20.0f) :
                            (-3.14159f * 0.2f + (1.6f * 3.14159f) * i / 20.0f);
                        draw->PathLineTo(ImVec2(loopCenter.x + loopRadius * cosf(angle), 
                                                loopCenter.y + loopRadius * sinf(angle)));
                    }
                }
                draw->PathStroke(selfCol, 0, selfActive ? 2.5f : 1.5f);
                
                // Arrow at end
                float arrowAngle = (t.from == 1) ? 3.14159f * 2.7f : 3.14159f * 1.3f;
                ImVec2 arrowEnd(loopCenter.x + loopRadius * cosf(arrowAngle), 
                               loopCenter.y + loopRadius * sinf(arrowAngle));
                float as = 6.0f;
                draw->AddTriangleFilled(
                    arrowEnd,
                    ImVec2(arrowEnd.x + as * cosf(arrowAngle + 0.7f), arrowEnd.y + as * sinf(arrowAngle + 0.7f)),
                    ImVec2(arrowEnd.x + as * cosf(arrowAngle - 0.5f), arrowEnd.y + as * sinf(arrowAngle - 0.5f)),
                    selfCol);
                
                // Label beside the loop
                ImVec2 lblSize = ImGui::CalcTextSize(t.label);
                float lblX = (t.from == 1) ? 
                    (loopCenter.x - loopRadius - lblSize.x - 5) :  // Left of loop for q1
                    (loopCenter.x + loopRadius + 5);               // Right of loop for q2
                draw->AddRectFilled(ImVec2(lblX - 3, loopCenter.y - lblSize.y/2 - 2),
                    ImVec2(lblX + lblSize.x + 3, loopCenter.y + lblSize.y/2 + 2),
                    IM_COL32(30, 35, 50, 220), 3.0f);
                draw->AddText(ImVec2(lblX, loopCenter.y - lblSize.y/2), 
                    selfActive ? IM_COL32(150, 255, 180, 255) : IM_COL32(150, 160, 200, 255), t.label);
            } else {
                // Regular edge
                float dx = p2.x - p1.x;
                float dy = p2.y - p1.y;
                float dist = sqrtf(dx*dx + dy*dy);
                if (dist < 1.0f) continue;
                
                ImVec2 start(p1.x + dx/dist * nodeRadius, p1.y + dy/dist * nodeRadius);
                ImVec2 end(p2.x - dx/dist * nodeRadius, p2.y - dy/dist * nodeRadius);
                
                // Glow for active
                if (isActive) {
                    draw->AddLine(start, end, IM_COL32(80, 255, 120, 60), 10.0f);
                }
                draw->AddLine(start, end, edgeCol, thick);
                
                // Arrowhead
                float angle = atan2f(dy, dx);
                float as = isActive ? 10.0f : 7.0f;
                draw->AddTriangleFilled(
                    end,
                    ImVec2(end.x - as * cosf(angle - 0.35f), end.y - as * sinf(angle - 0.35f)),
                    ImVec2(end.x - as * cosf(angle + 0.35f), end.y - as * sinf(angle + 0.35f)),
                    edgeCol);
                
                // Edge label
                ImVec2 mid((start.x + end.x) * 0.5f, (start.y + end.y) * 0.5f);
                float perpX = -dy / dist * 12.0f;
                float perpY = dx / dist * 12.0f;
                ImVec2 lblSize = ImGui::CalcTextSize(t.label);
                ImVec2 lblPos(mid.x + perpX - lblSize.x * 0.5f, mid.y + perpY - lblSize.y * 0.5f);
                
                // Background for label
                draw->AddRectFilled(ImVec2(lblPos.x - 3, lblPos.y - 1), 
                    ImVec2(lblPos.x + lblSize.x + 3, lblPos.y + lblSize.y + 1),
                    IM_COL32(30, 35, 50, 230), 3.0f);
                draw->AddText(lblPos, isActive ? IM_COL32(150, 255, 180, 255) : IM_COL32(140, 150, 180, 255), t.label);
            }
        }
        
        // Draw nodes
        for (int i = 0; i < 4; i++) {
            ImVec2 pos = states[i].pos;
            bool isHighlight = (i == activeState);
            
            // Shadow
            draw->AddCircleFilled(ImVec2(pos.x + 3, pos.y + 3), nodeRadius, IM_COL32(0, 0, 0, 50));
            
            // Glow for highlighted
            if (isHighlight) {
                draw->AddCircleFilled(pos, nodeRadius + 10, IM_COL32(255, 220, 80, 50));
                draw->AddCircleFilled(pos, nodeRadius + 5, IM_COL32(255, 220, 80, 80));
            }
            
            // Fill
            ImU32 fillCol = isHighlight ? IM_COL32(255, 220, 80, 255) : states[i].color;
            draw->AddCircleFilled(pos, nodeRadius, fillCol);
            
            // Border
            ImU32 borderCol = isHighlight ? IM_COL32(255, 150, 0, 255) : states[i].borderColor;
            draw->AddCircle(pos, nodeRadius, borderCol, 0, isHighlight ? 3.5f : 2.0f);
            
            // Double circle for accept states
            if (states[i].isAccept) {
                draw->AddCircle(pos, nodeRadius - 5.0f, borderCol, 0, 1.5f);
            }
            
            // Start arrow
            if (i == 0) {
                ImVec2 arrowStart(pos.x - 60, pos.y);
                ImVec2 arrowEnd(pos.x - nodeRadius - 3, pos.y);
                draw->AddLine(arrowStart, arrowEnd, IM_COL32(80, 120, 255, 100), 6.0f);
                draw->AddLine(arrowStart, arrowEnd, IM_COL32(80, 120, 255, 255), 2.5f);
                draw->AddTriangleFilled(arrowEnd,
                    ImVec2(arrowEnd.x - 8, arrowEnd.y - 5),
                    ImVec2(arrowEnd.x - 8, arrowEnd.y + 5),
                    IM_COL32(80, 120, 255, 255));
                draw->AddText(ImVec2(arrowStart.x - 45, arrowStart.y - 8), 
                    IM_COL32(80, 120, 255, 255), "START");
            }
            
            // State label inside (q0, q1, etc.)
            ImVec2 ts = ImGui::CalcTextSize(states[i].label);
            ImU32 textCol = isHighlight ? IM_COL32(60, 40, 0, 255) : IM_COL32(30, 35, 50, 255);
            draw->AddText(ImVec2(pos.x - ts.x * 0.5f, pos.y - ts.y * 0.5f), textCol, states[i].label);
            
            // Token type below states
            const char* tokenType = states[i].typeName;
            ImVec2 typeSize = ImGui::CalcTextSize(tokenType);
            ImVec2 typePos(pos.x - typeSize.x * 0.5f, pos.y + nodeRadius + 8);
            draw->AddRectFilled(ImVec2(typePos.x - 4, typePos.y - 2),
                ImVec2(typePos.x + typeSize.x + 4, typePos.y + typeSize.y + 2),
                states[i].isAccept ? states[i].borderColor : IM_COL32(60, 70, 100, 255), 4.0f);
            draw->AddText(typePos, IM_COL32(255, 255, 255, 255), tokenType);
        }
        
        // Legend
        float lx = origin.x + 450;
        float ly = origin.y + 50;
        
        draw->AddText(ImVec2(lx, ly), IM_COL32(160, 170, 200, 255), "Token Types:");
        ly += 20;
        
        // ID
        draw->AddCircleFilled(ImVec2(lx + 8, ly + 7), 6, IM_COL32(100, 180, 255, 255));
        draw->AddText(ImVec2(lx + 20, ly), IM_COL32(180, 200, 255, 255), "ID (abc, x1)");
        ly += 18;
        
        // Number  
        draw->AddCircleFilled(ImVec2(lx + 8, ly + 7), 6, IM_COL32(100, 220, 130, 255));
        draw->AddText(ImVec2(lx + 20, ly), IM_COL32(180, 255, 200, 255), "NUM (123, 3.14)");
        ly += 18;
        
        // Operator
        draw->AddCircleFilled(ImVec2(lx + 8, ly + 7), 6, IM_COL32(255, 180, 100, 255));
        draw->AddText(ImVec2(lx + 20, ly), IM_COL32(255, 220, 180, 255), "OP (+,-,*,/)");
        
        // Reserve space
        ImGui::Dummy(ImVec2(580, 305));
    }

    // Animated PDA Grammar Diagram - Shows derivation path through grammar
    inline void DrawPDAAnimated(const std::string& currentAction, const std::string& stackTop) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 origin = ImGui::GetCursorScreenPos();
        
        // Panel dimensions
        float panelW = 560.0f, panelH = 200.0f;
        ImVec2 panelTL(origin.x, origin.y);
        ImVec2 panelBR(origin.x + panelW, origin.y + panelH);
        
        // Background gradient
        draw->AddRectFilledMultiColor(panelTL, panelBR, 
            IM_COL32(25, 28, 40, 255), IM_COL32(30, 35, 50, 255),
            IM_COL32(20, 23, 35, 255), IM_COL32(35, 40, 55, 255));
        draw->AddRect(panelTL, panelBR, IM_COL32(70, 90, 130, 200), 4.0f, 0, 1.5f);
        
        // Title
        draw->AddText(ImVec2(origin.x + 12, origin.y + 5), IM_COL32(200, 210, 240, 255), 
            "PDA GRAMMAR DERIVATION");
        
        // Determine active state
        std::string activeId = "start";
        if (currentAction.find("Accept") != std::string::npos) activeId = "accept";
        else if (currentAction.find("Error") != std::string::npos) activeId = "error";
        else if (currentAction.find("E ->") != std::string::npos || 
                 currentAction.find("Expand E ->") != std::string::npos) activeId = "E";
        else if (currentAction.find("E' ->") != std::string::npos ||
                 currentAction.find("Expand E'") != std::string::npos) activeId = "Ep";
        else if (currentAction.find("T ->") != std::string::npos ||
                 currentAction.find("Expand T ->") != std::string::npos) activeId = "T";
        else if (currentAction.find("T' ->") != std::string::npos ||
                 currentAction.find("Expand T'") != std::string::npos) activeId = "Tp";
        else if (currentAction.find("F ->") != std::string::npos ||
                 currentAction.find("Expand F") != std::string::npos) activeId = "F";
        else if (currentAction.find("Match number") != std::string::npos) activeId = "num";
        else if (currentAction.find("Match") != std::string::npos) activeId = "op";
        else if (stackTop == "E") activeId = "E";
        else if (stackTop == "E'") activeId = "Ep";
        else if (stackTop == "T") activeId = "T";
        else if (stackTop == "T'") activeId = "Tp";
        else if (stackTop == "F") activeId = "F";
        
        // New cleaner layout: Horizontal flow with continuations below
        float nodeR = 18.0f;
        float hSpace = 80.0f;  // Horizontal spacing
        
        // Main row (top): Start -> E -> T -> F -> NUM -> Accept
        float mainY = origin.y + 65;
        float x0 = origin.x + 45;  // Start
        float x1 = x0 + hSpace;    // E
        float x2 = x1 + hSpace;    // T
        float x3 = x2 + hSpace;    // F
        float x4 = x3 + hSpace;    // NUM
        float x5 = x4 + hSpace;    // Accept
        
        // Bottom row: E', T', ID, OP
        float bottomY = origin.y + 150;
        
        struct GState { const char* id; const char* lbl; ImVec2 p; ImU32 c; bool acc; };
        GState S[] = {
            {"start", "Start", {x0, mainY}, IM_COL32(130, 160, 200, 255), false},
            {"E",     "E",     {x1, mainY}, IM_COL32(100, 150, 255, 255), false},
            {"T",     "T",     {x2, mainY}, IM_COL32(160, 120, 220, 255), false},
            {"F",     "F",     {x3, mainY}, IM_COL32(255, 180, 100, 255), false},
            {"num",   "NUM",   {x4, mainY}, IM_COL32(100, 220, 120, 255), false},
            {"accept","OK",    {x5, mainY}, IM_COL32(80, 200, 100, 255), true},
            {"Ep",    "E'",    {x1, bottomY}, IM_COL32(80, 120, 200, 255), false},
            {"Tp",    "T'",    {x2, bottomY}, IM_COL32(130, 100, 180, 255), false},
            {"id",    "ID",    {x3, bottomY}, IM_COL32(100, 200, 140, 255), false},
            {"op",    "OP",    {x4, bottomY}, IM_COL32(120, 200, 180, 255), false}
        };
        int N = 10;
        
        // Arrow helper with curved option
        auto arrow = [&](int fi, int ti, const char* lbl, bool curved = false) {
            ImVec2 f = S[fi].p, t = S[ti].p;
            float dx = t.x - f.x, dy = t.y - f.y, d = sqrtf(dx*dx + dy*dy);
            if (d < 1) return;
            
            ImVec2 s(f.x + dx/d * nodeR, f.y + dy/d * nodeR);
            ImVec2 e(t.x - dx/d * nodeR, t.y - dy/d * nodeR);
            bool act = (activeId == S[fi].id);
            ImU32 col = act ? IM_COL32(80, 255, 130, 255) : IM_COL32(70, 80, 100, 160);
            float thick = act ? 2.0f : 1.0f;
            
            if (act) draw->AddLine(s, e, IM_COL32(80, 255, 130, 40), 5.0f);
            draw->AddLine(s, e, col, thick);
            
            // Arrowhead
            float ang = atan2f(dy, dx);
            draw->AddTriangleFilled(e, 
                ImVec2(e.x - 5*cosf(ang-0.4f), e.y - 5*sinf(ang-0.4f)),
                ImVec2(e.x - 5*cosf(ang+0.4f), e.y - 5*sinf(ang+0.4f)), col);
            
            // Label
            if (strlen(lbl) > 0) {
                ImVec2 mid((s.x+e.x)*0.5f, (s.y+e.y)*0.5f);
                if (dy > 10) mid.x -= 12; else mid.y -= 10;
                draw->AddText(mid, act ? IM_COL32(160,255,180,255) : IM_COL32(100,110,130,200), lbl);
            }
        };
        
        // Grammar transitions:
        // Main flow: Start -> E -> T -> F -> NUM -> Accept
        arrow(0, 1, "");       // Start -> E
        arrow(1, 2, "T");      // E -> T
        arrow(2, 3, "F");      // T -> F
        arrow(3, 4, "num");    // F -> NUM
        arrow(4, 5, "");       // NUM -> Accept
        
        // Continuations
        arrow(1, 6, "E'");     // E -> E'
        arrow(2, 7, "T'");     // T -> T'
        
        // E' -> T (loop back)
        arrow(6, 2, "T");      // E' -> T
        
        // T' -> F (loop back)  
        arrow(7, 3, "F");      // T' -> F
        
        // F alternatives
        arrow(3, 8, "id");     // F -> ID
        arrow(3, 9, "()");     // F -> OP (parens)
        
        // Terminals to Accept
        arrow(8, 5, "");       // ID -> Accept
        arrow(9, 5, "");       // OP -> Accept
        
        // Draw states
        for (int i = 0; i < N; i++) {
            bool act = (activeId == S[i].id);
            ImVec2 p = S[i].p;
            
            // Shadow
            draw->AddCircleFilled(ImVec2(p.x+1, p.y+1), nodeR, IM_COL32(0,0,0,30));
            
            // Glow for active
            if (act) {
                draw->AddCircleFilled(p, nodeR + 6, IM_COL32(255, 230, 80, 50));
                draw->AddCircleFilled(p, nodeR + 3, IM_COL32(255, 230, 80, 90));
            }
            
            // Fill & border
            draw->AddCircleFilled(p, nodeR, act ? IM_COL32(255, 220, 80, 255) : S[i].c);
            draw->AddCircle(p, nodeR, act ? IM_COL32(255, 200, 50, 255) : IM_COL32(50, 60, 90, 200), 0, act ? 2.0f : 1.2f);
            
            // Double circle for accept
            if (S[i].acc) draw->AddCircle(p, nodeR - 4, IM_COL32(50, 60, 90, 200), 0, 1.2f);
            
            // Label
            ImVec2 ts = ImGui::CalcTextSize(S[i].lbl);
            draw->AddText(ImVec2(p.x - ts.x*0.5f, p.y - ts.y*0.5f), 
                act ? IM_COL32(30, 20, 0, 255) : IM_COL32(255, 255, 255, 255), S[i].lbl);
        }
        
        // Start arrow
        float arrStart = x0 - nodeR - 25;
        draw->AddLine(ImVec2(arrStart, mainY), ImVec2(x0 - nodeR - 2, mainY), IM_COL32(100, 150, 220, 255), 2.0f);
        draw->AddTriangleFilled(ImVec2(x0 - nodeR - 2, mainY),
            ImVec2(x0 - nodeR - 8, mainY - 4), ImVec2(x0 - nodeR - 8, mainY + 4), IM_COL32(100, 150, 220, 255));
        
        ImGui::Dummy(ImVec2(panelW, panelH + 5));
    }
}

