#pragma once

#include "engine/database_engine.hpp"

#include <iosfwd>
#include <string>

namespace duradb {

class Repl {
  public:
    int run(std::istream &input, std::ostream &output);

    void process_line(const std::string &line, std::ostream &output);

  private:
    DatabaseEngine engine_;
};

} // namespace duradb
