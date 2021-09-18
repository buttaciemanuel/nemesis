#include "nemesis/analysis/checker.hpp"
#include "nemesis/analysis/evaluator.hpp"
#include "nemesis/analysis/pattern_matcher.hpp"
#include "nemesis/analysis/type_matcher.hpp"
#include "utils/strings.hpp"

#include <map>
#include <functional>
#include <iostream>

namespace nemesis {
    environment::environment(const ast::node* enclosing, environment* parent) : 
        enclosing_(enclosing), parent_(parent) 
    {
        if (parent_) parent_->children_.push_back(this);
    }

    const ast::declaration* environment::value(std::string name, bool recursive) const
    {
        auto result = values_.find(name);
        
        if (result != values_.end()) {
            return result->second;
        }
        else if (parent_ && recursive) {
            return parent_->value(name);
        }
        
        return nullptr;
    }

    const ast::declaration* environment::function(std::string name, bool recursive) const
    {
        auto result = functions_.find(name);

        if (result != functions_.end()) {
            return result->second;
        }
        else if (parent_ && recursive) {
            return parent_->function(name);
        }

        return nullptr;
    }

    const ast::type_declaration* environment::type(std::string name, bool recursive) const
    {
        auto result = types_.find(name);
        
        if (result != types_.end()) {
            return result->second;
        }
        else if (parent_ && recursive) {
            return parent_->type(name);
        }

        return nullptr;
    }

    const ast::concept_declaration* environment::concept(std::string name, bool recursive) const
    {
        auto result = concepts_.find(name);
        
        if (result != concepts_.end()) {
            return result->second;
        }
        else if (parent_ && recursive) {
            return parent_->concept(name);
        }

        return nullptr;
    }

    void environment::value(std::string name, const ast::declaration* decl)
    {
        if (name == "_") return;
        values_.emplace(name, decl);
        // sets scope only if it was not already set before
        if (!decl->annotation().scope) decl->annotation().scope = enclosing_;
    }

    void environment::function(std::string name, const ast::declaration* fdecl)
    {
        if (name == "_") return;
        functions_.emplace(name, fdecl);
        // sets scope only if it was not already set before
        if (!fdecl->annotation().scope) fdecl->annotation().scope = enclosing_;
    }

    void environment::type(std::string name, const ast::type_declaration* tdecl)
    {
        if (name == "_") return;
        types_.emplace(name, tdecl);
        // sets scope only if it was not already set before
        if (!tdecl->annotation().scope) tdecl->annotation().scope = enclosing_;
    }

    void environment::concept(std::string name, const ast::concept_declaration* cdecl)
    {
        if (name == "_") return;
        concepts_.emplace(name, cdecl);
        // sets scope only if it was not already set before
        if (!cdecl->annotation().scope) cdecl->annotation().scope = enclosing_;
    }

    bool environment::inside(environment::kind ctx) const
    {
        auto scope = this;

        switch (ctx) {
            case kind::block:
                return dynamic_cast<const ast::block_expression*>(scope->enclosing_);
            case kind::declaration:
                return dynamic_cast<const ast::declaration*>(scope->enclosing_);
            case kind::function:
                while (scope) {
                    if (dynamic_cast<const ast::function_declaration*>(scope->enclosing_)) return true;
                    else if (dynamic_cast<const ast::function_expression*>(scope->enclosing_)) return true;
                    else if (dynamic_cast<const ast::expression*>(scope->enclosing_)) scope = scope->parent_;
                    else if (dynamic_cast<const ast::source_unit_declaration*>(scope->enclosing_)) return false;
                    else if (dynamic_cast<const ast::test_declaration*>(scope->enclosing_)) return false;
                    else if (dynamic_cast<const ast::declaration*>(scope->enclosing_)) return false;
                }

                return false;
            case kind::global:
                return dynamic_cast<const ast::source_unit_declaration*>(scope->enclosing_); 
            case kind::loop:
                while (scope) {
                    if (dynamic_cast<const ast::function_declaration*>(scope->enclosing_)) return false;
                    else if (dynamic_cast<const ast::function_expression*>(scope->enclosing_)) return false;
                    else if (dynamic_cast<const ast::for_loop_expression*>(scope->enclosing_)) return true;
                    else if (dynamic_cast<const ast::for_range_expression*>(scope->enclosing_)) return true;
                    else if (dynamic_cast<const ast::expression*>(scope->enclosing_)) scope = scope->parent_;
                    else if (dynamic_cast<const ast::source_unit_declaration*>(scope->enclosing_)) return false;
                    else if (dynamic_cast<const ast::test_declaration*>(scope->enclosing_)) return false;
                    else if (dynamic_cast<const ast::declaration*>(scope->enclosing_)) return false;
                }

                return false;
            case kind::test:
                while (scope) {
                    if (dynamic_cast<const ast::expression*>(scope->enclosing_)) scope = scope->parent_;
                    else if (dynamic_cast<const ast::source_unit_declaration*>(scope->enclosing_)) return false;
                    else if (dynamic_cast<const ast::test_declaration*>(scope->enclosing_)) return true;
                    else if (dynamic_cast<const ast::declaration*>(scope->enclosing_)) return false;
                }

                return false;
            default:
                break;
        }

        return false;
    }

    const ast::node* environment::outscope(kind ctx) const
    {
        auto scope = this;

        switch (ctx) {
            case kind::block:
                return dynamic_cast<const ast::block_expression*>(scope->enclosing_);
            case kind::declaration:
                return dynamic_cast<const ast::declaration*>(scope->enclosing_);
            case kind::function:
                while (scope && !dynamic_cast<const ast::workspace*>(scope->enclosing_)) {
                    if (dynamic_cast<const ast::function_declaration*>(scope->enclosing_)) return scope->enclosing_;
                    else if (dynamic_cast<const ast::property_declaration*>(scope->enclosing_)) return scope->enclosing_;
                    else if (dynamic_cast<const ast::function_expression*>(scope->enclosing_)) return scope->enclosing_;
                    else if (dynamic_cast<const ast::expression*>(scope->enclosing_)) scope = scope->parent_;
                    else if (dynamic_cast<const ast::source_unit_declaration*>(scope->enclosing_)) return nullptr;
                    else if (dynamic_cast<const ast::test_declaration*>(scope->enclosing_)) return nullptr;
                    else if (dynamic_cast<const ast::declaration*>(scope->enclosing_)) return nullptr;
                }

                return nullptr;
            case kind::global:
                return dynamic_cast<const ast::source_unit_declaration*>(scope->enclosing_); 
            case kind::loop:
                while (scope && !dynamic_cast<const ast::workspace*>(scope->enclosing_)) {
                    if (dynamic_cast<const ast::function_declaration*>(scope->enclosing_)) return nullptr;
                    else if (dynamic_cast<const ast::property_declaration*>(scope->enclosing_)) return nullptr;
                    else if (dynamic_cast<const ast::function_expression*>(scope->enclosing_)) return nullptr;
                    else if (dynamic_cast<const ast::for_loop_expression*>(scope->enclosing_)) return scope->enclosing_;
                    else if (dynamic_cast<const ast::for_range_expression*>(scope->enclosing_)) return scope->enclosing_;
                    else if (dynamic_cast<const ast::expression*>(scope->enclosing_)) scope = scope->parent_;
                    else if (dynamic_cast<const ast::source_unit_declaration*>(scope->enclosing_)) return nullptr;
                    else if (dynamic_cast<const ast::test_declaration*>(scope->enclosing_)) return nullptr;
                    else if (dynamic_cast<const ast::declaration*>(scope->enclosing_)) return nullptr;
                }

                return nullptr;
            case kind::test:
                while (scope && !dynamic_cast<const ast::workspace*>(scope->enclosing_)) {
                    if (dynamic_cast<const ast::expression*>(scope->enclosing_)) scope = scope->parent_;
                    else if (dynamic_cast<const ast::source_unit_declaration*>(scope->enclosing_)) return nullptr;
                    else if (dynamic_cast<const ast::test_declaration*>(scope->enclosing_)) return scope->enclosing_;
                    else if (dynamic_cast<const ast::declaration*>(scope->enclosing_)) return nullptr;
                }

                return nullptr;
            case kind::workspace:
                for (; scope && !dynamic_cast<const ast::workspace*>(scope->enclosing_); scope = scope->parent_);
                return scope ? scope->enclosing_ : nullptr;
            default:
                break;
        }

        return nullptr;
    }

    bool environment::has_ancestor_scope(environment* candidate) const
    {
        if (!parent_) return false;
        if (parent_ == candidate) return true;
        return parent_->has_ancestor_scope(candidate);
    }

    std::string environment::canonical(std::string type) const
    {
        std::string name = type;
        const environment* env = this;
        // finds its scope
        for (; !env && env->types_.find(name) == env->types_.end(); env = env->parent_);
        // until it reaches global scope it appends the declaration block name in front
        for (; !env; env = env->parent_) {
            if (auto unit = dynamic_cast<const ast::source_unit_declaration*>(env->enclosing_)) {
                if (auto workspace = dynamic_cast<const ast::workspace_declaration*>(unit->workspace().get())) {
                    name.insert(0, workspace->path().lexeme().string());
                }
            }
        }

        return name;
    }

    checker::scope::scope(checker* instance, const ast::node* enclosing) : instance_(instance)
    {
        instance_->begin_scope(enclosing);
    }

    checker::scope::~scope() { instance_->end_scope(); }
        
    checker::~checker() 
    {
        for (auto pair : scopes_) {
            if (pair.second) delete pair.second;
        }
    }
        
    environment* checker::begin_scope(const ast::node* enclosing)
    {
        auto result = scopes_.find(enclosing);

        if (result == scopes_.end()) {
            scope_ = new environment(enclosing, scope_);
            scopes_.emplace(enclosing, scope_);
        }
        else {
            scope_ = result->second;
        }

        return scope_;
    }

    void checker::end_scope()
    {
        if (!scope_) throw std::logic_error("checker: end_scope(): environment cannot be null on scope ending");
        scope_ = scope_->parent();
    }

    void checker::add_to_scope(environment* scope, ast::pointer<ast::declaration> decl, const ast::statement* after, bool is_after) const
    {
        while (scope) {
            switch (scope->enclosing()->kind()) {
                case ast::kind::source_unit_declaration:
                case ast::kind::workspace:
                {
                    workspace()->globals.push_back(decl.get());
                    auto source_unit = std::static_pointer_cast<ast::source_unit_declaration>(file_->ast());
                    auto pos = std::find_if(source_unit->statements().begin(), source_unit->statements().end(), [&] (ast::pointer<ast::statement> stmt) { return stmt.get() == after; });
                    if (is_after) source_unit->statements().insert(++pos, decl);
                    else source_unit->statements().insert(pos, decl);
                    return;
                }
                case ast::kind::block_expression:
                {
                    auto block = static_cast<const ast::block_expression*>(scope->enclosing());
                    auto pos = std::find_if(block->statements().begin(), block->statements().end(), [&] (ast::pointer<ast::statement> stmt) { return stmt.get() == after; });
                    if (is_after) block->statements().insert(++pos, decl);
                    else block->statements().insert(pos, decl);
                    return;
                }
                default:
                    scope = scope->parent();
            }
        }

        throw std::invalid_argument("checker::add_to_scope(): invalid type of scope for adding declaration");
    }

    ast::workspace* checker::workspace() const
    {
        for (auto scope = scope_; scope; scope = scope->parent()) {
            if (auto n = dynamic_cast<const ast::workspace*>(scope->enclosing())) {
                return const_cast<ast::workspace*>(n);
            }
        }

        return nullptr;
    }

    size_t checker::path_contains_subpath(const std::string& path, const std::string& subpath)
    {
        if (subpath.size() > path.size() || path.size() == 0) return 0;

        auto i1 = path.begin(), i2 = subpath.begin();
        size_t identifiers = 1;

        for (; i2 < subpath.end(); ++i1, ++i2) {
            if (*i1 != *i2) return 0;
            else if (*i1 == '.') ++identifiers;
        }

        if (i1 != path.end() && *i1 != '.') return 0;

        return identifiers;
    }

    ast::pointer<ast::type> checker::instantiate_type(const ast::type_declaration& tdecl, substitutions subs)
    {
        static std::stack<const ast::type_declaration*> level;
        constexpr unsigned max_depth = 10;

        // generation of generic type name with substitutions
        std::ostringstream oss;
        auto builder = token::builder();
        auto context = subs.context();

        oss << tdecl.name().lexeme();

        if (auto clause = std::dynamic_pointer_cast<ast::generic_clause_declaration>(tdecl.generic())) {
            oss << "(";
            for (auto param : clause->parameters()) {
                if (auto type = std::dynamic_pointer_cast<ast::generic_type_parameter_declaration>(param)) {
                    if (param != clause->parameters().front()) oss << ", " << subs.type(type.get())->second->string();
                    else {
                        oss << subs.type(type.get())->second->string();
                    }
                }
                else if (auto constant = std::dynamic_pointer_cast<ast::generic_const_parameter_declaration>(param)) {
                    if (param != clause->parameters().front()) oss << ", " << subs.constant(constant.get())->second;
                    else oss << subs.constant(constant.get())->second;
                }
            }
            oss << ")";
        }
        
        auto tname = oss.str();
        auto workspace = this->workspace();
        // first thing we check if generic type has already been instatiated inside the current workspace
        auto found = workspace->instantiated.find(tname);
        // we must allocate a new declaration tree for instantiated generic type
        if (found == workspace->instantiated.end()) {
            // push the new type declaration to instantiate on the depth stack
            level.push(&tdecl);
            // if stack depth exceed the thresholds, then the checking is aborted
            if (level.size() > max_depth) {
                auto builder = diagnostic::builder()
                        .severity(diagnostic::severity::error)
                        .message("Maximum depth reached when instantiating generic type. This is weird...")
                        .explanation("I need to abort when such an error occurs.");
                // all declarations on the stack are removed and printed
                for (; level.size() > 1; level.pop()) builder.highlight(level.top()->name().range());

                builder.highlight(level.top()->name().range());
                builder.location(level.top()->range().begin());
                level.pop();
                // error is printed to the programmer
                publisher().publish(builder.build());
                // abort semantic analysis
                throw abort_error();
            }

            auto map = subs.map();
            auto clone = std::static_pointer_cast<ast::type_declaration>(tdecl.clone());
            subs.root(clone.get());
            subs.substitute();
            clone->name() = token(token::kind::identifier, utf8::span::builder().concat(tname.data(), tname.size()).build(), tdecl.name().location());
            clone->annotation().scope = tdecl.annotation().scope;

            std::unordered_map<std::string, types::parameter> arguments;

            for (auto value : subs.constants()) {
                auto cdecl = static_cast<const ast::generic_const_parameter_declaration*>(value.first);
                types::parameter constarg;
                constarg.kind = types::parameter::kind::value;
                constarg.value = value.second;
                constarg.type = cdecl->annotation().type;
                arguments.emplace(cdecl->name().lexeme().string(), constarg);
            }

            for (auto type : subs.types()) {
                auto tdecl = static_cast<const ast::generic_type_parameter_declaration*>(type.first);
                types::parameter typearg;
                typearg.kind = types::parameter::kind::type;
                typearg.type = type.second;
                arguments.emplace(tdecl->name().lexeme().string(), typearg);
            }

            // analysis is performed on concept constraints, if any
            if (auto generic = std::dynamic_pointer_cast<ast::generic_clause_declaration>(clone->generic())) {
                if (generic->constraint()) {
                    generic->constraint()->accept(*this);
                    // we check if instantiation is possible due to the outcome of concept test
                    if (!generic->constraint()->annotation().value.b) return nullptr;
                }
            }

            clone->generic(nullptr);

            auto saved = scope_;
            
            scope_ = scopes_.at(clone->annotation().scope);
            auto source_workspace = this->workspace();

            // add instantiated type
            workspace->instantiated.emplace(tname, clone);
            source_workspace->instantiated.emplace(tname, clone);

            try { clone->accept(*this); } catch (abort_error&) { throw; }

            scope_ = saved;
            
            // instantiation declaration is both added to source workspace and to instantiantion workspace
            if (std::find(workspace->types.begin(), workspace->types.end(), clone->annotation().type) == workspace->types.end()) workspace->types.push_back(clone->annotation().type);
            if (std::find(source_workspace->types.begin(), source_workspace->types.end(), clone->annotation().type) == source_workspace->types.end()) source_workspace->types.push_back(clone->annotation().type);
            
            if (scopes_.count(&tdecl)) {
                ast::pointers<ast::declaration> declarations;

                subs.context(context);

                for (auto constant : scopes_.at(&tdecl)->values()) {
                    auto cloned = constant.second->clone();
                    cloned->annotation().resolved = cloned->annotation().visited = false;
                    cloned->annotation().scope = clone.get();
                    subs.root(cloned.get());
                    subs.substitute();
                    declarations.push_back(cloned);
                }

                subs.context(context);

                for (auto type : scopes_.at(&tdecl)->types()) {
                    auto cloned = type.second->clone();
                    cloned->annotation().resolved = cloned->annotation().visited = false;
                    cloned->annotation().scope = clone.get();
                    if (type.second->generic() && scopes_.count(type.second->generic().get())) subs.context(scopes_.at(type.second->generic().get()));
                    else subs.context(context);
                    subs.root(cloned.get());
                    subs.substitute();
                    declarations.push_back(cloned);
                }

                subs.context(context);

                for (auto function : scopes_.at(&tdecl)->functions()) {
                    auto cloned = function.second->clone();
                    cloned->annotation().resolved = cloned->annotation().visited = false;
                    cloned->annotation().scope = clone.get();
                    subs.root(cloned.get());
                    subs.substitute();
                    declarations.push_back(cloned);
                }

                auto name = ast::create<ast::identifier_expression>(clone->name().range(), clone->name(), ast::pointers<ast::expression>());
                name->annotation().type = clone->annotation().type;
                name->annotation().istype = true;
                ast::pointer<ast::declaration> extend = ast::create<ast::extend_declaration>(source_range(), ast::pointer<ast::declaration>(), ast::create<ast::path_type_expression>(clone->name().range(), name, nullptr), ast::pointers<ast::expression>(), declarations);

                scope_ = scopes_.at(clone->annotation().scope);

                auto pass = pass_;

                pass_ = pass::second;

                try { extend->accept(*this); } catch (abort_error&) { throw; } catch (...) { scope_ = saved; }

                pass_ = pass::third;

                try { extend->accept(*this); } catch (abort_error&) { throw; } catch (...) { scope_ = saved; }

                pass_ = pass;

                scope_ = saved;

                workspace->textensions.push_back(extend);
            }

            types::parametrized(clone->annotation().type, tdecl.annotation().type, arguments);

            clone->annotation().type = clone->annotation().type->substitute(clone->annotation().type, map);
            // pops declaration from instantiation stack
            level.pop();
            
            return clone->annotation().type;
        }
        else {
            return found->second->annotation().type;
        }

        return nullptr;
    }

    bool checker::instantiate_concept(const ast::concept_declaration& cdecl, substitutions subs)
    {
        static std::stack<const ast::concept_declaration*> level;
        constexpr unsigned max_depth = 10;

        // generation of generic type name with substitutions
        std::ostringstream oss;
        auto builder = token::builder();

        oss << cdecl.name().lexeme();

        if (auto clause = std::dynamic_pointer_cast<ast::generic_clause_declaration>(cdecl.generic())) {
            oss << "(";
            for (auto param : clause->parameters()) {
                if (auto type = std::dynamic_pointer_cast<ast::generic_type_parameter_declaration>(param)) {
                    if (param != clause->parameters().front()) oss << ", " << subs.type(type.get())->second->string();
                    else {
                        oss << subs.type(type.get())->second->string();
                    }
                }
                else if (auto constant = std::dynamic_pointer_cast<ast::generic_const_parameter_declaration>(param)) {
                    if (param != clause->parameters().front()) oss << ", " << subs.constant(constant.get())->second;
                    else oss << subs.constant(constant.get())->second;
                }
            }
            oss << ")";
        }
        
        auto cname = oss.str();
        auto workspace = this->workspace();

        if (workspace->tested_concept.count(cname)) return workspace->tested_concept.at(cname);

        // we must allocate a new declaration tree for instantiated generic concept
        // push the new concept declaration to instantiate on the depth stack
        level.push(&cdecl);
        // if stack depth exceed the thresholds, then the checking is aborted
        if (level.size() > max_depth) {
            auto builder = diagnostic::builder()
                    .severity(diagnostic::severity::error)
                    .message("Maximum depth reached when instantiating generic concept. This is weird...")
                    .explanation("I need to abort when such an error occurs.");
            // all declarations on the stack are removed and printed
            for (; level.size() > 1; level.pop()) builder.highlight(level.top()->name().range());

            builder.highlight(level.top()->name().range());
            builder.location(level.top()->range().begin());
            level.pop();
            // error is printed to the programmer
            publisher().publish(builder.build());
            // abort semantic analysis
            throw abort_error();
        }

        auto map = subs.map();
        auto clone = std::static_pointer_cast<ast::concept_declaration>(cdecl.clone());
        subs.root(clone.get());
        subs.substitute();
        clone->generic() = nullptr;
        clone->annotation().scope = cdecl.annotation().scope;

        std::unordered_map<std::string, types::parameter> arguments;

        for (auto value : subs.constants()) {
            auto cdecl = static_cast<const ast::generic_const_parameter_declaration*>(value.first);
            types::parameter constarg;
            constarg.kind = types::parameter::kind::value;
            constarg.value = value.second;
            constarg.type = cdecl->annotation().type;
            arguments.emplace(cdecl->name().lexeme().string(), constarg);
        }

        for (auto type : subs.types()) {
            auto tdecl = static_cast<const ast::generic_type_parameter_declaration*>(type.first);
            types::parameter typearg;
            typearg.kind = types::parameter::kind::type;
            typearg.type = type.second;
            arguments.emplace(tdecl->name().lexeme().string(), typearg);
        }

        auto saved = scope_;
        
        scope_ = scopes_.at(clone->annotation().scope);

        try { clone->accept(*this); } catch (abort_error&) { throw; }

        scope_ = saved;

        // pops declaration from instantiation stack
        level.pop();
        // count of matched prototypes
        unsigned matches = 0;
        // test that concept is satisfied for current types
        for (auto prototype : clone->prototypes()) {
            // substitution of parameters inside type
            prototype->annotation().type = prototype->annotation().type->substitute(prototype->annotation().type, map);
            // looking for prototype
            if (auto function = std::dynamic_pointer_cast<ast::function_declaration>(prototype)) {
                // for each type to which concept test is applied we look for a function with the expected prototype
                for (auto arg : subs.types()) {
                    auto type = arg.second;
                    if (!type->declaration() || scopes_.count(type->declaration()) == 0) continue;
                    for (auto test : scopes_.at(type->declaration())->functions()) {
                        if (test.second->kind() != ast::kind::function_declaration || test.first != function->name().lexeme().string()) continue;
                        auto testfunction = static_cast<const ast::function_declaration*>(test.second);
                        if (testfunction->generic() || testfunction->parameters().size() != function->parameters().size()) continue;
                        // test for type mismatch
                        if (!types::compatible(testfunction->annotation().type, function->annotation().type)) continue;
                        // now we have match for this function
                        ++matches;
                    }
                }
            }
            else if (auto property = std::dynamic_pointer_cast<ast::property_declaration>(prototype)) {
                // for each type to which concept test is applied we look for a property with the expected prototype
                for (auto arg : subs.types()) {
                    auto type = arg.second;
                    if (!type->declaration() || scopes_.count(type->declaration()) == 0) continue;
                    for (auto test : scopes_.at(type->declaration())->functions()) {
                        if (test.second->kind() != ast::kind::property_declaration || test.first != property->name().lexeme().string()) continue;
                        auto testproperty = static_cast<const ast::property_declaration*>(test.second);
                        if (testproperty->parameters().size() != property->parameters().size()) continue;
                        // test for type mismatch
                        if (!types::compatible(testproperty->annotation().type, property->annotation().type)) continue;
                        // now we have match for this function
                        ++matches;
                    }
                }
            }
        }
        // test if we have all matches
        //if (matches < clone->prototypes().size()) std::cout << "no match for this prototype (" << matches << "/" << clone->prototypes().size() << ")\n";
        //else std::cout << "focking match for this prototype (" << matches << "/" << clone->prototypes().size() << ")\n";

        workspace->tested_concept.emplace(cname, matches == clone->prototypes().size());

        return matches == clone->prototypes().size();
    }

    ast::pointer<ast::function_declaration> checker::instantiate_function(const ast::function_declaration& fdecl, substitutions subs)
    {
        // generation of generic type name with substitutions
        std::ostringstream oss;
        auto builder = token::builder();

        if (fdecl.annotation().scope) {
            if (auto parent = dynamic_cast<const ast::type_declaration*>(fdecl.annotation().scope)) oss << parent->annotation().type->string() << ".";
        }

        oss << fdecl.name().lexeme();

        if (auto clause = std::dynamic_pointer_cast<ast::generic_clause_declaration>(fdecl.generic())) {
            oss << "(";
            for (auto param : clause->parameters()) {
                if (auto type = std::dynamic_pointer_cast<ast::generic_type_parameter_declaration>(param)) {
                    if (param != clause->parameters().front()) oss << ", " << subs.type(type.get())->second->string();
                    else {
                        oss << subs.type(type.get())->second->string();
                    }
                }
                else if (auto constant = std::dynamic_pointer_cast<ast::generic_const_parameter_declaration>(param)) {
                    if (param != clause->parameters().front()) oss << ", " << subs.constant(constant.get())->second;
                    else oss << subs.constant(constant.get())->second;
                }
            }
            oss << ")";
        }
        
        auto fname = oss.str();
        auto workspace = this->workspace();
        // first thing we check if generic type has already been instatiated inside the current workspace
        auto found = workspace->instantiated_functions.find(fname);
        // we must allocate a new declaration tree for instantiated generic type
        if (found == workspace->instantiated_functions.end()) {
            auto clone = std::static_pointer_cast<ast::function_declaration>(fdecl.clone());
            subs.root(clone.get());
            subs.context(scopes_.at(&fdecl));
            subs.substitute();
            clone->name() = token(token::kind::identifier, utf8::span::builder().concat(fname.data(), fname.size()).build(), fdecl.name().location());
            clone->annotation().scope = fdecl.annotation().scope;

            auto map = subs.map();

            if (auto tspec = std::dynamic_pointer_cast<ast::type_expression>(clone->return_type_expression())) tspec->clear();
            for (auto param : clone->parameters()) if (auto tspec = std::dynamic_pointer_cast<ast::type_expression>(std::static_pointer_cast<ast::parameter_declaration>(param)->type_expression())) tspec->clear();

            std::unordered_map<std::string, types::parameter> arguments;

            for (auto value : subs.constants()) {
                auto cdecl = static_cast<const ast::generic_const_parameter_declaration*>(value.first);
                types::parameter constarg;
                constarg.kind = types::parameter::kind::value;
                constarg.value = value.second;
                constarg.type = cdecl->annotation().type;
                arguments.emplace(cdecl->name().lexeme().string(), constarg);
            }

            for (auto type : subs.types()) {
                auto tdecl = static_cast<const ast::generic_type_parameter_declaration*>(type.first);
                types::parameter typearg;
                typearg.kind = types::parameter::kind::type;
                typearg.type = type.second;
                arguments.emplace(tdecl->name().lexeme().string(), typearg);
            }

            // analysis is performed on concept constraints, if any
            if (auto generic = std::dynamic_pointer_cast<ast::generic_clause_declaration>(clone->generic())) {
                if (generic->constraint()) {
                    generic->constraint()->accept(*this);
                    // we check if instantiation is possible due to the outcome of concept test
                    if (!generic->constraint()->annotation().value.b) return nullptr;
                }
            }

            clone->generic() = nullptr;
            
            // instantiated function is visited for analysis
            auto saved = scope_;
            
            scope_ = scopes_.at(clone->annotation().scope);
            
            try { clone->accept(*this); } catch (abort_error&) { throw; }

            clone->annotation().type = clone->annotation().type->substitute(clone->annotation().type, map);

            scope_ = saved;

            // adds instantiated function
            workspace->instantiated_functions.emplace(fname, clone);

            return clone;
        }
        else {
            return found->second;
        }

        return nullptr;
    }

    ast::pointer<ast::expression> checker::implicit_cast(ast::pointer<ast::type> type, ast::pointer<ast::expression> expression)
    {
        // assumes types are compatible
        if (type->category() == expression->annotation().type->category() && (type->category() == ast::type::category::integer_type || type->category() == ast::type::category::rational_type || type->category() == ast::type::category::float_type || type->category() == ast::type::category::complex_type)) {
            // implicit cast between numeric types of different sizes or signedness
            if (!types::compatible(type, expression->annotation().type, true)) {
                ast::pointer<ast::expression> result = ast::create<ast::implicit_conversion_expression>(expression->range(), expression);
                result->annotation().type = type;
                return result;
            }
            else return nullptr;
        }
        // implicit upcasting/downcasting
        else if (type->category() == ast::type::category::pointer_type && expression->annotation().type->category() == ast::type::category::pointer_type && types::compatible(type, expression->annotation().type)) {
            auto resbase = std::static_pointer_cast<ast::pointer_type>(type)->base(), origbase = std::static_pointer_cast<ast::pointer_type>(expression->annotation().type)->base();
            // upcast
            if (auto behaviour = std::dynamic_pointer_cast<ast::behaviour_type>(resbase)) {
                if (behaviour->implementor(origbase)) {
                    ast::pointer<ast::expression> result = ast::create<ast::implicit_conversion_expression>(expression->range(), expression);
                    result->annotation().type = type;
                    return result;
                }
                else return nullptr;
            }
            // downcast
            else if (auto behaviour = std::dynamic_pointer_cast<ast::behaviour_type>(origbase)) {
                if (behaviour->implementor(resbase)) {
                    ast::pointer<ast::expression> result = ast::create<ast::implicit_conversion_expression>(expression->range(), expression);
                    result->annotation().type = type;
                    return result;
                }
                else return nullptr;
            }
            else if (!types::compatible(type, expression->annotation().type, true)) {
                ast::pointer<ast::expression> result = ast::create<ast::implicit_conversion_expression>(expression->range(), expression);
                result->annotation().type = type;
                return result;
            }

            return nullptr;
        }
        // implicit address of
        else if (type->category() == ast::type::category::pointer_type && types::compatible(std::static_pointer_cast<ast::pointer_type>(type)->base(), expression->annotation().type, false)) {
            ast::pointer<ast::expression> result = ast::create<ast::unary_expression>(expression->range(), token(token::kind::amp, utf8::span::builder().concat("&").build(), source_location()), expression);
            result->annotation().type = type;
            result = ast::create<ast::parenthesis_expression>(expression->range(), result);
            result->annotation().type = type;
            return result;
        }
        // implicit dereference
        else if (expression->annotation().type->category() == ast::type::category::pointer_type && types::compatible(type, std::static_pointer_cast<ast::pointer_type>(expression->annotation().type)->base(), false)) {
            ast::pointer<ast::expression> result = ast::create<ast::unary_expression>(expression->range(), token(token::kind::star, utf8::span::builder().concat("*").build(), source_location()), expression);
            result->annotation().type = type;
            result = ast::create<ast::parenthesis_expression>(expression->range(), result);
            result->annotation().type = type;
            return result;
        }
        // implicit cast
        else if (!types::compatible(type, expression->annotation().type, true)) {
            ast::pointer<ast::expression> result = ast::create<ast::implicit_conversion_expression>(expression->range(), expression);
            result->annotation().type = type;
            return result;
        }

        return nullptr;
    }

    ast::pointer<ast::expression> checker::implicit_forced_cast(ast::pointer<ast::type> type, ast::pointer<ast::expression> expression)
    {
        // if strict cast is needed
        if (!types::compatible(type, expression->annotation().type)) {
            auto result = ast::create<ast::implicit_conversion_expression>(expression->range(), expression);
            result->annotation().type = type;
            return result;
        }
        // no cast needed
        return expression;
    }

    void checker::test_immutable_assignment(const ast::var_declaration& decl, const ast::expression& value) const
    {
        if (!types::assignment_compatible(decl.annotation().type, value.annotation().type) || !decl.is_mutable()) return;

        if (decl.annotation().type->category() == ast::type::category::pointer_type) {
            if (auto immutable = value.immutable()) {
                auto builder = diagnostic::builder()
                                .small(true)
                                .severity(diagnostic::severity::error)
                                .location(value.range().begin())
                                .message("You can't assign an immutable value to a mutable pointer, sh*thead! \\ This way one could violate mutability, don't you think?")
                                .highlight(decl.name().range(), diagnostic::highlighter::mode::light)
                                .highlight(value.range());

                if (auto vardecl = dynamic_cast<const ast::var_declaration*>(immutable)) {
                    builder.note(vardecl->name().range(), diagnostic::format("Look at variable `$` declaration.", vardecl->name().lexeme()));
                    if (!vardecl->value()) builder.insertion(source_range(vardecl->name().location(), 1), "mutable ", "This is what you need to make the variable mutable.");
                    else builder.insertion(source_range(vardecl->range().begin(), 1), "mutable ", "This is what you need to make the variable mutable.");
                }
                else if (auto constdecl = dynamic_cast<const ast::generic_const_parameter_declaration*>(immutable)) {
                    builder.note(constdecl->name().range(), diagnostic::format("Look at generic constant `$` declaration.", constdecl->name().lexeme()));
                }
                else if (auto constdecl = dynamic_cast<const ast::const_declaration*>(immutable)) {
                    builder.note(constdecl->name().range(), diagnostic::format("Look at constant `$` declaration.", constdecl->name().lexeme()));
                }
                else if (auto fndecl = dynamic_cast<const ast::function_declaration*>(immutable)) {
                    builder.note(fndecl->name().range(), diagnostic::format("Look at function `$` declaration.", fndecl->name().lexeme()));
                }
                else if (auto fndecl = dynamic_cast<const ast::property_declaration*>(immutable)) {
                    builder.note(fndecl->name().range(), diagnostic::format("Look at property `$` declaration.", fndecl->name().lexeme()));
                }
                
                publisher().publish(builder.build());
                throw semantic_error();
            }
        }
    }

    void checker::test_immutable_assignment(ast::pointer<ast::type> lvalue, const ast::expression& rvalue) const
    {
        if (!types::assignment_compatible(lvalue, rvalue.annotation().type) || !lvalue->mutability) return;
        
        if (lvalue->category() == ast::type::category::pointer_type) {
            if (auto immutable = rvalue.immutable()) {
                auto builder = diagnostic::builder()
                                .small(true)
                                .severity(diagnostic::severity::error)
                                .location(rvalue.range().begin())
                                .message(diagnostic::format("You can't assign an immutable value to a mutable pointer `$`, sh*thead! \\ This way one could violate mutability, don't you think?", lvalue->string()))
                                .highlight(rvalue.range(), diagnostic::format("expected mutable $", lvalue->string()));

                if (auto vardecl = dynamic_cast<const ast::var_declaration*>(immutable)) {
                    builder.note(vardecl->name().range(), diagnostic::format("Look at variable `$` declaration.", vardecl->name().lexeme()));
                    if (!vardecl->value()) builder.insertion(source_range(vardecl->name().location(), 1), "mutable ", "This is what you need to make the variable mutable.");
                    else builder.insertion(source_range(vardecl->range().begin(), 1), "mutable ", "This is what you need to make the variable mutable.");
                }
                else if (auto constdecl = dynamic_cast<const ast::generic_const_parameter_declaration*>(immutable)) {
                    builder.note(constdecl->name().range(), diagnostic::format("Look at generic constant `$` declaration.", constdecl->name().lexeme()));
                }
                else if (auto constdecl = dynamic_cast<const ast::const_declaration*>(immutable)) {
                    builder.note(constdecl->name().range(), diagnostic::format("Look at constant `$` declaration.", constdecl->name().lexeme()));
                }
                else if (auto fndecl = dynamic_cast<const ast::function_declaration*>(immutable)) {
                    builder.note(fndecl->name().range(), diagnostic::format("Look at function `$` declaration.", fndecl->name().lexeme()));
                }
                else if (auto fndecl = dynamic_cast<const ast::property_declaration*>(immutable)) {
                    builder.note(fndecl->name().range(), diagnostic::format("Look at property `$` declaration.", fndecl->name().lexeme()));
                }
                
                publisher().publish(builder.build());
                throw semantic_error();
            }
        }
    }

    void checker::add_type(ast::pointer<ast::type> type)
    {
        auto workspace = this->workspace();
        // if anonymous types was not found, it is added to anonymous types list
        if (std::count_if(workspace->types.begin(), workspace->types.end(), [&] (ast::pointer<ast::type> x) { return types::compatible(x, type); }) == 0) {
            workspace->types.push_back(type);
        }
    }

    ast::pointer<ast::var_declaration> checker::create_temporary_var(const ast::expression& value) const
    {
        auto binding = ast::create<ast::var_declaration>(value.range(), std::vector<token>(), token::builder().artificial(true).kind(token::kind::identifier).lexeme(utf8::span::builder().concat(("__temp" + std::to_string(rand())).data()).build()).build(), nullptr, value.clone());
        binding->annotation().type = value.annotation().type;
        return binding;
    }

    void checker::mismatch(source_range x, source_range y, const std::string& message, const std::string& explanation, const std::string& inlined)
    {
        auto diag = diagnostic::builder()
                    .location(y.begin())
                    .severity(diagnostic::severity::error)
                    .highlight(x, diagnostic::highlighter::mode::light)
                    .highlight(y)
                    .message(message)
                    .explanation(explanation)
                    .build();
        
        publisher().publish(diag);
    }

    void checker::error(const ast::unary_expression& expr, const std::string& message, const std::string& explanation, const std::string& inlined)
    {
        auto diag = diagnostic::builder()
                    .location(expr.unary_operator().location())
                    .severity(diagnostic::severity::error)
                    .highlight(expr.unary_operator().range())
                    .highlight(expr.expression()->range(), inlined, diagnostic::highlighter::mode::light)
                    .message(message)
                    .explanation(explanation)
                    .build();
        
        publisher().publish(diag);
        throw semantic_error();
    }
    
    void checker::error(const ast::binary_expression& expr, const std::string& message, const std::string& explanation, const std::string& inlined)
    {
        auto diag = diagnostic::builder()
                    .location(expr.binary_operator().location())
                    .severity(diagnostic::severity::error)
                    .highlight(expr.binary_operator().range())
                    .highlight(expr.left()->range(), diagnostic::highlighter::mode::light)
                    .highlight(expr.right()->range(), inlined, diagnostic::highlighter::mode::light)
                    .message(message)
                    .explanation(explanation)
                    .build();
        
        publisher().publish(diag);
        throw semantic_error();
    }

    void checker::error(const ast::assignment_statement& stmt, const std::string& message, const std::string& explanation, const std::string& inlined)
    {
        auto diag = diagnostic::builder()
                    .location(stmt.assignment_operator().location())
                    .severity(diagnostic::severity::error)
                    .highlight(stmt.assignment_operator().range())
                    .highlight(stmt.left()->range(), diagnostic::highlighter::mode::light)
                    .highlight(stmt.right()->range(), inlined, diagnostic::highlighter::mode::light)
                    .message(message)
                    .explanation(explanation)
                    .build();
        
        publisher().publish(diag);
        throw semantic_error();
    }

    void checker::error(source_range highlight, const std::string& message, const std::string& explanation, const std::string& inlined)
    {
        auto diag = diagnostic::builder()
                    .location(highlight.begin())
                    .severity(diagnostic::severity::error)
                    .highlight(highlight, inlined)
                    .message(message)
                    .explanation(explanation)
                    .build();
        
        publisher().publish(diag);
        throw semantic_error();
    }

    void checker::report(const ast::unary_expression& expr, const std::string& message, const std::string& explanation, const std::string& inlined)
    {
        auto diag = diagnostic::builder()
                    .location(expr.unary_operator().location())
                    .severity(diagnostic::severity::error)
                    .highlight(expr.unary_operator().range())
                    .highlight(expr.expression()->range(), inlined, diagnostic::highlighter::mode::light)
                    .message(message)
                    .explanation(explanation)
                    .build();
        
        publisher().publish(diag);
    }
    
    void checker::report(const ast::binary_expression& expr, const std::string& message, const std::string& explanation, const std::string& inlined)
    {
        auto diag = diagnostic::builder()
                    .location(expr.binary_operator().location())
                    .severity(diagnostic::severity::error)
                    .highlight(expr.binary_operator().range())
                    .highlight(expr.left()->range(), diagnostic::highlighter::mode::light)
                    .highlight(expr.right()->range(), inlined, diagnostic::highlighter::mode::light)
                    .message(message)
                    .explanation(explanation)
                    .build();
        
        publisher().publish(diag);
    }

    void checker::report(source_range highlight, const std::string& message, const std::string& explanation, const std::string& inlined)
    {
        auto diag = diagnostic::builder()
                    .location(highlight.begin())
                    .severity(diagnostic::severity::error)
                    .highlight(highlight, inlined)
                    .message(message)
                    .explanation(explanation)
                    .build();
        
        publisher().publish(diag);
    }

    void checker::warning(const ast::unary_expression& expr, const std::string& message, const std::string& explanation, const std::string& inlined)
    {
        auto diag = diagnostic::builder()
                    .location(expr.unary_operator().location())
                    .severity(diagnostic::severity::warning)
                    .highlight(expr.unary_operator().range())
                    .highlight(expr.expression()->range(), inlined, diagnostic::highlighter::mode::light)
                    .message(message)
                    .explanation(explanation)
                    .build();
        
        publisher().publish(diag);
    }
    
    void checker::warning(const ast::binary_expression& expr, const std::string& message, const std::string& explanation, const std::string& inlined)
    {
        auto diag = diagnostic::builder()
                    .location(expr.binary_operator().location())
                    .severity(diagnostic::severity::warning)
                    .highlight(expr.binary_operator().range())
                    .highlight(expr.left()->range(), diagnostic::highlighter::mode::light)
                    .highlight(expr.right()->range(), inlined, diagnostic::highlighter::mode::light)
                    .message(message)
                    .explanation(explanation)
                    .build();
        
        publisher().publish(diag);
    }

    void checker::warning(source_range highlight, const std::string& message, const std::string& explanation, const std::string& inlined)
    {
        auto diag = diagnostic::builder()
                    .location(highlight.begin())
                    .severity(diagnostic::severity::warning)
                    .highlight(highlight, inlined)
                    .message(message)
                    .explanation(explanation)
                    .build();
        
        publisher().publish(diag);
    }

    void checker::immutability(const ast::postfix_expression& expr)
    {
        if (auto immutable = expr.expression()->immutable()) {
            auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(expr.postfix().location())
                            .message("You can't modify an immutable value, pr*ck!")
                            .highlight(expr.postfix().range())
                            .highlight(expr.expression()->range(), diagnostic::highlighter::mode::light);
            
            if (auto vardecl = dynamic_cast<const ast::var_declaration*>(immutable)) {
                builder.note(vardecl->name().range(), diagnostic::format("Look at variable `$` declaration.", vardecl->name().lexeme()));
                if (!vardecl->value()) builder.insertion(source_range(vardecl->name().location(), 1), "mutable ", "This is what you need to make the variable mutable.");
                else builder.insertion(source_range(vardecl->range().begin(), 1), "mutable ", "This is what you need to make the variable mutable.");
            }
            else if (auto constdecl = dynamic_cast<const ast::generic_const_parameter_declaration*>(immutable)) {
                builder.note(constdecl->name().range(), diagnostic::format("Look at generic constant `$` declaration.", constdecl->name().lexeme()));
            }
            else if (auto constdecl = dynamic_cast<const ast::const_declaration*>(immutable)) {
                builder.note(constdecl->name().range(), diagnostic::format("Look at constant `$` declaration.", constdecl->name().lexeme()));
            }
            else if (auto fndecl = dynamic_cast<const ast::function_declaration*>(immutable)) {
                builder.note(fndecl->name().range(), diagnostic::format("Look at function `$` declaration.", fndecl->name().lexeme()));
            }
            else if (auto fndecl = dynamic_cast<const ast::property_declaration*>(immutable)) {
                builder.note(fndecl->name().range(), diagnostic::format("Look at property `$` declaration.", fndecl->name().lexeme()));
            }
            
            publisher().publish(builder.build());
            expr.invalid(true);
        }
    }

    void checker::immutability(const ast::unary_expression& expr)
    {
        if (auto immutable = expr.expression()->immutable()) {
            auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(expr.unary_operator().location())
                            .message("You can't modify an immutable value, pr*ck!")
                            .highlight(expr.unary_operator().range())
                            .highlight(expr.expression()->range(), diagnostic::highlighter::mode::light);
            
            if (auto vardecl = dynamic_cast<const ast::var_declaration*>(immutable)) {
                builder.note(vardecl->name().range(), diagnostic::format("Look at variable `$` declaration.", vardecl->name().lexeme()));
                if (!vardecl->value()) builder.insertion(source_range(vardecl->name().location(), 1), "mutable ", "This is what you need to make the variable mutable.");
                else builder.insertion(source_range(vardecl->range().begin(), 1), "mutable ", "This is what you need to make the variable mutable.");
            }
            else if (auto constdecl = dynamic_cast<const ast::generic_const_parameter_declaration*>(immutable)) {
                builder.note(constdecl->name().range(), diagnostic::format("Look at generic constant `$` declaration.", constdecl->name().lexeme()));
            }
            else if (auto constdecl = dynamic_cast<const ast::const_declaration*>(immutable)) {
                builder.note(constdecl->name().range(), diagnostic::format("Look at constant `$` declaration.", constdecl->name().lexeme()));
            }
            else if (auto fndecl = dynamic_cast<const ast::function_declaration*>(immutable)) {
                builder.note(fndecl->name().range(), diagnostic::format("Look at function `$` declaration.", fndecl->name().lexeme()));
            }
            else if (auto fndecl = dynamic_cast<const ast::property_declaration*>(immutable)) {
                builder.note(fndecl->name().range(), diagnostic::format("Look at property `$` declaration.", fndecl->name().lexeme()));
            }
            
            publisher().publish(builder.build());
            expr.invalid(true);
        }
    }
    
    void checker::immutability(const ast::assignment_statement& stmt)
    {
        if (auto immutable = stmt.left()->immutable()) {
            auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(stmt.assignment_operator().location())
                            .message("You can't modify an immutable value, pr*ck!")
                            .highlight(stmt.assignment_operator().range())
                            .highlight(stmt.left()->range(), diagnostic::highlighter::mode::light)
                            .highlight(stmt.right()->range(), diagnostic::highlighter::mode::light);
            
            if (auto vardecl = dynamic_cast<const ast::var_declaration*>(immutable)) {
                builder.note(vardecl->name().range(), diagnostic::format("Look at variable `$` declaration.", vardecl->name().lexeme()));
                if (!vardecl->value()) builder.insertion(source_range(vardecl->name().location(), 1), "mutable ", "This is what you need to make the variable mutable.");
                else builder.insertion(source_range(vardecl->range().begin(), 1), "mutable ", "This is what you need to make the variable mutable.");
            }
            else if (auto constdecl = dynamic_cast<const ast::generic_const_parameter_declaration*>(immutable)) {
                builder.note(constdecl->name().range(), diagnostic::format("Look at generic constant `$` declaration.", constdecl->name().lexeme()));
            }
            else if (auto constdecl = dynamic_cast<const ast::const_declaration*>(immutable)) {
                builder.note(constdecl->name().range(), diagnostic::format("Look at constant `$` declaration.", constdecl->name().lexeme()));
            }
            else if (auto fndecl = dynamic_cast<const ast::function_declaration*>(immutable)) {
                builder.note(fndecl->name().range(), diagnostic::format("Look at function `$` declaration.", fndecl->name().lexeme()));
            }
            else if (auto fndecl = dynamic_cast<const ast::property_declaration*>(immutable)) {
                builder.note(fndecl->name().range(), diagnostic::format("Look at property `$` declaration.", fndecl->name().lexeme()));
            }
            
            publisher().publish(builder.build());
            stmt.invalid(true);
        }
    }

    constval checker::evaluate(ast::pointer<ast::expression> expr)
    {
        evaluator evaluator(*this);
        return evaluator.evaluate(expr);
    }

    const ast::declaration* checker::resolve(const ast::path& path) const
    {
        /**
         * RESOLVE(A.B...C):
         *      se A.B... è il nome del workspace corrente => cerca nel workspace corrent
         *      se A.B... è il nome di uno dei workspace importati => cerca dentro uno dei workspace importati
         *      altrimenti cerca a partire dallo scope attuale
         */
        size_t pos = 0;
        environment* scope = nullptr;
        const ast::declaration* result = nullptr;
        ast::workspace* workspace = this->workspace();
        std::string name = ast::path_to_string(path);
        int local = path_contains_subpath(name, workspace->name);

        if (local > 0) {
            scope = scopes_.at(workspace);
            pos = local;
        }
        else if (!workspace->imports.empty()) {
            for (auto imported : workspace->imports) {
                // if imported workspace is a subpath of path, then the name is used from imported workspace
                // it looks for the longest match of subpath (workspace) inside path
                size_t positions = path_contains_subpath(name, imported.first);
                if (positions > pos) {
                    scope = scopes_.at(imported.second);
                    pos = positions;
                }
            }
        }
        
        if (pos == 0) {
            scope = scope_;
        }

        while (result && pos < path.size() - 1) {
            result = scope->type(path.at(pos++).lexeme().string());
            if (result) scope = scopes_.at(result);
        }
        // TODO: search for types or functions or variables
        if (pos == path.size() - 1) {
            result = scope->type(path.back().lexeme().string());
        }
        
        return result;
    }

    const ast::declaration* checker::resolve_type(const ast::path& path, const environment* context) const
    {
        /**
         * RESOLVE(A.B...C):
         *      se A.B... è il nome del workspace corrente => cerca nel workspace corrent
         *      se A.B... è il nome di uno dei workspace importati => cerca dentro uno dei workspace importati
         *      altrimenti cerca a partire dallo scope attuale prima le variabili, poi funzioni, poi tipi
         */
        size_t pos = 0;
        const environment* scope = context ? context : scope_;
        const ast::declaration* result = nullptr;
        ast::workspace* workspace = this->workspace();
        std::string name = ast::path_to_string(path);
        
        // first attempt is from current scope or passed context
        while (pos < path.size() - 1) {
            result = scope->type(path.at(pos++).lexeme().string());
            if (!result || scopes_.count(result) == 0) break;
            scope = scopes_.at(result);
        }
        
        if (pos == path.size() - 1) {
            if (auto type = scope->type(path.back().lexeme().string())) return type;
        }
        
        // otherwise lookup is made globally
        int local = path_contains_subpath(name, workspace->name);

        if (local > 0) {
            scope = scopes_.at(workspace);
            pos = local;
        }
        else if (!workspace->imports.empty()) {
            for (auto imported : workspace->imports) {
                // if imported workspace is a subpath of path, then the name is used from imported workspace
                // it looks for the longest match of subpath (workspace) inside path
                size_t positions = path_contains_subpath(name, imported.first);
                if (positions > pos) {
                    scope = scopes_.at(imported.second);
                    pos = positions;
                }
            }
        }

        while (pos < path.size() - 1) {
            result = scope->type(path.at(pos++).lexeme().string());
            if (!result) break; 
            scope = scopes_.at(result);
        }
        
        if (pos == path.size() - 1) {
            if (auto type = scope->type(path.back().lexeme().string())) return type;
        }
        
        return nullptr;
    }

    const ast::declaration* checker::resolve_variable(const ast::path& path, const environment* context) const
    {
        /**
         * RESOLVE(A.B...C):
         *      se A.B... è il nome del workspace corrente => cerca nel workspace corrent
         *      se A.B... è il nome di uno dei workspace importati => cerca dentro uno dei workspace importati
         *      altrimenti cerca a partire dallo scope attuale prima le variabili, poi funzioni, poi tipi
         */
        size_t pos = 0;
        const environment* scope = context ? context : scope_;
        const ast::declaration* result = nullptr;
        ast::workspace* workspace = this->workspace();
        std::string name = ast::path_to_string(path);
        
        // first attempt is from current scope or passed context
        while (pos < path.size() - 1) {
            result = scope->type(path.at(pos++).lexeme().string());
            if (!result) break; 
            scope = scopes_.at(result);
        }
        
        if (pos == path.size() - 1) {
            if (auto var = scope->value(path.back().lexeme().string())) return var;
            else if (auto func = scope->function(path.back().lexeme().string())) return func;
            else if (auto type = scope->type(path.back().lexeme().string())) return type;
        }
        
        // otherwise lookup is made globally
        int local = path_contains_subpath(name, workspace->name);

        if (local > 0) {
            scope = scopes_.at(workspace);
            pos = local;
        }
        else if (!workspace->imports.empty()) {
            for (auto imported : workspace->imports) {
                // if imported workspace is a subpath of path, then the name is used from imported workspace
                // it looks for the longest match of subpath (workspace) inside path
                size_t positions = path_contains_subpath(name, imported.first);
                if (positions > pos) {
                    scope = scopes_.at(imported.second);
                    pos = positions;
                }
            }
        }

        while (pos < path.size() - 1) {
            result = scope->type(path.at(pos++).lexeme().string());
            if (!result) break; 
            scope = scopes_.at(result);
        }
        
        if (pos == path.size() - 1) {
            if (auto var = scope->value(path.back().lexeme().string())) return var;
            else if (auto func = scope->function(path.back().lexeme().string())) return func;
            else if (auto type = scope->type(path.back().lexeme().string())) return type;
        }
        
        return result;
    }

    std::string checker::fullname(const ast::declaration *decl) const
    {
        std::stack<std::string> levels;
        
        for (bool done = false; decl && !done;) {
            switch (decl->kind()) {
                case ast::kind::workspace:
                    // in current workspace name is omitted, otherwise it's chained
                    if (decl != workspace()) levels.push(static_cast<const ast::workspace*>(decl)->name);
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
                    levels.push(std::to_string(static_cast<const ast::tuple_field_declaration*>(decl)->index()));
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

            for (; !levels.empty(); levels.pop()) result << "." << levels.top();
        }

        return result.str();
    }

    namespace impl {
        extern std::unordered_map<std::string, ast::pointer<ast::type>> builtins;
    }

    std::unordered_map<std::string, const ast::declaration*> checker::similars(const std::string& name, const environment* scope) const
    {
        std::unordered_map<std::string, const ast::declaration*> result;

        for (auto pair : impl::builtins) {
            if (utils::levenshtein_distance(name, pair.first) < 2) {
                result.emplace(pair.first, nullptr);
            }
        }

        for (const environment* into = scope; into; into = into->parent()) {
            for (auto pair : into->types()) {
                if (utils::levenshtein_distance(name, pair.first) < 2) {
                    result.emplace(fullname(pair.second), pair.second);
                }
            }
            for (auto pair : into->concepts()) {
                if (utils::levenshtein_distance(name, pair.first) < 2) {
                    result.emplace(fullname(pair.second), pair.second);
                }
            }
            for (auto pair : into->functions()) {
                if (utils::levenshtein_distance(name, pair.first) < 2) {
                    result.emplace(fullname(pair.second), pair.second);
                }
            }
            for (auto pair : into->values()) {
                if (utils::levenshtein_distance(name, pair.first) < 2) {
                    result.emplace(fullname(pair.second), pair.second);
                }
            }
        }

        return result;
    }

    void checker::do_imports()
    {
        // cycle dependency search
        using edge = std::pair<ast::workspace*, ast::workspace*>;
        std::map<edge, ast::use_declaration*> edges;
        std::unordered_map<ast::workspace*, bool> visited, resolved;
        std::function<void(ast::workspace* node)> dfs = [&](ast::workspace* node) {
            visited.at(node) = true;

            for (auto source : node->sources) {
                auto ast = std::dynamic_pointer_cast<ast::source_unit_declaration>(source.second->ast());
                for (auto stmt : ast->imports()) {
                    auto import = std::dynamic_pointer_cast<ast::use_declaration>(stmt);
                    auto name = import->path().lexeme().string();
                    auto result = compilation_.workspaces().find(name);
                    // the workspace was not found by name
                    if (result == compilation_.workspaces().end()) {
                        report(import->path().range(), diagnostic::format("I can't find workspace `$` dammit, are you sure you have declared it?", name));
                    }
                    // this is a noose in graph theory which is a cycle around the same vertex
                    else if (result->second.get() == node) {
                        report(import->path().range(), diagnostic::format("You cannot import workspace `$` inside itself, idiot.", name));
                    }
                    // not yet resolved
                    else if (!resolved.at(result->second.get())) {
                        // if already visited but not yet resolved then there is a cyle (backward arrow inside dfs tree)
                        if (visited.at(result->second.get())) {
                            report(import->path().range(), diagnostic::format("Importing workspace `$` creates a damn cyclic dependency, dammit!", name));
                        }
                        // if never seen, then we visit recursively and then make the import
                        else {
                            // its dependency are traversed recursively
                            dfs(result->second.get());
                            // import is performed at workspace level
                            node->imports.emplace(result->first, result->second.get());
                            edges.emplace(std::make_pair(node, result->second.get()), import.get());
                        }
                    }
                    // already resolved, which means completed, and it is a duplicate arrow, which means a redundant import, then it is reported
                    else if (node->imports.find(name) != node->imports.end()) {
                        auto diag = diagnostic::builder()
                                    .severity(diagnostic::severity::warning)
                                    .location(import->path().location())
                                    .message(diagnostic::format("You have already imported workspace `$` inside workspace `$`, idiot.", name, node->name))
                                    .highlight(import->path().range(), "redundant")
                                    .note(edges.at(std::make_pair(node, result->second.get()))->path().range(), "This is the first declaration of usage in case you forgot.")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    // already resolved, so all its dependecy were resolved either, and the import is performed at workspace level
                    else {
                        node->imports.emplace(result->first, result->second.get());
                        edges.emplace(std::make_pair(node, result->second.get()), import.get());
                    }
                }
            }

            resolved.at(node) = true;
        };

        for (auto pair : compilation_.workspaces()) {
            visited.emplace(pair.second.get(), false);
            resolved.emplace(pair.second.get(), false);
        }

        for (auto workspace : compilation_.workspaces()) {
            if (!visited.at(workspace.second.get())) dfs(workspace.second.get());
        }
    }

    void checker::import_core_library_in_workspaces()
    {
        // if core library is excluded by default at higher level, then this pass is skipped
        if (compilation_.packages().count("core") == 0 || compilation_.workspaces().count("core") == 0) return;
        // core workspace
        auto core = compilation_.workspaces().at("core").get();
        // imports all symbols from `core` library inside other workspaces to be referenced without `.` notation
        // for example `core.println` could be written equally as `println`
        // for each workspace, `core` library workspace is added as implicit import library
        for (auto other : compilation_.workspaces()) {
            if (other.first == "core") continue;
            other.second->imports["core"] = core;
        }
        // imports types
        for (auto symbol : scopes_.at(core)->types()) {
            for (auto other : compilation_.workspaces()) {
                if (other.first == "core") continue;
                scopes_.at(other.second.get())->type(symbol.first, symbol.second);
            }
        }
        // imports global constants and variables
        for (auto symbol : scopes_.at(core)->values()) {
            for (auto other : compilation_.workspaces()) {
                if (other.first == "core") continue;
                scopes_.at(other.second.get())->value(symbol.first, symbol.second);
            }
        }
        // imports concepts
        for (auto symbol : scopes_.at(core)->concepts()) {
            for (auto other : compilation_.workspaces()) {
                if (other.first == "core") continue;
                scopes_.at(other.second.get())->concept(symbol.first, symbol.second);
            }
        }
        // imports functions
        for (auto symbol : scopes_.at(core)->functions()) {
            for (auto other : compilation_.workspaces()) {
                if (other.first == "core") continue;
                scopes_.at(other.second.get())->function(symbol.first, symbol.second);
            }
        }
    }

    void checker::visit(const ast::bit_field_type_expression& expr)
    {
        unsigned bits = std::atoi(expr.size().lexeme().cdata());
        
        if (bits == 0 || bits > 256) {
            expr.annotation().type = types::unknown();
            if (bits == 0) report(expr.range(), "Bit field size cannot be zero, idiot!");
            else report(expr.range(), "Bit field size cannot exceed 256 bits, dammit!");
        }
        else {
            expr.annotation().istype = true;
            expr.annotation().type = types::bitfield(bits);
        }
    }
    
    void checker::visit(const ast::array_type_expression& expr) 
    {
        expr.annotation().istype = true;
        expr.element_type()->accept(*this);
        auto base = expr.element_type()->annotation().type;
        
        if (expr.size()) {
            expr.size()->accept(*this);

            try {
                expr.size()->annotation().value = evaluate(expr.size());
            } 
            catch (evaluator::generic_evaluation&) {}

            if (expr.size()->annotation().type->category() != ast::type::category::integer_type) {
                expr.annotation().type = types::unknown();
                error(expr.size()->range(), diagnostic::format("Array size must have integer type, dammit! I found type `$`.", expr.size()->annotation().type->string()));
            }
            else if (auto sizety = std::dynamic_pointer_cast<ast::integer_type>(expr.size()->annotation().value.type)) {
                if (sizety->is_signed()) {
                    if (expr.size()->annotation().value.i.value() < 0) {
                        expr.annotation().type = types::unknown();
                        error(expr.size()->range(), "Array size cannot be negative, idiot!");
                    }
                    else {
                        expr.annotation().type = types::array(base, expr.size()->annotation().value.i.value());
                    }
                } 
                else {
                    expr.annotation().type = types::array(base, expr.size()->annotation().value.u.value());
                }
            }
            else if (auto parametric = dynamic_cast<const ast::generic_const_parameter_declaration*>(expr.size()->annotation().referencing)) {
                auto type = types::array(base, 0);
                type->parametric_size() = parametric;
                expr.annotation().type = type;
            }
        }
        else {
            expr.annotation().type = types::slice(base);
        }
    }
    
    void checker::visit(const ast::tuple_type_expression& expr)
    {
        expr.annotation().istype = true;
        ast::types components;

        for (auto expr : expr.types()) {
            expr->accept(*this);
            components.push_back(expr->annotation().type);
        }

        expr.annotation().type = types::tuple(components);
        // adds anonymous aggregated type to the workspace
        add_type(expr.annotation().type);
    }
    
    void checker::visit(const ast::pointer_type_expression& expr) 
    {
        expr.annotation().istype = true;
        // a pointer type is the only case in which recursive type
        // definitions are allowed
        try { expr.pointee_type()->accept(*this); }
        catch (cyclic_symbol_error& err) {
            // if the error concers the pointee type then
            // it is not considered an error because a pointer
            // creates a level of indirection and it has a compile
            // time known size unless it is a cycle concerning an alias
            // so a definition is missing
            if (dynamic_cast<const ast::alias_declaration*>(err.declaration)) throw;
        }

        expr.annotation().type = types::pointer(expr.pointee_type()->annotation().type);
    }
    
    void checker::visit(const ast::function_type_expression& expr) 
    {
        expr.annotation().istype = true;
        ast::types formals;

        for (auto param : expr.parameter_types()) {
            param->accept(*this);
            param->annotation().type->mutability = std::dynamic_pointer_cast<ast::type_expression>(param)->is_mutable();
            formals.push_back(param->annotation().type);
        }

        if (expr.return_type_expression()) {
            expr.return_type_expression()->accept(*this);
            expr.annotation().type = types::function(formals, expr.return_type_expression()->annotation().type);
        }
        else {
            expr.annotation().type = types::function(formals, types::unit());
        }
    }

    void checker::visit(const ast::variant_type_expression& expr) 
    {
        std::vector<std::pair<ast::pointer<ast::type>, source_range>> map;
        ast::types types;

        expr.annotation().istype = true;
        
        for (auto typexpr : expr.types()) {
            typexpr->accept(*this);

            auto other = std::find_if(map.begin(), map.end(), [&](const std::pair<ast::pointer<ast::type>, source_range>& p) {
                return types::compatible(p.first, typexpr->annotation().type);
            });
            
            if (other != map.end()) {
                auto diag = diagnostic::builder()
                            .location(typexpr->range().begin())
                            .severity(diagnostic::severity::error)
                            .message(diagnostic::format("You have already included type `$` inside this variant, idiot!", typexpr->annotation().type->string()))
                            .highlight(typexpr->range(), "conflicting")
                            .note(other->second, "Here's the same type, you f*cker!")
                            .build();
                
                publisher().publish(diag);
            }
            // behaviour cannot be instantiated, so it is not valid as variant type
            else if (typexpr->annotation().type->category() == ast::type::category::behaviour_type) {
                expr.invalid(true);
                report(typexpr->range(), diagnostic::format("You can't use behaviour `$` as variant type because it cannot be instantiated, c*nt!", typexpr->annotation().type->string()));
            }
            else {
                map.emplace_back(typexpr->annotation().type, typexpr->range());
                types.push_back(typexpr->annotation().type);
            }
        }

        expr.annotation().type = types::variant(types);
        // adds anonymous aggregated type to the workspace
        add_type(expr.annotation().type);
    }

    void checker::visit(const ast::record_type_expression& expr) 
    {
        ast::structure_type::components fields;

        expr.annotation().istype = true;

        for (auto decl : expr.fields()) {
            auto field = std::dynamic_pointer_cast<ast::field_declaration>(decl);
            field->accept(*this);
            fields.emplace_back(field->name().lexeme().string(), field->type_expression()->annotation().type);
        }

        expr.annotation().type = types::record(fields);
        // adds anonymous aggregated type to the workspace
        add_type(expr.annotation().type);
    }
    
    void checker::visit(const ast::literal_expression& expr)
    {
        if (expr.invalid() || !expr.value().valid) throw semantic_error();

        switch (expr.value().kind()) {
            case token::kind::true_kw:
            case token::kind::false_kw:
                expr.annotation().type = types::boolean();
                break;
            case token::kind::integer_literal:
            {
                std::string value = expr.value().lexeme().string();
                size_t suffix = value.find('u');
                if (suffix == value.npos) suffix = value.find('i');
                if (suffix != value.npos) expr.annotation().type = types::builtin(value.substr(suffix));
                else expr.annotation().type = types::sint(32);
                break;
            }
            case token::kind::real_literal:
            {
                std::string value = expr.value().lexeme().string();
                size_t suffix = value.find('f');
                if (suffix != value.npos) expr.annotation().type = types::builtin(value.substr(suffix));
                else expr.annotation().type = types::floating(32);
                break;
            }
            case token::kind::imag_literal:
                expr.annotation().type = types::complex(64);
                break;
            case token::kind::char_literal:
                expr.annotation().type = types::character();
                break;
            case token::kind::string_literal:
                if (expr.value().lexeme().cdata()[expr.value().lexeme().size() - 1] == 's') expr.annotation().type = types::string();
                else expr.annotation().type = types::chars();
                break;
            default:
                expr.annotation().type = types::unknown();
        }

        expr.annotation().value = evaluate(expr.clone());
    }

    void checker::visit(const ast::path_type_expression& expr)
    {
        // left expression could be a type name inside a member expression
        expr.expression()->annotation().mustvalue = false;
        expr.expression()->annotation().musttype = true;
        expr.expression()->accept(*this);

        if (!expr.expression()->annotation().istype) {
            error(expr.expression()->range(), "I was expecting a type here, but I found a value!", "", "expected type");
        }
        else if (!expr.expression()->annotation().type || expr.expression()->annotation().type->category() == ast::type::category::unknown_type) {
            expr.invalid(true);
            expr.annotation().type = types::unknown();
            throw semantic_error();
        }

        if (!expr.member()) {
            expr.annotation() = expr.expression()->annotation();
            return;
        }

        // right expression is a member (field, static function or object function/property)
        auto member = std::static_pointer_cast<ast::identifier_expression>(expr.member());
        std::string item = member->identifier().lexeme().string();

        if (auto builtin = types::builtin(expr.expression()->annotation().type->string())) {
                std::string item = member->identifier().lexeme().string();
                if (member->is_generic()) error(expr.member()->range(), diagnostic::format("There is no generic function `$` associated to builtin type `$`, idiot!", item, builtin->string()));

                switch (builtin->category()) {
                    case ast::type::category::integer_type:
                        if (item == "BITS") expr.annotation().type = types::usize();
                        else if (item == "MIN") expr.annotation().type = builtin;
                        else if (item == "MAX") expr.annotation().type = builtin;
                        else error(expr.member()->range(), diagnostic::format("There is no symbol `$` associated to builtin type `$`, idiot!", item, builtin->string()));
                        break;
                    case ast::type::category::rational_type:
                        if (item == "BITS") expr.annotation().type = types::usize();
                        else error(expr.member()->range(), diagnostic::format("There is no symbol `$` associated to builtin type `$`, idiot!", item, builtin->string()));
                        break;
                    case ast::type::category::float_type:
                        if (item == "BITS") expr.annotation().type = types::usize();
                        else if (item == "MIN") expr.annotation().type = builtin;
                        else if (item == "MAX") expr.annotation().type = builtin;
                        else if (item == "INFINITY") expr.annotation().type = builtin;
                        else if (item == "NAN") expr.annotation().type = builtin;
                        else error(expr.member()->range(), diagnostic::format("There is no symbol `$` associated to builtin type `$`, idiot!", item, builtin->string()));
                        break;
                    case ast::type::category::complex_type:
                        if (item == "BITS") expr.annotation().type = types::usize();
                        else error(expr.member()->range(), diagnostic::format("There is no symbol `$` associated to builtin type `$`, idiot!", item, builtin->string()));
                        break;
                    default:
                        throw semantic_error();
                }

                try { expr.annotation().value = evaluate(expr.clone()); } catch (evaluator::generic_evaluation&) {}
            }
        else if (!expr.expression()->annotation().type->declaration()) throw std::invalid_argument("visit(member_expression): no declaration, to be implemented " + expr.expression()->annotation().type->string());
        else {
            expr.member()->annotation().mustvalue = false;
            expr.member()->annotation().associated = scopes_.at(expr.expression()->annotation().type->declaration());
            expr.member()->annotation().substitution = expr.expression()->annotation().substitution;
            expr.member()->accept(*this);
            expr.annotation().type = expr.member()->annotation().type;
            expr.annotation().value = expr.member()->annotation().value;
            expr.annotation().istype = expr.member()->annotation().istype;
        }
    }

    void checker::visit(const ast::identifier_expression& expr) 
    {   
        std::string name = expr.identifier().lexeme().string();
        bool mistake = false;
        bool cyclic = false;

        // annotation may have been done before in substitutions
        if (expr.annotation().type && (expr.annotation().istype || expr.annotation().value.type));
        else if (expr.annotation().associated) {
            expr.annotation().type = types::unknown();

            if (auto vardecl = expr.annotation().associated->value(name)) {
                // check that whether we are inside a function expression (lambda) and
                // we are trying to access a local variable, which is illegal since
                // a function expression is not defined as a closure and it's not able
                // to capture the local environment
                if (auto fn = expr.annotation().associated->outscope(environment::kind::function)) {
                    // get scope of resolved variable name
                    auto varscope = scopes_.at(vardecl->annotation().scope), fnscope = scopes_.at(fn);
                    auto var = dynamic_cast<const ast::var_declaration*>(vardecl);
                    // test if var scope is a local scope and it's an ancestor for function scope
                    if (var && dynamic_cast<const ast::expression*>(varscope->enclosing()) && fnscope->has_ancestor_scope(varscope)) {
                        auto diag = diagnostic::builder()
                                    .location(expr.range().begin())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("Variable `$` cannot captured inside function scope from local scope!", name))
                                    .highlight(expr.range())
                                    .note(var->name().range(), diagnostic::format("This is `$` declaration from local scope.", name))
                                    .build();
                        
                        publisher().publish(diag);
                        expr.invalid(true);
                    }
                }
                
                if (!vardecl->annotation().resolved) {
                    auto saved = scope_;
                    if (vardecl->annotation().visited) cyclic = true;
                    else try {
                        scope_ = scopes_.at(vardecl->annotation().scope);
                        vardecl->accept(*this); 
                        scope_ = saved;
                    }
                    catch (cyclic_symbol_error& err) {
                        publisher().publish(err.diagnostic());
                        vardecl->annotation().resolved = true;
                        scope_ = saved;
                    }
                    catch (semantic_error& err) {
                        vardecl->annotation().resolved = true;
                        scope_ = saved;
                    }
                }

                if (cyclic) throw cyclic_symbol_error(&expr, vardecl);

                expr.annotation().type = vardecl->annotation().type;
                expr.annotation().referencing = vardecl;
                ++vardecl->annotation().usecount;

                if (auto var = dynamic_cast<const ast::var_declaration*>(vardecl)) {
                    expr.annotation().value = var->value()->annotation().value;
                }
                else if (auto constant = dynamic_cast<const ast::const_declaration*>(vardecl)) {
                    expr.annotation().value = constant->value()->annotation().value;
                }
                else if (dynamic_cast<const ast::generic_const_parameter_declaration*>(vardecl)) {
                    expr.annotation().isparametric = true;
                }
            }
            else if (auto typedecl = expr.annotation().associated->type(name)) {
                if (!typedecl->annotation().resolved) {
                    auto saved = scope_;
                    if (typedecl->annotation().visited) cyclic = true;
                    else try { 
                        scope_ = scopes_.at(typedecl->annotation().scope);
                        typedecl->accept(*this); 
                        scope_ = saved;
                    }
                    catch (cyclic_symbol_error& err) {
                        publisher().publish(err.diagnostic());
                        typedecl->annotation().resolved = true;
                        scope_ = saved;
                    }
                    catch (semantic_error& err) {
                        typedecl->annotation().resolved = true;
                        scope_ = saved;
                    }
                }

                expr.annotation().istype = true;
                expr.annotation().type = typedecl->annotation().type;
                expr.annotation().referencing = typedecl;
                ++typedecl->annotation().usecount;

                if (expr.annotation().substitution) expr.annotation().type = expr.annotation().type->substitute(expr.annotation().type, expr.annotation().substitution->map);

                if (typedecl->is_hidden() && scopes_.at(typedecl)->outscope(environment::kind::workspace) != workspace()) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(expr.range().begin())
                                .message(diagnostic::format("Type `$` is not accessible in this context, idiot!", name))
                                .highlight(expr.range(), "inaccessible")
                                .note(typedecl->name().range(), diagnostic::format("As you can see type `$` was declared with `hide` specifier.", name))
                                .build();
                        
                    publisher().publish(diag);
                    throw semantic_error();
                }

                // checks if there were provided any generic arguments
                if (expr.is_generic()) {
                    // if there were not generic parameters in declaration, then we have an error
                    if (!typedecl->generic()) {
                        auto diag = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(expr.range().begin())
                                    .message(diagnostic::format("Type `$` doesn't take any generic parameters, idiot!", name))
                                    .highlight(expr.range())
                                    .note(typedecl->name().range(), diagnostic::format("As you can see type `$` was declared without generic parameters.", name))
                                    .build();
                            
                        publisher().publish(diag);
                        mistake = true;
                    }
                    else {
                        bool concrete = true;
                        auto expected = std::dynamic_pointer_cast<ast::generic_clause_declaration>(typedecl->generic());
                        std::unordered_map<const ast::declaration*, ast::pointer<ast::type>> tsubstitutions;
                        std::unordered_map<const ast::declaration*, constval> csubstitutions;
                        substitutions sub(scopes_.at(typedecl->generic().get()), nullptr);
                        
                        if (expected->parameters().size() != expr.generics().size()) {
                            auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.range().begin())
                                        .message(diagnostic::format("Type `$` takes `$` generic parameters, you gave it `$`, idiot!", name, expected->parameters().size(), expr.generics().size()))
                                        .highlight(expr.range())
                                        .note(typedecl->name().range(), diagnostic::format("As you can see type `$` was declared with exactly `$` generic parameters.", name, expected->parameters().size()))
                                        .build();
                            
                            publisher().publish(diag);
                            mistake = true;
                        }
                        else for (size_t i = 0; i < expected->parameters().size(); ++i)
                        {
                            if (auto constant = std::dynamic_pointer_cast<ast::generic_const_parameter_declaration>(expected->parameters().at(i))) {
                                if (auto ambiguous = std::dynamic_pointer_cast<ast::type_expression>(expr.generics().at(i))) {
                                    if (ambiguous->is_ambiguous()) {
                                        auto newexpr = ambiguous->as_expression();
                                        newexpr->accept(*this);
                                        expr.generics().at(i) = newexpr;

                                        if (newexpr->annotation().istype) {
                                            auto diag = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(ambiguous->range().begin())
                                                .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                                .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                                .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                .build();
                                
                                            publisher().publish(diag);
                                            mistake = true;
                                        }
                                        else if (!types::assignment_compatible(constant->annotation().type, newexpr->annotation().type)) {
                                            auto diag = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(ambiguous->range().begin())
                                                .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                                .highlight(ambiguous->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                                .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                .build();
                                
                                            publisher().publish(diag);
                                            mistake = true;
                                        }
                                        else {
                                            try { newexpr->annotation().value = evaluate(newexpr); } catch (evaluator::generic_evaluation&) { concrete = false; newexpr->annotation().isparametric = true; }
                                            csubstitutions.emplace(constant.get(), newexpr->annotation().value);
                                            sub.put(constant.get(), newexpr->annotation().value);
                                        }
                                    }
                                    else {
                                        ambiguous->accept(*this);

                                        auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(ambiguous->range().begin())
                                            .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), ambiguous->annotation().type->string()))
                                            .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                            .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                            .build();
                            
                                        publisher().publish(diag);
                                        mistake = true;
                                    }
                                }
                                else {
                                    expr.generics().at(i)->accept(*this);

                                    if (!types::assignment_compatible(constant->annotation().type, expr.generics().at(i)->annotation().type)) {
                                        auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(expr.generics().at(i)->range().begin())
                                            .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), expr.generics().at(i)->annotation().type->string()))
                                            .highlight(expr.generics().at(i)->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                            .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                            .build();
                            
                                        publisher().publish(diag);
                                        mistake = true;
                                    }
                                    else {
                                        try { expr.generics().at(i)->annotation().value = evaluate(expr.generics().at(i)); } catch (evaluator::generic_evaluation&) { concrete = false; expr.generics().at(i)->annotation().isparametric = true; }
                                        csubstitutions.emplace(constant.get(), expr.generics().at(i)->annotation().value);
                                        sub.put(constant.get(), expr.generics().at(i)->annotation().value);
                                    }
                                }
                            }
                            else if (auto type = std::dynamic_pointer_cast<ast::generic_type_parameter_declaration>(expected->parameters().at(i))) {
                                expr.generics().at(i)->annotation().mustvalue = false;
                                expr.generics().at(i)->accept(*this);

                                if (!expr.generics().at(i)->annotation().istype) {
                                    auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(expr.generics().at(i)->range().begin())
                                            .message("I need type parameter here but I found an expression instead, dammit!")
                                            .highlight(expr.generics().at(i)->range(), "expected type")
                                            .note(type->range(), diagnostic::format("Type `$` declares `$` as a type parameter.", name, type->name().lexeme()))
                                            .build();
                            
                                    publisher().publish(diag);
                                    mistake = true;
                                }
                                else {
                                    if (auto typeexpr = std::dynamic_pointer_cast<ast::type_expression>(expr.generics().at(i))) {
                                        if (typeexpr->is_parametric()) concrete = false;
                                    }
                                    
                                    tsubstitutions.emplace(type.get(), expr.generics().at(i)->annotation().type);
                                    sub.put(type.get(), expr.generics().at(i)->annotation().type);
                                }
                            }
                        }
                        // concrete means that a generic type indipendent from any other parametric type so it can be constructed
                        // by monomorphization
                        if (concrete && !mistake) {
                            auto instantiated = instantiate_type(*typedecl, sub);

                            if (!instantiated) {
                                std::ostringstream oss;

                                oss << "Instantiation of type `" << typedecl->name().lexeme() << "` failed with the following arguments";
                                
                                for (auto t : sub.types()) oss << " \\ • " << dynamic_cast<const ast::generic_type_parameter_declaration*>(t.first)->name().lexeme() << " = " << t.second->string();
                                for (auto v : sub.constants()) oss << " \\ • " << dynamic_cast<const ast::generic_const_parameter_declaration*>(v.first)->name().lexeme() << " = " << v.second.simple();

                                oss << " \\ \\ Substitution of generic arguments does not satisfy concept contraints, dammit!";

                                auto builder = diagnostic::builder()
                                               .location(expr.range().begin())
                                               .severity(diagnostic::severity::error)
                                               .message(oss.str())
                                               .small(true)
                                               .highlight(expr.range(), "concept failure");

                                if (typedecl->generic()) {
                                    if (auto constraint = std::static_pointer_cast<ast::generic_clause_declaration>(typedecl->generic())->constraint()) builder.highlight(constraint->range(), diagnostic::highlighter::mode::light);
                                }

                                publisher().publish(builder.build());
                                expr.annotation().type = types::unknown();
                                expr.invalid(true);
                                throw semantic_error();
                            }
                            
                            expr.annotation().type = instantiated;
                            expr.annotation().referencing = instantiated->declaration();
                        }
                    }
                }
                else if (auto generic = std::static_pointer_cast<ast::generic_clause_declaration>(typedecl->generic())) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(expr.range().begin())
                                .message(diagnostic::format("Type `$` is parametric so it expects at least `$` generic parameters!", name, generic->parameters().size()))
                                .highlight(expr.range())
                                .note(typedecl->name().range(), diagnostic::format("Look at type `$` declaration.", name))
                                .build();
                        
                    publisher().publish(diag);
                    mistake = true;
                }

                if (cyclic) throw cyclic_symbol_error(&expr, typedecl);
                if (mistake) throw semantic_error();
            }
            else if (auto fn = expr.annotation().associated->function(name)) {
                expr.annotation().iscallable = true;
                expr.annotation().referencing = fn;
                ++fn->annotation().usecount;

                if (!fn->annotation().resolved) {
                    auto saved = scope_;
                    if (fn->annotation().visited) cyclic = true;
                    else try { 
                        scope_ = scopes_.at(fn->annotation().scope);
                        fn->accept(*this); 
                        scope_ = saved;
                    }
                    catch (cyclic_symbol_error& err) {
                        publisher().publish(err.diagnostic());
                        fn->annotation().resolved = true;
                        scope_ = saved;
                    }
                    catch (semantic_error& err) {
                        fn->annotation().resolved = true;
                        scope_ = saved;
                    }
                }

                if (auto fndecl = dynamic_cast<const ast::function_declaration*>(fn)) expr.annotation().type = fndecl->annotation().type;
                else if (auto prdecl = dynamic_cast<const ast::property_declaration*>(fn)) expr.annotation().type = prdecl->annotation().type;

                if (fn->is_hidden() && scopes_.at(fn)->outscope(environment::kind::workspace) != workspace()) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(expr.range().begin())
                                .message(diagnostic::format("$ `$` is not accessible in this context, idiot!", fn->kind() == ast::kind::function_declaration ? "Function" : "Property", name))
                                .highlight(expr.range(), "inaccessible")
                                .note(fn->kind() == ast::kind::function_declaration ? static_cast<const ast::function_declaration*>(fn)->name().range() : static_cast<const ast::property_declaration*>(fn)->name().range(), diagnostic::format("As you can see $ `$` was declared with `hide` specifier.", fn->kind() == ast::kind::function_declaration ? "function" : "property", name))
                                .build();
                        
                    publisher().publish(diag);
                    throw semantic_error();
                }

                auto name = fn->kind() == ast::kind::function_declaration ? static_cast<const ast::function_declaration*>(fn)->name() : static_cast<const ast::property_declaration*>(fn)->name();
                bool mistake = false;
                substitutions sub(scope_, nullptr);

                // we try generic instantiation of function if requested from annotation
                if (!expr.annotation().deduce);
                else if (!expr.generics().empty() && (fn->kind() == ast::kind::property_declaration || !static_cast<const ast::function_declaration*>(fn)->generic())) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(expr.range().begin())
                                .message(diagnostic::format("$ `$` doesn't take any generic parameters, idiot!", fn->kind() == ast::kind::function_declaration ? "Function" : "Property", name.lexeme()))
                                .highlight(expr.range())
                                .note(name.range(), diagnostic::format("As you can see $ `$` was declared without generic parameters.", fn->kind() == ast::kind::function_declaration ? "function" : "property", name))
                                .build();
                        
                    publisher().publish(diag);
                    mistake = true;
                }
                else if (fn->kind() == ast::kind::function_declaration && static_cast<const ast::function_declaration*>(fn)->generic()) {
                    auto expected = std::dynamic_pointer_cast<ast::generic_clause_declaration>(static_cast<const ast::function_declaration*>(fn)->generic());
                    
                    if (expected->parameters().size() != expr.generics().size()) {
                        auto diag = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(expr.range().begin())
                                    .message(diagnostic::format("$ `$` takes `$` generic parameters, you gave it `$`, idiot!", fn->kind() == ast::kind::function_declaration ? "Function" : "Property", name.lexeme(), expected->parameters().size(), expr.generics().size()))
                                    .highlight(expr.range())
                                    .note(name.range(), diagnostic::format("As you can see $ `$` was declared with exactly `$` generic parameters.", fn->kind() == ast::kind::function_declaration ? "function" : "property", name.lexeme(), expected->parameters().size()))
                                    .build();
                        
                        publisher().publish(diag);
                        mistake = true;
                    }
                    else for (size_t i = 0; i < expr.generics().size(); ++i)
                    {
                        if (auto constant = std::dynamic_pointer_cast<ast::generic_const_parameter_declaration>(expected->parameters().at(i))) {
                            if (auto ambiguous = std::dynamic_pointer_cast<ast::type_expression>(expr.generics().at(i))) {
                                if (ambiguous->is_ambiguous()) {
                                    auto newexpr = ambiguous->as_expression();
                                    newexpr->accept(*this);
                                    expr.generics().at(i) = newexpr;

                                    if (newexpr->annotation().istype) {
                                        auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(ambiguous->range().begin())
                                            .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                            .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                            .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                            .build();
                            
                                        publisher().publish(diag);
                                        mistake = true;
                                    }
                                    else if (!types::compatible(constant->annotation().type, newexpr->annotation().type)) {
                                        auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(ambiguous->range().begin())
                                            .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                            .highlight(ambiguous->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                            .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                            .build();
                            
                                        publisher().publish(diag);
                                        mistake = true;
                                    }
                                    else {
                                        try { newexpr->annotation().value = evaluate(newexpr); } catch (evaluator::generic_evaluation&) { newexpr->annotation().isparametric = true; }
                                        sub.put(constant.get(), newexpr->annotation().value);
                                    }
                                }
                                else {
                                    ambiguous->accept(*this);

                                    auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(ambiguous->range().begin())
                                        .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), ambiguous->annotation().type->string()))
                                        .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                        .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                        .build();
                        
                                    publisher().publish(diag);
                                    mistake = true;
                                }
                            }
                            else {
                                expr.generics().at(i)->accept(*this);

                                if (!types::compatible(constant->annotation().type, expr.generics().at(i)->annotation().type)) {
                                    auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.generics().at(i)->range().begin())
                                        .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), expr.generics().at(i)->annotation().type->string()))
                                        .highlight(expr.generics().at(i)->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                        .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                        .build();
                        
                                    publisher().publish(diag);
                                    mistake = true;
                                }
                                else {
                                    try { expr.generics().at(i)->annotation().value = evaluate(expr.generics().at(i)); } catch (evaluator::generic_evaluation&) { expr.generics().at(i)->annotation().isparametric = true; }
                                    sub.put(constant.get(), expr.generics().at(i)->annotation().value);
                                }
                            }
                        }
                        else if (auto type = std::dynamic_pointer_cast<ast::generic_type_parameter_declaration>(expected->parameters().at(i))) {
                            expr.generics().at(i)->annotation().mustvalue = false;
                            expr.generics().at(i)->accept(*this);

                            if (!expr.generics().at(i)->annotation().istype) {
                                auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.generics().at(i)->range().begin())
                                        .message("I need type parameter here but I found an expression instead, dammit!")
                                        .highlight(expr.generics().at(i)->range(), "expected type")
                                        .note(type->range(), diagnostic::format("Type `$` declares `$` as a type parameter.", name, type->name().lexeme()))
                                        .build();
                        
                                publisher().publish(diag);
                                mistake = true;
                            }
                            else {
                                sub.put(type.get(), expr.generics().at(i)->annotation().type);
                            }
                        }
                    }
                
                    if (mistake) {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        throw semantic_error();
                    }
                }
            }
            else if (auto ct = expr.annotation().associated->concept(name)) {
                if (!ct->annotation().resolved) {
                    auto saved = scope_;
                    if (ct->annotation().visited) cyclic = true;
                    else try { 
                        scope_ = scopes_.at(ct->annotation().scope);
                        ct->accept(*this); 
                        scope_ = saved;
                    }
                    catch (cyclic_symbol_error& err) {
                        publisher().publish(err.diagnostic());
                        ct->annotation().resolved = true;
                        scope_ = saved;
                    }
                    catch (semantic_error& err) {
                        ct->annotation().resolved = true;
                        scope_ = saved;
                    }
                }

                expr.annotation().istype = false;
                expr.annotation().isconcept = true;
                expr.annotation().type = types::boolean();
                expr.annotation().referencing = ct;
                ++ct->annotation().usecount;

                if (ct->is_hidden() && scopes_.at(ct)->outscope(environment::kind::workspace) != workspace()) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(expr.range().begin())
                                .message(diagnostic::format("Concept `$` is not accessible in this context, idiot!", name))
                                .highlight(expr.range(), "inaccessible")
                                .note(ct->name().range(), diagnostic::format("As you can see concept `$` was declared with `hide` specifier.", name))
                                .build();
                        
                    publisher().publish(diag);
                    throw semantic_error();
                }

                // checks if there were provided any generic arguments
                if (expr.is_generic()) {
                    // if there were not generic parameters in declaration, then we have an error
                    if (!ct->generic()) {
                        auto diag = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(expr.range().begin())
                                    .message(diagnostic::format("Concept `$` doesn't take any generic parameters, idiot!", name))
                                    .highlight(expr.range())
                                    .note(ct->name().range(), diagnostic::format("As you can see concept `$` was declared without generic parameters.", name))
                                    .build();
                            
                        publisher().publish(diag);
                        mistake = true;
                    }
                    else {
                        bool concrete = true;
                        auto expected = std::dynamic_pointer_cast<ast::generic_clause_declaration>(ct->generic());
                        std::unordered_map<const ast::declaration*, ast::pointer<ast::type>> tsubstitutions;
                        std::unordered_map<const ast::declaration*, constval> csubstitutions;
                        substitutions sub(scopes_.at(ct->generic().get()), nullptr);
                        
                        if (expected->parameters().size() != expr.generics().size()) {
                            auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.range().begin())
                                        .message(diagnostic::format("Concept `$` takes `$` generic parameters, you gave it `$`, idiot!", name, expected->parameters().size(), expr.generics().size()))
                                        .highlight(expr.range())
                                        .note(ct->name().range(), diagnostic::format("As you can see concept `$` was declared with exactly `$` generic parameters.", name, expected->parameters().size()))
                                        .build();
                            
                            publisher().publish(diag);
                            mistake = true;
                        }
                        else for (size_t i = 0; i < expected->parameters().size(); ++i)
                        {
                            if (auto constant = std::dynamic_pointer_cast<ast::generic_const_parameter_declaration>(expected->parameters().at(i))) {
                                if (auto ambiguous = std::dynamic_pointer_cast<ast::type_expression>(expr.generics().at(i))) {
                                    if (ambiguous->is_ambiguous()) {
                                        auto newexpr = ambiguous->as_expression();
                                        newexpr->accept(*this);
                                        expr.generics().at(i) = newexpr;

                                        if (newexpr->annotation().istype) {
                                            auto diag = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(ambiguous->range().begin())
                                                .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                                .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                                .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                .build();
                                
                                            publisher().publish(diag);
                                            mistake = true;
                                        }
                                        else if (!types::assignment_compatible(constant->annotation().type, newexpr->annotation().type)) {
                                            auto diag = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(ambiguous->range().begin())
                                                .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                                .highlight(ambiguous->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                                .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                .build();
                                
                                            publisher().publish(diag);
                                            mistake = true;
                                        }
                                        else {
                                            try { newexpr->annotation().value = evaluate(newexpr); } catch (evaluator::generic_evaluation&) { concrete = false; newexpr->annotation().isparametric = true; }
                                            csubstitutions.emplace(constant.get(), newexpr->annotation().value);
                                            sub.put(constant.get(), newexpr->annotation().value);
                                        }
                                    }
                                    else {
                                        ambiguous->accept(*this);

                                        auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(ambiguous->range().begin())
                                            .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), ambiguous->annotation().type->string()))
                                            .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                            .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                            .build();
                            
                                        publisher().publish(diag);
                                        mistake = true;
                                    }
                                }
                                else {
                                    expr.generics().at(i)->accept(*this);

                                    if (!types::assignment_compatible(constant->annotation().type, expr.generics().at(i)->annotation().type)) {
                                        auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(expr.generics().at(i)->range().begin())
                                            .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), expr.generics().at(i)->annotation().type->string()))
                                            .highlight(expr.generics().at(i)->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                            .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                            .build();
                            
                                        publisher().publish(diag);
                                        mistake = true;
                                    }
                                    else {
                                        try { expr.generics().at(i)->annotation().value = evaluate(expr.generics().at(i)); } catch (evaluator::generic_evaluation&) { concrete = false; expr.generics().at(i)->annotation().isparametric = true; }
                                        csubstitutions.emplace(constant.get(), expr.generics().at(i)->annotation().value);
                                        sub.put(constant.get(), expr.generics().at(i)->annotation().value);
                                    }
                                }
                            }
                            else if (auto type = std::dynamic_pointer_cast<ast::generic_type_parameter_declaration>(expected->parameters().at(i))) {
                                expr.generics().at(i)->annotation().mustvalue = false;
                                expr.generics().at(i)->accept(*this);

                                if (!expr.generics().at(i)->annotation().istype) {
                                    auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(expr.generics().at(i)->range().begin())
                                            .message("I need type parameter here but I found an expression instead, dammit!")
                                            .highlight(expr.generics().at(i)->range(), "expected type")
                                            .note(type->range(), diagnostic::format("Type `$` declares `$` as a type parameter.", name, type->name().lexeme()))
                                            .build();
                            
                                    publisher().publish(diag);
                                    mistake = true;
                                }
                                else {
                                    if (auto typeexpr = std::dynamic_pointer_cast<ast::type_expression>(expr.generics().at(i))) {
                                        if (typeexpr->is_parametric()) concrete = false;
                                    }
                                    
                                    tsubstitutions.emplace(type.get(), expr.generics().at(i)->annotation().type);
                                    sub.put(type.get(), expr.generics().at(i)->annotation().type);
                                }
                            }
                        }
                        // concrete means that a generic type independent from any other parametric type so it can be constructed by monomorphization
                        if (concrete && !mistake) {
                            expr.annotation().value.type = types::boolean();
                            expr.annotation().value.b = instantiate_concept(*ct, sub);
                        }
                    }
                }
                else if (auto generic = std::static_pointer_cast<ast::generic_clause_declaration>(ct->generic())) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(expr.range().begin())
                                .message(diagnostic::format("Concept `$` is parametric so it expects at least `$` generic parameters!", name, generic->parameters().size()))
                                .highlight(expr.range())
                                .note(ct->name().range(), diagnostic::format("Look at concept `$` declaration.", name))
                                .build();
                        
                    publisher().publish(diag);
                    mistake = true;
                }

                if (cyclic) throw cyclic_symbol_error(&expr, ct);
                if (mistake) throw semantic_error();
            }
            else {
                std::ostringstream oss;
                auto similars = this->similars(expr.identifier().lexeme().string(), expr.annotation().associated);
            
                if (!similars.empty()) {
                    oss << "Maybe you meant any of these";
                    for (auto similar : similars) oss << " \\ • " << similar.first;
                }
            
                expr.invalid(true);
                expr.annotation().type = types::unknown();
                error(expr.range(), diagnostic::format("I cannot find any symbol whose name is `$`, are you sure you have declare it, dumb*ss?", name), oss.str());
            }
        }
        else {
            expr.annotation().type = types::unknown();

            // checks if name references a workspace
            auto it = compilation_.workspaces().find(expr.identifier().lexeme().string());

            if (it != compilation_.workspaces().end()) {
                if (expr.generics().size()) error(expr.range(), diagnostic::format("You cannot use generic arguments with workspace name `$`, c*nt!", it->first));
                // current workspace
                auto current = workspace();
                // checks if workspace is imported inside this unit, note that
                // `use` directive is purely formal and declarative (even though necessary for readability and expressiveness)
                if (it->first != current->name && current->imports.count(it->first) == 0) {
                    expr.annotation().type = types::unknown();
                    expr.invalid(true);
                    error(expr.identifier().range(), diagnostic::format("If you wanna use workspace `$`, then you must import it, idiot! \\ I suggest writing `use $` inside this workspace.", it->first, it->first), "", "not imported");
                }
                // correct, then name reference is annotated
                expr.annotation().istype = true;
                expr.annotation().type = it->second->type;
                expr.annotation().referencing = it->second.get();
                expr.annotation().associated = scopes_.at(it->second.get());
                ++it->second->annotation().usecount;
            }
            else if (auto vardecl = scope_->value(name)) {
                // check that whether we are inside a function expression (lambda) and
                // we are trying to access a local variable, which is illegal since
                // a function expression is not defined as a closure and it's not able
                // to capture the local environment
                if (auto fn = scope_->outscope(environment::kind::function)) {
                    // get scope of resolved variable name
                    auto varscope = scopes_.at(vardecl->annotation().scope), fnscope = scopes_.at(fn);
                    auto var = dynamic_cast<const ast::var_declaration*>(vardecl);
                    // test if var scope is a local scope and it's an ancestor for function scope
                    if (var && dynamic_cast<const ast::expression*>(varscope->enclosing()) && fnscope->has_ancestor_scope(varscope)) {
                        auto diag = diagnostic::builder()
                                    .location(expr.range().begin())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("Variable `$` cannot captured inside function scope from local scope!", name))
                                    .highlight(expr.range())
                                    .note(var->name().range(), diagnostic::format("This is `$` declaration from local scope.", name))
                                    .build();
                        
                        publisher().publish(diag);
                        expr.invalid(true);
                    }
                }
                // resolve unresolved variable or constant
                if (!vardecl->annotation().resolved) {
                    auto saved = scope_;
                    if (vardecl->annotation().visited) cyclic = true;
                    else try {
                        scope_ = scopes_.at(vardecl->annotation().scope);
                        vardecl->accept(*this); 
                        scope_ = saved;
                    }
                    catch (cyclic_symbol_error& err) {
                        publisher().publish(err.diagnostic());
                        vardecl->annotation().resolved = true;
                        scope_ = saved;
                    }
                    catch (semantic_error& err) {
                        vardecl->annotation().resolved = true;
                        scope_ = saved;
                    }
                }
                // if it were the name was mapped to a tupled declaration, when now we can get the splitted declaration
                if (vardecl->kind() == ast::kind::var_tupled_declaration || vardecl->kind() == ast::kind::const_tupled_declaration) vardecl = scope_->value(name);
                
                if (cyclic) throw cyclic_symbol_error(&expr, vardecl);

                expr.annotation().type = vardecl->annotation().type;
                expr.annotation().referencing = vardecl;
                ++vardecl->annotation().usecount;

                if (auto var = dynamic_cast<const ast::var_declaration*>(vardecl)) {
                    if (var->value()) expr.annotation().value = var->value()->annotation().value;
                }
                else if (auto constant = dynamic_cast<const ast::const_declaration*>(vardecl)) {
                    expr.annotation().value = constant->value()->annotation().value;
                }
                else if (dynamic_cast<const ast::generic_const_parameter_declaration*>(vardecl)) {
                    expr.annotation().isparametric = true;
                }
            }
            else if (auto type = types::builtin(name)) {
                expr.annotation().istype = true;
                expr.annotation().type = type;
            }
            else if (auto typedecl = scope_->type(name)) {
                if (!typedecl->annotation().resolved) {
                    auto saved = scope_;
                    if (typedecl->annotation().visited) cyclic = true;
                    else try { 
                        scope_ = scopes_.at(typedecl->annotation().scope);
                        typedecl->accept(*this); 
                        scope_ = saved;
                    }
                    catch (cyclic_symbol_error& err) {
                        publisher().publish(err.diagnostic());
                        typedecl->annotation().resolved = true;
                        scope_ = saved;
                    }
                    catch (semantic_error& err) {
                        typedecl->annotation().resolved = true;
                        scope_ = saved;
                    }
                }

                expr.annotation().istype = true;
                expr.annotation().type = typedecl->annotation().type;
                expr.annotation().referencing = typedecl;
                ++typedecl->annotation().usecount;

                if (typedecl->is_hidden() && scopes_.at(typedecl)->outscope(environment::kind::workspace) != workspace()) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(expr.range().begin())
                                .message(diagnostic::format("Type `$` is not accessible in this context, idiot!", name))
                                .highlight(expr.range(), "inaccessible")
                                .note(typedecl->name().range(), diagnostic::format("As you can see type `$` was declared with `hide` specifier.", name))
                                .build();
                        
                    publisher().publish(diag);
                    throw semantic_error();
                }

                // checks if there were provided any generic arguments
                if (expr.is_generic()) {
                    // if there were not generic parameters in declaration, then we have an error
                    if (!typedecl->generic()) {
                        auto diag = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(expr.range().begin())
                                    .message(diagnostic::format("Type `$` doesn't take any generic parameters, idiot!", name))
                                    .highlight(expr.range())
                                    .note(typedecl->name().range(), diagnostic::format("As you can see type `$` was declared without generic parameters.", name))
                                    .build();
                            
                        publisher().publish(diag);
                        mistake = true;
                    }
                    else {
                        bool concrete = true;
                        auto expected = std::dynamic_pointer_cast<ast::generic_clause_declaration>(typedecl->generic());
                        std::unordered_map<const ast::declaration*, ast::pointer<ast::type>> tsubstitutions;
                        std::unordered_map<const ast::declaration*, constval> csubstitutions;
                        substitutions sub(scopes_.at(typedecl->generic().get()), nullptr);
                        
                        if (expected->parameters().size() != expr.generics().size()) {
                            auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.range().begin())
                                        .message(diagnostic::format("Type `$` takes `$` generic parameters, you gave it `$`, idiot!", name, expected->parameters().size(), expr.generics().size()))
                                        .highlight(expr.range())
                                        .note(typedecl->name().range(), diagnostic::format("As you can see type `$` was declared with exactly `$` generic parameters.", name, expected->parameters().size()))
                                        .build();
                            
                            publisher().publish(diag);
                            mistake = true;
                        }
                        else for (size_t i = 0; i < expected->parameters().size(); ++i)
                        {
                            if (auto constant = std::dynamic_pointer_cast<ast::generic_const_parameter_declaration>(expected->parameters().at(i))) {
                                if (auto ambiguous = std::dynamic_pointer_cast<ast::type_expression>(expr.generics().at(i))) {
                                    if (ambiguous->is_ambiguous()) {
                                        auto newexpr = ambiguous->as_expression();
                                        newexpr->accept(*this);
                                        expr.generics().at(i) = newexpr;

                                        if (newexpr->annotation().istype) {
                                            auto diag = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(ambiguous->range().begin())
                                                .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                                .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                                .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                .build();
                                
                                            publisher().publish(diag);
                                            mistake = true;
                                        }
                                        else if (!types::assignment_compatible(constant->annotation().type, newexpr->annotation().type)) {
                                            auto diag = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(ambiguous->range().begin())
                                                .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                                .highlight(ambiguous->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                                .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                .build();
                                
                                            publisher().publish(diag);
                                            mistake = true;
                                        }
                                        else {
                                            try { newexpr->annotation().value = evaluate(newexpr); } catch (evaluator::generic_evaluation&) { concrete = false; newexpr->annotation().isparametric = true; }
                                            csubstitutions.emplace(constant.get(), newexpr->annotation().value);
                                            sub.put(constant.get(), newexpr->annotation().value);
                                        }
                                    }
                                    else {
                                        ambiguous->accept(*this);

                                        auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(ambiguous->range().begin())
                                            .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), ambiguous->annotation().type->string()))
                                            .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                            .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                            .build();
                            
                                        publisher().publish(diag);
                                        mistake = true;
                                    }
                                }
                                else {
                                    expr.generics().at(i)->accept(*this);

                                    if (!types::assignment_compatible(constant->annotation().type, expr.generics().at(i)->annotation().type)) {
                                        auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(expr.generics().at(i)->range().begin())
                                            .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), expr.generics().at(i)->annotation().type->string()))
                                            .highlight(expr.generics().at(i)->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                            .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                            .build();
                            
                                        publisher().publish(diag);
                                        mistake = true;
                                    }
                                    else {
                                        try { expr.generics().at(i)->annotation().value = evaluate(expr.generics().at(i)); } catch (evaluator::generic_evaluation&) { concrete = false; expr.generics().at(i)->annotation().isparametric = true; }
                                        csubstitutions.emplace(constant.get(), expr.generics().at(i)->annotation().value);
                                        sub.put(constant.get(), expr.generics().at(i)->annotation().value);
                                    }
                                }
                            }
                            else if (auto type = std::dynamic_pointer_cast<ast::generic_type_parameter_declaration>(expected->parameters().at(i))) {
                                expr.generics().at(i)->annotation().mustvalue = false;
                                expr.generics().at(i)->accept(*this);

                                if (!expr.generics().at(i)->annotation().istype) {
                                    auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(expr.generics().at(i)->range().begin())
                                            .message("I need type parameter here but I found an expression instead, dammit!")
                                            .highlight(expr.generics().at(i)->range(), "expected type")
                                            .note(type->range(), diagnostic::format("Type `$` declares `$` as a type parameter.", name, type->name().lexeme()))
                                            .build();
                            
                                    publisher().publish(diag);
                                    mistake = true;
                                }
                                else {
                                    if (auto typeexpr = std::dynamic_pointer_cast<ast::type_expression>(expr.generics().at(i))) {
                                        if (typeexpr->is_parametric()) concrete = false;
                                    }
                                    
                                    tsubstitutions.emplace(type.get(), expr.generics().at(i)->annotation().type);
                                    sub.put(type.get(), expr.generics().at(i)->annotation().type);
                                }
                            }
                        }
                        // concrete means that a generic type indipendent from any other parametric type so it can be constructed
                        // by monomorphization
                        if (concrete && !mistake) {
                            auto instantiated = instantiate_type(*typedecl, sub);
                            
                            if (!instantiated) {
                                std::ostringstream oss;

                                oss << "Instantiation of type `" << typedecl->name().lexeme() << "` failed with the following arguments";
                                
                                for (auto t : sub.types()) oss << " \\ • " << dynamic_cast<const ast::generic_type_parameter_declaration*>(t.first)->name().lexeme() << " = " << t.second->string();
                                for (auto v : sub.constants()) oss << " \\ • " << dynamic_cast<const ast::generic_const_parameter_declaration*>(v.first)->name().lexeme() << " = " << v.second.simple();

                                oss << " \\ \\ Substitution of generic arguments does not satisfy concept contraints, dammit!";

                                auto builder = diagnostic::builder()
                                               .location(expr.range().begin())
                                               .severity(diagnostic::severity::error)
                                               .message(oss.str())
                                               .small(true)
                                               .highlight(expr.range(), "concept failure");

                                if (typedecl->generic()) {
                                    if (auto constraint = std::static_pointer_cast<ast::generic_clause_declaration>(typedecl->generic())->constraint()) builder.highlight(constraint->range(), diagnostic::highlighter::mode::light);
                                }

                                publisher().publish(builder.build());
                                expr.annotation().type = types::unknown();
                                expr.invalid(true);
                                throw semantic_error();
                            }

                            std::unordered_map<const ast::declaration*, impl::parameter> map;
                            
                            for (auto t : sub.types()) map.emplace(t.first, impl::parameter::make_type(t.second));
                            for (auto v : sub.constants()) map.emplace(v.first, impl::parameter::make_value(v.second));
                            
                            instantiated = instantiated->substitute(instantiated, map);
                            expr.annotation().type = instantiated;
                            expr.annotation().referencing = instantiated->declaration();
                        }
                        else {
                            std::unordered_map<const ast::declaration*, impl::parameter> map;
                            
                            for (auto t : sub.types()) map.emplace(t.first, impl::parameter::make_type(t.second));
                            for (auto v : sub.constants()) map.emplace(v.first, impl::parameter::make_value(v.second));
                            for (size_t i = 0; i < expected->parameters().size(); ++i) {
                                if (auto constparam = dynamic_cast<const ast::generic_const_parameter_declaration*>(expr.generics().at(i)->annotation().referencing)) {
                                    impl::parameter parametric;
                                    parametric.kind = impl::parameter::kind::value;
                                    parametric.value.type = expected->parameters().at(i)->annotation().type;
                                    parametric.type = expected->parameters().at(i)->annotation().type;
                                    parametric.referencing = constparam;
                                    map.at(expected->parameters().at(i).get()) = parametric;
                                }
                            }
                            
                            expr.annotation().type = expr.annotation().type->substitute(expr.annotation().type, map);
                            expr.annotation().isparametric = true;
                            expr.annotation().substitution = ast::create<substitution>(map);
                        }
                    }
                }
                else if (auto generic = std::static_pointer_cast<ast::generic_clause_declaration>(typedecl->generic())) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(expr.range().begin())
                                .message(diagnostic::format("Type `$` is parametric so it expects at least `$` generic parameters!", name, generic->parameters().size()))
                                .highlight(expr.range())
                                .note(typedecl->name().range(), diagnostic::format("Look at type `$` declaration.", name))
                                .build();
                        
                    publisher().publish(diag);
                    mistake = true;
                }

                if (cyclic) throw cyclic_symbol_error(&expr, typedecl);
                if (mistake) throw semantic_error();
            }
            else if (auto fn = scope_->function(name)) {
                expr.annotation().iscallable = true;
                expr.annotation().referencing = fn;
                ++fn->annotation().usecount;

                if (!fn->annotation().resolved) {
                    auto saved = scope_;
                    if (fn->annotation().visited) cyclic = true;
                    else try { 
                        scope_ = scopes_.at(fn->annotation().scope);
                        fn->accept(*this); 
                        scope_ = saved;
                    }
                    catch (cyclic_symbol_error& err) {
                        publisher().publish(err.diagnostic());
                        fn->annotation().resolved = true;
                        scope_ = saved;
                    }
                    catch (semantic_error& err) {
                        fn->annotation().resolved = true;
                        scope_ = saved;
                    }
                }

                if (auto fndecl = dynamic_cast<const ast::function_declaration*>(fn)) expr.annotation().type = fndecl->annotation().type;
                else if (auto prdecl = dynamic_cast<const ast::property_declaration*>(fn)) expr.annotation().type = prdecl->annotation().type;

                if (fn->is_hidden() && scopes_.at(fn)->outscope(environment::kind::workspace) != workspace()) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(expr.range().begin())
                                .message(diagnostic::format("$ `$` is not accessible in this context, idiot!", fn->kind() == ast::kind::function_declaration ? "Function" : "Property", name))
                                .highlight(expr.range(), "inaccessible")
                                .note(fn->kind() == ast::kind::function_declaration ? static_cast<const ast::function_declaration*>(fn)->name().range() : static_cast<const ast::property_declaration*>(fn)->name().range(), diagnostic::format("As you can see $ `$` was declared with `hide` specifier.", fn->kind() == ast::kind::function_declaration ? "function" : "property", name))
                                .build();
                        
                    publisher().publish(diag);
                    throw semantic_error();
                }
                
                auto name = fn->kind() == ast::kind::function_declaration ? static_cast<const ast::function_declaration*>(fn)->name() : static_cast<const ast::property_declaration*>(fn)->name();
                bool mistake = false, concrete = true;
                substitutions sub(scope_, nullptr);

                // we try generic instantiation of function if requested from annotation
                if (!expr.annotation().deduce);
                else if (!expr.generics().empty() && (fn->kind() == ast::kind::property_declaration || !static_cast<const ast::function_declaration*>(fn)->generic())) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(expr.range().begin())
                                .message(diagnostic::format("$ `$` doesn't take any generic parameters, idiot!", fn->kind() == ast::kind::function_declaration ? "Function" : "Property", name.lexeme()))
                                .highlight(expr.range())
                                .note(name.range(), diagnostic::format("As you can see $ `$` was declared without generic parameters.", fn->kind() == ast::kind::function_declaration ? "function" : "property", name))
                                .build();
                        
                    publisher().publish(diag);
                    mistake = true;
                }
                else if (fn->kind() == ast::kind::function_declaration && static_cast<const ast::function_declaration*>(fn)->generic()) {
                    auto expected = std::dynamic_pointer_cast<ast::generic_clause_declaration>(static_cast<const ast::function_declaration*>(fn)->generic());
                    
                    if (expected->parameters().size() != expr.generics().size()) {
                        auto diag = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(expr.range().begin())
                                    .message(diagnostic::format("$ `$` takes `$` generic parameters, you gave it `$`, idiot!", fn->kind() == ast::kind::function_declaration ? "Function" : "Property", name.lexeme(), expected->parameters().size(), expr.generics().size()))
                                    .highlight(expr.range())
                                    .note(name.range(), diagnostic::format("As you can see $ `$` was declared with exactly `$` generic parameters.", fn->kind() == ast::kind::function_declaration ? "function" : "property", name.lexeme(), expected->parameters().size()))
                                    .build();
                        
                        publisher().publish(diag);
                        mistake = true;
                    }
                    else for (size_t i = 0; i < expr.generics().size(); ++i)
                    {
                        if (auto constant = std::dynamic_pointer_cast<ast::generic_const_parameter_declaration>(expected->parameters().at(i))) {
                            if (auto ambiguous = std::dynamic_pointer_cast<ast::type_expression>(expr.generics().at(i))) {
                                if (ambiguous->is_ambiguous()) {
                                    auto newexpr = ambiguous->as_expression();
                                    newexpr->accept(*this);
                                    expr.generics().at(i) = newexpr;

                                    if (newexpr->annotation().istype) {
                                        auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(ambiguous->range().begin())
                                            .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                            .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                            .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                            .build();
                            
                                        publisher().publish(diag);
                                        mistake = true;
                                    }
                                    else if (!types::compatible(constant->annotation().type, newexpr->annotation().type)) {
                                        auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(ambiguous->range().begin())
                                            .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                            .highlight(ambiguous->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                            .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                            .build();
                            
                                        publisher().publish(diag);
                                        mistake = true;
                                    }
                                    else {
                                        try { newexpr->annotation().value = evaluate(newexpr); } catch (evaluator::generic_evaluation&) { concrete = false; newexpr->annotation().isparametric = true; }
                                        sub.put(constant.get(), newexpr->annotation().value);
                                    }
                                }
                                else {
                                    ambiguous->accept(*this);

                                    auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(ambiguous->range().begin())
                                        .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), ambiguous->annotation().type->string()))
                                        .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                        .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                        .build();
                        
                                    publisher().publish(diag);
                                    mistake = true;
                                }
                            }
                            else {
                                expr.generics().at(i)->accept(*this);

                                if (!types::compatible(constant->annotation().type, expr.generics().at(i)->annotation().type)) {
                                    auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.generics().at(i)->range().begin())
                                        .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), expr.generics().at(i)->annotation().type->string()))
                                        .highlight(expr.generics().at(i)->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                        .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                        .build();
                        
                                    publisher().publish(diag);
                                    mistake = true;
                                }
                                else {
                                    try { expr.generics().at(i)->annotation().value = evaluate(expr.generics().at(i)); } catch (evaluator::generic_evaluation&) { concrete = false; expr.generics().at(i)->annotation().isparametric = true; }
                                    sub.put(constant.get(), expr.generics().at(i)->annotation().value);
                                }
                            }
                        }
                        else if (auto type = std::dynamic_pointer_cast<ast::generic_type_parameter_declaration>(expected->parameters().at(i))) {
                            expr.generics().at(i)->annotation().mustvalue = false;
                            expr.generics().at(i)->accept(*this);

                            if (!expr.generics().at(i)->annotation().istype) {
                                auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.generics().at(i)->range().begin())
                                        .message("I need type parameter here but I found an expression instead, dammit!")
                                        .highlight(expr.generics().at(i)->range(), "expected type")
                                        .note(type->range(), diagnostic::format("Type `$` declares `$` as a type parameter.", name, type->name().lexeme()))
                                        .build();
                        
                                publisher().publish(diag);
                                mistake = true;
                            }
                            else {
                                if (auto typeexpr = std::dynamic_pointer_cast<ast::type_expression>(expr.generics().at(i))) {
                                    if (typeexpr->is_parametric()) concrete = false;
                                }
                                
                                sub.put(type.get(), expr.generics().at(i)->annotation().type);
                            }
                        }
                    }
                
                    if (mistake) {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        throw semantic_error();
                    }

                    if (!concrete) {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        throw semantic_error();
                    }

                    auto instantiated = instantiate_function(*static_cast<const ast::function_declaration*>(fn), sub);
                    expr.annotation().referencing = instantiated.get();
                    expr.annotation().type = std::dynamic_pointer_cast<ast::function_type>(instantiated->annotation().type);
                }
            }
            else if (auto ct = scope_->concept(name)) {
                if (!ct->annotation().resolved) {
                    auto saved = scope_;
                    if (ct->annotation().visited) cyclic = true;
                    else try { 
                        scope_ = scopes_.at(ct->annotation().scope);
                        ct->accept(*this); 
                        scope_ = saved;
                    }
                    catch (cyclic_symbol_error& err) {
                        publisher().publish(err.diagnostic());
                        ct->annotation().resolved = true;
                        scope_ = saved;
                    }
                    catch (semantic_error& err) {
                        ct->annotation().resolved = true;
                        scope_ = saved;
                    }
                }

                expr.annotation().istype = false;
                expr.annotation().isconcept = true;
                expr.annotation().type = types::boolean();
                expr.annotation().referencing = ct;
                ++ct->annotation().usecount;

                if (ct->is_hidden() && scopes_.at(ct)->outscope(environment::kind::workspace) != workspace()) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(expr.range().begin())
                                .message(diagnostic::format("Concept `$` is not accessible in this context, idiot!", name))
                                .highlight(expr.range(), "inaccessible")
                                .note(ct->name().range(), diagnostic::format("As you can see concept `$` was declared with `hide` specifier.", name))
                                .build();
                        
                    publisher().publish(diag);
                    throw semantic_error();
                }

                // checks if there were provided any generic arguments
                if (expr.is_generic()) {
                    // if there were not generic parameters in declaration, then we have an error
                    if (!ct->generic()) {
                        auto diag = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(expr.range().begin())
                                    .message(diagnostic::format("Concept `$` doesn't take any generic parameters, idiot!", name))
                                    .highlight(expr.range())
                                    .note(ct->name().range(), diagnostic::format("As you can see concept `$` was declared without generic parameters.", name))
                                    .build();
                            
                        publisher().publish(diag);
                        mistake = true;
                    }
                    else {
                        bool concrete = true;
                        auto expected = std::dynamic_pointer_cast<ast::generic_clause_declaration>(ct->generic());
                        std::unordered_map<const ast::declaration*, ast::pointer<ast::type>> tsubstitutions;
                        std::unordered_map<const ast::declaration*, constval> csubstitutions;
                        substitutions sub(scopes_.at(ct->generic().get()), nullptr);
                        
                        if (expected->parameters().size() != expr.generics().size()) {
                            auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.range().begin())
                                        .message(diagnostic::format("Concept `$` takes `$` generic parameters, you gave it `$`, idiot!", name, expected->parameters().size(), expr.generics().size()))
                                        .highlight(expr.range())
                                        .note(ct->name().range(), diagnostic::format("As you can see concept `$` was declared with exactly `$` generic parameters.", name, expected->parameters().size()))
                                        .build();
                            
                            publisher().publish(diag);
                            mistake = true;
                        }
                        else for (size_t i = 0; i < expected->parameters().size(); ++i)
                        {
                            if (auto constant = std::dynamic_pointer_cast<ast::generic_const_parameter_declaration>(expected->parameters().at(i))) {
                                if (auto ambiguous = std::dynamic_pointer_cast<ast::type_expression>(expr.generics().at(i))) {
                                    if (ambiguous->is_ambiguous()) {
                                        auto newexpr = ambiguous->as_expression();
                                        newexpr->accept(*this);
                                        expr.generics().at(i) = newexpr;

                                        if (newexpr->annotation().istype) {
                                            auto diag = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(ambiguous->range().begin())
                                                .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                                .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                                .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                .build();
                                
                                            publisher().publish(diag);
                                            mistake = true;
                                        }
                                        else if (!types::assignment_compatible(constant->annotation().type, newexpr->annotation().type)) {
                                            auto diag = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(ambiguous->range().begin())
                                                .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                                .highlight(ambiguous->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                                .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                .build();
                                
                                            publisher().publish(diag);
                                            mistake = true;
                                        }
                                        else {
                                            try { newexpr->annotation().value = evaluate(newexpr); } catch (evaluator::generic_evaluation&) { concrete = false; newexpr->annotation().isparametric = true; }
                                            csubstitutions.emplace(constant.get(), newexpr->annotation().value);
                                            sub.put(constant.get(), newexpr->annotation().value);
                                        }
                                    }
                                    else {
                                        ambiguous->accept(*this);

                                        auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(ambiguous->range().begin())
                                            .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), ambiguous->annotation().type->string()))
                                            .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                            .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                            .build();
                            
                                        publisher().publish(diag);
                                        mistake = true;
                                    }
                                }
                                else {
                                    expr.generics().at(i)->accept(*this);

                                    if (!types::assignment_compatible(constant->annotation().type, expr.generics().at(i)->annotation().type)) {
                                        auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(expr.generics().at(i)->range().begin())
                                            .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), expr.generics().at(i)->annotation().type->string()))
                                            .highlight(expr.generics().at(i)->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                            .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                            .build();
                            
                                        publisher().publish(diag);
                                        mistake = true;
                                    }
                                    else {
                                        try { expr.generics().at(i)->annotation().value = evaluate(expr.generics().at(i)); } catch (evaluator::generic_evaluation&) { concrete = false; expr.generics().at(i)->annotation().isparametric = true; }
                                        csubstitutions.emplace(constant.get(), expr.generics().at(i)->annotation().value);
                                        sub.put(constant.get(), expr.generics().at(i)->annotation().value);
                                    }
                                }
                            }
                            else if (auto type = std::dynamic_pointer_cast<ast::generic_type_parameter_declaration>(expected->parameters().at(i))) {
                                expr.generics().at(i)->annotation().mustvalue = false;
                                expr.generics().at(i)->accept(*this);

                                if (!expr.generics().at(i)->annotation().istype) {
                                    auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(expr.generics().at(i)->range().begin())
                                            .message("I need type parameter here but I found an expression instead, dammit!")
                                            .highlight(expr.generics().at(i)->range(), "expected type")
                                            .note(type->range(), diagnostic::format("Type `$` declares `$` as a type parameter.", name, type->name().lexeme()))
                                            .build();
                            
                                    publisher().publish(diag);
                                    mistake = true;
                                }
                                else {
                                    if (auto typeexpr = std::dynamic_pointer_cast<ast::type_expression>(expr.generics().at(i))) {
                                        if (typeexpr->is_parametric()) concrete = false;
                                    }
                                    
                                    tsubstitutions.emplace(type.get(), expr.generics().at(i)->annotation().type);
                                    sub.put(type.get(), expr.generics().at(i)->annotation().type);
                                }
                            }
                        }
                        // concrete means that a generic type independent from any other parametric type so it can be constructed by monomorphization
                        if (concrete && !mistake) {
                            expr.annotation().value.type = types::boolean();
                            expr.annotation().value.b = instantiate_concept(*ct, sub);
                        }
                    }
                }
                else if (auto generic = std::static_pointer_cast<ast::generic_clause_declaration>(ct->generic())) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(expr.range().begin())
                                .message(diagnostic::format("Concept `$` is parametric so it expects at least `$` generic parameters!", name, generic->parameters().size()))
                                .highlight(expr.range())
                                .note(ct->name().range(), diagnostic::format("Look at concept `$` declaration.", name))
                                .build();
                        
                    publisher().publish(diag);
                    mistake = true;
                }

                if (cyclic) throw cyclic_symbol_error(&expr, ct);
                if (mistake) throw semantic_error();
            }
            else if (!expr.annotation().ispattern) {
                std::ostringstream oss;
                auto similars = this->similars(expr.identifier().lexeme().string(), scope_);
            
                if (!similars.empty()) {
                    oss << "Maybe you meant any of these";
                    for (auto similar : similars) oss << " \\ • " << similar.first;
                }
            
                expr.invalid(true);
                expr.annotation().type = types::unknown();
                error(expr.range(), diagnostic::format("I cannot find any symbol whose name is `$`, are you sure you have declare it, dumb*ss?", name), oss.str());
            }
        }

        // check that this is resolved as a value when a value is expected
        if (expr.annotation().mustvalue && expr.annotation().istype) {
            expr.invalid(true);
            error(expr.range(), diagnostic::format("I was expecting a value but I found type `$` instead!", expr.annotation().type->string()), "", "expected value");
        }

        // check that this is resolved as a type when a type is expected
        if (expr.annotation().musttype && !expr.annotation().istype) {
            expr.invalid(true);
            error(expr.range(), "I was expecting a type but I found value instead!", "", "expected type");
        }
    }

    void checker::visit(const ast::tuple_expression& expr) 
    {
        ast::types components;

        for (auto element : expr.elements()) {
            element->accept(*this);
            components.push_back(element->annotation().type);
        }

        expr.annotation().type = types::tuple(components);
    }

    void checker::visit(const ast::array_expression& expr)
    {
        ast::pointer<ast::type> base = types::unknown();

        if (!expr.elements().empty()) {
            expr.elements().front()->accept(*this);
            base = expr.elements().front()->annotation().type;

            for (size_t i = 1; i < expr.elements().size(); ++i) {
                expr.elements().at(i)->accept(*this);
                auto elemty = expr.elements().at(i)->annotation().type;

                if (!elemty || elemty->category() == ast::type::category::unknown_type);
                else if (!types::assignment_compatible(base, elemty)) {
                    auto diag = diagnostic::builder()
                                .location(expr.elements().at(i)->range().begin())
                                .small(true)
                                .severity(diagnostic::severity::error)
                                .message(diagnostic::format("Array element should have type `$` but I found type `$` instead, dammit!", base->string(), elemty->string()))
                                .highlight(expr.elements().at(i)->range())
                                .note(expr.elements().front()->range(), diagnostic::format("You idiot make me think that element type should be `$` from here.", base->string()))
                                .build();
                    
                    publisher().publish(diag);
                    expr.invalid(true);
                }
                else if (auto implicit = implicit_cast(base, expr.elements().at(i))) {
                    expr.elements().at(i) = implicit;
                }
            }
        }

        expr.annotation().type = types::array(base, expr.elements().size());

        if (!expr.elements().empty() && !dynamic_cast<const ast::contract_statement*>(statement_)) {
            // temporary array must be instantiated and bound to an array object, otherwise it won't be allocated on stack
            auto binding = create_temporary_var(expr);
            binding->annotation().scope = scope_->enclosing();
            // later insertion before use
            pending_insertions.emplace_back(scope_, binding, statement_, false);
            // referencing
            expr.annotation().referencing = binding.get();
        }
    }

    void checker::visit(const ast::array_sized_expression& expr) 
    {
        expr.size()->accept(*this);
        expr.value()->accept(*this);

        try { 
            expr.size()->annotation().value = evaluate(expr.size());
            // TODO: remove, size type will be determined by recursive visit
            expr.size()->annotation().type = expr.size()->annotation().value.type;
        }
        catch (evaluator::generic_evaluation&) {}

        if (expr.size()->annotation().type->category() == ast::type::category::unknown_type) { 
            expr.annotation().type = types::unknown(); 
            throw semantic_error(); 
        }
        else if (expr.size()->annotation().type->category() != ast::type::category::integer_type) {
            expr.annotation().type = types::unknown();
            error(expr.size()->range(), diagnostic::format("I was expecting integer type but I found type `$` instead, you c*nt!", expr.size()->annotation().type->string()), "", "expected integer");
        }
        else if (auto sizety = std::dynamic_pointer_cast<ast::integer_type>(expr.size()->annotation().type)) {
            if (sizety->is_signed()) {
                if (expr.size()->annotation().value.i.value() < 0) {
                    expr.annotation().type = types::unknown();
                    error(expr.size()->range(), "Array size cannot be negative, idiot!");
                }
                else {
                    expr.annotation().type = types::array(expr.value()->annotation().type, expr.size()->annotation().value.i.value());
                }
            } 
            else {
                expr.annotation().type = types::array(expr.value()->annotation().type, expr.size()->annotation().value.u.value());
            }
        }

        if (dynamic_cast<const ast::contract_statement*>(statement_)) return;

        // temporary array must be instantiated and bound to an array object, otherwise it won't be allocated on stack
        auto binding = create_temporary_var(expr);
        binding->annotation().scope = scope_->enclosing();
        // later insertion before use
        pending_insertions.emplace_back(scope_, binding, statement_, false);
        // referencing
        expr.annotation().referencing = binding.get();
    }

    void checker::visit(const ast::parenthesis_expression& expr) 
    {
        expr.expression()->accept(*this);
        expr.annotation() = expr.expression()->annotation();
    }

    void checker::visit(const ast::block_expression& expr) 
    {
        if (expr.annotation().type) return;

        expr.annotation().type = types::unknown();
        // block scope
        auto saved = begin_scope(&expr);
        // if we are in function, property or for loop, then we must test contracts
        ast::pointers<ast::statement> contracts;
        // loop scope
        if (auto loop = scope_->outscope(environment::kind::loop)) {
            if (auto forloop = dynamic_cast<const ast::for_loop_expression*>(loop)) contracts = forloop->contracts();
            else if (auto forrange = dynamic_cast<const ast::for_range_expression*>(loop)) contracts = forrange->contracts();
        }
        // function or property scope
        else if (auto fdecl = scope_->outscope(environment::kind::function)) {
            if (auto function = dynamic_cast<const ast::function_declaration*>(fdecl)) contracts = function->contracts();
            else if (auto property = dynamic_cast<const ast::property_declaration*>(fdecl)) contracts = property->contracts();
        }
        // first pass, only type, constant, variable, function declarations names are registered for forward definitions
        for (ast::pointer<ast::node> stmt : expr.statements()) try {
            // already resolved, like when variable which is automatically casted
            if (auto decl = std::dynamic_pointer_cast<ast::declaration>(stmt)) {
                if (decl->annotation().resolved) continue;
            }
            // register symbols
            if (auto tdecl = std::dynamic_pointer_cast<ast::type_declaration>(stmt)) {
                std::string name = tdecl->name().lexeme().string();
                auto other = scope_->type(name);
                if (workspace()->name == name) {
                    auto diag = diagnostic::builder()
                                .location(tdecl->name().location())
                                .severity(diagnostic::severity::error)
                                .message(diagnostic::format("This type name conflicts with workspace name `$`, c*nt!", name))
                                .highlight(tdecl->name().range(), "conflicting")
                                .build();
                    
                    publisher().publish(diag);
                }
                else if (types::builtin(name)) {
                    tdecl->annotation().type = types::unknown();
                    error(tdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your type, idiot!", name));
                }
                else if (other && other != tdecl.get()) {
                    tdecl->annotation().type = types::unknown();
                    auto diag = diagnostic::builder()
                                .location(tdecl->name().location())
                                .severity(diagnostic::severity::error)
                                .message(diagnostic::format("You have already declared a type named `$`, idiot!", name))
                                .highlight(tdecl->name().range(), "conflicting")
                                .note(other->name().range(), "Here's the homonymous declaration, you f*cker!")
                                .build();
                    
                    publisher().publish(diag);
                }
                else {
                    scope_->type(name, tdecl.get());
                }
            }
            else if (auto cdecl = std::dynamic_pointer_cast<ast::const_declaration>(stmt)) {
                    std::string name = cdecl->name().lexeme().string();
                    auto other = scope_->value(name);
                    if (workspace()->name == name) {
                        auto diag = diagnostic::builder()
                                    .location(cdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("This constant name conflicts with workspace name `$`, c*nt!", name))
                                    .highlight(cdecl->name().range(), "conflicting")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else if (types::builtin(name)) {
                        cdecl->annotation().type = types::unknown();
                        error(cdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your constant, idiot!", name));
                    }
                    else if (other && other != cdecl.get()) {
                        cdecl->annotation().type = types::unknown();
                        auto builder = diagnostic::builder()
                                    .location(cdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .highlight(cdecl->name().range(), "conflicting");
                        
                        if (auto constant = dynamic_cast<const ast::const_declaration*>(other)) {
                            builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                   .note(constant->name().range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        else if (auto constant = dynamic_cast<const ast::const_tupled_declaration*>(other)) {
                            auto underline = std::find_if(constant->names().begin(), constant->names().end(), [&] (token t) { return t.lexeme().compare(cdecl->name().lexeme()) == 0; });
                            builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        else if (auto variable = dynamic_cast<const ast::var_declaration*>(other)) {
                            builder.message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                                   .note(variable->name().range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        else if (auto variable = dynamic_cast<const ast::var_tupled_declaration*>(other)) {
                            auto underline = std::find_if(variable->names().begin(), variable->names().end(), [&] (token t) { return t.lexeme().compare(cdecl->name().lexeme()) == 0; });
                            builder.message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                                .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        
                        publisher().publish(builder.build());
                    }
                    else {
                        scope_->value(name, cdecl.get());
                    }
                }
            else if (auto cdecl = std::dynamic_pointer_cast<ast::const_tupled_declaration>(stmt)) {
                for (auto id : cdecl->names()) {
                    std::string name = id.lexeme().string();
                    auto other = scope_->value(name);
                    // variable declaration is marked as erraneous by default
                    cdecl->invalid(true);
                    
                    if (workspace()->name == name) {
                        auto diag = diagnostic::builder()
                                    .location(id.location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("This constant name conflicts with workspace name `$`, c*nt!", name))
                                    .highlight(id.range(), "conflicting")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else if (types::builtin(name)) {
                        cdecl->annotation().type = types::unknown();
                        error(id.range(), diagnostic::format("You cannot steal primitive type name `$` to create your constant, idiot!", name));
                    }
                    else if (other && other != cdecl.get()) {
                        cdecl->annotation().type = types::unknown();
                        auto builder = diagnostic::builder()
                                    .location(id.location())
                                    .severity(diagnostic::severity::error)
                                    .highlight(id.range(), "conflicting");
                        
                        if (auto constant = dynamic_cast<const ast::const_declaration*>(other)) {
                            builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                .note(constant->name().range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        else if (auto constant = dynamic_cast<const ast::const_tupled_declaration*>(other)) {
                            auto underline = std::find_if(constant->names().begin(), constant->names().end(), [&] (token t) { return t.lexeme().compare(id.lexeme()) == 0; });
                            builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        else if (auto variable = dynamic_cast<const ast::var_declaration*>(other)) {
                            builder.message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                                .note(variable->name().range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        else if (auto variable = dynamic_cast<const ast::var_tupled_declaration*>(other)) {
                            auto underline = std::find_if(variable->names().begin(), variable->names().end(), [&] (token t) { return t.lexeme().compare(id.lexeme()) == 0; });
                            builder.message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                                .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        
                        publisher().publish(builder.build());
                    }
                    else {
                        // the only case in which our variable will be analyzed because there are not conflicts
                        cdecl->invalid(false);
                        scope_->value(name, cdecl.get());
                    }
                }        
            }
            else if (auto vdecl = std::dynamic_pointer_cast<ast::var_declaration>(stmt)) {
                std::string name = vdecl->name().lexeme().string();
                auto other = scope_->value(name);
                // variable declaration is marked as erraneous by default
                vdecl->invalid(true);
                
                if (workspace()->name == name) {
                    auto diag = diagnostic::builder()
                                .location(vdecl->name().location())
                                .severity(diagnostic::severity::error)
                                .message(diagnostic::format("This variable name conflicts with workspace name `$`, c*nt!", name))
                                .highlight(vdecl->name().range(), "conflicting")
                                .build();
                    
                    publisher().publish(diag);
                }
                else if (types::builtin(name)) {
                    vdecl->annotation().type = types::unknown();
                    error(vdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your variable, idiot!", name));
                }
                else if (other && other != vdecl.get()) {
                    vdecl->annotation().type = types::unknown();
                    auto builder = diagnostic::builder()
                                .location(vdecl->name().location())
                                .severity(diagnostic::severity::error)
                                .highlight(vdecl->name().range(), "conflicting");
                    
                    if (auto constant = dynamic_cast<const ast::const_declaration*>(other)) {
                        builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                .note(constant->name().range(), "Here's the homonymous declaration, you f*cker!");
                    }
                    else if (auto constant = dynamic_cast<const ast::const_tupled_declaration*>(other)) {
                        auto underline = std::find_if(constant->names().begin(), constant->names().end(), [&] (token t) { return t.lexeme().compare(vdecl->name().lexeme()) == 0; });
                        builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                            .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                    }
                    else if (auto variable = dynamic_cast<const ast::var_declaration*>(other)) {
                        builder.message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                                .note(variable->name().range(), "Here's the homonymous declaration, you f*cker!");
                    }
                    else if (auto variable = dynamic_cast<const ast::var_tupled_declaration*>(other)) {
                        auto underline = std::find_if(variable->names().begin(), variable->names().end(), [&] (token t) { return t.lexeme().compare(vdecl->name().lexeme()) == 0; });
                        builder.message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                            .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                    }
                    
                    publisher().publish(builder.build());
                }
                else {
                    // the only case in which our variable will be analyzed because there are not conflicts
                    vdecl->invalid(false);
                    scope_->value(name, vdecl.get());
                }
            }
            else if (auto vdecl = std::dynamic_pointer_cast<ast::var_tupled_declaration>(stmt)) {
                for (auto id : vdecl->names()) {
                    std::string name = id.lexeme().string();
                    auto other = scope_->value(name);
                    // variable declaration is marked as erraneous by default
                    vdecl->invalid(true);
                    
                    if (workspace()->name == name) {
                        auto diag = diagnostic::builder()
                                    .location(id.location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("This variable name conflicts with workspace name `$`, c*nt!", name))
                                    .highlight(id.range(), "conflicting")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else if (types::builtin(name)) {
                        vdecl->annotation().type = types::unknown();
                        error(id.range(), diagnostic::format("You cannot steal primitive type name `$` to create your variable, idiot!", name));
                    }
                    else if (other && other != vdecl.get()) {
                        vdecl->annotation().type = types::unknown();
                        auto builder = diagnostic::builder()
                                    .location(id.location())
                                    .severity(diagnostic::severity::error)
                                    .highlight(id.range(), "conflicting");
                        
                        if (auto constant = dynamic_cast<const ast::const_declaration*>(other)) {
                            builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                .note(constant->name().range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        else if (auto constant = dynamic_cast<const ast::const_tupled_declaration*>(other)) {
                            auto underline = std::find_if(constant->names().begin(), constant->names().end(), [&] (token t) { return t.lexeme().compare(id.lexeme()) == 0; });
                            builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        else if (auto variable = dynamic_cast<const ast::var_declaration*>(other)) {
                            builder.message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                                .note(variable->name().range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        else if (auto variable = dynamic_cast<const ast::var_tupled_declaration*>(other)) {
                            auto underline = std::find_if(variable->names().begin(), variable->names().end(), [&] (token t) { return t.lexeme().compare(id.lexeme()) == 0; });
                            builder.message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                                .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        
                        publisher().publish(builder.build());
                    }
                    else {
                        // the only case in which our variable will be analyzed because there are not conflicts
                        vdecl->invalid(false);
                        scope_->value(name, vdecl.get());
                    }
                }        
            }
            else if (auto fdecl = std::dynamic_pointer_cast<ast::function_declaration>(stmt)) {
            std::string name = fdecl->name().lexeme().string();
            auto other = scope_->function(name);
            if (workspace()->name == name) {
                auto diag = diagnostic::builder()
                            .location(fdecl->name().location())
                            .severity(diagnostic::severity::error)
                            .message(diagnostic::format("This function name conflicts with workspace name `$`, c*nt!", name))
                            .highlight(fdecl->name().range(), "conflicting")
                            .build();
                
                publisher().publish(diag);
            }
            else if (types::builtin(name)) {
                fdecl->annotation().type = types::unknown();
                error(fdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your function, idiot!", name));
            }
            else if (other && other != fdecl.get()) {
                fdecl->annotation().type = types::unknown();
                auto diag = diagnostic::builder()
                            .location(fdecl->name().location())
                            .severity(diagnostic::severity::error)
                            .message(diagnostic::format("You have already declared a function named `$`, idiot!", name))
                            .highlight(fdecl->name().range(), "conflicting")
                            .note(static_cast<const ast::function_declaration*>(other)->name().range(), "Here's the homonymous declaration, you f*cker!")
                            .build();
                
                publisher().publish(diag);
            }
            else {
                scope_->function(name, fdecl.get());
            }
        }
        }
        catch (semantic_error& err) { 
            scope_ = saved; 
        }
        // save external pass
        auto old = pass_;
        pass_ = pass::second;
        // in second pass extend are traversed to register nested names for types, costants, functions
        // extended types are fully resolved in other to add nested declarations to their namespaces
        // first generic extension are traversed
        for (ast::pointer<ast::node> stmt : expr.statements()) {
            if (auto extdecl = std::dynamic_pointer_cast<ast::extend_declaration>(stmt)) try {
                if (extdecl->generic()) extdecl->accept(*this); 
            } 
            catch (semantic_error& err) { scope_ = saved; }
            else if (auto bdecl = std::dynamic_pointer_cast<ast::behaviour_declaration>(stmt)) try {
                bdecl->accept(*this);
            }
            catch (semantic_error& err) { scope_ = saved; }
        }
        // then specialized generics
        for (ast::pointer<ast::node> stmt : expr.statements()) {
            if (auto extdecl = std::dynamic_pointer_cast<ast::extend_declaration>(stmt)) try {
                if (!extdecl->generic()) extdecl->accept(*this); 
            } 
            catch (semantic_error& err) { scope_ = saved; }
        }
        // restore external pass
        pass_ = old;
        // third pass for constructing types, constants
        // workspace scope
        for (auto pair : scope_->types()) try {
            // if not resolved then the type is traversed
            if (!pair.second->annotation().visited) pair.second->accept(*this);
        }
        catch (cyclic_symbol_error& err) {
            // recursive type error is printed
            publisher().publish(err.diagnostic());
            // if we catch an exception then we consider type resolved, at least to minimize errors
            // or false positives in detecting recursive cycles
            pair.second->annotation().resolved = true;
            // restore scope
            scope_ = saved;
        }
        catch (semantic_error& err) {
            // if we catch an exception then we consider type resolved, at least to minimize errors
            // or false positives in detecting recursive cycles
            pair.second->annotation().resolved = true;
            // restore scope
            scope_ = saved;
        }
        // all workspace constants are fully checked
        for (auto pair : scope_->values()) try {
            // variables are analyzed later
            if (dynamic_cast<const ast::var_declaration*>(pair.second) || dynamic_cast<const ast::var_tupled_declaration*>(pair.second)) continue;
            // if not resolved then the type is traversed
            if (!pair.second->annotation().visited) pair.second->accept(*this);
        }
        catch (cyclic_symbol_error& err) {
            // recursive type error is printed
            publisher().publish(err.diagnostic());
            // if we catch an exception then we consider type resolved, at least to minimize errors
            // or false positives in detecting recursive cycles
            pair.second->annotation().resolved = true;
            // restore scope
            scope_ = saved;
        }
        catch (semantic_error& err) {
            // if we catch an exception then we consider type resolved, at least to minimize errors
            // or false positives in detecting recursive cycles
            pair.second->annotation().resolved = true;
            // restore scope
            scope_ = saved;
        }
        // third pass
        pass_ = pass::third;
        // third pass is used to construct nested types, constants, functions and properties inside extend blocks
        // first generic extension are traversed
        for (ast::pointer<ast::node> stmt : expr.statements()) {
            if (auto extdecl = std::dynamic_pointer_cast<ast::extend_declaration>(stmt)) try {
                if (extdecl->generic()) extdecl->accept(*this); 
            } 
            catch (semantic_error& err) { scope_ = saved; }
            else if (auto bdecl = std::dynamic_pointer_cast<ast::behaviour_declaration>(stmt)) try {
                bdecl->accept(*this);
            }
            catch (semantic_error& err) { scope_ = saved; }
        }
        // then specialized generics
        for (ast::pointer<ast::node> stmt : expr.statements()) {
            if (auto extdecl = std::dynamic_pointer_cast<ast::extend_declaration>(stmt)) try {
                if (!extdecl->generic()) extdecl->accept(*this); 
            } 
            catch (semantic_error& err) { scope_ = saved; }
        }
        // restore external pass
        pass_ = old;
        // remove variables names to avoid conflicts
        std::set<std::string> vars_to_remove;
        for (auto pair : scope_->values()) if (dynamic_cast<const ast::var_declaration*>(pair.second) || dynamic_cast<const ast::var_tupled_declaration*>(pair.second)) vars_to_remove.insert(pair.first);
        for (auto var : vars_to_remove) scope_->values().erase(var);
        // contracts at the beginning
        for (auto contract : contracts) {
            auto prev = statement_;
            statement_ = contract.get();
            if (std::static_pointer_cast<ast::contract_statement>(contract)->is_require()) try { contract->accept(*this); } catch (abort_error&) { throw; } catch (...) { statement_ = prev; throw; }
            statement_ = prev;
        }
        // fourth pass is used to check all others statements including
        // functions
        // tests
        // extern blocks
        // variables
        // type of the whole expression
        for (ast::pointer<ast::statement> stmt : expr.statements()) {
            // these were already fully checked
            if (std::dynamic_pointer_cast<ast::type_declaration>(stmt) || std::dynamic_pointer_cast<ast::const_declaration>(stmt) || std::dynamic_pointer_cast<ast::const_tupled_declaration>(stmt) || std::dynamic_pointer_cast<ast::extend_declaration>(stmt)) continue;
            // already resolved, like when variable which is automatically casted
            if (auto decl = std::dynamic_pointer_cast<ast::declaration>(stmt)) {
                if (decl->annotation().resolved) continue;
            }
            
            auto prev = statement_;
            // current statement
            statement_ = stmt.get();
            // other statement to check
            try {
                stmt->accept(*this); 
            }
            catch (cyclic_symbol_error& err) {
                // recursive type error is printed
                publisher().publish(err.diagnostic());
                // if we catch an exception then we consider type resolved, at least to minimize errors
                // or false positives in detecting recursive cycles
                if (auto decl = std::dynamic_pointer_cast<ast::declaration>(stmt)) decl->annotation().resolved = true;
                // restore scope
                scope_ = saved;
            }
            catch (semantic_error& err) {
                // if we catch an exception then we consider type resolved, at least to minimize errors
                // or false positives in detecting recursive cycles
                if (auto decl = std::dynamic_pointer_cast<ast::declaration>(stmt)) decl->annotation().resolved = true;
                // restore scope
                scope_ = saved;
            }
            // set current statement
            statement_ = prev;
        }
        // contracts at exit
        for (auto contract : contracts) {
            auto prev = statement_;
            statement_ = contract.get();
            if (!std::static_pointer_cast<ast::contract_statement>(contract)->is_require()) try { contract->accept(*this); } catch (abort_error&) { throw; } catch (...) { statement_ = prev; throw; }
            statement_ = prev;
        }

        end_scope();

        // check if return type of whole block were set
        if (!expr.annotation().type || expr.annotation().type->category() == ast::type::category::unknown_type) {
            // if no statements, then unit type
            if (expr.statements().empty()) expr.annotation().type = types::unit();
            // otherwise, if last statement contains an expression, then its type is used
            else if (auto stmt = std::dynamic_pointer_cast<ast::expression_statement>(expr.statements().back())) {
                expr.annotation().type = stmt->expression()->annotation().type;
                // set exprnode
                if (auto block = std::dynamic_pointer_cast<ast::block_expression>(stmt->expression())) {
                    expr.exprnode() = block->exprnode();
                }
                else expr.exprnode() = stmt.get();
            }
            // return statement
            else if (auto stmt = std::dynamic_pointer_cast<ast::return_statement>(expr.statements().back())) {
                if (stmt->expression()) {
                    expr.annotation().type = stmt->expression()->annotation().type;
                    // set exprnode
                    if (auto block = std::dynamic_pointer_cast<ast::block_expression>(stmt->expression())) {
                        expr.exprnode() = block->exprnode();
                    }
                    else expr.exprnode() = stmt.get();
                }
                else {
                    expr.annotation().type = types::unit();
                    expr.exprnode() = stmt.get();
                }
            }
            // otherwise last statement was not an expression, this means unit type
            else expr.annotation().type = types::unit();
        }

        // set expression node for block
        if (!expr.exprnode()) expr.exprnode() = &expr;

        // set for last
        if (!expr.annotation().type) expr.annotation().type = types::unknown();
    }

    void checker::visit(const ast::function_expression& expr)
    {
        std::unordered_map<std::string, token> names;
        ast::types formals;
        // open function scope to declare parameters and local variables
        auto saved = begin_scope(&expr);

        for (auto pdecl : expr.parameters()) {
            pdecl->accept(*this);
            formals.push_back(pdecl->annotation().type);
            auto param = std::dynamic_pointer_cast<ast::parameter_declaration>(pdecl);
            auto other = names.find(param->name().lexeme().string());
            if (other != names.end()) {
                auto diag = diagnostic::builder()
                            .location(param->name().location())
                            .severity(diagnostic::severity::error)
                            .message(diagnostic::format("You have already declared a parameter named `$`, idiot!", param->name().lexeme()))
                            .highlight(param->name().range(), "conflicting")
                            .note(other->second.range(), "Here's the homonymous parameter, you f*cker!")
                            .build();
                
                publisher().publish(diag);
            }
            else {
                names.emplace(param->name().lexeme().string(), param->name());
                scope_->value(param->name().lexeme().string(), param.get());
            }
        }

        if (expr.return_type_expression()) try { expr.return_type_expression()->accept(*this); } catch (semantic_error& err) { scope_ = saved; }

        auto result_type = expr.return_type_expression() ? expr.return_type_expression()->annotation().type : types::unit();
        // type is annoted before body is checked
        expr.annotation().type = types::function(formals, result_type);

        if (expr.body())  try { 
            expr.body()->accept(*this); 
            if (auto block = std::dynamic_pointer_cast<ast::block_expression>(expr.body())) {
                if (auto exprstmt = dynamic_cast<const ast::expression_statement*>(block->exprnode())) {
                    if (result_type->category() != ast::type::category::unknown_type &&
                        exprstmt->expression()->annotation().type->category() != ast::type::category::unknown_type &&
                        !types::compatible(result_type, types::unit()) &&
                        !types::assignment_compatible(result_type, exprstmt->expression()->annotation().type)) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(exprstmt->expression()->range().begin())
                                    .message(diagnostic::format("I was expecting return type `$` but I found type `$` instead, idiot!", result_type->string(), exprstmt->expression()->annotation().type->string()))
                                    .highlight(exprstmt->expression()->range(), diagnostic::format("expected $", result_type->string()));

                        if (expr.return_type_expression()) builder.note(expr.return_type_expression()->range(), diagnostic::format("You can see that return type of function expression is `$`.", result_type->string()));
                        else builder.insertion(source_range(expr.body()->range().begin(), 1), diagnostic::format("$ ", exprstmt->expression()->annotation().type->string()), "Try adding return type to definition.");

                        publisher().publish(builder.build());
                        expr.invalid(true);
                    }
                    else if (auto implicit = implicit_cast(result_type, exprstmt->expression())) {
                        exprstmt->expression() = implicit;
                    }
                }
            }
            else if (auto exprbody = std::dynamic_pointer_cast<ast::expression>(expr.body())) {
                if (result_type->category() != ast::type::category::unknown_type &&
                    exprbody->annotation().type->category() != ast::type::category::unknown_type &&
                    !types::assignment_compatible(result_type, exprbody->annotation().type)) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(exprbody->range().begin())
                                .message(diagnostic::format("I was expecting return type `$` but I found type `$` instead, idiot!", result_type->string(), exprbody->annotation().type->string()))
                                .highlight(exprbody->range(), diagnostic::format("expected $", result_type->string()));

                    if (expr.return_type_expression()) builder.note(expr.return_type_expression()->range(), diagnostic::format("You can see that return type of function expression is `$`.", result_type->string()));
                    else builder.replacement(expr.result_range(), diagnostic::format("$ ", exprbody->annotation().type->string()), "Try adding return type to definition.");

                    publisher().publish(builder.build());
                    expr.invalid(true);
                }
            }
        } 
        catch (semantic_error& err) { scope_ = saved; }

        // end function scope
        end_scope();
    }

    void checker::visit(const ast::postfix_expression& expr)
    {
        if (expr.invalid()) throw semantic_error();

        expr.expression()->accept(*this);

        if (expr.expression()->annotation().type->category() == ast::type::category::unknown_type) throw semantic_error();

        auto rtype = expr.expression()->annotation().type;

        switch (expr.postfix().kind()) {
            case token::kind::plus_plus:
                if (rtype->category() == ast::type::category::integer_type) {
                    expr.annotation().type = rtype;
                    immutability(expr);
                }
                else {
                    auto diag = diagnostic::builder()
                                .location(expr.postfix().location())
                                .small(true)
                                .severity(diagnostic::severity::error)
                                .message(diagnostic::format("Increment operator `++` requires an integer type, I found type `$` instead!", rtype->string()))
                                .highlight(expr.postfix().range())
                                .highlight(expr.expression()->range(), diagnostic::highlighter::mode::light)
                                .build();
                    
                    publisher().publish(diag);
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    throw semantic_error();
                }
                break;
            case token::kind::minus_minus:
                if (rtype->category() == ast::type::category::integer_type) {
                    expr.annotation().type = rtype;
                    immutability(expr);    
                }
                else {
                    auto diag = diagnostic::builder()
                                .location(expr.postfix().location())
                                .small(true)
                                .severity(diagnostic::severity::error)
                                .message(diagnostic::format("Decrement operator `--` requires an integer type, I found type `$` instead!", rtype->string()))
                                .highlight(expr.postfix().range())
                                .highlight(expr.expression()->range(), diagnostic::highlighter::mode::light)
                                .build();
                    
                    publisher().publish(diag);
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    throw semantic_error();
                }
                break;
            default:
                expr.annotation().type = types::unknown();
                break;
        }
    }
    
    void checker::visit(const ast::call_expression& expr) 
    {
        // test for primitive calls like `__format`
        if (auto identifier = std::dynamic_pointer_cast<ast::identifier_expression>(expr.callee())) {
            if (!identifier->is_generic() && identifier->identifier().lexeme().string() == "__format") {
                ast::types formals;
                unsigned i = 0;

                for (auto arg : expr.arguments()) {
                    const ast::property_declaration* conversion = nullptr;

                    arg->accept(*this);
                    formals.push_back(arg->annotation().type);

                    // checks if it is convertible to string
                    if (!is_string_convertible(arg->annotation().type, conversion)) {
                        auto tname = arg->annotation().type->string();
                        auto builder = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(arg->range().begin())
                                        .message(diagnostic::format("Type `$` cannot be formatted as a string since it's missing conversion procedure!", tname))
                                        .highlight(arg->range(), "missing string conversion")
                                        .explanation(diagnostic::format("I suggest providing string conversion property `str`, like this \\ \\ `extend $ { .str(self: $) string {...} }`", tname, tname));

                        publisher().publish(builder.build());
                    }
                    // implicit conversion to string calling 'str' property
                    else if (conversion) {
                        auto property = ast::create<ast::identifier_expression>(source_range(), token::builder().artificial(true).kind(token::kind::identifier).lexeme(utf8::span::builder().concat("str").build()).build(), ast::pointers<ast::expression>());
                        property->annotation().type = conversion->annotation().type;
                        property->annotation().referencing = conversion;
                        expr.arguments().at(i) = ast::create<ast::member_expression>(expr.arguments().at(i)->range(), expr.arguments().at(i), property);
                        expr.arguments().at(i)->annotation().type = types::string();
                    }

                    ++i;
                }

                if (auto implicit = implicit_cast(types::string(), expr.arguments().front())) expr.arguments().front() = implicit;
                expr.annotation().type = types::string();
                expr.callee()->annotation().type = types::function(formals, types::string());
                return;
            }
        }
        // normal call
        if (auto member = std::dynamic_pointer_cast<ast::member_expression>(expr.callee())) {
            // enable resolution of callee as a type for costructing tuples or structures
            member->annotation().mustvalue = false;
            // generics are deduced at this level from function call, so we don't need them to be resolved from visit(identifier_expression&)
            member->member()->annotation().deduce = false;
            member->accept(*this);
            if (!member->annotation().type || member->annotation().type->category() == ast::type::category::unknown_type) {
                expr.invalid(true);
                expr.annotation().type = types::unknown();
                throw semantic_error();
            }
            else if (member->annotation().istype) {
                switch (member->annotation().type->category()) {
                case ast::type::category::tuple_type:
                {
                    auto tuple_type = std::static_pointer_cast<ast::tuple_type>(member->annotation().type);

                    if (expr.arguments().size() != tuple_type->components().size()) {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(member->range(), diagnostic::format("Tuple type `$` expects `$` components but you gave it `$`, pr*ck!", member->annotation().type->string(), tuple_type->components().size(), expr.arguments().size()));
                    }
                    
                    for (std::size_t i = 0; i < tuple_type->components().size(); ++i) {
                        expr.arguments().at(i)->accept(*this);
                        if (tuple_type->components().at(i)->category() == ast::type::category::unknown_type || expr.arguments().at(i)->annotation().type->category() == ast::type::category::unknown_type) continue;
                        else if (types::assignment_compatible(tuple_type->components().at(i), expr.arguments().at(i)->annotation().type)) {
                            if (auto implicit = implicit_cast(tuple_type->components().at(i), expr.arguments().at(i))) {
                                expr.arguments().at(i) = implicit;
                            }
                            else {
                                expr.arguments().at(i)->annotation().type = tuple_type->components().at(i);
                            }
                        }
                        else {
                            auto builder = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.arguments().at(i)->range().begin())
                                        .message(diagnostic::format("This component must have type `$`, but I found `$`, idiot!", tuple_type->components().at(i)->string(), expr.arguments().at(i)->annotation().type->string()))
                                        .highlight(expr.arguments().at(i)->range(), diagnostic::format("expected $", tuple_type->components().at(i)->string()));

                            publisher().publish(builder.build());
                        }
                    }
                    
                    expr.annotation().type = member->annotation().type;

                    break;
                }
                case ast::type::category::structure_type:
                {
                    auto structure_type = std::static_pointer_cast<ast::structure_type>(member->annotation().type);

                    if (expr.arguments().size() != structure_type->fields().size()) {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(member->range(), diagnostic::format("Structure type `$` expects `$` fields but you gave it `$`, pr*ck!", member->annotation().type->string(), structure_type->fields().size(), expr.arguments().size()));
                    }
                    
                    for (std::size_t i = 0; i < structure_type->fields().size(); ++i) {
                        expr.arguments().at(i)->accept(*this);

                        if (structure_type->declaration()) {
                            auto item = static_cast<const ast::record_declaration*>(structure_type->declaration())->fields().at(i);
                            std::string name = std::static_pointer_cast<ast::field_declaration>(item)->name().lexeme().string();
                            
                            if ((item->is_hidden() || name[0] == '_') && !scope_->has_ancestor_scope(scopes_.at(structure_type->declaration()))) {
                                auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(expr.arguments().at(i)->range().begin())
                                            .message(diagnostic::format("You can't initialize hidden field `$`, dammit!", name))
                                            .highlight(expr.arguments().at(i)->range(), "inaccessible")
                                            .note(static_cast<const ast::field_declaration*>(item.get())->name().range(), diagnostic::format("Look at field `$` declaration, idiot.", name))
                                            .build();

                                publisher().publish(diag);
                                expr.invalid(true);
                            }
                        }

                        if (structure_type->fields().at(i).type->category() == ast::type::category::unknown_type || expr.arguments().at(i)->annotation().type->category() == ast::type::category::unknown_type) continue;
                        else if (types::assignment_compatible(structure_type->fields().at(i).type, expr.arguments().at(i)->annotation().type)) {
                            if (auto implicit = implicit_cast(structure_type->fields().at(i).type, expr.arguments().at(i))) {
                                expr.arguments().at(i) = implicit;
                            }
                            else {
                                expr.arguments().at(i)->annotation().type = structure_type->fields().at(i).type;
                            }
                        }
                        else {
                            auto builder = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.arguments().at(i)->range().begin())
                                        .message(diagnostic::format("Field `$` must have type `$`, but I found `$`, idiot!", structure_type->fields().at(i).name, structure_type->fields().at(i).type->string(), expr.arguments().at(i)->annotation().type->string()))
                                        .highlight(expr.arguments().at(i)->range(), diagnostic::format("expected $", structure_type->fields().at(i).type->string()));

                            publisher().publish(builder.build());
                        }
                    }
                    
                    expr.annotation().type = member->annotation().type;
                    
                    break;
                }
                default:
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    if (expr.callee()->annotation().type->category() == ast::type::category::behaviour_type) error(expr.callee()->range(), diagnostic::format("Behaviour type `$` cannot be instantiated, idiot!", expr.callee()->annotation().type->string()));
                    else error(member->range(), diagnostic::format("I was expecting a callable value but I found type `$` instead!", member->annotation().type->string()), "", "expected value");
                }
            }
            else if (member->annotation().type->category() != ast::type::category::function_type) {
                expr.invalid(true);
                expr.annotation().type = types::unknown();
                error(expr.callee()->range(), diagnostic::format("This expression must be callable but it has type `$`, idiot!", expr.callee()->annotation().type->string()), "", "expected function");
            }
            else {
                auto fntype = std::static_pointer_cast<ast::function_type>(expr.callee()->annotation().type);
                auto fn = expr.callee()->annotation().referencing;
                auto name = fn->kind() == ast::kind::function_declaration ? static_cast<const ast::function_declaration*>(fn)->name() : static_cast<const ast::property_declaration*>(fn)->name();
                ast::pointer<ast::type> variadic = nullptr;
                expr.annotation().type = fntype->result();
                // if function is called from type namespace then it expects all its arguments
                if (member->expression()->annotation().istype) {
                    if (auto fndecl = dynamic_cast<const ast::function_declaration*>(fn)) {
                        if (!fndecl->parameters().empty() && std::dynamic_pointer_cast<ast::parameter_declaration>(fndecl->parameters().back())->is_variadic()) variadic = std::dynamic_pointer_cast<ast::slice_type>(fndecl->parameters().back()->annotation().type)->base();
                    }

                    if (expr.arguments().size() < fntype->formals().size() || (expr.arguments().size() > fntype->formals().size() && !variadic)) {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr.callee()->range(), diagnostic::format("This function expects `$` arguments but you gave it `$`, pr*ck!", fntype->formals().size(), expr.arguments().size()));
                    }

                    auto identifier = std::dynamic_pointer_cast<ast::identifier_expression>(member->member());
                    bool mistake = false, concrete = true;
                    substitutions sub(scope_, nullptr);

                    // checks if there were provided any generic arguments
                    if (identifier && identifier->is_generic()) {
                        // if there were not generic parameters in declaration, then we have an error
                        if (fn->kind() == ast::kind::property_declaration || !static_cast<const ast::function_declaration*>(fn)->generic()) {
                            auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.range().begin())
                                        .message(diagnostic::format("$ `$` doesn't take any generic parameters, idiot!", fn->kind() == ast::kind::function_declaration ? "Function" : "Property", name.lexeme()))
                                        .highlight(expr.range())
                                        .note(name.range(), diagnostic::format("As you can see $ `$` was declared without generic parameters.", fn->kind() == ast::kind::function_declaration ? "function" : "property", name.lexeme()))
                                        .build();
                                
                            publisher().publish(diag);
                            mistake = true;
                        }
                        else {
                            auto expected = std::dynamic_pointer_cast<ast::generic_clause_declaration>(static_cast<const ast::function_declaration*>(fn)->generic());
                            
                            if (expected->parameters().size() < identifier->generics().size()) {
                                auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(identifier->range().begin())
                                            .message(diagnostic::format("$ `$` takes no more than `$` generic parameters, you gave it `$`, idiot!", fn->kind() == ast::kind::function_declaration ? "Function" : "Property", name.lexeme(), expected->parameters().size(), identifier->generics().size()))
                                            .highlight(identifier->range())
                                            .note(name.range(), diagnostic::format("As you can see $ `$` was declared with exactly `$` generic parameters.", fn->kind() == ast::kind::function_declaration ? "function" : "property", name.lexeme(), expected->parameters().size()))
                                            .build();
                                
                                publisher().publish(diag);
                                mistake = true;
                            }
                            else for (size_t i = 0; i < identifier->generics().size(); ++i)
                            {
                                if (auto constant = std::dynamic_pointer_cast<ast::generic_const_parameter_declaration>(expected->parameters().at(i))) {
                                    if (auto ambiguous = std::dynamic_pointer_cast<ast::type_expression>(identifier->generics().at(i))) {
                                        if (ambiguous->is_ambiguous()) {
                                            auto newexpr = ambiguous->as_expression();
                                            newexpr->accept(*this);
                                            identifier->generics().at(i) = newexpr;

                                            if (newexpr->annotation().istype) {
                                                auto diag = diagnostic::builder()
                                                    .severity(diagnostic::severity::error)
                                                    .location(ambiguous->range().begin())
                                                    .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                                    .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                                    .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                    .build();
                                    
                                                publisher().publish(diag);
                                                mistake = true;
                                            }
                                            else if (!types::compatible(constant->annotation().type, newexpr->annotation().type)) {
                                                auto diag = diagnostic::builder()
                                                    .severity(diagnostic::severity::error)
                                                    .location(ambiguous->range().begin())
                                                    .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                                    .highlight(ambiguous->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                                    .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                    .build();
                                    
                                                publisher().publish(diag);
                                                mistake = true;
                                            }
                                            else {
                                                try { newexpr->annotation().value = evaluate(newexpr); } catch (evaluator::generic_evaluation&) { concrete = false; newexpr->annotation().isparametric = true; }
                                                sub.put(constant.get(), newexpr->annotation().value);
                                            }
                                        }
                                        else {
                                            ambiguous->accept(*this);

                                            auto diag = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(ambiguous->range().begin())
                                                .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), ambiguous->annotation().type->string()))
                                                .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                                .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                .build();
                                
                                            publisher().publish(diag);
                                            mistake = true;
                                        }
                                    }
                                    else {
                                        identifier->generics().at(i)->accept(*this);

                                        if (!types::compatible(constant->annotation().type, identifier->generics().at(i)->annotation().type)) {
                                            auto diag = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(identifier->generics().at(i)->range().begin())
                                                .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), identifier->generics().at(i)->annotation().type->string()))
                                                .highlight(identifier->generics().at(i)->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                                .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                .build();
                                
                                            publisher().publish(diag);
                                            mistake = true;
                                        }
                                        else {
                                            try { identifier->generics().at(i)->annotation().value = evaluate(identifier->generics().at(i)); } catch (evaluator::generic_evaluation&) { concrete = false; identifier->generics().at(i)->annotation().isparametric = true; }
                                            sub.put(constant.get(), identifier->generics().at(i)->annotation().value);
                                        }
                                    }
                                }
                                else if (auto type = std::dynamic_pointer_cast<ast::generic_type_parameter_declaration>(expected->parameters().at(i))) {
                                    identifier->generics().at(i)->annotation().mustvalue = false;
                                    identifier->generics().at(i)->accept(*this);

                                    if (!identifier->generics().at(i)->annotation().istype) {
                                        auto diag = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(identifier->generics().at(i)->range().begin())
                                                .message("I need type parameter here but I found an expression instead, dammit!")
                                                .highlight(identifier->generics().at(i)->range(), "expected type")
                                                .note(type->range(), diagnostic::format("Type `$` declares `$` as a type parameter.", name, type->name().lexeme()))
                                                .build();
                                
                                        publisher().publish(diag);
                                        mistake = true;
                                    }
                                    else {
                                        if (auto typeexpr = std::dynamic_pointer_cast<ast::type_expression>(identifier->generics().at(i))) {
                                            if (typeexpr->is_parametric()) concrete = false;
                                        }
                                        
                                        sub.put(type.get(), identifier->generics().at(i)->annotation().type);
                                    }
                                }
                            }
                        
                            if (mistake) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                throw semantic_error();
                            }
                        }
                    }

                    if (auto fdecl = dynamic_cast<const ast::function_declaration*>(identifier->annotation().referencing)) {
                        if (auto generic = std::dynamic_pointer_cast<ast::generic_clause_declaration>(fdecl->generic())) {
                            // matcher will contain all generics bindings
                            ast::type_matcher::result match;

                            sub.context(scopes_.at(generic.get()));

                            // first explicit generic arguments are added to match list
                            for (std::size_t i = 0; i < identifier->generics().size(); ++i) {
                                if (auto constparam = std::dynamic_pointer_cast<ast::generic_const_parameter_declaration>(generic->parameters().at(i))) {
                                    match.value(constparam->name().lexeme().string(), identifier->generics().at(i)->annotation().value);
                                }
                                else if (auto typeparam = std::dynamic_pointer_cast<ast::generic_type_parameter_declaration>(generic->parameters().at(i))) {
                                    match.type(typeparam->name().lexeme().string(), identifier->generics().at(i)->annotation().type);
                                }
                            }

                            // then we try to deduce generic arguments from function arguments
                            for (std::size_t i = 0; i < fdecl->parameters().size(); ++i) {
                                expr.arguments().at(i)->accept(*this);

                                ast::type_matcher matcher(expr.arguments().at(i), fdecl->parameters().at(i)->annotation().type, publisher());
                                if (!matcher.match(expr.arguments().at(i)->annotation().type, match, std::static_pointer_cast<ast::parameter_declaration>(fdecl->parameters().at(i))->is_variadic()) && !match.duplication) {
                                    auto param = std::static_pointer_cast<ast::parameter_declaration>(fdecl->parameters().at(i));
                                    auto builder = diagnostic::builder()
                                                .location(expr.arguments().at(i)->range().begin())
                                                .severity(diagnostic::severity::error)
                                                .small(true)
                                                .highlight(expr.arguments().at(i)->range(), diagnostic::format("expected $", param->annotation().type->string()))
                                                .message(diagnostic::format("Type mismatch between argument and parameter, found `$` and `$`.", expr.arguments().at(i)->annotation().type->string(), param->annotation().type->string()))
                                                .note(param->name().range(), diagnostic::format("Have a look at parameter `$` idiot.", param->name().lexeme()));
                                    
                                    publisher().publish(builder.build());
                                }
                            }

                            if (!match) {
                                expr.annotation().type = types::unknown();
                                expr.invalid(true);
                                throw semantic_error();
                            }
                            else {
                                for (auto generic_parameter : generic->parameters()) {
                                    token name = generic_parameter->kind() == ast::kind::generic_const_parameter_declaration ? std::static_pointer_cast<ast::generic_const_parameter_declaration>(generic_parameter)->name() : std::static_pointer_cast<ast::generic_type_parameter_declaration>(generic_parameter)->name();
                                    auto found = match.bindings.find(name.lexeme().string());
                                    if (found == match.bindings.end() || (generic_parameter->kind() == ast::kind::generic_type_parameter_declaration && found->second.type->category() == ast::type::category::unknown_type)) {
                                        std::ostringstream oss;

                                        oss << fdecl->name().lexeme() << "!(" << name.lexeme() << ")";

                                        auto builder = diagnostic::builder()
                                                    .location(expr.range().begin())
                                                    .severity(diagnostic::severity::error)
                                                    .small(true)
                                                    .highlight(expr.callee()->range())
                                                    .highlight(name.range(), diagnostic::highlighter::mode::light)
                                                    .message(diagnostic::format("I can't deduce $ parameter `$` in this function call, dammit!", generic_parameter->kind() == ast::kind::generic_const_parameter_declaration ? "value" : "type", name.lexeme()))
                                                    .replacement(expr.callee()->range(), oss.str(), diagnostic::format("Try specifying argument `$` explicit to make it clear!", name.lexeme()));
                                        
                                        publisher().publish(builder.build());
                                        concrete = false;
                                    }
                                    else if (generic_parameter->kind() == ast::kind::generic_const_parameter_declaration) {
                                        sub.put(generic_parameter.get(), found->second.value);
                                    }
                                    else {
                                        sub.put(generic_parameter.get(), found->second.type);
                                    }
                                }
                                /*
                                std::cout << "TYPE MATCHER BINDINGS " << fdecl->name().lexeme() << '\n';
                                for (auto binding : match.bindings) {
                                    if (binding.second.kind == ast::type_matcher::parameter::kind::type) {
                                        std::cout << binding.first << " = " << binding.second.type->string() << '\n';
                                    }
                                    else {
                                        std::cout << binding.first << " = " << binding.second.value << '\n';
                                    }
                                }
                                */
                                if (!concrete) {
                                    expr.invalid(true);
                                    expr.annotation().type = types::unknown();
                                    throw semantic_error();
                                }

                                auto instantiated = instantiate_function(*fdecl, sub);
                                identifier->annotation().referencing = instantiated.get();
                                fntype = std::dynamic_pointer_cast<ast::function_type>(instantiated->annotation().type);
                                expr.annotation().type = fntype->result();
                            }
                        }
                        // checks if there were provided any generic arguments
                        else if (identifier && identifier->is_generic()) {
                            auto fn = identifier->annotation().referencing;
                            auto name = fn->kind() == ast::kind::function_declaration ? static_cast<const ast::function_declaration*>(fn)->name() : static_cast<const ast::property_declaration*>(fn)->name();
                            auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.range().begin())
                                        .message(diagnostic::format("$ `$` doesn't take any generic parameters, idiot!", fn->kind() == ast::kind::function_declaration ? "Function" : "Property", name.lexeme()))
                                        .highlight(expr.callee()->range())
                                        .note(name.range(), diagnostic::format("As you can see $ `$` was declared without generic parameters.", fn->kind() == ast::kind::function_declaration ? "function" : "property", name.lexeme()))
                                        .build();
                                
                            publisher().publish(diag);

                            expr.invalid(true);
                            expr.annotation().type = types::unknown();
                            throw semantic_error();
                        }
                    }
                    else if (auto vdecl = dynamic_cast<const ast::var_declaration*>(identifier->annotation().referencing)) {
                        if (identifier && identifier->is_generic()) {
                            auto name = vdecl->name();
                            auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.range().begin())
                                        .message(diagnostic::format("Callable `$` doesn't take any generic parameters, idiot!", name.lexeme()))
                                        .highlight(expr.callee()->range())
                                        .note(name.range(), diagnostic::format("As you can see callable `$` was declared without generic parameters.", name.lexeme()))
                                        .build();
                                
                            publisher().publish(diag);
                            expr.invalid(true);
                            expr.annotation().type = types::unknown();
                            throw semantic_error();
                        }
                    }
                    
                    ast::pointers<ast::declaration> params;

                    if (fn->kind() == ast::kind::function_declaration) params = static_cast<const ast::function_declaration*>(fn)->parameters();
                    else if (fn->kind() == ast::kind::property_declaration) params = static_cast<const ast::property_declaration*>(fn)->parameters();

                    for (std::size_t i = 0; i < fntype->formals().size(); ++i) {
                        // variadic case
                        if (variadic && variadic->category() != ast::type::category::unknown_type && i == fntype->formals().size() - 1) {
                            variadic = std::static_pointer_cast<ast::slice_type>(fntype->formals().back())->base();
                            ast::pointers<ast::expression> elements;
                            
                            for (std::size_t j = i; j < expr.arguments().size(); ++j) {
                                expr.arguments().at(j)->accept(*this);
                                if (expr.arguments().at(j)->annotation().type->category() == ast::type::category::unknown_type) continue;
                                else if (types::assignment_compatible(variadic, expr.arguments().at(j)->annotation().type)) {
                                    if (auto implicit = implicit_cast(variadic, expr.arguments().at(j))) {
                                        expr.arguments().at(j) = implicit;
                                    }
                                    else {
                                        expr.arguments().at(j)->annotation().type = variadic;
                                    }

                                    if (fn) test_immutable_assignment(*std::dynamic_pointer_cast<ast::parameter_declaration>(params.back()), *expr.arguments().at(j));
                                }
                                else {
                                    auto builder = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(expr.arguments().at(j)->range().begin())
                                                .message(diagnostic::format("This argument must have type `$`, but I found `$`, idiot!", variadic->string(), expr.arguments().at(j)->annotation().type->string()))
                                                .highlight(expr.arguments().at(j)->range(), diagnostic::format("expected $", variadic->string()));

                                    publisher().publish(builder.build());
                                }
                                elements.push_back(expr.arguments().at(j));
                            }
                            
                            auto array = ast::create<ast::array_expression>(source_range(expr.arguments().at(i)->range().begin(), expr.arguments().back()->range().end()), elements);
                            array->annotation().type = types::array(variadic, expr.arguments().size() - i);

                            auto binding = create_temporary_var(*array);
                            binding->annotation().scope = scope_->enclosing();
                            pending_insertions.emplace_back(scope_, binding, statement_, false);

                            array->annotation().referencing = binding.get();
                            
                            for (std::size_t j = expr.arguments().size() - 1; j > i; --j) expr.arguments().pop_back();

                            expr.arguments().back() = implicit_forced_cast(types::slice(variadic), array);
                        }
                        else {
                            expr.arguments().at(i)->accept(*this);
                            if (fntype->formals().at(i)->category() == ast::type::category::unknown_type || expr.arguments().at(i)->annotation().type->category() == ast::type::category::unknown_type) continue;
                            else if (types::assignment_compatible(fntype->formals().at(i), expr.arguments().at(i)->annotation().type)) {
                                if (auto implicit = implicit_cast(fntype->formals().at(i), expr.arguments().at(i))) {
                                    expr.arguments().at(i) = implicit;
                                }
                                else {
                                    expr.arguments().at(i)->annotation().type = fntype->formals().at(i);
                                }

                                if (fn) test_immutable_assignment(*std::dynamic_pointer_cast<ast::parameter_declaration>(params.at(i)), *expr.arguments().at(i));
                            }
                            else {
                                std::cout << fntype->formals().at(i) << "\n" << expr.arguments().at(i)->annotation().type << '\n';
                                auto builder = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(expr.arguments().at(i)->range().begin())
                                            .message(diagnostic::format("__This argument must have type `$`, but I found `$`, idiot!", fntype->formals().at(i)->string(), expr.arguments().at(i)->annotation().type->string()))
                                            .highlight(expr.arguments().at(i)->range(), diagnostic::format("expected $", fntype->formals().at(i)->string()));

                                publisher().publish(builder.build());
                            }
                        }
                    }
                }
                // if function is called through oop notation as a method, then first parameter, the object, is implicitly passed
                else if (member->expression()->annotation().type->declaration() || (member->expression()->annotation().type->category() == ast::type::category::pointer_type && std::static_pointer_cast<ast::pointer_type>(member->expression()->annotation().type)->base()->declaration())) {
                    auto identifier = std::dynamic_pointer_cast<ast::identifier_expression>(member->member());
                    auto fn = identifier->annotation().referencing;
                    auto name = fn->kind() == ast::kind::function_declaration ? static_cast<const ast::function_declaration*>(fn)->name() : static_cast<const ast::property_declaration*>(fn)->name();
                    bool mistake = false, concrete = true;
                    substitutions sub(scope_, nullptr);

                    if (auto fndecl = dynamic_cast<const ast::function_declaration*>(fn)) {
                        if (!fndecl->parameters().empty() && std::dynamic_pointer_cast<ast::parameter_declaration>(fndecl->parameters().back())->is_variadic()) variadic = std::dynamic_pointer_cast<ast::slice_type>(fndecl->parameters().back()->annotation().type)->base();
                    }

                    if (expr.arguments().size() < fntype->formals().size() - 1 || (expr.arguments().size() > fntype->formals().size() - 1 && !variadic)) {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr.callee()->range(), diagnostic::format("This function expects `$` arguments when called as a method but you gave it `$`, pr*ck!", fntype->formals().size() - 1, expr.arguments().size()));
                    }
                    // previous analysis were done inside member expression check
                    // checks if there were provided any generic arguments
                    if (identifier && identifier->is_generic()) {
                        // if there were not generic parameters in declaration, then we have an error
                        if (fn->kind() == ast::kind::property_declaration || !static_cast<const ast::function_declaration*>(fn)->generic()) {
                            auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.range().begin())
                                        .message(diagnostic::format("$ `$` doesn't take any generic parameters, idiot!", fn->kind() == ast::kind::function_declaration ? "Function" : "Property", name))
                                        .highlight(expr.range())
                                        .note(name.range(), diagnostic::format("As you can see $ `$` was declared without generic parameters.", fn->kind() == ast::kind::function_declaration ? "function" : "property", name))
                                        .build();
                                
                            publisher().publish(diag);
                            mistake = true;
                        }
                        else {
                            auto expected = std::dynamic_pointer_cast<ast::generic_clause_declaration>(static_cast<const ast::function_declaration*>(fn)->generic());
                            
                            if (expected->parameters().size() < identifier->generics().size()) {
                                auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(identifier->range().begin())
                                            .message(diagnostic::format("$ `$` takes no more than `$` generic parameters, you gave it `$`, idiot!", fn->kind() == ast::kind::function_declaration ? "Function" : "Property", name, expected->parameters().size(), identifier->generics().size()))
                                            .highlight(identifier->range())
                                            .note(name.range(), diagnostic::format("As you can see $ `$` was declared with exactly `$` generic parameters.", fn->kind() == ast::kind::function_declaration ? "function" : "property", name, expected->parameters().size()))
                                            .build();
                                
                                publisher().publish(diag);
                                mistake = true;
                            }
                            else for (size_t i = 0; i < identifier->generics().size(); ++i)
                            {
                                if (auto constant = std::dynamic_pointer_cast<ast::generic_const_parameter_declaration>(expected->parameters().at(i))) {
                                    if (auto ambiguous = std::dynamic_pointer_cast<ast::type_expression>(identifier->generics().at(i))) {
                                        if (ambiguous->is_ambiguous()) {
                                            auto newexpr = ambiguous->as_expression();
                                            newexpr->accept(*this);
                                            identifier->generics().at(i) = newexpr;

                                            if (newexpr->annotation().istype) {
                                                auto diag = diagnostic::builder()
                                                    .severity(diagnostic::severity::error)
                                                    .location(ambiguous->range().begin())
                                                    .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                                    .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                                    .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                    .build();
                                    
                                                publisher().publish(diag);
                                                mistake = true;
                                            }
                                            else if (!types::compatible(constant->annotation().type, newexpr->annotation().type)) {
                                                auto diag = diagnostic::builder()
                                                    .severity(diagnostic::severity::error)
                                                    .location(ambiguous->range().begin())
                                                    .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                                    .highlight(ambiguous->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                                    .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                    .build();
                                    
                                                publisher().publish(diag);
                                                mistake = true;
                                            }
                                            else {
                                                try { newexpr->annotation().value = evaluate(newexpr); } catch (evaluator::generic_evaluation&) { concrete = false; newexpr->annotation().isparametric = true; }
                                                sub.put(constant.get(), newexpr->annotation().value);
                                            }
                                        }
                                        else {
                                            ambiguous->accept(*this);

                                            auto diag = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(ambiguous->range().begin())
                                                .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), ambiguous->annotation().type->string()))
                                                .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                                .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                .build();
                                
                                            publisher().publish(diag);
                                            mistake = true;
                                        }
                                    }
                                    else {
                                        identifier->generics().at(i)->accept(*this);

                                        if (!types::compatible(constant->annotation().type, identifier->generics().at(i)->annotation().type)) {
                                            auto diag = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(identifier->generics().at(i)->range().begin())
                                                .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), identifier->generics().at(i)->annotation().type->string()))
                                                .highlight(identifier->generics().at(i)->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                                .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                .build();
                                
                                            publisher().publish(diag);
                                            mistake = true;
                                        }
                                        else {
                                            try { identifier->generics().at(i)->annotation().value = evaluate(identifier->generics().at(i)); } catch (evaluator::generic_evaluation&) { concrete = false; identifier->generics().at(i)->annotation().isparametric = true; }
                                            sub.put(constant.get(), identifier->generics().at(i)->annotation().value);
                                        }
                                    }
                                }
                                else if (auto type = std::dynamic_pointer_cast<ast::generic_type_parameter_declaration>(expected->parameters().at(i))) {
                                    identifier->generics().at(i)->annotation().mustvalue = false;
                                    identifier->generics().at(i)->accept(*this);

                                    if (!identifier->generics().at(i)->annotation().istype) {
                                        auto diag = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(identifier->generics().at(i)->range().begin())
                                                .message("I need type parameter here but I found an expression instead, dammit!")
                                                .highlight(identifier->generics().at(i)->range(), "expected type")
                                                .note(type->range(), diagnostic::format("Type `$` declares `$` as a type parameter.", name, type->name().lexeme()))
                                                .build();
                                
                                        publisher().publish(diag);
                                        mistake = true;
                                    }
                                    else {
                                        if (auto typeexpr = std::dynamic_pointer_cast<ast::type_expression>(identifier->generics().at(i))) {
                                            if (typeexpr->is_parametric()) concrete = false;
                                        }
                                        
                                        sub.put(type.get(), identifier->generics().at(i)->annotation().type);
                                    }
                                }
                            }
                        
                            if (mistake) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                throw semantic_error();
                            }
                        }
                    }

                    if (auto fdecl = dynamic_cast<const ast::function_declaration*>(identifier->annotation().referencing)) {
                        if (auto generic = std::dynamic_pointer_cast<ast::generic_clause_declaration>(fdecl->generic())) {
                            // matcher will contain all generics bindings
                            ast::type_matcher::result match;

                            sub.context(scopes_.at(generic.get()));

                            // first explicit generic arguments are added to match list
                            for (std::size_t i = 0; i < identifier->generics().size(); ++i) {
                                if (auto constparam = std::dynamic_pointer_cast<ast::generic_const_parameter_declaration>(generic->parameters().at(i))) {
                                    match.value(constparam->name().lexeme().string(), identifier->generics().at(i)->annotation().value);
                                }
                                else if (auto typeparam = std::dynamic_pointer_cast<ast::generic_type_parameter_declaration>(generic->parameters().at(i))) {
                                    match.type(typeparam->name().lexeme().string(), identifier->generics().at(i)->annotation().type);
                                }
                            }

                            if (!fdecl->parameters().empty()) {
                                ast::type_matcher matcher(member->expression(), fdecl->parameters().front()->annotation().type, publisher());
                                matcher.match(member->expression()->annotation().type, match, std::static_pointer_cast<ast::parameter_declaration>(fdecl->parameters().front())->is_variadic());
                            }

                            // then we try to deduce generic arguments from function arguments
                            for (std::size_t i = 1; i < fdecl->parameters().size(); ++i) {
                                expr.arguments().at(i - 1)->accept(*this);

                                ast::type_matcher matcher(expr.arguments().at(i - 1), fdecl->parameters().at(i)->annotation().type, publisher());
                                if (!matcher.match(expr.arguments().at(i - 1)->annotation().type, match, std::static_pointer_cast<ast::parameter_declaration>(fdecl->parameters().at(i))->is_variadic()) && !match.duplication) {
                                    auto param = std::static_pointer_cast<ast::parameter_declaration>(fdecl->parameters().at(i));
                                    auto builder = diagnostic::builder()
                                                .location(expr.arguments().at(i)->range().begin())
                                                .severity(diagnostic::severity::error)
                                                .small(true)
                                                .highlight(expr.arguments().at(i)->range(), diagnostic::format("expected $", param->annotation().type->string()))
                                                .message(diagnostic::format("Type mismatch between argument and parameter, found `$` and `$`.", expr.arguments().at(i - 1)->annotation().type->string(), param->annotation().type->string()))
                                                .note(param->name().range(), diagnostic::format("Have a look at parameter `$` idiot.", param->name().lexeme()));
                                    
                                    publisher().publish(builder.build());
                                }
                            }

                            if (!match) {
                                expr.annotation().type = types::unknown();
                                expr.invalid(true);
                                throw semantic_error();
                            }
                            else {
                                for (auto generic_parameter : generic->parameters()) {
                                    token name = generic_parameter->kind() == ast::kind::generic_const_parameter_declaration ? std::static_pointer_cast<ast::generic_const_parameter_declaration>(generic_parameter)->name() : std::static_pointer_cast<ast::generic_type_parameter_declaration>(generic_parameter)->name();
                                    auto found = match.bindings.find(name.lexeme().string());
                                    if (found == match.bindings.end() || (generic_parameter->kind() == ast::kind::generic_type_parameter_declaration && found->second.type->category() == ast::type::category::unknown_type)) {
                                        std::ostringstream oss;

                                        oss << fdecl->name().lexeme() << "!(" << name.lexeme() << ")";

                                        auto builder = diagnostic::builder()
                                                    .location(expr.range().begin())
                                                    .severity(diagnostic::severity::error)
                                                    .small(true)
                                                    .highlight(expr.callee()->range())
                                                    .highlight(name.range(), diagnostic::highlighter::mode::light)
                                                    .message(diagnostic::format("I can't deduce $ parameter `$` in this function call, dammit!", generic_parameter->kind() == ast::kind::generic_const_parameter_declaration ? "value" : "type", name.lexeme()))
                                                    .replacement(expr.callee()->range(), oss.str(), diagnostic::format("Try specifying argument `$` explicit to make it clear!", name.lexeme()));
                                        
                                        publisher().publish(builder.build());
                                        concrete = false;
                                    }
                                    else if (generic_parameter->kind() == ast::kind::generic_const_parameter_declaration) {
                                        sub.put(generic_parameter.get(), found->second.value);
                                    }
                                    else {
                                        sub.put(generic_parameter.get(), found->second.type);
                                    }
                                }
                                /*
                                std::cout << "TYPE MATCHER BINDINGS " << fdecl->name().lexeme() << '\n';
                                for (auto binding : match.bindings) {
                                    if (binding.second.kind == ast::type_matcher::parameter::kind::type) {
                                        std::cout << binding.first << " = " << binding.second.type->string() << '\n';
                                    }
                                    else {
                                        std::cout << binding.first << " = " << binding.second.value << '\n';
                                    }
                                }
                                */
                                if (!concrete) {
                                    expr.invalid(true);
                                    expr.annotation().type = types::unknown();
                                    throw semantic_error();
                                }

                                auto instantiated = instantiate_function(*fdecl, sub);
                                identifier->annotation().referencing = instantiated.get();
                                fntype = std::dynamic_pointer_cast<ast::function_type>(instantiated->annotation().type);
                                expr.annotation().type = fntype->result();
                            }
                        }
                        // checks if there were provided any generic arguments
                        else if (identifier && identifier->is_generic()) {
                            auto fn = identifier->annotation().referencing;
                            auto name = fn->kind() == ast::kind::function_declaration ? static_cast<const ast::function_declaration*>(fn)->name() : static_cast<const ast::property_declaration*>(fn)->name();
                            auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.range().begin())
                                        .message(diagnostic::format("$ `$` doesn't take any generic parameters, idiot!", fn->kind() == ast::kind::function_declaration ? "Function" : "Property", name.lexeme()))
                                        .highlight(expr.callee()->range())
                                        .note(name.range(), diagnostic::format("As you can see $ `$` was declared without generic parameters.", fn->kind() == ast::kind::function_declaration ? "function" : "property", name.lexeme()))
                                        .build();
                                
                            publisher().publish(diag);

                            expr.invalid(true);
                            expr.annotation().type = types::unknown();
                            throw semantic_error();
                        }
                    }
                    else if (auto vdecl = dynamic_cast<const ast::var_declaration*>(identifier->annotation().referencing)) {
                        if (identifier && identifier->is_generic()) {
                            auto name = vdecl->name();
                            auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.range().begin())
                                        .message(diagnostic::format("Callable `$` doesn't take any generic parameters, idiot!", name.lexeme()))
                                        .highlight(expr.callee()->range())
                                        .note(name.range(), diagnostic::format("As you can see callable `$` was declared without generic parameters.", name.lexeme()))
                                        .build();
                                
                            publisher().publish(diag);
                            expr.invalid(true);
                            expr.annotation().type = types::unknown();
                            throw semantic_error();
                        }
                    }

                    ast::pointers<ast::declaration> params;

                    if (fn->kind() == ast::kind::function_declaration) params = static_cast<const ast::function_declaration*>(fn)->parameters();
                    else if (fn->kind() == ast::kind::property_declaration) params = static_cast<const ast::property_declaration*>(fn)->parameters();

                    if (fn) test_immutable_assignment(*std::dynamic_pointer_cast<ast::parameter_declaration>(params.front()), *member->expression());

                    // implicit cast for object parameter (first hidden parameter in oop)
                    if (auto implicit = implicit_cast(fntype->formals().front(), member->expression())) member->expression() = implicit;
                    
                    // first parameter is implicit
                    for (std::size_t i = 0; i < fntype->formals().size() - 1; ++i) {
                        // variadic case
                        if (variadic && variadic->category() != ast::type::category::unknown_type && i == fntype->formals().size() - 2) {
                            variadic = std::static_pointer_cast<ast::slice_type>(fntype->formals().back())->base();
                            ast::pointers<ast::expression> elements;
                            
                            for (std::size_t j = i; j < expr.arguments().size(); ++j) {
                                expr.arguments().at(j)->accept(*this);
                                if (expr.arguments().at(j)->annotation().type->category() == ast::type::category::unknown_type) continue;
                                else if (types::assignment_compatible(variadic, expr.arguments().at(j)->annotation().type)) {
                                    if (auto implicit = implicit_cast(variadic, expr.arguments().at(j))) {
                                        expr.arguments().at(j) = implicit;
                                    }
                                    else {
                                        expr.arguments().at(j)->annotation().type = variadic;
                                    }
                                }
                                else {
                                    auto builder = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(expr.arguments().at(j)->range().begin())
                                                .message(diagnostic::format("This argument must have type `$`, but I found `$`, idiot!", variadic->string(), expr.arguments().at(j)->annotation().type->string()))
                                                .highlight(expr.arguments().at(j)->range(), diagnostic::format("expected $", variadic->string()));

                                    publisher().publish(builder.build());
                                }
                                elements.push_back(expr.arguments().at(j));
                            }
                            
                            auto array = ast::create<ast::array_expression>(source_range(expr.arguments().at(i)->range().begin(), expr.arguments().back()->range().end()), elements);
                            array->annotation().type = types::array(variadic, expr.arguments().size() - i);

                            auto binding = create_temporary_var(*array);
                            binding->annotation().scope = scope_->enclosing();
                            pending_insertions.emplace_back(scope_, binding, statement_, false);

                            array->annotation().referencing = binding.get();
                            
                            for (std::size_t j = expr.arguments().size() - 1; j > i; --j) expr.arguments().pop_back();

                            expr.arguments().back() = implicit_forced_cast(types::slice(variadic), array);
                        }
                        else {
                            expr.arguments().at(i)->accept(*this);
                            if (fntype->formals().at(i + 1)->category() == ast::type::category::unknown_type || expr.arguments().at(i)->annotation().type->category() == ast::type::category::unknown_type) continue;
                            else if (types::assignment_compatible(fntype->formals().at(i + 1), expr.arguments().at(i)->annotation().type)) {
                                if (auto implicit = implicit_cast(fntype->formals().at(i + 1), expr.arguments().at(i))) {
                                    expr.arguments().at(i) = implicit;
                                }
                                else {
                                    expr.arguments().at(i)->annotation().type = fntype->formals().at(i + 1);
                                }

                                if (fn) test_immutable_assignment(*std::dynamic_pointer_cast<ast::parameter_declaration>(params.at(i + 1)), *expr.arguments().at(i));
                            }
                            else {
                                auto builder = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(expr.arguments().at(i)->range().begin())
                                            .message(diagnostic::format("This argument must have type `$`, but I found `$`, idiot!", fntype->formals().at(i + 1)->string(), expr.arguments().at(i)->annotation().type->string()))
                                            .highlight(expr.arguments().at(i)->range(), diagnostic::format("expected $", fntype->formals().at(i + 1)->string()));

                                publisher().publish(builder.build());
                            }
                        }
                    }
                
                }
                // otherwise it is a field whose type is function which is called and it may seems to be a method but it is just a (callable) field
                else {
                    if (expr.arguments().size() != fntype->formals().size()) {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr.callee()->range(), diagnostic::format("This function expects `$` arguments but you gave it `$`, pr*ck!", fntype->formals().size(), expr.arguments().size()));
                    }
                    
                    for (std::size_t i = 0; i < fntype->formals().size(); ++i) {
                        expr.arguments().at(i)->accept(*this);
                        if (fntype->formals().at(i)->category() == ast::type::category::unknown_type || expr.arguments().at(i)->annotation().type->category() == ast::type::category::unknown_type) continue;
                        else if (types::assignment_compatible(fntype->formals().at(i), expr.arguments().at(i)->annotation().type)) {
                            if (auto implicit = implicit_cast(fntype->formals().at(i), expr.arguments().at(i))) {
                                expr.arguments().at(i) = implicit;
                            }
                            else {
                                expr.arguments().at(i)->annotation().type = fntype->formals().at(i);
                            }
                        }
                        else {
                            auto builder = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.arguments().at(i)->range().begin())
                                        .message(diagnostic::format("This argument must have type `$`, but I found `$`, idiot!", fntype->formals().at(i)->string(), expr.arguments().at(i)->annotation().type->string()))
                                        .highlight(expr.arguments().at(i)->range(), diagnostic::format("expected $", fntype->formals().at(i)->string()));

                            publisher().publish(builder.build());
                        }
                    }
                }
            }
        }
        else {
            // enable resolution of callee as a type for costructing tuples or structures
            expr.callee()->annotation().mustvalue = false;
            // generics are deduced at this level from function call, so we don't need them to be resolved from visit(identifier_expression&)
            expr.callee()->annotation().deduce = false;
            expr.callee()->accept(*this);
            if (!expr.callee()->annotation().type || expr.callee()->annotation().type->category() == ast::type::category::unknown_type) {
                expr.invalid(true);
                expr.annotation().type = types::unknown();
                throw semantic_error();
            }
            else if (expr.callee()->annotation().istype) {
                switch (expr.callee()->annotation().type->category()) {
                case ast::type::category::tuple_type:
                {
                    auto tuple_type = std::static_pointer_cast<ast::tuple_type>(expr.callee()->annotation().type);

                    if (expr.arguments().size() != tuple_type->components().size()) {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr.callee()->range(), diagnostic::format("Tuple type `$` expects `$` components but you gave it `$`, pr*ck!", expr.callee()->annotation().type->string(), tuple_type->components().size(), expr.arguments().size()));
                    }
                    
                    for (std::size_t i = 0; i < tuple_type->components().size(); ++i) {
                        expr.arguments().at(i)->accept(*this);
                        if (tuple_type->components().at(i)->category() == ast::type::category::unknown_type || expr.arguments().at(i)->annotation().type->category() == ast::type::category::unknown_type) continue;
                        else if (types::assignment_compatible(tuple_type->components().at(i), expr.arguments().at(i)->annotation().type)) {
                            if (auto implicit = implicit_cast(tuple_type->components().at(i), expr.arguments().at(i))) {
                                expr.arguments().at(i) = implicit;
                            }
                            else {
                                expr.arguments().at(i)->annotation().type = tuple_type->components().at(i);
                            }
                        }
                        else {
                            auto builder = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.arguments().at(i)->range().begin())
                                        .message(diagnostic::format("This component must have type `$`, but I found `$`, idiot!", tuple_type->components().at(i)->string(), expr.arguments().at(i)->annotation().type->string()))
                                        .highlight(expr.arguments().at(i)->range(), diagnostic::format("expected $", tuple_type->components().at(i)->string()));

                            publisher().publish(builder.build());
                        }
                    }
                    
                    expr.annotation().type = expr.callee()->annotation().type;
                    break;
                }
                case ast::type::category::structure_type:
                {
                    auto structure_type = std::static_pointer_cast<ast::structure_type>(expr.callee()->annotation().type);

                    if (expr.arguments().size() != structure_type->fields().size()) {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr.callee()->range(), diagnostic::format("Structure type `$` expects `$` fields but you gave it `$`, pr*ck!", expr.callee()->annotation().type->string(), structure_type->fields().size(), expr.arguments().size()));
                    }
                    
                    for (std::size_t i = 0; i < structure_type->fields().size(); ++i) {
                        expr.arguments().at(i)->accept(*this);

                        if (structure_type->declaration()) {
                            auto item = static_cast<const ast::record_declaration*>(structure_type->declaration())->fields().at(i);
                            std::string name = std::static_pointer_cast<ast::field_declaration>(item)->name().lexeme().string();
                            
                            if ((item->is_hidden() || name[0] == '_') && !scope_->has_ancestor_scope(scopes_.at(structure_type->declaration()))) {
                                auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(expr.arguments().at(i)->range().begin())
                                            .message(diagnostic::format("You can't initialize hidden field `$`, dammit!", name))
                                            .highlight(expr.arguments().at(i)->range(), "inaccessible")
                                            .note(static_cast<const ast::field_declaration*>(item.get())->name().range(), diagnostic::format("Look at field `$` declaration, idiot.", name))
                                            .build();

                                publisher().publish(diag);
                                expr.invalid(true);
                            }
                        }

                        if (structure_type->fields().at(i).type->category() == ast::type::category::unknown_type || expr.arguments().at(i)->annotation().type->category() == ast::type::category::unknown_type) continue;
                        else if (types::assignment_compatible(structure_type->fields().at(i).type, expr.arguments().at(i)->annotation().type)) {
                            if (auto implicit = implicit_cast(structure_type->fields().at(i).type, expr.arguments().at(i))) {
                                expr.arguments().at(i) = implicit;
                            }
                            else {
                                expr.arguments().at(i)->annotation().type = structure_type->fields().at(i).type;
                            }
                        }
                        else {
                            auto builder = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.arguments().at(i)->range().begin())
                                        .message(diagnostic::format("Field `$` must have type `$`, but I found `$`, idiot!", structure_type->fields().at(i).name, structure_type->fields().at(i).type->string(), expr.arguments().at(i)->annotation().type->string()))
                                        .highlight(expr.arguments().at(i)->range(), diagnostic::format("expected $", structure_type->fields().at(i).type->string()));

                            publisher().publish(builder.build());
                        }
                    }
                    
                    expr.annotation().type = expr.callee()->annotation().type;
                    break;
                }
                default:
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    if (expr.callee()->annotation().type->category() == ast::type::category::behaviour_type) error(expr.callee()->range(), diagnostic::format("Behaviour type `$` cannot be instantiated, idiot!", expr.callee()->annotation().type->string()));
                    else error(expr.callee()->range(), diagnostic::format("I was expecting a callable value but I found type `$` instead!", expr.callee()->annotation().type->string()), "", "expected value");
                }
            }
            else if (expr.callee()->annotation().type->category() != ast::type::category::function_type) {
                expr.invalid(true);
                expr.annotation().type = types::unknown();
                error(expr.callee()->range(), diagnostic::format("This expression must be callable but it has type `$`, idiot!", expr.callee()->annotation().type->string()), "", "expected function");
            }
            else {
                auto fntype = std::static_pointer_cast<ast::function_type>(expr.callee()->annotation().type);
                expr.annotation().type = fntype->result();

                auto identifier = std::dynamic_pointer_cast<ast::identifier_expression>(expr.callee());
                auto fn = identifier->annotation().referencing;
                auto name = fn->kind() == ast::kind::function_declaration ? static_cast<const ast::function_declaration*>(fn)->name() : static_cast<const ast::property_declaration*>(fn)->name();
                ast::pointer<ast::type> variadic = nullptr;
                bool mistake = false, concrete = true;
                substitutions sub(scope_, nullptr);

                if (auto fndecl = dynamic_cast<const ast::function_declaration*>(fn)) {
                    if (!fndecl->parameters().empty() && std::dynamic_pointer_cast<ast::parameter_declaration>(fndecl->parameters().back())->is_variadic()) variadic = std::dynamic_pointer_cast<ast::slice_type>(fndecl->parameters().back()->annotation().type)->base();
                }

                if (expr.arguments().size() < fntype->formals().size() || (expr.arguments().size() > fntype->formals().size() && !variadic)) {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr.callee()->range(), diagnostic::format("This function expects `$` arguments but you gave it `$`, pr*ck!", fntype->formals().size(), expr.arguments().size()));
                }

                // checks if there were provided any generic arguments
                if (identifier && identifier->is_generic()) {
                    // if there were not generic parameters in declaration, then we have an error
                    if (fn->kind() == ast::kind::property_declaration || !static_cast<const ast::function_declaration*>(fn)->generic()) {
                        auto diag = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(expr.range().begin())
                                    .message(diagnostic::format("$ `$` doesn't take any generic parameters, idiot!", fn->kind() == ast::kind::function_declaration ? "Function" : "Property", name.lexeme()))
                                    .highlight(expr.range())
                                    .note(name.range(), diagnostic::format("As you can see $ `$` was declared without generic parameters.", fn->kind() == ast::kind::function_declaration ? "function" : "property", name.lexeme()))
                                    .build();
                            
                        publisher().publish(diag);
                        mistake = true;
                    }
                    else {
                        auto expected = std::dynamic_pointer_cast<ast::generic_clause_declaration>(static_cast<const ast::function_declaration*>(fn)->generic());
                        
                        if (expected->parameters().size() < identifier->generics().size()) {
                            auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(identifier->range().begin())
                                        .message(diagnostic::format("$ `$` takes no more than `$` generic parameters, you gave it `$`, idiot!", fn->kind() == ast::kind::function_declaration ? "Function" : "Property", name.lexeme(), expected->parameters().size(), identifier->generics().size()))
                                        .highlight(identifier->range())
                                        .note(name.range(), diagnostic::format("As you can see $ `$` was declared with exactly `$` generic parameters.", fn->kind() == ast::kind::function_declaration ? "function" : "property", name.lexeme(), expected->parameters().size()))
                                        .build();
                            
                            publisher().publish(diag);
                            mistake = true;
                        }
                        else for (size_t i = 0; i < identifier->generics().size(); ++i)
                        {
                            if (auto constant = std::dynamic_pointer_cast<ast::generic_const_parameter_declaration>(expected->parameters().at(i))) {
                                if (auto ambiguous = std::dynamic_pointer_cast<ast::type_expression>(identifier->generics().at(i))) {
                                    if (ambiguous->is_ambiguous()) {
                                        auto newexpr = ambiguous->as_expression();
                                        newexpr->accept(*this);
                                        identifier->generics().at(i) = newexpr;

                                        if (newexpr->annotation().istype) {
                                            auto diag = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(ambiguous->range().begin())
                                                .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                                .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                                .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                .build();
                                
                                            publisher().publish(diag);
                                            mistake = true;
                                        }
                                        else if (!types::compatible(constant->annotation().type, newexpr->annotation().type)) {
                                            auto diag = diagnostic::builder()
                                                .severity(diagnostic::severity::error)
                                                .location(ambiguous->range().begin())
                                                .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), newexpr->annotation().type->string()))
                                                .highlight(ambiguous->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                                .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                                .build();
                                
                                            publisher().publish(diag);
                                            mistake = true;
                                        }
                                        else {
                                            try { newexpr->annotation().value = evaluate(newexpr); } catch (evaluator::generic_evaluation&) { concrete = false; newexpr->annotation().isparametric = true; }
                                            sub.put(constant.get(), newexpr->annotation().value);
                                        }
                                    }
                                    else {
                                        ambiguous->accept(*this);

                                        auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(ambiguous->range().begin())
                                            .message(diagnostic::format("I need constant value of type `$` but I found type `$` instead, dammit!", constant->annotation().type->string(), ambiguous->annotation().type->string()))
                                            .highlight(ambiguous->range(), diagnostic::format("expected value", constant->annotation().type->string()))
                                            .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                            .build();
                            
                                        publisher().publish(diag);
                                        mistake = true;
                                    }
                                }
                                else {
                                    identifier->generics().at(i)->accept(*this);

                                    if (!types::compatible(constant->annotation().type, identifier->generics().at(i)->annotation().type)) {
                                        auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(identifier->generics().at(i)->range().begin())
                                            .message(diagnostic::format("I was expecting type `$` for constant parameter but I found type `$`, idiot!", constant->annotation().type->string(), identifier->generics().at(i)->annotation().type->string()))
                                            .highlight(identifier->generics().at(i)->range(), diagnostic::format("expected $", constant->annotation().type->string()))
                                            .note(constant->range(), diagnostic::format("Look at parameter `$` declaration.", constant->name().lexeme()))
                                            .build();
                            
                                        publisher().publish(diag);
                                        mistake = true;
                                    }
                                    else {
                                        try { identifier->generics().at(i)->annotation().value = evaluate(identifier->generics().at(i)); } catch (evaluator::generic_evaluation&) { concrete = false; identifier->generics().at(i)->annotation().isparametric = true; }
                                        sub.put(constant.get(), identifier->generics().at(i)->annotation().value);
                                    }
                                }
                            }
                            else if (auto type = std::dynamic_pointer_cast<ast::generic_type_parameter_declaration>(expected->parameters().at(i))) {
                                identifier->generics().at(i)->annotation().mustvalue = false;
                                identifier->generics().at(i)->accept(*this);

                                if (!identifier->generics().at(i)->annotation().istype) {
                                    auto diag = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(identifier->generics().at(i)->range().begin())
                                            .message("I need type parameter here but I found an expression instead, dammit!")
                                            .highlight(identifier->generics().at(i)->range(), "expected type")
                                            .note(type->range(), diagnostic::format("Type `$` declares `$` as a type parameter.", name, type->name().lexeme()))
                                            .build();
                            
                                    publisher().publish(diag);
                                    mistake = true;
                                }
                                else {
                                    if (auto typeexpr = std::dynamic_pointer_cast<ast::type_expression>(identifier->generics().at(i))) {
                                        if (typeexpr->is_parametric()) concrete = false;
                                    }
                                    
                                    sub.put(type.get(), identifier->generics().at(i)->annotation().type);
                                }
                            }
                        }
                        
                        if (mistake) {
                            expr.invalid(true);
                            expr.annotation().type = types::unknown();
                            throw semantic_error();
                        }
                    }
                }
                // generic instantiation if function depends on generics
                if (auto fdecl = dynamic_cast<const ast::function_declaration*>(identifier->annotation().referencing)) {
                    if (auto generic = std::dynamic_pointer_cast<ast::generic_clause_declaration>(fdecl->generic())) {
                        // matcher will contain all generics bindings
                        ast::type_matcher::result match;

                        sub.context(scopes_.at(generic.get()));

                        // first explicit generic arguments are added to match list
                        for (std::size_t i = 0; i < identifier->generics().size(); ++i) {
                            if (auto constparam = std::dynamic_pointer_cast<ast::generic_const_parameter_declaration>(generic->parameters().at(i))) {
                                match.value(constparam->name().lexeme().string(), identifier->generics().at(i)->annotation().value);
                            }
                            else if (auto typeparam = std::dynamic_pointer_cast<ast::generic_type_parameter_declaration>(generic->parameters().at(i))) {
                                match.type(typeparam->name().lexeme().string(), identifier->generics().at(i)->annotation().type);
                            }
                        }

                        // then we try to deduce generic arguments from function arguments
                        for (std::size_t i = 0; i < fdecl->parameters().size(); ++i) {
                            expr.arguments().at(i)->accept(*this);

                            ast::type_matcher matcher(expr.arguments().at(i), fdecl->parameters().at(i)->annotation().type, publisher());
                            if (!matcher.match(expr.arguments().at(i)->annotation().type, match, std::static_pointer_cast<ast::parameter_declaration>(fdecl->parameters().at(i))->is_variadic()) && !match.duplication) {
                                auto param = std::static_pointer_cast<ast::parameter_declaration>(fdecl->parameters().at(i));
                                auto builder = diagnostic::builder()
                                            .location(expr.arguments().at(i)->range().begin())
                                            .severity(diagnostic::severity::error)
                                            .small(true)
                                            .highlight(expr.arguments().at(i)->range(), diagnostic::format("expected $", param->annotation().type->string()))
                                            .message(diagnostic::format("Type mismatch between argument and parameter, found `$` and `$`.", expr.arguments().at(i)->annotation().type->string(), param->annotation().type->string()))
                                            .note(param->name().range(), diagnostic::format("Have a look at parameter `$` idiot.", param->name().lexeme()));
                                
                                publisher().publish(builder.build());
                            }
                        }

                        if (!match) {
                            expr.annotation().type = types::unknown();
                            expr.invalid(true);
                            throw semantic_error();
                        }
                        else {
                            for (auto generic_parameter : generic->parameters()) {
                                token name = generic_parameter->kind() == ast::kind::generic_const_parameter_declaration ? std::static_pointer_cast<ast::generic_const_parameter_declaration>(generic_parameter)->name() : std::static_pointer_cast<ast::generic_type_parameter_declaration>(generic_parameter)->name();
                                auto found = match.bindings.find(name.lexeme().string());
                                if (found == match.bindings.end() || (generic_parameter->kind() == ast::kind::generic_type_parameter_declaration && found->second.type->category() == ast::type::category::unknown_type)) {
                                    std::ostringstream oss;

                                    oss << fdecl->name().lexeme() << "!(" << name.lexeme() << ")";

                                    auto builder = diagnostic::builder()
                                                .location(expr.range().begin())
                                                .severity(diagnostic::severity::error)
                                                .small(true)
                                                .highlight(expr.callee()->range())
                                                .highlight(name.range(), diagnostic::highlighter::mode::light)
                                                .message(diagnostic::format("I can't deduce $ parameter `$` in this function call, dammit!", generic_parameter->kind() == ast::kind::generic_const_parameter_declaration ? "value" : "type", name.lexeme()))
                                                .replacement(expr.callee()->range(), oss.str(), diagnostic::format("Try specifying argument `$` explicit to make it clear!", name.lexeme()));
                                    
                                    publisher().publish(builder.build());
                                    concrete = false;
                                }
                                else if (generic_parameter->kind() == ast::kind::generic_const_parameter_declaration) {
                                    sub.put(generic_parameter.get(), found->second.value);
                                }
                                else {
                                    sub.put(generic_parameter.get(), found->second.type);
                                }
                            }
                            /*
                            std::cout << "TYPE MATCHER BINDINGS " << fdecl->name().lexeme() << '\n';
                            for (auto binding : match.bindings) {
                                if (binding.second.kind == ast::type_matcher::parameter::kind::type) {
                                    std::cout << binding.first << " = " << binding.second.type->string() << '\n';
                                }
                                else {
                                    std::cout << binding.first << " = " << binding.second.value << '\n';
                                }
                            }
                            */
                            if (!concrete) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                throw semantic_error();
                            }

                            auto instantiated = instantiate_function(*fdecl, sub);

                            if (!instantiated) {
                                std::ostringstream oss;

                                oss << "Instantiation of function `" << fdecl->name().lexeme() << "` failed with the following arguments";
                                
                                for (auto t : sub.types()) oss << " \\ • " << dynamic_cast<const ast::generic_type_parameter_declaration*>(t.first)->name().lexeme() << " = " << t.second->string();
                                for (auto v : sub.constants()) oss << " \\ • " << dynamic_cast<const ast::generic_const_parameter_declaration*>(v.first)->name().lexeme() << " = " << v.second.simple();

                                oss << " \\ \\ Substitution of generic arguments does not satisfy concept contraints, dammit!";

                                auto builder = diagnostic::builder()
                                               .location(expr.range().begin())
                                               .severity(diagnostic::severity::error)
                                               .message(oss.str())
                                               .small(true)
                                               .highlight(expr.range(), "concept failure");

                                if (fdecl->generic()) {
                                    if (auto constraint = std::static_pointer_cast<ast::generic_clause_declaration>(fdecl->generic())->constraint()) builder.highlight(constraint->range(), diagnostic::highlighter::mode::light);
                                }

                                publisher().publish(builder.build());
                                expr.annotation().type = types::unknown();
                                expr.invalid(true);
                                throw semantic_error();
                            }

                            identifier->annotation().referencing = instantiated.get();
                            fntype = std::dynamic_pointer_cast<ast::function_type>(instantiated->annotation().type);
                            expr.annotation().type = fntype->result();
                        }
                    }
                    // checks if there were provided any generic arguments
                    else if (identifier && identifier->is_generic()) {
                        auto fn = identifier->annotation().referencing;
                        auto name = fn->kind() == ast::kind::function_declaration ? static_cast<const ast::function_declaration*>(fn)->name() : static_cast<const ast::property_declaration*>(fn)->name();
                        auto diag = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(expr.range().begin())
                                    .message(diagnostic::format("$ `$` doesn't take any generic parameters, idiot!", fn->kind() == ast::kind::function_declaration ? "Function" : "Property", name.lexeme()))
                                    .highlight(expr.callee()->range())
                                    .note(name.range(), diagnostic::format("As you can see $ `$` was declared without generic parameters.", fn->kind() == ast::kind::function_declaration ? "function" : "property", name.lexeme()))
                                    .build();
                            
                        publisher().publish(diag);

                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        throw semantic_error();
                    }
                }
                else if (auto vdecl = dynamic_cast<const ast::var_declaration*>(identifier->annotation().referencing)) {
                    if (identifier && identifier->is_generic()) {
                        auto name = vdecl->name();
                        auto diag = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(expr.range().begin())
                                    .message(diagnostic::format("Callable `$` doesn't take any generic parameters, idiot!", name.lexeme()))
                                    .highlight(expr.callee()->range())
                                    .note(name.range(), diagnostic::format("As you can see callable `$` was declared without generic parameters.", name.lexeme()))
                                    .build();
                            
                        publisher().publish(diag);
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        throw semantic_error();
                    }
                }

                ast::pointers<ast::declaration> params;

                if (!identifier->annotation().referencing);
                else if (identifier->annotation().referencing->kind() == ast::kind::function_declaration) params = static_cast<const ast::function_declaration*>(identifier->annotation().referencing)->parameters();
                else if (identifier->annotation().referencing->kind() == ast::kind::property_declaration) params = static_cast<const ast::property_declaration*>(identifier->annotation().referencing)->parameters();

                // analysis of arguments
                for (std::size_t i = 0; i < fntype->formals().size(); ++i) {
                    // variadic case
                    if (variadic && variadic->category() != ast::type::category::unknown_type && i == fntype->formals().size() - 1) {
                        variadic = std::static_pointer_cast<ast::slice_type>(fntype->formals().back())->base();
                        ast::pointers<ast::expression> elements;
                        
                        for (std::size_t j = i; j < expr.arguments().size(); ++j) {
                            expr.arguments().at(j)->accept(*this);
                            if (expr.arguments().at(j)->annotation().type->category() == ast::type::category::unknown_type) continue;
                            else if (types::assignment_compatible(variadic, expr.arguments().at(j)->annotation().type)) {
                                if (auto implicit = implicit_cast(variadic, expr.arguments().at(j))) {
                                    expr.arguments().at(j) = implicit;
                                }
                                else {
                                    expr.arguments().at(j)->annotation().type = variadic;
                                }

                                if (identifier->annotation().referencing) test_immutable_assignment(*std::dynamic_pointer_cast<ast::parameter_declaration>(params.back()), *expr.arguments().at(j));
                            }
                            else {
                                auto builder = diagnostic::builder()
                                            .severity(diagnostic::severity::error)
                                            .location(expr.arguments().at(j)->range().begin())
                                            .message(diagnostic::format("This argument must have type `$`, but I found `$`, idiot!", variadic->string(), expr.arguments().at(j)->annotation().type->string()))
                                            .highlight(expr.arguments().at(j)->range(), diagnostic::format("expected $", variadic->string()));

                                publisher().publish(builder.build());
                            }
                            elements.push_back(expr.arguments().at(j));
                        }
                        
                        auto array = ast::create<ast::array_expression>(source_range(expr.arguments().at(i)->range().begin(), expr.arguments().back()->range().end()), elements);
                        array->annotation().type = types::array(variadic, expr.arguments().size() - i);

                        auto binding = create_temporary_var(*array);
                        binding->annotation().scope = scope_->enclosing();
                        pending_insertions.emplace_back(scope_, binding, statement_, false);

                        array->annotation().referencing = binding.get();
                        
                        for (std::size_t j = expr.arguments().size() - 1; j > i; --j) expr.arguments().pop_back();

                        expr.arguments().back() = implicit_forced_cast(types::slice(variadic), array);
                    }
                    else {
                        expr.arguments().at(i)->accept(*this);
                        if (fntype->formals().at(i)->category() == ast::type::category::unknown_type || !expr.arguments().at(i)->annotation().type || expr.arguments().at(i)->annotation().type->category() == ast::type::category::unknown_type) continue;
                        else if (types::assignment_compatible(fntype->formals().at(i), expr.arguments().at(i)->annotation().type)) {
                            if (auto implicit = implicit_cast(fntype->formals().at(i), expr.arguments().at(i))) {
                                expr.arguments().at(i) = implicit;
                            }
                            else {
                                expr.arguments().at(i)->annotation().type = fntype->formals().at(i);
                            }

                            if (identifier->annotation().referencing) test_immutable_assignment(fntype->formals().at(i), *expr.arguments().at(i));
                        }
                        else {
                            auto builder = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.arguments().at(i)->range().begin())
                                        .message(diagnostic::format("This argument must have type `$`, but I found `$`, idiot!", fntype->formals().at(i)->string(), expr.arguments().at(i)->annotation().type->string()))
                                        .highlight(expr.arguments().at(i)->range(), diagnostic::format("expected $", fntype->formals().at(i)->string()));

                            publisher().publish(builder.build());
                        }
                    }
                }
            }
        }
    }

    namespace impl {
        std::string to_identifier_type_name(const std::string& type_name) 
        {
            std::ostringstream oss;
            for (std::size_t i = 0; i < type_name.size() - 1; ++i) {
                if (std::isalnum(type_name.at(i)) && type_name.at(i + 1) == '(') {
                    oss << type_name.at(i) << "!(";
                    ++i;
                }
                else oss << type_name.at(i);
            }
            oss << type_name.back();
            return oss.str();
        }
    }
    
    void checker::visit(const ast::member_expression& expr) 
    {
        // right expression is a member (field, static function or object function/property)
        auto member = std::static_pointer_cast<ast::identifier_expression>(expr.member());
        std::string item = member->identifier().lexeme().string();
        // left expression could be a type name inside a member expression
        expr.expression()->annotation().mustvalue = false;
        expr.expression()->accept(*this);

        if (!expr.expression()->annotation().type || expr.expression()->annotation().type->category() == ast::type::category::unknown_type) {
            expr.invalid(true);
            expr.annotation().type = types::unknown();
            throw semantic_error();
        }
        else if (expr.expression()->annotation().istype) {
            if (auto builtin = types::builtin(expr.expression()->annotation().type->string())) {
                std::string item = member->identifier().lexeme().string();
                if (member->is_generic()) error(expr.member()->range(), diagnostic::format("There is no generic function `$` associated to builtin type `$`, idiot!", item, builtin->string()));

                switch (builtin->category()) {
                    case ast::type::category::integer_type:
                        if (item == "BITS") expr.annotation().type = types::usize();
                        else if (item == "MIN") expr.annotation().type = builtin;
                        else if (item == "MAX") expr.annotation().type = builtin;
                        else error(expr.member()->range(), diagnostic::format("There is no symbol `$` associated to builtin type `$`, idiot!", item, builtin->string()));
                        break;
                    case ast::type::category::rational_type:
                        if (item == "BITS") expr.annotation().type = types::usize();
                        else error(expr.member()->range(), diagnostic::format("There is no symbol `$` associated to builtin type `$`, idiot!", item, builtin->string()));
                        break;
                    case ast::type::category::float_type:
                        if (item == "BITS") expr.annotation().type = types::usize();
                        else if (item == "MIN") expr.annotation().type = builtin;
                        else if (item == "MAX") expr.annotation().type = builtin;
                        else if (item == "INFINITY") expr.annotation().type = builtin;
                        else if (item == "NAN") expr.annotation().type = builtin;
                        else error(expr.member()->range(), diagnostic::format("There is no symbol `$` associated to builtin type `$`, idiot!", item, builtin->string()));
                        break;
                    case ast::type::category::complex_type:
                        if (item == "BITS") expr.annotation().type = types::usize();
                        else error(expr.member()->range(), diagnostic::format("There is no symbol `$` associated to builtin type `$`, idiot!", item, builtin->string()));
                        break;
                    default:
                        throw semantic_error();
                }

                try { expr.annotation().value = evaluate(expr.clone()); } catch (evaluator::generic_evaluation&) {}
            }
            else if (!expr.expression()->annotation().type->declaration()) {
                expr.invalid(true);
                expr.annotation().type = types::unknown();
                error(expr.member()->range(), diagnostic::format("There is no symbol `$` associated to type `$`, idiot!", item, expr.expression()->annotation().type->string()));
            }
            else {
                expr.member()->annotation().mustvalue = 0;
                if (expr.expression()->annotation().associated) expr.member()->annotation().associated = expr.expression()->annotation().associated;
                else expr.member()->annotation().associated = scopes_.at(expr.expression()->annotation().type->declaration());
                expr.member()->annotation().substitution = expr.expression()->annotation().substitution;
                expr.member()->accept(*this);
                expr.annotation().type = expr.member()->annotation().type;
                expr.annotation().value = expr.member()->annotation().value;
                expr.annotation().istype = expr.member()->annotation().istype;
                expr.annotation().referencing = expr.member()->annotation().referencing;
            }
        }
        else if (!expr.expression()->annotation().type || expr.expression()->annotation().type->category() == ast::type::category::unknown_type) {
            expr.invalid(true);
            expr.annotation().type = types::unknown();
            throw semantic_error();
        }
        else if (expr.expression()->annotation().type->declaration() || (expr.expression()->annotation().type->category() == ast::type::category::pointer_type && std::static_pointer_cast<ast::pointer_type>(expr.expression()->annotation().type)->base()->declaration())) {            
            ast::pointer<ast::type> object_type = expr.expression()->annotation().type;
            bool resolved = false;
            // dereference pointer
            if (auto ptrty = std::dynamic_pointer_cast<ast::pointer_type>(expr.expression()->annotation().type)) {
                //expr.expression() = implicit_cast(ptrty->base(), expr.expression());
                object_type = ptrty->base();
            }
            // The order of precedence for members of an object is
            // [1] Fields
            if (object_type->category() == ast::type::category::structure_type) {
                auto structure_type = std::static_pointer_cast<ast::structure_type>(object_type);
                for (unsigned i = 0; !resolved && i < structure_type->fields().size(); ++i) {
                    if (structure_type->fields().at(i).name == item) {
                        expr.annotation().type = expr.member()->annotation().type = structure_type->fields().at(i).type;
                        expr.annotation().referencing = expr.member()->annotation().referencing = static_cast<const ast::record_declaration*>(structure_type->declaration())->fields().at(i).get();
                        resolved = true;
                    }
                }
            }
            // [2] Methods
            if (!resolved) {
                auto saved = scope_;
                scope_ = scopes_.at(object_type->declaration());
                expr.member()->annotation().mustvalue = false;
                expr.member()->accept(*this);
                expr.member()->annotation().mustvalue = true;
                scope_ = saved;
                // if symbol is a type it cannot be accessed through instance but must be accessed through its type
                if (auto type = dynamic_cast<const ast::type_declaration*>(expr.member()->annotation().referencing)) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(expr.range().begin())
                                .message("You can't access nested type from instance, pr*ck!")
                                .highlight(expr.member()->range())
                                .replacement(expr.expression()->range(), impl::to_identifier_type_name(object_type->string()), diagnostic::format("Nested type `$` must be accessed directly from type `$`.", type->name().lexeme(), object_type->string()))
                                .build();
                        
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    publisher().publish(diag);
                    throw semantic_error();
                }
                // if symbol is a constant it cannot be accessed through instance but must be accessed through its type
                else if (auto constant = dynamic_cast<const ast::const_declaration*>(expr.member()->annotation().referencing)) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(expr.range().begin())
                                .message("You can't access nested constant from instance, pr*ck!")
                                .highlight(expr.member()->range())
                                .replacement(expr.expression()->range(), impl::to_identifier_type_name(object_type->string()), diagnostic::format("Nested constant `$` must be accessed directly from type `$`.", constant->name().lexeme(), object_type->string()))
                                .build();
                        
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    publisher().publish(diag);
                    throw semantic_error();
                }
                else if (auto function = dynamic_cast<const ast::function_declaration*>(expr.member()->annotation().referencing)) {
                    resolved = true;
                    expr.annotation() = expr.member()->annotation();

                    if (function->parameters().size() == 0 || !types::assignment_compatible(function->parameters().front()->annotation().type, expr.expression()->annotation().type)) {
                        auto diag = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(expr.range().begin())
                                    .message(diagnostic::format("You can't call shared function `$` associated to type `$` like a method, b*tch!", function->name().lexeme(), expr.expression()->annotation().type->string()))
                                    .highlight(expr.member()->range())
                                    .note(function->name().range(), diagnostic::format("You can see `$` is a shared function because it doesn't take first parameter of type `$`.", function->name().lexeme(), expr.expression()->annotation().type->string()))
                                    .replacement(expr.expression()->range(), impl::to_identifier_type_name(expr.expression()->annotation().type->string()), diagnostic::format("Shared function `$` must be accessed directly from type `$`.", function->name().lexeme(), expr.expression()->annotation().type->string()))
                                    .build();
                            
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        publisher().publish(diag);
                        throw semantic_error();
                    }
                }
                else if (auto property = dynamic_cast<const ast::property_declaration*>(expr.member()->annotation().referencing)) {
                    if (property->invalid()) {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        throw semantic_error();
                    }
                    else {
                        resolved = true;
                        // execution of property returns its result type
                        expr.annotation().type = std::static_pointer_cast<ast::function_type>(property->annotation().type)->result();
                        expr.annotation().referencing = property;
                        // implicit cast of passed object
                        if (auto implicit = implicit_cast(property->parameters().front()->annotation().type, expr.expression())) expr.expression() = implicit;
                    }
                }
            }
            // no way, no symbol found
            if (!resolved) {
                expr.invalid(true);
                expr.annotation().type = types::unknown();
                error(expr.member()->range(), diagnostic::format("There is no symbol `$` associated to type `$`, idiot!", item, object_type->string()));
            }
            // test that symbol is not hidden in current scope
            else if (expr.annotation().referencing->is_hidden() && !scope_->has_ancestor_scope(scopes_.at(object_type->declaration()))) {
                expr.invalid(true);
                auto diag = diagnostic::builder()
                                .location(expr.range().begin())
                                .severity(diagnostic::severity::error)
                                .message(diagnostic::format("You cannot access hidden symbol `$`, c*nt!", item))
                                .highlight(expr.expression()->range(), diagnostic::highlighter::mode::light)
                                .highlight(expr.member()->range(), "inaccessible")
                                .build();
                
                publisher().publish(diag);
                throw semantic_error();
            }
        }
        else {
            switch (expr.expression()->annotation().type->category()) {
                case ast::type::category::rational_type:
                    if (item == "numerator" || item == "denominator") expr.annotation().type = types::sint(std::static_pointer_cast<ast::rational_type>(expr.expression()->annotation().type)->bits() / 2);
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr.member()->range(), diagnostic::format("There is no symbol `$` associated to builtin type `$`, idiot!", item, expr.expression()->annotation().type->string()));
                    }
                    break;
                case ast::type::category::complex_type:
                    if (item == "real" || item == "imaginary") expr.annotation().type = types::floating(std::static_pointer_cast<ast::complex_type>(expr.expression()->annotation().type)->bits() / 2);
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr.member()->range(), diagnostic::format("There is no symbol `$` associated to builtin type `$`, idiot!", item, expr.expression()->annotation().type->string()));
                    }
                    break;
                case ast::type::category::array_type:
                case ast::type::category::slice_type:
                case ast::type::category::tuple_type:
                    if (item == "size") expr.annotation().type = types::usize();
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr.member()->range(), diagnostic::format("There is no symbol `$` associated to type `$`, idiot!", item, expr.expression()->annotation().type->string()));
                    }
                    break;
                case ast::type::category::string_type:
                case ast::type::category::chars_type:
                    if (item == "size" || item == "length") expr.annotation().type = types::usize();
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr.member()->range(), diagnostic::format("There is no symbol `$` associated to builtin type `$`, idiot!", item, expr.expression()->annotation().type->string()));
                    }
                    break;
                case ast::type::category::structure_type:
                {
                    auto structure_type = std::static_pointer_cast<ast::structure_type>(expr.expression()->annotation().type);
                    bool resolved = false;
                    for (unsigned i = 0; !resolved && i < structure_type->fields().size(); ++i) {
                        if (structure_type->fields().at(i).name == item) {
                            expr.annotation().type = expr.member()->annotation().type = structure_type->fields().at(i).type;
                            resolved = true;
                        }
                    }
                    if (!resolved) {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr.member()->range(), diagnostic::format("There is no symbol `$` associated to type `$`, idiot!", item, expr.expression()->annotation().type->string()));
                    }
                    break;
                }
                default:
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    if (types::builtin(expr.expression()->annotation().type->string())) error(expr.member()->range(), diagnostic::format("There is no symbol `$` associated to builtin type `$`, idiot!", item, expr.expression()->annotation().type->string()));
                    else error(expr.member()->range(), diagnostic::format("There is no symbol `$` associated to type `$`, idiot!", item, expr.expression()->annotation().type->string()));
            }
        }

        // check that this is resolved as a value when a value is expected
        if (expr.annotation().mustvalue && expr.annotation().istype) {
            expr.invalid(true);
            error(expr.range(), diagnostic::format("I was expecting a value but I found type `$` instead!", expr.annotation().type->string()), "", "expected value");
        }

        // check that this is resolved as a type when a type is expected
        if (expr.annotation().musttype && !expr.annotation().istype) {
            expr.invalid(true);
            error(expr.range(), "I was expecting a type but I found value instead!", "", "expected type");
        }
    }
    
    void checker::visit(const ast::array_index_expression& expr)
    {
        expr.annotation().type = types::unknown();

        expr.expression()->accept(*this);
        expr.index()->accept(*this);

        if (!expr.expression()->annotation().type || expr.expression()->annotation().type->category() == ast::type::category::unknown_type ||
            !expr.index()->annotation().type || expr.index()->annotation().type->category() == ast::type::category::unknown_type) {
            expr.invalid(true);
            throw semantic_error();
        }

        ast::pointer<ast::type> base;

        if (auto array_type = std::dynamic_pointer_cast<ast::array_type>(expr.expression()->annotation().type)) base = array_type->base();
        else if (auto slice_type = std::dynamic_pointer_cast<ast::slice_type>(expr.expression()->annotation().type)) base = slice_type->base();
        else error(expr.expression()->range(), diagnostic::format("I was expecting an array or slice for indexing, I found `$`, f*cker!", expr.expression()->annotation().type->string()), "", "expected array or slice");

        if (auto rngtype = std::dynamic_pointer_cast<ast::range_type>(expr.index()->annotation().type)) {
            if (rngtype->base()->category() != ast::type::category::integer_type) {
                auto diag = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(expr.index()->range().begin())
                            .message(diagnostic::format("Array range must have integer bounds, I found `$`, you idiot!", rngtype->base()->string()))
                            .highlight(expr.index()->range(), "expected integer")
                            .highlight(expr.expression()->range(), diagnostic::highlighter::mode::light)
                            .build();
                    
                publisher().publish(diag);
                expr.invalid(true);
                throw semantic_error();
            }
            else {
                expr.annotation().type = types::slice(base);
            }
        }
        else if (expr.index()->annotation().type->category() != ast::type::category::integer_type) {
            auto diag = diagnostic::builder()
                        .severity(diagnostic::severity::error)
                        .location(expr.index()->range().begin())
                        .message(diagnostic::format("Array index must have integer type, I found `$`, c*nt!", expr.index()->annotation().type->string()))
                        .highlight(expr.index()->range(), "expected integer")
                        .highlight(expr.expression()->range(), diagnostic::highlighter::mode::light)
                        .build();
                
            publisher().publish(diag);
            expr.invalid(true);
            throw semantic_error();
        }
        else {
            expr.annotation().type = base;
        }
    }
    
    void checker::visit(const ast::tuple_index_expression& expr)
    {
        expr.annotation().type = types::unknown();

        expr.expression()->accept(*this);

        if (!expr.expression()->annotation().type || expr.expression()->annotation().type->category() == ast::type::category::unknown_type) {
            expr.invalid(true);
            throw semantic_error();
        }

        ast::pointer<ast::tuple_type> tuple_type = nullptr;

        if (auto tuple = std::dynamic_pointer_cast<ast::tuple_type>(expr.expression()->annotation().type)) tuple_type = tuple;
        else if (auto pointer = std::dynamic_pointer_cast<ast::pointer_type>(expr.expression()->annotation().type)) {
            if (auto tuple = std::dynamic_pointer_cast<ast::tuple_type>(pointer->base())) tuple_type = tuple;
        }
        
        if (!tuple_type) error(expr.expression()->range(), diagnostic::format("I was expecting a tuple for indexing, I found `$`, f*cker!", expr.expression()->annotation().type->string()), "", "expected tuple");

        std::size_t index = evaluator::integer_parse(expr.index().lexeme().string()).i.value();

        if (index < 0 || index >= tuple_type->components().size()) {
            auto diag = diagnostic::builder()
                        .severity(diagnostic::severity::error)
                        .location(expr.index().range().begin())
                        .message(diagnostic::format("Tuple index must be in range `0`..`$`, c*nt!", tuple_type->components().size()))
                        .highlight(expr.index().range(), "out of range")
                        .highlight(expr.expression()->range(), diagnostic::highlighter::mode::light)
                        .build();
                
            publisher().publish(diag);
            expr.invalid(true);
            throw semantic_error();
        }
        else {
            expr.annotation().type = tuple_type->components().at(index);
        }
    }

    void checker::visit(const ast::record_expression& expr)
    {
        expr.annotation().type = types::unknown();

        if (expr.callee()) {
            expr.callee()->annotation().mustvalue = false;
            expr.callee()->annotation().musttype = true;
            expr.callee()->accept(*this);

            if (!expr.callee()->annotation().type || expr.callee()->annotation().type->category() == ast::type::category::unknown_type) {
                expr.invalid(true);
                throw semantic_error();
            }
            else if (!expr.callee()->annotation().istype) {
                expr.invalid(true);
                error(expr.callee()->range(), "I need a type here, but I found a value, idiot!", "", "expected type");
            }
            else if (expr.callee()->annotation().type->category() != ast::type::category::structure_type) {
                auto builder = diagnostic::builder()
                                .location(expr.callee()->range().begin())
                                .severity(diagnostic::severity::error)
                                .message(diagnostic::format("Only structure types can be constructed this way, but I found type `$`.", expr.callee()->annotation().type->string()))
                                .highlight(expr.callee()->range(), "expected structure");

                if (auto typedecl = dynamic_cast<const ast::type_declaration*>(expr.callee()->annotation().type->declaration())) {
                    builder.note(typedecl->name().range(), diagnostic::format("As you can see type `$` is not declared as a structure.", typedecl->annotation().type->string()));
                }

                publisher().publish(builder.build());
                expr.invalid(true);
                throw semantic_error();
            }

            expr.annotation().type = expr.callee()->annotation().type;

            std::unordered_map<std::string, bool> initialized;
            std::unordered_map<std::string, token> names;
            auto structure = std::static_pointer_cast<ast::structure_type>(expr.callee()->annotation().type);

            for (auto field : structure->fields()) initialized.emplace(field.name, false);

            for (auto& init : expr.initializers()) {
                auto name = init.field().lexeme().string();
                auto is_initialized = initialized.find(name);
                
                if (is_initialized == initialized.end()) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(init.field().location())
                                .message(diagnostic::format("There is no such field named `$` in structure `$`, sh*thead!", name, structure->string()))
                                .highlight(init.field().range(), "inexistent");

                    std::ostringstream explanation;
                    unsigned similars = 0;
                    
                    for (auto pair : initialized) {
                        if (utils::levenshtein_distance(name, pair.first) < 2) {
                            explanation << " \\ • " << pair.first;
                            ++similars;
                        }
                    }

                    if (similars > 0) {
                        builder.explanation("Maybe you mean any of these fields:" + explanation.str());
                    }

                    if (auto typedecl = dynamic_cast<const ast::type_declaration*>(structure->declaration())) {
                        builder.note(typedecl->name().range(), diagnostic::format("Take a look at type `$` declaration.", structure->string()));
                    }
                        
                    publisher().publish(builder.build());
                    expr.invalid(true);
                }
                else if (is_initialized->second) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(init.field().location())
                                .message(diagnostic::format("Field `$` has already been initialized, idiot!", name))
                                .highlight(init.field().range())
                                .highlight(names.at(name).range(), diagnostic::highlighter::mode::light)
                                .build();

                    publisher().publish(diag);
                    expr.invalid(true);
                }
                else {
                    if (structure->declaration()) {
                        auto items = static_cast<const ast::record_declaration*>(structure->declaration())->fields();
                        auto item = std::find_if(items.begin(), items.end(), [&] (ast::pointer<ast::declaration> decl) { return std::static_pointer_cast<ast::field_declaration>(decl)->name().lexeme().string() == name; });
                        
                        if ((item->get()->is_hidden() || name[0] == '_') && !scope_->has_ancestor_scope(scopes_.at(structure->declaration()))) {
                            auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(init.field().location())
                                        .message(diagnostic::format("You can't initialize hidden field `$`, dammit!", name))
                                        .highlight(init.field().range(), "inaccessible")
                                        .note(static_cast<const ast::field_declaration*>(item->get())->name().range(), diagnostic::format("Look at field `$` declaration, idiot.", name))
                                        .build();

                            publisher().publish(diag);
                            expr.invalid(true);
                        }
                    }

                    auto type = std::find_if(structure->fields().begin(), structure->fields().end(), [&] (ast::structure_type::component field) { return field.name == name; })->type;

                    init.value()->accept(*this);

                    if (!types::assignment_compatible(type, init.value()->annotation().type)) {
                        auto diag = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(init.field().location())
                                    .message(diagnostic::format("Field `$` has type `$`, but I found `$`, dammit!", name, type->string(), init.value()->annotation().type->string()))
                                    .highlight(init.field().range(), diagnostic::highlighter::mode::light)
                                    .highlight(init.value()->range(), diagnostic::format("expected `$`", type->string()))
                                    .build();

                        publisher().publish(diag);
                        expr.invalid(true);
                    }
                    else {
                        if (auto implicit = implicit_cast(type, init.value())) {
                            init.value() = implicit;
                        }

                        initialized.at(name) = true;
                        names.emplace(name, init.field());
                    }
                }
            }

            if (names.size() < structure->fields().size()) {
                std::ostringstream oss;
                unsigned int count = 0;

                for (auto pair : initialized) {
                    if (!pair.second) {
                        if (count == 0) oss << "`" << pair.first << "`";
                        else oss << ", `" << pair.first << "`";
                        ++count;
                    }
                }

                error(expr.callee()->range(), diagnostic::format("You forgot to initialize field $, idiot.", oss.str()));
            }
        }
        else {
            std::unordered_map<std::string, token> names;
            ast::structure_type::components fields;

            for (auto& init : expr.initializers()) {
                auto name = init.field().lexeme().string();
                auto result = names.find(name);
                
                if (result != names.end()) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(init.field().location())
                                .message(diagnostic::format("Field `$` has already been initialized, idiot!", name))
                                .highlight(init.field().range())
                                .highlight(result->second.range(), diagnostic::highlighter::mode::light)
                                .build();

                    publisher().publish(diag);
                    expr.invalid(true);
                }
                else {
                    init.value()->accept(*this);

                    if (!init.value()->annotation().type || init.value()->annotation().type->category() == ast::type::category::unknown_type);
                    else fields.emplace_back(name, init.value()->annotation().type);

                    names.emplace(name, init.field());
                }
            }

            expr.annotation().type = types::record(fields);
        }
    }

    void checker::visit(const ast::unary_expression& expr) 
    {
        if (expr.invalid()) throw semantic_error();

        expr.expression()->accept(*this);

        if (expr.expression()->annotation().type->category() == ast::type::category::unknown_type) throw semantic_error();

        auto rtype = expr.expression()->annotation().type;

        switch (expr.unary_operator().kind()) {
            case token::kind::plus:
                if (rtype->category() == ast::type::category::integer_type ||
                    rtype->category() == ast::type::category::rational_type ||
                    rtype->category() == ast::type::category::float_type ||
                    rtype->category() == ast::type::category::complex_type) {
                    expr.annotation().type = rtype;
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Unary operator `+` requires an arithmetic type, I found type `$` instead!", rtype->string()), "", "expected arithmetic");
                }
                break;
            case token::kind::minus:
                if (rtype->category() == ast::type::category::integer_type) {
                    expr.annotation().type = types::sint(std::static_pointer_cast<ast::integer_type>(rtype)->bits());
                }
                else if (rtype->category() == ast::type::category::rational_type ||
                         rtype->category() == ast::type::category::float_type ||
                         rtype->category() == ast::type::category::complex_type) {
                    expr.annotation().type = rtype;
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Unary operator `-` requires an arithmetic type, I found type `$` instead!", rtype->string()), "", "expected arithmetic");
                }
                break;
            case token::kind::tilde:
                if (rtype->category() == ast::type::category::integer_type) {
                    expr.annotation().type = rtype;
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Unary operator `~` requires an integer type, I found type `$` instead!", rtype->string()), "", "expected integer");
                }
                break;
            case token::kind::bang:
                if (rtype->category() == ast::type::category::bool_type) {
                    expr.annotation().type = rtype;
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Unary operator `!` requires a boolean type, I found type `$` instead!", rtype->string()), "", "expected bool");
                }
                break;
            case token::kind::plus_plus:
                if (rtype->category() == ast::type::category::integer_type) {
                    expr.annotation().type = rtype;
                    immutability(expr);
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Increment operator `++` requires an integer type, I found type `$` instead!", rtype->string()), "", "expected integer");
                }
                break;
            case token::kind::minus_minus:
                if (rtype->category() == ast::type::category::integer_type) {
                    expr.annotation().type = rtype;
                    immutability(expr);
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Decrement operator `--` requires an integer type, I found type `$` instead!", rtype->string()), "", "expected integer");
                }
                break;
            case token::kind::amp:
                if (!expr.expression()->lvalue()) {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("You cannot take the address of a temporary object of type `$`, idiot!", rtype->string()), "", "expected lvalue");
                }
                else expr.annotation().type = types::pointer(rtype);
                break;
            case token::kind::star:
                if (rtype->category() != ast::type::category::pointer_type) {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Dereference operator `*` requires a pointer type, I found type `$` instead!", rtype->string()), "", "expected pointer");
                }
                else expr.annotation().type = std::static_pointer_cast<ast::pointer_type>(rtype)->base();
                break;
            default:
                expr.annotation().type = types::unknown();
                break;
        }
    }

    void checker::visit(const ast::binary_expression& expr) 
    {
        if (expr.invalid()) throw semantic_error();

        expr.left()->accept(*this);
        expr.right()->accept(*this);

        if (!expr.left()->annotation().type || expr.left()->annotation().type->category() == ast::type::category::unknown_type || !expr.right()->annotation().type || expr.right()->annotation().type->category() == ast::type::category::unknown_type) throw semantic_error();

        auto lefttype = expr.left()->annotation().type, righttype = expr.right()->annotation().type;

        switch (expr.binary_operator().kind()) {
            case token::kind::plus:
                if (lefttype->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        if (ltype->is_signed() || rtype->is_signed()) expr.annotation().type = types::sint(bits);
                        else expr.annotation().type = types::uint(bits);
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::rational(std::max(2 * ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        expr.annotation().type = types::floating(bits);
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(2 * ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::pointer_type) {
                        expr.annotation().type = righttype;
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot add `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::rational_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::rational_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = types::rational(std::max(ltype->bits(), 2 * rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::rational(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits() / 2, rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot add `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::float_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::float_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        expr.annotation().type = types::floating(bits);
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits(), rtype->bits() / 2));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits() * 2, rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot add `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::complex_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::complex_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot add `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::char_type) {
                    if (righttype->category() == ast::type::category::chars_type) {
                        expr.annotation().type = types::string();
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::string_type) expr.annotation().type = types::string();
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot add `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::chars_type) {
                    if (righttype->category() == ast::type::category::char_type) expr.annotation().type = types::string();
                    else if (righttype->category() == ast::type::category::chars_type) {
                        expr.annotation().type = types::string();
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::string_type) {
                        expr.annotation().type = types::string();
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot add `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::string_type) {
                    if (righttype->category() == ast::type::category::char_type) expr.annotation().type = types::string();
                    else if (righttype->category() == ast::type::category::chars_type || righttype->category() == ast::type::category::string_type) {
                        expr.annotation().type = types::string();
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot add `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::pointer_type) {
                    if (righttype->category() == ast::type::category::integer_type) expr.annotation().type = lefttype;
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot add `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("You cannot add `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                }

                break;
            case token::kind::minus:
                if (lefttype->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        expr.annotation().type = types::sint(bits);
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::rational(std::max(2 * ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        expr.annotation().type = types::floating(bits);
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(2 * ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot subtract `$` from `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::rational_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::rational_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = types::rational(std::max(ltype->bits(), 2 * rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::rational(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits() / 2, rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot subtract `$` from `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::float_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::float_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        expr.annotation().type = types::floating(bits);
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits(), rtype->bits() / 2));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits() * 2, rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot subtract `$` from `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::complex_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::complex_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot subtract `$` from `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::pointer_type) {
                    if (righttype->category() == ast::type::category::integer_type) expr.annotation().type = lefttype;
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot subtract `$` from `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("You cannot subtract `$` from `$`, b*tch!", righttype->string(), lefttype->string()));
                }

                break;
            case token::kind::star:
                if (lefttype->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        if (ltype->is_signed() || rtype->is_signed()) expr.annotation().type = types::sint(bits);
                        else expr.annotation().type = types::uint(bits);
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::rational(std::max(2 * ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        expr.annotation().type = types::floating(bits);
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(2 * ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot multiply `$` by `$`, b*tch!", lefttype->string(), righttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::rational_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::rational_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = types::rational(std::max(ltype->bits(), 2 * rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::rational(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits() / 2, rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot multiply `$` by `$`, b*tch!", lefttype->string(), righttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::float_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::float_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        size_t bits = std::max(ltype->bits(), rtype->bits());
                        expr.annotation().type = types::floating(bits);
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits(), rtype->bits() / 2));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits() * 2, rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot multiply `$` by `$`, b*tch!", lefttype->string(), righttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::complex_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::complex_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot multiply `$` by `$`, b*tch!", lefttype->string(), righttype->string()));
                    }
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("You cannot multiply `$` by `$`, b*tch!", lefttype->string(), righttype->string()));
                }

                break;
            case token::kind::slash:
                if (lefttype->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        auto common = types::sint(std::max(ltype->bits(), rtype->bits()));
                        expr.annotation().type = types::rational(2 * std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(common, expr.left());
                        expr.right() = implicit_forced_cast(common, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::rational(std::max(2 * ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(2 * ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot divide `$` by `$`, b*tch!", lefttype->string(), righttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::rational_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::rational_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = types::rational(std::max(ltype->bits(), 2 * rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::rational(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits() / 2, rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot divide `$` by `$`, b*tch!", lefttype->string(), righttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::float_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::float_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits(), rtype->bits() / 2));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits() * 2, rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot divide `$` by `$`, b*tch!", lefttype->string(), righttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::complex_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::complex_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot divide `$` by `$`, b*tch!", lefttype->string(), righttype->string()));
                    }
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("You cannot divide `$` by `$`, b*tch!", lefttype->string(), righttype->string()));
                }

                break;
            case token::kind::percent:
                if (lefttype->category() == ast::type::category::integer_type && righttype->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                    auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                    if (ltype->is_signed() || rtype->is_signed()) expr.annotation().type = types::sint(std::max(ltype->bits(), rtype->bits()));
                    else expr.annotation().type = types::uint(std::max(ltype->bits(), rtype->bits()));
                    expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                    expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("You cannot mod `$` by `$`, b*tch!", lefttype->string(), righttype->string()));
                }

                break;
            case token::kind::star_star:
                if (lefttype->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits(), rtype->bits() / 2));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(2 * ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot raise `$` to `$`, b*tch!", lefttype->string(), righttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::rational_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::rational_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits() / 2, rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits() / 2, rtype->bits() / 2));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits() / 2, rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot raise `$` to `$`, b*tch!", lefttype->string(), righttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::float_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::float_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits(), rtype->bits() / 2));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = types::floating(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits() * 2, rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot raise `$` to `$`, b*tch!", lefttype->string(), righttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::complex_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::complex_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), 2 * rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = types::complex(std::max(ltype->bits(), rtype->bits()));
                        expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                        expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot raise `$` to `$`, b*tch!", lefttype->string(), righttype->string()));
                    }
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("You cannot raise `$` to `$`, b*tch!", lefttype->string(), righttype->string()));
                }
                
                break;
            case token::kind::less_less:
                if (lefttype->category() == ast::type::category::integer_type && righttype->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                    auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype); 
                    expr.annotation().type = ltype;
                    expr.right() = implicit_forced_cast(ltype, expr.right());
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Shift operator `<<` requires integer types, I found `$` and `$` instead!", lefttype->string(), righttype->string()));
                }

                break;
            case token::kind::greater_greater:
                if (lefttype->category() == ast::type::category::integer_type && righttype->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                    auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                    expr.annotation().type = ltype;
                    expr.right() = implicit_forced_cast(ltype, expr.right());
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Shift operator `>>` requires integer types, I found `$` and `$` instead!", lefttype->string(), righttype->string()));
                }

                break;
            case token::kind::as_kw:
                if (auto variant = std::dynamic_pointer_cast<ast::variant_type>(lefttype)) {
                    if (variant->contains(righttype)) warning(expr, diagnostic::format("Explicit conversion from `$` to `$` may crash at run-time!", lefttype->string(), righttype->string()));
                    else error(expr, diagnostic::format("Variant `$` does not include `$` among its types so conversion is not possible, idiot!", lefttype->string(), righttype->string()));
                    expr.annotation().type = righttype;
                }
                else if (lefttype->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = rtype;
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = rtype;
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = rtype;
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = rtype;
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot convert type `$` to `$`, c*nt!", lefttype->string(), righttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::rational_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::rational_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = rtype;
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = rtype;
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = rtype;
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = rtype;
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot convert type `$` to `$`, c*nt!", lefttype->string(), righttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::float_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::float_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = rtype;
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = rtype;
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = rtype;
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = rtype;
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot convert type `$` to `$`, c*nt!", lefttype->string(), righttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::complex_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::complex_type>(lefttype);
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = rtype;
                    }
                    else if (righttype->category() == ast::type::category::rational_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::rational_type>(righttype);
                        expr.annotation().type = rtype;
                    }
                    else if (righttype->category() == ast::type::category::float_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::float_type>(righttype);
                        expr.annotation().type = rtype;
                    }
                    else if (righttype->category() == ast::type::category::complex_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::complex_type>(righttype);
                        expr.annotation().type = rtype;
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot convert type `$` to `$`, c*nt!", lefttype->string(), righttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::char_type) {
                    if (righttype->category() == ast::type::category::integer_type) {
                        auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                        expr.annotation().type = rtype;
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot convert type `$` to `$`, c*nt!", lefttype->string(), righttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::chars_type) {
                    if (righttype->category() == ast::type::category::string_type) {
                        // TODO: explicit construction of string (dynamic-allocated) from byte array
                        expr.annotation().type = righttype;
                    }
                    else if (types::compatible(righttype, types::slice(types::uint(8)))) {
                        expr.annotation().type = righttype;
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot convert type `$` to `$`, c*nt!", lefttype->string(), righttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::string_type) {
                    if (righttype->category() == ast::type::category::chars_type) {
                        // TODO: explicit extraction of byte array reference from string
                        expr.annotation().type = righttype;
                    }
                    else if (types::compatible(righttype, types::slice(types::uint(8)))) {
                        expr.annotation().type = righttype;
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot convert type `$` to `$`, c*nt!", lefttype->string(), righttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::pointer_type && righttype->category() == ast::type::category::pointer_type) {
                    // FIXME: upcast and downcast
                    auto lpointee = std::static_pointer_cast<ast::pointer_type>(lefttype)->base();
                    auto rpointee = std::static_pointer_cast<ast::pointer_type>(righttype)->base();
                    if (rpointee->category() == ast::type::category::behaviour_type) {
                        auto behaviour = std::static_pointer_cast<ast::behaviour_type>(rpointee);
                        if (!behaviour->implementor(lpointee)) {
                            expr.invalid(true);
                            expr.annotation().type = types::unknown();
                            if (lpointee->category() != ast::type::category::behaviour_type) error(expr, diagnostic::format("You cannot convert type `$` to `$`, c*nt! \\ Type `$` must implements behaviour `$` to allow upcast!", lefttype->string(), righttype->string(), lpointee->string(), behaviour->string()));
                            else error(expr, diagnostic::format("You cannot convert type `$` to `$`, c*nt!", lefttype->string(), righttype->string(), lpointee->string(), behaviour->string()));
                        }
                        else expr.annotation().type = righttype;
                    }
                    else if (lpointee->category() == ast::type::category::behaviour_type) {
                        auto behaviour = std::static_pointer_cast<ast::behaviour_type>(lpointee);
                        if (!behaviour->implementor(rpointee)) {
                            expr.invalid(true);
                            expr.annotation().type = types::unknown();
                            error(expr, diagnostic::format("You cannot convert type `$` to `$`, c*nt! \\ Type `$` must implements behaviour `$` to allow downcast!", lefttype->string(), righttype->string(), rpointee->string(), behaviour->string()));
                        }
                        else {
                            warning(expr, diagnostic::format("Explicit conversion from `$` to `$` (alias downcast) may crash at run-time!", lefttype->string(), righttype->string()));
                            expr.annotation().type = righttype;
                        }
                    }
                }
                else if (lefttype->category() == ast::type::category::array_type && righttype->category() == ast::type::category::slice_type) {
                    if (types::compatible(std::static_pointer_cast<ast::array_type>(lefttype)->base(), std::static_pointer_cast<ast::slice_type>(righttype)->base())) {
                        expr.annotation().type = righttype;
                    }
                    else {
                        expr.invalid(true);
                        expr.annotation().type = types::unknown();
                        error(expr, diagnostic::format("You cannot convert type `$` to `$`, c*nt!", lefttype->string(), righttype->string()));
                    }
                }
                else if (types::assignment_compatible(lefttype, righttype)) {
                    expr.annotation().type = righttype;
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("You cannot convert type `$` to `$`, c*nt!", lefttype->string(), righttype->string()));
                }
                
                break;
            case token::kind::amp:
                if (lefttype->category() == ast::type::category::integer_type && righttype->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                    auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                    if (ltype->is_signed() || rtype->is_signed()) expr.annotation().type = types::sint(std::max(ltype->bits(), rtype->bits()));
                    else expr.annotation().type = types::uint(std::max(ltype->bits(), rtype->bits()));
                    expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                    expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                }
                else if (lefttype->category() == ast::type::category::bool_type && righttype->category() == ast::type::category::bool_type && expr.left()->annotation().isconcept && expr.right()->annotation().isconcept) {
                    expr.annotation().isconcept = true;
                    expr.annotation().type = types::boolean();
                    expr.annotation().value.type = types::boolean();
                    expr.annotation().value.b = expr.left()->annotation().value.b && expr.right()->annotation().value.b;
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Bitwise operator `&` requires integer types, I found `$` and `$` instead!", lefttype->string(), righttype->string()));
                }

                break;
            case token::kind::line:
                if (lefttype->category() == ast::type::category::integer_type && righttype->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                    auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                    if (ltype->is_signed() || rtype->is_signed()) expr.annotation().type = types::sint(std::max(ltype->bits(), rtype->bits()));
                    else expr.annotation().type = types::uint(std::max(ltype->bits(), rtype->bits()));
                    expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                    expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                }
                else if (lefttype->category() == ast::type::category::bool_type && righttype->category() == ast::type::category::bool_type && expr.left()->annotation().isconcept && expr.right()->annotation().isconcept) {
                    expr.annotation().isconcept = true;
                    expr.annotation().type = types::boolean();
                    expr.annotation().value.type = types::boolean();
                    expr.annotation().value.b = expr.left()->annotation().value.b || expr.right()->annotation().value.b;
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Bitwise operator `|` requires integer types, I found `$` and `$` instead!", lefttype->string(), righttype->string()));
                }

                break;
            case token::kind::caret:
                if (lefttype->category() == ast::type::category::integer_type && righttype->category() == ast::type::category::integer_type) {
                    auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                    auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                    if (ltype->is_signed() || rtype->is_signed()) expr.annotation().type = types::sint(std::max(ltype->bits(), rtype->bits()));
                    else expr.annotation().type = types::uint(std::max(ltype->bits(), rtype->bits()));
                    expr.left() = implicit_forced_cast(expr.annotation().type, expr.left());
                    expr.right() = implicit_forced_cast(expr.annotation().type, expr.right());
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Bitwise operator `^` requires integer types, I found `$` and `$` instead!", lefttype->string(), righttype->string()));
                }

                break;
            case token::kind::amp_amp:
                if (lefttype->category() == ast::type::category::bool_type && righttype->category() == ast::type::category::bool_type) {
                    expr.annotation().type = types::boolean();
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Bitwise operator `&&` requires boolean types, I found `$` and `$` instead!", lefttype->string(), righttype->string()));
                }
                break;
            case token::kind::line_line:
                if (lefttype->category() == ast::type::category::bool_type && righttype->category() == ast::type::category::bool_type) {
                    expr.annotation().type = types::boolean();
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Logic operator `||` requires boolean types, I found `$` and `$` instead!", lefttype->string(), righttype->string()));
                }
                break;
            case token::kind::equal_equal:
                if ((lefttype->category() == ast::type::category::chars_type && righttype->category() == ast::type::category::string_type) ||
                    (lefttype->category() == ast::type::category::string_type && righttype->category() == ast::type::category::chars_type) ||
                    lefttype->category() == righttype->category()) {
                    switch (lefttype->category()) {
                        case ast::type::category::integer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                            auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                            if ((ltype->is_signed() && !rtype->is_signed()) || (!ltype->is_signed() && rtype->is_signed())) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `==` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()), "", "different signedness");
                            }
                            else if (ltype->is_signed()) {
                                expr.annotation().type = types::boolean();
                            }
                            else {
                                expr.annotation().type = types::boolean();
                            }
                            break;
                        }
                        case ast::type::category::rational_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::float_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::complex_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::chars_type:
                        case ast::type::category::string_type:
                            expr.left() = implicit_forced_cast(types::string(), expr.left());
                            expr.right() = implicit_forced_cast(types::string(), expr.right());
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::char_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::bool_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::array_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::array_type>(lefttype);
                            auto rtype = std::dynamic_pointer_cast<ast::array_type>(righttype);
                            if (!types::compatible(ltype->base(), rtype->base())) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `==` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()), "", diagnostic::format("different component $ and $", ltype->base()->string(), rtype->base()->string()));
                            }
                            else if (ltype->size() != rtype->size()) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `==` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()), diagnostic::format("different sizes $ and $", ltype->size(), rtype->size()));
                            }
                            else {
                                expr.annotation().type = types::boolean();
                            }
                            break;
                        }
                        case ast::type::category::tuple_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::tuple_type>(lefttype);
                            auto rtype = std::dynamic_pointer_cast<ast::tuple_type>(righttype);
                            if (!types::compatible(lefttype, righttype)) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `==` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()));
                            }
                            else {
                                expr.annotation().type = types::boolean();
                            }
                            break;
                        }
                        case ast::type::category::pointer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::pointer_type>(lefttype);
                            auto rtype = std::dynamic_pointer_cast<ast::pointer_type>(righttype);
                            if (!types::compatible(ltype->base(), rtype->base())) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `==` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()), "", diagnostic::format("different bases $ and $", ltype->base()->string(), rtype->base()->string()));
                            }
                            else {
                                expr.annotation().type = types::boolean();
                            }
                            break;
                        }
                        default:
                            expr.invalid(true);
                            expr.annotation().type = types::unknown();
                            error(expr, diagnostic::format("Comparison operator `==` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()));
                    }
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Comparison operator `==` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()));
                }
                break;
            case token::kind::bang_equal:
                if ((lefttype->category() == ast::type::category::chars_type && righttype->category() == ast::type::category::string_type) ||
                    (lefttype->category() == ast::type::category::string_type && righttype->category() == ast::type::category::chars_type) ||
                    lefttype->category() == righttype->category()) {
                    switch (lefttype->category()) {
                        case ast::type::category::integer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                            auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                            if ((ltype->is_signed() && !rtype->is_signed()) || (!ltype->is_signed() && rtype->is_signed())) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `!=` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()), "", "different signedness");
                            }
                            else if (ltype->is_signed()) {
                                expr.annotation().type = types::boolean();
                            }
                            else {
                                expr.annotation().type = types::boolean();
                            }
                            break;
                        }
                        case ast::type::category::rational_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::float_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::complex_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::chars_type:
                        case ast::type::category::string_type:
                            expr.annotation().type = types::boolean();
                            expr.left() = implicit_forced_cast(types::string(), expr.left());
                            expr.right() = implicit_forced_cast(types::string(), expr.right());
                            break;
                        case ast::type::category::char_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::bool_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::array_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::array_type>(lefttype);
                            auto rtype = std::dynamic_pointer_cast<ast::array_type>(righttype);
                            if (!types::compatible(ltype->base(), rtype->base())) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `!=` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()), "", diagnostic::format("different component $ and $", ltype->base()->string(), rtype->base()->string()));
                            }
                            else if (ltype->size() != rtype->size()) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `!=` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()), diagnostic::format("different sizes $ and $", ltype->size(), rtype->size()));
                            }
                            else {
                                expr.annotation().type = types::boolean();
                            }
                            break;
                        }
                        case ast::type::category::tuple_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::tuple_type>(lefttype);
                            auto rtype = std::dynamic_pointer_cast<ast::tuple_type>(righttype);
                            if (!types::compatible(lefttype, righttype)) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `!=` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()));
                            }
                            else {
                                expr.annotation().type = types::boolean();
                            }
                            break;
                        }
                        case ast::type::category::pointer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::pointer_type>(lefttype);
                            auto rtype = std::dynamic_pointer_cast<ast::pointer_type>(righttype);
                            if (!types::compatible(ltype->base(), rtype->base())) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `!=` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()), "", diagnostic::format("different bases $ and $", ltype->base()->string(), rtype->base()->string()));
                            }
                            else {
                                expr.annotation().type = types::boolean();
                            }
                            break;
                        }
                        default:
                            expr.invalid(true);
                            expr.annotation().type = types::unknown();
                            error(expr, diagnostic::format("Comparison operator `!=` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()));
                    }
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Comparison operator `!=` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()));
                }
                break;
            case token::kind::less:
                if ((lefttype->category() == ast::type::category::chars_type && righttype->category() == ast::type::category::string_type) ||
                    (lefttype->category() == ast::type::category::string_type && righttype->category() == ast::type::category::chars_type) ||
                    lefttype->category() == righttype->category()) {
                    switch (lefttype->category()) {
                        case ast::type::category::integer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                            auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                            if ((ltype->is_signed() && !rtype->is_signed()) || (!ltype->is_signed() && rtype->is_signed())) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `<` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()), "", "different signedness");
                            }
                            else if (ltype->is_signed()) expr.annotation().type = types::boolean();
                            else expr.annotation().type = types::boolean();
                            break;
                        }
                        case ast::type::category::rational_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::float_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::chars_type:
                        case ast::type::category::string_type:
                            expr.annotation().type = types::boolean();
                            expr.left() = implicit_forced_cast(types::string(), expr.left());
                            expr.right() = implicit_forced_cast(types::string(), expr.right());
                            break;
                        case ast::type::category::char_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::pointer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::pointer_type>(lefttype);
                            auto rtype = std::dynamic_pointer_cast<ast::pointer_type>(righttype);
                            if (!types::compatible(ltype->base(), rtype->base())) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `<` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()), "", diagnostic::format("different bases $ and $", ltype->base()->string(), rtype->base()->string()));
                            }
                            else {
                                expr.annotation().type = types::boolean();
                            }
                            break;
                        }
                        default:
                            expr.invalid(true);
                            expr.annotation().type = types::unknown();
                            error(expr, diagnostic::format("Comparison operator `<` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()));
                    }
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Comparison operator `<` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()));
                }
                break;
            case token::kind::less_equal:
                if ((lefttype->category() == ast::type::category::chars_type && righttype->category() == ast::type::category::string_type) ||
                    (lefttype->category() == ast::type::category::string_type && righttype->category() == ast::type::category::chars_type) ||
                    lefttype->category() == righttype->category()) {
                    switch (lefttype->category()) {
                        case ast::type::category::integer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                            auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                            if ((ltype->is_signed() && !rtype->is_signed()) || (!ltype->is_signed() && rtype->is_signed())) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `<=` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()), "", "different signedness");
                            }
                            else if (ltype->is_signed()) expr.annotation().type = types::boolean();
                            else expr.annotation().type = types::boolean();
                            break;
                        }
                        case ast::type::category::rational_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::float_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::chars_type:
                        case ast::type::category::string_type:
                            expr.annotation().type = types::boolean();
                            expr.left() = implicit_forced_cast(types::string(), expr.left());
                            expr.right() = implicit_forced_cast(types::string(), expr.right());
                            break;
                        case ast::type::category::char_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::pointer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::pointer_type>(lefttype);
                            auto rtype = std::dynamic_pointer_cast<ast::pointer_type>(righttype);
                            if (!types::compatible(ltype->base(), rtype->base())) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `<=` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()), "", diagnostic::format("different bases $ and $", ltype->base()->string(), rtype->base()->string()));
                            }
                            else {
                                expr.annotation().type = types::boolean();
                            }
                            break;
                        }
                        default:
                            expr.invalid(true);
                            expr.annotation().type = types::unknown();
                            error(expr, diagnostic::format("Comparison operator `<=` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()));
                    }
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Comparison operator `<=` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()));
                }
                break;
            case token::kind::greater:
                if ((lefttype->category() == ast::type::category::chars_type && righttype->category() == ast::type::category::string_type) ||
                    (lefttype->category() == ast::type::category::string_type && righttype->category() == ast::type::category::chars_type) ||
                    lefttype->category() == righttype->category()) {
                    switch (lefttype->category()) {
                        case ast::type::category::integer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                            auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                            if ((ltype->is_signed() && !rtype->is_signed()) || (!ltype->is_signed() && rtype->is_signed())) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `>` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()), "", "different signedness");
                            }
                            else if (ltype->is_signed()) expr.annotation().type = types::boolean();
                            else expr.annotation().type = types::boolean();
                            break;
                        }
                        case ast::type::category::rational_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::float_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::chars_type:
                        case ast::type::category::string_type:
                            expr.annotation().type = types::boolean();
                            expr.left() = implicit_forced_cast(types::string(), expr.left());
                            expr.right() = implicit_forced_cast(types::string(), expr.right());
                            break;
                        case ast::type::category::char_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::pointer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::pointer_type>(lefttype);
                            auto rtype = std::dynamic_pointer_cast<ast::pointer_type>(righttype);
                            if (!types::compatible(ltype->base(), rtype->base())) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `>` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()), "", diagnostic::format("different bases $ and $", ltype->base()->string(), rtype->base()->string()));
                            }
                            else {
                                expr.annotation().type = types::boolean();
                            }
                            break;
                        }
                        default:
                            expr.invalid(true);
                            expr.annotation().type = types::unknown();
                            error(expr, diagnostic::format("Comparison operator `>` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()));
                    }
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Comparison operator `>` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()));
                }
                break;
            case token::kind::greater_equal:
                if ((lefttype->category() == ast::type::category::chars_type && righttype->category() == ast::type::category::string_type) ||
                    (lefttype->category() == ast::type::category::string_type && righttype->category() == ast::type::category::chars_type) ||
                    lefttype->category() == righttype->category()) {
                    switch (lefttype->category()) {
                        case ast::type::category::integer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::integer_type>(lefttype);
                            auto rtype = std::dynamic_pointer_cast<ast::integer_type>(righttype);
                            if ((ltype->is_signed() && !rtype->is_signed()) || (!ltype->is_signed() && rtype->is_signed())) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `>=` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()), "", "different signedness");
                            }
                            else if (ltype->is_signed()) expr.annotation().type = types::boolean();
                            else expr.annotation().type = types::boolean();
                            break;
                        }
                        case ast::type::category::rational_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::float_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::chars_type:
                        case ast::type::category::string_type:
                            expr.annotation().type = types::boolean();
                            expr.left() = implicit_forced_cast(types::string(), expr.left());
                            expr.right() = implicit_forced_cast(types::string(), expr.right());
                            break;
                        case ast::type::category::char_type:
                            expr.annotation().type = types::boolean();
                            break;
                        case ast::type::category::pointer_type:
                        {
                            auto ltype = std::dynamic_pointer_cast<ast::pointer_type>(lefttype);
                            auto rtype = std::dynamic_pointer_cast<ast::pointer_type>(righttype);
                            if (!types::compatible(ltype->base(), rtype->base())) {
                                expr.invalid(true);
                                expr.annotation().type = types::unknown();
                                error(expr, diagnostic::format("Comparison operator `>=` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()), "", diagnostic::format("different bases $ and $", ltype->base()->string(), rtype->base()->string()));
                            }
                            else {
                                expr.annotation().type = types::boolean();
                            }
                            break;
                        }
                        default:
                            expr.invalid(true);
                            expr.annotation().type = types::unknown();
                            error(expr, diagnostic::format("Comparison operator `>=` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()));
                    }
                }
                else {
                    expr.invalid(true);
                    expr.annotation().type = types::unknown();
                    error(expr, diagnostic::format("Comparison operator `>=` cannot be used with `$` and `$`, idiot!", lefttype->string(), righttype->string()));
                }
                break;
            default:
                expr.annotation().type = types::unknown();
                throw semantic_error();
        }
    }

    void checker::visit(const ast::range_expression& expr)
    {
        expr.annotation().type = types::unknown();
        if (expr.start()) {
            expr.start()->accept(*this);
            switch (expr.start()->annotation().type->category()) {
            case ast::type::category::integer_type:
            case ast::type::category::float_type:
            case ast::type::category::char_type:
                break;
            default:
                error(expr.start()->range(), diagnostic::format("Type `$` is not suitable for ranges. I would use integers, reals or characters, dammit!", expr.start()->annotation().type->string()), "", "expected integer, real or character");
            }
        }

        if (expr.end()) {
            expr.end()->accept(*this);

            switch (expr.end()->annotation().type->category()) {
            case ast::type::category::integer_type:
            case ast::type::category::float_type:
            case ast::type::category::char_type:
                break;
            default:
                error(expr.end()->range(), diagnostic::format("Type `$` is not suitable for ranges. I would use integers, reals or characters, dammit!", expr.end()->annotation().type->string()), "", "expected integer, real or character");
            }
        }

        ast::pointer<ast::type> base = types::sint();

        if (expr.start() && expr.end()) {
            if (types::assignment_compatible(expr.start()->annotation().type, expr.end()->annotation().type)) {
                base = expr.start()->annotation().type;
                if (auto implicit = implicit_cast(expr.start()->annotation().type, expr.end())) {
                    expr.end() = implicit;
                }
            }
            else {
                expr.invalid(true);
                mismatch(expr.start()->range(), expr.end()->range(), diagnostic::format("Type mismatch between range bounds, found `$` and `$` sh*thead!", expr.start()->annotation().type->string(), expr.end()->annotation().type->string()), "", diagnostic::format("expected $", expr.start()->annotation().type->string()));
                throw semantic_error();
            }
        }
        else if (expr.start()) {
            base = expr.start()->annotation().type;
        }
        else if (expr.end()) {
            base = expr.end()->annotation().type;
        }

        expr.annotation().type = types::range(base, !expr.is_inclusive());
    }

    void checker::visit(const ast::ignore_pattern_expression& expr) 
    {
        expr.annotation().type = types::unknown();
    }

    void checker::visit(const ast::literal_pattern_expression& expr)
    {
        if (expr.invalid() || !expr.value().valid) throw semantic_error();

        switch (expr.value().kind()) {
            case token::kind::true_kw:
            case token::kind::false_kw:
                expr.annotation().type = types::boolean();
                break;
            case token::kind::integer_literal:
            {
                std::string value = expr.value().lexeme().string();
                size_t suffix = value.find('u');
                if (suffix == value.npos) suffix = value.find('i');
                if (suffix != value.npos) expr.annotation().type = types::builtin(value.substr(suffix));
                else expr.annotation().type = types::sint(32);
                break;
            }
            case token::kind::real_literal:
            {
                std::string value = expr.value().lexeme().string();
                size_t suffix = value.find('f');
                if (suffix != value.npos) expr.annotation().type = types::builtin(value.substr(suffix));
                else expr.annotation().type = types::floating(32);
                break;
            }
            case token::kind::imag_literal:
                expr.annotation().type = types::complex(64);
                break;
            case token::kind::char_literal:
                expr.annotation().type = types::character();
                break;
            case token::kind::string_literal:
                if (expr.value().lexeme().cdata()[expr.value().lexeme().size() - 1] == 's') expr.annotation().type = types::string();
                else expr.annotation().type = types::chars();
                break;
            default:
                expr.annotation().type = types::unknown();
        }
    }

    void checker::visit(const ast::path_pattern_expression& expr)
    {
        expr.annotation().type = types::unknown();
        
        expr.path()->annotation().ispattern = true;
        expr.path()->annotation().mustvalue = false;
        expr.path()->annotation().musttype = false;

        expr.path()->accept(*this);

        if (expr.path()->annotation().istype) {
            expr.invalid(true);
            error(expr.range(), diagnostic::format("Type `$` cannot be used as pattern. I was expecting binding or constant, idiot!", expr.path()->annotation().type->string()), "", "expected constant or binding");
        }
        else if (auto referenced = expr.path()->annotation().referencing) {
            if (referenced->kind() != ast::kind::const_declaration && referenced->kind() != ast::kind::generic_const_parameter_declaration) {
                auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(expr.range().begin())
                            .highlight(expr.range());

                if (auto var = dynamic_cast<const ast::var_declaration*>(referenced)) {
                    builder.message(diagnostic::format("Pattern must be a constant, which variable `$` is not, c*nt!", var->name().lexeme())).note(var->name().range(), diagnostic::format("Look at variable `$` declaration.", var->name().lexeme()));
                }
                else if (auto type = dynamic_cast<const ast::type_declaration*>(referenced)) {
                    builder.message(diagnostic::format("Pattern must be a constant, which type `$` is not, c*nt!", type->name().lexeme())).note(type->name().range(), diagnostic::format("Look at type `$` declaration.", type->name().lexeme()));
                }
                else if (auto property = dynamic_cast<const ast::property_declaration*>(referenced)) {
                    builder.message(diagnostic::format("Pattern must be a constant, which property `$` is not, c*nt!", property->name().lexeme())).note(property->name().range(), diagnostic::format("Look at property `$` declaration.", property->name().lexeme()));
                }
                else if (auto function = dynamic_cast<const ast::function_declaration*>(referenced)) {
                    builder.message(diagnostic::format("Pattern must be a constant, which function `$` is not, c*nt!", function->name().lexeme())).note(function->name().range(), diagnostic::format("Look at function `$` declaration.", function->name().lexeme()));
                }

                publisher().publish(builder.build());
                expr.invalid(true);
                throw semantic_error();
            }
            else {
                expr.annotation().type = expr.path()->annotation().type;
            }
        }

        expr.annotation() = expr.path()->annotation();
    }

    void checker::visit(const ast::tuple_pattern_expression& expr)
    {
        expr.annotation().type = types::unknown();

        ast::types types;

        for (std::size_t i = 0; i < expr.elements().size(); ++i) {
            expr.elements().at(i)->accept(*this);
            if (auto type = expr.elements().at(i)->annotation().type) types.push_back(type);
            if (expr.elements().at(i)->kind() == ast::kind::ignore_pattern_expression && i < expr.elements().size() - 1) {
                report(expr.elements().at(i)->range(), "Pattern `...` can only be used in last position as a component, idiot!");
                expr.invalid(true);
                break;
            }
        }

        expr.annotation().type = types::tuple(types);
    }

    void checker::visit(const ast::array_pattern_expression& expr)
    {
        ast::pointer<ast::type> base = types::unknown();

        if (!expr.elements().empty()) {
            expr.elements().front()->accept(*this);
            base = expr.elements().front()->annotation().type;

            for (size_t i = 1; i < expr.elements().size(); ++i) {
                expr.elements().at(i)->accept(*this);
                auto elemty = expr.elements().at(i)->annotation().type;

                if (expr.elements().at(i)->kind() == ast::kind::ignore_pattern_expression && i < expr.elements().size() - 1) {
                    report(expr.elements().at(i)->range(), "Pattern `...` can only be used in last position as an element, idiot!");
                    expr.invalid(true);
                    break;
                }
                
                // type checking is demanded to pattern matching analyzer
            }
        }

        expr.annotation().type = types::array(base, expr.elements().size());
    }

    void checker::visit(const ast::record_pattern_expression& expr) 
    {
        expr.annotation().type = types::unknown();
        
        expr.path()->annotation().mustvalue = false;
        expr.path()->annotation().musttype = true;

        expr.path()->accept(*this);

        if (!expr.path()->annotation().type || expr.path()->annotation().type->category() == ast::type::category::unknown_type) {
            expr.invalid(true);
            throw semantic_error();
        }
        
        switch (expr.path()->annotation().type->category()) {
        case ast::type::category::tuple_type:
        {
            auto tuple_type = std::static_pointer_cast<ast::tuple_type>(expr.path()->annotation().type);

            if (expr.fields().size() > tuple_type->components().size()) {
                expr.invalid(true);
                expr.annotation().type = types::unknown();
                error(expr.path()->range(), diagnostic::format("Tuple type `$` expects `$` components but you gave it `$`, pr*ck!", expr.path()->annotation().type->string(), tuple_type->components().size(), expr.fields().size()));
            }
            
            for (std::size_t i = 0; i < expr.fields().size(); ++i) {
                // type checking of fields is demanded to pattern matching analyzer
                expr.fields().at(i)->accept(*this);

                if (expr.fields().at(i)->kind() == ast::kind::ignore_pattern_expression && i < expr.fields().size() - 1) {
                    report(expr.fields().at(i)->range(), "Pattern `...` can only be used in last position as a component, idiot!");
                    expr.invalid(true);
                    break;
                }
            }
            
            if (!expr.invalid() && !expr.fields().empty() && expr.fields().size() < tuple_type->components().size() && expr.fields().back()->kind() != ast::kind::ignore_pattern_expression) {
                std::ostringstream oss;
                oss << "`" << expr.fields().size() << "`";
                for (auto i = expr.fields().size() + 1; i < tuple_type->components().size(); ++i) oss << ", `" << i << "`";
                error(expr.path()->range(), diagnostic::format("You forgot to initialize component $, idiot.", oss.str()));
            }
            else if (!expr.invalid() && expr.fields().empty() && !tuple_type->components().empty()) {
                std::ostringstream oss;
                oss << "`" << 0 << "`";
                for (std::size_t i = 1; i < tuple_type->components().size(); ++i) oss << ", `" << i << "`";
                error(expr.path()->range(), diagnostic::format("You forgot to initialize component $, idiot.", oss.str()));
            }

            expr.annotation().type = expr.path()->annotation().type;

            break;
        }
        case ast::type::category::structure_type:
        {
            auto structure_type = std::static_pointer_cast<ast::structure_type>(expr.path()->annotation().type);

            if (expr.fields().size() > structure_type->fields().size()) {
                expr.invalid(true);
                expr.annotation().type = types::unknown();
                error(expr.path()->range(), diagnostic::format("Structure type `$` expects `$` fields but you gave it `$`, pr*ck!", expr.path()->annotation().type->string(), structure_type->fields().size(), expr.fields().size()));
            }
            
            for (std::size_t i = 0; i < expr.fields().size(); ++i) {
                // type checking of fields is demanded to pattern matching analyzer
                expr.fields().at(i)->accept(*this);

                if (expr.fields().at(i)->kind() == ast::kind::ignore_pattern_expression && i < expr.fields().size() - 1) {
                    report(expr.fields().at(i)->range(), "Pattern `...` can only be used in last position as a field, idiot!");
                    expr.invalid(true);
                    break;
                }

                if (structure_type->declaration()) {
                    auto item = static_cast<const ast::record_declaration*>(structure_type->declaration())->fields().at(i);
                    std::string name = std::static_pointer_cast<ast::field_declaration>(item)->name().lexeme().string();
                    // checks if private
                    if ((item->is_hidden() || name[0] == '_') && !scope_->has_ancestor_scope(scopes_.at(structure_type->declaration()))) {
                        auto diag = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(expr.fields().at(i)->range().begin())
                                    .message(diagnostic::format("You can't initialize hidden field `$`, dammit!", name))
                                    .highlight(expr.fields().at(i)->range(), "inaccessible")
                                    .note(static_cast<const ast::field_declaration*>(item.get())->name().range(), diagnostic::format("Look at field `$` declaration, idiot.", name))
                                    .build();

                        publisher().publish(diag);
                        expr.invalid(true);
                    }
                }
            }
            
            if (!expr.invalid() && !expr.fields().empty() && expr.fields().size() < structure_type->fields().size() && expr.fields().back()->kind() != ast::kind::ignore_pattern_expression) {
                std::ostringstream oss;
                oss << "`" << structure_type->fields().at(expr.fields().size()).name << "`";
                for (auto i = expr.fields().size() + 1; i < structure_type->fields().size(); ++i) oss << ", `" << structure_type->fields().at(i).name << "`";
                error(expr.path()->range(), diagnostic::format("You forgot to initialize field $, idiot.", oss.str()));
            }
            else if (!expr.invalid() && expr.fields().empty() && !structure_type->fields().empty()) {
                std::ostringstream oss;
                oss << "`" << structure_type->fields().front().name << "`";
                for (std::size_t i = 1; i < structure_type->fields().size(); ++i) oss << ", `" << structure_type->fields().at(i).name << "`";
                error(expr.path()->range(), diagnostic::format("You forgot to initialize field $, idiot.", oss.str()));
            }
            
            expr.annotation().type = expr.path()->annotation().type;
            
            break;
        }
        default:
            error(expr.range(), diagnostic::format("Type `$` is not suitable for this pattern. I need a structure or tuple, idiot!", expr.path()->annotation().type->string()), "", "expected structure or tuple");
        }
    }

    void checker::visit(const ast::labeled_record_pattern_expression& expr) 
    {
        expr.annotation().type = types::unknown();
        
        expr.path()->annotation().mustvalue = false;
        expr.path()->annotation().musttype = true;

        expr.path()->accept(*this);

        if (!expr.path()->annotation().type || expr.path()->annotation().type->category() == ast::type::category::unknown_type) {
            expr.invalid(true);
            throw semantic_error();
        }

        if (auto structure_type = std::dynamic_pointer_cast<ast::structure_type>(expr.path()->annotation().type)) {
            if (expr.fields().size() > structure_type->fields().size()) {
                expr.invalid(true);
                expr.annotation().type = types::unknown();
                error(expr.path()->range(), diagnostic::format("Structure type `$` expects `$` fields but you gave it `$`, pr*ck!", expr.path()->annotation().type->string(), structure_type->fields().size(), expr.fields().size()));
            }

            std::unordered_map<std::string, token> names;
            
            for (std::size_t i = 0; i < expr.fields().size(); ++i) {
                std::string name = expr.fields().at(i).field.lexeme().string();
                auto found = std::find_if(structure_type->fields().begin(), structure_type->fields().end(), [&] (ast::structure_type::component field) { return field.name== name; });
                // looking for this field
                if (found == structure_type->fields().end()) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(expr.fields().at(i).field.location())
                                .message(diagnostic::format("There is no such field named `$` in structure `$`, sh*thead!", name, structure_type->string()))
                                .highlight(expr.fields().at(i).field.range(), "inexistent");

                    std::ostringstream explanation;
                    unsigned similars = 0;
                    
                    for (auto pair : structure_type->fields()) {
                        if (utils::levenshtein_distance(name, pair.name) < 2) {
                            explanation << " \\ • " << pair.name;
                            ++similars;
                        }
                    }

                    if (similars > 0) {
                        builder.explanation("Maybe you mean any of these fields:" + explanation.str());
                    }

                    if (auto typedecl = dynamic_cast<const ast::type_declaration*>(structure_type->declaration())) {
                        builder.note(typedecl->name().range(), diagnostic::format("Take a look at type `$` declaration.", structure_type->string()));
                    }
                        
                    publisher().publish(builder.build());
                    expr.invalid(true);
                }
                // test for duplicates field initializers
                else if (names.count(name)) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(expr.fields().at(i).field.location())
                                .message(diagnostic::format("Field `$` has already been initialized, idiot!", name))
                                .highlight(expr.fields().at(i).field.range())
                                .highlight(names.at(name).range(), diagnostic::highlighter::mode::light)
                                .build();

                    publisher().publish(diag);
                    expr.invalid(true);
                }
                else {
                    // checks if field access is private from current context
                    if (auto typedecl = dynamic_cast<const ast::record_declaration*>(structure_type->declaration())) {
                        auto item = std::find_if(typedecl->fields().begin(), typedecl->fields().end(), [&] (ast::pointer<ast::declaration> fdecl) { return name == std::static_pointer_cast<ast::field_declaration>(fdecl)->name().lexeme().string(); });
                        // checks if private
                        if ((item->get()->is_hidden() || name[0] == '_') && !scope_->has_ancestor_scope(scopes_.at(structure_type->declaration()))) {
                            auto diag = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(expr.fields().at(i).field.location())
                                        .message(diagnostic::format("You can't initialize hidden field `$`, dammit!", name))
                                        .highlight(expr.fields().at(i).field.range(), "inaccessible")
                                        .note(static_cast<const ast::field_declaration*>(item->get())->name().range(), diagnostic::format("Look at field `$` declaration, idiot.", name))
                                        .build();

                            publisher().publish(diag);
                            expr.invalid(true);
                        }
                    }

                    names.emplace(name, expr.fields().at(i).field);
                }
                // type checking of fields is demanded to pattern matching analyzer
                expr.fields().at(i).value->accept(*this);
            }
            
            expr.annotation().type = expr.path()->annotation().type;
        }
        else {
            error(expr.range(), diagnostic::format("Type `$` is not suitable for this pattern. I need a structure, idiot!", expr.path()->annotation().type->string()), "", "expected structure");
        }    
    }

    void checker::visit(const ast::range_pattern_expression& expr)
    {
        expr.annotation().type = types::unknown();
        if (expr.start()) {
            expr.start()->accept(*this);
            switch (expr.start()->annotation().type->category()) {
            case ast::type::category::integer_type:
            case ast::type::category::float_type:
            case ast::type::category::char_type:
                break;
            default:
                error(expr.start()->range(), diagnostic::format("Type `$` is not suitable for ranges. I would use integers, reals or characters, dammit!", expr.start()->annotation().type->string()), "", "expected integer, real or character");
            }
        }

        if (expr.end()) {
            expr.end()->accept(*this);

            switch (expr.end()->annotation().type->category()) {
            case ast::type::category::integer_type:
            case ast::type::category::float_type:
            case ast::type::category::char_type:
                break;
            default:
                error(expr.end()->range(), diagnostic::format("Type `$` is not suitable for ranges. I would use integers, reals or characters, dammit!", expr.end()->annotation().type->string()), "", "expected integer, real or character");
            }
        }

        ast::pointer<ast::type> base = types::sint();

        if (expr.start() && expr.end()) {
            if (types::assignment_compatible(expr.start()->annotation().type, expr.end()->annotation().type)) {
                base = expr.start()->annotation().type;
                if (auto implicit = implicit_cast(expr.start()->annotation().type, expr.end())) {
                    expr.end() = implicit;
                }
            }
            else {
                expr.invalid(true);
                mismatch(expr.start()->range(), expr.end()->range(), diagnostic::format("Type mismatch between range bounds, found `$` and `$` sh*thead!", expr.start()->annotation().type->string(), expr.end()->annotation().type->string()), "", diagnostic::format("expected $", expr.start()->annotation().type->string()));
                throw semantic_error();
            }
        }
        else if (expr.start()) {
            base = expr.start()->annotation().type;
        }
        else if (expr.end()) {
            base = expr.end()->annotation().type;
        }

        // range type must be the same of the bounds for pattern matching
        expr.annotation().type = base;
    }

    void checker::visit(const ast::or_pattern_expression& expr)
    {
        expr.annotation().type = types::unknown();
        expr.left()->accept(*this);
        expr.right()->accept(*this);
        // type checking is demanded to pattern analyzer
        expr.annotation().type = expr.left()->annotation().type;
    }

    void checker::visit(const ast::when_expression& expr)
    {
        ast::pointer<ast::expression> exprnode = nullptr;

        expr.annotation().type = types::unknown();
        expr.condition()->accept(*this);

        if (!expr.condition()->annotation().type || expr.condition()->annotation().type->category() == ast::type::category::unknown_type) {
            expr.invalid(true);
            throw semantic_error();
        }

        for (auto& branch : expr.branches()) {
            branch.pattern()->accept(*this);

            if (branch.pattern()->invalid()) continue;

            ast::pattern_matcher pattern_matcher(*branch.pattern(), publisher(), *this);
            auto result = pattern_matcher.match(*expr.condition());

            if (!result) continue;

            begin_scope(branch.pattern().get());
            // all bindings are declared inside block with scope
            for (auto decl : result.declarations) {
                decl->annotation().visited = true;
                decl->annotation().resolved = true;
                decl->annotation().scope = branch.pattern().get();
                scope_->value(std::static_pointer_cast<ast::var_declaration>(decl)->name().lexeme().string(), decl.get());
            }
            // declarations manually inserted as statements
            if (auto block = std::dynamic_pointer_cast<ast::block_expression>(branch.body())) {
                block->statements().insert(block->statements().begin(), result.declarations.begin(), result.declarations.end());
            }
            else {
                ast::pointers<ast::statement> stmts {};
                stmts.insert(stmts.begin(), result.declarations.begin(), result.declarations.end());
                stmts.push_back(ast::create<ast::expression_statement>(branch.body()->range(), branch.body()));
                auto newbody = ast::create<ast::block_expression>(branch.body()->range(), stmts);
                newbody->exprnode() = branch.body().get();
                newbody->annotation().type = branch.body()->annotation().type;
                newbody->annotation().value = branch.body()->annotation().value;
                branch.body() = newbody;
            }
            // compiled pattern condition is associated to pattern
            if (result.condition) {
                std::dynamic_pointer_cast<ast::pattern_expression>(branch.pattern())->compiled() = result.condition;
            }

            branch.body()->accept(*this);
            end_scope();

            if (branch.body()->annotation().type && branch.body()->annotation().type->category() != ast::type::category::unknown_type) {
                if (!exprnode) {
                    exprnode = branch.body();
                }
                else if (!types::assignment_compatible(exprnode->annotation().type, branch.body()->annotation().type)) {
                    auto diag = diagnostic::builder()
                                .location(branch.body()->range().begin())
                                .small(true)
                                .severity(diagnostic::severity::error)
                                .message(diagnostic::format("Body should have type `$` but I found type `$` instead, dammit!", exprnode->annotation().type->string(), branch.body()->annotation().type->string()))
                                .highlight(branch.body()->range())
                                .note(exprnode->range(), diagnostic::format("You idiot make me think that result type should be `$` from here.", exprnode->annotation().type->string()))
                                .build();
                    
                    publisher().publish(diag);
                    expr.invalid(true);
                }
                else if (auto implicit = implicit_cast(exprnode->annotation().type, branch.body())) {
                    branch.body() = implicit;
                }
            }
        }

        if (expr.else_body()) {
            expr.else_body()->accept(*this);
            if (exprnode && exprnode->annotation().type && exprnode->annotation().type->category() != ast::type::category::unknown_type &&
                expr.else_body()->annotation().type && expr.else_body()->annotation().type->category() != ast::type::category::unknown_type &&
                !types::compatible(exprnode->annotation().type, expr.else_body()->annotation().type)) {
                auto else_block = std::dynamic_pointer_cast<ast::block_expression>(expr.else_body());
                source_range range1 = exprnode->range(), range2;

                if (else_block) {
                    if (else_block->exprnode()) range2 = else_block->exprnode()->range();
                    else range2 = else_block->range();
                }
                else range2 = expr.else_body()->range();

                auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(range2.begin())
                            .message(diagnostic::format("When body has type `$` but else body has type `$`, dammit!", exprnode->annotation().type->string(), expr.else_body()->annotation().type->string()))
                            .note(range1, diagnostic::format("As you can see when body has type `$`.", exprnode->annotation().type->string()))
                            .highlight(range2);

                publisher().publish(builder.build());
                expr.invalid(true);
            }
        }

        if (exprnode) expr.annotation().type = exprnode->annotation().type;
    }

    void checker::visit(const ast::when_pattern_expression& expr)
    {
        expr.annotation().type = types::unknown();
        expr.condition()->accept(*this);
        expr.pattern()->accept(*this);

        if (!expr.condition()->annotation().type || expr.condition()->annotation().type->category() == ast::type::category::unknown_type) {
            expr.invalid(true);
            throw semantic_error();
        }

        ast::pattern_matcher pattern_matcher(*expr.pattern(), publisher(), *this);
        auto result = pattern_matcher.match(*expr.condition());

        begin_scope(&expr);

        if (result) {
            // all bindings are declared inside block with scope
            for (auto decl : result.declarations) {
                decl->annotation().visited = true;
                decl->annotation().resolved = true;
                decl->annotation().scope = &expr;
                scope_->value(std::static_pointer_cast<ast::var_declaration>(decl)->name().lexeme().string(), decl.get());
            }
            // declarations manually inserted as statements
            if (auto block = std::dynamic_pointer_cast<ast::block_expression>(expr.body())) {
                block->statements().insert(block->statements().begin(), result.declarations.begin(), result.declarations.end());
            }
            // compiled pattern condition is associated to pattern
            if (result.condition) {
                std::dynamic_pointer_cast<ast::pattern_expression>(expr.pattern())->compiled() = result.condition;
            }
        }

        expr.body()->accept(*this);

        end_scope();

        if (expr.else_body()) {
            expr.else_body()->accept(*this);
            if (expr.body()->annotation().type && expr.body()->annotation().type->category() != ast::type::category::unknown_type &&
                expr.else_body()->annotation().type && expr.else_body()->annotation().type->category() != ast::type::category::unknown_type &&
                !types::compatible(expr.body()->annotation().type, expr.else_body()->annotation().type)) {
                auto then_block = std::dynamic_pointer_cast<ast::block_expression>(expr.body());
                auto else_block = std::dynamic_pointer_cast<ast::block_expression>(expr.else_body());
                source_range range1, range2;

                if (then_block) {
                    if (then_block->exprnode()) range1 = then_block->exprnode()->range();
                    else range1 = then_block->range();
                }
                else range1 = expr.body()->range();

                if (else_block) {
                    if (else_block->exprnode()) range2 = else_block->exprnode()->range();
                    else range2 = else_block->range();
                }
                else range2 = expr.else_body()->range();

                auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(range2.begin())
                            .message(diagnostic::format("When body has type `$` but else body has type `$`, dammit!", expr.body()->annotation().type->string(), expr.else_body()->annotation().type->string()))
                            .note(range1, diagnostic::format("As you can see when body has type `$`.", expr.body()->annotation().type->string()))
                            .highlight(range2);

                publisher().publish(builder.build());
                expr.invalid(true);
            }
        }

        if (expr.body()->annotation().type) expr.annotation().type = expr.body()->annotation().type;
    }

    void checker::visit(const ast::when_cast_expression& expr) 
    {
        expr.condition()->accept(*this);
        expr.type_expression()->accept(*this);

        if (expr.condition()->annotation().type->category() == ast::type::category::variant_type) {
            auto variant = std::static_pointer_cast<ast::variant_type>(expr.condition()->annotation().type);
            if (!variant->contains(expr.type_expression()->annotation().type)) {
                expr.invalid(true);
                report(expr.type_expression()->range(), diagnostic::format("Type `$` is not included among variant `$` subtypes, you c*nt!", expr.type_expression()->annotation().type->string(), variant->string()));
            }
        }
        else if (expr.condition()->annotation().type->category() == ast::type::category::unknown_type) {
            expr.invalid(true);
        }
        else {
            auto builder = diagnostic::builder()
                        .severity(diagnostic::severity::error)
                        .location(expr.condition()->range().begin())
                        .message(diagnostic::format("I need a variant type which contains at least `$`, I found `$` instead, idiot!", expr.type_expression()->annotation().type->string(), expr.condition()->annotation().type->string()))
                        .note(expr.type_expression()->range(), diagnostic::format("Type `$` is what you are testing against to perform an automatic cast inside when body!", expr.type_expression()->annotation().type->string()))
                        .highlight(expr.condition()->range());

            publisher().publish(builder.build());
            expr.invalid(true);
        }

        auto outer = scope_;

        begin_scope(&expr);

        // put converted variable inside scope if condition expression is a variable and converted type
        // is not a variant itself
        if (auto var_ref = std::dynamic_pointer_cast<ast::identifier_expression>(expr.condition())) {
            auto decl = outer->value(var_ref->identifier().lexeme().string());
            if (!var_ref->is_generic() && decl && expr.body()) {
                auto body = std::static_pointer_cast<ast::block_expression>(expr.body());
                // first of all creates a temporary variable whose value is converted variant
                // like 'val __tempx = original as T'
                auto tempname = "__temp" + std::to_string(std::rand());
                auto temp = ast::create<ast::var_declaration>(decl->range(), std::vector<token>(), token(), nullptr, nullptr);
                temp->name() = token::builder().artificial(true).location(temp->name().location()).kind(token::kind::identifier).lexeme(utf8::span::builder().concat(tempname.data(), tempname.size()).build()).build();
                temp->type_expression() = nullptr;
                temp->annotation().type = expr.type_expression()->annotation().type;
                temp->annotation().scope = &expr;
                temp->annotation().visited = temp->annotation().resolved = true;
                temp->value() = implicit_forced_cast(expr.type_expression()->annotation().type, var_ref);
                temp->set_mutable(dynamic_cast<const ast::var_declaration*>(decl)->is_mutable());
                // this is reference to temporary variable, which is '__tempx'
                auto tempref = ast::create<ast::identifier_expression>(var_ref->range(), temp->name(), ast::pointers<ast::expression>());
                tempref->annotation().referencing = temp.get();
                tempref->annotation().type = expr.type_expression()->annotation().type;
                // then redeclares original variable with unpacked value, which is temporary variable's value
                // like 'val original = __tempx'
                // this is done since we can't intialize original with its homonymous from external scope
                // this would cause ambiguity
                auto converted = ast::create<ast::var_declaration>(decl->range(), std::vector<token>(), token(), nullptr, nullptr);
                converted->name() = var_ref->identifier();
                converted->type_expression() = nullptr;
                converted->annotation().type = expr.type_expression()->annotation().type;
                converted->annotation().scope = &expr;
                converted->annotation().visited = converted->annotation().resolved = true;
                converted->value() = tempref;
                converted->set_mutable(dynamic_cast<const ast::var_declaration*>(decl)->is_mutable());
                // then adds var to block and then to scope
                body->statements().insert(body->statements().begin(), converted);
                body->statements().insert(body->statements().begin(), temp);
                scope_->value(var_ref->identifier().lexeme().string(), converted.get());
            }
        }

        if (expr.body()) expr.body()->accept(*this);

        if (expr.else_body()) {
            expr.else_body()->accept(*this);
            if (expr.body()->annotation().type && expr.body()->annotation().type->category() != ast::type::category::unknown_type &&
                expr.else_body()->annotation().type && expr.else_body()->annotation().type->category() != ast::type::category::unknown_type &&
                !types::compatible(expr.body()->annotation().type, expr.else_body()->annotation().type)) {
                auto then_block = std::dynamic_pointer_cast<ast::block_expression>(expr.body());
                auto else_block = std::dynamic_pointer_cast<ast::block_expression>(expr.else_body());
                source_range range1, range2;

                if (then_block) {
                    if (then_block->exprnode()) range1 = then_block->exprnode()->range();
                    else range1 = then_block->range();
                }
                else range1 = expr.body()->range();

                if (else_block) {
                    if (else_block->exprnode()) range2 = else_block->exprnode()->range();
                    else range2 = else_block->range();
                }
                else range2 = expr.else_body()->range();

                auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(range2.begin())
                            .message(diagnostic::format("When body has type `$` but else body has type `$`, dammit!", expr.body()->annotation().type->string(), expr.else_body()->annotation().type->string()))
                            .note(range1, diagnostic::format("As you can see when body has type `$`.", expr.body()->annotation().type->string()))
                            .highlight(range2);

                publisher().publish(builder.build());
                expr.invalid(true);
            }
        }

        end_scope();

        scope_ = outer;

        expr.annotation().type = expr.body()->annotation().type;
    }

    void checker::visit(const ast::for_range_expression& expr) 
    {
        auto outer = scope_;
        // required to contain break/continue statements
        begin_scope(&expr);
        // iteration variable
        expr.variable()->accept(*this);
        expr.condition()->accept(*this);
        // explicit type annotation for iteration variable
        if (auto array_type = std::dynamic_pointer_cast<ast::array_type>(expr.condition()->annotation().type)) expr.variable()->annotation().type = array_type->base();
        else if (auto slice_type = std::dynamic_pointer_cast<ast::slice_type>(expr.condition()->annotation().type)) expr.variable()->annotation().type = slice_type->base();
        else if (auto range_type = std::dynamic_pointer_cast<ast::range_type>(expr.condition()->annotation().type)) expr.variable()->annotation().type = range_type->base();
        else {
            expr.invalid(true);
            report(expr.condition()->range(), diagnostic::format("I need an array, slice or range to iterate through, I found `$` instead, idiot!", expr.condition()->annotation().type->string()));
        }

        if (expr.body()) {
            expr.body()->accept(*this);
            auto body = std::static_pointer_cast<ast::block_expression>(expr.body());
            // type and expr node of a loop must be only from break instructions
            if (dynamic_cast<const ast::expression_statement*>(body->exprnode())) {
                body->exprnode() = body.get();
                body->annotation().type = types::unit();
            }
        }

        end_scope();
        scope_ = outer;

        if (expr.body()->annotation().type && expr.else_body()) {
            expr.else_body()->accept(*this);
            if (expr.body()->annotation().type && expr.body()->annotation().type->category() != ast::type::category::unknown_type &&
                expr.else_body()->annotation().type && expr.else_body()->annotation().type->category() != ast::type::category::unknown_type &&
                !types::compatible(expr.body()->annotation().type, expr.else_body()->annotation().type)) {
                auto block = std::dynamic_pointer_cast<ast::block_expression>(expr.body());
                auto else_block = std::dynamic_pointer_cast<ast::block_expression>(expr.else_body());
                source_range range1, range2;

                if (block) {
                    if (block->exprnode()) range1 = block->exprnode()->range();
                    else range1 = block->range();
                }
                else range1 = expr.body()->range();

                if (else_block) {
                    if (else_block->exprnode()) range2 = else_block->exprnode()->range();
                    else range2 = else_block->range();
                }
                else range2 = expr.else_body()->range();

                auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(range2.begin())
                            .message(diagnostic::format("For body has type `$` but else body has type `$`, dammit!", expr.body()->annotation().type->string(), expr.else_body()->annotation().type->string()))
                            .note(range1, diagnostic::format("As you can see if body has type `$`.", expr.body()->annotation().type->string()))
                            .highlight(range2)
                            .explanation("For body takes its type from any `break` statement it contains, otherwise it defaults to type `()`.");

                publisher().publish(builder.build());
                expr.invalid(true);
            }
        }

        expr.annotation().type = expr.body()->annotation().type;
    }

    void checker::visit(const ast::for_loop_expression& expr) 
    {
        auto outer = scope_;

        if (expr.condition()) {
            expr.condition()->accept(*this);

            if (expr.condition()->annotation().type->category() != ast::type::category::bool_type) {
                expr.invalid(true);
                report(expr.condition()->range(), diagnostic::format("You know that any condition must have `bool` type, I found `$` instead, idiot!", expr.condition()->annotation().type->string()));
            }
        }

        // required to contain break/continue statements
        begin_scope(&expr);

        if (expr.body()) {
            expr.body()->accept(*this);
            auto body = std::static_pointer_cast<ast::block_expression>(expr.body());
            // type and expr node of a loop must be only from break instructions
            if (dynamic_cast<const ast::expression_statement*>(body->exprnode())) {
                body->exprnode() = body.get();
                body->annotation().type = types::unit();
            }
        }

        end_scope();
        scope_ = outer;

        if (expr.body()->annotation().type && expr.else_body()) {
            expr.else_body()->accept(*this);
            if (expr.body()->annotation().type && expr.body()->annotation().type->category() != ast::type::category::unknown_type &&
                expr.else_body()->annotation().type && expr.else_body()->annotation().type->category() != ast::type::category::unknown_type &&
                !types::compatible(expr.body()->annotation().type, expr.else_body()->annotation().type)) {
                auto block = std::dynamic_pointer_cast<ast::block_expression>(expr.body());
                auto else_block = std::dynamic_pointer_cast<ast::block_expression>(expr.else_body());
                source_range range1, range2;

                if (block) {
                    if (block->exprnode()) range1 = block->exprnode()->range();
                    else range1 = block->range();
                }
                else range1 = expr.body()->range();

                if (else_block) {
                    if (else_block->exprnode()) range2 = else_block->exprnode()->range();
                    else range2 = else_block->range();
                }
                else range2 = expr.else_body()->range();

                auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(range2.begin())
                            .message(diagnostic::format("For body has type `$` but else body has type `$`, dammit!", expr.body()->annotation().type->string(), expr.else_body()->annotation().type->string()))
                            .note(range1, diagnostic::format("As you can see if body has type `$`.", expr.body()->annotation().type->string()))
                            .highlight(range2)
                            .explanation("For body takes its type from any `break` statement it contains, otherwise it defaults to type `()`.");

                publisher().publish(builder.build());
                expr.invalid(true);
            }
        }

        expr.annotation().type = expr.body()->annotation().type;
    }

    void checker::visit(const ast::if_expression& expr) 
    {
        expr.condition()->accept(*this);

        if (expr.condition()->annotation().type->category() != ast::type::category::bool_type) {
            expr.invalid(true);
            report(expr.condition()->range(), diagnostic::format("You know that any condition must have `bool` type, I found `$` instead, idiot!", expr.condition()->annotation().type->string()));
        }

        if (expr.body()) expr.body()->accept(*this);

        if (expr.else_body()) {
            expr.else_body()->accept(*this);
            if (expr.body()->annotation().type && expr.body()->annotation().type->category() != ast::type::category::unknown_type &&
                expr.else_body()->annotation().type && expr.else_body()->annotation().type->category() != ast::type::category::unknown_type &&
                !types::compatible(expr.body()->annotation().type, expr.else_body()->annotation().type)) {
                auto then_block = std::dynamic_pointer_cast<ast::block_expression>(expr.body());
                auto else_block = std::dynamic_pointer_cast<ast::block_expression>(expr.else_body());
                source_range range1, range2;

                if (then_block) {
                    if (then_block->exprnode()) range1 = then_block->exprnode()->range();
                    else range1 = then_block->range();
                }
                else range1 = expr.body()->range();

                if (else_block) {
                    if (else_block->exprnode()) range2 = else_block->exprnode()->range();
                    else range2 = else_block->range();
                }
                else range2 = expr.else_body()->range();

                auto builder = diagnostic::builder()
                            .severity(diagnostic::severity::error)
                            .location(range2.begin())
                            .message(diagnostic::format("If body has type `$` but else body has type `$`, dammit!", expr.body()->annotation().type->string(), expr.else_body()->annotation().type->string()))
                            .note(range1, diagnostic::format("As you can see if body has type `$`.", expr.body()->annotation().type->string()))
                            .highlight(range2);

                publisher().publish(builder.build());
                expr.invalid(true);
            }
        }

        expr.annotation().type = expr.body()->annotation().type;
    }

    void checker::visit(const ast::null_statement& stmt) 
    {
        stmt.annotation().scope = scope_->enclosing();
        stmt.annotation().visited = true;
        stmt.annotation().resolved = true;
    }

    void checker::visit(const ast::expression_statement& stmt) 
    {
        stmt.annotation().scope = scope_->enclosing();
        stmt.annotation().visited = true;
        stmt.expression()->accept(*this);
        stmt.annotation().resolved = true;
    }

    void checker::visit(const ast::assignment_statement& stmt)
    {
        stmt.annotation().scope = scope_->enclosing();
        stmt.annotation().visited = true;

        stmt.left()->accept(*this);
        stmt.right()->accept(*this);

        if (!stmt.left()->annotation().type || stmt.left()->annotation().type->category() == ast::type::category::unknown_type || !stmt.right()->annotation().type || stmt.right()->annotation().type->category() == ast::type::category::unknown_type) {
            stmt.invalid(true);
            throw semantic_error();
        }

        auto lefttype = stmt.left()->annotation().type, righttype = stmt.right()->annotation().type;

        switch (stmt.assignment_operator().kind()) {
            case token::kind::equal:
                if (!types::assignment_compatible(stmt.left()->annotation().type, stmt.right()->annotation().type)) {
                    auto diag = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(stmt.assignment_operator().location())
                                .message(diagnostic::format("You cannot assign `$` to `$`, idiot!", stmt.right()->annotation().type->string(), stmt.left()->annotation().type->string()))
                                .highlight(stmt.assignment_operator().range())
                                .highlight(stmt.left()->range(), diagnostic::highlighter::mode::light)
                                .highlight(stmt.right()->range(), diagnostic::format("expected $", stmt.left()->annotation().type->string()), diagnostic::highlighter::mode::light)
                                .build();
                    
                    publisher().publish(diag);
                    stmt.invalid(true);
                }
                else if (auto implicit = implicit_cast(stmt.left()->annotation().type, stmt.right())) {
                    stmt.right() = implicit;
                }

                break;
            case token::kind::plus_equal:
                if (lefttype->category() == ast::type::category::integer_type) {
                    if (righttype->category() == ast::type::category::integer_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot add and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::rational_type) {
                    if (righttype->category() == ast::type::category::integer_type ||
                        righttype->category() == ast::type::category::rational_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot add and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::float_type) {
                    if (righttype->category() == ast::type::category::integer_type ||
                        righttype->category() == ast::type::category::rational_type ||
                        righttype->category() == ast::type::category::float_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot add and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::complex_type) {
                    if (righttype->category() == ast::type::category::integer_type ||
                        righttype->category() == ast::type::category::rational_type ||
                        righttype->category() == ast::type::category::float_type ||
                        righttype->category() == ast::type::category::complex_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot add and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::chars_type) {
                    if (righttype->category() == ast::type::category::char_type || 
                        righttype->category() == ast::type::category::chars_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot add and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::string_type) {
                    if (righttype->category() == ast::type::category::char_type ||
                        righttype->category() == ast::type::category::chars_type || 
                        righttype->category() == ast::type::category::string_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot add and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::pointer_type) {
                    if (righttype->category() == ast::type::category::integer_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot add and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else {
                    stmt.invalid(true);
                    error(stmt, diagnostic::format("You cannot add and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                }

                break;
            case token::kind::minus_equal:
                if (lefttype->category() == ast::type::category::integer_type) {
                    if (righttype->category() == ast::type::category::integer_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot subtract and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::rational_type) {
                    if (righttype->category() == ast::type::category::integer_type ||
                        righttype->category() == ast::type::category::rational_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot subtract and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::float_type) {
                    if (righttype->category() == ast::type::category::integer_type ||
                        righttype->category() == ast::type::category::rational_type ||
                        righttype->category() == ast::type::category::float_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot subtract and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::complex_type) {
                    if (righttype->category() == ast::type::category::integer_type ||
                        righttype->category() == ast::type::category::rational_type ||
                        righttype->category() == ast::type::category::float_type ||
                        righttype->category() == ast::type::category::complex_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot subtract and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::pointer_type) {
                    if (righttype->category() == ast::type::category::integer_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot subtract and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else {
                    stmt.invalid(true);
                    error(stmt, diagnostic::format("You cannot subtract and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                }

                break;
            case token::kind::star_equal:
                if (lefttype->category() == ast::type::category::integer_type) {
                    if (righttype->category() == ast::type::category::integer_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot multiply and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::rational_type) {
                    if (righttype->category() == ast::type::category::integer_type ||
                        righttype->category() == ast::type::category::rational_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot multiply and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::float_type) {
                    if (righttype->category() == ast::type::category::integer_type ||
                        righttype->category() == ast::type::category::rational_type ||
                        righttype->category() == ast::type::category::float_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot multiply and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::complex_type) {
                    if (righttype->category() == ast::type::category::integer_type ||
                        righttype->category() == ast::type::category::rational_type ||
                        righttype->category() == ast::type::category::float_type ||
                        righttype->category() == ast::type::category::complex_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot multiply and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::pointer_type) {
                    if (righttype->category() == ast::type::category::integer_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot multiply and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else {
                    stmt.invalid(true);
                    error(stmt, diagnostic::format("You cannot multiply and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                }

                break;
            case token::kind::slash_equal:
                if (lefttype->category() == ast::type::category::integer_type) {
                    if (righttype->category() == ast::type::category::integer_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot divide and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::rational_type) {
                    if (righttype->category() == ast::type::category::integer_type ||
                        righttype->category() == ast::type::category::rational_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot divide and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::float_type) {
                    if (righttype->category() == ast::type::category::integer_type ||
                        righttype->category() == ast::type::category::rational_type ||
                        righttype->category() == ast::type::category::float_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot divide and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::complex_type) {
                    if (righttype->category() == ast::type::category::integer_type ||
                        righttype->category() == ast::type::category::rational_type ||
                        righttype->category() == ast::type::category::float_type ||
                        righttype->category() == ast::type::category::complex_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot divide and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::pointer_type) {
                    if (righttype->category() == ast::type::category::integer_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot divide and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else {
                    stmt.invalid(true);
                    error(stmt, diagnostic::format("You cannot divide and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                }

                break;
            case token::kind::percent_equal:
                if (lefttype->category() != ast::type::category::integer_type || righttype->category() != ast::type::category::integer_type) {
                    stmt.invalid(true);
                    error(stmt, diagnostic::format("You cannot mod and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                }

                break;
            case token::kind::star_star_equal:
                if (lefttype->category() == ast::type::category::integer_type) {
                    if (righttype->category() == ast::type::category::integer_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot raise and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::rational_type) {
                    stmt.invalid(true);
                    error(stmt, diagnostic::format("You cannot raise and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                }
                else if (lefttype->category() == ast::type::category::float_type) {
                    if (righttype->category() == ast::type::category::integer_type ||
                        righttype->category() == ast::type::category::rational_type ||
                        righttype->category() == ast::type::category::float_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot raise and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else if (lefttype->category() == ast::type::category::complex_type) {
                    if (righttype->category() == ast::type::category::integer_type ||
                        righttype->category() == ast::type::category::rational_type ||
                        righttype->category() == ast::type::category::float_type ||
                        righttype->category() == ast::type::category::complex_type);
                    else {
                        stmt.invalid(true);
                        error(stmt, diagnostic::format("You cannot raise and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                    }
                }
                else {
                    stmt.invalid(true);
                    error(stmt, diagnostic::format("You cannot raise and assign `$` to `$`, b*tch!", righttype->string(), lefttype->string()));
                }
                
                break;
            case token::kind::amp_equal:
                if (lefttype->category() == ast::type::category::integer_type && righttype->category() == ast::type::category::integer_type);
                else if (lefttype->category() == ast::type::category::bool_type && righttype->category() == ast::type::category::bool_type && stmt.left()->annotation().isconcept && stmt.right()->annotation().isconcept);
                else {
                    stmt.invalid(true);
                    error(stmt, diagnostic::format("Bitwise assignment `&=` requires integer types, I found `$` and `$` instead!", lefttype->string(), righttype->string()));
                }
                break;
            case token::kind::line_equal:
                if (lefttype->category() == ast::type::category::integer_type && righttype->category() == ast::type::category::integer_type);
                else if (lefttype->category() == ast::type::category::bool_type && righttype->category() == ast::type::category::bool_type && stmt.left()->annotation().isconcept && stmt.right()->annotation().isconcept);
                else {
                    stmt.invalid(true);
                    error(stmt, diagnostic::format("Bitwise assignment `|=` requires integer types, I found `$` and `$` instead!", lefttype->string(), righttype->string()));
                }
                break;
            case token::kind::caret_equal:
                if (lefttype->category() == ast::type::category::integer_type && righttype->category() == ast::type::category::integer_type);
                else {
                    stmt.invalid(true);
                    error(stmt, diagnostic::format("Bitwise assignment `^=` requires integer types, I found `$` and `$` instead!", lefttype->string(), righttype->string()));
                }
                break;
            default:
                stmt.invalid(true);
                throw semantic_error();
        }

        immutability(stmt);

        stmt.annotation().resolved = true;
    }

    void checker::visit(const ast::return_statement& stmt)
    {
        stmt.annotation().scope = scope_->enclosing();
        stmt.annotation().visited = true;

        if (stmt.expression()) stmt.expression()->accept(*this);

        if (auto fn = scope_->outscope(environment::kind::function)) {
            ast::pointer<ast::type> expected = nullptr;
            
            if (auto fndecl = dynamic_cast<const ast::function_declaration*>(fn)) {
                if (fndecl->return_type_expression()) expected = fndecl->return_type_expression()->annotation().type;
                else expected = types::unit();

                if (!stmt.expression()) {
                    if (!types::assignment_compatible(expected, types::unit())) {
                        auto builder = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(stmt.range().begin())
                                        .message(diagnostic::format("Function `$` expects return type `$` but I found type `()` instead, idiot!", fndecl->name().lexeme(), expected->string()))
                                        .highlight(stmt.range());

                        if (fndecl->return_type_expression()) builder.note(fndecl->return_type_expression()->range(), diagnostic::format("You can see that return type of function `$` is `$`.", fndecl->name().lexeme(), expected->string()));

                        publisher().publish(builder.build());
                    }
                }
                else if (expected->category() != ast::type::category::unknown_type &&
                    stmt.expression()->annotation().type->category() != ast::type::category::unknown_type &&
                    !types::assignment_compatible(expected, stmt.expression()->annotation().type)) {
                    auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(stmt.expression()->range().begin())
                                    .message(diagnostic::format("Function `$` expects return type `$` but I found type `$` instead, idiot!", fndecl->name().lexeme(), expected->string(), stmt.expression()->annotation().type->string()))
                                    .highlight(stmt.expression()->range());

                    if (fndecl->return_type_expression()) builder.note(fndecl->return_type_expression()->range(), diagnostic::format("You can see that return type of function `$` is `$`.", fndecl->name().lexeme(), expected->string()));

                    publisher().publish(builder.build());
                }

                if (auto body = std::static_pointer_cast<ast::block_expression>(fndecl->body())) body->exprnode() = &stmt;
            }
            else if (auto fndecl = dynamic_cast<const ast::property_declaration*>(fn)) {
                if (fndecl->return_type_expression()) expected = fndecl->return_type_expression()->annotation().type;
                else expected = types::unit();

                if (!stmt.expression()) {
                    if (!types::assignment_compatible(expected, types::unit())) {
                        auto builder = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(stmt.range().begin())
                                        .message(diagnostic::format("Property `$` expects return type `$` but I found type `()` instead, idiot!", fndecl->name().lexeme(), expected->string()))
                                        .highlight(stmt.range());

                        if (fndecl->return_type_expression()) builder.note(fndecl->return_type_expression()->range(), diagnostic::format("You can see that return type of property `$` is `$`.", fndecl->name().lexeme(), expected->string()));

                        publisher().publish(builder.build());
                    }
                }
                else if (expected->category() != ast::type::category::unknown_type &&
                    stmt.expression()->annotation().type->category() != ast::type::category::unknown_type &&
                    !types::assignment_compatible(expected, stmt.expression()->annotation().type)) {
                    auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(stmt.expression()->range().begin())
                                    .message(diagnostic::format("Property `$` expects return type `$` but I found type `$` instead, idiot!", fndecl->name().lexeme(), expected->string(), stmt.expression()->annotation().type->string()))
                                    .highlight(stmt.expression()->range());

                    if (fndecl->return_type_expression()) builder.note(fndecl->return_type_expression()->range(), diagnostic::format("You can see that return type of property `$` is `$`.", fndecl->name().lexeme(), expected->string()));

                    publisher().publish(builder.build());
                }

                if (auto body = std::static_pointer_cast<ast::block_expression>(fndecl->body())) body->exprnode() = &stmt;
            }
            else if (auto fnexpr = dynamic_cast<const ast::function_expression*>(fn)) {
                if (fnexpr->return_type_expression()) expected = fnexpr->return_type_expression()->annotation().type;
                else expected = types::unit();

                if (!stmt.expression()) {
                    if (!types::assignment_compatible(expected, types::unit())) {
                        auto builder = diagnostic::builder()
                                        .severity(diagnostic::severity::error)
                                        .location(stmt.range().begin())
                                        .message(diagnostic::format("I was expecting return type `$` but I found type `()` instead, idiot!", expected->string()))
                                        .highlight(stmt.range());

                        if (fnexpr->return_type_expression()) builder.note(fnexpr->return_type_expression()->range(), diagnostic::format("You can see that return type of function expression is `$`.", expected->string()));

                        publisher().publish(builder.build());
                    }
                }
                else if (expected->category() != ast::type::category::unknown_type &&
                    stmt.expression()->annotation().type->category() != ast::type::category::unknown_type &&
                    !types::assignment_compatible(expected, stmt.expression()->annotation().type)) {
                    auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(stmt.expression()->range().begin())
                                    .message(diagnostic::format("I was expecting return type `$` but I found type `$` instead, idiot!", expected->string(), stmt.expression()->annotation().type->string()))
                                    .highlight(stmt.expression()->range(), diagnostic::format("expected $", expected->string()));

                    if (fnexpr->return_type_expression()) builder.note(fnexpr->return_type_expression()->range(), diagnostic::format("You can see that return type of function expression is `$`.", expected->string()));

                    publisher().publish(builder.build());
                }
                else if (auto implicit = implicit_cast(expected, stmt.expression())) {
                    stmt.expression() = implicit;
                }

                if (auto body = std::static_pointer_cast<ast::block_expression>(fnexpr->body())) body->exprnode() = &stmt;
            }
        }
        else report(stmt.range(), "You cannot use a `return` statement outside function, dammit!");

        stmt.annotation().resolved = true;
    }

    void checker::visit(const ast::break_statement& stmt)
    {
        stmt.annotation().scope = scope_->enclosing();
        stmt.annotation().visited = true;

        if (stmt.expression()) stmt.expression()->accept(*this);

        if (auto decl = scope_->outscope(environment::kind::loop)) {
            ast::pointer<ast::type> expected = nullptr;
            
            if (auto range = dynamic_cast<const ast::for_range_expression*>(decl)) {
                if (range->body()) {
                    if (range->body()->annotation().type) expected = range->body()->annotation().type;
                    else {
                        range->body()->annotation().type = stmt.expression()->annotation().type;
                        std::static_pointer_cast<ast::block_expression>(range->body())->exprnode() = &stmt;
                    }
                }

                if (expected && expected->category() != ast::type::category::unknown_type &&
                    stmt.expression()->annotation().type->category() != ast::type::category::unknown_type &&
                    !types::compatible(expected, stmt.expression()->annotation().type)) {
                    auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(stmt.expression()->range().begin())
                                    .message(diagnostic::format("For range expects break type `$` but I found type `$` instead, idiot!", expected->string(), stmt.expression()->annotation().type->string()))
                                    .highlight(stmt.expression()->range());

                    publisher().publish(builder.build());
                }
            }
            else if (auto loop = dynamic_cast<const ast::for_loop_expression*>(decl)) {
                if (loop->body()) {
                    if (loop->body()->annotation().type) expected = loop->body()->annotation().type;
                    else {
                        loop->body()->annotation().type = stmt.expression()->annotation().type;
                        std::static_pointer_cast<ast::block_expression>(loop->body())->exprnode() = &stmt;
                    }
                }

                if (expected && expected && expected->category() != ast::type::category::unknown_type &&
                    stmt.expression()->annotation().type->category() != ast::type::category::unknown_type &&
                    !types::compatible(expected, stmt.expression()->annotation().type)) {
                    auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(stmt.expression()->range().begin())
                                    .message(diagnostic::format("For loop expects break type `$` but I found type `$` instead, idiot!", expected->string(), stmt.expression()->annotation().type->string()))
                                    .highlight(stmt.expression()->range());

                    publisher().publish(builder.build());               
                }
            }
        }
        else report(stmt.range(), "You cannot use a `break` statement outside loop, dammit!");

        stmt.annotation().resolved = true;
    }

    void checker::visit(const ast::continue_statement& stmt) 
    {
        stmt.annotation().scope = scope_->enclosing();
        stmt.annotation().visited = true;

        if (!scope_->inside(environment::kind::loop)) {
            report(stmt.range(), "You cannot use a `continue` statement outside loop, dammit!");
        }

        stmt.annotation().resolved = true;
    }

    void checker::visit(const ast::contract_statement& stmt) 
    {
        stmt.annotation().scope = scope_->enclosing();
        stmt.annotation().visited = true;

        stmt.condition()->accept(*this);
        if (stmt.condition()->annotation().type->category() != ast::type::category::bool_type) report(stmt.condition()->range(), diagnostic::format("Contract condition must have `bool` type but I found `$`, you f*cker!", stmt.condition()->annotation().type->string()), "", "expected bool");

        stmt.annotation().resolved = true;
    }

    void checker::visit(const ast::field_declaration& decl) 
    {
        decl.type_expression()->accept(*this);
        decl.annotation().type = decl.type_expression()->annotation().type;
    }

    void checker::visit(const ast::tuple_field_declaration& decl) 
    {
        decl.type_expression()->accept(*this);
        decl.annotation().type = decl.type_expression()->annotation().type;
    }

    void checker::visit(const ast::parameter_declaration& decl) 
    {
        decl.type_expression()->accept(*this);
        // varidic `...i32` is treated as slice `[i32]`
        if (decl.is_variadic()) decl.annotation().type = types::slice(decl.type_expression()->annotation().type);
        else decl.annotation().type = decl.type_expression()->annotation().type;
        // mutability bit
        decl.annotation().type->mutability = std::dynamic_pointer_cast<ast::type_expression>(decl.type_expression())->is_mutable();
    }

    void checker::visit(const ast::var_declaration& decl)
    {
        decl.annotation().type = types::unknown();

        if (decl.invalid()) return;

        auto saved = scope_;

        if (decl.type_expression()) decl.type_expression()->accept(*this);
        
        decl.annotation().visited = true;
        
        if (decl.value()) try { decl.value()->accept(*this); } catch (checker::abort_error&) { throw; } catch (...) { scope_ = saved; }
        /*
        if (decl.type_expression() && decl.type_expression()->annotation().type)
            std::cout << "annotated type for `" << decl.name().lexeme() << "` is " << decl.type_expression()->annotation().type->string() << '\n';
        if (decl.value() && decl.value()->annotation().type)
            std::cout << "inferred type for `" << decl.name().lexeme() << "` is " << decl.value()->annotation().type->string() << '\n';
        */
        if (!decl.type_expression()) {
            decl.annotation().type = decl.value() ? decl.value()->annotation().type : types::unknown();
        }
        else if (decl.value()) {
            if (!decl.type_expression()->annotation().type || decl.type_expression()->annotation().type->category() == ast::type::category::unknown_type || !decl.value()->annotation().type || decl.value()->annotation().type->category() == ast::type::category::unknown_type);
            else if (types::assignment_compatible(decl.type_expression()->annotation().type, decl.value()->annotation().type)) {
                if (auto implicit = implicit_cast(decl.type_expression()->annotation().type, decl.value())) {
                    decl.value() = implicit;
                }
                
                decl.annotation().type = decl.type_expression()->annotation().type;
            }
            else {
                std::string expected = decl.type_expression()->annotation().type->string();
                std::string found = decl.value()->annotation().type->string();
                auto diag = diagnostic::builder()
                            .location(decl.value()->range().begin())
                            .small(true)
                            .severity(diagnostic::severity::error)
                            .message(diagnostic::format("Variable value should have type `$` but I found type `$` instead, idiot!", expected, found))
                            .highlight(decl.value()->range(), diagnostic::format("expected $", expected))
                            .note(decl.type_expression()->range(), diagnostic::format("Here you tell me that variable type is `$`, dumb*ss!", expected))
                            .build();
                
                publisher().publish(diag);
                decl.invalid(true);
                decl.annotation().type = decl.type_expression()->annotation().type;
            }
        }
        else {
            decl.annotation().type = decl.type_expression()->annotation().type;
        }

        if (decl.annotation().type) decl.annotation().type->mutability = decl.is_mutable();

        // test that immutability is preserved
        if (decl.value()) test_immutable_assignment(decl, *decl.value());
        
        std::string name = decl.name().lexeme().string();

        // you cannot define a symbol whose name is a primitive type
        if (auto primitive = types::builtin(name)) {
            decl.annotation().type = types::unknown();
            report(decl.name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your variable, idiot!", name));
        }
        else {
            auto other = scope_->value(name);
            
            if (other && other != &decl) {
                auto builder = diagnostic::builder()
                            .location(decl.name().location())
                            .small(true)
                            .severity(diagnostic::severity::error)
                            .message(diagnostic::format("You have already declared a variable named `$`, idiot!", decl.name().lexeme()))
                            .highlight(decl.name().range(), "conflicting");

                if (auto constdecl = dynamic_cast<const ast::const_declaration*>(other)) {
                    builder.note(constdecl->name().range(), "Here's the homonymous declaration, you idiot!");
                }
                else if (auto vardecl = dynamic_cast<const ast::var_declaration*>(other)) {
                    builder.note(vardecl->name().range(), "Here's the homonymous declaration, you idiot!");
                }
                
                publisher().publish(builder.build());
            }
            else {
                scope_->value(name, &decl);
            }
        }

        // for array binding
        if (decl.annotation().type->category() == ast::type::category::array_type && decl.value() && (decl.value()->kind() == ast::kind::array_expression || decl.value()->kind() == ast::kind::array_sized_expression)) {
            // remove temporary from stack allocation
            pending_insertions.remove_if([&] (const decltype(pending_insertions)::value_type& t) { return decl.value()->annotation().referencing == std::get<1>(t).get(); });
            // remove reference
            decl.value()->annotation().referencing = nullptr;
        }

        decl.annotation().resolved = true;
        if (decl.annotation().type) decl.annotation().type->mutability = decl.is_mutable();
        else decl.annotation().type = types::unknown();
        // recorded as global
        if (auto workspace = dynamic_cast<const ast::workspace*>(scope_->enclosing())) const_cast<ast::workspace*>(workspace)->globals.push_back(&decl);
    }

    void checker::visit(const ast::var_tupled_declaration& decl)
    {
        decl.annotation().type = types::unknown();

        if (decl.invalid()) return;

        if (decl.type_expression()) report(decl.type_expression()->range(), "You cannot annotate type when using destructuring, idiot!");

        decl.annotation().visited = true;
        
        if (decl.value()) {
            decl.value()->accept(*this);
            decl.annotation().type = decl.value()->annotation().type;
        }
        
        if (auto tuplety = std::dynamic_pointer_cast<ast::tuple_type>(decl.value()->annotation().type)) {
            if (tuplety->components().size() != decl.names().size()) {
                auto builder = diagnostic::builder()
                                .location(decl.value()->range().begin())
                                .small(true)
                                .severity(diagnostic::severity::error)
                                .message(diagnostic::format("Tuple must have `$` components as the number of variables, but I count `$`.", decl.names().size(), tuplety->components().size()))
                                .highlight(decl.value()->range(), diagnostic::format("expected $ components", decl.names().size()));

                for (auto name : decl.names()) builder.highlight(name.range(), diagnostic::highlighter::mode::light);

                publisher().publish(builder.build());
                decl.invalid(true);
                throw semantic_error();
            }
        }
        else if (auto structurety = std::dynamic_pointer_cast<ast::structure_type>(decl.value()->annotation().type)) {
            if (structurety->fields().size() != decl.names().size()) {
                auto builder = diagnostic::builder()
                                .location(decl.value()->range().begin())
                                .small(true)
                                .severity(diagnostic::severity::error)
                                .message(diagnostic::format("Structure must have `$` fields as the number of variables, but I count `$`.", decl.names().size(), structurety->fields().size()))
                                .highlight(decl.value()->range(), diagnostic::format("expected $ fields", decl.names().size()));

                for (auto name : decl.names()) builder.highlight(name.range(), diagnostic::highlighter::mode::light);

                publisher().publish(builder.build());
                decl.invalid(true);
                throw semantic_error();
            }
        }
        else if (auto arrayty = std::dynamic_pointer_cast<ast::array_type>(decl.value()->annotation().type)) {
            if (arrayty->size() != decl.names().size()) {
                auto builder = diagnostic::builder()
                                .location(decl.value()->range().begin())
                                .small(true)
                                .severity(diagnostic::severity::error)
                                .message(diagnostic::format("Array must have `$` elements as the number of variables, but I count `$`.", decl.names().size(), arrayty->size()))
                                .highlight(decl.value()->range(), diagnostic::format("expected $ elements", decl.names().size()));

                for (auto name : decl.names()) builder.highlight(name.range(), diagnostic::highlighter::mode::light);

                publisher().publish(builder.build());
                decl.invalid(true);
                throw semantic_error();
            }
        }
        else {
            auto builder = diagnostic::builder()
                            .location(decl.value()->range().begin())
                            .small(true)
                            .severity(diagnostic::severity::error)
                            .message(diagnostic::format("Destructuring variable declaration expects tuple, structure or array value, but I found `$`", decl.value()->annotation().type->string()))
                            .highlight(decl.value()->range(), "expected tuple, structure or array");

            for (auto name : decl.names()) builder.highlight(name.range(), diagnostic::highlighter::mode::light);

            publisher().publish(builder.build());
            decl.invalid(true);
            throw semantic_error();
        }

        std::string structured_name = "__";

        for (auto id : decl.names()) structured_name.append(id.lexeme().string());

        auto clone = ast::create<ast::var_declaration>(decl.range(), std::vector<token>(), token::builder().artificial(true).kind(token::kind::identifier).lexeme(utf8::span::builder().concat(structured_name.data()).build()).build(), nullptr, decl.value());
        clone->annotation() = decl.annotation();
        // this is done for smart pointers management
        workspace()->saved.push_back(clone);
        // add artificial var declaration to current scope
        pending_insertions.emplace_back(scope_, clone, &decl, true);

        scope_->value(structured_name, clone.get());

        unsigned field = 0;

        for (auto identifier : decl.names()) {
            std::string name = identifier.lexeme().string();

            // you cannot define a symbol whose name is a primitive type
            if (auto primitive = types::builtin(name)) {
                decl.annotation().type = types::unknown();
                report(identifier.range(), diagnostic::format("You cannot steal primitive type name `$` to create your variable, idiot!", name));
            }
            else {
                auto other = scope_->value(name);
                if (other && other != &decl) {
                    auto builder = diagnostic::builder()
                                .location(identifier.location())
                                .small(true)
                                .severity(diagnostic::severity::error)
                                .message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                                .highlight(identifier.range(), "conflicting");

                    if (auto constdecl = dynamic_cast<const ast::const_declaration*>(other)) {
                        builder.note(constdecl->name().range(), "Here's the homonymous declaration, you idiot!");
                    }
                    else if (auto vardecl = dynamic_cast<const ast::var_declaration*>(other)) {
                        builder.note(vardecl->name().range(), "Here's the homonymous declaration, you idiot!");
                    }
                    
                    publisher().publish(builder.build());
                }
                else {
                    ast::pointer<ast::expression> value = nullptr;
                    auto referenced = ast::create<ast::identifier_expression>(decl.value()->range(), token::builder().artificial(true).kind(token::kind::identifier).lexeme(utf8::span::builder().concat(structured_name.data()).build()).build(), ast::pointers<ast::expression>(), false);
                    referenced->annotation().type = decl.annotation().type;
                    referenced->annotation().referencing = clone.get();

                    if (auto tuplety = std::dynamic_pointer_cast<ast::tuple_type>(decl.annotation().type)) {
                        value = ast::create<ast::tuple_index_expression>(decl.value()->range(), referenced, token::builder().artificial(true).kind(token::kind::integer_literal).lexeme(utf8::span::builder().concat(std::to_string(field).data()).build()).build());
                        value->annotation().type = tuplety->components().at(field);
                    }
                    else if (auto structurety = std::dynamic_pointer_cast<ast::structure_type>(decl.annotation().type)) {
                        auto item = std::static_pointer_cast<ast::structure_type>(decl.annotation().type)->fields().at(field);
                        auto field = ast::create<ast::identifier_expression>(decl.value()->range(), token::builder().kind(token::kind::identifier).artificial(true).lexeme(utf8::span::builder().concat(item.name.data()).build()).build(), ast::pointers<ast::expression>(), false);
                        field->annotation().type = item.type;
                        if (auto sdecl = static_cast<const ast::record_declaration*>(structurety->declaration())) {
                            auto it = std::find_if(sdecl->fields().begin(), sdecl->fields().end(), [&] (ast::pointer<ast::declaration> fdecl) { return std::static_pointer_cast<ast::field_declaration>(fdecl)->name().lexeme().string() == item.name; });
                            field->annotation().referencing = it->get();
                        }
                        value = ast::create<ast::member_expression>(decl.value()->range(), referenced, field);
                        value->annotation().type = item.type;
                    }
                    else if (auto arrayty = std::dynamic_pointer_cast<ast::array_type>(decl.annotation().type)) {
                        value = ast::create<ast::array_index_expression>(decl.value()->range(), referenced, ast::create<ast::literal_expression>(token::builder().artificial(true).kind(token::kind::integer_literal).lexeme(utf8::span::builder().concat(std::to_string(field).data()).build()).build()));
                        value->annotation().type = arrayty->base();
                    }

                    auto vardecl = ast::create<ast::var_declaration>(decl.range(), std::vector<token>(), identifier, ast::pointer<ast::expression>(), value);
                    vardecl->annotation().type = value->annotation().type;
                    if (vardecl->annotation().type) vardecl->annotation().type->mutability = decl.is_mutable();
                    // test that immutability is preserved
                    if (decl.value()) test_immutable_assignment(*vardecl, *decl.value());
                    // this is done for smart pointers management
                    workspace()->saved.push_back(vardecl);

                    scope_->remove(vardecl.get());
                    scope_->value(name, vardecl.get());
                    // add artificial var declaration to current scope
                    pending_insertions.emplace_back(scope_, vardecl, clone.get(), true);
                }
            }

            ++field;
        }

        decl.annotation().resolved = true;

        // for array binding
        if (decl.annotation().type->category() == ast::type::category::array_type && decl.value() && (decl.value()->kind() == ast::kind::array_expression || decl.value()->kind() == ast::kind::array_sized_expression)) {
            // remove temporary from stack allocation
            pending_insertions.remove_if([&] (const decltype(pending_insertions)::value_type& t) { return decl.value()->annotation().referencing == std::get<1>(t).get(); });
            // remove reference
            decl.value()->annotation().referencing = nullptr;
        }
    }

    void checker::visit(const ast::const_declaration& decl)
    {
        decl.annotation().type = types::unknown();
        
        if (decl.invalid()) return;

        if (decl.type_expression()) decl.type_expression()->accept(*this);
        decl.annotation().visited = true;
        decl.value()->accept(*this);

        if (decl.type_expression() && decl.type_expression()->annotation().type) {
            if (!types::builtin(decl.type_expression()->annotation().type->string())) {
                decl.invalid(true);
                decl.annotation().type = types::unknown();
                error(decl.type_expression()->range(), diagnostic::format("Constants must have primitive type, you f*cker! I found `$` instead.", decl.type_expression()->annotation().type->string()));
            }
        }
        
        constval value;
        
        try { 
            value = evaluate(decl.value());
            decl.value()->annotation().type = value.type;
            decl.value()->annotation().value = value;
        } 
        catch (evaluator::generic_evaluation&) {}

        if (!decl.type_expression()) {
            decl.annotation().type = decl.value()->annotation().type;
        }
        else if (!decl.type_expression()->annotation().type || decl.type_expression()->annotation().type->category() == ast::type::category::unknown_type || !decl.value()->annotation().type || decl.value()->annotation().type->category() == ast::type::category::unknown_type);
        else if (types::assignment_compatible(decl.type_expression()->annotation().type, decl.value()->annotation().type)) {
            if (auto implicit = implicit_cast(decl.type_expression()->annotation().type, decl.value())) {
                decl.value() = implicit;
            }
            
            decl.annotation().type = decl.type_expression()->annotation().type;
        }
        else {
            std::string expected = decl.type_expression()->annotation().type->string();
            std::string found = decl.value()->annotation().type->string();
            auto diag = diagnostic::builder()
                        .location(decl.value()->range().begin())
                        .small(true)
                        .severity(diagnostic::severity::error)
                        .message(diagnostic::format("Constant value should have type `$` but I found type `$` instead, idiot!", expected, found))
                        .highlight(decl.value()->range(), diagnostic::format("expected $", expected))
                        .note(decl.type_expression()->range(), diagnostic::format("Here you tell me that constant type is `$`, dumb*ss!", expected))
                        .build();
            
            publisher().publish(diag);
            decl.invalid(true);
            decl.annotation().type = decl.type_expression()->annotation().type;
        }
        
        std::string name = decl.name().lexeme().string();

        // you cannot define a symbol whose name is a primitive type
        if (auto primitive = types::builtin(name)) {
            decl.annotation().type = types::unknown();
            report(decl.name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your constant, idiot!", name));
        }
        else {
            auto other = scope_->value(name);

            if (other && other != &decl) {
                auto builder = diagnostic::builder()
                            .location(decl.name().location())
                            .small(true)
                            .severity(diagnostic::severity::error)
                            .message(diagnostic::format("You have already declared a variable named `$`, idiot!", decl.name().lexeme()))
                            .highlight(decl.name().range(), "conflicting");

                if (auto constdecl = dynamic_cast<const ast::const_declaration*>(other)) {
                    builder.note(constdecl->name().range(), "Here's the homonymous declaration, you idiot!");
                }
                else if (auto vardecl = dynamic_cast<const ast::var_declaration*>(other)) {
                    builder.note(vardecl->name().range(), "Here's the homonymous declaration, you idiot!");
                }
                
                publisher().publish(builder.build());
            }
            else {
                scope_->value(name, &decl);
            }
        }

        decl.annotation().resolved = true;
        // recorded as global
        if (auto workspace = dynamic_cast<const ast::workspace*>(scope_->enclosing())) const_cast<ast::workspace*>(workspace)->globals.push_back(&decl);
    }

    void checker::visit(const ast::const_tupled_declaration& decl) 
    {
        decl.annotation().type = types::unknown();

        if (decl.invalid()) return;

        if (decl.type_expression()) report(decl.type_expression()->range(), "You cannot annotate type when using destructuring, idiot!");

        decl.annotation().visited = true;
        
        if (decl.value()) {
            decl.value()->accept(*this);
            decl.annotation().type = decl.value()->annotation().type;
        }

        constval value;
        
        try { 
            value = evaluate(decl.value());
            decl.value()->annotation().type = value.type;
            decl.value()->annotation().value = value;
        } 
        catch (evaluator::generic_evaluation&) {}
        
        if (auto tuplety = std::dynamic_pointer_cast<ast::tuple_type>(decl.value()->annotation().type)) {
            if (tuplety->components().size() != decl.names().size()) {
                auto builder = diagnostic::builder()
                                .location(decl.value()->range().begin())
                                .small(true)
                                .severity(diagnostic::severity::error)
                                .message(diagnostic::format("Tuple must have `$` components as the number of variables, but I count `$`.", decl.names().size(), tuplety->components().size()))
                                .highlight(decl.value()->range(), diagnostic::format("expected $ components", decl.names().size()));

                for (auto name : decl.names()) builder.highlight(name.range(), diagnostic::highlighter::mode::light);

                publisher().publish(builder.build());
                decl.invalid(true);
                throw semantic_error();
            }
        }
        else if (auto arrayty = std::dynamic_pointer_cast<ast::array_type>(decl.value()->annotation().type)) {
            if (arrayty->size() != decl.names().size()) {
                auto builder = diagnostic::builder()
                                .location(decl.value()->range().begin())
                                .small(true)
                                .severity(diagnostic::severity::error)
                                .message(diagnostic::format("Array must have `$` elements as the number of variables, but I count `$`.", decl.names().size(), arrayty->size()))
                                .highlight(decl.value()->range(), diagnostic::format("expected $ elements", decl.names().size()));

                for (auto name : decl.names()) builder.highlight(name.range(), diagnostic::highlighter::mode::light);

                publisher().publish(builder.build());
                decl.invalid(true);
                throw semantic_error();
            }
        }
        else {
            auto builder = diagnostic::builder()
                            .location(decl.value()->range().begin())
                            .small(true)
                            .severity(diagnostic::severity::error)
                            .message(diagnostic::format("Destructuring variable declaration expects tuple, structure or array value, but I found `$`", decl.value()->annotation().type->string()))
                            .highlight(decl.value()->range(), "expected tuple or array");

            for (auto name : decl.names()) builder.highlight(name.range(), diagnostic::highlighter::mode::light);

            publisher().publish(builder.build());
            decl.invalid(true);
            throw semantic_error();
        }

        std::string structured_name = "__";

        for (auto id : decl.names()) structured_name.append(id.lexeme().string());

        auto clone = ast::create<ast::const_declaration>(decl.range(), token::builder().artificial(true).kind(token::kind::identifier).lexeme(utf8::span::builder().concat(structured_name.data()).build()).build(), nullptr, decl.value());
        clone->annotation() = decl.annotation();
        // this is done for smart pointers management
        workspace()->saved.push_back(clone);
        // add artificial var declaration to current scope
        pending_insertions.emplace_back(scope_, clone, &decl, true);

        scope_->value(structured_name, clone.get());

        unsigned field = 0;

        scope_->remove(&decl);

        for (auto identifier : decl.names()) {
            std::string name = identifier.lexeme().string();

            // you cannot define a symbol whose name is a primitive type
            if (auto primitive = types::builtin(name)) {
                decl.annotation().type = types::unknown();
                report(identifier.range(), diagnostic::format("You cannot steal primitive type name `$` to create your variable, idiot!", name));
            }
            else {
                auto other = scope_->value(name);
                
                if (other && other != &decl) {
                    auto builder = diagnostic::builder()
                                .location(identifier.location())
                                .small(true)
                                .severity(diagnostic::severity::error)
                                .message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                                .highlight(identifier.range(), "conflicting");

                    if (auto constdecl = dynamic_cast<const ast::const_declaration*>(other)) {
                        builder.note(constdecl->name().range(), "Here's the homonymous declaration, you idiot!");
                    }
                    else if (auto vardecl = dynamic_cast<const ast::var_declaration*>(other)) {
                        builder.note(vardecl->name().range(), "Here's the homonymous declaration, you idiot!");
                    }
                    
                    publisher().publish(builder.build());
                }
                else {
                    ast::pointer<ast::expression> value = nullptr;
                    auto referenced = ast::create<ast::identifier_expression>(decl.value()->range(), token::builder().artificial(true).kind(token::kind::identifier).lexeme(utf8::span::builder().concat(structured_name.data()).build()).build(), ast::pointers<ast::expression>(), false);
                    referenced->annotation().type = decl.annotation().type;
                    referenced->annotation().referencing = clone.get();

                    if (auto tuplety = std::dynamic_pointer_cast<ast::tuple_type>(decl.annotation().type)) {
                        value = ast::create<ast::tuple_index_expression>(decl.value()->range(), referenced, token::builder().artificial(true).kind(token::kind::integer_literal).lexeme(utf8::span::builder().concat(std::to_string(field).data()).build()).build());
                        value->annotation().type = tuplety->components().at(field);
                    }
                    else if (auto arrayty = std::dynamic_pointer_cast<ast::array_type>(decl.annotation().type)) {
                        value = ast::create<ast::array_index_expression>(decl.value()->range(), referenced, ast::create<ast::literal_expression>(token::builder().artificial(true).kind(token::kind::integer_literal).lexeme(utf8::span::builder().concat(std::to_string(field).data()).build()).build()));
                        value->annotation().type = arrayty->base();
                    }

                    auto constdecl = ast::create<ast::const_declaration>(decl.range(), identifier, ast::pointer<ast::expression>(), value);
                    constdecl->annotation().type = value->annotation().type;

                    try { 
                        constval value = evaluate(constdecl->value());
                        constdecl->value()->annotation().type = value.type;
                        constdecl->value()->annotation().value = value;
                    } 
                    catch (evaluator::generic_evaluation&) {}
                
                    workspace()->saved.push_back(constdecl);

                    scope_->remove(constdecl.get());
                    scope_->value(name, constdecl.get());

                    pending_insertions.emplace_back(scope_, constdecl, clone.get(), true);
                }
            }

            ++field;
        }

        decl.annotation().resolved = true;
    }

    void checker::visit(const ast::generic_clause_declaration& decl) 
    {
        for (auto param : decl.parameters()) {
            try { param->accept(*this); } catch (semantic_error&) { param->annotation().resolved = true; }
            if (param->invalid()) decl.invalid(true);
        }
    }

    void checker::visit(const ast::generic_const_parameter_declaration& decl) 
    {
        // generic constant may shadow outside constants
        std::string name = decl.name().lexeme().string();
        auto other = scope_->value(name, false);

        decl.annotation().visited = true;
        decl.type_expression()->accept(*this);

        if (other && other != &decl) {
            auto diag = diagnostic::builder()
                        .location(decl.name().location())
                        .small(true)
                        .severity(diagnostic::severity::error)
                        .message(diagnostic::format("You have already declared constant parameter named `$`, idiot!", name))
                        .highlight(decl.name().range(), "conflicting")
                        .note(dynamic_cast<const ast::generic_const_parameter_declaration*>(other)->name().range(), "Here's the previous declaration.")
                        .build();

            publisher().publish(diag);
            decl.invalid(true);
        }
        else {
            auto other = scope_->value(name);

            if (other && other->kind() == ast::kind::generic_const_parameter_declaration && static_cast<const ast::generic_const_parameter_declaration*>(other)->name() != decl.name()) {
                auto diag = diagnostic::builder()
                            .location(decl.name().location())
                            .small(true)
                            .severity(diagnostic::severity::error)
                            .message(diagnostic::format("You will shadow constant parameter `$` from outer scope, idiot!", name))
                            .highlight(decl.name().range())
                            .note(static_cast<const ast::generic_const_parameter_declaration*>(other)->name().range(), "Here's the other declaration.")
                            .build();

                publisher().publish(diag);
                decl.invalid(true);
            }
            
            scope_->value(name, &decl);
            decl.annotation().type = decl.type_expression()->annotation().type;
        }

        decl.annotation().resolved = true;
    }

    void checker::visit(const ast::generic_type_parameter_declaration& decl) 
    {
        // generic type parameter may shadow homonyms in parent scopes
        std::string name = decl.name().lexeme().string();
        auto other = scope_->type(name, false);

        decl.annotation().visited = true;

        if (other && other != &decl) {
            auto diag = diagnostic::builder()
                        .location(decl.name().location())
                        .small(true)
                        .severity(diagnostic::severity::error)
                        .message(diagnostic::format("You have already declared type parameter named `$`, idiot!", name))
                        .highlight(decl.name().range(), "conflicting")
                        .note(dynamic_cast<const ast::generic_type_parameter_declaration*>(other)->name().range(), "Here's the previous declaration.")
                        .build();

            publisher().publish(diag);
            decl.invalid(true);
        }
        else {
            auto other = scope_->type(name);

            if (other && other->name() != decl.name() && other->kind() == ast::kind::generic_type_parameter_declaration) {
                auto diag = diagnostic::builder()
                            .location(decl.name().location())
                            .small(true)
                            .severity(diagnostic::severity::error)
                            .message(diagnostic::format("You will shadow type parameter `$` from outer scope, idiot!", name))
                            .highlight(decl.name().range())
                            .note(static_cast<const ast::generic_type_parameter_declaration*>(other)->name().range(), "Here's the other declaration.")
                            .build();

                publisher().publish(diag);
                decl.invalid(true);
            }
            
            decl.annotation().type = types::generic();
            decl.annotation().type->declaration(&decl);
            scope_->type(name, &decl);
        }

        decl.annotation().resolved = true;
    }

    void checker::visit(const ast::test_declaration& decl) 
    {
        decl.body()->accept(*this); 
    }

    void checker::visit(const ast::function_declaration& decl) 
    {
        std::unordered_map<std::string, token> names;
        ast::types formals;

        decl.annotation().visited = true;

        if (decl.generic()) {
            begin_scope(decl.generic().get());
            decl.generic()->accept(*this);
        }
        
        // open function scope to declare parameters and local variables
        auto saved = begin_scope(&decl);

        for (auto pdecl : decl.parameters()) {
            pdecl->accept(*this);
            auto param = std::dynamic_pointer_cast<ast::parameter_declaration>(pdecl);
            auto other = names.find(param->name().lexeme().string());
            if (other != names.end()) {
                auto diag = diagnostic::builder()
                            .location(param->name().location())
                            .severity(diagnostic::severity::error)
                            .message(diagnostic::format("You have already declared a parameter named `$`, idiot!", param->name().lexeme()))
                            .highlight(param->name().range(), "conflicting")
                            .note(other->second.range(), "Here's the homonymous parameter, you f*cker!")
                            .build();
                
                publisher().publish(diag);
            }
            else {
                names.emplace(param->name().lexeme().string(), param->name());
                saved->value(param->name().lexeme().string(), param.get());
                formals.push_back(param->annotation().type);
            }
        }

        if (decl.return_type_expression()) try { decl.return_type_expression()->accept(*this); } catch (semantic_error& err) { scope_ = saved; }
        
        auto result_type = decl.return_type_expression() ? decl.return_type_expression()->annotation().type : types::unit();
        // type is annotated here so it is available if recursive calls occurs and function type is queried
        decl.annotation().type = types::function(formals, result_type);
        // since this function depends on its generics arguments all checking is postponed to che instantiated version of this function
        if (decl.generic());
        else if (!decl.body()) {
            if (auto outer = decl.annotation().scope) {
                if (outer->kind() != ast::kind::behaviour_declaration && outer->kind() != ast::kind::concept_declaration && outer->kind() != ast::kind::concept_declaration && !decl.external) {
                    decl.invalid(true);
                    report(decl.name().range(), "You must provide a body for this function, idiot!", "", "expected body");
                }
            }
        }
        else try {
            decl.body()->accept(*this);
            if (auto block = std::dynamic_pointer_cast<ast::block_expression>(decl.body())) {
                if (auto exprstmt = dynamic_cast<const ast::expression_statement*>(block->exprnode())) {
                    if (result_type && result_type->category() != ast::type::category::unknown_type &&
                        exprstmt->expression()->annotation().type && exprstmt->expression()->annotation().type->category() != ast::type::category::unknown_type &&
                        !types::compatible(result_type, types::unit()) &&
                        !types::assignment_compatible(result_type, exprstmt->expression()->annotation().type)) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(exprstmt->expression()->range().begin())
                                    .message(diagnostic::format("Function `$` expects return type `$` but I found type `$` instead, idiot!", decl.name().lexeme(), result_type->string(), exprstmt->expression()->annotation().type->string()))
                                    .highlight(exprstmt->expression()->range());

                        if (decl.return_type_expression()) {
                            builder.note(decl.return_type_expression()->range(), diagnostic::format("You can see that return type of function `$` is `$`.", decl.name().lexeme(), result_type->string()));
                            builder.replacement(decl.return_type_expression()->range(), exprstmt->expression()->annotation().type->string(), "Try replacing return type in definition.");
                        }

                        publisher().publish(builder.build());
                        decl.invalid(true);
                    }
                }
                else if (dynamic_cast<const ast::return_statement*>(block->exprnode()));
                // missing return statement or expression node
                else if (!types::compatible(types::unit(), result_type)) {
                    decl.invalid(true);
                    report(decl.name().range(), diagnostic::format("You forgot to return any result from function that expects `$`, idiot!", result_type->string()));
                }
            }
            else if (auto exprbody = std::dynamic_pointer_cast<ast::expression>(decl.body())) {
                if (result_type->category() != ast::type::category::unknown_type &&
                    exprbody->annotation().type->category() != ast::type::category::unknown_type &&
                    !types::assignment_compatible(result_type, exprbody->annotation().type)) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(exprbody->range().begin())
                                .message(diagnostic::format("Function `$` expects return type `$` but I found type `$` instead, idiot!", decl.name().lexeme(), result_type->string(), exprbody->annotation().type->string()))
                                .highlight(exprbody->range());

                    if (decl.return_type_expression()) {
                        builder.note(decl.return_type_expression()->range(), diagnostic::format("You can see that return type of function `$` is `$`.", decl.name().lexeme(), result_type->string()));
                        builder.replacement(decl.return_type_expression()->range(), exprbody->annotation().type->string(), "Try replacing return type in definition.");
                    }

                    publisher().publish(builder.build());
                    decl.invalid(true);
                }
            }
        }
        catch (semantic_error& err) { scope_ = saved; }

        // end function scope
        end_scope();

        if (decl.generic()) end_scope();
        
        // check entry point signature
        if (&decl == entry_point_ && !types::compatible(types::function({}, types::unit()), decl.annotation().type) && !types::compatible(types::function({ types::slice(types::chars()) }, types::unit()), decl.annotation().type)) {
            error(decl.name().range(), "Entry point has wrong signature, it must be something like \\ • start(args: [chars]) {} \\ • start() {} \\ \\ Okay?", "", "wrong signature");
        }

        decl.annotation().resolved = true;

        // check that it is conformant to behaviour if prototype
        if (auto behaviour = dynamic_cast<const ast::behaviour_declaration*>(scope_->outscope(environment::kind::declaration))) {
            if (decl.parameters().size() == 0) {
                decl.invalid(true);
                report(decl.name().range(), diagnostic::format("Function `$` must take at least parameter one parameter of behaviour type `*$`, idiot.", decl.name().lexeme(), behaviour->name().lexeme()));
            }
            else if (!types::compatible(types::pointer(behaviour->annotation().type), decl.parameters().front()->annotation().type)) {
                decl.invalid(true);
                report(decl.name().range(), diagnostic::format("Function `$` must take parameter of behaviour type `*$` as first argument, found `$`, idiot.", decl.name().lexeme(), behaviour->annotation().type->string(), decl.parameters().front()->annotation().type->string()));
            }
        }

        // if not associated function, then it is added to all workspace functions
        if (!decl.generic() && !dynamic_cast<const ast::type_declaration*>(scope_->enclosing())) workspace()->functions.push_back(&decl);
    }

    void checker::visit(const ast::property_declaration& decl) 
    {
        std::unordered_map<std::string, token> names;
        ast::types formals;

        decl.annotation().visited = true;

        auto typescope = dynamic_cast<const ast::type_declaration*>(decl.annotation().scope);
        
        // open function scope to declare parameters and local variables
        auto saved = begin_scope(&decl);

        for (auto pdecl : decl.parameters()) {
            pdecl->accept(*this);
            auto param = std::dynamic_pointer_cast<ast::parameter_declaration>(pdecl);
            auto other = names.find(param->name().lexeme().string());
            if (other != names.end()) {
                auto diag = diagnostic::builder()
                            .location(param->name().location())
                            .severity(diagnostic::severity::error)
                            .message(diagnostic::format("You have already declared a parameter named `$`, idiot!", param->name().lexeme()))
                            .highlight(param->name().range(), "conflicting")
                            .note(other->second.range(), "Here's the homonymous parameter, you f*cker!")
                            .build();
                
                publisher().publish(diag);
            }
            else {
                names.emplace(param->name().lexeme().string(), param->name());
                saved->value(param->name().lexeme().string(), param.get());
                formals.push_back(param->type_expression()->annotation().type);
            }
        }

        if (decl.parameters().size() != 1) {
            decl.invalid(true);
            report(decl.name().range(), diagnostic::format("Property `$` must take exactly `1` parameter of type `$`, you gave it `$`, c*nt!", decl.name().lexeme(), typescope->annotation().type->string(), decl.parameters().size()));
        }
        else if (!decl.parameters().front()->annotation().type || decl.parameters().front()->annotation().type->category() == ast::type::category::unknown_type) {
            decl.invalid(true);
        }
        // maybe property is inside a concept
        else if (typescope && !types::assignment_compatible(decl.parameters().front()->annotation().type, typescope->annotation().type)) {
            decl.invalid(true);
            report(decl.name().range(), diagnostic::format("Property `$` parameter must have type `$`, I found `$` instead, c*nt!", decl.name().lexeme(), typescope->annotation().type->string(), decl.parameters().front()->annotation().type->string()));
        }

        if (decl.return_type_expression()) try { decl.return_type_expression()->accept(*this); } catch (semantic_error& err) { scope_ = saved; }

        auto result_type = decl.return_type_expression() ? decl.return_type_expression()->annotation().type : types::unit();

        if (!decl.body()) {
            if (auto outer = scope_->parent()->outscope(environment::kind::declaration)) {
                if (outer->kind() != ast::kind::behaviour_declaration && outer->kind() != ast::kind::extern_declaration) {
                    decl.invalid(true);
                    report(decl.name().range(), "You must provide a body for this property, idiot!", "", "expected body");
                }
            }
        }
        else try { 
            decl.body()->accept(*this); 
            if (auto block = std::dynamic_pointer_cast<ast::block_expression>(decl.body())) {
                if (auto exprstmt = dynamic_cast<const ast::expression_statement*>(block->exprnode())) {
                    if (result_type->category() != ast::type::category::unknown_type &&
                        exprstmt->expression()->annotation().type && exprstmt->expression()->annotation().type->category() != ast::type::category::unknown_type &&
                        !types::compatible(result_type, types::unit()) &&
                        !types::assignment_compatible(result_type, exprstmt->expression()->annotation().type)) {
                        auto builder = diagnostic::builder()
                                    .severity(diagnostic::severity::error)
                                    .location(exprstmt->expression()->range().begin())
                                    .message(diagnostic::format("Property `$` expects return type `$` but I found type `$` instead, idiot!", decl.name().lexeme(), result_type->string(), exprstmt->expression()->annotation().type->string()))
                                    .highlight(exprstmt->expression()->range());

                        if (decl.return_type_expression()) {
                            builder.note(decl.return_type_expression()->range(), diagnostic::format("You can see that return type of property `$` is `$`.", decl.name().lexeme(), result_type->string()));
                            builder.replacement(decl.return_type_expression()->range(), exprstmt->expression()->annotation().type->string(), "Try replacing return type in definition.");
                        }

                        publisher().publish(builder.build());
                        decl.invalid(true);
                    }
                }
                else if (dynamic_cast<const ast::return_statement*>(block->exprnode()));
                // missing return statement or expression node
                else if (!types::compatible(types::unit(), result_type)) {
                    decl.invalid(true);
                    report(decl.name().range(), diagnostic::format("You forgot to return any result from function that expects `$`, idiot!", result_type->string()));
                }
            }
            else if (auto exprbody = std::dynamic_pointer_cast<ast::expression>(decl.body())) {
                if (result_type->category() != ast::type::category::unknown_type &&
                    exprbody->annotation().type->category() != ast::type::category::unknown_type &&
                    !types::assignment_compatible(result_type, exprbody->annotation().type)) {
                    auto builder = diagnostic::builder()
                                .severity(diagnostic::severity::error)
                                .location(exprbody->range().begin())
                                .message(diagnostic::format("Property `$` expects return type `$` but I found type `$` instead, idiot!", decl.name().lexeme(), result_type->string(), exprbody->annotation().type->string()))
                                .highlight(exprbody->range());

                    if (decl.return_type_expression()) {
                        builder.note(decl.return_type_expression()->range(), diagnostic::format("You can see that return type of property `$` is `$`.", decl.name().lexeme(), result_type->string()));
                        builder.replacement(decl.return_type_expression()->range(), exprbody->annotation().type->string(), "Try replacing return type in definition.");
                    }

                    publisher().publish(builder.build());
                    decl.invalid(true);
                }
            }
        } 
        catch (semantic_error& err) { scope_ = saved; }

        // end function scope
        end_scope();

        auto fntype = types::function(formals, result_type);

        decl.annotation().resolved = true;
        decl.annotation().type = fntype;

        // check that it is conformant to behaviour if prototype
        if (auto behaviour = dynamic_cast<const ast::behaviour_declaration*>(scope_->outscope(environment::kind::declaration))) {
            if (!types::compatible(types::pointer(behaviour->annotation().type), decl.parameters().front()->annotation().type)) {
                decl.invalid(true);
                report(decl.name().range(), diagnostic::format("Property `$` must take parameter of behaviour type `*$` as first argument, found `$`, idiot.", decl.name().lexeme(), behaviour->annotation().type->string(), decl.parameters().front()->annotation().type->string()));
            }
        }

        // checks for implicit conversion or operator functions or properties
        if (!decl.invalid() && decl.name().lexeme().string() == "str") {
            if (!types::compatible(typescope->annotation().type, fntype->formals().front()) || fntype->result()->category() != ast::type::category::string_type) {
                auto tname = typescope->annotation().type->string();
                warning(decl.name().range(), diagnostic::format("Property `str` won't be considered for conversion of type `$` to `string` as it doesn't respect its prototype!", tname), diagnostic::format("I suggest providing string conversion property `str`, like this \\ \\ `extend $ { .str(self: $) string {...} }`", tname, tname), "wrong prototype");
            }
        }
    }

    void checker::visit(const ast::concept_declaration& decl)
    {
        if (decl.invalid()) return;

        decl.annotation().visited = true;

        if (decl.generic()) {
            begin_scope(decl.generic().get());
            decl.generic()->accept(*this);
        }

        begin_scope(&decl);

        auto saved = scope_;

        if (decl.base()) try { 
            decl.base()->accept(*this);
        }
        catch (abort_error&) { throw; }
        catch (cyclic_symbol_error& e) 
        {
            publisher().publish(e.diagnostic());
            scope_ = saved;
        }
        catch (semantic_error&) { scope_ = saved; }

        if (pass_ == pass::second) {
            for (auto prototype : decl.prototypes()) try {
                if (auto fdecl = std::dynamic_pointer_cast<ast::function_declaration>(prototype)) {
                    std::string name = fdecl->name().lexeme().string();
                    auto other = scope_->function(name);
                    if (workspace()->name == name) {
                        auto diag = diagnostic::builder()
                                    .location(fdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("This function name conflicts with workspace name `$`, c*nt!", name))
                                    .highlight(fdecl->name().range(), "conflicting")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else if (types::builtin(name)) {
                        fdecl->annotation().type = types::unknown();
                        error(fdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your function, idiot!", name));
                    }
                    else if (other && other != fdecl.get()) {
                        fdecl->annotation().type = types::unknown();
                        auto diag = diagnostic::builder()
                                    .location(fdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("You have already declared a function or property named `$`, idiot!", name))
                                    .highlight(fdecl->name().range(), "conflicting")
                                    .note(static_cast<const ast::function_declaration*>(other)->name().range(), "Here's the homonymous declaration, you f*cker!")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else {
                        scope_->function(name, fdecl.get());
                    }
                }
                else if (auto pdecl = std::dynamic_pointer_cast<ast::property_declaration>(prototype)) {
                    std::string name = pdecl->name().lexeme().string();
                    auto other = scope_->function(name);
                    if (workspace()->name == name) {
                        auto diag = diagnostic::builder()
                                    .location(pdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("This property name conflicts with workspace name `$`, c*nt!", name))
                                    .highlight(pdecl->name().range(), "conflicting")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else if (types::builtin(name)) {
                        pdecl->annotation().type = types::unknown();
                        error(pdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your property, idiot!", name));
                    }
                    else if (other && other != pdecl.get()) {
                        pdecl->annotation().type = types::unknown();
                        auto diag = diagnostic::builder()
                                    .location(pdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("You have already declared a function or property named `$`, idiot!", name))
                                    .highlight(pdecl->name().range(), "conflicting")
                                    .note(static_cast<const ast::function_declaration*>(other)->name().range(), "Here's the homonymous declaration, you f*cker!")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else {
                        scope_->function(name, pdecl.get());
                    }
                }
            }
            catch (semantic_error& err) { scope_ = saved; }
        }
        else if (pass_ == pass::third) {
            for (auto prototype : scope_->functions()) prototype.second->accept(*this);
        }

        end_scope();

        if (decl.generic()) end_scope();

        scope_->concept(decl.name().lexeme().string(), &decl);

        decl.annotation().resolved = true;
    }

    bool checker::is_partially_specialized(ast::pointer<ast::path_type_expression> expr)
    {
        bool invalid = false;

        if (auto member = std::static_pointer_cast<ast::identifier_expression>(expr->member())) {
            // all generic arguments must be in simple form like A, T, S and composite or nested types are not admitted because a pattern_matcher would be necessary in that case
            for (std::size_t i = 0; i < member->generics().size(); ++i) {
                bool specialization = false;

                if (auto typearg = std::dynamic_pointer_cast<ast::path_type_expression>(member->generics().at(i))) {
                    if (!typearg->is_parametric() || typearg->annotation().type->category() != ast::type::category::generic_type) specialization = true;
                }
                else if (auto constarg = std::dynamic_pointer_cast<ast::identifier_expression>(member->generics().at(i))) {
                    if (!constarg->annotation().isparametric) specialization = true;
                }
                else specialization = true; 

                if (specialization) {
                    invalid = true;
                    report(member->generics().at(i)->range(), diagnostic::format("Partial specialization of parametric type `$` is not allowed, idiot! \\ You can only use simple generic parameters as arguments!", member->annotation().type->string()));
                }
            }
            // due to previous errors
            if (invalid) {
                throw semantic_error();
            }
        }

        if (auto left = std::dynamic_pointer_cast<ast::identifier_expression>(expr->expression())) {
            // all generic arguments must be in simple form like A, T, S and composite or nested types are not admitted because a pattern_matcher would be necessary in that case
            for (std::size_t i = 0; i < left->generics().size(); ++i) {
                bool specialization = false;

                if (auto typearg = std::dynamic_pointer_cast<ast::path_type_expression>(left->generics().at(i))) {
                    if (!typearg->is_parametric() || typearg->annotation().type->category() != ast::type::category::generic_type) specialization = true;
                }
                else if (auto constarg = std::dynamic_pointer_cast<ast::identifier_expression>(left->generics().at(i))) {
                    if (!constarg->annotation().isparametric) specialization = true;
                }
                else specialization = true; 

                if (specialization) {
                    invalid = true;
                    report(left->generics().at(i)->range(), diagnostic::format("Partial specialization of parametric type `$` is not allowed, idiot! \\ You can only use simple generic parameters as arguments!", left->annotation().type->string()));
                }
            }
            // due to previous errors
            if (invalid) {
                throw semantic_error();
            }
        }
        else if (auto left = std::dynamic_pointer_cast<ast::path_type_expression>(expr->expression())) {
            return is_partially_specialized(left);
        }

        return false;
    }

    const ast::function_declaration* checker::is_cloneable(ast::pointer<ast::type> type) const
    {
        if (!types::builtin(type->string()) && type->declaration()) {
            for (auto fn : scopes_.at(type->declaration())->functions()) {
                if (fn.first != "clone" || fn.second->kind() != ast::kind::function_declaration) continue;
                auto fdecl = static_cast<const ast::function_declaration*>(fn.second);
                auto result = std::static_pointer_cast<ast::function_type>(fdecl->annotation().type)->result();
                if (fdecl->generic() || fdecl->parameters().size() != 1) continue;
                auto paramdecl = std::static_pointer_cast<ast::parameter_declaration>(fdecl->parameters().front());
                if (paramdecl->is_variadic()) continue;
                if (!types::assignment_compatible(type, paramdecl->annotation().type) || !types::compatible(type, result)) continue;
                return fdecl;
            }
        }

        return nullptr;
    }

    const ast::function_declaration* checker::is_destructible(ast::pointer<ast::type> type) const
    {
        if (!types::builtin(type->string()) && type->declaration()) {
            for (auto fn : scopes_.at(type->declaration())->functions()) {
                if (fn.first != "destroy" || fn.second->kind() != ast::kind::function_declaration) continue;
                auto fdecl = static_cast<const ast::function_declaration*>(fn.second);
                if (!types::compatible(types::unit(), std::static_pointer_cast<ast::function_type>(fdecl->annotation().type)->result())) continue;
                if (fdecl->generic() || fdecl->parameters().size() != 1) continue;
                auto paramdecl = std::static_pointer_cast<ast::parameter_declaration>(fdecl->parameters().front());
                if (paramdecl->is_variadic()) continue;
                if (!types::assignment_compatible(type, paramdecl->annotation().type)) continue;
                return fdecl;
            }
        }

        return nullptr;
    }

    const ast::function_declaration* checker::is_default_constructible(ast::pointer<ast::type> type) const
    {
        if (!types::builtin(type->string()) && type->declaration()) {
            for (auto fn : scopes_.at(type->declaration())->functions()) {
                if (fn.first != "default" || fn.second->kind() != ast::kind::function_declaration) continue;
                auto fdecl = static_cast<const ast::function_declaration*>(fn.second);
                if (!types::compatible(type, std::static_pointer_cast<ast::function_type>(fdecl->annotation().type)->result())) continue;
                if (fdecl->generic() || !fdecl->parameters().empty()) continue;
                return fdecl;
            }
        }

        return nullptr;
    }

    bool checker::is_string_convertible(ast::pointer<ast::type> type, const ast::property_declaration*& procedure) const
    {
        if (type->declaration()) {
            for (auto fn : scopes_.at(type->declaration())->functions()) {
                if (fn.first != "str" || fn.second->kind() == ast::kind::function_declaration) continue;
                auto fdecl = static_cast<const ast::property_declaration*>(fn.second);
                if (!types::compatible(types::string(), std::static_pointer_cast<ast::function_type>(fdecl->annotation().type)->result()) || fdecl->parameters().size() != 1 || !types::compatible(type, fdecl->parameters().front()->annotation().type)) continue;
                procedure = fdecl;
                return true;
            }
        }
        else switch (type->category()) {
            case ast::type::category::variant_type:
            case ast::type::category::structure_type:
            case ast::type::category::tuple_type:
            case ast::type::category::function_type:
            case ast::type::category::slice_type:
            case ast::type::category::array_type:
            case ast::type::category::behaviour_type:
            case ast::type::category::unknown_type:
            case ast::type::category::workspace_type:
            case ast::type::category::generic_type:
                return false;
            default:
                return true;
        }

        return false;
    }

    void checker::visit(const ast::extend_declaration& decl) 
    {
        if (decl.invalid()) return;

        auto outer = scope_;

        if (pass_ == pass::second) {
            if (decl.generic()) {
                begin_scope(decl.generic().get());
                decl.generic()->accept(*this);
            }

            try {
                decl.type_expression()->accept(*this); 
            } 
            catch (semantic_error&) {
                decl.invalid(true);
            }
            catch (cyclic_symbol_error&) {
                decl.invalid(true);
            }

            if (decl.generic()) end_scope();

            if (!decl.type_expression()->annotation().type || decl.invalid()) {
                decl.invalid(true);
                scope_ = outer;
                return;
            }

            if (types::builtin(decl.type_expression()->annotation().type->string())) {
                decl.invalid(true);
                error(decl.type_expression()->range(), diagnostic::format("Primitive type `$` cannot be extended, sh*t!", decl.type_expression()->annotation().type->string()));
            }

            types::extends(&decl, decl.type_expression()->annotation().type);

            auto typexpr = std::static_pointer_cast<ast::path_type_expression>(decl.type_expression());
            
            if (decl.generic()) {
                if (!typexpr->is_parametric()) {
                    error(decl.generic()->range(), diagnostic::format("You can't use generic parameters when extending a non parametric type like `$`.", typexpr->annotation().type->string()));
                }
                else if (is_partially_specialized(typexpr)) {
                    decl.invalid(true);
                    throw semantic_error();
                }
            }
            
            auto saved = begin_scope(decl.type_expression()->annotation().type->declaration());

            if (auto generics = std::static_pointer_cast<ast::generic_clause_declaration>(decl.generic())) {
                for (auto generic : generics->parameters()) {
                    if (generic->invalid()) continue;
                    scope_->define(generic.get());
                }
            }

            for (ast::pointer<ast::node> stmt : decl.declarations()) try {
                if (auto tdecl = std::dynamic_pointer_cast<ast::type_declaration>(stmt)) {
                    std::string name = tdecl->name().lexeme().string();
                    auto other = scope_->type(name, false);
                    if (workspace()->name == name) {
                        auto diag = diagnostic::builder()
                                    .location(tdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("This type name conflicts with workspace name `$`, c*nt!", name))
                                    .highlight(tdecl->name().range(), "conflicting")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else if (types::builtin(name)) {
                        tdecl->annotation().type = types::unknown();
                        error(tdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your type, idiot!", name));
                    }
                    else if (other && other != tdecl.get()) {
                        tdecl->annotation().type = types::unknown();
                        auto diag = diagnostic::builder()
                                    .location(tdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("You have already declared a type named `$`, idiot!", name))
                                    .highlight(tdecl->name().range(), "conflicting")
                                    .note(other->name().range(), "Here's the homonymous declaration, you f*cker!")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else {
                        scope_->type(name, tdecl.get());
                    }
                }
                else if (auto cdecl = std::dynamic_pointer_cast<ast::const_declaration>(stmt)) {
                    std::string name = cdecl->name().lexeme().string();
                    auto other = scope_->value(name, false);
                    if (workspace()->name == name) {
                        auto diag = diagnostic::builder()
                                    .location(cdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("This constant name conflicts with workspace name `$`, c*nt!", name))
                                    .highlight(cdecl->name().range(), "conflicting")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else if (types::builtin(name)) {
                        cdecl->annotation().type = types::unknown();
                        error(cdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your constant, idiot!", name));
                    }
                    else if (other && other != cdecl.get()) {
                        cdecl->annotation().type = types::unknown();
                        auto builder = diagnostic::builder()
                                    .location(cdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .highlight(cdecl->name().range(), "conflicting");
                        
                        if (auto constant = dynamic_cast<const ast::const_declaration*>(other)) {
                            builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                   .note(constant->name().range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        else if (auto constant = dynamic_cast<const ast::const_tupled_declaration*>(other)) {
                            auto underline = std::find_if(constant->names().begin(), constant->names().end(), [&] (token t) { return t.lexeme().compare(cdecl->name().lexeme()) == 0; });
                            builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        
                        publisher().publish(builder.build());
                    }
                    else {
                        scope_->value(name, cdecl.get());
                    }
                }
                else if (auto cdecl = std::dynamic_pointer_cast<ast::const_tupled_declaration>(stmt)) {
                    for (auto id : cdecl->names()) {
                        std::string name = id.lexeme().string();
                        auto other = scope_->value(name, false);
                        // variable declaration is marked as erraneous by default
                        cdecl->invalid(true);
                        
                        if (workspace()->name == name) {
                            auto diag = diagnostic::builder()
                                        .location(id.location())
                                        .severity(diagnostic::severity::error)
                                        .message(diagnostic::format("This constant name conflicts with workspace name `$`, c*nt!", name))
                                        .highlight(id.range(), "conflicting")
                                        .build();
                            
                            publisher().publish(diag);
                        }
                        else if (types::builtin(name)) {
                            cdecl->annotation().type = types::unknown();
                            error(id.range(), diagnostic::format("You cannot steal primitive type name `$` to create your constant, idiot!", name));
                        }
                        else if (other && other != cdecl.get()) {
                            cdecl->annotation().type = types::unknown();
                            auto builder = diagnostic::builder()
                                        .location(id.location())
                                        .severity(diagnostic::severity::error)
                                        .highlight(id.range(), "conflicting");
                            
                            if (auto constant = dynamic_cast<const ast::const_declaration*>(other)) {
                                builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                    .note(constant->name().range(), "Here's the homonymous declaration, you f*cker!");
                            }
                            else if (auto constant = dynamic_cast<const ast::const_tupled_declaration*>(other)) {
                                auto underline = std::find_if(constant->names().begin(), constant->names().end(), [&] (token t) { return t.lexeme().compare(id.lexeme()) == 0; });
                                builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                    .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                            }
                            
                            publisher().publish(builder.build());
                        }
                        else {
                            // the only case in which our variable will be analyzed because there are not conflicts
                            cdecl->invalid(false);
                            scope_->value(name, cdecl.get());
                        }
                    }        
                }
                else if (auto fdecl = std::dynamic_pointer_cast<ast::function_declaration>(stmt)) {
                    std::string name = fdecl->name().lexeme().string();
                    auto other = scope_->function(name, false);
                    if (workspace()->name == name) {
                        auto diag = diagnostic::builder()
                                    .location(fdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("This function name conflicts with workspace name `$`, c*nt!", name))
                                    .highlight(fdecl->name().range(), "conflicting")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else if (types::builtin(name)) {
                        fdecl->annotation().type = types::unknown();
                        error(fdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your function, idiot!", name));
                    }
                    else if (other && other != fdecl.get()) {
                        fdecl->annotation().type = types::unknown();
                        auto diag = diagnostic::builder()
                                    .location(fdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("You have already declared a function or property named `$`, idiot!", name))
                                    .highlight(fdecl->name().range(), "conflicting")
                                    .note(static_cast<const ast::function_declaration*>(other)->name().range(), "Here's the homonymous declaration, you f*cker!")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else {
                        scope_->function(name, fdecl.get());
                    }
                }
                else if (auto pdecl = std::dynamic_pointer_cast<ast::property_declaration>(stmt)) {
                    std::string name = pdecl->name().lexeme().string();
                    auto other = scope_->function(name, false);
                    if (workspace()->name == name) {
                        auto diag = diagnostic::builder()
                                    .location(pdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("This property name conflicts with workspace name `$`, c*nt!", name))
                                    .highlight(pdecl->name().range(), "conflicting")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else if (types::builtin(name)) {
                        pdecl->annotation().type = types::unknown();
                        error(pdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your property, idiot!", name));
                    }
                    else if (other && other != pdecl.get()) {
                        pdecl->annotation().type = types::unknown();
                        auto diag = diagnostic::builder()
                                    .location(pdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("You have already declared a function or property named `$`, idiot!", name))
                                    .highlight(pdecl->name().range(), "conflicting")
                                    .note(static_cast<const ast::function_declaration*>(other)->name().range(), "Here's the homonymous declaration, you f*cker!")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else {
                        scope_->function(name, pdecl.get());
                    }
                }
            }
            catch (semantic_error& err) { scope_ = saved; }

            // for generic types' declarations
            auto inner = scope_;
            for (auto pair : scope_->types()) try {
                if (pair.second->generic()) try { begin_scope(pair.second->generic().get()); pair.second->generic()->accept(*this); end_scope(); } catch (abort_error&) { throw; } catch (...) { scope_ = inner; }
                scope_ = inner;

                if (auto outclause = std::static_pointer_cast<ast::generic_clause_declaration>(decl.generic())) {
                    if (auto inclause = std::static_pointer_cast<ast::generic_clause_declaration>(pair.second->generic())) {
                        inclause->parameters().insert(inclause->parameters().end(), outclause->parameters().begin(), outclause->parameters().end());
                    }
                    else pair.second->generic() = outclause;
                    types::parametric(pair.second->annotation().type, std::static_pointer_cast<ast::generic_clause_declaration>(pair.second->generic()));
                }
            }
            catch (cyclic_symbol_error& err) {
                // recursive type error is printed
                publisher().publish(err.diagnostic());
                // if we catch an exception then we consider type resolved, at least to minimize errors
                // or false positives in detecting recursive cycles
                pair.second->annotation().resolved = true;
                // restore scope
                scope_ = saved;
            }
            catch (semantic_error& err) {
                // if we catch an exception then we consider type resolved, at least to minimize errors
                // or false positives in detecting recursive cycles
                pair.second->annotation().resolved = true;
                // restore scope
                scope_ = saved;
            }

            if (auto generics = std::static_pointer_cast<ast::generic_clause_declaration>(decl.generic())) {
                for (auto generic : generics->parameters()) {
                    if (generic->invalid()) continue;
                    scope_->remove(generic.get());
                }
            }

            end_scope();
        }
        // evaluation and checking will be performed only if declaration is not generic because checking is delayed
        // to generic instatiation with concrete parameters (?)
        else if (pass_ == pass::third && !decl.generic()) {
            begin_scope(decl.type_expression()->annotation().type->declaration());
            
            auto saved = scope_;

            // all extend types are fully checked
            for (auto pair : scope_->types()) try {
                // if not resolved then the type is traversed
                if (!pair.second->annotation().visited) pair.second->accept(*this);
            }
            catch (cyclic_symbol_error& err) {
                // recursive type error is printed
                publisher().publish(err.diagnostic());
                // if we catch an exception then we consider type resolved, at least to minimize errors
                // or false positives in detecting recursive cycles
                pair.second->annotation().resolved = true;
                // restore scope
                scope_ = saved;
            }
            catch (semantic_error& err) {
                // if we catch an exception then we consider type resolved, at least to minimize errors
                // or false positives in detecting recursive cycles
                pair.second->annotation().resolved = true;
                // restore scope
                scope_ = saved;
            }
            // all extend constants are fully checked
            for (auto pair : scope_->values()) try {
                // if not resolved then the type is traversed
                if (!pair.second->annotation().visited) pair.second->accept(*this);
            }
            catch (cyclic_symbol_error& err) {
                // recursive type error is printed
                publisher().publish(err.diagnostic());
                // if we catch an exception then we consider type resolved, at least to minimize errors
                // or false positives in detecting recursive cycles
                pair.second->annotation().resolved = true;
                // restore scope
                scope_ = saved;
            }
            catch (semantic_error& err) {
                // if we catch an exception then we consider type resolved, at least to minimize errors
                // or false positives in detecting recursive cycles
                pair.second->annotation().resolved = true;
                // restore scope
                scope_ = saved;
            }
            // all extend functions/properties are fully checked
            for (auto pair : scope_->functions()) try {
                // if not resolved then the type is traversed
                if (!pair.second->annotation().visited) pair.second->accept(*this);
            }
            catch (cyclic_symbol_error& err) {
                // recursive type error is printed
                publisher().publish(err.diagnostic());
                // if we catch an exception then we consider type resolved, at least to minimize errors
                // or false positives in detecting recursive cycles
                pair.second->annotation().resolved = true;
                // restore scope
                scope_ = saved;
            }
            catch (semantic_error& err) {
                // if we catch an exception then we consider type resolved, at least to minimize errors
                // or false positives in detecting recursive cycles
                pair.second->annotation().resolved = true;
                // restore scope
                scope_ = saved;
            }

            end_scope();

            for (auto behaviour : decl.behaviours()) {
                behaviour->accept(*this);
                if (behaviour->annotation().type->category() != ast::type::category::behaviour_type) error(behaviour->range(), diagnostic::format("I was expecting behaviour type here, but I found `$`, dammit!", behaviour->annotation().type->string()), "", "expected behaviour");
                auto behaviour_decl = static_cast<const ast::behaviour_declaration*>(behaviour->annotation().type->declaration());
                // implements behaviour
                types::implements(decl.type_expression()->annotation().type, behaviour_decl->annotation().type);
                // count of matched prototypes
                unsigned matches = 0;
                // for each prototype of the behaviour we verify that it is implemented inside the type extension
                for (auto prototype : behaviour_decl->declarations()) {
                    bool found = false;
                    // looking for prototype
                    if (auto function = std::dynamic_pointer_cast<ast::function_declaration>(prototype)) {
                        // for each type to which concept test is applied we look for a function with the expected prototype
                        for (auto test : scopes_.at(decl.type_expression()->annotation().type->declaration())->functions()) {
                            if (test.second->kind() != ast::kind::function_declaration || test.first != function->name().lexeme().string()) continue;
                            auto testfunction = static_cast<const ast::function_declaration*>(test.second);
                            if (testfunction->generic() || testfunction->parameters().size() != function->parameters().size()) continue;
                            // test for type mismatch
                            if (!types::compatible(testfunction->annotation().type, function->annotation().type)) continue;
                            // now we have match for this function
                            ++matches;
                            found = true;
                        }
                        // if prototype was abstract and it is not implemented, then we have an error
                        if (!found && !function->body()) {
                            auto diag = diagnostic::builder()
                                    .location(decl.type_expression()->range().begin())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("Type `$` must implement abstract function `$` when extending behaviour `$`, c*nt!", decl.type_expression()->annotation().type->string(), function->name().lexeme(), behaviour_decl->name().lexeme()))
                                    .highlight(decl.type_expression()->range())
                                    .highlight(behaviour->range(), diagnostic::highlighter::mode::light)
                                    .note(function->name().range(), diagnostic::format("Here's prototype of function `$` for behaviour `$`.", function->name().lexeme(), behaviour_decl->annotation().type->string()))
                                    .build();
                        
                            publisher().publish(diag);
                        }
                        else if (!found) {
                            // inherit original definition of prototype
                            scopes_.at(decl.type_expression()->annotation().type->declaration())->function(function->name().lexeme().string(), function.get());
                        }
                    }
                    else if (auto property = std::dynamic_pointer_cast<ast::property_declaration>(prototype)) {
                        // for each type to which concept test is applied we look for a property with the expected prototype
                        for (auto test : scopes_.at(decl.type_expression()->annotation().type->declaration())->functions()) {
                            if (test.second->kind() != ast::kind::property_declaration || test.first != property->name().lexeme().string()) continue;
                            auto testproperty = static_cast<const ast::property_declaration*>(test.second);
                            if (testproperty->parameters().size() != property->parameters().size()) continue;
                            // test for type mismatch
                            if (!types::compatible(testproperty->annotation().type, property->annotation().type)) continue;
                            // now we have match for this function
                            ++matches;
                            found = true;
                        }
                        // if prototype was abstract and it is not implemented, then we have an error
                        if (!found && !property->body()) {
                            auto diag = diagnostic::builder()
                                    .location(decl.type_expression()->range().begin())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("Type `$` must implement abstract property `$` when extending behaviour `$`, c*nt!", decl.type_expression()->annotation().type->string(), property->name().lexeme(), behaviour_decl->name().lexeme()))
                                    .highlight(decl.type_expression()->range())
                                    .highlight(behaviour->range(), diagnostic::highlighter::mode::light)
                                    .note(property->name().range(), diagnostic::format("Here's prototype of property `$` for behaviour `$`.", property->name().lexeme(), behaviour_decl->annotation().type->string()))
                                    .build();
                        
                            publisher().publish(diag);
                        }
                        else if (!found) {
                            // inherit original definition of prototype
                            scopes_.at(decl.type_expression()->annotation().type->declaration())->function(property->name().lexeme().string(), property.get());
                        }
                    }
                }
            }
        }

        scope_ = outer;
    }

    void checker::visit(const ast::behaviour_declaration& decl)
    {
        if (decl.invalid()) return;

        decl.annotation().visited = true;

        decl.annotation().type = types::behaviour();
        decl.annotation().type->declaration(&decl);

        if (decl.generic()) {
            begin_scope(decl.generic().get());
            decl.generic()->accept(*this);
        }

        begin_scope(&decl);

        auto saved = scope_;

        if (pass_ == pass::second) {
            for (auto prototype : decl.declarations()) try {
                if (auto fdecl = std::dynamic_pointer_cast<ast::function_declaration>(prototype)) {
                    std::string name = fdecl->name().lexeme().string();
                    auto other = scope_->function(name, false);
                    if (workspace()->name == name) {
                        auto diag = diagnostic::builder()
                                    .location(fdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("This function name conflicts with workspace name `$`, c*nt!", name))
                                    .highlight(fdecl->name().range(), "conflicting")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else if (types::builtin(name)) {
                        fdecl->annotation().type = types::unknown();
                        error(fdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your function, idiot!", name));
                    }
                    else if (other && other != fdecl.get()) {
                        fdecl->annotation().type = types::unknown();
                        auto diag = diagnostic::builder()
                                    .location(fdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("You have already declared a function or property named `$`, idiot!", name))
                                    .highlight(fdecl->name().range(), "conflicting")
                                    .note(static_cast<const ast::function_declaration*>(other)->name().range(), "Here's the homonymous declaration, you f*cker!")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else {
                        scope_->function(name, fdecl.get());
                    }
                }
                else if (auto pdecl = std::dynamic_pointer_cast<ast::property_declaration>(prototype)) {
                    std::string name = pdecl->name().lexeme().string();
                    auto other = scope_->function(name, false);
                    if (workspace()->name == name) {
                        auto diag = diagnostic::builder()
                                    .location(pdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("This property name conflicts with workspace name `$`, c*nt!", name))
                                    .highlight(pdecl->name().range(), "conflicting")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else if (types::builtin(name)) {
                        pdecl->annotation().type = types::unknown();
                        error(pdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your property, idiot!", name));
                    }
                    else if (other && other != pdecl.get()) {
                        pdecl->annotation().type = types::unknown();
                        auto diag = diagnostic::builder()
                                    .location(pdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("You have already declared a function or property named `$`, idiot!", name))
                                    .highlight(pdecl->name().range(), "conflicting")
                                    .note(static_cast<const ast::function_declaration*>(other)->name().range(), "Here's the homonymous declaration, you f*cker!")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else {
                        scope_->function(name, pdecl.get());
                    }
                }
            }
            catch (semantic_error& err) { scope_ = saved; }
        }
        else if (pass_ == pass::third) {
            for (auto prototype : scope_->functions()) prototype.second->accept(*this);
        }

        end_scope();

        if (decl.generic()) end_scope();

        scope_->type(decl.name().lexeme().string(), &decl);

        decl.annotation().resolved = true;
        
        if (!decl.generic()) add_type(decl.annotation().type);
    }

    void checker::visit(const ast::extern_declaration& decl)
    {
        for (auto prototype : decl.declarations()) {
            if (pass_ == pass::first) {
                if (auto fdecl = std::dynamic_pointer_cast<ast::function_declaration>(prototype)) {
                    std::string name = fdecl->name().lexeme().string();
                    auto other = scope_->function(name, false);
                    if (workspace()->name == name) {
                        auto diag = diagnostic::builder()
                                    .location(fdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("This function name conflicts with workspace name `$`, c*nt!", name))
                                    .highlight(fdecl->name().range(), "conflicting")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else if (types::builtin(name)) {
                        fdecl->annotation().type = types::unknown();
                        error(fdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your function, idiot!", name));
                    }
                    else if (other && other != fdecl.get()) {
                        fdecl->annotation().type = types::unknown();
                        auto diag = diagnostic::builder()
                                    .location(fdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("You have already declared a function or property named `$`, idiot!", name))
                                    .highlight(fdecl->name().range(), "conflicting")
                                    .note(static_cast<const ast::function_declaration*>(other)->name().range(), "Here's the homonymous declaration, you f*cker!")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else {
                        scope_->function(name, fdecl.get());
                    }

                    if (fdecl->body()) {
                        fdecl->invalid(true);
                        auto diag = diagnostic::builder()
                                .location(fdecl->name().location())
                                .small(true)
                                .severity(diagnostic::severity::error)
                                .message("You can't provide body for external function, idiot!")
                                .highlight(fdecl->name().range(), diagnostic::highlighter::mode::light)
                                .highlight(fdecl->body()->range())
                                .build();
                    
                        publisher().publish(diag);
                    }
                    
                    if (fdecl->generic()) {
                        fdecl->invalid(true);
                        auto diag = diagnostic::builder()
                                .location(fdecl->name().location())
                                .small(true)
                                .severity(diagnostic::severity::error)
                                .message("You can't provide generic parameters for external function, idiot!")
                                .highlight(fdecl->name().range(), diagnostic::highlighter::mode::light)
                                .highlight(fdecl->generic()->range())
                                .build();
                    
                        publisher().publish(diag);
                    }

                    if (fdecl->invalid()) throw semantic_error();

                    fdecl->external = true;
                }
                
                prototype->accept(*this);
                prototype->annotation().resolved = prototype->annotation().visited = true;
            }
        }
    }

    void checker::visit(const ast::range_declaration& decl) 
    {
        if (decl.invalid()) throw semantic_error(); 

        std::string name = decl.name().lexeme().string();
        auto range = std::dynamic_pointer_cast<ast::range_expression>(decl.constraint());
        ast::pointer<ast::type> ltype = nullptr, rtype = nullptr, type = nullptr;
        bool invalid = false;

        decl.annotation().type = types::range(nullptr, true);
        decl.annotation().type->declaration(&decl);

        if (decl.generic()) {
            begin_scope(decl.generic().get());
            decl.generic()->accept(*this);
            types::parametric(decl.annotation().type, std::static_pointer_cast<ast::generic_clause_declaration>(decl.generic()));
        }

        begin_scope(&decl);

        decl.annotation().visited = true;
        
        if (range->start()) {
            range->start()->accept(*this);
            try { range->start()->annotation().value = evaluate(range->start()); } catch (evaluator::generic_evaluation&) {}
            ltype = range->start()->annotation().type;

            switch (ltype->category()) {
                case ast::type::category::integer_type:
                case ast::type::category::rational_type:
                case ast::type::category::float_type:
                case ast::type::category::complex_type:
                case ast::type::category::char_type:
                    break;
                default:
                    report(range->start()->range(), diagnostic::format("You cannot use type `$` in range, you idiot!", ltype->string()));
                    invalid = true;
            }
        }

        if (!range->end()) rtype = ltype;

        if (range->end()) {
            range->end()->accept(*this);
            try { range->end()->annotation().value = evaluate(range->end()); } catch (evaluator::generic_evaluation&) {}
            rtype = range->end()->annotation().type;

            switch (rtype->category()) {
                case ast::type::category::integer_type:
                case ast::type::category::rational_type:
                case ast::type::category::float_type:
                case ast::type::category::complex_type:
                case ast::type::category::char_type:
                    break;
                default:
                    report(range->end()->range(), diagnostic::format("You cannot use type `$` in range, you idiot!", rtype->string()));
                    invalid = true;
            }
        }

        if (!range->start()) ltype = rtype;

        if (!range->start() && !range->end()) {
            ltype = rtype = types::sint(32);
        }
        else if (!invalid && ltype->category() != ast::type::category::unknown_type && rtype->category() != ast::type::category::unknown_type && !types::compatible(ltype, rtype)) {
            mismatch(range->start()->range(), range->end()->range(), diagnostic::format("Range start has type `$` while range end has type `$`, f*cking inconsistent!", ltype->string(), rtype->string()));
        }

        end_scope();

        if (decl.generic()) end_scope();
        
        std::static_pointer_cast<ast::range_type>(decl.annotation().type)->base(ltype);
        std::static_pointer_cast<ast::range_type>(decl.annotation().type)->open(!range->is_inclusive());
        scope_->type(name, &decl);
        decl.annotation().resolved = true;

        if (!decl.generic()) add_type(decl.annotation().type);
    }

    void checker::visit(const ast::record_declaration& decl) 
    {
        std::string name = decl.name().lexeme().string();

        if (!decl.fields().empty() && std::dynamic_pointer_cast<ast::tuple_field_declaration>(decl.fields().front())) decl.annotation().type = types::tuple({});
        else decl.annotation().type =  types::record({});

        decl.annotation().type->declaration(&decl);

        if (decl.generic()) {
            begin_scope(decl.generic().get());
            decl.generic()->accept(*this);
            types::parametric(decl.annotation().type, std::static_pointer_cast<ast::generic_clause_declaration>(decl.generic()));
        }

        begin_scope(&decl);

        decl.annotation().visited = true;

        if (decl.annotation().type->category() == ast::type::category::tuple_type) {
            ast::types components;

            for (auto field_decl : decl.fields()) {
                auto field = std::dynamic_pointer_cast<ast::tuple_field_declaration>(field_decl);
                field->accept(*this);
                components.push_back(field->type_expression()->annotation().type);
            }

            std::static_pointer_cast<ast::tuple_type>(decl.annotation().type)->components(components);
        }
        // record structure
        else {
            std::unordered_map<std::string, token> names;
            ast::structure_type::components fields;

            for (auto field_decl : decl.fields()) {
                auto field = std::dynamic_pointer_cast<ast::field_declaration>(field_decl);
                field->accept(*this);
                auto other = names.find(field->name().lexeme().string());
                if (other != names.end()) {
                    auto diag = diagnostic::builder()
                                .location(field->name().location())
                                .severity(diagnostic::severity::error)
                                .message(diagnostic::format("You have already declared a field named `$`, idiot!", field->name().lexeme()))
                                .highlight(field->name().range(), "conflicting")
                                .note(other->second.range(), "Here's the homonymous field, you f*cker!")
                                .build();
                    
                    publisher().publish(diag);
                }
                else {
                    names.emplace(field->name().lexeme().string(), field->name());
                    fields.emplace_back(field->name().lexeme().string(), field->type_expression()->annotation().type);
                }
            }

            std::static_pointer_cast<ast::structure_type>(decl.annotation().type)->fields(fields);
        }

        end_scope();

        if (decl.generic()) end_scope();
        
        scope_->type(name, &decl);
        decl.annotation().resolved = true;

        if (!decl.generic()) add_type(decl.annotation().type);
    }

    void checker::visit(const ast::variant_declaration& decl)
    {
        std::string name = decl.name().lexeme().string();
        std::vector<std::pair<ast::pointer<ast::type>, source_range>> map;
        ast::types types;

        decl.annotation().type = types::variant({});
        decl.annotation().type->declaration(&decl);

        if (decl.generic()) {
            begin_scope(decl.generic().get());
            decl.generic()->accept(*this);
            types::parametric(decl.annotation().type, std::static_pointer_cast<ast::generic_clause_declaration>(decl.generic()));
        }

        begin_scope(&decl);

        decl.annotation().visited = true;
        
        for (auto t : decl.types()) {
            t->accept(*this);

            auto other = std::find_if(map.begin(), map.end(), [&](const std::pair<ast::pointer<ast::type>, source_range>& p) {
                return types::compatible(p.first, t->annotation().type);
            });
            
            if (other != map.end()) {
                auto diag = diagnostic::builder()
                            .location(t->range().begin())
                            .severity(diagnostic::severity::error)
                            .message(diagnostic::format("You have already included type `$` inside variant `$`, idiot!", t->annotation().type->string(), name))
                            .highlight(t->range(), "conflicting")
                            .note(other->second, "Here's the same type, you f*cker!")
                            .build();
                
                publisher().publish(diag);
            }
            else if (t->annotation().type->category() == ast::type::category::behaviour_type) {
                decl.invalid(true);
                report(t->range(), diagnostic::format("You can't use behaviour `$` as variant type because it cannot be instantiated, c*nt!", t->annotation().type->string()));
            }
            else {
                map.emplace_back(t->annotation().type, t->range());
                types.push_back(t->annotation().type);
            }
        }

        end_scope();

        if (decl.generic()) end_scope();

        std::static_pointer_cast<ast::variant_type>(decl.annotation().type)->types(types);
        scope_->type(name, &decl);
        decl.annotation().resolved = true;
        
        if (!decl.generic()) add_type(decl.annotation().type);
    }

    void checker::visit(const ast::alias_declaration& decl) 
    {
        std::string name = decl.name().lexeme().string();
        
        if (decl.generic()) {
            begin_scope(decl.generic().get());
            decl.generic()->accept(*this);
        }

        decl.annotation().visited = true;
        decl.type_expression()->accept(*this);

        if (decl.generic()) end_scope();

        if (auto t = decl.type_expression()->annotation().type) {
            decl.annotation().type = t;
            scope_->type(name, &decl);
        }

        if (decl.generic()) types::parametric(decl.annotation().type, std::static_pointer_cast<ast::generic_clause_declaration>(decl.generic()));
        
        decl.annotation().resolved = true;
    }

    void checker::visit(const ast::use_declaration& decl) {}

    void checker::visit(const ast::workspace_declaration& decl) {}

    void checker::visit(const ast::source_unit_declaration& decl) 
    {
        // zero pass, workspace are created inside compilation and associated to their correspective packages
        if (pass_ == pass::zero) {
            // two scenarios
            // 1) current file has a workspace declaration
            //    a) if workspace is already declared, then we checks that it resides in the same physical package
            //       i) if so, correct, then we add this source file to the existing workspace
            //       ii) otherwise, violation (workspaces must resides in only one package and cannot be shared among them), so abort
            //    b) workspace does not exists, so we create it, we add this source file and add the workspace to current package
            // 2) current file is anoymous, which means it has no workspace relationship, so an anonymous (random name) workspace is created,
            //    current file is added to the latter and the worskspace is silently added to current package
            if (auto wdecl = std::dynamic_pointer_cast<ast::workspace_declaration>(decl.workspace())) {
                std::string name = wdecl->path().lexeme().string();
                auto result = compilation_.workspaces().find(name);
                // if workspace does not exist, it is created and associated to current package
                if (result == compilation_.workspaces().end()) {
                    auto workspace = ast::create<ast::workspace>(name, package_);
                    workspace->type = types::workspace();
                    workspace->type->declaration(workspace.get());
                    workspace->sources.emplace(file_->name().string(), file_);
                    compilation_.workspaces().emplace(name, workspace);
                    decl.annotation().workspace = workspace.get();
                }
                // otherwise it checks referenced workspace resides in the same package as its workspace declarator
                else if (result->second->package != package_) {
                    report(wdecl->path().range(), diagnostic::format("How dare you extend workspace `$` outside its package `$`?! You just can't, idiot.", name, result->second->package));
                    // such a violation is punished by skipping the whole file from analysis and aborting analysis
                    result->second->sources.erase(file_->name().string());
                    compilation_.get_source_handler().remove(file_->name());
                    throw abort_error();
                }
                // workspace exists and resides in this package, correct
                else {
                    result->second->sources.emplace(file_->name().string(), file_);
                    decl.annotation().workspace = result->second.get();
                }
            }
            // workspace is anonymous, so its definitions cannot be exported, so it is silently added to current package
            else {
                auto name = "__" + std::to_string(std::hash<utf8::span>()(file_->name()));
                auto workspace = ast::create<ast::workspace>(name, package_);
                workspace->type = types::workspace();
                workspace->type->declaration(workspace.get());
                workspace->sources.emplace(file_->name().string(), file_);
                compilation_.workspaces().emplace(name, workspace);
                decl.annotation().workspace = workspace.get();
            }
        }
        // first pass is useful to register declarations' names for forward definitions, so we can
        // already find conflicts in each workspace since top-level declarations are pushed into their
        // correspective workspace, declarations' names are
        // i) types
        // ii) constants
        // iii) variables
        // iv) functions (even extern functions)
        else if (pass_ == pass::first) {
            for (ast::pointer<ast::node> stmt : decl.statements()) {
                if (auto tdecl = std::dynamic_pointer_cast<ast::type_declaration>(stmt)) try {
                    std::string name = tdecl->name().lexeme().string();
                    auto other = scope_->type(name);
                    if (workspace()->name == name) {
                        auto diag = diagnostic::builder()
                                    .location(tdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("This type name conflicts with workspace name `$`, c*nt!", name))
                                    .highlight(tdecl->name().range(), "conflicting")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else if (types::builtin(name)) {
                        tdecl->annotation().type = types::unknown();
                        error(tdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your type, idiot!", name));
                    }
                    else if (other && other != tdecl.get()) {
                        tdecl->annotation().type = types::unknown();
                        auto diag = diagnostic::builder()
                                    .location(tdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("You have already declared a type named `$`, idiot!", name))
                                    .highlight(tdecl->name().range(), "conflicting")
                                    .note(other->name().range(), "Here's the homonymous declaration, you f*cker!")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else {
                       scope_->type(name, tdecl.get());
                    }
                }
                catch (semantic_error& err) {}
                else if (auto cdecl = std::dynamic_pointer_cast<ast::concept_declaration>(stmt)) try {
                    std::string name = cdecl->name().lexeme().string();
                    auto other = scope_->concept(name);
                    if (workspace()->name == name) {
                        auto diag = diagnostic::builder()
                                    .location(tdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("This concept name conflicts with workspace name `$`, c*nt!", name))
                                    .highlight(cdecl->name().range(), "conflicting")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else if (types::builtin(name)) {
                        error(tdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your concept, idiot!", name));
                    }
                    else if (other && other != cdecl.get()) {
                        auto diag = diagnostic::builder()
                                    .location(cdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("You have already declared a concept named `$`, idiot!", name))
                                    .highlight(cdecl->name().range(), "conflicting")
                                    .note(other->name().range(), "Here's the homonymous declaration, you f*cker!")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else {
                       scope_->concept(name, cdecl.get());
                    }
                }
                catch (semantic_error& err) {}
                else if (auto cdecl = std::dynamic_pointer_cast<ast::const_declaration>(stmt)) try {
                    std::string name = cdecl->name().lexeme().string();
                    auto other = scope_->value(name);
                    if (workspace()->name == name) {
                        auto diag = diagnostic::builder()
                                    .location(cdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("This constant name conflicts with workspace name `$`, c*nt!", name))
                                    .highlight(cdecl->name().range(), "conflicting")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else if (types::builtin(name)) {
                        cdecl->annotation().type = types::unknown();
                        error(cdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your constant, idiot!", name));
                    }
                    else if (other && other != cdecl.get()) {
                        cdecl->annotation().type = types::unknown();
                        auto builder = diagnostic::builder()
                                    .location(cdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .highlight(cdecl->name().range(), "conflicting");
                        
                        if (auto constant = dynamic_cast<const ast::const_declaration*>(other)) {
                            builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                   .note(constant->name().range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        else if (auto constant = dynamic_cast<const ast::const_tupled_declaration*>(other)) {
                            auto underline = std::find_if(constant->names().begin(), constant->names().end(), [&] (token t) { return t.lexeme().compare(cdecl->name().lexeme()) == 0; });
                            builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        else if (auto variable = dynamic_cast<const ast::var_declaration*>(other)) {
                            builder.message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                                   .note(variable->name().range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        else if (auto variable = dynamic_cast<const ast::var_tupled_declaration*>(other)) {
                            auto underline = std::find_if(variable->names().begin(), variable->names().end(), [&] (token t) { return t.lexeme().compare(cdecl->name().lexeme()) == 0; });
                            builder.message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                                .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        
                        publisher().publish(builder.build());
                    }
                    else {
                        scope_->value(name, cdecl.get());
                    }
                }
                catch (semantic_error& err) {}
                else if (auto cdecl = std::dynamic_pointer_cast<ast::const_tupled_declaration>(stmt)) try {
                    for (auto id : cdecl->names()) {
                        std::string name = id.lexeme().string();
                        auto other = scope_->value(name);
                        // variable declaration is marked as erraneous by default
                        cdecl->invalid(true);
                        
                        if (workspace()->name == name) {
                            auto diag = diagnostic::builder()
                                        .location(id.location())
                                        .severity(diagnostic::severity::error)
                                        .message(diagnostic::format("This constant name conflicts with workspace name `$`, c*nt!", name))
                                        .highlight(id.range(), "conflicting")
                                        .build();
                            
                            publisher().publish(diag);
                        }
                        else if (types::builtin(name)) {
                            cdecl->annotation().type = types::unknown();
                            error(id.range(), diagnostic::format("You cannot steal primitive type name `$` to create your constant, idiot!", name));
                        }
                        else if (other && other != cdecl.get()) {
                            cdecl->annotation().type = types::unknown();
                            auto builder = diagnostic::builder()
                                        .location(id.location())
                                        .severity(diagnostic::severity::error)
                                        .highlight(id.range(), "conflicting");
                            
                            if (auto constant = dynamic_cast<const ast::const_declaration*>(other)) {
                                builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                    .note(constant->name().range(), "Here's the homonymous declaration, you f*cker!");
                            }
                            else if (auto constant = dynamic_cast<const ast::const_tupled_declaration*>(other)) {
                                auto underline = std::find_if(constant->names().begin(), constant->names().end(), [&] (token t) { return t.lexeme().compare(id.lexeme()) == 0; });
                                builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                    .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                            }
                            else if (auto variable = dynamic_cast<const ast::var_declaration*>(other)) {
                                builder.message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                                    .note(variable->name().range(), "Here's the homonymous declaration, you f*cker!");
                            }
                            else if (auto variable = dynamic_cast<const ast::var_tupled_declaration*>(other)) {
                                auto underline = std::find_if(variable->names().begin(), variable->names().end(), [&] (token t) { return t.lexeme().compare(id.lexeme()) == 0; });
                                builder.message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                                    .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                            }
                            
                            publisher().publish(builder.build());
                        }
                        else {
                            // the only case in which our variable will be analyzed because there are not conflicts
                            cdecl->invalid(false);
                            scope_->value(name, cdecl.get());
                        }
                    }        
                }
                catch (semantic_error& err) {}
                else if (auto vdecl = std::dynamic_pointer_cast<ast::var_declaration>(stmt)) try {
                    std::string name = vdecl->name().lexeme().string();
                    auto other = scope_->value(name);
                    // variable declaration is marked as erraneous by default
                    vdecl->invalid(true);
                    
                    if (workspace()->name == name) {
                        auto diag = diagnostic::builder()
                                    .location(vdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("This variable name conflicts with workspace name `$`, c*nt!", name))
                                    .highlight(vdecl->name().range(), "conflicting")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else if (types::builtin(name)) {
                        vdecl->annotation().type = types::unknown();
                        error(vdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your variable, idiot!", name));
                    }
                    else if (other && other != vdecl.get()) {
                        vdecl->annotation().type = types::unknown();
                        auto builder = diagnostic::builder()
                                    .location(vdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .highlight(vdecl->name().range(), "conflicting");
                        
                        if (auto constant = dynamic_cast<const ast::const_declaration*>(other)) {
                            builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                   .note(constant->name().range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        else if (auto constant = dynamic_cast<const ast::const_tupled_declaration*>(other)) {
                            auto underline = std::find_if(constant->names().begin(), constant->names().end(), [&] (token t) { return t.lexeme().compare(vdecl->name().lexeme()) == 0; });
                            builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        else if (auto variable = dynamic_cast<const ast::var_declaration*>(other)) {
                            builder.message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                                   .note(variable->name().range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        else if (auto variable = dynamic_cast<const ast::var_tupled_declaration*>(other)) {
                            auto underline = std::find_if(variable->names().begin(), variable->names().end(), [&] (token t) { return t.lexeme().compare(vdecl->name().lexeme()) == 0; });
                            builder.message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                                .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                        }
                        
                        publisher().publish(builder.build());
                    }
                    else {
                        // the only case in which our variable will be analyzed because there are not conflicts
                        vdecl->invalid(false);
                        scope_->value(name, vdecl.get());
                    }
                }
                catch (semantic_error& err) {}
                else if (auto vdecl = std::dynamic_pointer_cast<ast::var_tupled_declaration>(stmt)) try {
                    for (auto id : vdecl->names()) {
                        std::string name = id.lexeme().string();
                        auto other = scope_->value(name);
                        // variable declaration is marked as erraneous by default
                        vdecl->invalid(true);
                        
                        if (workspace()->name == name) {
                            auto diag = diagnostic::builder()
                                        .location(id.location())
                                        .severity(diagnostic::severity::error)
                                        .message(diagnostic::format("This variable name conflicts with workspace name `$`, c*nt!", name))
                                        .highlight(id.range(), "conflicting")
                                        .build();
                            
                            publisher().publish(diag);
                        }
                        else if (types::builtin(name)) {
                            vdecl->annotation().type = types::unknown();
                            error(id.range(), diagnostic::format("You cannot steal primitive type name `$` to create your variable, idiot!", name));
                        }
                        else if (other && other != vdecl.get()) {
                            vdecl->annotation().type = types::unknown();
                            auto builder = diagnostic::builder()
                                        .location(id.location())
                                        .severity(diagnostic::severity::error)
                                        .highlight(id.range(), "conflicting");
                            
                            if (auto constant = dynamic_cast<const ast::const_declaration*>(other)) {
                                builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                    .note(constant->name().range(), "Here's the homonymous declaration, you f*cker!");
                            }
                            else if (auto constant = dynamic_cast<const ast::const_tupled_declaration*>(other)) {
                                auto underline = std::find_if(constant->names().begin(), constant->names().end(), [&] (token t) { return t.lexeme().compare(id.lexeme()) == 0; });
                                builder.message(diagnostic::format("You have already declared a constant named `$`, idiot!", name))
                                    .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                            }
                            else if (auto variable = dynamic_cast<const ast::var_declaration*>(other)) {
                                builder.message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                                    .note(variable->name().range(), "Here's the homonymous declaration, you f*cker!");
                            }
                            else if (auto variable = dynamic_cast<const ast::var_tupled_declaration*>(other)) {
                                auto underline = std::find_if(variable->names().begin(), variable->names().end(), [&] (token t) { return t.lexeme().compare(id.lexeme()) == 0; });
                                builder.message(diagnostic::format("You have already declared a variable named `$`, idiot!", name))
                                    .note(underline->range(), "Here's the homonymous declaration, you f*cker!");
                            }
                            
                            publisher().publish(builder.build());
                        }
                        else {
                            // the only case in which our variable will be analyzed because there are not conflicts
                            vdecl->invalid(false);
                            scope_->value(name, vdecl.get());
                        }
                    }        
                }
                catch (semantic_error& err) {}
                else if (auto fdecl = std::dynamic_pointer_cast<ast::function_declaration>(stmt)) try {
                    std::string name = fdecl->name().lexeme().string();
                    auto other = scope_->function(name);
                    if (workspace()->name == name) {
                        auto diag = diagnostic::builder()
                                    .location(fdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("This function name conflicts with workspace name `$`, c*nt!", name))
                                    .highlight(fdecl->name().range(), "conflicting")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else if (types::builtin(name)) {
                        fdecl->annotation().type = types::unknown();
                        error(fdecl->name().range(), diagnostic::format("You cannot steal primitive type name `$` to create your function, idiot!", name));
                    }
                    else if (other && other != fdecl.get()) {
                        fdecl->annotation().type = types::unknown();
                        auto diag = diagnostic::builder()
                                    .location(fdecl->name().location())
                                    .severity(diagnostic::severity::error)
                                    .message(diagnostic::format("You have already declared a function named `$`, idiot!", name))
                                    .highlight(fdecl->name().range(), "conflicting")
                                    .note(static_cast<const ast::function_declaration*>(other)->name().range(), "Here's the homonymous declaration, you f*cker!")
                                    .build();
                        
                        publisher().publish(diag);
                    }
                    else {
                        scope_->function(name, fdecl.get());

                        if (name == "start") {
                            if (entry_point_) {
                                auto diag = diagnostic::builder()
                                        .location(fdecl->name().location())
                                        .severity(diagnostic::severity::error)
                                        .message("You have already declared the entry point for this program, idiot!")
                                        .highlight(fdecl->name().range(), "conflicting")
                                        .note(entry_point_->name().range(), "Here's the other entry point, you f*cker!")
                                        .build();
                        
                                publisher().publish(diag);
                            }
                            else entry_point_ = fdecl.get();
                        }
                    }
                }
                catch (semantic_error& err) {}
                else if (auto ext = std::dynamic_pointer_cast<ast::extern_declaration>(stmt)) try {
                    ext->accept(*this);
                }
                catch (semantic_error& err) {}
            }
        }
        // second pass, type extension are traversed to register nested names for types, costants, functions
        // extended types are fully resolved in other to add nested declarations to their namespaces and even inside concepts
        else if (pass_ == pass::second) {
            auto saved = scope_;
            // first generic extension are traversed
            for (ast::pointer<ast::node> stmt : decl.statements()) {
                if (auto extdecl = std::dynamic_pointer_cast<ast::extend_declaration>(stmt)) try {
                    if (extdecl->generic()) extdecl->accept(*this); 
                } 
                catch (semantic_error& err) { scope_ = saved; }
                else if (auto cdecl = std::dynamic_pointer_cast<ast::concept_declaration>(stmt)) try {
                    if (cdecl->generic()) cdecl->accept(*this); 
                } 
                catch (semantic_error& err) { scope_ = saved; }
                else if (auto bdecl = std::dynamic_pointer_cast<ast::behaviour_declaration>(stmt)) try {
                    bdecl->accept(*this);
                }
                catch (semantic_error& err) { scope_ = saved; }
            }
            // then specialized generics
            for (ast::pointer<ast::node> stmt : decl.statements()) {
                if (auto extdecl = std::dynamic_pointer_cast<ast::extend_declaration>(stmt)) try {
                    if (!extdecl->generic()) extdecl->accept(*this); 
                } 
                catch (semantic_error& err) { scope_ = saved; }
            }
        }
        // third pass is used to construct nested types, constants, functions and properties inside extend blocks
        // and types, properties inside concepts, functions inside behaviours
        else if (pass_ == pass::third) {
            auto saved = scope_;
            // first generic extension are traversed
            for (ast::pointer<ast::node> stmt : decl.statements()) {
                if (auto extdecl = std::dynamic_pointer_cast<ast::extend_declaration>(stmt)) try {
                    if (extdecl->generic()) extdecl->accept(*this); 
                } 
                catch (semantic_error& err) { scope_ = saved; }
                else if (auto cdecl = std::dynamic_pointer_cast<ast::concept_declaration>(stmt)) try {
                    if (cdecl->generic()) cdecl->accept(*this); 
                } 
                catch (semantic_error& err) { scope_ = saved; }
                else if (auto bdecl = std::dynamic_pointer_cast<ast::behaviour_declaration>(stmt)) try {
                    bdecl->accept(*this);
                }
                catch (semantic_error& err) { scope_ = saved; }
            }
            // then specialized generics
            for (ast::pointer<ast::node> stmt : decl.statements()) {
                if (auto extdecl = std::dynamic_pointer_cast<ast::extend_declaration>(stmt)) try {
                    if (!extdecl->generic()) extdecl->accept(*this); 
                } 
                catch (semantic_error& err) { scope_ = saved; }
            }
        }
        // fourth and last pass, used to fully check remaining statements, which are
        // i) variables
        // ii) constants
        // iii) functions
        // iv) tests 
        else {
            auto saved = scope_;
            for (ast::pointer<ast::statement> stmt : decl.statements()) {
                // current statement
                auto prev = statement_;
                statement_ = stmt.get();
                if (std::dynamic_pointer_cast<ast::test_declaration>(stmt) || 
                    std::dynamic_pointer_cast<ast::function_declaration>(stmt) ||
                    std::dynamic_pointer_cast<ast::var_declaration>(stmt) ||
                    std::dynamic_pointer_cast<ast::var_tupled_declaration>(stmt)) try {
                    if (!stmt->invalid() && !std::static_pointer_cast<ast::declaration>(stmt)->annotation().resolved) stmt->accept(*this); 
                }
                catch (semantic_error& err) { scope_ = saved; }
                // reset current statement
                statement_ = prev;
            }

            // adds all pending and artificial insertion inside the ast because before it would have invalidate the tree (std::vector<ast::statement>)
            while (!pending_insertions.empty()) {
                add_to_scope(std::get<0>(pending_insertions.front()), std::get<1>(pending_insertions.front()), std::get<2>(pending_insertions.front()), std::get<3>(pending_insertions.front()));
                pending_insertions.pop_front();
            }
        }
    }

    void checker::check() try {
        // pass zero
        pass_ = pass::zero;
        // each source file is associated to a workspace, and each workspace (which are namespaces)
        // is associated to only one package
        // it would be good pratice that one package contains only one workspace and their name corresponds
        for (auto dependency : compilation_.dependencies()) {
            // sets current package
            package_ = dependency.name;
            // traverse each source file for each package
            for (auto source : dependency.sources) {
                // sets current source file
                file_ = source;
                source->ast()->accept(*this);
            }
        }
        // current package as last package to check
        package_ = compilation_.current().name;
        // traverse each source file of current package
        for (auto source : compilation_.current().sources) {
            // sets current source file
            file_ = source;
            source->ast()->accept(*this);
        }
        // first pass
        pass_ = pass::first;
        // all `use` directives are analyzed importing symbols from one workspace to another
        // this is possible between a package and another when one its a dependency of the other,
        // so all public definitions may be exported in the other workspace
        do_imports();
        // for each workspace are registered names of
        // i) types
        // ii) variables
        // iii) constants
        // iv) functions (even extern functions)
        for (auto workspace : compilation_.workspaces()) {
            // workspace scope
            struct scope scope(this, workspace.second.get());
            // sets current package
            package_ = workspace.second->package;
            // each source file belonging to a workspace is traversed
            for (auto pair : workspace.second->sources) {
                // sets current file
                file_ = pair.second;
                file_->ast()->accept(*this);
            }
        }
        // second pass
        pass_ = pass::second;
        // for each workspace, type extension block are visited, so
        // i) extended type are resolved
        // ii) associated types and constants' names are registered
        for (auto workspace : compilation_.workspaces()) {
            // workspace scope
            struct scope scope(this, workspace.second.get());
            // sets current package
            package_ = workspace.second->package;
            // each source file belonging to a workspace is traversed
            for (auto pair : workspace.second->sources) {
                // sets current file
                file_ = pair.second;
                file_->ast()->accept(*this);
            }
        }
        // for simplicity all name definitions from `core` library don't need to be explicitly imported and referenced with `core` for all other workspaces
        import_core_library_in_workspaces();
        // third pass
        pass_ = pass::third;
        // for each workspace remaining types are fully constructed
        for (auto workspace : compilation_.workspaces()) {
            // workspace scope
            struct scope scope(this, workspace.second.get());
            // sets current package
            package_ = workspace.second->package;
            // save current scope for restoring
            auto saved = scope_;
            // all workspace types are fully checked
            for (auto pair : scope_->types()) try {
                // if not resolved then the type is traversed
                if (!pair.second->annotation().visited && workspace.second.get() == scopes_.at(pair.second->annotation().scope)->outscope(environment::kind::workspace)) pair.second->accept(*this);
            }
            catch (cyclic_symbol_error& err) {
                // recursive type error is printed
                publisher().publish(err.diagnostic());
                // if we catch an exception then we consider type resolved, at least to minimize errors
                // or false positives in detecting recursive cycles
                pair.second->annotation().resolved = true;
                // restore scope
                scope_ = saved;
            }
            catch (semantic_error& err) {
                // if we catch an exception then we consider type resolved, at least to minimize errors
                // or false positives in detecting recursive cycles
                pair.second->annotation().resolved = true;
                // restore scope
                scope_ = saved;
            }
            // all workspace constants are fully checked
            for (auto pair : scope_->values()) try {
                // variables are analyzed later, after resolution of constants
                if (pair.second->kind() == ast::kind::var_declaration || pair.second->kind() == ast::kind::var_tupled_declaration) continue;
                // if not resolved then the constant is traversed
                if (!pair.second->annotation().visited && workspace.second.get() == scopes_.at(pair.second->annotation().scope)->outscope(environment::kind::workspace)) pair.second->accept(*this);
            }
            catch (cyclic_symbol_error& err) {
                // recursive constant error is printed
                publisher().publish(err.diagnostic());
                // if we catch an exception then we consider type resolved, at least to minimize errors
                // or false positives in detecting recursive cycles
                pair.second->annotation().resolved = true;
                // restore scope
                scope_ = saved;
            }
            catch (semantic_error& err) {
                // if we catch an exception then we consider constant resolved, at least to minimize errors
                // or false positives in detecting recursive cycles
                pair.second->annotation().resolved = true;
                // restore scope
                scope_ = saved;
            }
            // all workspace concepts are fully checked
            for (auto pair : scope_->concepts()) try {
                // if not resolved then the type is traversed
                if (!pair.second->annotation().visited && workspace.second.get() == scopes_.at(pair.second->annotation().scope)->outscope(environment::kind::workspace)) pair.second->accept(*this);
            }
            catch (cyclic_symbol_error& err) {
                // recursive concept error is printed
                publisher().publish(err.diagnostic());
                // if we catch an exception then we consider concept resolved, at least to minimize errors
                // or false positives in detecting recursive cycles
                pair.second->annotation().resolved = true;
                // restore scope
                scope_ = saved;
            }
            catch (semantic_error& err) {
                // if we catch an exception then we consider concept resolved, at least to minimize errors
                // or false positives in detecting recursive cycles
                pair.second->annotation().resolved = true;
                // restore scope
                scope_ = saved;
            }
        }
        // type extension block in each workspace are fully traversed to construct all previously registered constants and types' names
        for (auto workspace : compilation_.workspaces()) {
            // sets current package
            package_ = workspace.second->package;
            // workspace scope
            struct scope scope(this, workspace.second.get());
            // for each source file belonging to the current workspace
            for (auto pair : workspace.second->sources) {
                // sets current file
                file_ = pair.second;
                pair.second->ast()->accept(*this);
            }   
        }
        // for each workspace remove variables names to avoid conflicts
        for (auto workspace : compilation_.workspaces()) {
            // sets current package
            package_ = workspace.second->package;
            // workspace scope
            struct scope scope(this, workspace.second.get());
            // remove all variables from scope
            std::set<std::string> vars_to_remove;
            for (auto pair : scope_->values()) if (dynamic_cast<const ast::var_declaration*>(pair.second) || dynamic_cast<const ast::var_tupled_declaration*>(pair.second)) vars_to_remove.insert(pair.first);
            for (auto var : vars_to_remove) scope_->values().erase(var);
        }
        // fourth and last pass, used to fully check remaining statements, which are
        // i) variables
        // ii) constants
        // iii) functions
        // iv) tests 
        pass_ = pass::fourth;
        // for each workspace it fully checks remaining statements
        for (auto workspace : compilation_.workspaces()) {
            // sets current package
            package_ = workspace.second->package;
            // workspace scope
            struct scope scope(this, workspace.second.get());
            // for each source file belonging to the current workspace
            for (auto pair : workspace.second->sources) {
                // sets current file
                file_ = pair.second;
                pair.second->ast()->accept(*this);
            }   
        }
    }
    catch (abort_error&) {}

    void substitutions::substitute() { root_->accept(*this); }

    void substitutions::visit(const ast::bit_field_type_expression& expr) {}

    void substitutions::visit(const ast::path_type_expression& expr)
    {
        if (!expr.member()) expr.expression()->accept(*this);
    }
    
    void substitutions::visit(const ast::array_type_expression& expr) 
    { 
        expr.element_type()->accept(*this);
        if (expr.size()) expr.size()->accept(*this);
    }
    
    void substitutions::visit(const ast::tuple_type_expression& expr) { for (auto t : expr.types()) t->accept(*this); }
    
    void substitutions::visit(const ast::pointer_type_expression& expr) 
    { 
        expr.pointee_type()->accept(*this); 
    }
    
    void substitutions::visit(const ast::function_type_expression& expr) 
    {
        for (auto param : expr.parameter_types()) param->accept(*this);
        if (expr.return_type_expression()) expr.return_type_expression()->accept(*this);
    }

    void substitutions::visit(const ast::variant_type_expression& expr) { for (auto typexpr : expr.types()) typexpr->accept(*this); }

    void substitutions::visit(const ast::record_type_expression& expr) { for (auto decl : expr.fields()) decl->accept(*this); }

    void substitutions::visit(const ast::literal_expression& expr) {}

    void substitutions::visit(const ast::identifier_expression& expr) 
    {
        for (auto arg : expr.generics()) arg->accept(*this);

        auto result = expr.annotation().referencing;
        if (!result) result = context_->value(expr.identifier().lexeme().string());
        if (!result) result = context_->type(expr.identifier().lexeme().string());
        if (!result) return;

        auto maybe_const = constant(result);
        auto maybe_type = type(result);

        if (maybe_const != constants().end()) {
            expr.annotation().istype = false;
            expr.annotation().value = maybe_const->second;
            expr.annotation().type = maybe_const->second.type;
            expr.annotation().referencing = maybe_const->first;
            std::cout << "generic const " << expr.identifier() << " has value " << expr.annotation().value << '\n';
        }
        else if (maybe_type != types().end()) {
            expr.annotation().istype = true;
            expr.annotation().type = maybe_type->second;
            expr.annotation().referencing = maybe_type->second->declaration();
            expr.identifier() = token(token::kind::identifier, utf8::span::builder().concat(maybe_type->second->string().data()).build(), expr.identifier().location());
        }
    }
        
    void substitutions::visit(const ast::tuple_expression& expr) { for (auto elem : expr.elements()) elem->accept(*this); }
        
    void substitutions::visit(const ast::array_expression& expr) { for (auto elem : expr.elements()) elem->accept(*this); }
        
    void substitutions::visit(const ast::array_sized_expression& expr) 
    {
        expr.value()->accept(*this);
        expr.size()->accept(*this);
    }
        
    void substitutions::visit(const ast::parenthesis_expression& expr) { expr.expression()->accept(*this); }
        
    void substitutions::visit(const ast::block_expression& expr) { for (auto stmt : expr.statements()) stmt->accept(*this); }
        
    void substitutions::visit(const ast::function_expression& expr) 
    {
        if (expr.return_type_expression()) expr.return_type_expression()->accept(*this);
        
        for (auto param : expr.parameters()) param->accept(*this);

        expr.body()->accept(*this);
    }
        
    void substitutions::visit(const ast::postfix_expression& expr) { expr.expression()->accept(*this); }
        
    void substitutions::visit(const ast::call_expression& expr) 
    { 
        expr.callee()->accept(*this);
        for (auto arg : expr.arguments()) arg->accept(*this);
    }
        
    void substitutions::visit(const ast::member_expression& expr) 
    {
        expr.expression()->accept(*this);
        expr.member()->accept(*this);
    }
        
    void substitutions::visit(const ast::array_index_expression& expr) 
    {
        expr.expression()->accept(*this);
        expr.index()->accept(*this);
    }
        
    void substitutions::visit(const ast::tuple_index_expression& expr) { expr.expression()->accept(*this); }
        
    void substitutions::visit(const ast::record_expression& expr)
    {
        if (expr.callee()) expr.callee()->accept(*this);
        for (auto init : expr.initializers()) init.value()->accept(*this);
    }
        
    void substitutions::visit(const ast::unary_expression& expr) { expr.expression()->accept(*this); }
        
    void substitutions::visit(const ast::binary_expression& expr) 
    {
        expr.left()->accept(*this);
        expr.right()->accept(*this);
    }
        
    void substitutions::visit(const ast::range_expression& expr) 
    {
        if (expr.start()) expr.start()->accept(*this);
        if (expr.end()) expr.end()->accept(*this);
    }
        
    void substitutions::visit(const ast::ignore_pattern_expression& expr) {}
        
    void substitutions::visit(const ast::literal_pattern_expression& expr) {}
        
    void substitutions::visit(const ast::path_pattern_expression& expr) { expr.path()->accept(*this); }
        
    void substitutions::visit(const ast::tuple_pattern_expression& expr) { for (auto elem : expr.elements()) elem->accept(*this); }
        
    void substitutions::visit(const ast::array_pattern_expression& expr) { for (auto elem : expr.elements()) elem->accept(*this); }
        
    void substitutions::visit(const ast::record_pattern_expression& expr) 
    { 
        expr.path()->accept(*this);
        for (auto field : expr.fields()) field->accept(*this);
    }

    void substitutions::visit(const ast::labeled_record_pattern_expression& expr) 
    { 
        expr.path()->accept(*this);
        for (auto field : expr.fields()) field.value->accept(*this);
    }
        
    void substitutions::visit(const ast::range_pattern_expression& expr) 
    {
        if (expr.start()) expr.start()->accept(*this);
        if (expr.end()) expr.end()->accept(*this);
    }
        
    void substitutions::visit(const ast::or_pattern_expression& expr) 
    {
        expr.left()->accept(*this);
        expr.right()->accept(*this);
    }
        
    void substitutions::visit(const ast::when_expression& expr) 
    {
        expr.condition()->accept(*this);

        for (auto branch : expr.branches()) {
            branch.pattern()->accept(*this);
            branch.body()->accept(*this);
        }

        if (expr.else_body()) expr.else_body()->accept(*this);
    }
        
    void substitutions::visit(const ast::when_pattern_expression& expr) 
    {
        expr.condition()->accept(*this);
        expr.pattern()->accept(*this);
        expr.body()->accept(*this);
        
        if (expr.else_body()) expr.else_body()->accept(*this);
    }
        
    void substitutions::visit(const ast::when_cast_expression& expr) 
    {
        expr.condition()->accept(*this);
        expr.type_expression()->accept(*this);
        expr.body()->accept(*this);
        
        if (expr.else_body()) expr.else_body()->accept(*this);
    }
        
    void substitutions::visit(const ast::for_range_expression& expr) 
    {
        for (auto contract : expr.contracts()) contract->accept(*this);

        expr.variable()->accept(*this);
        expr.condition()->accept(*this);
        expr.body()->accept(*this);
        
        if (expr.else_body()) expr.else_body()->accept(*this);
    }
        
    void substitutions::visit(const ast::for_loop_expression& expr) 
    {
        for (auto contract : expr.contracts()) contract->accept(*this);

        expr.condition()->accept(*this);
        expr.body()->accept(*this);
        
        if (expr.else_body()) expr.else_body()->accept(*this);
    }
        
    void substitutions::visit(const ast::if_expression& expr) 
    {
        expr.condition()->accept(*this);
        expr.body()->accept(*this);
        
        if (expr.else_body()) expr.else_body()->accept(*this);
    }

    void substitutions::visit(const ast::null_statement& stmt) {}

    void substitutions::visit(const ast::expression_statement& stmt) { stmt.expression()->accept(*this); }

    void substitutions::visit(const ast::assignment_statement& stmt) 
    {
        stmt.left()->accept(*this);
        stmt.right()->accept(*this);
    }

    void substitutions::visit(const ast::return_statement& stmt) { if (stmt.expression()) stmt.expression()->accept(*this); }

    void substitutions::visit(const ast::break_statement& stmt) { if (stmt.expression()) stmt.expression()->accept(*this); }
    
    void substitutions::visit(const ast::continue_statement& stmt) {}
    
    void substitutions::visit(const ast::contract_statement& stmt) { stmt.condition()->accept(*this); }
    
    void substitutions::visit(const ast::field_declaration& decl) { decl.type_expression()->accept(*this); }

    void substitutions::visit(const ast::tuple_field_declaration& decl) { decl.type_expression()->accept(*this); }

    void substitutions::visit(const ast::parameter_declaration& decl) { decl.type_expression()->accept(*this); }
    
    void substitutions::visit(const ast::var_declaration& decl) 
    {
        if (decl.type_expression()) decl.type_expression()->accept(*this);
        if (decl.value()) decl.value()->accept(*this);
    }
    
    void substitutions::visit(const ast::var_tupled_declaration& decl)
    {
        if (decl.type_expression()) decl.type_expression()->accept(*this);
        decl.value()->accept(*this);
    }
    
    void substitutions::visit(const ast::const_declaration& decl)
    {
        if (decl.type_expression()) decl.type_expression()->accept(*this);
        decl.value()->accept(*this);
    }
    
    void substitutions::visit(const ast::const_tupled_declaration& decl)
    {
        if (decl.type_expression()) decl.type_expression()->accept(*this);
        decl.value()->accept(*this);
    }
    
    void substitutions::visit(const ast::generic_clause_declaration& decl)
    {
        if (decl.constraint()) decl.constraint()->accept(*this);
    }
    
    void substitutions::visit(const ast::generic_const_parameter_declaration& decl) {}
    
    void substitutions::visit(const ast::generic_type_parameter_declaration& decl) {}
    
    void substitutions::visit(const ast::test_declaration& decl) { decl.body()->accept(*this); }
    
    void substitutions::visit(const ast::function_declaration& decl)
    {
        // substitutions may be done for concept conditions
        if (decl.generic()) decl.generic()->accept(*this);
        for (auto contract : decl.contracts()) contract->accept(*this);
        for (auto param : decl.parameters()) param->accept(*this);
        if (decl.return_type_expression()) decl.return_type_expression()->accept(*this);
        if (decl.body()) decl.body()->accept(*this);
    }
    
    void substitutions::visit(const ast::property_declaration& decl)
    {
        for (auto param : decl.parameters()) param->accept(*this);
        for (auto contract : decl.contracts()) contract->accept(*this);
        if (decl.return_type_expression()) decl.return_type_expression()->accept(*this);
        if (decl.body()) decl.body()->accept(*this);
    }
    
    void substitutions::visit(const ast::concept_declaration& decl)
    {
        if (decl.base()) decl.base()->accept(*this);
        for (auto prototype : decl.prototypes()) prototype->accept(*this);
    }
    
    void substitutions::visit(const ast::extend_declaration& decl)
    {
        decl.type_expression()->accept(*this);
        for (auto b : decl.behaviours()) b->accept(*this);
        for (auto d : decl.declarations()) d->accept(*this);
    }
    
    void substitutions::visit(const ast::behaviour_declaration& decl) 
    {
        // substitutions may be done for concept conditions
        if (decl.generic()) decl.generic()->accept(*this); 
        for (auto d : decl.declarations()) d->accept(*this); 
    }
    
    void substitutions::visit(const ast::extern_declaration& decl) {}
    
    void substitutions::visit(const ast::range_declaration& decl) 
    {
        // substitutions may be done for concept conditions
        if (decl.generic()) decl.generic()->accept(*this); 
        decl.constraint()->accept(*this); 
    }
    
    void substitutions::visit(const ast::record_declaration& decl) 
    {
        // substitutions may be done for concept conditions
        if (decl.generic()) decl.generic()->accept(*this); 
        for (auto field : decl.fields()) field->accept(*this); 
    }
        
    void substitutions::visit(const ast::variant_declaration& decl) 
    {
        // substitutions may be done for concept conditions
        if (decl.generic()) decl.generic()->accept(*this); 
        for (auto t : decl.types()) t->accept(*this); 
    }
        
    void substitutions::visit(const ast::alias_declaration& decl) 
    {
        // substitutions may be done for concept conditions
        if (decl.generic()) decl.generic()->accept(*this); 
        decl.type_expression()->accept(*this); 
    }
    
    void substitutions::visit(const ast::use_declaration& decl) {}
    
    void substitutions::visit(const ast::workspace_declaration& decl) {}
    
    void substitutions::visit(const ast::source_unit_declaration& decl) {}
}
