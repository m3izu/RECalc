#pragma once
#include <vector>
#include <map>
#include <string>
#include <stack>
#include <set>
#include <memory>

struct NState {
    int id;
    std::map<char, std::vector<NState*>> trans; // char '\0' used for epsilon
    bool accept = false;
    NState(int i) : id(i) {}
};

struct NFAFragment {
    NState* start;
    NState* accept;
};

class ThompsonNFA {
    std::vector<std::unique_ptr<NState>> owned;
    int nextId = 0;
public:
    NState* start = nullptr;
    NState* accept = nullptr;
    std::vector<std::string> trace;

    ThompsonNFA() = default;

    NState* makeState() {
        owned.push_back(std::make_unique<NState>(nextId));
        nextId++; // Fixed: Increment nextId
        return owned.back().get();
    }

    // Insert explicit concatenation operator '.' into regex
    static std::string insertConcat(const std::string &in) {
        std::string out;
        auto isSymbol = [](char c){ return c!='|' && c!='*' && c!=')' && c!='('; };
        for (size_t i=0;i<in.size();++i) {
            char c = in[i];
            out.push_back(c);
            if (i+1<in.size()) {
                char d = in[i+1];
                if ((isSymbol(c) || c==')' || c=='*') && (isSymbol(d) || d=='(')) out.push_back('.');
            }
        }
        return out;
    }

    // shunting-yard for regex to postfix
    static std::string toPostfix(const std::string &in) {
        std::string input = insertConcat(in);
        std::string out;
        std::stack<char> ops;
        auto prec = [](char o){ if (o=='*') return 3; if (o=='.') return 2; if (o=='|') return 1; return 0; };
        for (char c : input) {
            if (c=='(') { ops.push(c); }
            else if (c==')') { while(!ops.empty() && ops.top()!='(') { out.push_back(ops.top()); ops.pop(); } if(!ops.empty()) ops.pop(); }
            else if (c=='*' || c=='|' || c=='.') {
                while(!ops.empty() && prec(ops.top())>=prec(c)) { out.push_back(ops.top()); ops.pop(); }
                ops.push(c);
            } else {
                out.push_back(c);
            }
        }
        while(!ops.empty()) { out.push_back(ops.top()); ops.pop(); }
        return out;
    }

    void buildFromRegex(const std::string &regex) {
        trace.clear();
        owned.clear();
        nextId = 0;
        std::string postfix = toPostfix(regex);
        std::stack<NFAFragment> st;
        for (char c : postfix) {
            if (c=='.') {
                auto b = st.top(); st.pop();
                auto a = st.top(); st.pop();
                // connect a.accept -> epsilon -> b.start
                a.accept->trans[0].push_back(b.start);
                NFAFragment f{a.start, b.accept};
                st.push(f);
            } else if (c=='|') {
                auto b = st.top(); st.pop();
                auto a = st.top(); st.pop();
                NState* s = makeState();
                NState* e = makeState();
                s->trans[0].push_back(a.start);
                s->trans[0].push_back(b.start);
                a.accept->trans[0].push_back(e);
                b.accept->trans[0].push_back(e);
                NFAFragment f{s,e};
                st.push(f);
            } else if (c=='*') {
                auto a = st.top(); st.pop();
                NState* s = makeState();
                NState* e = makeState();
                s->trans[0].push_back(a.start);
                s->trans[0].push_back(e);
                a.accept->trans[0].push_back(a.start);
                a.accept->trans[0].push_back(e);
                NFAFragment f{s,e};
                st.push(f);
            } else {
                NState* s = makeState();
                NState* e = makeState();
                s->trans[c].push_back(e);
                NFAFragment f{s,e};
                st.push(f);
            }
        }
        if (st.empty()) { start = accept = nullptr; return; }
        auto top = st.top(); st.pop();
        start = top.start; accept = top.accept; accept->accept = true;
        // produce human-readable transitions
        for (auto &p : owned) {
            for (auto &kv : p->trans) {
                char sym = kv.first;
                for (auto *t : kv.second) {
                    std::string s = "q" + std::to_string(p->id) + " -" + (sym==0?std::string("eps"):std::string(1,sym)) + "-> q" + std::to_string(t->id);
                    trace.push_back(s);
                }
            }
        }
    }

    // epsilon-closure
    void epsilonClosure(const std::set<NState*> &input, std::set<NState*> &out, std::vector<std::string> *traceSteps=nullptr) {
        std::stack<NState*> st;
        for (auto *s : input) { if (!out.count(s)) { out.insert(s); st.push(s); } }
        while(!st.empty()) {
            NState* cur = st.top(); st.pop();
            auto it = cur->trans.find(0);
            if (it==cur->trans.end()) continue;
                for (auto *nxt : it->second) {
                if (!out.count(nxt)) { out.insert(nxt); st.push(nxt); if (traceSteps) traceSteps->push_back("eps-closure add q"+std::to_string(nxt->id)); }
            }
        }
    }

    bool simulate(const std::string &s, std::vector<std::string> &outSteps) {
        outSteps.clear();
        if (!start) return false;
        std::set<NState*> current;
        std::set<NState*> startSet;
        startSet.insert(start);
        epsilonClosure(startSet, current, &outSteps);
        outSteps.push_back("Start closure size=" + std::to_string(current.size()));
        for (size_t i=0;i<s.size();++i) {
            char c = s[i];
            outSteps.push_back(std::string("Read '") + c + "'");
            std::set<NState*> nexts;
            for (auto *stt : current) {
                auto it = stt->trans.find(c);
                if (it!=stt->trans.end()) {
                    for (auto *t : it->second) nexts.insert(t);
                }
            }
            std::set<NState*> nextsClosure;
            epsilonClosure(nexts, nextsClosure, &outSteps);
            current = nextsClosure;
            outSteps.push_back("Active states: " + std::to_string(current.size()));
        }
        for (auto *sstate : current) if (sstate->accept) { outSteps.push_back("Accepted"); return true; }
        outSteps.push_back("Rejected");
        return false;
    }
};
