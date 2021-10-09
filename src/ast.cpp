#include "nemesis/diagnostics/diagnostic.hpp"
#include "nemesis/parser/ast.hpp"

namespace nemesis {
    namespace ast {
        pointers<expression> clone(const pointers<expression>& source)
        {
            pointers<expression> result;
            for (auto expr : source) result.push_back(expr->clone());
            return result;
        }
        
        pointers<statement> clone(const pointers<statement>& source)
        {
            pointers<statement> result;
            for (auto stmt : source) result.push_back(stmt->sclone());
            return result;
        }
        
        pointers<declaration> clone(const pointers<declaration>& source)
        {
            pointers<declaration> result;
            for (auto decl : source) result.push_back(decl->clone());
            return result;
        }

        std::string path_to_string(const ast::path& p)
        {
            if (p.empty()) throw std::invalid_argument("impl::path_to_string: empty path to convert to string");

            std::ostringstream oss;

            oss << p.front().lexeme();

            for (std::size_t i = 1; i < p.size(); ++i) oss << '.' << p.at(i).lexeme(); 

            return oss.str();
        }

        source_range range_of_path(const ast::path& p) 
        {
            return source_range(p.front().location(), p.back().range().end());
        }
        
        node::node(source_range range) : range_(range), invalid_(0) {}

        node::~node() {}

        bool node::invalid() const { return invalid_; }

        void node::invalid(bool err) const { invalid_ = err; }

        source_range& node::range() const { return range_; }

        expression::expression(source_range range) : node(range) {}

        expression::~expression() {}

        statement::statement(source_range range) : node(range) {}

        statement::~statement() {}

        null_statement::null_statement(source_range range) : statement(range) {}
             
        null_statement::~null_statement() {}
            
        void null_statement::accept(visitor& visitor) const { visitor.visit(*this); }

        expression_statement::expression_statement(source_range range, pointer<ast::expression> expr) :
            statement(range),
            expr_(expr)
        {}
             
        expression_statement::~expression_statement() {}
            
        pointer<ast::expression>& expression_statement::expression() const { return expr_; }
            
        void expression_statement::accept(visitor& visitor) const { visitor.visit(*this); }

        assignment_statement::assignment_statement(source_range range, token op, pointer<ast::expression> left, pointer<ast::expression> right) :
            statement(range),
            op_(op),
            left_(left),
            right_(right)
        {}
            
        assignment_statement::~assignment_statement() {}
            
        token& assignment_statement::assignment_operator() const { return op_; }
            
        pointer<ast::expression>& assignment_statement::left() const { return left_; }
            
        pointer<ast::expression>& assignment_statement::right() const { return right_; }
            
        void assignment_statement::accept(visitor& visitor) const { visitor.visit(*this); }

        later_statement::later_statement(source_range range, pointer<ast::expression> expr) :
            ast::statement(range),
            expr_(expr)
        {}
            
        later_statement::~later_statement() {}
            
        pointer<ast::expression>& later_statement::expression() const { return expr_; }
            
        void later_statement::accept(visitor& visitor) const { visitor.visit(*this); }
        
        return_statement::return_statement(source_range range, pointer<ast::expression> expr) :
            ast::statement(range),
            expr_(expr)
        {}
            
        return_statement::~return_statement() {}
            
        pointer<ast::expression>& return_statement::expression() const { return expr_; }
            
        void return_statement::accept(visitor& visitor) const { visitor.visit(*this); }

        break_statement::break_statement(source_range range, pointer<ast::expression> expr) :
            ast::statement(range),
            expr_(expr)
        {}
            
        break_statement::~break_statement() {}
            
        pointer<ast::expression>& break_statement::expression() const { return expr_; }
            
        void break_statement::accept(visitor& visitor) const { visitor.visit(*this); }

        continue_statement::continue_statement(source_range range) :
            ast::statement(range)
        {}
            
        continue_statement::~continue_statement() {}
            
        void continue_statement::accept(visitor& visitor) const { visitor.visit(*this); }

        test_declaration::test_declaration(source_range range, token name, pointer<ast::expression> body) :
            declaration(range),
            name_(name),
            body_(body)
        {}
            
        test_declaration::~test_declaration() {}
            
        token& test_declaration::name() const { return name_; }
            
        pointer<ast::expression>& test_declaration::body() const { return body_; }
            
        void test_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        contract_statement::contract_statement(source_range range, token specifier, pointer<ast::expression> expr) :
            statement(range),
            specifier_(specifier),
            condition_(expr)
        {}

        contract_statement::~contract_statement() {}

        token& contract_statement::specifier() const { return specifier_; }
        
        pointer<ast::expression>& contract_statement::condition() const { return condition_; }
        
        bool contract_statement::is_require() const { return specifier_.is(token::kind::require_kw); }
       
        bool contract_statement::is_ensure() const { return specifier_.is(token::kind::ensure_kw); }
        
        bool contract_statement::is_invariant() const { return specifier_.is(token::kind::invariant_kw); }

        void contract_statement::accept(visitor& visitor) const { visitor.visit(*this); }

        declaration::declaration(source_range range) : statement(range), hidden_(false) {}

        declaration::~declaration() {}

        void declaration::hidden(bool flag) { hidden_ = flag; }
            
        bool declaration::is_hidden() const { return hidden_; }

        type_declaration::type_declaration(source_range range, token name, pointer<ast::declaration> generic) :
            declaration(range),
            name_(name),
            generic_(generic)
        {}
        
        type_declaration::~type_declaration() {}
        
        token& type_declaration::name() const { return name_; }
        
        pointer<ast::declaration> type_declaration::generic() const { return generic_; }

        void type_declaration::generic(pointer<ast::declaration> clause) { generic_ = clause; }

        type_expression::type_expression(source_range range) : expression(range), mutable_(false) {}

        type_expression::~type_expression() {}

        bit_field_type_expression::bit_field_type_expression(token value) :
            type_expression(source_range(value.location(), value.lexeme().width())),
            size_{value}
        {}

        bit_field_type_expression::~bit_field_type_expression() {}

        token& bit_field_type_expression::size() const { return size_; }

        bool bit_field_type_expression::is_ambiguous() const { return true; }

        pointer<expression> bit_field_type_expression::as_expression() const
        {
            return create<literal_expression>(size_);
        }

        bool bit_field_type_expression::is_path() const { return false; }

        bool bit_field_type_expression::is_assignable() const { return false; }

        void bit_field_type_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        path_type_expression::path_type_expression(source_range range, pointer<ast::expression> expr, pointer<ast::expression> member) :
            type_expression(range),
            expr_(expr),
            member_(member)
        {}
        
        path_type_expression::~path_type_expression() {}
        
        pointer<ast::expression>& path_type_expression::expression() const { return expr_; }
        
        pointer<ast::expression>& path_type_expression::member() const { return member_; }
        
        bool path_type_expression::is_path() const { return true; }
        
        bool path_type_expression::is_assignable() const { return false; }
        
        bool path_type_expression::is_ambiguous() const 
        {
            if (auto left = std::dynamic_pointer_cast<ast::identifier_expression>(expr_)) {
                for (auto generic : left->generics()) {
                    if (auto targ = std::dynamic_pointer_cast<ast::type_expression>(generic)) {
                        if (!targ->is_ambiguous()) return false;
                    }
                }
            }

            if (auto right = std::dynamic_pointer_cast<ast::identifier_expression>(expr_)) {
                for (auto generic : right->generics()) {
                    if (auto targ = std::dynamic_pointer_cast<ast::type_expression>(generic)) {
                        if (!targ->is_ambiguous()) return false;
                    }
                }
            }

            return true;
        }

        pointer<ast::expression> path_type_expression::as_expression() const
        {
            if (!is_ambiguous()) return nullptr;

            auto result = expr_;

            if (auto member = std::dynamic_pointer_cast<ast::identifier_expression>(member_)) {
                if (auto texpr = std::dynamic_pointer_cast<ast::type_expression>(expr_)) {
                    result = create<member_expression>(source_range(range_.begin(), member->identifier().range().end()), texpr->as_expression(), member);
                }
                else {
                    result = create<member_expression>(source_range(range_.begin(), member->identifier().range().end()), expr_, member);
                }

                if (!member->generics().empty()) {
                    ast::pointers<ast::expression> args;
                    for (std::size_t i = 1; i < member->generics().size(); ++i) {
                        if (auto texpr = std::dynamic_pointer_cast<type_expression>(member->generics().at(i))) {
                            args.push_back(texpr->as_expression());
                        }
                        else args.push_back(member->generics().at(i));
                    }
                    
                    result = create<call_expression>(range_, result, args);
                }
            }
            
            return result;
        }

        void path_type_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        array_type_expression::array_type_expression(source_range range, pointer<ast::expression> element_ty, pointer<ast::expression> size) :
            type_expression(range),
            element_type_(element_ty),
            size_(size)
        {}
            
        array_type_expression::~array_type_expression() {}

        pointer<ast::expression>& array_type_expression::element_type() const { return element_type_; }
        
        pointer<ast::expression>& array_type_expression::size() const { return size_; }
        
        bool array_type_expression::is_sized() const { return size_ != nullptr; }

        bool array_type_expression::is_ambiguous() const { return dynamic_cast<const type_expression*>(element_type_.get())->is_ambiguous(); }

        pointer<expression> array_type_expression::as_expression() const
        {
            if (!is_ambiguous()) return nullptr;

            auto elem = std::dynamic_pointer_cast<type_expression>(element_type_)->as_expression();

            if (size_) return create<array_sized_expression>(range_, elem, size_);

            return create<array_expression>(range_, pointers<expression> { elem });
        }

        bool array_type_expression::is_path() const { return false; }

        bool array_type_expression::is_assignable() const { return false; }

        void array_type_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        tuple_type_expression::tuple_type_expression(source_range range, pointers<ast::expression> types) :
            type_expression(range),
            types_(types)
        {}
            
        tuple_type_expression::~tuple_type_expression() {}
            
        pointers<ast::expression>& tuple_type_expression::types() const { return types_; }

        bool tuple_type_expression::is_ambiguous() const
        {
            for (pointer<ast::expression> ty : types_) {
                if (!dynamic_cast<const type_expression*>(ty.get())->is_ambiguous()) return false;
            }

            return true;
        }

        pointer<expression> tuple_type_expression::as_expression() const
        {
            if (!is_ambiguous()) return nullptr;

            if (types_.size() != 1) {
                pointers<expression> elements;
                for (auto t : types_) elements.push_back(std::dynamic_pointer_cast<type_expression>(t)->as_expression());
                return create<tuple_expression>(range_, elements);
            }

            return create<parenthesis_expression>(range_, std::dynamic_pointer_cast<type_expression>(types_.front())->as_expression());
        }

        bool tuple_type_expression::is_path() const { return false; }

        bool tuple_type_expression::is_assignable() const { return false; }

        void tuple_type_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        record_type_expression::record_type_expression(source_range range, pointers<ast::declaration> fields) :
            type_expression(range),
            fields_(fields)
        {}
        
