#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <unordered_map>

#include <nemesis/parser/ast.hpp>

namespace nemesis {
    class environment {
    public:
        enum class kind { workspace, global, function, test, block, loop, declaration };

        environment(const ast::node* enclosing, environment* parent = nullptr);
        const ast::node* enclosing() const { return enclosing_; }
        environment* parent() const { return parent_; }
        void parent(environment* value) { parent_ = value; }
        
        const ast::declaration* value(std::string name, bool recursive = true) const;
        const ast::declaration* function(std::string name, bool recursive = true) const;
        const ast::type_declaration* type(std::string name, bool recursive = true) const;
        const ast::concept_declaration* concept(std::string name, bool recursive = true) const;

        void define(const ast::declaration* decl)
        {
            if (auto tdecl = dynamic_cast<const ast::type_declaration*>(decl)) type(tdecl->name().lexeme().string(), tdecl);
            else if (auto vdecl = dynamic_cast<const ast::var_declaration*>(decl)) value(vdecl->name().lexeme().string(), decl);
            else if (auto cdecl = dynamic_cast<const ast::const_declaration*>(decl)) value(cdecl->name().lexeme().string(), decl);
            else if (auto cpdecl = dynamic_cast<const ast::generic_const_parameter_declaration*>(decl)) value(cpdecl->name().lexeme().string(), decl);
            else if (auto fdecl = dynamic_cast<const ast::function_declaration*>(decl)) function(fdecl->name().lexeme().string(), decl);
            else if (auto pdecl = dynamic_cast<const ast::property_declaration*>(decl)) function(pdecl->name().lexeme().string(), decl);
            else if (auto cdecl = dynamic_cast<const ast::concept_declaration*>(decl)) concept(cdecl->name().lexeme().string(), cdecl);
        }

        void remove(const ast::declaration* decl)
        {
            if (auto tdecl = dynamic_cast<const ast::type_declaration*>(decl)) types_.erase(tdecl->name().lexeme().string());
            else if (auto vdecl = dynamic_cast<const ast::var_declaration*>(decl)) values_.erase(vdecl->name().lexeme().string());
            else if (auto cdecl = dynamic_cast<const ast::const_declaration*>(decl)) values_.erase(cdecl->name().lexeme().string());
            else if (auto cpdecl = dynamic_cast<const ast::generic_const_parameter_declaration*>(decl)) values_.erase(cpdecl->name().lexeme().string());
            else if (auto fdecl = dynamic_cast<const ast::function_declaration*>(decl)) functions_.erase(fdecl->name().lexeme().string());
            else if (auto pdecl = dynamic_cast<const ast::property_declaration*>(decl)) functions_.erase(pdecl->name().lexeme().string());
            else if (auto cdecl = dynamic_cast<const ast::concept_declaration*>(decl)) concepts_.erase(cdecl->name().lexeme().string());
        }

        void value(std::string name, const ast::declaration* decl);
        void function(std::string name, const ast::declaration* fdecl);
        void type(std::string name, const ast::type_declaration* tdecl);
        void concept(std::string name, const ast::concept_declaration* tdecl);

        std::unordered_map<std::string, const ast::declaration*>& values() const { return values_; }
        std::unordered_map<std::string, const ast::declaration*>& functions() const { return functions_; }
        std::unordered_map<std::string, const ast::type_declaration*>& types() const { return types_; }
        std::unordered_map<std::string, const ast::concept_declaration*>& concepts() const { return concepts_; }

        bool inside(kind ctx) const;
        const ast::node* outscope(kind ctx) const;
        bool has_ancestor_scope(environment* candidate) const;

        std::string canonical(std::string type) const;

        void dump() const
        {
            auto scope = this;
            do {
                if (auto n = dynamic_cast<const ast::workspace*>(scope->enclosing_)) std::cout << "|\n" << n->name << '\n';
                else if (auto d = dynamic_cast<const ast::declaration*>(scope->enclosing_)) std::cout << "|\n" << ast::printer().print(*d) << '\n';
                else if (auto n = dynamic_cast<const ast::statement*>(scope->enclosing_)) std::cout << "|\n" << ast::printer().print(*n) << '\n';
                else if (auto e = dynamic_cast<const ast::expression*>(scope->enclosing_)) std::cout << "|\n" << ast::printer().print(*e) << '\n';
                scope = scope->parent_;
            } 
            while (scope);
        }
    private:
        const ast::node* enclosing_ = nullptr;
        environment* parent_ = nullptr;
        std::vector<environment*> children_;
        mutable std::unordered_map<std::string, const ast::declaration*> values_;
        mutable std::unordered_map<std::string, const ast::declaration*> functions_;
        mutable std::unordered_map<std::string, const ast::type_declaration*> types_;
        mutable std::unordered_map<std::string, const ast::concept_declaration*> concepts_;
    };
}

#endif