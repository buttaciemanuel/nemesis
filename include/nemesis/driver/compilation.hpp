#ifndef COMPILATION_HPP
#define COMPILATION_HPP

#include <list>
#include <string>

namespace nemesis {
    class compilation {
    public:
        struct source {
            std::string path;
            std::string content;
            source(std::string path, std::string content) : path(path), content(content) {}
        };

        compilation(diagnostic_publisher& publisher) : publisher_(publisher) {}
        void source(std::string path, std::string content) { sources.emplace_back(path, content); }
        void builtin(std::string path) { builtins.emplace_back(path, std::string()); }
        int compile(std::string output = "a.out");
        int run(std::vector<const char*> args = {});
    private:
        diagnostic_publisher& publisher_;
        std::list<struct source> builtins;
        std::list<struct source> sources;
    };
}

#endif