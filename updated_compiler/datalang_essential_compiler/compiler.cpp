#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <set>
#include <algorithm>
#include <cctype>
using namespace std;

struct Token
{
    string type, lexeme;
    int line, column;
};

// A small AST: Program -> Statement nodes -> argument/identifier nodes.
struct ASTNode
{
    string type; // PROGRAM, LOAD, SELECT, FILTER, IDENTIFIER, STRING, NUMBER, OPERATOR, etc.
    string value;
    string original; // original source line for statement nodes
    int line;
    vector<ASTNode> children;
};

struct IR
{
    string operation, argument;
};
struct Symbol
{
    string name, type;
};

string trim(string s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
string upper(string s)
{
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
              { return (char)toupper(c); });
    return s;
}
string lower(string s)
{
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
              { return (char)tolower(c); });
    return s;
}
string unquote(string s)
{
    s = trim(s);
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.substr(1, s.size() - 2);
    return s;
}
vector<string> splitComma(const string &s)
{
    vector<string> out;
    string item;
    stringstream ss(s);
    while (getline(ss, item, ','))
        out.push_back(trim(item));
    return out;
}
bool isKeyword(const string &s)
{
    static const set<string> words = {"LOAD", "SELECT", "FILTER", "SORT", "GROUP", "CALCULATE", "DISPLAY", "SAVE", "ASC", "DESC"};
    return words.count(upper(s)) > 0;
}
bool isIdentifier(const Token &t) { return t.type == "IDENTIFIER"; }

// PHASE 1: Lexer generates a token stream.
bool lexerPhase(const vector<string> &lines, vector<Token> &tokens)
{
    cout << "\n========== PHASE 1: LEXER ==========\n";
    for (size_t i = 0; i < lines.size(); ++i)
    {
        const string &s = lines[i];
        size_t j = 0;
        while (j < s.size())
        {
            unsigned char ch = (unsigned char)s[j];
            if (isspace(ch))
            {
                ++j;
                continue;
            }
            if (s[j] == '#')
                break;
            int col = (int)j + 1;
            if (s[j] == '"')
            {
                size_t start = j++;
                bool closed = false;
                while (j < s.size())
                {
                    if (s[j] == '"')
                    {
                        ++j;
                        closed = true;
                        break;
                    }
                    ++j;
                }
                if (!closed)
                {
                    cout << "Lexical error at line " << i + 1 << ", column " << col << ": unterminated string.\n";
                    return false;
                }
                tokens.push_back({"STRING", s.substr(start, j - start), (int)i + 1, col});
                continue;
            }
            if (isalpha(ch) || s[j] == '_')
            {
                size_t start = j++;
                while (j < s.size() && (isalnum((unsigned char)s[j]) || s[j] == '_'))
                    ++j;
                string word = s.substr(start, j - start);
                tokens.push_back({isKeyword(word) ? upper(word) : "IDENTIFIER", word, (int)i + 1, col});
                continue;
            }
            if (isdigit(ch))
            {
                size_t start = j++;
                while (j < s.size() && isdigit((unsigned char)s[j]))
                    ++j;
                if (j < s.size() && s[j] == '.' && j + 1 < s.size() && isdigit((unsigned char)s[j + 1]))
                {
                    ++j;
                    while (j < s.size() && isdigit((unsigned char)s[j]))
                        ++j;
                }
                tokens.push_back({"NUMBER", s.substr(start, j - start), (int)i + 1, col});
                continue;
            }
            string op(1, s[j]);
            if (j + 1 < s.size())
            {
                string two = s.substr(j, 2);
                if (two == ">=" || two == "<=" || two == "==" || two == "!=")
                {
                    op = two;
                    ++j;
                }
            }
            if (op == ",")
                tokens.push_back({"COMMA", op, (int)i + 1, col});
            else if (op == "(")
                tokens.push_back({"LPAREN", op, (int)i + 1, col});
            else if (op == ")")
                tokens.push_back({"RPAREN", op, (int)i + 1, col});
            else if (op == ">" || op == "<" || op == ">=" || op == "<=" || op == "==" || op == "!=" || op == "=" || op == "+" || op == "-" || op == "*" || op == "/")
                tokens.push_back({"OPERATOR", op, (int)i + 1, col});
            else
            {
                cout << "Lexical error at line " << i + 1 << ", column " << col << ": invalid character '" << s[j] << "'.\n";
                return false;
            }
            ++j;
        }
    }
    cout << "Generated token stream:\n";
    for (const auto &t : tokens)
        cout << "  Line " << t.line << ": <" << t.type << ", \"" << t.lexeme << "\">\n";
    cout << "Lexing successful. Total tokens: " << tokens.size() << ".\n";
    return true;
}