        record_type_expression::~record_type_expression() {}
        
        bool record_type_expression::is_ambiguous() const
        {
            for (pointer<ast::declaration> decl : fields_) {
                auto field = dynamic_cast<const field_declaration*>(decl.get());
                auto ty = dynamic_cast<const type_expression*>(field->type_expression().get());
                if (!ty->is_ambiguous()) return false;
            }

            return true;
        }

        pointer<expression> record_type_expression::as_expression() const
        {
            if (!is_ambiguous()) return nullptr;

            std::vector<record_expression::initializer> fields;

            for (pointer<ast::declaration> decl : fields_) {
                auto field = static_cast<const field_declaration*>(decl.get());
                auto ty = static_cast<const type_expression*>(field->type_expression().get());
                fields.emplace_back(field->name(), ty->as_expression());
            }

            return create<record_expression>(range_, nullptr, fields);
        }
        
        bool record_type_expression::is_path() const { return false; }
        
        bool record_type_expression::is_assignable() const { return false; }
        
        pointers<ast::declaration>& record_type_expression::fields() const { return fields_; }
        
        void record_type_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        function_type_expression::function_type_expression(source_range range, pointers<ast::expression> parameter_ty, pointer<ast::expression> return_ty) :
            type_expression(range),
            parameter_types_(parameter_ty),
            return_type_(return_ty)
        {}
            
        function_type_expression::~function_type_expression() {}
            
        pointers<ast::expression>& function_type_expression::parameter_types() const { return parameter_types_; }
            
        pointer<ast::expression>& function_type_expression::return_type_expression() const { return return_type_; }

        bool function_type_expression::is_ambiguous() const { return false; }

        pointer<expression> function_type_expression::as_expression() const { return nullptr; }

        bool function_type_expression::is_path() const { return false; }

        bool function_type_expression::is_assignable() const { return false; }

        void function_type_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        pointer_type_expression::pointer_type_expression(source_range range, pointer<ast::expression> pointee_ty) :
            type_expression(range),
            pointee_type_(pointee_ty)
        {}
            
        pointer_type_expression::~pointer_type_expression() {}
            
        pointer<ast::expression>& pointer_type_expression::pointee_type() const { return pointee_type_; }

        bool pointer_type_expression::is_ambiguous() const { return static_cast<const type_expression*>(pointee_type_.get())->is_ambiguous(); }

        pointer<expression> pointer_type_expression::as_expression() const
        {
            if (!is_ambiguous()) return nullptr;

            token star = token(token::kind::star, utf8::span::builder().concat("*").build(), source_location());

            return create<unary_expression>(range_, star, static_cast<const type_expression*>(pointee_type_.get())->as_expression());
        }

        bool pointer_type_expression::is_path() const { return false; }

        bool pointer_type_expression::is_assignable() const { return false; }

        void pointer_type_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        variant_type_expression::variant_type_expression(source_range range, pointers<ast::expression> types) :
            type_expression(range),
            types_(types)
        {}
        
        variant_type_expression::~variant_type_expression() {}
        
        pointers<ast::expression>& variant_type_expression::types() const { return types_; }
        
        bool variant_type_expression::is_ambiguous() const
        {
            for (pointer<ast::expression> ty : types_) {
                if (!static_cast<const type_expression*>(ty.get())->is_ambiguous()) return false;
            }

            return true;
        }

        pointer<expression> variant_type_expression::as_expression() const
        {
            if (!is_ambiguous()) return nullptr;

            pointer<expression> expr = static_cast<const type_expression*>(types_.front().get())->as_expression();
            token line = token(token::kind::line, utf8::span::builder().concat("|").build(), source_location());

            for (std::size_t i = 1; i < types_.size(); ++i) {
                auto right = static_cast<const type_expression*>(types_.at(i).get())->as_expression();
                expr = create<binary_expression>(source_range(range_.begin(), types_.at(i)->range().end()), line, expr, right);
            }

            return expr;
        }
        
        bool variant_type_expression::is_path() const { return false; }
        
        bool variant_type_expression::is_assignable() const { return false; }
        
        void variant_type_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        literal_expression::literal_expression(token value) :
            expression(source_range(value.location(), value.lexeme().width())),
            value_(value)
        {}
        
        literal_expression::~literal_expression() {}
        
        token& literal_expression::value() const { return value_; }

        bool literal_expression::is_boolean() const 
        { 
            return value_.is(token::kind::false_kw) || value_.is(token::kind::true_kw);
        }

        bool literal_expression::is_character() const { return value_.is(token::kind::char_literal); }

        bool literal_expression::is_string_primitive() const 
        { 
            if (value_.is(token::kind::string_literal) && value_.valid) {
                return value_.lexeme().cdata()[value_.lexeme().size() - 1] == '\"';
            }

            return false;
        }

        bool literal_expression::is_string() const { return value_.is(token::kind::string_literal); }

        bool literal_expression::is_integer() const { return value_.is(token::kind::integer_literal); }

        bool literal_expression::is_real() const { return value_.is(token::kind::real_literal); }

        bool literal_expression::is_imaginary() const { return value_.is(token::kind::imag_literal); }

        bool literal_expression::is_path() const { return false; }

        bool literal_expression::is_assignable() const { return false; }

        void literal_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        identifier_expression::identifier_expression(source_range range, token value, pointers<ast::expression> generics, bool is_generic) :
            expression(range),
            value_(value),
            generics_(generics),
            generic_(is_generic)
        {}
        
        identifier_expression::~identifier_expression() {}
        
        token& identifier_expression::identifier() const { return value_; }

        bool identifier_expression::is_underscore() const { return value_.lexeme().compare(utf8::span("_")) == 0; }

        pointers<ast::expression>& identifier_expression::generics() const { return generics_; }

        bool identifier_expression::is_generic() const { return generic_; }

        bool identifier_expression::is_path() const { return true; }

        bool identifier_expression::is_assignable() const { return true; }

        void identifier_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        tuple_expression::tuple_expression(source_range range, pointers<ast::expression> elements) : 
            expression(range),
            elements_(elements) 
        {}

        tuple_expression::~tuple_expression() {}
            
        pointers<ast::expression>& tuple_expression::elements() const { return elements_; }

        bool tuple_expression::is_path() const { return false; }

        bool tuple_expression::is_assignable() const
        {
            for (pointer<expression> elem : elements_) {
                if (!elem->is_assignable()) {
                    return false;
                }
            }

            return true;
        }
            
        void tuple_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        array_expression::array_expression(source_range range, pointers<ast::expression> elements) : 
            expression(range),
            elements_(elements)
        {}

        array_expression::~array_expression() {}
            
        pointers<ast::expression>& array_expression::elements() const { return elements_; }

        bool array_expression::is_path() const { return false; }

        bool array_expression::is_assignable() const { return false; }
            
        void array_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        array_sized_expression::array_sized_expression(source_range range, pointer<ast::expression> value, pointer<ast::expression> size) :
            expression(range),
            value_(value),
            size_(size)
        {}
        
        array_sized_expression::~array_sized_expression() {}
        
        pointer<ast::expression>& array_sized_expression::value() const { return value_; }
        
        pointer<ast::expression>& array_sized_expression::size() const { return size_; }

        bool array_sized_expression::is_path() const { return false; }

        bool array_sized_expression::is_assignable() const { return false; }
        
        void array_sized_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        parenthesis_expression::parenthesis_expression(source_range range, pointer<ast::expression> expr) :
            ast::expression(range),
            expr_(expr)
        {}
            
        parenthesis_expression::~parenthesis_expression() {}
        
        pointer<ast::expression>& parenthesis_expression::expression() const { return expr_; }

        bool parenthesis_expression::is_path() const { return false; }

        bool parenthesis_expression::is_assignable() const { return false; }
        
        void parenthesis_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        block_expression::block_expression(source_range range, pointers<ast::statement> stmts) :
            expression(range),
            statements_(stmts)
        {}
        
        block_expression::~block_expression() {}

        bool block_expression::is_path() const { return false; }

        bool block_expression::is_assignable() const { return false; }

        pointers<ast::statement>& block_expression::statements() const { return statements_; }

        void block_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        function_expression::function_expression(source_range range, pointers<ast::declaration> parameters, pointer<ast::expression> return_type_expression, pointer<ast::expression> body) :
            expression(range),
            parameters_(parameters),
            return_type_(return_type_expression),
            body_(body)
        {}

        function_expression::~function_expression() {}

        pointers<ast::declaration>& function_expression::parameters() const { return parameters_; }

        pointer<ast::expression>& function_expression::return_type_expression() const { return return_type_; }

        pointer<ast::expression>& function_expression::body() const { return body_; }

        bool function_expression::is_path() const { return false; }

        bool function_expression::is_assignable() const { return false; }
        
        void function_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        postfix_expression::postfix_expression(source_range range, pointer<ast::expression> expr, token postfix) :
            ast::expression(range),
            expr_(expr),
            postfix_(postfix)
        {}

        postfix_expression::~postfix_expression() {}

        pointer<ast::expression>& postfix_expression::expression() const { return expr_; }
        
        token& postfix_expression::postfix() const { return postfix_; }

        bool postfix_expression::is_path() const { return false; }

        bool postfix_expression::is_assignable() const { return false; }
        
        void postfix_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        call_expression::call_expression(source_range range, pointer<ast::expression> callee, pointers<ast::expression> args) :
            ast::expression(range),
            callee_(callee),
            args_(args)
        {}
        
        call_expression::~call_expression() {}
        
        pointer<ast::expression>& call_expression::callee() const { return callee_; }
        
        pointers<ast::expression>& call_expression::arguments() const { return args_; }

        bool call_expression::is_method_call() const { return dynamic_cast<const ast::member_expression*>(callee_.get()) != nullptr; }

        bool call_expression::is_path() const { return false; }

        bool call_expression::is_assignable() const { return true; }
        
        void call_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        member_expression::member_expression(source_range range, pointer<ast::expression> expr, pointer<ast::expression> member) :
            ast::expression(range),
            expr_(expr),
            member_(member)
        {}
        
        member_expression::~member_expression() {}
            
        pointer<ast::expression>& member_expression::expression() const { return expr_; }
            
        pointer<ast::expression>& member_expression::member() const { return member_; }

        bool member_expression::is_path() const { return expr_->is_path(); }

        bool member_expression::is_assignable() const { return true; }
            
        void member_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        array_index_expression::array_index_expression(source_range range, pointer<ast::expression> expr, pointer<ast::expression> index) :
            ast::expression(range),
            expr_(expr),
            index_(index)
        {}
        
        array_index_expression::~array_index_expression() {}
            
        pointer<ast::expression>& array_index_expression::expression() const { return expr_; }
            
        pointer<ast::expression>& array_index_expression::index() const { return index_; }

        bool array_index_expression::is_path() const { return false; }

        bool array_index_expression::is_assignable() const { return true; }
        
