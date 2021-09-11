#include <algorithm>
#include <fstream>
#include <stack>
#include <regex>

#include "nemesis/codegen/code_generator.hpp"
#include "nemesis/analysis/environment.hpp"
#include "nemesis/analysis/evaluator.hpp"

namespace nemesis {
    code_generator::code_generator(checker& checker) : checker_(checker) {}

    code_generator::~code_generator() {}
    
    std::list<compilation::target> code_generator::generate()
    {
        // cpp target files
        std::list<compilation::target> targets;
        // public header will contains all public declarations and headers from `cpp` directories
        filestream exported("__exported.h");
        // adds all `src` header files (.h, .hpp)
        for (auto package : checker_.compilation().packages()) {
            // include headers from `src` directory
            for (auto cpp : package.second.cpp_sources) {
                // if extension is '.h' or '.hpp' then file is included
                if (cpp->has_type(source_file::filetype::header)) exported.stream() << "#include \"" << cpp->name() << "\"\n";
            }
        }
        // all exported declarations are added to a standard header file
        for (auto workspace : checker_.compilation().workspaces()) {
            exported.stream() << "/* Forward declarations from workspace '" << workspace.first << "' */\n";
            // if package associated to current workspace is builtin, then workspace is not compiled
            if (!checker_.compilation().package(workspace.second->package).builtin) {
                // sets current library
                workspace_ = workspace.second;
                // forward type declarations
                for (auto type : workspace.second->types) {
                    if (auto typedecl = dynamic_cast<const ast::type_declaration*>(type->declaration())) exported.stream() << prototype(typedecl) << ";\n";
                    else exported.stream() << "struct " << emit(type) << ";\n";
                }
                // forward function declarations
                for (auto fndecl : workspace.second->functions) if (!fndecl->generic()) exported.stream() << prototype(fndecl) << ";\n";
            }
        }
        // adds exported header file to targets
        targets.push_front({ "__exported.h", exported.stream().str(), true });

        std::cout << "--- __exported.h ---\n" << exported.stream().str() << '\n';

        // for each workspace it generates a new C++ file to compile if workspace's package is not set as builtin
        for (auto workspace : checker_.compilation().workspaces()) {
            // whenever we encounter a builtin package then we skip its workspaces' code generation
            if (checker_.compilation().package(workspace.second->package).builtin) continue;
            // target name
            std::string target = workspace.first + ".cpp";
            // constructs a new file stream
            output_ = filestream(target);
            // include public header
            output_.stream() << "#include \"__exported.h\"\n";
            // emits all type declarations inside each file
            output_.stream() << "/* Type definitions */\n";
            for (auto type : workspace.second->types) {
                if (auto typedecl = dynamic_cast<const ast::type_declaration*>(type->declaration())) typedecl->accept(*this);
                else emit_anonymous_type(type);
            }
            // global variables definitions
            output_.stream() << "/* Variable and constants definitions */\n";
            for (auto valuedecl : workspace.second->globals) valuedecl->accept(*this);
            // function definitions
            output_.stream() << "/* Function definitions */\n";
            for (auto fndecl : workspace.second->functions) if (!fndecl->generic()) fndecl->accept(*this);
            // adds new cpp file to targets list
            targets.push_back({ target, output_.stream().str() });

            std::cout << "---" << workspace.first << ".cpp---\n" << output_.stream().str() << '\n';
        }
        // yields cpp targets list
        return targets;
    }

    namespace impl {
        static std::unordered_map<std::string, std::string> cpp_builtins = {
            { "unit", "void" },
            { "bool", "bool" },
            { "char", "__char" },
            { "chars", "__chars" },
            { "string", "std::string" },
            { "u8", "std::uint8_t" },
            { "u16", "std::uint16_t" },
            { "u32", "std::uint32_t" },
            { "u64", "std::uint64_t" },
            { "u128", "std::uint128_t" },
            { "usize", sizeof(size_t) == 4 ? "std::uint32_t" : "std::uint64_t" },
            { "i8", "std::int8_t" },
            { "i16", "std::int16_t" },
            { "i32", "std::int32_t" },
            { "i64", "std::int64_t" },
            { "i128", "std::int128_t" },
            { "isize", sizeof(size_t) == 4 ? "std::int32_t" : "std::int64_t" },
            { "r16", "__rational<std::int8_t>" },
            { "r32", "__rational<std::int16_t>" },
            { "r64", "__rational<std::int32_t>" },
            { "r128", "__rational<std::int64_t>" },
            { "r256", "__rational<std::int128_t>" },
            { "f32", "float" },
            { "f64", "double" },
            { "f128", "long double" },
            { "c64", "std::complex<float>" },
            { "c128", "std::complex<double>" },
            { "c256", "std::complex<long double>" }
        };
    }

    void code_generator::trace(bool flag) { trace_ = flag; }

    bool code_generator::trace() const { return trace_; }

    std::string code_generator::emit(ast::pointer<ast::type> type) const
    {
        auto it = impl::cpp_builtins.find(type->string());

        if (it != impl::cpp_builtins.end()) return it->second;

        if (type->declaration()) return fullname(type->declaration());
        else if (type->category() == ast::type::category::array_type) {
            std::ostringstream suffix;

            do {
                auto array_type = std::static_pointer_cast<ast::array_type>(type);
                suffix << "[" << array_type->size() << "]";
                type = array_type->base();
            }
            while (type->category() == ast::type::category::array_type);

            return emit(type) + suffix.str();
        }
        else if (auto tuple_type = std::dynamic_pointer_cast<ast::tuple_type>(type)) {
            std::ostringstream result;

            result << "std::tuple<";

            if (tuple_type->components().size() >= 1) result << emit(tuple_type->components().front());
            
            for (std::size_t i = 1; i < tuple_type->components().size(); ++i) result << ", " << emit(tuple_type->components().at(i));

            result << ">";

            return result.str();
        }
        else if (auto pointer_type = std::dynamic_pointer_cast<ast::pointer_type>(type)) {
            return emit(pointer_type->base()) + "*";
        }
        else if (auto slice_type = std::dynamic_pointer_cast<ast::slice_type>(type)) {
            return "__slice<" + emit(slice_type->base()) + ">";
        } 
        else if (auto fun_type = std::dynamic_pointer_cast<ast::function_type>(type)) {
            std::ostringstream result;

            if (types::compatible(types::unit(), fun_type->result())) result << "void";
            else result << emit(fun_type->result());

            result << " (*)(";

            for (unsigned i = 0; i < fun_type->formals().size(); ++i) {
                if (i > 0) result << ", ";
                result << emit(fun_type->formals().at(i));
            }

            result << ")";

            return result.str();
        }
        // anonymous structure/variant is instantiated
        else if (type->category() == ast::type::category::structure_type || type->category() == ast::type::category::variant_type) return "__T" + std::to_string(std::hash<std::string>()(type->string()));

        auto result = type->string();

        std::replace(result.begin(), result.end(), '.', '_');

        return result;
    }

    std::string code_generator::emit(ast::pointer<ast::type> type, std::string variable) const
    {
        auto it = impl::cpp_builtins.find(type->string());

        if (it != impl::cpp_builtins.end()) return it->second + " " + variable;

        if (type->declaration()) return fullname(type->declaration()) + " " + variable;
        else if (type->category() == ast::type::category::array_type) {
            std::ostringstream suffix;

            do {
                auto array_type = std::static_pointer_cast<ast::array_type>(type);
                suffix << "[" << array_type->size() << "]";
                type = array_type->base();
            }
            while (type->category() == ast::type::category::array_type);

            return emit(type) + " " + variable + suffix.str();
        }
        else if (auto tuple_type = std::dynamic_pointer_cast<ast::tuple_type>(type)) {
            std::ostringstream result;

            result << "std::tuple<";

            if (tuple_type->components().size() >= 1) result << emit(tuple_type->components().front());
            
            for (std::size_t i = 1; i < tuple_type->components().size(); ++i) result << ", " << emit(tuple_type->components().at(i));

            result << ">";

            return result.str() + " " + variable;
        }
        else if (auto pointer_type = std::dynamic_pointer_cast<ast::pointer_type>(type)) {
            return emit(pointer_type->base()) + "* " + variable;
        }
        else if (auto slice_type = std::dynamic_pointer_cast<ast::slice_type>(type)) {
            return "__slice<" + emit(slice_type->base()) + "> " + variable;
        }
        else if (auto fun_type = std::dynamic_pointer_cast<ast::function_type>(type)) {
            std::ostringstream result;

            if (types::compatible(types::unit(), fun_type->result())) result << "void";
            else result << emit(fun_type->result());

            result << " (*" << variable << ")(";

            for (unsigned i = 0; i < fun_type->formals().size(); ++i) {
                if (i > 0) result << ", ";
                result << emit(fun_type->formals().at(i));
            }

            result << ")";

            return result.str();
        }
        // anonymous structure/variant is instantiated
        else if (type->category() == ast::type::category::structure_type || type->category() == ast::type::category::variant_type) return "__T" + std::to_string(std::hash<std::string>()(type->string())) + " " + variable;

        auto result = type->string();

        std::replace(result.begin(), result.end(), '.', '_');

        return result + " " + variable;
    }

