#ifndef CARESCRIPT_DEFS_HPP
#define CARESCRIPT_DEFS_HPP

#include <cmath>
#include <fstream>
#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <filesystem>
#include <exception>
#include <functional>
#include <any>

#include "catpkgs/kittenlexer/kittenlexer.hpp"

#include "carescript-types.hpp"
#include "carescript-macromagic.hpp"

namespace carescript {

template<typename _Tp>
concept ScriptValueType = std::is_base_of<carescript::ScriptValue,_Tp>::value;

// Wrapper class to perfom tasks on a
// subclass of the abstract class "ScriptValue"
struct ScriptVariable {
    std::unique_ptr<ScriptValue> value = nullptr;

    bool operator==(const ScriptVariable& sv) const noexcept {
        return *sv.value == *value;
    }
    
    ScriptVariable() {}
    ScriptVariable(ScriptValue* ptr): value(ptr) {}
    ScriptVariable(const ScriptVariable& var) {
        if(var.value.get() != nullptr) value.reset(var.value->copy());
    }

    template<typename _Tp>
    ScriptVariable(_Tp a) {
        // note here: we use `from` but it's class internally only
        // defined for ScriptValue pointers.
        // the user can define other `from` functions inside the
        // `carescript` namespace to effectivly overload the
        // constructor of this class
        from(*this,a);
    }

    ScriptVariable& operator=(const ScriptVariable& var) noexcept {
        if(var.value.get() != nullptr) value.reset(var.value->copy());
        return *this;
    }

    std::string get_type() const noexcept {
        return value.get()->get_type();
    }
    std::string printable() const noexcept{
        return value.get()->to_printable();
    }
    std::string string() const noexcept {
        return value.get()->to_string();
    }

    template<typename _Tp>
    operator _Tp() const noexcept {
        return get_value<_Tp>(*this);
    }

    inline friend void from(ScriptVariable& var, ScriptValue* a) noexcept {
        var.value.reset(a);
    }
};

// checks if a variable has a specific type
template<ScriptValueType _Tval>
inline bool is_typeof(const carescript::ScriptVariable& var) noexcept {
    _Tval inst;
    return var.get_type() == inst.get_type();
}

// checks if two subclasses of ScriptValue are the same
template<ScriptValueType _Tp1, ScriptValueType _Tp2>
inline bool is_same_type() noexcept {
    _Tp1 v1;
    _Tp2 v2;
    return v1.get_type() == v2.get_type();
}

// checks if two ScriptVariable instances have the same type
inline bool is_same_type(const ScriptVariable& v1,const ScriptVariable& v2) noexcept {
    return v1.get_type() == v2.get_type();
}

// checks if a variable is null
inline bool is_null(const ScriptVariable& v) noexcept {
    return v.value.get() == nullptr || is_typeof<ScriptNullValue>(v);
}

// returns the unwrapped type of a variable
template<typename _Tp>
inline auto get_value(const carescript::ScriptVariable& v) noexcept {
    return ((const _Tp*)v.value.get())->get_value();
}

// modify only if you know what you're doing
inline ScriptVariable script_null = new ScriptNullValue();
inline ScriptVariable script_true = new ScriptNumberValue(true);
inline ScriptVariable script_false = new ScriptNumberValue(false);

struct Interpreter;
struct ScriptLabel;
// general storage class for the current state of execution
struct ScriptSettings {
    Interpreter& interpreter;
    int line = 0;
    bool exit = false;
    std::stack<bool> should_run;
    std::map<std::string,ScriptVariable> variables;
    std::map<std::string,ScriptVariable> constants;
    std::map<std::string,ScriptLabel> labels;
    std::filesystem::path parent_path;
    int ignore_endifs = 0;
    ScriptVariable return_value = script_null;

    std::string error_msg;
    bool raw_error = false;
    std::stack<std::string> label;

    std::map<std::string,std::any> storage;

    ScriptSettings(Interpreter& i): interpreter(i) {}

