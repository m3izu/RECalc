#pragma once
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <string>
#include <unordered_map>
#include <cctype>

// Token IDs matching reference
enum TokenID {
    TK_ID = 1,
    TK_NUMBER,
    TK_PLUS,
    TK_MINUS,
    TK_STAR,
    TK_SLASH,
    TK_LPAREN,
    TK_RPAREN,
    TK_WS
};

inline std::string tokenName(int id) {
    switch(id) {
        case TK_ID: return "[ID]";
        case TK_NUMBER: return "[NUMBER]";
        case TK_PLUS: return "[PLUS]";
        case TK_MINUS: return "[MINUS]";
        case TK_STAR: return "[STAR]";
        case TK_SLASH: return "[SLASH]";
        case TK_LPAREN: return "[LPAREN]";
        case TK_RPAREN: return "[RPAREN]";
        case TK_WS: return "[WS]";
        default: return "[?]";
    }
}

// Label kinds for NFA transitions
enum LabelKind {
    L_EPS,              // Epsilon transition
    L_CHAR,             // Specific character
    L_DIGIT,            // [0-9]
    L_LETTER,           // [a-zA-Z]
    L_ALNUM_UNDERSCORE  // [a-zA-Z0-9_]
};

inline std::string labelKindStr(LabelKind kind, char ch) {
    switch(kind) {
        case L_EPS: return "ε";
        case L_CHAR: 
            if (ch == ' ') return "sp";
            if (ch == '\t') return "\\t";
            if (ch == '.') return ".";
            return std::string(1, ch);
        case L_DIGIT: return "[0-9]";
        case L_LETTER: return "[a-zA-Z]";
        case L_ALNUM_UNDERSCORE: return "[alnum_]";
        default: return "?";
    }
}

inline bool labelMatches(LabelKind kind, char c, char expectedChar = 0) {
    switch(kind) {
        case L_CHAR: return c == expectedChar;
        case L_DIGIT: return std::isdigit(static_cast<unsigned char>(c));
        case L_LETTER: return std::isalpha(static_cast<unsigned char>(c));
        case L_ALNUM_UNDERSCORE: return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
        default: return false;
    }
}

// NFA Transition
struct LexerNFATrans {
    int to;
    LabelKind kind;
    char ch;
    LexerNFATrans(int t = 0, LabelKind k = L_EPS, char c = 0) : to(t), kind(k), ch(c) {}
};

// NFA State
struct LexerNFAState {
    int id;
    std::vector<LexerNFATrans> trans;
    LexerNFAState(int i = 0) : id(i) {}
};

// NFA Fragment
struct LexerFrag {
    int start, accept;
    LexerFrag(int s = 0, int a = 0) : start(s), accept(a) {}
};

// Complete NFA
struct FullNFA {
    std::vector<LexerNFAState> states;
    int start = -1;
    std::unordered_map<int, int> acceptToken;
    
    int newState() {
        int id = states.size();
        states.emplace_back(id);
        return id;
    }
};

// Thompson's Construction
inline LexerFrag makeAtomic(FullNFA& nfa, LabelKind kind, char ch = 0) {
    int s = nfa.newState();
    int a = nfa.newState();
    nfa.states[s].trans.emplace_back(a, kind, ch);
    return {s, a};
}

inline LexerFrag concatFrag(FullNFA& nfa, const LexerFrag& a, const LexerFrag& b) {
    nfa.states[a.accept].trans.emplace_back(b.start, L_EPS, 0);
    return {a.start, b.accept};
}

inline LexerFrag unionFrag(FullNFA& nfa, const LexerFrag& a, const LexerFrag& b) {
    int s = nfa.newState();
    int x = nfa.newState();
    nfa.states[s].trans.emplace_back(a.start, L_EPS, 0);
    nfa.states[s].trans.emplace_back(b.start, L_EPS, 0);
    nfa.states[a.accept].trans.emplace_back(x, L_EPS, 0);
    nfa.states[b.accept].trans.emplace_back(x, L_EPS, 0);
    return {s, x};
}

inline LexerFrag starFrag(FullNFA& nfa, const LexerFrag& f) {
    int s = nfa.newState();
    int a = nfa.newState();
    nfa.states[s].trans.emplace_back(f.start, L_EPS, 0);
    nfa.states[s].trans.emplace_back(a, L_EPS, 0);
    nfa.states[f.accept].trans.emplace_back(f.start, L_EPS, 0);
    nfa.states[f.accept].trans.emplace_back(a, L_EPS, 0);
    return {s, a};
}

inline LexerFrag plusFrag(FullNFA& nfa, const LexerFrag& f) {
    int s = nfa.newState();
    int a = nfa.newState();
    nfa.states[s].trans.emplace_back(f.start, L_EPS, 0);
    nfa.states[f.accept].trans.emplace_back(f.start, L_EPS, 0);
    nfa.states[f.accept].trans.emplace_back(a, L_EPS, 0);
    return {s, a};
}

inline LexerFrag optFrag(FullNFA& nfa, const LexerFrag& f) {
    int s = nfa.newState();
    int a = nfa.newState();
    nfa.states[s].trans.emplace_back(f.start, L_EPS, 0);
    nfa.states[s].trans.emplace_back(a, L_EPS, 0);
    nfa.states[f.accept].trans.emplace_back(a, L_EPS, 0);
    return {s, a};
}