// PHASE 2: Parser consumes tokens and builds a simple AST.
bool parserPhase(const vector<string> &lines, const vector<Token> &tokens, ASTNode &ast)
{
    cout << "\n========== PHASE 2: PARSER ==========\n";
    ast = {"PROGRAM", "Program", "", 0, {}};
    size_t pos = 0;
    while (pos < tokens.size())
    {
        size_t start = pos;
        int lineNo = tokens[pos].line;
        while (pos < tokens.size() && tokens[pos].line == lineNo)
            ++pos;
        vector<Token> row(tokens.begin() + start, tokens.begin() + pos);
        if (row.empty())
            continue;
        string cmd = upper(row[0].lexeme);
        bool valid = true;
        auto fail = [&](const string &msg)
        { cout << "Syntax error at line " << lineNo << ": " << msg << "\n"; valid=false; };

        if (cmd == "LOAD" || cmd == "SAVE")
        {
            if (row.size() != 2 || row[1].type != "STRING")
                fail(cmd + " expects exactly one quoted filename.");
        }
        else if (cmd == "SELECT")
        {
            if (row.size() < 2)
                fail("SELECT expects one or more column names.");
            else
            {
                bool expectName = true;
                for (size_t k = 1; k < row.size(); ++k)
                {
                    if (expectName)
                    {
                        if (!(isIdentifier(row[k]) || row[k].lexeme == "*"))
                        {
                            fail("expected a column name.");
                            break;
                        }
                    }
                    else if (row[k].type != "COMMA")
                    {
                        fail("expected a comma between column names.");
                        break;
                    }
                    expectName = !expectName;
                }
                if (valid && expectName)
                    fail("SELECT cannot end with a comma.");
            }
        }
        else if (cmd == "FILTER")
        {
            if (row.size() != 4 || !isIdentifier(row[1]) || row[2].type != "OPERATOR" || !(row[3].type == "NUMBER" || row[3].type == "STRING" || isIdentifier(row[3])))
                fail("FILTER syntax: FILTER column comparison_operator value (e.g. FILTER marks > 75).");
            else if (set<string>{">", "<", ">=", "<=", "==", "!="}.count(row[2].lexeme) == 0)
                fail("FILTER requires >, <, >=, <=, ==, or !=.");
        }
        else if (cmd == "SORT")
        {
            if (row.size() != 3 || !isIdentifier(row[1]) || !(upper(row[2].lexeme) == "ASC" || upper(row[2].lexeme) == "DESC"))
                fail("SORT syntax: SORT column ASC|DESC.");
        }
        else if (cmd == "GROUP")
        {
            if (row.size() != 2 || !isIdentifier(row[1]))
                fail("GROUP expects one column name.");
        }
        else if (cmd == "CALCULATE")
        {
            if (row.size() != 5 || !isIdentifier(row[1]) || row[2].type != "LPAREN" || !isIdentifier(row[3]) || row[4].type != "RPAREN")
                fail("CALCULATE syntax: CALCULATE AVG(column), SUM(column), MIN(column), MAX(column), or COUNT(column).");
            else
            {
                string fn = upper(row[1].lexeme);
                if (fn != "AVG" && fn != "SUM" && fn != "MIN" && fn != "MAX" && fn != "COUNT")
                    fail("unsupported aggregate '" + row[1].lexeme + "'.");
            }
        }
        else if (cmd == "DISPLAY")
        {
            if (row.size() != 1)
                fail("DISPLAY takes no arguments.");
        }
        else
            fail("unknown command '" + row[0].lexeme + "'.");

        if (!valid)
            return false;
        string original = lines[lineNo - 1];
        size_t comment = original.find('#');
        if (comment != string::npos)
            original = original.substr(0, comment);
        original = trim(original);
        size_t commandEnd = row[0].column - 1 + row[0].lexeme.size();
        string rest = commandEnd < original.size() ? trim(original.substr(commandEnd)) : "";

        ASTNode statement = {cmd, rest, original, lineNo, {}};
        // Keep argument tokens as child nodes so semantic analysis can inspect the AST.
        for (size_t k = 1; k < row.size(); ++k)
        {
            string type = row[k].type;
            if (cmd == "SELECT" && type == "OPERATOR" && row[k].lexeme == "*")
                type = "WILDCARD";
            statement.children.push_back({type, row[k].lexeme, "", row[k].line, {}});
        }
        ast.children.push_back(statement);
        cout << "Built AST node: " << cmd << " (line " << lineNo << ")\n";
    }
    if (ast.children.empty())
    {
        cout << "Syntax error: no commands entered.\n";
        return false;
    }
    cout << "Parsing successful. AST contains " << ast.children.size() << " statement nodes.\n";
    cout << "AST:\n  PROGRAM\n";
    for (const auto &node : ast.children)
    {
        cout << "    " << node.type;
        if (!node.value.empty())
            cout << " [" << node.value << "]";
        cout << "\n";
        for (const auto &child : node.children)
            cout << "      " << child.type << " [" << child.value << "]\n";
    }
    return true;
}

