#pragma once

#include <juliet/common.hpp>
#include <juliet/render.hpp>

namespace juliet {

    struct renderer_thread_pool {
        BS::light_thread_pool _threads;

        inline renderer_thread_pool(const std::size_t num_threads)
        :
            _threads(num_threads - 1)
        {
            [[assume(num_threads > 0)]];
        }

        void threaded_render_by_iteration(this renderer_thread_pool &self, juliet::iterative_frame_renderer auto &renderer, const juliet::iterative_set auto &set) {
            const auto num_tasks = static_cast<juliet::coord>(self._threads.get_thread_count());

            const auto resolution = renderer.resolution();

            const auto width = resolution.width();
            const auto pixels_per_thread = resolution.area() / (num_tasks + 1);

            for (const auto i : std::views::iota(0z, num_tasks)) {
                const auto start_index = (i + 0) * pixels_per_thread;
                const auto end_index   = (i + 1) * pixels_per_thread;

                self._threads.detach_task([&renderer, &set, start_index, end_index, width]() {
                    for (const auto index : std::views::iota(start_index, end_index)) {
                        const auto coords = juliet::coords{
                            index % width,
                            index / width,
                        };

                        renderer.render_by_iteration_at(coords, set);
                    }
                });
            }

            for (const auto index : std::views::iota(num_tasks * pixels_per_thread, resolution.area())) {
                const auto coords = juliet::coords{
                    index % width,
                    index / width,
                };

                renderer.render_by_iteration_at(coords, set);
            }

            self._threads.wait();
        }

        void threaded_render_region_by_iteration(
            this renderer_thread_pool &self,
            juliet::iterative_frame_renderer auto &renderer,
            juliet::screen_region auto &&region,
            const juliet::iterative_set auto &set
        ) {
            const auto num_tasks = static_cast<std::size_t>(self._threads.get_thread_count());

            const auto pixels_per_thread = std::ranges::size(region) / (num_tasks + 1);

            auto it = std::ranges::begin(region);
            for (const auto _ : std::views::iota(0uz, num_tasks)) {
                self._threads.detach_task([&renderer, &set, it, pixels_per_thread] {
                    for (const juliet::coords coords : std::views::counted(it, pixels_per_thread)) {
                        renderer.render_by_iteration_at(coords, set);
                    }
                });

                std::advance(it, pixels_per_thread);
            }

            for (const juliet::coords coords : std::ranges::subrange(it, std::ranges::end(region))) {
                renderer.render_by_iteration_at(coords, set);
            }

            self._threads.wait();
        }

        void threaded_render_missing_edges_by_iteration(
            this renderer_thread_pool &self,
            juliet::iterative_frame_renderer auto &renderer,
            const juliet::iterative_set auto &set,

            const juliet::coord offset_x,
            const juliet::coord offset_y
        ) {
            const auto resolution = renderer.resolution();

            if (std::abs(offset_x) >= resolution.width() || std::abs(offset_y) >= resolution.height()) {
                self.threaded_render_by_iteration(renderer, set);

                return;
            }

            /* NOTE: Could probably deduplicate code here but don't know that it'd be worth it. */

            if (offset_x > 0) {
                self.threaded_render_region_by_iteration(
                    renderer,

                    juliet::coords::rectangle(
                        {0z,       0z},
                        {offset_x, resolution.height()}
                    ),

                    set
                );

                if (offset_y > 0) {
                    self.threaded_render_region_by_iteration(
                        renderer,

                        juliet::coords::rectangle(
                            {offset_x,           0z},
                            {resolution.width(), offset_y}
                        ),

                        set
                    );
                } else if (offset_y < 0) {
                    self.threaded_render_region_by_iteration(
                        renderer,

                        juliet::coords::rectangle(
                            {offset_x,           resolution.height() + offset_y},
                            {resolution.width(), resolution.height()}
                        ),

                        set
                    );
                }
            } else if (offset_x < 0) {
                self.threaded_render_region_by_iteration(
                    renderer,

                    juliet::coords::rectangle(
                        {resolution.width() + offset_x, 0z},
                        {resolution.width(),            resolution.height()}
                    ),

                    set
                );

                if (offset_y > 0) {
                    self.threaded_render_region_by_iteration(
                        renderer,

                        juliet::coords::rectangle(
                            {0z,                            0z},
                            {resolution.width() + offset_x, offset_y}
                        ),

                        set
                    );
                } else if (offset_y < 0) {
                    self.threaded_render_region_by_iteration(
                        renderer,

                        juliet::coords::rectangle(
                            {0z,                            resolution.height() + offset_y},
                            {resolution.width() + offset_x, resolution.height()}
                        ),

                        set
                    );
                }
            } else {
                if (offset_y > 0) {
                    self.threaded_render_region_by_iteration(
                        renderer,

                        juliet::coords::rectangle(
                            {0z,                 0z},
                            {resolution.width(), offset_y}
                        ),

                        set
                    );
                } else if (offset_y < 0) {
                    self.threaded_render_region_by_iteration(
                        renderer,

                        juliet::coords::rectangle(
                            {0z,                 resolution.height() + offset_y},
                            {resolution.width(), resolution.height()}
                        ),

                        set
                    );
                }
            }
        }
    };

}
