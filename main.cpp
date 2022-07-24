/* actor model assembly assembly assembly */
#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cctype>
#include <vector>
#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <set>

namespace TokenType {
    enum Enum : char {
        error,
        eof,
        identifier,
        number,
        string,
        send_to,
        send_back,
        last_
    };

    bool is_valid(char e) {
        return e < last_;
    }

    const char *to_string(Enum e) {
        static const char *s[] = {
            "error",
            "eof",
            "identifier",
            "number",
            "string",
            ">>",
            "<<",
        };

        return s[e];
    }
};

using ETokenType = TokenType::Enum;

struct StackAllocator {
    std::uint8_t *data = nullptr;
    std::uint8_t *current = nullptr;
    std::size_t available = 0;
    
    ~StackAllocator() {
        if(data != nullptr) delete[] data;
    }

    std::ptrdiff_t used() const { return current - data; }

    void preallocate(std::size_t bytes) {
        if(bytes == 0) return;
        available = bytes;
        std::uint8_t *ptr = new std::uint8_t[available];
        if(data != nullptr) std::memcpy(ptr, data, available);
        current = ptr + used();
        data = ptr;
    }

    uint8_t *allocate(std::size_t size) {
        return allocate<std::uint8_t>(size);
    }

    template<typename T>
    T *allocate(std::size_t count = 1, std::size_t chunk = 512) {
        if(count == 0) return (T*)current;
        std::size_t size = count * sizeof(T);
        if(used() + size > available)
            preallocate(available + chunk);
        auto *ptr = (T*)current;
        current += size;
        return ptr;
    }
};

struct Token {
    const char type;
    union Value {
        const char *const string;
        const intptr_t integer;
    } value;

    Token(const Token &other)
        : type(other.type)
        , value(other.value) { }

    Token(char type, Value value)
        : type(type)
        , value(value) {}

    Token(const char *const string)
        : type(TokenType::identifier)
        , value({ .string = string }) { }

    Token(std::intptr_t integer)
        : type(TokenType::number)
        , value({ .integer = integer }) { }


    static Token eof() {
        return { TokenType::eof, {} };
    }

    static std::string to_string(const Token &token) {
        using namespace std::literals;
        return "Token{"s + (
            TokenType::is_valid(token.type)
            ? token.type == TokenType::number
            ? "number, " + std::to_string(token.value.integer) + "}"
            : token.type == TokenType::identifier
            ? "identifier, "s + token.value.string + "}"
            : token.type == TokenType::string
            ? "string, "s + token.value.string + "}"
            : TokenType::to_string((ETokenType)token.type) + "}"s
            : "'"s + token.type + "'}"
        );
    }
};

struct Lexer {
    std::istream &ins;
    std::unique_ptr<Token> buffer = nullptr;
    StackAllocator allocator;

    Lexer(std::istream &ins) : ins(ins) {
        allocator.allocate(1); // init
    }

    auto peek() -> const Token & {
        if(buffer == nullptr || buffer->type == TokenType::error)
            buffer = std::make_unique<Token>(get());
        return *buffer.get();
    }

