#pragma once

#include <juliet/includes.hpp>

namespace juliet {

    using scalar  = std::float64_t;
    using complex = std::complex<juliet::scalar>;

    inline namespace literals {

        constexpr juliet::scalar operator ""_scalar(const long double scalar) noexcept {
            return static_cast<juliet::scalar>(scalar);
        }

    }

}
