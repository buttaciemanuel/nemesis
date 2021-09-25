#include <stdexcept>

#include "nemesis/analysis/type.hpp"

namespace nemesis {
    constval::constval()
    {
        std::memset(reinterpret_cast<void*>(this), 0, sizeof(constval));
        type = types::unknown();
        new (&s) string();
        new (&seq) sequence();

    }

    namespace ast {
        void printer::visit(const implicit_conversion_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "implicit_conversion_expression " << impl::color::reset << expr.range().begin() << " `" << expr.expression()->annotation().type->string() << "` => `"<< impl::color::white  << expr.annotation().type->string() << impl::color::reset << "`\n";

            prefix_.push(true);
            expr.expression()->accept(*this);
            prefix_.pop();
        }

        bool bit_field_type_expression::is_parametric() const { return false; }

        bool path_type_expression::is_parametric() const
        {
           if (annotation_.isparametric) return true;
            else if (!annotation_.istype) return false;
            else if (annotation_.type->category() == ast::type::category::generic_type) return true;
            
            if (auto member = std::dynamic_pointer_cast<ast::identifier_expression>(member_)) {
                for (auto generic : member->generics()) {
                    if (generic->annotation().isparametric) return true;
                    else if (auto tparam = std::dynamic_pointer_cast<ast::type_expression>(generic)) {
                        if (tparam->is_parametric()) return true;
                    }
                    else if (auto referenced = generic->annotation().referencing) {
                        if (referenced->kind() == ast::kind::generic_type_parameter_declaration || referenced->kind() == ast::kind::generic_const_parameter_declaration) return true;
                    }
                }
            }

            if (auto expr = std::dynamic_pointer_cast<ast::identifier_expression>(expr_)) {
                for (auto generic : expr->generics()) {
                    if (generic->annotation().isparametric) return true;
                    else if (auto tparam = std::dynamic_pointer_cast<ast::type_expression>(generic)) {
                        if (tparam->is_parametric()) return true;
                    }
                    else if (auto referenced = generic->annotation().referencing) {
                        if (referenced->kind() == ast::kind::generic_type_parameter_declaration || referenced->kind() == ast::kind::generic_const_parameter_declaration) return true;
                    }
                }
            }
            else if (auto expr = std::dynamic_pointer_cast<ast::path_type_expression>(expr_)) {
                return expr->is_parametric();
            }

            return false;
        }

        bool array_type_expression::is_parametric() const
        {
            return std::static_pointer_cast<ast::type_expression>(element_type_)->is_parametric();
        }

        bool tuple_type_expression::is_parametric() const
        {
            for (auto param : types_) {
                if (auto tparam = std::dynamic_pointer_cast<ast::type_expression>(param)) {
                    if (tparam->is_parametric()) return true;
                }
            }
            
            return false;
        }

        bool record_type_expression::is_parametric() const
        {
            for (auto f : fields_) {
                if (auto tfield = std::dynamic_pointer_cast<ast::tuple_field_declaration>(f)) {
                    if (std::static_pointer_cast<ast::type_expression>(tfield->type_expression())->is_parametric()) return true;
                }
                else if (auto field = std::dynamic_pointer_cast<ast::field_declaration>(f)) {
                    if (std::static_pointer_cast<ast::type_expression>(field->type_expression())->is_parametric()) return true;
                }
            }
            
            return false;
        }

        bool function_type_expression::is_parametric() const
        {
            for (auto tparam : parameter_types_) {
                if (std::static_pointer_cast<ast::type_expression>(tparam)->is_parametric()) return true;
            }

            if (return_type_ && std::static_pointer_cast<ast::type_expression>(return_type_)->is_parametric()) return true;
            
            return false;
        }

        std::string pointer_type::string(bool absolute) const 
        {
            if (base_->category() == ast::type::category::variant_type && !base_->declaration() && std::static_pointer_cast<ast::variant_type>(base_)->types().size() > 1) {
                return "*{ " + base_->string() + " }";
            }
            return "*" + base_->string(); 
        }

        bool pointer_type_expression::is_parametric() const
        {
            return std::static_pointer_cast<ast::type_expression>(pointee_type_)->is_parametric();   
        }

