#include <stack>
#include <unordered_map>
#include <unordered_map>

#include "nemesis/tokenizer/tokenizer.hpp"

#define __NEMESIS_DISCARD_COMMENTS__ 1

namespace nemesis {
    bool tokenizer::is_underscore(codepoint val) { return val == 0x5f; }

    bool tokenizer::is_letter(codepoint val) { return (val >= 0x41 && val <= 0x5a) || (val >= 0x61 && val <= 0x7a); }
    
    bool tokenizer::is_digit(codepoint val) { return val >= 0x30 && val <= 0x39; }
    
    bool tokenizer::is_oct_digit(codepoint val) { return val >= 0x30 && val <= 0x37; }

    bool tokenizer::is_bin_digit(codepoint val) { return val == 0x30 || val == 0x31; }
    
    bool tokenizer::is_hex_digit(codepoint val) 
    { 
        return (val >= 0x30 && val <= 0x39) ||  
                (val >= 0x41 && val <= 0x46) || 
                (val >= 0x61 && val <= 0x66); 
    }
    
    bool tokenizer::is_unicode_character(codepoint val) { return val >= 0x0 && val <= 0x10ffff; }

    bool tokenizer::is_unicode_identifier_start(codepoint val)
    {
        return (val >= 'A' && val <= 'Z') || (val >= 'a' && val <= 'z') || val == '_';
#if false
        /* arabic */
        return (val >= 0x600 && val <= 0x6ff) || (val >= 0x750 && val <= 0x77f) || (val >= 0x8a0 && val <= 0x8ff) || (val >= 0xfb50 && val <= 0xfdff) || (val >= 0xfe70 && val <= 0xfeff) ||
        /* armenian */
        (val >= 0x530 && val <= 0x58f) || (val >= 0xfb00 && val <= 0xfb17) ||
        /* bengali */
        (val >= 0x980 && val <= 0x9ff) ||
        /* bopomofo */
        (val >= 0x3100 && val <= 0x312f) || (val >= 0x31a0 && val <= 0x31bf) ||
        /* cyrillic */
        (val >= 0x400 && val <= 0x4ff) || (val >= 0x500 && val <= 0x52f) || (val >= 0x2de0 && val <= 0x2dff) || (val >= 0xa640 && val <= 0xa69f) || (val >= 0x1c80 && val <= 0x1c8f) ||
        /* devanagari */
        (val >= 0x900 && val <= 0x97f) || (val >= 0xa8e0 && val <= 0xa8ff) ||
        /* ethiopic */
        (val >= 0x1200 && val <= 0x137f) || (val >= 0x1380 && val <= 0x139f) || (val >= 0x2d80 && val <= 0x2ddf) || (val >= 0xab00 && val <= 0xab2f) || (val >= 0x1c80 && val <= 0x1c8f) ||
        /* georgian */
        (val >= 0x10a0 && val <= 0x10ff) || (val >= 0x2d00 && val <= 0x2d2f) || (val >= 0x1c90 && val <= 0x1cbf) ||
        /* greek */
        (val >= 0x370 && val <= 0x3ff) || (val >= 0x1f00 && val <= 0x1fff) ||
        /* gujarati */
        (val >= 0xa80 && val <= 0xaff) ||
        /* gurmukhi */
        (val >= 0xa00 && val <= 0xa7f) ||
        /* han */
        (val >= 0x4e00 && val <= 0x9fcc) || (val >= 0x3400 && val <= 0x4db5) || (val >= 0x20000 && val <= 0x2a6d6) || (val >= 0x2a700 && val <= 0x2b734) || (val >= 0x2b740 && val <= 0x2b81d) ||
        /* hangul */
        (val >= 0xac00 && val <= 0xd7af) || (val >= 0x1100 && val <= 0x11ff) || (val >= 0x3130 && val <= 0x318f) || (val >= 0xa960 && val <= 0xa97f) || (val >= 0xd7b0 && val <= 0xd7ff) ||
        /* hebrew */
        (val >= 0x590 && val <= 0x5ff) || (val >= 0xfb1d && val <= 0xfb4f) ||
        /* hiragana */
        (val >= 0x3040 && val <= 0x309f) || (val >= 0x1b000 && val <= 0x1b0ff) || (val >= 0x1b100 && val <= 0x1b12f) || (val >= 0x1b130 && val <= 0x1b16f) ||
        /* kannada */
        (val >= 0x0c80 && val <= 0x0cff) ||
        /* katakana */
        (val >= 0x30a0 && val <= 0x30ff) || (val >= 0x31f0 && val <= 0x31ff) || (val >= 0x3200 && val <= 0x32ff) || (val >= 0xff00 && val <= 0xffef) || (val >= 0x1b000 && val <= 0x1b0ff) || (val >= 0x1b130 && val <= 0x1b16f) ||
        /* khmer */
        (val >= 0x01780 && val <= 0x17ff) ||
        /* lao */
        (val >= 0xe80 && val <= 0xeff) ||
        /* latin */
        (val >= 0x41 && val <= 0x5a) || (val >= 0x41 && val <= 0x5a) || val == 0x5f || (val >= 0x61 && val <= 0x7a) || (val >= 0x80 && val <= 0xff) || (val >= 0x100 && val <= 0x17f) || (val >= 0x180 && val <= 0x24f) || (val >= 0x250 && val <= 0x2af) || (val >= 0x2b0 && val <= 0x2ff) || (val >= 0x1d00 && val <= 0x1d7f) || (val >= 0x1d80 && val <= 0x1dbf) || (val >= 0x1e00 && val <= 0x1eff) || (val >= 0x2070 && val <= 0x209f) || (val >= 0x2100 && val <= 0x214f) || (val >= 0x2c60 && val <= 0x2c7f) || (val >= 0xa720 && val <= 0xa7ff) || (val >= 0xab30 && val <= 0xab6f) ||
        /* malayalam */
        (val >= 0xd00 && val <= 0xdf7f) ||
        /* myanmar */
        (val >= 0x1000 && val <= 0x109f) || (val >= 0xaa60 && val <= 0xaa7f) || (val >= 0xa9e0 && val <= 0xa9ff) ||
        /* oriya */
        (val >= 0xb00 && val <= 0xb7f) ||
        /* sinhala */
        (val >= 0xd80 && val <= 0xdff) ||
        /* tamil */
        (val >= 0xb80 && val <= 0xbff) || (val >= 0x11fc0 && val <= 0x11fff) ||
        /* telugu */
        (val >= 0xc00 && val <= 0xc7f) ||
        /* thaana */
        (val >= 0x780 && val <= 0x7bf) ||
        /* thai */
        (val >= 0xe00 && val <= 0xe7f) ||
        /* tibetan */
        (val >= 0xf00 && val <= 0xfff);
#endif
    }
    
    bool tokenizer::is_unicode_identifier_part(codepoint val)
    {
        return is_unicode_identifier_start(val) || is_digit(val);
    }
    
    bool tokenizer::is_whitespace(codepoint val)
    {
        switch (val) {
            /* tab */
            case 0x9:
            /* newline */
            case 0xa:
            /* vertical tab */
            case 0xb:
            /* form feed */
            case 0xc:
            /* carriage return */
            case 0xd:
            /* space */
            case 0x20:
            /* next line */
            case 0x85:
            /* left-to-right mark */
            case 0x200e:
            /* right-to-left-mark */
            case 0x200f:
            /* line separator */
            case 0x2028:
            /* paragraph separator */
            case 0x2029:
                return true;
        }
        
        return false;
    }
    
    bool tokenizer::is_newline(codepoint val) { return val == 0xa || val == 0xd; }

    bool tokenizer::is_character_quote(codepoint val) { return val == '\''; }

    bool tokenizer::is_string_quote(codepoint val) { return val == '\"'; }

