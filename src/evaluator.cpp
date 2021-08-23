#include "nemesis/analysis/evaluator.hpp"
#include "nemesis/analysis/checker.hpp"
#include "nemesis/analysis/type.hpp"

#include <iostream>

std::ostream& operator<<(std::ostream& os, nemesis::constval val)
{
    if (!val.type) return os << "unknown";

    switch (val.type->category()) {
        case nemesis::ast::type::category::unknown_type:
            return os << "unknown";
        case nemesis::ast::type::category::bool_type:
            return os << std::boolalpha << val.b;
        case nemesis::ast::type::category::char_type:
        {
            nemesis::byte units[5] = { 0 };
            units[nemesis::utf8::encode(val.ch, units)] = '\0';
            return os << "\'" << units << "\'";
        }
        case nemesis::ast::type::category::chars_type:
            return os << "\"" << val.s << "\"";
        case nemesis::ast::type::category::string_type:
            return os << "\"" << val.s << "\"s";
        case nemesis::ast::type::category::integer_type:
        {
            auto type = std::dynamic_pointer_cast<nemesis::ast::integer_type>(val.type);
            if (type->is_signed()) return os << val.i << "i" << type->bits();
            else return os << val.u << "u" << type->bits();
        }
        case nemesis::ast::type::category::rational_type:
        {
            return os << val.r;
        }
        case nemesis::ast::type::category::float_type:
        {
            auto type = std::dynamic_pointer_cast<nemesis::ast::float_type>(val.type);
            return os << val.f << "f" << type->bits();
        }
        case nemesis::ast::type::category::complex_type:
        {
            os << val.c;
        }
        case nemesis::ast::type::category::tuple_type:
            if (val.seq.empty()) return os << "()";
            else {
                os << "(" << val.seq.front();
                for (size_t i = 1; i < val.seq.size(); ++i) os << ", " << val.seq.at(i);
                return os << ")";
            }
        case nemesis::ast::type::category::array_type:
            if (val.seq.empty()) return os << "[]";
            else {
                os << "[" << val.seq.front();
                for (size_t i = 1; i < val.seq.size(); ++i) os << ", " << val.seq.at(i);
                return os << "]";
            }
        default:
            throw std::invalid_argument("constval::operator<<: invalid constant value type " + val.type->string());
    }

    return os;
}

bool operator==(const nemesis::constval& l, const nemesis::constval& r)
{
    if ((l.type->category() == nemesis::ast::type::category::chars_type && r.type->category() == nemesis::ast::type::category::string_type) ||
        (l.type->category() == nemesis::ast::type::category::string_type && r.type->category() == nemesis::ast::type::category::chars_type) ||
        l.type->category() == r.type->category()) {
        switch (l.type->category()) {
            case nemesis::ast::type::category::bool_type:
                return l.b == r.b;
            case nemesis::ast::type::category::char_type:
                return l.ch == r.ch;
            case nemesis::ast::type::category::chars_type:
            case nemesis::ast::type::category::string_type:
                return l.s == r.s;
            case nemesis::ast::type::category::integer_type:
            {
                auto ltype = std::dynamic_pointer_cast<nemesis::ast::integer_type>(l.type);
                auto rtype = std::dynamic_pointer_cast<nemesis::ast::integer_type>(r.type);
                if ((ltype->is_signed() && !rtype->is_signed()) || (!ltype->is_signed() && rtype->is_signed())) throw nemesis::evaluator::error();
                if (ltype->is_signed()) return l.i == r.i;
                else return l.u == r.u;
            }
            case nemesis::ast::type::category::rational_type:
                return l.r == r.r;
            case nemesis::ast::type::category::float_type:
                return l.f == r.f;
            case nemesis::ast::type::category::complex_type:
                return l.c == r.c;
            case nemesis::ast::type::category::tuple_type:
            {
                auto ltype = std::dynamic_pointer_cast<nemesis::ast::tuple_type>(l.type);
                auto rtype = std::dynamic_pointer_cast<nemesis::ast::tuple_type>(r.type);
                if (ltype->length() != rtype->length()) throw nemesis::evaluator::error();
                for (size_t i = 0; i < ltype->length(); ++i) {
                    if (!(l.seq.at(i) == r.seq.at(i))) return false;
                }
                return true;
            }
            case nemesis::ast::type::category::array_type:
            {
                auto ltype = std::dynamic_pointer_cast<nemesis::ast::array_type>(l.type);
                auto rtype = std::dynamic_pointer_cast<nemesis::ast::array_type>(r.type);
                if (ltype->size() != rtype->size()) throw nemesis::evaluator::error();
                for (size_t i = 0; i < ltype->size(); ++i) {
                    if (!(l.seq.at(i) == r.seq.at(i))) return false;
                }
                return true;
            }
            default:
                return false;
        }
    }

    throw nemesis::evaluator::error();
}

namespace nemesis {
    namespace impl {
        struct hash_pair {
            template<class A, class B>
            size_t operator()(const std::pair<A, B>& p) const {
                size_t hash = std::hash<A>()(p.first);
                hash <<= sizeof(size_t) * 4;
                hash ^= std::hash<B>()(p.second);
                return std::hash<size_t>()(hash);
            }
        };

        struct hash_vector {
            size_t operator()(const std::vector<constval>& values) const {
                if (values.empty()) return 0;
                size_t hash = values.front().hash();
                for (size_t i = 1; i < values.size(); ++i) {
                    hash <<= sizeof(size_t) * 4;
                    hash ^= values.at(i).hash();
                }
                return std::hash<size_t>()(hash);
            }
        };
    }

    size_t constval::hash() const
    {
        switch (this->type->category()) {
            case nemesis::ast::type::category::unknown_type:
                return std::numeric_limits<size_t>::max();
            case nemesis::ast::type::category::bool_type:
                return static_cast<size_t>(this->b);
            case nemesis::ast::type::category::char_type:
                return static_cast<size_t>(this->ch);
            case nemesis::ast::type::category::chars_type:
            case nemesis::ast::type::category::string_type:
                return std::hash<std::string>()(this->s);
            case nemesis::ast::type::category::integer_type:
            {
                auto type = std::dynamic_pointer_cast<nemesis::ast::integer_type>(this->type);
                if (type->is_signed()) return static_cast<size_t>(this->i.value());
                return static_cast<size_t>(this->u.value());
            }
            case nemesis::ast::type::category::rational_type:
            {
                return impl::hash_pair()(std::make_pair(this->r.numerator().value(), this->r.denominator().value()));
            }
            case nemesis::ast::type::category::float_type:
            {
                return std::hash<long double>()(this->f.value());
            }
            case nemesis::ast::type::category::complex_type:
            {
                return impl::hash_pair()(std::make_pair(this->c.real().value(), this->c.imag().value()));
            }
            case nemesis::ast::type::category::tuple_type:
            case nemesis::ast::type::category::array_type:
                return impl::hash_vector()(this->seq);
            default:
                throw std::invalid_argument("constval::hash(): invalid constant value type " + this->type->string());
        }

        return std::numeric_limits<size_t>::max();
    }

    std::string constval::str() const
    {
        std::ostringstream oss;
        oss << *this;
        return oss.str();
    }

    std::string constval::description() const
    {
        std::ostringstream os;
        os << "constval(";

        switch (this->type->category()) {
            case nemesis::ast::type::category::unknown_type:
                os << "unknown";
                break;
            case nemesis::ast::type::category::bool_type:
                os << "bool, " << std::boolalpha << this->b;
                break;
            case nemesis::ast::type::category::char_type:
            {
                nemesis::byte units[5] = { 0 };
                units[nemesis::utf8::encode(this->ch, units)] = '\0';
                os << "char, \'" << units << "\'";
                break;
            }
            case nemesis::ast::type::category::chars_type:
                os << "chars, \"" << this->s << "\"";
                break;
            case nemesis::ast::type::category::string_type:
                os << "string, \"" << this->s << "\"s";
                break;
            case nemesis::ast::type::category::integer_type:
            {
                auto type = std::dynamic_pointer_cast<nemesis::ast::integer_type>(this->type);
                if (type->is_signed()) os << "i" << type->bits() << ", " << this->i << "i" << type->bits();
                else os << "u" << type->bits() << ", " << this->u << "u" << type->bits();
                break;
            }
            case nemesis::ast::type::category::rational_type:
            {
                auto type = std::dynamic_pointer_cast<nemesis::ast::rational_type>(this->type);
                os << "r" << type->bits() << ", " << this->r;
                break;
            }
            case nemesis::ast::type::category::float_type:
            {
                auto type = std::dynamic_pointer_cast<nemesis::ast::float_type>(this->type);
                os << "f" << type->bits() << ", " << this->f << "f" << type->bits();
                break;
            }
            case nemesis::ast::type::category::complex_type:
            {
                auto type = std::dynamic_pointer_cast<nemesis::ast::complex_type>(this->type);
                os << "c" << type->bits() << ", ";
                os << this->c;
                break;
            }
            case nemesis::ast::type::category::tuple_type:
                if (this->seq.empty()) os << "tuple, ()";
                else {
                    os << "tuple, (" << this->seq.front().description();
                    for (size_t i = 1; i < this->seq.size(); ++i) os << ", " << this->seq.at(i).description();
                    os << ")";
                }
                break;
            case nemesis::ast::type::category::array_type:
                if (this->seq.empty()) os << "array, []";
                else {
                    os << "array, [" << this->seq.front().description();
                    for (size_t i = 1; i < this->seq.size(); ++i) os << ", " << this->seq.at(i).description();
                    os << "]";
                }
                break;
            default:
                throw std::invalid_argument("constval::operator<<: invalid constant value type " + this->type->string());
        }

        os << ")";
        return os.str();
    }

    std::string constval::simple() const
    {
        std::ostringstream os;

        switch (this->type->category()) {
            case nemesis::ast::type::category::unknown_type:
                os << "_";
                break;
            case nemesis::ast::type::category::bool_type:
                os << std::boolalpha << this->b;
                break;
            case nemesis::ast::type::category::char_type:
            {
                nemesis::byte units[5] = { 0 };
                units[nemesis::utf8::encode(this->ch, units)] = '\0';
                os << "\'" << units << "\'";
                break;
            }
            case nemesis::ast::type::category::chars_type:
                os << "\"" << this->s << "\"";
                break;
            case nemesis::ast::type::category::string_type:
                os << "\"" << this->s << "\"";
                break;
            case nemesis::ast::type::category::integer_type:
            {
                auto type = std::dynamic_pointer_cast<nemesis::ast::integer_type>(this->type);
                if (type->is_signed()) os << this->i;
                else os << this->u;
                break;
            }
            case nemesis::ast::type::category::rational_type:
            {
                auto type = std::dynamic_pointer_cast<nemesis::ast::rational_type>(this->type);
                os << this->r;
                break;
            }
            case nemesis::ast::type::category::float_type:
            {
                auto type = std::dynamic_pointer_cast<nemesis::ast::float_type>(this->type);
                os << this->f;
                break;
            }
            case nemesis::ast::type::category::complex_type:
            {
                auto type = std::dynamic_pointer_cast<nemesis::ast::complex_type>(this->type);
                os << this->c;
                break;
            }
            case nemesis::ast::type::category::tuple_type:
                if (this->seq.empty()) os << "()";
                else {
                    os << "(" << this->seq.front().simple();
                    for (size_t i = 1; i < this->seq.size(); ++i) os << ", " << this->seq.at(i).simple();
                    os << ")";
                }
                break;
            case nemesis::ast::type::category::array_type:
                if (this->seq.empty()) os << "[]";
                else {
                    os << "[" << this->seq.front().simple();
                    for (size_t i = 1; i < this->seq.size(); ++i) os << ", " << this->seq.at(i).simple();
                    os << "]";
                }
                break;
            default:
                throw std::invalid_argument("constval::simple: invalid constant value type " + this->type->string());
        }

        return os.str();
    }

