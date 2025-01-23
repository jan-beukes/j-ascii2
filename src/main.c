#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "ascii.h"

#define WINDOW_SIZE 1200
#define FRAME_TIME (1000.0f / 60.0f)

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
    int fps;

    SDL_Camera *camera;
    int cam_index;
    int dev_count;
    SDL_CameraID *devices;
} cam_state = {0};

SDL_Window *window;
SDL_Renderer *renderer;

void update_window_title() {
    if (!cam_state.ready) {
        SDL_SetWindowTitle(window, "J-Ascii2");
        return;
    }
    // Title
    char title[256];
    SDL_CameraID device = SDL_GetCameraID(cam_state.camera);
    const char *name = SDL_GetCameraName(device);
    sprintf(title, "%s | %dx%d %dfps", name, g_state.frame_w, g_state.frame_h, cam_state.fps);
    SDL_SetWindowTitle(window, title);
}

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
            int fps = f->framerate_numerator / f->framerate_denominator;
            if (fps > best_fps) {
                best_fps = fps;
                best_format = f;
            }
        }
    }

    cam_state.camera = SDL_OpenCamera(device, best_format);
    if (cam_state.camera == NULL) {
        SDL_free(formats);
        cam_state.ready = false;
        return false;
    }

    // update state
    cam_state.ready = true;
    cam_state.resx = best_format->width;
    cam_state.resy = best_format->height;
    cam_state.fps = best_format->framerate_numerator / best_format->framerate_denominator;

    g_state.window_height = g_state.window_width * ((float)cam_state.resy / cam_state.resx);
    g_state.frame_w = DEFAULT_RES;
    g_state.frame_h = g_state.frame_w * ((float)cam_state.resy / cam_state.resx);

    // update these on new camera open
    ascii_update_font_size((float)g_state.window_height / g_state.frame_h);
    update_window_title();

    SDL_free(formats);
    return true;
}

void load_cameras() {
    // load devices
    cam_state.devices = SDL_GetCameras(&cam_state.dev_count);
    if (cam_state.devices == NULL) {
        ERROR("Couldn't enumerate devices\n%s", SDL_GetError());
    } else if (cam_state.dev_count == 0) {
        ERROR("No camera device found\n%s", SDL_GetError());
    }
}

// set camera to current index + given offset
void set_camera(int offset) {
    SDL_assert(offset == 0 || offset == 1 || offset == -1);
    if (cam_state.dev_count == 0 || cam_state.devices == NULL) {
        cam_state.camera = NULL;
        cam_state.ready = false;
        cam_state.cam_index = 0;
        return;
    }

    int index = cam_state.cam_index + offset;
    // wrap cameras
    if (index <= -1) index = cam_state.dev_count - 1;
    else if (index >= cam_state.dev_count) index = 0;

    if (index == cam_state.cam_index && cam_state.ready) return; // nothing happened

    SDL_CameraID device = cam_state.devices[index];

    if (open_camera(device)) {
        SDL_SetWindowSize(window, g_state.window_width, g_state.window_height);
        if (g_state.fbo != NULL) SDL_DestroyTexture(g_state.fbo);
        g_state.fbo = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET,
                                        g_state.window_width, g_state.window_height);
    } else {
        update_window_title(); // update to default
        ERROR("Couldn't open camera %s\n%s", SDL_GetCameraName(device), SDL_GetError());
    }

    cam_state.cam_index = index;
}

void init_sdl() {
    // SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_CAMERA)) {
        ERROR("Failed to initialize SDL\n%s", SDL_GetError());
        EXIT(69);
    }

    // Camera
    cam_state.resx = g_state.window_width;
    cam_state.resy = g_state.window_height;
    cam_state.camera = NULL;
    cam_state.ready = false;
    cam_state.cam_index = 0;

    load_cameras();
    SDL_CameraID device = cam_state.devices[cam_state.cam_index];
    if (!open_camera(device)) {
        ERROR("Couldn't open camera %s\n%s", SDL_GetCameraName(device), SDL_GetError());
    }

    // renderer
    if (!SDL_CreateWindowAndRenderer("J-Ascii2", g_state.window_width,
                                     g_state.window_height, 0, &window, &renderer)) {
        ERROR("Failed to create window and renderer\n%s", SDL_GetError());
        EXIT(69);
    }
    if (cam_state.ready)
        g_state.fbo = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET,
                                        g_state.window_width, g_state.window_height);


    float font_size = (float)g_state.window_height / g_state.frame_h;
    ascii_init(renderer, font_size);
}