    bool tokenizer::is_symbol(codepoint val)
    {
        switch (val) {
            case '@':
            case '/':
            case '.':
            case ':':
            case ';':
            case ',':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '+':
            case '-':
            case '*':
            case '%':
            case '!':
            case '>':
            case '<':
            case '=':
            case '&':
            case '|':
            case '^':
            case '~':
                return true;
            default:
                break;
        }

        return false;
    }
    
    namespace impl {
        /**
         * This hash maps gives is used for identifier lookup, which
         * is to test whether an identifier is a keyword. Hash functions
         * takes O(k) on string of k bytes where lookup itself is O(1).
         * In summary total lookup takes O(k).
         * This method is chosen over linear test with basic strcmp
         * which would take O(n*k) with n keywords.
         */
        static std::unordered_map<utf8::span, enum token::kind> keywords ({
            { utf8::span("app"), token::kind::app_kw },
            { utf8::span("as"), token::kind::as_kw },
            { utf8::span("behaviour"), token::kind::behaviour_kw },
            { utf8::span("break"), token::kind::break_kw },
            { utf8::span("concept"), token::kind::concept_kw },
            { utf8::span("const"), token::kind::const_kw },
            { utf8::span("continue"), token::kind::continue_kw },
            { utf8::span("else"), token::kind::else_kw },
            { utf8::span("ensure"), token::kind::ensure_kw },
            { utf8::span("extend"), token::kind::extend_kw },
            { utf8::span("extern"), token::kind::extern_kw },
            { utf8::span("false"), token::kind::false_kw },
            { utf8::span("for"), token::kind::for_kw },
            { utf8::span("function"), token::kind::function_kw },
            { utf8::span("hide"), token::kind::hide_kw },
            { utf8::span("if"), token::kind::if_kw },
            { utf8::span("in"), token::kind::in_kw },
            { utf8::span("invariant"), token::kind::invariant_kw },
            { utf8::span("is"), token::kind::is_kw },
            { utf8::span("mutable"), token::kind::mutable_kw },
            { utf8::span("lib"), token::kind::lib_kw },
            { utf8::span("range"), token::kind::range_kw },
            { utf8::span("require"), token::kind::require_kw },
            { utf8::span("return"), token::kind::return_kw },
            { utf8::span("when"), token::kind::when_kw },
            { utf8::span("static"), token::kind::static_kw },
            { utf8::span("test"), token::kind::test_kw },
            { utf8::span("true"), token::kind::true_kw },
            { utf8::span("type"), token::kind::type_kw },
            { utf8::span("union"), token::kind::union_kw },
            { utf8::span("use"), token::kind::use_kw },
            { utf8::span("val"), token::kind::val_kw }
        }, static_cast<std::size_t>(token::kind::nkinds));
        /**
         * This hash set contains suffixes for numeric literals
         */
        static std::unordered_map<utf8::span, enum token::kind> suffixes ({
            { utf8::span("u8"), token::kind::integer_literal },
            { utf8::span("u16"), token::kind::integer_literal },
            { utf8::span("u32"), token::kind::integer_literal },
            { utf8::span("u64"), token::kind::integer_literal },
            { utf8::span("u128"), token::kind::integer_literal },
            { utf8::span("usize"), token::kind::integer_literal },
            { utf8::span("i8"), token::kind::integer_literal },
            { utf8::span("i16"), token::kind::integer_literal },
            { utf8::span("i32"), token::kind::integer_literal },
            { utf8::span("i64"), token::kind::integer_literal },
            { utf8::span("i128"), token::kind::integer_literal },
            { utf8::span("isize"), token::kind::integer_literal },
            { utf8::span("f32"), token::kind::real_literal },
            { utf8::span("f64"), token::kind::real_literal },
            { utf8::span("f128"), token::kind::real_literal },
            { utf8::span("i"), token::kind::imag_literal }
        }, static_cast<std::size_t>(token::kind::nkinds));
    }

    void tokenizer::init()
    {
        // skip utf-8 byte order mark at beginning of file eventually
        if (file_.source().data()[0] == 0xef && 
            file_.source().data()[1] == 0xbb &&
            file_.source().data()[2] == 0xbf) {
            start_.iter = utf8::span::iterator(file_.source().data() + 3);
        }
        else {
            start_.iter = file_.source().begin();
        }

        // fills line table of source file with O(n) cycle
        utf8::span::iterator line = start_.iter, cur = line;
        while (cur != file_.source().end()) {
            if (is_newline(*cur)) {
                file_.line_table_.push_back(file_.source().subspan(line, cur));
                line = cur + 1;
            }
            
            ++cur;
        }
        // last line terminates with eof
        file_.line_table_.push_back(file_.source().subspan(line, cur));

        start_.location.filename = file_.name();
        start_.location.line = 1;
        start_.location.column = 1;

        end_.iter = file_.source().end();

        state_ = start_;
    }

    tokenizer::tokenizer(source_file& file, diagnostic_publisher& publisher) :
        file_(file),
        publisher_(publisher),
        start_(),
        end_(),
        state_()
    {
        init();
    }

    source_file& tokenizer::get_source_file() const { return file_; }

    diagnostic_publisher& tokenizer::get_diagnostic_publisher() const { return publisher_; }
    
    struct tokenizer::state tokenizer::state() const { return state_; }

    void tokenizer::start(struct state s) { start_ = s; }
        
    void tokenizer::end(struct state s) { end_ = s; }
    
    bool tokenizer::eof() const { return state_.iter == end_.iter; }

    bool tokenizer::eol() const {
        // eol returns true eventually if some whitespaces 
        // divide the current position from end of buffer or newline
        // a lookup must be done in the following text to
        // go through whitespaces who precede newline of end of buffer
        utf8::span::iterator copy(state_.iter);
        while (copy < end_.iter &&
               !is_newline(*copy) && 
               is_whitespace(*copy)) {
            ++copy;
        }

        return copy == end_.iter || is_newline(*copy); 
    }
    
    void tokenizer::restore(struct state s) { state_ = s; }

    void tokenizer::set_tokens(tokens *tokens) { tokens_ = tokens; }
        
    tokenizer::tokens *tokenizer::get_tokens() const { return tokens_; }
    
    codepoint tokenizer::current() const 
    {
        // at the end of file sentinel character (null terminator)
        // is returned
        if (eof()) return 0x0; 
        return *(state_.iter); 
    }
    
    codepoint tokenizer::next() const
    {
        // at the end of file sentinel character (null terminator)
        // is returned
        utf8::span::iterator next_pos = state_.iter + 1;
        if (next_pos == file_.source().end()) return 0x0;
        return *next_pos;
    }

    source_range tokenizer::range(struct state begin, struct state end)
    {
        return source_range(begin.location.line, begin.location.column, end.location.line, end.location.column, file_.name());
    }

    utf8::span tokenizer::extract(struct state begin, struct state end)
    {
        return file_.source().subspan(begin.iter, end.iter);
    }

    // pointer advances of a unicode character which means from 1 to 4 bytes in UTF-8 encoding
    // column advances of the number of spaces required by a character displayed on monitor (wcwidth())
    void tokenizer::advance()
    {
        if (eof()) return;

        if (is_newline(current())) {
            if (!get_tokens()->empty()) get_tokens()->back().eol = 1;
            ++state_.location.line;
            state_.location.column = 1;
        }
        else {
            state_.location.column += utf8::width(*(state_.iter));
        }

        ++state_.iter;
    }