    std::string code_generator::emit(constval value) const
    {
        switch (value.type->category()) {
            case nemesis::ast::type::category::bool_type:
                return (value.b ? "true" : "false");
            case nemesis::ast::type::category::char_type:
                return std::to_string(value.ch);
            case nemesis::ast::type::category::chars_type:
                return "__chars(\"" + value.s + "\")";
            case nemesis::ast::type::category::string_type:
                return "std::string(\"" + value.s + "\")";
            case nemesis::ast::type::category::integer_type:
            {
                auto type = std::dynamic_pointer_cast<nemesis::ast::integer_type>(value.type);
                if (type->is_signed()) return std::to_string(value.i.value()); 
                else return std::to_string(value.u.value()) + "u";
            }
            case nemesis::ast::type::category::rational_type:
            {
                auto type = std::dynamic_pointer_cast<nemesis::ast::rational_type>(value.type);
                return emit(type) + "(" + std::to_string(value.r.numerator().value()) + ", " + std::to_string(value.r.denominator().value()) + ")";
            }
            case nemesis::ast::type::category::float_type:
                return std::to_string(value.f.value());
            case nemesis::ast::type::category::complex_type:
            {
                auto type = std::dynamic_pointer_cast<nemesis::ast::complex_type>(value.type);
                return emit(type) + "(" + std::to_string(value.c.real().value()) + ", " + std::to_string(value.c.imag().value()) + ")";
            }
            case nemesis::ast::type::category::tuple_type:
                if (value.seq.empty()) return "std::tuple<>()";
                else {
                    std::ostringstream os;
                    os << emit(value.type) << "(" << emit(value.seq.front());
                    for (size_t i = 1; i < value.seq.size(); ++i) os << ", " << emit(value.seq.at(i));
                    os << ")";
                    return os.str();
                }
            case nemesis::ast::type::category::array_type:
                if (value.seq.empty()) return "{}";
                else {
                    std::ostringstream os;
                    os << "{" << emit(value.seq.front());
                    for (size_t i = 1; i < value.seq.size(); ++i) os << ", " << emit(value.seq.at(i));
                    os << "}";
                    return os.str();
                }
            default:
                throw std::invalid_argument("code_generator::emit: invalid constant value type " + value.type->string());
        }
    }

    void code_generator::emit_anonymous_type(ast::pointer<ast::type> type)
    {
        struct guard guard(output_);

        if (auto tuple_type = std::dynamic_pointer_cast<ast::tuple_type>(type)) {
            output_.line() << "struct " << emit(type) << " {\n";
            
            {
                struct guard inner(output_);
                for (unsigned int i = 0; i < tuple_type->components().size(); ++i) output_.line() << emit(tuple_type->components().at(i), "_" + std::to_string(i));
            }

            output_.line() << "};\n";
        }
        else if (auto structure_type = std::dynamic_pointer_cast<ast::structure_type>(type)) {
            output_.line() << "struct " << emit(type) << " {\n";
            
            {
                struct guard inner(output_);
                for (auto field : structure_type->fields()) output_.line() << emit(field.type, field.name);
            }

            output_.line() << "};\n";
        }
        else if (auto variant_type = std::dynamic_pointer_cast<ast::variant_type>(type)) {
            output_.line() << "struct " << emit(type) << " {\n";

            {
                struct guard inner(output_);
                // tag field is used to identify current type
                output_.line() << "std::size_t __tag;\n";
                // for each subtype there exists a field whose name is the hash of type name inside an anonymous union (no duplicate types!!)
                output_.line() << "union {\n";
                
                {
                    struct guard inner(output_);
                    // each field name is the hash of its type (unique within a variant)
                    for (auto subtype : variant_type->types()) {
                        output_.line() << emit(subtype, "_" + std::to_string(std::hash<std::string>()(subtype->string()))) << ";\n";
                    }
                }
                
                output_.line() << "};\n";
                // constructor, copy-constructor and destructor are defined for non-trivial members of the union
                output_.line() << emit(type) << "() {}\n";
                output_.line() << emit(type) << "(const " << emit(type) << "& other) : __tag(other.__tag) {\n";
                {
                    struct guard inner(output_);
                    output_.line() << "switch (other.__tag) {\n";
                    for (auto subtype : std::static_pointer_cast<ast::variant_type>(type)->types()) {
                        auto hash = std::to_string(std::hash<std::string>()(subtype->string()));
                        output_.line() << "case " << hash << "ull: _" << hash << " = other._" << hash << "; break;\n";
                    }
                    output_.line() << "default: break;\n";
                    output_.line() << "}\n";
                }
                output_.line() << "}\n";
                output_.line() << "~" << emit(type) << "() {}\n";
            }

            output_.line() << "};\n";

            // create designated initializers for each variant type
            for (auto subtype : variant_type->types()) {
                auto hash = std::to_string(std::hash<std::string>()(subtype->string()));
                
                // first emits constructors for variant using each of its types
                output_.line() << emit(type) << " " << emit(type) << "_init_" << hash << "(" << emit(subtype, "init") << ") {\n";
                
                {
                    struct guard inner(output_);
                    output_.line() << emit(type, "result") << ";\n";
                    output_.line() << "result.__tag = " << hash << "ull;\n";
                    output_.line() << "result._" << hash << " = init;\n";
                    output_.line() << "return result;\n";
                }

                output_.line() << "}\n";

                // then emits explicit conversions to each of its types
                output_.line() << emit(subtype) << " " << emit(type) << "_as_" << hash << "(" << emit(type, "self") << ", const char* file, int line, int column) {\n";
                
                {
                    struct guard inner(output_);
                    output_.line() << "if (self.__tag != " << hash << "ull) __crash(\"damn, you cannot explictly convert variant since it has a different run-time type!\", file, line, column);\n";
                    output_.line() << "return self._" << hash << "; \n";
                }

                output_.line() << "}\n";
            }
        }
        else throw std::invalid_argument("code_generator::emit_anonymous_type(): invalid type " + type->string());
    }

    void code_generator::emit_in_contracts(const ast::node& current)
    {
        if (auto outer = checker_.scopes().at(&current)->outscope(environment::kind::loop)) {
            // first, before return statement, we must test invariant and ensure contracts, if any
            if (auto loop = dynamic_cast<const ast::for_loop_expression*>(outer)) {
                for (auto contract : loop->contracts()) if (!std::static_pointer_cast<ast::contract_statement>(contract)->is_ensure()) contract->accept(*this);
            }
            else if (auto loop = dynamic_cast<const ast::for_range_expression*>(outer)) {
                for (auto contract : loop->contracts()) if (!std::static_pointer_cast<ast::contract_statement>(contract)->is_ensure()) contract->accept(*this);
            }
        }
        else if (auto outer = checker_.scopes().at(&current)->outscope(environment::kind::function)) {
            if (auto function = dynamic_cast<const ast::function_declaration*>(outer)) {
                for (auto contract : function->contracts()) if (!std::static_pointer_cast<ast::contract_statement>(contract)->is_ensure()) contract->accept(*this);
            }
            else if (auto function = dynamic_cast<const ast::property_declaration*>(outer)) {
                for (auto contract : function->contracts()) if (!std::static_pointer_cast<ast::contract_statement>(contract)->is_ensure()) contract->accept(*this);
            }
        }
    }
    
    void code_generator::emit_out_contracts(const ast::node& current)
    {
        if (auto outer = checker_.scopes().at(&current)->outscope(environment::kind::loop)) {
            if (auto loop = dynamic_cast<const ast::for_loop_expression*>(outer)) {
                for (auto contract : loop->contracts()) if (!std::static_pointer_cast<ast::contract_statement>(contract)->is_require()) contract->accept(*this);
            }
            else if (auto loop = dynamic_cast<const ast::for_range_expression*>(outer)) {
                for (auto contract : loop->contracts()) if (!std::static_pointer_cast<ast::contract_statement>(contract)->is_require()) contract->accept(*this);
            }
        }
        else if (auto outer = checker_.scopes().at(&current)->outscope(environment::kind::function)) {
            if (auto function = dynamic_cast<const ast::function_declaration*>(outer)) {
                for (auto contract : function->contracts()) if (!std::static_pointer_cast<ast::contract_statement>(contract)->is_require()) contract->accept(*this);
            }
            else if (auto function = dynamic_cast<const ast::property_declaration*>(outer)) {
                for (auto contract : function->contracts()) if (!std::static_pointer_cast<ast::contract_statement>(contract)->is_require()) contract->accept(*this);
            }
        }
    }
    
    std::string code_generator::fullname(const ast::declaration *decl) const
    {
        if (decl == checker_.entry_point()) return "main";

        std::stack<std::string> levels;
        
        for (bool done = false; decl && !done;) {
            switch (decl->kind()) {
                case ast::kind::workspace:
                    // in case of primitive workspace `core`, then we need to rearrange declaration name to be preceeded by `__`
                    if (static_cast<const ast::workspace*>(decl)->name == "core") levels.push("_");
                    // normal workspace, then its name is used
                    else levels.push(static_cast<const ast::workspace*>(decl)->name);
                    done = true;
                    break;
                case ast::kind::function_declaration:
                    // function name scope is added only if it is the function full name to be resolved
                    if (!levels.empty()) done = true;
                    else levels.push(static_cast<const ast::function_declaration*>(decl)->name().lexeme().string());
                    break;
                case ast::kind::property_declaration:
                    // property name scope is added only if it is the property full name to be resolved
                    if (!levels.empty()) done = true;
                    else levels.push(static_cast<const ast::property_declaration*>(decl)->name().lexeme().string());
                    break;
                case ast::kind::behaviour_declaration:
                case ast::kind::record_declaration:
                case ast::kind::range_declaration:
                case ast::kind::variant_declaration:
                case ast::kind::alias_declaration:
                    levels.push(static_cast<const ast::type_declaration*>(decl)->name().lexeme().string());
                    break;
                case ast::kind::var_declaration:
                    levels.push(static_cast<const ast::var_declaration*>(decl)->name().lexeme().string());
                    break;
                case ast::kind::const_declaration:
                    levels.push(static_cast<const ast::var_declaration*>(decl)->name().lexeme().string());
                    break;
                case ast::kind::field_declaration:
                    levels.push(static_cast<const ast::field_declaration*>(decl)->name().lexeme().string());
                    done = true;
                    break;
                case ast::kind::tuple_field_declaration:
                    levels.push("_" + std::to_string(static_cast<const ast::tuple_field_declaration*>(decl)->index()));
                    done = true;
                    break;
                case ast::kind::parameter_declaration:
                    levels.push(static_cast<const ast::parameter_declaration*>(decl)->name().lexeme().string());
                    break;
                default:
                    break;
            }

            if (auto newscope = dynamic_cast<const ast::declaration*>(decl->annotation().scope)) decl = newscope;
            else done = true;
        }

        std::ostringstream result;

        if (!levels.empty()) {
            result << levels.top();
            levels.pop();

            for (; !levels.empty(); levels.pop()) result << "_" << levels.top();
        }

        auto name = result.str();

        std::replace_if(name.begin(), name.end(), [](char c) { return c == '(' || c == ')'; }, '_');
        name = std::regex_replace(name, std::regex(", |,"), "_");
        
        return name;
    }

