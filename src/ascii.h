#ifndef ASCII_H
#define ASCII_H
#include <SDL3/SDL.h>

#define DEFAULT_SCALE 0.2f
#define DEFAULT_FONT_PATH "res/font.ttf"
#define ASCII_TABLE " .',:;xlxokXdO0KN"

//"            .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$"


void ascii_update_font_size(float size);
void ascii_init(SDL_Renderer *renderer, float font_size);
void ascii_render(SDL_Renderer *renderer, SDL_FRect *dst_rect, SDL_Surface *frame);

void render_string(char *str, int x, int y, SDL_Color c);

#endif
