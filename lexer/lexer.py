import re


class LexicalError(Exception):
    def __init__(self, message, position):
        self.message = message
        self.position = position
        super().__init__(message)


TOKEN_TYPES = [
    ("LOAD", r"\bLOAD\b"),
    ("FILTER", r"\bFILTER\b"),
    ("AVERAGE", r"\bAVERAGE\b"),
    ("SORT", r"\bSORT\b"),
    ("DISPLAY", r"\bDISPLAY\b"),

    ("ASC", r"\bASC\b"),
    ("DESC", r"\bDESC\b"),

    ("OPERATOR", r">=|<=|==|!=|>|<"),

    ("STRING", r'"[^"]*"'),

    ("NUMBER", r"\d+(\.\d+)?"),

    ("IDENTIFIER", r"[a-zA-Z_][a-zA-Z0-9_]*"),

    ("NEWLINE", r"\n"),

    ("WHITESPACE", r"[ \t]+"),
]


class Lexer:

    def tokenize(self, code):

        tokens = []

        token_regex = "|".join(
            f"(?P<{token_type}>{pattern})"
            for token_type, pattern in TOKEN_TYPES
        )

        position = 0

        while position < len(code):

            match = re.match(
                token_regex,
                code[position:]
            )

            if match is None:

                invalid_character = code[position]

                raise LexicalError(
                    f"Invalid character '{invalid_character}'",
                    position
                )

            token_type = match.lastgroup
            value = match.group()

            if token_type != "WHITESPACE":
                tokens.append((token_type, value))

            position += len(value)

        return tokens