// PHASE 3: Semantic analysis consumes the AST and uses a simple symbol table.
bool semanticPhase(const ASTNode &ast, map<string, Symbol> &symbolTable)
{
    cout << "\n========== PHASE 3: SEMANTIC ANALYZER ==========\n";
    symbolTable = {{"name", {"name", "string"}}, {"age", {"age", "integer"}}, {"marks", {"marks", "number"}}, {"department", {"department", "string"}}};
    cout << "Symbol table (example dataset schema):\n";
    for (const auto &entry : symbolTable)
        cout << "  " << entry.first << " : " << entry.second.type << "\n";

    int loadCount = 0;
    for (size_t i = 0; i < ast.children.size(); ++i)
    {
        const ASTNode &node = ast.children[i];
        if (node.type == "LOAD")
        {
            ++loadCount;
            if (i != 0 || loadCount > 1)
            {
                cout << "Semantic error at line " << node.line << ": LOAD must appear exactly once, as the first command.\n";
                return false;
            }
            continue;
        }
        if (loadCount == 0)
        {
            cout << "Semantic error at line " << node.line << ": LOAD must be the first command.\n";
            return false;
        }
        for (size_t c = 0; c < node.children.size(); ++c)
        {
            const ASTNode &child = node.children[c];
            // Check only positions that represent column names, not FILTER values or aggregate names.
            bool isColumn = false;
            if (node.type == "SELECT")
                isColumn = (child.type == "IDENTIFIER");
            else if (node.type == "FILTER" || node.type == "SORT" || node.type == "GROUP")
                isColumn = (c == 0 && child.type == "IDENTIFIER");
            else if (node.type == "CALCULATE")
                isColumn = (c == 2 && child.type == "IDENTIFIER");
            if (isColumn && symbolTable.find(child.value) == symbolTable.end())
            {
                cout << "Semantic error at line " << node.line << ": unknown column '" << child.value << "'.\n";
                return false;
            }
        }
    }
    if (loadCount != 1)
    {
        cout << "Semantic error: exactly one LOAD command is required.\n";
        return false;
    }
    cout << "Semantic analysis successful.\n";
    return true;
}

// PHASE 4: Convert AST statement nodes into the existing IR format.
vector<IR> irPhase(const ASTNode &ast)
{
    cout << "\n========== PHASE 4: INTERMEDIATE REPRESENTATION ==========\n";
    vector<IR> ir;
    for (const auto &s : ast.children)
    {
        string op = s.type;
        if (op == "LOAD")
            op = "LOAD_DATA";
        else if (op == "SELECT")
            op = "SELECT_COLUMNS";
        else if (op == "FILTER")
            op = "FILTER_ROWS";
        else if (op == "SORT")
            op = "SORT_ROWS";
        else if (op == "GROUP")
            op = "GROUP_ROWS";
        else if (op == "CALCULATE")
            op = "AGGREGATE";
        else if (op == "DISPLAY")
            op = "DISPLAY_RESULT";
        else if (op == "SAVE")
            op = "SAVE_RESULT";
        ir.push_back({op, s.value});
    }
    for (const auto &x : ir)
        cout << x.operation << (x.argument.empty() ? "" : " " + x.argument) << "\n";
    return ir;
}

