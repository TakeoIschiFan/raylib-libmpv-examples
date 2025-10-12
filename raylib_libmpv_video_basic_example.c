#include "mpv/client.h"
#include "mpv/render.h"
#include "mpv/render_gl.h"
#include "raylib.h"
#include "rlgl.h"
#include <stdio.h>

static void on_mpv_render_updates(void *ctx) { (void)ctx; }

static void on_mpv_events(void *ctx) {}

static void *get_mpv_gl_proc_address(void *ctx, const char *name) {
  (void)ctx;
  return (void *)rlGetProcAddress(name);
}

int main(void) {
  const char *videoPath = "./test.mp4";
  const int screenWidth = 1280;
  const int screenHeight = 720;

  InitWindow(screenWidth, screenHeight, "Raylib + MPV");
  rlDisableBackfaceCulling();

  RenderTexture2D mpv_tex = LoadRenderTexture(screenWidth, screenHeight);

  mpv_handle *mpv_ctx = mpv_create();
  if (!mpv_ctx) {
    fprintf(stderr, "Could not create libmpv wrapper, Bailing.\n");
    CloseWindow();
    return -1;
  }

  mpv_set_option_string(mpv_ctx, "vo", "libmpv");
  mpv_set_option_string(mpv_ctx, "hwdec", "auto");
  // if you want more debugging
  // mpv_set_option_string(mpv_ctx, "msg-level", "all=v");
  // mpv_set_option_string(mpv_ctx, "terminal", "yes");

  int error = mpv_initialize(mpv_ctx);
  if (error != MPV_ERROR_SUCCESS) {
    fprintf(stderr, "Could not initialize libmpv wrapper: %s, Bailing.\n",
            mpv_error_string(error));
    mpv_destroy(mpv_ctx);
    CloseWindow();
    return -1;
  }

  mpv_opengl_init_params gl_init = {
      .get_proc_address = get_mpv_gl_proc_address,
  };

  mpv_render_param params[] = {
      {MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_OPENGL},
      {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init},
      {MPV_RENDER_PARAM_ADVANCED_CONTROL, &(int){0}},
      {0}};

  mpv_render_context *mpv_render_ctx = NULL;
  error = mpv_render_context_create(&mpv_render_ctx, mpv_ctx, params);
  if (error != MPV_ERROR_SUCCESS) {
    fprintf(stderr, "Could not initialize libmpv render context: %s, Bailing.",
            mpv_error_string(error));
    mpv_destroy(mpv_ctx);
    mpv_render_context_free(mpv_render_ctx);
    CloseWindow();
    return -1;
  }

  mpv_render_context_set_update_callback(mpv_render_ctx, on_mpv_render_updates,
                                         NULL);
  mpv_set_wakeup_callback(mpv_ctx, on_mpv_events, NULL);

  const char *cmd[] = {"loadfile", videoPath, NULL};
  mpv_command_async(mpv_ctx, 0, cmd);

  while (!WindowShouldClose()) {

    mpv_render_context_update(mpv_render_ctx);

    mpv_opengl_fbo mpv_fbo = {
        .fbo = mpv_tex.id,
        .w = screenWidth,
        .h = screenHeight,
    };
    int flip_y = 1;
    mpv_render_param render_params[] = {{MPV_RENDER_PARAM_OPENGL_FBO, &mpv_fbo},
                                        {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
                                        {0}};

    mpv_render_context_render(mpv_render_ctx, render_params);

    BeginDrawing();
    DrawTextureRec(mpv_tex.texture,
                   (Rectangle){0.f, 0.f, screenWidth, -1.f * screenHeight},
                   (Vector2){0, 0}, WHITE);
    ClearBackground(RAYWHITE);
    EndDrawing();
  }

  mpv_render_context_free(mpv_render_ctx);
  mpv_destroy(mpv_ctx);
  CloseWindow();
  return 0;
}