        void array_index_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        tuple_index_expression::tuple_index_expression(source_range range, pointer<ast::expression> expr, token index) :
            ast::expression(range),
            expr_(expr),
            index_(index)
        {}
            
        tuple_index_expression::~tuple_index_expression() {}
            
        pointer<ast::expression>& tuple_index_expression::expression() const { return expr_; }
            
        token& tuple_index_expression::index() const { return index_; }
        
        bool tuple_index_expression::is_path() const { return false; }

        bool tuple_index_expression::is_assignable() const { return true; }

        void tuple_index_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        record_expression::initializer::initializer(token field, pointer<ast::expression> init) :
            field_(field),
            init_(init)
        {}
                
        record_expression::initializer::~initializer() {}
                
        token& record_expression::initializer::field() const { return field_; }
                
        pointer<ast::expression>& record_expression::initializer::value() const { return init_; }

        record_expression::record_expression(source_range range, pointer<ast::expression> callee, std::vector<record_expression::initializer> initializers) : 
            ast::expression(range),
            callee_(callee),
            inits_(initializers)
        {}
            
        record_expression::~record_expression() {}

        bool record_expression::is_anonymous() const { return !callee_; }

        pointer<ast::expression>& record_expression::callee() const { return callee_; }
            
        std::vector<record_expression::initializer>& record_expression::initializers() const { return inits_; }

        bool record_expression::is_path() const { return false; }

        bool record_expression::is_assignable() const { return false; }
            
        void record_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        unary_expression::unary_expression(source_range range, token op, pointer<ast::expression> expr) :
            ast::expression(range),
            op_(op),
            expr_(expr)
        {}
        
        unary_expression::~unary_expression() {}
            
        token& unary_expression::unary_operator() const { return op_; }
        
        pointer<ast::expression>& unary_expression::expression() const { return expr_; }

        bool unary_expression::is_path() const { return false; }

        bool unary_expression::is_assignable() const { return op_.is(token::kind::star); }
        
        void unary_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        implicit_conversion_expression::implicit_conversion_expression(source_range range, pointer<ast::expression> expr) :
            ast::expression(range),
            expr_(expr)
        {}
        
        implicit_conversion_expression::~implicit_conversion_expression() {}
        
        pointer<ast::expression>& implicit_conversion_expression::expression() const { return expr_; }
        
        bool implicit_conversion_expression::is_path() const { return false; }
       
        bool implicit_conversion_expression::is_assignable() const { return false; }
        
        void implicit_conversion_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        binary_expression::binary_expression(source_range range, token op, pointer<ast::expression> left, pointer<ast::expression> right) :
            ast::expression(range),
            op_(op),
            left_(left),
            right_(right)
        {}
        
        binary_expression::~binary_expression() {}
            
        token& binary_expression::binary_operator() const { return op_; }
            
        pointer<ast::expression>& binary_expression::left() const { return left_; }
            
        pointer<ast::expression>& binary_expression::right() const { return right_; }

        bool binary_expression::is_conversion() const { return op_.is(token::kind::as_kw); }

        bool binary_expression::is_path() const { return false; }

        bool binary_expression::is_assignable() const { return false; }
            
        void binary_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        range_expression::range_expression(source_range range, token op, pointer<ast::expression> start, pointer<ast::expression> end) :
            ast::expression(range),
            op_(op),
            start_(start),
            end_(end)
        {}
        
        range_expression::~range_expression() {}
            
        token& range_expression::range_operator() const { return op_; }
            
        pointer<ast::expression>& range_expression::start() const { return start_; }
            
        pointer<ast::expression>& range_expression::end() const { return end_; }

        bool range_expression::is_inclusive() const { return op_.is(token::kind::dot_dot_equal); }

        bool range_expression::is_path() const { return false; }

        bool range_expression::is_assignable() const { return false; }
            
        void range_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        pattern_expression::pattern_expression(source_range range) : ast::expression(range) {}

        pattern_expression::~pattern_expression() {}

        path_pattern_expression::path_pattern_expression(ast::pointer<ast::expression> path) : 
            pattern_expression(path->range()),
            path_(path)
        {}
            
        path_pattern_expression::~path_pattern_expression() {}
            
        bool path_pattern_expression::is_path() const { return true; }

        bool path_pattern_expression::is_assignable() const { return false; }

        bool path_pattern_expression::is_underscore() const 
        {
            if (auto member = std::dynamic_pointer_cast<ast::identifier_expression>(std::static_pointer_cast<ast::path_type_expression>(path_))) {
                return member->identifier().lexeme().compare(utf8::span("_")) == 0; 
            }

            return false;
        }

        ast::pointer<ast::expression>& path_pattern_expression::path() const { return path_; }
        
        void path_pattern_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        ignore_pattern_expression::ignore_pattern_expression(source_range range) : 
            pattern_expression(range)
        {}
            
        ignore_pattern_expression::~ignore_pattern_expression() {}
            
        bool ignore_pattern_expression::is_path() const { return false; }

        bool ignore_pattern_expression::is_assignable() const { return false; }
        
        void ignore_pattern_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        literal_pattern_expression::literal_pattern_expression(token value) : 
            pattern_expression(value.range()),
            value_(value)
        {}
            
        literal_pattern_expression::~literal_pattern_expression() {}
            
        bool literal_pattern_expression::is_path() const { return false; }

        bool literal_pattern_expression::is_assignable() const { return false; }

        token& literal_pattern_expression::value() const { return value_; }
        
        void literal_pattern_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        tuple_pattern_expression::tuple_pattern_expression(source_range range, pointers<ast::expression> elements) :
            pattern_expression(range),
            elements_(elements)
        {}
            
        tuple_pattern_expression::~tuple_pattern_expression() {}
            
        pointers<ast::expression>& tuple_pattern_expression::elements() const { return elements_; }
            
        bool tuple_pattern_expression::is_path() const { return false; }

        bool tuple_pattern_expression::is_assignable() const { return false; }
            
        void tuple_pattern_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        array_pattern_expression::array_pattern_expression(source_range range, pointers<ast::expression> elements) :
            pattern_expression(range),
            elements_(elements)
        {}
            
        array_pattern_expression::~array_pattern_expression() {}
            
        pointers<ast::expression>& array_pattern_expression::elements() const { return elements_; }
            
        bool array_pattern_expression::is_path() const { return false; }

        bool array_pattern_expression::is_assignable() const { return false; }
            
        void array_pattern_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        record_pattern_expression::record_pattern_expression(source_range range, ast::pointer<ast::expression> path, pointers<ast::expression> fields) :
            pattern_expression(range),
            path_(path),
            fields_(fields)
        {}
            
        record_pattern_expression::~record_pattern_expression() {}
            
        ast::pointer<ast::expression>& record_pattern_expression::path() const { return path_; }
            
        pointers<ast::expression>& record_pattern_expression::fields() const { return fields_; }
            
        bool record_pattern_expression::is_path() const { return false; }

        bool record_pattern_expression::is_assignable() const { return fields_.empty(); }
            
        void record_pattern_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        labeled_record_pattern_expression::labeled_record_pattern_expression(source_range range, ast::pointer<ast::expression> path, std::vector<labeled_record_pattern_expression::initializer> fields) :
            pattern_expression(range),
            path_(path),
            fields_(fields)
        {}
        
        labeled_record_pattern_expression::~labeled_record_pattern_expression() {}
        
        ast::pointer<ast::expression>& labeled_record_pattern_expression::path() const { return path_; }
        
        std::vector<labeled_record_pattern_expression::initializer>& labeled_record_pattern_expression::fields() const { return fields_; }
        
        bool labeled_record_pattern_expression::is_path() const { return false; }
        
        bool labeled_record_pattern_expression::is_assignable() const { return false; }
        
        void labeled_record_pattern_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        range_pattern_expression::range_pattern_expression(source_range range, token op, pointer<ast::expression> start, pointer<ast::expression> end) :
            pattern_expression(range),
            range_operator_(op),
            start_(start),
            end_(end)
        {}
            
        range_pattern_expression::~range_pattern_expression() {}

        token& range_pattern_expression::range_operator() const { return range_operator_; }
            
        pointer<ast::expression>& range_pattern_expression::start() const { return start_; }
            
        pointer<ast::expression>& range_pattern_expression::end() const { return end_; }

        bool range_pattern_expression::is_inclusive() const { return range_operator_.is(token::kind::dot_dot_equal); }
        
        bool range_pattern_expression::is_path() const { return false; }

        bool range_pattern_expression::is_assignable() const { return false; }
            
        void range_pattern_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        or_pattern_expression::or_pattern_expression(source_range range, token op, pointer<ast::expression> left, pointer<ast::expression> right) :
            pattern_expression(range),
            or_operator_(op),
            left_(left),
            right_(right)
        {}
            
        or_pattern_expression::~or_pattern_expression() {}

        token& or_pattern_expression::or_operator() const { return or_operator_; }
            
        pointer<ast::expression>& or_pattern_expression::left() const { return left_; }
            
        pointer<ast::expression>& or_pattern_expression::right() const { return right_; }
            
        bool or_pattern_expression::is_path() const { return false; }

        bool or_pattern_expression::is_assignable() const { return false; }
            
        void or_pattern_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        cast_pattern_expression::cast_pattern_expression(source_range range, pointer<ast::expression> type) :
            pattern_expression(range),
            type_expr_(type)
        {}
        
        cast_pattern_expression::~cast_pattern_expression() {}
        
        pointer<ast::expression>& cast_pattern_expression::type_expression() const { return type_expr_; }
        
        bool cast_pattern_expression::is_path() const { return false; }
        
        bool cast_pattern_expression::is_assignable() const { return false; }
        
        void cast_pattern_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        when_expression::branch::branch(pointer<ast::expression> pattern, pointer<ast::expression> body) :
            pattern_(pattern),
            body_(body)
        {}
                
        when_expression::branch::~branch() {}
                
        pointer<ast::expression>& when_expression::branch::pattern() const { return pattern_; }
                
        pointer<ast::expression>& when_expression::branch::body() const { return body_; }

        when_expression::when_expression(source_range range, pointer<ast::expression> cond, std::vector<when_expression::branch> branches, pointer<ast::expression> else_body) :
            ast::expression(range),
            condition_(cond),
            branches_(branches),
            else_body_(else_body)
        {}
        
        when_expression::~when_expression() {}
        
        pointer<ast::expression>& when_expression::condition() const { return condition_; }
            
        std::vector<when_expression::branch>& when_expression::branches() const { return branches_; }
        
        pointer<ast::expression>& when_expression::else_body() const { return else_body_; }

        bool when_expression::is_path() const { return false; }

        bool when_expression::is_assignable() const { return false; }
            
        void when_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        when_pattern_expression::when_pattern_expression(source_range range, pointer<ast::expression> cond, pointer<ast::expression> pattern, pointer<ast::expression> body, pointer<ast::expression> else_body) :
            ast::expression(range),
            condition_(cond),
            pattern_(pattern),
            body_(body),
            else_body_(else_body)
        {}
        