    namespace impl {
        static const std::string const_expr_explanation = "Only numbers, characters, strings, tuples or arrays are allowed inside constant expressions. \\ You can do: • arithmetic operations, for examples `1 + e ** (PI * 1i)` • type conversions, like `1 / 2 as f32` • string manipulations, like `\"Hello \" + '\\u1F30E'` • array or tuple manipulations, for example `(1, ['a', 'b']).1[0] == 'a'`";

        int64_t stoi(const std::string& s) 
        {
            int64_t result;
            
            if (s.size() > 1) {
                if (s.at(0) == '0' && s.at(1) == 'x') {
                    result = std::stoll(s.substr(2), nullptr, 16);
                }
                else if (s.at(0) == '0' && s.at(1) == 'o') {
                    result = std::stoll(s.substr(2), nullptr, 8);
                }
                else if (s.at(0) == '0' && s.at(1) == 'b') {
                    result = std::stoll(s.substr(2), nullptr, 2);
                }
                else {
                    result = std::stoll(s);
                }
            }
            else {
                result = std::stoll(s);
            }

            return result; 
        }

        uint64_t stoui(const std::string& s) 
        {
            uint64_t result;
            
            if (s.size() > 1) {
                if (s.at(0) == '0' && s.at(1) == 'x') {
                    result = std::stoull(s.substr(2), nullptr, 16);
                }
                else if (s.at(0) == '0' && s.at(1) == 'o') {
                    result = std::stoull(s.substr(2), nullptr, 8);
                }
                else if (s.at(0) == '0' && s.at(1) == 'b') {
                    result = std::stoull(s.substr(2), nullptr, 2);
                }
                else {
                    result = std::stoull(s);
                }
            }
            else {
                result = std::stoull(s);
            }

            return result; 
        }

        long double stof(const std::string& s) 
        {
            long double result;

            result = std::stold(s);

            return result; 
        }

        codepoint stoc(const std::string& s)
        {
            if (s.front() == '\\') {
                switch (s.at(1)) {
                    case 'a': return '\a';
                    case 'f': return '\f';
                    case 'n': return '\n';
                    case 'r': return '\r';
                    case 't': return '\t';
                    case 'v': return '\v';
                    case '"': return '\"';
                    case '\'': return '\'';
                    case '\\': return '\\';
                    case '0': return '\0';
                    case 'u': return std::stoi(s.substr(2), nullptr, 16);
                    case 'o': return std::stoi(s.substr(2), nullptr, 8);
                    case 'x': return std::stoi(s.substr(2), nullptr, 16);
                    case 'b': return std::stoi(s.substr(1), nullptr, 2);
                    default:
                        break;
                }
            }
            
            return utf8::decode(reinterpret_cast<const byte*>(s.data()));
        }

        std::string stos(const std::string& s)
        {
            std::string result;

            for (size_t i = 0; i < s.size();) {
                if (s.at(i) == '\\') {
                    switch (s.at(++i)) {
                        case 'a': result.append("\a"); ++i; break;
                        case 'f': result.append("\f"); ++i; break;
                        case 'n': result.append("\n"); ++i; break;
                        case 'r': result.append("\r"); ++i; break;
                        case 't': result.append("\t"); ++i; break;
                        case 'v': result.append("\v"); ++i; break;
                        case '"': result.append("\""); ++i; break;
                        case '\'': result.append("\'"); ++i; break;
                        case '\\': result.append("\\"); ++i; break;
                        case '0': result.append("\0"); ++i; break;
                        case 'u':
                        {
                            size_t count = 0;
                            constval::character codepoint = std::stoi(s.substr(++i), &count, 16);
                            byte units[5] = { 0 };
                            size_t len = utf8::encode(codepoint, units);
                            result.append(reinterpret_cast<char*>(units), len);
                            i += count;
                            break;
                        }
                        case 'o':
                        {
                            size_t count = 0;
                            result += (char) std::stoi(s.substr(++i), &count, 8);
                            i += count;
                            break;
                        }
                        case 'x':
                        {
                            size_t count = 0;
                            result += (char) std::stoi(s.substr(++i), &count, 16);
                            i += count;
                            break;
                        }
                        case 'b':
                        {
                            size_t count = 0;
                            result += (char) std::stoi(s.substr(++i), &count, 2);
                            i += count;
                            break;
                        }
                        default:
                            break;
                    }
                }
                else {
                    result += s.at(i++);
                }
            }
            
            return result;
        }
    }

    evaluator::evaluator(checker& checker) :
        checker_(checker),
        stack_()
    {}

    constval evaluator::evaluate(ast::pointer<ast::expression> expr)
    {
        constval result;

        try {
            expr->accept(*this);
            result = pop();
        }
        catch (evaluator::error& e) {
            result.type = types::unknown();
        }
        catch (evaluator::generic_evaluation& g) {
            throw;
        }

        return result;
    }

    constval evaluator::integer_parse(const std::string& value)
    {
        constval result;
        size_t suffix = value.find('u');
        
        if (suffix == value.npos) suffix = value.find('i');
        
        if (suffix != value.npos) {
            result.type = types::builtin(value.substr(suffix));
            if (value.at(suffix) == 'i') {
                try {
                    result.i.size(std::dynamic_pointer_cast<ast::integer_type>(result.type)->bits());
                    result.i.value(impl::stoi(value.substr(0, suffix)));
                }
                catch (...)
                {
                    result.i.overflow(true);
                    return result; 
                }
            }
            else {
                try {
                    result.u.size(std::dynamic_pointer_cast<ast::integer_type>(result.type)->bits());
                    result.u.value(impl::stoui(value.substr(0, suffix)));
                } 
                catch (...) 
                { 
                    result.u.overflow(true); 
                    return result;
                }
            }
        }
        else {
            result.type = types::sint(32);
            try {
                result.i.size(32);
                result.i.value(impl::stoi(value));
            } 
            catch (...) 
            { 
                result.i.overflow(true);
                return result; 
            }
        }

        return result;
    }

    constval evaluator::float_parse(const std::string& value)
    {
        constval result;
        size_t suffix = value.find('f');
        
        if (suffix != value.npos) {
            result.type = types::builtin(value.substr(suffix));
            try {
                result.f.size(std::dynamic_pointer_cast<ast::float_type>(result.type)->bits());
                result.f.value(impl::stof(value.substr(0, suffix)));
            }
            catch (...) {
                result.f.invalid(true);
                return result;
            }
        }
        else {
            result.type = types::floating(32);
            try {
                result.f.size(32);
                result.f.value(impl::stof(value.substr(0, suffix)));
            }
            catch (...) {
                result.f.invalid(true);
                return result;
            }
        }

        return result;
    }

    constval evaluator::imag_parse(const std::string& value)
    {
        constval result;
        size_t suffix = value.find('i');
        constval::real imag;

        result.type = types::complex(64);
        result.c = constval::complex();

        try {
            imag.value(impl::stof(value.substr(0, suffix)));
            result.c.imag(imag);
        }
        catch (...) {
            imag.invalid(true);
            result.c.imag(imag);
            return result;
        }

        return result;
    }

    void evaluator::test_operation_error(const ast::unary_expression& expr, const std::string& operation, constval result)
    {
        if (!result.type) throw std::invalid_argument(diagnostic::format("evaluator: test_operation_error: result type must be not null in $ operation", operation));
        
        switch (result.type->category()) {
            case ast::type::category::integer_type:
                if (result.i.overflow() || result.u.overflow()) {
                    checker_.error(expr, diagnostic::format("This $ will provoke an overflow, dammit!", operation), "", diagnostic::format("$ overflow", result.type->string()));
                    throw evaluator::error();
                }
                break;
            case ast::type::category::rational_type:
                if (result.r.overflow()) {
                    checker_.error(expr, diagnostic::format("This $ will provoke an overflow, dammit!", operation), "", diagnostic::format("$ overflow", result.type->string()));
                    throw evaluator::error();
                }
                break;
            case ast::type::category::float_type:
                if (result.f.zerodiv()) {
                    checker_.error(expr, "You are dividing by zero here! Didn't they tell you that's f*cking illegal?", "", "zero divisor");
                    throw evaluator::error();
                }
                else if (result.f.overflow()) {
                    checker_.error(expr, diagnostic::format("This $ will provoke an overflow, dammit!", operation), "", diagnostic::format("$ overflow", result.type->string()));
                    throw evaluator::error();
                }
                else if (result.f.invalid()) {
                    checker_.error(expr, diagnostic::format("This $ will return an invalid number, dammit!", operation));
                    throw evaluator::error();
                }
                else if (result.f.inexact()) {
                    checker_.warning(expr, diagnostic::format("This $ may return an inexact number, dammit!", operation));
                }
                break;
            case ast::type::category::complex_type:
                if (result.c.zerodiv()) {
                    checker_.error(expr, "You are dividing by zero here! Didn't they tell you that's f*cking illegal?", "", "zero divisor");
                    throw evaluator::error();
                }
                else if (result.c.overflow()) {
                    checker_.error(expr, diagnostic::format("This $ will provoke an overflow, dammit!", operation), "", diagnostic::format("$ overflow", result.type->string()));
                    throw evaluator::error();
                }
                else if (result.c.invalid()) {
                    checker_.error(expr, diagnostic::format("This $ will return an invalid number, dammit!", operation));
                    throw evaluator::error();
                }
                else if (result.c.inexact()) {
                    checker_.warning(expr, diagnostic::format("This $ may return an inexact number, dammit!", operation));
                }
                break;
            default:
                break;
        }
    }

