/**
 * @file pm.hpp
 * 
 * This file contains basic procedures behind the engine
 * of Nemesis package manager
 */
#ifndef PM_HPP
#define PM_HPP

#include <list>
#include <regex>
#include <unordered_map>

#include "nemesis/driver/compilation.hpp"

namespace nemesis {
    /**
     * Package manager namespace
     */
    namespace pm {
        /**
         * Verifies that value is a string
         */
        inline bool is_valid_string_value(std::string value) { return value.size() > 1 && ((value.front() == '\'' && value.back() == '\'') || (value.front() == '\"' && value.back() == '\"')); }
        /**
         * Verifies that value is a boolean
         */
        inline bool is_valid_boolean_value(std::string value) { return value == "true" || value == "false"; }
        /**
         * Verifies that value is a hex digest
         */
        inline bool is_valid_hash_value(std::string value) { return /* FOR MD5 std::regex_match(value, std::regex("^[0-9|a-f|A-F]{32}$"))*/std::regex_match(value, std::regex("^[0-9|a-f|A-F]+$")); }
        /**
         * Verifies that package name is valid
         */
        inline bool is_valid_package_name(std::string name) { return std::regex_match(name, std::regex("^([A-z|a-z]|_)([1-9|A-z|a-z]|_)*$")); }
        /**
         * Verifies that package version is valid
         */
        inline bool is_valid_package_version(std::string version) { return std::regex_match(version, std::regex("^(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)$")); }
        /**
         * Compare two version numbers
         * @return 1 if a > b
         * @return 0 if a == b
         * @return -1 if a < b
         */
        int compare_version(std::string a, std::string b);
        /**
         * Exception used for errors
         */
        struct exception {};
        /**
         * A dependency of current package
         */
        struct package {
            /**
             * Name of depedency package
             */
            std::string name;
            /**
             * Version of depedency package
             */
            std::string version;
        };
        /**
         * Manifest file information
         */
        struct manifest {
            /**
             * Current package may be an application or library
             */
            enum kind { app, lib, none } kind = kind::none;
            /**
             * Name of current package
             */
            std::string name;
            /**
             * Version of current package
             */
            std::string version;
            /**
             * When a package is builtin, then its `.ns` source files
             * inside `src` directory are not compiled to C++, just its 
             * cpp files inside `.cpp` get involved
             */
            bool builtin = false;
            /**
             * List of dependencies of current package, no particular order
             */
            std::unordered_map<std::string, package> dependencies;
        };
        /**
         * Lock file information 
         */
        struct lock {
            /**
             * Information in lock file about a package
             */
            struct info {
                /**
                 * Name of package
                 */
                std::string name;
                /**
                 * Version of package
                 */
                std::string version;
                /**
                 * Tells if builtin for compilation
                 */
                bool builtin;
                /**
                 * Hash (hex string) of package
                 */
                std::string hash;
                /**
                 * Path of package on local disk
                 */
                std::string path;
            };
            /**
             * Current package may be an application or library
             */
            enum manifest::kind kind;
            /**
             * Current package information
             */
            info package;
            /**
             * List of dependencies in order of compilation, obtained
             * form topological sort of dependency graph
             */
            std::list<info> dependencies;
        };
        /**
         * Dependency graph, used for downloading packages, resolving conflicts and construct lock file
         */
        struct dependency_graph {
            /**
             * A node contains info about its package and its dependencies
             */
            struct node {
                /**
                 * Package name and version
                 */
                pm::package package;
                /**
                 * Package dependencies
                 */
                std::list<pm::package> edges;
            };
            /**
             * Constructs empty graph
             */
            dependency_graph() = default;
            /**
             * List of nodes associated to their name, without version, so we can found duplicates of different versions (all resolved)
             */
            std::unordered_map<std::string, node> nodes;
            /**
             * Topological sort for downloading and compiling
             */
            std::list<pm::lock::info> topological;
        };
        /**
         * Package manage class
         */
        class manager {
        public:
            /**
             * Path for cached packages' archives
             */
            static constexpr const char cache_path[] = ".cache";
            /**
             * Manifest file path
             */
            static constexpr const char manifest_path[] = "nemesis.manifest";
            /**
             * Lock file path
             */
            static constexpr const char lock_path[] = "nemesis.lock";
            /**
             * Dependencies directory path
             */
            static constexpr const char dependencies_path[] = "libs";
            /**
             * Path for Nemesis source files
             */
            static constexpr const char sources_path[] = "src";
            /**
             * Path for C++ source files
             */
            static constexpr const char cpp_sources_path[] = "cpp";
            /**
             * Path for executable file
             */
            static constexpr const char executable_path[] = "application";
            /**
             * Global instance for curl management
             */
            static manager& instance(diagnostic_publisher& publisher, source_handler& handler);
            /**
             * Destructor for global instance
             */
            ~manager();
            /**
             * Parse a manifest file and constructs a manifest information
             * @return Manifest information
             */
            manifest parse_manifest_file(std::string path) const;
            /**
             * Parse a lock file and constructs a lock information
             * @return Lock information
             */
            lock parse_lock_file(std::string path) const;
            /**
             * Resolve dependencies from manifest file and generates a writes lock file
             * @return Lock file information
             */
            lock generate_lock_file(pm::manifest manifest, std::string where);
            /**
             * Install a new dependency, generates new lock file and writes it on disk
             */
            lock add_dependency(pm::manifest manifest, std::string lockpath, std::string name, std::string version = {});
            /**
             * Remove a dependency, generates new lock file and writes it on disk
             */
            lock remove_dependency(pm::manifest manifest, std::string lockpath, std::string name);
            /**
             * Constructs compilation chain from lock file
             */
            compilation build_compilation_chain(pm::lock lockfile) const;
            /**
             * Restore old manifest file if any
             * Old manifest would be set by those calls that could change it, like `add_dependency`, `remove_dependency`
             */
            void restore() const;
        private:
            /**
             * Constructs a package manager
             * @param publisher Diagnostic publisher
             */
            manager(diagnostic_publisher& publisher, source_handler& handler);
            /**
             * Prints an error
             */
            template<typename... Args>
            void error(std::string format, Args... args) const 
            { 
                publisher_.publish(diagnostic::builder().severity(diagnostic::severity::error).message(diagnostic::format(format, args...)).build());
                throw pm::exception(); 
            }
            /**
             * Prints a warning
             */
            template<typename... Args>
            void warning(std::string format, Args... args) const { publisher_.publish(diagnostic::builder().severity(diagnostic::severity::warning).message(diagnostic::format(format, args...)).build()); }
            /**
             * Prints a message
             */
            template<typename... Args>
            void message(std::string format, Args... args) const { publisher_.publish(diagnostic::builder().severity(diagnostic::severity::none).message(diagnostic::format(format, args...)).build()); } 
            /**
             * Parse a manifest file from file buffer and constructs a manifest information
             * @return Manifest information
             */
            manifest parse_manifest_file_from_buffer(std::istream& stream) const;
            /**
             * Unzip package <package> archive at <path> and return its manifest file
             */
            manifest unzip_package_manifest(std::string package, std::string path);
            /**
             * Downloads package and stores it cache directory and return its dependencies
             */
            std::list<pm::package> download_package(pm::package& package, pm::lock::info& info);
            /**
             * Extracts all files of package archive at <archive> into <to> directory
             */
            void extract_package_archive(std::string archive, std::string to);
            /**
             * Recursive depth first search with package dependencies to build dependency graph
             */
            void dfs(dependency_graph& graph, std::set<std::string>& visited, package current);
            /**
             * Constructs dependency tree from manifest file and creates lock file
             */
            dependency_graph resolve(pm::manifest manifest);
            /**
             * Prints out manifest file given information
             */
            void dump_manifest_file(pm::manifest manifest, std::string path) const;
            /**
             * Prints out lock file given information
             */
            void dump_lock_file(pm::lock lock, std::string path) const;
            /**
             * Adds core library to compilation chain when this is not specified otherwise
             */
            void load_core_library(compilation& compilation) const;
            /**
             * Open all sources files beloging to package
             */
            void load_package_workspace(compilation& compilation, pm::lock::info package, pm::lock lockfile, bool is_dependency = false) const;
            /**
             * Diagnostic publisher
             */
            diagnostic_publisher& publisher_;
            /**
             * Source handler
             */
            source_handler& source_handler_;
            /**
             * Manifest info which is stored before change and may be restored later
             */
            manifest restored_;
        };
    }
}

#endif