        when_pattern_expression::~when_pattern_expression() {}
        
        pointer<ast::expression>& when_pattern_expression::condition() const { return condition_; }
        
        pointer<ast::expression>& when_pattern_expression::pattern() const { return pattern_; }
        
        pointer<ast::expression>& when_pattern_expression::body() const { return body_; }
        
        pointer<ast::expression>& when_pattern_expression::else_body() const { return else_body_; }
        
        bool when_pattern_expression::is_path() const { return false; }
        
        bool when_pattern_expression::is_assignable() const { return false; }
        
        void when_pattern_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        when_cast_expression::when_cast_expression(source_range range, pointer<ast::expression> cond, pointer<ast::expression> type_expr, pointer<ast::expression> body, pointer<ast::expression> else_body) :
            ast::expression(range),
            condition_(cond),
            type_expr_(type_expr),
            body_(body),
            else_body_(else_body)
        {}
        
        when_cast_expression::~when_cast_expression() {}
        
        pointer<ast::expression>& when_cast_expression::condition() const { return condition_; }
        
        pointer<ast::expression>& when_cast_expression::type_expression() const { return type_expr_; }
        
        pointer<ast::expression>& when_cast_expression::body() const { return body_; }
        
        pointer<ast::expression>& when_cast_expression::else_body() const { return else_body_; }
        
        bool when_cast_expression::is_path() const { return false; }
        
        bool when_cast_expression::is_assignable() const { return false; }
        
        void when_cast_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        for_range_expression::for_range_expression(source_range range, pointer<ast::declaration> var, pointer<ast::expression> condition, pointer<ast::expression> body, pointer<ast::expression> else_body, pointers<ast::statement> contracts) :
            ast::expression(range),
            var_(var),
            condition_(condition),
            body_(body),
            else_body_(else_body),
            contracts_(contracts)
        {}
            
        for_range_expression::~for_range_expression() {}
            
        pointer<ast::declaration> for_range_expression::variable() const { return var_; }
            
        pointer<ast::expression>& for_range_expression::condition() const { return condition_; }
            
        pointer<ast::expression>& for_range_expression::body() const { return body_; }

        pointer<ast::expression>& for_range_expression::else_body() const { return else_body_; }

        pointers<ast::statement>& for_range_expression::contracts() const { return contracts_; }
            
        bool for_range_expression::is_path() const { return false; }

        bool for_range_expression::is_assignable() const { return false; }
        
        void for_range_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        for_loop_expression::for_loop_expression(source_range range, pointer<ast::expression> condition, pointer<ast::expression> body, pointer<ast::expression> else_body, pointers<ast::statement> contracts) :
            ast::expression(range),
            condition_(condition),
            body_(body),
            else_body_(else_body),
            contracts_(contracts)
        {}
            
        for_loop_expression::~for_loop_expression() {}
            
        pointer<ast::expression>& for_loop_expression::condition() const { return condition_; }
            
        pointer<ast::expression>& for_loop_expression::body() const { return body_; }

        pointer<ast::expression>& for_loop_expression::else_body() const { return else_body_; }

        pointers<ast::statement>& for_loop_expression::contracts() const { return contracts_; }
            
        bool for_loop_expression::is_path() const { return false; }

        bool for_loop_expression::is_assignable() const { return false; }
            
        void for_loop_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        if_expression::if_expression(source_range range, pointer<ast::expression> condition, pointer<ast::expression> body, pointer<ast::expression> else_body) :
            ast::expression(range),
            condition_(condition),
            body_(body),
            else_body_(else_body)
        {}
            
        if_expression::~if_expression() {}
            
        pointer<ast::expression>& if_expression::condition() const { return condition_; }
            
        pointer<ast::expression>& if_expression::body() const { return body_; }

        pointer<ast::expression>& if_expression::else_body() const { return else_body_; }
            
        bool if_expression::is_path() const { return false; }

        bool if_expression::is_assignable() const { return false; }
            
        void if_expression::accept(visitor& visitor) const { visitor.visit(*this); }

        field_declaration::field_declaration(source_range range, token name, pointer<ast::expression> ty_expr) :
            declaration(range),
            name_(name),
            type_expr_(ty_expr)
        {}
            
        field_declaration::~field_declaration() {}

        token& field_declaration::name() const { return name_; }
            
        pointer<ast::expression>& field_declaration::type_expression() const { return type_expr_; }

        void field_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        tuple_field_declaration::tuple_field_declaration(source_range range, unsigned index, pointer<ast::expression> ty_expr) :
            declaration(range),
            index_(index),
            type_expr_(ty_expr)
        {}
            
        tuple_field_declaration::~tuple_field_declaration() {}
            
        unsigned tuple_field_declaration::index() const { return index_; }
            
        pointer<ast::expression>& tuple_field_declaration::type_expression() const { return type_expr_; }

        void tuple_field_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        parameter_declaration::parameter_declaration(source_range range, token name, pointer<ast::expression> ty_expr, bool is_mutable, bool is_variadic) :
            var_declaration(range, {}, name, ty_expr, nullptr),
            variadic_(is_variadic)
        {
            mutable_ = is_mutable;
            automatic_ = true;
            static_ = false;
        }
            
        parameter_declaration::~parameter_declaration() {}

        bool parameter_declaration::is_variadic() const { return variadic_; }
            
        void parameter_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        var_declaration::var_declaration(source_range range, std::vector<token> specifiers, token name, pointer<ast::expression> ty_expr, pointer<ast::expression> value) :
            declaration(range),
            name_(name),
            type_expr_(ty_expr),
            value_(value),
            mutable_(0),
            automatic_(0),
            static_(0)
        {
            for (const token& tok : specifiers) {
                switch (tok.kind()) {
                    case token::kind::mutable_kw:
                        mutable_ = 1;
                        break;
                    case token::kind::static_kw:
                        static_ = 1;
                        break;
                    default:
                        break;
                }
            }

            if (!static_) {
                automatic_ = 1;
            }
        }
            
        var_declaration::~var_declaration() {}
            
        bool var_declaration::is_mutable() const { return mutable_; }
            
        bool var_declaration::is_automatic() const { return automatic_; }
            
        bool var_declaration::is_static() const { return static_; }

        token& var_declaration::name() const { return name_; }

        pointer<ast::expression>& var_declaration::type_expression() const { return type_expr_; }

        pointer<ast::expression>& var_declaration::value() const { return value_; }
            
        void var_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        var_tupled_declaration::var_tupled_declaration(source_range range, std::vector<token> specifiers, std::vector<token> names, pointer<ast::expression> ty_expr, pointer<ast::expression> value) :
            declaration(range),
            names_(names),
            type_expression_(ty_expr),
            value_(value),
            mutable_(0),
            automatic_(0),
            static_(0)
        {
            for (const token& tok : specifiers) {
                switch (tok.kind()) {
                    case token::kind::mutable_kw:
                        mutable_ = 1;
                        break;
                    case token::kind::static_kw:
                        static_ = 1;
                        break;
                    default:
                        break;
                }
            }

            if (!static_) {
                automatic_ = 1;
            }
        }
            
        var_tupled_declaration::~var_tupled_declaration() {}
            
        bool var_tupled_declaration::is_mutable() const { return mutable_; }
            
        bool var_tupled_declaration::is_automatic() const { return automatic_; }
            
        bool var_tupled_declaration::is_static() const { return static_; }
            
        std::vector<token>& var_tupled_declaration::names() const { return names_; }
            
        pointer<ast::expression>& var_tupled_declaration::type_expression() const { return type_expression_; }

        pointer<ast::expression>& var_tupled_declaration::value() const { return value_; }
            
        void var_tupled_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        const_declaration::const_declaration(source_range range, token name, pointer<ast::expression> ty_expr, pointer<ast::expression> value) : 
            declaration(range),
            name_(name),
            type_expr_(ty_expr),
            value_(value) 
        {}
            
        const_declaration::~const_declaration() {}

        token& const_declaration::name() const { return name_; }

        pointer<ast::expression>& const_declaration::type_expression() const { return type_expr_; }

        pointer<ast::expression>& const_declaration::value() const { return value_; }
            
        void const_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        const_tupled_declaration::const_tupled_declaration(source_range range, std::vector<token> names, pointer<ast::expression> ty_expr, pointer<ast::expression> value) :
            declaration(range),
            names_(names),
            type_expression_(ty_expr),
            value_(value)
        {}
            
        const_tupled_declaration::~const_tupled_declaration() {}
            
        std::vector<token>& const_tupled_declaration::names() const { return names_; }
            
        pointer<ast::expression>& const_tupled_declaration::type_expression() const { return type_expression_; }

        pointer<ast::expression>& const_tupled_declaration::value() const { return value_; }
            
        void const_tupled_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        generic_clause_declaration::generic_clause_declaration(source_range range, ast::pointers<ast::declaration> parameters, ast::pointer<expression> constraint) :
            declaration(range),
            parameters_(parameters),
            constraint_(constraint)
        {}
        
        generic_clause_declaration::~generic_clause_declaration() {}
        
        ast::pointers<ast::declaration>& generic_clause_declaration::parameters() const { return parameters_; }
        
        ast::pointer<ast::expression>& generic_clause_declaration::constraint() const { return constraint_; }
        
        void generic_clause_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        generic_const_parameter_declaration::generic_const_parameter_declaration(source_range range, token name, pointer<ast::expression> type_expr) :
            declaration(range),
            name_(name),
            type_expr_(type_expr)
        {}
            
        generic_const_parameter_declaration::~generic_const_parameter_declaration() {}
            
        token& generic_const_parameter_declaration::name() const { return name_; }
            
        pointer<ast::expression>& generic_const_parameter_declaration::type_expression() const { return type_expr_; }
            
        void generic_const_parameter_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        generic_type_parameter_declaration::generic_type_parameter_declaration(source_range range, token name) : type_declaration(range, name, {}) {}
            
        generic_type_parameter_declaration::~generic_type_parameter_declaration() {}
            
        void generic_type_parameter_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        property_declaration::property_declaration(source_range range, token name, pointers<ast::declaration> params, pointer<ast::expression> return_type_expr, pointer<ast::expression> body, pointers<ast::statement> contracts) :
            declaration(range),
            name_(name),
            params_(params),
            return_type_expr_(return_type_expr),
            body_(body),
            contracts_(contracts)
        {}
        
        property_declaration::~property_declaration() {}
            
        token& property_declaration::name() const { return name_; }
            
        pointers<ast::declaration>& property_declaration::parameters() const { return params_; }
            
        pointer<ast::expression>& property_declaration::return_type_expression() const { return return_type_expr_; }
            
        pointer<ast::expression>& property_declaration::body() const { return body_; }

        pointers<ast::statement>& property_declaration::contracts() const { return contracts_; }

        void property_declaration::accept(visitor& visitor) const { visitor.visit(*this); }
        
