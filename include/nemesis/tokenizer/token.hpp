/**
 * @file token.hpp
 * @author Emanuel Buttaci
 * 
 * This file contains the token class and related functions.
 */
#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>
#include <ostream>

#include "nemesis/diagnostics/diagnostic.hpp"
#include "nemesis/source/source.hpp"

namespace nemesis {
    /**
     * A token represents the smallest unit of
     * a language because it is a lexeme with
     * associated information about its source
     */
    class token {
        friend class builder;
    public:
        /**
         * Token kind specifies whether a token
         * is an operator, identifier, keyword or
         * others.
         */
        enum class kind : byte {
            /** unicode identifier */
            identifier = 0,
            /** character literal */
            char_literal,
            /** string literal */
            string_literal,
            /** integer literal */
            integer_literal,
            /** realing-point literal */
            real_literal,
            /** imaginary literal with 'i' or 'j' suffix */
            imag_literal,
            /** single-line or multi-line comment */
            comment,
            /** 'as' **/
            as_kw,
            /** 'behaviour' */
            behaviour_kw,
            /** 'break' */
            break_kw,
            /** `concept` */
            concept_kw,
            /** 'const' */
            const_kw,
            /** 'continue' */
            continue_kw,
            /** 'else' */
            else_kw,
            /** 'ensure' */
            ensure_kw,
            /** 'extend' */
            extend_kw,
            /** 'extern' */
            extern_kw,
            /** 'false', which is a boolean literal */
            false_kw,
            /** 'for' */
            for_kw,
            /** 'function' */
            function_kw,
            /** 'hide' */
            hide_kw,
            /** 'if */
            if_kw,
            /** 'in' */
            in_kw,
            /** 'invariant' */
            invariant_kw,
            /** `is` */
            is_kw,
            /** 'mutable' */
            mutable_kw,
            /** 'nucleus' */
            nucleus_kw,
            /** `range` */
            range_kw,
            /** 'require' */
            require_kw,
            /** 'return' */
            return_kw,
            /** 'when' */
            when_kw,
            /** 'static' */
            static_kw,
            /** 'test' */
            test_kw,
            /** 'true', which is a boolean literal */
            true_kw,
            /** `type` */
            type_kw,
            /** 'union' */
            union_kw,
            /** `use` */
            use_kw,
            /** 'var' */
            val_kw,
            /** '.' */
            dot,
            /** ':' */
            colon,
            /** ';' */ 
            semicolon,
            /** ',' */
            comma,
            /** '..' */
            dot_dot,
            /** '..=' */
            dot_dot_equal,
            /** '...' */
            ellipsis,
            /** '=>' **/
            equal_greater,
            /** '(' */
            left_parenthesis,
            /** ')' */
            right_parenthesis,
            /** '[' */
            left_bracket,
            /** ']' */
            right_bracket,
            /** '{' */
            left_brace,
            /** '}' */
            right_brace,
            /** '+' */
            plus,
            /** '-' */
            minus,
            /** '**' */
            star_star,
            /** '*' */
            star,
            /** '/' */
            slash,
            /** '%' */
            percent,
            /** '!' */
            bang,
            /** '>>' */
            greater_greater,
            /** '<<' */
            less_less,
            /** '<' */
            less,
            /** '>' */
            greater,
            /** '<=' */
            less_equal,
            /** '>=' */
            greater_equal,
            /** '==' */
            equal_equal,
            /** '!=' */
            bang_equal,
            /** '&&' */
            amp_amp,
            /** '||' */
            line_line,
            /** '=' */
            equal,
            /** '**=' */
            star_star_equal,
            /** '*=' */
            star_equal,
            /** '/=' */
            slash_equal,
            /** '%=' */
            percent_equal,
            /** '+=' */
            plus_equal,
            /** '-=' */
            minus_equal,
            /** '<<=' */
            left_left_equal,
            /** '>>=' */
            right_right_equal,
            /** '&=' */
            amp_equal,
            /** '|=' */
            line_equal,
            /** '^=' */
            caret_equal,
            /** '++' */
            plus_plus,
            /** '--' */
            minus_minus,
            /** '~' */
            tilde,
            /** '&' */
            amp,
            /** '|' */
            line,
            /** '^' */
            caret,
            /** end of file indicator */
            eof,
            /** used for uninitialized token */
            unknown,
            /** total count of tokens kind */
            nkinds
        };

        class builder;