    std::string code_generator::prototype(const ast::type_declaration* decl) const
    { 
        if (decl->generic()) throw std::invalid_argument("code_generator::prototype: cannot generate prototype for generic type " + decl->annotation().type->string());
        return "struct " + fullname(decl);
    }
        
    std::string code_generator::prototype(const ast::function_declaration* decl) const
    {
        if (decl->generic()) throw std::invalid_argument("code_generator::prototype: cannot generate prototype for generic function " + decl->name().lexeme().string());
        auto fntype = std::static_pointer_cast<ast::function_type>(decl->annotation().type);
        std::ostringstream result;

        if (decl == checker_.entry_point()) return "int main(int __argc, char **__argv)";

        // each associated is a static (class) method
        if (dynamic_cast<const ast::type_declaration*>(checker_.scopes().at(decl)->outscope(environment::kind::declaration))) result << "static ";

        if (types::compatible(types::unit(), fntype->result())) result << "void ";
        else result << emit(fntype->result()) << " ";

        result << fullname(decl) << "(";

        for (std::size_t i = 0; i < decl->parameters().size(); ++i) {
            if (i > 0) result << ", ";
            result << emit(decl->parameters().at(i)->annotation().type, std::static_pointer_cast<ast::parameter_declaration>(decl->parameters().at(i))->name().lexeme().string());
        }

        result << ")";

        return result.str();
    }

    std::string code_generator::prototype(const ast::property_declaration* decl) const
    {
        auto fntype = std::static_pointer_cast<ast::function_type>(decl->annotation().type);
        std::ostringstream result;

        // each associated is a static (class) method
        if (dynamic_cast<const ast::type_declaration*>(checker_.scopes().at(decl)->outscope(environment::kind::declaration))) result << "static ";

        if (types::compatible(types::unit(), fntype->result())) result << "void ";
        else result << emit(fntype->result()) << " ";

        result << fullname(decl) << "(" << emit(decl->parameters().front()->annotation().type, std::static_pointer_cast<ast::parameter_declaration>(decl->parameters().front())->name().lexeme().string()) << ")";

        return result.str();
    }

    void code_generator::visit(const ast::extend_declaration& decl) {}
    
    void code_generator::visit(const ast::behaviour_declaration& decl) 
    {
        if (decl.generic()) return;

        // its virtual table type as a structure
        {
            guard guard(output_);

            output_.line() << "struct __vtable_" << fullname(&decl) << " {\n";
            
            {
                struct guard inner(output_);
                // offset size to determine at which distance (in bytes) the base object (of behavioural type) begins
                output_.line() << "std::size_t __offset;\n";
            }

            for (auto prototype : decl.declarations()) {
                struct guard inner(output_);
                // function pointers to virtual methods
                if (auto function = std::dynamic_pointer_cast<ast::function_declaration>(prototype)) output_.line() << "void (*" << function->name().lexeme() << ")(void*);\n";
                else if (auto property = std::dynamic_pointer_cast<ast::property_declaration>(prototype)) output_.line() << "void (*" << property->name().lexeme() << ")(void*);\n";
            }

            output_.line() << "};\n";
        }
        // then base type is emitted for upcasting and downcasting (it contains pointer to virtual table)
        {
            guard guard(output_);

            output_.line() << prototype(&decl) << " {\n";

            {
                struct guard inner(output_);
                
                output_.line() << "__vtable_" << fullname(&decl) << "* __vptr_" << fullname(&decl) << ";\n";
            }

            output_.line() << "};\n";
        }
        // now emits dispatcher functions which will call directly virtual table functions
        for (auto proto : decl.declarations()) {
            if (auto function = std::dynamic_pointer_cast<ast::function_declaration>(proto)) {
                auto self = std::static_pointer_cast<ast::parameter_declaration>(function->parameters().front())->name().lexeme();
                output_.line() << "inline " << prototype(function.get()) << " { return ((" << emit(function->annotation().type) << ") " << self << "->__vptr_" << fullname(&decl) << "->" << function->name().lexeme() << ")(";
                // first argument is object whose real address is computed by using the offset in its virtual table associated to current behaviour
                output_.stream() << "(" << emit(function->parameters().front()->annotation().type) << ") ((char*) " << self << " - " << self << "->__vptr_" << fullname(&decl) << "->__offset)";
                for (unsigned i = 1; i < function->parameters().size(); ++i) {
                    output_.stream() << ", " << std::static_pointer_cast<ast::parameter_declaration>(function->parameters().at(i))->name().lexeme();
                }
                output_.stream() << "); }\n";
            }
            else if (auto function = std::dynamic_pointer_cast<ast::property_declaration>(proto)) {
                auto self = std::static_pointer_cast<ast::parameter_declaration>(function->parameters().front())->name().lexeme();
                output_.line() << "inline " << prototype(function.get()) << " { return ((" << emit(function->annotation().type) << ") " << self << "->__vptr_" << fullname(&decl) << "->" << function->name().lexeme() << ")(";
                // first argument is object whose real address is computed by using the offset in its virtual table associated to current behaviour
                output_.stream() << "(" << emit(function->parameters().front()->annotation().type) << ") ((char*) " << self << " - " << self << "->__vptr_" << fullname(&decl) << "->__offset)";
                for (unsigned i = 1; i < function->parameters().size(); ++i) {
                    output_.stream() << ", " << std::static_pointer_cast<ast::parameter_declaration>(function->parameters().at(i))->name().lexeme();
                }
                output_.stream() << "); }\n";
            }
        }
    }
    
    void code_generator::visit(const ast::extern_declaration& decl) {}
    
    void code_generator::visit(const ast::range_declaration& decl) {}
    
    void code_generator::visit(const ast::record_declaration& decl)
    {
        if (decl.generic()) return;

        {
            guard guard(output_);

            output_.line() << prototype(&decl) << " {\n";
            // first emits vpointers if any
            if (types::implementors().count(decl.annotation().type)) {
                struct guard inner(output_);

                for (auto behaviour : types::implementors().at(decl.annotation().type)) {
                    auto bname = fullname(behaviour->declaration());
                    output_.line() << "__vtable_" << bname << "* __vptr_" << bname << ";\n";
                }
            }
            // then fields
            for (auto field : decl.fields()) field->accept(*this);
            // prototype of constructor for fields
            {
                struct guard inner(output_);
                unsigned ifield = 0;
                output_.line() << emit(decl.annotation().type) << "(";
                for (auto field : decl.fields()) {
                    if (ifield > 0) output_.stream() << ", "; 
                    output_.stream() << emit(field->annotation().type, std::static_pointer_cast<ast::field_declaration>(field)->name().lexeme().string());
                    ++ifield;
                }
                output_.stream() << ");\n";
            }
            output_.line() << "};\n";
        }

        if (checker_.scopes().count(&decl)) {
            for (auto constant : checker_.scopes().at(&decl)->values()) constant.second->accept(*this);
            for (auto function : checker_.scopes().at(&decl)->functions()) function.second->accept(*this);
        }

        // emit vtables for this type
        if (types::implementors().count(decl.annotation().type)) {
            struct guard inner(output_);

            for (auto behaviour : types::implementors().at(decl.annotation().type)) {
                auto bname = fullname(behaviour->declaration());
                output_.line() << "static __vtable_" << bname << " __vtable_" << fullname(behaviour->declaration()) << "_for_" << emit(decl.annotation().type) << " = { ";
                // set offset of this vptr inside structure
                output_.stream() << "offsetof(" << emit(decl.annotation().type) << ", __vptr_" << bname << ")";
                // fill vtable with function pointers
                for (auto prototype : static_cast<const ast::behaviour_declaration*>(behaviour->declaration())->declarations()) {
                    output_.stream() << ", ";
                    if (auto function = std::dynamic_pointer_cast<ast::function_declaration>(prototype)) output_.stream() << "(void (*)(void*)) &" << emit(decl.annotation().type) << "_" << function->name().lexeme();
                    else if (auto function = std::dynamic_pointer_cast<ast::property_declaration>(prototype)) output_.stream() << "(void (*)(void*)) &" << emit(decl.annotation().type) << "_" << function->name().lexeme();
                }
                output_.stream() << " };\n";
            }
        }

        // emit constructor initializing virtual pointers eventually
        {
            struct guard inner(output_);
            unsigned ifield = 0;
            output_.line() << emit(decl.annotation().type) << "::" << emit(decl.annotation().type) << "(";
            for (auto field : decl.fields()) {
                if (ifield > 0) output_.stream() << ", "; 
                output_.stream() << emit(field->annotation().type, std::static_pointer_cast<ast::field_declaration>(field)->name().lexeme().string());
                ++ifield;
            }
            output_.stream() << ") : ";
            ifield = 0;
            // initialize vpointers
            if (types::implementors().count(decl.annotation().type)) {
                for (auto behaviour : types::implementors().at(decl.annotation().type)) {
                    auto bname = fullname(behaviour->declaration());
                    if (ifield > 0) output_.stream() << ", ";
                    output_.stream() << "__vptr_" << bname << "(&__vtable_" << bname << "_for_" << emit(decl.annotation().type) << ")";
                    ++ifield;
                }
            }
            // initialize fields
            for (auto field : decl.fields()) {
                auto fname = std::static_pointer_cast<ast::field_declaration>(field)->name().lexeme().string();
                if (ifield > 0) output_.stream() << ", ";
                output_.stream() << fname << "(" << fname << ")";
                ++ifield;
            }
            output_.stream() << " {}\n";
        }
    }
    