        function_declaration::function_declaration(source_range range, token name, pointer<ast::declaration> generic, pointers<ast::declaration> params, pointer<ast::expression> return_type_expr, pointer<ast::expression> body, pointers<ast::statement> contracts) :
            declaration(range),
            external(false),
            name_(name),
            generic_(generic),
            params_(params),
            return_type_expr_(return_type_expr),
            body_(body),
            contracts_(contracts)
        {}
        
        function_declaration::~function_declaration() {}
            
        token& function_declaration::name() const { return name_; }
            
        pointer<ast::declaration>& function_declaration::generic() const { return generic_; }
            
        pointers<ast::declaration>& function_declaration::parameters() const { return params_; }
            
        pointer<ast::expression>& function_declaration::return_type_expression() const { return return_type_expr_; }
            
        pointer<ast::expression>& function_declaration::body() const { return body_; }

        pointers<ast::statement>& function_declaration::contracts() const { return contracts_; }
            
        void function_declaration::accept(visitor& visitor) const { visitor.visit(*this); }
        
        concept_declaration::concept_declaration(source_range range, pointer<ast::declaration> generic, token name, pointer<ast::expression> base, pointers<ast::declaration> prototypes) :
            declaration(range),
            generic_(generic),
            name_(name),
            base_(base),
            prototypes_(prototypes)
        {}
        
        concept_declaration::~concept_declaration() {}
            
        pointer<ast::declaration>& concept_declaration::generic() const { return generic_; }
            
        token& concept_declaration::name() const { return name_; }

        pointer<ast::expression>& concept_declaration::base() const { return base_; }    
        
        pointers<ast::declaration>& concept_declaration::prototypes() const { return prototypes_; }
            
        void concept_declaration::accept(visitor& visitor) const { visitor.visit(*this); }
        
        extend_declaration::extend_declaration(source_range range, pointer<ast::declaration> generic, pointer<ast::expression> type_expr, pointers<ast::expression> behaviours, pointers<ast::declaration> declarations) :
            declaration(range),
            generic_(generic),
            type_expr_(type_expr),
            behaviours_(behaviours),
            declarations_(declarations)
        {}
        
        extend_declaration::~extend_declaration() {}
            
        pointer<ast::declaration> extend_declaration::generic() const { return generic_; }
            
        pointer<ast::expression>& extend_declaration::type_expression() const { return type_expr_; }
            
        pointers<ast::expression>& extend_declaration::behaviours() const { return behaviours_; }
            
        pointers<ast::declaration>& extend_declaration::declarations() const { return declarations_; }
            
        void extend_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        behaviour_declaration::behaviour_declaration(source_range range, pointer<ast::declaration> generic, token name, pointers<ast::declaration> declarations) :
            type_declaration(range, name, generic),
            declarations_(declarations)
        {}
        
        behaviour_declaration::~behaviour_declaration() {}
            
        pointers<ast::declaration>& behaviour_declaration::declarations() const { return declarations_; }
            
        void behaviour_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        extern_declaration::extern_declaration(source_range range, pointers<ast::declaration> declarations) :
            declaration(range),
            declarations_(declarations)
        {}
            
        extern_declaration::~extern_declaration() {}
            
        pointers<ast::declaration>& extern_declaration::declarations() const { return declarations_; }
            
        void extern_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        range_declaration::range_declaration(source_range range, token name, pointer<ast::declaration> generic, pointer<ast::expression> constraint) :
            type_declaration(range, name, generic),
            constraint_(constraint)
        {}
        
        range_declaration::~range_declaration() {}
        
        pointer<ast::expression>& range_declaration::constraint() const { return constraint_; }
        
        void range_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        record_declaration::record_declaration(source_range range, token name, pointer<ast::declaration> generic, pointers<ast::declaration> fields, bool is_union) :
            type_declaration(range, name, generic),
            union_(is_union),
            fields_(fields)
        {}
            
        record_declaration::~record_declaration() {}
            
        bool record_declaration::is_struct() const { return !union_; }
            
        bool record_declaration::is_union() const { return union_; }
    
        pointers<ast::declaration>& record_declaration::fields() const { return fields_; }
            
        void record_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        variant_declaration::variant_declaration(source_range range, token name, pointer<ast::declaration> generic, pointers<ast::expression> types) :
            type_declaration(range, name, generic),
            types_(types)
        {}
            
        variant_declaration::~variant_declaration() {}
            
        pointers<ast::expression>& variant_declaration::types() const { return types_; }
            
        void variant_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        alias_declaration::alias_declaration(source_range range, token name, pointer<ast::declaration> generic, pointer<ast::expression> type_expr) :
            type_declaration(range, name, generic),
            type_expr_(type_expr)
        {}
            
        alias_declaration::~alias_declaration() {}
            
        pointer<ast::expression>& alias_declaration::type_expression() const { return type_expr_; }
            
        void alias_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        use_declaration::use_declaration(source_range range, token path) :
            declaration(range),
            path_(path)
        {}
            
        use_declaration::~use_declaration() {}
            
        token& use_declaration::path() const { return path_; }
        
        void use_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        workspace_declaration::workspace_declaration(source_range range, token path) :
            declaration(range),
            path_(path)
        {}
            
        workspace_declaration::~workspace_declaration() {}
        
        token& workspace_declaration::path() const { return path_; }
            
        void workspace_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        source_unit_declaration::source_unit_declaration(source_range range, pointer<ast::statement> workspace, 
                                    pointers<ast::statement> imports,
                                    pointers<ast::statement> statements) :
            declaration(range),
            workspace_(workspace),
            imports_(imports),
            statements_(statements)
        {}
        
        source_unit_declaration::~source_unit_declaration() {}
        
        pointer<ast::statement> source_unit_declaration::workspace() const { return workspace_; }
        
        pointers<ast::statement>& source_unit_declaration::imports() const { return imports_; }
        
        pointers<ast::statement>& source_unit_declaration::statements() const { return statements_; }

        void source_unit_declaration::accept(visitor& visitor) const { visitor.visit(*this); }

        void printer::prefix::top(bool last) { lasts_.back() = last; }

        void printer::prefix::push(bool last) { lasts_.push_back(last); }

        void printer::prefix::pop() { lasts_.pop_back(); }
        
        std::string printer::prefix::str(const ast::node& node) const
        {
            std::ostringstream ss;
            auto end = lasts_.end(), it = lasts_.begin();
            --end;

            for (; it != end; ++it) {
                ss << (*it ? "    " : "│   ");
            }

            ss << (*it ? "└─> " : "├─> ");

            if (node.invalid()) ss << "<invalid> ";
            
            return ss.str();
        }

        std::string printer::prefix::str(const ast::declaration& decl) const
        {
            std::ostringstream ss;
            auto end = lasts_.end(), it = lasts_.begin();
            --end;

            for (; it != end; ++it) {
                ss << (*it ? "    " : "│   ");
            }

            ss << (*it ? "└─> " : "├─> ");

            if (decl.invalid()) ss << "<invalid> ";
            if (decl.is_hidden()) ss << "<hidden> ";
            
            return ss.str();
        }

        void printer::visit(const bit_field_type_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::magenta << "bit_field_type_expression " << impl::color::reset << expr.range().begin() << " `" << impl::color::white << expr.size().lexeme() << impl::color::reset << "`\n";
        }
        
        void printer::visit(const path_type_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::magenta << "path_type_expression " << impl::color::reset << expr.range().begin() << "\n";

            prefix_.push(!expr.member());

            expr.expression()->accept(*this);

            if (expr.member()) {
                prefix_.top(true);
                expr.member()->accept(*this);
            }

            prefix_.pop();
        }
        
        void printer::visit(const array_type_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::magenta << "array_type_expression " << impl::color::reset << expr.range().begin() << "\n";
            
            prefix_.push(!expr.size());
            expr.element_type()->accept(*this);
            prefix_.pop();

            if (expr.size()) {
                prefix_.push(true);
                expr.size()->accept(*this);
                prefix_.pop();
            }
        }
        
        void printer::visit(const tuple_type_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::magenta << "tuple_type_expression " << impl::color::reset << expr.range().begin() << "\n";
            
            if (!expr.types().empty()) {
                prefix_.push(expr.types().size() == 1);

                for (std::size_t ity = 0; ity < expr.types().size() - 1; ++ity) {
                    expr.types().at(ity)->accept(*this);
                }

                prefix_.top(true);
                expr.types().back()->accept(*this);
                prefix_.pop();
            }
        }
        
        void printer::visit(const pointer_type_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::magenta << "pointer_type_expression " << impl::color::reset << expr.range().begin() << "\n";
            
            prefix_.push(true);
            expr.pointee_type()->accept(*this);
            prefix_.pop();
        }
        
        void printer::visit(const function_type_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::magenta << "function_type_expression " << impl::color::reset << expr.range().begin() << "\n";
            
            if (expr.return_type_expression()) {
                prefix_.push(expr.parameter_types().empty());
                expr.return_type_expression()->accept(*this);
                prefix_.pop();
            }

            if (!expr.parameter_types().empty()) {
                prefix_.push(expr.parameter_types().size() == 1);

                for (std::size_t iparam = 0; iparam < expr.parameter_types().size() - 1; ++iparam) {
                    expr.parameter_types().at(iparam)->accept(*this);
                }

                prefix_.top(true);
                expr.parameter_types().back()->accept(*this);
                prefix_.pop();
            }
        }

        void printer::visit(const record_type_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::magenta << "record_type_expression " << impl::color::reset << expr.range().begin() << "\n";
            
            if (!expr.fields().empty()) {
                prefix_.push(expr.fields().size() == 1);

                for (std::size_t ity = 0; ity < expr.fields().size() - 1; ++ity) {
                    expr.fields().at(ity)->accept(*this);
                }

                prefix_.top(true);
                expr.fields().back()->accept(*this);
                prefix_.pop();
            }
        }

        void printer::visit(const variant_type_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::magenta << "variant_type_expression " << impl::color::reset << expr.range().begin() << "\n";
            
            if (!expr.types().empty()) {
                prefix_.push(expr.types().size() == 1);

                for (std::size_t ity = 0; ity < expr.types().size() - 1; ++ity) {
                    expr.types().at(ity)->accept(*this);
                }

                prefix_.top(true);
                expr.types().back()->accept(*this);
                prefix_.pop();
            }
        }
        
        void printer::visit(const literal_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "literal_expression " << impl::color::reset << expr.value().location() << " `" << impl::color::white << expr.value().lexeme() << impl::color::reset << "`\n";
        }

        void printer::visit(const identifier_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "identifier_expression " << impl::color::reset;
            if (expr.is_generic()) stream_ << "<generic> ";
            stream_ << expr.identifier().location() << " `" << impl::color::white << expr.identifier().lexeme() << impl::color::reset << "`";
            if (expr.annotation().value.type) stream_ << " has value " << impl::color::white << expr.annotation().value << impl::color::reset;
            stream_ << '\n';
            
            if (!expr.generics().empty()) {
                prefix_.push(false);

                for (std::size_t iarg = 0; iarg < expr.generics().size() - 1; ++iarg) {
                    expr.generics().at(iarg)->accept(*this);
                }

                prefix_.top(true);

                expr.generics().back()->accept(*this);
                
                prefix_.pop();
            }
        }

