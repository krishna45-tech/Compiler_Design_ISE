class Parser:

    def __init__(self, tokens):

        self.tokens = tokens
        self.position = 0
        self.current_line = 1

    def current_token(self):

        if self.position < len(self.tokens):
            return self.tokens[self.position]

        return None

    def advance(self):

        token = self.current_token()

        if token and token[0] == "NEWLINE":
            self.current_line += 1

        self.position += 1

        return token

    def expect(self, token_type):

        token = self.current_token()

        if token is None:
            raise SyntaxError(
                f"Expected {token_type} on line {self.current_line}"
            )

        if token[0] != token_type:
            raise SyntaxError(
                f"Expected {token_type}, "
                f"got {token[0]} on line {self.current_line}"
            )

        self.advance()

        return token[1]

    def parse(self):

        ast = []

        while self.current_token() is not None:

            if self.current_token()[0] == "NEWLINE":
                self.advance()
                continue

            statement = self.parse_statement()

            ast.append(statement)

            if (
                self.current_token() is not None
                and self.current_token()[0] == "NEWLINE"
            ):
                self.advance()

        return ast

    def parse_statement(self):

        token = self.current_token()

        if token is None:
            return None

        token_type = token[0]

        if token_type == "LOAD":
            return self.parse_load()

        elif token_type == "FILTER":
            return self.parse_filter()

        elif token_type == "AVERAGE":
            return self.parse_average()

        elif token_type == "SORT":
            return self.parse_sort()

        elif token_type == "DISPLAY":
            return self.parse_display()

        raise SyntaxError(
            f"Unexpected command '{token[1]}' "
            f"on line {self.current_line}"
        )

    def parse_load(self):

        self.expect("LOAD")

        filename = self.expect("STRING")

        return {
            "type": "LOAD",
            "filename": filename.strip('"'),
            "line": self.current_line
        }

    def parse_filter(self):

        self.expect("FILTER")

        column = self.expect("IDENTIFIER")

        operator = self.expect("OPERATOR")

        token = self.current_token()

        if token is None or token[0] not in ("NUMBER", "STRING"):

            raise SyntaxError(
                f"FILTER expects a number or string value "
                f"on line {self.current_line}"
            )

        value = token[1]

        self.advance()

        return {
            "type": "FILTER",
            "column": column,
            "operator": operator,
            "value": value,
            "line": self.current_line
        }

    def parse_average(self):

        self.expect("AVERAGE")

        column = self.expect("IDENTIFIER")

        return {
            "type": "AVERAGE",
            "column": column,
            "line": self.current_line
        }

    def parse_sort(self):

        self.expect("SORT")

        column = self.expect("IDENTIFIER")

        token = self.current_token()

        if token is None or token[0] not in ("ASC", "DESC"):

            raise SyntaxError(
                f"SORT expects ASC or DESC "
                f"on line {self.current_line}"
            )

        order = token[1]

        self.advance()

        return {
            "type": "SORT",
            "column": column,
            "order": order,
            "line": self.current_line
        }

    def parse_display(self):

        self.expect("DISPLAY")

        return {
            "type": "DISPLAY",
            "line": self.current_line
        }