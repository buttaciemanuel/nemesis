#ifndef TYPE_HPP
#define TYPE_HPP

#include <unordered_map>

#include "nemesis/parser/ast.hpp"

namespace nemesis {
    namespace impl {
        struct parameter {
            enum class kind { empty, type, value };
            ast::pointer<ast::type> type = nullptr;
            constval value;
            const ast::declaration* referencing = nullptr;
            enum kind kind = kind::empty;
            bool variadic = false;

            static parameter make_value(constval value) 
            {
                parameter result;
                result.kind = kind::value;
                result.value = value;
                result.type = value.type;
                return result;
            }

            static parameter make_type(ast::pointer<ast::type> type) 
            {
                parameter result;
                result.kind = kind::type;
                result.type = type;
                return result;
            }
        };
    }

    struct substitution {
        std::unordered_map<const ast::declaration*, impl::parameter> map;
        substitution(const std::unordered_map<const ast::declaration*, impl::parameter>& map) : map(map) {}
    };

    namespace ast {
        class type {
        public:
            enum class category {
                integer_type,
                rational_type,
                float_type,
                complex_type,
                bool_type,
                char_type,
                chars_type,
                string_type,
                array_type,
                slice_type,
                tuple_type,
                pointer_type,
                range_type,
                function_type,
                structure_type,
                variant_type,
                behaviour_type,
                generic_type,
                workspace_type,
                unknown_type
            };
            virtual enum category category() const = 0;
            virtual std::string string(bool absolute = true) const = 0;
            virtual ~type() {}
            void declaration(const ast::declaration* decl) { declaration_ = decl; }
            virtual const ast::declaration* declaration() const { return declaration_; }
            std::string prefix() const 
            {
                std::string result;
                
                for (const ast::node* node = declaration_; node;) {
                    if (auto declaration = dynamic_cast<const ast::declaration*>(node)) {
                        if (auto typescope = dynamic_cast<const ast::type_declaration*>(declaration->annotation().scope)) result.insert(0, typescope->name().lexeme().string().append("."));
                        node = declaration->annotation().scope;
                    }
                    else break;
                }
                
                return result;
            }
            ast::pointer<ast::type> parent() const {
                if (declaration_) {
                    if (auto typescope = dynamic_cast<const ast::type_declaration*>(declaration_->annotation().scope)) return typescope->annotation().type;
                }

                return nullptr;
            }
            // before is current type wrapped as a smart pointer, while map is a mapping between generic type/value declaration and argument (constval or type)
            virtual ast::pointer<ast::type> substitute(ast::pointer<ast::type> before, std::unordered_map<const ast::declaration*, impl::parameter> map) const { return before; }
            // mutability bit, used only for declarations of variables' types and for return types
            bool mutability : 1;
        protected:
            type() : mutability(false) {}
            const ast::declaration* declaration_ = nullptr;
        };

        using types = std::vector<pointer<type>>;

        class unknown_type : public type {
        public:
            unknown_type() : type() {}
            ~unknown_type() {}
            std::string string(bool absolute = true) const { return "_"; }
            enum category category() const { return category::unknown_type; }
        };

        class workspace_type : public type {
        public:
            workspace_type() : type() {}
            ~workspace_type() {}
            std::string string(bool absolute = true) const { return static_cast<const ast::workspace*>(declaration_)->name; }
            enum category category() const { return category::workspace_type; }
            ast::pointer<ast::type> substitute(ast::pointer<ast::type> before, std::unordered_map<const ast::declaration*, impl::parameter> map) const { return before; }
        };

        class generic_type : public type {
        public:
            generic_type() : type() {}
            ~generic_type() {}
            std::string string(bool absolute = true) const { return "$" + static_cast<const ast::generic_type_parameter_declaration*>(declaration_)->name().lexeme().string(); }
            enum category category() const { return category::generic_type; }
            ast::pointer<ast::type> substitute(ast::pointer<ast::type> before, std::unordered_map<const ast::declaration*, impl::parameter> map) const
            {
                if (map.count(declaration_)) return map.at(declaration_).type;
                return before;
            }
        };