        bool variant_type_expression::is_parametric() const
        {
            for (auto param : types_) {
                if (auto tparam = std::dynamic_pointer_cast<ast::type_expression>(param)) {
                    if (tparam->is_parametric()) return true;
                }
            }
            
            return false;
        }
    }
    namespace impl {
        std::unordered_map<std::string, ast::pointer<ast::type>> builtins { 
            { "unit", types::unit() },
            { "bool", types::boolean() },
            { "char", types::character() },
            { "chars", types::chars() },
            { "string", types::string() },
            { "u8", types::uint(8) },
            { "u16", types::uint(16) },
            { "u32", types::uint(32) },
            { "u64", types::uint(64) },
            { "u128", types::uint(128) },
            { "usize", sizeof(size_t) == 4 ? types::uint(32) : types::uint(64) },
            { "i8", types::sint(8) },
            { "i16", types::sint(16) },
            { "i32", types::sint(32) },
            { "i64", types::sint(64) },
            { "i128", types::sint(128) },
            { "isize", sizeof(size_t) == 4 ? types::sint(32) : types::sint(64) },
            { "r16", types::rational(16) },
            { "r32", types::rational(32) },
            { "r64", types::rational(64) },
            { "r128", types::rational(128) },
            { "r256", types::rational(256) },
            { "f32", types::floating(32) },
            { "f64", types::floating(64) },
            { "f128", types::floating(128) },
            { "c64", types::complex(64) },
            { "c128", types::complex(128) },
            { "c256", types::complex(256) }
        };
    }
    
    bool ast::variant_type::contains(ast::pointer<ast::type> subtype) const
    {
        for (auto ty : types_) {
            if (nemesis::types::compatible(subtype, ty)) return true;
        }

        return false;
    }

    void ast::behaviour_type::implements(ast::pointer<ast::type> type) { implementors.insert(type); }

    bool ast::behaviour_type::implementor(ast::pointer<ast::type> type) const
    {
        for (auto implementor : implementors) {
            if (nemesis::types::compatible(type, implementor)) return true;
        }

        return false;
    }
    
    ast::types types::others_ {};

    std::unordered_map<ast::pointer<ast::type>, std::set<ast::pointer<ast::type>>> types::implementors_ {};

    std::unordered_map<ast::pointer<ast::type>, std::set<const ast::declaration*>> types::extenders_ {};

    std::unordered_map<ast::pointer<ast::type>, types::parametrized_type_info> types::parametrized_ {};

    std::unordered_map<ast::pointer<ast::type>, ast::pointer<ast::generic_clause_declaration>> types::parametrics_ {};

    ast::pointer<ast::type> types::builtin(std::string name)
    {

        auto result = impl::builtins.find(name);

        if (result != impl::builtins.end()) return result->second;

        return nullptr;
    }

    ast::pointer<ast::unknown_type> types::unknown() 
    {
        static ast::pointer<ast::unknown_type> unknown_(ast::create<ast::unknown_type>());
        return unknown_; 
    }
    
    ast::pointer<ast::workspace_type> types::workspace() 
    {
        static ast::pointer<ast::workspace_type> workspace_(ast::create<ast::workspace_type>());
        return workspace_; 
    }

    ast::pointer<ast::generic_type> types::generic() 
    {
        auto type = ast::create<ast::generic_type>();
        others_.push_back(type);
        return type;
    }

    ast::pointer<ast::integer_type> types::bitfield(unsigned bits)
    {
        static std::vector<ast::pointer<ast::integer_type>> bits_;
        if (bits_.empty()) for (unsigned i = 1; i <= 256; ++i) bits_.push_back(ast::create<ast::integer_type>(i, false));
        if (bits > bits_.size()) throw std::invalid_argument("types::bitfield: bits must be less or equal 256");
        return bits_.at(bits - 1);
    }
    
    ast::pointer<ast::tuple_type> types::unit() 
    {
        static ast::pointer<ast::tuple_type> unit_(ast::create<ast::tuple_type>(ast::types()));
        return unit_; 
    }
    
