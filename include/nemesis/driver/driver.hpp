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

namespace nemesis {
    /**
     * The driver handles the whole toolchain which comprehends
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
         * Standard library pathnames
         */
        static const char* builtin_files[];
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
                 * Just compiles files without running
                 */
                compile = 0x8,
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
         * @return Driver executable pathname 
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
         * Exit code
         */
        int exit_code_;
        /**
         * Argument options
         */
        options options_;
        /**
         * Run-time arguments to pass if compile option is not set
         */
        std::vector<const char*> runtime_args_;
        /**
         * Driver executable pathname
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