#pragma once

#include "catalog/catalog_constants.hpp"
#include "engine/cluster.hpp"
#include "engine/session.hpp"

#include <iosfwd>
#include <string>

namespace duradb {

class Repl {
  public:
    explicit Repl(std::string database_name = std::string(kDefaultDatabase));

    int run(std::istream &input, std::ostream &output);

    void process_line(const std::string &line, std::ostream &output);

  private:
    Cluster cluster_;
    Session session_;
};

} // namespace duradb
