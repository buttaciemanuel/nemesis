#ifndef CHECKER_HPP
#define CHECKER_HPP

#include <stack>

#include "nemesis/analysis/type.hpp"
#include "nemesis/analysis/environment.hpp"
#include "nemesis/driver/compilation.hpp"

namespace nemesis {
    class substitutions;
    class code_generator;

    namespace ast {
        class pattern_matcher;
    }

    class checker : public ast::visitor {
        friend class evaluator;
        friend class substitutions;
        friend class ast::pattern_matcher;
        friend class code_generator;
    public:
        // semantic errors in the program that are printed and then analysis continues
        struct semantic_error {};
        // for errors that cause the interruption of static analysis
        struct abort_error {};
        // for cyclic definitions of types or variables
        struct cyclic_symbol_error {
            const ast::expression* expression;
            const ast::declaration* declaration;

            cyclic_symbol_error(const ast::expression* e, const ast::declaration* d) :
                expression(e),
                declaration(d)
            {}

            class diagnostic diagnostic() const {
                auto builder = diagnostic::builder().severity(diagnostic::severity::error).location(expression->range().begin()).highlight(expression->range());
                if (auto vardecl = dynamic_cast<const ast::var_declaration*>(declaration)) {
                    builder.message(diagnostic::format("Cyclic definition for variable `$` is evil, idiot!", vardecl->name().lexeme()))
                            .note(vardecl->name().range(), diagnostic::format("This is variable `$` declaration.", vardecl->name().lexeme()));
                }
                else if (auto constdecl = dynamic_cast<const ast::const_declaration*>(declaration)) {
                    builder.message(diagnostic::format("Cyclic definition for constant `$` is evil, idiot!", constdecl->name().lexeme()))
                            .note(constdecl->name().range(), diagnostic::format("This is constant `$` declaration.", constdecl->name().lexeme()));
                }
                else if (auto typedecl = dynamic_cast<const ast::type_declaration*>(declaration)) {
                    builder.message(diagnostic::format("Cyclic definition for type `$` is evil, idiot!", typedecl->name().lexeme()))
                            .note(typedecl->name().range(), diagnostic::format("This is type `$` declaration.", typedecl->name().lexeme()));

                    if (!dynamic_cast<const ast::alias_declaration*>(declaration))
                        builder.insertion(source_range(expression->range().begin(), 1), "*", "I would use an indirect link like a pointer, if I were you.");
                }
                else if (auto cdecl = dynamic_cast<const ast::concept_declaration*>(declaration)) {
                    builder.message(diagnostic::format("Cyclic definition for concept `$` is evil, idiot!", cdecl->name().lexeme()))
                            .note(cdecl->name().range(), diagnostic::format("This is concept `$` declaration.", cdecl->name().lexeme()));
                }

                return builder.build();
            }
        };

        struct scope {
            scope(checker* instance, const ast::node* enclosing);
            ~scope();
        private:
            checker* instance_;
        };
        