        /** 
         * Builds an empty token 
         */
        token();
        /**
         * Construct a new token
         * 
         * @param k token kind
         * @param lexeme lexeme of text
         * @param loc source location in text
         */
        token(enum kind k, utf8::span lexeme, source_location loc);
        /**
         * @return token kind
         */
        enum kind kind() const;
        /**
         * @return token lexeme
         */
        utf8::span lexeme() const;
        /**
         * @return token source location
         */
        source_location location() const;
        /**
         * @note This must be a source line range
         * @return Source line range of token
         */
        source_range range() const;
        /**
         * Display information as string
         * 
         * @return String information
         */
        std::string description() const;
        /**
         * Tests if token's kind is this
         * 
         * @param k token kind to test
         * @return true if k == kind()
         * @return false otherwise
         */
        bool is(enum kind k) const;
        /**
         * @return true if token is a literal
         * @return false otherwise
         */
        bool is_literal() const;
        /**
         * @return true if token is a keyword
         * @return false otherwise
         */
        bool is_keyword() const;
        /**
         * @return true if token is a unary operator
         * @return false otherwise
         */
        bool is_unary_operator() const;
        /**
         * @return true if token is a binary operator
         * @return false otherwise
         */
        bool is_binary_operator() const;
        /**
         * @return true if token is an assignment operator
         * @return false otherwise
         */
        bool is_assignment_operator() const;
        /**
         * @return true if token is a generic operator
         * @return false otherwise
         */
        bool is_operator() const;
        /**
         * This bit-field is 1 if token is in the last position
         * of its line, so that it precedes newline or null terminator
         */
        unsigned eol : 1;
        /**
         * This bit-field is set to 0 if token is not valid, for example
         * an empty or too large character, a corrupted number literal, an
         * unterminated string or others
         */
        unsigned valid : 1;
        /**
         * This bit-field tells whether token is injected by the compiler
         * to make parsing process easier.
         * The most useant case is the one of an interpolated string
         * 
         * For example `"Hello {name1}, my name is {name2}"` string literal 
         * is converted in the following artificial tokens:
         * 
         * `__format` `(` `"Hello ?, my name is ?"` `,` `name1` `,` `name2` `)`
         * 
         * which will be a call expression for the parser
         */
        unsigned artificial : 1;
    private:
        /**
         * Token kind
         */
        enum kind kind_ = kind::unknown;
        /**
         * Token lexeme
         */
        utf8::span lexeme_;
        /**
         * Token source location
         */
        source_location location_;
    };

    /**
     * This class helps building a token
     */
    class token::builder {
    public:
        /**
         * Constructs a token builder
         */
        builder() = default;
        /**
         * Sets token kind
         * 
         * @param k token kind
         * @return reference to itself
         */
        builder& kind(enum token::kind k);
        /**
         * Sets token lexeme
         * 
         * @param lexeme token lexeme
         * @return reference to itself
         */
        builder& lexeme(utf8::span lexeme);
        /**
         * Sets token source location
         * 
         * @param loc token location in source text
         * @return reference to itself
         */
        builder& location(source_location loc);
        /**
         * Sets token eol bit-field
         * @note By default it is set to 0
         * @param flag Bit field value to set
         * @return reference to itself
         */
        builder& eol(bool flag = true);
        /**
         * Sets token valid bit-field
         * @note By default it is set to 1
         * @param flag Bit field value to set
         * @return reference to itself
         */
        builder& valid(bool flag = true);
        /**
         * Sets token artificial flag
         * @note By default is is set to 0
         * @param flag Bit-field to set
         * @return reference to itself
         */
        builder& artificial(bool flag = true);
        /**
         * @return constructed token 
         */
        token build() const;
    private:
        /**
         * Token object to build
         */
        token token_;
    };

    namespace impl {
        /**
         * Gets the name for a specific token kind
         * 
         * @param k token kind
         * @return token kind as string 
         */
        std::string to_string(enum token::kind k);
    }
}

/**
 * Compares two token for equality
 */
bool operator==(const nemesis::token& lhs, const nemesis::token& rhs);

/**
 * Compares two token for disequality
 */
bool operator!=(const nemesis::token& lhs, const nemesis::token& rhs);

/**
 * Prints a token to the output stream
 * 
 * @param stream reference to the stream
 * @param t token
 * @return reference to the stream
 */
std::ostream& operator<<(std::ostream& stream, nemesis::token t);

#endif // TOKEN_HPP