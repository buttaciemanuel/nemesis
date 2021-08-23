/**
 * @file parser.hpp
 * @author Emanuel Buttaci
 * This file contains the parser to perform
 * syntax analysis.
 */
#ifndef PARSER_HPP
#define PARSER_HPP

#include "nemesis/parser/ast.hpp"
#include "nemesis/tokenizer/tokenizer.hpp"

namespace nemesis {
    /**
     * The parser is the fundamental unit which takes tokens as input,
     * analyzes the syntax and produces a parse tree as output, more
     * precisely an abstract syntax tree because it will be annotated
     */
    class parser {
    public:
        /**
         * The parser state keeps track of the current position inside
         * the input tokens stream and the current recursion level
         */
        struct state {
            /**
             * Current recursion depth, starts from 0
             */
            int recursion_depth;
            /**
             * Iterator inside the tokens stream which point to the
             * current token
             */
            typename tokenizer::tokens::const_iterator iter;
        };
        /**
         * A syntax error is an exception thrown whenever a serious syntax error occurs
         * and it is very difficult to proceed further without causing cascade errors.
         */
        struct syntax_error {};
        /** 
         * The guard increment recursion depth every time a new
         * recursive rule is taken during parsing and decrements
         * it at the exit
         */
        class guard {
        public:
            /**
             * Maximum recursion depth
             */
            static constexpr int max_depth = 128;
            /**
             * Maximum number of statements inside a block
             */
            static constexpr int max_statements = 256;
            /**
             * Maximum number of names inside a path name
             */
            static constexpr int max_path_names = 32;
            /**
             * Maximum number of elements in different contexts, including
             * array literals, tuple literals, record fields or variant kinds
             */
            static constexpr int max_elements = 128;
            /**
             * Maximum number of parameters inside a function
             */
            static constexpr int max_parameters = 32;
            /**
             * Constructs a new guard object and increments parser recursion depth
             * @param parser Reference to the parser
             */
            guard(parser* parser);
            /**
             * Destroys the guard object and drecrements parser recursion depth
             */
            ~guard();
        private:
            /**
             * Reference to the parser
             */
            parser& parser_;
        };

