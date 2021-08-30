/**
 * @file source.hpp
 * @author Emanuel Buttaci
 * This file contains classes to handle source files' buffers and information
 * 
 */
#ifndef SOURCE_HANDLER_HPP
#define SOURCE_HANDLER_HPP

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

#include "nemesis/utf8/span.hpp"

namespace nemesis {
    class token;
    class tokenizer;
    class source_file;
    namespace ast { class node; }

    /**
     * Contains information about source file, line and column
     * @note 1 unicode character corresponds to 1 column position
     * @note Line and column positions start from 1 and not 0
     */
    struct source_location {
        /**
         * Line in source file
         */
        unsigned line;
        /**
         * Column in source file
         */
        unsigned column;
        /**
         * Source file name
         * @see source_file
         */
        utf8::span filename;
        /**
         * Default constructors builds an invalid location skipped
         * by the diagnostics
         */
        source_location();
        /**
         * Construct a new source location object
         * @param lineno Line number
         * @param colno Column number
         * @param file File name
         */
        source_location(unsigned lineno, unsigned colno, utf8::span file);
        /**
         * Tests if location is poiting to source file location
         * 
         * @return false If one of the fields is not valid
         * @return false Otherwise
         */
        bool valid() const;
    };

    /**
     * Represents the location range [bline:bcolumn, eline:ecolumn)
     * @note It's an exclusive range for last column character (eline:ecolumn)
     */
    struct source_range {
        /**
         * Begin line in source file
         */
        unsigned bline;
        /**
         * End line in source file
         */
        unsigned eline;
        /**
         * Begin column in source file at bline
         */
        unsigned bcolumn;
        /**
         * End column in source file at eline
         */
        unsigned ecolumn;
        /**
         * Source file name
         * @see source_file
         */
        utf8::span filename;
        /**
         * Default constructor
         */
        source_range();
        /**
         * onstruct a new source range object
         * @param bline Beginning line
         * @param bcol Begininning column
         * @param eline Ending line
         * @param ecol Ending column
         * @param file File name
         */
        source_range(unsigned bline, unsigned bcol, unsigned eline, unsigned ecol, utf8::span file);
        /**
         *  Constructs a source range from a source location and width
         * @note The range is a source line range because it is by default
         * built on the same line as `begin`
         * @param begin Starting location
         * @param cols Number of columns
         * @return Source range
         */
        source_range(source_location begin, unsigned cols);
        /**
         *  Constructs a source range from two source locations
         * @note The range can span multiple lines
         * @param begin Starting location
         * @param end Ending location, excluded
         * @return Source range
         */
        source_range(source_location begin, source_location end);
        /**
         * @return Source location of the beginning of the range
         */
        source_location begin() const;
        /**
         * @return Source location of the end of the range
         */
        source_location end() const;
        /**
         * Sets source location of the beginning of the range
         * @param loc Beginning location
         * @return Reference to itself
         */
        source_range& begin(source_location loc);
        /**
         * Sets source location of the end of the range
         * @param loc Ending location
         * @return Reference to itself
         */
        source_range& end(source_location loc);
    };

    /**
     * It loads files' contents into internal cache
     */
    class source_handler {
    public:
        source_handler(const source_handler&) = delete;
        void operator=(const source_handler&) = delete;
        /**
         * Returns global instance using singleton pattern for simplicity
         */
        static source_handler& instance();
        /**
         * Deallocates all source files' buffers (if any)
         */
        ~source_handler();
        /**
         * Reads a file into cache
         * 
         * @param filename Path name of file
         * @return true On success
         * @return false On failure
         */
        bool load(utf8::span filename);
        /**
         * Deallocates source file linked to the file id
         * 
         * @param filename File name
         */
        void remove(utf8::span filename);
        /**
         * Retrieves source file
         * 
         * @param filename File name
         * @return Source file object reference
         */
        source_file& get(utf8::span filename) const;
        /**
         * Returns the vector of source file objects
         * 
         * @return Source files
         */
        const std::unordered_map<utf8::span, source_file*>& sources() const;
        /**
         * Returns the vector of cpp source file objects
         * 
         * @return Source files
         */
        const std::unordered_map<utf8::span, source_file*>& cppsources() const;
    private:
        /**
         * Construct a new source handler object
         */
        source_handler();
        /**
         * Vector of source file pointers, each index is a file id
         */
        std::unordered_map<utf8::span, source_file*> files_;
        /**
         * Vector of cpp source file pointer
         */
        std::unordered_map<utf8::span, source_file*> cpp_files_;
    };

    /**
     * A source file holds a buffer with source file content
     * @note When source file is destroyed, its buffer is deallocated automatically
     */
    class source_file {
        friend class tokenizer;
        friend class source_handler;
        friend class scanner;
        /**
         * Raw buffer holding content
         */
        struct buffer {
            /**
             * Pointer to raw data
             */
            byte *data;
            /**
             * Number of raw bytes
             */
            int size;
            /**
             * Allocates data buffer on the heap
             * 
             * @param size Number of bytes to allocate
             */
            buffer(int size);
            /**
             * Automatically deallocates data
             */
            ~buffer();
        };
    public:
        /**
         * Kind of source file
         */
        enum class filetype { header, cpp, nemesis, other };
        /**
         * Disable for memory management
         */
        source_file(const source_file&) = delete;
        /**
         * Disable for memory management
         */
        source_file& operator=(const source_file&) = delete;
        /**
         * @return Source handler owner
         */
        source_handler& get_source_handler() const;
        /**
         * Returns source file name
         *
         * @return File name
         */
        utf8::span name() const;
        /**
         * Returns source content
         * 
         * @return File content as UTF-8 encoded buffer
         */
        utf8::span source() const;
        /**
         * @return Number of lines (separated by `\n`) inside the source file
         */
        std::size_t lines_count() const;
        /**
         * Fetches requested line
         * 
         * @param index Line number (starts from 1)
         * @return Source line as UTF-8 encoded buffer
         */
        utf8::span line(unsigned index) const;
        /**
         * Gets the specified characters range
         * 
         * @param rng Characters range
         * @return Range of characters
         */
        utf8::span range(source_range rng) const;
        /**
         * Sets source unit declaration for AST
         * 
         * @param ast Associated abstract syntax tree
         */
        void ast(std::shared_ptr<ast::node> ast);
        /**
         * Gets source unit declaration for AST
         */
        std::shared_ptr<ast::node> ast() const;
        /**
         * Sets file as builtin
         */
        void builtin(bool flag) { builtin_ = flag; }
        /**
         * @return True if file is builtin
         */
        bool builtin() const { return builtin_; }
        /**
         * Test file type
         * @return True if file is of that type
         */
        bool has_type(filetype type) const;
    private:
        /**
         * Construct a new source file and
         * allocates the internal buffer
         * @param handler Source handler who owns this file
         * @param id Source file id
         * @param name Source file name
         * @param size Source file size in bytes
         */
        source_file(source_handler& handler, utf8::span name, int size);
        /**
         * Source handler owner
         */
        source_handler *handler_;
        /**
         * Source file name
         */
        utf8::span name_;
        /**
         * Source file buffer
         */
        buffer buffer_;
        /**
         * Table of source lines
         */
        std::vector<utf8::span> line_table_;
        /**
         * Source unit declaration associated after parsing, so AST for this file
         */
        std::shared_ptr<ast::node> ast_ = nullptr;
        /**
         * Flag for builtin files
         */
        bool builtin_ = false;
    };
}

std::ostream& operator<<(std::ostream& os, nemesis::source_location loc);

#endif // SOURCE_HANDLER_HPP