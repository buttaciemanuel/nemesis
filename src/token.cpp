#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "nemesis/tokenizer/token.hpp"

std::ostream& operator<<(std::ostream& stream, nemesis::token t)
{
    if (t.location().valid()) {
        stream << t.location().filename << ":"
               << t.location().line << ":" 
               << t.location().column << " ";
    }
    stream << nemesis::impl::to_string(t.kind());
    
    if (!t.is(nemesis::token::kind::eof))
        stream << " `" << nemesis::impl::color::white << t.lexeme() << nemesis::impl::color::reset << "`";
    
    if (t.eol)
        stream << " <eol>";
    if (!t.valid)
        stream << " <invalid>";
    if (t.artificial)
        stream << " <artificial>";
    
    return stream;
}

bool operator==(const nemesis::token& lhs, const nemesis::token& rhs)
{
    return lhs.kind() == rhs.kind() && lhs.lexeme() == rhs.lexeme() && lhs.location().filename == rhs.location().filename && lhs.location().line == rhs.location().line && lhs.location().column == rhs.location().column;
}

bool operator!=(const nemesis::token& lhs, const nemesis::token& rhs)
{
    return lhs.kind() != rhs.kind() || !(lhs.lexeme() == rhs.lexeme()) || !(lhs.location().filename == rhs.location().filename) || lhs.location().line != rhs.location().line || lhs.location().column != rhs.location().column;
}

namespace nemesis {
    std::string to_string(enum token::kind k) 
    {
        return "";
    }

    token::token(enum token::kind k, utf8::span lexeme, source_location loc) :
        eol(0),
        valid(1),
        artificial(0),
        kind_(k),
        lexeme_(lexeme),
        location_(loc)
    {}

    token::token() : eol(0), valid(1), artificial(0) {}

    enum token::kind token::kind() const { return kind_; }
        
    utf8::span token::lexeme() const { return lexeme_; }
        
    source_location token::location() const { return location_; }

    source_range token::range() const
    {
        // if it is a multiline comment
        if (kind_ == kind::comment && lexeme_.cdata()[1] == '*') {
            // skip last '\n' if it closes the token like in a single line comment
            unsigned newlines = std::count(lexeme_.cdata(), lexeme_.cdata() + lexeme_.size(), '\n');
            
            int last = lexeme_.size() - 1;
            
            while (last >= 0 && lexeme_.cdata()[last] != '\n') --last;
            ++last;
            
            unsigned ecols = utf8::span(lexeme_.cdata() + last, lexeme_.size() - last).width();
            
            return source_range(location_.line, location_.column, location_.line + newlines, ecols, location_.filename);
        }
        else if (kind_ == kind::identifier) {
            unsigned cols = 0;
            for (auto i = lexeme_.begin(); i != lexeme_.end() && *i != '('; cols += utf8::width(*i), ++i);
            return source_range(location_, cols);
        }

        return source_range(location_, lexeme_.width());
    }

    std::string token::description() const
    {
        std::ostringstream oss;
        oss << *this;
        return oss.str();
    }

    bool token::is(enum kind k) const { return kind_ == k; }
    
    bool token::is_literal() const
    {
        switch (kind_) {
            case kind::char_literal:
            case kind::string_literal:
            case kind::integer_literal:
            case kind::real_literal:
            case kind::imag_literal:
            case kind::true_kw:
            case kind::false_kw:
                return true;
            default:
                break;
        }

        return false;
    }