    void code_generator::visit(const ast::variant_declaration& decl) 
    {
        if (decl.generic()) return;

        {
            guard guard(output_);

            output_.line() << prototype(&decl) << " {\n";

            {
                struct guard inner(output_);
                // first emits vpointers if any
                if (types::implementors().count(decl.annotation().type)) {
                    for (auto behaviour : types::implementors().at(decl.annotation().type)) {
                        auto bname = fullname(behaviour->declaration());
                        output_.line() << "__vtable_" << bname << "* __vptr_" << bname << ";\n";
                    }
                }
                // tag field is used to identify current type
                output_.line() << "std::size_t __tag;\n";
                // for each subtype there exists a field whose name is the hash of type name inside an anonymous union (no duplicate types!!)
                output_.line() << "union {\n";
                
                {
                    struct guard inner(output_);
                    // each field name is the hash of its type (unique within a variant)
                    for (auto type : std::static_pointer_cast<ast::variant_type>(decl.annotation().type)->types()) {
                        output_.line() << emit(type, "_" + std::to_string(std::hash<std::string>()(type->string()))) << ";\n";
                    }
                }
                
                output_.line() << "};\n";

                // definitions of routine functions for non-trivial members of the union
                // constructor
                output_.line() << emit(decl.annotation().type) << "();\n";
                // copy constructor
                output_.line() << emit(decl.annotation().type) << "(const " << emit(decl.annotation().type) << "& other) : __tag(other.__tag) {\n";
                {
                    struct guard inner(output_);
                    output_.line() << "switch (other.__tag) {\n";
                    for (auto subtype : std::static_pointer_cast<ast::variant_type>(decl.annotation().type)->types()) {
                        auto hash = std::to_string(std::hash<std::string>()(subtype->string()));
                        output_.line() << "case " << hash << "ull: new(&_" << hash << ") " << emit(subtype) << "(other._" << hash << "); break;\n"; 
                    }
                    output_.line() << "default: break;\n";
                    output_.line() << "}\n";
                }
                output_.line() << "}\n";
                // copy assignment
                output_.line() << "void operator=(const " << emit(decl.annotation().type) << "& other) {\n";
                {
                    struct guard inner(output_);
                    output_.line() << "__tag = other.__tag;\n";
                    output_.line() << "switch (other.__tag) {\n";
                    for (auto subtype : std::static_pointer_cast<ast::variant_type>(decl.annotation().type)->types()) {
                        auto hash = std::to_string(std::hash<std::string>()(subtype->string()));
                        output_.line() << "case " << hash << "ull: new(&_" << hash << ") " << emit(subtype) << "(other._" << hash << "); break;\n";
                    }
                    output_.line() << "default: break;\n";
                    output_.line() << "}\n";
                }
                output_.line() << "}\n";
                // destructor
                output_.line() << "~" << emit(decl.annotation().type) << "() {}\n";
            }

            output_.line() << "};\n";
        }

        // emit designated initializers' prototypes for each variant type
        for (auto type : std::static_pointer_cast<ast::variant_type>(decl.annotation().type)->types()) {
            struct guard inner(output_);
            auto hash = std::to_string(std::hash<std::string>()(type->string()));
            // emits constructor for each of its types
            output_.line() << emit(decl.annotation().type) << " " << emit(decl.annotation().type) << "_init_" << hash << "(" << emit(type, "init") << ");\n";
            // then emits explicit conversions to each of its types
            output_.line() << emit(type) << " " << emit(decl.annotation().type) << "_as_" << hash << "(" << emit(decl.annotation().type, "self") << ", const char* file, int line, int column);\n";
        }

        if (checker_.scopes().count(&decl)) {
            for (auto constant : checker_.scopes().at(&decl)->values()) constant.second->accept(*this);
            for (auto function : checker_.scopes().at(&decl)->functions()) function.second->accept(*this);
        }

        // emit vtables for this type
        if (types::implementors().count(decl.annotation().type)) {
            struct guard inner(output_);

            for (auto behaviour : types::implementors().at(decl.annotation().type)) {
                auto bname = fullname(behaviour->declaration());
                output_.line() << "static __vtable_" << bname << " __vtable_" << fullname(behaviour->declaration()) << "_for_" << emit(decl.annotation().type) << " = { ";
                // set offset of this vptr inside structure
                output_.stream() << "offsetof(" << emit(decl.annotation().type) << ", __vptr_" << bname << ")";
                // fill vtable with function pointers
                for (auto prototype : static_cast<const ast::behaviour_declaration*>(behaviour->declaration())->declarations()) {
                    output_.stream() << ", ";
                    if (auto function = std::dynamic_pointer_cast<ast::function_declaration>(prototype)) output_.stream() << "(void (*)(void*)) &" << emit(decl.annotation().type) << "_" << function->name().lexeme();
                    else if (auto function = std::dynamic_pointer_cast<ast::property_declaration>(prototype)) output_.stream() << "(void (*)(void*)) &" << emit(decl.annotation().type) << "_" << function->name().lexeme();
                }
                output_.stream() << " };\n";
            }
        }

        // emit basic constructor filling vpointers eventyally
        output_.line() << emit(decl.annotation().type) << "::" << emit(decl.annotation().type) << "()";
        // initialize vpointers
        if (types::implementors().count(decl.annotation().type)) {
            output_.stream() << " : ";
            unsigned ifield = 0;
            for (auto behaviour : types::implementors().at(decl.annotation().type)) {
                auto bname = fullname(behaviour->declaration());
                if (ifield > 0) output_.stream() << ", ";
                output_.stream() << "__vptr_" << bname << "(&__vtable_" << bname << "_for_" << emit(decl.annotation().type) << ")";
                ++ifield;
            }
        }
        output_.stream() << " {}\n";

        // create designated initializers for each variant type
        for (auto type : std::static_pointer_cast<ast::variant_type>(decl.annotation().type)->types()) {
            struct guard inner(output_);
            auto hash = std::to_string(std::hash<std::string>()(type->string()));
            
            // emits constructor for each of its types
            output_.line() << emit(decl.annotation().type) << " " << emit(decl.annotation().type) << "_init_" << hash << "(" << emit(type, "init") << ") {\n";
            
            {
                struct guard inner(output_);
                output_.line() << emit(decl.annotation().type, "result") << ";\n";
                output_.line() << "result.__tag = " << hash << "ull;\n";
                output_.line() << "new(&result._" << hash << ") " << emit(type) << "(init);\n";
                output_.line() << "return result;\n";
            }

            output_.line() << "}\n";

            // then emits explicit conversions to each of its types
            output_.line() << emit(type) << " " << emit(decl.annotation().type) << "_as_" << hash << "(" << emit(decl.annotation().type, "self") << ", const char* file, int line, int column) {\n";
            
            {
                struct guard inner(output_);
                output_.line() << "if (self.__tag != " << hash << "ull) __crash(\"damn, you cannot explictly convert variant since it has a different run-time type!\", file, line, column);\n";
                output_.line() << "return self._" << hash << "; \n";
            }

            output_.line() << "}\n";
        }
    }

    void code_generator::visit(const ast::const_declaration& decl)
    {
        /* constants should not be recorded as they are substituted like macros */ return;

        // each associated is a static constant
        if (dynamic_cast<const ast::type_declaration*>(decl.annotation().scope)) output_.line() << "static ";
        else output_.line();

        output_.stream() << "constexpr " << emit(decl.annotation().type, fullname(&decl)) << " = " << emit(decl.value()->annotation().value) << ";\n";
    }

    void code_generator::visit(const ast::var_declaration& decl)
    {
        output_.stream() << "#if __DEVELOPMENT__\n";
        if (decl.annotation().scope->kind() != ast::kind::workspace) output_.line() << "__record.location(" << decl.range().bline << ", " << decl.range().bcolumn << ");\n";
        output_.stream() << "#endif\n";

        output_.line() << emit(decl.annotation().type, fullname(&decl));

        if (decl.value()) {
            switch (decl.value()->kind()) {
                case ast::kind::if_expression:
                case ast::kind::when_cast_expression:
                case ast::kind::when_pattern_expression:
                case ast::kind::when_expression:
                case ast::kind::for_loop_expression:
                case ast::kind::for_range_expression:
                    output_.stream() << ";\n";
                    result_vars.push(decl.name().lexeme().string());
                    output_.line();
                    decl.value()->accept(*this);
                    result_vars.pop();
                    break;
                default:
                    output_.stream() << " = "; 
                    decl.value()->accept(*this);
                    output_.stream() << ";\n";
            }
        }
    }

