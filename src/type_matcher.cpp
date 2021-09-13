#include <algorithm>

#include "nemesis/analysis/type_matcher.hpp"

namespace nemesis {
    namespace ast {
        type_matcher::type_matcher(ast::pointer<ast::expression> context, ast::pointer<ast::type> pattern, diagnostic_publisher& publisher) : context_(context), pattern_(pattern), publisher_(publisher) {}
        
        void type_matcher::error(source_range range, const std::string& message, const std::string& explanation, const std::string& inlined) const
        {
            auto diag = diagnostic::builder()
                        .location(context_->range().begin())
                        .severity(diagnostic::severity::error)
                        .small(true)
                        .highlight(range, diagnostic::highlighter::mode::light)
                        .highlight(context_->range(), inlined)
                        .message(message)
                        .explanation(explanation)
                        .build();
            
            publisher_.publish(diag);
            throw mismatch();
        }

        bool type_matcher::match(ast::pointer<ast::type> expression, result& result, bool variadic_pattern) const
        {
            try {
                auto pattern = parameter::make_type(pattern_);
                pattern.variadic = variadic_pattern;
                match(pattern, parameter::make_type(expression), result);
            }
            catch (mismatch&) {
                result.invalidate();
                return false;
            }

            return true;
        }

        void type_matcher::match(parameter pattern, parameter expression, struct result& result) const
        {
            if (auto constparam = dynamic_cast<const ast::generic_const_parameter_declaration*>(pattern.referencing)) {
                //std::cout << "matching const param " << constparam->name() << " against " << expression.value << '\n';
                if (expression.kind == parameter::kind::type) {
                    //std::cout << "found a type then value is expected\n";
                    throw mismatch();
                }
                
                auto key = constparam->name().lexeme().string();
                auto found = result.bindings.find(key);
                
                if (found != result.bindings.end()) {
                    if (found->second.referencing && expression.referencing && found->second.referencing != expression.referencing) {
                        result.duplication = true;
                        error(constparam->name().range(), diagnostic::format("Parameter `$` has already been bound to constant `$`. You cannot rebind it to `$`, idiot!", key, dynamic_cast<const ast::generic_const_parameter_declaration*>(found->second.referencing)->name().lexeme(), dynamic_cast<const ast::generic_const_parameter_declaration*>(expression.referencing)->name().lexeme()), "", diagnostic::format("duplicating $", key));
                    }
                    else if (!found->second.referencing && !expression.referencing) {
                        if (!::operator==(found->second.value, expression.value)) {
                            result.duplication = true;
                            error(constparam->name().range(), diagnostic::format("Parameter `$` has already been bound to value `$`. You cannot rebind it to `$`, idiot!", key, found->second.value.simple(), expression.value.simple(), "", diagnostic::format("duplicating $", key)));
                        }
                    }
                    else {
                        throw mismatch();
                    }
                }
                else result.value(key, expression.value);
            }
            else if (pattern.kind == parameter::kind::type) {
                if (expression.kind == parameter::kind::value) {
                    //std::cout << "found a value then type is expected\n";
                    throw mismatch();
                }

                switch (pattern.type->category()) {
                case ast::type::category::generic_type:
                {
                    auto key = static_cast<const ast::type_declaration*>(pattern.type->declaration())->name().lexeme().string();
                    auto found = result.bindings.find(key);
                    if (found != result.bindings.end() && expression.type->category() != ast::type::category::unknown_type && !nemesis::types::compatible(found->second.type, expression.type)) {
                        result.duplication = true;
                        error(pattern.type->declaration()->range(), diagnostic::format("Parameter `$` has already been bound to type `$`. You cannot rebind it to `$`, idiot!", key, found->second.type->string(), expression.type->string()), "", diagnostic::format("duplicating $", key));
                    }
                    else {
                        result.type(key, expression.type);
                    }
                    break;
                }
                case ast::type::category::pointer_type:
                    if (expression.type->category() == ast::type::category::pointer_type) match(parameter::make_type(std::static_pointer_cast<ast::pointer_type>(pattern.type)->base()), parameter::make_type(std::static_pointer_cast<ast::pointer_type>(expression.type)->base()), result);
                    else match(parameter::make_type(std::static_pointer_cast<ast::pointer_type>(pattern.type)->base()), expression, result);

                    break;
                case ast::type::category::slice_type:
                    if (expression.type->category() == ast::type::category::slice_type) {
                        match(parameter::make_type(std::static_pointer_cast<ast::slice_type>(pattern.type)->base()), parameter::make_type(std::static_pointer_cast<ast::slice_type>(expression.type)->base()), result);
                    }
                    else if (expression.type->category() == ast::type::category::array_type) {
                        match(parameter::make_type(std::static_pointer_cast<ast::slice_type>(pattern.type)->base()), parameter::make_type(std::static_pointer_cast<ast::array_type>(expression.type)->base()), result);
                    }
                    else if (pattern.variadic) {
                        match(parameter::make_type(std::static_pointer_cast<ast::slice_type>(pattern.type)->base()), expression, result);
                    }
                    else {
                        //std::cout << "slice_type: expected slice or array type, found " << expression.type->string() << '\n';
                        throw mismatch(); 
                    }

                    break;
                case ast::type::category::array_type:
                {
                    if (expression.type->category() != ast::type::category::array_type) {
                        //std::cout << "array_type: expected array type, found " << expression.type->string() << '\n';
                        throw mismatch(); 
                    }

                    auto array_pattern = std::static_pointer_cast<ast::array_type>(pattern.type), array_expression = std::static_pointer_cast<ast::array_type>(expression.type);
                    match(parameter::make_type(array_pattern->base()), parameter::make_type(array_expression->base()), result);

                    constval size;
                    size.type = nemesis::types::usize();
                    size.u.size(nemesis::types::usize()->bits());
                    size.u.value(array_expression->size());

                    //std::cout << "matching pattern " << array_pattern->string() << " with parametric size = " << array_pattern->parametric_size() << '\n';
                    //std::cout << "array type has size " << array_expression->size() << '\n';

                    if (auto parametric = array_pattern->parametric_size()) {
                        parameter value;
                        value.kind = parameter::kind::value;
                        value.value.type = parametric->annotation().type;
                        value.type = parametric->annotation().type;
                        value.referencing = parametric;
                        match(value, parameter::make_value(size), result);
                    }
                    else {
                        constval value;
                        value.type = nemesis::types::usize();
                        value.u.size(nemesis::types::usize()->bits());
                        value.u.value(array_pattern->size());
                        match(parameter::make_value(value), parameter::make_value(size), result);
                    }

                    break;
                }
                case ast::type::category::tuple_type:
                {
                    auto tuple_pattern = std::static_pointer_cast<ast::tuple_type>(pattern.type);

                    if (expression.type->category() != ast::type::category::tuple_type) {
                        //std::cout << "tuple_type: expected tuple type, found " << expression.type->string() << '\n';
                        throw mismatch();
                    }

                    auto tuple_type = std::static_pointer_cast<ast::tuple_type>(expression.type);

                    if (tuple_pattern->components().size() != tuple_type->components().size()) {
                        //std::cout << "tuple_type_expression: expected " << tuple_pattern->components().size() << " components, found " << tuple_type->components().size() << '\n';
                        throw mismatch();
                    }
                    
                    for (unsigned i = 0; i < tuple_pattern->components().size(); ++i) match(parameter::make_type(tuple_pattern->components().at(i)), parameter::make_type(tuple_type->components().at(i)), result);

                    break;
                }
                case ast::type::category::variant_type:
                {
                    auto variant_pattern = std::static_pointer_cast<ast::variant_type>(pattern.type);

                    if (expression.type->category() != ast::type::category::variant_type) {
                        //std::cout << "variant_type: expected variant type, found " << expression.type->string() << '\n';
                        throw mismatch();
                    }

                    auto variant_type = std::static_pointer_cast<ast::variant_type>(expression.type);

                    if (variant_pattern->types().size() != variant_type->types().size()) {
                        //std::cout << "variant_type: expected " << variant_pattern->types().size() << " compoenents, found " << variant_type->types().size() << '\n';
                        throw mismatch();
                    }
                    
                    for (unsigned i = 0; i < variant_pattern->types().size(); ++i) match(parameter::make_type(variant_pattern->types().at(i)), parameter::make_type(variant_type->types().at(i)), result);

                    break;
                }
                case ast::type::category::structure_type:
                {
                    auto record_pattern = std::static_pointer_cast<ast::structure_type>(pattern.type);

                    if (expression.type->category() != ast::type::category::structure_type) {
                        //std::cout << "structure_type: expected record type, found " << expression.type->string() << '\n';
                        throw mismatch();
                    }

                    auto record_type = std::static_pointer_cast<ast::structure_type>(expression.type);

                    if (record_pattern->fields().size() != record_type->fields().size()) {
                        //std::cout << "structure_type: expected " << record_pattern->fields().size() << " fields, found " << record_type->fields().size() << '\n';
                        throw mismatch();
                    }
                    
                    for (auto field : record_pattern->fields()) {
                        auto found = std::find_if(record_type->fields().begin(), record_type->fields().end(), [&] (ast::structure_type::component f) { return f.name == field.name; });
                        
                        if (found == record_type->fields().end()) {
                            //std::cout << "structure_type: field " << field.name << " does not exist";
                            throw mismatch();
                        }

                        match(parameter::make_type(field.type), parameter::make_type(found->type), result);
                    }

                    break;
                }
                case ast::type::category::function_type:
                {
                    auto function_pattern = std::static_pointer_cast<ast::function_type>(pattern.type);

                    if (expression.kind == parameter::kind::value) {
                        //std::cout << "function_type: found a value then type is expected\n";
                        throw mismatch();
                    }
                    else if (expression.type->category() != ast::type::category::function_type) {
                        //std::cout << "function_type: expected function type, found " << expression.type->string() << '\n';
                        throw mismatch();
                    }

                    auto function_type = std::static_pointer_cast<ast::function_type>(expression.type);

                    if (function_pattern->formals().size() != function_type->formals().size()) {
                        //std::cout << "function_type: expected " << function_pattern->formals().size() << " parameters, found " << function_type->formals().size() << '\n';
                        throw mismatch();
                    }

                    match(parameter::make_type(function_pattern->result()), parameter::make_type(function_type->result()), result);

                    for (unsigned i = 0; i < function_pattern->formals().size(); ++i) {
                        match(parameter::make_type(function_pattern->formals().at(i)), parameter::make_type(function_type->formals().at(i)), result);
                    }

                    break;
                }
                default:
                    if (!nemesis::types::compatible(pattern.type, expression.type)) {
                        //std::cout << "mismatch, " << pattern.type->string() << " and " << expression.type->string() << '\n';
                        throw mismatch();
                    }
                }
            }
        }
    }
}