        checker(nemesis::compilation& compilation) : compilation_(compilation) {}
        ~checker();
        void check();
        const class nemesis::compilation& compilation() const { return compilation_; }
        source_file& source() const { return *file_; }
        diagnostic_publisher& publisher() const { return compilation_.get_diagnostic_publisher(); }
        environment& scope() const { return *scope_; }
        std::unordered_map<const ast::node*, environment*>& scopes() { return scopes_; }
        environment* get_scope_by_context(const ast::node* root) const { return scopes_.at(root); }
        ast::workspace* workspace() const;
        const ast::function_declaration* entry_point() const { return entry_point_; }
        void warning(source_range highlight, const std::string& message, const std::string& explanation = "", const std::string& inlined = "");
        void warning(const ast::unary_expression& expr, const std::string& message, const std::string& explanation = "", const std::string& inlined = "");
        void warning(const ast::binary_expression& expr, const std::string& message, const std::string& explanation = "", const std::string& inlined = "");
        void report(source_range highlight, const std::string& message, const std::string& explanation = "", const std::string& inlined = "");
        void report(const ast::unary_expression& expr, const std::string& message, const std::string& explanation = "", const std::string& inlined = "");
        void report(const ast::binary_expression& expr, const std::string& message, const std::string& explanation = "", const std::string& inlined = "");
        void error(source_range highlight, const std::string& message, const std::string& explanation = "", const std::string& inlined = "");
        void error(const ast::unary_expression& expr, const std::string& message, const std::string& explanation = "", const std::string& inlined = "");
        void error(const ast::binary_expression& expr, const std::string& message, const std::string& explanation = "", const std::string& inlined = "");
        void error(const ast::assignment_statement& stmt, const std::string& message, const std::string& explanation = "", const std::string& inlined = "");
        void immutability(const ast::postfix_expression& expr);
        void immutability(const ast::unary_expression& expr);
        void immutability(const ast::assignment_statement& stmt);
        void mismatch(source_range x, source_range y, const std::string& message, const std::string& explanation = "", const std::string& inlined = "");
        constval evaluate(ast::pointer<ast::expression> expr);
        const ast::declaration* resolve(const ast::path& path) const;
        const ast::declaration* resolve_type(const ast::path& path, const environment* context = nullptr) const;
        const ast::declaration* resolve_variable(const ast::path& path, const environment* context = nullptr) const;
        std::string fullname(const ast::declaration* decl) const;
    private:
        environment* begin_scope(const ast::node* enclosing);
        void end_scope();
        void add_to_scope(environment* scope, ast::pointer<ast::declaration> decl, const ast::statement* after = nullptr, bool is_after = true) const;
        std::unordered_map<std::string, const ast::declaration*> similars(const std::string& name, const environment* scope) const;
        void do_imports();
        void import_core_library_in_workspaces();
        static size_t path_contains_subpath(const std::string& path, const std::string& subpath);
        ast::pointer<ast::type> instantiate_type(const ast::type_declaration& tdecl, substitutions subs);
        bool instantiate_concept(const ast::concept_declaration& cdecl, substitutions subs);
        ast::pointer<ast::function_declaration> instantiate_function(const ast::function_declaration& fdecl, substitutions subs);
        static ast::pointer<ast::expression> implicit_cast(ast::pointer<ast::type> type, ast::pointer<ast::expression> expression);
        static ast::pointer<ast::expression> implicit_forced_cast(ast::pointer<ast::type> type, ast::pointer<ast::expression> expression);
        bool is_partially_specialized(ast::pointer<ast::path_type_expression> expr);
        const ast::function_declaration* is_cloneable(ast::pointer<ast::type> type) const;
        const ast::function_declaration* is_destructible(ast::pointer<ast::type> type) const;
        const ast::function_declaration* is_default_constructible(ast::pointer<ast::type> type) const;
        const ast::function_declaration* is_iterable(ast::pointer<ast::type> type) const;
        const ast::function_declaration* is_iterator(ast::pointer<ast::type> type) const;
        const ast::function_declaration* is_indexable(ast::pointer<ast::type> type) const;
        bool is_string_convertible(ast::pointer<ast::type> type, const ast::property_declaration*& procedure) const;
        void test_immutable_assignment(const ast::var_declaration& decl, const ast::expression& value) const;
        void test_immutable_assignment(ast::pointer<ast::type> lvalue, const ast::expression& rvalue) const;
        void add_type(ast::pointer<ast::type> type);
        void add_function(const ast::function_declaration* fn);
        ast::pointer<ast::var_declaration> create_temporary_var(const ast::expression& value) const;