    bool tokenizer::comment()
    {
        if (current() == '/') {
            token tok;
            struct state saved = state_;
            // single line comment
            if (next() == '/') {
                advance();
                advance();
                // advances till the end of the line
                while (!eof() && !is_newline(current())) {
                    if (!is_unicode_character(current())) {
                        struct state err = state_;
                        advance();
                        get_tokens()->push_back(token::builder().valid(false).location(err.location).lexeme(extract(err, state_)).eol(eol()).build());
                        byte encoded[5];
                        utf8::encode(*err.iter, encoded);
                        // unrecognized character in input
                        diagnostic diag = diagnostic::builder()
                                          .severity(diagnostic::severity::error)
                                          .location(err.location)
                                          .message(diagnostic::format("I don't think U+${x} (aka `$`) is a valid sh*t in the middle of a program.", *err.iter, encoded))
                                          .explanation("I remind you that only some unicode characters are valid inside the program, including: • letters `A`..`Z` `a`..`z` • numbers `0`..`9` • punctuation `{` `}` `[` `]` `(` `)` `.` `...` `:` `,` `;` • operators `|` `||` `&` `&&` `!` `~` `=` `<` `>` `<=` `>=` `==` ecc")
                                          .highlight(range(err, state_), "garbage")
                                          .build();
                        publisher_.publish(diag);
                    }
                    else {
                        advance();
                    }
                }
#if !__NEMESIS_DISCARD_COMMENTS__                
                tok = token::builder()
                      .kind(token::kind::comment)
                      .location(saved.location)
                      .lexeme(extract(saved, state_))
                      .eol(true)
                      .build();

                get_tokens()->push_back(tok);
#endif
                return true;
            }
            // multi line comment, they can be nested
            else if (next() == '*') {
                std::stack<struct state> comments;
                comments.push(saved);
                advance();
                advance();
                // parse all nested comments
                while (!eof() && !comments.empty()) {
                    // beginning of nested comment
                    if (current() == '/' && next() == '*') {
                        comments.push(state_);
                        advance();
                        advance();
                    }
                    // end of most recent nested comment or parent comment
                    else if (current() == '*' && next() == '/') {
                        advance();
                        advance();
                        comments.pop();
                    }
                    // invalid character
                    else if (!is_unicode_character(current())) {
                        struct state err = state_;
                        advance();
                        get_tokens()->push_back(token::builder().valid(false).location(err.location).lexeme(extract(err, state_)).eol(eol()).build());
                        byte encoded[5];
                        utf8::encode(*err.iter, encoded);
                        // unrecognized character in input
                        diagnostic diag = diagnostic::builder()
                                          .severity(diagnostic::severity::error)
                                          .location(err.location)
                                          .message(diagnostic::format("I don't think U+${x} (aka `$`) is a valid sh*t in the middle of a program.", *err.iter, encoded))
                                          .explanation("I remind you that only some unicode characters are valid inside the program, including: • letters `A`..`Z` `a`..`z` • numbers `0`..`9` • punctuation `{` `}` `[` `]` `(` `)` `.` `...` `:` `,` `;` • operators `|` `||` `&` `&&` `!` `~` `=` `<` `>` `<=` `>=` `==` ecc")
                                          .highlight(range(err, state_), "garbage")
                                          .build();
                        publisher_.publish(diag);
                    }
                    // unicode character
                    else {
                        advance();
                    }
                }
                // if stack is not empty, all unterminated comment are showed
                while (!comments.empty()) {
                    if (comments.size() == 1) {
                        diagnostic diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(comments.top().location)
                                            .message("I can't see a f*cking end for this comment!")
                                            .explanation("There exist two kind of comments: • single-line comments which start with `//` • multi-line comments which are enclosed by `/*` and `*/`")
                                            .small(true)
                                            .insertion(range(state_, state_), "*/", "I suggest putting `*/` to end your comment")
                                            .highlight(range(comments.top(), state_), "missing `*/`")
                                            .build();
                        
                        publisher_.publish(diag);
                    }
                    comments.pop();
                }
#if !__NEMESIS_DISCARD_COMMENTS__  
                // comment is built from the parent multi line comment
                tok = token::builder()
                      .kind(token::kind::comment)
                      .location(saved.location)
                      .lexeme(extract(saved, state_))
                      .eol(eol())
                      .build();

                get_tokens()->push_back(tok);
#endif
                return true;
            }

            return false;
        }

        return false;
    }

    bool tokenizer::escape()
    {
        if (current() == '\\') {
            struct state saved = state_;
            int digits = 0;
            advance();
            switch (current()) {
                case 'a':
                case 'f':
                case 'n':
                case 'r':
                case 't':
                case 'v':
                case '"':
                case '\'':
                case '\\':
                case '0':
                    advance();
                    return true;
                case 'u':
                    advance();
                    // for validation this must be between 1 and 6
                    while (!eof() && is_hex_digit(current())) {
                        advance();
                        ++digits;
                    }

                    if (digits == 0) {
                        diagnostic diag = diagnostic::builder()
                                          .severity(diagnostic::severity::error)
                                          .location(saved.location)
                                          .highlight(range(saved, state_), "missing hex digits")
                                          .message("You forgot the hex digits after this unicode value, idiot!")
                                          .explanation("I expect to see from 1 to 6 hexadecimal digits inside unicode escape, for example: • `\\u1f595` represents `🖕` emoji • `\\u03c0` represents `π` greek letter • `\\u41` represents `A` letter")
                                          .build();
                        publisher_.publish(diag);
                    }
                    else if (digits > 6) {
                        diagnostic diag = diagnostic::builder()
                                          .severity(diagnostic::severity::error)
                                          .location(saved.location)
                                          .highlight(range(saved, state_), "too many digits")
                                          .message(diagnostic::format("I count $ hexadecimal digits. You cannot use more that 6, b*tch!", digits))
                                          .explanation("I expect to see from 1 to 6 hexadecimal digits inside unicode escape, for example: • `\\u1f595` represents `🖕` emoji • `\\u03c0` represents `π` greek letter • `\\u41` represents `A` letter")
                                          .build();
                        publisher_.publish(diag);
                    }
                    return true;
                case 'o':
                    advance();
                    // for validation this must be between 1 and 3
                    while (!eof() && is_oct_digit(current())) {
                        advance();
                        ++digits;
                    }
                    if (digits == 0) {
                        diagnostic diag = diagnostic::builder()
                                          .severity(diagnostic::severity::error)
                                          .location(saved.location)
                                          .highlight(range(saved, state_), "missing octal digits")
                                          .message("You forgot the octal digits after this octal value, idiot!")
                                          .explanation("I expect to see from 1 to 3 octal digits inside octal escape, for example: • `\\o101` represents `A` letter or `65` number • `\\o41` represents `!` letter or `33` number")
                                          .build();
                        publisher_.publish(diag);
                    }
                    else if (digits > 3) {
                        diagnostic diag = diagnostic::builder()
                                          .severity(diagnostic::severity::error)
                                          .location(saved.location)
                                          .highlight(range(saved, state_), "too many digits")
                                          .message(diagnostic::format("I count $ octal digits. You cannot use more that 3, b*tch!", digits))
                                          .explanation("I expect to see from 1 to 3 octal digits inside octal escape, for example: • `\\o101` represents `A` letter or `65` number • `\\o41` represents `!` letter or `33` number")
                                          .build();
                        publisher_.publish(diag);
                    }
                    return true;
                case 'x':
                    advance();
                    // for validation this must be between 1 and 2
                    while (!eof() && is_hex_digit(current())) {
                        advance();
                        ++digits;
                    }
                    if (digits == 0) {
                        diagnostic diag = diagnostic::builder()
                                          .severity(diagnostic::severity::error)
                                          .location(saved.location)
                                          .highlight(range(saved, state_), "missing hex digits")
                                          .message("You forgot the hex digits after this hex value, idiot!")
                                          .explanation("I expect to see from 1 to 2 hex digits inside hex escape, for example: • `\\x41` represents `A` letter or `65` number • `\\x21` represents `!` letter or `33` number")
                                          .build();
                        publisher_.publish(diag);
                    }
                    else if (digits > 2) {
                        diagnostic diag = diagnostic::builder()
                                          .severity(diagnostic::severity::error)
                                          .location(saved.location)
                                          .highlight(range(saved, state_), "too many digits")
                                          .message(diagnostic::format("I count $ hex digits. You cannot use more that 2, b*tch!", digits))
                                          .explanation("I expect to see from 1 to 2 hex digits inside hex escape, for example: • `\\x41` represents `A` letter or `65` number • `\\x21` represents `!` letter or `33` number")
                                          .build();
                        publisher_.publish(diag);
                    }
                    return true;
                case 'b':
                    advance();
                    // for validation this must be between 1 and 8
                    while (!eof() && is_bin_digit(current())) {
                        advance();
                        ++digits;
                    }
                    if (digits == 0) {
                        diagnostic diag = diagnostic::builder()
                                          .severity(diagnostic::severity::error)
                                          .location(saved.location)
                                          .highlight(range(saved, state_), "missing binary digits")
                                          .message("You forgot the bin digits after this bin value, idiot!")
                                          .explanation("I expect to see from 1 to 8 bin digits inside bin escape, for example: • `\\b1000001` represents `A` letter or `65` number • `\\b100001` represents `!` letter or `33` number")
                                          .build();
                        publisher_.publish(diag);
                    }
                    else if (digits > 8) {
                        diagnostic diag = diagnostic::builder()
                                          .severity(diagnostic::severity::error)
                                          .location(saved.location)
                                          .highlight(range(saved, state_), "too many digits")
                                          .message(diagnostic::format("I count $ bin digits. You cannot use more that 8, b*tch!", digits))
                                          .explanation("I expect to see from 1 to 8 bin digits inside bin escape, for example: • `\\b1000001` represents `A` letter or `65` number • `\\b100001` represents `!` letter or `33` number")
                                          .build();
                        publisher_.publish(diag);
                    }
                    return true;
                default:
                    // wrong escape is skipped
                    advance();
                    
                    diagnostic diag = diagnostic::builder()
                                      .severity(diagnostic::severity::error)
                                      .location(saved.location)
                                      .highlight(range(saved, state_), "dafuq")
                                      .message("What the hell is this escape sequence? Never heard of it before.")
                                      .explanation("Note that valid escape sequences are: • `\\a` for alarm • `\\f` for line feed • `\\n` for line ending • `\\r` for carriage return • `\\t` for horizontal tab • `\\v` for vertical tab • `\\\\` for double slash • `\\\"` for double quote • `\\'` for single quote • `\\0` for null terminator (like C strings) • `\\u` for unicode escape • `\\o` for octal escape • `\\x` for hexadecimal escape • `\\b` for binary escape")
                                      .build();
                    publisher_.publish(diag);

                    return true;
            }
        }

        return false;
    }