    auto get() -> Token {
        if(buffer != nullptr) {
            auto token = *buffer;
            buffer = nullptr;
            return token;
        }

        char c;
        while(std::isspace(c = ins.peek())) ins.get();
        if(c == EOF) return { TokenType::eof, {} };
        if(std::isalpha(c) || c == '_') {
            auto used = allocator.used();
            while(std::isalnum(c) || c == '_') {
                *allocator.allocate<char>(1) = ins.get();
                c = ins.peek();
            }
            return { (char*)allocator.data + used };
        } else if(c == '"') {
            auto used = allocator.used();
            ins.get();
            while(c != '"') {
                *allocator.allocate<char>(1) = ins.get();
                c = ins.peek();
            }
            c = ins.get();
            return { TokenType::string, {
                .string = (char*)allocator.data + used
            } };
        }
        if(std::isdigit(c) || c == '-') {
            bool negative = false;
            if(c == '-') {
                negative = true;
                ins.get();
                c = ins.peek();
            }
            // if(c == '0') {
            //     if(ins.peek() == 'x') {
            //         c = (ins.get(), '0');
            //         std::size_t value = 0;
            //         while(std::isxdigit(c) || c == '_') {
            //             c = ins.get();
            //             if(c == '_') continue;
            //             if(c > '9') c -= 'A' - 0xA;
            //             else c -= '0';
            //             value = value * 16 + c;
            //             c = ins.peek();
            //         }
            //         return { negative ? (std::intptr_t)value : -(std::intptr_t)value };
            //     } else if(ins.peek() == 'o') {
            //         c = (ins.get(), '0');
            //         std::size_t value = 0;
            //         while((std::isdigit(c) && c < '8') || c == '_') {
            //             c -= '0';
            //             if(c == '_' - '0') continue;
            //             value = value * 8 + c;
            //             c = ins.peek();
            //         }
            //         return { negative ? (std::intptr_t)value : -(std::intptr_t)value };
            //     } else if(ins.peek() == 'b') {
            //         c = (ins.get(), '0');
            //         std::size_t value = 0;
            //         while((std::isdigit(c) && c < '8') || c == '_') {
            //             c = ins.get() - '0';
            //             if(c == '_' - '0') continue;
            //             value = value * 2 + c;
            //             c = ins.peek();
            //         }
            //         return { negative ? (std::intptr_t)value : -(std::intptr_t)value };
            //     }
            // }
            size_t value = 0;
            while((std::isdigit(c) && c < '8') || c == '_') {
                c = ins.get() - '0';
                if(c == '_' - '0') continue;
                value = value * 10 + c;
                c = ins.peek();
            }
            return { negative ? -(std::intptr_t)value : (std::intptr_t)value };
        }

        c = ins.get();
        if(c == '>' && ins.peek() == '>') return { (ins.get(), TokenType::send_to), {} };
        if(c == '<' && ins.peek() == '<') return { (ins.get(), TokenType::send_back), {} };

        return { c, {} };
    }
};

/** actual c string, doesn't allocate anything. */
class CString {
    const char *string_;
public:
    CString() : string_(nullptr) { }
    CString(const char *string) : string_(string) { }
    operator const char*() const { return string_; }
    CString &operator++() { string_++; return *this; }
    CString &operator--() { string_--; return *this; }
    CString operator+(std::size_t other) { return CString(string_ + other); }
    CString operator+(std::ptrdiff_t other) { return CString(string_ + other); }
    CString operator-(std::size_t other) { return CString(string_ + other); }
    CString operator-(std::ptrdiff_t other) { return CString(string_ + other); }
    bool operator==(CString other) { return std::strcmp(string_, other.string_) == 0; }
    bool operator!=(CString other) { return std::strcmp(string_, other.string_) != 0; }
    bool operator>(CString other) { return std::strcmp(string_, other.string_) > 0; }
    bool operator>=(CString other) { return std::strcmp(string_, other.string_) >= 0; }
    bool operator<(CString other) { return std::strcmp(string_, other.string_) < 0; }
    bool operator<=(CString other) { return std::strcmp(string_, other.string_) <= 0; }
};

template<>
struct std::hash<CString> {
    std::size_t operator()(CString s) const noexcept {
        return std::hash<std::string_view>()(std::string_view{(const char*)s});
    }
};

namespace ast {
    using Tag = std::size_t;

    struct Value {
        enum class Type {
            error,
            chain_param,
            variable,
            number,
            string
        } type;
        union {
            uintptr_t parameter; /* chain parameter '%N' */
            intptr_t number; /* number 'K' */
            const char *string; /* string '"S"' */
            const char *variable; /* variable 'X' */
        };

        void output(std::ostream &out) const {
            switch(type) {
                case Type::error:
                    out << "[error value]";
                    break;
                case Type::chain_param:
                    out << "%" << parameter;
                    break;
                case Type::variable:
                    out << variable;
                    break;
                case Type::number:
                    out << number;
                    break;
                case Type::string:
                    out << variable;
                    break;
            }
        }
    };

