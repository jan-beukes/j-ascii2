#include <stdio.h>
#include <stdlib.h>

#include <SDL3_ttf/SDL_ttf.h>
#include "ascii.h"

#define ERROR(fmt, ...) SDL_Log("ERROR: " fmt, ##__VA_ARGS__)
#define EXIT(code) ({SDL_Quit(); exit(code);})

#define GRAY(R, G, B) (0.2126f*R + 0.7152f*G + 0.0722f*B)

TTF_TextEngine *engine;
TTF_Text *text;
TTF_Font *font;
char *ascii_table;
int ascii_table_len;

void ascii_update_font_size(float size) {
    TTF_SetFontSize(font, size);
}

void render_string(char *str, int x, int y, SDL_Color col) {
    TTF_SetTextString(text, str, 0);
    TTF_SetTextColor(text, col.r, col.g, col.b, col.a);
    TTF_DrawRendererText(text, x, y);
}

void render_char(char c, int x, int y, SDL_Color col) {
    TTF_SetTextString(text, &c, 1);
    TTF_SetTextColor(text, col.r, col.g, col.b, col.a);
    TTF_DrawRendererText(text, x, y);
}

void ascii_init(SDL_Renderer *renderer, float font_size) {
    if (!TTF_Init()) {
        ERROR("Couldn't initialize SDL_ttf\n%s", SDL_GetError());
        EXIT(69);
    }

    ascii_table = ASCII_TABLE;
    ascii_table_len = strlen(ascii_table);

    engine = TTF_CreateRendererTextEngine(renderer);
    font = TTF_OpenFont(DEFAULT_FONT_PATH, font_size);
    text = TTF_CreateText(engine, font, "", 0);
}

#define BYTES_PER_PIXEL 3
SDL_Color get_pixel_color(SDL_Surface *img, int x, int y) {
    Uint8 *pixels = (Uint8 *)img->pixels;
    Uint8 *pixel = pixels + y *img->pitch + x*BYTES_PER_PIXEL; // address of this pixel

    SDL_Color color;
    // no 3 byte int type
    if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
        color.r = pixel[0];
        color.g = pixel[1];
        color.b = pixel[2];
    } else {
        color.r = pixel[2];
        color.g = pixel[1];
        color.b = pixel[0];
    }
    color.a = 255;

    return color;
}

// render the given surface with the ascii renderer
// format is in RGB24 since webcam formats are so sus
void ascii_render(SDL_Renderer *renderer, SDL_FRect *dst_rect, SDL_Surface *frame) {
    SDL_assert(frame->format == SDL_PIXELFORMAT_RGB24);

    SDL_SetRenderDrawColor(renderer, 0x18, 0x18, 0x18, 0xFF);
    SDL_RenderClear(renderer);

    float char_width = dst_rect->w / frame->w;
    float char_height = dst_rect->h / frame->h;
    for(int y = 0; y < frame->h; y++) { 
        int y_pos = char_height * y;
        for (int x = 0; x < frame->w; x++) {
            int x_pos = char_width * x;

            SDL_Color color = get_pixel_color(frame, x, y);

            Uint8 gray = GRAY(color.r, color.g, color.b);
            int index = gray * ((ascii_table_len - 1) / 255.0f);
            char c = ascii_table[index];

            render_char(c, x_pos, y_pos, color);
        }
    }

}

