#include "../carescript-api.hpp"

CARESCRIPT_EXTENSION

/*
    This is a template for an extension that
    implements simple lists
*/

class ListType : public ScriptValue {
public:
    using val_t = std::vector<ScriptVariable>;
    val_t list;

    val_t get_value() const { return list; }
    const std::string get_type() const override { return "List"; }
    bool operator==(const ScriptValue* p) const {
        return p->get_type() == get_type() && ((ListType*)p)->get_value() == get_value();
    };
    std::string to_printable() const {
        std::string s = "[";
        for(auto i : list) {
            s += i.string() + ",";
        }
        if(s != "[") s.pop_back();
        return s + "]";
    }
    std::string to_string() const {
        return to_printable();
    }
    ScriptValue* copy() const {
        return new ListType(list);
    }

    ListType() {}
    ListType(val_t v) : list(v) {}
    
};

class ListExtention : public Extension {
public:
    BuiltinList get_builtins() override {
        return {
            {"push",{-1,[](const ScriptArglist& args,ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ListType);
                cc_builtin_arg_min(args,2);
                ListType list = get_value<ListType>(args[0]);
                for(size_t i = 1; i < args.size(); ++i) {
                    list.list.push_back(args[i]);
                }
                return new ListType(list);
            }}},
            {"pop",{-1,[](const ScriptArglist& args,ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ListType);
                cc_builtin_arg_range(args,1,2);
                size_t count = 1;
                if(args.size() == 2) {
                    cc_builtin_var_requires(args[1],ScriptNumberValue);
                    count = (size_t)get_value<ScriptNumberValue>(args[1]);
                }
                ListType list = get_value<ListType>(args[0]);
                _cc_error_if(list.list.size() <= count,"popped not existing element (size below 0)");
                for(size_t i = 0; i < count; ++i) list.list.pop_back();
                return new ListType(list);
            }}}
        };
    }

    OperatorList get_operators() { 
        return {
            {{"+",{{0,ScriptOperator::BINARY,[](const ScriptVariable& left, const ScriptVariable& right, ScriptSettings& settings)->ScriptVariable {
                cc_operator_same_type(right,left,"+");
                cc_operator_var_requires(right,"+",ListType);
                auto lvec = get_value<ListType>(left);
                auto rvec = get_value<ListType>(right);
                for(auto& i : rvec) lvec.push_back(i);
                return new ListType(lvec);
            }}}}},
        }; 
    }
    TypeList get_types() { 
        return {
            [](KittenToken src, ScriptSettings& settings)->ScriptValue* {
                if(src.str) return nullptr;
                KittenLexer lx = KittenLexer()
                    .add_capsule('[',']')
                    .add_ignore(',')
                    .erase_empty()
                    .add_stringq('"')
                    ;
                auto lex1 = lx.lex(src.src);
                if(lex1.size() != 1) return nullptr;
                lex1[0].src.erase(lex1[0].src.begin());
                lex1[0].src.erase(lex1[0].src.end()-1);

                auto plist = lx.lex(lex1[0].src);
                ListType list;
                for(auto i : plist) {
                    if(i.str) i.src = "\"" + i.src + "\"";
                    list.list.push_back(evaluate_expression(i.src,settings));
                }
                return new ListType(list.list);
            }
        }; 
    }
    MacroList get_macros() { return {}; }
}static inline inst;

namespace carescript {
    // syntax sugar to enable
    // ScriptVariable name = std::vector<ScriptVariable>(...);
    inline void from(ScriptVariable& var, ListType::val_t list) {
        var = new ListType(list);
    }
}

CARESCRIPT_EXTENSION_GETEXT(
    return &inst;
)