        void printer::visit(const tuple_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "tuple_expression " << impl::color::reset << expr.range().begin() << "\n";
            
            if (!expr.elements().empty()) {
                prefix_.push(expr.elements().size() == 1);

                for (std::size_t ielem = 0; ielem < expr.elements().size() - 1; ++ielem) {
                    expr.elements().at(ielem)->accept(*this);
                }

                prefix_.top(true);
                expr.elements().back()->accept(*this);
                prefix_.pop();
            }
        }

        void printer::visit(const array_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "array_expression " << impl::color::reset << expr.range().begin() << "\n";

            if (!expr.elements().empty()) {
                prefix_.push(expr.elements().size() == 1);

                for (std::size_t ielem = 0; ielem < expr.elements().size() - 1; ++ielem) {
                    expr.elements().at(ielem)->accept(*this);
                }

                prefix_.top(true);
                expr.elements().back()->accept(*this);
                prefix_.pop();
            }
        }

        void printer::visit(const array_sized_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "array_sized_expression " << impl::color::reset << expr.range().begin() << "\n";

            prefix_.push(false);
            expr.value()->accept(*this);
            prefix_.top(true);
            expr.size()->accept(*this);
            prefix_.pop();
        }

        void printer::visit(const parenthesis_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "parenthesis_expression " << impl::color::reset << expr.range().begin() << "\n";

            prefix_.push(true);
            expr.expression()->accept(*this);
            prefix_.pop();
        }

        void printer::visit(const block_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "block_expression " << impl::color::reset << expr.range().begin() << "\n";

            if (!expr.statements().empty()) {
                prefix_.push(false);

                for (std::size_t istmt = 0; istmt < expr.statements().size() - 1; ++istmt) {
                    expr.statements().at(istmt)->accept(*this);
                }

                prefix_.top(true);
                expr.statements().back()->accept(*this);
                prefix_.pop();
            }
        }

        void printer::visit(const function_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "function_expression " << impl::color::reset << expr.range().begin() << "\n";

            prefix_.push(false);

            for (pointer<ast::declaration> param : expr.parameters()) {
                param->accept(*this);
            }

            if (expr.return_type_expression()) {
                expr.return_type_expression()->accept(*this);
            }

            prefix_.top(true);

            expr.body()->accept(*this);

            prefix_.pop();
        }

        void printer::visit(const postfix_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "postfix_expression " << impl::color::reset << expr.postfix().location() << " `" << impl::color::white << expr.postfix().lexeme() << impl::color::reset << "`\n";

            prefix_.push(true);
            expr.expression()->accept(*this);
            prefix_.pop();
        }
        
        void printer::visit(const call_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "call_expression " << impl::color::reset;
            if (expr.is_method_call()) stream_ << "<method> ";
            stream_ << expr.range().begin() << "\n";

            prefix_.push(expr.arguments().empty());
            expr.callee()->accept(*this);
            prefix_.top(expr.arguments().size() == 1);

            if (!expr.arguments().empty()) {
                for (std::size_t iarg = 0; iarg < expr.arguments().size() - 1; ++iarg) {
                    expr.arguments().at(iarg)->accept(*this);
                }

                prefix_.top(true);
                expr.arguments().back()->accept(*this);
            }

            prefix_.pop();
        }
        
        void printer::visit(const member_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "member_expression " << impl::color::reset << expr.range().begin() << "\n";
    
            prefix_.push(false);
            expr.expression()->accept(*this);
            prefix_.top(true);
            expr.member()->accept(*this);
            prefix_.pop();
        }
        
        void printer::visit(const array_index_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "array_index_expression " << impl::color::reset << expr.range().begin() << "\n";

            prefix_.push(false);
            expr.expression()->accept(*this);
            prefix_.top(true);
            expr.index()->accept(*this);
            prefix_.pop();
        }
        
        void printer::visit(const tuple_index_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "tuple_index_expression " << impl::color::reset << expr.range().begin() << " `." << impl::color::white << expr.index().lexeme() << impl::color::reset << "`\n";

            prefix_.push(true);
            expr.expression()->accept(*this);
            prefix_.pop();
        }

        void printer::visit(const record_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "record_expression " << impl::color::reset;
            if (expr.is_anonymous()) stream_ << "<anonymous> ";
            stream_ << expr.range().begin() << "\n" << std::flush;

            if (expr.callee()) {
                prefix_.push(expr.initializers().empty());
                expr.callee()->accept(*this);
                prefix_.pop();
            }

            if (!expr.initializers().empty()) {
                prefix_.push(false);

                for (std::size_t ifield = 0; ifield < expr.initializers().size() - 1; ++ifield) {
                    stream_ << prefix_.str(*expr.initializers().at(ifield).value()) << impl::color::yellow << "initializer " << impl::color::reset << expr.initializers().at(ifield).field().location() 
                            << " `." << impl::color::white << expr.initializers().at(ifield).field().lexeme() << impl::color::reset << "`\n";
                    prefix_.push(true);
                    expr.initializers().at(ifield).value()->accept(*this);
                    prefix_.pop();
                }

                prefix_.top(true);
                stream_ << prefix_.str(*expr.initializers().back().value()) << impl::color::yellow << "initializer " << impl::color::reset << expr.initializers().back().field().location() 
                        << " `." << impl::color::white << expr.initializers().back().field().lexeme() << impl::color::reset << "`\n";
                prefix_.push(true);
                expr.initializers().back().value()->accept(*this);
                prefix_.pop();
                
                prefix_.pop();
            }
        }

        void printer::visit(const unary_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "unary_expression " << impl::color::reset << expr.unary_operator().location() << " `" << impl::color::white << expr.unary_operator().lexeme() << impl::color::reset << "`\n";

            prefix_.push(true);
            expr.expression()->accept(*this);
            prefix_.pop();
        }

        void printer::visit(const binary_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "binary_expression " << impl::color::reset;
            if (expr.is_conversion()) stream_ << "<type-conversion> ";
            stream_ << expr.binary_operator().location() << " `" << impl::color::white << expr.binary_operator().lexeme() << impl::color::reset << "`\n";

            prefix_.push(!expr.right());
            if (expr.left()) expr.left()->accept(*this);
            prefix_.top(true);
            if (expr.right()) expr.right()->accept(*this);
            prefix_.pop();
        }

        void printer::visit(const range_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "range_expression " << impl::color::reset << expr.range_operator().location() << " `"<< impl::color::white  << expr.range_operator().lexeme() << impl::color::reset << "`\n";

            prefix_.push(!expr.end());
            if (expr.start()) expr.start()->accept(*this);
            prefix_.top(true);
            if (expr.end()) expr.end()->accept(*this);
            prefix_.pop();
        }

        void printer::visit(const ignore_pattern_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "ignore_pattern_expression " << impl::color::reset << expr.range().begin() << "\n";
        }

        void printer::visit(const literal_pattern_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "literal_pattern_expression " << impl::color::reset << expr.value().location() << " `" << impl::color::white << expr.value().lexeme() << impl::color::reset << "`\n";
        }

        void printer::visit(const path_pattern_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "path_pattern_expression " << impl::color::reset << expr.range().begin() << '\n';

            prefix_.push(true);

            expr.path()->accept(*this);

            prefix_.pop();
        }

        void printer::visit(const tuple_pattern_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "tuple_pattern_expression " << impl::color::reset << expr.range().begin() << "\n";

            if (!expr.elements().empty()) {
                prefix_.push(expr.elements().size() == 1);

                for (std::size_t ielem = 0; ielem < expr.elements().size() - 1; ++ielem) {
                    expr.elements().at(ielem)->accept(*this);
                }

                prefix_.top(true);
                expr.elements().back()->accept(*this);
                prefix_.pop();
            }
        }

        void printer::visit(const array_pattern_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "array_pattern_expression " << impl::color::reset << expr.range().begin() << "\n";

            if (!expr.elements().empty()) {
                prefix_.push(expr.elements().size() == 1);

                for (std::size_t ielem = 0; ielem < expr.elements().size() - 1; ++ielem) {
                    expr.elements().at(ielem)->accept(*this);
                }

                prefix_.top(true);
                expr.elements().back()->accept(*this);
                prefix_.pop();
            }
        }

        void printer::visit(const record_pattern_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "record_pattern_expression " << impl::color::reset << expr.range().begin() << '\n';

            prefix_.push(expr.fields().empty());

            expr.path()->accept(*this);

            if (!expr.fields().empty()) {
                prefix_.top(expr.fields().size() == 1);

                for (std::size_t ifield = 0; ifield < expr.fields().size() - 1; ++ifield) {
                    expr.fields().at(ifield)->accept(*this);
                }

                prefix_.top(true);
                expr.fields().back()->accept(*this);
            }

            prefix_.pop();
        }

        void printer::visit(const labeled_record_pattern_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "labeled_record_pattern_expression " << impl::color::reset << expr.range().begin() << '\n';

            prefix_.push(expr.fields().empty());

            expr.path()->accept(*this);

            if (!expr.fields().empty()) {
                prefix_.top(expr.fields().size() == 1);

                for (std::size_t ifield = 0; ifield < expr.fields().size() - 1; ++ifield) {
                    stream_ << prefix_.str(*expr.fields().at(ifield).value) << impl::color::yellow << "initializer " << impl::color::reset << expr.fields().at(ifield).field.location() << " `" << impl::color::white << expr.fields().at(ifield).field.lexeme() << impl::color::reset << "`\n";
                    prefix_.push(true);
                    expr.fields().at(ifield).value->accept(*this);
                    prefix_.pop();
                }

                stream_ << prefix_.str(*expr.fields().back().value) << impl::color::yellow << "initializer " << impl::color::reset << expr.fields().back().field.location() << " `" << impl::color::white << expr.fields().back().field.lexeme() << impl::color::reset << "`\n";
                prefix_.push(true);
                expr.fields().back().value->accept(*this);
                prefix_.pop();
            }

            prefix_.pop();
        }

        void printer::visit(const range_pattern_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "range_pattern_expression " << impl::color::reset << expr.range_operator().location() << " `" << expr.range_operator().lexeme() << "`\n";

            prefix_.push(!expr.end());
            if (expr.start()) expr.start()->accept(*this);
            prefix_.top(true);
            if (expr.end()) expr.end()->accept(*this);
            prefix_.pop();
        }

        void printer::visit(const or_pattern_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "or_pattern_expression " << impl::color::reset << expr.or_operator().location() << " `" << expr.or_operator().lexeme() << "`\n";

            prefix_.push(false);
            expr.left()->accept(*this);
            prefix_.top(true);
            expr.right()->accept(*this);
            prefix_.pop();
        }

        void printer::visit(const cast_pattern_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "cast_pattern_expression " << impl::color::reset << "\n";

            prefix_.push(true);
            expr.type_expression()->accept(*this);
            prefix_.pop();
        }

