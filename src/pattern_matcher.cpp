#include <algorithm>

#include "nemesis/analysis/pattern_matcher.hpp"
#include "nemesis/analysis/type.hpp"
#include "nemesis/analysis/checker.hpp"

namespace nemesis {
    namespace ast {
        pattern_matcher::binding::binding(const token& name, const ast::expression* value) : name(name), value(value) {}

        bool pattern_matcher::binding::operator<(const binding& other) const { return name.lexeme().compare(other.name.lexeme()) < 0; }

        pattern_matcher::pattern_matcher(const ast::expression& pattern, diagnostic_publisher& publisher, checker& checker) : pattern_(&pattern), publisher_(publisher), checker_(checker) {}

        void pattern_matcher::mismatched(const ast::expression* pattern, ast::pointer<ast::type> expected, const ast::expression* expression) const
        {
            auto diag = diagnostic::builder()
                        .location(pattern->range().begin())
                        .severity(diagnostic::severity::error)
                        .highlight(pattern->range(), diagnostic::format("expected $", expected->string()))
                        .highlight(expression->range(), diagnostic::highlighter::mode::light)
                        .message(diagnostic::format("Type mismatch between pattern and expression, found `$` and `$`, f*cker!", pattern->annotation().type->string(), expected->string()))
                        .build();
            
            publisher_.publish(diag);
            pattern->invalid(true);
            throw pattern_matcher::mismatch();
        }

        void pattern_matcher::error(const ast::expression* pattern, const std::string& message, const std::string& explanation, const std::string& inlined) const
        {
            auto diag = diagnostic::builder()
                        .location(pattern->range().begin())
                        .severity(diagnostic::severity::error)
                        .highlight(pattern->range(), inlined)
                        .message(message)
                        .explanation(explanation)
                        .build();
            
            publisher_.publish(diag);
            throw pattern_matcher::mismatch();
        }

        bool pattern_matcher::bind(const token& name, const ast::expression* value, result& result) const
        {
            if (name.lexeme().string().compare("_") == 0) return false;
            
            if (!result.put(name, value)) {
                auto diag = diagnostic::builder()
                            .location(name.location())
                            .severity(diagnostic::severity::error)
                            .highlight(name.range(), "conflicting")
                            .message(diagnostic::format("You already have created a binding with variable `$` in this pattern, idiot!", name.lexeme()))
                            .note(result.bindings.find(binding(name, value))->name.range(), "Here's the other binding you created before.")
                            .build();
                
                publisher_.publish(diag);
                throw pattern_matcher::mismatch();
            }

            return true;
        }

        pattern_matcher::result pattern_matcher::match(const ast::expression& expression) const
        {
            result result;
            
            try {
                result.condition = match(pattern_, expression.annotation().type, &expression, expression.clone(), result);
            }
            catch (mismatch&) {
                result.invalidate();
            }

            return result;
        }

