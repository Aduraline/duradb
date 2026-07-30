#include "repl/repl.hpp"

#include "catalog/catalog_constants.hpp"

#include <iostream>
#include <string>

int main(int argc, char **argv) {
    std::string database_name = std::string(duradb::kDefaultDatabase);
    if (argc > 1) {
        database_name = argv[1];
    }

    duradb::Repl repl(std::move(database_name));
    return repl.run(std::cin, std::cout);
}