    void code_generator::visit(const ast::field_declaration& decl)
    {
        guard guard(output_);

        output_.line() << emit(decl.annotation().type, decl.name().lexeme().string()) << ";\n";
    }

    void code_generator::visit(const ast::tuple_field_declaration& decl)
    {
        guard guard(output_);

        output_.line() << emit(decl.annotation().type, "_" + std::to_string(decl.index())) << ";\n";
    }

    void code_generator::visit(const ast::parameter_declaration& decl)
    {
        output_.stream() << emit(decl.annotation().type, decl.name().lexeme().string());
    }

    void code_generator::visit(const ast::function_declaration& decl)
    {
        if (decl.generic()) return;

        guard guard(output_);
        auto fntype = std::static_pointer_cast<ast::function_type>(decl.annotation().type);

        output_.line() << prototype(&decl);

        if (decl.body()) {
            output_.stream() << " {\n";
            {
                struct guard inner(output_);
                // stack activation record for stacktrace
                output_.stream() << "#if __DEVELOPMENT__\n";
                output_.line() << "__stack_activation_record __record(\"" << decl.name().location().filename << "\", \"" << decl.name().lexeme() << "\", " << decl.name().location().line << ", " << decl.name().location().column << ");\n";
                output_.stream() << "#endif\n";
                // contracts
                emit_in_contracts(decl);
            }
            // if entry point it initializes arguments properly from __argc, __argv
            if (&decl == checker_.entry_point() && !decl.parameters().empty()) {
                struct guard inner(output_);
                auto args_name = std::static_pointer_cast<ast::parameter_declaration>(decl.parameters().front())->name().lexeme();
                output_.line() << "std::vector<__chars> __args_buffer((std::size_t) __argc);\n";
                output_.line() << "for (auto i = 0; i < __argc; ++i) __args_buffer[i] = __chars(__argv[i]);\n";
                output_.line();
                decl.parameters().front()->accept(*this);
                output_.stream() << " = __slice<__chars>(__args_buffer.data(), __argc);\n";
            }
            // body traversal
            switch (decl.body()->kind()) {
                case ast::kind::if_expression:
                case ast::kind::when_cast_expression:
                case ast::kind::when_pattern_expression:
                case ast::kind::when_expression:
                case ast::kind::for_loop_expression:
                case ast::kind::for_range_expression:
                {
                    // for complex expression, it creates a block with that expression as expression statement (exprnode)
                    auto stmts = ast::pointers<ast::statement>();
                    stmts.push_back(ast::create<ast::expression_statement>(decl.body()->range(), decl.body()));
                    auto body = ast::create<ast::block_expression>(decl.body()->range(), stmts);
                    checker_.scopes().emplace(body.get(), new environment(body.get(), checker_.scopes().at(&decl)));
                    body->exprnode() = stmts.front().get();
                    body->annotation().type = decl.body()->annotation().type;
                    decl.body() = body;
                    checker_.scopes().emplace(decl.body().get(), new environment(decl.body().get(), checker_.scopes().at(&decl)));
                    decl.body()->accept(*this);
                    break;
                }
                case ast::kind::block_expression:
                    decl.body()->accept(*this);
                    break;
                default:
                {
                    // on exit we must emit out contracts
                    emit_out_contracts(decl);
                    // simple expression with return
                    struct guard inner(output_);
                    output_.line() << "return ";
                    decl.body()->accept(*this);
                    output_.stream() << ";\n";
                }
            }

            output_.line() << "}\n";
        }
        else output_.stream() << ";\n";
    }

    void code_generator::visit(const ast::property_declaration& decl)
    {
        guard guard(output_);
        auto fntype = std::static_pointer_cast<ast::function_type>(decl.annotation().type);

        output_.line() << prototype(&decl);

        if (decl.body()) {
            output_.stream() << " {\n";
            {
                struct guard inner(output_);
                // stack activation record for stacktrace
                output_.stream() << "#if __DEVELOPMENT__\n";
                output_.line() << "__stack_activation_record __record(\"" << decl.name().location().filename << "\", \"" << decl.name().lexeme() << "\", " << decl.name().location().line << ", " << decl.name().location().column << ");\n";
                output_.stream() << "#endif\n";
                // contracts
                emit_in_contracts(decl);
            }
            // body traversal
            switch (decl.body()->kind()) {
                case ast::kind::if_expression:
                case ast::kind::when_cast_expression:
                case ast::kind::when_pattern_expression:
                case ast::kind::when_expression:
                case ast::kind::for_loop_expression:
                case ast::kind::for_range_expression:
                {
                    // for complex expression, it creates a block with that expression as expression statement (exprnode)
                    auto stmts = ast::pointers<ast::statement>();
                    stmts.push_back(ast::create<ast::expression_statement>(decl.body()->range(), decl.body()));
                    auto body = ast::create<ast::block_expression>(decl.body()->range(), stmts);
                    checker_.scopes().emplace(body.get(), new environment(body.get(), checker_.scopes().at(&decl)));
                    body->exprnode() = stmts.front().get();
                    body->annotation().type = decl.body()->annotation().type;
                    decl.body() = body;
                    checker_.scopes().emplace(decl.body().get(), new environment(decl.body().get(), checker_.scopes().at(&decl)));
                    decl.body()->accept(*this);
                    break;
                }
                case ast::kind::block_expression:
                    decl.body()->accept(*this);
                    break;
                default:
                {
                    // on exit we must emit out contracts
                    emit_out_contracts(decl);
                    // simple expression with return
                    struct guard inner(output_);
                    output_.line() << "return ";
                    decl.body()->accept(*this);
                    output_.stream() << ";\n";
                }
            }

            output_.line() << "}\n";
        }
        else output_.stream() << ";\n";
    }

    bool code_generator::emit_if_constant(const ast::expression& expr)
    {
        if (expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) {
            output_.stream() << emit(expr.annotation().value);
            return true;
        }

        return false;
    }

    void code_generator::visit(const ast::bit_field_type_expression& expr) { output_.stream() << emit(expr.annotation().type); }

    void code_generator::visit(const ast::path_type_expression& expr) { output_.stream() << emit(expr.annotation().type); }
    
    void code_generator::visit(const ast::array_type_expression& expr) { output_.stream() << emit(expr.annotation().type); }
    
    void code_generator::visit(const ast::tuple_type_expression& expr) { output_.stream() << emit(expr.annotation().type); }
    
    void code_generator::visit(const ast::pointer_type_expression& expr) { output_.stream() << emit(expr.annotation().type); }
    
    void code_generator::visit(const ast::function_type_expression& expr) { output_.stream() << emit(expr.annotation().type); }
    
    void code_generator::visit(const ast::record_type_expression& expr) { output_.stream() << emit(expr.annotation().type); }
    
    void code_generator::visit(const ast::variant_type_expression& expr) { output_.stream() << emit(expr.annotation().type); }

    void code_generator::visit(const ast::literal_expression& expr) { output_.stream() << emit(expr.annotation().value); }

    void code_generator::visit(const ast::unary_expression& expr)
    {
        if (emit_if_constant(expr)) return;

        output_.stream() << "(";

        switch (expr.unary_operator().kind()) {
            case token::kind::plus:
                output_.stream() << "+";
                break;
            case token::kind::minus:
                output_.stream() << "-";
                break;
            case token::kind::tilde:
                output_.stream() << "~";
                break;
            case token::kind::bang:
                output_.stream() << "!";
                break;
            case token::kind::plus_plus:
                output_.stream() << "++";
                break;
            case token::kind::minus_minus:
                output_.stream() << "--";
                break;
            case token::kind::amp:
                output_.stream() << "&";
                break;
            case token::kind::star:
                output_.stream() << "*";
                break;
            default:
                break;
        }

        expr.expression()->accept(*this);

        output_.stream() << ")";
    }

