#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP

#include <stack>

#include "nemesis/analysis/environment.hpp"

namespace nemesis {
    class type_factory;
    class checker;

    class evaluator : public ast::visitor {
    public:
        struct error {};
        struct generic_evaluation {};
        evaluator(checker& checker);
        constval evaluate(ast::pointer<ast::expression> expr);
        static constval integer_parse(const std::string& value);
        static constval float_parse(const std::string& value);
        static constval imag_parse(const std::string& value);
    private:
        void test_operation_error(const ast::unary_expression& expr, const std::string& operation, constval result);
        void test_operation_error(const ast::binary_expression& expr, const std::string& operation, constval result);
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
        void visit(const ast::block_expression& expr) {}
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
        void visit(const ast::implicit_conversion_expression& expr);

        void push(constval val) { stack_.push(val); }
        
        constval pop() 
        {
            constval result = stack_.top();
            stack_.pop();
            return result;
        }
        
        checker& checker_;  
        std::stack<constval> stack_;
    };
}

#endif