    void clear() noexcept {
        line = 0;
        exit = false;
        should_run = std::stack<bool>();
        variables.clear();
        constants.clear();
        labels.clear();
        parent_path = "";
        ignore_endifs = 0;
        return_value = script_null;
        error_msg = "";
        raw_error = false;
        label = std::stack<std::string>();
        storage.clear();
    }
};

// storage class for an operator
struct ScriptOperator {
    // higher priority -> the later it gets executed
    int priority = 0;
    // UNKNOWN -> internally used, nono, don't touch it!
    enum {UNARY, BINARY, UNKNOWN} type;
    // if UNARY, `right` will always be script_null
    ScriptVariable(*run)(const ScriptVariable& left, const ScriptVariable& right, ScriptSettings& settings) = nullptr;
};

using ScriptArglist = std::vector<ScriptVariable>;
// simple C-like replacement macro, not recursive
using ScriptMacro = std::pair<std::string,std::string>;
// instead of evaluated arguments, this get's the raw input
using ScriptRawBuiltin = ScriptVariable(*)(const std::string& source, ScriptSettings& settings);
// tries to evaluate a string into a variable, nullptr if fauled
using ScriptTypeCheck = ScriptValue*(*)(KittenToken src, ScriptSettings& settings);

// storage class for a builtin function
struct ScriptBuiltin {
    // <0 disables this check
    int arg_count = -1;
    // return `script_null` for no return value
    ScriptVariable(*exec)(const ScriptArglist&,ScriptSettings&);
};

// storage class for a label
struct ScriptLabel {
    std::vector<std::string> arglist;
    lexed_kittens lines;
    int line = 0;
};

extern std::map<std::string,ScriptBuiltin> default_script_builtins;
extern std::map<std::string,std::vector<ScriptOperator>> default_script_operators;
extern std::vector<ScriptTypeCheck> default_script_typechecks;
extern std::unordered_map<std::string,std::string> default_script_macros;

// bakes an extension into the interpreter
bool bake_extension(std::string name, ScriptSettings& settings) noexcept;
class Extension;
bool bake_extension(Extension* extension, ScriptSettings& settings) noexcept;

// runs a "main" function of a script
std::string run_script(std::string source, ScriptSettings& settings) noexcept;
// runs a specific label with the given parameters
std::string run_label(std::string label_name, std::map<std::string,ScriptLabel> labels, ScriptSettings& settings, std::filesystem::path parent_path , std::vector<ScriptVariable> args) noexcept;

// preprocesses the file into the interpreter
std::map<std::string,ScriptLabel> pre_process(std::string source, ScriptSettings& settings) noexcept;
std::vector<ScriptVariable> parse_argumentlist(std::string source, ScriptSettings& settings) noexcept;
// evaluates an expression and returns the result
ScriptVariable evaluate_expression(std::string source, ScriptSettings& settings) noexcept;
void parse_const_preprog(std::string source, ScriptSettings& settings) noexcept;

inline static bool is_operator_char(char) noexcept;

// default lexers, can be accessed and configured by the user 
struct LexerCollection {
    KittenLexer argumentlist = KittenLexer()
        .add_capsule('(',')')
        .add_capsule('[',']')
        .add_capsule('{','}')
        .add_stringq('"')
        .add_ignore(' ')
        .add_ignore('\t')
        .add_ignore('\n')
        .ignore_backslash_opts()
        .add_con_extract(is_operator_char)
        .add_extract(',')
        .erase_empty();
    KittenLexer expression = KittenLexer()
        .add_stringq('"')
        .add_capsule('(',')')
        .add_capsule('[',']')
        .add_capsule('{','}')
        .add_con_extract(is_operator_char)
        .add_ignore(' ')
        .add_ignore('\t')
        .add_backslashopt('t','\t')
        .add_backslashopt('n','\n')
        .add_backslashopt('r','\r')
        .add_backslashopt('\\','\\')
        .add_backslashopt('"','\"')
        .erase_empty();
    KittenLexer preprocess = KittenLexer()
        .add_stringq('"')
        .add_capsule('(',')')
        .add_capsule('[',']')
        .add_ignore(' ')
        .add_ignore('\t')
        .add_linebreak('\n')
        .add_lineskip('#')
        .add_extract('@')
        .ignore_backslash_opts()
        .erase_empty();
    
