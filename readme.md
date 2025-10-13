# Raylib libmpv examples

![Raylib libmpv example](rick.gif)

Embed a video container in a Raylib project via libmpv. These examples are very much experimental for now.

All examples are documented with comments.

- `raylib_libmpv_video_basic_example.c` renders a MPV video
- `raylib_libmpv_video_advanced_example.c` shows how to observe MPV properties, how to decouple the video and game render timings and how to use `MPV_RENDER_PARAM_ADVANCED_CONTROL`.
- `raylib_libmpv_video_rick.c` draws the example scene above.

## Issues

Raylib manages its own openGL state via its `rlgl.h` backend. The MPV renderer is opaque and does alter some of that openGL state, so some unexpected rendering artifacts could occur, which needs to be checked. So far I have found:

- Disable back-face culling with `rlDisableBackfaceCulling()` to get MPV to render to a custom FBO at all.
- Reset the viewport after every mpv draw call with `rlViewport()`.

## Building

We need a specific function that is only available in the Raylib master branch but not yet included in the last major release as of writing (version 5.5). So please consult the [Raylib documentation](https://github.com/raysan5/raylib/?tab=readme-ov-file#build-and-installation) for building raylib from source.

Building and linking using my system libmpv and pkg-config worked fine.

```sh
RAYLIB_DIR=/path/to/local/raylib

cc raylib_libmpv_video_basic_example.c \
   -I$RAYLIB_DIR/include $(pkg-config --cflags mpv) \
   -o example \
   -L$RAYLIB_DIR/lib -Wl,-rpath,$RAYLIB_DIR/lib \
   -lraylib -lm $(pkg-config --libs mpv)

```
