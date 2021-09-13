#include "nemesis/parser/parser.hpp"

#include <iostream>

namespace nemesis {
    namespace impl {
        // explanations for diagnostic messages will help user understanding concepts on which errors may have been committed
        static const std::string guard_explanation = diagnostic::format("It may seem weird but some limits are imposed during compilation: • recursion depth cannot exceed $ • number of arguments cannot exceed $ • number of statements inside a function cannot exceed $ • number of elements (array, tuple, record, variant) cannot exceed $ • number of names inside path cannot exceed $", parser::guard::max_depth, parser::guard::max_parameters, parser::guard::max_statements, parser::guard::max_elements, parser::guard::max_path_names);
        static const std::string path_explanation = "Path are constructed with dots and specify the full name of a declaration. You can also use explict generic argument list if you want. Here's some examples: • `ns.math.PI` specifies the path of PI constant • `ns.datastruct.List!(f32)` is a path with explicit generic instantiation";
        static const std::string field_expr_explanation = "You can access structure field using the following notations: • `structure.field` for record structure (named field) • `tuple.index`, like `tuple.1` for tuple structure (anonymous field)";
        static const std::string parenthesis_expr_explanation = "Parenthesis expression is useful to give some operations precedence over others, for example if we look at `3 * (2 + 3i)` the addition inside parenthesis has precedence over the product.";
        static const std::string array_init_explanation = "You can initialize an array in two ways: • as a list of elements `[ a, b, c ]` • by specifing its values and length `[ value : length ]`";
        static const std::string tuple_init_explanation = "You can initialize a tuple by its list of elements, for example: • `val single = (\"Only me\",)` for a single item tuple • `val tuple = (\"John Doe\", 35)` for a multi items tuple";
        static const std::string record_init_explanation = "You can initialize a record (struct or union) in this way: • `val anonymous = (name: \"John Doe\", age: 35)` for anonymous record • `val person = Person(name: \"John Doe\", age: 35)` for named record";
        static const std::string function_expr_explanation = "A function expression constructs an anonymous function you can use and pass like a variable, so it is very handy in some contexts, for example: • `val factorial = function(n: u32) u32 = /*...*/` as a variable • `call(function() {/*...*/})` for passing as a parameter";
        static const std::string function_type_expr_explanation = "A function type is composed by its list of parameter types and return type, for example: • `function(u32) char` takes an integer as argument and returns a character • `function() (string, c32)` takes no argument and returns a tuple";
        static const std::string array_type_expr_explanation = "An array type is composed by its element type and eventually its length, for example: • `[u32 : 10]` is a ten integers array • `[r64]` is a slice (array reference) of rationals";
        static const std::string tuple_type_expr_explanation = "An tuple type is composed by its element types, for example: • `(u32, char)` is an integer, character tuple • `()` is the empty tuple type, also called `unit`";
        static const std::string record_type_expr_explanation = "A record type is composed by its fields, for example: • `(integer: u32, character: char)` is an integer, character record";
        static const std::string pointer_type_expr_explanation = "A pointer type is used to reference a single item or multiple items, for example: • `*u32` is an integer pointer • `*[u8]` is a pointer to a sequence of bytes whose length is unknown";
        static const std::string variadic_type_expr_explanation = "A variadic type, allowed only as the last of a function, lets you declare a variable number of parameters of the same type you can iterate over using for range, here's an example: • `accumulate(args: ...i32) i32 {/*...*/}` takes a variable number of integers";
        static const std::string variant_type_expr_explanation = "A variant type is a union of types, here's an example: • `val a: none | i32 = get_value()` type could be none or integer • `accumulate(args: ...(i32 | f32)) i32 {/*...*/}` takes a variable number of integers or floats";
        static const std::string block_expr_explanation = "A block expression, enclosed by braces, is used to contain statements. When the last statement is an expression, then the result value of the block is the expression itself. For example: • `{ ++i; println(\"{i}\"); i }` increments `i`, prints `i` and returns its value";
        static const std::string function_call_explanation = "A function call lets you execute the code of a function by passing its arguments. For example: • `factorial(10)` calls `factorial` function • `get_item!(f32)()` calls a function by specifying its generic argument";
        static const std::string array_index_expr_explanation = "You can get an array element or even a slice using brackets, for example: • `array[index]` gives the element at the `index` position • `array[i..f]` gives a slice of the array in the range `[i, f)`";
        static const std::string unary_operator_explanation = "There are different unary operators and they must be followed by an expression: • `+operand` or `-operand` as unary plus and minus • `&operand` to get the address (pointer) of the operand • `*pointer` to dereference a pointer and get the value • `~operand` and `!operand` for bitwise and logical negation • `++operand` and `--operand` for incrementing and drecrementing";
        static const std::string binary_expr_explanation = "There are a lot of binary operations and all use infix notation, for example: • algebraic: `+` `-` `*` `/` `**` `%` • bitwise: `&` `^` `|` `<<` `>>` • comparison: `==` `!=` `<` `>` `<=` `>=` • logical: `&&` `||`";
        static const std::string type_conversion_explanation = "You can use conversion operator `as` when you want to get a type conversion, for example: • `123.3f32 as u32` results in integer `123u32` • `&[1, 2, 3] as *[u32]` results in a pointer to a sequence whose length is unknown";
        static const std::string range_expr_explanation = "Range expressions are mainly useful in these contexts: • iterating over a range, like `for i in 0..n {/*...*/}` • extracting a slice, like `val slice = array[start..=end]` • matching a range inside a when branch, like `1..2 | 10..=11 => /*...*/`";
        static const std::string is_expr_explanation = "When writing a when expression is too verbose you can use is expression as an if condition. It's useful for matching and destructuring an expression, for example: • `if var is Some(value) { println({value}) }` destructures a variant • `if array is [..., value, 10] {/*...*/}` destructures an array • `if tuple is (_, 1, _) {/*...*/}` destructures a tuple";
        static const std::string pattern_expr_explanation = "Pattern expressions are used to find matches against tested values inside a when block or is expression. Let's have a look at different patterns: • `1 | 2 | 3 => /*...*/` to match a value against `1`, `2` or `3` • `1..10 | 100..=200 => /*...*/` to find out whether a value is in at least one range • `[_, 2, value] => {/*...*/}` to match against an array and destructure it • `(1, 2, ...) => {/*...*/}` to match against a tuple and destructure it • `Kind(1, 'a', value) => {/*...*/}` to match and destructure a variant • `Record(a: 1.., b: _) => {/*...*/}` to match and destructure a record \\ Moreover `_` is used to create a useless binding for a value, while `...` is useful to skip elements (array, tuple, record) and must be used in first or last position.";
        static const std::string when_expr_explanation = "A when expression is used for pattern matching of a value. In this way you are able to test your value against different patterns and eventually destructure it. Here's an example: \\2 `when value { \\4 1 | 2 => /* value is 1 or 2 */ \\4 3..10 => /* value is in range [3, 10) */ \\4 (1, second, ...) => /* value is a tuple which is destructured */ \\4 [_, _, _] => /* value is a three elements array */ \\4 VariantKind(saved) => /* value is a variant which gets destructured */ \\2 }` \\ You can even use short version for automatic type casting or pattern matching, for example.  • `when expr = [1, value, ...] {}` for pattern matching • `when expr is i32 {}` for automatic type casting";
        static const std::string if_expr_explanation = "Here's an example of if expression: \\2 `val result = if n % 2 == 0 { \"even\"s } else { \"odd\"s }`";
        static const std::string for_expr_explanation = "You can use for expression for different purposes: • iterating over a range, for example: \\4 `for dog: *Dog in dogs { dog.bark() }` • iterating while a condition is true: \\4 `for i < n && f() { /*...*/ }` • even infinite loop: \\4 `for { /*...*/ }`";
        static const std::string constraint_expr_explanation = "When specifing a generic type parameter you can also impose some contraints over the type, for example: • `function(T) if Compare(T) & Print(T) ...` function requires T to be comparable AND printable • `type(T) Print(T) | Default(T) ...` type requires T to be printable OR default contructible";
        static const std::string assign_stmt_explanation = "When using the assignment statement the right operand may be a: • variable, like `a = 10`  • dereferenced expression, for example `*ptr = 10` • array or slice index expression, like `arr[i] = value` • field expression, for example `structure.field = value` or `tuple.1 = value` \\ Moreover you can perform different assignments based on algebraic operations: \\2 `++` `--` `+=` `-=` `/=` `%=` `*=` `**=` `<<=` `>>=` `&=` `|=` `^=`";
        static const std::string jump_stmt_explanation = "There are three examples of jumping out of flow statements: • break statement for breaking out of a loop, eventually returning a value, for example: \\4 `break value` // yields 4 out of loop • continue expression to jump directly to the next iteration • return expression to jump out of a function, eventually returning a value, for example: \\4 `return \"hello\"` // returns \"hello\" outside the function";
        static const std::string function_decl_explanation = "You can declare a function in different ways: • `factorial(n: u32) u32 = /*...*/` without `function` keyword • `function(T) sort(seq: [T]) {/*...*/}` for a generic function • `function(T) if Add(T) sum(...args: T) T {/*...*/}` takes a variable number of arguments \\ When a function parameter is declared as mutable then it can be modified by the function. When a mutable parameter is passed by pointer it means that eventual changes will be reflected outside the call.";
        static const std::string property_decl_explanation = "You can declare a property of a type instance inside an extend block. A property can be accessed just like a field in read-only mode on an instance. Here's some example: • `.determinant(m: SquareMatrix(f32, N)) f32 {...}` defines a property on a matrix instance • `.area(s: *Shape) f32 = ...` defines a property on a (maybe polymorphic) shape instance";
        static const std::string record_decl_explanation = "You can declare aggregate types with named fields (record) or anonymous fields (tuple). What's more you can declare an aggregate as a structure or as a union, here's an example: • `type(T) vector(hide data: *[T])` declares a structure type with hidden field `data` • `type Person(chars, u32)` declares a tuple structure • `type U union(u32, f32)` declares a union of integer and real";
        static const std::string range_decl_explanation = "A range type defines a numeric type that must fall in a certain range, otherwise it causes runtime error. For example: • `type Natural range 1..` defines a natural (positive non zero integer) numeric type";
        static const std::string variant_decl_explanation = "A variant type, also called sum type, may conver a set of different subtypes, for example: • `type(T) Maybe is T | None` defines an optional type which can conver two subtypes • `type Shape = Square(f32) | Rect(f32, f32) | ...` alias may cover a lot of shape subtypes \\ Finally you can destructure a variant type using when expression block or is expression.";
        static const std::string alias_decl_explanation = "An alias type defines an alternative name for an existing type, for example: \\2 `type(T, N: usize) SquareMatrix = Matrix(T, N, N)` defines a new alias for `Matrix(T, N, N)`";
        static const std::string type_decl_explanation = "You can define a new type as: • aggregate type, for example union or structure, which can have named of anonymous fields • variant type, which can cover different kinds • range type, which defines a numeric type that must follow range constraints • alias type, which defines an alternative name for an existing type";
        static const std::string generic_param_decl_explanation = "Generic parameters, types or constants, appear both in parameterize functions or types: • for example `function(T) if Clone(T) ...` function depends on a clonable T type • for example `type(N: usize) ...` type depends on integer N parameter \\ In order to instantiate explicitly a generic type or function then you need to specify its generic parameters list through `!(...)` notation. \\ Here's an example for creating a list of integers: \\2 `// explicit istantiation \\2 mutable val list: List(u32) = List!(u32).create()`";
        static const std::string workspace_decl_explanation = "When a source file contains an application or library directive its public type and function declarations are automatically exported to all the files who share the common workspace and reside in the same directory. Each file cannot contain more than one workspace directive, on the other hand without any directive source file's type and function declarations are invisible by default. \\ See the example directory `math` with its files all sharing `lib math` directive at the very beginning to belong to the `math` library (homonym directory): \\2 math \\3 ├─integrals.ns \\3 ├─matrix.ns \\3 └─derivatives.ns";
        static const std::string contract_stmt_explanation = "Contract statements help you catching runtime errors by testing conditional expressions. \\ There exist three kinds of contracts: • require statement is executed at the beginning of a function call • ensure statement is executed at the end of a function call • invariant statement is executed in both moments \\ Here's an example: \\2 `// requires the sequence to be sorted \\2 function bsearch(seq: [i32]) require sorted(seq) {...}` \\ Another context where to test an invariant is for loop: \\2 `// test wheter `i < n` at the beginning and ending of each iteration \\2 for i < 7 invariant i < n {...}`";
        static const std::string use_decl_explanation = "Use declaration is useful to import types, functions or variables definitions from other libraryes. In the same library it is redundant to import such definitions. Here's an example: \\2 `use ns.math.SquareMatrix` // imports SquareMatrix type definition \\ However invisible declarations, marked with `hide`, can't be exported.";
        static const std::string extend_decl_explanation = "An extend block must be used in order to extend a data type with some features like functions, properties, constants and type definitions. You can even declare what shared behaviours your type may inherit. Here's an example: \\2 `extend Rect as Shape { \\4 // function to create a rectangle, callable as Rect.create(b, h) \\4 create(base: f32, height: f32) Rect = Rect(base: base, height: height) \\4 // polymorphic property to compute area \\4 .area(r: *Rect) f32 = r.base * r.height \\4 ... \\2 }` \\ The interesting thing of defining functions inside an extend block is that those functions can be called as methods (like `object.method(args)`) on a type instance.";
        static const std::string behaviour_decl_explanation = "Polymorphic behaviour is achieved by extending a shared behaviour. In order to behave polymorphically an object must be passed by pointer. Upcasting to the base abstract (pointer) type is also possible. A shared behaviour is composed of functions and properties. \\ Here's an example of behaviour definition: \\2 `// behaviour for a Shape object \\2 behaviour Shape { \\4 // polymorphic property \\4 .area(s: *Shape) f32 \\2 }` \\ Now types `Triangle` and `Circle` will extend `Shape` shared behaviour: \\2 `// Triangle defined as Triangle(base: f32, height: f32) \\2 extend Triangle as Shape { \\4 .area(t: *Triangle) f32 = (t.base * t.height) / 2 \\2 } \\2 // Circle defined as Circle(radius: f32) \\2 extend Circle as Shape { \\4 .area(c: *Circle) f32 = 3.14159 * c.radius ** 2 \\2 }`";
        static const std::string concept_decl_explanation = "A concept definition is like a set of contraints (prototypes) over a type. For example, type `T` is considered to be comparable if it defines all compare operations. Here's what it would like: \\2 `concept(T) Compare { \\4 equal(left: T, right: T) bool \\4 not_equal(left: T, right: T) bool \\4 less(left: T, right: T) bool \\4 greater(left: T, right: T) bool \\4 less_equal(left: T, right: T) bool \\4 greater_equal(left: T, right: T) bool \\2 }` \\ Now if we define a type `U` which defines this functions in its extend block, then `Compare(U)` will always be satisfied as a constraint.";
        static const std::string extern_decl_explanation = "An extern block contains prototypes of functions defined in another compilation unit using C application binary interface. Here's an example: \\2 `extern { \\4 // this function is defined in another compilation unit and will have external linkage \\4 memcpy(dest: *[u8], src: *[u8], n: usize) \\2 }`";
        static const std::string var_decl_explanation = "To preserve state you can use variables or constants. Here's the differences: • constants, declared as `const`, are evaluated at compile time and cannot be modified • variables, declared as `val`, are evaluated at run-time and can have different lifetimes \\ When a variable is defined as static then it will live as longer as the program lives, otherwise an automatic variable's lifetime is bounded to its enclosing block. Moreover when a variable is defined as: • `val` then it means it cannot be reassigned and its value cannot be changed • `mutable val` then it may be reassigned and its value may be changed \\ Here's an example: \\2 `// variable whose value may be changed \\2 mutable val person = Person(name: \"John Doe\", age: 35) \\2 // compile-time constant \\2 const SIZE: usize = 2**32 \\2 // tupled variable declaration for destructuring \\2 val (name, age) = people.get(i)`";
        static const std::string test_decl_explanation = "Test are very useful for testing singular units of code. Here's an example: \\2 `// this will be executed in non-release mode for unit testing \\2 test my_test {...}`";
        static const std::string source_unit_decl_explanation = "At top level context only certain declarations are allowed, including: • library declaration • use declarations • variable or constant declarations • type declarations (including extend, behaviour and concept) • function declarations • test declaration";
        static const std::string local_decl_explanation = "Inside local context only certain statements are allowed, including: • use declarations • variable or constant declarations • type declarations (excluding behaviour and extend) • function declarations • test declaration • jump declaration (break or continue only inside loop) • expression statements (including if, for, when, blocks and others)";
    }

