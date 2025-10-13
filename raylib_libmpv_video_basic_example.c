#include "mpv/client.h"
#include "mpv/render.h"
#include "mpv/render_gl.h"
#include "raylib.h"
#include "rlgl.h"
#include <stdio.h>

/*
    In this basic example, we setup the boilerplate required to draw a video in a Raylib RenderTexture.
*/

/* Unused */
static void on_mpv_render_updates(void* ctx) { (void)ctx; }
static void on_mpv_events(void* ctx) {}

/* Tell MPV where to find OpenGL functions */
static void* get_mpv_gl_proc_address(void* ctx, const char* name) {
    (void)ctx;
    return (void*)rlGetProcAddress(name);
}

int main(void) {

    const char* videoPath = "./test.mp4";
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Raylib + MPV");

    /* IMPORTANT! Raylib will not render correctly unless you disable this */
    rlDisableBackfaceCulling();

    /* Create GPU framebuffer for MPV to render into */
    RenderTexture2D mpv_tex = LoadRenderTexture(screenWidth, screenHeight);

    /* Create MPV context */
    mpv_handle* mpv_ctx = mpv_create();
    if (!mpv_ctx) {
        fprintf(stderr, "Could not create libmpv wrapper, Bailing.\n");
        CloseWindow();
        return -1;
    }

    mpv_set_option_string(mpv_ctx, "vo", "libmpv");  // required
    mpv_set_option_string(mpv_ctx, "hwdec", "auto"); // set "no" for software rendering
    // if you want more debugging
    // mpv_set_option_string(mpv_ctx, "msg-level", "all=v");
    // mpv_set_option_string(mpv_ctx, "terminal", "yes");

    /* Initialize MPV context */
    int error = mpv_initialize(mpv_ctx);
    if (error != MPV_ERROR_SUCCESS) {
        fprintf(stderr, "Could not initialize libmpv wrapper: %s, Bailing.\n", mpv_error_string(error));
        mpv_destroy(mpv_ctx);
        CloseWindow();
        return -1;
    }

    /* Initialize MPV OpenGL rendering context */
    mpv_opengl_init_params gl_init = {
        .get_proc_address = get_mpv_gl_proc_address, // callback we defined earlier
    };

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_OPENGL},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init},
        {MPV_RENDER_PARAM_ADVANCED_CONTROL, &(int){0}}, // See the advanced example for advanced control.
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

    /* Set MPV and render callbacks. We will not use these in this basic example, but setting them is recommended as the
     * event queues can fill if you do not flush them, leading to unexpected behavior. */
    mpv_render_context_set_update_callback(mpv_render_ctx, on_mpv_render_updates, NULL);
    mpv_set_wakeup_callback(mpv_ctx, on_mpv_events, NULL);

    /* Load video file */
    const char* cmd[] = {"loadfile", videoPath, NULL};
    mpv_command(mpv_ctx, cmd);

    while (!WindowShouldClose()) {

        /*
            Render the MPV video into the buffer we created earlier.
            If you just want to render in the default Raylib window, you an skip creating a RenderTexture and just pass
           .fbo = 0. You do not need BeginTextureMode(), MPV sets up the correct openGL state itself.

            mpv_render_context_render will block until a frame becomes available, essentially locking your game
           framerate to the video framerate. See the advanced example for custom frame timings.
        */
        mpv_opengl_fbo mpv_fbo = {
            .fbo = mpv_tex.id,
            .w = screenWidth,
            .h = screenHeight,
        };
        int flip_y = 0;
        mpv_render_param render_params[] = {
            {MPV_RENDER_PARAM_OPENGL_FBO, &mpv_fbo}, {MPV_RENDER_PARAM_FLIP_Y, &flip_y}, {0}};

        mpv_render_context_render(mpv_render_ctx, render_params);
        rlViewport(0, 0, screenWidth, screenHeight);

        BeginDrawing();
        // drawing
        DrawTextureRec(mpv_tex.texture, (Rectangle){0.f, 0.f, screenWidth, screenHeight}, (Vector2){0, 0}, WHITE);
        ClearBackground(RAYWHITE);
        EndDrawing();
    }

    /* Free the MPV resources before exiting */
    UnloadRenderTexture(mpv_tex);
    mpv_render_context_free(mpv_render_ctx);
    mpv_destroy(mpv_ctx);
    CloseWindow();
    return 0;
}
