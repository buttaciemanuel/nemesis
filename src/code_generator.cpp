#include <algorithm>
#include <fstream>
#include <stack>

#include "nemesis/codegen/code_generator.hpp"
#include "nemesis/analysis/environment.hpp"
#include "nemesis/analysis/evaluator.hpp"

namespace nemesis {
    code_generator::code_generator(std::unordered_map<std::string, ast::pointer<ast::nucleus>>& units, std::unordered_map<const ast::node*, environment*> scopes, diagnostic_publisher& publisher, enum kind k) : 
        units_(units),
        scopes_(scopes),
        publisher_(publisher),
        kind_(k)
    {}

    code_generator::~code_generator() {}
    
    compilation code_generator::generate()
    {
        compilation compilation(publisher_);
        // for each nucleus it generates an output file (C or C++)
        for (auto unit : units_) {
            // if unit is builtin then its code is loaded though standard library files
            if (unit.second->builtin) {
                compilation.builtin("libns/nscore/" + unit.first + ".cpp");
                continue;
            }
            // otherwise code is generated
            new (&output_) filestream(unit.first);
            // sets current nucleus
            nucleus_ = unit.second;
            // write header and info
            output_.stream() << "/* Compiled version of nucleus '" << unit.first << "' in language " << (kind_ == kind::CPP ? "C++" : "C") << " */\n";
            // imports definitions from standard library 'nscore'
            output_.stream() << "#include \"../libns/nscore/nscore.h\"\n";
            // forward type declarations
            output_.stream() << "/* Forward type declarations */\n";
            for (auto anonymous : unit.second->anonymous_types) output_.stream() << "struct " << emit(anonymous) << ";\n";
            for (auto typedecl : unit.second->types) if (!typedecl->generic()) output_.stream() << prototype(typedecl) << ";\n";
            // forward function declarations
            output_.stream() << "/* Forward function declarations */\n";
            for (auto fndecl : unit.second->functions) if (!fndecl->generic()) output_.stream() << prototype(fndecl) << ";\n";
            // emits all type declarations inside each file
            output_.stream() << "/* Type definitions */\n";
            for (auto anonymous : unit.second->anonymous_types) emit_anonymous_type(anonymous);
            for (auto typedecl : unit.second->types) if (!typedecl->generic()) typedecl->accept(*this);
            // Global variables definitions
            output_.stream() << "/* Variable and constants definitions */\n";
            for (auto valuedecl : unit.second->globals) valuedecl->accept(*this);
            // function definitions
            output_.stream() << "/* Function definitions */\n";
            for (auto fndecl : unit.second->functions) if (!fndecl->generic()) fndecl->accept(*this);
            // file output
            compilation.source("examples/" + unit.first + ".cpp", output_.stream().str());
            
            std::cout << "---" << unit.first << ".cpp---\n" << output_.stream().str() << '\n';
        }

        return compilation;
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

    enum code_generator::kind code_generator::kind() const { return kind_; }

    void code_generator::set_entry_point(const ast::function_declaration* decl) { entry_point_ = decl; }

    const ast::function_declaration* code_generator::entry_point() const { return entry_point_; }

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
            
            for (auto i = 1; i < tuple_type->components().size(); ++i) result << ", " << emit(tuple_type->components().at(i));

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
            
            for (auto i = 1; i < tuple_type->components().size(); ++i) result << ", " << emit(tuple_type->components().at(i));

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
                else return std::to_string(value.u.value());
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
                
                output_.line() << emit(type) << " " << emit(type) << "_init_" << hash << "(" << emit(subtype, "init") << ") {\n";
                
                {
                    struct guard inner(output_);
                    output_.line() << emit(type, "result") << ";\n";
                    output_.line() << "result.__tag = " << hash << "ull;\n";
                    output_.line() << "result._" << hash << " = init;\n";
                    output_.line() << "return result;\n";
                }

                output_.line() << "}\n";
            }
        }
        else throw std::invalid_argument("code_generator::emit_anonymous_type(): invalid type " + type->string());
    }

    std::string code_generator::fullname(const ast::declaration *decl) const
    {
        if (decl == entry_point_) return "main";

        std::stack<std::string> levels;
        
        for (bool done = false; decl && !done;) {
            switch (decl->kind()) {
                case ast::kind::nucleus:
                    levels.push(static_cast<const ast::nucleus*>(decl)->name);
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

        if (decl == entry_point_) return "int main(int __argc, char **__argv)";

        // each associated is a static (class) method
        if (dynamic_cast<const ast::type_declaration*>(scopes_.at(decl)->outscope(environment::kind::declaration))) result << "static ";

        if (types::compatible(types::unit(), fntype->result())) result << "void ";
        else result << emit(fntype->result()) << " ";

        result << fullname(decl) << "(";

        for (int i = 0; i < decl->parameters().size(); ++i) {
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
        if (dynamic_cast<const ast::type_declaration*>(scopes_.at(decl)->outscope(environment::kind::declaration))) result << "static ";

        if (types::compatible(types::unit(), fntype->result())) result << "void ";
        else result << emit(fntype->result()) << " ";

        result << /*decl->name().lexeme()*/fullname(decl) << "(" << emit(decl->parameters().front()->annotation().type, std::static_pointer_cast<ast::parameter_declaration>(decl->parameters().front())->name().lexeme().string()) << ")";

        return result.str();
    }

    void code_generator::visit(const ast::extend_declaration& decl) {}
    
    void code_generator::visit(const ast::behaviour_declaration& decl) {}
    
    void code_generator::visit(const ast::extern_declaration& decl) {}
    
    void code_generator::visit(const ast::range_declaration& decl) {}
    
    void code_generator::visit(const ast::record_declaration& decl)
    {
        if (decl.generic()) return;

        {
            guard guard(output_);

            output_.line() << prototype(&decl) << " {\n";

            for (auto field : decl.fields()) field->accept(*this);

            output_.line() << "};\n";
        }

        if (scopes_.count(&decl)) {
            for (auto constant : scopes_.at(&decl)->values()) constant.second->accept(*this);
            for (auto function : scopes_.at(&decl)->functions()) function.second->accept(*this);
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
            }

            // constructor, copy-constructor and destructor are defined for non-trivial members of the union
            output_.line() << emit(decl.annotation().type) << "() {}\n";
            output_.line() << emit(decl.annotation().type) << "(const " << emit(decl.annotation().type) << "& other) : __tag(other.__tag) {\n";
            {
                struct guard inner(output_);
                output_.line() << "switch (other.__tag) {\n";
                for (auto subtype : std::static_pointer_cast<ast::variant_type>(decl.annotation().type)->types()) {
                    auto hash = std::to_string(std::hash<std::string>()(subtype->string()));
                    output_.line() << "case " << hash << "ull: _" << hash << " = other._" << hash << "; break;\n";
                }
                output_.line() << "default: break;\n";
                output_.line() << "}\n";
            }
            output_.line() << "}\n";
            output_.line() << "~" << emit(decl.annotation().type) << "() {}\n";

            output_.line() << "};\n";
        }

        if (scopes_.count(&decl)) {
            for (auto constant : scopes_.at(&decl)->values()) constant.second->accept(*this);
            for (auto function : scopes_.at(&decl)->functions()) function.second->accept(*this);
        }

        // create designated initializers for each variant type
        for (auto type : std::static_pointer_cast<ast::variant_type>(decl.annotation().type)->types()) {
            struct guard inner(output_);
            auto hash = std::to_string(std::hash<std::string>()(type->string()));
            
            output_.line() << emit(decl.annotation().type) << " " << emit(decl.annotation().type) << "_init_" << hash << "(" << emit(type, "init") << ") {\n";
            
            {
                struct guard inner(output_);
                output_.line() << emit(decl.annotation().type, "result") << ";\n";
                output_.line() << "result.__tag = " << hash << "ull;\n";
                output_.line() << "result._" << hash << " = init;\n";
                output_.line() << "return result;\n";
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
            // if entry point it initializes arguments properly from __argc, __argv
            if (&decl == entry_point_ && !decl.parameters().empty()) {
                struct guard inner(output_);
                auto args_name = std::static_pointer_cast<ast::parameter_declaration>(decl.parameters().front())->name().lexeme();
                output_.line() << "std::vector<__chars> __args_buffer((std::size_t) __argc);\n";
                output_.line() << "for (int i = 0; i < __argc; ++i) __args_buffer[i] = __chars(__argv[i]);\n";
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
                    body->exprnode() = stmts.front().get();
                    body->annotation().type = decl.body()->annotation().type;
                    decl.body() = body;
                    scopes_.emplace(decl.body().get(), new environment(decl.body().get(), scopes_.at(&decl)));
                    decl.body()->accept(*this);
                    break;
                }
                case ast::kind::block_expression:
                    decl.body()->accept(*this);
                    break;
                default:
                {
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
                    body->exprnode() = stmts.front().get();
                    body->annotation().type = decl.body()->annotation().type;
                    decl.body() = body;
                    scopes_.emplace(decl.body().get(), new environment(decl.body().get(), scopes_.at(&decl)));
                    decl.body()->accept(*this);
                    break;
                }
                case ast::kind::block_expression:
                    decl.body()->accept(*this);
                    break;
                default:
                {
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
        
                switch (result->category()) {
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
            // builtin functions
            if (auto fn = dynamic_cast<const ast::function_declaration*>(expr.annotation().referencing)) {
                // strips name from generic arguments
                auto non_generic_name = fn->name().lexeme().string();
                auto first = non_generic_name.find_first_of('(');
                auto last = non_generic_name.find_last_of(')');
                if (first != -1 && last != -1) non_generic_name.erase(first, last);
                // test name against builtin functions
                if (non_generic_name == "allocate") output_.stream() << "nscore_allocate<" << emit(expr.generics().front()->annotation().type) << ">";
                else if (non_generic_name == "deallocate") output_.stream() << "nscore_deallocate";
                else if (non_generic_name == "free") output_.stream() << "nscore_free";
                else if (non_generic_name == "sizeof") output_.stream() << "nscore_sizeof<" << emit(expr.generics().front()->annotation().type) << ">";
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

        output_.stream() << "{";
        
        for (unsigned i = 0; i < expr.elements().size(); ++i) {
            if (i > 0) output_.stream() << ", ";
            expr.elements().at(i)->accept(*this);
        }

        output_.stream() << "}";
    }
    
    void code_generator::visit(const ast::array_sized_expression& expr) 
    {
        if (emit_if_constant(expr)) return;

        output_.stream() << "{";
        
        for (unsigned i = 0; i < expr.size()->annotation().value.u.value(); ++i) {
            if (i > 0) output_.stream() << ", ";
            expr.value()->accept(*this);
        }

        output_.stream() << "}";
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
            if (auto method = dynamic_cast<const ast::function_declaration*>(expr.member()->annotation().referencing)) expr.member()->accept(*this);
            // method (property) call because it is a computed property
            else if (auto method = dynamic_cast<const ast::property_declaration*>(expr.member()->annotation().referencing)) {
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
                
                // with crash() call we implicitly pass file and line info to print the error
                if (expr.callee()->annotation().referencing && fullname(expr.callee()->annotation().referencing) == "nscore_crash") output_.stream() << ", \"" << expr.range().filename.string() << "\", " << expr.range().bline;

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

        // if type is declared as a tuple, then we have field access
        if (expr.expression()->annotation().type->declaration()) {
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
        else switch (result->category()) {
            case ast::type::category::chars_type:
                if (original->category() == ast::type::category::string_type) {
                    output_.stream() << emit(result) << "(";
                    expr.expression()->accept(*this);
                    output_.stream() << ".data())";
                }
                break;
            case ast::type::category::string_type:
                if (original->category() == ast::type::category::chars_type) {
                    output_.stream() << emit(result) << "(";
                    expr.expression()->accept(*this);
                    output_.stream() << ".data())";
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

        if (auto fn = scopes_.at(&expr)->outscope(environment::kind::function)) {
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

        std::cout << ast::printer().print(*std::dynamic_pointer_cast<ast::pattern_expression>(expr.pattern())->compiled()) << '\n';
        
        output_.stream() << "if (";
        std::dynamic_pointer_cast<ast::pattern_expression>(expr.pattern())->compiled()->accept(*this);
        output_.stream() << ") {\n";
        expr.body()->accept(*this);
        output_.line() << "}\n";
        if (expr.else_body()) {
            output_.line() << "else {\n";
            expr.else_body()->accept(*this);
            output_.line() << "}\n";
        }
    }

    void code_generator::visit(const ast::expression_statement& stmt)
    {
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
        if (stmt.expression()) {
            switch (stmt.expression()->kind()) {
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
        else output_.line() << "return";

        output_.stream() << ";\n";
    }
}