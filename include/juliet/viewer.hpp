#pragma once

#include <juliet/common.hpp>
#include <juliet/sets.hpp>
#include <juliet/rgb_renderer.hpp>
#include <juliet/renderer_thread_pool.hpp>

namespace juliet {

    /* TODO: Separate set APIs more. */

    namespace impl {

        template<typename Generator, typename Duration>
        using generated_set = decltype(auto{
            std::invoke(std::declval<Generator &>(), std::declval<Duration>())
        });

        template<typename Generator, typename Duration>
        concept set_generator = (
            std::movable<Generator> &&

            std::regular_invocable<Generator &, const Duration &> &&

            /* For initialization. */
            std::regular_invocable<Generator &, Duration> &&

            std::same_as<
                impl::generated_set<Generator, const Duration &>,
                impl::generated_set<Generator, Duration>
            > &&

            juliet::iterative_set<impl::generated_set<Generator, Duration>>
        );

        template<typename Generator, typename Duration>
        struct update_data {
            [[no_unique_address]] Generator generator;

            bool paused = false;

            Duration elapsed_time = Duration::zero();

            constexpr update_data(Generator generator)
            :
                generator(std::move(generator))
            {}

            constexpr decltype(auto) generate_set(this update_data &self) {
                return std::invoke(self.generator, std::as_const(self.elapsed_time));
            }

            constexpr void toggle_pause(this update_data &self) {
                self.paused = !self.paused;
            }
        };

        struct empty_type {
            constexpr empty_type() = default;

            constexpr empty_type(auto &&...) {}
        };

        /* Used for window event handling. */
        template<typename... Callables>
        struct overloaded : Callables... {
            using Callables::operator ()...;
        };

    }

    template<typename SetOrGenerator>
    requires (juliet::iterative_set<SetOrGenerator> || impl::set_generator<SetOrGenerator, std::chrono::steady_clock::duration>)
    struct viewer {
        static constexpr bool _is_static_set = juliet::iterative_set<SetOrGenerator>;

        using _duration = std::chrono::steady_clock::duration;

        using set = decltype([]() {
            if constexpr (_is_static_set) {
                return std::type_identity<SetOrGenerator>{};
            } else {
                return std::type_identity<impl::generated_set<SetOrGenerator, _duration>>{};
            }
        }())::type;

        using _update_data = decltype([]() {
            if constexpr (_is_static_set) {
                return std::type_identity<impl::empty_type>{};
            } else {
                return std::type_identity<impl::update_data<SetOrGenerator, _duration>>{};
            }
        }())::type;

        static constexpr const char *save_location = "out.png";

        static constexpr juliet::coord high_res_scale = 6;

        struct _translation_info {
            struct offset {
                juliet::coord offset_x;
                juliet::coord offset_y;
            };

            bool _is_translating = false;
            std::int32_t _last_x;
            std::int32_t _last_y;

            constexpr bool is_translating(this const _translation_info self) {
                return self._is_translating;
            }

            constexpr void begin_translating(this _translation_info &self, const std::int32_t orig_x, const std::int32_t orig_y) {
                self._is_translating = true;

                self._last_x = orig_x;
                self._last_y = orig_y;
            }

            constexpr void end_translating(this _translation_info &self) {
                self._is_translating = false;
            }

            constexpr offset update(this _translation_info &self, const std::int32_t new_x, const std::int32_t new_y) {
                const auto shift = offset{new_x - self._last_x, new_y - self._last_y};

                self._last_x = new_x;
                self._last_y = new_y;

                return shift;
            }
        };

        [[no_unique_address]] set _set;
        [[no_unique_address]] _update_data _update_info;

        sf::RenderWindow _window;

        juliet::rgba_renderer _renderer;
        juliet::renderer_thread_pool _pool;

        bool _fine_controls = false;

        _translation_info _translation;

        viewer(
            const std::uint32_t width,
            const std::uint32_t height,

            SetOrGenerator set,

            const std::size_t num_threads = std::thread::hardware_concurrency(),

            const sf::String &title = "Juliet"
        )
        requires (_is_static_set)
        :
            _set(std::move(set)),
            _window(sf::VideoMode({width, height}), title),
            _renderer(juliet::resolution{width, height}),
            _pool(num_threads)
        {}

