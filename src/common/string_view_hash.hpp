#pragma once

#include <functional>
#include <string_view>

namespace duradb {

struct StringViewHash {
    using is_transparent = void;

    std::size_t operator()(const std::string_view value) const noexcept {
        return std::hash<std::string_view>{}(value);
    }
};

struct StringViewEqual {
    using is_transparent = void;

    bool operator()(const std::string_view lhs, const std::string_view rhs) const noexcept {
        return lhs == rhs;
    }
};

} // namespace duradb
