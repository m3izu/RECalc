# RECalc: Visual Arithmetic & Regex Engine Documentation
Mariann Mesa, Franco Galendez, Aldwyn Betonio, Dax Romualdez

## 1. High-Level Overview
**RECalc** is an educational visualization tool that demonstrates two fundamental computer science concepts:
1.  **Compiler Design**: How computers understand and evaluate mathematical expressions.
2.  **Automata Theory**: How regular expressions are processed using Finite Automata.

The application provides a Graphical User Interface (GUI) built with **SFML** and **ImGui** to visualize the internal steps of these processes in real-time.

---

## 2. Arithmetic Engine (Compiler Theory)
The Arithmetic mode processes mathematical expressions (e.g., `3 * (4 + 2)`) through a standard compiler pipeline: **Lexing → Parsing → Evaluation**.

### 2.1 Lexical Analysis (The Lexer)
*   **Goal**: Convert raw text into a stream of meaningful "Tokens".
*   **Process**: The [Lexer](file:///z:/kod/automatafpit/Lexer.h#22-23) scans the input string character by character.
    *   It ignores whitespace.
    *   It groups digits into `TOK_NUMBER` (e.g., "1", "2", "3" becomes `123`).
    *   It identifies symbols like `+`, `-`, `*`, `/`, `(`, `)` and assigns them types (e.g., `TOK_PLUS`).
*   **Code**: [Lexer.h](file:///z:/kod/automatafpit/Lexer.h)
    *   [tokenizeAll()](file:///z:/kod/automatafpit/Lexer.h#32-39): The main loop driving the scan.
    *   [nextTokenInternal()](file:///z:/kod/automatafpit/Lexer.h#40-67): The logic to identify the next token type.

#### Example: `10 + 5`
| Input Char | Action | Token Generated |
| :--- | :--- | :--- |
| `1` | Start Number | ... |
| `0` | Continue Number | `TOK_NUMBER("10")` |
| ` ` | Skip Whitespace | - |
| `+` | Match Symbol | `TOK_PLUS("+")` |
| ` ` | Skip Whitespace | - |
| `5` | Start Number | `TOK_NUMBER("5")` |

### 2.2 Syntax Analysis (The Parser)
*   **Goal**: Organize tokens into a hierarchical structure called an **Abstract Syntax Tree (AST)** that respects order of operations (precedence).
*   **Method**: **Recursive Descent Parsing**.
*   **Grammar**:
    *   [Expression](file:///z:/kod/automatafpit/Parser.h#42-43) -> [AddSub](file:///z:/kod/automatafpit/Parser.h#44-57)
    *   [AddSub](file:///z:/kod/automatafpit/Parser.h#44-57) -> [MulDiv](file:///z:/kod/automatafpit/Parser.h#58-71) (+/- [MulDiv](file:///z:/kod/automatafpit/Parser.h#58-71))*
    *   [MulDiv](file:///z:/kod/automatafpit/Parser.h#58-71) -> [Unary](file:///z:/kod/automatafpit/Parser.h#22-23) (*// [Unary](file:///z:/kod/automatafpit/Parser.h#22-23))*
    *   [Unary](file:///z:/kod/automatafpit/Parser.h#22-23) -> (+/-) [Unary](file:///z:/kod/automatafpit/Parser.h#22-23) | [Primary](file:///z:/kod/automatafpit/Parser.h#83-100)
    *   [Primary](file:///z:/kod/automatafpit/Parser.h#83-100) -> [Number](file:///z:/kod/automatafpit/Parser.h#14-18) | `(` [Expression](file:///z:/kod/automatafpit/Parser.h#42-43) `)`
*   **Code**: [Parser.h](file:///z:/kod/automatafpit/Parser.h)
    *   Each grammar rule corresponds to a function (e.g., [parseAddSub](file:///z:/kod/automatafpit/Parser.h#44-57), [parseMulDiv](file:///z:/kod/automatafpit/Parser.h#58-71)).

#### Example: `3 * (4 + 2)`
The parser builds this tree structure:
```text
Binary(*)      <-- Multiplication happens last (Root)
├── Number(3)  <-- Left Operand
└── Binary(+)  <-- Right Operand (Result of addition)
    ├── Number(4)
    └── Number(2)
```
**How it works**:
1.  [parseAddSub](file:///z:/kod/automatafpit/Parser.h#44-57) calls [parseMulDiv](file:///z:/kod/automatafpit/Parser.h#58-71).
2.  [parseMulDiv](file:///z:/kod/automatafpit/Parser.h#58-71) sees `3` and `*`. It knows it needs a Right hand side.
3.  It calls [parseUnary](file:///z:/kod/automatafpit/Parser.h#72-82) -> [parsePrimary](file:///z:/kod/automatafpit/Parser.h#83-100).
4.  [parsePrimary](file:///z:/kod/automatafpit/Parser.h#83-100) sees `(`. It recursively calls [parseExpression](file:///z:/kod/automatafpit/Parser.h#42-43) for the inside.
5.  Inside [(4+2)](file:///z:/kod/automatafpit/main.cpp#17-152), [parseAddSub](file:///z:/kod/automatafpit/Parser.h#44-57) builds the [Binary(+)](file:///z:/kod/automatafpit/Parser.h#25-30) node.
6.  The [Binary(+)](file:///z:/kod/automatafpit/Parser.h#25-30) node becomes the Right child of the [Binary(*)](file:///z:/kod/automatafpit/Parser.h#25-30).

### 2.3 Evaluation
*   **Goal**: Calculate the result by traversing the AST.
*   **Process**: A recursive function visits each node.
    *   If [NumberNode](file:///z:/kod/automatafpit/Parser.h#14-18): Return value.
    *   If [BinaryNode](file:///z:/kod/automatafpit/Parser.h#25-30): Recursively evaluate Left and Right children, then apply the operator.
*   **Code**: [evalAST()](file:///z:/kod/automatafpit/Parser.h#102-123) in [Parser.h](file:///z:/kod/automatafpit/Parser.h).

---

## 3. Regex Engine (Automata Theory)
The Regex mode demonstrates how text patterns are matched using **Non-Deterministic Finite Automata (NFA)**.

### 3.1 Preprocessing & Postfix Conversion
*   **Explicit Concatenation**: Standard regex writes `ab` for "a then b". The engine first inserts an explicit `.` operator: `a.b`.
*   **Shunting-Yard Algorithm**: Converts the infix regex (e.g., `a|b`) into **Postfix Notation** (e.g., `ab|`) to make it easier to build the machine.
    *   *Example*: [(a|b)*c](file:///z:/kod/automatafpit/main.cpp#17-152) -> `ab|*c.`
*   **Code**: `ThompsonNFA::insertConcat` and `ThompsonNFA::toPostfix` in [NFA.h](file:///z:/kod/automatafpit/NFA.h).

### 3.2 Thompson's Construction (NFA Builder)
*   **Goal**: Turn the postfix string into a state machine.
*   **Method**: A stack-based approach where every operator constructs a small NFA fragment and pushes it back.
*   **Rules**:
    1.  **Symbol `a`**: Create `Start -a-> Accept`. Push to stack.
    2.  **Concatenation `.`**: Pop B, Pop A. Connect `A.Accept -eps-> B.Start`. Push new fragment `A.Start -> B.Accept`.
    3.  **Union `|`**: Pop B, Pop A. Create new Start `S` and Accept `E`. Connect `S -eps-> A.Start`, `S -eps-> B.Start`. Connect `A.Accept -eps-> E`, `B.Accept -eps-> E`. Push `S -> E`.
    4.  **Kleene Star `*`**: Pop A. Create `S` and `E`. Connect `S -eps-> A.Start`, `A.Accept -eps-> S` (Loop), `S -eps-> E` (Skip). Push `S -> E`.
*   **Code**: `ThompsonNFA::buildFromRegex` in [NFA.h](file:///z:/kod/automatafpit/NFA.h).

#### Example: `a|b` (Postfix: `ab|`)
1.  Read `a`: Stack = `[Frag(a)]`
2.  Read `b`: Stack = `[Frag(a), Frag(b)]`
3.  Read `|`:
    *   Pop [Frag(b)](file:///z:/kod/automatafpit/NFA.h#16-20), Pop [Frag(a)](file:///z:/kod/automatafpit/NFA.h#16-20).
    *   Create Split State `S`.
    *   Add edges: `S -> Start(a)`, `S -> Start(b)`.
    *   Stack = `[Frag(a|b)]`.

### 3.3 Simulation (The Matcher)
*   **Goal**: Check if a string matches the pattern.
*   **Method**: **NFA Simulation** (without converting to DFA).
    *   **Active States**: The engine tracks a *set* of states it is currently in.
    *   **Epsilon Closure**: If we are in state A, and A has an epsilon transition ([eps](file:///z:/kod/automatafpit/NFA.h#130-143)) to B, we are instantly in B as well. This is calculated recursively.
    *   **Step**: For each character in the input:
        1.  Find all transitions from current active states that match the character.
        2.  Compute the Epsilon Closure of those new states.
        3.  Update the set of active states.
    *   **Result**: If the final set of active states includes the **Accept State**, the string matches.
*   **Code**: `ThompsonNFA::simulate` and [epsilonClosure](file:///z:/kod/automatafpit/NFA.h#130-143) in [NFA.h](file:///z:/kod/automatafpit/NFA.h).

#### Detailed Trace: `a|b` matching "a"
1.  **Start**: Active = `{Start}`.
2.  **Initial Epsilon Closure**:
    *   `Start` has epsilons to `Start(a)` and `Start(b)`.
    *   Active Set becomes `{Start, Start(a), Start(b)}`.
3.  **Read 'a'**:
    *   Check transitions for `Start`: None for 'a'.
    *   Check transitions for `Start(a)`: Has `a` -> `Accept(a)`.
    *   Check transitions for `Start(b)`: None for 'a'.
    *   New States = `{Accept(a)}`.
4.  **Epsilon Closure (After Read)**:
    *   `Accept(a)` has epsilon to `FinalAccept`.
    *   Active Set becomes `{Accept(a), FinalAccept}`.
5.  **End**: Active Set contains `FinalAccept`. **MATCH!**

---

## 4. Code Structure Overview

| File | Responsibility | Key Classes/Functions |
| :--- | :--- | :--- |
| **[main.cpp](file:///z:/kod/automatafpit/main.cpp)** | **Application Entry & GUI** | [main()](file:///z:/kod/automatafpit/main.cpp#17-152): Sets up SFML window, ImGui loop, and handles user input. Calls the engines. |
| **[Lexer.h](file:///z:/kod/automatafpit/Lexer.h)** | **Tokenization** | [Lexer](file:///z:/kod/automatafpit/Lexer.h#22-23): Breaks string into [Token](file:///z:/kod/automatafpit/Lexer.h#8-13) vector. `TokenType` enum. |
| **[Parser.h](file:///z:/kod/automatafpit/Parser.h)** | **AST & Parsing** | [Parser](file:///z:/kod/automatafpit/Parser.h#31-101): Recursive descent logic. [ASTNode](file:///z:/kod/automatafpit/Parser.h#10-13), [BinaryNode](file:///z:/kod/automatafpit/Parser.h#25-30), [evalAST()](file:///z:/kod/automatafpit/Parser.h#102-123). |
| **[NFA.h](file:///z:/kod/automatafpit/NFA.h)** | **Regex Logic** | [ThompsonNFA](file:///z:/kod/automatafpit/NFA.h#21-172): Handles regex parsing, NFA construction, and simulation. [NState](file:///z:/kod/automatafpit/NFA.h#13-14) struct. |
| **[tests.cpp](file:///z:/kod/automatafpit/tests.cpp)** | **Verification** | Unit tests for both engines to ensure correctness. |
| **[CMakeLists.txt](file:///z:/kod/automatafpit/CMakeLists.txt)** | **Build System** | Configures the project, links SFML/ImGui, and defines executables (`recalc` and `tests`). |