        viewer(
            const std::uint32_t width,
            const std::uint32_t height,

            SetOrGenerator generator,

            const std::size_t num_threads = std::thread::hardware_concurrency(),

            const sf::String &title = "Juliet"
        )
        requires (!_is_static_set)
        :
            _set(std::invoke(generator, _duration::zero())),
            _update_info(std::move(generator)),
            _window(sf::VideoMode({width, height}), title),
            _renderer(juliet::resolution{width, height}),
            _pool(num_threads)
        {}

        void toggle_fine_controls(this viewer &self) {
            self._fine_controls = !self._fine_controls;
        }

        void update(this viewer &self) requires (!_is_static_set) {
            self._set = self._update_info.generate_set();
        }

        void move_forward(this viewer &self, const _duration added_time) requires (!_is_static_set) {
            self._update_info.elapsed_time += added_time;

            self.update();
        }

        void move_backward(this viewer &self, const _duration removed_time) requires (!_is_static_set) {
            self._update_info.elapsed_time -= removed_time;

            self.update();
        }

        juliet::resolution resolution(this const viewer &self) {
            return self._renderer.resolution();
        }

        void save(this const viewer &self) {
            self._renderer.save_png(save_location);
        }

        void high_res_save(this viewer &self) {
            const auto frame = self._renderer.frame();

            auto high_res_renderer = juliet::rgb_renderer(self.resolution().scale(high_res_scale), juliet::frame{
                frame.center,
                frame.pixel_scale / static_cast<juliet::scalar>(high_res_scale)
            });

            self._pool.threaded_render_by_iteration(high_res_renderer, self._set);

            high_res_renderer.save_png(save_location);
        }

        void reset_frame(this viewer &self) {
            self._renderer.set_complete_frame();
        }

        void _translate_frame(this viewer &self, const _translation_info::offset shift) {
            /* NOTE: We move the frame the opposite direction. */
            self._renderer.translate_frame_by_coords(-shift.offset_x, -shift.offset_y);
        }

        void _translate_pixels_and_update(this viewer &self, const _translation_info::offset shift) {
            self._renderer.translate_pixels_by_coords(shift.offset_x, shift.offset_y);

            self._pool.threaded_render_missing_edges_by_iteration(
                self._renderer,
                self._set,
                shift.offset_x,
                shift.offset_y
            );

            self.update_window();
        }

        bool _translate(this viewer &self, const _translation_info::offset shift) {
            /* Returns whether a new draw should be requested. */

            self._translate_frame(shift);

            if constexpr (!_is_static_set) {
                /*
                    If we're not paused we'll have to redraw the
                    frame anyways, so don't bother shuffling pixels.
                */
                if (!self._update_info.paused) {
                    return true;
                }
            }

            if (std::abs(shift.offset_x) >= self.resolution().width()) {
                return true;
            }

            if (std::abs(shift.offset_y) >= self.resolution().height()) {
                return true;
            }

            self._translate_pixels_and_update(shift);

            return false;
        }

        void update_window(this viewer &self) {
            const auto [width, height] = self.resolution();

            auto texture = sf::Texture({
                static_cast<std::uint32_t>(width),
                static_cast<std::uint32_t>(height)
            });

            texture.update(
                /* NOTE: 'reinterpret_cast' does not make me happy. */
                reinterpret_cast<const std::uint8_t *>(
                    self._renderer.pixels().data()
                )
            );

            const auto sprite = sf::Sprite(texture);

            self._window.draw(sprite);
            self._window.display();
        }

        void draw(this viewer &self) {
            self._pool.threaded_render_by_iteration(self._renderer, self._set);

            self.update_window();
        }

