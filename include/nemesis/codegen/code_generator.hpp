#ifndef CODE_GENERATOR_HPP
#define CODE_GENERATOR_HPP

#include "nemesis/analysis/checker.hpp"

namespace nemesis {
    class code_generator : public ast::visitor {
    public:
        code_generator(checker& checker);
        ~code_generator();
        // compiles source files and generates cpp target source files
        std::list<compilation::target> generate();
        void trace(bool flag);
        bool trace() const;
        std::string emit(ast::pointer<ast::type> type) const;
        std::string emit(ast::pointer<ast::type> type, std::string variable) const;
        std::string emit(constval value) const;
        std::string prototype(const ast::type_declaration* decl) const;
        std::string prototype(const ast::function_declaration* decl) const;
        std::string prototype(const ast::property_declaration* decl) const;
        std::string fullname(const ast::declaration *decl) const;
    private:
        struct guard;

        class filestream {
            friend struct guard;
        public:
            filestream() = default;
            filestream(std::string filepath) : path_(filepath), stream_() {}
            std::string path() const { return path_; }
            std::ostringstream& stream() { return stream_; }
            std::ostringstream& line() 
            {
                if (indent_ > 0) stream_ << std::string(4 * (indent_ - 1), ' '); 
                return stream_;
            } 
        private:
            std::string path_;
            std::ostringstream stream_;
            std::size_t indent_ = 0;
        };

        struct guard {
        public:
            guard(filestream& stream) : stream_(stream) { ++stream_.indent_; }
            ~guard() { --stream_.indent_; }
        private:
            filestream& stream_;
        };

        bool emit_if_constant(const ast::expression& expr);
        void emit_anonymous_type(ast::pointer<ast::type> type);
        void emit_lambda_type(const ast::function_expression* lambda);
        void emit_in_contracts(const ast::node& current);
        void emit_out_contracts(const ast::node& current);

        void visit(const ast::bit_field_type_expression& expr);
        void visit(const ast::path_type_expression& expr);
        void visit(const ast::array_type_expression& expr);
        void visit(const ast::tuple_type_expression& expr);
        void visit(const ast::pointer_type_expression& expr);
        void visit(const ast::function_type_expression& expr);
        void visit(const ast::record_type_expression& expr);
        void visit(const ast::variant_type_expression& expr);
        
        void visit(const ast::implicit_conversion_expression& expr);
        void visit(const ast::literal_expression& expr);
        void visit(const ast::postfix_expression& expr);
        void visit(const ast::unary_expression& expr);
        void visit(const ast::binary_expression& expr);
        void visit(const ast::identifier_expression& expr);
        void visit(const ast::tuple_expression& expr);
        void visit(const ast::array_expression& expr);
        void visit(const ast::array_sized_expression& expr);
        void visit(const ast::parenthesis_expression& expr);
        void visit(const ast::range_expression& expr);
        void visit(const ast::record_expression& expr);
        void visit(const ast::member_expression& expr);
        void visit(const ast::call_expression& expr);
        void visit(const ast::array_index_expression& expr);
        void visit(const ast::tuple_index_expression& expr);
        void visit(const ast::block_expression& expr);
        void visit(const ast::if_expression& expr);
        void visit(const ast::when_cast_expression& expr);
        void visit(const ast::when_pattern_expression& expr);
        void visit(const ast::when_expression& expr);
        void visit(const ast::for_loop_expression& expr);
        void visit(const ast::for_range_expression& expr);
        void visit(const ast::function_expression& expr);

        void visit(const ast::expression_statement& stmt);
        void visit(const ast::assignment_statement& stmt);
        void visit(const ast::return_statement& stmt);
        void visit(const ast::contract_statement& stmt);

        void visit(const ast::parameter_declaration& decl);
        void visit(const ast::field_declaration& decl);
        void visit(const ast::tuple_field_declaration& decl);
        void visit(const ast::var_declaration& decl);
        void visit(const ast::const_declaration& decl);
        void visit(const ast::function_declaration& decl);
        void visit(const ast::property_declaration& decl);
        void visit(const ast::extend_declaration& decl);
        void visit(const ast::behaviour_declaration& decl);
        void visit(const ast::extern_declaration& decl);
        void visit(const ast::range_declaration& decl);
        void visit(const ast::record_declaration& decl);
        void visit(const ast::variant_declaration& decl);
        /**
         * Semantic checker contains all information produced by previous analysis
         */
        checker& checker_;
        /**
         * Current workspace, used to generate name of current compilation unit for name mangling
         */
        ast::pointer<ast::workspace> workspace_ = nullptr;
        /**
         * Current output file stream
         */
        filestream output_;
        /**
         * Stack of variables to be assigned from scopes, for example
         * 'val a = if true { "ok" } else { "damn" }'
         * variable 'a' is pushed onto the stack to get something like
         * if (true) { a = "ok" } else { a = "damn" }
         */
        std::stack<std::string> result_vars;
        /**
         * Back tracing mode, slower
         * Not set by default to speed up the application
         */
        bool trace_ = false;
    };
}

#endif