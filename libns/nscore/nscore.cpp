#include <iostream>

#include "nscore.h"

static std::stack<__stack_entry> __stack_table;

std::size_t __get_size(std::string arg) { return arg.size(); }

std::size_t __get_length(std::string arg) { return __chars(arg.data(), arg.size()).length(); }

__stack_activation_record::__stack_activation_record(const char* file, const char* function, unsigned int line, unsigned int column) { __stack_table.emplace(file, function, line, column); }

__stack_activation_record::~__stack_activation_record() { __stack_table.pop(); }

void __stack_activation_record::location(int line, int column)
{
    if (!__stack_table.empty()) {
        __stack_table.top().line = line;
        __stack_table.top().column = column;
    }
}

void nscore_println(std::string s) { std::cout << s << '\n'; }

void nscore_crash(std::string message, const char* file, int line, int column) 
{
    if (!file || line <= 0) std::cerr << "• crash: " << message << '\n';
    else std::cerr << "• crash at " << file << ":" << line << ":" << column << ": " << message << '\n';
    nscore_stacktrace();
    std::exit(EXIT_FAILURE);
}

void nscore_exit(std::int32_t code) { std::exit(code); }

void nscore_stacktrace()
{
    if (__stack_table.empty()) return;

    std::cout << "• stack traces\n";

    for (unsigned int depth = 0; !__stack_table.empty(); __stack_table.pop(), ++depth) {
        __stack_entry entry = __stack_table.top();
        std::cout << "  #" << depth << " " << entry.function  << " at " << entry.file << ":" << entry.line << ":" << entry.column << "\n";
    }
}