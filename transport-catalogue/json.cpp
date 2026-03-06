#include "json.h"
#include <cctype>
#include <sstream>

using namespace std;

namespace json {
namespace {

Node LoadNode(istream& input);

string LoadLiteral(istream& input) {
    string s;
    while (isalpha(input.peek())) {
        s.push_back(static_cast<char>(input.get()));
    }
    return s;
}

Node LoadNull(istream& input) {
    if (auto s = LoadLiteral(input); s == "null") {
        return Node(nullptr);
    }
    throw ParsingError("Failed to parse 'null'");
}

Node LoadBool(istream& input) {
    auto s = LoadLiteral(input);
    if (s == "true") return Node(true);
    if (s == "false") return Node(false);
    throw ParsingError("Failed to parse boolean");
}

Node LoadNumber(istream& input) {
    string parsed_num;
    auto read_char = [&parsed_num, &input] { parsed_num += static_cast<char>(input.get()); };
    auto read_digits = [&input, read_char] {
        if (!isdigit(input.peek())) throw ParsingError("A digit is expected");
        while (isdigit(input.peek())) read_char();
    };

    if (input.peek() == '-') read_char();
    if (input.peek() == '0') {
        read_char();
    } else {
        read_digits();
    }

    bool is_int = true;
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') read_char();
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            try { return Node(stoi(parsed_num)); } catch (...) {}
        }
        return Node(stod(parsed_num));
    } catch (...) { throw ParsingError("Failed to parse number"); }
}

Node LoadString(istream& input) {
    string s;
    char ch;
    while (input.get(ch)) {
        if (ch == '"') {
            return Node(move(s));
        } else if (ch == '\\') {
            input.get(ch);
            switch (ch) {
                case 'n': s.push_back('\n'); break;
                case 't': s.push_back('\t'); break;
                case 'r': s.push_back('\r'); break;
                case '"': s.push_back('"'); break;
                case '\\': s.push_back('\\'); break;
                default: throw ParsingError("Unrecognized escape sequence");
            }
        } else {
            s.push_back(ch);
        }
    }
    throw ParsingError("String parsing error");
}

Node LoadArray(istream& input) {
    Array result;
    char c;
    for (; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }
    return Node(move(result));
}

Node LoadDict(istream& input) {
    Dict result;
    char c;
    for (; input >> c && c != '}';) {
        if (c == ',') {
            input >> c;
        }
        if (c != '"') throw ParsingError("Key must be a string");
        
        string key = LoadString(input).AsString();
        input >> c;
        if (c != ':') throw ParsingError("Missing colon in Dictionary");
        
        result.insert({move(key), LoadNode(input)});
    }
    return Node(move(result));
}

Node LoadNode(istream& input) {
    char c;
    input >> c;
    if (c == '[') return LoadArray(input);
    if (c == '{') return LoadDict(input);
    if (c == '"') return LoadString(input);
    if (c == 't' || c == 'f') {
        input.putback(c);
        return LoadBool(input);
    }
    if (c == 'n') {
        input.putback(c);
        return LoadNull(input);
    }
    input.putback(c);
    return LoadNumber(input);
}

}  // namespace

Node::Node(std::nullptr_t) : value_(nullptr) {}
Node::Node(Array array) : value_(move(array)) {}
Node::Node(Dict map) : value_(move(map)) {}
Node::Node(bool value) : value_(value) {}
Node::Node(int value) : value_(value) {}
Node::Node(double value) : value_(value) {}
Node::Node(string value) : value_(move(value)) {}

bool Node::IsNull() const { return holds_alternative<std::nullptr_t>(value_); }
bool Node::IsInt() const { return holds_alternative<int>(value_); }
bool Node::IsDouble() const { return IsInt() || IsPureDouble(); }
bool Node::IsPureDouble() const { return holds_alternative<double>(value_); }
bool Node::IsBool() const { return holds_alternative<bool>(value_); }
bool Node::IsString() const { return holds_alternative<string>(value_); }
bool Node::IsArray() const { return holds_alternative<Array>(value_); }
bool Node::IsDict() const { return holds_alternative<Dict>(value_); }

int Node::AsInt() const { if (!IsInt()) throw logic_error("Not an int"); return get<int>(value_); }
double Node::AsDouble() const { if (!IsDouble()) throw logic_error("Not a double"); return IsPureDouble() ? get<double>(value_) : get<int>(value_); }
bool Node::AsBool() const { if (!IsBool()) throw logic_error("Not a bool"); return get<bool>(value_); }
const string& Node::AsString() const { if (!IsString()) throw logic_error("Not a string"); return get<string>(value_); }
const Array& Node::AsArray() const { if (!IsArray()) throw logic_error("Not an array"); return get<Array>(value_); }
const Dict& Node::AsDict() const { if (!IsDict()) throw logic_error("Not a dict"); return get<Dict>(value_); }

Document::Document(Node root) : root_(move(root)) {}
const Node& Document::GetRoot() const { return root_; }

Document Load(istream& input) { return Document{LoadNode(input)}; }

struct PrintContext {
    ostream& out;
    int indent_step = 4;
    int indent = 0;
    void PrintIndent() const { for (int i = 0; i < indent; ++i) out.put(' '); }
    PrintContext Indented() const { return {out, indent_step, indent + indent_step}; }
};

void PrintNode(const Node& node, const PrintContext& ctx);

void PrintValue(const std::string& value, const PrintContext& ctx) {
    ctx.out << '"';

    for (const char c : value) {
        if (c == '\r') ctx.out << "\\r";
        else if (c == '\n') ctx.out << "\\n";
        else if (c == '"') ctx.out << "\\\"";
        else if (c == '\\') ctx.out << "\\\\";
        else ctx.out << c;
    }
    ctx.out << '"';
}

void PrintValue(const std::nullptr_t&, const PrintContext& ctx) { ctx.out << "null"; }
void PrintValue(bool value, const PrintContext& ctx) { ctx.out << (value ? "true" : "false"); }
void PrintValue(int value, const PrintContext& ctx) { ctx.out << value; }
void PrintValue(double value, const PrintContext& ctx) { ctx.out << value; }

void PrintValue(const Array& array, const PrintContext& ctx) {
    if (array.empty()) { ctx.out << "[]"; return; }
    ctx.out << "[\n";
    auto inner_ctx = ctx.Indented();
    bool first = true;
    for (const auto& node : array) {
        if (!first) ctx.out << ",\n";
        first = false;
        inner_ctx.PrintIndent();
        PrintNode(node, inner_ctx);
    }
    ctx.out << '\n';
    ctx.PrintIndent();
    ctx.out << ']';
}

void PrintValue(const Dict& dict, const PrintContext& ctx) {
    if (dict.empty()) { ctx.out << "{}"; return; }
    ctx.out << "{\n";
    auto inner_ctx = ctx.Indented();
    bool first = true;
    for (const auto& [key, node] : dict) {
        if (!first) ctx.out << ",\n";
        first = false;
        inner_ctx.PrintIndent();
        PrintValue(key, inner_ctx);
        ctx.out << ": ";
        PrintNode(node, inner_ctx);
    }
    ctx.out << '\n';
    ctx.PrintIndent();
    ctx.out << '}';
}

void PrintNode(const Node& node, const PrintContext& ctx) {
    std::visit([&ctx](const auto& value) { PrintValue(value, ctx); }, node.GetValue());
}

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), PrintContext{output});
}

}  // namespace json