    ast::pointer<ast::bool_type> types::boolean() 
    {
        static ast::pointer<ast::bool_type> bool_(ast::create<ast::bool_type>());
        return bool_; 
    }
    
    ast::pointer<ast::char_type> types::character() 
    {
        static ast::pointer<ast::char_type> char_(ast::create<ast::char_type>());
        return char_; 
    }
    
    ast::pointer<ast::chars_type> types::chars() 
    {
        static ast::pointer<ast::chars_type> chars_(ast::create<ast::chars_type>());
        return chars_; 
    }
    
    ast::pointer<ast::string_type> types::string() 
    {
        static ast::pointer<ast::string_type> string_(ast::create<ast::string_type>());
        return string_; 
    }

    ast::pointer<ast::integer_type> types::isize() { return sint(8 * sizeof(size_t)); }
    
    ast::pointer<ast::integer_type> types::usize() { return uint(8 * sizeof(size_t)); }
    
    ast::pointer<ast::integer_type> types::sint(unsigned bits)
    {
        static std::vector<ast::pointer<ast::integer_type>> ints_ {
            ast::create<ast::integer_type>(8, true),
            ast::create<ast::integer_type>(16, true),
            ast::create<ast::integer_type>(32, true),
            ast::create<ast::integer_type>(64, true),
            ast::create<ast::integer_type>(128, true)
        };

        switch (bits) {
            case 8:
                return ints_.at(0);
            case 16:
                return ints_.at(1);
            case 32:
                return ints_.at(2);
            case 64:
                return ints_.at(3);
            case 128:
                return ints_.at(4);
            default:
                throw std::invalid_argument("types::bitfield: invalid bits count for integer type");
        }

        return nullptr;
    }
    
    ast::pointer<ast::integer_type> types::uint(unsigned bits)
    {
        static std::vector<ast::pointer<ast::integer_type>> uints_ {
            ast::create<ast::integer_type>(8, false),
            ast::create<ast::integer_type>(16, false),
            ast::create<ast::integer_type>(32, false),
            ast::create<ast::integer_type>(64, false),
            ast::create<ast::integer_type>(128, false)
        };

        switch (bits) {
            case 8:
                return uints_.at(0);
            case 16:
                return uints_.at(1);
            case 32:
                return uints_.at(2);
            case 64:
                return uints_.at(3);
            case 128:
                return uints_.at(4);
            default:
                throw std::invalid_argument("types::bitfield: invalid bits count for unsigned integer type");
        }

        return nullptr;
    }
    
    ast::pointer<ast::rational_type> types::rational(unsigned bits)
    {
        static std::vector<ast::pointer<ast::rational_type>> rationals_ {
            ast::create<ast::rational_type>(16),
            ast::create<ast::rational_type>(32),
            ast::create<ast::rational_type>(64),
            ast::create<ast::rational_type>(128),
            ast::create<ast::rational_type>(256)
        };

        switch (bits) {
            case 16:
                return rationals_.at(0);
            case 32:
                return rationals_.at(1);
            case 64:
                return rationals_.at(2);
            case 128:
                return rationals_.at(3);
            case 256:
                return rationals_.at(4);
            default:
                throw std::invalid_argument("types::bitfield: invalid bits count for rational type");
        }

        return nullptr;
    }
    
    ast::pointer<ast::float_type> types::floating(unsigned bits)
    {
        static std::vector<ast::pointer<ast::float_type>> floats_ {
            ast::create<ast::float_type>(32),
            ast::create<ast::float_type>(64),
            ast::create<ast::float_type>(128),
        };

        switch (bits) {
            case 32:
                return floats_.at(0);
            case 64:
                return floats_.at(1);
            case 128:
                return floats_.at(2);
            default:
                throw std::invalid_argument("types::bitfield: invalid bits count for float type");
        }

        return nullptr;
    }
    
    ast::pointer<ast::complex_type> types::complex(unsigned bits)
    {
        static std::vector<ast::pointer<ast::complex_type>> complexes_ {
            ast::create<ast::complex_type>(64),
            ast::create<ast::complex_type>(128),
            ast::create<ast::complex_type>(256)
        };

        switch (bits) {
            case 64:
                return complexes_.at(0);
            case 128:
                return complexes_.at(1);
            case 256:
                return complexes_.at(2);
            default:
                throw std::invalid_argument("types::bitfield: invalid bits count for complex type");
        }

        return nullptr;
    }

