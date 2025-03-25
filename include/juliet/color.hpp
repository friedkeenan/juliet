#pragma once

#include <juliet/common.hpp>

namespace juliet::color {

    /*
        Thank you Aly (https://github.com/s5bug) for helping me
        understand color space stuff enough to write this code.
    */

    struct rgba {
        static const rgba white;
        static const rgba black;

        std::uint8_t red;
        std::uint8_t green;
        std::uint8_t blue;
        std::uint8_t alpha;

        static constexpr rgba gray(std::uint8_t brightness) {
            return {brightness, brightness, brightness, 0xFF};
        }

        constexpr bool operator ==(const rgba &) const = default;
    };

    constexpr color::rgba color::rgba::white = {0xFF, 0xFF, 0xFF, 0xFF};
    constexpr color::rgba color::rgba::black = {0x00, 0x00, 0x00, 0xFF};

    struct rgb {
        static const rgb white;
        static const rgb black;

        std::uint8_t red;
        std::uint8_t green;
        std::uint8_t blue;

        static constexpr rgb gray(const std::uint8_t brightness) {
            return {brightness, brightness, brightness};
        }

        constexpr color::rgba rgba(this const rgb self) {
            return {self.red, self.green, self.blue, 0xFF};
        }

        constexpr bool operator ==(const rgb &) const = default;
    };

    constexpr color::rgb color::rgb::white = {0xFF, 0xFF, 0xFF};
    constexpr color::rgb color::rgb::black = {0x00, 0x00, 0x00};

    struct white_point {
        static const white_point d65;

        std::float32_t x;
        std::float32_t y;
        std::float32_t z;
    };

    constexpr color::white_point color::white_point::d65 = {0.95047f32, 1.0f32, 1.08883f32};

    struct rgb_space {
        static const rgb_space standard;

        std::float32_t xr, yr;
        std::float32_t xg, yg;
        std::float32_t xb, yb;
    };

    constexpr color::rgb_space color::rgb_space::standard = {
        0.64f32, 0.33f32,
        0.30f32, 0.60f32,
        0.15f32, 0.06f32
    };

    namespace impl {

        /*
            NOTE: This is just a quick and dirty implementation.

            Also note: I hate matrix math.
        */
        template<std::size_t Rows, std::size_t Columns>
        requires ((Rows == 2 && Columns == 2) || (Rows == 3 && Columns == 3))
        struct matrix {
            static constexpr bool Is_2_2 = (Rows == 2 && Columns == 2);
            static constexpr bool Is_3_3 = (Rows == 3 && Columns == 3);

            std::array<std::float32_t, Rows * Columns> _elements = {};

            constexpr matrix() = default;

            template<std::convertible_to<std::float32_t>... Args>
            requires (sizeof...(Args) == Rows * Columns)
            constexpr matrix(const Args... args) : _elements({args...}) {}

            constexpr const std::float32_t &operator [](this const matrix &self, const std::size_t row_index, const std::size_t column_index) {
                [[assume(row_index    < Rows)]];
                [[assume(column_index < Columns)]];

                return self._elements[row_index * Columns + column_index];
            }

            friend constexpr matrix operator *(const std::float32_t lhs, const matrix &rhs) {
                auto result = impl::matrix<Rows, Columns>();

                for (auto [result_elem, self_elem] : std::views::zip(result._elements, rhs._elements)) {
                    result_elem = lhs * self_elem;
                }

                return result;
            }

            constexpr std::array<std::float32_t, Rows> multiply_vector(this const matrix &self, const std::array<std::float32_t, Rows> vector) {
                std::array<std::float32_t, Rows> result = {};

                for (const auto i : std::views::iota(0uz, Rows)) {
                    for (const auto j : std::views::iota(0uz, Columns)) {
                        result[i] += vector[j] * self[i, j];
                    }
                }

                return result;
            }