        void printer::visit(const when_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "when_expression " << impl::color::reset << expr.range().begin() << "\n";

            prefix_.push(expr.branches().empty() && !expr.else_body());
            expr.condition()->accept(*this);
            prefix_.pop();

            if (!expr.branches().empty()) {
                prefix_.push(expr.branches().size() == 1 && !expr.else_body());

                for (std::size_t ibranch = 0; ibranch < expr.branches().size() - 1; ++ibranch) {
                    stream_ << prefix_.str(*expr.branches().at(ibranch).pattern()) << impl::color::yellow << "branch " << impl::color::reset << expr.branches().at(ibranch).pattern()->range().begin() << "\n";
                    prefix_.push(false);
                    expr.branches().at(ibranch).pattern()->accept(*this);
                    prefix_.top(true);
                    expr.branches().at(ibranch).body()->accept(*this);
                    prefix_.pop();
                }

                prefix_.top(!expr.else_body());
                stream_ << prefix_.str(*expr.branches().back().pattern()) << impl::color::yellow << "branch " << impl::color::reset << expr.branches().back().pattern()->range().begin() << "\n";
                prefix_.push(false);
                expr.branches().back().pattern()->accept(*this);
                prefix_.top(true);
                expr.branches().back().body()->accept(*this);
                prefix_.pop();
                
                prefix_.pop();
            }

            if (expr.else_body()) {
                prefix_.push(true);
                expr.else_body()->accept(*this);
                prefix_.pop();
            }
        }

        void printer::visit(const when_pattern_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "when_pattern_expression " << impl::color::reset << expr.range().begin() << "\n";

            prefix_.push(false);
            expr.condition()->accept(*this);
            expr.pattern()->accept(*this);
            prefix_.top(!expr.else_body());
            expr.body()->accept(*this);
            if (expr.else_body()) {
                prefix_.top(true);
                expr.else_body()->accept(*this);
            }
            prefix_.pop();
        }

        void printer::visit(const when_cast_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "when_cast_expression " << impl::color::reset << expr.range().begin() << "\n";

            prefix_.push(false);
            expr.condition()->accept(*this);
            expr.type_expression()->accept(*this);
            prefix_.top(!expr.else_body());
            expr.body()->accept(*this);
            if (expr.else_body()) {
                prefix_.top(true);
                expr.else_body()->accept(*this);
            }
            prefix_.pop();
        }

        void printer::visit(const for_range_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "for_range_expression " << impl::color::reset << expr.range().begin() << "\n";

            prefix_.push(false);
            for (auto contract : expr.contracts()) contract->accept(*this);
            expr.variable()->accept(*this);
            expr.condition()->accept(*this);
            prefix_.top(!expr.else_body());
            expr.body()->accept(*this);

            if (expr.else_body()) {
                prefix_.top(true);
                expr.else_body()->accept(*this);
            }

            prefix_.pop();
        }

        void printer::visit(const for_loop_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "for_loop_expression " << impl::color::reset << expr.range().begin() << "\n";

            prefix_.push(false);
            for (auto contract : expr.contracts()) contract->accept(*this);
            if (expr.condition()) expr.condition()->accept(*this);
            prefix_.top(!expr.else_body());
            expr.body()->accept(*this);

            if (expr.else_body()) {
                prefix_.top(true);
                expr.else_body()->accept(*this);
            }
            prefix_.pop();
        }

        void printer::visit(const if_expression& expr)
        {
            stream_ << prefix_.str(expr) << impl::color::yellow << "if_expression " << impl::color::reset << expr.range().begin() << "\n";

            prefix_.push(false);
            if (expr.condition()) expr.condition()->accept(*this);
            prefix_.top(!expr.else_body());
            expr.body()->accept(*this);
            if (expr.else_body()) {
                prefix_.top(true);
                expr.else_body()->accept(*this);
            }
            prefix_.pop();
        }

        void printer::visit(const null_statement& stmt)
        {
            stream_ << prefix_.str(stmt) << impl::color::blue << "null_statement " << impl::color::reset << stmt.range().begin() << "\n";
        }

        void printer::visit(const expression_statement& stmt)
        {
            stream_ << prefix_.str(stmt) << impl::color::blue << "expression_statement " << impl::color::reset << stmt.range().begin() << "\n";
            
            if (stmt.expression()) {
                prefix_.push(true);
                stmt.expression()->accept(*this);
                prefix_.pop();
            }
        }

        void printer::visit(const assignment_statement& stmt)
        {
            stream_ << prefix_.str(stmt) << impl::color::blue << "assignment_statement " << impl::color::reset << stmt.range().begin() << " `" << impl::color::white << stmt.assignment_operator().lexeme() << impl::color::reset << "`\n";
            
            prefix_.push(false);
            stmt.left()->accept(*this);
            prefix_.top(true);
            stmt.right()->accept(*this);
            prefix_.pop();
        }

        void printer::visit(const later_statement& stmt)
        {
            stream_ << prefix_.str(stmt) << impl::color::blue << "later_statement " << impl::color::reset << stmt.range().begin() << "\n";
            
            if (stmt.expression()) {
                prefix_.push(true);
                stmt.expression()->accept(*this);
                prefix_.pop();
            }
        }
        
        void printer::visit(const return_statement& stmt)
        {
            stream_ << prefix_.str(stmt) << impl::color::blue << "return_statement " << impl::color::reset << stmt.range().begin() << "\n";
            
            if (stmt.expression()) {
                prefix_.push(true);
                stmt.expression()->accept(*this);
                prefix_.pop();
            }
        }

        void printer::visit(const break_statement& stmt)
        {
            stream_ << prefix_.str(stmt) << impl::color::blue << "break_statement " << impl::color::reset << stmt.range().begin() << "\n";
            
            if (stmt.expression()) {
                prefix_.push(true);
                stmt.expression()->accept(*this);
                prefix_.pop();
            }
        }

        void printer::visit(const continue_statement& stmt)
        {
            stream_ << prefix_.str(stmt) << impl::color::blue << "continue_statement " << impl::color::reset << stmt.range().begin() << "\n";
        }

        void printer::visit(const contract_statement& stmt)
        {
            stream_ << prefix_.str(stmt) << impl::color::blue << "contract_statement " << impl::color::reset << stmt.range().begin() << " `" << impl::color::white << stmt.specifier().lexeme() << impl::color::reset << "`\n";

            prefix_.push(true);
            stmt.condition()->accept(*this);
            prefix_.pop();
        }

        void printer::visit(const field_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "field_declaration " << impl::color::reset << decl.range().begin() << " `." << impl::color::white << decl.name().lexeme() << impl::color::reset << "`\n";

            if (decl.type_expression()) {
                prefix_.push(true);
                decl.type_expression()->accept(*this);
                prefix_.pop();
            }
        }

        void printer::visit(const tuple_field_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "tuple_field_declaration " << impl::color::reset << decl.range().begin() << " ." << decl.index() << "\n";

            if (decl.type_expression()) {
                prefix_.push(true);
                decl.type_expression()->accept(*this);
                prefix_.pop();
            }
        }

        void printer::visit(const parameter_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "parameter_declaration " << impl::color::reset;
            if (decl.is_mutable()) stream_ << "<mutable> ";
            if (decl.is_variadic()) stream_ << "<variadic> ";
            stream_ << decl.range().begin() << " `" << impl::color::white << decl.name().lexeme() << impl::color::reset << "`\n";

            if (decl.type_expression()) {
                prefix_.push(true);
                decl.type_expression()->accept(*this);
                prefix_.pop();
            }
        }

        void printer::visit(const var_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "var_declaration" << impl::color::reset;
            if (decl.is_mutable()) stream_ << " <mutable>";
            if (decl.is_automatic()) stream_ << " <automatic>";
            else if (decl.is_static()) stream_ << " <static>";
            stream_ << " " << decl.range().begin() << " `" << impl::color::white << decl.name().lexeme() << impl::color::reset << "`\n";

            if (decl.type_expression()) {
                prefix_.push(!decl.value());
                decl.type_expression()->accept(*this);
                prefix_.pop();
            }

            if (decl.value()) {
                prefix_.push(true);
                decl.value()->accept(*this);
                prefix_.pop();
            }
        }

        void printer::visit(const var_tupled_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "var_tupled_declaration" << impl::color::reset;
            if (decl.is_mutable()) stream_ << " <mutable>";
            if (decl.is_automatic()) stream_ << " <automatic>";
            else if (decl.is_static()) stream_ << " <static>";
            stream_ << " " << decl.range().begin() << " `" << impl::color::white << decl.names().front().lexeme() << impl::color::reset << "`";
            for (std::size_t ivar = 1; ivar < decl.names().size(); ++ivar) {
                stream_ << ", `" << impl::color::white << decl.names().at(ivar).lexeme() << impl::color::reset << "`";
            }
            stream_ << "\n";

            if (decl.type_expression()) {
                prefix_.push(false);
                decl.type_expression()->accept(*this);
                prefix_.pop();
            }

            prefix_.push(true);
            decl.value()->accept(*this);
            prefix_.pop();
        }

        void printer::visit(const const_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "const_declaration " << impl::color::reset << decl.range().begin() << " `" << impl::color::white << decl.name().lexeme() << impl::color::reset << "`";
            if (decl.value()->annotation().type) stream_ << " has value " << impl::color::white << decl.value()->annotation().value << impl::color::reset;
            stream_ << '\n';

            if (decl.type_expression()) {
                prefix_.push(false);
                decl.type_expression()->accept(*this);
                prefix_.pop();
            }

            prefix_.push(true);
            decl.value()->accept(*this);
            prefix_.pop();
        }

        void printer::visit(const const_tupled_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "const_tupled_declaration " << impl::color::reset;
            stream_ << " " << decl.range().begin() << " `" << impl::color::white << decl.names().front().lexeme() << impl::color::reset << "`";
            for (std::size_t ivar = 1; ivar < decl.names().size(); ++ivar) {
                stream_ << ", `" << impl::color::white << decl.names().at(ivar).lexeme() << impl::color::reset << "`";
            }
            if (decl.value()->annotation().type) stream_ << " has value " << impl::color::white << decl.value()->annotation().value << impl::color::reset;
            stream_ << "\n";

            if (decl.type_expression()) {
                prefix_.push(false);
                decl.type_expression()->accept(*this);
                prefix_.pop();
            }

            prefix_.push(true);
            decl.value()->accept(*this);
            prefix_.pop();
        }

        void printer::visit(const generic_clause_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "generic_clause_declaration " << impl::color::reset << "\n";

            prefix_.push(false);

            if (!decl.parameters().empty()) {
                for (std::size_t iparam = 0; iparam < decl.parameters().size() - 1; ++iparam) {
                    decl.parameters().at(iparam)->accept(*this);
                }

                prefix_.top(!decl.constraint());
                decl.parameters().back()->accept(*this);
            }

            if (decl.constraint()) {
                prefix_.top(true);
                decl.constraint()->accept(*this);
            }

            prefix_.pop();
        }