        void visit(const ast::bit_field_type_expression& expr);
        void visit(const ast::path_type_expression& expr);
        void visit(const ast::array_type_expression& expr);
        void visit(const ast::tuple_type_expression& expr);
        void visit(const ast::pointer_type_expression& expr);
        void visit(const ast::function_type_expression& expr);
        void visit(const ast::record_type_expression& expr);
        void visit(const ast::variant_type_expression& expr);
        void visit(const ast::literal_expression& expr);
        void visit(const ast::identifier_expression& expr);
        void visit(const ast::tuple_expression& expr);
        void visit(const ast::array_expression& expr);
        void visit(const ast::array_sized_expression& expr);
        void visit(const ast::parenthesis_expression& expr);
        void visit(const ast::block_expression& expr);
        void visit(const ast::function_expression& expr);
        void visit(const ast::postfix_expression& expr);
        void visit(const ast::call_expression& expr);
        void visit(const ast::member_expression& expr);
        void visit(const ast::array_index_expression& expr);
        void visit(const ast::tuple_index_expression& expr);
        void visit(const ast::record_expression& expr);
        void visit(const ast::unary_expression& expr);
        void visit(const ast::binary_expression& expr);
        void visit(const ast::range_expression& expr);
        void visit(const ast::ignore_pattern_expression& expr);
        void visit(const ast::literal_pattern_expression& expr);
        void visit(const ast::path_pattern_expression& expr);
        void visit(const ast::tuple_pattern_expression& expr);
        void visit(const ast::array_pattern_expression& expr);
        void visit(const ast::record_pattern_expression& expr);
        void visit(const ast::labeled_record_pattern_expression& expr);
        void visit(const ast::range_pattern_expression& expr);
        void visit(const ast::or_pattern_expression& expr);
        void visit(const ast::when_expression& expr);
        void visit(const ast::when_pattern_expression& expr);
        void visit(const ast::when_cast_expression& expr);
        void visit(const ast::for_range_expression& expr);
        void visit(const ast::for_loop_expression& expr);
        void visit(const ast::if_expression& expr);
        void visit(const ast::null_statement& stmt);
        void visit(const ast::expression_statement& stmt);
        void visit(const ast::assignment_statement& stmt);
        void visit(const ast::later_statement& stmt);
        void visit(const ast::return_statement& stmt);
        void visit(const ast::break_statement& stmt);
        void visit(const ast::continue_statement& stmt);
        void visit(const ast::contract_statement& stmt);
        void visit(const ast::field_declaration& decl);
        void visit(const ast::tuple_field_declaration& decl);
        void visit(const ast::parameter_declaration& decl);
        void visit(const ast::var_declaration& decl);
        void visit(const ast::var_tupled_declaration& decl);
        void visit(const ast::const_declaration& decl);
        void visit(const ast::const_tupled_declaration& decl);
        void visit(const ast::generic_clause_declaration& decl);
        void visit(const ast::generic_const_parameter_declaration& decl);
        void visit(const ast::generic_type_parameter_declaration& decl);
        void visit(const ast::test_declaration& decl);
        void visit(const ast::function_declaration& decl);
        void visit(const ast::property_declaration& decl);
        void visit(const ast::concept_declaration& decl);
        void visit(const ast::extend_declaration& decl);
        void visit(const ast::behaviour_declaration& decl);
        void visit(const ast::extern_declaration& decl);
        void visit(const ast::range_declaration& decl);
        void visit(const ast::record_declaration& decl);
        void visit(const ast::variant_declaration& decl);
        void visit(const ast::alias_declaration& decl);
        void visit(const ast::use_declaration& decl);
        void visit(const ast::workspace_declaration& decl);
        void visit(const ast::source_unit_declaration& decl);
        /**
         * This is the compilation object, which contains the root for all libraries to analyze
         * The order of analysis is the same in which they are declared inside the object
         */
        class compilation& compilation_;
        /**
         * Current package name
         */
        std::string package_;
        /**
         * Current analyzed file
         */
        source_file* file_;
        /**
         * Current statement
         */
        const ast::statement* statement_ = nullptr;
        /**
         * Entry point of execution
         */
        const ast::function_declaration* entry_point_ = nullptr;
        /**
         * Current scope environment
         */
        environment* scope_ = nullptr;
        /**
         * All scopes
         */
        std::unordered_map<const ast::node*, environment*> scopes_;
        /**
         * Queue of declarations to be added to scope, this is performed later
         * as it would invalidate vector of statements because of insertion
         * Boolean value stands for after (true) or before (false) the passed statement
         */
        std::list<std::tuple<environment*, ast::pointer<ast::declaration>, const ast::statement*, bool>> pending_insertions;
        /**
         * Checker pass on an abstract syntax file
         * Zero pass registers workspace in root
         * First pass registers the workspace type declarations
         * Second pass fully checks global type declarations
         * Third pass fully checks remained statements
         */
        enum class pass { zero, first, second, third, fourth } pass_;
    };

