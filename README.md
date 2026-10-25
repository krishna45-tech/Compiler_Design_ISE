# Data Language Compiler

A simple compiler implementation for a custom Data Language built using Python.

## Features

The compiler performs the following phases:

1. **Lexical Analysis** – Converts the input program into tokens.
2. **Syntax Analysis** – Validates the grammar and generates an Abstract Syntax Tree (AST).
3. **Semantic Analysis** – Checks the logical correctness of commands and data operations.
4. **Execution** – Executes valid data operations on the dataset.

## Supported Commands

### LOAD

Loads a CSV dataset.

```text
LOAD "students.csv"