    void clear() {
        argumentlist = expression = preprocess = KittenLexer();
    }
};

class Interpreter;
// storage class to temporarily store states of the interpreter
struct InterpreterState {
    std::map<std::string,ScriptBuiltin> script_builtins;
    std::map<std::string,std::vector<ScriptOperator>> script_operators;
    std::vector<ScriptTypeCheck> script_typechecks;
    std::unordered_map<std::string,std::string> script_macros;
    std::unordered_map<std::string,ScriptRawBuiltin> script_rawbuiltins;

    LexerCollection lexers;

    InterpreterState() {}
    InterpreterState(const Interpreter& interp) { save(interp); }
    InterpreterState(
        const std::map<std::string,ScriptBuiltin>& a,
        const std::map<std::string,std::vector<ScriptOperator>>& b,
        const std::vector<ScriptTypeCheck>& c,
        const std::unordered_map<std::string,std::string>& d,
        const std::unordered_map<std::string,ScriptRawBuiltin>& e,
        const LexerCollection& f):
        script_builtins(a),
        script_operators(b),
        script_typechecks(c),
        script_macros(d),
        script_rawbuiltins(e),
        lexers(f) {}

    void load(Interpreter& interp) const noexcept;
    void save(const Interpreter& interp) noexcept;

    InterpreterState& operator=(const std::map<std::string,ScriptBuiltin>& a) noexcept {
        script_builtins = a;
        return *this;
    }
    InterpreterState& operator=(const std::map<std::string,std::vector<ScriptOperator>>& a) noexcept {
        script_operators = a;
        return *this;
    }
    InterpreterState& operator=(const std::vector<ScriptTypeCheck>& a) noexcept {
        script_typechecks = a;
        return *this;
    }
    InterpreterState& operator=(const std::unordered_map<std::string,std::string>& a) noexcept {
        script_macros = a;
        return *this;
    }
    InterpreterState& operator=(const std::unordered_map<std::string,ScriptRawBuiltin>& a) noexcept {
        script_rawbuiltins = a;
        return *this;
    }
    InterpreterState& operator=(const LexerCollection& a) noexcept {
        lexers = a;
        return *this;
    }

    InterpreterState& add(const std::map<std::string,ScriptBuiltin>& a) noexcept {
        script_builtins.insert(a.begin(),a.end());
        return *this;
    }
    InterpreterState& add(const std::map<std::string,std::vector<ScriptOperator>>& a) noexcept {
        for(auto& i : script_operators) {
            for(auto j : a.at(i.first)) {
                i.second.push_back(j);
            }
        }
        return *this;
    }
    InterpreterState& add(const std::vector<ScriptTypeCheck>& a) noexcept {
        for(auto i : a) {
            script_typechecks.push_back(i);
        }
        return *this;
    }
    InterpreterState& add(const std::unordered_map<std::string,std::string>& a) noexcept {
        script_macros.insert(a.begin(),a.end());
        return *this;
    }
    InterpreterState& add(const std::unordered_map<std::string,ScriptRawBuiltin>& a) noexcept {
        script_rawbuiltins.insert(a.begin(),a.end());
        return *this;
    }
};

// helper class for handling errors
struct InterpreterError  {
    Interpreter& interpreter;
private:
    bool has_value = false;
    ScriptVariable value = script_null;
public:
    InterpreterError(Interpreter& i): interpreter(i) {}
    InterpreterError(Interpreter& i, ScriptVariable val): interpreter(i), value(val) { has_value = true; }
    InterpreterError& on_error(std::function<void(Interpreter&)>) noexcept;
    InterpreterError& otherwise(std::function<void(Interpreter&)>) noexcept;
    InterpreterError& throw_error();
    ScriptVariable get_value() { return value; }
    ScriptVariable get_value_or(ScriptVariable var) { return has_value ? value : var; }
    Interpreter& chain() noexcept { return interpreter; }
    operator ScriptVariable() const noexcept { return value; }
};

// wrapper and storage class for a simpler API usage
class Interpreter {
    std::map<int,InterpreterState> states;
    std::function<void(Interpreter&)> on_error_f;

    void error_check() {
        if(settings.error_msg != "" && on_error_f) on_error_f(*this);
    }
public:
    std::map<std::string,ScriptBuiltin> script_builtins = default_script_builtins;
    std::map<std::string,std::vector<ScriptOperator>> script_operators = default_script_operators;
    std::vector<ScriptTypeCheck> script_typechecks = default_script_typechecks;
    std::unordered_map<std::string,std::string> script_macros = default_script_macros;
    std::unordered_map<std::string,ScriptRawBuiltin> script_rawbuiltins;
    
