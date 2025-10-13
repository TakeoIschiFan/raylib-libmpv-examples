#include "mpv/client.h"
#include "mpv/render.h"
#include "mpv/render_gl.h"
#include "raylib.h"
#include "rlgl.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
    In this advanced example, we show how to achieve custom frame timings,
   property observing and other advanced features using
   MPV_RENDER_PARAM_ADVANCED_CONTROL. Please read the libmpv client.h, render.h
   & render_gl.h  headers for more information.

    - You have to define a render callback (on_mpv_render_updates). This
   function can now be called even when there is no new frame.

    - When on_mpv_render_updates is triggered, you are supposed to call
   mpv_render_context_update ASAP to check (using a flag) if a frame is supposed to
   be drawn

    - Threading considerations if you are rendering on a seperate thread

        a) only one of the mpv_render_* functions can be called at the same time
        b) when calling mpv_render_* functions, the openGL context must be
   current
        c) *You cannot call any core MPV functions from a rendering thread*
   (except those explicitly marked safe)
        d) *You cannot call any render MPV
   functions from a callback*

    See the basic example for code left undocumented here.
*/

static bool render_event = false;
static double time_pos = 69.0;
static double duration = 420.0;

static void on_mpv_render_events(void* ctx) { render_event = true; }

static void on_mpv_events(void* ctx) {
    /* Note that blocking in this callback is bad form. The MPV header requests
     * you keep these callback functions lean and poll events from a different thread (like
     * we sketched for the on_mpv_render_updates function above), but this
     * approach works well enough for a simple app  */
    while (true) {
        mpv_event* event = mpv_wait_event((mpv_handle*)ctx, 0);
        if (event->event_id == MPV_EVENT_NONE) {
            break;
        }

        switch (event->event_id) {
            /* You receive MPV_EVENT_PROPERTY_CHANGE events for every mpv_observe_property you issue. */
            case MPV_EVENT_PROPERTY_CHANGE: {
                mpv_event_property* prop = (mpv_event_property*)event->data;
                if (strcmp(prop->name, "time-pos") == 0) {
                    if (prop->format == MPV_FORMAT_DOUBLE) {
                        time_pos = *(double*)prop->data;
                    }
                }
                if (strcmp(prop->name, "duration") == 0) {
                    if (prop->format == MPV_FORMAT_DOUBLE) {
                        duration = *(double*)prop->data;
                    }
                }
            } break;
            default: {
                /* Ignore other events*/
            }
        };
    }
}

static void* get_mpv_gl_proc_address(void* ctx, const char* name) {
    (void)ctx;
    return (void*)rlGetProcAddress(name);
}

int main(void) {

    const char* videoPath = "./test.mp4";
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Raylib + MPV");
    rlDisableBackfaceCulling();

    /* Uncoupling the video and game frame rate, so set a game frame rate here */
    SetTargetFPS(240);

    RenderTexture2D mpv_tex = LoadRenderTexture(screenWidth, screenHeight);

    mpv_handle* mpv_ctx = mpv_create();
    if (!mpv_ctx) {
        fprintf(stderr, "Could not create libmpv wrapper, Bailing.\n");
        CloseWindow();
        return -1;
    }

    mpv_set_option_string(mpv_ctx, "vo", "libmpv");
    mpv_set_option_string(mpv_ctx, "hwdec", "auto");
    mpv_set_option_string(mpv_ctx, "vd-lavc-hdr",
                          "yes"); // direct rendering, possible extra performance
    mpv_set_option_string(mpv_ctx, "video-timing-offset",
                          "0"); // audio video timing, we will time frames manually
    mpv_set_option_string(mpv_ctx, "msg-level", "all=v");
    mpv_set_option_string(mpv_ctx, "terminal", "yes");

    int error = mpv_initialize(mpv_ctx);
    if (error != MPV_ERROR_SUCCESS) {
        fprintf(stderr, "Could not initialize libmpv wrapper: %s, Bailing.\n", mpv_error_string(error));
        mpv_destroy(mpv_ctx);
        CloseWindow();
        return -1;
    }

    //
    mpv_observe_property(mpv_ctx, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv_ctx, 0, "time-pos", MPV_FORMAT_DOUBLE);

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
    /* async now required, as explained above */
    mpv_command_async(mpv_ctx, 0, cmd);

    while (!WindowShouldClose()) {
        if (render_event) {
            render_event = false;

            /* Call mpv_render_context_update and only render when the
             * MPV_RENDER_UPDATE_FRAME flag is set */
            if (mpv_render_context_update(mpv_render_ctx) & MPV_RENDER_UPDATE_FRAME) {
                mpv_opengl_fbo mpv_fbo = {
                    .fbo = mpv_tex.id,
                    .w = screenWidth,
                    .h = screenHeight,
                };
                int flip_y = 0;
                mpv_render_param render_params[] = {
                    {MPV_RENDER_PARAM_OPENGL_FBO, &mpv_fbo}, {MPV_RENDER_PARAM_FLIP_Y, &flip_y}, {0}};

                mpv_render_context_render(mpv_render_ctx, render_params);
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawTextureRec(mpv_tex.texture, (Rectangle){0.f, 0.f, screenWidth, screenHeight}, (Vector2){0, 0}, WHITE);
        /* Draw a progress bar using the properties we are observing */
        if (duration > 0.0) {
            float progress = (float)(time_pos / duration);
            float barHeight = 8.0f;
            float barWidth = screenWidth * progress;

            DrawRectangle(0, screenHeight - barHeight, screenWidth, barHeight, GRAY);
            DrawRectangle(0, screenHeight - barHeight, barWidth, barHeight, SKYBLUE);
        }

        EndDrawing();

        /* Report a frame to MPV, supposedly this can help with timing */
        mpv_render_context_report_swap(mpv_render_ctx);
    }

    UnloadRenderTexture(mpv_tex);
    mpv_render_context_free(mpv_render_ctx);
    mpv_destroy(mpv_ctx);
    CloseWindow();
    return 0;
}