        class integer_type : public type {
        public:
            integer_type(unsigned bits, bool is_signed) : type(), bits_(bits), signed_(is_signed) {}
            ~integer_type() {}
            unsigned bits() const { return bits_; }
            bool is_signed() const { return signed_; }
            std::string string(bool absolute = true) const 
            {
                switch (bits_) {
                    case 8:
                    case 16:
                    case 32:
                    case 64:
                    case 128:
                        return (signed_ ? "i" : "u") + std::to_string(bits_);
                    default:
                        return "bits" + std::to_string(bits_);
                }

                return {};
            }
            enum category category() const { return category::integer_type; }
        private:
            unsigned bits_;
            bool signed_;
        };

        class rational_type : public type {
        public:
            rational_type(unsigned bits) : type(), bits_(bits) {}
            ~rational_type() {}
            unsigned bits() const { return bits_; }
            std::string string(bool absolute = true) const { return "r" + std::to_string(bits_); }
            enum category category() const { return category::rational_type; }
        private:
            unsigned bits_;
        };

        class float_type : public type {
        public:
            float_type(unsigned bits) : type(), bits_(bits) {}
            ~float_type() {}
            unsigned bits() const { return bits_; }
            std::string string(bool absolute = true) const { return "f" + std::to_string(bits_); }
            enum category category() const { return category::float_type; }
        private:
            unsigned bits_;
        };

        class complex_type : public type {
        public:
            complex_type(unsigned bits) : type(), bits_(bits) {}
            ~complex_type() {}
            unsigned bits() const { return bits_; }
            std::string string(bool absolute = true) const { return "c" + std::to_string(bits_); }
            enum category category() const { return category::complex_type; }
        private:
            unsigned bits_;
        };

        class bool_type : public type {
        public:
            bool_type() : type() {}
            ~bool_type() {}
            std::string string(bool absolute = true) const { return "bool"; }
            enum category category() const { return category::bool_type; }
        };

        class char_type : public type {
        public:
            char_type() : type() {}
            ~char_type() {}
            std::string string(bool absolute = true) const { return "char"; }
            enum category category() const { return category::char_type; }
        };

        class chars_type : public type {
        public:
            chars_type() : type() {}
            ~chars_type() {}
            std::string string(bool absolute = true) const { return "chars"; }
            enum category category() const { return category::chars_type; }
        };

        class string_type : public type {
        public:
            string_type() : type() {}
            ~string_type() {}
            std::string string(bool absolute = true) const { return "string"; }
            enum category category() const { return category::string_type; }
        };

        class array_type : public type {
        public:
            array_type(pointer<type> base, unsigned size) : type(), base_(base), size_(size) {}
            ~array_type() {}
            pointer<type> base() const { return base_; }
            unsigned size() const { return size_; }
            std::string string(bool absolute = true) const 
            {
                if (size_ > 0) return "[" + base_->string() + " : " + std::to_string(size_) + "]";
                if (parametric_size_) return "[" + base_->string() + " : $" + parametric_size_->name().lexeme().string() + "]"; 
                return "[" + base_->string() + " : _]";
            }
            enum category category() const { return category::array_type; }
            const ast::generic_const_parameter_declaration*& parametric_size() const { return parametric_size_; }
            ast::pointer<ast::type> substitute(ast::pointer<ast::type> before, std::unordered_map<const ast::declaration*, impl::parameter> map) const
            {
                auto base = base_->substitute(base_, map);
                
                if (parametric_size_ && map.count(parametric_size_)) {
                    auto result = ast::create<ast::array_type>(base, 0);
                    auto size = map.at(parametric_size_);
                    if (auto parametric = dynamic_cast<const ast::generic_const_parameter_declaration*>(size.referencing)) {
                        result->parametric_size_ = parametric;
                    }
                    else if (size.kind == impl::parameter::kind::value) {
                        result->size_ = size.value.u.value();
                        result->parametric_size_ = nullptr;
                    }
                    return result;
                }
                
                if (base_ == base) return before;
                
                auto result = ast::create<ast::array_type>(base, size_);
                result->mutability = mutability;
                
                return result;
            }
        private:
            pointer<type> base_;
            unsigned size_;
            mutable const ast::generic_const_parameter_declaration* parametric_size_ = nullptr;
        };