    void evaluator::test_operation_error(const ast::binary_expression& expr, const std::string& operation, constval result)
    {
        if (!result.type) throw std::invalid_argument(diagnostic::format("evaluator: test_operation_error: result type must be not null in $ operation", operation));
        
        switch (result.type->category()) {
            case ast::type::category::integer_type:
                if (result.i.overflow() || result.u.overflow()) {
                    checker_.error(expr, diagnostic::format("This $ will provoke an overflow, dammit!", operation), "", diagnostic::format("$ overflow", result.type->string()));
                    throw evaluator::error();
                }
                break;
            case ast::type::category::rational_type:
                if (result.r.overflow()) {
                    checker_.error(expr, diagnostic::format("This $ will provoke an overflow, dammit!", operation), "", diagnostic::format("$ overflow", result.type->string()));
                    throw evaluator::error();
                }
                break;
            case ast::type::category::float_type:
                if (result.f.zerodiv()) {
                    checker_.error(expr, "You are dividing by zero here! Didn't they tell you that's f*cking illegal?", "", "zero divisor");
                    throw evaluator::error();
                }
                else if (result.f.overflow()) {
                    checker_.error(expr, diagnostic::format("This $ will provoke an overflow, dammit!", operation), "", diagnostic::format("$ overflow", result.type->string()));
                    throw evaluator::error();
                }
                else if (result.f.invalid()) {
                    checker_.error(expr, diagnostic::format("This $ will return an invalid number, dammit!", operation));
                    throw evaluator::error();
                }
                else if (result.f.inexact()) {
                    checker_.warning(expr, diagnostic::format("This $ may return an inexact number, dammit!", operation));
                }
                break;
            case ast::type::category::complex_type:
                if (result.c.zerodiv()) {
                    checker_.error(expr, "You are dividing by zero here! Didn't they tell you that's f*cking illegal?", "", "zero divisor");
                    throw evaluator::error();
                }
                else if (result.c.overflow()) {
                    checker_.error(expr, diagnostic::format("This $ will provoke an overflow, dammit!", operation), "", diagnostic::format("$ overflow", result.type->string()));
                    throw evaluator::error();
                }
                else if (result.c.invalid()) {
                    checker_.error(expr, diagnostic::format("This $ will return an invalid number, dammit!", operation));
                    throw evaluator::error();
                }
                else if (result.c.inexact()) {
                    checker_.warning(expr, diagnostic::format("This $ may return an inexact number, dammit!", operation));
                }
                break;
            default:
                break;
        }
    }

    void evaluator::visit(const ast::bit_field_type_expression& expr) 
    {
        if (!expr.size().valid) throw evaluator::error();

        constval result;
        result.type = types::bitfield(impl::stoui(expr.size().lexeme().string()));
        push(result);
    }

    void evaluator::visit(const ast::path_type_expression& expr) 
    {
        if (expr.invalid()) throw evaluator::error();

        constval result;
        result.type = types::builtin(expr.annotation().type->string());
        if (!result.type) throw evaluator::error();
        push(result);
    }
        
    void evaluator::visit(const ast::array_type_expression& expr)
    {
        if (expr.invalid() || !expr.size()) throw evaluator::error();

        constval result;
        expr.element_type()->accept(*this);
        constval base = pop();
        expr.size()->accept(*this);
        constval len = pop();
        
        if (auto length = std::dynamic_pointer_cast<ast::integer_type>(len.type)) {
            if (length->is_signed()) result.type = types::array(base.type, len.i.value());
            else result.type = types::array(base.type, len.u.value());
        }
        else {
            throw evaluator::error();
        }
        
        push(result);
    }
    
    void evaluator::visit(const ast::tuple_type_expression& expr) 
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();

        constval result;
        ast::types types;

        for (auto tyexpr : expr.types()) {
            tyexpr->accept(*this);
            result.seq.push_back(pop());
            types.push_back(result.seq.back().type);
        }
        
