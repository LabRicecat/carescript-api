#include "../carescript-api.hpp"

/*
    This is a template for how you
    might use this API
*/

const std::string source_code = R"(
@const[
    the_answer = 12
]

@add[x,y]
    return($x + $y)

@mul[x,y]
    if($y is 1 or $y is 0)
        return($x)
    endif()
    if($y is 0)
        return(0)
    endif()

    return(call(mul,$x,$y - 1) + $x)

@main[]
    echoln("Hello, World!")
)";

int main() {
    using namespace carescript;
    Interpreter interp;
    ScriptVariable value = interp.eval(R"(
        echoln("Test begins...")
        return(12)
    )").get_value_or(0);

    try {
        interp.pre_process(source_code).throw_error();
        ScriptVariable res = interp.run("mul",9,8).throw_error().get_value_or(script_null);
        ScriptVariable res2 = interp.run("add",5,213).throw_error().get_value_or(script_null);

        std::cout << res.printable() << "\n";
        std::cout << res2.printable() << "\n";
    }
    catch(Interpreter interp) {
        std::cout << interp.error() << "\n";
    }
}