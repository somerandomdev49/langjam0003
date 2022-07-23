/* actor model assembly assembly assembly */
#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cctype>
#include <vector>
#include <cassert>

namespace TokenType {
    enum Enum : char {
        error,
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
            "identifier",
            "number",
            "string"
        };

        return s[e];
    }
};

using ETokenType = TokenType::Enum;

struct StackAllocator {
    std::uint8_t *data;
    std::uint8_t *current;
    std::size_t available;
    
    std::ptrdiff_t used() const { return current - data; }

    void preallocate(std::size_t bytes) {
        available = bytes;
    }

    uint8_t *allocate(std::size_t size) {
        return allocate<std::uint8_t>(size);
    }

    template<typename T>
    T *allocate(std::size_t count = 1) {
        assert(count > 0);
        std::size_t size = count * sizeof(T);
        if(used() + size > available) {
            preallocate(used() + size);
        }
    }
};

struct Token {
    char type;
    union {
        const char *string;
        intptr_t integer;
    };

    static bool is_valid(const Token &token) {
        return TokenType::is_valid(token.type) || token.string.empty();
    }

    static std::string to_string(const Token &token) {
        using namespace std::literals;
        return "Token{"s + (
            TokenType::is_valid(token.type)
            ? TokenType::to_string((ETokenType)token.type)
                + ", '"s + token.string + "'}"
            : "'"s + token.type + "'}"
        );
    }
};

struct Lexer {
    std::vector<Token> buffer;

    Token get(std::istream &ins) {
        char c;
        while(std::isspace(c = ins.get()));
        if(std::isalpha(c) || c == '_') {
            Token token { TokenType::identifier, "" };
            while(std::isalnum(c) || c == '_') {
                token.string += ins.get();
                c = ins.peek();
            }
            return token;
        }
        if(std::isdigit(c) || c == '-') {
            Token token { TokenType::number, "" };
            bool negative = false;
            if(c == '-') { negative = true; c = ins.get(); }
            if(c == '0') {
                if(ins.peek() == 'x') {
                    int n = 0;
                    token.string += ins.get();
                    int value = 0;
                    while(std::isxdigit(c) || c == '_') {
                        c = ins.get();
                        if(c == '_') continue;
                        if(c > '9') c -= 'A' - 0xA;
                        else c -= '0';
                        value += c * std::pow(16, n++);
                        c = ins.peek();
                    }
                    token.string = std::to_string(value);
                } else if(ins.peek() == 'o') {
                    int n = 0;
                    token.string += ins.get();
                    int value = 0;
                    while((std::isdigit(c) && c < '8') || c == '_') {
                        c = ins.get();
                        if(c == '_' - '0') continue;
                        value += (c - '0') * std::pow(8, n++);
                        c = ins.peek();
                    }
                    token.string = (std::intptr_t)value;
                } else if(ins.peek() == 'b') {
                    token.string += ins.get();
                    while(c == '0' || c == '1' || c == '_') {
                        token.string += ins.get();
                        c = ins.peek();
                    }
                }
            }
            
            return token;
        }
    };
};

auto main() -> int {
    return 0;
}
