#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "ascii.h"

#define WINDOW_SIZE 1200
#define FRAME_TIME (1000.0f / 1000.0f)

#define ERROR(fmt, ...) SDL_Log("ERROR: " fmt, ##__VA_ARGS__)
#define EXIT(code) ({SDL_Quit(); exit(code);})

// Global state

struct {
    float scale;
    int window_width;
    int window_height;
    int frame_w, frame_h;
    SDL_Texture *fbo;

    Uint64 time_prev;
    Uint64 time_delta;
} g_state = {0};

struct {
    bool ready;
    int resx;
    int resy;
    SDL_Camera *camera;
    SDL_CameraID cam_id;
    // video mode
} cam_state = {0};

SDL_Window *window;
SDL_Renderer *renderer;

bool open_camera(SDL_CameraID device) {
    if (cam_state.camera != NULL) SDL_CloseCamera(cam_state.camera);

    // select best format
    int format_count = 0;
    SDL_CameraSpec **formats = SDL_GetCameraSupportedFormats(device, &format_count);
    SDL_CameraSpec *best_format = NULL;
    if (formats == NULL) {
        ERROR("Couldn't get camera formats\n%s", SDL_GetError());
    } else {
        int best_fps = 0;
        for (int i = 0; i < format_count; i++) {
            SDL_CameraSpec *f = formats[i];
            int fps = f->framerate_numerator;
            if (fps > best_fps) {
                best_fps = fps;
                best_format = f;
            } else if (fps <= best_fps) break;
        }
    }

    cam_state.camera = SDL_OpenCamera(device, best_format);
    if (cam_state.camera == NULL) {
        SDL_free(formats);
        return false;
    }

    cam_state.ready = true;
    cam_state.resx = best_format->width;
    cam_state.resy = best_format->height;
    cam_state.cam_id = device;

    g_state.window_height = g_state.window_width * ((float)cam_state.resy / cam_state.resx);
    g_state.frame_w = (g_state.scale * cam_state.resx);
    g_state.frame_h = g_state.frame_w * ((float)cam_state.resy / cam_state.resx);

    SDL_free(formats);
    return true;
}

void init_camera() {
    int dev_count;
    SDL_CameraID *devices = SDL_GetCameras(&dev_count);

    cam_state.resx = g_state.window_width;
    cam_state.resy = g_state.window_height;
    cam_state.camera = NULL;
    cam_state.ready = false;

    if (devices == NULL) {
        ERROR("Couldn't enumerate devices\n%s", SDL_GetError());
    } else if (dev_count == 0) {
        ERROR("No camera device found\n%s", SDL_GetError());
    } else {
        // just open first camera
        if(!open_camera(devices[0])) {
            ERROR("Couldn't open camera %d\n%s", devices[0], SDL_GetError());
        }
    }
    SDL_free(devices);
}

void init_sdl() {
    // SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_CAMERA)) {
        ERROR("Failed to initialize SDL\n%s", SDL_GetError());
        EXIT(69);
    }

    // Camera
    init_camera();

    if (!SDL_CreateWindowAndRenderer("J-Ascii2", g_state.window_width,
                                     g_state.window_height, 0, &window, &renderer)) {
        ERROR("Failed to create window and renderer\n%s", SDL_GetError());
        EXIT(69);
    }

    float font_size = (float)g_state.window_height / g_state.frame_h;
    ascii_init(renderer, font_size);
}

void deinit() {
    
    SDL_CloseCamera(cam_state.camera);
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    TTF_Quit();
    SDL_Quit();
}

void handle_events(bool *quit) {
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) *quit = true;
        if (e.type == SDL_EVENT_KEY_DOWN) {
            switch (e.key.key) {

                case SDLK_ESCAPE: *quit = true;
                break;

#define SCALE_STEP 0.02f
                case SDLK_EQUALS: {
                    g_state.scale += SCALE_STEP;
                    g_state.scale = g_state.scale > 0.5f ? 0.5f : g_state.scale;
                    g_state.frame_w = (g_state.scale * cam_state.resx);
                    g_state.frame_h = g_state.frame_w * ((float)cam_state.resy / cam_state.resx);
                    ascii_update_font_size(g_state.window_height / g_state.frame_h);
                }
                break;
                case SDLK_MINUS: {
                    g_state.scale -= SCALE_STEP;
                    g_state.scale = g_state.scale < 0.01f ? 0.01f : g_state.scale;
                    g_state.frame_w = (g_state.scale * cam_state.resx);
                    g_state.frame_h = g_state.frame_w * ((float)cam_state.resy / cam_state.resx);
                    ascii_update_font_size(g_state.window_height / g_state.frame_h);
                }
                break;

            }
        }

        // camera events
        if (e.type == SDL_EVENT_CAMERA_DEVICE_ADDED) {
            SDL_Log("Camera device added");
            // new camera connection
            if (!cam_state.ready) {
                if (open_camera(e.cdevice.which)) {
                    SDL_SetWindowSize(window, g_state.window_width, g_state.window_height);
                    g_state.fbo = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET,
                                                    g_state.window_width, g_state.window_height);
                }
                else 
                    ERROR("Couldn't open camera %d\n%s", e.cdevice.which, SDL_GetError());
            }
        } else if (e.type == SDL_EVENT_CAMERA_DEVICE_REMOVED) {
            SDL_Log("Camera device removed");
            if (e.cdevice.which == cam_state.cam_id) {
                cam_state.camera = NULL;
                cam_state.ready = false;
            }
        }
    }

}

int main() {
    bool quit = false;
    g_state.scale = DEFAULT_SCALE;
    g_state.window_width = WINDOW_SIZE;
    g_state.window_height = WINDOW_SIZE;
    g_state.time_prev = 0;
    init_sdl();

    g_state.fbo = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET,
                                    g_state.window_width, g_state.window_height);
    while(!quit) {

        // camera frame
        SDL_Surface *camera_frame = SDL_AcquireCameraFrame(cam_state.camera, NULL);
        // since camera provides at fixed fps we dont update texture until new frame
        if (camera_frame) {
            // scale frame
            SDL_Surface *frame = SDL_CreateSurface(g_state.frame_w, g_state.frame_h, SDL_PIXELFORMAT_RGB24);
            SDL_BlitSurfaceScaled(camera_frame, NULL, frame, NULL, SDL_SCALEMODE_NEAREST);

            SDL_FRect dst_rect = {0, 0, g_state.window_width, g_state.window_height};

            SDL_SetRenderTarget(renderer, g_state.fbo);
            ascii_render(renderer, &dst_rect, frame);
            SDL_SetRenderTarget(renderer, NULL);

            SDL_ReleaseCameraFrame(cam_state.camera, camera_frame);
            SDL_DestroySurface(frame);
        }

        // render
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, g_state.fbo, NULL, NULL);
        SDL_RenderPresent(renderer);

        // input
        handle_events(&quit);

        // FPS cap
        Uint64 time = SDL_GetTicks();
        g_state.time_delta = time - g_state.time_prev;
        Uint32 delay_time = (Uint32)FRAME_TIME - g_state.time_delta;
        if (g_state.time_delta < FRAME_TIME) {
            SDL_Delay(delay_time);
        }
        g_state.time_delta += delay_time;
        g_state.time_prev = time + delay_time;

        char title[32];
        sprintf(title, "%ld", 1000 / g_state.time_delta);
        SDL_SetWindowTitle(window, title);
    }

    deinit();
    return 0;
}
