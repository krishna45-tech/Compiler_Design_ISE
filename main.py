import os

from lexer.lexer import Lexer, LexicalError
from parser.parser import Parser
from semantic.semantic_analyzer import (
    SemanticAnalyzer,
    SemanticError
)
from executor.executor import Executor


BASE_DIR = os.path.dirname(
    os.path.abspath(__file__)
)

EXAMPLES_DIR = os.path.join(
    BASE_DIR,
    "examples"
)

PROGRAM_PATH = os.path.join(
    EXAMPLES_DIR,
    "program.txt"
)


def print_header():

    print("\n" + "=" * 55)
    print("        DATA LANGUAGE COMPILER v1.0")
    print("=" * 55)


def show_lexical_error(code, error):

    print("\n[LEXICAL ERROR]")
    print(f"✗ {error.message}")

    lines = code.splitlines()

    position = error.position

    current_position = 0

    for line_number, line in enumerate(lines, start=1):

        line_length = len(line) + 1

        if position < current_position + line_length:

            column = position - current_position

            print(f"\nLine {line_number}:")
            print(line)
            print(" " * column + "^")

            break

        current_position += line_length


def main():

    print_header()

    # Check program file
    if not os.path.exists(PROGRAM_PATH):

        print(
            f"\nError: program.txt file not found at:\n"
            f"{PROGRAM_PATH}"
        )

        return

    # Read source program
    with open(
        PROGRAM_PATH,
        "r",
        encoding="utf-8"
    ) as file:

        code = file.read()

    # =================================================
    # PHASE 1 — LEXICAL ANALYSIS
    # =================================================

    print("\n[PHASE 1] LEXICAL ANALYSIS")
    print("-" * 55)

    try:

        lexer = Lexer()

        tokens = lexer.tokenize(code)

        print("✓ Tokenization successful")

        print(
            f"✓ Total tokens generated: "
            f"{len(tokens)}"
        )

    except LexicalError as error:

        show_lexical_error(code, error)

        return

    # =================================================
    # PHASE 2 — SYNTAX ANALYSIS
    # =================================================

    print("\n[PHASE 2] SYNTAX ANALYSIS")
    print("-" * 55)

    try:

        parser = Parser(tokens)

        ast = parser.parse()

        print("✓ Program syntax is valid")

        print(
            f"✓ AST generated with "
            f"{len(ast)} statements"
        )

    except SyntaxError as error:

        print("\n[SYNTAX ERROR]")
        print(f"✗ {error}")

        return

    # =================================================
    # PHASE 3 — SEMANTIC ANALYSIS
    # =================================================

    print("\n[PHASE 3] SEMANTIC ANALYSIS")
    print("-" * 55)

    try:

        analyzer = SemanticAnalyzer(
            ast,
            EXAMPLES_DIR
        )

        analyzer.analyze()

        print("✓ Semantic validation successful")

    except SemanticError as error:

        print("\n[SEMANTIC ERROR]")
        print(f"✗ {error.message}")

        lines = code.splitlines()

        if error.line <= len(lines):

            print(
                f"\nLine {error.line}:"
            )

            print(
                lines[error.line - 1]
            )

            print("^")

        return

    # =================================================
    # PHASE 4 — EXECUTION
    # =================================================

    print("\n[PHASE 4] EXECUTION")
    print("-" * 55)

    executor = Executor(
        ast,
        EXAMPLES_DIR
    )

    executor.execute()

    # =================================================
    # SUCCESS
    # =================================================

    print("\n" + "=" * 55)
    print("         ✓ COMPILATION SUCCESSFUL")
    print("=" * 55)


if __name__ == "__main__":
    main()