#include <iostream>
#include <string>
#include <unordered_set>
#include <cctype>

using namespace std;

// ---------------- TOKEN TYPES ----------------

enum class TokenType {
    KEYWORD,
    FUNCTION,
    IDENTIFIER,
    INTEGER,
    FLOAT,
    STRING,
    OPERATOR,
    COMMA,
    LPAREN,
    RPAREN,
    EOF_TOKEN,
    ERROR
};

// ---------------- TOKEN ----------------

struct Token {
    TokenType type;
    string lexeme;
    int line;
    int column;
};

// ---------------- TOKEN NAME ----------------

string tokenName(TokenType type) {

    switch (type) {

        case TokenType::KEYWORD:    return "KEYWORD";
        case TokenType::FUNCTION:   return "FUNCTION";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::INTEGER:    return "INTEGER";
        case TokenType::FLOAT:      return "FLOAT";
        case TokenType::STRING:     return "STRING";
        case TokenType::OPERATOR:   return "OPERATOR";
        case TokenType::COMMA:      return "COMMA";
        case TokenType::LPAREN:     return "LPAREN";
        case TokenType::RPAREN:     return "RPAREN";
        case TokenType::EOF_TOKEN:  return "EOF";
        case TokenType::ERROR:      return "ERROR";
    }

    return "UNKNOWN";
}

// ---------------- LEXER ----------------

class Lexer {

private:

    string source;
    size_t pos;
    int line;
    int column;

    unordered_set<string> keywords = {
        "LOAD",
        "SELECT",
        "FILTER",
        "SORT",
        "GROUP",
        "CALCULATE",
        "DISPLAY",
        "SAVE",
        "ASC",
        "DESC",
        "AND",
        "OR",
        "NOT",
        "TRUE",
        "FALSE"
    };

    unordered_set<string> functions = {
        "AVG",
        "SUM",
        "MIN",
        "MAX",
        "COUNT"
    };

public:

    Lexer(const string& input) {

        source = input;
        pos = 0;
        line = 1;
        column = 1;
    }

    // ---------------- CURRENT CHARACTER ----------------

    char currentChar() {

        if (pos >= source.length())
            return '\0';

        return source[pos];
    }

    // ---------------- NEXT CHARACTER ----------------

    char peek() {

        if (pos + 1 >= source.length())
            return '\0';

        return source[pos + 1];
    }

    // ---------------- ADVANCE ----------------

    void advance() {

        if (currentChar() == '\n') {
            line++;
            column = 1;
        }
        else {
            column++;
        }

        pos++;
    }

    // ---------------- SKIP WHITESPACE ----------------

    void skipWhitespace() {

        while (currentChar() != '\0' &&
               isspace(static_cast<unsigned char>(currentChar()))) {

            advance();
        }
    }

    // ---------------- SKIP COMMENT ----------------

    void skipComment() {

        while (currentChar() != '\0' &&
               currentChar() != '\n') {

            advance();
        }
    }

    // ---------------- READ WORD ----------------

    Token readWord() {

        int startLine = line;
        int startColumn = column;

        string word;

        while (isalnum(static_cast<unsigned char>(currentChar())) ||
               currentChar() == '_') {

            word += currentChar();
            advance();
        }

        // Check invalid identifier such as 123abc
        if (isdigit(static_cast<unsigned char>(word[0]))) {

            return {
                TokenType::ERROR,
                "Invalid identifier: " + word,
                startLine,
                startColumn
            };
        }

        if (keywords.count(word)) {

            return {
                TokenType::KEYWORD,
                word,
                startLine,
                startColumn
            };
        }

        if (functions.count(word)) {

            return {
                TokenType::FUNCTION,
                word,
                startLine,
                startColumn
            };
        }

        return {
            TokenType::IDENTIFIER,
            word,
            startLine,
            startColumn
        };
    }

    // ---------------- READ NUMBER ----------------

    Token readNumber() {

        int startLine = line;
        int startColumn = column;

        string number;
        bool hasDot = false;

        while (isdigit(static_cast<unsigned char>(currentChar())) ||
               currentChar() == '.') {

            if (currentChar() == '.') {

                if (hasDot) {

                    // Consume remaining numeric characters
                    while (isdigit(static_cast<unsigned char>(currentChar())) ||
                           currentChar() == '.') {

                        number += currentChar();
                        advance();
                    }

                    return {
                        TokenType::ERROR,
                        "Invalid number: " + number,
                        startLine,
                        startColumn
                    };
                }

                hasDot = true;
            }

            number += currentChar();
            advance();
        }

        // Check for something like 123abc
        if (isalpha(static_cast<unsigned char>(currentChar())) ||
            currentChar() == '_') {

            while (isalnum(static_cast<unsigned char>(currentChar())) ||
                   currentChar() == '_') {

                number += currentChar();
                advance();
            }

            return {
                TokenType::ERROR,
                "Invalid identifier: " + number,
                startLine,
                startColumn
            };
        }

        // "." by itself or "123." is considered invalid
        if (hasDot && number.back() == '.') {

            return {
                TokenType::ERROR,
                "Invalid floating-point number: " + number,
                startLine,
                startColumn
            };
        }

        if (hasDot) {

            return {
                TokenType::FLOAT,
                number,
                startLine,
                startColumn
            };
        }

        return {
            TokenType::INTEGER,
            number,
            startLine,
            startColumn
        };
    }

