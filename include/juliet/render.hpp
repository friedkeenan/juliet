#pragma once

#include <juliet/common.hpp>
#include <juliet/sets.hpp>

namespace juliet {

    using coord = decltype(0z);

    struct coords {
        static constexpr auto rectangle(const coords top_left, const coords bottom_right) {
            return (
                std::views::cartesian_product(
                    std::views::iota(top_left.y, bottom_right.y),
                    std::views::iota(top_left.x, bottom_right.x)
                ) |

                std::views::transform([](const auto flipped_coords) {
                    const auto &[y, x] = flipped_coords;

                    return coords{x, y};
                })
            );
        }

        juliet::coord x;
        juliet::coord y;
    };

    template<typename Region>
    concept screen_region = (
        std::ranges::forward_range<Region> &&

        std::ranges::sized_range<Region> &&

        std::convertible_to<std::ranges::range_reference_t<Region>, juliet::coords>
    );

    static_assert(juliet::screen_region<decltype(juliet::coords::rectangle({0z, 0z}, {0z, 0z}))>);

    struct resolution {
        juliet::coord _width;
        juliet::coord _height;

        constexpr juliet::coord width(this const resolution self) {
            return self._width;
        }

        constexpr juliet::coord height(this const resolution self) {
            return self._height;
        }

        constexpr juliet::coord min_length(this const resolution self) {
            return std::min(self.width(), self.height());
        }

        constexpr juliet::coord area(this const resolution self) {
            return self.width() * self.height();
        }

        constexpr resolution scale(this const resolution self, const juliet::coord scale) {
            return {
                scale * self.width(),
                scale * self.height()
            };
        }

        constexpr auto x_coords(this const resolution self) {
            return std::views::iota(0z, self.width());
        }

        constexpr auto y_coords(this const resolution self) {
            return std::views::iota(0z, self.height());
        }

        constexpr juliet::screen_region auto screen_coords(this const resolution self) {
            return juliet::coords::rectangle({0z, 0z}, {self.width(), self.height()});
        }

        constexpr juliet::coords to_graphwise_coord(this const resolution self, const juliet::coords coords) {
            return {
                 coords.x - (self.width()  / 2),
                 coords.y - (self.height() / 2)
            };
        }
    };

    struct frame {
        juliet::complex center;
        juliet::scalar  pixel_scale;

        static constexpr frame complete(const juliet::resolution resolution) {
            return {
                juliet::complex{},

                4.0_scalar / static_cast<juliet::scalar>(resolution.min_length())
            };
        }

        constexpr juliet::complex number_at_screen_coords(this const frame &self, const juliet::resolution resolution, const juliet::coords coords) {
            const auto graph = resolution.to_graphwise_coord(coords);

            return (
                self.center +

                self.pixel_scale * juliet::complex{
                    static_cast<juliet::scalar>(graph.x),
                    static_cast<juliet::scalar>(graph.y)
                }
            );
        }
    };

    struct frame_renderer_interface;

    namespace impl {

        template<typename Renderer>
        concept frame_renderer = (
            std::derived_from<Renderer, juliet::frame_renderer_interface> &&

            std::movable<Renderer> &&

            requires {
                { Renderer::max_iterations } -> std::same_as<const std::size_t &>;

                typename Renderer::color;
            } &&

            requires(const Renderer &renderer) {
                { renderer.resolution() } -> std::same_as<juliet::resolution>;
                { renderer.frame() }      -> std::same_as<juliet::frame>;
            } &&

            requires(Renderer &renderer, const juliet::coords &coords) {
                /* NOTE: Should also be thread-safe. */
                renderer.set_pixel(coords, std::declval<typename Renderer::color>());
            }
        );

        template<typename Renderer>
        concept iterative_frame_renderer = (
            impl::frame_renderer<Renderer> &&

            requires(const Renderer &renderer, const std::size_t &iterations) {
                /*
                    NOTE: We pass the max iterations as a template
                    parameter so that more-derived classes can
                    configure it without the implementer having to
                    account for that themselves.

                    It could also be useful in the future if we
                    implement different max iterations at different
                    zoom levels, for example.
                */
                { renderer.template color_for_iterations<Renderer::max_iterations>(iterations) } -> std::same_as<typename Renderer::color>;
            }
        );

    }

    template<typename Renderer>
    concept frame_renderer = impl::frame_renderer<std::remove_cvref_t<Renderer>>;

    template<typename Renderer>
    concept iterative_frame_renderer = impl::iterative_frame_renderer<std::remove_cvref_t<Renderer>>;

    struct frame_renderer_interface {
        static constexpr std::size_t max_iterations = 500;

        constexpr void render_by_iteration_at(
            this juliet::iterative_frame_renderer auto &self,
            const juliet::coords coords,
            const juliet::iterative_set auto &set
        ) {
            static constexpr auto MaxIterations = std::remove_cvref_t<decltype(self)>::max_iterations;

            const auto frame      = std::as_const(self).frame();
            const auto resolution = std::as_const(self).resolution();

            const auto num = frame.number_at_screen_coords(resolution, coords);

            const auto iterations = set.template iterations_before_escape<MaxIterations>(num);

            self.set_pixel(coords, std::as_const(self).template color_for_iterations<MaxIterations>(iterations));
        }

        constexpr void render_by_iteration(this juliet::iterative_frame_renderer auto &self, const juliet::iterative_set auto &set) {
            self.render_region_by_iteration(self.resolution().screen_coords(), set);
        }

        constexpr void render_region_by_iteration(
            this juliet::iterative_frame_renderer auto &self,
            juliet::screen_region auto &&region,
            const juliet::iterative_set auto &set
        ) {
            static constexpr auto MaxIterations = std::remove_cvref_t<decltype(self)>::max_iterations;

            const auto frame      = std::as_const(self).frame();
            const auto resolution = std::as_const(self).resolution();

            for (const juliet::coords coords : std::forward<decltype(region)>(region)) {
                const auto num = frame.number_at_screen_coords(resolution, coords);

                const auto iterations = set.template iterations_before_escape<MaxIterations>(num);

                self.set_pixel(coords, std::as_const(self).template color_for_iterations<MaxIterations>(iterations));
            }
        }
    };

}
