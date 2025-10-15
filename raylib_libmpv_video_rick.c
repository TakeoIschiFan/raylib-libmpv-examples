#include "mpv/client.h"
#include "mpv/render.h"
#include "mpv/render_gl.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
    This example draws the gif in the readme.
*/

static bool render_event = false;
static double time_pos = 69.0;
static double duration = 420.0;

static void on_mpv_render_events(void* ctx) { render_event = true; }

static void on_mpv_events(void* ctx) {}

static void* get_mpv_gl_proc_address(void* ctx, const char* name) {
    (void)ctx;
    return (void*)rlGetProcAddress(name);
}

int main(void) {

    const char* videoPath = "./rick.mp4";
    const int screenWidth = 1280;
    const int screenHeight = 720;
    const int videoWidth = 192;
    const int videoHeight = 144;

    Vector2 rickPos = {((float)screenWidth / 2) - ((float)videoWidth / 2),
                       ((float)screenHeight / 2) - ((float)videoHeight / 2)};
    Vector2 rickVel = {3, 3};

    InitWindow(screenWidth, screenHeight, "Raylib + MPV");
    rlDisableBackfaceCulling();

    SetTargetFPS(60);

    RenderTexture2D mpv_tex = LoadRenderTexture(videoWidth, videoHeight);

    mpv_handle* mpv_ctx = mpv_create();
    if (!mpv_ctx) {
        fprintf(stderr, "Could not create libmpv wrapper, Bailing.\n");
        CloseWindow();
        return -1;
    }

    mpv_set_option_string(mpv_ctx, "vo", "libmpv");
    mpv_set_option_string(mpv_ctx, "hwdec", "auto");
    mpv_set_option_string(mpv_ctx, "vd-lavc-hdr", "yes");
    mpv_set_option_string(mpv_ctx, "video-timing-offset", "0");
    mpv_set_option_string(mpv_ctx, "msg-level", "all=v");
    mpv_set_option_string(mpv_ctx, "terminal", "yes");

    int error = mpv_initialize(mpv_ctx);
    if (error != MPV_ERROR_SUCCESS) {
        fprintf(stderr, "Could not initialize libmpv wrapper: %s, Bailing.\n", mpv_error_string(error));
        mpv_destroy(mpv_ctx);
        CloseWindow();
        return -1;
    }

    mpv_opengl_init_params gl_init = {
        .get_proc_address = get_mpv_gl_proc_address,
    };

    mpv_render_param params[] = {{MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_OPENGL},
                                 {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init},
                                 {MPV_RENDER_PARAM_BLOCK_FOR_TARGET_TIME, 0},    // unsetting render blocking for timing
                                 {MPV_RENDER_PARAM_ADVANCED_CONTROL, &(int){1}}, // Setting advanced control
                                 {0}};

    mpv_render_context* mpv_render_ctx = NULL;
    error = mpv_render_context_create(&mpv_render_ctx, mpv_ctx, params);
    if (error != MPV_ERROR_SUCCESS) {
        fprintf(stderr, "Could not initialize libmpv render context: %s, Bailing.", mpv_error_string(error));
        mpv_render_context_free(mpv_render_ctx);
        mpv_destroy(mpv_ctx);
        CloseWindow();
        return -1;
    }

    mpv_render_context_set_update_callback(mpv_render_ctx, on_mpv_render_events, NULL);
    mpv_set_wakeup_callback(mpv_ctx, on_mpv_events, mpv_ctx);

    const char* cmd[] = {"loadfile", videoPath, NULL};
    mpv_command_async(mpv_ctx, 0, cmd);

    while (!WindowShouldClose()) {

        // check for collisions with walls
        if (rickPos.y <= 0) { // top
            rickVel.y *= -1;
        }
        if (rickPos.y >= screenHeight - videoHeight) { // bottom
            rickVel.y *= -1;
        }
        if (rickPos.x <= 0) { // left
            rickVel.x *= -1;
        }
        if (rickPos.x >= screenWidth - videoWidth) { // right
            rickVel.x *= -1;
        }

        // updating at fixed times so no need for a deltatime
        rickPos = Vector2Add(rickPos, rickVel);

        if (render_event) {
            render_event = false;

            if (mpv_render_context_update(mpv_render_ctx) & MPV_RENDER_UPDATE_FRAME) {
                mpv_opengl_fbo mpv_fbo = {
                    .fbo = mpv_tex.id,
                    .w = videoWidth,
                    .h = videoHeight,
                };
                int flip_y = 1;
                mpv_render_param render_params[] = {
                    {MPV_RENDER_PARAM_OPENGL_FBO, &mpv_fbo}, {MPV_RENDER_PARAM_FLIP_Y, &flip_y}, {0}};

                mpv_render_context_render(mpv_render_ctx, render_params);
                rlViewport(0, 0, screenWidth, screenHeight);
                rlEnableColorBlend();
            }
        }
        BeginDrawing();
        ClearBackground(SKYBLUE);
        DrawTextureRec(mpv_tex.texture, (Rectangle){0.f, 0.f, videoWidth, -1.f * videoHeight}, rickPos, WHITE);
        EndDrawing();

        mpv_render_context_report_swap(mpv_render_ctx);
    }

    UnloadRenderTexture(mpv_tex);
    mpv_render_context_free(mpv_render_ctx);
    mpv_destroy(mpv_ctx);
    CloseWindow();
    return 0;
}