        class slice_type : public type {
        public:
            slice_type(pointer<type> base) : type(), base_(base) {}
            ~slice_type() {}
            pointer<type> base() const { return base_; }
            std::string string(bool absolute = true) const { return "[" + base_->string() + "]"; }
            enum category category() const { return category::slice_type; }
            ast::pointer<ast::type> substitute(ast::pointer<ast::type> before, std::unordered_map<const ast::declaration*, impl::parameter> map) const
            {
                auto base = base_->substitute(base_, map);

                if (base_ == base) return before;

                auto result = ast::create<ast::slice_type>(base);
                result->mutability = mutability;
                
                return result;
            }
        private:
            pointer<type> base_;
        };

        class tuple_type : public type {
        public:
            tuple_type(types components) : type(), components_(components) {}
            ~tuple_type() {}
            const types& components() const { return components_; }
            void components(types value) { components_ = value; }
            unsigned length() const { return components_.size(); }
            std::string string(bool absolute = true) const 
            {
                if (declaration_) {
                    if (absolute) return prefix() + dynamic_cast<const ast::record_declaration*>(declaration_)->name().lexeme().string();
                    else return dynamic_cast<const ast::record_declaration*>(declaration_)->name().lexeme().string();
                }
                else if (components_.size() == 0) return "()";
                else {
                    std::string str = '(' + components_.front()->string();
                    for (std::size_t i = 1; i < components_.size(); ++i) str = str + ", " + components_.at(i)->string();
                    str += ')';
                    return str;
                }
            }
            enum category category() const { return category::tuple_type; }
            ast::pointer<ast::type> substitute(ast::pointer<ast::type> before, std::unordered_map<const ast::declaration*, impl::parameter> map) const
            {
                bool change = false;
                types components;
                for (auto old : components_) {
                    auto component = old->substitute(old, map);
                    if (old != component) change = true;
                    components.push_back(component);
                }
                if (change) {
                    auto result = ast::create<ast::tuple_type>(components);
                    result->declaration_ = declaration_;
                    result->mutability = mutability;
                    return result;
                } 
                return before;
            }
        private:
            types components_;
        };

        class pointer_type : public type {
        public:
            pointer_type(pointer<type> base) : type(), base_(base) {}
            ~pointer_type() {}
            pointer<type> base() const { return base_; }
            std::string string(bool absolute = true) const;
            enum category category() const { return category::pointer_type; }
            ast::pointer<ast::type> substitute(ast::pointer<ast::type> before, std::unordered_map<const ast::declaration*, impl::parameter> map) const
            {
                auto base = base_->substitute(base_, map);
                if (base_ == base) return before;

                auto result = ast::create<ast::pointer_type>(base);
                result->mutability = mutability;

                return result;
            }
        private:
            pointer<type> base_;
        };

        class range_type : public type {
        public:
            range_type(pointer<type> base, bool open) : type(), base_(base), open_(open) {}
            ~range_type() {}
            bool is_open() const { return open_; }
            void open(bool flag) { open_ = flag; }
            void base(pointer<type> b) { base_ = b; }
            pointer<type> base() const { return base_; }
            std::string string(bool absolute = true) const 
            { 
                if (declaration_) {
                    if (absolute) return prefix() + dynamic_cast<const ast::range_declaration*>(declaration_)->name().lexeme().string();
                    else return dynamic_cast<const ast::range_declaration*>(declaration_)->name().lexeme().string();
                }
                return base_->string() + (open_ ? ".." : "..=") + base_->string(); 
            }
            enum category category() const { return category::range_type; }
            ast::pointer<ast::type> substitute(ast::pointer<ast::type> before, std::unordered_map<const ast::declaration*, impl::parameter> map) const
            {
                auto base = base_->substitute(base_, map);
                if (base_ == base) return before;

                auto result = ast::create<ast::range_type>(base, open_);
                result->mutability = mutability;
                
                return result;
            }
        private:
            pointer<type> base_;
            bool open_;
        };

