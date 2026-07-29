#include "repl/repl.hpp"

#include <iostream>

int main() {
    duradb::Repl repl;
    return repl.run(std::cin, std::cout);
}
