#include <cstdio>
#include <numbers>
#include <juliet/juliet.hpp>

using namespace juliet::literals;

int main() {
    /* To just have a viewer for a static set, just pass it the set directly. */
    // auto app = juliet::viewer(512, 512, juliet::mandelbrot_set);

    /*
        To have a viewer for a set that changes based on the elapsed time,
        pass it a function which takes an elapsed duration and returns a set.
    */
    auto app = juliet::viewer(512, 512, [](const auto elapsed) {
        /*
            Generate Julia sets whose constants lay on the outer
            edge of the disk of the Mandelbrot set centered on -1.
        */

        /* The period, in seconds. */
        static constexpr auto Period = 240.0_scalar;

        const auto scalar_duration = std::chrono::duration_cast<
            std::chrono::duration<juliet::scalar>
        >(elapsed).count();

        const auto t     = scalar_duration / Period;
        const auto theta = 2.0_scalar * std::numbers::pi_v<juliet::scalar> * t;

        const auto c = juliet::complex{
            0.25_scalar * std::cos(theta) - 1.0_scalar,
            0.25_scalar * std::sin(theta)
        };

        return juliet::quadratic_julia_set{c};
    });

    /* Run the viewer. */
    app.run();
}