    bool token::is_keyword() const
    {
        switch (kind_) {
            case token::kind::app_kw:
            case token::kind::as_kw:
            case token::kind::behaviour_kw:
            case token::kind::break_kw:
            case token::kind::concept_kw:
            case token::kind::const_kw:
            case token::kind::continue_kw:
            case token::kind::else_kw:
            case token::kind::extend_kw:
            case token::kind::extern_kw:
            case token::kind::false_kw:
            case token::kind::for_kw:
            case token::kind::function_kw:
            case token::kind::hide_kw:
            case token::kind::if_kw:
            case token::kind::in_kw:
            case token::kind::is_kw:
            case token::kind::mutable_kw:
            case token::kind::lib_kw:
            case token::kind::range_kw:
            case token::kind::return_kw:
            case token::kind::when_kw:
            case token::kind::static_kw:
            case token::kind::test_kw:
            case token::kind::true_kw:
            case token::kind::type_kw:
            case token::kind::union_kw:
            case token::kind::use_kw:
            case token::kind::val_kw:
                return true;
            default:
                break;
        }

        return false;
    }

    bool token::is_unary_operator() const
    {
        switch (kind_) {
            case kind::plus:
            case kind::minus:
            case kind::tilde:
            case kind::bang:
            case kind::plus_plus:
            case kind::minus_minus:
            case kind::amp:
            case kind::star:
                return true;
            default:
                break;
        }

        return false;
    }

    bool token::is_binary_operator() const
    {
        switch (kind_) {
            case kind::dot:
            case kind::plus:
            case kind::minus:
            case kind::star:
            case kind::slash:
            case kind::percent:
            case kind::star_star:
            case kind::less_less:
            case kind::greater_greater:
            case kind::amp:
            case kind::caret:
            case kind::line:
            case kind::less:
            case kind::greater:
            case kind::less_equal:
            case kind::greater_equal:
            case kind::equal_equal:
            case kind::bang_equal:
            case kind::amp_amp:
            case kind::line_line:
            case kind::dot_dot:
            case kind::dot_dot_equal:
                return true;
            default:
                break;
        }

        return false;
    }

    bool token::is_assignment_operator() const
    {
        switch (kind_) {
            case kind::equal:
            case kind::star_star_equal:
            case kind::star_equal:
            case kind::slash_equal:
            case kind::percent_equal:
            case kind::plus_equal:
            case kind::minus_equal:
            case kind::left_left_equal:
            case kind::right_right_equal:
            case kind::amp_equal:
            case kind::line_equal:
            case kind::caret_equal:
                return true;
            default:
                break;
        }

        return false;
    }

    bool token::is_operator() const
    {
        return is_unary_operator() || 
               is_binary_operator() || 
               is_assignment_operator();
    }

    token::builder& token::builder::kind(enum token::kind k)
    {
        token_.kind_ = k;
        return *this;
    }
        
    token::builder& token::builder::lexeme(utf8::span lexeme)
    {
        token_.lexeme_ = lexeme;
        return *this;
    }
    
    token::builder& token::builder::location(source_location loc)
    {
        token_.location_ = loc;
        return *this;
    }
    
    token::builder& token::builder::eol(bool flag)
    {
        token_.eol = flag;
        return *this;
    }

    token::builder& token::builder::valid(bool flag)
    {
        token_.valid = flag;
        return *this;
    }

    token::builder& token::builder::artificial(bool flag)
    {
        token_.artificial = flag;
        return *this;
    }
    
    token token::builder::build() const { return token_; }