    void code_generator::visit(const ast::binary_expression& expr) 
    {
        if (emit_if_constant(expr)) return;

        switch (expr.binary_operator().kind()) {
            case token::kind::plus:
                expr.left()->accept(*this);
                output_.stream() << " + ";
                expr.right()->accept(*this);
                break;
            case token::kind::minus:
                expr.left()->accept(*this);
                output_.stream() << " - ";
                expr.right()->accept(*this);
                break;
            case token::kind::star:
                expr.left()->accept(*this);
                output_.stream() << " * ";
                expr.right()->accept(*this);
                break;
            case token::kind::slash:
                if (expr.annotation().type->category() == ast::type::category::rational_type) {
                    output_.stream() << emit(expr.annotation().type) << "(";
                    expr.left()->accept(*this);
                    output_.stream() << ", ";
                    expr.right()->accept(*this);
                    output_.stream() << ")";
                }
                else {
                    expr.left()->accept(*this);
                    output_.stream() << " / ";
                    expr.right()->accept(*this);
                }
                break;
            case token::kind::percent:
                expr.left()->accept(*this);
                output_.stream() << " % ";
                expr.right()->accept(*this);
                break;
            case token::kind::star_star:
                output_.stream() << "std::pow(";
                expr.left()->accept(*this);
                output_.stream() << ", ";
                expr.right()->accept(*this);
                output_.stream() << ")";
                break;
            case token::kind::as_kw:
            {
                auto original = expr.left()->annotation().type, result = expr.right()->annotation().type;

                // explicit unpacking of variant type
                if (auto variant = std::dynamic_pointer_cast<ast::variant_type>(original)) {
                    if (!variant->contains(result)) throw std::invalid_argument("code_generator::visit(const ast::implicit_conversion_expression&): invalid variant conversion");
                    // explicit conversion call
                    output_.stream() << emit(original) << "_as_" << std::hash<std::string>()(result->string()) << "(";
                    expr.left()->accept(*this);
                    output_.stream() << ", \"" << expr.binary_operator().location().filename << "\", " << expr.binary_operator().location().line << ", " << expr.binary_operator().location().column << ")";
                }
                // implicit upcasting
                else if (result->category() == ast::type::category::pointer_type && std::static_pointer_cast<ast::pointer_type>(result)->base()->category() == ast::type::category::behaviour_type && original->category() == ast::type::category::pointer_type) {
                    auto implementor = std::static_pointer_cast<ast::pointer_type>(original)->base();
                    auto behaviour = std::static_pointer_cast<ast::behaviour_type>(std::static_pointer_cast<ast::pointer_type>(result)->base());
                    if (behaviour->implementor(implementor)) {
                        output_.stream() << "(" << emit(result) << ") ((char*) ";
                        expr.left()->accept(*this);
                        output_.stream() << " + offsetof(" << emit(implementor) << ", __vptr_" << emit(behaviour) << "))";
                    }
                }
                // implicit downcast
                else if (original->category() == ast::type::category::pointer_type && std::static_pointer_cast<ast::pointer_type>(original)->base()->category() == ast::type::category::behaviour_type && result->category() == ast::type::category::pointer_type) {
                    auto implementor = std::static_pointer_cast<ast::pointer_type>(result)->base();
                    auto behaviour = std::static_pointer_cast<ast::behaviour_type>(std::static_pointer_cast<ast::pointer_type>(original)->base());
                    if (behaviour->implementor(implementor)) {
                        output_.stream() << "(" << emit(result) << ") ((char*) ";
                        expr.left()->accept(*this);
                        output_.stream() << " - offsetof(" << emit(implementor) << ", __vptr_" << emit(behaviour) << "))";
                    }
                }
                else switch (result->category()) {
                    case ast::type::category::chars_type:
                        if (original->category() == ast::type::category::string_type) {
                            output_.stream() << emit(result) << "(";
                            expr.left()->accept(*this);
                            output_.stream() << ".data())";
                        }
                        break;
                    case ast::type::category::string_type:
                        if (original->category() == ast::type::category::chars_type) {
                            output_.stream() << emit(result) << "(";
                            expr.left()->accept(*this);
                            output_.stream() << ".data())";
                        }
                        break;
                    case ast::type::category::variant_type:
                        // designated intializer for variant type
                        output_.stream() << emit(result) << "_init_" << std::hash<std::string>()(original->string()) << "(";
                        expr.left()->accept(*this);
                        output_.stream() << ")";
                        break;
                    case ast::type::category::pointer_type:
                        // conversion made from slice
                        if (original->category() == ast::type::category::slice_type) {
                            expr.left()->accept(*this);
                            output_.stream() << ".data()";
                        }
                        break;
                    default:
                        output_.stream() << "(" << emit(result) << ") ";
                        expr.left()->accept(*this);
                }

                break;
            }
            case token::kind::equal_equal:
                expr.left()->accept(*this);
                output_.stream() << " == ";
                expr.right()->accept(*this);
                break;
            case token::kind::bang_equal:
                expr.left()->accept(*this);
                output_.stream() << " != ";
                expr.right()->accept(*this);
                break;
            case token::kind::less:
                expr.left()->accept(*this);
                output_.stream() << " < ";
                expr.right()->accept(*this);
                break;
            case token::kind::greater:
                expr.left()->accept(*this);
                output_.stream() << " > ";
                expr.right()->accept(*this);
                break;
            case token::kind::less_equal:
                expr.left()->accept(*this);
                output_.stream() << " <= ";
                expr.right()->accept(*this);
                break;
            case token::kind::greater_equal:
                expr.left()->accept(*this);
                output_.stream() << " >= ";
                expr.right()->accept(*this);
                break;
            case token::kind::amp_amp:
                expr.left()->accept(*this);
                output_.stream() << " && ";
                expr.right()->accept(*this);
                break;
            case token::kind::line_line:
                expr.left()->accept(*this);
                output_.stream() << " && ";
                expr.right()->accept(*this);
                break;
            default:
                break;
        }
    }
    
    void code_generator::visit(const ast::identifier_expression& expr) 
    {
        if (expr.annotation().referencing) {
            // emit constant
            if ((expr.annotation().referencing->kind() == ast::kind::const_declaration || expr.annotation().referencing->kind() == ast::kind::generic_const_parameter_declaration) && expr.annotation().value.type && expr.annotation().value.type->category() != ast::type::category::unknown_type) {
                output_.stream() << emit(expr.annotation().value);
            }
            // builtin functions
            else if (auto fn = dynamic_cast<const ast::function_declaration*>(expr.annotation().referencing)) {
                // strips name from generic arguments
                auto non_generic_name = fn->name().lexeme().string();
                auto first = non_generic_name.find_first_of('(');
                auto last = non_generic_name.find_last_of(')');
                if (first != std::string::npos && last != std::string::npos) non_generic_name.erase(first, last);
                // test name against builtin functions
                if (non_generic_name == "allocate") output_.stream() << "__allocate<" << emit(expr.generics().front()->annotation().type) << ">";
                else if (non_generic_name == "deallocate") output_.stream() << "__deallocate";
                else if (non_generic_name == "free") output_.stream() << "__free";
                else if (non_generic_name == "sizeof") output_.stream() << "__sizeof<" << emit(expr.generics().front()->annotation().type) << ">";
                else output_.stream() << fullname(expr.annotation().referencing);
            }
            else output_.stream() << fullname(expr.annotation().referencing);
        }
        else output_.stream() << expr.identifier().lexeme();
    }
    
    void code_generator::visit(const ast::tuple_expression& expr)
    {
        if (emit_if_constant(expr)) return;

        output_.stream() << emit(expr.annotation().type) << "(";
        
        for (unsigned i = 0; i < expr.elements().size(); ++i) {
            if (i > 0) output_.stream() << ", ";
            expr.elements().at(i)->accept(*this);
        }

        output_.stream() << ")";
    }
    
    void code_generator::visit(const ast::array_expression& expr)
    {
        if (emit_if_constant(expr)) return;

        // array initializer has been bound to array object so that it could be allocated on stack
        if (auto temporary = dynamic_cast<const ast::var_declaration*>(expr.annotation().referencing)) output_.stream() << temporary->name().lexeme();
        // this is the real binding
        else {
            output_.stream() << "{";
            
            for (unsigned i = 0; i < expr.elements().size(); ++i) {
                if (i > 0) output_.stream() << ", ";
                expr.elements().at(i)->accept(*this);
            }

            output_.stream() << "}";
        }
    }
    
    void code_generator::visit(const ast::array_sized_expression& expr) 
    {
        if (emit_if_constant(expr)) return;

        // array initializer has been bound to array object so that it could be allocated on stack
        if (auto temporary = dynamic_cast<const ast::var_declaration*>(expr.annotation().referencing)) output_.stream() << temporary->name().lexeme();
        // this is the real binding
        else {
            output_.stream() << "{";
            
            for (unsigned i = 0; i < expr.size()->annotation().value.u.value(); ++i) {
                if (i > 0) output_.stream() << ", ";
                expr.value()->accept(*this);
            }

            output_.stream() << "}";
        }
    }
    
    void code_generator::visit(const ast::parenthesis_expression& expr) 
    {
        if (emit_if_constant(expr)) return;

        output_.stream() << "(";
        expr.expression()->accept(*this);
        output_.stream() << ")";
    }

    void code_generator::visit(const ast::member_expression& expr)
    {
        if (emit_if_constant(expr)) return;

        if (!expr.expression()->annotation().istype) {
            // method (function) access
            if (dynamic_cast<const ast::function_declaration*>(expr.member()->annotation().referencing)) expr.member()->accept(*this);
            // method (property) call because it is a computed property
            else if (dynamic_cast<const ast::property_declaration*>(expr.member()->annotation().referencing)) {
                expr.member()->accept(*this);
                output_.stream() << "(";
                // when property is called in OOP notation then callee is first argument object
                expr.expression()->accept(*this);
                output_.stream() << ")";
            }
            // builtin field access is translated in function call
            else if (!expr.member()->annotation().referencing) {
                switch (expr.expression()->annotation().type->category()) {
                    case ast::type::category::structure_type:
                    case ast::type::category::tuple_type:
                    case ast::type::category::variant_type:
                        expr.expression()->accept(*this);
                        if (expr.expression()->annotation().type->category() == ast::type::category::pointer_type) output_.stream() << "->";
                        else output_.stream() << ".";
                        expr.member()->accept(*this);
                        break;
                    default:
                        output_.stream() << "__get_";
                        expr.member()->accept(*this);
                        output_.stream() << "(";
                        expr.expression()->accept(*this);
                        output_.stream() << ")";
                }
            }
            // field access on structure or tuple object
            else {
                expr.expression()->accept(*this);
                if (expr.expression()->annotation().type->category() == ast::type::category::pointer_type) output_.stream() << "->";
                else output_.stream() << ".";
                expr.member()->accept(*this);
            }
        }
        // associated name access
        else if (expr.annotation().referencing) expr.member()->accept(*this);
    }

