DataLang compiler - essential improvements

Compile on Windows with MinGW g++:
  g++ -std=c++17 compiler.cpp -o compiler.exe
  .\compiler.exe

Enter DataLang commands, then type END on a separate line.

Example:
LOAD "students.csv"
SELECT name, marks
FILTER marks > 75
SORT marks DESC
DISPLAY
END

Implemented phases:
1. Lexer: produces tokens with type, lexeme, line, and column.
2. Parser: consumes tokens, checks basic command grammar, and builds a simple AST.
3. Semantic analyzer: receives the AST, checks LOAD ordering and column references using a predefined symbol table.
4. IR generation: converts AST statement nodes to IR.
5. Optimizer: removes duplicate consecutive identical SORT instructions.
6. Code generator: emits Python/Pandas code.

The symbol table is an example schema (name, age, marks, department). Update it for your dataset.
The generated Python code is printed; the C++ compiler does not execute it.