    ast::pointer<ast::structure_type> types::record(ast::structure_type::components fields)
    {
        auto type = ast::create<ast::structure_type>(fields);
        others_.push_back(type);
        return type;
    }

    ast::pointer<ast::variant_type> types::variant(ast::types types)
    {
        auto type = ast::create<ast::variant_type>(types);
        others_.push_back(type);
        return type;
    }

    ast::pointer<ast::behaviour_type> types::behaviour()
    {
        auto type = ast::create<ast::behaviour_type>();
        others_.push_back(type);
        return type;
    }

    ast::pointer<ast::array_type> types::array(ast::pointer<ast::type> base, unsigned size)
    {
        auto type = ast::create<ast::array_type>(base, size);
        others_.push_back(type);
        return type;
    }

    ast::pointer<ast::tuple_type> types::tuple(ast::types types)
    {
        auto type = ast::create<ast::tuple_type>(types);
        others_.push_back(type);
        return type;
    }
        
    ast::pointer<ast::slice_type> types::slice(ast::pointer<ast::type> base)
    {
        auto type = ast::create<ast::slice_type>(base);
        others_.push_back(type);
        return type;
    }

    ast::pointer<ast::range_type> types::range(ast::pointer<ast::type> base, bool open)
    {
        auto type = ast::create<ast::range_type>(base, open);
        others_.push_back(type);
        return type;
    }

    ast::pointer<ast::pointer_type> types::pointer(ast::pointer<ast::type> base) 
    { 
        auto type = ast::create<ast::pointer_type>(base);
        others_.push_back(type);
        return type;
    }

    ast::pointer<ast::function_type> types::function(ast::types formals, ast::pointer<ast::type> result, bool lambda)
    { 
        auto type = ast::create<ast::function_type>(formals, result, lambda);
        others_.push_back(type);
        return type;
    }

    void types::implements(ast::pointer<ast::type> implementor, ast::pointer<ast::type> behaviour)
    {
        if (implementors_.find(implementor) == implementors_.end()) implementors_.emplace(implementor, std::set<ast::pointer<ast::type>> { behaviour });
        else implementors_[implementor].insert(behaviour);

        std::static_pointer_cast<ast::behaviour_type>(behaviour)->implements(implementor);
    }

    void types::extends(const ast::declaration* extender, ast::pointer<ast::type> type)
    {
        if (extenders_.find(type) == extenders_.end()) extenders_.emplace(type, std::set<const ast::declaration*> { extender });
        else extenders_[type].insert(extender);
    }

