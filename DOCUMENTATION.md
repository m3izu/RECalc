# RECalc - Lexical Analyzer Pipeline Documentation

## Table of Contents
1. [Project Overview](#1-project-overview)
2. [Beginner's Guide](#2-beginners-guide-understanding-the-concepts)
3. [Glossary of Terms](#3-glossary-of-terms)
4. [Theoretical Foundation](#4-theoretical-foundation)
5. [Pipeline Architecture](#5-pipeline-architecture)
6. [Stage 0: NFA Construction](#6-stage-0-nfa-construction)
7. [Stage 1: Input & Live Tokenization](#7-stage-1-input--live-tokenization-preview)
8. [Stage 2: DFA Tokenization](#8-stage-2-dfa-tokenization)
9. [Stage 3: PDA Parsing](#9-stage-3-pda-parsing)
10. [Implementation Details](#10-implementation-details)
11. [Grammar Specification](#11-grammar-specification)
12. [FAQs (40 Questions)](#12-frequently-asked-questions-faqs)

---

## 2. Beginner's Guide: Understanding the Concepts

> **If you're new to automata and formal languages, start here!** This section explains everything in simple terms with real-world analogies.

### What is a Compiler Doing?

Imagine you're reading a recipe written in French, but you only understand English. A **compiler** is like a translator that:
1. **Reads** the French text character by character
2. **Groups** characters into words (tokenization)
3. **Checks** if the sentence structure makes sense (parsing)
4. **Translates** to English (code generation)

Our project focuses on steps 1-3: reading, grouping, and checking.

### The Vending Machine Analogy (What is an Automaton?)

Think of a **vending machine**:
- It has **states**: waiting, accepting coins, dispensing
- It reads **input**: coins and button presses
- It **transitions** between states based on input
- It has an **accept state**: successfully dispensed item

```
     ┌──────────┐   insert coin   ┌──────────┐   press button   ┌──────────┐
────►│  WAITING │───────────────►│  READY   │────────────────►│ DISPENSE │
     └──────────┘                 └──────────┘                  └──────────┘
```

This is exactly what a **Finite Automaton** does! It reads input and changes states based on what it sees.

### Why Can't a Vending Machine Count Parentheses?

The vending machine has **limited memory** - it only knows its current state, not history. 

**Problem**: Match parentheses like `((()))`
- After seeing `(((`, how many `)` do we need? **Three.**
- But a simple machine doesn't have a counter!

**Solution**: Add a **stack** (like a pile of plates)
- See `(` → push plate onto stack
- See `)` → pop plate from stack
- At end, stack should be empty!

This is a **Pushdown Automaton (PDA)** - a vending machine with a stack!

### The Recipe Analogy (What is a Grammar?)

A grammar is like a recipe structure:
```
MEAL       → APPETIZER + MAIN_COURSE + DESSERT
MAIN_COURSE → PROTEIN + SIDE
SIDE       → VEGETABLES | RICE | POTATOES
```

This defines valid "meals". Similarly, our grammar defines valid expressions:
```
EXPRESSION → TERM + MORE_TERMS
TERM       → FACTOR × MORE_FACTORS  
FACTOR     → NUMBER | VARIABLE | (EXPRESSION)
```

### The Three Stages Explained Simply

#### Stage 1: Tokenization (Finding Words)

**Like:** Reading "THE CAT SAT" and identifying: [ARTICLE, NOUN, VERB]

**Input:** `3 + foo * 2`
**Output:** `[NUMBER, PLUS, IDENTIFIER, TIMES, NUMBER]`

The computer groups characters into meaningful units called **tokens**.

#### Stage 2: DFA Scanning (Character by Character)

**Like:** A spell-checker that reads letter by letter

The DFA (Deterministic Finite Automaton) is like a very simple robot:
- **It can only remember ONE thing**: its current state
- **It reads ONE character at a time**
- **It follows strict rules**: "If I see a letter, go to the IDENTIFIER state"

```
You type: f o o
DFA sees: 'f' → go to ID state
          'o' → stay in ID state  
          'o' → stay in ID state
          ' ' → emit TOKEN: IDENTIFIER("foo")
```

#### Stage 3: PDA Parsing (Checking Structure)

**Like:** A grammar checker that verifies sentence structure

The PDA has something special: a **stack** (like a pile of cards)
- When it sees a rule to expand, it **pushes** symbols onto the stack
- When it matches input, it **pops** from the stack
- If the stack is empty when input is done, **SUCCESS!**

### Visual Metaphors

#### The Stack as a Stack of Plates
```
           ┌─────┐
           │  F  │  ← TOP (work on this first!)
           ├─────┤
           │  T' │
           ├─────┤
           │  E' │
           ├─────┤
           │  $  │  ← BOTTOM (end marker)
           └─────┘
```
You can only:
- **Push**: Add plate on top
- **Pop**: Remove plate from top
- **Peek**: Look at top plate

#### State Transitions as a Map
```
Think of states as CITIES and transitions as ROADS:

         letter                    letter or digit
    ┌───────────────────────────┬────────────────────┐
    │                           │                    │
    ▼                           │                    ▼
┌───────┐     digit        ┌────┴────┐          ┌────────┐
│ START │─────────────────►│   NUM   │          │   ID   │◄─┘
└───────┘                  └─────────┘          └────────┘
```

### Common Questions from Beginners

**Q: Why is this so complicated? Can't the computer just understand math?**
> Computers are dumb! They only understand 1s and 0s. We need to build up from simple rules. An automaton is the simplest possible "understanding machine."

**Q: What's the difference between NFA and DFA?**
> - **DFA**: Like a GPS with ONE route per destination. Clear, but sometimes longer.
> - **NFA**: Like having MULTIPLE possible routes. More flexible, but need to try all options.

**Q: Why do we need both tokenization AND parsing?**
> - **Tokenization**: Finds the words ("THE", "CAT", "SAT")
> - **Parsing**: Checks grammar ("THE CAT SAT" is valid, "CAT THE SAT" is not)
>
> You can't check grammar on random letters - you need words first!

**Q: What does ε (epsilon) mean?**
> It means "nothing" or "empty". Like a silent letter:
> - Transition on 'a': Move when you see 'a'
> - Transition on ε: Move without seeing anything (free move!)

**Q: Why is the stack so important?**
> The stack gives the machine **memory**. Without it:
> - DFA can match: `(())` ← YES, but how? (can't count)
> - With stack: Push `(`, push `(`, pop `)`, pop `)` → EMPTY = MATCHED!

### Design Rationale: Why We Made These Choices

This section explains **WHY** we chose each component and approach.

#### Why Thompson's Construction for NFA?

| Alternative | Why NOT | Why Thompson's |
|-------------|---------|----------------|
| Direct DFA construction | Complex, not systematic | Simple, one rule per regex operator |
| Glushkov's construction | More complex states | Cleaner, one start/one accept |
| Brzozowski's derivatives | Abstract, hard to visualize | Easy to draw and animate |

**Our reasoning**: Thompson's construction is the most **visual** and **educational**. Each regex operator (concatenation, alternation, Kleene star) maps to a clear diagram pattern.

#### Why DFA for Tokenization?

| Alternative | Why NOT | Why DFA |
|-------------|---------|---------|
| NFA directly | Would need backtracking | Deterministic, one path |
| Hand-coded if-else | Error-prone, not general | Systematic, provably correct |
| Regex library | Black box, not educational | Shows the actual process |

**Our reasoning**: DFAs are **fast** (O(n) linear time) and **deterministic** (no backtracking). Perfect for teaching how real lexers work.

#### Why LL(1) Parsing?

| Alternative | Why NOT | Why LL(1) |
|-------------|---------|-----------|
| Recursive descent | Similar, but less formal | Table-driven, clear structure |
| LR(1) parsing | More powerful but complex | Simple enough to understand |
| Earley parsing | Handles any grammar | Overkill for our simple grammar |
| Hand-written parser | Not generalizable | Demonstrates formal methods |

**Our reasoning**: LL(1) is the **simplest table-driven parser**. The parsing table clearly shows which production to use. It's perfect for education.

#### Why This Specific Grammar?

```
E  → T E'      (not E → E + T)
E' → + T E'    (not left-recursive)
```

| Issue | Problem | Our Solution |
|-------|---------|--------------|
| Left recursion | Causes infinite loop in LL | Eliminated with E' |
| Ambiguity | Multiple parse trees | One derivation only |
| Precedence | `3+4*5` could be wrong | Built into grammar structure |

**Our reasoning**: The grammar is specifically designed for LL(1):
- No left recursion (would cause infinite loop)
- Left-factored (no common prefixes)
- Precedence encoded by nesting (T inside E, F inside T)

#### Why a Stack-Based PDA Visualization?

| Alternative | Why NOT | Why Stack-Based |
|-------------|---------|-----------------|
| Tree visualization | Complex to animate | Stack is simpler |
| State diagram only | Doesn't show memory | Stack shows the "why" |
| Table lookup only | Abstract, not visual | Physical stack metaphor |

**Our reasoning**: The stack is the **key insight** of context-free parsing. Showing it physically helps students understand WHY PDAs are more powerful than DFAs.

#### Why SFML + ImGui?

| Alternative | Why NOT | Why SFML + ImGui |
|-------------|---------|------------------|
| Qt | Heavy, complex signals | Lightweight, immediate mode |
| Web (HTML/JS) | No C++ integration | Native performance |
| OpenGL raw | Too low-level | ImGui handles widgets |
| Console only | No visualization | Graphics essential for education |

**Our reasoning**: ImGui's **immediate mode** paradigm is perfect for step-by-step animations. No complex state management, just draw each frame.

#### Why These Token Types?

| Token | Why Included | Why This Way |
|-------|--------------|--------------|
| NUMBER | Essential for math | Includes decimals |
| IDENTIFIER | Variables in expressions | Standard naming rules |
| OPERATORS | Basic arithmetic | +, -, *, / only (keep simple) |
| PARENS | Grouping/precedence | Essential for expressions |

**Not included**: 
- Unary minus: Complicates grammar significantly
- Exponentiation: Right-associative, different handling
- Functions: Would need call syntax, too complex

**Our reasoning**: We included **exactly what's needed** for basic arithmetic expressions. Adding more would obscure the core concepts.

#### Why Animate Step-by-Step?

| Alternative | Why NOT | Why Animation |
|-------------|---------|---------------|
| Final result only | Doesn't teach | Process is the point |
| All steps at once | Overwhelming | One step at a time |
| No controls | Can't study | Pause, step, reset |

**Our reasoning**: **The process IS the lesson**. Seeing each transition helps students understand causality: "This token caused this action."

#### Why Color Coding?

| Element | Color | WHY This Color |
|---------|-------|----------------|
| Current token | Yellow | High visibility, "spotlight" |
| Consumed tokens | Gray | "Done", less prominent |
| Future tokens | Blue | Calm, waiting |
| Error states | Red | Universal danger signal |
| Accept states | Green | Universal success signal |
| Stack top | Orange | Attention, "work here next" |

**Our reasoning**: Colors follow **universal conventions** (red=bad, green=good) and **direct attention** to what matters at each step.

### Complete Walkthrough: Processing "a + 3 * b"

Let's trace through the ENTIRE pipeline with a real example.

#### STEP 0: Raw Input
```
User types: a + 3 * b
Characters: ['a', ' ', '+', ' ', '3', ' ', '*', ' ', 'b']
```

#### STEP 1: Lexical Analysis (Tokenization)

The DFA scans character by character:

| Position | Character | DFA State | Action |
|----------|-----------|-----------|--------|
| 0 | 'a' | START → ID | Letter detected, enter ID state |
| 1 | ' ' | ID → START | Whitespace, emit token, return to start |
| 2 | '+' | START → OP | Operator detected |
| 3 | ' ' | OP → START | Emit OP(+), skip whitespace |
| 4 | '3' | START → NUM | Digit detected |
| 5 | ' ' | NUM → START | Emit NUM(3), skip whitespace |
| 6 | '*' | START → OP | Operator detected |
| 7 | ' ' | OP → START | Emit OP(*), skip whitespace |
| 8 | 'b' | START → ID | Letter detected |
| EOF | - | ID → END | Emit ID(b), add END token |

**Token Stream Produced:**
```
[ID("a"), OP("+"), NUM("3"), OP("*"), ID("b"), END("$")]
```

#### STEP 2: Parsing Initialization

**Initial Stack:** `$ E` (bottom marker and start symbol)
**Initial Input:** `ID(a) OP(+) NUM(3) OP(*) ID(b) $`

```
Stack (bottom to top):  $ E
                        ↑
Input cursor:           ID  +  3  *  b  $
                        ↑
```

#### STEP 3: Parsing Trace (Detailed)

| Step | Stack | Input | Lookahead | Table Lookup | Action | Explanation |
|------|-------|-------|-----------|--------------|--------|-------------|
| 1 | `$ E` | `ID + 3 * b $` | ID | E[ID] = TE' | Push E' T | E produces T followed by E' |
| 2 | `$ E' T` | `ID + 3 * b $` | ID | T[ID] = FT' | Push T' F | T produces F followed by T' |
| 3 | `$ E' T' F` | `ID + 3 * b $` | ID | F[ID] = id | Push id | F produces identifier |
| 4 | `$ E' T' id` | `ID + 3 * b $` | ID | Match! | Pop, Advance | Terminal matches input |
| 5 | `$ E' T'` | `+ 3 * b $` | + | T'[+] = ε | Pop | No more multiplications |
| 6 | `$ E'` | `+ 3 * b $` | + | E'[+] = +TE' | Push E' T + | Addition detected |
| 7 | `$ E' T +` | `+ 3 * b $` | + | Match! | Pop, Advance | Terminal matches |
| 8 | `$ E' T` | `3 * b $` | NUM | T[NUM] = FT' | Push T' F | Term expansion |
| 9 | `$ E' T' F` | `3 * b $` | NUM | F[NUM] = num | Push num | Factor is number |
| 10 | `$ E' T' num` | `3 * b $` | NUM | Match! | Pop, Advance | Terminal matches |
| 11 | `$ E' T'` | `* b $` | * | T'[*] = *FT' | Push T' F * | Multiplication detected |
| 12 | `$ E' T' F *` | `* b $` | * | Match! | Pop, Advance | Terminal matches |
| 13 | `$ E' T' F` | `b $` | ID | F[ID] = id | Push id | Factor is identifier |
| 14 | `$ E' T' id` | `b $` | ID | Match! | Pop, Advance | Terminal matches |
| 15 | `$ E' T'` | `$` | $ | T'[$] = ε | Pop | No more multiplications |
| 16 | `$ E'` | `$` | $ | E'[$] = ε | Pop | No more additions |
| 17 | `$` | `$` | $ | Accept! | SUCCESS | Both empty! |

#### STEP 4: Understanding Each Parse Action

**Expansion Actions (Non-terminals):**
- When top of stack is a non-terminal (E, E', T, T', F)
- Look up in parsing table using [non-terminal, lookahead]
- Replace non-terminal with right-hand side of production (reversed)

**Match Actions (Terminals):**
- When top of stack is a terminal (+, -, *, /, num, id, etc.)
- Compare with current input token
- If equal: pop stack, advance input
- If not equal: ERROR

**Epsilon Actions (ε):**
- Production produces nothing
- Simply pop the non-terminal from stack
- Do NOT consume any input

#### STEP 5: Precedence Demonstration

Notice how `3 * b` is handled:
1. After matching `3`, T' sees `*`
2. T' expands to `* F T'` (continues multiplication)
3. This groups `3 * b` together BEFORE returning to E'

The grammar structure ensures:
```
a + (3 * b)   ← Correct! * binds tighter than +
NOT: (a + 3) * b
```

#### Visual Stack Evolution

```
Step 1:  $ E
Step 2:  $ E' T
Step 3:  $ E' T' F
Step 4:  $ E' T' id  →  Match 'a'
Step 5:  $ E' T'     →  ε (no more *)
Step 6:  $ E'        →  E' sees '+', expands
Step 7:  $ E' T +    →  Match '+'
Step 8:  $ E' T      →  T expands for '3'
Step 9:  $ E' T' F
Step 10: $ E' T' num →  Match '3'
Step 11: $ E' T'     →  T' sees '*', expands
Step 12: $ E' T' F * →  Match '*'
Step 13: $ E' T' F   →  F expands for 'b'
Step 14: $ E' T' id  →  Match 'b'
Step 15: $ E' T'     →  ε (end of term)
Step 16: $ E'        →  ε (end of expression)
Step 17: $           →  ACCEPT!
```

#### Token Stream Animation

As parsing progresses, tokens are consumed:

```
Start:    [a] [+] [3] [*] [b] [$]
           ↑ current

Step 4:   [a] [+] [3] [*] [b] [$]
           ✓   ↑ current

Step 7:   [a] [+] [3] [*] [b] [$]
           ✓   ✓   ↑ current

Step 10:  [a] [+] [3] [*] [b] [$]
           ✓   ✓   ✓   ↑ current

Step 12:  [a] [+] [3] [*] [b] [$]
           ✓   ✓   ✓   ✓   ↑ current

Step 14:  [a] [+] [3] [*] [b] [$]
           ✓   ✓   ✓   ✓   ✓   ↑ current

Step 17:  [a] [+] [3] [*] [b] [$]
           ✓   ✓   ✓   ✓   ✓   ✓  ACCEPT!
```

#### Error Example: "a + * b"

What happens with invalid input?

| Step | Stack | Input | Action |
|------|-------|-------|--------|
| 1-5 | ... | ... | (same as before, match 'a') |
| 6 | `$ E'` | `* b $` | E' sees `*` |
| - | - | - | E'[*] = **ERROR** |

The table has no entry for E' with lookahead `*` because:
- E' expects `+`, `-`, `)`, or `$`
- `*` is not valid after a complete term

**Error Message**: "Expected '+', '-', ')' or end of expression, but got '*'"

---

## 3. Glossary of Terms

### Automata Theory Terms

| Term | Definition |
|------|------------|
| **Automaton** | An abstract machine that can be in one of a finite number of states, transitioning between states based on input symbols |
| **State** | A configuration of an automaton at a given point in time, representing the "memory" of what has been processed |
| **Transition** | A rule that specifies how an automaton moves from one state to another based on an input symbol |
| **Accept State** | A state that indicates successful recognition of input (shown as double circle) |
| **Start State** | The initial state where an automaton begins processing (marked with arrow) |
| **Alphabet (Σ)** | The finite set of symbols that an automaton can read as input |
| **Epsilon (ε)** | The empty string; a transition that moves between states without consuming input |

### Finite Automata Terms

| Term | Definition |
|------|------------|
| **DFA** | Deterministic Finite Automaton - has exactly one transition per symbol from each state |
| **NFA** | Non-deterministic Finite Automaton - can have multiple transitions for the same symbol |
| **Epsilon Transition** | A transition that occurs without reading any input symbol |
| **Subset Construction** | Algorithm to convert an NFA to an equivalent DFA |
| **Thompson's Construction** | Algorithm to convert a regular expression to an NFA |

### Grammar and Language Terms

| Term | Definition |
|------|------------|
| **Grammar** | A set of rules that define the structure of valid strings in a language |
| **Terminal** | A symbol that appears directly in the input (e.g., `+`, `num`, `id`) |
| **Non-terminal** | A symbol that can be replaced by other symbols via production rules (e.g., E, T, F) |
| **Production Rule** | A rule that defines how a non-terminal can be replaced (e.g., E → T E') |
| **Derivation** | The process of applying production rules to generate a string |
| **CFG** | Context-Free Grammar - grammar where each rule has a single non-terminal on the left |
| **Regular Language** | A language that can be recognized by a DFA/NFA (less powerful than CFG) |
| **Context-Free Language** | A language that can be recognized by a PDA (more powerful than regular) |

### Parsing Terms

| Term | Definition |
|------|------------|
| **Parser** | A program that analyzes input according to grammar rules |
| **PDA** | Pushdown Automaton - a finite automaton with a stack for memory |
| **Stack** | A LIFO (Last In, First Out) data structure used by PDAs |
| **LL(1)** | Left-to-right, Leftmost derivation, 1 token lookahead parsing |
| **Lookahead** | The next token examined to decide which production to apply |
| **Parse Tree** | A tree representation of how a string is derived from the grammar |
| **Left Recursion** | When a non-terminal refers to itself as the leftmost symbol (problematic for LL) |
| **Left Factoring** | Technique to eliminate common prefixes in grammar rules |

### Lexical Analysis Terms

| Term | Definition |
|------|------------|
| **Lexer/Tokenizer** | A program that converts character streams into token streams |
| **Token** | A categorized unit of the input (e.g., NUMBER, IDENTIFIER, OPERATOR) |
| **Lexeme** | The actual string value of a token (e.g., "foo", "42", "+") |
| **Pattern** | A rule describing the set of lexemes matching a token type |
| **Regular Expression** | A notation for describing patterns in text |

### Compiler Terms

| Term | Definition |
|------|------------|
| **Lexical Analysis** | First phase of compilation - converting characters to tokens |
| **Syntax Analysis** | Second phase - verifying tokens follow grammar rules |
| **Semantic Analysis** | Third phase - type checking and meaning verification |
| **AST** | Abstract Syntax Tree - simplified parse tree focusing on structure |
| **Front-end** | Lexical + Syntax + Semantic analysis portions of a compiler |

### Operator Terms

| Term | Definition |
|------|------------|
| **Precedence** | The priority of operators (higher precedence operators bind tighter) |
| **Associativity** | Direction of evaluation for operators of same precedence (left or right) |
| **Binary Operator** | Operator that takes two operands (e.g., `a + b`) |
| **Unary Operator** | Operator that takes one operand (e.g., `-a`) - not supported in this implementation |

---

## 1. Project Overview

### What is RECalc?
RECalc is an **educational visualization tool** that demonstrates the complete pipeline of a lexical analyzer and parser. It shows how compilers and interpreters process mathematical expressions from raw text into structured representations.

### Purpose
- **Educational**: Visualize abstract compiler concepts in real-time
- **Interactive**: Step through each phase of compilation
- **Comprehensive**: Covers lexical analysis (tokenization) and syntax analysis (parsing)

### Key Features
- Thompson's Construction NFA visualization
- Animated DFA tokenization with state transitions
- LL(1) PDA parsing with stack visualization
- Step-by-step animation controls
- Real-time token highlighting

---

## 2. Theoretical Foundation

### 2.1 Regular Expressions (Regex)

**Definition**: A regular expression is a sequence of characters that defines a search pattern, primarily used for pattern matching with strings.

**Why are they important in compilers?**
Regular expressions define the lexical structure of programming languages. Each token type (identifier, number, operator) is defined by a regex pattern.

**Patterns used in RECalc:**
| Token Type | Regex Pattern | Description |
|------------|---------------|-------------|
| Identifier | `[a-zA-Z_][a-zA-Z0-9_]*` | Starts with letter/underscore, followed by alphanumeric |
| Number | `[0-9]+(\.[0-9]+)?` | One or more digits, optionally with decimal |
| Operators | `+`, `-`, `*`, `/` | Single character operators |
| Parentheses | `(`, `)` | Grouping symbols |

### 2.2 Finite Automata

**Deterministic Finite Automaton (DFA)**
- Has exactly ONE transition for each symbol from each state
- No epsilon (ε) transitions
- Faster execution but potentially more states

**Non-deterministic Finite Automaton (NFA)**
- Can have MULTIPLE transitions for the same symbol
- Allows epsilon (ε) transitions (transitions without consuming input)
- Easier to construct from regex

**Key Relationship**: Every NFA can be converted to an equivalent DFA (Subset Construction Algorithm).

### 2.3 Pushdown Automata (PDA)

**Definition**: A PDA is a finite automaton enhanced with a **stack** memory. This allows it to recognize context-free languages, which regular automata cannot.

**Why PDAs for parsing?**
- Regular expressions cannot handle nested structures (like balanced parentheses)
- PDAs can track nesting depth using the stack
- Example: `((a + b) * c)` requires counting parentheses

**Components of a PDA:**
1. **States**: Finite set of states (q0, q1, ...)
2. **Input Alphabet**: Symbols to read (tokens)
3. **Stack Alphabet**: Symbols that can be pushed/popped
4. **Transition Function**: δ(state, input, stack_top) → (new_state, stack_operation)
5. **Start State**: Initial state (q0)
6. **Accept States**: States indicating successful parse

### 2.4 Context-Free Grammar (CFG)

**Definition**: A formal grammar where every production rule has the form:
```
A → α
```
Where A is a single non-terminal and α is a string of terminals and/or non-terminals.

**CFG used in RECalc:**
```
E  → T E'
E' → + T E' | - T E' | ε
T  → F T'
T' → * F T' | / F T' | ε
F  → ( E ) | num | id
```

**Why this grammar?**
- Left-factored to eliminate ambiguity
- Suitable for LL(1) parsing (no left recursion)
- Correctly implements operator precedence (`*` and `/` bind tighter than `+` and `-`)

---

## 3. Pipeline Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        RECalc COMPILER PIPELINE                         │
└─────────────────────────────────────────────────────────────────────────┘

    INPUT STRING                    TOKENS                      PARSE TREE
       "3 + a"          →        [NUM, OP, ID]        →        Accept/Reject
          │                           │                              │
          ▼                           ▼                              ▼
    ┌──────────┐              ┌──────────────┐              ┌──────────────┐
    │  STAGE 0 │              │   STAGE 2    │              │   STAGE 3    │
    │   NFA    │              │     DFA      │              │     PDA      │
    │ Thompson │     ───►     │ Tokenization │     ───►     │   Parsing    │
    │Construct.│              │              │              │              │
    └──────────┘              └──────────────┘              └──────────────┘
          │                           │                              │
    Builds pattern              Scans input and              Uses stack to
    recognition                 produces tokens              verify grammar
    automaton                   with types                   and structure
```

### Data Flow:
1. **NFA Construction** (offline): Build automaton from regex patterns
2. **Tokenization** (Stage 2): Convert character stream to token stream
3. **Parsing** (Stage 3): Verify token stream follows grammar rules

---

## 4. Stage 0: NFA Construction

### Thompson's Construction Algorithm

**Purpose**: Convert regular expressions into equivalent NFAs.

**Key Properties:**
- Each regex operator maps to a specific NFA structure
- Simple, systematic construction
- Results in NFA with ONE start state and ONE accept state

### Construction Rules:

**1. Single Character (a)**
```
    ┌───┐  a   ┌───┐
───►│ s │─────►│ f │⊙
    └───┘      └───┘
```

**2. Concatenation (AB)**
```
    ┌─────────┐  ε   ┌─────────┐
───►│   NFA_A │─────►│   NFA_B │───►
    └─────────┘      └─────────┘
```

**3. Alternation (A|B)**
```
              ┌─────────┐
         ε  ─►│   NFA_A │─► ε
           ╱  └─────────┘    ╲
    ┌───┐ ╱                    ╲ ┌───┐
───►│ s │                        │ f │⊙
    └───┘ ╲                    ╱ └───┘
           ╲  ┌─────────┐    ╱
         ε  ─►│   NFA_B │─► ε
              └─────────┘
```

**4. Kleene Star (A*)**
```
                    ε
              ┌───────────┐
              │           │
              ▼           │
    ┌───┐  ε  ┌─────────┐ │  ε   ┌───┐
───►│ s │────►│   NFA_A │─┴─────►│ f │⊙
    └───┘     └─────────┘        └───┘
      │              ε             ▲
      └────────────────────────────┘
```

### Implementation in RECalc (LexerNFA.h)

The `buildCombinedNFA()` function constructs the complete NFA:

```cpp
FullNFA buildCombinedNFA() {
    FullNFA nfa;
    // Start state (q0)
    int start = nfa.addState();
    nfa.startState = start;
    
    // Build pattern NFAs for each token type
    buildIdentifierNFA(nfa, start);   // [a-zA-Z_][a-zA-Z0-9_]*
    buildNumberNFA(nfa, start);       // [0-9]+(\.[0-9]+)?
    buildOperatorNFA(nfa, start);     // +, -, *, /
    buildParenthesisNFA(nfa, start);  // (, )
    
    return nfa;
}
```

---

## 5. Stage 1: Input & Live Tokenization Preview

### Purpose
Provides real-time feedback as the user types their expression.

### Features:
- **Input Field**: Text box for entering mathematical expressions
- **Live Preview**: Tokens are displayed immediately with color coding
- **Validation**: Errors are shown for invalid symbols

### Token Color Coding:
| Token Type | Color | Visual |
|------------|-------|--------|
| IDENTIFIER | Blue | Variables like `a`, `foo`, `x1` |
| NUMBER | Green | Numeric values like `3`, `42.5` |
| OPERATOR | Orange | `+`, `-`, `*`, `/` |
| LPAREN/RPAREN | Purple | `(`, `)` |
| INVALID | Red | Unrecognized symbols |

### Validation Checks:
1. **Invalid Symbols**: Characters not in the NFA alphabet
2. **Adjacent Operators**: `3 + + 4` (error)
3. **Trailing Operators**: `3 + 4 +` (error)
4. **Unbalanced Parentheses**: `(3 + 4` (error)
5. **Empty Parentheses**: `()` (error)

---

## 6. Stage 2: DFA Tokenization

### How DFA Tokenization Works

The DFA scans the input string character by character, transitioning between states until it reaches an accept state (token recognized) or an error state.

### DFA State Diagram:
```
                        ┌────┐
                   ────►│ q0 │
          START         └──┬─┘
                           │
         ┌─────────────────┼─────────────────┐
         │ [a-zA-Z]        │ [0-9]           │ +−*/()
         ▼                 ▼                 ▼
      ╔═════╗           ╔═════╗           ╔═════╗
      ║ q1  ║           ║ q2  ║           ║ q3  ║
      ╚═════╝           ╚═════╝           ╚═════╝
   [alnum_]↺         [0-9.]↺              
        ID               NUM                OP
```

### States:
| State | Name | Accept? | Description |
|-------|------|---------|-------------|
| q0 | START | No | Initial state |
| q1 | ID | Yes | Identifier (letter followed by alphanumeric) |
| q2 | NUM | Yes | Number (digits, optional decimal) |
| q3 | OP | Yes | Operator (+, -, *, /, (, )) |

### Tokenization Algorithm:
```
1. Start at state q0
2. Read next character
3. If whitespace: skip, continue
4. Transition to appropriate state based on character
5. If in accept state and next char doesn't continue token:
   - Emit token
   - Return to q0
6. Repeat until end of input
```

### Example: Tokenizing "a + 3"
| Step | Input | State | Action |
|------|-------|-------|--------|
| 1 | `a` | q0 → q1 | Transition on letter |
| 2 | ` ` | q1 | Whitespace, emit ID('a') |
| 3 | `+` | q0 → q3 | Emit OP('+') |
| 4 | ` ` | q0 | Skip whitespace |
| 5 | `3` | q0 → q2 | Transition on digit |
| 6 | EOF | q2 | Emit NUM('3') |

**Result**: `[ID('a'), OP('+'), NUM('3'), END('$')]`

---

## 7. Stage 3: PDA Parsing

### LL(1) Parsing Overview

**LL(1)** means:
- **L**: Left-to-right scanning of input
- **L**: Leftmost derivation
- **(1)**: One token lookahead

### Grammar Productions:
```
E  → T E'        (Expression is Term followed by E')
E' → + T E'      (E' handles addition)
E' → - T E'      (E' handles subtraction)
E' → ε           (E' can be empty)
T  → F T'        (Term is Factor followed by T')
T' → * F T'      (T' handles multiplication)
T' → / F T'      (T' handles division)
T' → ε           (T' can be empty)
F  → ( E )       (Factor can be parenthesized expression)
F  → num         (Factor can be a number)
F  → id          (Factor can be an identifier)
```

### LL(1) Parsing Table:

|     | num  | id   | +     | -     | *     | /     | (     | )   | $   |
|-----|------|------|-------|-------|-------|-------|-------|-----|-----|
| E   | TE'  | TE'  | -     | -     | -     | -     | TE'   | -   | -   |
| E'  | -    | -    | +TE'  | -TE'  | -     | -     | -     | ε   | ε   |
| T   | FT'  | FT'  | -     | -     | -     | -     | FT'   | -   | -   |
| T'  | -    | -    | ε     | ε     | *FT'  | /FT'  | -     | ε   | ε   |
| F   | num  | id   | -     | -     | -     | -     | (E)   | -   | -   |

### How to Read the Table:
- **Row**: Current top of stack (non-terminal)
- **Column**: Current input token (lookahead)
- **Cell**: Production to apply (or error `-`)

### PDA Algorithm:
```python
stack = ['$', 'E']  # Initialize with end marker and start symbol

while stack is not empty:
    top = stack.pop()
    current = input.peek()
    
    if top is terminal:
        if top == current:
            input.advance()  # Match!
        else:
            ERROR("Mismatch")
    
    elif top is non-terminal:
        production = table[top][current]
        if production exists:
            push production symbols onto stack (reversed)
        else:
            ERROR("No production")
    
    elif top == '$' and current == '$':
        ACCEPT()
```

### Example Parse: "3 + a"

**Tokens**: `NUM(3), OP(+), ID(a), END($)`

| Step | Stack | Input | Action |
|------|-------|-------|--------|
| 1 | `$ E` | `NUM + ID $` | E → T E' |
| 2 | `$ E' T` | `NUM + ID $` | T → F T' |
| 3 | `$ E' T' F` | `NUM + ID $` | F → num |
| 4 | `$ E' T' num` | `NUM + ID $` | Match NUM |
| 5 | `$ E' T'` | `+ ID $` | T' → ε |
| 6 | `$ E'` | `+ ID $` | E' → + T E' |
| 7 | `$ E' T +` | `+ ID $` | Match + |
| 8 | `$ E' T` | `ID $` | T → F T' |
| 9 | `$ E' T' F` | `ID $` | F → id |
| 10 | `$ E' T' id` | `ID $` | Match ID |
| 11 | `$ E' T'` | `$` | T' → ε |
| 12 | `$ E'` | `$` | E' → ε |
| 13 | `$` | `$` | **ACCEPT** |

### PDA State Diagram:
```
                        ┌────┐
                   ────►│ q0 │ (Start)
                        └──┬─┘
                           │
         ┌─────────────────┼─────────────────┐
         ▼                 ▼                 ▼
      ┌─────┐           ┌─────┐           ┌─────┐
      │  E  │           │  T  │           │  F  │
      └──┬──┘           └──┬──┘           └──┬──┘
         │                 │                 │
      ┌──┴──┐           ┌──┴──┐           ┌──┴──────┬───────┐
      ▼     ▼           ▼     ▼           ▼         ▼       ▼
   ┌────┐ ┌────┐     ┌────┐ ┌────┐     ┌─────┐   ┌────┐  ┌────┐
   │ E' │ │ T  │     │ T' │ │ F  │     │ NUM │   │ ID │  │ OP │
   └────┘ └────┘     └────┘ └────┘     └─────┘   └────┘  └────┘
                                          │         │       │
                                          └─────────┴───────┘
                                                    ▼
                                                ┌──────┐
                                                │  OK  │ (Accept)
                                                └──────┘
```

---

## 8. Implementation Details

### File Structure:
```
recalc rev/
├── main.cpp          # Main application, UI, pipeline control
├── Lexer.h           # Tokenizer implementation
├── LexerNFA.h        # Thompson's NFA construction
├── DFA.h             # DFA state machine for tokenization
├── PDA.h             # Pushdown automaton parser
├── NFA.h             # NFA data structures
├── Visualizer.h      # All visualization drawing functions
└── SubsetConstruction.h  # NFA to DFA conversion
```

### Key Data Structures:

**Token (Lexer.h)**
```cpp
struct Token {
    TokenType type;    // TOK_ID, TOK_NUMBER, TOK_PLUS, etc.
    std::string value; // Actual string value
    int pos;           // Position in input
};
```

**PDAStep (PDA.h)**
```cpp
struct PDAStep {
    std::string stackState;     // Current stack contents
    std::string inputRemaining; // Remaining input
    std::string action;         // Expand/Match/Accept/Error
    std::string explanation;    // Educational description
    int inputPosition;          // For highlighting
};
```

**NFA State (LexerNFA.h)**
```cpp
struct NFAState {
    int id;
    bool isAccept;
    std::string tokenType;  // "ID", "NUMBER", "OPERATOR", etc.
    std::map<char, std::vector<int>> transitions;
    std::vector<int> epsilonTransitions;
};
```

### Technologies Used:
- **SFML**: Window management and rendering
- **Dear ImGui**: Immediate-mode GUI framework
- **C++17**: Modern C++ features

---

## 9. Grammar Specification

### Extended Backus-Naur Form (EBNF):
```ebnf
expression = term, { ("+" | "-"), term };
term       = factor, { ("*" | "/"), factor };
factor     = number | identifier | "(", expression, ")";
number     = digit, { digit }, [ ".", digit, { digit } ];
identifier = letter, { letter | digit | "_" };
letter     = "a" | "b" | ... | "z" | "A" | ... | "Z";
digit      = "0" | "1" | ... | "9";
```

### Operator Precedence (Lowest to Highest):
1. `+`, `-` (Addition, Subtraction)
2. `*`, `/` (Multiplication, Division)
3. `()` (Parentheses - highest)

### Associativity:
All operators are **left-associative**.
- `3 - 2 - 1` = `(3 - 2) - 1` = `0`
- `8 / 4 / 2` = `(8 / 4) / 2` = `1`

---

## 10. Test Cases

### Valid Expressions (Should ACCEPT)

| # | Input | Tokens | Parse Result | Notes |
|---|-------|--------|--------------|-------|
| 1 | `3` | NUM(3) | ✅ Accept | Single number |
| 2 | `a` | ID(a) | ✅ Accept | Single identifier |
| 3 | `3 + 4` | NUM OP NUM | ✅ Accept | Simple addition |
| 4 | `a * b` | ID OP ID | ✅ Accept | Simple multiplication |
| 5 | `3 + 4 * 5` | NUM OP NUM OP NUM | ✅ Accept | Precedence test (=23) |
| 6 | `(3 + 4) * 5` | LP NUM OP NUM RP OP NUM | ✅ Accept | Parentheses override (=35) |
| 7 | `a + b + c` | ID OP ID OP ID | ✅ Accept | Left associativity |
| 8 | `x1 + y2 * z3` | ID OP ID OP ID | ✅ Accept | Alphanumeric identifiers |
| 9 | `((a))` | LP LP ID RP RP | ✅ Accept | Nested parentheses |
| 10 | `3.14 * r * r` | NUM OP ID OP ID | ✅ Accept | Decimal number |
| 11 | `(a + b) * (c + d)` | Complex | ✅ Accept | Multiple groups |
| 12 | `x / y - z` | ID OP ID OP ID | ✅ Accept | Division and subtraction |
| 13 | `foo + bar` | ID OP ID | ✅ Accept | Multi-char identifiers |
| 14 | `1 + 2 + 3 + 4` | NUM OP NUM OP NUM OP NUM | ✅ Accept | Chain of additions |
| 15 | `a * (b + c * d)` | Complex | ✅ Accept | Mixed precedence |

### Invalid Expressions (Should REJECT)

| # | Input | Error Type | Error Location | Explanation |
|---|-------|------------|----------------|-------------|
| 1 | `3 +` | Syntax | End | Trailing operator |
| 2 | `+ 3` | Syntax | Start | Leading operator |
| 3 | `3 + + 4` | Syntax | Position 4 | Consecutive operators |
| 4 | `3 4` | Syntax | Position 2 | Missing operator |
| 5 | `(3 + 4` | Lexer | End | Unclosed parenthesis |
| 6 | `3 + 4)` | Syntax | Position 6 | Unexpected closing paren |
| 7 | `()` | Syntax | Position 1 | Empty parentheses |
| 8 | `3 @ 4` | Lexer | Position 2 | Invalid character |
| 9 | `3 + * 4` | Syntax | Position 4 | * after + |
| 10 | `((3 + 4)` | Lexer | End | Unbalanced parentheses |
| 11 | `3..14` | Lexer | Position 2 | Invalid number format |
| 12 | `(a +)` | Syntax | Position 4 | Operator before ) |
| 13 | `* 3` | Syntax | Start | Expression starts with * |
| 14 | `3 / / 4` | Syntax | Position 4 | Consecutive / |
| 15 | `a b c` | Syntax | Position 2 | Multiple identifiers |

### Edge Cases

| # | Input | Expected | Why It's Tricky |
|---|-------|----------|-----------------|
| 1 | `0` | ✅ Accept | Zero is valid number |
| 2 | `   3   ` | ✅ Accept | Whitespace handling |
| 3 | `a_b_c` | ✅ Accept | Underscores in identifiers |
| 4 | `_var` | ✅ Accept | Identifier starting with _ |
| 5 | `x1y2z3` | ✅ Accept | Mixed alphanumeric |
| 6 | `3.0` | ✅ Accept | Decimal with zero |
| 7 | `.5` | ❌ Reject | Must start with digit |
| 8 | `3.` | ❌ Reject | Must have digits after . |

---

## 11. References and Bibliography

### Textbooks

1. **Hopcroft, J. E., Motwani, R., & Ullman, J. D.** (2006). *Introduction to Automata Theory, Languages, and Computation* (3rd ed.). Pearson.
   - Chapter 2: Finite Automata
   - Chapter 3: Regular Expressions and Languages
   - Chapter 5: Context-Free Grammars and Languages
   - Chapter 6: Pushdown Automata

2. **Aho, A. V., Lam, M. S., Sethi, R., & Ullman, J. D.** (2006). *Compilers: Principles, Techniques, and Tools* (2nd ed.). Pearson. (The "Dragon Book")
   - Chapter 3: Lexical Analysis
   - Chapter 4: Syntax Analysis

3. **Sipser, M.** (2012). *Introduction to the Theory of Computation* (3rd ed.). Cengage Learning.
   - Chapter 1: Regular Languages
   - Chapter 2: Context-Free Languages

### Academic Papers

4. **Thompson, K.** (1968). "Programming Techniques: Regular expression search algorithm." *Communications of the ACM*, 11(6), 419-422.
   - Original Thompson's Construction algorithm

5. **Rabin, M. O., & Scott, D.** (1959). "Finite automata and their decision problems." *IBM Journal of Research and Development*, 3(2), 114-125.
   - Foundation of finite automata theory

### Online Resources

6. **GeeksforGeeks** - Compiler Design Tutorials
   - https://www.geeksforgeeks.org/compiler-design-tutorials/

7. **Stanford CS143** - Compilers Course Materials
   - https://web.stanford.edu/class/cs143/

8. **JFLAP** - Java Formal Languages and Automata Package
   - http://www.jflap.org/

### Tools and Libraries Used

9. **SFML** - Simple and Fast Multimedia Library
   - https://www.sfml-dev.org/
   - Version 2.5.x

10. **Dear ImGui** - Immediate Mode GUI
    - https://github.com/ocornut/imgui
    - Used for UI rendering and controls

---

## 12. Frequently Asked Questions (FAQs)

### Theory Questions

**Q1: Why use an NFA instead of directly building a DFA?**
> NFAs are easier to construct from regular expressions using Thompson's construction. The construction is systematic and each regex operator maps directly to an NFA pattern. While NFAs have multiple possible transitions, they can always be converted to equivalent DFAs using the subset construction algorithm.

**Q2: What is the difference between a DFA and an NFA?**
> - **DFA**: Has exactly one transition per symbol from each state. No ambiguity, faster execution.
> - **NFA**: Can have multiple transitions for the same symbol, including epsilon (ε) transitions. Easier to construct but requires conversion for execution.

**Q3: Why can't regular expressions/DFAs handle nested parentheses?**
> Regular languages (recognized by DFAs) cannot count. To match balanced parentheses like `((()))`, you need to remember how many opening parens you've seen. This requires unbounded memory, which DFAs don't have. PDAs solve this with a stack.

**Q4: What does LL(1) mean and why is it important?**
> - **L**: Left-to-right scanning
> - **L**: Leftmost derivation
> - **(1)**: One token lookahead
> 
> LL(1) is important because it enables efficient, deterministic parsing. With just one lookahead token, we can always decide which production to use without backtracking.

**Q5: Why is the grammar left-factored?**
> Left factoring eliminates common prefixes in grammar productions. For example:
> - Before: `E → E + T | T` (left recursive - infinite loop!)
> - After: `E → T E'` and `E' → + T E' | ε` (no left recursion)
> 
> This is required for LL(1) parsing.

**Q6: What is operator precedence and how is it encoded in the grammar?**
> Operator precedence determines which operations are performed first. It's encoded by the grammar's structure:
> - Lower precedence operators (`+`, `-`) appear in higher-level rules (E)
> - Higher precedence operators (`*`, `/`) appear in lower-level rules (T)
> 
> This ensures `3 + 4 * 5` = `3 + (4 * 5)` = `23`, not `(3 + 4) * 5` = `35`.

**Q7: What is epsilon (ε) and why is it used?**
> Epsilon represents the "empty string" - producing nothing. It's used to make parts of the grammar optional. For example, `E' → ε` means E' can disappear, making the `+ T E'` part optional in expressions.

### Implementation Questions

**Q8: How does the tokenizer handle whitespace?**
> Whitespace is skipped during tokenization. The lexer's `nextTokenInternal()` function has a loop:
> ```cpp
> while (pos < input.size() && isspace(input[pos])) pos++;
> ```
> This consumes all whitespace before reading the next token.

**Q9: How does the parser track the current token being processed?**
> Each `PDAStep` contains an `inputPosition` field that stores the index of the current token. The visualization uses this to highlight the current token in yellow, consumed tokens in gray, and future tokens in blue.

**Q10: What happens when the parser encounters an error?**
> The parser immediately stops and records an error step with a descriptive message. For example:
> - "Mismatch: Expected '+' but got '*'"
> - "Invalid Factor start. Expected '(', Number, or Identifier"
> 
> The trace shows exactly where parsing failed.

**Q11: How is the stack visualized?**
> The stack is drawn horizontally from left to right. The leftmost element is the stack bottom (`$`), and the rightmost element is the top. The current top element is highlighted in orange.

**Q12: Why are there separate animation speeds for different stages?**
> Actually, the animation speed is shared across all stages via the `playSpeed` variable. Changing it in one stage affects all stages. Default is 0.5 seconds per step.

### Practical Questions

**Q13: What expressions can this parser handle?**
> Valid expressions include:
> - Numbers: `42`, `3.14`
> - Identifiers: `x`, `foo`, `var_1`
> - Operators: `+`, `-`, `*`, `/`
> - Parentheses: `(`, `)`
> 
> Examples: `3 + 4`, `a * b + c`, `(x + y) * z`, `3.14 * r * r`

**Q14: What are the limitations?**
> - No support for unary operators (`-3`)
> - No support for exponentiation (`2^3`)
> - No support for comparison operators (`<`, `>`, `==`)
> - No support for functions (`sin(x)`)
> - Maximum reasonable expression length for visualization

**Q15: How would you extend this to support more operators?**
> 1. Add new token types in `Lexer.h` (e.g., `TOK_POWER`)
> 2. Add pattern recognition in the lexer's `nextTokenInternal()`
> 3. Update the grammar with new productions
> 4. Update the LL(1) parsing table
> 5. Add cases in the PDA parser

**Q16: Can this parser evaluate expressions?**
> No, this implementation only performs **syntax analysis** (parsing). It verifies that the expression is grammatically correct. To evaluate expressions, you would need to:
> 1. Build an Abstract Syntax Tree (AST)
> 2. Walk the tree to compute values

### Defense-Specific Questions

**Q17: Why did you choose Thompson's construction over other methods?**
> Thompson's construction is:
> - Systematic and easy to implement
> - Produces NFAs with exactly one start and one accept state
> - Linear in the size of the regex
> - Well-suited for educational visualization

**Q18: Could you use a different parsing algorithm?**
> Yes, alternatives include:
> - **LR parsing**: More powerful, handles more grammars, but more complex
> - **Recursive descent**: Intuitive, matches grammar structure directly
> - **Earley parsing**: Handles any CFG, but less efficient
> 
> We chose LL(1) because it's simple, efficient, and the grammar is already in the right form.

**Q19: What is the time complexity of each stage?**
> - **Tokenization**: O(n) where n is input length
> - **Parsing**: O(n) where n is number of tokens
> - **Total**: O(n) linear time

**Q20: How does this relate to real compilers?**
> Real compilers follow similar stages:
> 1. **Lexical Analysis**: Tokenization (like our DFA stage)
> 2. **Syntax Analysis**: Parsing (like our PDA stage)
> 3. **Semantic Analysis**: Type checking, etc. (not implemented)
> 4. **Code Generation**: Output machine code (not implemented)
> 
> Our implementation covers the first two stages that are fundamental to all compilers.

### Advanced Theory Questions

**Q21: What is the Chomsky Hierarchy and where do our languages fit?**
> The Chomsky Hierarchy classifies languages by computational power:
> - **Type 3 (Regular)**: Recognized by DFA/NFA - used for tokenization
> - **Type 2 (Context-Free)**: Recognized by PDA - used for parsing
> - **Type 1 (Context-Sensitive)**: Recognized by linear-bounded automata
> - **Type 0 (Recursively Enumerable)**: Recognized by Turing machines
>
> Our lexer uses Type 3 (Regular) and our parser uses Type 2 (Context-Free).

**Q22: What is the Pumping Lemma and why does it matter?**
> The Pumping Lemma is a property that all regular languages must satisfy. It's used to PROVE that certain languages are NOT regular.
>
> Example: The language {aⁿbⁿ | n ≥ 0} (equal a's and b's) is NOT regular because it fails the pumping lemma. This is why we need PDAs for balanced parentheses.

**Q23: What is FIRST and FOLLOW in LL parsing?**
> - **FIRST(α)**: Set of terminals that can begin strings derived from α
> - **FOLLOW(A)**: Set of terminals that can appear immediately after A in some derivation
>
> These sets are used to construct the LL(1) parsing table. For example:
> - FIRST(E) = {num, id, (}
> - FOLLOW(E') = {), $}

**Q24: How do you know if a grammar is LL(1)?**
> A grammar is LL(1) if for every non-terminal A with productions A → α | β:
> 1. FIRST(α) ∩ FIRST(β) = ∅ (no common first terminals)
> 2. If ε ∈ FIRST(α), then FIRST(β) ∩ FOLLOW(A) = ∅
>
> Our grammar is carefully designed to satisfy these conditions.

**Q25: What is ambiguity in grammars?**
> A grammar is ambiguous if there exists a string with multiple distinct parse trees. For example:
> - Ambiguous: `E → E + E | num` allows "1+2+3" to be parsed two ways
> - Our grammar is unambiguous because precedence/associativity are built in.

### Error Handling Questions

**Q26: What types of errors can the lexer detect?**
> 1. **Invalid characters**: Symbols not in the alphabet (e.g., `@`, `#`, `%`)
> 2. **Malformed numbers**: Invalid decimal formats
> 3. **Consecutive operators**: `+ +` (two operators in a row)
> 4. **Trailing operators**: Expression ending with operator
> 5. **Unbalanced parentheses**: Missing opening or closing paren

**Q27: What types of errors can the parser detect?**
> 1. **Syntax errors**: Tokens in wrong order (e.g., `3 + * 4`)
> 2. **Missing operands**: `+ 3` (operator without left operand)
> 3. **Missing operators**: `3 4` (two operands without operator)
> 4. **Invalid factor**: Expected number/id/( but got something else

**Q28: How does error recovery work?**
> Currently, both lexer and parser use **panic mode** recovery:
> - Stop at first error
> - Report descriptive message
> - Highlight error location
>
> More advanced compilers use techniques like:
> - **Error productions**: Add grammar rules for common errors
> - **Phrase-level recovery**: Skip tokens until valid point found
> - **Global correction**: Find minimum edits to make input valid

### Implementation Deep-Dive Questions

**Q29: Why use ImGui for the GUI?**
> ImGui (Immediate Mode GUI) is chosen because:
> - Easy to integrate with SFML
> - No complex widget state management
> - Excellent for real-time visualization
> - Custom drawing with ImDrawList
> - Fast rendering suitable for animations

**Q30: How is the animation system implemented?**
> The animation uses a timer-based approach:
> ```cpp
> if (isPlaying) {
>     playTimer += deltaTime;
>     if (playTimer >= playSpeed) {
>         playTimer = 0;
>         currentStep++;
>     }
> }
> ```
> `playSpeed` controls delay between steps (0.1s to 2.0s).

**Q31: How does the stack visualization work?**
> The stack is drawn horizontally using ImGui's ImDrawList:
> 1. Parse stackState string (space-separated symbols)
> 2. Draw each symbol as a colored rectangle
> 3. Highlight the top (rightmost) element in orange
> 4. Position elements left-to-right

**Q32: How is token highlighting synchronized with parsing?**
> Each PDAStep stores an `inputPosition` field:
> - Set when the step is recorded
> - Used to highlight current token (yellow)
> - Tokens before position shown in gray (consumed)
> - Tokens after position shown in blue (pending)

### Comparison Questions

**Q33: How does LL(1) compare to LR(1) parsing?**
> | Feature | LL(1) | LR(1) |
> |---------|-------|-------|
> | Direction | Top-down | Bottom-up |
> | Power | Less powerful | More powerful |
> | Complexity | Simpler | More complex |
> | Left Recursion | Not allowed | Allowed |
> | Use Case | Hand-written parsers | Parser generators |
>
> We chose LL(1) for educational clarity.

**Q34: What's the difference between a parse tree and an AST?**
> - **Parse Tree**: Shows every production applied, including all non-terminals
> - **AST**: Simplified tree focusing on semantics, omits unnecessary nodes
>
> Example for "3 + 4":
> - Parse Tree: E → T E' → F T' E' → 3 ε + T E' → ...
> - AST: (+, 3, 4)

**Q35: How does this compare to flex/bison?**
> | This Project | flex/bison |
> |--------------|------------|
> | Educational focus | Production use |
> | Manual implementation | Generated code |
> | Visual animation | Command-line |
> | Simple grammar | Complex grammars |
> | Single expression | Full languages |

### Project-Specific Questions

**Q36: What happens when the user types in real-time?**
> The input is tokenized immediately using the lexer. Each keystroke triggers:
> 1. Clear previous tokens
> 2. Run tokenizer on new input
> 3. Display tokens with color coding
> 4. Show any validation errors
>
> This provides instant feedback without running the full parser.

**Q37: How does the application handle very long expressions?**
> The application can handle expressions of reasonable length. Limitations:
> - Display may scroll for very long token streams
> - Stack depth is limited by system memory
> - Animation may become slow with hundreds of steps
>
> For educational purposes, expressions under 50 tokens work best.

**Q38: Can the parser be modified to support new operators?**
> Yes! To add a new operator (e.g., `^` for power):
> 1. Add `TOK_POWER` to token types
> 2. Modify lexer to recognize `^`
> 3. Add power rule to grammar (consider right-associativity)
> 4. Update parsing table and PDA code
> 5. Update visualization colors

**Q39: Why is there a separate NFA visualization stage?**
> The NFA stage shows Thompson's Construction - how regular expressions become automata. This is typically only covered theoretically. Visualizing it:
> - Shows the building blocks of pattern matching
> - Explains why NFA→DFA conversion is needed
> - Connects regex theory to practical lexers

**Q40: What educational concepts are demonstrated?**
> | Concept | Where Demonstrated |
> |---------|-------------------|
> | Regular expressions | Pattern definitions |
> | Thompson's construction | NFA building |
> | NFA to DFA conversion | Subset construction |
> | DFA traversal | Token scanning |
> | Context-free grammars | Parse rules |
> | LL(1) parsing | Table-driven parsing |
> | Stack operations | PDA visualization |
> | Operator precedence | Grammar structure |

### Conceptual Deep-Dive Questions

**Q41: What makes a language "regular" vs "context-free"?**
> - **Regular**: Can be described by regex, recognized by DFA. Limited to patterns without counting or matching.
> - **Context-Free**: Requires a stack. Can handle nested structures, matching, counting.
>
> Example:
> - Regular: `a*b*` (any a's followed by any b's)
> - Context-Free: `aⁿbⁿ` (equal number of a's and b's)

**Q42: Why does left recursion cause infinite loops?**
> Consider: `E → E + T`
> 1. See E, look up E in table
> 2. Replace E with "E + T"
> 3. Now top of stack is E again
> 4. GOTO step 1 (infinite loop!)
>
> Solution: Use `E → T E'` and `E' → + T E'` instead.

**Q43: What is the role of the $ symbol?**
> `$` is the **end-of-input marker**. It serves two purposes:
> 1. **In input**: Marks where the token stream ends
> 2. **In stack**: Marks the bottom of the stack
>
> Accept condition: Both stack and input show only `$`.

**Q44: How does the grammar encode operator precedence?**
> Precedence is built into the grammar's **nesting structure**:
> ```
> E (handles + -)
>   └─ T (handles * /)
>        └─ F (base values)
> ```
> Since T is "inside" E, multiplication is evaluated before T returns to E for addition.

**Q45: What is the difference between scanning and parsing?**
> | Scanning (Lexing) | Parsing |
> |-------------------|---------|
> | Characters → Tokens | Tokens → Structure |
> | Uses DFA | Uses PDA |
> | Recognizes patterns | Verifies grammar |
> | Linear patterns | Nested structures |
> | "What words?" | "Valid sentence?" |

### Practical Application Questions

**Q46: How would you add support for functions like sin(x)?**
> 1. Add `TOK_FUNCTION` token type
> 2. Lexer recognizes function names (sin, cos, etc.)
> 3. Add grammar rule: `F → FUNC ( E )`
> 4. Update parsing table
> 5. Handle in PDA code
>
> This shows extensibility of the grammar approach.

**Q47: Could this parser evaluate the expression?**
> The current parser only validates syntax. To evaluate:
> 1. Build an AST during parsing
> 2. Add value fields to AST nodes
> 3. Traverse tree post-order
> 4. Compute values at each node
>
> This is a natural next step but out of scope for this project.

**Q48: How would you handle comments in the input?**
> Add comment handling in the lexer:
> ```cpp
> if (input[pos] == '/' && input[pos+1] == '/') {
>     // Skip until newline
>     while (pos < input.size() && input[pos] != '\n') pos++;
> }
> ```
> Comments are stripped before parsing.

**Q49: What would change for a different programming language?**
> To adapt for a different language (e.g., JSON):
> 1. Define new token types (STRING, COLON, COMMA, BRACE)
> 2. Write new regex patterns for lexer
> 3. Design grammar for new syntax
> 4. Rebuild parsing table
> 5. The core DFA/PDA concepts remain the same!

**Q50: How do real compilers like GCC handle this?**
> Real compilers use similar concepts but optimized:
> - Lexer: Hand-optimized DFA or generated by flex
> - Parser: Often LR(1) generated by bison/yacc
> - Additional passes: Semantic analysis, optimization, code generation
>
> Our project demonstrates the fundamental algorithms that underlie these tools.

---

## Appendix A: Symbol Reference

| Symbol | Meaning |
|--------|---------|
| → | "produces" in grammar rules |
| \| | "or" - alternative productions |
| ε | Epsilon - empty string |
| $ | End-of-input marker |
| ⊙ | Accept state (double circle) |
| q0, q1, ... | State names |

---

## Appendix B: Quick Reference

### Valid Token Types:
- `IDENTIFIER`: a, foo, x1, var_name
- `NUMBER`: 3, 42, 3.14, 0.5
- `OPERATOR`: +, -, *, /
- `LPAREN`: (
- `RPAREN`: )
- `END`: $ (end of input)

### Grammar Rules:
```
E  → T E'
E' → + T E' | - T E' | ε
T  → F T'
T' → * F T' | / F T' | ε  
F  → ( E ) | num | id
```

### Accept Conditions:
- Stack contains only `$`
- Input contains only `$` (or TOK_END)
- Both conditions met = **ACCEPT**

---

## Project Team

### Developers

| Name | Role |
|------|------|
| **Franco Galendez** | Team Member |
| **Mariann Mesa** | Team Member |
| **Aldwyn Betonio** | Team Member |
| **Dax Romualdez** | Team Member |

### Institution

**University of Science and Technology of Southern Philippines (USTP)**

**Program**: Bachelor of Science in Computer Science

**Section**: CS3D

**Course**: Automata Theory and Formal Languages

---

## Acknowledgments

This project was developed as part of the Automata Theory and Formal Languages course to demonstrate:
- Thompson's Construction for NFA building
- DFA-based tokenization (lexical analysis)
- LL(1) PDA parsing (syntax analysis)
- Visual representation of compiler front-end concepts

---

*Documentation generated for RECalc - Educational Lexical Analyzer Pipeline*

*University of Science and Technology of Southern Philippines - CS3D*

*Last updated: December 2024*