    // ---------------- READ STRING ----------------

    Token readString() {

        int startLine = line;
        int startColumn = column;

        string str;

        // Skip opening quote
        advance();

        while (currentChar() != '\0' &&
               currentChar() != '"') {

            if (currentChar() == '\n') {

                return {
                    TokenType::ERROR,
                    "Unterminated string",
                    startLine,
                    startColumn
                };
            }

            str += currentChar();
            advance();
        }

        if (currentChar() == '\0') {

            return {
                TokenType::ERROR,
                "Unterminated string",
                startLine,
                startColumn
            };
        }

        // Skip closing quote
        advance();

        return {
            TokenType::STRING,
            str,
            startLine,
            startColumn
        };
    }

    // ---------------- READ OPERATOR ----------------

    Token readOperator() {

        int startLine = line;
        int startColumn = column;

        char first = currentChar();
        char second = peek();

        // Two-character operators

        if ((first == '>' && second == '=') ||
            (first == '<' && second == '=') ||
            (first == '=' && second == '=') ||
            (first == '!' && second == '=')) {

            string op;

            op += first;
            advance();

            op += currentChar();
            advance();

            return {
                TokenType::OPERATOR,
                op,
                startLine,
                startColumn
            };
        }

        // Single-character operators

        if (first == '>' ||
            first == '<') {

            advance();

            return {
                TokenType::OPERATOR,
                string(1, first),
                startLine,
                startColumn
            };
        }

        // Single '=' is not supported
        if (first == '=') {

            advance();

            return {
                TokenType::ERROR,
                "Invalid operator '='. Use '==' for comparison.",
                startLine,
                startColumn
            };
        }

        // Single '!' is not supported
        if (first == '!') {

            advance();

            return {
                TokenType::ERROR,
                "Invalid operator '!'. Use '!='.",
                startLine,
                startColumn
            };
        }

        return {
            TokenType::ERROR,
            "Invalid operator",
            startLine,
            startColumn
        };
    }

    // ---------------- NEXT TOKEN ----------------

    Token getNextToken() {

        while (true) {

            skipWhitespace();

            int startLine = line;
            int startColumn = column;

            char ch = currentChar();

            // EOF
            if (ch == '\0') {

                return {
                    TokenType::EOF_TOKEN,
                    "",
                    startLine,
                    startColumn
                };
            }

            // Comment
            if (ch == '#') {

                skipComment();
                continue;
            }

            // Word / identifier / keyword / function
            if (isalpha(static_cast<unsigned char>(ch)) ||
                ch == '_') {

                return readWord();
            }

            // Number
            if (isdigit(static_cast<unsigned char>(ch))) {

                return readNumber();
            }

            // String
            if (ch == '"') {

                return readString();
            }

            // Operators
            if (ch == '>' ||
                ch == '<' ||
                ch == '=' ||
                ch == '!') {

                return readOperator();
            }

            // Comma
            if (ch == ',') {

                advance();

                return {
                    TokenType::COMMA,
                    ",",
                    startLine,
                    startColumn
                };
            }

            // Left parenthesis
            if (ch == '(') {

                advance();

                return {
                    TokenType::LPAREN,
                    "(",
                    startLine,
                    startColumn
                };
            }

            // Right parenthesis
            if (ch == ')') {

                advance();

                return {
                    TokenType::RPAREN,
                    ")",
                    startLine,
                    startColumn
                };
            }

            // Invalid character
            string error = "Invalid character: ";
            error += ch;

            advance();

            return {
                TokenType::ERROR,
                error,
                startLine,
                startColumn
            };
        }
    }
};

// ---------------- MAIN ----------------

int main() {

    string source;
    string inputLine;

    cout << "Enter DataLang program.\n";
    cout << "Enter END on a separate line to finish.\n\n";

    while (getline(cin, inputLine)) {

        if (inputLine == "END")
            break;

        source += inputLine;
        source += '\n';
    }

    Lexer lexer(source);

    cout << "\n========== TOKEN STREAM ==========\n\n";

    while (true) {

        Token token = lexer.getNextToken();

        cout << "Line " << token.line
             << ", Column " << token.column
             << " -> "
             << tokenName(token.type);

        if (!token.lexeme.empty()) {

            cout << " : " << token.lexeme;
        }

        cout << '\n';

        if (token.type == TokenType::ERROR) {

            cout << "\nLexical analysis failed.\n";
            break;
        }

        if (token.type == TokenType::EOF_TOKEN) {

            cout << "\nLexical analysis completed successfully.\n";
            break;
        }
    }

    return 0;
}