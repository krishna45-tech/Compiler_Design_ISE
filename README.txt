DataLang simple end-to-end compiler

Compile:
  g++ -std=c++17 compiler.cpp -o compiler

Run:
  ./compiler

Paste DataLang commands, then type END on its own line.

Example:
LOAD "students.csv"
SELECT name, age, marks
FILTER marks > 75
SORT marks DESC
DISPLAY
SAVE "result.csv"
END

The program runs six basic phases in sequence: lexer, parser, semantic analysis,
intermediate representation, optimization, and Python/Pandas code generation.

Notes:
- The semantic analyzer uses an example schema: name, age, marks, department.
- FILTER currently supports a single comparison (e.g. marks > 75).
- The optimizer only removes repeated identical consecutive SORT instructions.
- Generated Python requires pandas and the input CSV file to exist.