    bool tokenizer::character()
    {
        if (is_character_quote(current())) {
            token tok;
            struct state saved = state_;
            bool valid = true;
            // count of characters inside the literal
            // for validation, this must be 1 at the end of literal
            unsigned count = 0;
            advance();
            while (!eof()) {
                if (is_newline(current())) {
                    diagnostic diag = diagnostic::builder()
                                      .severity(diagnostic::severity::error)
                                      .location(saved.location)
                                      .highlight(range(saved, state_), "missing `'`")
                                      .insertion(range(state_, state_), "\'", "I suggest putting `\'` to end your character")
                                      .message("I can't see a f*cking end for this character!")
                                      .explanation("Character literals, enclosed by single quote `\'`, must lie one a single line and contain one unicode character, for example: • `'\\u1F4A9'` represents `💩` emoji • `'A'` represents `A` letter")
                                      .build();
                    publisher_.publish(diag);

                    tok = token::builder().kind(token::kind::char_literal)
                        .lexeme(extract(saved, state_))
                        .location(saved.location)
                        .valid(valid)
                        .eol(true)
                        .valid(false)
                        .build();
                    
                    get_tokens()->push_back(tok);
                    
                    return false;
                }
                else if (is_character_quote(current())) {
                    advance();

                    if (count == 0) {
                        diagnostic diag = diagnostic::builder()
                                          .severity(diagnostic::severity::error)
                                          .location(saved.location)
                                          .highlight(range(saved, state_), "empty")
                                          .message("This character is ridiculously empty.")
                                          .explanation("Character literals, enclosed by single quote `\'`, must lie one a single line and contain one unicode character, for example: • `'\\u1F4A9'` represents `💩` emoji • `'A'` represents `A` letter")
                                          .build();
                        publisher_.publish(diag);

                        valid = false;
                    }
                    else if (count > 1) {
                        diagnostic diag = diagnostic::builder()
                                          .severity(diagnostic::severity::error)
                                          .location(saved.location)
                                          .highlight(range(saved, state_), "too many characters")
                                          .message(diagnostic::format("I count $ characters. There is a f*cking reason this is a character and not a string!", count))
                                          .explanation("Character literals, enclosed by single quote `\'`, must lie one a single line and contain one unicode character, for example: • `'\\u1F4A9'` represents `💩` emoji • `'A'` represents `A` letter")
                                          .build();
                        publisher_.publish(diag);

                        valid = false;
                    }

                    tok = token::builder()
                          .kind(token::kind::char_literal)
                          .lexeme(extract(saved, state_))
                          .location(saved.location)
                          .valid(valid)
                          .eol(eol())
                          .build();

                    get_tokens()->push_back(tok);

                    return true;
                }
                else if (current() == '\\') {
                    escape(); 
                    ++count;
                }
                else if (is_unicode_character(current())) {
                    advance();
                    ++count;
                }
                else {
                    struct state err = state_;
                    advance();
                    get_tokens()->push_back(token::builder().valid(false).location(err.location).lexeme(extract(err, state_)).eol(eol()).build());
                    byte encoded[5];
                    utf8::encode(*err.iter, encoded);
                    ++count;

                    diagnostic diag = diagnostic::builder()
                                      .severity(diagnostic::severity::error)
                                      .location(err.location)
                                      .highlight(range(err, state_))
                                      .message(diagnostic::format("I don't think U+${x} (aka `$`) is a valid sh*t in the middle of a program.", *err.iter, encoded))
                                      .explanation("I remind you that only some unicode characters are valid inside the program, including: • letters `A`..`Z` `a`..`z` • numbers `0`..`9` • punctuation `{` `}` `[` `]` `(` `)` `.` `...` `:` `,` `;` • operators `|` `||` `&` `&&` `!` `~` `=` `<` `>` `<=` `>=` `==` ecc")
                                      .highlight(range(err, state_), "garbage")
                                      .build();
                    publisher_.publish(diag);

                    valid = false;
                }
            }

            diagnostic diag = diagnostic::builder()
                              .severity(diagnostic::severity::error)
                              .location(saved.location)
                              .highlight(range(saved, state_), "missing `'`")
                              .insertion(range(state_, state_), "\'", "I suggest putting `\'` to end your character")
                              .message("I can't see a f*cking end for this character!")
                              .explanation("Character literals, enclosed by single quote `\'`, must lie one a single line and contain one unicode character, for example: • `'\\u1F4A9'` represents `💩` emoji • `'A'` represents `A` letter")
                              .build();
            publisher_.publish(diag);

            tok = token::builder().kind(token::kind::char_literal)
                .lexeme(extract(saved, state_))
                .location(saved.location)
                .valid(valid)
                .eol(true)
                .valid(false)
                .build();
            
            get_tokens()->push_back(tok);

            return false;
        }

        return false;
    }

