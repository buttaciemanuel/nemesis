#ifndef COMPILATION_HPP
#define COMPILATION_HPP

#include <list>
#include <string>

#include "nemesis/source/source.hpp"

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

    class Compilation {
        using sources = std::list<source_file*>;
        struct node {
            std::string name;
            std::string version;
            sources sources;
            
            static node workspace(Compilation::sources sources) { return node{"", "", sources}; }
            static node library(std::string name, std::string version, Compilation::sources sources) { return node{name, version, sources}; }
        };
    public:
        Compilation(diagnostic_publisher& publisher) : publisher_(publisher) {}
        diagnostic_publisher& publisher() const { return publisher_; }
        /**
         * Compile current workspace as a normal application
         */
        void workspace(sources sources) { workspace_ = node::workspace(sources); }
        /**
         * Compile current workspace as a library
         */
        void library(std::string name, std::string version, sources sources) { workspace_ = node::library(name, version, sources); }
        /**
         * Adds a dependency library to current workspace
         */
        void dependency(std::string name, std::string version, sources sources) { dependencies_.push_back(node::library(name, version, sources)); }
        /**
         * @return Current workspace
         */
        node workspace() const { return workspace_; }
        /**
         * @return List of dependencies
         */
        std::list<node> dependencies() const { return dependencies_; }
    private:
        diagnostic_publisher& publisher_;
        node workspace_;
        std::list<node> dependencies_;
    };
}

#endif