        bool handle_key_pressed(this viewer &self, const sf::Event::KeyPressed &event) {
            switch (event.code) {
                case sf::Keyboard::Key::S: {
                    if (event.shift) {
                        self.high_res_save();
                    } else {
                        self.save();
                    }
                } break;

                case sf::Keyboard::Key::R: {
                    self.reset_frame();

                    return true;
                } break;

                case sf::Keyboard::Key::Space: {
                    if constexpr (!_is_static_set) {
                        self._update_info.toggle_pause();
                    }
                } break;

                case sf::Keyboard::Key::Enter: {
                    if constexpr (!_is_static_set) {
                        if (!self._update_info.paused) {
                            break;
                        }

                        const auto step = [&]() {
                            static constexpr auto BigStep   = std::chrono::duration_cast<_duration>(std::chrono::milliseconds(10));
                            static constexpr auto SmallStep = std::chrono::duration_cast<_duration>(std::chrono::milliseconds(1));

                            if (self._fine_controls) {
                                return SmallStep;
                            }

                            return BigStep;
                        }();

                        if (event.shift) {
                            self.move_backward(step);
                        } else {
                            self.move_forward(step);
                        }

                        return true;
                    }
                } break;

                default: break;
            }

            return false;
        }

        bool handle_event(this viewer &self, const sf::Event &event) {
            return event.visit(impl::overloaded{
                [&](sf::Event::Closed) {
                    self._window.close();

                    return false;
                },

                [&](const sf::Event::Resized event) {
                    /* TODO: Shuffle pixels and render edges like with translation? Probably not. */
                    self._renderer.resize(juliet::resolution{event.size.x, event.size.y});

                    self._window.setView(sf::View(
                        sf::FloatRect(
                            {0, 0},

                            {
                                static_cast<float>(event.size.x),
                                static_cast<float>(event.size.y)
                            }
                        )
                    ));

                    return true;
                },

                [&](const sf::Event::MouseWheelScrolled event) {
                    if (event.wheel != sf::Mouse::Wheel::Vertical) {
                        return false;
                    }

                    const auto scale = [&]() -> juliet::scalar {
                        if (self._fine_controls) {
                            return 0.99;
                        }

                        return 0.9;
                    }();

                    if (event.delta > 0) {
                        self._renderer.scale_pixel_width(scale);
                    } else {
                        self._renderer.unscale_pixel_width(scale);
                    }

                    return true;
                },

                [&](const sf::Event::MouseButtonPressed event) {
                    switch (event.button) {
                        case sf::Mouse::Button::Left: {
                            self._translation.begin_translating(event.position.x, event.position.y);
                        } break;

                        case sf::Mouse::Button::Right: {
                            const auto [width, height] = self.resolution();

                            return self._translate({
                                width  / 2z - static_cast<juliet::coord>(event.position.x),
                                height / 2z - static_cast<juliet::coord>(event.position.y)
                            });
                        } break;

                        case sf::Mouse::Button::Middle: {
                            self.toggle_fine_controls();
                        } break;

                        default: break;
                    }

                    return false;
                },

                [&](const sf::Event::MouseButtonReleased event) {
                    if (event.button == sf::Mouse::Button::Left) {
                        self._translation.end_translating();
                    }

                    return false;
                },

                [&](const sf::Event::MouseMoved event) {
                    if (!self._translation.is_translating()) {
                        return false;
                    }

                    const auto shift = self._translation.update(event.position.x, event.position.y);

                    return self._translate(shift);
                },

                [&](const sf::Event::KeyPressed &event) {
                    return self.handle_key_pressed(event);
                },

                [](auto &&) {
                    return false;
                }
            });
        }

        void run(this viewer &self) {
            auto last_time = []() {
                if constexpr (_is_static_set) {
                    return impl::empty_type{};
                } else {
                    return std::chrono::steady_clock::now();
                }
            }();

            while (self._window.isOpen()) {
                bool request_draw = false;

                while (true) {
                    const auto event = self._window.pollEvent();
                    if (!event.has_value()) {
                        break;
                    }

                    request_draw = (request_draw || self.handle_event(*event));
                }

                if constexpr (_is_static_set) {
                    if (request_draw) {
                        self.draw();
                    }
                } else {
                    const auto now = std::chrono::steady_clock::now();

                    if (!self._update_info.paused) {
                        self.move_forward(now - last_time);

                        self.draw();
                    } else if (request_draw) {
                        self.draw();
                    }

                    last_time = now;
                }

            }
        }
    };

}
