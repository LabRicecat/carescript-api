#include "../carescript-api.hpp"

/*
    This is a template for an extension that
    implements simple references
*/

CARESCRIPT_EXTENSION

class ReferenceType : public ScriptValue {
public:
    ScriptVariable* ref;

    ScriptVariable* get_value() const { return ref; }
    ScriptVariable* get_value() { return ref; }
    const std::string get_type() const noexcept override { return "Reference"; }
    bool operator==(const ScriptValue* p) const noexcept {
        return false;
    };
    std::string to_printable() const noexcept {
        return "ref(" + ref->printable() + ")";
    }
    std::string to_string() const noexcept {
        return to_printable();
    }
    ScriptValue* copy() const noexcept {
        ReferenceType* a = new ReferenceType;
        a->ref = ref;
        return a;
    }

    ReferenceType() {}
    ReferenceType(ScriptVariable& v) : ref(&v) {}
    ReferenceType(ScriptVariable* v) : ref(v) {}
    
};

class ReferenceExtention : public Extension {
public:
    BuiltinList get_builtins() override {
        return {
            {"deref",{1,[](const ScriptArglist& args,ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ReferenceType);
                ReferenceType ref = get_value<ReferenceType>(args[0]);
                return ref.get_value()->value->copy();
            }}},
            {"ref",{1,[](const ScriptArglist& args,ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ScriptNameValue);
                ScriptVariable& var = settings.variables[get_value<ScriptNameValue>(args[0])];
                return new ReferenceType(var);
            }}},
            {"setref",{2,[](const ScriptArglist& args,ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ReferenceType);
                ScriptVariable* var = get_value<ReferenceType>(args[0]);
                *var = args[1];
                return script_null;
            }}}
        };
    }

    OperatorList get_operators() { return {}; }
    TypeList get_types() { return {}; }
    MacroList get_macros() { return {}; }
};

CARESCRIPT_EXTENSION_GETEXT(
    return new ReferenceExtention();
)