        if (expr.types().empty()) result.type = types::unit();
        else result.type = types::tuple(types);
        push(result);
    }
    
    void evaluator::visit(const ast::pointer_type_expression& expr) 
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use pointer type in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error(); 
    }
        
    void evaluator::visit(const ast::function_type_expression& expr) 
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use function type in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error(); 
    }
        
    void evaluator::visit(const ast::variant_type_expression& expr) 
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use variant type in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error(); 
    }

    void evaluator::visit(const ast::record_type_expression& expr) 
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use record type in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error(); 
    }

    void evaluator::visit(const ast::literal_expression& expr)
    {
        if (expr.invalid() || !expr.value().valid) throw evaluator::error();

        constval result;
        std::string value = expr.value().lexeme().string();

        switch (expr.value().kind()) {
            case token::kind::true_kw:
                result.type = types::boolean();
                result.b = true;
                break;
            case token::kind::false_kw:
                result.type = types::boolean();
                result.b = false;
                break;
            case token::kind::integer_literal:
                result = integer_parse(value);
                if (result.i.overflow() || result.u.overflow()) {
                    checker_.error(expr.range(), "This integer number will provoke a damn overflow, idiot!", "", diagnostic::format("too large for type $", result.type->string()));
                    throw evaluator::error();
                }
                break;
            case token::kind::real_literal:
                result = float_parse(value);
                if (result.f.invalid()) {
                    checker_.error(expr.range(), "This real number is not valid, idiot!");
                    throw evaluator::error();
                }
                else if (result.f.undeflow()) {
                    checker_.error(expr.range(), "This real number will provoke a damn underflow, idiot!", "", diagnostic::format("too small for type $", result.type->string()));
                    throw evaluator::error();
                }
                else if (result.f.overflow()) {
                    checker_.error(expr.range(), "This real number will provoke a damn overflow, idiot!", "", diagnostic::format("too large for type $", result.type->string()));
                    throw evaluator::error();
                }
                break;
            case token::kind::imag_literal:
                result = imag_parse(value);
                if (result.c.imag().invalid()) {
                    checker_.error(expr.range(), "This imaginary number is not valid, idiot!");
                    throw evaluator::error();
                }
                else if (result.c.imag().undeflow()) {
                    checker_.error(expr.range(), "This imaginary number will provoke a damn underflow, idiot!", "", diagnostic::format("too small for type $", result.type->string()));
                    throw evaluator::error();
                }
                else if (result.c.imag().overflow()) {
                    checker_.error(expr.range(), "This imaginary number will provoke a damn overflow, idiot!", "", diagnostic::format("too large for type $", result.type->string()));
                    throw evaluator::error();
                }
                break;
            case token::kind::char_literal:
                result.type = types::character();
                // skip first and last single quote
                result.ch = impl::stoc(value.substr(1, value.size() - 2));
                break;
            case token::kind::string_literal:
                if (value.back() == 's') {
                    result.type = types::string();
                    // skip first and last double quote and 's' suffix
                    result.s.assign(impl::stos(value.substr(1, value.size() - 3)));
                }
                else {
                    result.type = types::chars();
                    // skip first and last double quote
                    result.s.assign(impl::stos(value.substr(1, value.size() - 2)));
                }
                break;
            default:
                result.type = types::unknown();
        }

        push(result);
    }

    void evaluator::visit(const ast::identifier_expression& expr)
    {
        if (expr.invalid() || expr.is_generic()) throw evaluator::error();

        // if expression was recognized as type name then we don't need to perform any computation
        if (expr.annotation().istype) return;

        // if value was computed by a substitution then there is no need to compute the value as it is stored
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type && expr.annotation().value.type->category() != ast::type::category::generic_type) {
            push(expr.annotation().value);
            return;
        }

        constval result;
        const ast::declaration* decl = checker_.resolve_variable({ expr.identifier() });

        if (!decl) throw evaluator::error();
        // cannot compute value of generic constant parameter without substitution
        if (expr.annotation().isparametric || dynamic_cast<const ast::generic_const_parameter_declaration*>(decl)) {
            throw evaluator::generic_evaluation();
        }
        // otherwise proceed with computation if name raference is a constant
        else if (auto vardecl = dynamic_cast<const ast::var_declaration*>(decl)) {
            auto diag = diagnostic::builder()
                            .location(expr.range().begin())
                            .severity(diagnostic::severity::error)
                            .small(true)
                            .message("You cannot use variables, which are evaluated at run-time, inside constant expression, dammit!")
                            .highlight(expr.range(), "expected constant")
                            .note(vardecl->name().range(), diagnostic::format("As you can see `$` is not declared as constant.", vardecl->name().lexeme()))
                            .build();
                
            checker_.publisher().publish(diag);
            throw evaluator::error();
        }
        else if (auto vardecl = dynamic_cast<const ast::var_tupled_declaration*>(decl)) {
            auto builder = diagnostic::builder()
                            .location(expr.range().begin())
                            .severity(diagnostic::severity::error)
                            .small(true)
                            .message("You cannot use variables, which are evaluated at run-time, inside constant expression, dammit!")
                            .highlight(expr.range(), "expected constant");

            for (auto name : vardecl->names()) builder.highlight(name.range(), diagnostic::highlighter::mode::light);
                
            checker_.publisher().publish(builder.build());
            throw evaluator::error();
        }
        else if (auto constdecl = dynamic_cast<const ast::const_declaration*>(decl)) {
            result = constdecl->value()->annotation().value;
        }
        else if (auto constdecl = dynamic_cast<const ast::const_tupled_declaration*>(decl)) {
            result = constdecl->value()->annotation().value;
        }

        push(result);
    }

    void evaluator::visit(const ast::range_expression& expr) 
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use ranges in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error(); 
    }

    void evaluator::visit(const ast::tuple_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();

        constval result;

        if (expr.elements().empty()) {
            result.type = types::unit();
        }
        else {
            ast::types types;

            for (auto element : expr.elements()) {
                element->accept(*this);
                constval val = pop();
                result.seq.push_back(val);
                types.push_back(val.type);
            }

            result.type = types::tuple(types);
        }

        push(result);
    }
    
    void evaluator::visit(const ast::array_expression& expr) 
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();

        constval result;

        if (expr.elements().empty()) {
            result.type = types::unknown();
        }
        else {
            expr.elements().front()->accept(*this);
            constval val = pop();
            result.type = types::array(val.type, expr.elements().size());
            result.seq.push_back(val);

            for (size_t i = 1; i < expr.elements().size(); ++i) {
                expr.elements().at(i)->accept(*this);
                constval val = pop();
                result.seq.push_back(val);
            }
        }

        push(result);
    }

    void evaluator::visit(const ast::array_sized_expression& expr) 
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();

        constval result;
        expr.value()->accept(*this);
        constval val = pop();
        expr.size()->accept(*this);
        constval len = pop();
        
        for (size_t i = 0; i < len.u.value(); ++i) result.seq.push_back(val);
        
        result.type = types::array(val.type, len.u.value());
        push(result);
    }

    void evaluator::visit(const ast::parenthesis_expression& expr) 
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();

        expr.expression()->accept(*this);
    }

    void evaluator::visit(const ast::postfix_expression& expr) 
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), diagnostic::format("You cannot use operator `$` in constant expression, idiot!", expr.postfix().lexeme()), impl::const_expr_explanation); 
        throw evaluator::error(); 
    }

    void evaluator::visit(const ast::member_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        auto member = std::dynamic_pointer_cast<ast::identifier_expression>(expr.member());
        // left expression could be a type name inside a member expression
        expr.expression()->annotation().mustvalue = false;
        expr.expression()->accept(*this);

        if (member->is_generic()) checker_.error(expr.member()->range(), "You cannot use generic symbol in constant expression, idiot!", impl::const_expr_explanation);

        constval result;

        if (expr.expression()->annotation().istype) {
            if (auto builtin = types::builtin(expr.expression()->annotation().type->string())) {
                std::string item = member->identifier().lexeme().string();
                if (member->is_generic()) throw evaluator::error();

                switch (builtin->category()) {
                    case ast::type::category::integer_type:
                    {
                        auto inttype = std::static_pointer_cast<ast::integer_type>(builtin);
                        if (item == "BITS") {
                            result.type = types::usize();
                            result.u.precision(utils::precisions::bitsword);
                            result.u.value(inttype->bits());
                        }
                        else if (item == "MIN") {
                            result.type = builtin;
                            if (inttype->is_signed()) {
                                result.i.size(inttype->bits());
                                result.i.value(constval::integer::min(result.i.precision()));
                            }
                            else {
                                result.u.size(inttype->bits());
                                result.u.value(constval::uinteger::min(result.u.precision()));
                            }
                        }
                        else if (item == "MAX") {
                            result.type = builtin;
                            if (inttype->is_signed()) {
                                result.i.size(inttype->bits());
                                result.i.value(constval::integer::max(result.i.precision()));
                            }
                            else {
                                result.u.size(inttype->bits());
                                result.u.value(constval::uinteger::max(result.u.precision()));
                            }
                        }
                        else throw evaluator::error();
                        break;
                    }
                    case ast::type::category::rational_type:
                        if (item == "BITS") {
                            result.type = types::usize();
                            result.u.precision(utils::precisions::bitsword);
                            result.u.value(std::static_pointer_cast<ast::rational_type>(builtin)->bits());
                        }
                        else throw evaluator::error();
                        break;
                    case ast::type::category::float_type:
                    {
                        auto realtype = std::static_pointer_cast<ast::float_type>(builtin);
                        if (item == "BITS") {
                            result.type = types::usize();
                            result.u.precision(utils::precisions::bitsword);
                            result.u.value(realtype->bits());
                        }
                        else if (item == "MIN") {
                            result.type = builtin;
                            result.f.size(realtype->bits());
                            result.f.value(constval::real::min(result.f.precision()));
                        }
                        else if (item == "MAX") {
                            result.type = builtin;
                            result.f.size(realtype->bits());
                            result.f.value(constval::real::max(result.f.precision()));
                        }
                        else if (item == "INFINITY") {
                            result.type = builtin;
                            result.f.size(realtype->bits());
                            result.f.value(constval::real::inf(result.f.precision()));
                        }
                        else if (item == "NAN") {
                            result.type = builtin;
                            result.f.size(realtype->bits());
                            result.f.value(constval::real::nan(result.f.precision()));
                        }
                        else throw evaluator::error();
                        break;
                    }
                    case ast::type::category::complex_type:
                        if (item == "BITS") {
                            result.type = types::usize();
                            result.u.precision(utils::precisions::bitsword);
                            result.u.value(std::static_pointer_cast<ast::complex_type>(builtin)->bits());
                        }
                        else throw evaluator::error();
                        break;
                    default:
                        throw evaluator::error();
                }
            }
            else throw evaluator::error();
        }
        else {
            constval left = pop();

            std::string name = member->identifier().lexeme().string();
            switch (left.type->category()) {
                case ast::type::category::rational_type:
                {
                    auto type = std::dynamic_pointer_cast<ast::rational_type>(left.type);
                    result.type = types::sint(type->bits() / 2);
                    if (name == "numerator") {
                        result.i = left.r.numerator();
                    }
                    else if (name == "denominator") {
                        result.i = left.r.denominator();
                    }
                    else {
                        result.type = types::unknown();
                    }
                    break;
                }
                case ast::type::category::complex_type:
                {
                    auto type = std::dynamic_pointer_cast<ast::complex_type>(left.type);
                    result.type = types::floating(type->bits() / 2);
                    if (name == "real") {
                        result.f = left.c.real();
                    }
                    else if (name == "imaginary") {
                        result.f = left.c.imag();
                    }
                    else {
                        result.type = types::unknown();
                    }
                    break;
                }
                case ast::type::category::array_type:
                case ast::type::category::slice_type:
                case ast::type::category::tuple_type:
                    if (name == "size") {
                        result.type = types::usize();
                        result.u.precision(utils::precisions::bitsword);
                        result.u.value(left.seq.size());
                    }
                    else {
                        result.type = types::unknown();
                    }
                    break;
                case ast::type::category::string_type:
                case ast::type::category::chars_type:
                    if (name == "size") {
                        result.type = types::usize();
                        result.u.precision(utils::precisions::bitsword);
                        result.u.value(left.s.size());
                    }
                    else if (name == "length") {
                        result.type = types::usize();
                        result.u.precision(utils::precisions::bitsword);
                        result.u.value(utf8::span(left.s.data(), static_cast<int>(left.s.size())).length());
                    }
                    else {
                        result.type = types::unknown();
                    }
                    break;
                default:
                    result.type = types::unknown();
            }
        }

        push(result);
    }

    void evaluator::visit(const ast::array_index_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();

        expr.expression()->accept(*this);
        constval left = pop();
        expr.index()->accept(*this);
        constval index = pop();
        constval result;

        // slice extraction is a run-time operation even though it can be done at static time
        if (left.type->category() != ast::type::category::array_type || index.type->category() != ast::type::category::integer_type) {
            result.type = types::unknown();
        }
        else {
            if (index.u.value() >= left.seq.size()) {
                auto diag = diagnostic::builder()
                            .location(expr.range().begin())
                            .severity(diagnostic::severity::error)
                            .small(true)
                            .message(diagnostic::format("You trying to access element at index $ while array size is $, idiot!", index.u, left.seq.size()))
                            .highlight(expr.index()->range(), "out of range")
                            .highlight(expr.expression()->range(), diagnostic::highlighter::mode::light)
                            .build();
                
                checker_.publisher().publish(diag);
                throw evaluator::error();
            }

            result = left.seq.at(index.u.value());
        }

        push(result);
    }
    
    void evaluator::visit(const ast::tuple_index_expression& expr) 
    {
        if (expr.invalid() || !expr.index().valid) throw evaluator::error();

        expr.expression()->accept(*this);
        constval left = pop();
        constval index;
        index.type = types::usize();
        index.u.precision(utils::precisions::bitsword);
        index.u.value(impl::stoui(expr.index().lexeme().string()));
        constval result;

        // slice extraction is a run-time operation even though it can be done at static time
        if (left.type->category() != ast::type::category::tuple_type) {
            result.type = types::unknown();
        }
        else {
            if (index.u.value() >= left.seq.size()) {
                auto diag = diagnostic::builder()
                            .location(expr.range().begin())
                            .severity(diagnostic::severity::error)
                            .small(true)
                            .message(diagnostic::format("You trying to access element at index $ while tuple size is $, idiot!", index.u, left.seq.size()))
                            .highlight(expr.index().range(), "out of range")
                            .highlight(expr.expression()->range(), diagnostic::highlighter::mode::light)
                            .build();
                
                checker_.publisher().publish(diag);
                throw evaluator::error();throw evaluator::error();
            }

            result = left.seq.at(index.u.value());
        }

        push(result);
    }

    void evaluator::visit(const ast::unary_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();

        expr.expression()->accept(*this);
        constval right = pop();
        constval result;
        std::feclearexcept(FE_ALL_EXCEPT);

        switch (expr.unary_operator().kind()) {
            case token::kind::plus:
                if (right.type->category() == ast::type::category::integer_type ||
                    right.type->category() == ast::type::category::rational_type ||
                    right.type->category() == ast::type::category::float_type ||
                    right.type->category() == ast::type::category::complex_type) {
                    result = right;
                }
                else {
                    result.type = types::unknown();
                }
                break;
            case token::kind::minus:
                if (right.type->category() == ast::type::category::integer_type) {
                    // even if operand is unsigned, then result is signed
                    auto type = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                    result.type = types::sint(type->bits());
                    if (type->is_signed()) result.i = -right.i;
                    else result.i = -constval::integer(right.u);
                }
                else if (right.type->category() == ast::type::category::rational_type) {
                    result = right;
                    result.r.numerator() = -result.r.numerator();
                }
                else if (right.type->category() == ast::type::category::float_type) {
                    result = right;
                    result.f = -result.f;
                }
                else if (right.type->category() == ast::type::category::complex_type) {
                    result = right;
                    result.c = -result.c;
                }
                else {
                    result.type = types::unknown();
                }
                
                test_operation_error(expr, "negation", result);
                break;
            case token::kind::tilde:
                if (right.type->category() == ast::type::category::integer_type) {
                    auto type = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                    result.type = type;
                    if (type->is_signed()) result.i = ~right.i;
                    else result.u = ~right.u;
                }
                else {
                    result.type = types::unknown();
                }
                break;
            case token::kind::bang:
                if (right.type->category() == ast::type::category::bool_type) {
                    result.type = right.type;
                    result.b = !right.b;
                }
                else {
                    result.type = types::unknown();
                }
                break;
            case token::kind::plus_plus:
                checker_.error(expr.range(), "You cannot use operator `++` in constant expression, idiot!", impl::const_expr_explanation); 
                throw evaluator::error();
            case token::kind::minus_minus:
                checker_.error(expr.range(), "You cannot use operator `--` in constant expression, idiot!", impl::const_expr_explanation); 
                throw evaluator::error();
            case token::kind::amp:
                checker_.error(expr.range(), "You cannot use operator `&` in constant expression, idiot!", impl::const_expr_explanation); 
                throw evaluator::error();
            case token::kind::star:
                checker_.error(expr.range(), "You cannot use operator `*` in constant expression, idiot!", impl::const_expr_explanation); 
                throw evaluator::error();
            default:
                result.type = types::unknown();
        }

        push(result);
    }

    void evaluator::visit(const ast::implicit_conversion_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();

        expr.expression()->accept(*this);
        constval left = pop();
        constval result;

        if (left.type->category() == ast::type::category::integer_type) {
            auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
            if (expr.annotation().type->category() == ast::type::category::integer_type) {
                auto rtype = std::dynamic_pointer_cast<ast::integer_type>(expr.annotation().type);
                result.type = rtype;
                if (rtype->is_signed()) {
                    if (ltype->is_signed()) result.i = left.i;
                    else result.i = left.u;
                    result.i.size(rtype->bits());
                }
                else {
                    if (ltype->is_signed()) { result.u.precision(left.i.precision()); result.u.value(left.i.value()); }
                    else result.u = left.u;
                    result.u.size(rtype->bits());
                }
            }
            else if (expr.annotation().type->category() == ast::type::category::rational_type) {
                auto rtype = std::dynamic_pointer_cast<ast::rational_type>(expr.annotation().type);
                result.type = rtype;
                if (ltype->is_signed()) result.r = constval::rational(left.i);
                else result.r = constval::rational(left.u);
                result.r.size(rtype->bits());
            }
            else if (expr.annotation().type->category() == ast::type::category::float_type) {
                auto rtype = std::dynamic_pointer_cast<ast::float_type>(expr.annotation().type);
                result.type = rtype;
                if (ltype->is_signed()) result.f = constval::real(left.i);
                else result.f = constval::real(left.u);
                result.f.size(rtype->bits());
            }
            else if (expr.annotation().type->category() == ast::type::category::complex_type) {
                auto rtype = std::dynamic_pointer_cast<ast::complex_type>(expr.annotation().type);
                result.type = rtype;
                if (ltype->is_signed()) result.c = constval::complex(left.i);
                else result.c = constval::complex(left.u);
                result.c.size(rtype->bits());
            }
            else {
                result.type = types::unknown();
            }
        }
        else if (left.type->category() == ast::type::category::rational_type) {
            auto ltype = std::dynamic_pointer_cast<ast::rational_type>(left.type);
            if (expr.annotation().type->category() == ast::type::category::integer_type) {
                auto rtype = std::dynamic_pointer_cast<ast::integer_type>(expr.annotation().type);
                result.type = rtype;
                if (rtype->is_signed()) { result.i.size(rtype->bits()); result.i.value(static_cast<int64_t>(left.r.real().value())); }
                else { result.u.size(rtype->bits()); result.u.value(static_cast<uint64_t>(left.r.real().value())); }
            }
            else if (expr.annotation().type->category() == ast::type::category::rational_type) {
                auto rtype = std::dynamic_pointer_cast<ast::rational_type>(expr.annotation().type);
                result.type = rtype;
                result.r = left.r;
                result.r.size(rtype->bits());
            }
            else if (expr.annotation().type->category() == ast::type::category::float_type) {
                auto rtype = std::dynamic_pointer_cast<ast::float_type>(expr.annotation().type);
                result.type = rtype;
                result.f = left.r.real();
                result.f.size(rtype->bits());
            }
            else if (expr.annotation().type->category() == ast::type::category::complex_type) {
                auto rtype = std::dynamic_pointer_cast<ast::complex_type>(expr.annotation().type);
                result.type = rtype;
                result.c = constval::complex(left.r.real());
                result.c.size(rtype->bits());
            }
            else {
                result.type = types::unknown();
            }
        }
        else if (left.type->category() == ast::type::category::float_type) {
            auto ltype = std::dynamic_pointer_cast<ast::float_type>(left.type);
            if (expr.annotation().type->category() == ast::type::category::integer_type) {
                auto rtype = std::dynamic_pointer_cast<ast::integer_type>(expr.annotation().type);
                result.type = rtype;
                result.i.value(static_cast<int64_t>(left.f.value()));
                result.i.size(rtype->bits());
            }
            else if (expr.annotation().type->category() == ast::type::category::rational_type) {
                auto rtype = std::dynamic_pointer_cast<ast::rational_type>(expr.annotation().type);
                result.type = rtype;
                result.r = constval::rational(left.f);
                result.r.size(rtype->bits());
            }
            else if (expr.annotation().type->category() == ast::type::category::float_type) {
                auto rtype = std::dynamic_pointer_cast<ast::float_type>(expr.annotation().type);
                result.type = rtype;
                result.f = left.f;
                result.f.size(rtype->bits());
            }
            else if (expr.annotation().type->category() == ast::type::category::complex_type) {
                auto rtype = std::dynamic_pointer_cast<ast::complex_type>(expr.annotation().type);
                result.type = rtype;
                result.c = constval::complex(left.f);
                result.c.size(rtype->bits());
            }
            else {
                result.type = types::unknown();
            }
        }
        else if (left.type->category() == ast::type::category::complex_type) {
            auto ltype = std::dynamic_pointer_cast<ast::complex_type>(left.type);
            if (expr.annotation().type->category() == ast::type::category::integer_type) {
                auto rtype = std::dynamic_pointer_cast<ast::integer_type>(expr.annotation().type);
                result.type = rtype;
                
                if (rtype->is_signed()) {
                    result.i.value(static_cast<int64_t>(left.c.real().value()));
                    result.i.size(rtype->bits());
                }
                else {
                    result.u.value(static_cast<uint64_t>(left.c.real().value()));
                    result.u.size(rtype->bits());
                }
            }
            else if (expr.annotation().type->category() == ast::type::category::rational_type) {
                auto rtype = std::dynamic_pointer_cast<ast::rational_type>(expr.annotation().type);
                result.type = rtype;
                constval::integer numerator(rtype->bits() / 2);
                numerator.value(static_cast<int64_t>(left.c.real().value()));
                result.r = constval::rational(numerator);
                result.r.size(rtype->bits());
            }
            else if (expr.annotation().type->category() == ast::type::category::float_type) {
                auto rtype = std::dynamic_pointer_cast<ast::float_type>(expr.annotation().type);
                result.type = rtype;
                result.f = left.c.real();
                result.f.size(rtype->bits());
            }
            else if (expr.annotation().type->category() == ast::type::category::complex_type) {
                auto rtype = std::dynamic_pointer_cast<ast::complex_type>(expr.annotation().type);
                result.type = rtype;
                result.c = left.c;
                result.c.size(rtype->bits());
            }
            else {
                result.type = types::unknown();
            }
        }
        else if (left.type->category() == ast::type::category::char_type) {
            if (expr.annotation().type->category() == ast::type::category::integer_type) {
                auto rtype = std::dynamic_pointer_cast<ast::integer_type>(expr.annotation().type);
                result.type = rtype;
                
                if (rtype->is_signed()) {
                    result.i.value(left.ch);
                    result.i.size(rtype->bits());
                }
                else {
                    result.u.value(left.ch);
                    result.u.size(rtype->bits());
                }
            }
            else {
                result.type = types::unknown();
            }
        }
        else if (left.type->category() == ast::type::category::chars_type) {
            if (expr.annotation().type->category() == ast::type::category::chars_type) {
                result = left;
            }
            else if (expr.annotation().type->category() == ast::type::category::string_type) {
                result.type = expr.annotation().type;
                result.s = left.s;
            }
            else {
                result.type = types::unknown();
            }
        }
        else if (left.type->category() == ast::type::category::string_type) {
            if (expr.annotation().type->category() == ast::type::category::chars_type) {
                result.type = expr.annotation().type;
                result.s = left.s;
            }
            else if (expr.annotation().type->category() == ast::type::category::string_type) {
                result = left;
            }
            else {
                result.type = types::unknown();
            }
        }
        else {
            result.type = types::unknown();
        }
        
        push(result);
    }

    void evaluator::visit(const ast::binary_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();

        expr.left()->accept(*this);
        constval left = pop();
        expr.right()->accept(*this);
        constval right = pop();
        constval result;
        std::feclearexcept(FE_ALL_EXCEPT);

        switch (expr.binary_operator().kind()) {
            case token::kind::plus:
                if (left.type->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        if (ltype->is_signed() || rtype->is_signed()) {
                            result.type = types::sint(bits);
                            if (ltype->is_signed()) {
                                if (rtype->is_signed()) result.i = left.i + right.i;
                                else result.i = left.i + right.u;
                            }
                            else {
                                if (rtype->is_signed()) result.i = constval::integer(left.u) + right.i;
                            }
                        }
                        else {
                            result.type = types::uint(bits);
                            result.u = left.u + right.u;
                        }
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = types::rational(std::max(2 * ltype->bits(), rtype->bits()));
                        if (ltype->is_signed()) result.r = constval::rational(left.i) + right.r;
                        else result.r = constval::rational(left.u) + right.r;
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        result.type = types::floating(bits);
                        if (ltype->is_signed()) result.f = constval::real(left.i) + right.f;
                        else result.f = constval::real(left.u) + right.f;
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(2 * ltype->bits(), rtype->bits()));
                        if (ltype->is_signed()) result.c = constval::complex(left.i) + right.c;
                        else result.c = constval::complex(left.u) + right.c;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::rational_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::rational_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        result.type = types::rational(std::max(ltype->bits(), 2 * rtype->bits()));
                        result.r = left.r + constval::rational(rtype->is_signed() ? right.i : right.u);
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = types::rational(std::max(ltype->bits(), rtype->bits()));
                        result.r = left.r + right.r;
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits() / 2, rtype->bits()));
                        result.f = left.r.real() + right.f;
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        result.c = constval::complex(left.r.real()) + right.c;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::float_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::float_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        result.type = types::floating(bits);
                        if (rtype->is_signed()) result.f = left.f + constval::real(right.i);
                        else result.f = left.f + constval::real(right.u);
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits(), rtype->bits() / 2));
                        result.f = left.f + right.r.real();
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        result.f = left.f + right.f;
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits() * 2, rtype->bits()));
                        result.c = constval::complex(left.f) + right.c;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::complex_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::complex_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        result.c = left.c + constval::complex(rtype->is_signed() ? right.i : right.u);
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        result.c = left.c + constval::complex(right.r.real());
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        result.c = left.c + constval::complex(right.f);
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        result.c = left.c + right.c;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::char_type) {
                    if (right.type->category() == ast::type::category::chars_type) {
                        result.type = types::string();
                        byte units[5] = { 0 };
                        units[utf8::encode(left.ch, units)] = 0;
                        result.s = std::string(reinterpret_cast<char*>(units)) + right.s;
                    }
                    else if (right.type->category() == ast::type::category::string_type) {
                        result.type = types::string();
                        byte units[5] = { 0 };
                        units[utf8::encode(left.ch, units)] = 0;
                        result.s = std::string(reinterpret_cast<char*>(units)) + right.s;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::chars_type) {
                    if (right.type->category() == ast::type::category::char_type) {
                        result.type = types::string();
                        result.s = left.s;
                        byte units[5] = { 0 };
                        units[utf8::encode(right.ch, units)] = 0;
                        result.s.append(reinterpret_cast<char*>(units));
                    }
                    else if (right.type->category() == ast::type::category::chars_type) {
                        result.type = types::string();
                        result.s = left.s + right.s;
                    }
                    else if (right.type->category() == ast::type::category::string_type) {
                        result.type = types::string();
                        result.s = left.s + right.s;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::string_type) {
                    if (right.type->category() == ast::type::category::char_type) {
                        result.type = types::string();
                        result.s = left.s;
                        byte units[5] = { 0 };
                        units[utf8::encode(right.ch, units)] = 0;
                        result.s.append(reinterpret_cast<char*>(units));
                    }
                    else if (right.type->category() == ast::type::category::chars_type || right.type->category() == ast::type::category::string_type) {
                        result.type = types::string();
                        result.s = left.s + right.s;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else {
                    result.type = types::unknown();
                }

                test_operation_error(expr, "addition", result);
                break;
            case token::kind::minus:
                if (left.type->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        result.type = types::sint(bits);
                        if (ltype->is_signed()) {
                            if (rtype->is_signed()) result.i = left.i - right.i;
                            else result.i = left.i - constval::integer(right.u);
                        }
                        else {
                            if (rtype->is_signed()) result.i = constval::integer(left.u) - right.i;
                            else result.i = constval::integer(left.u) - constval::integer(right.u);
                        }
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = types::rational(std::max(2 * ltype->bits(), rtype->bits()));
                        if (ltype->is_signed()) result.r = constval::rational(left.i) - right.r;
                        else result.r = constval::rational(left.u) - right.r;
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        result.type = types::floating(bits);
                        if (ltype->is_signed()) result.f = constval::real(left.i) - right.f;
                        else result.f = constval::real(left.u) - right.f;
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(2 * ltype->bits(), rtype->bits()));
                        if (ltype->is_signed()) result.c = constval::complex(left.i) - right.c;
                        else result.c = constval::complex(left.u) - right.c;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::rational_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::rational_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        result.type = types::rational(std::max(ltype->bits(), 2 * rtype->bits()));
                        result.r = left.r - constval::rational(rtype->is_signed() ? right.i : right.u);
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = types::rational(std::max(ltype->bits(), rtype->bits()));
                        result.r = left.r - right.r;
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits() / 2, rtype->bits()));
                        result.f = left.r.real() - right.f;
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        result.c = constval::complex(left.r.real()) - right.c;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::float_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::float_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        result.type = types::floating(bits);
                        if (rtype->is_signed()) result.f = left.f - constval::real(right.i);
                        else result.f = left.f - constval::real(right.u);
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits(), rtype->bits() / 2));
                        result.f = left.f - right.r.real();
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        result.f = left.f - right.f;
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits() * 2, rtype->bits()));
                        result.c = constval::complex(left.f) - right.c;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::complex_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::complex_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        result.c = left.c - constval::complex(rtype->is_signed() ? right.i : right.u);
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        result.c = left.c - constval::complex(right.r.real());
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        result.c = left.c - constval::complex(right.f);
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        result.c = left.c - right.c;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else {
                    result.type = types::unknown();
                }

                test_operation_error(expr, "subtracion", result);
                break;
            case token::kind::star:
                if (left.type->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        if (ltype->is_signed() || rtype->is_signed()) {
                            result.type = types::sint(bits);
                            if (ltype->is_signed()) {
                                if (rtype->is_signed()) result.i = left.i * right.i;
                                else result.i = left.i * right.u;
                            }
                            else {
                                if (rtype->is_signed()) result.i = constval::integer(left.u) * right.i;
                            }
                        }
                        else {
                            result.type = types::uint(bits);
                            result.u = left.u * right.u;
                        }
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = types::rational(std::max(2 * ltype->bits(), rtype->bits()));
                        if (ltype->is_signed()) result.r = constval::rational(left.i) * right.r;
                        else result.r = constval::rational(left.u) * right.r;
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        result.type = types::floating(bits);
                        if (ltype->is_signed()) result.f = constval::real(left.i) + right.f;
                        else result.f = constval::real(left.u) * right.f;
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(2 * ltype->bits(), rtype->bits()));
                        if (ltype->is_signed()) result.c = constval::complex(left.i) * right.c;
                        else result.c = constval::complex(left.u) * right.c;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::rational_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::rational_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        result.type = types::rational(std::max(ltype->bits(), 2 * rtype->bits()));
                        result.r = left.r * constval::rational(rtype->is_signed() ? right.i : right.u);
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = types::rational(std::max(ltype->bits(), rtype->bits()));
                        result.r = left.r * right.r;
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits() / 2, rtype->bits()));
                        result.f = left.r.real() * right.f;
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        result.c = constval::complex(left.r.real()) * right.c;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::float_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::float_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        result.type = types::floating(bits);
                        if (rtype->is_signed()) result.f = left.f * constval::real(right.i);
                        else result.f = left.f * constval::real(right.u);
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits(), rtype->bits() / 2));
                        result.f = left.f * right.r.real();
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        result.f = left.f * right.f;
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits() * 2, rtype->bits()));
                        result.c = constval::complex(left.f) * right.c;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::complex_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::complex_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        result.c = left.c * constval::complex(rtype->is_signed() ? right.i : right.u);
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        result.c = left.c * constval::complex(right.r.real());
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        result.c = left.c * constval::complex(right.f);
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), rtype->bits()));
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else {
                    result.type = types::unknown();
                }

                test_operation_error(expr, "multiplication", result);
                break;
            case token::kind::slash:
                if (left.type->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        
                        if (right.u.value() == 0 || right.i.value() == 0) {
                            checker_.error(expr, "You are dividing by zero here! Didn't they tell you that's f*cking illegal?", "", "zero divisor");
                            throw evaluator::error();
                        }

                        result.type = types::rational(2 * std::max(ltype->bits(), rtype->bits()));

                        if (ltype->is_signed()) {
                            if (rtype->is_signed()) result.r = constval::rational(left.i, right.i);
                            else result.r = constval::rational(left.i, right.u);
                        }
                        else {
                            if (rtype->is_signed()) result.r = constval::rational(left.u, right.i);
                            else result.r = constval::rational(left.u, right.u);
                        }
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        if (right.r.numerator().value() == 0) {
                            checker_.error(expr, "You are dividing by zero here! Didn't they tell you that's f*cking illegal?", "", "zero divisor");
                            throw evaluator::error();
                        }
                        
                        result.type = types::rational(std::max(2 * ltype->bits(), rtype->bits()));
                        if (ltype->is_signed()) result.r = constval::rational(left.i) / right.r;
                        else result.r = constval::rational(left.u) / right.r;
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        if (ltype->is_signed()) result.f = constval::real(left.i) / right.f;
                        result.f = constval::real(left.u) / right.f;
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(2 * ltype->bits(), rtype->bits()));
                        if (ltype->is_signed()) result.c = constval::complex(left.i) / right.c;
                        else result.c = constval::complex(left.u) / right.c;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::rational_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::rational_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        
                        if ((rtype->is_signed() ? right.i : right.u).value() == 0) {
                            checker_.error(expr, "You are dividing by zero here! Didn't they tell you that's f*cking illegal?", "", "zero divisor");
                            throw evaluator::error();
                        }
                        
                        result.type = types::rational(std::max(ltype->bits(), 2 * rtype->bits()));
                        result.r = left.r / constval::rational(rtype->is_signed() ? right.i : right.u);
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        
                        if (right.r.numerator().value() == 0) {
                            checker_.error(expr, "You are dividing by zero here! Didn't they tell you that's f*cking illegal?", "", "zero divisor");
                            throw evaluator::error();
                        }
                        
                        result.type = types::rational(std::max(ltype->bits(), rtype->bits()));
                        result.r = left.r / right.r;
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits() / 2, rtype->bits()));
                        result.f = left.r.real() / right.f;
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        result.c = constval::complex(left.r.real()) / right.c;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::float_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::float_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        
                        if ((rtype->is_signed() ? right.i : right.u).value() == 0) {
                            checker_.error(expr, "You are dividing by zero here! Didn't they tell you that's f*cking illegal?", "", "zero divisor");
                            throw evaluator::error();
                        }
                        
                        result.type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        result.f = left.f / (rtype->is_signed() ? right.i : right.u);
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        
                        if (right.r.numerator().value() == 0) {
                            checker_.error(expr, "You are dividing by zero here! Didn't they tell you that's f*cking illegal?", "", "zero divisor");
                            throw evaluator::error();
                        }
                        
                        result.type = types::floating(std::max(ltype->bits(), rtype->bits() / 2));
                        result.f = left.f * right.r.real();
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        result.f = left.f / right.f;
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits() * 2, rtype->bits()));
                        result.c = constval::complex(left.f) / right.c;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::complex_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::complex_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        
                        if ((rtype->is_signed() ? right.i : right.u).value() == 0) {
                            checker_.error(expr, "You are dividing by zero here! Didn't they tell you that's f*cking illegal?", "", "zero divisor");
                            throw evaluator::error();
                        }
                        
                        result.type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        result.c = left.c / constval::complex(rtype->is_signed() ? right.i : right.u);
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        
                        if (right.r.numerator().value() == 0) {
                            checker_.error(expr, "You are dividing by zero here! Didn't they tell you that's f*cking illegal?", "", "zero divisor");
                            throw evaluator::error();
                        }
                        
                        result.type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        result.c = left.c * constval::complex(right.r.real());
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        result.c = left.c / constval::complex(right.f);
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        result.c = left.c / right.c;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else {
                    result.type = types::unknown();
                }

                test_operation_error(expr, "division", result);
                break;
            case token::kind::percent:
                if (left.type->category() == ast::type::category::integer_type && right.type->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                    auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                    
                    if (right.i.value() == 0 || right.u.value() == 0) {
                        checker_.error(expr, "You are dividing by zero here! Didn't they tell you that's f*cking illegal?", "", "zero divisor");
                        throw evaluator::error();
                    }

                    if (ltype->is_signed() || rtype->is_signed()) result.type = types::sint(std::max(ltype->bits(), rtype->bits()));
                    else result.type = types::uint(std::max(ltype->bits(), rtype->bits()));

                    if (ltype->is_signed()) {
                        if (rtype->is_signed()) result.i = left.i % right.i;
                        else result.i = left.i % right.u;
                    }
                    else {
                        if (rtype->is_signed()) result.i = constval::integer(left.u) % right.i;
                        else result.u = left.u % right.u;
                    }
                }
                else {
                    result.type = types::unknown();
                }

                test_operation_error(expr, "modulus", result);
                break;
            case token::kind::star_star:
                if (left.type->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        if (rtype->is_signed()) {
                            if (ltype->is_signed()) result.f = powf(left.i, right.i);
                            else result.f = powf(left.u, right.i);
                        }
                        else {
                            if (ltype->is_signed()) result.f = powf(left.i, right.u);
                            else result.f = powf(left.u, right.u);
                        }
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits(), rtype->bits() / 2));
                        if (ltype->is_signed()) result.f = powf(left.i, right.r.real());
                        else result.f = powf(left.u, right.r.real());
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        if (ltype->is_signed()) result.f = powf(left.i, right.f);
                        else result.f = powf(left.u, right.f);
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(2 * ltype->bits(), rtype->bits()));
                        if (ltype->is_signed()) result.c = powc(constval::complex(left.i), right.c);
                        else result.c = powc(constval::complex(left.u), right.c);
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::rational_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::rational_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits() / 2, rtype->bits()));
                        if (rtype->is_signed()) result.f = powf(left.r.real(), right.i);
                        else result.f = powf(left.r.real(), right.u);
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits() / 2, rtype->bits() / 2));
                        result.f = powf(left.r.real(), right.r.real());
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits() / 2, rtype->bits()));
                        result.f = powf(left.r.real(), right.f);
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        result.c = powc(constval::complex(left.r.real()), right.c);
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::float_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::float_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        if (rtype->is_signed()) result.f = powf(left.f, right.i);
                        else result.f = powf(left.f, right.u);
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits(), rtype->bits() / 2));
                        result.f = powf(left.f, right.r.real());
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        result.f = powf(left.f, right.f);
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits() * 2, rtype->bits()));
                        result.c = powc(constval::complex(left.f), right.c);
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::complex_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::complex_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        if (rtype->is_signed()) result.c = powc(left.c, constval::complex(right.i));
                        else result.c = powc(left.c, constval::complex(right.u));
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        result.c = powc(left.c, constval::complex(right.r.real()));
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        result.c = powc(left.c, constval::complex(right.f));
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        result.c = powc(left.c, right.c);
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else {
                    result.type = types::unknown();
                }

                test_operation_error(expr, "power", result);
                break;
            case token::kind::less_less:
                if (left.type->category() == ast::type::category::integer_type && right.type->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                    auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type); 
                    result.type = ltype;
                    if (ltype->is_signed()) {
                        if (rtype->is_signed()) result.i = left.i << right.i;
                        else result.i = left.i << constval::integer(right.u);
                    }
                    else {
                        if (rtype->is_signed()) {
                            constval::uinteger temp(right.i.precision());
                            temp.value(right.i.value());
                            result.u = left.u << temp;
                        }
                        else result.u = left.u << right.u;
                    }
                }
                else {
                    result.type = types::unknown();
                }

                test_operation_error(expr, "shift", result);
                break;
            case token::kind::greater_greater:
                if (left.type->category() == ast::type::category::integer_type && right.type->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                    auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                    result.type = ltype;
                    if (ltype->is_signed()) {
                        if (rtype->is_signed()) result.i = left.i >> right.i;
                        else result.i = left.i >> constval::integer(right.u);
                    }
                    else {
                        if (rtype->is_signed()) {
                            constval::uinteger temp(right.i.precision());
                            temp.value(right.i.value());
                            result.u = left.u >> temp;
                        }
                        else result.u = left.u >> right.u;
                    }
                }
                else {
                    result.type = types::unknown();
                }

                test_operation_error(expr, "shift", result);
                break;
            case token::kind::as_kw:
                if (left.type->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        result.type = rtype;
                        if (rtype->is_signed()) {
                            if (ltype->is_signed()) result.i = left.i;
                            else result.i = left.u;
                            result.i.size(rtype->bits());
                        }
                        else {
                            if (ltype->is_signed()) { result.u.precision(left.i.precision()); result.u.value(left.i.value()); }
                            else result.u = left.u;
                            result.u.size(rtype->bits());
                        }
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = rtype;
                        if (ltype->is_signed()) result.r = constval::rational(left.i);
                        else result.r = constval::rational(left.u);
                        result.r.size(rtype->bits());
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = rtype;
                        if (ltype->is_signed()) result.f = constval::real(left.i);
                        else result.f = constval::real(left.u);
                        result.f.size(rtype->bits());
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = rtype;
                        if (ltype->is_signed()) result.c = constval::complex(left.i);
                        else result.c = constval::complex(left.u);
                        result.c.size(rtype->bits());
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::rational_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::rational_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        result.type = rtype;
                        if (rtype->is_signed()) { result.i.size(rtype->bits()); result.i.value(static_cast<int64_t>(left.r.real().value())); }
                        else { result.u.size(rtype->bits()); result.u.value(static_cast<uint64_t>(left.r.real().value())); }
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = rtype;
                        result.r = left.r;
                        result.r.size(rtype->bits());
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = rtype;
                        result.f = left.r.real();
                        result.f.size(rtype->bits());
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = rtype;
                        result.c = constval::complex(left.r.real());
                        result.c.size(rtype->bits());
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::float_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::float_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        result.type = rtype;
                        result.i.value(static_cast<int64_t>(left.f.value()));
                        result.i.size(rtype->bits());
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = rtype;
                        result.r = constval::rational(left.f);
                        result.r.size(rtype->bits());
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = rtype;
                        result.f = left.f;
                        result.f.size(rtype->bits());
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = rtype;
                        result.c = constval::complex(left.f);
                        result.c.size(rtype->bits());
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::complex_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::complex_type>(left.type);
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        result.type = rtype;
                        
                        if (rtype->is_signed()) {
                            result.i.value(static_cast<int64_t>(left.c.real().value()));
                            result.i.size(rtype->bits());
                        }
                        else {
                            result.u.value(static_cast<uint64_t>(left.c.real().value()));
                            result.u.size(rtype->bits());
                        }
                    }
                    else if (right.type->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                        result.type = rtype;
                        constval::integer numerator(rtype->bits() / 2);
                        numerator.value(static_cast<int64_t>(left.c.real().value()));
                        result.r = constval::rational(numerator);
                        result.r.size(rtype->bits());
                    }
                    else if (right.type->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                        result.type = rtype;
                        result.f = left.c.real();
                        result.f.size(rtype->bits());
                    }
                    else if (right.type->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                        result.type = rtype;
                        result.c = left.c;
                        result.c.size(rtype->bits());
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::char_type) {
                    if (right.type->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                        result.type = rtype;
                        
                        if (rtype->is_signed()) {
                            result.i.value(left.ch);
                            result.i.size(rtype->bits());
                        }
                        else {
                            result.u.value(left.ch);
                            result.u.size(rtype->bits());
                        }
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::chars_type) {
                    if (right.type->category() == ast::type::category::chars_type) {
                        result = left;
                    }
                    else if (right.type->category() == ast::type::category::string_type) {
                        result.type = right.type;
                        result.s = left.s;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else if (left.type->category() == ast::type::category::string_type) {
                    if (right.type->category() == ast::type::category::chars_type) {
                        result.type = right.type;
                        result.s = left.s;
                    }
                    else if (right.type->category() == ast::type::category::string_type) {
                        result = left;
                    }
                    else {
                        result.type = types::unknown();
                    }
                }
                else {
                    result.type = types::unknown();
                }
                
                test_operation_error(expr, "conversion", result);
                break;
            case token::kind::amp:
                if (left.type->category() == ast::type::category::integer_type && right.type->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                    auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                    if (ltype->is_signed() || rtype->is_signed()) {
                        result.type = types::sint(std::max(ltype->bits(), rtype->bits()));
                        if (ltype->is_signed()) {
                            if (rtype->is_signed()) result.i = left.i & right.i;
                            else result.i = left.i & right.u;
                        }
                        else {
                            if (rtype->is_signed()) result.i = constval::integer(left.u) & right.i;
                        }
                    }
                    else {
                        result.type = types::uint(std::max(ltype->bits(), rtype->bits()));
                        result.u = left.u & right.u;
                    }
                }
                else {
                    result.type = types::unknown();
                }
                break;
            case token::kind::line:
                if (left.type->category() == ast::type::category::integer_type && right.type->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                    auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                    if (ltype->is_signed() || rtype->is_signed()) {
                        result.type = types::sint(std::max(ltype->bits(), rtype->bits()));
                        if (ltype->is_signed()) {
                            if (rtype->is_signed()) result.i = left.i | right.i;
                            else result.i = left.i | right.u;
                        }
                        else {
                            if (rtype->is_signed()) result.i = constval::integer(left.u) | right.i;
                        }
                    }
                    else {
                        result.type = types::uint(std::max(ltype->bits(), rtype->bits()));
                        result.u = left.u | right.u;
                    }
                }
                else {
                    result.type = types::unknown();
                }
                break;
            case token::kind::caret:
                if (left.type->category() == ast::type::category::integer_type && right.type->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                    auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                    if (ltype->is_signed() || rtype->is_signed()) {
                        result.type = types::sint(std::max(ltype->bits(), rtype->bits()));
                        if (ltype->is_signed()) {
                            if (rtype->is_signed()) result.i = left.i ^ right.i;
                            else result.i = left.i ^ right.u;
                        }
                        else {
                            if (rtype->is_signed()) result.i = constval::integer(left.u) ^ right.i;
                        }
                    }
                    else {
                        result.type = types::uint(std::max(ltype->bits(), rtype->bits()));
                        result.u = left.u ^ right.u;
                    }
                }
                else {
                    result.type = types::unknown();
                }
                break;
            case token::kind::amp_amp:
                if (left.type->category() == ast::type::category::bool_type && right.type->category() == ast::type::category::bool_type) {
                    result.type = types::boolean();
                    result.b = left.b && right.b;
                }
                else {
                    result.type = types::unknown();
                }
                break;
            case token::kind::line_line:
                if (left.type->category() == ast::type::category::bool_type && right.type->category() == ast::type::category::bool_type) {
                    result.type = types::boolean();
                    result.b = left.b || right.b;
                }
                else {
                    result.type = types::unknown();
                }
                break;
            case token::kind::equal_equal:
                if ((left.type->category() == ast::type::category::chars_type && right.type->category() == ast::type::category::string_type) ||
                    (left.type->category() == ast::type::category::string_type && right.type->category() == ast::type::category::chars_type) ||
                    left.type->category() == right.type->category()) {
                    switch (left.type->category()) {
                        case ast::type::category::integer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                            if ((ltype->is_signed() && !rtype->is_signed()) || (!ltype->is_signed() && rtype->is_signed())) {
                                result.type = types::unknown();
                            }
                            else if (ltype->is_signed()) {
                                result.type = types::boolean();
                                result.b = (left.i == right.i);
                            }
                            else {
                                result.type = types::boolean();
                                result.b = (left.u == right.u);
                            }
                            break;
                        }
                        case ast::type::category::rational_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::rational_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                            result.type = types::boolean();
                            result.b = (left.r == right.r);
                            break;
                        }
                        case ast::type::category::float_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::float_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                            result.type = types::boolean();
                            result.b = (left.f == right.f);
                            break;
                        }
                        case ast::type::category::complex_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::complex_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                            result.type = types::boolean();
                            result.b = (left.c == right.c);
                            break;
                        }
                        case ast::type::category::chars_type:
                        case ast::type::category::string_type:
                        {
                            result.type = types::boolean();
                            result.b = (left.s == right.s);
                            break;
                        }
                        case ast::type::category::char_type:
                        {
                            result.type = types::boolean();
                            result.b = (left.ch == right.ch);
                            break;
                        }
                        case ast::type::category::bool_type:
                        {
                            result.type = types::boolean();
                            result.b = (left.b == right.b);
                            break;
                        }
                        case ast::type::category::array_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::array_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::array_type>(right.type);
                            if (ltype->size() != rtype->size()) {
                                result.type = types::unknown();
                            }
                            else {
                                result.type = types::boolean();
                                result.b = true;
                                for (size_t i = 0; i < ltype->size() && result.b; ++i) {
                                    if (!(left.seq.at(i) == right.seq.at(i))) result.b = false;
                                }
                            }
                            break;
                        }
                        case ast::type::category::tuple_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::tuple_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::tuple_type>(right.type);
                            if (ltype->length() != rtype->length()) {
                                result.type = types::unknown();
                            }
                            else {
                                result.type = types::boolean();
                                result.b = true;
                                for (size_t i = 0; i < ltype->length() && result.b; ++i) {
                                    if (!(left.seq.at(i) == right.seq.at(i))) result.b = false;
                                }
                            }
                            break;
                        }
                        default:
                            result.type = types::unknown();
                    }
                }
                else {
                    result.type = types::unknown();
                }
                break;
            case token::kind::bang_equal:
                if ((left.type->category() == ast::type::category::chars_type && right.type->category() == ast::type::category::string_type) ||
                    (left.type->category() == ast::type::category::string_type && right.type->category() == ast::type::category::chars_type) ||
                    left.type->category() == right.type->category()) {
                    switch (left.type->category()) {
                        case ast::type::category::integer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                            if ((ltype->is_signed() && !rtype->is_signed()) || (!ltype->is_signed() && rtype->is_signed())) {
                                result.type = types::unknown();
                            }
                            else if (ltype->is_signed()) {
                                result.type = types::boolean();
                                result.b = (left.i != right.i);
                            }
                            else {
                                result.type = types::boolean();
                                result.b = (left.u != right.u);
                            }
                            break;
                        }
                        case ast::type::category::rational_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::rational_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                            result.type = types::boolean();
                            result.b = (left.r != right.r);
                            break;
                        }
                        case ast::type::category::float_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::float_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                            result.type = types::boolean();
                            result.b = (left.f != right.f);
                            break;
                        }
                        case ast::type::category::complex_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::complex_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::complex_type>(right.type);
                            result.type = types::boolean();
                            result.b = (left.c != right.c);
                            break;
                        }
                        case ast::type::category::chars_type:
                        case ast::type::category::string_type:
                        {
                            result.type = types::boolean();
                            result.b = (left.s != right.s);
                            break;
                        }
                        case ast::type::category::char_type:
                        {
                            result.type = types::boolean();
                            result.b = (left.ch != right.ch);
                            break;
                        }
                        case ast::type::category::bool_type:
                        {
                            result.type = types::boolean();
                            result.b = (left.b != right.b);
                            break;
                        }
                        case ast::type::category::array_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::array_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::array_type>(right.type);
                            if (ltype->size() != rtype->size()) {
                                result.type = types::unknown();
                            }
                            else {
                                result.type = types::boolean();
                                result.b = true;
                                size_t i = 0;
                                for (; i < ltype->size(); ++i) {
                                    if (!(left.seq.at(i) == right.seq.at(i))) break;
                                }
                                if (i == ltype->size()) result.b = false;
                            }
                            break;
                        }
                        case ast::type::category::tuple_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::tuple_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::tuple_type>(right.type);
                            if (ltype->length() != rtype->length()) {
                                result.type = types::unknown();
                            }
                            else {
                                result.type = types::boolean();
                                result.b = true;
                                size_t i = 0;
                                for (; i < ltype->length(); ++i) {
                                    if (!(left.seq.at(i) == right.seq.at(i))) break;
                                }
                                if (i == ltype->length()) result.b = false;
                            }
                            break;
                        }
                        default:
                            result.type = types::unknown();
                    }
                }
                else {
                    result.type = types::unknown();
                }
                break;
            case token::kind::less:
                if ((left.type->category() == ast::type::category::chars_type && right.type->category() == ast::type::category::string_type) ||
                    (left.type->category() == ast::type::category::string_type && right.type->category() == ast::type::category::chars_type) ||
                    left.type->category() == right.type->category()) {
                    switch (left.type->category()) {
                        case ast::type::category::integer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                            if ((ltype->is_signed() && !rtype->is_signed()) || (!ltype->is_signed() && rtype->is_signed())) {
                                result.type = types::unknown();
                            }
                            else if (ltype->is_signed()) {
                                result.type = types::boolean();
                                result.b = (left.i < right.i);
                            }
                            else {
                                result.type = types::boolean();
                                result.b = (left.u < right.u);
                            }
                            break;
                        }
                        case ast::type::category::rational_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::rational_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                            result.type = types::boolean();
                            result.b = (left.r < right.r);
                            break;
                        }
                        case ast::type::category::float_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::float_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                            result.type = types::boolean();
                            result.b = (left.f < right.f);
                            break;
                        }
                        case ast::type::category::chars_type:
                        case ast::type::category::string_type:
                        {
                            result.type = types::boolean();
                            result.b = (left.s < right.s);
                            break;
                        }
                        case ast::type::category::char_type:
                        {
                            result.type = types::boolean();
                            result.b = (left.ch < right.ch);
                            break;
                        }
                        default:
                            result.type = types::unknown();
                    }
                }
                else {
                    result.type = types::unknown();
                }
                break;
            case token::kind::less_equal:
                if ((left.type->category() == ast::type::category::chars_type && right.type->category() == ast::type::category::string_type) ||
                    (left.type->category() == ast::type::category::string_type && right.type->category() == ast::type::category::chars_type) ||
                    left.type->category() == right.type->category()) {
                    switch (left.type->category()) {
                        case ast::type::category::integer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                            if ((ltype->is_signed() && !rtype->is_signed()) || (!ltype->is_signed() && rtype->is_signed())) {
                                result.type = types::unknown();
                            }
                            else if (ltype->is_signed()) {
                                result.type = types::boolean();
                                result.b = (left.i <= right.i);
                            }
                            else {
                                result.type = types::boolean();
                                result.b = (left.u <= right.u);
                            }
                            break;
                        }
                        case ast::type::category::rational_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::rational_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                            result.type = types::boolean();
                            result.b = (left.r <= right.r);
                            break;
                        }
                        case ast::type::category::float_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::float_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                            result.type = types::boolean();
                            result.b = (left.f <= right.f);
                            break;
                        }
                        case ast::type::category::chars_type:
                        case ast::type::category::string_type:
                        {
                            result.type = types::boolean();
                            result.b = (left.s <= right.s);
                            break;
                        }
                        case ast::type::category::char_type:
                        {
                            result.type = types::boolean();
                            result.b = (left.ch <= right.ch);
                            break;
                        }
                        default:
                            result.type = types::unknown();
                    }
                }
                else {
                    result.type = types::unknown();
                }
                break;
            case token::kind::greater:
                if ((left.type->category() == ast::type::category::chars_type && right.type->category() == ast::type::category::string_type) ||
                    (left.type->category() == ast::type::category::string_type && right.type->category() == ast::type::category::chars_type) ||
                    left.type->category() == right.type->category()) {
                    switch (left.type->category()) {
                        case ast::type::category::integer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                            if ((ltype->is_signed() && !rtype->is_signed()) || (!ltype->is_signed() && rtype->is_signed())) {
                                result.type = types::unknown();
                            }
                            else if (ltype->is_signed()) {
                                result.type = types::boolean();
                                result.b = (left.i > right.i);
                            }
                            else {
                                result.type = types::boolean();
                                result.b = (left.u > right.u);
                            }
                            break;
                        }
                        case ast::type::category::rational_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::rational_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                            result.type = types::boolean();
                            result.b = (left.r > right.r);
                            break;
                        }
                        case ast::type::category::float_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::float_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                            result.type = types::boolean();
                            result.b = (left.f > right.f);
                            break;
                        }
                        case ast::type::category::chars_type:
                        case ast::type::category::string_type:
                        {
                            result.type = types::boolean();
                            result.b = (left.s > right.s);
                            break;
                        }
                        case ast::type::category::char_type:
                        {
                            result.type = types::boolean();
                            result.b = (left.ch > right.ch);
                            break;
                        }
                        default:
                            result.type = types::unknown();
                    }
                }
                else {
                    result.type = types::unknown();
                }
                break;
            case token::kind::greater_equal:
                if ((left.type->category() == ast::type::category::chars_type && right.type->category() == ast::type::category::string_type) ||
                    (left.type->category() == ast::type::category::string_type && right.type->category() == ast::type::category::chars_type) ||
                    left.type->category() == right.type->category()) {
                    switch (left.type->category()) {
                        case ast::type::category::integer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::integer_type>(right.type);
                            if ((ltype->is_signed() && !rtype->is_signed()) || (!ltype->is_signed() && rtype->is_signed())) {
                                result.type = types::unknown();
                            }
                            else if (ltype->is_signed()) {
                                result.type = types::boolean();
                                result.b = (left.i >= right.i);
                            }
                            else {
                                result.type = types::boolean();
                                result.b = (left.u >= right.u);
                            }
                            break;
                        }
                        case ast::type::category::rational_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::rational_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::rational_type>(right.type);
                            result.type = types::boolean();
                            result.b = (left.r >= right.r);
                            break;
                        }
                        case ast::type::category::float_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::float_type>(left.type);
                            auto rtype = std::dynamic_pointer_cast<ast::float_type>(right.type);
                            result.type = types::boolean();
                            result.b = (left.f >= right.f);
                            break;
                        }
                        case ast::type::category::chars_type:
                        case ast::type::category::string_type:
                        {
                            result.type = types::boolean();
                            result.b = (left.s >= right.s);
                            break;
                        }
                        case ast::type::category::char_type:
                        {
                            result.type = types::boolean();
                            result.b = (left.ch >= right.ch);
                            break;
                        }
                        default:
                            result.type = types::unknown();
                    }
                }
                else {
                    result.type = types::unknown();
                }
                break;
            default:
                result.type = types::unknown();
        }

        push(result);
    }

    void evaluator::visit(const ast::function_expression& expr) 
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use anonymous functions in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }
    
    void evaluator::visit(const ast::call_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot make function calls in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }

    void evaluator::visit(const ast::record_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot create structures or unions in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }

    void evaluator::visit(const ast::ignore_pattern_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use patterns in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }
    
    void evaluator::visit(const ast::literal_pattern_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use patterns in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }
        
    void evaluator::visit(const ast::path_pattern_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use patterns in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }
        
    void evaluator::visit(const ast::tuple_pattern_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use patterns in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }
        
    void evaluator::visit(const ast::array_pattern_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use patterns in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }
        
    void evaluator::visit(const ast::record_pattern_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use patterns in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }

    void evaluator::visit(const ast::labeled_record_pattern_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use patterns in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }
        
    void evaluator::visit(const ast::range_pattern_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use patterns in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }
        
    void evaluator::visit(const ast::or_pattern_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use patterns in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }
        
    void evaluator::visit(const ast::when_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use when in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }

    void evaluator::visit(const ast::when_pattern_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use when in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }

    void evaluator::visit(const ast::when_cast_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use when in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }
        
    void evaluator::visit(const ast::for_range_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use for range in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }
        
    void evaluator::visit(const ast::for_loop_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use for loop in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }
        
    void evaluator::visit(const ast::if_expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) return push(expr.annotation().value);
        if (expr.invalid()) throw evaluator::error();
        checker_.error(expr.range(), "You cannot use if in constant expression, idiot!", impl::const_expr_explanation); 
        throw evaluator::error();
    }
}
