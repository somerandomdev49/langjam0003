#include <cstdio>
#include <string_view>
// extern auto printf(const char *format, ...) -> int;

auto main() -> int {
    printf("there are %d printable characters.\n", 127 - 32);

    std::string_view invalid_chars = "()[]{}";
    int skip = 0;
    for(char i = 32; i < 127; ++i) {
        if(invalid_chars.find(i) != invalid_chars.npos) {
            skip += 1;
            continue;
        }
        char n = i - skip;
        printf("    #%d, '%c', %02dd, %02xh\n", n - 32, i, n - 32, n - 32);
    }
    return 0;
}


