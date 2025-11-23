RECalc: Visual Arithmetic & Regex Engine Documentation
1. High-Level Overview
RECalc is an educational visualization tool that demonstrates two fundamental computer science concepts:

Compiler Design: How computers understand and evaluate mathematical expressions.
Automata Theory: How regular expressions are processed using Finite Automata.
The application provides a Graphical User Interface (GUI) built with SFML and ImGui to visualize the internal steps of these processes in real-time.

2. Arithmetic Engine (Compiler Theory)
The Arithmetic mode processes mathematical expressions (e.g., 3 * (4 + 2)) through a standard compiler pipeline: Lexing → Parsing → Evaluation.

2.1 Lexical Analysis (The Lexer)
Goal: Convert raw text into a stream of meaningful "Tokens".
Process: The 
Lexer
 scans the input string character by character.
It ignores whitespace.
It groups digits into TOK_NUMBER (e.g., "1", "2", "3" becomes 123).
It identifies symbols like +, -, *, /, (, ) and assigns them types (e.g., TOK_PLUS).
Code: 
Lexer.h
tokenizeAll()
: The main loop driving the scan.
nextTokenInternal()
: The logic to identify the next token type.
Example: 10 + 5
Input Char	Action	Token Generated
1	Start Number	...
0	Continue Number	TOK_NUMBER("10")
 	Skip Whitespace	-
+	Match Symbol	TOK_PLUS("+")
 	Skip Whitespace	-
5	Start Number	TOK_NUMBER("5")
2.2 Syntax Analysis (The Parser)
Goal: Organize tokens into a hierarchical structure called an Abstract Syntax Tree (AST) that respects order of operations (precedence).
Method: Recursive Descent Parsing.
Grammar:
Expression
 -> 
AddSub
AddSub
 -> 
MulDiv
 (+/- 
MulDiv
)*
MulDiv
 -> 
Unary
 (// 
Unary
)
Unary
 -> (+/-) 
Unary
 | 
Primary
Primary
 -> 
Number
 | ( 
Expression
 )
Code: 
Parser.h
Each grammar rule corresponds to a function (e.g., 
parseAddSub
, 
parseMulDiv
).
Example: 3 * (4 + 2)
The parser builds this tree structure:

Binary(*)      <-- Multiplication happens last (Root)
├── Number(3)  <-- Left Operand
└── Binary(+)  <-- Right Operand (Result of addition)
    ├── Number(4)
    └── Number(2)
How it works:

parseAddSub
 calls 
parseMulDiv
.
parseMulDiv
 sees 3 and *. It knows it needs a Right hand side.
It calls 
parseUnary
 -> 
parsePrimary
.
parsePrimary
 sees (. It recursively calls 
parseExpression
 for the inside.
Inside 
(4+2)
, 
parseAddSub
 builds the 
Binary(+)
 node.
The 
Binary(+)
 node becomes the Right child of the 
Binary(*)
.
2.3 Evaluation
Goal: Calculate the result by traversing the AST.
Process: A recursive function visits each node.
If 
NumberNode
: Return value.
If 
BinaryNode
: Recursively evaluate Left and Right children, then apply the operator.
Code: 
evalAST()
 in 
Parser.h
.
3. Regex Engine (Automata Theory)
The Regex mode demonstrates how text patterns are matched using Non-Deterministic Finite Automata (NFA).

3.1 Preprocessing & Postfix Conversion
Explicit Concatenation: Standard regex writes ab for "a then b". The engine first inserts an explicit . operator: a.b.
Shunting-Yard Algorithm: Converts the infix regex (e.g., a|b) into Postfix Notation (e.g., ab|) to make it easier to build the machine.
Example: 
(a|b)*c
 -> ab|*c.
Code: ThompsonNFA::insertConcat and ThompsonNFA::toPostfix in 
NFA.h
.
3.2 Thompson's Construction (NFA Builder)
Goal: Turn the postfix string into a state machine.
Method: A stack-based approach where every operator constructs a small NFA fragment and pushes it back.
Rules:
Symbol a: Create Start -a-> Accept. Push to stack.
Concatenation .: Pop B, Pop A. Connect A.Accept -eps-> B.Start. Push new fragment A.Start -> B.Accept.
Union |: Pop B, Pop A. Create new Start S and Accept E. Connect S -eps-> A.Start, S -eps-> B.Start. Connect A.Accept -eps-> E, B.Accept -eps-> E. Push S -> E.
Kleene Star *: Pop A. Create S and E. Connect S -eps-> A.Start, A.Accept -eps-> S (Loop), S -eps-> E (Skip). Push S -> E.
Code: ThompsonNFA::buildFromRegex in 
NFA.h
.
Example: a|b (Postfix: ab|)
Read a: Stack = [Frag(a)]
Read b: Stack = [Frag(a), Frag(b)]
Read |:
Pop 
Frag(b)
, Pop 
Frag(a)
.
Create Split State S.
Add edges: S -> Start(a), S -> Start(b).
Stack = [Frag(a|b)].
3.3 Simulation (The Matcher)
Goal: Check if a string matches the pattern.
Method: NFA Simulation (without converting to DFA).
Active States: The engine tracks a set of states it is currently in.
Epsilon Closure: If we are in state A, and A has an epsilon transition (
eps
) to B, we are instantly in B as well. This is calculated recursively.
Step: For each character in the input:
Find all transitions from current active states that match the character.
Compute the Epsilon Closure of those new states.
Update the set of active states.
Result: If the final set of active states includes the Accept State, the string matches.
Code: ThompsonNFA::simulate and 
epsilonClosure
 in 
NFA.h
.
Detailed Trace: a|b matching "a"
Start: Active = {Start}.
Initial Epsilon Closure:
Start has epsilons to Start(a) and Start(b).
Active Set becomes {Start, Start(a), Start(b)}.
Read 'a':
Check transitions for Start: None for 'a'.
Check transitions for Start(a): Has a -> Accept(a).
Check transitions for Start(b): None for 'a'.
New States = {Accept(a)}.
Epsilon Closure (After Read):
Accept(a) has epsilon to FinalAccept.
Active Set becomes {Accept(a), FinalAccept}.
End: Active Set contains FinalAccept. MATCH!
4. Code Structure Overview
File	Responsibility	Key Classes/Functions
main.cpp
Application Entry & GUI	
main()
: Sets up SFML window, ImGui loop, and handles user input. Calls the engines.
Lexer.h
Tokenization	
Lexer
: Breaks string into 
Token
 vector. TokenType enum.
Parser.h
AST & Parsing	
Parser
: Recursive descent logic. 
ASTNode
, 
BinaryNode
, 
evalAST()
.
NFA.h
Regex Logic	
ThompsonNFA
: Handles regex parsing, NFA construction, and simulation. 
NState
 struct.
tests.cpp
Verification	Unit tests for both engines to ensure correctness.
CMakeLists.txt
Build System	Configures the project, links SFML/ImGui, and defines executables (recalc and tests).