void deinit() {
    
    SDL_free(cam_state.devices);
    SDL_CloseCamera(cam_state.camera);
    SDL_DestroyTexture(g_state.fbo);
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

                // Frame scale
#define SCALE_STEP 1.2f
#define LIMIT_UPPER 256
#define LIMIT_LOWER 16
                case SDLK_EQUALS: {
                    g_state.frame_w /= SCALE_STEP;
                    g_state.frame_w = g_state.frame_w < LIMIT_LOWER ? LIMIT_LOWER : g_state.frame_w;
                    g_state.frame_h = g_state.frame_w * ((float)cam_state.resy / cam_state.resx);
                    ascii_update_font_size((float)g_state.window_height / g_state.frame_h);
                    update_window_title();
                }
                break;
                case SDLK_MINUS: {
                    g_state.frame_w *= SCALE_STEP;
                    g_state.frame_w = g_state.frame_w > LIMIT_UPPER ? LIMIT_UPPER : g_state.frame_w;
                    g_state.frame_h = g_state.frame_w * ((float)cam_state.resy / cam_state.resx);
                    ascii_update_font_size((float)g_state.window_height / g_state.frame_h);
                    update_window_title();
                }
                break;

                case SDLK_RIGHT:
                    set_camera(1);
                break;
                case SDLK_LEFT:
                    set_camera(-1);
                break;
            }
        }

        //--Camera events---
        if (e.type == SDL_EVENT_CAMERA_DEVICE_ADDED) {
            SDL_CameraID device = e.cdevice.which;
            SDL_Log("%s connected", SDL_GetCameraName(device));
        }
        if (e.type == SDL_EVENT_CAMERA_DEVICE_REMOVED) {
            SDL_CameraID device = e.cdevice.which;
            SDL_Log("%s disconnected", SDL_GetCameraName(device));
            cam_state.ready = false;
            update_window_title();
        }
    }

}

int main() {
    bool quit = false;
    g_state.window_width = WINDOW_SIZE;
    g_state.window_height = WINDOW_SIZE;
    g_state.time_prev = 0;
    g_state.time_delta = FRAME_TIME;
    init_sdl();

    update_window_title();
    while(!quit) {
        // input
        handle_events(&quit);


        // camera frame
        SDL_FRect frame_rect = {0, 0, g_state.window_width, g_state.window_height};
        SDL_Surface *camera_frame = SDL_AcquireCameraFrame(cam_state.camera, NULL);

        // since camera provides at fixed fps we dont update texture until new frame
        if (camera_frame) {
            // scale frame
            SDL_Surface *frame = SDL_CreateSurface(g_state.frame_w, g_state.frame_h, SDL_PIXELFORMAT_RGB24);
            SDL_BlitSurfaceScaled(camera_frame, NULL, frame, NULL, SDL_SCALEMODE_NEAREST);
            SDL_ReleaseCameraFrame(cam_state.camera, camera_frame);

            SDL_SetRenderTarget(renderer, g_state.fbo);
            ascii_render(renderer, &frame_rect, frame);
            SDL_SetRenderTarget(renderer, NULL);

            SDL_DestroySurface(frame);
        }

        //---Render---
        SDL_RenderClear(renderer);

        // not connected
        if (!cam_state.ready) {
            open_camera(cam_state.devices[cam_state.cam_index]);
            if (get_render_mode() != MODE_NON_ASCII) set_render_mode(MODE_NON_ASCII);
            char *text = "Disconnected...";
            int w, h;
            measure_string(text, &w, &h);
            render_string(text, (g_state.window_width - w) / 2, (g_state.window_height - h) / 2,
                          (SDL_Color){255, 0, 0, 255});
        } else {
            if (get_render_mode() != MODE_ASCII) set_render_mode(MODE_ASCII);
            SDL_RenderTexture(renderer, g_state.fbo, NULL, NULL);
        }
        SDL_RenderPresent(renderer);

        // FPS cap
        Uint64 time = SDL_GetTicks();
        g_state.time_delta = time - g_state.time_prev;
        Uint32 delay_time = (Uint32)FRAME_TIME - g_state.time_delta;
        if (g_state.time_delta < FRAME_TIME) {
            SDL_Delay(delay_time);
        }
        g_state.time_delta += delay_time;
        g_state.time_prev = time + delay_time;

    }

    deinit();
    return 0;
}
