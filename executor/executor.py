import csv
import os


class Executor:

    def __init__(self, ast, base_directory):

        self.ast = ast
        self.base_directory = base_directory
        self.data = []
        self.average_result = None

    def execute(self):

        for statement in self.ast:

            command = statement["type"]

            if command == "LOAD":
                self.execute_load(statement)

            elif command == "FILTER":
                self.execute_filter(statement)

            elif command == "AVERAGE":
                self.execute_average(statement)

            elif command == "SORT":
                self.execute_sort(statement)

            elif command == "DISPLAY":
                self.execute_display()

    def execute_load(self, statement):

        filename = statement["filename"]

        file_path = os.path.join(
            self.base_directory,
            filename
        )

        with open(
            file_path,
            "r",
            encoding="utf-8"
        ) as file:

            reader = csv.DictReader(file)

            self.data = list(reader)

    def execute_filter(self, statement):

        column = statement["column"]
        operator = statement["operator"]
        value = statement["value"]

        filtered_data = []

        for row in self.data:

            cell_value = row[column]

            try:
                cell_value = float(cell_value)
                compare_value = float(value)

            except ValueError:

                compare_value = value.strip('"')

            result = False

            if operator == ">":
                result = cell_value > compare_value

            elif operator == "<":
                result = cell_value < compare_value

            elif operator == ">=":
                result = cell_value >= compare_value

            elif operator == "<=":
                result = cell_value <= compare_value

            elif operator == "==":
                result = cell_value == compare_value

            elif operator == "!=":
                result = cell_value != compare_value

            if result:
                filtered_data.append(row)

        self.data = filtered_data

    def execute_average(self, statement):

        column = statement["column"]

        values = [
            float(row[column])
            for row in self.data
        ]

        if values:
            self.average_result = sum(values) / len(values)

        else:
            self.average_result = 0

    def execute_sort(self, statement):

        column = statement["column"]

        reverse = statement["order"] == "DESC"

        try:

            self.data.sort(
                key=lambda row: float(row[column]),
                reverse=reverse
            )

        except ValueError:

            self.data.sort(
                key=lambda row: row[column],
                reverse=reverse
            )

    def execute_display(self):

        print("\n===== EXECUTION RESULT =====\n")

        if self.average_result is not None:

            print(
                f"Average = "
                f"{self.average_result:.2f}\n"
            )

        if not self.data:

            print("No data to display.")

            return

        columns = self.data[0].keys()

        print(" | ".join(columns))

        print("-" * 50)

        for row in self.data:

            print(
                " | ".join(
                    str(row[column])
                    for column in columns
                )
            )