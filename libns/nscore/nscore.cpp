#include <iostream>

#include "nscore.h"

std::size_t __get_size(std::string arg) { return arg.size(); }

std::size_t __get_length(std::string arg) { return __chars(arg.data(), arg.size()).length(); }

void nscore_println(std::string s) { std::cout << s << '\n'; }

void nscore_crash(std::string message, const char* file, int line) 
{
    if (!file || line <= 0) std::cerr << "• crash: " << message << '\n';
    else std::cerr << "• crash at " << file << ":" << line << ": " << message << '\n';
    std::abort();
}

void nscore_exit(std::int32_t code) { std::exit(code); }