    struct Command {
        std::unordered_set<Tag> pre, post;
        virtual ~Command() = default;
        virtual void output(std::ostream &out) const {
            out << "        [default command]";
        }
    protected:
        void output_pre(std::ostream &out) const {
            out << "        ";
            if(!pre.empty()) {
                out << "[";
                auto it = pre.begin();
                out << *it++;
                while(it != pre.end()) {
                    out << ", " << *it++;
                }
                out << "] ";
            }
        }

        void output_post(std::ostream &out) const {
            if(!post.empty()) {
                out << " [";
                auto it = post.begin();
                out << *it++;
                while(it != post.end()) {
                    out << ", " << *it++;
                }
                out << "]";
            }
        }
    };

    struct ErrorCommand : Command {
        void output(std::ostream &out) const override {
            out << "        [error command]";
        }
    };

    struct SendCommand : Command {
        CString name;
        std::vector<Value> parameters;
        Value to;
        void output(std::ostream &out) const override {
            output_pre(out);
            out << "(" << name << ") ";
            if(!parameters.empty()) {
                auto it = parameters.begin();
                (*it++).output(out);
                while(it != parameters.end()) {
                    out << " ";
                    (*it++).output(out);
                }
            }
            out << " >> ";
            to.output(out);
            output_post(out);
        }
    };

    struct SendBackCommand : Command {
        std::vector<Value> parameters;
    };

    struct ChainCommand : Command {
        std::unique_ptr<Command> left, right;
    };

    struct MessageHandler {
        std::vector<CString> parameters;
        std::vector<std::unique_ptr<Command>> commands;

        void output(std::ostream &out) const {
            if(!parameters.empty()) {
                auto it = parameters.begin();
                out << *it++;
                while(it != parameters.end()) {
                    out << " " << *it++;
                }
            }
            out << " {\n";
            for(const auto &command : commands) {
                command->output(out);
                out << ";\n";
            }
            out << "    }\n";
        }
    };

    struct Actor {
        CString name;
        std::unordered_set<CString> variables;
        std::unordered_map<CString, MessageHandler> handlers;

        void output(std::ostream &out) const {
            out << name;
            if(!variables.empty()) {
                out << " : ";
                auto it = variables.begin();
                out << *it++;
                while(it != variables.end()) {
                    out << ", " << *it++;
                }
            }
            out << " {\n";
            for(const auto &[name, handler] : handlers) {
                out << "    (" << name << ") ";
                handler.output(out);
            }
            out << "}\n";
        }
    };
}

struct Parser {
    Lexer &lexer;
    StackAllocator &allocator;

    CString copy_(const char *str) {
        char *ptr = allocator.allocate<char>(std::strlen(str) + 1);
        std::memcpy(ptr, str, std::strlen(str) + 1);
        return ptr;
    }

    void error(const std::string &message) {
        std::cerr << "\033[1;31mError:\033[m " << message << std::endl;
    }

    template<char T>
    Token expect(Token::Value value = {}) {
        auto tok = lexer.peek();
        if(tok.type == T) return lexer.get();
        error("expected " + (TokenType::is_valid(T)
            ? std::string(TokenType::to_string((ETokenType)T))
            : std::string(1, T)) + ", but got " + Token::to_string(tok));
        return { T, value };
    }

    std::string parseName() {
        expect<'('>();
        std::stringstream name;
        name << expect<TokenType::identifier>({ .string = "?" }).value.string;
        while(lexer.peek().type != ')') {
            name << ' ' << expect<TokenType::identifier>({ .string = "?" }).value.string;
        }
        expect<')'>();
        return name.str();
    }