    bool tokenizer::string()
    {
        if (is_string_quote(current())) {
            token tok;
            struct state saved = state_;
            advance();
            // error flag is necessary when we set the error but
            // we must continue scanning the string so at the
            // end of the literal we know if it valid
            bool valid = true;
            // format string to hold placeholders eventually
            utf8::span::builder fmt;
            // arguments to be formatted into the string
            tokens args;
            // number of interpolated expressions
            unsigned expressions = 0;
            // opening quote
            fmt.add('\"');

            while (!eof()) {
                if (is_newline(current())) {
                    diagnostic diag = diagnostic::builder()
                                      .severity(diagnostic::severity::error)
                                      .location(saved.location)
                                      .highlight(range(saved, state_), "missing `\"`")
                                      .insertion(range(state_, state_), "\"", "I suggest putting `\"` to end your string")
                                      .message("I can't see a f*cking end for this string!")
                                      .explanation("String literals, enclosed by double quote `\"`, must lie one a single line. For example: • `\"You useless \\u1F4A9\"` represents `You useless 💩` raw string (type `chars`) • `\"val = {value}\"` contains an interpolated expression • `\"special\"s` is heap allocated (type `string`) \\ There exists two types of string literals: • primitive string whose type is `chars` (similar to `[u8]`) • heap-allocated string (`s` suffix) whose type is `string`")
                                      .build();
                    publisher_.publish(diag);

                    tok = token::builder().kind(token::kind::string_literal)
                            .lexeme(extract(saved, state_))
                            .location(saved.location)
                            .valid(valid)
                            .eol(true)
                            .valid(false)
                            .build();
                    // to help the parser recognizing an expression, even an invalid token is inserted into the stream
                    get_tokens()->push_back(tok);
                    
                    return false;
                }
                else if (is_string_quote(current())) {
                    advance();
                    fmt.add('\"');
                    // the only valid suffix is 's'
                    if (is_letter(current())) {
                        struct state suffix_saved = state_;
                        advance();
                        while (is_letter(current())) advance();

                        if (extract(suffix_saved, state_).compare(utf8::span("s")) != 0) {
                            diagnostic diag = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(suffix_saved.location)
                                                .highlight(range(saved, suffix_saved), diagnostic::highlighter::mode::light)
                                                .highlight(range(suffix_saved, state_), "maybe `s`")
                                                .message("I have no clue what this string suffix means. Can you tell me?")
                                                .explanation("String literals, enclosed by double quote `\"`, must lie one a single line. For example: • `\"You useless \\u1F4A9\"` represents `You useless 💩` raw string (type `chars`) • `\"val = {value}\"` contains an interpolated expression • `\"special\"s` is heap allocated (type `string`) \\ There exists two types of string literals: • primitive string whose type is `chars` (similar to `[u8]`) • heap-allocated string (`s` suffix) whose type is `string`")
                                                .build();
                            
                            publisher_.publish(diag);
                            valid = false;
                        }
                        else {
                            fmt.add('s');
                        }
                    }
                    // if there is at least one interpolated expressions
                    // the tokenizer injects a format() call
                    // the reason why even invalid strings with valid interpolated expressions
                    // are built is because the parser will have the possibility to parse the
                    // interpolated expressions even though the format string literal is well formed
                    if (expressions > 0) {
                        // `format` function name is injected with opening and closing parenthesis
                        token function = token::builder()
                                         .kind(token::kind::identifier)
                                         .location(saved.location)
                                         .lexeme(utf8::span::builder().concat("__format").build())
                                         .artificial(true)
                                         .build();
                        // format opening parenthesis
                        token open = token::builder()
                                     .kind(token::kind::left_parenthesis)
                                     .lexeme(utf8::span::builder().add('(').build())
                                     .location(saved.location)
                                     .artificial(true)
                                     .build();
                        // format closing parenthesis
                        token close = token::builder()
                                      .kind(token::kind::right_parenthesis)
                                      .lexeme(utf8::span::builder().add(')').build())
                                      .location(state_.location)
                                      .artificial(true)
                                      .eol(eol()) // closing parenthesis is last item injected
                                      .build();
                        // format string format which contains place holders
                        // if string is not valid for some reason then format
                        // string will take note of this
                        token format = token::builder()
                                       .kind(token::kind::string_literal)
                                       .location(saved.location)
                                       .lexeme(fmt.build())
                                       .valid(valid)
                                       .artificial(true)
                                       .build();
                        // injects all artificial tokens inside the output
                        get_tokens()->push_back(function);
                        get_tokens()->push_back(open);
                        get_tokens()->push_back(format);
                        get_tokens()->splice(get_tokens()->end(), args);
                        get_tokens()->push_back(close);
                    }
                    // basic string literal with no interpolated expressions inside
                    else {
                        tok = token::builder().kind(token::kind::string_literal)
                              .lexeme(extract(saved, state_))
                              .location(saved.location)
                              .valid(valid)
                              .eol(eol())
                              .build();
                        // literal with no addition in inserted into token output stream
                        get_tokens()->push_back(tok);
                    }
                    
                    return true;
                }
                else if (current() == '\\') {
                    // iterator which points to the beginning of the escape
                    utf8::span::iterator iter = state_.iter;
                    escape();
                    // add escape sequence to format string
                    for (; iter < state_.iter; ++iter) fmt.add(*iter);
                }
                else if (current() == '{') {
                    // tokens beloging to current interpolated expression
                    tokens expr;
                    // if interpolated expression is valid
                    if (interpolation(expr)) {
                        // if not empty the interpolated expression is added to the format() call
                        // and place holder is set
                        if (!expr.empty()) {
                            // insert a comma after the last expression (format string or interpolated expression)
                            token comma = token::builder()
                                            .kind(token::kind::comma)
                                            .location(expr.front().location())
                                            .lexeme(utf8::span::builder().add(',').build())
                                            .artificial(true)
                                            .build();
                            // injects the comma
                            args.push_back(comma);
                            // add current expression tokens to format() call arguments
                            args.splice(args.end(), expr);
                            // add place holder to format string
                            fmt.add('?');
                            // increment count of valid interpolated expressions
                            ++expressions;
                        }
                        else {
                            // error is set on empty interpolated expression
                            valid = false;
                        }
                    }
                    else {
                        // interpolation is badly used so we reached the end of the line
                        // parent string is then left unterminated and corrupted
                        diagnostic diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(saved.location)
                                        .highlight(range(saved, state_))
                                        .message("You messed this string up with your damned interpolation errors!")
                                        .explanation("String literals, enclosed by double quote `\"`, must lie one a single line. For example: • `\"You useless \\u1F4A9\"` represents `You useless 💩` raw string (type `chars`) • `\"val = {value}\"` contains an interpolated expression • `\"special\"s` is heap allocated (type `string`) \\ There exists two types of string literals: • primitive string whose type is `chars` (similar to `[u8]`) • heap-allocated string (`s` suffix) whose type is `string`")
                                        .build();
                        publisher_.publish(diag);
                        // invalid literal string is inserted into the stream
                        tok = token::builder().kind(token::kind::string_literal)
                            .lexeme(extract(saved, state_))
                            .location(saved.location)
                            .valid(valid)
                            .eol(true)
                            .valid(false)
                            .build();

                        get_tokens()->push_back(tok);
                        // avoid cascade error of not terminating parent string on the same line
                        return false;
                    }
                }
                else if (is_unicode_character(current())) {
                    // adds character in format string
                    fmt.add(current());
                    advance();
                }
                else {
                    // adds forbidden character in format string
                    fmt.add(current());
                    // save state of error
                    struct state err = state_;
                    advance();
                    get_tokens()->push_back(token::builder().valid(false).location(err.location).lexeme(extract(err, state_)).eol(eol()).build());
                    byte encoded[5];
                    utf8::encode(*err.iter, encoded);
                    // emits error diagnostic
                    diagnostic diag = diagnostic::builder()
                                      .severity(diagnostic::severity::error)
                                      .location(err.location)
                                      .message(diagnostic::format("I don't think U+${x} (aka `$`) is a valid sh*t in the middle of a program.", *err.iter, encoded))
                                      .explanation("I remind you that only some unicode characters are valid inside the program, including: • letters `A`..`Z` `a`..`z` • numbers `0`..`9` • punctuation `{` `}` `[` `]` `(` `)` `.` `...` `:` `,` `;` • operators `|` `||` `&` `&&` `!` `~` `=` `<` `>` `<=` `>=` `==` ecc")
                                      .highlight(range(err, state_), "garbage")
                                      .build();
                    publisher_.publish(diag);
                    // error is set on invalid unicode character
                    valid = false;
                }
            }

            // at this point we reached end of file
            diagnostic diag = diagnostic::builder()
                              .severity(diagnostic::severity::error)
                              .location(state_.location)
                              .highlight(range(saved, state_), "missing `\"`")
                              .insertion(range(state_, state_), "\"", "I suggest putting `\"` to end your string")
                              .message("I can't see a f*cking end for this string!")
                              .explanation("String literals, enclosed by double quote `\"`, must lie one a single line. For example: • `\"You useless \\u1F4A9\"` represents `You useless 💩` raw string (type `chars`) • `\"val = {value}\"` contains an interpolated expression • `\"special\"s` is heap allocated (type `string`) \\ There exists two types of string literals: • primitive string whose type is `chars` (similar to `[u8]`) • heap-allocated string (`s` suffix) whose type is `string`")
                              .build();
            publisher_.publish(diag);
            // invalid literal string is inserted into the stream
            tok = token::builder().kind(token::kind::string_literal)
                .lexeme(extract(saved, state_))
                .location(saved.location)
                .valid(valid)
                .eol(true)
                .valid(false)
                .build();
            
            get_tokens()->push_back(tok);

            return false;
        }

        return false;
    }

