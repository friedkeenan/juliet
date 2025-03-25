#pragma once

#include <juliet/common.hpp>
#include <juliet/render.hpp>

namespace juliet {

    template<juliet::resolution Resolution>
    struct ascii_renderer: juliet::frame_renderer_interface {
        using color = bool;

        static constexpr char off_character = '.';
        static constexpr char on_character  = '#';

        std::array<color, Resolution.area()> _pixels = {};

        juliet::frame _frame = juliet::frame::complete(Resolution);

        constexpr ascii_renderer() = default;

        constexpr explicit ascii_renderer(juliet::frame frame) : _frame(frame) {}

        constexpr juliet::resolution resolution(this const ascii_renderer &) {
            return Resolution;
        }

        constexpr juliet::frame frame(this const ascii_renderer &self) {
            return self._frame;
        }

        constexpr color get_pixel(this const ascii_renderer &self, const juliet::coords coords) {
            return self._pixels[coords.y * Resolution.width() + coords.x];
        }

        constexpr void set_pixel(this ascii_renderer &self, const juliet::coords coords, const color color) {
            self._pixels[coords.y * Resolution.width() + coords.x] = color;
        }

        template<std::size_t MaxIterations>
        static constexpr color color_for_iterations(const std::size_t iterations) {
            return iterations == MaxIterations;
        }

        constexpr std::array<char, Resolution.area() + Resolution.height()> build_chars(this const ascii_renderer &self) {
            std::array<char, Resolution.area() + Resolution.height()> chars = {};

            auto next_char = chars.begin();
            for (const auto y : std::views::iota(0z, Resolution.height())) {
                for (const auto x : std::views::iota(0z, Resolution.width())) {
                    if (self.get_pixel(juliet::coords{x, y})) {
                        *next_char = on_character;
                    } else {
                        *next_char = off_character;
                    }

                    ++next_char;
                }

                *next_char = '\n';
                ++next_char;
            }

            return chars;
        }

        void print(this const ascii_renderer &self) {
            for (const auto y : std::views::iota(0z, Resolution.height())) {
                for (const auto x : std::views::iota(0z, Resolution.width())) {
                    if (self.get_pixel(juliet::coords{x, y})) {
                        std::putchar(on_character);
                    } else {
                        std::putchar(off_character);
                    }
                }

                std::putchar('\n');
            }
        }
    };

    namespace test {
        static_assert(juliet::iterative_frame_renderer<juliet::ascii_renderer<juliet::resolution{25, 25}>>);

        struct test_ascii_renderer: juliet::ascii_renderer<juliet::resolution{25, 25}> {
            static constexpr std::size_t max_iterations = 55;
        };

        consteval bool ascii_test() {
            auto renderer = test_ascii_renderer();
            renderer.render_by_iteration(juliet::mandelbrot_set);

            return std::string_view(renderer.build_chars()) == (
                ".........................\n"
                ".........................\n"
                ".........................\n"
                ".........................\n"
                ".........................\n"
                ".........................\n"
                ".........................\n"
                "...........#.............\n"
                "...........#.............\n"
                ".........#####...........\n"
                "........#######..........\n"
                ".....##.#######..........\n"
                "##############...........\n"
                ".....##.#######..........\n"
                "........#######..........\n"
                ".........#####...........\n"
                "...........#.............\n"
                "...........#.............\n"
                ".........................\n"
                ".........................\n"
                ".........................\n"
                ".........................\n"
                ".........................\n"
                ".........................\n"
                ".........................\n"
            );
        }

        static_assert(ascii_test());

    }

}
