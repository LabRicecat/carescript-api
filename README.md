# carescript-api
A simple API for a highly dynamic DSL

## Installation 
**Note: the carescript API is not a standallone project, it's designed to be used as a dependency!**  

To install the carescriptAPI, install the [CatCaretaker](https://github.com/LabRicecat/CatCaretaker) and run:
```
$ catcare download labricecat/carescript-api@main
```

Now you can include the file `catpkgs/carescript-api/carescript-api.hpp` and can start!  

## Usage
### Language layout

Carescript is very simple, it's always:
```
function(<args...>)
function(<args...>)
...
```
There are also pre processor options starting with `@`:
```
@main[]
    echoln("This is the main function")
    set(a,45)
    echoln($a + 4)
    echoln(call(foo,5))

@foo[a]
    return($a * 2)
```
Pre processor functionallity is fixed, but the functions shown here are only a possible  
implementation, although there is a default implementation available.  
The code above defines two labels (simmilar to functions):
 - `main`: here will *usually* start the program
 - `foo`: which takes one parameter and will return the argument times 2

Variables are stored locally to each label and can not be shared.

### Using the interpreter
```c++
// Includes everything of the API
#include "catpkgs/catcare-api/carescript-api.hpp"

int main() {
    // creates a new interpreter instance
    carescript::Interpreter interpreter;

    // save the current state as id 0
    interpreter.save(0);

    // loads an external extension, relative as well as absolute paths are valid.
    interpreter.bake("path/to/extension");

    interpreter.on_error([](carescript::Interpreter& interp) {
        std::cout << interp.error() << "\n";
        // This code executes when an error occurs
    });

    // pre processes the code
    interpreter.pre_process("source-code");
    
    // runs the "main" label
    interpreter.run();

    // runs the label "some_label" with the arguments 1, 2 and 3
    interpreter.run("some_label",1,2,3);

    // runs the label "label_with_return" and unwraps the return value
    carescript::ScriptVariable value = interpreter.run("label_with_return").get_value();

    interpreter.load(0); // loads the saved state with id 0
    // note: this will also remove the loaded extension from before!
}
```
### Manually adding functionallity
```c++
int main() {
    using namespace carescript;
    // creates a new interpreter instance
    Interpreter interpreter;

    // adds a function called "repeatly_say" with two arguments
    // usage: repeatly_say("<text>",times)
    interpreter.add_builtin("repeatly_say",{
    //  v-- argument count, set it to negative to disable this check
        2,[](const ScriptArglist& args, ScriptSettings& settings)->ScriptVariable {
            cc_builtin_if_ignore(); // skips this function in case of an failed "if"
            cc_builtin_var_requires(args[0],ScriptStringValue); // argument 1 must be a string
            cc_builtin_var_requires(args[1],ScriptNumberValue); // argument 2 must be a number
            
            // gets the number value
            int count = get_value<ScriptNumberValue>(args[1]);

            for(size_t i = 0; i < count; ++i) {
                // outputs the arguments printable text
                std::cout << args[0].printable() << "\n";
            }

            // returns null as no notable return value
            return script_null;
        }  
    });
}
```
**Note: there are more `.add_*` methods, such as for operators, macros, etc...
### Writing an extension
```c++
#include "carescript-api.hpp"

CARESCRIPT_EXTENSION

// Implent a new type by adding a class that implements the ScriptValue interface
class ListType : public ScriptValue {
public:
    using val_t = std::vector<ScriptVariable>;
    val_t list;

    val_t get_value() const noexcept { return list; }
    val_t& get_value() noexcept { return list; }
    const std::string get_type() const noexcept override { return "List"; }
    bool operator==(const ScriptValue* p) const noexcept override {
        return p->get_type() == get_type() && ((ListType*)p)->get_value() == get_value();
    };
    std::string to_printable() const noexcept override {
        std::string s = "[";
        for(auto i : list) {
            s += i.string() + ",";
        }
        if(s != "[") s.pop_back();
        return s + "]";
    }
    std::string to_string() const noexcept override {
        return to_printable();
    }
    ScriptValue* copy() const noexcept override {
        return new ListType(list);
    }

    ListType() {}
    ListType(val_t v) : list(v) {}
};

// write a main extension class that implements the Extension interface
class ListExtention : public Extension {
public:
    // functions to add
    BuiltinList get_builtins() override {
        return {
            // usage: push(list,to_push1,to_push2,...)
            {"push",{-1,[](const ScriptArglist& args,ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                // Note how we can already use "ListType" here as any other type
                cc_builtin_var_requires(args[0],ListType);
                cc_builtin_arg_min(args,2);
                ListType list = get_value<ListType>(args[0]);
                for(size_t i = 1; i < args.size(); ++i) {
                    list.list.push_back(args[i]);
                }
                return new ListType(list);
            }}},
            // usage: pop(list,[times])
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

    // to add some operators
    OperatorList get_operators() { 
        return {
            // adds a "+" operator to join lists together
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

    // to implement what a list literal looks like
    // example: [1,2, 4*5, "hello"]
    TypeList get_types() { 
        return {
            [](KittenToken src, ScriptSettings& settings)->ScriptValue* {
                if(src.str) return nullptr; // on error, return a nullptr
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
};

// At the end, define how the extension gets instances
CARESCRIPT_EXTENSION_GETEXT(
    return new ListExtention(); // this will get freed automatically, don't worry
)
```
More examples can be found in the `template/` directory.

To add an extension either use
```c++
carescript::bake_extension("name.so",interpreter);
```
or inside of the script do
```py
@bake[
    "extension1"
    "extension2"
    ...
]
```
#### Adding an extension
To add an extension either add it at compile time using the `bake_extension` method, or compile it into a  
shared object with the following command:
```sh
$ g++ -fPIC -shared -std=c++20 -g extension.cpp -o extension.so
```
(`clang++` instead of `g++` is also possible)