    LexerCollection lexer;
    ScriptSettings settings = ScriptSettings(*this);
    
    void save(int id) noexcept {
        states[id].save(*this);
    }

    void load(int id) noexcept {
        states[id].load(*this);
    }

    void clear() {
        script_builtins.clear();
        script_operators.clear();
        script_typechecks.clear();
        script_macros.clear();
        script_rawbuiltins.clear();
        lexer.clear();
        settings.clear();
    }

    operator bool() {
        return settings.error_msg == "";
    }
    operator ScriptSettings&() {
        return settings;
    }

    InterpreterError pre_process(std::string source) {
        settings.error_msg = "";
        settings.labels = ::carescript::pre_process(source,settings);
        error_check();
        return *this;
    }

    InterpreterError run() {
        settings.return_value = script_null;
        settings.line = 1;
        settings.exit = false;
        settings.error_msg = run_label("main",settings.labels,settings,"",{});
        settings.exit = false;
        error_check();
        return is_null(settings.return_value) ? *this : InterpreterError(*this,settings.return_value);
    }

    template<typename... _Targs>
    InterpreterError run(std::string label, _Targs ...targs) {
        std::vector<ScriptVariable> args = {targs...};
        return run(label,args);
    }
    InterpreterError run(std::string label, std::vector<ScriptVariable> args) noexcept {
        settings.return_value = script_null;
        settings.line = 1;
        settings.exit = false;
        settings.error_msg = run_label(label,settings.labels,settings,"",args);
        settings.exit = false;
        error_check();
        return is_null(settings.return_value) ? *this : InterpreterError(*this,settings.return_value);
    }

    InterpreterError eval(std::string source) noexcept {
        settings.return_value = script_null;
        settings.error_msg = run_script(source,settings);
        settings.exit = false;
        error_check();
        return is_null(settings.return_value) ? *this : InterpreterError(*this,settings.return_value);
    }
    InterpreterError expression(std::string source) {
        auto ret = evaluate_expression(source,settings);
        error_check();
        return is_null(ret) ? *this : InterpreterError(*this,ret);
    }

    int to_local_line(int line) const noexcept { return line - settings.labels.at(settings.label.top()).line; }
    int to_global_line(int line) const noexcept { return line + settings.labels.at(settings.label.top()).line; }

    void on_error(std::function<void(Interpreter&)> fun) noexcept {
        on_error_f = fun;
    }

    std::string error() const noexcept { return settings.error_msg; }

    Interpreter& add_builtin(const std::string& name, const ScriptBuiltin& builtin) noexcept {
        script_builtins[name] = builtin;
        return *this;
    }
    Interpreter& add_operator(const std::string& name, const ScriptOperator& _operator) {
        if(lexer.expression.lex(name).size() != 1) 
            throw "Carescript: Operator name must be 1 token";
        script_operators[name].push_back(_operator);
        return *this;
    }
    Interpreter& add_typecheck(const ScriptTypeCheck& typecheck) noexcept {
        script_typechecks.push_back(typecheck);
        return *this;
    }
    Interpreter& add_macro(const std::string& macro, const std::string& replacement) noexcept {
        script_macros[macro] = replacement;
        return *this;
    }
    Interpreter& add_rawbuiltin(std::string name, const ScriptRawBuiltin& rawbuiltin) noexcept {
        script_rawbuiltins[name] = rawbuiltin;
        return *this;
    }
    InterpreterError bake(std::string file) noexcept {
        if(!bake_extension(file,settings)) 
            settings.error_msg = "error while baking: " + file;
        return *this;
    }
    InterpreterError bake(Extension* ext) noexcept {
        if(!bake_extension(ext,settings)) 
            settings.error_msg = "error while baking: <compiled>";
        return *this;
    }