    void code_generator::visit(const ast::call_expression& expr)
    {
        if (emit_if_constant(expr)) return;

        expr.callee()->accept(*this);

        // object (record or tuple) construction 
        if (expr.callee()->annotation().istype) {
            if (expr.callee()->annotation().type->category() == ast::type::category::structure_type) {
                if (!expr.callee()->annotation().type->declaration()) throw std::invalid_argument("code_generator::visit(const ast::call_expression&): temporary struct type not yet implemented!");
                
                output_.stream() << "{";

                for (unsigned i = 0; i < expr.arguments().size(); ++i) {
                    if (i > 0) output_.stream() << ", ";
                    expr.arguments().at(i)->accept(*this);
                }

                output_.stream() << "}";
            }
            else if (expr.callee()->annotation().type->category() == ast::type::category::tuple_type) {
                output_.stream() << "{";

                for (unsigned i = 0; i < expr.arguments().size(); ++i) {
                    if (i > 0) output_.stream() << ", ";
                    expr.arguments().at(i)->accept(*this);
                }

                output_.stream() << "}";
            }
        }
        // function or method call
        else {
            auto fntype = std::dynamic_pointer_cast<ast::function_type>(expr.callee()->annotation().type);
            // ordinary function call
            if (fntype->formals().size() == expr.arguments().size()) {
                output_.stream() << "(";

                for (unsigned i = 0; i < expr.arguments().size(); ++i) {
                    if (i > 0) output_.stream() << ", ";
                    expr.arguments().at(i)->accept(*this);
                }
                
                // with crash() or assert() call we implicitly pass file and line info to print the error
                if (expr.callee()->annotation().referencing) {
                    std::string full_name = fullname(expr.callee()->annotation().referencing);
                    if (full_name == "__crash" || full_name == "__assert") output_.stream() << ", \"" << expr.range().filename.string() << "\", " << expr.range().bline << ", " << expr.range().bcolumn;
                }

                output_.stream() << ")";
            }
            // method call
            else if (auto member_expr = std::dynamic_pointer_cast<ast::member_expression>(expr.callee())) {
                if (fntype->formals().size() == expr.arguments().size() + 1) {
                    output_.stream() << "(";
                    // object is passed as first parameter
                    member_expr->expression()->accept(*this);

                    for (auto arg : expr.arguments()) {
                        output_.stream() << ", ";
                        arg->accept(*this);
                    }
                    
                    output_.stream() << ")";
                }
            }
        }
    }

    void code_generator::visit(const ast::array_index_expression& expr)
    {
        if (emit_if_constant(expr)) return;

        expr.expression()->accept(*this);
        output_.stream() << "[";
        expr.index()->accept(*this);
        output_.stream() << "]";
    }

    void code_generator::visit(const ast::tuple_index_expression& expr)
    {
        if (emit_if_constant(expr)) return;

        // if left expression is a pointer to a tuple
        if (expr.expression()->annotation().type->category() == ast::type::category::pointer_type) {
            expr.expression()->accept(*this);
            output_.stream() << "->_" << emit(evaluator::integer_parse(expr.index().lexeme().string()));
        }
        // if type is declared as a tuple, then we have field access
        else if (expr.expression()->annotation().type->declaration()) {
            expr.expression()->accept(*this);
            output_.stream() << "._" << emit(evaluator::integer_parse(expr.index().lexeme().string()));
        }
        // otherwise tuple access
        else {
            output_.stream() << "std::get<" << emit(evaluator::integer_parse(expr.index().lexeme().string())) << ">(";
            expr.expression()->accept(*this);
            output_.stream() << ")";
        }
    }

    void code_generator::visit(const ast::record_expression& expr)
    {
        if (emit_if_constant(expr)) return;

        if (auto tuple = std::dynamic_pointer_cast<ast::tuple_type>(expr.annotation().type)) {
            // std::tuple can be constructed with {} in C++, like other objects
            output_.stream() << emit(tuple) << "{";

            for (unsigned i = 0; i < expr.initializers().size(); ++i) {
                if (i > 0) output_.stream() << ", ";
                expr.initializers().at(i).value()->accept(*this);
            }

            output_.stream() << "}";
        }
        else if (auto structure = std::dynamic_pointer_cast<ast::structure_type>(expr.annotation().type)) {
            output_.stream() << emit(structure) << "{";

            if (!structure->declaration()) throw std::invalid_argument("code_generator::visit(const ast::record_expression&): temporary struct type not yet implemented!");
            // brutal, to initialize fields in order I got an O(n^2)
            unsigned count = 0;
            for (auto field : structure->fields()) {
                auto it = std::find_if(expr.initializers().begin(), expr.initializers().end(), [&] (ast::record_expression::initializer init) { return field.name == init.field().lexeme().string(); });
                if (count++ > 0) output_.stream() << ", ";
                it->value()->accept(*this);
            }

            output_.stream() << "}";
        }
    }

    void code_generator::visit(const ast::implicit_conversion_expression& expr)
    {
        if (emit_if_constant(expr)) return;

        auto original = expr.expression()->annotation().type, result = expr.annotation().type;

        // implicit unpacking of variant type
        if (auto variant = std::dynamic_pointer_cast<ast::variant_type>(original)) {
            if (!variant->contains(result)) throw std::invalid_argument("code_generator::visit(const ast::implicit_conversion_expression&): invalid variant conversion");
            expr.expression()->accept(*this);
            output_.stream() << "._" << std::hash<std::string>()(result->string());
        }
        // implicit upcasting
        else if (result->category() == ast::type::category::pointer_type && std::static_pointer_cast<ast::pointer_type>(result)->base()->category() == ast::type::category::behaviour_type && original->category() == ast::type::category::pointer_type) {
            auto implementor = std::static_pointer_cast<ast::pointer_type>(original)->base();
            auto behaviour = std::static_pointer_cast<ast::behaviour_type>(std::static_pointer_cast<ast::pointer_type>(result)->base());
            if (behaviour->implementor(implementor)) {
                output_.stream() << "(" << emit(result) << ") ((char*) ";
                expr.expression()->accept(*this);
                output_.stream() << " + offsetof(" << emit(implementor) << ", __vptr_" << emit(behaviour) << "))";
            }
        }
        // implicit downcast
        else if (original->category() == ast::type::category::pointer_type && std::static_pointer_cast<ast::pointer_type>(original)->base()->category() == ast::type::category::behaviour_type && result->category() == ast::type::category::pointer_type) {
            auto implementor = std::static_pointer_cast<ast::pointer_type>(result)->base();
            auto behaviour = std::static_pointer_cast<ast::behaviour_type>(std::static_pointer_cast<ast::pointer_type>(original)->base());
            if (behaviour->implementor(implementor)) {
                output_.stream() << "(" << emit(result) << ") ((char*) ";
                expr.expression()->accept(*this);
                output_.stream() << " - offsetof(" << emit(implementor) << ", __vptr_" << emit(behaviour) << "))";
            }
        }
        else switch (result->category()) {
            case ast::type::category::chars_type:
                if (original->category() == ast::type::category::string_type) {
                    output_.stream() << emit(result) << "(";
                    expr.expression()->accept(*this);
                    output_.stream() << ".data())";
                }
                break;
            case ast::type::category::string_type:
                // construction from raw bytes
                if (original->category() == ast::type::category::chars_type) {
                    output_.stream() << emit(result) << "(";
                    expr.expression()->accept(*this);
                    output_.stream() << ".data())";
                }
                // implicit conversion through 'str' property
                else if (original->declaration()) {
                    output_.stream() << emit(original) << "_str(";
                    expr.expression()->accept(*this);
                    output_.stream() << ")";
                }
                break;
            case ast::type::category::variant_type:
                // designated intializer for variant type
                output_.stream() << emit(result) << "_init_" << std::hash<std::string>()(original->string()) << "(";
                expr.expression()->accept(*this);
                output_.stream() << ")";
                break;
            case ast::type::category::slice_type:
                // construction of slice
                if (original->category() == ast::type::category::array_type) {
                    output_.stream() << emit(result) << "(";
                    expr.expression()->accept(*this);
                    output_.stream() << ", " << std::static_pointer_cast<ast::array_type>(original)->size() << ")";
                }
                else if (original->category() == ast::type::category::chars_type || original->category() == ast::type::category::string_type) {
                    output_.stream() << emit(result) << "(";
                    expr.expression()->accept(*this);
                    output_.stream() << ".data(), ";
                    expr.expression()->accept(*this);
                    output_.stream() << ".size())";
                }
                break;
            default:
                output_.stream() << "(" << emit(result) << ") ";
                expr.expression()->accept(*this);
        }
    }