    ast::Value parseValue() {
        if(lexer.peek().type == TokenType::identifier) {
            return {
                .type = ast::Value::Type::variable,
                .variable = copy_(expect<TokenType::identifier>().value.string)
            };
        } else if(lexer.peek().type == TokenType::string) {
            return {
                .type = ast::Value::Type::string,
                .variable = copy_(expect<TokenType::identifier>().value.string)
            };
        } else if(lexer.peek().type == TokenType::number) {
            return {
                .type = ast::Value::Type::number,
                .number = expect<TokenType::number>().value.integer
            };
        } else if(lexer.peek().type == '%') {
            lexer.get();
            return {
                .type = ast::Value::Type::chain_param,
                .parameter = (std::size_t)expect<TokenType::number>({ .integer = 0 }).value.integer
            };
        } else {
            error("expected a value (variable, number or %index), but got " + Token::to_string(lexer.peek()) + " instead.");
            return { ast::Value::Type::error };
        }
    }

    std::unique_ptr<ast::Command> parseCommandBase(std::unordered_set<ast::Tag> &&pre = {}) {
        std::unique_ptr<ast::Command> ret;

        if(lexer.peek().type == '(') {
            auto name = parseName();
            auto cmd = std::make_unique<ast::SendCommand>();
            cmd->pre = std::move(pre);
            cmd->name = copy_(name.c_str());
            while(lexer.peek().type != TokenType::send_to) {
                cmd->parameters.push_back(parseValue());
            }
            expect<TokenType::send_to>();
            cmd->to = parseValue();
            ret = std::move(cmd);
        } else {
            error("expected a command, got " + Token::to_string(lexer.peek()) + " instead!");
            ret = std::make_unique<ast::ErrorCommand>();
        }

        if(lexer.peek().type == ',') {
            auto cmd = std::make_unique<ast::ChainCommand>();
            std::swap(cmd->pre, ret->pre);
            cmd->left = std::move(ret);
            cmd->right = parseCommandBase();
            ret = std::move(cmd);
        }

        return ret;
    }

    std::unique_ptr<ast::Command> parseCommand() {
        std::unordered_set<ast::Tag> pre;
        if(lexer.peek().type == '[') {
            lexer.get();
            while(lexer.peek().type != ']') {
                pre.insert(expect<TokenType::number>({ .integer = 0 }).value.integer);
                if(lexer.peek().type == ',') expect<','>();
                else break;
            }
            expect<']'>();
        }
        
        auto ret = parseCommandBase(std::move(pre));

        if(lexer.peek().type == '[') {
            lexer.get();
            while(lexer.peek().type != ']') {
                ret->post.insert(expect<TokenType::number>({ .integer = 0 }).value.integer);
                if(lexer.peek().type == ',') expect<','>();
                else break;
            }
            expect<']'>();
        }

        return ret;
    }

    void parseMessageHandler(ast::MessageHandler &handler) {
        while(lexer.peek().type != '{') {
            CString param = expect<TokenType::identifier>({ .string = "?" }).value.string;
            handler.parameters.push_back(copy_(param));
        }
        expect<'{'>();
        while(lexer.peek().type != '}') {
            handler.commands.push_back(parseCommand());
            expect<';'>();
        }
        expect<'}'>();
    }

    void parseActor(ast::Actor &actor) {
        actor.name = copy_(expect<TokenType::identifier>({ .string = "?" }).value.string);
        if(lexer.peek().type == ':') {
            lexer.get();
            do {
                CString var = expect<TokenType::identifier>({ .string = "?" }).value.string;
                actor.variables.insert(copy_(var));
            } while(lexer.peek().type != '{');
        }
        expect<'{'>();
        while(lexer.peek().type != '}') {
            auto name = parseName();
            if(actor.handlers.contains(name.c_str())) {
                error("handler for message (" + name + ") already exists!");
                ast::MessageHandler badHandler;
                parseMessageHandler(badHandler);
            } else parseMessageHandler(actor.handlers[name.c_str()]);
        }
        expect<'}'>();
    }
};

auto main() -> int {
    std::ifstream ins("test.amea");
    if(!ins) {
        std::cerr << "error opening file!" << std::endl;
        return 1;
    }
    Lexer lexer{ins};
    StackAllocator allocator;
    Parser parser{lexer, allocator};
    ast::Actor actor;
    parser.parseActor(actor);
    actor.output(std::cout);
    return 0;
}
