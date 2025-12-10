#pragma once
#include "NFA.h"
#include <vector>
#include <set>
#include <map>
#include <string>
#include <queue>
#include <algorithm>

// A DFA state is a set of NFA state IDs
struct DFAStateSet {
    std::set<int> nfaStates;
    int dfaId;
    bool isAccept = false;
    
    std::string label() const {
        if (nfaStates.empty()) return "∅";
        std::string s = "{";
        bool first = true;
        for (int id : nfaStates) {
            if (!first) s += ",";
            s += "q" + std::to_string(id);
            first = false;
        }
        s += "}";
        return s;
    }
};

struct SubsetDFATransition {
    int fromState;
    char symbol;
    int toState;
};

// Trace step for educational display
struct SubsetStep {
    std::string action;
    std::string detail;
    std::set<int> currentSet;
    std::set<int> resultSet;
};

class SubsetConstructionDFA {
public:
    std::vector<DFAStateSet> states;
    std::vector<SubsetDFATransition> transitions;
    std::vector<SubsetStep> trace;
    int startStateId = -1;
    std::set<char> alphabet;
    
    // Perform subset construction on the given NFA
    void build(const ThompsonNFA& nfa) {
        states.clear();
        transitions.clear();
        trace.clear();
        alphabet.clear();
        
        if (!nfa.start) return;
        
        // Collect alphabet from NFA
        for (const auto& state : nfa.owned) {
            for (const auto& [sym, _] : state->trans) {
                if (sym != 0) alphabet.insert(sym);
            }
        }
        
        trace.push_back({"Init", "Collecting alphabet: " + alphabetStr(), {}, {}});
        
        // Compute epsilon closure of start state
        std::set<int> startClosure = epsilonClosure(nfa, {nfa.start->id});
        
        trace.push_back({"ε-closure", "Start state closure", {nfa.start->id}, startClosure});
        
        // Create initial DFA state
        DFAStateSet startDFA;
        startDFA.nfaStates = startClosure;
        startDFA.dfaId = 0;
        startDFA.isAccept = containsAccept(nfa, startClosure);
        states.push_back(startDFA);
        startStateId = 0;
        
        // Map from set of NFA states to DFA state ID
        std::map<std::set<int>, int> stateMap;
        stateMap[startClosure] = 0;
        
        // BFS to explore all reachable DFA states
        std::queue<int> worklist;
        worklist.push(0);
        
        while (!worklist.empty()) {
            int currentDFAId = worklist.front();
            worklist.pop();
            
            const std::set<int>& currentNFASet = states[currentDFAId].nfaStates;
            
            trace.push_back({"Process", "Processing DFA state " + std::to_string(currentDFAId) + " = " + states[currentDFAId].label(), currentNFASet, {}});
            
            for (char sym : alphabet) {
                // Compute move(currentNFASet, sym)
                std::set<int> moveResult = move(nfa, currentNFASet, sym);
                
                if (moveResult.empty()) continue;
                
                // Compute epsilon closure of move result
                std::set<int> nextClosure = epsilonClosure(nfa, moveResult);
                
                trace.push_back({"δ(" + states[currentDFAId].label() + ", " + std::string(1,sym) + ")", 
                                 "Move then ε-closure", currentNFASet, nextClosure});
                
                // Check if this DFA state already exists
                if (stateMap.find(nextClosure) == stateMap.end()) {
                    // Create new DFA state
                    DFAStateSet newState;
                    newState.nfaStates = nextClosure;
                    newState.dfaId = (int)states.size();
                    newState.isAccept = containsAccept(nfa, nextClosure);
                    states.push_back(newState);
                    stateMap[nextClosure] = newState.dfaId;
                    worklist.push(newState.dfaId);
                    
                    trace.push_back({"New State", "Created DFA state " + std::to_string(newState.dfaId), {}, nextClosure});
                }
                
                // Add transition
                transitions.push_back({currentDFAId, sym, stateMap[nextClosure]});
            }
        }
        
        trace.push_back({"Done", "Subset construction complete. " + std::to_string(states.size()) + " DFA states.", {}, {}});
    }
    
    // Simulate the DFA on input string
    bool simulate(const std::string& input) const {
        if (startStateId < 0) return false;
        int current = startStateId;
        for (char c : input) {
            bool found = false;
            for (const auto& t : transitions) {
                if (t.fromState == current && t.symbol == c) {
                    current = t.toState;
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return states[current].isAccept;
    }
    
private:
    std::string alphabetStr() const {
        std::string s = "{";
        bool first = true;
        for (char c : alphabet) {
            if (!first) s += ", ";
            s += c;
            first = false;
        }
        s += "}";
        return s;
    }
    
    // Compute epsilon closure of a set of NFA state IDs
    std::set<int> epsilonClosure(const ThompsonNFA& nfa, const std::set<int>& stateIds) const {
        std::set<int> closure = stateIds;
        std::queue<int> worklist;
        for (int id : stateIds) worklist.push(id);
        
        while (!worklist.empty()) {
            int current = worklist.front();
            worklist.pop();
            
            // Find the NState with this ID
            for (const auto& state : nfa.owned) {
                if (state->id == current) {
                    auto it = state->trans.find(0); // epsilon
                    if (it != state->trans.end()) {
                        for (auto* next : it->second) {
                            if (closure.find(next->id) == closure.end()) {
                                closure.insert(next->id);
                                worklist.push(next->id);
                            }
                        }
                    }
                    break;
                }
            }
        }
        return closure;
    }
    
    // Compute move(stateSet, symbol)
    std::set<int> move(const ThompsonNFA& nfa, const std::set<int>& stateIds, char symbol) const {
        std::set<int> result;
        for (int id : stateIds) {
            for (const auto& state : nfa.owned) {
                if (state->id == id) {
                    auto it = state->trans.find(symbol);
                    if (it != state->trans.end()) {
                        for (auto* next : it->second) {
                            result.insert(next->id);
                        }
                    }
                    break;
                }
            }
        }
        return result;
    }
    
    // Check if any NFA state in the set is an accept state
    bool containsAccept(const ThompsonNFA& nfa, const std::set<int>& stateIds) const {
        for (int id : stateIds) {
            for (const auto& state : nfa.owned) {
                if (state->id == id && state->accept) return true;
            }
        }
        return false;
    }
};
