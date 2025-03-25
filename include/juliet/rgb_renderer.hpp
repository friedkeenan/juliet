#pragma once

#include <juliet/common.hpp>
#include <juliet/render.hpp>
#include <juliet/color.hpp>

#include <fpng.h>

namespace juliet {

    template<std::size_t MaxIterations>
    constexpr color::lch lch_color_for_iterations(const std::size_t iterations) {
        static constexpr std::float32_t Exponent = 0.01f32;

        if (iterations == MaxIterations) {
            return color::lch::black;
        }

        const auto s = std::pow(
            static_cast<std::float32_t>(iterations) / static_cast<std::float32_t>(MaxIterations),

            Exponent
        );

        const auto v = 1.0f32 - std::pow(std::cos(s * std::numbers::pi_v<std::float32_t>), 2);

        const auto luminance = static_cast<std::float32_t>(75.0f32 - 75.0f32 * v);

        return color::lch{
            luminance,

            28.0f32 + luminance,

            std::fmod(std::pow(360.0f32 * s, 1.5f32), 360.0f32)
        };
    }

    namespace impl {
        template<typename Color>
        requires (std::same_as<Color, color::rgb> || std::same_as<Color, color::rgba>)
        struct rgb_based_renderer: juliet::frame_renderer_interface {
            static constexpr bool HasAlpha = std::same_as<Color, color::rgba>;

            using color = Color;

            juliet::resolution _resolution;
            juliet::frame      _frame;

            std::vector<color> _pixels;

            constexpr explicit rgb_based_renderer(const juliet::resolution resolution)
            :
                rgb_based_renderer(resolution, juliet::frame::complete(resolution))
            {}

            constexpr rgb_based_renderer(const juliet::resolution resolution, const juliet::frame &frame)
            :
                _resolution(resolution),
                _frame(frame),
                _pixels(resolution.area(), color{})
            {}

            constexpr std::span<const color> pixels(this const rgb_based_renderer &self) {
                return std::span(self._pixels);
            }

            constexpr juliet::resolution resolution(this const rgb_based_renderer &self) {
                return self._resolution;
            }

            constexpr void resize(this rgb_based_renderer &self, const juliet::resolution resolution) {
                self._resolution = resolution;

                self._pixels.resize(resolution.area());
            }

            constexpr juliet::frame frame(this const rgb_based_renderer &self) {
                return self._frame;
            }

            constexpr void set_complete_frame(this rgb_based_renderer &self) {
                self._frame = juliet::frame::complete(self._resolution);
            }

            constexpr void translate_frame_by_coords(this rgb_based_renderer &self, const juliet::coord offset_x, const juliet::coord offset_y) {
                self._frame.center += self._frame.pixel_scale * juliet::complex{
                    static_cast<juliet::scalar>(offset_x),
                    static_cast<juliet::scalar>(offset_y)
                };
            }

            constexpr void translate_pixels_by_coords(this rgb_based_renderer &self, const juliet::coord offset_x, const juliet::coord offset_y) {
                if (std::abs(offset_x) >= self._resolution.width()) {
                    return;
                }

                if (std::abs(offset_y) >= self._resolution.height()) {
                    return;
                }

                /* TODO: Make better? */

                const auto set_pixels = [&](auto x_coords, auto y_coords) {
                    for (const auto [y, x] : std::views::cartesian_product(y_coords, x_coords)) {
                        auto new_coords = juliet::coords{
                            x + offset_x,
                            y + offset_y,
                        };

                        if (new_coords.x < 0 || new_coords.x >= self._resolution.width()) {
                            continue;
                        }

                        if (new_coords.y < 0 || new_coords.y >= self._resolution.height()) {
                            continue;
                        }

                        self.set_pixel(new_coords, self.get_pixel({x, y}));
                    }
                };

                if (offset_x <= 0 && offset_y <= 0) {
                    set_pixels(self._resolution.x_coords(), self._resolution.y_coords());
                } else if (offset_x <= 0 && offset_y > 0) {
                    set_pixels(self._resolution.x_coords(), self._resolution.y_coords() | std::views::reverse);
                } else if (offset_x > 0 && offset_y <= 0) {
                    set_pixels(self._resolution.x_coords() | std::views::reverse, self._resolution.y_coords());
                } else {
                    set_pixels(self._resolution.x_coords() | std::views::reverse, self._resolution.y_coords() | std::views::reverse);
                }
            }

            constexpr void scale_pixel_width(this rgb_based_renderer &self, const juliet::scalar amount) {
                self._frame.pixel_scale *= amount;
            }

            constexpr void unscale_pixel_width(this rgb_based_renderer &self, const juliet::scalar amount) {
                self._frame.pixel_scale /= amount;
            }

            constexpr color get_pixel(this const rgb_based_renderer &self, const juliet::coords coords) {
                return self._pixels[coords.y * self._resolution.width() + coords.x];
            }

            constexpr void set_pixel(this rgb_based_renderer &self, const juliet::coords coords, const color color) {
                self._pixels[coords.y * self._resolution.width() + coords.x] = color;
            }

            template<std::size_t MaxIterations>
            static constexpr color color_for_iterations(const std::size_t iterations) {
                static constexpr auto Colors = []() {
                    std::array<color, MaxIterations + 1> colors;

                    for (const auto iterations : std::views::iota(0uz, colors.size())) {
                        if constexpr (HasAlpha) {
                            colors[iterations] = juliet::lch_color_for_iterations<MaxIterations>(iterations).rgb().rgba();
                        } else {
                            colors[iterations] = juliet::lch_color_for_iterations<MaxIterations>(iterations).rgb();
                        }
                    }

                    return colors;
                }();

                return Colors[iterations];
            }

            void save_png(this const rgb_based_renderer &self, const char *path) {
                static constexpr auto NumChannels = []() -> std::uint32_t {
                    if constexpr (HasAlpha) {
                        return 4;
                    } else {
                        return 3;
                    }
                }();

                /* NOTE: It's okay to call this multiple times. */
                fpng::fpng_init();

                fpng::fpng_encode_image_to_file(
                    path,
                    self._pixels.data(),

                    static_cast<std::uint32_t>(self._resolution.width()),
                    static_cast<std::uint32_t>(self._resolution.height()),

                    NumChannels
                );
            }
        };
    }

    using rgb_renderer  = impl::rgb_based_renderer<color::rgb>;
    using rgba_renderer = impl::rgb_based_renderer<color::rgba>;

    static_assert(juliet::iterative_frame_renderer<juliet::rgb_renderer>);
    static_assert(juliet::iterative_frame_renderer<juliet::rgba_renderer>);

}