    parser::guard::guard(parser* parser) : parser_(*parser) 
    {
        if (parser_.state_.recursion_depth >= max_depth) {
            auto builder = diagnostic::builder()
                        .severity(diagnostic::severity::error)
                        .location(parser_.current().location())
                        .message("Max recursion depth reached during parsing, f*cking hell!")
                        .explanation(impl::guard_explanation)
                        .highlight(source_range(parser_.current().range()), "here");
            
            parser_.abort(builder.build());
        }

        ++parser_.state_.recursion_depth;
    }
    
    parser::guard::~guard()
    {
        --parser_.state_.recursion_depth;
    }

    parser::parser(const tokenizer::tokens& tokens, source_file& file, diagnostic_publisher& publisher) :
        state_ { 0, tokens.cbegin() },
        tokens_(tokens),
        file_(file),
        publisher_(publisher)
    {}

    void parser::backtrack(state state) { state_ = state; }

    void parser::abort(diagnostic diag)
    {
        if (!silent()) publisher_.publish(diag);
        throw syntax_error();
    }

    void parser::advance()
    {
        if (!eof()) ++state_.iter;
    }
    
    token parser::previous() const
    {
        if (state_.iter != tokens_.cbegin()) {
            auto temp = state_.iter;
            return *(--temp);
        }
        return *state_.iter;
    }
        
    token parser::current() const { return *state_.iter; }
        
    token parser::next(int offset) const
    {
        if (!eof()) {
            auto temp = state_.iter;
            auto end = tokens_.cend();
            --end;
            while (temp != end && offset--) ++temp;
            return *temp;
        }
        return *state_.iter;
    }
    
    bool parser::eof() const { return current().is(token::kind::eof); }

    bool parser::match(enum token::kind k)
    {
        if (current().is(k)) {
            advance();
            return true;
        }

        return false;
    }

    token parser::consume(enum token::kind kind, const std::string& expected, const std::string& message, const std::string& explanation, bool advance_over)
    {
        if (!current().is(kind)) {
            auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(current().location())
                            .message(message)
                            .explanation(explanation)
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
            
            abort(builder.build());
        }

        token tok = current();
        if (advance_over) advance();
        return tok;
    }

    token parser::parenthesis(enum token::kind kind, const std::string& message, const std::string& explanation, token opening, bool advance_over)
    {
        std::string paren;
        std::string expected;

        switch (kind) {
            case token::kind::right_parenthesis:
                expected = ")";
                paren = "parenthesis";
                break;
            case token::kind::right_bracket:
                expected = "]";
                paren = "brackets";
                break;
            case token::kind::right_brace:
                expected = "}";
                paren = "braces";
                break;
            default:
                throw std::invalid_argument("parser::parenthesis(): invalid closing parenthesis kind");
        }

        if (!current().is(kind)) {
            auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(current().location())
                            .message(message)
                            .explanation(explanation)
                            .small(true)
                            .highlight(current().range(), diagnostic::format("expected `$`", expected))
                            .insertion(source_range(previous().range().end(), 1), expected, diagnostic::format("I suggest putting `$` to close $", expected, paren));

            if (!eof()) {
                if (current().is_keyword()) {
                    builder.message(diagnostic::format("$ I found keyword `$` instead.", message, current().lexeme()));
                }
                else {
                    builder.message(diagnostic::format("$ I found `$` instead.", message, current().lexeme()));
                }
            }
            
            abort(builder.build());
        }

        token tok = current();
        if (advance_over) advance();
        return tok;
    }

    void parser::report(source_range highlight, const std::string& message, const std::string& explanation, const std::string& inlined)
    {
        auto builder = diagnostic::builder()
                        .severity(diagnostic::severity::error)
                        .location(highlight.begin())
                        .message(message)
                        .explanation(explanation)
                        .highlight(highlight, inlined);
        
        publisher_.publish(builder.build());
    }

    void parser::fatal(source_range highlight, const std::string& message, const std::string& explanation, const std::string& inlined)
    {
        auto builder = diagnostic::builder()
                        .severity(diagnostic::severity::error)
                        .location(highlight.begin())
                        .message(message)
                        .explanation(explanation)
                        .highlight(highlight, inlined);
        
        abort(builder.build());
    }

    ast::pointer<ast::expression> parser::primary_expression()
    {
        guard guard(this);
        state saved = state_;

        if (current().is_literal()) {
            ast::pointer<ast::expression> result = ast::create<ast::literal_expression>(current());
            advance();
            return result;
        }
        else if (current().is(token::kind::identifier)) {
            return identifier_expression();
        }
        else if (match(token::kind::left_bracket)) {
            token open = previous();
            ast::pointers<ast::expression> elements;

            if (!current().is(token::kind::right_bracket)) {
                ast::pointer<ast::expression> elem = expect(expression(), "expression", "I need an array element here, idiot!", impl::array_init_explanation);
                elements.push_back(elem);
                
                if (match(token::kind::colon)) {
                    ast::pointer<ast::expression> size = expect(expression(), "expression", "After `:` I expect to see the array length, dammit!", impl::array_init_explanation);
                    parenthesis(token::kind::right_bracket, "You forgot `]` in array expression, idiot!", impl::array_init_explanation, open);
                    return ast::create<ast::array_sized_expression>(source_range(saved.iter->location(), previous().range().end()), elem, size);
                }
                else if (match(token::kind::comma)) {
                    do {
                        if (elements.size() >= guard::max_elements) {
                            auto builder = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(current().location())
                                        .message("Max number of elements inside array expression reached during parsing, f*cking hell!")
                                        .explanation(impl::guard_explanation)
                                        .highlight(source_range(current().range()), "here");
                            
                            abort(builder.build());
                        }

                        elem = expect(expression(), "expression", diagnostic::format("After `$` I need a bloody element inside the array.", previous().lexeme()), impl::array_init_explanation);
                        elements.push_back(elem);
                    }
                    while (match(token::kind::comma));
                }
            }

            parenthesis(token::kind::right_bracket, "You forgot `]` in array expression, idiot!", impl::array_init_explanation, open);

            return ast::create<ast::array_expression>(source_range(saved.iter->location(), previous().range().end()), elements);
        }
        else if (match(token::kind::left_parenthesis)) {
            token open = previous();
            ast::pointer<ast::expression> expr = nullptr;
            
            if (current().is(token::kind::identifier) && next().is(token::kind::colon)) {
                std::vector<ast::record_expression::initializer> inits;

                do {
                    if (inits.size() >= guard::max_parameters) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("Max number of parameters reached during parsing, f*cking hell!")
                                    .explanation(impl::guard_explanation)
                                    .highlight(source_range(current().range()), "here");
                        
                        abort(builder.build());
                    }

                    token field = consume(token::kind::identifier, "name", diagnostic::format("I expect field name after `$`, idiot!", previous().lexeme()), impl::record_init_explanation);
                    consume(token::kind::colon, "`:`", diagnostic::format("I expect `:` after field `$`, dumb*ss!", field.lexeme()), impl::record_init_explanation);
                    ast::pointer<ast::expression> init = expect(expression(), "expression", "I expect field value after `:`, don't you think?", impl::record_init_explanation);
                    inits.emplace_back(field, init);
                }
                while (match(token::kind::comma));

                parenthesis(token::kind::right_parenthesis, "You forgot `)` in record expression?", impl::record_init_explanation, open);

                expr = ast::create<ast::record_expression>(source_range(saved.iter->location(), previous().range().end()), nullptr, inits);
            }
            else if (!current().is(token::kind::right_parenthesis)) {
                bool tuple = false;
                ast::pointers<ast::expression> elements;
                ast::pointer<ast::expression> elem = nullptr;

                do {
                    if (elements.size() >= guard::max_elements) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("Max number of elements inside tuple expression reached during parsing, f*cking hell!")
                                    .explanation(impl::guard_explanation)
                                    .highlight(source_range(current().range()), "here");
                        
                        abort(builder.build());
                    }

                    if (previous().is(token::kind::comma)) {
                        elem = expect(expression(), "expression", "I need an element after `,` inside the tuple, dammit!", impl::tuple_init_explanation);
                    }
                    else {
                        elem = expect(expression(), "expression", "I need an expression inside these parenthesis, idiot!", impl::parenthesis_expr_explanation);
                    }

                    elements.push_back(elem);

                    if (current().is(token::kind::comma) && next().is(token::kind::right_parenthesis)) {
                        advance();
                        tuple = true;
                        break;
                    }
                }
                while (match(token::kind::comma));

                parenthesis(token::kind::right_parenthesis, diagnostic::format("You forgot `)` in $?", elements.size() == 1 ? "parenthesis expression" : "tuple expression"), elements.size() == 1 ? impl::parenthesis_expr_explanation : impl::tuple_init_explanation, open);

                if (elements.size() == 1 && !tuple) {
                    expr = ast::create<ast::parenthesis_expression>(source_range(saved.iter->location(), previous().range().end()), elements.front());
                }
                else {
                    expr = ast::create<ast::tuple_expression>(source_range(saved.iter->location(), previous().range().end()), elements);
                }
            }
            else if (match(token::kind::right_parenthesis)) {
                expr = ast::create<ast::tuple_expression>(source_range(saved.iter->location(), previous().range().end()), ast::pointers<ast::expression>());
            }