        ast::pointer<ast::expression> pattern_matcher::match(const ast::expression* pattern, ast::pointer<ast::type> expected, const ast::expression* expression, ast::pointer<ast::expression> tree, struct result& result) const
        {
            if (pattern->invalid()) return nullptr;

            switch (pattern->kind()) {
            case ast::kind::path_pattern_expression:
            {
                auto path_pattern = static_cast<const path_pattern_expression*>(pattern);
                // path cannot be a type, it could be:
                // 1) a binding, so a variable name
                if (path_pattern->path()->kind() == ast::kind::identifier_expression && pattern->annotation().type->category() == ast::type::category::unknown_type) {
                    auto identifier = std::static_pointer_cast<ast::identifier_expression>(path_pattern->path())->identifier();
                    if (bind(identifier, expression, result)) {
                        auto binding = ast::create<ast::var_declaration>(source_range(), std::vector<token>(), identifier, nullptr, tree);
                        binding->annotation().type = tree->annotation().type;
                        result.declarations.push_back(binding);
                    }
                }
                // 2) a constant name
                else if (!nemesis::types::assignment_compatible(expected, pattern->annotation().type)) {
                    mismatched(pattern, expected, expression);
                }
                else if (path_pattern->path()->annotation().referencing) {
                    auto newtree = tree;
                    ast::pointer<ast::expression> condition = nullptr;

                    if (auto implicit = checker::implicit_cast(pattern->annotation().type, tree)) {
                        newtree = implicit;
                    }
                    
                    if (auto variant = std::dynamic_pointer_cast<ast::variant_type>(expected)) {
                        if (variant->contains(pattern->annotation().type)) {
                            auto name = token::builder().artificial(true).kind(token::kind::identifier).lexeme(utf8::span::builder().concat("__tag").build()).build();
                            auto field = ast::create<ast::identifier_expression>(source_range(), name, ast::pointers<ast::expression>());
                            auto left = ast::create<ast::member_expression>(source_range(), tree, field);
                            auto typehash = std::to_string(std::hash<std::string>()(pattern->annotation().type->string()));
                            auto tag = token::builder().artificial(true).kind(token::kind::integer_literal).lexeme(utf8::span::builder().concat((typehash + "usize").data()).build()).build();
                            auto right = ast::create<ast::literal_expression>(tag);
                            right->annotation().type = nemesis::types::usize();
                            right->annotation().value = checker_.evaluate(right);
                            condition = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::equal_equal).lexeme(utf8::span::builder().concat("==").build()).build(), left, right);
                            condition->annotation().type = nemesis::types::boolean();
                            name = token::builder().artificial(true).kind(token::kind::identifier).lexeme(utf8::span::builder().concat(("_" + typehash).data()).build()).build();
                            field = ast::create<ast::identifier_expression>(source_range(), name, ast::pointers<ast::expression>());
                            newtree = left = ast::create<ast::member_expression>(source_range(), tree, field);
                        }
                    }

                    auto comparison = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::equal_equal).lexeme(utf8::span::builder().concat("==").build()).build(), newtree, from_pattern(pattern));
                    comparison->annotation().type = nemesis::types::boolean();