    bool tokenizer::reach_end_of_interpolation(struct state& end)
    {
        // we assume that the calling context is string() so we are inside a string 
        if (current() == '{') {
            // saved state is necessary to backtrack in case of success
            // otherwise on failure state reaches end of line
            struct state saved = state_;
            // stack of braces is necessary to correctly parse braces inside
            // interpolated expressions
            std::stack<struct state> braces;
            // initial brace is pushed onto the stack and 
            braces.push(state_);
            advance();
            // advances till end of line or file o until closing brace is encountred
            while (!eof() && !is_newline(current()) && !braces.empty()) {
                // opening brace pushed onto the stack
                if (current() == '{') {
                    braces.push(state_);
                    advance();
                }
                // closing brace is encountered
                else if (current() == '}') {
                    // last one is closing parenthesis of interpolated expression
                    if (braces.size() == 1) {
                        // so end position of interpolated expression is set
                        end = state_;
                    }
                    // otherwise advances if other brace
                    else {
                        advance();
                    }
                    // removes the last `{` brace from the stack which corresponds to this last `}`
                    braces.pop();
                }
                else {
                    // unicode character (no brace, newline or eof) so advances
                    advance();
                }
            }
            // if stack is not empty then interpolated expression is not terminated with closing brace `}`
            if (!braces.empty()) {
                while (!braces.empty()) {
                    // opening brace of interpolated expression
                    if (braces.size() == 1) {
                        // print error of unterminated interpolated expression
                        diagnostic diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(braces.top().location)
                                        .highlight(range(saved, state_), "missing `}`")
                                        .message("I can't see a f*cking end for this interpolated expression?!")
                                        .insertion(range(state_, state_), "}", "I suggest putting `}` to end your interpolated expression")
                                        .explanation("String literals, enclosed by double quote `\"`, must lie one a single line. For example: • `\"You useless \\u1F4A9\"` represents `You useless 💩` raw string (type `chars`) • `\"val = {value}\"` contains an interpolated expression • `\"special\"s` is heap allocated (type `string`) \\ There exists two types of string literals: • primitive string whose type is `chars` (similar to `[u8]`) • heap-allocated string (`s` suffix) whose type is `string`")
                                        .build();
                        publisher_.publish(diag);
                    }
                    // remove opening brace
                    braces.pop();
                }

                // invalid interpolated expression
                return false;
            }

            // we have just recognized an interpolated expression to
            // be scanned later so we restore state to the opening brace `{`
            restore(saved);

            return true;
        }

        return false;       
    }

    bool tokenizer::interpolation(tokens& expression)
    {
        // end will point eventually to closing `}' brace of interpolated expression
        struct state end;
        // this call is necessary to compute the end location of the interpolated expression
        // without performing tokenization
        if (reach_end_of_interpolation(end)) {
            // valid interpolation expression, at least it is empty
            // now end points to the closing `}` brace
            if (current() == '{') {
                struct state saved = state_;
                // skip `{' brace
                advance();
                // save start and end state to retore after
                struct state saved_end = end_, saved_start = start_;
                // start is set to the first character after `{` which is expression start
                this->start(state_);
                // set eof at the closing brace `}` of interpolated expression
                this->end(end);
                // scan tokens inside the interpolated expression in range [saved + 1, end)
                tokenize(expression, false); 
                // restore original eof to point to the end of parent buffer
                // maybe we are in a nested level of interpolated expression
                // and parent eof is parent closing `}`, otherwise if we are
                // in level 1 of interpolated expression, then saved_end points
                // to the effective end of file
                this->end(saved_end);
                // restore original start
                this->start(saved_start);
                // skip closing '}' of interpolated expression
                advance();
                // a error is emitted for an empty interpolated expression
                // no place holder is added to format string if expression is empty
                if (expression.empty()) {
                    diagnostic diag = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(saved.location)
                                    .highlight(range(saved, state_), "empty")
                                    .message("Empty interpolated expression is bloody nonsense.")
                                    .explanation("String literals, enclosed by double quote `\"`, must lie one a single line. For example: • `\"You useless \\u1F4A9\"` represents `You useless 💩` raw string (type `chars`) • `\"val = {value}\"` contains an interpolated expression • `\"special\"s` is heap allocated (type `string`) \\ There exists two types of string literals: • primitive string whose type is `chars` (similar to `[u8]`) • heap-allocated string (`s` suffix) whose type is `string`")
                                    .build();
                    publisher_.publish(diag);
                }
                // true is returned as interpolated expression, even though empty eventually, is well parsed
                return true;
            }
        }

        return false;
    }
    