// PHASE 5: Small optimization: remove repeated identical consecutive sorts.
vector<IR> optimizePhase(const vector<IR> &input)
{
    cout << "\n========== PHASE 5: OPTIMIZER ==========\n";
    vector<IR> out;
    for (const auto &x : input)
    {
        if (!out.empty() && x.operation == "SORT_ROWS" && out.back().operation == x.operation && out.back().argument == x.argument)
        {
            cout << "Removed repeated instruction: SORT_ROWS " << x.argument << "\n";
            continue;
        }
        out.push_back(x);
    }
    cout << "Optimized IR contains " << out.size() << " instructions.\n";
    return out;
}

// PHASE 6: Generate Python/Pandas code from the optimized IR.
void codegenPhase(const vector<IR> &ir)
{
    cout << "\n========== PHASE 6: GENERATED PYTHON/PANDAS CODE ==========\n";
    cout << "import pandas as pd\n\ndf = None\n";
    for (const auto &x : ir)
    {
        if (x.operation == "LOAD_DATA")
            cout << "df = pd.read_csv(\"" << unquote(x.argument) << "\")\n";
        else if (x.operation == "SELECT_COLUMNS")
        {
            auto cols = splitComma(x.argument);
            if (cols.size() == 1 && cols[0] == "*")
                continue;
            cout << "df = df[[";
            for (size_t i = 0; i < cols.size(); ++i)
            {
                if (i)
                    cout << ", ";
                cout << "\"" << cols[i] << "\"";
            }
            cout << "]]\n";
        }
        else if (x.operation == "FILTER_ROWS")
        {
            string col, op, val;
            stringstream ss(x.argument);
            ss >> col >> op >> val;
            if (!col.empty() && !op.empty() && !val.empty())
            {
                if (val.size() >= 2 && val.front() == '"' && val.back() == '"')
                    cout << "df = df[df[\"" << col << "\"] " << op << " " << val << "]\n";
                else
                    cout << "df = df[df[\"" << col << "\"] " << op << " " << val << "]\n";
            }
            else
                cout << "# Unsupported FILTER expression: " << x.argument << "\n";
        }
        else if (x.operation == "SORT_ROWS")
        {
            string col, order;
            stringstream ss(x.argument);
            ss >> col >> order;
            cout << "df = df.sort_values(\"" << col << "\", ascending=" << (upper(order) == "DESC" ? "False" : "True") << ")\n";
        }
        else if (x.operation == "GROUP_ROWS")
            cout << "df = df.groupby(\"" << x.argument << "\")\n";
        else if (x.operation == "AGGREGATE")
        {
            size_t a = x.argument.find('('), b = x.argument.find(')');
            if (a != string::npos && b > a)
            {
                string fn = upper(trim(x.argument.substr(0, a))), col = trim(x.argument.substr(a + 1, b - a - 1));
                if (fn == "AVG")
                    fn = "mean";
                else
                    fn = lower(fn);
                cout << "print(df[\"" << col << "\"]." << fn << "())\n";
            }
        }
        else if (x.operation == "DISPLAY_RESULT")
            cout << "print(df)\n";
        else if (x.operation == "SAVE_RESULT")
            cout << "df.to_csv(\"" << unquote(x.argument) << "\", index=False)\n";
    }
}

int main()
{
    vector<string> lines;
    string line;
    cout << "DataLang Compiler (enter END on a separate line to compile)\n";
    while (getline(cin, line) && trim(line) != "END")
        lines.push_back(line);

    vector<Token> tokens;
    if (!lexerPhase(lines, tokens))
        return 1;

    ASTNode ast;
    if (!parserPhase(lines, tokens, ast))
        return 1;

    map<string, Symbol> symbolTable;
    if (!semanticPhase(ast, symbolTable))
        return 1;

    vector<IR> ir = irPhase(ast);
    vector<IR> optimized = optimizePhase(ir);
    codegenPhase(optimized);

    cout << "\nCompilation successful. Copy the generated Python code into a .py file and run it with pandas installed.\n";
    return 0;
}