    void code_generator::visit(const ast::block_expression& expr)
    {
        if (emit_if_constant(expr)) return;

        guard guard(output_);
        bool is_function_body = false;

        if (auto fn = checker_.scopes().at(&expr)->outscope(environment::kind::function)) {
            auto body = fn->kind() == ast::kind::function_declaration ? static_cast<const ast::function_declaration*>(fn)->body().get() : static_cast<const ast::property_declaration*>(fn)->body().get();
            is_function_body = &expr == body;
            if (is_function_body && !types::compatible(expr.annotation().type, types::unit())) {
                output_.line() << emit(expr.annotation().type, "__result") << ";\n";
                result_vars.push("__result");
            }
        }

        for (auto stmt : expr.statements()) {
            // all nested type declaration are declared in global scope
            if (dynamic_cast<ast::type_declaration*>(stmt.get())) continue;
            // if expr node is set, then we declare it as variable '__result'
            if (stmt.get() == expr.exprnode() && stmt->kind() == ast::kind::expression_statement) {
                auto value = static_cast<const ast::expression_statement*>(stmt.get())->expression();
                switch (value->kind()) {
                    case ast::kind::if_expression:
                    case ast::kind::when_cast_expression:
                    case ast::kind::when_pattern_expression:
                    case ast::kind::when_expression:
                    case ast::kind::for_loop_expression:
                    case ast::kind::for_range_expression:
                        stmt->accept(*this);
                        break;
                    default:
                        // if expression statement has no type, its result is not saved to be returned
                        if (types::compatible(types::unit(), value->annotation().type)) stmt->accept(*this);
                        // save results only if its type is not (), which is 'void' in C/C++
                        else if (!result_vars.empty()) {
                            output_.stream() << "#if __DEVELOPMENT__\n";
                            output_.line() << "__record.location(" << stmt->range().bline << ", " << stmt->range().bcolumn << ");\n";
                            output_.stream() << "#endif\n";
                            output_.line() << result_vars.top() << " = ";
                            value->accept(*this);
                            output_.stream() << ";\n";
                        }
                        break;
                }
            }
            else stmt->accept(*this);
        }
        // 'return __result' is emitted only if we are inside main function or property body and at its end
        if (is_function_body && expr.exprnode() && expr.exprnode()->kind() != ast::kind::return_statement && !types::compatible(expr.annotation().type, types::unit())) {
            // on exit we must emit out contracts
            emit_out_contracts(*checker_.scopes().at(&expr)->outscope(environment::kind::function));
            // return result
            result_vars.pop();
            output_.line() << "return __result;\n";
        }
    }

    void code_generator::visit(const ast::if_expression& expr)
    {
        if (emit_if_constant(expr)) return;

        output_.stream() << "if (";
        expr.condition()->accept(*this);
        output_.stream() << ") {\n";
        expr.body()->accept(*this);
        output_.line() << "}\n";
        if (expr.else_body()) {
            if (expr.else_body()->kind() == ast::kind::if_expression) {
                output_.line() << "else ";
                expr.else_body()->accept(*this);
            }
            else {
                output_.line() << "else {\n";
                expr.else_body()->accept(*this);
                output_.line() << "}\n";
            }
        }
    }

    void code_generator::visit(const ast::when_cast_expression& expr)
    {
        if (emit_if_constant(expr)) return;

        output_.stream() << "if (";
        expr.condition()->accept(*this);
        output_.stream() << ".__tag == " << std::to_string(std::hash<std::string>()(expr.type_expression()->annotation().type->string())) << "ull) {\n";
        expr.body()->accept(*this);
        output_.line() << "}\n";
        if (expr.else_body()) {
            output_.line() << "else {\n";
            expr.else_body()->accept(*this);
            output_.line() << "}\n";
        }
    }

    void code_generator::visit(const ast::when_pattern_expression& expr)
    {
        if (emit_if_constant(expr)) return;

        output_.stream() << "if (";
        if (auto condition = std::dynamic_pointer_cast<ast::pattern_expression>(expr.pattern())->compiled()) condition->accept(*this);
        else output_.stream() << "true";
        output_.stream() << ") {\n";
        expr.body()->accept(*this);
        output_.line() << "}\n";
        if (expr.else_body()) {
            output_.line() << "else {\n";
            expr.else_body()->accept(*this);
            output_.line() << "}\n";
        }
    }

    void code_generator::visit(const ast::when_expression& expr)
    {
        if (emit_if_constant(expr)) return;
        
        unsigned i = 0;

        for (auto branch : expr.branches()) {
            if (i > 0) output_.line() << "else if (";
            else output_.stream() << "if (";
            if (auto condition = std::dynamic_pointer_cast<ast::pattern_expression>(branch.pattern())->compiled()) condition->accept(*this);
            else output_.stream() << "true";
            output_.stream() << ") {\n";
            branch.body()->accept(*this);
            output_.line() << "}\n";
            ++i;
        }
        
        if (expr.else_body()) {
            output_.line() << "else {\n";
            expr.else_body()->accept(*this);
            output_.line() << "}\n";
        }
    }

    void code_generator::visit(const ast::expression_statement& stmt)
    {
        // stack trace update of location
        output_.stream() << "#if __DEVELOPMENT__\n";
        output_.line() << "__record.location(" << stmt.range().bline << ", " << stmt.range().bcolumn << ");\n";
        output_.stream() << "#endif\n";

        output_.line();

        stmt.expression()->accept(*this);

        switch (stmt.expression()->kind()) {
            case ast::kind::if_expression:
            case ast::kind::when_cast_expression:
            case ast::kind::when_pattern_expression:
            case ast::kind::when_expression:
            case ast::kind::for_loop_expression:
            case ast::kind::for_range_expression:
                break;
            default:
                output_.stream() << ";\n";
        }
    }

    void code_generator::visit(const ast::assignment_statement& stmt)
    {
        // stack trace update of location
        output_.stream() << "#if __DEVELOPMENT__\n";
        output_.line() << "__record.location(" << stmt.range().bline << ", " << stmt.range().bcolumn << ");\n";
        output_.stream() << "#endif\n";

        output_.line();
        // 1) first of all save assigned value inside temporary variable '__temp...', like 'val __temp... = rhs'
        // this is done in order to compute complex values returned from control structures like if/for/when
        std::string tempvar = "__temp" + std::to_string(std::rand());

        switch (stmt.right()->kind()) {
            case ast::kind::if_expression:
            case ast::kind::when_cast_expression:
            case ast::kind::when_pattern_expression:
            case ast::kind::when_expression:
            case ast::kind::for_loop_expression:
            case ast::kind::for_range_expression:
                output_.stream() << emit(stmt.left()->annotation().type, tempvar) << ";\n";
                result_vars.push(tempvar);
                output_.line();
                stmt.right()->accept(*this);
                result_vars.pop();
                break;
            default:
                output_.stream() << emit(stmt.left()->annotation().type, tempvar) << " = "; 
                stmt.right()->accept(*this);
                output_.stream() << ";\n";
        }

        output_.line();

        // 2) then assignment is performed 'lhs = __temp...'
        switch (stmt.assignment_operator().kind()) {
            case token::kind::equal:
                stmt.left()->accept(*this);
                output_.stream() << " = " << tempvar;
                break;
            case token::kind::plus_equal:
                stmt.left()->accept(*this);
                output_.stream() << " += " << tempvar;
                break;
            case token::kind::minus_equal:
                stmt.left()->accept(*this);
                output_.stream() << " -= " << tempvar;
                break;
            case token::kind::star_equal:
                stmt.left()->accept(*this);
                output_.stream() << " *= " << tempvar;
                break;
            case token::kind::slash_equal:
                stmt.left()->accept(*this);
                output_.stream() << " /= " << tempvar;
                break;
            case token::kind::percent_equal:
                stmt.left()->accept(*this);
                output_.stream() << " %= " << tempvar;
                break;
            case token::kind::star_star_equal:
                stmt.left()->accept(*this);
                output_.stream() << " = std::pow(";
                stmt.left()->accept(*this);
                output_.stream() << ", " << tempvar << ")";
                break;
            case token::kind::amp_equal:
                stmt.left()->accept(*this);
                output_.stream() << " &= " << tempvar;
                break;
            case token::kind::line_equal:
                stmt.left()->accept(*this);
                output_.stream() << " |= " << tempvar;
                break;
            case token::kind::caret_equal:
                stmt.left()->accept(*this);
                output_.stream() << " ^= " << tempvar;
                break;
            default:
                throw std::invalid_argument("code_generator::visit(const ast::assignment_statement&): invalid operator");
        }

        output_.stream() << ";\n";
    }

    void code_generator::visit(const ast::return_statement& stmt)
    {
        // first, before return statement, we must test invariant and ensure contracts, if any
        emit_out_contracts(*stmt.annotation().scope);

        // stack trace update of location
        output_.stream() << "#if __DEVELOPMENT__\n";
        output_.line() << "__record.location(" << stmt.range().bline << ", " << stmt.range().bcolumn << ");\n";
        output_.stream() << "#endif\n";

        if (stmt.expression()) {
            if (types::compatible(types::unit(), stmt.expression()->annotation().type)) {
                output_.line() << "return";
            }
            else switch (stmt.expression()->kind()) {
                case ast::kind::if_expression:
                case ast::kind::when_cast_expression:
                case ast::kind::when_pattern_expression:
                case ast::kind::when_expression:
                case ast::kind::for_loop_expression:
                case ast::kind::for_range_expression:
                {
                    std::string tempvar = "__temp" + std::to_string(std::rand());
                    output_.line() << emit(stmt.expression()->annotation().type, tempvar) << ";\n";
                    result_vars.push(tempvar);
                    output_.line();
                    stmt.expression()->accept(*this);
                    result_vars.pop();
                    output_.line() << "return " << tempvar;
                    break;
                }
                default:
                    output_.line() << "return "; 
                    stmt.expression()->accept(*this);
            }
        }
        else {
            auto outer = checker_.scopes().at(stmt.annotation().scope)->outscope(environment::kind::function);
            // check if we are inside entry point
            if (outer && outer == checker_.entry_point()) output_.line() << "return 0";
            // otherwise
            else output_.line() << "return";
        }

        output_.stream() << ";\n";
    }

    void code_generator::visit(const ast::contract_statement& stmt)
    {
        // stack trace update of location
        output_.stream() << "#if __DEVELOPMENT__\n";
        output_.line() << "__record.location(" << stmt.range().bline << ", " << stmt.range().bcolumn << ");\n";
        output_.stream() << "#endif\n";

        output_.line() << "__assert(";
        stmt.condition()->accept(*this);
        output_.stream() << ", \"contract failure, be more careful next time, dammit!\", \"" << stmt.range().filename << "\", " << stmt.range().bline << ", " << stmt.range().bcolumn << ");\n";
    }
}