    bool tokenizer::number()
    {
        if (is_digit(current())) {
            token tok;
            struct state saved = state_;
            enum token::kind kind = token::kind::integer_literal;
            bool valid = true;
            // hexadecimal integer
            if (current() == '0' && next() == 'x') {
                advance();
                advance();
                int count = 0;
                
                while (!eof() && (is_hex_digit(current()) || is_underscore(current()))) {
                    ++count;
                    advance();
                }

                if (count == 0) {
                    diagnostic diag = diagnostic::builder()
                                      .severity(diagnostic::severity::error)
                                      .location(state_.location)
                                      .highlight(range(saved, state_), "missing hex digits")
                                      .message("You forgot the hex digits after this hex number, idiot!")
                                      .explanation("Nemesis lets you construct three kinds of number literals: • integer: `123` `0b1010_1011` `0x41` `12u32` `0o101i8` ecc • real: `3.14f32` `0.3` ecc • imaginary: `1i` `0.4e-3i` ecc \\ You also may use these suffixes: • `i8` `i16` .. `i128` for signed integers • `u8` `u16` .. `u128` for unsigned integers • `f32` `f64` `f128` for reals  • `i` for imaginaries")
                                      .build();

                    publisher_.publish(diag);
                    valid = false;
                }
            }
            // octal integer
            else if (current() == '0' && next() == 'o') {
                advance();
                advance();
                int count = 0;
                
                while (!eof() && (is_digit(current()) || is_underscore(current()))) {
                    if (current() >= '0' && current() <= '7') ++count;
                    else if (current() == '8' || current() == '9') {
                        diagnostic diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(state_.location)
                                        .highlight(range(state_, state_), "must be 0, 1, .., 7")
                                        .message("These are not octal digits, idiot!")
                                        .explanation("Nemesis lets you construct three kinds of number literals: • integer: `123` `0b1010_1011` `0x41` `12u32` `0o101i8` ecc • real: `3.14f32` `0.3` ecc • imaginary: `1i` `0.4e-3i` ecc \\ You also may use these suffixes: • `i8` `i16` .. `i128` for signed integers • `u8` `u16` .. `u128` for unsigned integers • `f32` `f64` `f128` for reals  • `i` for imaginaries")
                                        .build();

                        publisher_.publish(diag);
                        valid = false;
                    }

                    advance();
                }

                if (count == 0) {
                    diagnostic diag = diagnostic::builder()
                                      .severity(diagnostic::severity::error)
                                      .location(state_.location)
                                      .highlight(range(saved, state_), "missing octal digits")
                                      .message("You forgot the octal digits after this octal number, idiot!")
                                      .explanation("Nemesis lets you construct three kinds of number literals: • integer: `123` `0b1010_1011` `0x41` `12u32` `0o101i8` ecc • real: `3.14f32` `0.3` ecc • imaginary: `1i` `0.4e-3i` ecc \\ You also may use these suffixes: • `i8` `i16` .. `i128` for signed integers • `u8` `u16` .. `u128` for unsigned integers • `f32` `f64` `f128` for reals  • `i` for imaginaries")
                                      .build();

                    publisher_.publish(diag);
                    valid = false;
                }
            }
            // binary integer
            else if (current() == '0' && next() == 'b') {
                advance();
                advance();
                int count = 0;
                
                while (!eof() && (is_digit(current()) || is_underscore(current()))) {
                    if (current() == '0' || current() == '1') ++count;
                    else if (current() >= '2' && current() <= '9') {
                        diagnostic diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(state_.location)
                                        .highlight(range(state_, state_), "must be 0 or 1")
                                        .message("These are not binary digits, idiot!")
                                        .explanation("Nemesis lets you construct three kinds of number literals: • integer: `123` `0b1010_1011` `0x41` `12u32` `0o101i8` ecc • real: `3.14f32` `0.3` ecc • imaginary: `1i` `0.4e-3i` ecc \\ You also may use these suffixes: • `i8` `i16` .. `i128` for signed integers • `u8` `u16` .. `u128` for unsigned integers • `f32` `f64` `f128` for reals  • `i` for imaginaries")
                                        .build();

                        publisher_.publish(diag);
                        valid = false;
                    }

                    advance();
                }

                if (count == 0) {
                    diagnostic diag = diagnostic::builder()
                                      .severity(diagnostic::severity::error)
                                      .location(state_.location)
                                      .highlight(range(saved, state_), "missing binary digits")
                                      .message("You forgot the binary digits after this binary number, idiot!")
                                      .explanation("Nemesis lets you construct three kinds of number literals: • integer: `123` `0b1010_1011` `0x41` `12u32` `0o101i8` ecc • real: `3.14f32` `0.3` ecc • imaginary: `1i` `0.4e-3i` ecc \\ You also may use these suffixes: • `i8` `i16` .. `i128` for signed integers • `u8` `u16` .. `u128` for unsigned integers • `f32` `f64` `f128` for reals  • `i` for imaginaries")
                                      .build();

                    publisher_.publish(diag);
                    valid = false;
                }
            }
            // decimal integer or real or complex
            else {
                advance();
                while (!eof() && (is_digit(current()) || is_underscore(current()))) advance();
                // real or complex with dot decimal part
                if (current() == '.' && is_digit(next())) {
                    advance();
                    advance();
                    while (!eof() && (is_digit(current()) || is_underscore(current()))) advance();
                    kind = token::kind::real_literal;   
                }
                // real or complex by default with exponentiation
                if (current() == 'E' || current() == 'e') {
                    advance();
                    if (current() == '+' || current() == '-') advance();
                    if (!is_digit(current())) {
                        diagnostic diag = diagnostic::builder()
                                      .severity(diagnostic::severity::error)
                                      .location(state_.location)
                                      .highlight(range(saved, state_), "missing exponent digits")
                                      .message("You forgot the digits after exponentiation in this number, idiot!")
                                      .explanation("Nemesis lets you construct three kinds of number literals: • integer: `123` `0b1010_1011` `0x41` `12u32` `0o101i8` ecc • real: `3.14f32` `0.3` ecc • imaginary: `1i` `0.4e-3i` ecc \\ You also may use these suffixes: • `i8` `i16` .. `i128` for signed integers • `u8` `u16` .. `u128` for unsigned integers • `f32` `f64` `f128` for reals  • `i` for imaginaries")
                                      .build();
                        publisher_.publish(diag);

                        valid = false;
                    }
                    else {
                        advance();
                        while (!eof() && (is_digit(current()) || is_underscore(current()))) advance();
                        kind = token::kind::real_literal;
                    }
                }
            }

            if (is_unicode_identifier_start(current())) {
                struct state suffix_saved = state_;
                
                advance();
                while (is_unicode_identifier_part(current())) advance();
                
                utf8::span suffix = extract(suffix_saved, state_);
                auto pair = impl::suffixes.find(suffix);

                // real suffix
                if (pair == impl::suffixes.end()) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(suffix_saved.location)
                                .highlight(range(saved, suffix_saved), diagnostic::highlighter::mode::light)
                                .message("I have no clue what this number suffix means. Can you tell me?")
                                .explanation("Nemesis lets you construct three kinds of number literals: • integer: `123` `0b1010_1011` `0x41` `12u32` `0o101i8` ecc • real: `3.14f32` `0.3` ecc • imaginary: `1i` `0.4e-3i` ecc \\ You also may use these suffixes: • `i8` `i16` .. `i128` for signed integers • `u8` `u16` .. `u128` for unsigned integers • `f32` `f64` `f128` for reals  • `i` for imaginaries");

                    if (kind == token::kind::integer_literal) builder.highlight(range(suffix_saved, state_), "maybe `u32` `i32` ecc");
                    else builder.highlight(range(suffix_saved, state_), "maybe `f32` `f64` `f128` `i`");

                    publisher_.publish(builder.build());
                    valid = false;
                }
                else {
                    if (kind == token::kind::real_literal && pair->second == token::kind::integer_literal) {
                        diagnostic diag = diagnostic::builder()
                                            .severity(diagnostic::severity::warning)
                                            .location(suffix_saved.location)
                                            .highlight(range(saved, suffix_saved), diagnostic::highlighter::mode::light)
                                            .highlight(range(suffix_saved, state_), "maybe `f32` `f64` `f128` `i`")
                                            .message("Are you really trying to mark a real number as an integer? This will make the real number lose its precision.")
                                            .explanation("Nemesis lets you construct three kinds of number literals: • integer: `123` `0b1010_1011` `0x41` `12u32` `0o101i8` ecc • real: `3.14f32` `0.3` ecc • imaginary: `1i` `0.4e-3i` ecc \\ You also may use these suffixes: • `i8` `i16` .. `i128` for signed integers • `u8` `u16` .. `u128` for unsigned integers • `f32` `f64` `f128` for reals  • `i` for imaginaries")
                                            .build();
                        publisher_.publish(diag);
                    }
                    
                    kind = pair->second;
                }
            }

            tok = token::builder()
                  .kind(kind)
                  .location(saved.location)
                  .lexeme(extract(saved, state_))
                  .valid(valid)
                  .eol(eol())
                  .build();

            get_tokens()->push_back(tok);

            return true;
        }

