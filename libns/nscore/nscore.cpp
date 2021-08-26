#include <iostream>
#include <fstream>
#include <mutex>
#include <unordered_map>

#include "nscore.h"

// stack table which is thread local
thread_local static std::stack<__stack_entry> __stack_table;
// file content table to print stack traces in a pretty way
static std::unordered_map<std::string, const char*> __sources;
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

    __sources[std::string(path)] = buffer;

    stream.close();

    return true;
}

static std::string __get_line_from_source(const char* path, unsigned line, unsigned column)
{
    if (__sources.count(path) == 0 && !__load_source_file(path)) return {};

    int i = 1;                                                                                                            
    const char* start = __sources[std::string(path)], *end;                                                                                           
  
    while (*start != '\0' && i < line) {
        while (*start != '\0' && *start != '\n') ++start;       
        ++i;
        ++start;
    }

    if (*start == '\0') return {};

    end = start;

    while (*end != '\0' && *end != '\n') ++end;

    std::string result(std::size_t(end - start), 0);

    std::copy(start, end, result.begin());

    return result;
}

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

void nscore_assert(bool condition, std::string message, const char* file, int line, int column) 
{
    if (condition) return;
    if (!file || line <= 0) std::cerr << "• violation";
    else std::cerr << "• violation at " << file << ":" << line << ":" << column;
    if (!message.empty()) std::cerr << ": " << message << '\n';
    nscore_stacktrace();
    std::exit(EXIT_FAILURE);
}

void nscore_exit(std::int32_t code) { std::exit(code); }

void nscore_stacktrace()
{
    if (__stack_table.empty()) return;

    std::unique_lock<std::mutex> lock(__sources_mutex);

    std::cout << "• stack traces\n";

    for (unsigned int depth = 0; !__stack_table.empty(); __stack_table.pop(), ++depth) {
        __stack_entry entry = __stack_table.top();
        std::string line = __get_line_from_source(entry.file, entry.line, entry.column);
        std::cout << "  #" << depth << " " << entry.function  << " at " << entry.file << ":" << entry.line << ":" << entry.column << "\n";
        if (!line.empty()) std::cout << " -> " << line << '\n';
    }
}