        void printer::visit(const generic_const_parameter_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "generic_const_parameter_declaration " << impl::color::reset << decl.range().begin() << " `" << impl::color::white << decl.name().lexeme() << impl::color::reset << "`\n";

            prefix_.push(true);
            decl.type_expression()->accept(*this);
            prefix_.pop();
        }

        void printer::visit(const generic_type_parameter_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "generic_type_parameter_declaration " << impl::color::reset << decl.range().begin() << " `" << impl::color::white << decl.name().lexeme() << impl::color::reset << "`\n";
        }

        void printer::visit(const test_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "test_declaration " << impl::color::reset << decl.range().begin() << " `" << impl::color::white << decl.name().lexeme() << impl::color::reset  << "`\n";
            prefix_.push(true);
            decl.body()->accept(*this);
            prefix_.pop();
        }

        void printer::visit(const function_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "function_declaration " << impl::color::reset;
            if (decl.generic()) stream_ << "<generic> ";
            stream_ << decl.range().begin() << " `" << impl::color::white << decl.name().lexeme() << impl::color::reset << "`\n";

            prefix_.push(false);

            if (decl.generic()) {
                prefix_.top(decl.contracts().empty() && decl.parameters().empty() && !decl.return_type_expression() && !decl.body());
                decl.generic()->accept(*this);
            }

            if (!decl.contracts().empty()) {
                for (std::size_t istmt = 0; istmt < decl.contracts().size() - 1; ++istmt) {
                    decl.contracts().at(istmt)->accept(*this);
                }

                prefix_.top(decl.parameters().empty() && !decl.return_type_expression() && !decl.body());
                decl.contracts().back()->accept(*this);
            }

            if (!decl.parameters().empty()) {
                prefix_.top(false);

                for (std::size_t iparam = 0; iparam < decl.parameters().size() - 1; ++iparam) {
                    decl.parameters().at(iparam)->accept(*this);
                }

                prefix_.top(!decl.return_type_expression() && !decl.body());

                decl.parameters().back()->accept(*this);
            }

            if (decl.return_type_expression()) {
                prefix_.top(!decl.body());
                decl.return_type_expression()->accept(*this);
            }

            if (decl.body()) {
                prefix_.top(true);
                decl.body()->accept(*this);
            }

            prefix_.pop();
        }

        void printer::visit(const property_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "property_declaration " << impl::color::reset << decl.range().begin() << " `" << impl::color::white << decl.name().lexeme() << impl::color::reset << "`\n";
            prefix_.push(false);

            if (!decl.contracts().empty()) {
                for (std::size_t istmt = 0; istmt < decl.contracts().size() - 1; ++istmt) {
                    decl.contracts().at(istmt)->accept(*this);
                }

                prefix_.top(decl.parameters().empty() && !decl.return_type_expression() && !decl.body());
                decl.contracts().back()->accept(*this);
            }

            if (!decl.parameters().empty()) {
                prefix_.top(false);

                for (std::size_t iparam = 0; iparam < decl.parameters().size() - 1; ++iparam) {
                    decl.parameters().at(iparam)->accept(*this);
                }

                prefix_.top(!decl.return_type_expression() && !decl.body());

                decl.parameters().back()->accept(*this);
            }

            if (decl.return_type_expression()) {
                prefix_.top(!decl.body());
                decl.return_type_expression()->accept(*this);
            }

            if (decl.body()) {
                prefix_.top(true);
                decl.body()->accept(*this);
            }

            prefix_.pop();
        }

        void printer::visit(const concept_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "concept_declaration " << impl::color::reset << decl.range().begin() << " `" << impl::color::white << decl.name().lexeme() << impl::color::reset << "`\n";

            prefix_.push(false);

            if (decl.generic()) {
                prefix_.top(!decl.base() && decl.prototypes().empty());
                decl.generic()->accept(*this);
            }

            if (decl.base()) {
                prefix_.top(decl.prototypes().empty());
                decl.base()->accept(*this);
            }

            if (!decl.prototypes().empty()) {
                prefix_.top(false);

                for (std::size_t idecl = 0; idecl < decl.prototypes().size() - 1; ++idecl) {
                    decl.prototypes().at(idecl)->accept(*this);
                }

                prefix_.top(true);

                decl.prototypes().back()->accept(*this);
            }

            prefix_.pop();
        }

        void printer::visit(const extend_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "extend_declaration " << impl::color::reset << decl.range().begin() << "\n";

            prefix_.push(false);

            if (decl.generic()) {
                decl.generic()->accept(*this);
            }

            prefix_.top(decl.behaviours().empty() && decl.declarations().empty());

            decl.type_expression()->accept(*this);

            if (!decl.behaviours().empty()) {
                prefix_.top(false);

                for (std::size_t iexpr = 0; iexpr < decl.behaviours().size() - 1; ++iexpr) {
                    decl.behaviours().at(iexpr)->accept(*this);
                }

                prefix_.top(decl.declarations().empty());
                decl.behaviours().back()->accept(*this);
            }
            
            if (!decl.declarations().empty()) {
                prefix_.top(false);

                for (std::size_t idecl = 0; idecl < decl.declarations().size() - 1; ++idecl) {
                    decl.declarations().at(idecl)->accept(*this);
                }

                prefix_.top(true);

                decl.declarations().back()->accept(*this);
            }

            prefix_.pop();
        }

        void printer::visit(const behaviour_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "behaviour_declaration " << impl::color::reset << decl.range().begin() << " `" << impl::color::white <<decl.name().lexeme() << impl::color::reset << "`\n";

            prefix_.push(false);

            if (decl.generic()) {
                prefix_.top(decl.declarations().empty());
                decl.generic()->accept(*this);
            }

            if (!decl.declarations().empty()) {
                prefix_.top(false);

                for (std::size_t ideclaration = 0; ideclaration < decl.declarations().size() - 1; ++ideclaration) {
                    decl.declarations().at(ideclaration)->accept(*this);
                }

                prefix_.top(true);

                decl.declarations().back()->accept(*this);
            }

            prefix_.pop();
        }

        void printer::visit(const extern_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "extern_declaration " << impl::color::reset << decl.range().begin() << "\n";

            prefix_.push(false);

            if (!decl.declarations().empty()) {
                prefix_.top(false);

                for (std::size_t idecl = 0; idecl < decl.declarations().size() - 1; ++idecl) {
                    decl.declarations().at(idecl)->accept(*this);
                }

                prefix_.top(true);

                decl.declarations().back()->accept(*this);
            }

            prefix_.pop();
        }

        void printer::visit(const range_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "range_declaration " << impl::color::reset;
            if (decl.generic()) stream_ << "<generic> ";
            stream_ << decl.range().begin() << " `" << impl::color::white << decl.name().lexeme() << impl::color::reset << "`\n";
            
            prefix_.push(false);

            if (decl.generic()) {
                decl.generic()->accept(*this);
            }

            prefix_.top(true);
            decl.constraint()->accept(*this);
            prefix_.pop();
        }

        void printer::visit(const record_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "record_declaration " << impl::color::reset;
            if (decl.generic()) stream_ << "<generic> ";
            if (decl.is_union()) stream_ << "<union> ";
            else stream_ << "<struct> ";
            stream_ << decl.range().begin() << " `" << impl::color::white << decl.name().lexeme() << impl::color::reset << "`\n";
            
            if (decl.generic()) {
                prefix_.push(decl.fields().empty());
                decl.generic()->accept(*this);
                prefix_.pop();
            }

            if (!decl.fields().empty()) {
                prefix_.push(decl.fields().size() == 1);

                for (std::size_t ifield = 0; ifield < decl.fields().size() - 1; ++ifield) {
                    decl.fields().at(ifield)->accept(*this);
                }

                prefix_.top(true);

                decl.fields().back()->accept(*this);

                prefix_.pop();
            }
        }

        void printer::visit(const variant_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "variant_declaration " << impl::color::reset;
            if (decl.generic()) stream_ << "<generic> ";
            stream_ << decl.range().begin() << " `" << impl::color::white << decl.name().lexeme() << impl::color::reset << "`\n";

            prefix_.push(false);

            if (decl.generic()) {
                prefix_.top(decl.types().empty());
                decl.generic()->accept(*this);
            }

            if (!decl.types().empty()) {
                prefix_.top(false);

                for (std::size_t ikind = 0; ikind < decl.types().size() - 1; ++ikind) {
                    decl.types().at(ikind)->accept(*this);
                }

                prefix_.top(true);

                decl.types().back()->accept(*this);
            }

            prefix_.pop();
        }

        void printer::visit(const alias_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "alias_declaration " << impl::color::reset;
            if (decl.generic()) stream_ << "<generic> ";
            stream_ << decl.range().begin() << " `" << impl::color::white << decl.name().lexeme() << impl::color::reset << "`\n";
            
            prefix_.push(false);

            if (decl.generic()) {
                decl.generic()->accept(*this);
            }

            prefix_.top(true);
            decl.type_expression()->accept(*this);  
            prefix_.pop();
        }

        void printer::visit(const use_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "use_declaration " << impl::color::reset << decl.range().begin() << " `" << impl::color::white << decl.path().lexeme() << impl::color::reset << "`\n";
        }

        void printer::visit(const workspace_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "workspace_declaration " << impl::color::reset << decl.range().begin() << " `" << impl::color::white << decl.path().lexeme() << impl::color::reset << "`\n";
        }

        void printer::visit(const source_unit_declaration& decl)
        {
            stream_ << prefix_.str(decl) << impl::color::green << "source_unit_declaration " << impl::color::reset << decl.range().begin();
            
            if (decl.workspace()) {
                const token& path = dynamic_cast<const ast::workspace_declaration*>(decl.workspace().get())->path();
                stream_ << " `" << impl::color::white << path.lexeme() << impl::color::reset << "`";
            }
            
            stream_ << "\n";

            prefix_.push(false);

            if (!decl.imports().empty()) {
                prefix_.top(decl.imports().size() == 1 && decl.statements().empty());

                for (std::size_t iparam = 0; iparam < decl.imports().size() - 1; ++iparam) {
                    decl.imports().at(iparam)->accept(*this);
                }

                prefix_.top(decl.statements().empty());

                decl.imports().back()->accept(*this);
            }

            if (!decl.statements().empty()) {
                prefix_.top(false);

                for (std::size_t ideclaration = 0; ideclaration < decl.statements().size() - 1; ++ideclaration) {
                    decl.statements().at(ideclaration)->accept(*this);
                }

                prefix_.top(true);

                decl.statements().back()->accept(*this);
            }

            prefix_.pop();
        }

        std::string printer::print(const statement& stmt)
        {
            prefix_.push(true);
            stmt.accept(*this);
            prefix_.pop();
            return stream_.str();
        }

        std::string printer::print(const expression& expr)
        {
            prefix_.push(true);
            expr.accept(*this);
            prefix_.pop();
            return stream_.str();
        }
    }
}