            constexpr std::float32_t determinant(this const matrix &self) {
                if constexpr (Is_2_2) {
                    return self[0, 0] * self[1, 1] - self[1, 0] * self[0, 1];
                } else {
                    const auto first = self[0, 0] * impl::matrix(
                        self[1, 1], self[1, 2],
                        self[2, 1], self[2, 2]
                    ).determinant();

                    const auto second = self[0, 1] * impl::matrix(
                        self[1, 0], self[1, 2],
                        self[2, 0], self[2, 2]
                    ).determinant();

                    const auto third = self[0, 2] * impl::matrix(
                        self[1, 0], self[1, 1],
                        self[2, 0], self[2, 1]
                    ).determinant();

                    return first - second + third;
                }
            }

            constexpr matrix inverse(this const matrix &self) {
                const auto inverse_determinant = 1.0f32 / self.determinant();

                if constexpr (Is_2_2) {
                    return inverse_determinant * impl::matrix(
                        +self[1, 1], -self[0, 1],
                        -self[1, 0], +self[0, 0]
                    );
                } else {
                    return inverse_determinant * impl::matrix(
                        impl::matrix(
                            self[1, 1], self[1, 2],
                            self[2, 1], self[2, 2]
                        ).determinant(),

                        impl::matrix(
                            self[0, 2], self[0, 1],
                            self[2, 2], self[2, 1]
                        ).determinant(),

                        impl::matrix(
                            self[0, 1], self[0, 2],
                            self[1, 1], self[1, 2]
                        ).determinant(),

                        /* NOTE: New row. */
                        impl::matrix(
                            self[1, 2], self[1, 0],
                            self[2, 2], self[2, 0]
                        ).determinant(),

                        impl::matrix(
                            self[0, 0], self[0, 2],
                            self[2, 0], self[2, 2]
                        ).determinant(),

                        impl::matrix(
                            self[0, 2], self[0, 0],
                            self[1, 2], self[1, 0]
                        ).determinant(),

                        /* NOTE: New row. */
                        impl::matrix(
                            self[1, 0], self[1, 1],
                            self[2, 0], self[2, 1]
                        ).determinant(),

                        impl::matrix(
                            self[0, 1], self[0, 0],
                            self[2, 1], self[2, 0]
                        ).determinant(),

                        impl::matrix(
                            self[0, 0], self[0, 1],
                            self[1, 0], self[1, 1]
                        ).determinant()
                    );
                }
            }
        };

        template<typename... Args>
        requires (sizeof...(Args) == 2 * 2)
        matrix(Args... args) -> matrix<2, 2>;

        template<typename... Args>
        requires (sizeof...(Args) == 3 * 3)
        matrix(Args... args) -> matrix<3, 3>;

    }

    template<color::white_point WhitePoint>
    struct xyz {
        std::float32_t x;
        std::float32_t y;
        std::float32_t z;

        template<color::rgb_space Space>
        static constexpr impl::matrix<3, 3> _xyz_to_linear_rgb() {
            /* NOTE: See http://brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html for more info. */

            const auto xr = Space.xr / Space.yr;
            const auto xg = Space.xg / Space.yg;
            const auto xb = Space.xb / Space.yb;

            const auto yr = 1.0f32;
            const auto yg = 1.0f32;
            const auto yb = 1.0f32;

            const auto zr = (1 - Space.xr - Space.yr) / Space.yr;
            const auto zg = (1 - Space.xg - Space.yg) / Space.yg;
            const auto zb = (1 - Space.xb - Space.yb) / Space.yb;

            const auto [sr, sg, sb] = (
                impl::matrix(
                    xr, xg, xb,
                    yr, yg, yb,
                    zr, zg, zb
                )
                .inverse()
                .multiply_vector({
                    WhitePoint.x,
                    WhitePoint.y,
                    WhitePoint.z
                })
            );

            return impl::matrix(
                sr * xr, sg * xg, sb * xb,
                sr * yr, sg * yr, sb * yb,
                sr * zr, sg * zg, sb * zb
            ).inverse();
        }

        static constexpr std::float32_t _linear_to_gamma(const std::float32_t component) {
            if (component <= 0.0031308f32) {
                return 12.92f32 * component;
            }

            return 1.055f32 * std::pow(component, 1.0f32 / 2.4f32) - 0.055f32;
        }