    namespace impl {
        std::string to_string(enum token::kind k)
        {
            switch (k) {
                case token::kind::identifier: return "identifier";
                case token::kind::char_literal: return "char_literal";
                case token::kind::string_literal: return "string_literal";
                case token::kind::integer_literal: return "integer_literal";
                case token::kind::real_literal: return "real_literal";
                case token::kind::imag_literal: return "imag_literal";
                case token::kind::comment: return "comment";
                case token::kind::app_kw: return "app_kw";
                case token::kind::as_kw: return "as_kw";
                case token::kind::behaviour_kw: return "behaviour_kw";
                case token::kind::break_kw: return "break_kw";
                case token::kind::concept_kw: return "concept_kw";
                case token::kind::const_kw: return "const_kw";
                case token::kind::continue_kw: return "continue_kw";
                case token::kind::else_kw: return "else_kw";
                case token::kind::ensure_kw: return "ensure_kw";
                case token::kind::extend_kw: return "extend_kw";
                case token::kind::extern_kw: return "extern_kw";
                case token::kind::false_kw: return "false_kw";
                case token::kind::for_kw: return "for_kw";
                case token::kind::function_kw: return "function_kw";
                case token::kind::hide_kw: return "hide_kw";
                case token::kind::if_kw: return "if_kw";
                case token::kind::in_kw: return "in_kw";
                case token::kind::invariant_kw: return "invariant_kw";
                case token::kind::is_kw: return "is_kw";
                case token::kind::mutable_kw: return "mutable_kw";
                case token::kind::lib_kw: return "lib_kw";
                case token::kind::range_kw: return "range_kw";
                case token::kind::return_kw: return "return_kw";
                case token::kind::require_kw: return "require_kw";
                case token::kind::when_kw: return "when_kw";
                case token::kind::static_kw: return "static_kw";
                case token::kind::test_kw: return "test_kw";
                case token::kind::true_kw: return "true_kw";
                case token::kind::type_kw: return "type_kw";
                case token::kind::union_kw: return "union_kw";
                case token::kind::use_kw: return "use_kw";
                case token::kind::val_kw: return "val_kw";
                case token::kind::dot: return "dot";
                case token::kind::colon: return "colon";
                case token::kind::semicolon: return "semicolon";
                case token::kind::comma: return "comma";
                case token::kind::dot_dot: return "dot_dot";
                case token::kind::dot_dot_equal: return "dot_dot_equal";
                case token::kind::ellipsis: return "ellipsis";
                case token::kind::equal_greater: return "equal_greater";
                case token::kind::left_parenthesis: return "left_parenthesis";
                case token::kind::right_parenthesis: return "right_parenthesis";
                case token::kind::left_bracket: return "left_bracket";
                case token::kind::right_bracket: return "right_bracket";
                case token::kind::left_brace: return "left_brace";
                case token::kind::right_brace: return "right_brace";
                case token::kind::plus: return "plus";
                case token::kind::minus: return "minus";
                case token::kind::star_star: return "star_star";
                case token::kind::star: return "star";
                case token::kind::slash: return "slash";
                case token::kind::percent: return "percent";
                case token::kind::bang: return "bang";
                case token::kind::greater_greater: return "greater_greater";
                case token::kind::less_less: return "less_less";
                case token::kind::less: return "less";
                case token::kind::greater: return "greater";
                case token::kind::less_equal: return "less_equal";
                case token::kind::greater_equal: return "greater_equal";
                case token::kind::equal_equal: return "equal_equal";
                case token::kind::bang_equal: return "bang_equal";
                case token::kind::amp_amp: return "amp_amp";
                case token::kind::line_line: return "line_line";
                case token::kind::equal: return "equal";
                case token::kind::star_star_equal: return "star_star_equal";
                case token::kind::star_equal: return "star_equal";
                case token::kind::slash_equal: return "slash_equal";
                case token::kind::percent_equal: return "percent_equal";
                case token::kind::plus_equal: return "plus_equal";
                case token::kind::minus_equal: return "minus_equal";
                case token::kind::left_left_equal: return "left_left_equal";
                case token::kind::right_right_equal: return "right_right_equal";
                case token::kind::amp_equal: return "amp_equal";
                case token::kind::line_equal: return "line_equal";
                case token::kind::caret_equal: return "xline_equal";
                case token::kind::plus_plus: return "plus_plus";
                case token::kind::minus_minus: return "minus_minus";
                case token::kind::tilde: return "tilde";
                case token::kind::amp: return "amp";
                case token::kind::line: return "line";
                case token::kind::caret: return "caret";
                case token::kind::eof: return "eof";
                case token::kind::unknown: return "unknown";
                default:
                    break;
            }

            throw std::invalid_argument("impl::to_string(): invalid token kind");
        }
    }
}