        friend class guard;
        /**
         * Constructs a new parser object
         * @param tokens Tokens stream produces by the tokenizer
         * @param file Source file reference
         * @param publisher Diagnostic publisher reference
         */
        parser(const tokenizer::tokens& tokens, source_file& file, diagnostic_publisher& publisher);
        /**
         * Constructs the abstract syntax tree performing syntax analysis
         * @return Parse tree (AST without annotation)
         */
        ast::pointer<ast::statement> parse();
    private:
        /**
         * Prints a diagnostic and abort parsing due to a severe error
         * @param diag 
         */
        void abort(diagnostic diag);
        /**
         * Advances into the tokens stream
         */
        void advance();
        /**
         * Restore the current state
         * @param state New state
         */
        void backtrack(state state);
        /**
         * @return Previous token inside the stream, if any
         */
        token previous() const;
        /**
         * @return Current token inside the stream, EOF if at the end 
         */
        token current() const;
        /**
         * @param offset Look ahead distance
         * @return Following token with a distance of `offset` from the current position
         */
        token next(int offset = 1) const;
        /**
         * @return true If current token is EOF
         * @return false Otherwise
         */
        bool eof() const;
        /**
         * If the current token has kind `k`, then it advances and returns true
         * @param k Token kind
         * @return true If current token kinds is k
         * @return false Otherwise
         */
        bool match(enum token::kind k);
        /**
         * Expects a token of the specified kind, otherwise syntax error 
         * @param kind Expected token kind
         * @param expected Expected token lexeme
         * @param message Message inserted into diagnostic on error
         * @param explanation Explananation
         * @param advance_over If true then whenever the expected token matches, then the parser advances
         * @return Expected token on success
         */
        token consume(enum token::kind kind, const std::string& expected, const std::string& message, const std::string& explanation, bool advance_over = true);
        /**
         * It is like `consume` but for closing parenthesis, so it prints a fix hint inside the diagnostic message on error
         * @param message Message insert into diagnostic
         * @param Explanation Message explanation
         * @param opening Openening parenthesis to match (used for diagnostic)
         * @param advance_over If true then whenever the expected token matches, then the parser advances
         * @return Expected token on success
         */
        token parenthesis(enum token::kind, const std::string& message, const std::string& explanation, token opening, bool advance_over = true);
        /**
         * Expects the node not to be null, which is the result of a parser recursive function, otherwise sever syntax error
         * @tparam AstNode Node type (ast::expression, ast::statement, ast::declaration)
         * @param node Result node, if null then severe syntax error
         * @param expected Expected lexeme
         * @param message Message inserted into diagnostic on error
         * @param explain Explanation of the situation that may have caused the error
         * @param fatal If true, then on error parsing aborts, otherwise the parser proceeds
         * @return The same node which was tested not to be null
         */
        template<typename AstNode> ast::pointer<AstNode> expect(ast::pointer<AstNode> node, const std::string& expected, const std::string& message, const std::string& explain, bool fatal = true);
        /**
         * Reports a diagnostic message highlighting a range inside source code
         * @note It is not fatal, so parsing proceeds after
         * @param highlight Range to highlight into source code
         * @param message Message inserted into diagnostic
         * @param explanation Message explanation
         * @param inlined Inlined message inserted into diagnostic
         */
        void report(source_range highlight, const std::string& message,const std::string& explanation, const std::string& inlined = "");
        /**
         * Reports a diagnostic message highlighting a range inside source code and then aborts
         * @note It is fatal, so parsing aborts
         * @param highlight Range to highlight into source code
         * @param message Message inserted into diagnostic
         * @param explanation Message explanation
         * @param inlined Inlined message inserted into diagnostic
         */
        void fatal(source_range highlight, const std::string& message, const std::string& explanation, const std::string& inlined = "");
        /**
         * @return Type name annotation, like `a.b` or `a<int>` or `a.b<int>`
         */
        ast::pointer<ast::expression> path_type_expression();
        /**
         * @return Type annotation, like `a.b | i32` or `a(int) | *Boh | [a: 10]` or `function(int) int` or `*int` or `[int: 10]`, ecc
         */
        ast::pointer<ast::expression> type_expression();
        /**
         * @return Single type annotation
         */
        ast::pointer<ast::expression> single_type_expression();
        /**
         * @return Field type annotation which can be a normal type or a size in bits (like `32`)
         */
        ast::pointer<ast::expression> field_type_expression();
        /**
         * @return Array type annotation, like `[int: N*N]` or `[int]`
         */
        ast::pointer<ast::expression> array_type_expression();
        /**
         * @return Function type annotation, like `function()` or `function(...int) int`
         */
        ast::pointer<ast::expression> function_type_expression();
        /**
         * @return Expression
         */
        ast::pointer<ast::expression> expression();
        /**
         * @return Primary expression, like an identifier (`var` or `self`) or literal (`10.3i` or `"hello"s`)
         */
        ast::pointer<ast::expression> primary_expression();
        /**
         * @return Postifx expression with postfix operator, like `a++` or `Person(name: "John Doe")` or `a.f()`
         */
        ast::pointer<ast::expression> postfix_expression();
        /**
         * @return Unary expression, like `!flag` or `++i` or `*ptr` or `&&var`
         */
        ast::pointer<ast::expression> unary_expression();
        /**
         * @return Type conversion expression, like `10i as int` or `array as *int`
         */
        ast::pointer<ast::expression> conversion_expression();
        /**
         * @return Power expression, like `e ** (-2)` or `n ** n`
         */
        ast::pointer<ast::expression> power_expression();
        /**
         * @return Multiplicative expression, like `a * b` or `a / b` or `a % b`
         */
        ast::pointer<ast::expression> multiplicative_expression();
        /**
         * @return Additive expression, like `a + b` or `a - b`
         */
        ast::pointer<ast::expression> additive_expression();
        /**
         * @return Shift expression, like `a << b` or `a >> b`
         */
        ast::pointer<ast::expression> shift_expression();
        /**
         * @return And expression, like `a & b`
         */
        ast::pointer<ast::expression> and_expression();
        /**
         * @return Xor expression, like `a ^ b`
         */
        ast::pointer<ast::expression> xor_expression();
        /**
         * @return Or expression, like `a | b`
         */
        ast::pointer<ast::expression> or_expression();
        /**
         * @return Range expression, like `1..` or `..2` or `1..=100`
         */
        ast::pointer<ast::expression> range_expression();
        /**
         * @return Comparison expression, like `a < b` or `a >= b`
         */
        ast::pointer<ast::expression> comparison_expression();
        /**
         * @return Equality expression, like `a == b` or `a != b`
         */
        ast::pointer<ast::expression> equality_expression();
        /**
         * @return Or expression, like `a && b`
         */
        ast::pointer<ast::expression> logic_and_expression();
        /**
         * @return Or expression, like `a || b`
         */
        ast::pointer<ast::expression> logic_or_expression();
        /**
         * @return Block expression, like `{...}`
         */
        ast::pointer<ast::expression> block_expression();
        /**
         * @return Identifier expression, like `name` or `name!(int, real, n * n)`
         */
        ast::pointer<ast::expression> identifier_expression(bool istype = false);
        /**
         * @return Primary pattern expression, like `a` or `a.b(1, _, value)` or `[1, _, 2]` or `1..=2`
         */
        ast::pointer<ast::expression> primary_pattern_expression();
        /**
         * @return Or pattern expression, like `a | b | c` or simply `a`
         */
        ast::pointer<ast::expression> or_pattern_expression();
        /**
         * @return Pattern expression, like `a..b | 1` or `is i32`
         */
        ast::pointer<ast::expression> pattern_expression();
        /**
         * @return when expression, like `when value {...}`
         */
        ast::pointer<ast::expression> when_expression();
        /**
         * @return For expression, like `for {...}` or `for i: int in [1, 2] {...} else {...}`
         */
        ast::pointer<ast::expression> for_expression();
        /**
         * @return If expression, like `if cond {...} else if cond2 {...} else => ...`
         */
        ast::pointer<ast::expression> if_expression();
        /**
         * @return Behaviour or expression, like `Default | (Print & Boh)` 
         */
        ast::pointer<ast::expression> constraint_or_expression();
        /**
         * @return Behaviour and expression, like `Default & Print` 
         */
        ast::pointer<ast::expression> constraint_and_expression();
        /**
         * @return Behaviour primary expression, like `Default` or `(Default | Print)` 
         */
        ast::pointer<ast::expression> constraint_primary_expression();
        /**
         * @note It does not return any value and therefore cannot be chained
         * @return Assignment statement, like `a.b *= expr` or `f() = value`
         */
        ast::pointer<ast::statement> assignment_statement();
        /**
         * @return Jump statement, like return, break or continue statement
         */
        ast::pointer<ast::statement> jump_statement();
        /**
         * Parses any kind of statement printing an error if out of its context
         * @return Statement like `a = b` or `function f(a: int) {...}` or `type(T) data(a: T)`
         */
        ast::pointer<ast::statement> statement();
        /**
         * @return Nucleus declaration, like `nucleus ns.core`
         */
        ast::pointer<ast::declaration> nucleus_declaration();
        /**
         * @return Import declaration, like `use ns.core`
         */
        ast::pointer<ast::declaration> use_declaration();
        /**
         * @return Variable or constant declaration, like `static mutable val i = 0`, `const SIZE: usize = 10`
         */
        ast::pointer<ast::declaration> variable_declaration();
        /**
         * A concept defines some contraints that a type must satisfay, for example
         * T is a numeric type if it defines addition and substraction over T
         * `concept(T) Numeric as Primitive {
         *     add(right: T, left: T) T
         *     subtract(right: T, left: T) T
         * }`
         * @return Concept declaration
         */
        ast::pointer<ast::declaration> concept_declaration();
        /**
         * @return Extern declaration, like `extern {
         *     function memcpy(mutable dest: *[byte], source: *[byte], size: usze)
         * }`
         */
        ast::pointer<ast::declaration> extern_declaration();
        /**
         * @return Extend declaration, like `extend(T, const N: usize) Array(T, N) as TypeName  {
         *     typename() string => "Array({T.typename()}, {N})"
         * }`
         */
        ast::pointer<ast::declaration> extend_declaration();
        /**
         * @return Behaviour declaration, like `behaviour(T: Number) RandomGenerator  {
         *     generate(self) T {...}
         * }`
         */
        ast::pointer<ast::declaration> behaviour_declaration();
        /**
         * @return Test declaration, for unit testing, like 
         * `test my_test {
         *     ...
         * }`
         */
        ast::pointer<ast::declaration> test_declaration();
        /**
         * Parses a name declaration, which is a couple of identifier and type annotation, 
         * like hide a: 3` which has eventually `hide` before
         * @return Parameter or named field declaration
         */
        ast::pointer<ast::declaration> field_declaration();
        /**
         * Parses a parameter declaration which may be mutable, like `mutable input: [T]`
         * @return Parameter declaration
         */
        ast::pointer<ast::declaration> parameter_declaration();
        /**
         * A generic parameter can be a generic type (like `T `)
         * or a compile time constant (like `SIZE: int`)
         * @return Generic type or generic constant declaration
         */
        ast::pointer<ast::declaration> generic_parameter_declaration();
        /**
         * A generic clause is composed by generic parameters list and a constraint clause eventually, like
         * `(T, N: usize) if Default(T) & Clone(T)`
         */
        ast::pointer<ast::declaration> generic_clause_declaration(bool constraints = false);
        /**
         * A type declaration may be a range or record or variant or alias type declaration 
         * @return Type declaration (like `type Record(a: int)` or `type(T, const N: usize) square = Matrix(T, N, N)`)
         */
        ast::pointer<ast::declaration> type_declaration();
        /**
         * A contract clause may be composed by multiple contract statements like
         * `require i < 0, assure i >= 0
         *  invariant sorted(sequence)`
         * @return List of contracts statement
         */
        ast::pointers<ast::statement> contract_clause_statements();
        /**
         * Parses a named function declaration in global context or inside `extern` block like
         * `function factorial(n: u32) u128 {...}` or `hide function accum(args: ...int) int {...}`
         * @param is_external If true, then it can't accept any body definition
         * @return Function declaration
         */
        ast::pointer<ast::declaration> function_declaration(bool is_external = false);
        /**
         * @return Property declaration inside extend block like `.determinant(m: matrix(T, M, N)) T = ...`
         */
        ast::pointer<ast::declaration> property_declaration();
        /**
         * @return Variant kind declaration, like 
         * `type Node {
         *     Expr(value: real),
         *     Name(name: string)
         * }`
         */
        ast::pointer<ast::declaration> variant_kind_declaration();
        /**
         * @return Source unit (file) declaration, like
         * `nucleus testing
         * use ns.core
         * 
         * test my_test {...}
         * 
         * function main() {...}`
         */
        ast::pointer<ast::declaration> source_unit_declaration();
        /**
         * Parses a path name like `a` or `a.b.c`
         * @param p Path reference
         * @return true If no error occurs
         * @return false Otherwise
         */
        bool path(ast::path& p);
        /**
         * Parses a generic arguments which can be a type annotation (like `string`) or a constant expression (like `var` or `1` or `2 ** 3`)
         * @return Generic argument expression
         */
        ast::pointer<ast::expression> generic_argument();
        /**
         * Parses a generic arguments list like `(int, N * N)` where each argument can be either a type or expression
         * @param args Generic arguments list reference
         * @return true If no error occurs
         * @return false Otherwise
         */
        bool generic_arguments_list(ast::pointers<ast::expression>& args);
        /**
         * Tests if the parsed statement is properly divided by a newline or semicolon from what follows next,
         * otherwise error. For example `val a = 10 ++b` would cause an error because the variable declaration
         * and the expression statement must be divided by a semicolon, like `val a = 10; ++b`, or must be
         * splitted on two different lines
         * @param before Parsed statement
         */
        void separator(ast::pointer<ast::statement> before);
        /**
         * Current parser state
         */
        state state_;
        /**
         * Tokens stream produced by the tokenizer
         */
        const tokenizer::tokens& tokens_;
        /**
         * Reference to source file 
         */
        source_file& file_;
        /**
         * Reference to diagnostic publisher
         */
        diagnostic_publisher& publisher_;
        /**
         * First and only nucleus declaration (used for diagnostics)
         */
        ast::pointer<ast::statement> nucleus_;

        int silent_mode_ = 0;
        void silence() { ++silent_mode_; }
        void unsilence() { --silent_mode_; }
        bool silent() const { return silent_mode_ > 0; }
    };

    template<typename AstNode>
    ast::pointer<AstNode> parser::expect(ast::pointer<AstNode> node, const std::string& expected, const std::string& message, const std::string& explain, bool fatal)
    {
        static_assert(std::is_base_of<ast::node, AstNode>::value, "parser::expect(): AstNode type must be subclass of ast::node");

        if (!node) {
            auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(current().location())
                            .message(message)
                            .explanation(explain)
                            .small(true)
                            .highlight(current().range(), diagnostic::format("expected $", expected));

            if (!eof()) {
                if (current().is_keyword()) {
                    builder.message(diagnostic::format("$ I found keyword `$` instead.", message, current().lexeme()));
                }
                else {
                    builder.message(diagnostic::format("$ I found `$` instead.", message, current().lexeme()));
                }
            }
            
            if (fatal) abort(builder.build());
            else publisher_.publish(builder.build());
        }

        return node;
    }
}

#endif // PARSER_HPP