    // performs substituions of
    // [1] parametric types $T
    // [2] parametric constants $C
    // substitions takes place from node `root` from which traversal starts
    // and for resolving names and types `context` is used as enclosing environment
    class substitutions : public ast::visitor {
    public:
        substitutions(const environment* context, const ast::node* root) : ast::visitor(), context_(context), root_(root) {}
        void put(const ast::declaration* tparam, ast::pointer<ast::type> substitution)
        {
            tsubstitutions.emplace(tparam, substitution);
        }
        void put(const ast::declaration* cparam, constval substitution)
        {
            csubstitutions.emplace(cparam, substitution);
        }
        std::unordered_map<const ast::declaration*, ast::pointer<ast::type>>::const_iterator type(const ast::declaration* ref) const
        {
            return tsubstitutions.find(ref);
        }
        std::unordered_map<const ast::declaration*, constval>::const_iterator constant(const ast::declaration* ref) const
        {
            return csubstitutions.find(ref);
        }
        const std::unordered_map<const ast::declaration*, ast::pointer<ast::type>>& types() const 
        {
            return tsubstitutions;
        }
        const std::unordered_map<const ast::declaration*, constval>& constants() const 
        {
            return csubstitutions;
        }
        std::unordered_map<const ast::declaration*, impl::parameter> map() const 
        {
            std::unordered_map<const ast::declaration*, impl::parameter> result;
            for (auto c : csubstitutions) result.emplace(c.first, impl::parameter::make_value(c.second));
            for (auto t : tsubstitutions) result.emplace(t.first, impl::parameter::make_type(t.second));
            return result;
        }
        const environment* context() const { return context_; }
        void context(const environment* context) { context_ = context; }
        const ast::node* root() const { return root_; }
        void root(ast::node* root) { root_ = root; }
        void substitute();
    private:
        void visit(const ast::bit_field_type_expression& expr);
        void visit(const ast::path_type_expression& expr);
        void visit(const ast::array_type_expression& expr);
        void visit(const ast::tuple_type_expression& expr);
        void visit(const ast::pointer_type_expression& expr);
        void visit(const ast::function_type_expression& expr);
        void visit(const ast::record_type_expression& expr);
        void visit(const ast::variant_type_expression& expr);
        void visit(const ast::literal_expression& expr);
        void visit(const ast::identifier_expression& expr);
        void visit(const ast::tuple_expression& expr);
        void visit(const ast::array_expression& expr);
        void visit(const ast::array_sized_expression& expr);
        void visit(const ast::parenthesis_expression& expr);
        void visit(const ast::block_expression& expr);
        void visit(const ast::function_expression& expr);
        void visit(const ast::postfix_expression& expr);
        void visit(const ast::call_expression& expr);
        void visit(const ast::member_expression& expr);
        void visit(const ast::array_index_expression& expr);
        void visit(const ast::tuple_index_expression& expr);
        void visit(const ast::record_expression& expr);
        void visit(const ast::unary_expression& expr);
        void visit(const ast::binary_expression& expr);
        void visit(const ast::range_expression& expr);
        void visit(const ast::ignore_pattern_expression& expr);
        void visit(const ast::literal_pattern_expression& expr);
        void visit(const ast::path_pattern_expression& expr);
        void visit(const ast::tuple_pattern_expression& expr);
        void visit(const ast::array_pattern_expression& expr);
        void visit(const ast::record_pattern_expression& expr);
        void visit(const ast::labeled_record_pattern_expression& expr);
        void visit(const ast::range_pattern_expression& expr);
        void visit(const ast::or_pattern_expression& expr);
        void visit(const ast::when_expression& expr);
        void visit(const ast::when_pattern_expression& expr);
        void visit(const ast::when_cast_expression& expr);
        void visit(const ast::for_range_expression& expr);
        void visit(const ast::for_loop_expression& expr);
        void visit(const ast::if_expression& expr);
        void visit(const ast::null_statement& stmt);
        void visit(const ast::expression_statement& stmt);
        void visit(const ast::assignment_statement& stmt);
        void visit(const ast::later_statement& stmt);
        void visit(const ast::return_statement& stmt);
        void visit(const ast::break_statement& stmt);
        void visit(const ast::continue_statement& stmt);
        void visit(const ast::contract_statement& stmt);
        void visit(const ast::field_declaration& decl);
        void visit(const ast::tuple_field_declaration& decl);
        void visit(const ast::parameter_declaration& decl);
        void visit(const ast::var_declaration& decl);
        void visit(const ast::var_tupled_declaration& decl);
        void visit(const ast::const_declaration& decl);
        void visit(const ast::const_tupled_declaration& decl);
        void visit(const ast::generic_clause_declaration& decl);
        void visit(const ast::generic_const_parameter_declaration& decl);
        void visit(const ast::generic_type_parameter_declaration& decl);
        void visit(const ast::test_declaration& decl);
        void visit(const ast::function_declaration& decl);
        void visit(const ast::property_declaration& decl);
        void visit(const ast::concept_declaration& decl);
        void visit(const ast::extend_declaration& decl);
        void visit(const ast::behaviour_declaration& decl);
        void visit(const ast::extern_declaration& decl);
        void visit(const ast::range_declaration& decl);
        void visit(const ast::record_declaration& decl);
        void visit(const ast::variant_declaration& decl);
        void visit(const ast::alias_declaration& decl);
        void visit(const ast::use_declaration& decl);
        void visit(const ast::workspace_declaration& decl);
        void visit(const ast::source_unit_declaration& decl);
        // context node, which is the scope for searching
        const environment* context_;
        // root node from which substitution starts and goes deep into its children
        const ast::node* root_;
        // substition of parametric types with types
        std::unordered_map<const ast::declaration*, ast::pointer<ast::type>> tsubstitutions;
        // substitution of parametric constants with constant values
        std::unordered_map<const ast::declaration*, constval> csubstitutions;
    };
}

#endif