    bool types::compatible(ast::pointer<ast::type> left, ast::pointer<ast::type> right, bool strict)
    {
        if (!left || !right || left->category() == ast::type::category::unknown_type || right->category() == ast::type::category::unknown_type) return false;

        if (left->category() != right->category()) return false;
        // name equivalence
        if (left->declaration() && right->declaration()) {
            return left->declaration() == right->declaration();
        }
        else if ((left->declaration() && !right->declaration()) || (!left->declaration() && right->declaration())) return false;
        // no declaration, so structural equivalence for tuple and structures
        switch (left->category()) {
            case ast::type::category::char_type:
            case ast::type::category::bool_type:
            case ast::type::category::chars_type:
            case ast::type::category::string_type:
                return true;
            case ast::type::category::integer_type:
                if (strict) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(left), rtype = std::dynamic_pointer_cast<ast::integer_type>(right);
                    return ltype->is_signed() == rtype->is_signed() && ltype->bits() == rtype->bits();
                }
                else {
                    return true;
                }
            case ast::type::category::rational_type:
                if (strict) {
                    auto ltype = std::dynamic_pointer_cast<ast::rational_type>(left), rtype = std::dynamic_pointer_cast<ast::rational_type>(right);
                    return ltype->bits() == rtype->bits();
                }
                else {
                    return true;
                }
            case ast::type::category::float_type:
                if (strict) {
                    auto ltype = std::dynamic_pointer_cast<ast::float_type>(left), rtype = std::dynamic_pointer_cast<ast::float_type>(right);
                    return ltype->bits() == rtype->bits();
                }
                else {
                    return true;
                } 
            case ast::type::category::complex_type:
                if (strict) {
                    auto ltype = std::dynamic_pointer_cast<ast::complex_type>(left), rtype = std::dynamic_pointer_cast<ast::complex_type>(right);
                    return ltype->bits() == rtype->bits();
                }
                else {
                    return true;
                }
            case ast::type::category::array_type:
            {
                auto ltype = std::dynamic_pointer_cast<ast::array_type>(left), rtype = std::dynamic_pointer_cast<ast::array_type>(right);
                return ltype->size() == rtype->size() && compatible(ltype->base(), rtype->base(), strict);
            }
            case ast::type::category::slice_type:
            {
                auto ltype = std::dynamic_pointer_cast<ast::slice_type>(left), rtype = std::dynamic_pointer_cast<ast::slice_type>(right);
                return compatible(ltype->base(), rtype->base(), strict);
            }
            case ast::type::category::tuple_type:
            {
                auto ltype = std::dynamic_pointer_cast<ast::tuple_type>(left), rtype = std::dynamic_pointer_cast<ast::tuple_type>(right);    
                if (ltype->length() != rtype->length()) return false;
                for (size_t i = 0; i < ltype->length(); ++i) {
                    if (!compatible(ltype->components().at(i), rtype->components().at(i), strict)) {
                        return false;
                    }
                }
                return true;
            }
            case ast::type::category::structure_type:
            {
                auto ltype = std::dynamic_pointer_cast<ast::structure_type>(left), rtype = std::dynamic_pointer_cast<ast::structure_type>(right);    
                if (ltype->fields().size() != rtype->fields().size()) return false;
                for (size_t i = 0; i < ltype->fields().size(); ++i) {
                    if (ltype->fields().at(i).name != rtype->fields().at(i).name || !compatible(ltype->fields().at(i).type, rtype->fields().at(i).type, strict)) {
                        return false;
                    }
                }
                return true;
            }
            case ast::type::category::variant_type:
            {
                auto ltype = std::dynamic_pointer_cast<ast::variant_type>(left), rtype = std::dynamic_pointer_cast<ast::variant_type>(right);
                if (ltype->types().size() != rtype->types().size()) return false;  
                for (size_t i = 0; i < ltype->types().size(); ++i) {
                    if (!compatible(ltype->types().at(i), rtype->types().at(i), strict)) {
                        return false;
                    }
                }
                return true;
            }
            case ast::type::category::function_type:
            {
                auto ltype = std::dynamic_pointer_cast<ast::function_type>(left), rtype = std::dynamic_pointer_cast<ast::function_type>(right);    
                if (ltype->formals().size() != rtype->formals().size() || !compatible(ltype->result(), rtype->result(), strict)) return false;
                for (size_t i = 0; i < ltype->formals().size(); ++i) {
                    if (!compatible(ltype->formals().at(i), rtype->formals().at(i), strict) || ltype->formals().at(i)->mutability != rtype->formals().at(i)->mutability) {
                        return false;
                    }
                }
                return true;
            }
            case ast::type::category::pointer_type:
            {
                auto ltype = std::dynamic_pointer_cast<ast::pointer_type>(left), rtype = std::dynamic_pointer_cast<ast::pointer_type>(right);
                if ((ltype->base()->category() == ast::type::category::behaviour_type && std::static_pointer_cast<ast::behaviour_type>(ltype->base())->implementor(rtype->base())) ||
                    (rtype->base()->category() == ast::type::category::behaviour_type && std::static_pointer_cast<ast::behaviour_type>(rtype->base())->implementor(ltype->base()))) 
                {
                    return true;
                }
                return compatible(ltype->base(), rtype->base(), strict);
            }
            case ast::type::category::range_type:
            {
                auto ltype = std::dynamic_pointer_cast<ast::range_type>(left), rtype = std::dynamic_pointer_cast<ast::range_type>(right);
                return compatible(ltype->base(), rtype->base(), strict) && ltype->is_open() == rtype->is_open();
            }
            default:
                break;
        }