        class function_type : public type {
        public:
            function_type(types formals, pointer<type> result) : type(), formals_(formals), result_(result) {}
            ~function_type() {}
            const types& formals() const { return formals_; }
            pointer<type> result() const { return result_; }
            std::string string(bool absolute = true) const 
            {
                std::string str = "function";
                if (formals_.empty()) str += "()";
                else {
                    str += '(' + (formals_.front()->mutability ? std::string("mutable ") : std::string()) + formals_.front()->string();
                    for (std::size_t i = 1; i < formals_.size(); ++i) str += ", " + (formals_.at(i)->mutability ? std::string("mutable ") : std::string()) + formals_.at(i)->string();
                    str += ')';
                }
                if (result_) str += " " + (result_->mutability ? std::string("mutable ") : std::string()) + result_->string();
                return str;
            }
            enum category category() const { return category::function_type; }
            ast::pointer<ast::type> substitute(ast::pointer<ast::type> before, std::unordered_map<const ast::declaration*, impl::parameter> map) const
            {
                types formals;
                auto result = result_->substitute(result_, map);
                bool change = result != result_;
                for (auto old : formals_) {
                    auto f = old->substitute(old, map);
                    if (old != f) change = true;
                    formals.push_back(f);
                }
                
                if (change) {
                    auto ret = ast::create<ast::function_type>(formals, result);
                    ret->mutability = mutability;
                    return ret;
                }

                return before;
            }
        private:
            types formals_;
            pointer<type> result_;
        };

        class structure_type : public type {
        public:
            struct component { 
                std::string name; 
                ast::pointer<ast::type> type;
                component(const std::string& name, ast::pointer<ast::type> type) : name(name), type(type) {}
            };

            using components = std::vector<component>;

            structure_type(components fields) : type(), fields_(fields) {}
            ~structure_type() {}
            const components& fields() const { return fields_; }
            void fields(components fields) { fields_ = fields; }
            std::string string(bool absolute = true) const 
            {
                if (declaration_) {
                    if (absolute) return prefix() + dynamic_cast<const ast::record_declaration*>(declaration_)->name().lexeme().string();
                    else return dynamic_cast<const ast::record_declaration*>(declaration_)->name().lexeme().string();
                }
                else if (fields_.empty()) return "()";
                else {
                    std::string str = '(' + fields_.front().name + ": " + fields_.front().type->string();
                    for (std::size_t i = 1; i < fields_.size(); ++i) str += ", " + fields_.at(i).name + ": " + fields_.at(i).type->string();
                    return str + ")";
                }
            }
            enum category category() const { return category::structure_type; }
            ast::pointer<ast::type> substitute(ast::pointer<ast::type> before, std::unordered_map<const ast::declaration*, impl::parameter> map) const
            {
                bool change = false;
                components fields;
                for (auto old : fields_) {
                    auto t = old.type->substitute(old.type, map);
                    if (old.type != t) change = true;
                    fields.push_back(component(old.name, t));
                }
                
                if (change) {
                    auto result = ast::create<ast::structure_type>(fields);
                    result->declaration_ = declaration_;
                    result->mutability = mutability;
                    return result;
                }
                
                return before;
            }
        private:
            components fields_;
        };

        class variant_type : public type {
        public:
            variant_type(const types& types) : type(), types_(types) {}
            ~variant_type() {}
            const ast::types& types() const { return types_; }
            void types(ast::types value) { types_ = value; }
            bool contains(ast::pointer<ast::type> subtype) const;
            std::string string(bool absolute = true) const
            {
                if (declaration_) {
                    if (absolute) return prefix() + dynamic_cast<const ast::variant_declaration*>(declaration_)->name().lexeme().string();
                    else return dynamic_cast<const ast::variant_declaration*>(declaration_)->name().lexeme().string();
                }
                else {
                    std::string str = types_.front()->string();
                    for (std::size_t i = 1; i < types_.size(); ++i) str += " | " + types_.at(i)->string();
                    return str;
                }
            }
            enum category category() const { return category::variant_type; }
            ast::pointer<ast::type> substitute(ast::pointer<ast::type> before, std::unordered_map<const ast::declaration*, impl::parameter> map) const
            {
                bool change = false;
                ast::types ts;
                for (auto old : types_) {
                    auto t = old->substitute(old, map);
                    if (old != t) change = true;
                    ts.push_back(t);
                }
                if (change) {
                    auto result = ast::create<ast::variant_type>(ts);
                    result->declaration_ = declaration_;
                    result->mutability = mutability;
                    return result;
                }
                return before;
            }
        private:
            ast::types types_;
        };

        class behaviour_type : public type {
        public:
            behaviour_type() : type() {}
            ~behaviour_type() {}
            std::string string(bool absolute = true) const 
            { 
                if (absolute) return prefix() + dynamic_cast<const behaviour_declaration*>(declaration_)->name().lexeme().string();
                else return dynamic_cast<const behaviour_declaration*>(declaration_)->name().lexeme().string();
            }
            enum category category() const { return category::behaviour_type; }
            void implements(ast::pointer<ast::type> type);
            bool implementor(ast::pointer<ast::type> type) const;
        private:
            std::set<ast::pointer<ast::type>> implementors;
        };
    }

