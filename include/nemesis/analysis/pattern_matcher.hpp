#ifndef PATTERN_MATCHER_HPP
#define PATTERN_MATCHER_HPP

#include <set>

#include "nemesis/parser/ast.hpp"

namespace nemesis {
    class checker;
    
    namespace ast {
        class pattern_matcher {
        public:
            struct mismatch {};

            struct binding {
                token name;
                const ast::expression* value;
                binding(const token& name, const ast::expression* value);
                bool operator<(const binding& other) const;
            };

            struct result {
                bool mismatch = false;
                std::set<binding> bindings;

                ast::pointer<ast::expression> condition;
                ast::pointers<ast::declaration> declarations;

                void invalidate() 
                {
                    mismatch = true;
                    bindings.clear();
                }

                bool put(token name, const ast::expression* value)
                {
                    binding element(name, value);
                    if (bindings.count(element)) return false;
                    bindings.insert(element);
                    return true;
                }

                operator bool() const { return !mismatch; }
            };

            pattern_matcher(const ast::expression& pattern, diagnostic_publisher& publisher, checker& checker);
            result match(const ast::expression& expression) const;
        private:
            // this method creates expressions from condition to match with comparisons against patterns recursively
            ast::pointer<ast::expression> match(const ast::expression* pattern, ast::pointer<ast::type> expected, const ast::expression* expression, ast::pointer<ast::expression> tree, struct result& result) const;
            void mismatched(const ast::expression* pattern, ast::pointer<ast::type> expected, const ast::expression* expression) const;
            void error(const ast::expression* pattern, const std::string& message, const std::string& explanation = "", const std::string& inlined = "") const;
            bool bind(const token& name, const ast::expression* value, result& result) const;
            ast::pointer<ast::expression> from_pattern(const ast::expression* pattern) const;

            const ast::expression* pattern_;
            diagnostic_publisher& publisher_;
            checker& checker_;
        };
    }
}

#endif