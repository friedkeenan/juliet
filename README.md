# Juliet

A C++ library/tool/toy for rendering sets of complex numbers, like the Mandelbrot set.

## Explanation

This project originated out of a desire to compare how I'd write and enjoy writing a Rust project versus a C++ project. I settled on wanting the project to be about rendering the Mandelbrot set and Julia sets, since it seemed a good combination of things I'd need to do, and because I always have an urge to draw something mathematical and just stare at it.

As one can infer by the fact that there isn't any Rust code in this repo, I ended up preferring the C++ version of the project. Rust did have quite a few aspects to like: the rendering process, for instance, with the [winit](https://github.com/rust-windowing/winit) and [pixels](https://github.com/parasyte/pixels) crates was *basically* pleasant, and for that matter dependency management was a breeze.

The long and short of it though is that I just found writing the C++ code to be more fun, and more pleasing. With C++, I am able to write much more generic code, the *vast* majority of which can be transparently executed at compile time, and the language aids me in those things basically every step of the way; I don't recall ever feeling like it was fighting me in doing that. In Rust, I'd try to set a default color type for the `FrameRenderer` trait, I'd be told I can't do that yet. I'd try to mark methods in an implementation of the `IterativeSet` trait as `const`, I'd be told that's impossible. I felt less able to unify APIs, and that I had to write more boilerplate code, or at least boilerplate code that was less programmatic, that was more rote and menial. In C++, templates and concepts and constexpr are just uber-powerful, and I felt empowered by them in a way that I didn't feel when writing Rust.

The real thing that really sunk Rust for me though for this project was that I couldn't precompute at compile time the colors that corresponded to the iterations it took to know that a certain number didn't converge. I use an algorithm that's based on [LCH colors](https://en.wikipedia.org/wiki/Plotting_algorithms_for_the_Mandelbrot_set#LCH_coloring), which need to be converted to RGB colors to be rendered. Doing that involves floating-point operations, like sine and cosine and exponentiation, which aren't `const` in Rust, and there's nothing the (otherwise excellent) [palette](https://github.com/Ogeon/palette) crate can do to help with that.

My understanding is that Rust does not have these floating-point operations as `const` because there's concern that there could be inconsistency between the values obtained at compile time versus at runtime. It's not an unfair concern, but C++ already solved this and has these operations as `constexpr`, standardized in C++23 and C++26, and implemented as an extension in GCC for much longer before that. The way C++ solves it is by saying that, actually it's okay for the values to differ at compile time versus runtime. In fairness, C++ already has a built-in way to do this: `if consteval` (or `if (std::is_constant_evaluated())` in C++20), so it's not that huge of a leap for the language. But the way the C++ reasoning goes is that floating-point numbers can be thought of in two different ways: as a discrete point on a number line, or as an interval on a number line, with varying precision. C++ opts for the latter view, so that even if you do get a different bitwise value at compile time versus runtime, they still will represent the same *semantic* value. That to my mind makes more sense anyways, but more importantly it enables really desirable and useful functionality.

Precomputing the colors ended up saving a fairly noticeable amount of time rendering, and after I accomplished that I just kept writing more code, fleshing out the project more and more, which I did not set out to do initially. But writing the C++ code invigorated me, and it was engaging to keep improving it. So the Rust version of this project ended up just naturally falling to the wayside.

## Demo

To see a demo of Juliet, you can build the project using meson:
```sh
meson setup build && cd build
meson compile
```

This will run the code in the [main.cpp](https://github.com/friedkeenan/juliet/blob/main/source/main.cpp) file, which will run a `juliet::viewer`, opening a window for viewing the provided sets. See below for its controls.

## The Viewer Interface

A `juliet::viewer` has the following controls:

- Scroll up to zoom in, scroll down to zoom out.
- Left click and drag to move the frame.
- Right click to move the frame so that the clicked-on point is the new center.
- Middle click to toggle "fine controls", which makes zooming and stepping more precise.
- Press the `S` key to save the current frame as an `out.png` file in the current directory.
- Press `Shift+S` to save the current frame rendered at a 6x scale as an `out.png` file in the current directory.
- Press `R` to reset the current frame.
- Press the space bar to pause or unpause the viewer.
- Press `Enter` to step the viewer forward when paused.
- Press `Shift+Enter` to step the viewer backward when paused.

A main goal with Juliet's viewer interface is to make sure that all displayed pixels always represent a full-fidelity render. This is why zooming is often very slow, because every frame is sincerely rendered, whereas other viewers typically just scale up or down the already-rendered pixels, only re-rerendering when zooming is over, which lets them feel much snappier. Moving the frame around with Juliet's viewer should usually feel pretty okay though, as when the frame is moved, it just slides around the already-rendered pixels appropriately and only renders the missing edges.

## Credits

- [Aly](https://github.com/s5bug) for helping me understand color space math enough to implement converting LCH colors to RGB.
- [bshoshany/thread-pool](https://github.com/bshoshany/thread-pool) for being the library I use to manage thread pools.
- [richgel999/fpng](https://github.com/richgel999/fpng) for being the library I use to save PNGs.
- [SFML](https://www.sfml-dev.org/) for being the library I use to render to a window.