        return false;
    }

    bool tokenizer::identifier()
    {
        if (is_unicode_identifier_start(current())) {
            token tok;
            struct state saved = state_;
            enum token::kind kind = token::kind::identifier;

            advance();
            while (!eof() && is_unicode_identifier_part(current())) advance();
            
            utf8::span lexeme = extract(saved, state_);
            auto pair = impl::keywords.find(lexeme);
            // identifier, not keyword
            if (pair != impl::keywords.end()) {
                kind = pair->second;
            }

            tok = token::builder()
                  .kind(kind)
                  .location(saved.location)
                  .lexeme(lexeme)
                  .eol(eol())
                  .build();

            // reserved words starts with '__'
            if (lexeme.size() > 1 && lexeme[0] == '_' && lexeme[1] == '_') {
                tok.valid = false;
                diagnostic diag = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(saved.location)
                                    .highlight(range(saved, state_))
                                    .message("You should know that words starting with `__` are reserved, d*mbass!")
                                    .build();

                publisher_.publish(diag);
            }
            get_tokens()->push_back(tok);

            return true;
        }

        return false;
    }

    void tokenizer::tokenize(tokens& tokens, bool inject_eof)
    {
        // saved previous tokens container for recursion levels caused by interpolation
        // at top level saved_tokens is initially nullptr
        tokenizer::tokens *saved_tokens = get_tokens();
        // now tokens will be saved in this new result
        set_tokens(&tokens);
        // reset state to beginning of input buffer
        restore(start_);
        // while end of file is not reached, then tokens are lexed
        // and pushed in current tokens list
        while (!eof()) {
            struct state saved = state_;
            enum token::kind kind = token::kind::unknown;
            switch (current()) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    number();
                    break;
                case '\"':
                    string();
                    break;
                case '\'':
                    character();
                    break;
                /* tab */
                case 0x9:
                /* newline */
                case 0xa:
                /* vertical tab */
                case 0xb:
                /* form feed */
                case 0xc:
                /* carriage return */
                case 0xd:
                /* space */
                case 0x20:
                /* next line */
                case 0x85:
                /* left-to-right mark */
                case 0x200e:
                /* right-to-left-mark */
                case 0x200f:
                /* line separator */
                case 0x2028:
                /* paragraph separator */
                case 0x2029:
                    advance();
                    break;
                /* operators and punctuation */
                case '/':
                    if (next() == '/' || next() == '*') {
                        comment();
                        break;
                    }
                    else advance();
                    if (current() == '=') {
                        advance();
                        kind = token::kind::slash_equal;
                    }
                    else {
                        kind = token::kind::slash;
                    }
                    break;
                case '.':
                    advance();
                    if (current() == '.') {
                        advance();
                        if (current() == '.') {
                            advance();
                            kind = token::kind::ellipsis;
                        }
                        else if (current() == '=') {
                            advance();
                            kind = token::kind::dot_dot_equal;
                        }
                        else {
                            kind = token::kind::dot_dot;
                        }
                    }
                    else {
                        kind = token::kind::dot;
                    }
                    break;
                case ':':
                    advance();
                    kind = token::kind::colon;
                    break;
                case ';':
                    advance();
                    kind = token::kind::semicolon;
                    break;
                case ',':
                    advance();
                    kind = token::kind::comma;
                    break;
                case '(':
                    advance();
                    kind = token::kind::left_parenthesis;
                    break;
                case ')':
                    advance();
                    kind = token::kind::right_parenthesis;
                    break;
                case '[':
                    advance();
                    kind = token::kind::left_bracket;
                    break;
                case ']':
                    advance();
                    kind = token::kind::right_bracket;
                    break;
                case '{':
                    advance();
                    kind = token::kind::left_brace;
                    break;
                case '}':
                    advance();
                    kind = token::kind::right_brace;
                    break;
                case '+':
                    advance();
                    if (current() == '+') {
                        advance();
                        kind = token::kind::plus_plus;
                    }
                    else if (current() == '=') {
                        advance();
                        kind = token::kind::plus_equal;
                    }
                    else {
                        kind = token::kind::plus;
                    }
                    break;
                case '-':
                    advance();
                    if (current() == '-') {
                        advance();
                        kind = token::kind::minus_minus;
                    }
                    else if (current() == '=') {
                        advance();
                        kind = token::kind::minus_equal;
                    }
                    else {
                        kind = token::kind::minus;
                    }
                    break;
                case '*':
                    advance();
                    if (current() == '*' && next() == '=') {
                        advance();
                        advance();
                        kind = token::kind::star_star_equal;
                    }
                    else if (current() == '=') {
                        advance();
                        kind = token::kind::star_equal;
                    }
                    else {
                        kind = token::kind::star;
                    }
                    break;
                case '%':
                    advance();
                    if (current() == '=') {
                        advance();
                        kind = token::kind::percent_equal;
                    }
                    else {
                        kind = token::kind::percent;
                    }
                    break;
                case '!':
                    advance();
                    if (current() == '=') {
                        advance();
                        kind = token::kind::bang_equal;
                    }
                    else {
                        kind = token::kind::bang;
                    }
                    break;
                case '>':
                    advance();
                    if (current() == '>' && next() == '=') {
                        advance();
                        advance();
                        kind = token::kind::right_right_equal;
                    }
                    else if (current() == '=') {
                        advance();
                        kind = token::kind::greater_equal;
                    }
                    else {
                        kind = token::kind::greater;
                    }
                    break;
                case '<':
                    advance();
                    if (current() == '<') {
                        advance();
                        if (current() == '=') {
                            advance();
                            kind = token::kind::left_left_equal;
                        }
                        else {
                            kind = token::kind::less_less;
                        }
                    }
                    else if (current() == '=') {
                        advance();
                        kind = token::kind::less_equal;
                    }
                    else {
                        kind = token::kind::less;
                    }
                    break;
                case '=':
                    advance();
                    if (current() == '=') {
                        advance();
                        kind = token::kind::equal_equal;
                    }
                    else if (current() == '>') {
                        advance();
                        kind = token::kind::equal_greater;
                    }
                    else {
                        kind = token::kind::equal;
                    }
                    break;
                case '&':
                    advance();
                    if (current() == '=') {
                        advance();
                        kind = token::kind::amp_equal;
                    }
                    else {
                        kind = token::kind::amp;
                    }
                    break;
                case '|':
                    advance();
                    if (current() == '|') {
                        advance();
                        kind = token::kind::line_line;
                    }
                    else if (current() == '=') {
                        advance();
                        kind = token::kind::line_equal;
                    }
                    else {
                        kind = token::kind::line;
                    }
                    break;
                case '^':
                    advance();
                    if (current() == '=') {
                        advance();
                        kind = token::kind::caret_equal;
                    }
                    else {
                        kind = token::kind::caret;
                    }
                    break;
                case '~':
                    advance();
                    kind = token::kind::tilde;
                    break;
                default:
                    if (!identifier()) {
                        // error state for invalid unicode character
                        struct state err = state_;
                        advance();
                        get_tokens()->push_back(token::builder().valid(false).location(err.location).lexeme(extract(err, state_)).eol(eol()).build());
                        byte encoded[5];
                        utf8::encode(*err.iter, encoded);
                        // unrecognized character in input
                        diagnostic diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(err.location)
                                        .message(diagnostic::format("I don't think U+${x} (aka `$`) is a valid sh*t in the middle of a program.", *err.iter, encoded))
                                        .explanation("I remind you that only some unicode characters are valid inside the program, including: • letters `A`..`Z` `a`..`z` • numbers `0`..`9` • punctuation `{` `}` `[` `]` `(` `)` `.` `...` `:` `,` `;` • operators `|` `||` `&` `&&` `!` `~` `=` `<` `>` `<=` `>=` `==` ecc")
                                        .highlight(range(err, state_), "garbage")
                                        .build();
                        publisher_.publish(diag);
                    }
            }
            
            // operator or punctuation token
            if (kind != token::kind::unknown) {
                get_tokens()->push_back(token::builder().kind(kind).location(saved.location).lexeme(extract(saved, state_)).eol(eol()).build());
            }
        }

        // reached end of file
        if (inject_eof) {
            get_tokens()->push_back(token::builder().kind(token::kind::eof).location(state_.location).artificial(true).eol(true).build());
        }
        // restore previous tokens container, this is a backtracking for recursion
        set_tokens(saved_tokens);
    }
}