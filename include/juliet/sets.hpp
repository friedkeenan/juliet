#pragma once

#include <juliet/common.hpp>

namespace juliet {

    template<typename Set>
    concept iterative_set = std::movable<std::remove_cvref_t<Set>> && requires(const Set &set, const juliet::complex num) {
        { set.template iterations_before_escape<0uz>(num) } -> std::same_as<std::size_t>;
    };

    struct mandelbrot_set_t {
        template<std::size_t MaxIterations>
        static constexpr std::size_t iterations_before_escape(const juliet::complex num) {
            static constexpr auto EscapeMagnitudeSquared = 2.0_scalar * 2.0_scalar;

            auto z = juliet::complex{};

            juliet::scalar real_sq = 0;
            juliet::scalar imag_sq = 0;

            for (const auto i : std::views::iota(0uz, MaxIterations)) {
                z.imag(2.0_scalar * z.real() * z.imag() + num.imag());
                z.real(real_sq - imag_sq + num.real());

                real_sq = z.real() * z.real();
                imag_sq = z.imag() * z.imag();

                if (real_sq + imag_sq > EscapeMagnitudeSquared) {
                    return i;
                }
            }

            return MaxIterations;
        }
    };

    static_assert(juliet::iterative_set<juliet::mandelbrot_set_t>);

    constexpr inline auto mandelbrot_set = mandelbrot_set_t{};

    struct quadratic_julia_set {
        juliet::complex constant;

        template<std::size_t MaxIterations>
        constexpr std::size_t iterations_before_escape(this const quadratic_julia_set self, juliet::complex num) {
            static constexpr juliet::scalar EscapeMagnitudeSquared = 2.0_scalar * 2.0_scalar;

            auto real_sq = num.real() * num.real();
            auto imag_sq = num.imag() * num.imag();

            for (const auto i : std::views::iota(0uz, MaxIterations)) {
                num.imag(2.0_scalar * num.real() * num.imag() + self.constant.imag());
                num.real(real_sq - imag_sq + self.constant.real());

                real_sq = num.real() * num.real();
                imag_sq = num.imag() * num.imag();

                if (real_sq + imag_sq > EscapeMagnitudeSquared) {
                    return i;
                }
            }

            return MaxIterations;
        }
    };

    static_assert(juliet::iterative_set<juliet::quadratic_julia_set>);

}