    bool has_builtin(std::string name) const noexcept {
        return script_builtins.find(name) != script_builtins.end();
    }
    ScriptBuiltin& get_builtin(std::string name) {
        return script_builtins[name];
    }
    bool has_macro(std::string name) const noexcept {
        return script_macros.find(name) != script_macros.end();
    }
    std::string& get_macro(std::string name) noexcept {
        return script_macros[name];
    }
    bool has_operator(std::string name) const noexcept {
        return script_operators.find(name) != script_operators.end();
    }
    std::vector<ScriptOperator>& get_operator(std::string name) noexcept {
        return script_operators[name];
    }
    bool has_rawbuiltin(std::string name) const noexcept {
        return script_rawbuiltins.find(name) != script_rawbuiltins.end();
    }
    ScriptRawBuiltin& get_rawbuiltin(std::string name) noexcept {
        return script_rawbuiltins[name];
    }

    bool has_variable(std::string name) const noexcept {
        return settings.variables.find(name) != settings.variables.end();
    }
    ScriptVariable& get_variable(std::string name) noexcept {
        return settings.variables[name];
    }
    
};

inline InterpreterError& InterpreterError::on_error(std::function<void(Interpreter&)> fun) noexcept {
    if(!interpreter) fun(interpreter);
    return *this;
}

inline InterpreterError& InterpreterError::otherwise(std::function<void(Interpreter&)> fun) noexcept {
    if(interpreter) fun(interpreter);
    return *this;
}

inline InterpreterError& InterpreterError::throw_error() {
    if(!interpreter) throw interpreter;
    return *this;
}

inline void InterpreterState::load(Interpreter& interp) const noexcept {
    interp.clear();
    interp.script_builtins = this->script_builtins;
    interp.script_operators = this->script_operators;
    interp.script_typechecks = this->script_typechecks;
    interp.script_macros = this->script_macros;
    interp.script_rawbuiltins = this->script_rawbuiltins;
    interp.lexer = this->lexers;
}
inline void InterpreterState::save(const Interpreter& interp) noexcept {
    script_builtins = interp.script_builtins;
    script_operators = interp.script_operators;
    script_typechecks = interp.script_typechecks;
    script_macros = interp.script_macros;
    script_rawbuiltins = interp.script_rawbuiltins;
    lexers = interp.lexer;
}

// converts a literal into a variable
inline ScriptVariable to_var(KittenToken src, ScriptSettings& settings) noexcept {
    ScriptVariable ret;
    for(auto i : settings.interpreter.script_typechecks) {
        ScriptValue* v = i(src,settings);
        if(v != nullptr) {
            return std::move(ScriptVariable(v));
        }
    }
    return script_null;
}

// checks if the token is a valid literal
inline bool valid_literal(KittenToken src,ScriptSettings& settings) {
    for(auto i : settings.interpreter.script_typechecks) {
        ScriptValue* v = i(src,settings);
        if(v != nullptr) {
            delete v;
            return true;
        }
    }
    return false;
}

// sets the error flag is the variable is null
inline ScriptVariable not_null_check(ScriptVariable var, ScriptSettings& settings) {
    if(is_null(var)) {
        settings.error_msg = "not allowed to be null!";
    }
    return var;
}

#define CARESCRIPT_EXTENSION using namespace carescript;
#define CARESCRIPT_EXTENSION_GETEXT_INLINE(...) extern "C" { inline carescript::Extension* get_extension() { __VA_ARGS__ } }
#define CARESCRIPT_EXTENSION_GETEXT(...) extern "C" { carescript::Extension* get_extension() { __VA_ARGS__ } }

using BuiltinList = std::unordered_map<std::string,ScriptBuiltin>;
using OperatorList = std::unordered_map<std::string,std::vector<ScriptOperator>>;
using MacroList = std::unordered_map<std::string,std::string>;
using TypeList = std::vector<ScriptTypeCheck>;

// abstract class to provide an interface for extensions
class Extension {
public:
    virtual BuiltinList get_builtins() = 0;
    virtual OperatorList get_operators() = 0;
    virtual MacroList get_macros() = 0;
    virtual TypeList get_types() = 0;
};

using get_extension_fun = Extension*(*)();

// external overloads for the ScriptVariable constructor

template<typename _Tp>
concept ArithmeticType = std::is_arithmetic<_Tp>::value;
template<ArithmeticType _Tp>
inline void from(carescript::ScriptVariable& var, _Tp integral) {
    var = new carescript::ScriptNumberValue(integral);
}
inline void from(carescript::ScriptVariable& var, std::string string) {
    var = new carescript::ScriptStringValue(string);
}

} /* namespace carescript */

#endif