                    if (condition) {
                        comparison = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::amp_amp).lexeme(utf8::span::builder().concat("&&").build()).build(), condition, comparison);
                        comparison->annotation().type = nemesis::types::boolean();
                    }

                    return comparison;
                }
                
                return nullptr;
            }
            case ast::kind::record_pattern_expression:
            {
                auto record_pattern = static_cast<const record_pattern_expression*>(pattern);
                if (!nemesis::types::assignment_compatible(expected, pattern->annotation().type)) {
                    mismatched(pattern, expected, expression);
                }
                else {
                    auto newtree = tree;
                    ast::pointer<ast::expression> condition = nullptr;

                    if (auto implicit = checker::implicit_cast(pattern->annotation().type, tree)) {
                        newtree = implicit;
                    }

                    if (auto variant = std::dynamic_pointer_cast<ast::variant_type>(expected)) {
                        if (variant->contains(pattern->annotation().type)) {
                            auto name = token::builder().artificial(true).kind(token::kind::identifier).lexeme(utf8::span::builder().concat("__tag").build()).build();
                            auto field = ast::create<ast::identifier_expression>(source_range(), name, ast::pointers<ast::expression>());
                            auto left = ast::create<ast::member_expression>(source_range(), tree, field);
                            auto typehash = std::to_string(std::hash<std::string>()(pattern->annotation().type->string()));
                            auto tag = token::builder().artificial(true).kind(token::kind::integer_literal).lexeme(utf8::span::builder().concat((typehash + "usize").data()).build()).build();
                            auto right = ast::create<ast::literal_expression>(tag);
                            right->annotation().type = nemesis::types::usize();
                            right->annotation().value = checker_.evaluate(right);
                            condition = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::equal_equal).lexeme(utf8::span::builder().concat("==").build()).build(), left, right);
                            condition->annotation().type = nemesis::types::boolean();
                        }
                    }

                    if (auto structure = std::dynamic_pointer_cast<ast::structure_type>(pattern->annotation().type)) {
                        for (auto i = 0; i < record_pattern->fields().size() && record_pattern->fields().at(i)->kind() != ast::kind::ignore_pattern_expression; ++i) {
                            auto name = token::builder().artificial(true).kind(token::kind::identifier).lexeme(utf8::span::builder().concat(structure->fields().at(i).name.data(), structure->fields().at(i).name.size()).build()).build();
                            auto field = ast::create<ast::identifier_expression>(source_range(), name, ast::pointers<ast::expression>());
                            auto member = ast::create<ast::member_expression>(source_range(), newtree, field);
                            member->annotation().type = structure->fields().at(i).type;
                            field->annotation().type = structure->fields().at(i).type;
                            if (auto expr = match(record_pattern->fields().at(i).get(), structure->fields().at(i).type, expression, member, result)) {
                                if (!condition) condition = expr;
                                else condition = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::amp_amp).lexeme(utf8::span::builder().concat("&&").build()).build(), condition, expr);
                            }
                        }
                    }
                    else if (auto tuple = std::dynamic_pointer_cast<ast::tuple_type>(pattern->annotation().type)) {
                        for (auto i = 0; i < record_pattern->fields().size() && record_pattern->fields().at(i)->kind() != ast::kind::ignore_pattern_expression; ++i) {
                            auto component = token::builder().artificial(true).kind(token::kind::integer_literal).lexeme(utf8::span::builder().concat(std::to_string(i).data()).build()).build();
                            auto member = ast::create<ast::tuple_index_expression>(source_range(), newtree, component);
                            member->annotation().type = tuple->components().at(i);
                            if (auto expr = match(record_pattern->fields().at(i).get(), tuple->components().at(i), expression, member, result)) {
                                if (!condition) condition = expr;
                                else condition = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::amp_amp).lexeme(utf8::span::builder().concat("&&").build()).build(), condition, expr);
                                condition->annotation().type = nemesis::types::boolean();
                            }
                        }
                    }

                    return condition;
                }
            }
            case ast::kind::labeled_record_pattern_expression:
            {
                auto record_pattern = static_cast<const labeled_record_pattern_expression*>(pattern);
                if (!nemesis::types::assignment_compatible(expected, pattern->annotation().type)) {
                    mismatched(pattern, expected, expression);
                }
                else {
                    auto newtree = tree;
                    ast::pointer<ast::expression> condition = nullptr;

                    if (auto implicit = checker::implicit_cast(pattern->annotation().type, tree)) {
                        newtree = implicit;
                    }
                    
                    if (auto variant = std::dynamic_pointer_cast<ast::variant_type>(expected)) {
                        if (variant->contains(pattern->annotation().type)) {
                            auto name = token::builder().artificial(true).kind(token::kind::identifier).lexeme(utf8::span::builder().concat("__tag").build()).build();
                            auto field = ast::create<ast::identifier_expression>(source_range(), name, ast::pointers<ast::expression>());
                            auto left = ast::create<ast::member_expression>(source_range(), tree, field);
                            auto typehash = std::to_string(std::hash<std::string>()(pattern->annotation().type->string()));
                            auto tag = token::builder().artificial(true).kind(token::kind::integer_literal).lexeme(utf8::span::builder().concat((typehash + "usize").data()).build()).build();
                            auto right = ast::create<ast::literal_expression>(tag);
                            right->annotation().type = nemesis::types::usize();
                            right->annotation().value = checker_.evaluate(right);
                            condition = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::equal_equal).lexeme(utf8::span::builder().concat("==").build()).build(), left, right);
                            condition->annotation().type = nemesis::types::boolean();
                        }
                    }

                    if (auto structure = std::dynamic_pointer_cast<ast::structure_type>(pattern->annotation().type)) {
                        for (auto i = 0; i < record_pattern->fields().size(); ++i) {
                            auto field = ast::create<ast::identifier_expression>(source_range(), record_pattern->fields().at(i).field, ast::pointers<ast::expression>());
                            auto member = ast::create<ast::member_expression>(source_range(), newtree, field);
                            auto type = std::find_if(structure->fields().begin(), structure->fields().end(), [&] (ast::structure_type::component field) { return field.name == record_pattern->fields().at(i).field.lexeme().string(); })->type;
                            member->annotation().type = type;
                            field->annotation().type = type;
                            if (auto expr = match(record_pattern->fields().at(i).value.get(), type, expression, member, result)) {
                                if (!condition) condition = expr;
                                else condition = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::amp_amp).lexeme(utf8::span::builder().concat("&&").build()).build(), condition, expr);
                            }
                        }
                    }

                    return condition;
                }
            }
            case ast::kind::literal_pattern_expression:
            {
                if (!nemesis::types::assignment_compatible(expected, pattern->annotation().type)) {
                    mismatched(pattern, expected, expression);
                }
                else {
                    auto newtree = tree;
                    ast::pointer<ast::expression> condition = nullptr;

                    if (auto implicit = checker::implicit_cast(pattern->annotation().type, tree)) {
                        newtree = implicit;
                    }
                    
                    if (auto variant = std::dynamic_pointer_cast<ast::variant_type>(expected)) {
                        if (variant->contains(pattern->annotation().type)) {
                            auto name = token::builder().artificial(true).kind(token::kind::identifier).lexeme(utf8::span::builder().concat("__tag").build()).build();
                            auto field = ast::create<ast::identifier_expression>(source_range(), name, ast::pointers<ast::expression>());
                            auto left = ast::create<ast::member_expression>(source_range(), tree, field);
                            auto typehash = std::to_string(std::hash<std::string>()(pattern->annotation().type->string()));
                            auto tag = token::builder().artificial(true).kind(token::kind::integer_literal).lexeme(utf8::span::builder().concat((typehash + "usize").data()).build()).build();
                            auto right = ast::create<ast::literal_expression>(tag);
                            right->annotation().type = nemesis::types::usize();
                            right->annotation().value = checker_.evaluate(right);
                            condition = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::equal_equal).lexeme(utf8::span::builder().concat("==").build()).build(), left, right);
                            condition->annotation().type = nemesis::types::boolean();
                            name = token::builder().artificial(true).kind(token::kind::identifier).lexeme(utf8::span::builder().concat(("_" + typehash).data()).build()).build();
                            field = ast::create<ast::identifier_expression>(source_range(), name, ast::pointers<ast::expression>());
                            newtree = left = ast::create<ast::member_expression>(source_range(), tree, field);
                        }
                    }

                    auto comparison = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::equal_equal).lexeme(utf8::span::builder().concat("==").build()).build(), newtree, from_pattern(pattern));
                    comparison->annotation().type = nemesis::types::boolean();

                    if (condition) {
                        comparison = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::amp_amp).lexeme(utf8::span::builder().concat("&&").build()).build(), condition, comparison);
                        comparison->annotation().type = nemesis::types::boolean();
                    }

                    return comparison;
                }
            }
            case ast::kind::range_pattern_expression:
            {
                if (!nemesis::types::assignment_compatible(expected, pattern->annotation().type)) {
                    mismatched(pattern, expected, expression);
                }
                else {
                    auto range = static_cast<const ast::range_pattern_expression*>(pattern);
                    ast::pointer<ast::expression> comparison, rcomparison;
                    auto newtree = tree;
                    ast::pointer<ast::expression> condition = nullptr;

                    if (auto implicit = checker::implicit_cast(pattern->annotation().type, tree)) {
                        newtree = implicit;
                    }
                    
                    if (auto variant = std::dynamic_pointer_cast<ast::variant_type>(expected)) {
                        if (variant->contains(pattern->annotation().type)) {
                            auto name = token::builder().artificial(true).kind(token::kind::identifier).lexeme(utf8::span::builder().concat("__tag").build()).build();
                            auto field = ast::create<ast::identifier_expression>(source_range(), name, ast::pointers<ast::expression>());
                            auto left = ast::create<ast::member_expression>(source_range(), tree, field);
                            auto typehash = std::to_string(std::hash<std::string>()(pattern->annotation().type->string()));
                            auto tag = token::builder().artificial(true).kind(token::kind::integer_literal).lexeme(utf8::span::builder().concat((typehash + "usize").data()).build()).build();
                            auto right = ast::create<ast::literal_expression>(tag);
                            right->annotation().type = nemesis::types::usize();
                            right->annotation().value = checker_.evaluate(right);
                            condition = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::equal_equal).lexeme(utf8::span::builder().concat("==").build()).build(), left, right);
                            condition->annotation().type = nemesis::types::boolean();
                            name = token::builder().artificial(true).kind(token::kind::identifier).lexeme(utf8::span::builder().concat(("_" + typehash).data()).build()).build();
                            field = ast::create<ast::identifier_expression>(source_range(), name, ast::pointers<ast::expression>());
                            newtree = left = ast::create<ast::member_expression>(source_range(), tree, field);
                        }
                    }

                    if (!range->start() && !range->end()) return nullptr;

                    if (range->start()) {
                        comparison = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::greater_equal).lexeme(utf8::span::builder().concat(">=").build()).build(), newtree, from_pattern(range->start().get()));
                        comparison->annotation().type = nemesis::types::boolean();
                    }

                    if (range->end()) {
                        if (range->is_inclusive()) rcomparison = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::less_equal).lexeme(utf8::span::builder().concat("<=").build()).build(), newtree, from_pattern(range->end().get()));
                        else rcomparison = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::less).lexeme(utf8::span::builder().concat("<").build()).build(), newtree, from_pattern(range->end().get()));
                        rcomparison->annotation().type = nemesis::types::boolean();
                    }

                    if (range->start() && range->end()) {
                        comparison = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::amp_amp).lexeme(utf8::span::builder().concat("&&").build()).build(), comparison, rcomparison);
                        comparison->annotation().type = nemesis::types::boolean();
                    }
                    else if (range->end()) comparison = rcomparison;

                    if (condition) {
                        comparison = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::amp_amp).lexeme(utf8::span::builder().concat("&&").build()).build(), condition, comparison);
                        comparison->annotation().type = nemesis::types::boolean();
                    }

                    return comparison;
                }
            }
            case ast::kind::tuple_pattern_expression:
            {
                ast::pointer<ast::expression> condition = nullptr;

                if (expected->category() != ast::type::category::tuple_type) {
                    mismatched(pattern, expected, expression);
                }
                else if (auto tuple_pattern_type = std::dynamic_pointer_cast<ast::tuple_type>(pattern->annotation().type)) {
                    auto tuple_pattern = static_cast<const ast::tuple_pattern_expression*>(pattern);
                    auto tuple_type = std::static_pointer_cast<ast::tuple_type>(expected);
                    
                    if (tuple_pattern_type->components().size() > tuple_type->components().size()) {
                        error(pattern, diagnostic::format("Too many elements in this tuple pattern. I count `$` when `$` are expected!", tuple_pattern_type->components().size(), tuple_type->components().size()));
                    }
                    else if (tuple_pattern_type->components().size() < tuple_type->components().size() && (tuple_pattern->elements().empty() || tuple_pattern->elements().back()->kind() != ast::kind::ignore_pattern_expression)) {
                        error(pattern, diagnostic::format("Too few elements in this tuple pattern. I count `$` when `$` are expected!", tuple_pattern_type->components().size(), tuple_type->components().size()));
                    }
                    else {
                        for (unsigned i = 0; i < tuple_pattern_type->components().size(); ++i) {
                            auto component = token::builder().artificial(true).kind(token::kind::integer_literal).lexeme(utf8::span::builder().concat(std::to_string(i).data()).build()).build();
                            auto member = ast::create<ast::tuple_index_expression>(source_range(), tree, component);
                            member->annotation().type = tuple_type->components().at(i);
                            if (auto expr = match(tuple_pattern->elements().at(i).get(), tuple_type->components().at(i), expression, member, result)) {
                                if (!condition) condition = expr;
                                else condition = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::amp_amp).lexeme(utf8::span::builder().concat("&&").build()).build(), condition, expr);
                                condition->annotation().type = nemesis::types::boolean();
                            }
                        }
                    }
                }
                
                return condition;
            }
            case ast::kind::array_pattern_expression:
            {
                auto array_pattern_type = std::dynamic_pointer_cast<ast::array_type>(pattern->annotation().type);
                ast::pointer<ast::expression> condition = nullptr;

                if (!array_pattern_type) return nullptr;

                if (auto slice_type = std::dynamic_pointer_cast<ast::slice_type>(expected)) {
                    auto array_pattern = static_cast<const ast::array_pattern_expression*>(pattern);

                    auto size = ast::create<ast::identifier_expression>(source_range(), token::builder().artificial(true).kind(token::kind::identifier).lexeme(utf8::span::builder().concat("size").build()).build(), ast::pointers<ast::expression>());
                    auto lhs = ast::create<ast::member_expression>(source_range(), tree, size);
                    lhs->annotation().type = nemesis::types::usize();
                    auto rhs = ast::create<ast::literal_expression>(token::builder().artificial(true).kind(token::kind::integer_literal).lexeme(utf8::span::builder().concat(std::to_string(array_pattern_type->size()).data()).build()).build());
                    rhs->annotation().type = nemesis::types::usize();
                    rhs->annotation().value = checker_.evaluate(rhs);
                    condition = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::equal_equal).lexeme(utf8::span::builder().concat("==").build()).build(), lhs, rhs);
                    condition->annotation().type = nemesis::types::boolean();

                    for (unsigned i = 0; i < array_pattern_type->size(); ++i) {
                        auto index = ast::create<ast::literal_expression>(token::builder().artificial(true).kind(token::kind::integer_literal).lexeme(utf8::span::builder().concat(std::to_string(i).data()).build()).build());
                        index->annotation().type = nemesis::types::usize();
                        index->annotation().value = checker_.evaluate(index);
                        auto member = ast::create<ast::array_index_expression>(source_range(), tree, index);
                        member->annotation().type = slice_type->base();
                        if (auto expr = match(array_pattern->elements().at(i).get(), slice_type->base(), expression, member, result)) {
                            if (!condition) condition = expr;
                            else condition = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::amp_amp).lexeme(utf8::span::builder().concat("&&").build()).build(), condition, expr);
                            condition->annotation().type = nemesis::types::boolean();
                        }
                    }
                }
                else if (auto array_type = std::dynamic_pointer_cast<ast::array_type>(expected)) {
                    auto array_pattern = static_cast<const ast::array_pattern_expression*>(pattern);
                    
                    if (array_pattern_type->size() > array_type->size()) {
                        error(pattern, diagnostic::format("Too many elements in this array pattern. I count `$` when `$` are expected!", array_pattern_type->size(), array_type->size()));
                    }
                    else if (array_pattern_type->size() < array_type->size() && (array_pattern->elements().empty() || array_pattern->elements().back()->kind() != ast::kind::ignore_pattern_expression)) {
                        error(pattern, diagnostic::format("Too few elements in this array pattern. I count `$` when `$` are expected!", array_pattern_type->size(), array_type->size()));
                    }
                    else {
                        for (unsigned i = 0; i < array_pattern_type->size(); ++i) {
                            auto index = ast::create<ast::literal_expression>(token::builder().artificial(true).kind(token::kind::integer_literal).lexeme(utf8::span::builder().concat(std::to_string(i).data()).build()).build());
                            index->annotation().type = nemesis::types::usize();
                            index->annotation().value = checker_.evaluate(index);
                            auto member = ast::create<ast::array_index_expression>(source_range(), tree, index);
                            member->annotation().type = array_type->base();
                            if (auto expr = match(array_pattern->elements().at(i).get(), array_type->base(), expression, member, result)) {
                                if (!condition) condition = expr;
                                else condition = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::amp_amp).lexeme(utf8::span::builder().concat("&&").build()).build(), condition, expr);
                                condition->annotation().type = nemesis::types::boolean();
                            }
                        }
                    }
                }
                
                return condition;
            }
            case ast::kind::or_pattern_expression:
            {
                auto or_pattern = static_cast<const ast::or_pattern_expression*>(pattern);
                auto left = match(or_pattern->left().get(), expected, expression, tree, result);
                auto right = match(or_pattern->right().get(), expected, expression, tree, result);
                
                if (left && right) {
                    auto result = ast::create<ast::binary_expression>(source_range(), token::builder().artificial(true).kind(token::kind::line_line).lexeme(utf8::span::builder().concat("||").build()).build(), left, right);
                    result->annotation().type = nemesis::types::boolean();
                    return result;
                }
                else if (left) {
                    return left;
                }
                else if (right) {
                    return right;
                }

                return nullptr;
            }
            case ast::kind::ignore_pattern_expression:
                return nullptr;
            case ast::kind::implicit_conversion_expression:
                return match(static_cast<const ast::implicit_conversion_expression*>(pattern)->expression().get(), expected, expression, tree, result);
            default:
                error(pattern, "This kind of pattern is not allowed, idiot!");
            }

            return nullptr;
        }

        ast::pointer<ast::expression> pattern_matcher::from_pattern(const ast::expression* pattern) const
        {
            switch (pattern->kind()) {
            case ast::kind::literal_pattern_expression:
            {
                auto result = ast::create<ast::literal_expression>(static_cast<const ast::literal_pattern_expression*>(pattern)->value());
                result->annotation().type = pattern->annotation().type;
                result->annotation().value = checker_.evaluate(result);
                return result;
            }
            case ast::kind::path_pattern_expression:
                if (pattern->annotation().value.type && pattern->annotation().value.type->category() != ast::type::category::unknown_type) {
                    auto result = ast::create<ast::literal_expression>(token::builder().artificial(true).lexeme(utf8::span::builder().build()).build());
                    result->annotation().value = pattern->annotation().value;
                    result->annotation().type = pattern->annotation().type;
                    return result;        
                }
                else if (pattern->annotation().type && pattern->annotation().referencing) {
                    auto result =  static_cast<const ast::path_pattern_expression*>(pattern)->path();
                    result->annotation() = pattern->annotation();
                    return result;
                }
            default:
                break;
            }

            return nullptr;
        }
    }
}