            return expr;
        }
        else if (match(token::kind::function_kw)) {
            ast::pointers<ast::declaration> params;
            ast::pointer<ast::expression> return_type = nullptr;

            token open = consume(token::kind::left_parenthesis, "`(`", "You forgot `(` in function expression, idiot!", impl::function_expr_explanation);

            if (!current().is(token::kind::right_parenthesis)) {
                ast::pointer<ast::declaration> param = nullptr;

                do {
                    if (params.size() >= guard::max_parameters) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("Max number of parameters reached during parsing, f*cking hell!")
                                    .explanation(impl::guard_explanation)
                                    .highlight(source_range(current().range()), "here");
                        
                        abort(builder.build());
                    }

                    param = expect(parameter_declaration(), "declaration", "I am expecting a parameter declaration inside function!", impl::function_expr_explanation);
                    params.push_back(param);
                }
                while (match(token::kind::comma));
            }

            parenthesis(token::kind::right_parenthesis, "You forgot `)` in function expression, idiot!", impl::function_expr_explanation, open);

            source_range result_range(state_.iter->location(), 0);
            
            if (!current().is(token::kind::left_brace) && !current().is(token::kind::equal)) {
                return_type = expect(type_expression(), "type", "I expect function return type here, pr*ck!", impl::function_expr_explanation);
                result_range = return_type->range();
            }

            ast::pointer<ast::expression> body = nullptr;

            if (match(token::kind::equal)) {
                body = expect(expression(), "expression", "I need the damn function body here, clown!", impl::function_expr_explanation);
            }
            else {
                body = expect(block_expression(), "body", "I need the damn function body here, clown!", impl::function_expr_explanation);
            }

            auto result = ast::create<ast::function_expression>(source_range(saved.iter->location(), previous().range().end()), params, return_type, body);
            result->result_range() = result_range;
            return result;
        }

        return nullptr;
    }

    ast::pointer<ast::expression> parser::function_type_expression()
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::function_kw)) {
            ast::pointers<ast::expression> param_types;
            ast::pointer<ast::expression> return_type = nullptr;

            token open = consume(token::kind::left_parenthesis, "`(`", "You forgot `(` in function type, idot!", impl::function_type_expr_explanation);

            if (!current().is(token::kind::right_parenthesis)) {
                ast::pointer<ast::expression> param_type;

                do {
                    if (param_types.size() >= guard::max_parameters) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("Max number of parameters reached during parsing, f*cking hell!")
                                    .explanation(impl::guard_explanation)
                                    .highlight(source_range(current().range()), "here");
                        
                        abort(builder.build());
                    }

                    bool is_mutable = match(token::kind::mutable_kw);
                    param_type = expect(type_expression(), "type", diagnostic::format("I can't find the damn parameter type after `$` inside function!", previous().lexeme()), impl::function_type_expr_explanation);
                    std::dynamic_pointer_cast<ast::type_expression>(param_type)->set_mutable(is_mutable);
                    param_types.push_back(param_type);
                }
                while (match(token::kind::comma));
            }

            parenthesis(token::kind::right_parenthesis, "You forgot `)` in function type, dammit!", impl::function_type_expr_explanation, open);

            return_type = type_expression();

            return ast::create<ast::function_type_expression>(source_range(saved.iter->location(), previous().range().end()), param_types, return_type);
        }

        return nullptr;
    }
    
    ast::pointer<ast::expression> parser::array_type_expression()
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::left_bracket)) {
            token open = previous();
            ast::pointer<ast::expression> element_type = expect(type_expression(), "type", "I need element type after `[` in array type, dammit!", impl::array_type_expr_explanation);
            ast::pointer<ast::expression> size = nullptr;
            
            if (match(token::kind::colon)) {
                size = expect(expression(), "length", "After `:` I expect to see the array length, don't you think?", impl::array_type_expr_explanation);
            }

            parenthesis(token::kind::right_bracket, "You forgot `]` in array type, dumb*ss", impl::array_type_expr_explanation, open);

            return ast::create<ast::array_type_expression>(source_range(saved.iter->location(), previous().range().end()), element_type, size);
        }

        return nullptr;
    }

    ast::pointer<ast::expression> parser::path_type_expression()
    {
        guard guard(this);
        state saved = state_;
        ast::pointer<ast::expression> result = identifier_expression(true);

        if (!result) return nullptr;

        if (!current().is(token::kind::dot)) return ast::create<ast::path_type_expression>(result->range(), result, nullptr);

        while (match(token::kind::dot)) {
            auto member = expect(identifier_expression(true), "type", "I was expecting a type after `.` in path, idiot!", impl::path_explanation);
            result = ast::create<ast::path_type_expression>(source_range(saved.iter->location(), previous().range().end()), result, member);
        }

        return result;
    }
    
    ast::pointer<ast::expression> parser::type_expression()
    {
        guard guard(this);
        state saved = state_;
        ast::pointer<ast::expression> single = single_type_expression();

        if (!single) return nullptr;

        if (!match(token::kind::line)) return single;

        ast::pointers<ast::expression> types { single };

        do {
            if (types.size() >= guard::max_elements) {
                auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(current().location())
                            .message("Max number of types inside variant reached during parsing, f*cking hell!")
                            .explanation(impl::guard_explanation)
                            .highlight(source_range(current().range()), "here");
                
                abort(builder.build());
            }

            types.push_back(expect(single_type_expression(), "type", "I need a type after `|` in variant type, idiot!", impl::variant_type_expr_explanation));
        }
        while (match(token::kind::line));

        return ast::create<ast::variant_type_expression>(source_range(saved.iter->location(), previous().range().end()), types);
    }

    ast::pointer<ast::expression> parser::single_type_expression()
    {
        guard guard(this);
        state saved = state_;

        if (current().is(token::kind::identifier)) {
            return path_type_expression();
        }
        else if (current().is(token::kind::function_kw)) {
            return function_type_expression();
        }
        else if (current().is(token::kind::left_bracket)) {
            return array_type_expression();
        }
        else if (match(token::kind::left_parenthesis)) {
            token open = previous();
            if (current().is(token::kind::identifier) && next().is(token::kind::colon)) {
                ast::pointers<ast::declaration> fields;

                do {
                    if (fields.size() >= guard::max_elements) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("Max number of fields inside record expression reached during parsing, f*cking hell!")
                                    .explanation(impl::guard_explanation)
                                    .highlight(source_range(current().range()), "here");
                        
                        abort(builder.build());
                    }

                    auto field = expect(field_declaration(), "field", "After `:` I need field type in record type!", impl::record_type_expr_explanation);
                    fields.push_back(field);
                }
                while (match(token::kind::comma));

                parenthesis(token::kind::right_parenthesis, "You forgot `)` in record type, holy sh*t!", impl::record_type_expr_explanation, open);

                return ast::create<ast::record_type_expression>(source_range(saved.iter->location(), previous().range().end()), fields);
            }
            else {
                ast::pointers<ast::expression> types;

                if (!current().is(token::kind::right_parenthesis)) {
                    ast::pointer<ast::expression> element_type = nullptr;
                    do {
                        if (types.size() >= guard::max_elements) {
                            auto builder = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(current().location())
                                        .message("Max number of elements inside tuple expression reached during parsing, f*cking hell!")
                                        .explanation(impl::guard_explanation)
                                        .highlight(source_range(current().range()), "here");
                            
                            abort(builder.build());
                        }

                        element_type = expect(type_expression(), "type", diagnostic::format("After `$` I need an element type in tuple type!", previous().lexeme()), impl::tuple_type_expr_explanation);
                        types.push_back(element_type);
                    }
                    while (match(token::kind::comma));
                }

                parenthesis(token::kind::right_parenthesis, "You forgot `)` in tuple type, holy sh*t!", impl::tuple_type_expr_explanation, open);

                return ast::create<ast::tuple_type_expression>(source_range(saved.iter->location(), previous().range().end()), types);
            }
        }
        else if (match(token::kind::star)) {
            ast::pointer<ast::expression> base_type = expect(single_type_expression(), "type", "I need the base type after `*` in pointer type, dammit!", impl::pointer_type_expr_explanation);
            return ast::create<ast::pointer_type_expression>(source_range(saved.iter->location(), previous().range().end()), base_type);
        }
        else if (match(token::kind::integer_literal)) {
            return ast::create<ast::bit_field_type_expression>(previous());
        }

        return nullptr;
    }

    ast::pointer<ast::expression> parser::field_type_expression()
    {
        guard guard(this);

        if (match(token::kind::integer_literal)) {
            return ast::create<ast::bit_field_type_expression>(previous());
        }
        
        return type_expression();
    }

    ast::pointer<ast::expression> parser::block_expression()
    {
        guard guard(this);
        state saved = state_;
        ast::pointers<ast::statement> stmts;

        if (match(token::kind::left_brace)) {
            token open = previous();

            while (!eof() && !current().is(token::kind::right_brace)) {
                if (stmts.size() >= guard::max_statements) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(current().location())
                                .message("Max number of statements reached during parsing, f*cking hell!")
                                .explanation(impl::guard_explanation)
                                .highlight(source_range(current().range()), "here");
                    
                    abort(builder.build());
                }

                ast::pointer<ast::statement> stmt = expect(statement(), "statement", "I need a f*cking statement here! What's this?", impl::block_expr_explanation);
                stmts.push_back(stmt);
            }

            parenthesis(token::kind::right_brace, "You forgot `}` in a block, idiot!", impl::block_expr_explanation, open);

            return ast::create<ast::block_expression>(source_range(saved.iter->location(), previous().range().end()), stmts);
        }

        return nullptr;
    }

    ast::pointer<ast::expression> parser::identifier_expression(bool istype)
    {
        if (match(token::kind::identifier)) {
            token name = previous();
            ast::pointers<ast::expression> generics;
            bool generic = false;

            if (name.eol);
            else if (istype) {
                generic = generic_arguments_list(generics);
            }
            else if (match(token::kind::bang) && !(generic = generic_arguments_list(generics))) {
                consume(token::kind::left_parenthesis, "`(`", diagnostic::format("I need `(` for generic arguments list after `!`, dammit!", previous().lexeme()), impl::generic_param_decl_explanation);
            }

            return ast::create<ast::identifier_expression>(source_range(name.location(), previous().range().end()), name, generics, generic);
        }

        return nullptr;
    }
    
    ast::pointer<ast::expression> parser::postfix_expression()
    {
        guard guard(this);
        state saved = state_;
        ast::pointer<ast::expression> expr = primary_expression();
        bool err = false;

        if (!expr) return nullptr;
        
        while (!previous().eol) {
            if (match(token::kind::plus_plus)) {
                if (!expr->is_assignable()) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(current().location())
                                .message("The left operand is not f*cking assignable, idiot!")
                                .highlight(expr->range())
                                .explanation(impl::assign_stmt_explanation);
            
                    publisher_.publish(builder.build());
                    expr->invalid(true);
                    err = true;
                }

                expr = ast::create<ast::postfix_expression>(source_range(saved.iter->location(), previous().range().end()), expr, previous());
                expr->invalid(err);
            }
            else if (match(token::kind::minus_minus)) {
                if (!expr->is_assignable()) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(current().location())
                                .message("The left operand is not f*cking assignable, idiot!")
                                .highlight(expr->range())
                                .explanation(impl::assign_stmt_explanation);
            
                    publisher_.publish(builder.build());
                    expr->invalid(true);
                    err = true;
                }

                expr = ast::create<ast::postfix_expression>(source_range(saved.iter->location(), previous().range().end()), expr, previous());
                expr->invalid(true);
            }
            else if (match(token::kind::dot)) {
                if (current().is(token::kind::identifier)) {
                    ast::pointer<ast::expression> member = identifier_expression();
                    expr = ast::create<ast::member_expression>(source_range(saved.iter->location(), previous().range().end()), expr, member);
                }
                else if (match(token::kind::integer_literal)) {
                    expr = ast::create<ast::tuple_index_expression>(source_range(saved.iter->location(), previous().range().end()), expr, previous());
                }
                else {
                    consume(token::kind::integer_literal, "name or index", "I need field name or tuple index after `.`, don't you think?", impl::field_expr_explanation);
                }
            }
            else if (current().is(token::kind::left_parenthesis) && next().is(token::kind::identifier) && next(2).is(token::kind::colon)) {
                token open = current();
                std::vector<ast::record_expression::initializer> inits;
                advance();

                do {
                    if (inits.size() >= guard::max_parameters) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("Max number of elements inside record expression reached during parsing, f*cking hell!")
                                    .explanation(impl::guard_explanation)
                                    .highlight(source_range(current().range()), "here");
                        
                        abort(builder.build());
                    }

                    token field = consume(token::kind::identifier, "name", diagnostic::format("I expect field name after `$`, idiot!", previous().lexeme()), impl::record_init_explanation);
                    consume(token::kind::colon, "`:`", diagnostic::format("I expect `:` after field `$`, don't you believe?", field.lexeme()), impl::record_init_explanation);
                    ast::pointer<ast::expression> init = expect(expression(), "expression", "I expect field value after `:`, don't you think?", impl::record_init_explanation);
                    inits.emplace_back(field, init);
                }
                while (match(token::kind::comma));

                parenthesis(token::kind::right_parenthesis, "You forgot `)` in record expression, idiot!", impl::record_init_explanation, open);
                bool err = false;

                if (!expr->is_path()) {
                    report(expr->range(), "I need a type name (like mother.Facker or Sacker!(Cock)) to construct an object instead of this sh*t!", "expected name", impl::record_init_explanation);
                    err = true;
                }

                expr = ast::create<ast::record_expression>(source_range(saved.iter->location(), previous().range().end()), expr, inits);
                expr->invalid(err);
            }
            else if (match(token::kind::left_parenthesis)) {
                token open = previous();
                ast::pointers<ast::expression> args;

                if (!current().is(token::kind::right_parenthesis)) {
                    ast::pointer<ast::expression> arg;

                    do {
                        if (args.size() >= guard::max_parameters) {
                            auto builder = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(current().location())
                                        .message("Max number of elements inside tuple expression reached during parsing, f*cking hell!")
                                        .explanation(impl::guard_explanation)
                                        .highlight(source_range(current().range()), "here");
                            
                            abort(builder.build());
                        }

                        arg = expect(expression(), "expression", diagnostic::format("I am expecting argument after `$` in function call, idiot!", previous().lexeme()), impl::function_call_explanation);
                        args.push_back(arg);
                    }
                    while (match(token::kind::comma));
                }

                parenthesis(token::kind::right_parenthesis, "You forgot `)` in function call, dammi!", impl::function_call_explanation, open);
                expr = ast::create<ast::call_expression>(source_range(saved.iter->location(), previous().range().end()), expr, args);
            }
            else if (match(token::kind::left_bracket)) {
                token open = previous();
                ast::pointer<ast::expression> index = expect(expression(), "expression", "I need array index after inside brackets, don't you think?", impl::array_index_expr_explanation);
                parenthesis(token::kind::right_bracket, "You forgot `]` in array index, holy sh*t!", impl::array_index_expr_explanation, open);
                expr = ast::create<ast::array_index_expression>(source_range(saved.iter->location(), previous().range().end()), expr, index);
            }
            else {
                break;
            }
        }

        return expr;
    }
    
    ast::pointer<ast::expression> parser::unary_expression()
    {
        guard guard(this);
        state saved = state_;

        token unary_operator;
        ast::pointer<ast::expression> expr = nullptr;

        switch (current().kind()) {
            case token::kind::plus:
            case token::kind::minus:
            case token::kind::tilde:
            case token::kind::bang:
            case token::kind::plus_plus:
            case token::kind::minus_minus:
            case token::kind::amp:
            case token::kind::star:
            {
                unary_operator = current();
                advance();
                expr = expect(unary_expression(), "expression", diagnostic::format("I was expecting to see an expression after unary operator `$`, don't you think?", unary_operator.lexeme()), impl::unary_operator_explanation);
                auto result = ast::create<ast::unary_expression>(source_range(saved.iter->location(), previous().range().end()), unary_operator, expr);
                if ((unary_operator.is(token::kind::plus_plus) || unary_operator.is(token::kind::minus_minus)) && !expr->is_assignable()) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(current().location())
                                .message("The right operand is not f*cking assignable, idiot!")
                                .highlight(expr->range())
                                .explanation(impl::assign_stmt_explanation);
            
                    publisher_.publish(builder.build());
                    expr->invalid(true);
                    result->invalid(true);
                }
                return result;
            }
            default:
                break;
        }

        return postfix_expression();
    }

    ast::pointer<ast::expression> parser::conversion_expression()
    {
        guard guard(this);
        ast::pointer<ast::expression> expr = unary_expression();
        // '**' operator is different from others because it's right associative
        while (expr && !previous().eol && match(token::kind::as_kw)) {
            token op = previous();
            ast::pointer<ast::expression> right = expect(type_expression(), "type", "I need a type in type conversion operation, b*tch!", impl::type_conversion_explanation);
            expr = ast::create<ast::binary_expression>(source_range(expr->range().begin(), right->range().end()), op, expr, right);
        }

        return expr;
    }
    
    ast::pointer<ast::expression> parser::power_expression()
    {
        guard guard(this);
        ast::pointer<ast::expression> expr = conversion_expression();
        // '**' operator is different from others because it's right associative
        while (expr && !previous().eol && (current().is(token::kind::star_star) || 
              (current().is(token::kind::star) && next().is(token::kind::star) && next().location().column == current().location().column + 1))) {
            token op;
            // '**' is spelled as two singular '*', '*' not to confuse with double pointer
            if (current().is(token::kind::star)) {
                op = token::builder()
                     .kind(token::kind::star_star)
                     .eol(next().eol)
                     .location(current().location())
                     .lexeme(utf8::span(current().lexeme().data(), 2, false))
                     .build();
                
                advance();
            }
            else {
                op = current();
            }

            advance();
            ast::pointer<ast::expression> right = expect(power_expression(), "expression", "I need an exponent after operator `**` in power expression, pr*ck!", impl::binary_expr_explanation);
            expr = ast::create<ast::binary_expression>(source_range(expr->range().begin(), right->range().end()), op, expr, right);
        }

        return expr;
    }

    ast::pointer<ast::expression> parser::multiplicative_expression()
    {
        guard guard(this);
        ast::pointer<ast::expression> expr = power_expression();

        while (expr && !previous().eol && ((current().is(token::kind::star) && (!next().is(token::kind::star) || next().location().column != current().location().column + 1)) || 
               current().is(token::kind::slash) || current().is(token::kind::percent))) {
            token op = current();
            advance();
            ast::pointer<ast::expression> right = expect(power_expression(), "expression", diagnostic::format("I need a right operand after operator `$`, don't you believe?", op.lexeme()), impl::binary_expr_explanation);
            expr = ast::create<ast::binary_expression>(source_range(expr->range().begin(), right->range().end()), op, expr, right);
        }

        return expr;
    }
    
    ast::pointer<ast::expression> parser::additive_expression()
    {
        guard guard(this);
        ast::pointer<ast::expression> expr = multiplicative_expression();

        while (expr && !previous().eol && (current().is(token::kind::plus) || current().is(token::kind::minus))) {
            token op = current();
            advance();
            ast::pointer<ast::expression> right = expect(multiplicative_expression(), "expression", diagnostic::format("I need a right operand after operator `$`, don't you believe?", op.lexeme()), impl::binary_expr_explanation);
            expr = ast::create<ast::binary_expression>(source_range(expr->range().begin(), right->range().end()), op, expr, right);
        }

        return expr;
    }

    ast::pointer<ast::expression> parser::shift_expression()
    {
        guard guard(this);
        ast::pointer<ast::expression> expr = additive_expression();

        while (expr && !previous().eol && (current().is(token::kind::less_less) || current().is(token::kind::greater_greater) || 
                       (current().is(token::kind::greater) && next().is(token::kind::greater) && next().location().column == current().location().column + 1))) {
            token op;
            // '>>' is spelled as two singular '>', '>' not to confuse with closing generics angle parenthesis
            if (current().is(token::kind::greater)) {
                op = token::builder()
                     .kind(token::kind::greater_greater)
                     .eol(next().eol)
                     .location(current().location())
                     .lexeme(utf8::span(current().lexeme().data(), 2, false))
                     .build();
                
                advance();
            }
            else {
                op = current();
            }

            advance();
            ast::pointer<ast::expression> right = expect(additive_expression(), "expression", diagnostic::format("I need a right operand after operator `$`, don't you believe?", op.lexeme()), impl::binary_expr_explanation);
            expr = ast::create<ast::binary_expression>(source_range(expr->range().begin(), right->range().end()), op, expr, right);
        }

        return expr;
    }

    ast::pointer<ast::expression> parser::and_expression()
    {
        guard guard(this);
        ast::pointer<ast::expression> expr = shift_expression();

        while (expr && !previous().eol && (current().is(token::kind::amp) && (!next().is(token::kind::amp) || next().location().column != current().location().column + 1))) {
            token op = current();
            advance();
            ast::pointer<ast::expression> right = expect(shift_expression(), "expression", diagnostic::format("I need a right operand after operator `$`, don't you believe?", op.lexeme()), impl::binary_expr_explanation);
            expr = ast::create<ast::binary_expression>(source_range(expr->range().begin(), right->range().end()), op, expr, right);
        }

        return expr;
    }

    ast::pointer<ast::expression> parser::xor_expression()
    {
        guard guard(this);
        ast::pointer<ast::expression> expr = and_expression();

        while (expr && !previous().eol && current().is(token::kind::caret)) {
            token op = current();
            advance();
            ast::pointer<ast::expression> right = expect(and_expression(), "expression", diagnostic::format("I need a right operand after operator `$`, don't you believe?", op.lexeme()), impl::binary_expr_explanation);
            expr = ast::create<ast::binary_expression>(source_range(expr->range().begin(), right->range().end()), op, expr, right);
        }

        return expr;
    }

    ast::pointer<ast::expression> parser::or_expression()
    {
        guard guard(this);

        ast::pointer<ast::expression> expr = xor_expression();

        while (expr && !previous().eol && current().is(token::kind::line)) {
            token op = current();
            advance();
            ast::pointer<ast::expression> right = expect(xor_expression(), "expression", diagnostic::format("I need a right operand after operator `$`, don't you believe?", op.lexeme()), impl::binary_expr_explanation);
            expr = ast::create<ast::binary_expression>(source_range(expr->range().begin(), right->range().end()), op, expr, right);
        }

        return expr;
    }

    ast::pointer<ast::expression> parser::range_expression()
    {
        guard guard(this);

        ast::pointer<ast::expression> expr = or_expression();

        while (!previous().eol) {
            if (match(token::kind::dot_dot)) {
                token op = previous();
                ast::pointer<ast::expression> right = or_expression();
                source_location begin, end;

                if (expr) begin = expr->range().begin();
                else begin = op.location();

                if (right) end = right->range().end();
                else end = op.range().end();
                
                bool err = false;
                ast::range_expression* chained = dynamic_cast<ast::range_expression*>(expr.get());
                if (chained) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(op.location())
                                .message("You cannot chain range expressions, pr*ck!")
                                .explanation(impl::range_expr_explanation)
                                .highlight(chained->range_operator().range())
                                .highlight(op.range());
            
                    publisher_.publish(builder.build());
                    err = true;
                }

                expr = ast::create<ast::range_expression>(source_range(begin, end), op, expr, right);
                expr->invalid(err);
            }
            else if (match(token::kind::dot_dot_equal)) {
                token op = previous();
                ast::pointer<ast::expression> right = expect(or_expression(), "expression", "I need the end of the range after inclusive operator `..=`, idiot!", impl::range_expr_explanation);
                source_location begin, end;

                if (expr) begin = expr->range().begin();
                else begin = op.location();
                
                end = right->range().end();

                bool err = false;
                ast::range_expression* chained = dynamic_cast<ast::range_expression*>(expr.get());
                if (chained) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(op.location())
                                .message("You cannot chain range expressions, pr*ck!")
                                .explanation(impl::range_expr_explanation)
                                .highlight(chained->range_operator().range())
                                .highlight(op.range());

                    publisher_.publish(builder.build());
                    err = true;
                }

                expr = ast::create<ast::range_expression>(source_range(begin, end), op, expr, right);
                expr->invalid(err);
            }
            else {
                break;
            }
        }

        return expr;
    }

    ast::pointer<ast::expression> parser::comparison_expression()
    {
        guard guard(this);
        ast::pointer<ast::expression> expr = range_expression();

        while (expr && !previous().eol && (current().is(token::kind::less) || 
                        (current().is(token::kind::greater) && (!next().is(token::kind::greater) || next().location().column != current().location().column + 1)) || 
                        current().is(token::kind::less_equal) || current().is(token::kind::greater_equal))) {
            token op = current();
            advance();
            ast::pointer<ast::expression> right = expect(range_expression(), "expression", diagnostic::format("I need a right operand after operator `$`, don't you believe?", op.lexeme()), impl::binary_expr_explanation);
            bool err = false;
            ast::binary_expression* chained = dynamic_cast<ast::binary_expression*>(expr.get());
            
            if (chained && (chained->binary_operator().is(token::kind::less) || chained->binary_operator().is(token::kind::greater) ||
                            chained->binary_operator().is(token::kind::less_equal) || chained->binary_operator().is(token::kind::greater_equal))) {
                std::ostringstream fix;
                fix << "&& " << file_.range(chained->right()->range()) << " ";

                auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(op.location())
                            .message("You cannot chain comparison expressions, pr*ck!")
                            .explanation(impl::binary_expr_explanation)
                            .highlight(chained->binary_operator().range())
                            .highlight(op.range())
                            .insertion(op.range(), fix.str(), "If you want to compare the same expression then try this way");
        
                publisher_.publish(builder.build());
                err = true;
            }
            
            expr = ast::create<ast::binary_expression>(source_range(expr->range().begin(), right->range().end()), op, expr, right);
            expr->invalid(err);
        }

        return expr;
    }
    
    ast::pointer<ast::expression> parser::equality_expression()
    {
        guard guard(this);

        ast::pointer<ast::expression> expr = comparison_expression();

        while (expr && !previous().eol && (current().is(token::kind::equal_equal) || current().is(token::kind::bang_equal))) {
            token op = current();
            advance();
            
            ast::pointer<ast::expression> right = expect(comparison_expression(), "expression", diagnostic::format("I need a right operand after operator `$`, don't you believe?", op.lexeme()), impl::binary_expr_explanation);
            bool err = false;
            ast::binary_expression* chained = dynamic_cast<ast::binary_expression*>(expr.get());
            
            if (chained && (chained->binary_operator().is(token::kind::equal_equal) || chained->binary_operator().is(token::kind::bang_equal))) {
                std::ostringstream fix;
                fix << "&& " << file_.range(chained->right()->range()) << " ";

                auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(op.location())
                            .message("You cannot chain comparison expressions, pr*ck!")
                            .explanation(impl::binary_expr_explanation)
                            .highlight(chained->binary_operator().range())
                            .highlight(op.range())
                            .insertion(op.range(), fix.str(), "If you want to compare the same expression then try this way");
        
                publisher_.publish(builder.build());
                err = true;
            }
            
            expr = ast::create<ast::binary_expression>(source_range(expr->range().begin(), right->range().end()), op, expr, right);
            expr->invalid(err);
        }

        return expr;
    }

    ast::pointer<ast::expression> parser::logic_and_expression()
    {
        guard guard(this);

        ast::pointer<ast::expression> expr = equality_expression();

        while (expr && !previous().eol && ((current().is(token::kind::amp) && next().is(token::kind::amp) && next().location().column == current().location().column + 1))) {
            token op;
            // '&&' is spelled as two singular '&', '&' not to confuse with double reference
            if (current().is(token::kind::amp)) {
                op = token::builder()
                     .kind(token::kind::amp_amp)
                     .eol(next().eol)
                     .location(current().location())
                     .lexeme(utf8::span(current().lexeme().data(), 2, false))
                     .build();
                
                advance();
            }
            else {
                op = current();
            }

            advance();
            ast::pointer<ast::expression> right = expect(equality_expression(), "expression", diagnostic::format("I need a right operand after operator `$`, don't you believe?", op.lexeme()), impl::binary_expr_explanation);
            expr = ast::create<ast::binary_expression>(source_range(expr->range().begin(), right->range().end()), op, expr, right);
        }

        return expr;
    }
    
    ast::pointer<ast::expression> parser::logic_or_expression()
    {
        guard guard(this);
        ast::pointer<ast::expression> expr = logic_and_expression();

        while (expr && !previous().eol && current().is(token::kind::line_line)) {
            token op = current();
            advance();
            ast::pointer<ast::expression> right = expect(logic_and_expression(), "expression", diagnostic::format("I need a right operand after operator `$`, don't you believe?", op.lexeme()), impl::binary_expr_explanation);
            expr = ast::create<ast::binary_expression>(source_range(expr->range().begin(), right->range().end()), op, expr, right);
        }

        return expr;
    }

    ast::pointer<ast::expression> parser::primary_pattern_expression()
    {
        guard guard(this);
        state saved = state_;
        ast::pointer<ast::expression> expr = nullptr;

        if (current().is_literal()) {
            expr = ast::create<ast::literal_pattern_expression>(current());
            advance();

            while (current().is(token::kind::dot_dot) || current().is(token::kind::dot_dot_equal)) {
                if (match(token::kind::dot_dot)) {
                    token op = previous();
                    ast::pointer<ast::expression> right = primary_pattern_expression();
                    source_location begin, end;

                    if (expr) begin = expr->range().begin();
                    else begin = op.location();

                    if (right) end = right->range().end();
                    else end = op.range().end();
                    
                    bool err = false;
                    ast::range_pattern_expression* chained = dynamic_cast<ast::range_pattern_expression*>(expr.get());
                    if (chained) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(op.location())
                                    .message("You cannot chain range patterns, pr*ck!")
                                    .highlight(chained->range_operator().range())
                                    .highlight(op.range())
                                    .explanation(impl::pattern_expr_explanation);

                        publisher_.publish(builder.build());
                        err = true;
                    }

                    expr = ast::create<ast::range_pattern_expression>(source_range(begin, end), op, expr, right);
                    expr->invalid(err);
                }
                else if (match(token::kind::dot_dot_equal)) {
                    token op = previous();
                    ast::pointer<ast::expression> right = expect(primary_pattern_expression(), "expression", "I need the end of the range after inclusive operator `..=` inside range pattern, b*tch!", impl::pattern_expr_explanation);
                    source_location begin, end;

                    if (expr) begin = expr->range().begin();
                    else begin = op.location();
                    
                    end = right->range().end();

                    bool err = false;
                    ast::range_pattern_expression* chained = dynamic_cast<ast::range_pattern_expression*>(expr.get());
                    if (chained) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(op.location())
                                    .message("You cannot chain range patterns, pr*ck!")
                                    .highlight(chained->range_operator().range())
                                    .highlight(op.range())
                                    .explanation(impl::pattern_expr_explanation);
                
                        publisher_.publish(builder.build());
                        err = true;
                    }

                    expr = ast::create<ast::range_pattern_expression>(source_range(begin, end), op, expr, right);
                    expr->invalid(err);
                }
                
                auto end = std::dynamic_pointer_cast<ast::range_pattern_expression>(expr)->end();

                if (end && !std::dynamic_pointer_cast<ast::literal_pattern_expression>(end) && !std::dynamic_pointer_cast<ast::path_pattern_expression>(end)) {
                    report(end->range(), "Only literals or constants are allowed as range pattern bounds, not this sh*t!", impl::pattern_expr_explanation, "expected literal or constant");
                    expr->invalid(true);
                }
            }

            return expr;
        }
        else if (match(token::kind::ellipsis)) {
            return ast::create<ast::ignore_pattern_expression>(source_range(saved.iter->location(), previous().range().end()));
        }
        else if (current().is(token::kind::identifier)) {
            ast::pointer<ast::expression> path = identifier_expression();

            while (match(token::kind::dot)) {
                auto member = expect(identifier_expression(), "name", "I was expecting name in path, idiot!", impl::path_explanation);
                path = ast::create<ast::member_expression>(source_range(path->range().begin(), previous().range().end()), path, member);
            }

            if (match(token::kind::left_parenthesis)) {
                token open = previous();

                if (match(token::kind::right_parenthesis)) {
                    return ast::create<ast::record_pattern_expression>(source_range(saved.iter->location(), previous().range().end()), path, ast::pointers<ast::expression>());
                }

                if (current().is(token::kind::identifier) && next().is(token::kind::colon)) {
                    std::vector<ast::labeled_record_pattern_expression::initializer> fields;

                    do {
                        if (fields.size() >= guard::max_parameters) {
                            auto builder = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(current().location())
                                        .message("Max number of elements inside record expression reached during parsing, f*cking hell!")
                                        .explanation(impl::guard_explanation)
                                        .highlight(source_range(current().range()), "here");
                            
                            abort(builder.build());
                        }

                        token name = consume(token::kind::identifier, "name", diagnostic::format("I need field name after `,` inside record pattern, c*nt.", previous().lexeme()), impl::pattern_expr_explanation);
                        consume(token::kind::colon, "`:`", "I expect `:` after field name, don't you believe?", impl::pattern_expr_explanation);
                        ast::pointer<ast::expression> field = expect(primary_pattern_expression(), "pattern", diagnostic::format("I need a field after `$` inside record pattern.", previous().lexeme()), impl::pattern_expr_explanation);
                        
                        if (field->kind() == ast::kind::ignore_pattern_expression) {
                            report(field->range(), "You cannot use `...` pattern as field value in this case, idiot!", impl::pattern_expr_explanation, "expected value pattern");
                        }

                        fields.emplace_back(name, field);
                    }
                    while (match(token::kind::comma));

                    parenthesis(token::kind::right_parenthesis, "You forgot `)` in record pattern, clown!", impl::pattern_expr_explanation, open);

                    return ast::create<ast::labeled_record_pattern_expression>(source_range(saved.iter->location(), previous().range().end()), path, fields);
                }
                else {
                    ast::pointers<ast::expression> fields;

                    do {
                        if (fields.size() >= guard::max_parameters) {
                            auto builder = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(current().location())
                                        .message("Max number of elements inside record expression reached during parsing, f*cking hell!")
                                        .explanation(impl::guard_explanation)
                                        .highlight(source_range(current().range()), "here");
                            
                            abort(builder.build());
                        }

                        ast::pointer<ast::expression> field = expect(primary_pattern_expression(), "pattern", diagnostic::format("I need a field after `$` inside record pattern.", previous().lexeme()), impl::pattern_expr_explanation);
                        fields.push_back(field);
                    }
                    while (match(token::kind::comma));

                    parenthesis(token::kind::right_parenthesis, "You forgot `)` in record pattern, clown!", impl::pattern_expr_explanation, open);

                    return ast::create<ast::record_pattern_expression>(source_range(saved.iter->location(), previous().range().end()), path, fields);
                }
            }
            
            expr = ast::create<ast::path_pattern_expression>(path);

            while (current().is(token::kind::dot_dot) || current().is(token::kind::dot_dot_equal)) {
                if (match(token::kind::dot_dot)) {
                    token op = previous();
                    ast::pointer<ast::expression> right = primary_pattern_expression();
                    source_location begin, end;

                    if (expr) begin = expr->range().begin();
                    else begin = op.location();

                    if (right) end = right->range().end();
                    else end = op.range().end();
                    
                    bool err = false;
                    ast::range_pattern_expression* chained = dynamic_cast<ast::range_pattern_expression*>(expr.get());
                    if (chained) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(op.location())
                                    .message("You cannot chain range patterns, pr*ck!")
                                    .highlight(chained->range_operator().range())
                                    .highlight(op.range())
                                    .explanation(impl::pattern_expr_explanation);

                        publisher_.publish(builder.build());
                        err = true;
                    }

                    expr = ast::create<ast::range_pattern_expression>(source_range(begin, end), op, expr, right);
                    expr->invalid(err);
                }
                else if (match(token::kind::dot_dot_equal)) {
                    token op = previous();
                    ast::pointer<ast::expression> right = expect(primary_pattern_expression(), "expression", "I need the end of the range after inclusive operator `..=` inside range pattern, b*tch!", impl::pattern_expr_explanation);
                    source_location begin, end;

                    if (expr) begin = expr->range().begin();
                    else begin = op.location();
                    
                    end = right->range().end();

                    bool err = false;
                    ast::range_pattern_expression* chained = dynamic_cast<ast::range_pattern_expression*>(expr.get());
                    if (chained) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(op.location())
                                    .message("You cannot chain range patterns, pr*ck!")
                                    .highlight(chained->range_operator().range())
                                    .highlight(op.range())
                                    .explanation(impl::pattern_expr_explanation);
                
                        publisher_.publish(builder.build());
                        err = true;
                    }

                    expr = ast::create<ast::range_pattern_expression>(source_range(begin, end), op, expr, right);
                    expr->invalid(err);
                }
                
                auto end = std::dynamic_pointer_cast<ast::range_pattern_expression>(expr)->end();

                if (end && !std::dynamic_pointer_cast<ast::literal_pattern_expression>(end) && !std::dynamic_pointer_cast<ast::path_pattern_expression>(end)) {
                    report(end->range(), "Only literals or constants are allowed as range pattern bounds, not this sh*t!", impl::pattern_expr_explanation, "expected literal or constant");
                    expr->invalid(true);
                }
            }

            return expr;
        }
        else if (match(token::kind::left_bracket)) {
            token open = previous();
            ast::pointers<ast::expression> elements;

            if (!current().is(token::kind::right_bracket)) {
                ast::pointer<ast::expression> elem = nullptr;

                do {
                    if (elements.size() >= guard::max_elements) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("Max number of elements inside array expression reached during parsing, f*cking hell!")
                                    .explanation(impl::guard_explanation)
                                    .highlight(source_range(current().range()), "here");
                        
                        abort(builder.build());
                    }

                    elem = expect(primary_pattern_expression(), "pattern", diagnostic::format("I need an element after `$` inside array pattern.", previous().lexeme()), impl::pattern_expr_explanation);
                    elements.push_back(elem);
                }
                while (match(token::kind::comma));
            }

            parenthesis(token::kind::right_bracket, "You forgot `]` in array pattern", impl::parenthesis_expr_explanation, open);

            return ast::create<ast::array_pattern_expression>(source_range(saved.iter->location(), previous().range().end()), elements);
        }
        else if (match(token::kind::left_parenthesis)) {
            token open = previous();
            ast::pointers<ast::expression> elements;
            
            if (!current().is(token::kind::right_parenthesis)) {
                ast::pointer<ast::expression> elem = nullptr;

                do {
                    if (elements.size() >= guard::max_elements) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("Max number of elements inside tuple expression reached during parsing, f*cking hell!")
                                    .explanation(impl::guard_explanation)
                                    .highlight(source_range(current().range()), "here");
                        
                        abort(builder.build());
                    }

                    elem = expect(primary_pattern_expression(), "pattern", diagnostic::format("I need an element after `$` inside tuple pattern.", previous().lexeme()), impl::pattern_expr_explanation);
                    elements.push_back(elem);

                    if (current().is(token::kind::comma) && next().is(token::kind::right_parenthesis)) {
                        advance();
                        break;
                    }
                }
                while (match(token::kind::comma));
            }

            parenthesis(token::kind::right_parenthesis, "You forgot `)` in tuple pattern", impl::pattern_expr_explanation, open);

            return ast::create<ast::tuple_pattern_expression>(source_range(saved.iter->location(), previous().range().end()), elements);
        }
        else while (current().is(token::kind::dot_dot) || current().is(token::kind::dot_dot_equal)) {
            if (match(token::kind::dot_dot)) {
                token op = previous();
                ast::pointer<ast::expression> right = primary_pattern_expression();
                source_location begin, end;

                if (expr) begin = expr->range().begin();
                else begin = op.location();

                if (right) end = right->range().end();
                else end = op.range().end();
                
                bool err = false;
                ast::range_pattern_expression* chained = dynamic_cast<ast::range_pattern_expression*>(expr.get());
                if (chained) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(op.location())
                                .message("You cannot chain range patterns, pr*ck!")
                                .highlight(chained->range_operator().range())
                                .highlight(op.range())
                                .explanation(impl::pattern_expr_explanation);

                    publisher_.publish(builder.build());
                    err = true;
                }

                expr = ast::create<ast::range_pattern_expression>(source_range(begin, end), op, expr, right);
                expr->invalid(err);
            }
            else if (match(token::kind::dot_dot_equal)) {
                token op = previous();
                ast::pointer<ast::expression> right = expect(primary_pattern_expression(), "expression", "I need the end of the range after inclusive operator `..=` inside range pattern, b*tch!", impl::pattern_expr_explanation);
                source_location begin, end;

                if (expr) begin = expr->range().begin();
                else begin = op.location();
                
                end = right->range().end();

                bool err = false;
                ast::range_pattern_expression* chained = dynamic_cast<ast::range_pattern_expression*>(expr.get());
                if (chained) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(op.location())
                                .message("You cannot chain range patterns, pr*ck!")
                                .highlight(chained->range_operator().range())
                                .highlight(op.range())
                                .explanation(impl::pattern_expr_explanation);
            
                    publisher_.publish(builder.build());
                    err = true;
                }

                expr = ast::create<ast::range_pattern_expression>(source_range(begin, end), op, expr, right);
                expr->invalid(err);
            }
            
            auto end = std::dynamic_pointer_cast<ast::range_pattern_expression>(expr)->end();

            if (end && !std::dynamic_pointer_cast<ast::literal_pattern_expression>(end) && !std::dynamic_pointer_cast<ast::path_pattern_expression>(end)) {
                report(end->range(), "Only literals or constants are allowed as range pattern bounds, not this sh*t!", impl::pattern_expr_explanation, "expected literal or constant");
                expr->invalid(true);
            }
        }

        
        return expr;
    }

    ast::pointer<ast::expression> parser::or_pattern_expression()
    {
        guard guard(this);

        ast::pointer<ast::expression> expr = primary_pattern_expression();

        while (expr && current().is(token::kind::line)) {
            token op = current();
            advance();
            ast::pointer<ast::expression> right = expect(primary_pattern_expression(), "pattern", "I was expecting a right operand after `|` in or pattern, dammit!", impl::pattern_expr_explanation);
            expr = ast::create<ast::or_pattern_expression>(source_range(expr->range().begin(), right->range().end()), op, expr, right);
        }

        return expr;
    }

    ast::pointer<ast::expression> parser::pattern_expression()
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::is_kw)) {
            ast::pointer<ast::expression> type = expect(type_expression(), "type", "I was expecting type after `is` in pattern, dammit!", impl::pattern_expr_explanation);
            return ast::create<ast::cast_pattern_expression>(source_range(saved.iter->location(), previous().range().end()), type);
        }

        return or_pattern_expression();
    }

    ast::pointer<ast::expression> parser::when_expression() 
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::when_kw)) {
            ast::pointer<ast::expression> condition = expect(logic_or_expression(), "expression", "I need condition after `when`, dammit!", impl::when_expr_explanation);
            ast::pointer<ast::expression> else_body = nullptr;

            if (match(token::kind::equal)) {
                ast::pointer<ast::expression> pattern = expect(pattern_expression(), "pattern", "I need pattern after `=` in pattern matching, idiot!", impl::when_expr_explanation);
                ast::pointer<ast::expression> body = expect(block_expression(), "body", "I expect when body here!", impl::when_expr_explanation);

                if (match(token::kind::else_kw)) {
                    if (current().is(token::kind::if_kw)) {
                        else_body = if_expression();
                    }
                    else if (current().is(token::kind::when_kw)) {
                        else_body = when_expression();
                    }
                    else {
                        else_body = block_expression();
                    }

                    expect(else_body, "body", diagnostic::format("I need else body after `$`, don't you think?", previous().lexeme()), impl::when_expr_explanation);
                }

                return ast::create<ast::when_pattern_expression>(source_range(saved.iter->location(), previous().range().end()), condition, pattern, body, else_body);
            }
            else if (match(token::kind::is_kw)) {
                ast::pointer<ast::expression> type = expect(type_expression(), "type", "I was expecting type after `is` in pattern, dammit!", impl::pattern_expr_explanation);
                ast::pointer<ast::expression> body = expect(block_expression(), "body", "I expect when body here!", impl::when_expr_explanation);

                if (match(token::kind::else_kw)) {
                    if (current().is(token::kind::if_kw)) {
                        else_body = if_expression();
                    }
                    else if (current().is(token::kind::when_kw)) {
                        else_body = when_expression();
                    }
                    else {
                        else_body = block_expression();
                    }

                    expect(else_body, "body", diagnostic::format("I need else body after `$`, don't you think?", previous().lexeme()), impl::when_expr_explanation);
                }

                return ast::create<ast::when_cast_expression>(source_range(saved.iter->location(), previous().range().end()), condition, type, body, else_body);
            }
            else {
                std::vector<ast::when_expression::branch> branches;

                token open = consume(token::kind::left_brace, "body", "I need when body here, idiot!", impl::when_expr_explanation);

                while (!eof() && !current().is(token::kind::right_brace)) {
                    if (branches.size() >= guard::max_statements) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("Max number of branches reached during parsing, f*cking hell!")
                                    .explanation(impl::guard_explanation)
                                    .highlight(current().range());
                        
                        abort(builder.build());
                    }

                    if (!branches.empty() && !previous().eol && !previous().is(token::kind::comma)) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("You cannot write when branches on the same line, pr*ck!")
                                    .insertion(source_range(previous().range().end(), 1), ",", "Try dividing when branches with `,` on the same line")
                                    .highlight(source_range(branches.back().pattern()->range().begin(), branches.back().body()->range().end()), diagnostic::highlighter::mode::light)
                                    .highlight(current().range())
                                    .explanation(impl::when_expr_explanation);

                        abort(builder.build());
                    }

                    ast::pointer<ast::expression> pattern = expect(pattern_expression(), "pattern", "I expect a pattern here, idiot!", impl::pattern_expr_explanation);
                    consume(token::kind::equal_greater, "`=>`", "I expect `=>` after pattern expression before body, idiot!", impl::when_expr_explanation);
                    ast::pointer<ast::expression> body = expect(expression(), "body", "I need a body after previous pattern, dammit!", impl::pattern_expr_explanation);
                    match(token::kind::comma);
                    branches.emplace_back(pattern, body);
                }

                parenthesis(token::kind::right_brace, "You forgot `}` after when body, dammit!", impl::when_expr_explanation, open);

                if (match(token::kind::else_kw)) {
                    if (current().is(token::kind::if_kw)) {
                        else_body = if_expression();
                    }
                    else if (current().is(token::kind::when_kw)) {
                        else_body = when_expression();
                    }
                    else {
                        else_body = block_expression();
                    }

                    expect(else_body, "body", diagnostic::format("I need else body after `$`, don't you think?", previous().lexeme()), impl::when_expr_explanation);
                }

                return ast::create<ast::when_expression>(source_range(saved.iter->location(), previous().range().end()), condition, branches, else_body);
            }
        }

        return nullptr;
    }

    ast::pointer<ast::expression> parser::for_expression()
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::for_kw)) {
            if (current().is(token::kind::mutable_kw) || (current().is(token::kind::identifier) && (next().is(token::kind::in_kw) || next().is(token::kind::colon)))) {
                std::vector<token> specifiers;

                if (match(token::kind::mutable_kw)) {
                    specifiers.push_back(previous());
                }

                token var = consume(token::kind::identifier, "name", "I variable name here, idiot!", impl::for_expr_explanation);
                ast::pointer<ast::expression> type = nullptr;

                if (match(token::kind::colon)) {
                    type = expect(type_expression(), "type", "I need variable type after `:`, don't you think?", impl::for_expr_explanation);
                }

                ast::pointer<ast::declaration> declaration = ast::create<ast::var_declaration>(source_range(saved.iter->location(), previous().range().end()), specifiers, var, type, ast::pointer<ast::expression>(nullptr));
                consume(token::kind::in_kw, "`in`", "I expect `in` keyword after variable declaration in for range, don't you believe?", impl::for_expr_explanation);
                ast::pointer<ast::expression> condition = expect(logic_or_expression(), "expression", "I expect condition after `in`, idiot!", impl::for_expr_explanation);
                ast::pointers<ast::statement> contracts = contract_clause_statements();
                ast::pointer<ast::expression> body = expect(block_expression(), "body", "I need for body here, dammit!", impl::for_expr_explanation);
                ast::pointer<ast::expression> else_body = nullptr;

                if (match(token::kind::else_kw)) {
                    if (current().is(token::kind::if_kw)) {
                        else_body = if_expression();
                    }
                    else if (current().is(token::kind::when_kw)) {
                        else_body = when_expression();
                    }
                    else {
                        else_body = block_expression();
                    }

                    expect(else_body, "body", diagnostic::format("I need else body after `$`, don't you think?", previous().lexeme()), impl::for_expr_explanation);
                }

                return ast::create<ast::for_range_expression>(source_range(saved.iter->location(), previous().range().end()), declaration, condition, body, else_body, contracts);
            }
            else {
                ast::pointer<ast::expression> condition = logic_or_expression();
                ast::pointers<ast::statement> contracts = contract_clause_statements();
                ast::pointer<ast::expression> body = expect(block_expression(), "body", "I need for body here, dammit!", impl::for_expr_explanation);
                ast::pointer<ast::expression> else_body = nullptr;

                if (match(token::kind::else_kw)) {
                    if (current().is(token::kind::if_kw)) {
                        else_body = if_expression();
                    }
                    else if (current().is(token::kind::when_kw)) {
                        else_body = when_expression();
                    }
                    else {
                        else_body = block_expression();
                    }

                    expect(else_body, "body", diagnostic::format("I need else body after `$`, don't you think?", previous().lexeme()), impl::for_expr_explanation);
                }

                return ast::create<ast::for_loop_expression>(source_range(saved.iter->location(), previous().range().end()), condition, body, else_body, contracts);
            }
        }

        return nullptr;
    }

    ast::pointer<ast::expression> parser::if_expression()
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::if_kw)) {
            ast::pointer<ast::expression> condition = expect(logic_or_expression(), "expression", "I need a condition after `if`, don't you believe?", impl::if_expr_explanation);
            ast::pointer<ast::expression> body = expect(block_expression(), "body", "I expect if body here!", impl::if_expr_explanation);
            ast::pointer<ast::expression> else_body = nullptr;

            if (match(token::kind::else_kw)) {
                if (current().is(token::kind::if_kw)) {
                    else_body = if_expression();
                }
                else if (current().is(token::kind::when_kw)) {
                    else_body = when_expression();
                }
                else {
                    else_body = block_expression();
                }

                expect(else_body, "body", diagnostic::format("I need else body after `$`, don't you think?", previous().lexeme()), impl::if_expr_explanation);
            }

            return ast::create<ast::if_expression>(source_range(saved.iter->location(), previous().range().end()), condition, body, else_body);
        }

        return nullptr;
    }

    ast::pointer<ast::expression> parser::constraint_or_expression()
    {
        guard guard(this);
        ast::pointer<ast::expression> expr = constraint_and_expression();
        
        if (!expr) return nullptr;

        while (match(token::kind::line)) {
            token op = previous();
            ast::pointer<ast::expression> right = expect(constraint_and_expression(), "constraint", "I expect a right operand after constraint operator `|`, idiot!", impl::constraint_expr_explanation);
            expr = ast::create<ast::binary_expression>(source_range(expr->range().begin(), right->range().end()), op, expr, right);
        }

        return expr;
    }
    
    ast::pointer<ast::expression> parser::constraint_and_expression()
    {
        guard guard(this);
        ast::pointer<ast::expression> expr = constraint_primary_expression();
        
        if (!expr) return nullptr;

        while (match(token::kind::amp)) {
            token op = previous();
            ast::pointer<ast::expression> right = expect(constraint_primary_expression(), "constraint", "I expect a right operand after constraint operator `&`, idiot!", impl::constraint_expr_explanation);
            expr = ast::create<ast::binary_expression>(source_range(expr->range().begin(), right->range().end()), op, expr, right);
        }

        return expr;
    }

    ast::pointer<ast::expression> parser::constraint_primary_expression()
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::left_parenthesis)) {
            token open = previous();
            ast::pointer<ast::expression> expr = expect(constraint_or_expression(), "constraint", "I expect a constraint inside parenthesis, idiot!", impl::constraint_expr_explanation);
            parenthesis(token::kind::right_parenthesis, "You forgot `)` after constraint expression, clown!", impl::constraint_expr_explanation, open);

            return ast::create<ast::parenthesis_expression>(source_range(saved.iter->location(), previous().range().end()), expr);
        }

        // concept test must be treated as a value instead of type
        if (auto path = path_type_expression()) return std::dynamic_pointer_cast<ast::type_expression>(path)->as_expression();
        
        return nullptr;
    }
    
    ast::pointer<ast::expression> parser::expression()
    {
        if (current().is(token::kind::when_kw)) {
            return when_expression();
        }
        else if (current().is(token::kind::for_kw)) {
            return for_expression();
        }
        else if (current().is(token::kind::if_kw)) {
            return if_expression();
        }
        else if (current().is(token::kind::left_brace)) {
            return block_expression();
        }

        return logic_or_expression();
    }

    ast::pointer<ast::statement> parser::assignment_statement()
    {
        guard guard(this);

        ast::pointer<ast::expression> expr = expression();
        ast::pointer<ast::assignment_statement> stmt = nullptr;

        if (expr && (!current().is_assignment_operator() || previous().eol)) {
            match(token::kind::semicolon);
            return ast::create<ast::expression_statement>(expr->range(), expr);
        }
        
        while (expr && !previous().eol && current().is_assignment_operator()) {
            token op = current();
            advance();
            
            ast::pointer<ast::expression> right = expect(expression(), "expression", diagnostic::format("I need a right operand after assignment operator `$`, don't you think?", op.lexeme()), impl::assign_stmt_explanation);
            bool err = false;
            match(token::kind::semicolon);

            if (stmt) {
                auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(op.location())
                            .message("You cannot chain assignment statements, pr*ck!")
                            .highlight(stmt->assignment_operator().range())
                            .highlight(op.range())
                            .explanation(impl::assign_stmt_explanation);
        
                publisher_.publish(builder.build());
                err = true;
                auto old = stmt;
                stmt = ast::create<ast::assignment_statement>(source_range(stmt->right()->range().begin(), previous().range().end()), op, stmt->right(), right);
                expr = right;
            }
            else {
                match(token::kind::semicolon);
                stmt = ast::create<ast::assignment_statement>(source_range(expr->range().begin(), previous().range().end()), op, expr, right);
            }

            if (!stmt->left()->is_assignable()) {
                auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(current().location())
                            .message("The left operand is not f*cking assignable, idiot!")
                            .highlight(stmt->left()->range())
                            .explanation(impl::assign_stmt_explanation);
        
                publisher_.publish(builder.build());
                err = true;
            }

            stmt->invalid(err);
        }

        return stmt;
    }

    ast::pointer<ast::statement> parser::jump_statement()
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::return_kw)) {
            if (previous().eol || current().is(token::kind::semicolon) || current().is(token::kind::right_brace)) {
                match(token::kind::semicolon);
                return ast::create<ast::return_statement>(previous().range(), nullptr);
            }
            else {
                ast::pointer<ast::expression> expr = expect(expression(), "expression", "I expect return value here, dumb*ss!", impl::jump_stmt_explanation);
                match(token::kind::semicolon);
                return ast::create<ast::return_statement>(source_range(saved.iter->location(), previous().range().end()), expr);
            }
        }
        else if (match(token::kind::break_kw)) {
            if (previous().eol || current().is(token::kind::semicolon)) {
                match(token::kind::semicolon);
                return ast::create<ast::break_statement>(previous().range(), nullptr);
            }
            else {
                ast::pointer<ast::expression> expr = expect(expression(), "expression", "I expect value after `break`, dumb*ss!", impl::jump_stmt_explanation);
                match(token::kind::semicolon);
                return ast::create<ast::break_statement>(source_range(saved.iter->location(), previous().range().end()), expr);
            }
        }
        else if (match(token::kind::continue_kw)) {
            match(token::kind::semicolon);
            return ast::create<ast::continue_statement>(previous().range());
        }

        return nullptr;
    }

    ast::pointer<ast::statement> parser::statement()
    {
        guard guard(this);
        ast::pointer<ast::statement> stmt = nullptr;
        
        switch (current().kind()) {
            case token::kind::return_kw:
            case token::kind::break_kw:
            case token::kind::continue_kw:
                stmt = jump_statement();
                break;
            case token::kind::mutable_kw:
            case token::kind::static_kw:
            case token::kind::val_kw:
            case token::kind::const_kw:
                stmt = variable_declaration();
                break;
            case token::kind::semicolon:
                stmt = ast::create<ast::null_statement>(previous().range());
                break;
            case token::kind::test_kw:
                stmt = test_declaration();
                break;
            case token::kind::function_kw:
                stmt = function_declaration();
                break;
            case token::kind::type_kw:
                stmt = type_declaration();
                break;
            case token::kind::extend_kw:
                stmt = extend_declaration();
                break;
            case token::kind::extern_kw:
                stmt = extern_declaration();
                break;
            case token::kind::behaviour_kw:
                stmt = behaviour_declaration();
                break;
            case token::kind::use_kw:
                stmt = use_declaration();
                break;
            case token::kind::app_kw:
            case token::kind::lib_kw:
                stmt = workspace_declaration();
                break;
            default:
                stmt = assignment_statement();
        }

        if (stmt) separator(stmt);

        return stmt;
    }

    ast::pointer<ast::declaration> parser::field_declaration()
    {
        guard guard(this);
        state saved = state_;
        bool hidden = false;

        if (match(token::kind::hide_kw)) {
            hidden = true;
            consume(token::kind::identifier, "name", "I need field name after `hide` keyword, b*tch!", impl::record_decl_explanation, false);
        }
        else if (match(token::kind::mutable_kw)) {
            fatal(previous().range(), "Mutability can only be applied to parameters, not fields, c*nt!", impl::record_decl_explanation, "garbage");
        }

        if (match(token::kind::identifier)) {
            token name = previous();
            consume(token::kind::colon, "`:`", "I expect `:` after field name, don't you believe?", impl::record_decl_explanation);
            ast::pointer<ast::expression> type_expr = expect(type_expression(), "type", "You forgot field type here, dammit!", impl::record_decl_explanation);
            ast::pointer<ast::declaration> decl = ast::create<ast::field_declaration>(source_range(saved.iter->location(), previous().range().end()), name, type_expr);
            decl->hidden(hidden);
            return decl;
        }

        return nullptr;
    }

    ast::pointer<ast::declaration> parser::parameter_declaration()
    {
        guard guard(this);
        state saved = state_;
        bool mutability = false, variadic = false;

        if (match(token::kind::mutable_kw)) {
            mutability = true;
            if (!current().is(token::kind::ellipsis) && !current().is(token::kind::identifier)) {
                consume(token::kind::identifier, "name", "I need parameter name after `mutable` keyword, idiot!", impl::function_decl_explanation, false);
            }
        }
        else if (match(token::kind::hide_kw)) {
            fatal(previous().range(), "You can only hide fields, not parameters, c*nt!", impl::function_decl_explanation, "garbage");
        }

        if (match(token::kind::ellipsis)) {
            variadic = true;
            consume(token::kind::identifier, "name", "I need parameter name after `...`, idiot!", impl::function_decl_explanation, false);
        }

        if (match(token::kind::identifier)) {
            token name = previous();
            consume(token::kind::colon, "`:`", diagnostic::format("I expect `:` after parameter `$`, idiot!", name.lexeme()), impl::function_decl_explanation);
            ast::pointer<ast::expression> type_expr = expect(type_expression(), "type", "You forgot parameter type here, holy sh*t!", impl::function_decl_explanation);
            std::dynamic_pointer_cast<ast::type_expression>(type_expr)->set_mutable(mutability);
            return ast::create<ast::parameter_declaration>(source_range(saved.iter->location(), previous().range().end()), name, type_expr, mutability, variadic);
        }

        return nullptr;
    }

    ast::pointer<ast::declaration> parser::generic_parameter_declaration()
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::identifier)) {
            token name = previous();

            if (match(token::kind::colon)) {
                ast::pointer<ast::expression> type_expr = expect(type_expression(), "type", "I need type after `:`, dammit!", impl::generic_param_decl_explanation);
                return ast::create<ast::generic_const_parameter_declaration>(source_range(saved.iter->location(), previous().range().end()), name, type_expr);
            }
            else {
                return ast::create<ast::generic_type_parameter_declaration>(source_range(saved.iter->location(), previous().range().end()), name);
            }
        }

        return nullptr;
    }

    ast::pointer<ast::declaration> parser::generic_clause_declaration(bool constraints)
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::left_parenthesis)) {
            token open = previous();
            ast::pointers<ast::declaration> params;
            ast::pointer<ast::expression> constraint = nullptr;

            do {
                if (params.size() >= guard::max_parameters) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(current().location())
                                .message("Max number of elements inside tuple expression reached during parsing, f*cking hell!")
                                .explanation(impl::guard_explanation)
                                .highlight(source_range(current().range()), "here");
                    
                    abort(builder.build());
                }

                ast::pointer<ast::declaration> param = expect(generic_parameter_declaration(), "declaration", "I need a generic type or constant declaration here, idiot!", impl::generic_param_decl_explanation);
                params.push_back(param);
            }
            while (match(token::kind::comma));

            parenthesis(token::kind::right_parenthesis, "You forgot `)` in generic parameters list, clown.", impl::function_decl_explanation, open);

            if (constraints && match(token::kind::if_kw)) {
                constraint = expect(constraint_or_expression(), "constraint", "I need a constraint here, dammit!", impl::constraint_expr_explanation);
            }

            return ast::create<ast::generic_clause_declaration>(source_range(saved.iter->location(), previous().range().end()), params, constraint);
        }

        return nullptr;
    }

    ast::pointer<ast::declaration> parser::type_declaration()
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::type_kw)) {
            ast::pointer<ast::declaration> generic = generic_clause_declaration(true);
            token name = consume(token::kind::identifier, "type", "I need type name in declaration after `type`, idiot!", impl::type_decl_explanation);

            if (match(token::kind::equal)) {
                ast::pointer<ast::expression> type_expr = expect(type_expression(), "type", diagnostic::format("I need a type to associate with alias `$`, b*tch!", name.lexeme()), impl::alias_decl_explanation);
                match(token::kind::semicolon);
                return ast::create<ast::alias_declaration>(source_range(saved.iter->location(), previous().range().end()), name, generic, type_expr);
            }
            else if (match(token::kind::is_kw)) {
                token brace = previous();
                ast::pointers<ast::expression> types;

                do {
                    if (types.size() >= guard::max_elements) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("Max number of types inside variant reached during parsing, f*cking hell!")
                                    .explanation(impl::guard_explanation)
                                    .highlight(source_range(current().range()), "here");
                        
                        abort(builder.build());
                    }

                    ast::pointer<ast::expression> type = expect(single_type_expression(), "type", "I expect a type here, idiot!", impl::variant_decl_explanation);
                    types.push_back(type);
                }
                while (match(token::kind::line));

                match(token::kind::semicolon);  
                ast::pointer<ast::declaration> decl = ast::create<ast::variant_declaration>(source_range(saved.iter->location(), previous().range().end()), name, generic, types);

                if (types.empty()) {
                    auto builder = diagnostic::builder()   
                                    .severity(diagnostic::severity::error)
                                    .small(true)
                                    .location(brace.location())
                                    .message("At least have the decency to fill this variant with one type, pr*ck!")
                                    .highlight(name.range(), diagnostic::highlighter::mode::light)
                                    .highlight(source_range(brace.location(), previous().range().end()), "empty")
                                    .explanation(impl::variant_decl_explanation);
                    
                    publisher_.publish(builder.build());
                    decl->invalid(true);
                }
                else if (types.size() == 1) {
                    auto builder = diagnostic::builder()   
                                    .severity(diagnostic::severity::warning)
                                    .small(true)
                                    .location(brace.location())
                                    .message("How could be useful to define one typed variant? I can't understand.")
                                    .highlight(name.range())
                                    .explanation(impl::variant_decl_explanation);
                    
                    publisher_.publish(builder.build());
                }

                return decl;
            }
            else if (match(token::kind::range_kw)) {
                ast::pointer<ast::expression> constraint = expect(range_expression(), "expression", "You forgot the range in range type, idiot!", impl::range_decl_explanation);                
                match(token::kind::semicolon);
                auto result = ast::create<ast::range_declaration>(source_range(saved.iter->location(), previous().range().end()), name, generic, constraint);

                if (auto range = std::dynamic_pointer_cast<ast::range_expression>(constraint)) {
                    if (range->start() && !std::dynamic_pointer_cast<ast::literal_expression>(range->start())) {
                        report(range->start()->range(), "I need a damn literal here, number, character or whatever!", impl::range_decl_explanation);
                        result->invalid(true);
                    }

                    if (range->end() && !std::dynamic_pointer_cast<ast::literal_expression>(range->end())) {
                        report(range->end()->range(), "I need a damn literal here, number, character or whatever!", impl::range_decl_explanation);
                        result->invalid(true);
                    }
                }
                else {
                    fatal(constraint->range(), "This is not the range I need!", impl::range_decl_explanation);
                }

                return result;
            }
            else if (current().is(token::kind::union_kw) || current().is(token::kind::left_parenthesis)) {
                bool is_union = false;
                std::string record;

                if (match(token::kind::union_kw)) {
                    is_union = true;
                    record = "union";
                }
                else {
                    is_union = false;
                    record = "struct";
                }

                ast::pointers<ast::declaration> fields;
                token open = consume(token::kind::left_parenthesis, "`(`", diagnostic::format("I expect `(` in $ declaration, don't you think?", record), impl::record_decl_explanation);

                if (!current().is(token::kind::right_parenthesis)) {
                    ast::pointer<ast::declaration> field;

                    if ((current().is(token::kind::identifier) && next().is(token::kind::colon)) || current().is(token::kind::hide_kw)) {
                        do {
                            if (fields.size() >= guard::max_elements) {
                                auto builder = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(current().location())
                                            .message("Max number of fields inside record reached during parsing, f*cking hell!")
                                            .explanation(impl::guard_explanation)
                                            .highlight(source_range(current().range()), "here");
                                
                                abort(builder.build());
                            }

                            field = expect(field_declaration(), "declaration", diagnostic::format("I need field declaration after `$` in $, idiot!", previous().lexeme(), record), impl::record_decl_explanation);
                            fields.push_back(field);
                        }
                        while (match(token::kind::comma));
                    }
                    else {
                        unsigned index = 0;

                        do {
                            if (fields.size() >= guard::max_elements) {
                                auto builder = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(current().location())
                                            .message("Max number of fields inside record reached during parsing, f*cking hell!")
                                            .explanation(impl::guard_explanation)
                                            .highlight(source_range(current().range()), "here");
                                
                                abort(builder.build());
                            }

                            ast::pointer<ast::expression> type_expr = expect(type_expression(), "declaration", diagnostic::format("I need tuple field declaration after `$` in $, idiot!", previous().lexeme(), record), impl::record_decl_explanation);
                            field = ast::create<ast::tuple_field_declaration>(source_range(saved.iter->location(), previous().range().end()), index++, type_expr);
                            fields.push_back(field);
                        }
                        while (match(token::kind::comma));
                    }
                }

                parenthesis(token::kind::right_parenthesis, diagnostic::format("You forgot `)` in $ declaration, dammit!", record), impl::record_decl_explanation, open);
                match(token::kind::semicolon);
                return ast::create<ast::record_declaration>(source_range(saved.iter->location(), previous().range().end()), name, generic, fields, is_union);
            }
            else if (match(token::kind::semicolon) || current().is(token::kind::right_brace) || name.eol) {
                return ast::create<ast::record_declaration>(source_range(saved.iter->location(), previous().range().end()), name, generic, ast::pointers<ast::declaration>(), false);
            }
            else {
                fatal(current().range(), "I expect a type declaration (struct, union, range, variant or alias) here!", impl::type_decl_explanation, "expected declaration");
            }
        }

        return nullptr;
    }

    ast::pointers<ast::statement> parser::contract_clause_statements()
    {
        guard guard(this);
        ast::pointers<ast::statement> contracts;

        while (!eof()) {
            ast::pointer<ast::statement> stmt = nullptr;

            switch (current().kind()) {
                case token::kind::ensure_kw:
                case token::kind::invariant_kw:
                case token::kind::require_kw:
                    if (!contracts.empty() && !previous().eol && !previous().is(token::kind::comma)) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("You cannot write contract statements on the same line, pr*ck!")
                                    .insertion(source_range(previous().range().end(), 1), ",", "Try dividing contract statements with `,` on the same line")
                                    .explanation(impl::contract_stmt_explanation)
                                    .highlight(contracts.back()->range());

                        abort(builder.build());
                    }
                    else {
                        advance();
                        token specifer = previous();
                        ast::pointer<ast::expression> condition = expect(logic_or_expression(), "expression", diagnostic::format("I need a damn condition after `$` keyword in contract statement!", specifer.lexeme()), impl::contract_stmt_explanation);
                        match(token::kind::comma);
                        stmt = ast::create<ast::contract_statement>(source_range(specifer.location(), previous().range().end()), specifer, condition);
                    }
                    break;
                default:
                    break;
            }

            if (!stmt) break;

            contracts.push_back(stmt);
        }

        return contracts;
    }

    ast::pointer<ast::declaration> parser::function_declaration(bool is_external)
    {
        guard guard(this);
        state saved = state_;

        if (current().is(token::kind::function_kw) || current().is(token::kind::identifier)) {
            bool err = false;
            ast::pointer<ast::declaration> clause = nullptr;
            ast::pointers<ast::declaration> params;

            if (match(token::kind::function_kw)) {
                clause = generic_clause_declaration(true);
            }

            token name = consume(token::kind::identifier, "name", "I need the damn function name here, don't you believe?", impl::function_decl_explanation);
            token open = consume(token::kind::left_parenthesis, "`(`", "You forgot `(` in function declaration, idiot!", impl::function_decl_explanation);

            if (!current().is(token::kind::right_parenthesis)) {
                ast::pointer<ast::declaration> param;

                do {
                    if (params.size() >= guard::max_parameters) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("Max number of elements inside tuple expression reached during parsing, f*cking hell!")
                                    .explanation(impl::guard_explanation)
                                    .highlight(source_range(current().range()), "here");
                        
                        abort(builder.build());
                    }

                    param = expect(parameter_declaration(), "declaration", diagnostic::format("I need a parameter declaration after `$` in function, don't you believe?", previous().lexeme()), impl::function_decl_explanation);
                    params.push_back(param);
                }
                while (match(token::kind::comma));
            }

            parenthesis(token::kind::right_parenthesis, "You forgot `)` in function declaration, clown!", impl::function_decl_explanation, open);

            token rparen = previous();
            ast::pointer<ast::expression> return_type_expr = nullptr;

            source_range result_range(state_.iter->location(), 1);

            switch (current().kind()) {
                case token::kind::ensure_kw:
                case token::kind::invariant_kw:
                case token::kind::require_kw:
                case token::kind::equal:
                case token::kind::left_brace:
                case token::kind::semicolon:
                    break;
                default:
                    if (!previous().eol) {
                        return_type_expr = expect(type_expression(), "type", "I expect function return type here, pr*ck!", impl::function_decl_explanation);
                        result_range = return_type_expr->range();
                    }
            }

            ast::pointers<ast::statement> contracts = contract_clause_statements();
            ast::pointer<ast::expression> body = nullptr;
            
            if (match(token::kind::equal)) {
                body = expect(expression(), "expression", "I need function body after `=`, idiot!", impl::function_decl_explanation);
            }
            else {
                body = block_expression();
            }

            match(token::kind::semicolon);
            auto decl = ast::create<ast::function_declaration>(source_range(saved.iter->location(), previous().range().end()), name, clause, params, return_type_expr, body, contracts);

            decl->result_range() = result_range;

            /*if (body && is_external) {
                auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .small(true)
                                .location(body->range().begin())
                                .message("You cannot provide an implementation for a function with external linkage!")
                                .removal(body->range(), "Remove function body")
                                .highlight(decl->range(), diagnostic::highlighter::mode::light)
                                .highlight(body->range(), "body is NOT allowed");
                
                publisher_.publish(builder.build());
                body->invalid(true);
                err = true;
            }
            else if (!is_external) {
                expect(body, "function body", "Where the fu*k is function body?");
            }*/

            decl->invalid(err);

            return decl;
        }

        return nullptr;
    }

    ast::pointer<ast::declaration> parser::property_declaration()
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::dot)) {
            bool err = false;
            ast::pointers<ast::declaration> params;
            token name = consume(token::kind::identifier, "name", "I need the damn property name after `.`, don't you believe?", impl::property_decl_explanation);
            token open = consume(token::kind::left_parenthesis, "`(`", "You forgot `(` in property declaration, idiot!", impl::property_decl_explanation);

            if (!current().is(token::kind::right_parenthesis)) {
                ast::pointer<ast::declaration> param;

                do {
                    if (params.size() >= guard::max_parameters) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("Max number of elements inside tuple expression reached during parsing, f*cking hell!")
                                    .explanation(impl::guard_explanation)
                                    .highlight(source_range(current().range()), "here");
                        
                        abort(builder.build());
                    }

                    param = expect(parameter_declaration(), "declaration", diagnostic::format("I need a parameter declaration after `$` in property, don't you believe?", previous().lexeme()), impl::property_decl_explanation);
                    params.push_back(param);
                }
                while (match(token::kind::comma));
            }

            parenthesis(token::kind::right_parenthesis, "You forgot `)` in property declaration, idiot!", impl::property_decl_explanation, open);

            token rparen = previous();
            ast::pointer<ast::expression> return_type_expr = nullptr;

            switch (current().kind()) {
                case token::kind::ensure_kw:
                case token::kind::invariant_kw:
                case token::kind::require_kw:
                case token::kind::equal:
                case token::kind::left_brace:
                case token::kind::semicolon:
                    break;
                default:
                    if (!previous().eol) {
                        return_type_expr = expect(type_expression(), "type", "I expect property return type here, pr*ck!", impl::property_decl_explanation);
                    }
            }

            ast::pointers<ast::statement> contracts = contract_clause_statements();
            ast::pointer<ast::expression> body = nullptr;
            
            if (match(token::kind::equal)) {
                body = expect(expression(), "expression", "I need property body after `=`, idiot!", impl::property_decl_explanation);
            }
            else {
                body = block_expression();
            }

            match(token::kind::semicolon);
            ast::pointer<ast::declaration> decl = ast::create<ast::property_declaration>(source_range(saved.iter->location(), previous().range().end()), name, params, return_type_expr, body, contracts);
            decl->invalid(err);

            return decl;
        }

        return nullptr;
    }

    bool parser::path(ast::path& p)
    {
        guard guard(this);

        if (match(token::kind::identifier)) {
            p.push_back(previous());

            while (match(token::kind::dot)) {
                if (p.size() >= guard::max_path_names) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(current().location())
                                .message("Max number of names inside path reached during parsing, f*cking hell!")
                                .explanation(impl::guard_explanation)
                                .highlight(source_range(current().range()), "here");
                    
                    abort(builder.build());
                }
                
                p.push_back(consume(token::kind::identifier, "name", "I need the damn name after `.` in path, f*cking hell!", impl::path_explanation));
            }

            return true;
        }

        return false;
    }

    ast::pointer<ast::expression> parser::generic_argument()
    {
        ast::pointer<ast::expression> argument;
        // a generic argument can be either a costant expression or a type
        // this ambiguity is resolved by the semantic analyzer when
        // using the generic parameters list from the generic type declaration
        state prev = state_;
        // silent error mode must be used for backtrack
        silence();
        // try parsing a type annotation, if it fails
        // then backtracking is performed
        try { 
            argument = type_expression(); 
            // if a number has been parsed as bit-field type then it must be a numeric literal expression
            if (auto bit_field_type = std::dynamic_pointer_cast<ast::bit_field_type_expression>(argument)) argument = bit_field_type->as_expression();
        } 
        catch (syntax_error& e) {
            argument = nullptr;
        }
        // silent error mode is disabled
        unsilence();
        // if parsing had success but let to a corrupt state then backtracking and retry
        // with expression()
        if (!current().is(token::kind::comma) && !current().is(token::kind::right_parenthesis)) {
            argument.reset();
        }
        // retry parsing an expression if it failed previosly
        if (!argument) {
            backtrack(prev);
            argument = expression();
        }

        return argument;
    }

    bool parser::generic_arguments_list(ast::pointers<ast::expression>& args)
    {
        if (match(token::kind::left_parenthesis)) {
            token open = previous();

            if (!current().is(token::kind::right_parenthesis)) {
                do {
                    if (args.size() >= guard::max_parameters) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("Max number of elements inside tuple expression reached during parsing, f*cking hell!")
                                    .explanation(impl::guard_explanation)
                                    .highlight(source_range(current().range()), "here");
                        
                        abort(builder.build());
                    }
                    // a generic argument can be either a costant expression or a type
                    ast::pointer<ast::expression> generic = expect(generic_argument(), "type or expression", diagnostic::format("I need a generic argument after `$` in generic arguments list, dumb*ss", previous().lexeme()), impl::generic_param_decl_explanation);
                    args.push_back(generic);
                }
                while (match(token::kind::comma));
            }

            parenthesis(token::kind::right_parenthesis, "You forgot `)` in generic arguments list, idiot!", impl::generic_param_decl_explanation, open);

            if (args.empty()) {
                report(source_range(open.location(), previous().range().end()), "At least have the decency to fill this empty list with a generic argument, idiot!", impl::generic_param_decl_explanation, "empty");
            }

            return true;
        }

        return false;
    }

    ast::pointer<ast::declaration> parser::variant_kind_declaration()
    {
        guard guard(this);
        state saved = state_;
        bool hidden = false;

        if (match(token::kind::hide_kw)) {
            hidden = true;
            consume(token::kind::identifier, "name", "I need variant kind name after `hide` keyword, sh*t!", impl::variant_decl_explanation, false);
        }

        if (match(token::kind::identifier)) {
            token name = previous();
            ast::pointers<ast::declaration> fields;

            if (match(token::kind::left_parenthesis)) {
                token open = previous();

                if (!current().is(token::kind::right_parenthesis)) {
                    ast::pointer<ast::declaration> field;

                    if (current().is(token::kind::hide_kw) || (current().is(token::kind::identifier) && next().is(token::kind::colon))) {
                        do {
                            if (fields.size() >= guard::max_elements) {
                                auto builder = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(current().location())
                                            .message("Max number of fields inside record reached during parsing, f*cking hell!")
                                            .explanation(impl::guard_explanation)
                                            .highlight(source_range(current().range()), "here");
                                
                                abort(builder.build());
                            }

                            field = expect(field_declaration(), "declaration", diagnostic::format("I need field declaration after `$` in variant kind, idiot!", previous().lexeme()), impl::variant_decl_explanation);
                            fields.push_back(field);
                        }
                        while (match(token::kind::comma));
                    }
                    else {
                        unsigned index = 0;

                        do {
                            if (fields.size() >= guard::max_elements) {
                                auto builder = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(current().location())
                                            .message("Max number of fields inside record reached during parsing, f*cking hell!")
                                            .explanation(impl::guard_explanation)
                                            .highlight(source_range(current().range()), "here");
                                
                                abort(builder.build());
                            }

                            ast::pointer<ast::expression> type_expr = expect(type_expression(), "declaration", diagnostic::format("I need tuple field declaration after `$` in variant kind, idiot!", previous().lexeme()), impl::variant_decl_explanation);
                            
                            field = ast::create<ast::tuple_field_declaration>(source_range(saved.iter->location(), previous().range().end()), index++, type_expr);
                            fields.push_back(field);
                        }
                        while (match(token::kind::comma));
                    }
                }

                parenthesis(token::kind::right_parenthesis, "You forgot `)` in variant kind declaration, dumb*ss", impl::variant_decl_explanation, open);            
            }

            ast::pointer<ast::declaration> decl = ast::create<ast::record_declaration>(source_range(saved.iter->location(), previous().range().end()), name, ast::pointer<ast::declaration>(nullptr), fields, false);
            decl->hidden(hidden);
            return decl;
        }

        return nullptr;
    }

    ast::pointer<ast::declaration> parser::workspace_declaration()
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::app_kw)) {
            token path = consume(token::kind::identifier, "name", "I need application name here, don't you think?", impl::workspace_decl_explanation);
            match(token::kind::semicolon);
            ast::pointer<ast::declaration> decl = ast::create<ast::workspace_declaration>(source_range(saved.iter->location(), previous().range().end()), path);
            separator(decl);

            if (workspace_) {
                const token& prev = dynamic_cast<ast::workspace_declaration*>(workspace_.get())->path();
                auto builder = diagnostic::builder()
                               .small(true)
                               .severity(diagnostic::severity::error)
                               .location(decl->range().begin())
                               .message(diagnostic::format("This `$` application declaration conflicts with previous one, dammit!", path.lexeme()))
                               .explanation(impl::workspace_decl_explanation)
                               .highlight(path.range(), "conflicting")
                               .note(prev.range(), diagnostic::format("This is the original declaration for nuclues `$`.", prev.lexeme()));

                publisher_.publish(builder.build());
                decl->invalid(true);
            }

            return decl;
        }
        else if (match(token::kind::lib_kw)) {
            token path = consume(token::kind::identifier, "name", "I need library name here, don't you think?", impl::workspace_decl_explanation);
            match(token::kind::semicolon);
            ast::pointer<ast::declaration> decl = ast::create<ast::workspace_declaration>(source_range(saved.iter->location(), previous().range().end()), path);
            separator(decl);

            if (workspace_) {
                const token& prev = dynamic_cast<ast::workspace_declaration*>(workspace_.get())->path();
                auto builder = diagnostic::builder()
                               .small(true)
                               .severity(diagnostic::severity::error)
                               .location(decl->range().begin())
                               .message(diagnostic::format("This `$` library declaration conflicts with previous one, dammit!", path.lexeme()))
                               .explanation(impl::workspace_decl_explanation)
                               .highlight(path.range(), "conflicting")
                               .note(prev.range(), diagnostic::format("This is the original declaration for nuclues `$`.", prev.lexeme()));

                publisher_.publish(builder.build());
                decl->invalid(true);
            }

            return decl;
        }

        return nullptr;
    }

    ast::pointer<ast::declaration> parser::use_declaration()
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::use_kw)) {
            token path = consume(token::kind::identifier, "name", "I need imported path name here, don't you think?", impl::use_decl_explanation);
            match(token::kind::semicolon);
            ast::pointer<ast::declaration> decl = ast::create<ast::use_declaration>(source_range(saved.iter->location(), previous().range().end()), path);
            separator(decl);
            return decl;
        }

        return nullptr;
    }

    ast::pointer<ast::declaration> parser::concept_declaration()
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::concept_kw)) {
            bool err = false;
            ast::pointer<ast::declaration> decl = nullptr;
            ast::pointer<ast::expression> base = nullptr;
            ast::pointers<ast::declaration> declarations;
            ast::pointer<ast::declaration> generic = generic_clause_declaration();
            token name = consume(token::kind::identifier, "name", "I need concept name in this place!", impl::concept_decl_explanation);
            
            if (match(token::kind::as_kw)) {
                base = expect(constraint_or_expression(), "constraint", "You forgot the contraint after `as` keyword, c*nt!", impl::constraint_expr_explanation);
            }

            if (match(token::kind::left_brace)) {
                token brace = previous();

                while (!eof() && !current().is(token::kind::right_brace)) {
                    if (declarations.size() >= guard::max_statements) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("Max number of declarations reached during parsing, f*cking hell!")
                                    .explanation(impl::guard_explanation)
                                    .highlight(source_range(current().range()), "here");
                        
                        abort(builder.build());
                    }

                    ast::pointer<ast::declaration> declaration = nullptr;

                    switch (current().kind()) {
                        case token::kind::function_kw:
                        case token::kind::identifier:
                            declaration = expect(function_declaration(), "declaration", "I was expecting a function declaration here!", impl::concept_decl_explanation);
                            break;
                        case token::kind::dot:
                            declaration = expect(property_declaration(), "declaration", "I was expecting a property declaration here!", impl::concept_decl_explanation);
                            break;
                        default:
                            expect(declaration, "declaration", "I want a damn function or property declaration in this place!", impl::concept_decl_explanation);
                    }

                    separator(declaration);
                    declarations.push_back(declaration);
                }
                
                parenthesis(token::kind::right_brace, "You forgot `}` in a concept block, dammit!", impl::concept_decl_explanation, brace);
            }
            else if (!base) {
                consume(token::kind::left_brace, "`{`", "I expect concept block here, c*nt!", impl::concept_decl_explanation);
            }

            match(token::kind::semicolon);
            decl = ast::create<ast::concept_declaration>(source_range(saved.iter->location(), previous().range().end()), generic, name, base, declarations);
            decl->invalid(err);
            return decl;
        }

        return nullptr;
    }

    ast::pointer<ast::declaration> parser::extern_declaration()
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::extern_kw)) {
            ast::pointers<ast::declaration> declarations;
            ast::pointer<ast::declaration> declaration = nullptr;
            token brace = consume(token::kind::left_brace, "`{`", "I need `{` in extern block, f*cking hell!", impl::extern_decl_explanation);;

            while (!eof() && !current().is(token::kind::right_brace)) {
                if (declarations.size() >= guard::max_statements) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(current().location())
                                .message("Max number of declarations reached during parsing, f*cking hell!")
                                .explanation(impl::guard_explanation)
                                .highlight(source_range(current().range()), "here");
                    
                    abort(builder.build());
                }

                ast::pointer<ast::declaration> decl = nullptr;

                switch (current().kind()) {
                    case token::kind::function_kw:
                    case token::kind::identifier:
                        decl = expect(function_declaration(true), "declaration", "I was expecting a function prototype inside extern block, idiot!", impl::extern_decl_explanation);
                        break;
                    default:
                        expect(declaration, "declaration", "I was expecting a function prototype inside extern block, idiot!", impl::extern_decl_explanation);
                }

                separator(decl);
                declarations.push_back(decl);
            }

            parenthesis(token::kind::right_brace, "You forgot `}` in a extern block, dammit!", impl::extern_decl_explanation, brace);
            match(token::kind::semicolon);
            declaration = ast::create<ast::extern_declaration>(source_range(saved.iter->location(), previous().range().end()), declarations);

            if (declarations.empty()) {
                auto builder = diagnostic::builder()   
                                .severity(diagnostic::severity::error)
                                .small(true)
                                .location(brace.location())
                                .message("At least have the decency to fill this extern block with one declaration!")
                                .explanation(impl::extern_decl_explanation)
                                .highlight(declaration->range(), diagnostic::highlighter::mode::light)
                                .highlight(source_range(brace.location(), previous().range().end()), "empty");
                
                publisher_.publish(builder.build());
                declaration->invalid(true);
            }

            return declaration;
        }
        
        return nullptr;
    }
    
    ast::pointer<ast::declaration> parser::extend_declaration()
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::extend_kw)) {
            bool err = false;
            ast::pointer<ast::declaration> decl;
            ast::pointers<ast::declaration> declarations;
            ast::pointer<ast::declaration> generic = generic_clause_declaration();
            ast::pointer<ast::expression> type_expr = expect(type_expression(), "type", "I need to known the name of the type to extend, don't you think?", impl::extend_decl_explanation);
            ast::pointers<ast::expression> behaviours;

            if (/*!dynamic_cast<ast::identifier_type_expression*>(type_expr.get())*/!dynamic_cast<ast::path_type_expression*>(type_expr.get())) {
                report(type_expr->range(), "I need a behaviour type name here, not this sh*t!", impl::extend_decl_explanation, "expected name");
                type_expr->invalid(true);
                err = true;
            }

            if (match(token::kind::as_kw)) {
                do {
                    ast::pointer<ast::expression> behaviour = expect(type_expression(), "type", diagnostic::format("I can't see any f*cking behaviour type name after `$`, dammit!", previous().lexeme()), impl::extend_decl_explanation);

                    if (/*!dynamic_cast<ast::identifier_type_expression*>(behaviour.get())*/!dynamic_cast<ast::path_type_expression*>(behaviour.get())) {
                        report(behaviour->range(), "This is not a behaviour name! You must specifiy a user-defined type name here!", impl::extend_decl_explanation, "expected type");
                        behaviour->invalid(true);
                        err = true;
                    }

                    behaviours.push_back(behaviour);
                }
                while (match(token::kind::comma));
            }

            if (match(token::kind::left_brace)) {
                token brace = previous();

                while (!eof() && !current().is(token::kind::right_brace)) {
                    if (declarations.size() >= guard::max_statements) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("Max number of declarations reached during parsing, f*cking hell!")
                                    .explanation(impl::guard_explanation)
                                    .highlight(source_range(current().range()), "here");
                        
                        abort(builder.build());
                    }

                    ast::pointer<ast::declaration> declaration = nullptr;
                    bool hidden = match(token::kind::hide_kw);

                    switch (current().kind()) {
                        case token::kind::function_kw:
                        case token::kind::identifier:
                            declaration = expect(function_declaration(), "declaration", "I was expecting a function declaration here!", impl::extend_decl_explanation);
                            break;
                        case token::kind::const_kw:
                            declaration = expect(variable_declaration(), "declaration", "I was expecting a constant declaration here!", impl::extend_decl_explanation);
                            break;
                        case token::kind::dot:
                            declaration = expect(property_declaration(), "declaration", "I was expecting a property declaration here!", impl::extend_decl_explanation);
                            break;
                        case token::kind::type_kw:
                            declaration = expect(type_declaration(), "declaration", "I was expecting a type declaration here!", impl::extend_decl_explanation);
                            break;
                        default:
                            expect(declaration, "declaration", "I want a damn function, property, type or constant declaration in this place!", impl::extend_decl_explanation);
                    }

                    separator(declaration);
                    declaration->hidden(hidden);
                    declarations.push_back(declaration);
                }
                
                parenthesis(token::kind::right_brace, "You forgot `}` in a extend block, idiot!", impl::extend_decl_explanation, brace);
            }

            match(token::kind::semicolon);
            decl = ast::create<ast::extend_declaration>(source_range(saved.iter->location(), previous().range().end()), generic, type_expr, behaviours, declarations);
            decl->invalid(err);
            return decl;
        }
        
        return nullptr;
    }
    
    ast::pointer<ast::declaration> parser::behaviour_declaration()
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::behaviour_kw)) {
            ast::pointer<ast::declaration> decl;
            ast::pointers<ast::declaration> declarations;
            ast::pointer<ast::declaration> generic = generic_clause_declaration();
            token name = consume(token::kind::identifier, "name", "I need behaviour name here, dumb*ss!", impl::behaviour_decl_explanation);
            token brace = consume(token::kind::left_brace, "`{`", "You forgot `{` in behaviour block, p*ssy!", impl::behaviour_decl_explanation);
            
            while (!eof() && !current().is(token::kind::right_brace)) {
                if (declarations.size() >= guard::max_statements) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(current().location())
                                .message("Max number of declarations reached during parsing, f*cking hell!")
                                .explanation(impl::guard_explanation)
                                .highlight(source_range(current().range()), "here");
                    
                    abort(builder.build());
                }

                ast::pointer<ast::declaration> declaration = nullptr;

                switch (current().kind()) {
                    case token::kind::function_kw:
                    case token::kind::identifier:
                        declaration = expect(function_declaration(), "declaration", "I was expecting a function declaration here, dammit!", impl::behaviour_decl_explanation);
                        break;
                    case token::kind::dot:
                        declaration = expect(property_declaration(), "declaration", "I was expecting a property declaration here, dammit!", impl::behaviour_decl_explanation);
                        break;
                    default:
                        expect(declaration, "declaration", "I want a damn function or property declaration in this place, dammit!", impl::behaviour_decl_explanation);
                }

                separator(declaration);
                declarations.push_back(declaration);
            }

            parenthesis(token::kind::right_brace, "You forgot `}` in behaviour block, idiot.", impl::behaviour_decl_explanation, brace);
            match(token::kind::semicolon);
            decl = ast::create<ast::behaviour_declaration>(source_range(saved.iter->location(), previous().range().end()), generic, name, declarations);

            if (declarations.empty()) {
                auto builder = diagnostic::builder()   
                                .severity(diagnostic::severity::error)
                                .small(true)
                                .location(brace.location())
                                .message("At least have the decency to fill this behaviour block with one function or property dumb*ss!")
                                .explanation(impl::behaviour_decl_explanation)
                                .highlight(name.range(), diagnostic::highlighter::mode::light)
                                .highlight(source_range(brace.location(), previous().range().end()), "empty");
                
                publisher_.publish(builder.build());
                decl->invalid(true);
            }

            return decl;
        }
        
        return nullptr;
    }

    ast::pointer<ast::declaration> parser::variable_declaration()
    {
        guard guard(this);
        state saved = state_;

        if (current().is(token::kind::mutable_kw) || current().is(token::kind::static_kw) || 
            current().is(token::kind::const_kw) || current().is(token::kind::val_kw)) {
            ast::pointer<ast::declaration> decl;
            std::vector<token> specifiers;
            std::vector<token> names;
            ast::pointer<ast::expression> type_expr = nullptr;
            ast::pointer<ast::expression> value = nullptr;
            bool mutability = false;
            bool constant = false;
            bool tupled = false;
            bool err = false;

            while (true) {
                if (match(token::kind::static_kw));
                else break;

                if (specifiers.size() > 0) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(current().location())
                                .message("You cannot use more than one lifetime specifier, pr*ck")
                                .explanation(impl::var_decl_explanation)
                                .highlight(specifiers.back().range(), diagnostic::highlighter::mode::light)
                                .highlight(previous().range(), "conflicting");
            
                    publisher_.publish(builder.build());
                    err = true;
                }
                else {
                    specifiers.push_back(previous());
                }
            }

            if (match(token::kind::mutable_kw)) {
                specifiers.push_back(previous());
                mutability = true;
            }

            if (match(token::kind::val_kw));
            else if (match(token::kind::const_kw)) {
                constant = true;

                if (!specifiers.empty()) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(current().location())
                                .message("Constants cannot have automatic, static or dynamic lifetime, b*tch!")
                                .explanation(impl::var_decl_explanation)
                                .replacement(previous().range(), "val", "If you want to customize lifetime or mutability then try this way")
                                .highlight(source_range(saved.iter->location(), specifiers.back().range().end()), diagnostic::highlighter::mode::light)
                                .highlight(previous().range(), "maybe `val`");
            
                    publisher_.publish(builder.build());
                    err = true;
                }
            }
            else {
                consume(token::kind::const_kw, specifiers.empty() ? "`val` or `const`" : "`val`", diagnostic::format("I need $ keyword after `$` here!", specifiers.empty() ? "`val` or `const`" : "`val`", previous().lexeme()), impl::var_decl_explanation);
            }

            if (current().is(token::kind::identifier) && next().is(token::kind::comma)) {
                tupled = true;

                do {
                    if (names.size() >= guard::max_elements) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(current().location())
                                    .message("Max number of elements inside tupled declaration reached during parsing, f*cking hell!")
                                    .explanation(impl::guard_explanation)
                                    .highlight(source_range(current().range()), "here");
                        
                        abort(builder.build());
                    }

                    names.push_back(consume(token::kind::identifier, "name", diagnostic::format("I expect tupled variable name after `$`, don't you think?", previous().lexeme()), impl::var_decl_explanation));
                }
                while (match(token::kind::comma));        
            }
            else if (match(token::kind::identifier)) {
                names.push_back(previous());
            }
            else {
                consume(token::kind::identifier, "name(s)", diagnostic::format("I need variable name after `$`, idiot!", previous().lexeme()), impl::var_decl_explanation);
            }

            if (match(token::kind::colon)) {
                type_expr = expect(type_expression(), "type", "I expect variable type after `:`, dammit!", impl::var_decl_explanation);
                if (!constant) std::dynamic_pointer_cast<ast::type_expression>(type_expr)->set_mutable(mutability);
            }

            consume(token::kind::equal, "`=`", "You always have to initialize a variable, I guess they already told you this a thousand times before, c*nt!", impl::var_decl_explanation);
            value = expect(expression(), "expression", "I need variable value after `=`, idiot!", impl::var_decl_explanation);
            match(token::kind::semicolon);

            if (constant) {
                if (tupled) {
                    decl = ast::create<ast::const_tupled_declaration>(source_range(saved.iter->location(), previous().range().end()), names, type_expr, value);
                }
                else {
                    decl = ast::create<ast::const_declaration>(source_range(saved.iter->location(), previous().range().end()), names.front(), type_expr, value);    
                }
            }
            else {
                if (tupled) {
                    decl = ast::create<ast::var_tupled_declaration>(source_range(saved.iter->location(), previous().range().end()), specifiers, names, type_expr, value);
                }
                else {
                    decl = ast::create<ast::var_declaration>(source_range(saved.iter->location(), previous().range().end()), specifiers, names.front(), type_expr, value);
                }
            }
            
            decl->invalid(err);
            return decl;
        }
        
        return nullptr;
    }

    ast::pointer<ast::declaration> parser::test_declaration()
    {
        guard guard(this);
        state saved = state_;

        if (match(token::kind::test_kw)) {
            token test = previous();
            token name = consume(token::kind::identifier, "name", "You have to give your test a name, don't you think?", impl::test_decl_explanation);
            ast::pointer<ast::expression> body = expect(block_expression(), "body", "I expect test block in this place, idiot!", impl::test_decl_explanation);
            match(token::kind::semicolon);
            return ast::create<ast::test_declaration>(source_range(saved.iter->location(), previous().range().end()), name, body);
        }

        return nullptr;
    }
    
    ast::pointer<ast::declaration> parser::source_unit_declaration()
    {
        guard guard(this);
        state saved = state_;
        ast::pointers<ast::statement> imports, statements;

        while (!eof()) try {
            if (statements.size() >= guard::max_statements) {
                auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(current().location())
                            .message("Max number of statements reached during parsing, f*cking hell!")
                            .explanation(impl::guard_explanation)
                            .highlight(source_range(current().range()), "here");
                
                abort(builder.build());
            }

            bool hidden = match(token::kind::hide_kw);
            ast::pointer<ast::declaration> decl = nullptr;

            switch (current().kind()) {
                case token::kind::app_kw:
                case token::kind::lib_kw:
                    decl = workspace_declaration();
                    if (workspace_) statements.push_back(decl);
                    else workspace_ = decl;
                    break;
                case token::kind::use_kw:
                    decl = use_declaration();
                    imports.push_back(decl);
                    break;
                case token::kind::function_kw:
                case token::kind::identifier:
                    decl = function_declaration();
                    statements.push_back(decl);
                    break;
                case token::kind::type_kw:
                    decl = type_declaration();
                    statements.push_back(decl);
                    break;
                case token::kind::concept_kw:
                    decl = concept_declaration();
                    statements.push_back(decl);
                    break;
                case token::kind::extern_kw:
                    decl = extern_declaration();
                    statements.push_back(decl);
                    break;
                case token::kind::extend_kw:
                    decl = extend_declaration();
                    statements.push_back(decl);
                    break;
                case token::kind::behaviour_kw:
                    decl = behaviour_declaration();
                    statements.push_back(decl);
                    break;
                case token::kind::static_kw:
                case token::kind::const_kw:
                case token::kind::mutable_kw:
                case token::kind::val_kw:
                    decl = variable_declaration();
                    statements.push_back(decl);
                    break;
                case token::kind::test_kw:
                    decl = test_declaration();
                    statements.push_back(decl);
                    break;
                default:
                    expect(decl, "declaration", "I was expecting a declaration here, dumb*ss!", impl::source_unit_decl_explanation);
            }

            if (decl) {
                decl->hidden(hidden);
            }

            separator(decl);
        }
        catch (...) {
            break;
        }

        return ast::create<ast::source_unit_declaration>(source_range(saved.iter->location(), previous().range().end()), workspace_, imports, statements);
    }

    void parser::separator(ast::pointer<ast::statement> before)
    {
        if (!previous().eol && !previous().is(token::kind::semicolon) && 
            !current().is(token::kind::eof) && !current().is(token::kind::semicolon) && !current().is(token::kind::right_brace)) {
            auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(current().location())
                            .explanation(impl::local_decl_explanation);

            switch (current().kind()) {
                case token::kind::app_kw:
                case token::kind::lib_kw:
                case token::kind::use_kw:
                case token::kind::function_kw:
                case token::kind::identifier:
                case token::kind::type_kw:
                case token::kind::concept_kw:
                case token::kind::extern_kw:
                case token::kind::extend_kw:
                case token::kind::behaviour_kw:
                case token::kind::static_kw:
                case token::kind::const_kw:
                case token::kind::val_kw:
                case token::kind::test_kw:
                    builder.message("You cannot write statements on the same line, pr*ck!")
                           .insertion(source_range(previous().range().end(), 1), ";", "Try dividing statements with `;` on the same line")
                           .highlight(before->range(), diagnostic::highlighter::mode::light)
                           .highlight(current().range(), "expected `;` before");
                    break;
                default:
                    builder.message("I was expecting a statement terminator here, idiot!")
                           .highlight(before->range(), diagnostic::highlighter::mode::light)
                           .highlight(current().range(), "maybe `;`");
            }

            abort(builder.build());
        }

        match(token::kind::semicolon);
    }

    ast::pointer<ast::statement> parser::parse()
    {
        try {
            return source_unit_declaration();
        }
        catch (syntax_error& err) {
            return nullptr;
        }

        return nullptr;
    }
}
