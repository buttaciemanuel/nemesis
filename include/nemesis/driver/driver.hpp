/**
 * @file driver.hpp
 * @author Emanuel Buttaci
 * This file defines driver class and compilation process
 * 
 */
#ifndef DRIVER_HPP
#define DRIVER_HPP

#include "nemesis/diagnostics/diagnostic.hpp"
#include "nemesis/source/source.hpp"
#include "nemesis/driver/compilation.hpp"

namespace nemesis {
    /**
     * The driver handles the whole toolchain which comprehends
     *     package manager
     *     compiler
     *     assembler
     *     linker
     * and links all those tools together
     */
    class driver {
    public:
        /**
         * Compiler description message
         */
        static const char description[];
        /**
         * Usage message
         */
        static const char usage[];
        /**
         * Help message for options
         */
        static const char help[];
        /**
         * This is the main command which is executed by the driver
         */
        enum class command {
            /**
             * Initialize the workspace directory, creating manifest and lock files
             */
            initialize,
            /**
             * Add a dependency to manifest file and update lock file
             */
            add,
            /**
             * Remove dependency from manifest file and update lock file
             */
            remove,
            /**
             * Build the entire workspace, generating an output if there is an entry point
             */
            build,
            /**
             * Clean workspace removing build information
             */
            clean,
            /**
             * Runs on fly the application if any, but before it builds the entire workspace
             */
            run,
            /**
             * Executes all test of current library
             */
            test
        };
        /**
         * This class encapsulates argument options
         * inside a bitset
         */
        class options {
        public:
            /**
             * Option kind
             */
            enum class kind : unsigned { 
                /**
                 * Display help information and does not compile any file
                 */
                help = 0x1, 
                /**
                 * Prints all the tokens extracted from source files
                 */
                tokens = 0x2, 
                /**
                 * Prints the abstract syntax tree generated from each source file
                 */
                ast = 0x4,
                /**
                 * Dumps stack-trace if the program fails
                 */
                trace = 0x10
            };
            options() = default;
            /**
             * Checks if option is set
             * 
             * @param k Option kind
             */
            bool is(kind k) const;
            /**
             * Sets the option
             * 
             * @param k Option kind
             */
            void set(kind k);
            /**
             * Unsets the option
             * 
             * @param k Option kind
             */
            void clear(kind k);
            /**
             * @return Raw bitset as unsigned integer
             */
            unsigned raw() const;
        private:
            /**
             * Bitset
             */
            unsigned bits_ = 0x0;
        };
        /**
         * Deleted empty constructor
         */
        driver() = delete;
        /**
         * Constructs a new driver bound to the diagnostic publisher
         * 
         * @param argc Number of arguments including executable path
         * @param argv String arguments including executable path
         * @param diagnostic_publisher Reference to diagnostic publisher
         */
        driver(int argc, char **argv, diagnostic_publisher& diagnostic_publisher);
        /**
         * @return Argument options 
         */
        options get_options() const;
        /**
         * @return Command
         */
        command get_command() const;
        /**
         * @return driver executable pathname 
         */
        utf8::span pathname() const;
        /**
         * @return Source handler bound to this driver 
         */
        source_handler& get_source_handler() const;
        /**
         * @return Diagnostic publisher bound to this driver 
         */
        diagnostic_publisher& get_diagnostic_publisher() const;
        /**
         * Runs the driver and start compilation toolchain
         * 
         * @return Exit code of the driver
         */
        int run();
    private:
        /**
         * Parses the executable arguments and extracts source files and options
         * 
         * @param argc Number of arguments
         * @param argv String arguments
         */
        void parse_arguments(int argc, char **argv);
        /**
         * Prints an error
         */
        template<typename... Args>
        void error(std::string format, Args... args) const { diagnostic_publisher_.publish(diagnostic::builder().severity(diagnostic::severity::error).message(diagnostic::format(format, args...)).build()); }
        /**
         * Prints a warning
         */
        template<typename... Args>
        void warning(std::string format, Args... args) const { diagnostic_publisher_.publish(diagnostic::builder().severity(diagnostic::severity::warning).message(diagnostic::format(format, args...)).build()); }
        /**
         * Prints a message
         */
        template<typename... Args>
        void message(std::string format, Args... args) const { diagnostic_publisher_.publish(diagnostic::builder().severity(diagnostic::severity::none).message(diagnostic::format(format, args...)).build()); }
        /**
         * Asks a question
         */
        template<typename InputType>
        void question(std::string question, InputType& input) const 
        { 
            std::cout << "• " << question << " ";    
            std::cin >> input;
        }
        /**
         * Checks that command <command> receives no more arguments than expected, where <count> is those received
         */
        void check_command_arguments_count(enum command command, unsigned count);
        /**
         * Executes 'init' command inside current workspace
         */
        void init();
        /**
         * Executes 'build' command inside current workspace
         */
        void build();
        /**
         * Executes `clean` or `test` command inside current workspace
         */
        void clean();
        /**
         * Executes `add` command inside current workspace
         */
        void add();
        /**
         * Executes `remove` command inside current workspace
         */
        void remove();
        /**
         * Compile source files given a compilation chain
         */
        void compile(class compilation& compilation);
        /**
         * Exit code
         */
        int exit_code_;
        /**
         * driver command
         */
        command command_;
        /**
         * Argument options
         */
        options options_;
        /**
         * Run-time arguments to pass if compile option is not set
         */
        std::vector<const char*> arguments_;
        /**
         * driver executable pathname
         */
        utf8::span pathname_;
        /**
         * Source files handler
         */
        source_handler& source_handler_;
        /**
         * Diagnostic publisher
         */
        diagnostic_publisher& diagnostic_publisher_;
    };
}

#endif // DRIVER_HPP