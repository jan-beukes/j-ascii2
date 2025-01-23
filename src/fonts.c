#include "fonts.h"
#include <SDL3/SDL.h>

SDL_IOStream *get_font_stream(char *font_name) {
    if (SDL_strcmp(font_name, "font.ttf") == 0) {
        return SDL_IOFromConstMem(font_ttf, font_ttf_len);
    }
    if (SDL_strcmp(font_name, "font2.ttf") == 0) {
        return SDL_IOFromConstMem(font2_ttf, font2_ttf_len);
    }
    return NULL;
}
