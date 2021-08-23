#ifndef TYPE_MATCHER_HPP
#define TYPE_MATCHER_HPP

#include "nemesis/analysis/type.hpp"

namespace nemesis {
    namespace ast {
        class type_matcher {
        public:
            using parameter = nemesis::types::parameter;
            
            struct mismatch {};

            struct result {
                bool duplication : 1;
                bool mismatch : 1;
                std::unordered_map<std::string, parameter> bindings;

                result() : duplication(false), mismatch(false) {}

                void invalidate() 
                {
                    mismatch = true;
                    bindings.clear();
                }

                bool type(std::string name, ast::pointer<ast::type> type)
                {
                    if (bindings.count(name)) return false;
                    parameter param;
                    param.kind = parameter::kind::type;
                    param.type = type;
                    bindings.emplace(name, param);
                    return true;
                }

                bool value(std::string name, constval value)
                {
                    if (bindings.count(name)) return false;
                    parameter param;
                    param.kind = parameter::kind::value;
                    param.value = value;
                    bindings.emplace(name, param);
                    return true;
                }

                operator bool() const { return !mismatch; }
            };
            
            type_matcher(ast::pointer<ast::expression> context, ast::pointer<ast::type> pattern, diagnostic_publisher& publisher);
            bool match(ast::pointer<ast::type> expression, result& result, bool variadic_pattern = false) const;
        private:
            // this method creates expressions from condition to match with comparisons against patterns recursively
            void match(ast::pointer<ast::expression> pattern, parameter expression, struct result& result) const;
            void match(parameter pattern, parameter expression, struct result& result) const;
            void error(source_range range, const std::string& message, const std::string& explanation = "", const std::string& inlined = "") const;

            ast::pointer<ast::expression> context_;
            ast::pointer<ast::type> pattern_;
            diagnostic_publisher& publisher_;
        };
    }
}

#endif