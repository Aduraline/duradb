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
    void process_sql(const std::string &sql, std::ostream &output);
    bool read_sql_buffer(const std::string &first_line, std::istream &input, std::ostream &output,
                         std::string &buffer, bool &multiline);

    Cluster cluster_;
    Session session_;
};

} // namespace duradb