        static constexpr std::uint8_t _to_octet(const std::float32_t component) {
            /*
                NOTE: We use the same algorithm as the 'palette' rust crate.

                Their algorithm is apparently taken from "Hacker's Delight"
                pages 378-380.

                This gets us to where 'lch(100.0, 0.0, 0.0)'' will be converted
                to 'rgb(255, 255, 255)', which I am happy with.
            */

            /* NOTE: '2^23' in 'std::float32_t' bits. */
            static constexpr auto C23_u32 = std::uint32_t{0x4b00'0000};
            static constexpr auto C23_f32 = std::bit_cast<std::float32_t>(C23_u32);

            const auto scaled  = 255.0f32 * std::clamp(component, std::float32_t{0.0f32}, std::float32_t{1.0f32});
            const auto shifted = scaled + C23_f32;

            return static_cast<std::uint8_t>(
                std::sub_sat(std::bit_cast<std::uint32_t>(shifted), C23_u32)
            );
        }

        template<color::rgb_space Space = color::rgb_space::standard>
        constexpr color::rgb rgb(this const xyz self) {
            static constexpr auto XyzToLinearRGB = _xyz_to_linear_rgb<Space>();

            const auto [linear_r, linear_g, linear_b] = XyzToLinearRGB.multiply_vector({
                self.x,
                self.y,
                self.z
            });

            const auto gamma_r = _linear_to_gamma(linear_r);
            const auto gamma_g = _linear_to_gamma(linear_g);
            const auto gamma_b = _linear_to_gamma(linear_b);

            return {
                _to_octet(gamma_r),
                _to_octet(gamma_g),
                _to_octet(gamma_b)
            };
        }

        constexpr bool operator ==(const xyz &) const = default;
    };

    struct lab {
        std::float32_t l;
        std::float32_t a;
        std::float32_t b;

        static constexpr std::float32_t _process_component(const std::float32_t component) {
            static constexpr std::float32_t Epsilon = 6.0f32 / 29.0f32;
            static constexpr std::float32_t Delta   = 4.0f32 / 29.0f32;

            /* NOTE: Kappa is equivalent to '3.0f32 * std::pow(Epsilon, 2.0f32)'. */
            static constexpr std::float32_t Kappa = 108.0f32 / 841.0f32;

            if (component > Epsilon) {
                return std::pow(component, 3.0f32);
            }

            return Kappa * (component - Delta);
        }

        template<color::white_point WhitePoint = color::white_point::d65>
        constexpr color::xyz<WhitePoint> xyz(this const lab self) {
            const auto y = (self.l + 16.0f32) / 116.0f32;
            const auto x = y + (self.a / 500.0f32);
            const auto z = y - (self.b / 200.0f32);

            return {
                WhitePoint.x * _process_component(x),
                WhitePoint.y * _process_component(y),
                WhitePoint.z * _process_component(z)
            };
        }

        constexpr bool operator ==(const lab &) const = default;
    };

    struct lch {
        static const lch white;
        static const lch black;

        std::float32_t l;
        std::float32_t c;
        std::float32_t h;

        constexpr color::lab lab(this const lch self) {
            static constexpr std::float32_t DegreesToRadians = std::numbers::pi_v<std::float32_t> / 180.0f32;

            return {
                self.l,
                self.c * std::cos(self.h * DegreesToRadians),
                self.c * std::sin(self.h * DegreesToRadians),
            };
        }

        constexpr color::rgb rgb(this const lch self) {
            return self.lab().xyz().rgb();
        }

        constexpr bool operator ==(const lch &) const = default;
    };

    constexpr color::lch color::lch::white = {100.0f32, 0.0f32, 0.0f32};
    constexpr color::lch color::lch::black = {0.0f32,   0.0f32, 0.0f32};

    static_assert(color::lch::white.lab() == color::lab{100.0f32, 0.0f32, 0.0f32});

    static_assert(color::lch::white.rgb() == color::rgb::white);
    static_assert(color::lch::black.rgb() == color::rgb::black);

}
