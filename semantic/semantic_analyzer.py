import csv
import os


class SemanticError(Exception):

    def __init__(self, message, line):

        self.message = message
        self.line = line

        super().__init__(message)


class SemanticAnalyzer:

    def __init__(self, ast, base_directory):

        self.ast = ast
        self.base_directory = base_directory
        self.schema = {}
        self.dataset_loaded = False

    def analyze(self):

        for statement in self.ast:

            statement_type = statement["type"]

            if statement_type == "LOAD":
                self.analyze_load(statement)

            elif statement_type == "FILTER":
                self.analyze_filter(statement)

            elif statement_type == "AVERAGE":
                self.analyze_average(statement)

            elif statement_type == "SORT":
                self.analyze_sort(statement)

            elif statement_type == "DISPLAY":

                if not self.dataset_loaded:
                    raise SemanticError(
                        "DISPLAY cannot be used before LOAD",
                        statement["line"]
                    )

        return True

    def analyze_load(self, statement):

        filename = statement["filename"]

        file_path = os.path.join(
            self.base_directory,
            filename
        )

        if not os.path.exists(file_path):

            raise SemanticError(
                f"Dataset '{filename}' does not exist.",
                statement["line"]
            )

        self.schema = self.detect_schema(file_path)

        self.dataset_loaded = True

        print(f"Dataset loaded: {filename}")
        print(f"Detected schema: {self.schema}")

    def detect_schema(self, file_path):

        schema = {}

        with open(
            file_path,
            "r",
            newline="",
            encoding="utf-8"
        ) as file:

            reader = csv.DictReader(file)

            rows = list(reader)

            if not rows:
                return schema

            for column in reader.fieldnames:

                is_numeric = True

                for row in rows:

                    value = row[column]

                    try:
                        float(value)

                    except ValueError:
                        is_numeric = False
                        break

                if is_numeric:
                    schema[column] = "NUMBER"

                else:
                    schema[column] = "STRING"

        return schema

    def check_column(self, column, line):

        if not self.dataset_loaded:

            raise SemanticError(
                "A dataset must be loaded before using columns.",
                line
            )

        if column not in self.schema:

            raise SemanticError(
                f"Column '{column}' does not exist in the dataset.",
                line
            )

    def analyze_filter(self, statement):

        column = statement["column"]

        self.check_column(
            column,
            statement["line"]
        )

    def analyze_average(self, statement):

        column = statement["column"]

        self.check_column(
            column,
            statement["line"]
        )

        if self.schema[column] != "NUMBER":

            raise SemanticError(
                f"AVERAGE can only be used with numeric columns. "
                f"'{column}' is {self.schema[column]}",
                statement["line"]
            )

    def analyze_sort(self, statement):

        column = statement["column"]

        self.check_column(
            column,
            statement["line"]
        )