    class types {
    public:
        using parameter = impl::parameter;

        struct parametrized_type_info {
            ast::pointer<ast::type> base;
            std::unordered_map<std::string, parameter> arguments;
        };
        
        types() = delete;
        static ast::pointer<ast::type> builtin(std::string name);
        static ast::pointer<ast::unknown_type> unknown();
        static ast::pointer<ast::workspace_type> workspace();
        static ast::pointer<ast::generic_type> generic();
        static ast::pointer<ast::integer_type> bitfield(unsigned bits);
        static ast::pointer<ast::tuple_type> unit();
        static ast::pointer<ast::bool_type> boolean();
        static ast::pointer<ast::char_type> character();
        static ast::pointer<ast::chars_type> chars();
        static ast::pointer<ast::string_type> string();
        static ast::pointer<ast::integer_type> isize();
        static ast::pointer<ast::integer_type> usize();
        static ast::pointer<ast::integer_type> sint(unsigned bits = 32);
        static ast::pointer<ast::integer_type> uint(unsigned bits = 32);
        static ast::pointer<ast::rational_type> rational(unsigned bits = 64);
        static ast::pointer<ast::float_type> floating(unsigned bits = 32);
        static ast::pointer<ast::complex_type> complex(unsigned bits = 64);
        static ast::pointer<ast::tuple_type> tuple(ast::types types);
        static ast::pointer<ast::array_type> array(ast::pointer<ast::type> base, unsigned size);
        static ast::pointer<ast::slice_type> slice(ast::pointer<ast::type> base);
        static ast::pointer<ast::pointer_type> pointer(ast::pointer<ast::type> base);
        static ast::pointer<ast::function_type> function(ast::types formals, ast::pointer<ast::type> result);
        static ast::pointer<ast::range_type> range(ast::pointer<ast::type> base, bool open);
        static ast::pointer<ast::structure_type> record(ast::structure_type::components fields);
        static ast::pointer<ast::variant_type> variant(ast::types types);
        static ast::pointer<ast::behaviour_type> behaviour();

        static const std::unordered_map<ast::pointer<ast::type>, std::set<ast::pointer<ast::type>>>& implementors() { return implementors_; }
        static const std::unordered_map<ast::pointer<ast::type>, std::set<const ast::declaration*>>& extenders() { return extenders_; }
        static const std::unordered_map<ast::pointer<ast::type>, parametrized_type_info>& parametrized() { return parametrized_; } 
        static const std::unordered_map<ast::pointer<ast::type>, ast::pointer<ast::generic_clause_declaration>>& parametrics() { return parametrics_; }
        static void implements(ast::pointer<ast::type> implementor, ast::pointer<ast::type> behaviour);
        static void extends(const ast::declaration* extender, ast::pointer<ast::type> type);
        // type `instantiated` is instantiated from parametric type `base` with substitution of generic parameters with concrete `arguments`
        static void parametrized(ast::pointer<ast::type> instantiated, ast::pointer<ast::type> base, std::unordered_map<std::string, parameter> arguments) { parametrized_.emplace(instantiated, parametrized_type_info { base, arguments }); }
        // type `base` depends of generic parameters list `parameters`
        static void parametric(ast::pointer<ast::type> base, ast::pointer<ast::generic_clause_declaration> parameters) { parametrics_.emplace(base, parameters); }

        static bool compatible(ast::pointer<ast::type> left, ast::pointer<ast::type> right, bool strict = true);
        static bool assignment_compatible(ast::pointer<ast::type> left, ast::pointer<ast::type> right);
    private:
        static ast::types others_;
        static std::unordered_map<ast::pointer<ast::type>, std::set<const ast::declaration*>> extenders_;
        static std::unordered_map<ast::pointer<ast::type>, std::set<ast::pointer<ast::type>>> implementors_;
        static std::unordered_map<ast::pointer<ast::type>, parametrized_type_info> parametrized_;
        static std::unordered_map<ast::pointer<ast::type>, ast::pointer<ast::generic_clause_declaration>> parametrics_;
    };
}

#endif // TYPE_HPP