// Build combined NFA
inline FullNFA buildCombinedNFA() {
    FullNFA nfa;
    nfa.start = nfa.newState();
    
    auto insert = [&](LexerFrag f, int tk) {
        nfa.states[nfa.start].trans.emplace_back(f.start, L_EPS, 0);
        nfa.acceptToken[f.accept] = tk;
    };
    
    // ID: letter (alnum|_)*
    auto letter = makeAtomic(nfa, L_LETTER);
    auto alnum = makeAtomic(nfa, L_ALNUM_UNDERSCORE);
    insert(concatFrag(nfa, letter, starFrag(nfa, alnum)), TK_ID);
    
    // NUMBER: digit+ (. digit+)?
    auto digit = makeAtomic(nfa, L_DIGIT);
    auto digitPlus = concatFrag(nfa, digit, starFrag(nfa, makeAtomic(nfa, L_DIGIT)));
    auto dot = makeAtomic(nfa, L_CHAR, '.');
    auto fractional = concatFrag(nfa, dot, 
                        concatFrag(nfa, makeAtomic(nfa, L_DIGIT), 
                                  starFrag(nfa, makeAtomic(nfa, L_DIGIT))));
    insert(concatFrag(nfa, digitPlus, optFrag(nfa, fractional)), TK_NUMBER);
    
    // Operators
    insert(makeAtomic(nfa, L_CHAR, '+'), TK_PLUS);
    insert(makeAtomic(nfa, L_CHAR, '-'), TK_MINUS);
    insert(makeAtomic(nfa, L_CHAR, '*'), TK_STAR);
    insert(makeAtomic(nfa, L_CHAR, '/'), TK_SLASH);
    insert(makeAtomic(nfa, L_CHAR, '('), TK_LPAREN);
    insert(makeAtomic(nfa, L_CHAR, ')'), TK_RPAREN);
    
    // Whitespace
    auto space = makeAtomic(nfa, L_CHAR, ' ');
    auto tab = makeAtomic(nfa, L_CHAR, '\t');
    insert(plusFrag(nfa, unionFrag(nfa, space, tab)), TK_WS);
    
    return nfa;
}

// DFA State for subset construction (renamed to avoid conflict with DFA.h)
struct LexerDFAState {
    int id = 0;
    std::unordered_map<char, int> trans;
    bool accept = false;
    std::vector<int> tokens;
    std::set<int> nfaStates;
    
    std::string label() const {
        if (nfaStates.size() <= 4) {
            std::string s = "{";
            bool first = true;
            for (int idx : nfaStates) {
                if (!first) s += ",";
                s += "q" + std::to_string(idx);
                first = false;
            }
            s += "}";
            return s;
        }
        return "D" + std::to_string(id);
    }
    
    std::string tokenLabel() const {
        if (!accept || tokens.empty()) return "";
        return tokenName(tokens[0]);
    }
};

// Epsilon closure
inline std::set<int> epsClosure(const FullNFA& nfa, const std::set<int>& in) {
    std::set<int> res = in;
    std::vector<int> st(in.begin(), in.end());
    
    while (!st.empty()) {
        int s = st.back();
        st.pop_back();
        for (auto& t : nfa.states[s].trans) {
            if (t.kind == L_EPS && !res.count(t.to)) {
                res.insert(t.to);
                st.push_back(t.to);
            }
        }
    }
    return res;
}

// Move via character
inline std::set<int> moveVia(const FullNFA& nfa, const std::set<int>& S, char c) {
    std::set<int> res;
    for (int s : S) {
        for (auto& t : nfa.states[s].trans) {
            if (labelMatches(t.kind, c, t.ch)) {
                res.insert(t.to);
            }
        }
    }
    return res;
}

// Get all printable characters
inline std::vector<char> allChars() {
    std::vector<char> v;
    for (int i = 32; i < 127; i++) v.push_back((char)i);
    v.push_back('\t');
    return v;
}

// Subset construction: NFA to DFA
inline std::vector<LexerDFAState> subsetConstruct(const FullNFA& nfa) {
    std::vector<LexerDFAState> dfa;
    std::map<std::set<int>, int> id;
    std::queue<std::set<int>> q;
    
    std::set<int> s0 = epsClosure(nfa, {nfa.start});
    id[s0] = 0;
    
    LexerDFAState startState;
    startState.id = 0;
    startState.nfaStates = s0;
    for (int s : s0) {
        if (nfa.acceptToken.count(s)) {
            startState.accept = true;
            startState.tokens.push_back(nfa.acceptToken.at(s));
        }
    }
    dfa.push_back(startState);
    
    q.push(s0);
    auto chars = allChars();
    
    while (!q.empty()) {
        auto S = q.front();
        q.pop();
        int sid = id[S];
        
        for (char c : chars) {
            auto mv = moveVia(nfa, S, c);
            if (mv.empty()) continue;
            
            auto U = epsClosure(nfa, mv);
            if (!id.count(U)) {
                int nid = dfa.size();
                id[U] = nid;
                
                LexerDFAState newState;
                newState.id = nid;
                newState.nfaStates = U;
                for (int s : U) {
                    if (nfa.acceptToken.count(s)) {
                        newState.accept = true;
                        newState.tokens.push_back(nfa.acceptToken.at(s));
                    }
                }
                dfa.push_back(newState);
                q.push(U);
            }
            
            dfa[sid].trans[c] = id[U];
        }
    }
    
    return dfa;
}
