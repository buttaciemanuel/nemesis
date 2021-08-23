/**
 * @file tokenizer.hpp
 * @author Emanuel Buttaci
 * 
 * This file contains the tokenizer class used to divide text into tokens
 * 
 */
#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <list>

#include "nemesis/diagnostics/diagnostic.hpp"
#include "nemesis/tokenizer/token.hpp"

namespace nemesis {
    /**
     * The tokenizer act between the source text and the parser.
     * Its aim is to divide the entire text into tokens which
     * are composed of lexeme, kind and metadata for information.
     * The approach I used is to scan the input buffer once for 
     * all and then returning all the tokens.
     * This choice was made for simplicity and because the number
     * of scanned tokens at a time is not always the same 
     * (for example an invalid token or a interpolated string).
     * The tokenizer injects artificial tokens only to replace a
     * valid interpolated strings with a format function call
     * with proper arguments. 
     */
    class tokenizer {
    public:
        /**
         * A tokenizer is a deterministic finite state automate (DFSA)
         * so a state must be kept in each phase.
         */
        struct state {
            /** Iterator to current character */
            utf8::span::iterator iter;
            /** Current source location (line, column) */
            source_location location;
        };

        /**
         * Default container type for tokens
         */
        using tokens = std::list<token>;

        /**
         * Construct a new tokenizer object
         * 
         * @param file Reference to input file which contains the buffer
         * @param publisher Diagnostic publisher
         */
        tokenizer(source_file& file, diagnostic_publisher& publisher);
        /**
         * @return Reference to input file
         */
        source_file& get_source_file() const;
        /**
         * @return Reference to diagnostic publisher
         */
        diagnostic_publisher& get_diagnostic_publisher() const;
        /**
         * @return Current scanning state
         */
        struct state state() const;
        /**
         * Sets beginning state
         * 
         * @param s State from which to start scanning
         */
        void start(struct state s);
        /**
         * Sets ending state
         * 
         * @param s State at which to end scanning
         */
        void end(struct state s);
        /**
         * Sets current state
         * 
         * @param s New current state
         */
        void restore(struct state s);
        /**
         * Divides the input file in tokens saved in `tokens`
         * @note The scanning begins at `start` state and terminates at `end` state
         * 
         * @param tokens Reference to the container of the scanned tokens 
         * @param inject_eof True for EOF token to be injected inside the tokens
         */
        void tokenize(tokens& tokens, bool inject_eof = true);
    private:
        /**
         * Initializes the tokenizer and sets the default start and end state.
         * Eventually skips UTF-8 BOM at the beginning of file. 
         * @note This is called once when the tokenizer is constructed.
         */
        void init();
        /**
         * @return true If last token was added and we are at end of input buffer
         * @return false End of input buffer not yet reached
         */
        bool eof() const;
        /**
         * @return true If a newline is ahead of the current token (whitespaces are not considered)
         * @return false There are other tokens on the same line after current token
         */
        bool eol() const;
        /**
         * @note It does not advance
         * @return Current unicode character (code point) 
         */
        codepoint current() const;
        /**
         * @note It does not advance
         * @return Next unicode character (code point) 
         */
        codepoint next() const;
        /**
         * Extracts a reference to the input buffer in [begin, end)
         * 
         * @param begin Starting state (included)
         * @param end Ending state (excluded)
         * @return Reference to input buffer as UTF-8 encoded span
         */
        utf8::span extract(struct state begin, struct state end);
        /**
         * Builds a source range in [begin, end) even amoung different lines
         * 
         * @param begin Starting state (included)
         * @param end Ending state (excluded)
         * @return Source range object
         */
        source_range range(struct state begin, struct state end);
        /**
         * Sets explicity the tokens container for the input buffer
         * 
         * @param tokens Reference to input buffer
         */
        void set_tokens(std::list<token> *tokens);
        /**
         * Gets the reference to the tokens container for the input buffer
         * 
         * @return Reference to the tokens container
         */
        tokens *get_tokens() const;
        /**
         * Advances of one unicode character in input
         */
        void advance();
        /**
         * Scans and skip an escape sequence
         * 
         * @return true If current characters are an escape sequence
         * @return false Otherwise
         */
        bool escape();
        /**
         * Scans a character literal and eventually adds it to the tokens
         * 
         * @return true If literal was correctly recognized
         * @return false Otherwise
         */
        bool character();
        /**
         * Scans a string literal and eventually adds it to the tokens
         * 
         * @return true If literal was correctly recognized
         * @return false Otherwise
         */
        bool string();
        /**
         * Scans a number literal and eventually adds it to the tokens
         * 
         * @return true If literal was correctly recognized
         * @return false Otherwise
         */
        bool number();
        /**
         * Scans a identifier or keyword and eventually adds it to the tokens
         * 
         * @return true If identifier or keyword was correctly recognized
         * @return false Otherwise
         */
        bool identifier();
        /**
         * Scans an operator or punctuation symbol and eventually adds it to the tokens
         * 
         * @return true If symbol was correctly recognized
         * @return false Otherwise
         */
        bool symbol();
        /**
         * Skips a comment (single line or multi line)
         * 
         * @return true If comment was properly formed 
         * @return false Otherwise
         */
        bool comment();
        /**
         * Skips whitespaces
         * 
         * @return true If whitespaces were skipped
         * @return false Otherwise
         */
        bool whitespace();
        /**
         * Reaches end of interpolated expression ( `}` ) or end of line
         * @note If the end was reached correctly then begin state is restored,
         * otherwise error state is kept for error reporting.
         * @param end Reference to end of interpolated expression
         * @return true If interpolated expression was properly formed
         * @return false Otherwise
         */
        bool reach_end_of_interpolation(struct state& end);
        /**
         * Scans an entire interpolated expression and extracts its tokens
         * inside `expression` list. Empty expressions are signaled as errors.
         * The tokens inside the expression will then be injects into the main
         * tokens list. This is called from the string() context
         * 
         * @param expression Reference to container of tokens inside the interpolated expression
         * @return true If interpolated expression was properly formed
         * @return false Otherwise
         */
        bool interpolation(std::list<token>& expression);
        /**
         * @param val Unicode code point
         * @return true If code point is ascii underscore
         * @return false Otherwise 
         */
        static bool is_underscore(codepoint val);
        /**
         * @param val Unicode code point
         * @return true If code point is ascii letter
         * @return false Otherwise 
         */
        static bool is_letter(codepoint val);
        /**
         * @param val Unicode code point
         * @return true If code point is decimal digit
         * @return false Otherwise
         */
        static bool is_digit(codepoint val);
        /**
         * @param val Unicode code point
         * @return true If code point is binary digit
         * @return false Otherwise
         */
        static bool is_bin_digit(codepoint val);
        /**
         * @param val Unicode code point
         * @return true If code point is octal digit
         * @return false Otherwise
         */
        static bool is_oct_digit(codepoint val);
        /**
         * @param val Unicode code point
         * @return true If code point is hexadecimal digit
         * @return false Otherwise
         */
        static bool is_hex_digit(codepoint val);
        /**
         * @param val Unicode code point
         * @note Read the reference to understand which
         * language scripts are considered valid as input
         * source.
         * @return true If code point is valid unicode character
         * @return false Otherwise
         */
        static bool is_unicode_character(codepoint val);
        /**
         * @param val Unicode code point
         * @return true If code point is valid identifier start
         * @return false Otherwise
         */
        static bool is_unicode_identifier_start(codepoint val);
        /**
         * @param val Unicode code point
         * @return true If code point is valid identifier part
         * @return false Otherwise
         */
        static bool is_unicode_identifier_part(codepoint val);
        /**
         * @param val Unicode code point
         * @return true If code point is whitespace
         * @return false Otherwise
         */
        static bool is_whitespace(codepoint val);
        /**
         * @param val Unicode code point
         * @return true If code point is newline
         * @return false Otherwise
         */
        static bool is_newline(codepoint val);
        /**
         * @param val Unicode code point
         * @return true If code point is string start (")
         * @return false Otherwise
         */
        static bool is_string_quote(codepoint val);
        /**
         * @param val Unicode code point
         * @return true If code point is char start (')
         * @return false Otherwise
         */
        static bool is_character_quote(codepoint val);
        /**
         * @param val Unicode code point
         * @return true If code point is Nemesis operator or punctuation
         * @return false Otherwise
         */
        static bool is_symbol(codepoint val);
        
        /** Source file reference, which contains the input buffer */
        source_file& file_;
        /** Diagnostic publisher */
        diagnostic_publisher& publisher_;
        /** Starting state from which scanning begins */
        struct state start_;
        /** Ending state at which scanning ends */
        struct state end_;
        /** Current state */
        struct state state_;
        /** Reference to token list to which add scanned tokens */
        tokens *tokens_ = nullptr;
    };
}

#endif // TOKENIZER_HPP