        return false;
    }

    // left is expected type, right is assigned type
    bool types::assignment_compatible(ast::pointer<ast::type> left, ast::pointer<ast::type> right)
    {
        // invalid types
        if (!left || !right || left->category() == ast::type::category::unknown_type || right->category() == ast::type::category::unknown_type) return false;
        // automatic dereference
        if (left->category() == ast::type::category::pointer_type && types::compatible(right, std::static_pointer_cast<ast::pointer_type>(left)->base(), false)) return true;
        // behaviour switch
        if (right->category() == ast::type::category::behaviour_type && left->category() != ast::type::category::behaviour_type) return assignment_compatible(right, left);

        switch (left->category())
        {
            // recursively called for components
            case ast::type::category::tuple_type:
            {
                auto ltype = std::dynamic_pointer_cast<ast::tuple_type>(left);
                if (auto rtype = std::dynamic_pointer_cast<ast::tuple_type>(right)) {
                    if (ltype->length() != rtype->length()) return false;
                    for (size_t i = 0; i < ltype->length(); ++i) {
                        if (!assignment_compatible(ltype->components().at(i), rtype->components().at(i))) {
                            return false;
                        }
                    }
                    return true;
                }
                return false;
            }
            // recursively called for base type
            case ast::type::category::array_type:
            {
                auto ltype = std::dynamic_pointer_cast<ast::array_type>(left);
                if (auto rtype = std::dynamic_pointer_cast<ast::array_type>(right)) {
                    return ltype->size() == rtype->size() && assignment_compatible(ltype->base(), rtype->base());
                }
                else if (auto rtype = std::dynamic_pointer_cast<ast::pointer_type>(right)) {
                    // array to pointer, not safe
                    return types::assignment_compatible(std::static_pointer_cast<ast::array_type>(left)->base(), rtype->base());
                }
                else return types::compatible(left, right, false);
            }
            // automatic cast from array to slice
            case ast::type::category::slice_type:
                if (auto rtype = std::dynamic_pointer_cast<ast::array_type>(right)) {
                    // array to slice
                    if (types::assignment_compatible(std::static_pointer_cast<ast::slice_type>(left)->base(), rtype->base())) return true;
                    // automatic cast from empty array of unknown type to slice
                    if (rtype->base()->category() == ast::type::category::unknown_type && rtype->size() == 0) return true;
                }
                else if (auto rtype = std::dynamic_pointer_cast<ast::pointer_type>(right)) {
                    // slice to pointer, not safe
                    return types::assignment_compatible(std::static_pointer_cast<ast::slice_type>(left)->base(), rtype->base());
                }
                else return types::compatible(left, right, false);
            // automatic cast from string to chars or viceversa
            case ast::type::category::chars_type:
            case ast::type::category::string_type:
                return right->category() == ast::type::category::string_type || right->category() == ast::type::category::chars_type;
            // automatic address of
            case ast::type::category::pointer_type:
                if (types::compatible(std::static_pointer_cast<ast::pointer_type>(left)->base(), right)) return true;
                else if (auto rtype = std::dynamic_pointer_cast<ast::pointer_type>(right)) {
                    // behaviour for upcasting
                    auto ltype = std::dynamic_pointer_cast<ast::pointer_type>(left);
                    if ((ltype->base()->category() == ast::type::category::behaviour_type && std::static_pointer_cast<ast::behaviour_type>(ltype->base())->implementor(rtype->base())) ||
                        (rtype->base()->category() == ast::type::category::behaviour_type && std::static_pointer_cast<ast::behaviour_type>(rtype->base())->implementor(ltype->base()))) return true;
                    return types::assignment_compatible(std::static_pointer_cast<ast::pointer_type>(left)->base(), rtype->base());
                }
                else return types::compatible(left, right, false);
            // left type is variant while right is a subtype
            case ast::type::category::variant_type:
                if (std::static_pointer_cast<ast::variant_type>(left)->contains(right)) return true;
                else return types::compatible(left, right, false);
            default:
                break;
        }

        return types::compatible(left, right, false);
    }
}