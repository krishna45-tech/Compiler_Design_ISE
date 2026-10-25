#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <set>
#include <algorithm>
#include <cctype>
using namespace std;

struct Statement
{
    string command, rest, original;
    int line;
};
struct IR
{
    string operation, argument;
};

string trim(string s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
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
bool startsWith(const string &s, const string &prefix) { return s.rfind(prefix, 0) == 0; }

// Phase 1: basic lexer check. Tokenizes each non-space item and checks quotes/characters.
bool lexerPhase(const vector<string> &lines)
{
    cout << "\n========== PHASE 1: LEXER ==========\n";
    for (size_t i = 0; i < lines.size(); ++i)
    {
        const string &s = lines[i];
        bool inString = false;
        for (size_t j = 0; j < s.size(); ++j)
        {
            if (s[j] == '"')
                inString = !inString;
            if (s[j] != '\"' && !inString && !(isalnum((unsigned char)s[j]) || isspace((unsigned char)s[j]) || s[j] == '_' || s[j] == '#' || s[j] == ',' || s[j] == '(' || s[j] == ')' || s[j] == '.' || s[j] == '>' || s[j] == '<' || s[j] == '=' || s[j] == '!' || s[j] == '-' || s[j] == '+' || s[j] == '*'))
            {
                cout << "Lexical error at line " << i + 1 << ": invalid character '" << s[j] << "'.\n";
                return false;
            }
            if (s[j] == '#')
                break;
        }
        if (inString)
        {
            cout << "Lexical error at line " << i + 1 << ": unterminated string.\n";
            return false;
        }
        string clean = s;
        size_t p = clean.find('#');
        if (p != string::npos)
            clean = clean.substr(0, p);
        clean = trim(clean);
        if (!clean.empty())
        {
            string word;
            stringstream ss(clean);
            ss >> word;
            cout << "Line " << i + 1 << ": command token = " << word << "\n";
        }
    }
    cout << "Lexing successful.\n";
    return true;
}

// Phase 2: parse each line into a simple statement representation.
bool parserPhase(const vector<string> &lines, vector<Statement> &statements)
{
    cout << "\n========== PHASE 2: PARSER ==========\n";
    set<string> commands = {"LOAD", "SELECT", "FILTER", "SORT", "GROUP", "CALCULATE", "DISPLAY", "SAVE"};
    for (size_t i = 0; i < lines.size(); ++i)
    {
        string s = lines[i];
        size_t p = s.find('#');
        if (p != string::npos)
            s = s.substr(0, p);
        s = trim(s);
        if (s.empty())
            continue;
        string cmd, rest;
        stringstream ss(s);
        ss >> cmd;
        getline(ss, rest);
        rest = trim(rest);
        bool valid = commands.count(cmd) > 0;
        if (cmd == "LOAD" || cmd == "SAVE")
            valid = valid && rest.size() >= 2 && rest.front() == '"' && rest.back() == '"';
        else if (cmd == "SELECT")
            valid = valid && !rest.empty();
        else if (cmd == "FILTER")
            valid = valid && (rest.find('>') != string::npos || rest.find('<') != string::npos || rest.find("==") != string::npos || rest.find("!=") != string::npos);
        else if (cmd == "SORT")
            valid = valid && (rest.size() > 4) && (rest.find(" ASC") != string::npos || rest.find(" DESC") != string::npos);
        else if (cmd == "GROUP")
            valid = valid && !rest.empty();
        else if (cmd == "CALCULATE")
            valid = valid && rest.find('(') != string::npos && rest.find(')') != string::npos;
        else if (cmd == "DISPLAY")
            valid = valid && rest.empty();
        if (!valid)
        {
            cout << "Syntax error at line " << i + 1 << ": " << s << "\n";
            return false;
        }
        statements.push_back({cmd, rest, s, (int)i + 1});
        cout << "Parsed: " << cmd << "\n";
    }
    if (statements.empty())
    {
        cout << "No commands entered.\n";
        return false;
    }
    cout << "Parsing successful.\n";
    return true;
}

// Phase 3: checks command order and column names against a small example schema.
bool semanticPhase(const vector<Statement> &st)
{
    cout << "\n========== PHASE 3: SEMANTIC ANALYZER ==========\n";
    set<string> columns = {"name", "age", "marks", "department"};
    bool loaded = false;
    int loadCount = 0;
    for (size_t i = 0; i < st.size(); ++i)
    {
        const auto &x = st[i];
        if (x.command == "LOAD")
        {
            loadCount++;
            if (i != 0 || loaded)
            {
                cout << "Semantic error: LOAD must appear once, as the first command.\n";
                return false;
            }
            loaded = true;
            continue;
        }
        if (!loaded)
        {
            cout << "Semantic error: LOAD must be the first command.\n";
            return false;
        }
        vector<string> used;
        if (x.command == "SELECT")
            used = splitComma(x.rest);
        else if (x.command == "SORT")
        {
            stringstream ss(x.rest);
            string c;
            ss >> c;
            used.push_back(c);
        }
        else if (x.command == "GROUP")
            used.push_back(x.rest);
        else if (x.command == "FILTER")
        {
            stringstream ss(x.rest);
            string c;
            ss >> c;
            used.push_back(c);
        }
        else if (x.command == "CALCULATE")
        {
            size_t a = x.rest.find('('), b = x.rest.find(')');
            if (a != string::npos && b > a)
                used.push_back(trim(x.rest.substr(a + 1, b - a - 1)));
        }
        for (string c : used)
            if (c != "*" && columns.count(c) == 0)
            {
                cout << "Semantic error: unknown column '" << c << "'. Example columns: name, age, marks, department.\n";
                return false;
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

string makeIRArgument(const Statement &s) { return s.rest; }
// Phase 4: converts statements to a simple intermediate representation.
vector<IR> irPhase(const vector<Statement> &st)
{
    cout << "\n========== PHASE 4: INTERMEDIATE REPRESENTATION ==========\n";
    vector<IR> ir;
    for (const auto &s : st)
    {
        string op = s.command;
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
        ir.push_back({op, makeIRArgument(s)});
    }
    for (const auto &x : ir)
        cout << x.operation << (x.argument.empty() ? "" : " " + x.argument) << "\n";
    return ir;
}

// Phase 5: removes repeated identical consecutive SORT instructions.
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

// Phase 6: generates a Python/Pandas script from optimized IR.
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
                cout << "df = df[df[\"" << col << "\"] " << op << " " << val << "]\n";
            else
                cout << "# Unsupported FILTER expression: " << x.argument << "\n";
        }
        else if (x.operation == "SORT_ROWS")
        {
            string col, order;
            stringstream ss(x.argument);
            ss >> col >> order;
            cout << "df = df.sort_values(\"" << col << "\", ascending=" << (order == "DESC" ? "False" : "True") << ")\n";
        }
        else if (x.operation == "GROUP_ROWS")
            cout << "df = df.groupby(\"" << x.argument << "\")\n";
        else if (x.operation == "AGGREGATE")
        {
            size_t a = x.argument.find('('), b = x.argument.find(')');
            if (a != string::npos && b > a)
            {
                string fn = trim(x.argument.substr(0, a)), col = trim(x.argument.substr(a + 1, b - a - 1));
                if (fn == "AVG")
                    fn = "mean";
                else if (fn == "COUNT")
                    fn = "count";
                else
                    transform(fn.begin(), fn.end(), fn.begin(), ::tolower);
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
    while (getline(cin, line) && line != "END")
        lines.push_back(line);
    if (!lexerPhase(lines))
        return 1;
    vector<Statement> statements;
    if (!parserPhase(lines, statements))
        return 1;
    if (!semanticPhase(statements))
        return 1;
    vector<IR> ir = irPhase(statements);
    vector<IR> optimized = optimizePhase(ir);
    codegenPhase(optimized);
    cout << "\nCompilation successful. Copy the generated Python code into a .py file and run it with pandas installed.\n";
    return 0;
}
