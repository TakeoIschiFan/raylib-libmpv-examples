# Raylib libmpv examples

Embed a video container in a Raylib project via libmpv. These examples are very much experimental for now.

## Building

To connect libmpv to the Raylib `rlgl.h` backend we need a specific function that is only available in the Raylib master branch but not yet included in the last major release as of writing (version 5.5). So please consult the [Raylib documentation](https://github.com/raysan5/raylib/?tab=readme-ov-file#build-and-installation) for building raylib from source.

Building and linking using my system libmpv and pkg-config works fine.

```sh
RAYLIB_DIR=/path/to/local/raylib

cc raylib_libmpv_video_basic_example.c \
   -I$RAYLIB_DIR/include $(pkg-config --cflags mpv) \
   -o example \
   -L$RAYLIB_DIR/lib -Wl,-rpath,$RAYLIB_DIR/lib \
   -lraylib -lm $(pkg-config --libs mpv)

```
