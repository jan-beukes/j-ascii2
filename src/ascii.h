#ifndef ASCII_H
#define ASCII_H
#include <SDL3/SDL.h>

#define DEFAULT_RES 100
#define DEFAULT_FONT_PATH "res/font.ttf"
//#define ASCII_TABLE " .',:;xlxokXdO0KN"
#define ASCII_TABLE "            .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$"

#define NON_ASCII_SIZE 50.0f

typedef enum {
    MODE_ASCII,
    MODE_NON_ASCII,
} Mode;

void set_render_mode(Mode mode);
Mode get_render_mode();

void ascii_update_font_size(float size);
void ascii_init(SDL_Renderer *renderer, float font_size);
void ascii_render(SDL_Renderer *renderer, SDL_FRect *dst_rect, SDL_Surface *frame);

void render_string(char *str, int x, int y, SDL_Color c);
void measure_string(char *str, int *w, int *h);

#endif
