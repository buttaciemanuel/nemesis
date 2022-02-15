#ifndef COMPILATION_HPP
#define COMPILATION_HPP

#include <fstream>
#include <list>
#include <string>
#include <memory>

#include "nemesis/source/source.hpp"

/** to remove **/ #include <iostream>

namespace nemesis {
    namespace ast {
        struct workspace;
    }

    class compilation {
    public:
        /**
         * Default name of executable file
         */
        static constexpr const char executable_name[] = "application";
        /**
         * List of source file
         */
        using sources = std::list<source_file*>;
        /**
         * compilation unit for target code
         */
        struct target {
            /**
             * compilation unit name
             */
            std::string name;
            /**
             * compilation unit content
             */
            std::string content;
            /**
             * If header, then file is created but not compiled
             */
            bool header = false;
        };
        /**
         * A package represents a node inside the dependency graph
         */
        struct package {
            /**
             * Name of package (library or application)
             */
            std::string name;
            /**
             * Package version
             */
            std::string version;
            /**
             * List of source files
             */
            compilation::sources sources;
            /**
             * List of cpp source files
             */
            compilation::sources cpp_sources;
            /**
             * Package kind, which is application or library
             */
            enum class kind { app, lib, none } kind = kind::none;
            /**
             * When a package is builtin, it means that its Nemesis source files in directory `src` are not translated in C++ code, but
             * its C++ source files in directory `cpp` are directly used. Otherwise, when builtin is not set, as default, all definitions
             * inside `src` source files are translated to their proper C++ compilation units, and compiled along with sources included
             * into `cpp` directory.
             * So basically `builtin = true` excludes `src` source files from being compiled to C++
             */
            bool builtin = false;
            /**
             * Construct a generic package
             */
            static package make(std::string name, std::string version, compilation::sources sources, compilation::sources cpp_sources = {}, bool builtin = false, enum kind kind = kind::none) { return package{name, version, sources, cpp_sources, kind, builtin}; }
        };
        /**
         * Constructs a compilation object
         */
        compilation(diagnostic_publisher& publisher, source_handler& handler) : publisher_(publisher), source_handler_(handler) {}
        /**
         * Construct current workspace
         */
        void current(std::string name, std::string version, sources sources, compilation::sources cpp_sources = {}, bool builtin = false, enum package::kind kind = package::kind::none) { package_ = package::make(name, version, sources, cpp_sources, builtin, kind); }
        /**
         * Adds a dependency library to current workspace
         */
        void dependency(std::string name, std::string version, sources sources, compilation::sources cpp_sources = {}, bool builtin = false) 
        { 
            auto pkg = package::make(name, version, sources, cpp_sources, builtin);
            dependencies_.push_back(pkg); 
            packages_.emplace(name, pkg);
        }
        /**
         * @return Current workspace
         */
        struct package& current() const { return package_; }
        /**
         * @return List of dependencies
         */
        std::list<struct package> dependencies() const { return dependencies_; }
        /**
         * @return Package associated to the package name
         * @note Key is <name>@<version>
         */
        struct package package(std::string name, std::string version) const 
        {
            std::string id = name + "@" + version;
            if (id == package_.name + "@" + package_.version) return package_;
            return packages_.at(id); 
        }
        /**
         * @return Package associated to the package name
         * @note Key is <name>
         */
        struct package package(std::string name) const
        {
            if (name == package_.name) return package_;
            return packages_.at(name);
        }
        /**
         * @return All packages, including current workspace
         */
        std::unordered_map<std::string, struct package> packages() const
        {
            auto result = packages_;
            result.emplace(package_.name, package_);
            return result;
        }
        /**
         * @return Namespaces which are declared as `lib` or `app` inside files
         */
        std::unordered_map<std::string, std::shared_ptr<ast::workspace>>& workspaces() const { return workspaces_; }
        /**
         * @return Diagnostic publisher
         */
        diagnostic_publisher& get_diagnostic_publisher() const { return publisher_; }
        /**
         * @return Source handler
         */
        source_handler& get_source_handler() const { return source_handler_; }
        /**
         * @param targets cpp compilation units
         * Builds the entire compilation chain emitting target code
         */
        bool build(std::list<target> targets) const
        {
            // name of executable file
            std::string executable = compilation::executable_name;
            // C++17 is used as default standard
            // adds builtin file
            std::string command = "g++ -std=c++17 -lm -ggdb3";
            // adds test flag for different behaviour at run-time
            if (test_) command += " -D __TEST__";
            // adds output file if it's an application
            if (package_.kind == package::kind::app) command += " -o " + executable;
            // or just compile if library
            else if (package_.kind == package::kind::lib) command += " -c -fsyntax-only";
            // adds to compilation all cpp targets file generated by code generation
            for (auto target : targets) {
                // creates the cpp file
                std::ofstream outfile(target.name.data());
                // prints its content
                outfile << target.content;
                // concat file path
                if (!target.header) command.append(" ").append(target.name);
            }
            // link in compilation all cpp source files from `cpp` directories of each package
            for (auto package : packages()) for (auto cpp : package.second.cpp_sources) {
                if (!cpp->has_type(source_file::filetype::header)) command.append(" ").append(cpp->name().string());
            }
            // link curl library
            command.append(" -lcurl");

            // std::cout << "-- " << command << " --\n";

            // save exit status
            int status = std::system(command.data());
            // error code
            std::error_code code;
            // remove all temporary cpp compilation units
            for (auto target : targets) std::filesystem::remove(target.name, code);
            // second round is for running compilation command
            if (status != 0) {
                publisher_.publish(diagnostic::builder().severity(diagnostic::severity::error).message("some errors occurred when compiling source files, this is extremely weird, f*ck...").build());
                return false;
            }
            // success
            publisher_.publish(diagnostic::builder().severity(diagnostic::severity::none).message("compilation success, mate!").build());
            // executes test if any and remove file
            if (test_) {
                status = std::system(("./" + executable).data());
                std::remove(executable.data());
                return status == 0;
            }
            // exit with success
            return true;
        }
        /**
         * Set test mode
         */
        void test(bool flag) { test_ = flag; }
        /**
         * Get test mode
         */
        bool test() const { return test_; }
    private:
        /**
         * Diagnostic publisher
         */
        diagnostic_publisher& publisher_;
        /**
         * Source handler
         */
        source_handler& source_handler_;
        /**
         * Current workspace package, which can be a library or simple application
         */
        mutable struct package package_;
        /**
         * List of dependencies in the order to be processed
         */
        std::list<struct package> dependencies_;
        /**
         * Each dependency is registered with this key <name>@<version> or <name>
         */
        std::unordered_map<std::string, struct package> packages_;
        /**
         * This is namespaces map, which associates an entire set of definitions (namespace) to each workspace name (which should be library or app name)
         */
        mutable std::unordered_map<std::string, std::shared_ptr<ast::workspace>> workspaces_;
        /**
         * Test mode is used for generating a different entry point for the execution of tests
         * instead of normal main() entry point. It is false by default
         */
        bool test_ = false;
    };
}

#endif