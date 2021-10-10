#include <iostream>
#include <iomanip>
#include <fstream>
#include <mutex>
#include <unordered_map>

#include "core.h"

// stack table which is thread local
thread_local static std::vector<__stack_entry> __stack_table;
// file content table to print stack traces in a pretty way
static std::unordered_map<std::string, std::unique_ptr<char>> __sources;
// mutex for shared access among threads to sources table in order to be updated with file content when printing errors
static std::mutex __sources_mutex;

static bool __load_source_file(const char* path)
{
    std::ifstream stream(path, std::ios_base::binary);

    if (!stream.is_open()) return false;

    auto fbuf = stream.rdbuf();
    std::size_t size = fbuf->pubseekoff(0, stream.end, stream.in);
    fbuf->pubseekpos(0, std::ios_base::in);

    auto buffer = new char[size + 1];

    fbuf->sgetn(buffer, size);

    buffer[size] = 0;

    __sources[std::string(path)].reset(buffer);

    stream.close();

    return true;
}

static std::string __get_line_from_source(const char* path, unsigned line, unsigned column, std::vector<std::string>& before, std::vector<std::string>& after)
{
    if (__sources.count(path) == 0 && !__load_source_file(path)) return {};

    int i = 1;                                                                                                            
    const char* start = __sources[std::string(path)].get(), *end, *old = start;
    constexpr const unsigned offset = 2;                                                                                        
  
    while (*start != '\0' && i < line) {
        while (*start != '\0' && *start != '\n') ++start;
        if (i >= line - offset && i < line) before.emplace_back(old, start);
        ++i;
        old = ++start;
    }

    if (*start == '\0') return {};

    end = start;

    while (*end != '\0' && *end != '\n') ++end;

    std::string result(std::size_t(end - start), 0);

    std::copy(start, end, result.begin());

    if (*end == '\0') return result;

    for (start = ++end; *start != '\0' && i < line + offset; ++i, start = ++end) {
        while (*end != '\0' && *end != '\n') ++end;
        after.emplace_back(start, end);
    }

    return result;
}

std::size_t __get_size(std::string arg) { return arg.size(); }

std::size_t __get_length(std::string arg) { return __chars(arg.data(), arg.size()).length(); }

__stack_activation_record::__stack_activation_record(const char* file, const char* function, unsigned int line, unsigned int column) { __stack_table.emplace_back(file, function, line, column); }

__stack_activation_record::~__stack_activation_record() { __stack_table.pop_back(); }

void __stack_activation_record::location(int line, int column)
{
    if (!__stack_table.empty()) {
        __stack_table.back().line = line;
        __stack_table.back().column = column;
    }
}

inline void __exit(std::int32_t code) 
{
#ifdef __TEST__
    throw __test_failure { code };
#else
    std::exit(code);
#endif
}

void __println(std::string s) 
{
#ifndef __TEST__ 
    std::cout << s << '\n'; 
#endif
}

void __crash(std::string message, const char* file, int line, int column) 
{
    if (!file || line <= 0) std::cerr << "• crash: " << message << '\n';
    else std::cerr << "• crash at " << file << ":" << line << ":" << column << ": " << message << '\n';
#if __DEVELOPMENT__
    __stacktrace();
#endif
    __exit(EXIT_FAILURE);
}

void __assert(bool condition, std::string message, const char* file, int line, int column) 
{
    if (condition) return;
    if (!file || line <= 0) std::cerr << "• violation";
    else std::cerr << "• violation at " << file << ":" << line << ":" << column;
    if (!message.empty()) std::cerr << ": " << message << '\n';
#if __DEVELOPMENT__
    __stacktrace();
#endif
    __exit(EXIT_FAILURE);
}

void __stacktrace()
{
    if (__stack_table.empty()) return;

    std::unique_lock<std::mutex> lock(__sources_mutex);

    std::cout << "• stack traces\n";

    for (unsigned int depth = 0; depth < __stack_table.size(); ++depth) {
        __stack_entry entry = __stack_table[__stack_table.size() - 1 - depth];
        std::vector<std::string> before, after;
        std::string line = __get_line_from_source(entry.file, entry.line, entry.column, before, after);
        unsigned iline = entry.line - before.size(), width = entry.line + after.size() < 10 ? 1 : std::log10(entry.line + after.size()) + 1;
        std::cout << "  #" << depth << " " << entry.function  << " at " << entry.file << ":" << entry.line << ":" << entry.column << "\n";
        if (line.empty()) continue;
        for (auto line : before) std::cout << "     " << std::setw(width) << iline++ << " | " << line << '\n';
        std::cout << "  -> " << std::setw(width) << iline++ << " | " << line << '\n';
        for (auto line : after) std::cout << "     " << std::setw(width) << iline++ << " | " << line << '\n';
    }

#ifndef __TEST__
    __stack_table.clear();
#endif
}

void __signal_handler(int signo)
{
    std::cout << "• just caught signal";
    
    switch (signo) {
        case SIGABRT:
            std::cout << " SIGABRT (abort)";
            break;
        case SIGFPE:
            std::cout << " SIGFPE (floating point exception)";
            break;
        case SIGINT:
            std::cout << " SIGINT (interrupt)";
            break;
        case SIGKILL:
            std::cout << " SIGKILL (kill)";
            break;
        case SIGQUIT:
            std::cout << " SIGQUIT (quit)";
            break;
        case SIGSEGV:
            std::cout << " SIGSEGV (segmentation fault)";
            break;
        default:
            break;
    }

    std::cout << ", terminating the program...\n";

    std::unique_lock<std::mutex> lock(__sources_mutex);
    
    std::cout << "• stack traces\n";

    for (unsigned int depth = 0; depth < __stack_table.size(); ++depth) {
        __stack_entry entry = __stack_table[__stack_table.size() - 1 - depth];
        std::cout << "  #" << depth << " " << entry.function  << " at " << entry.file << ":" << entry.line << ":" << entry.column << "\n";
    }

#ifndef __TEST__
    __stack_table.clear();
#endif

    _Exit(EXIT_FAILURE);
}