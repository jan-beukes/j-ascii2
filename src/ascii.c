#include <stdio.h>
#include <stdlib.h>

#include <SDL3_ttf/SDL_ttf.h>
#include "ascii.h"

#define ERROR(fmt, ...) SDL_Log("ERROR: " fmt, ##__VA_ARGS__)
#define EXIT(code) ({SDL_Quit(); exit(code);})

#define GRAY(R, G, B) (0.2126f*R + 0.7152f*G + 0.0722f*B)

// global state
TTF_TextEngine *engine;
TTF_Text *text;
TTF_Font *font;
Mode render_mode;
float font_size;
char *ascii_table;
int ascii_table_len;

void set_render_mode(Mode mode) {
    if (mode == MODE_ASCII) {
        TTF_SetFontSize(font, font_size);
    } else if (mode == MODE_NON_ASCII) {
        TTF_SetFontSize(font, NON_ASCII_SIZE);
    }
    render_mode = mode;
}

Mode get_render_mode() { return render_mode; };

void measure_string(char *str, int *w, int *h) {
    TTF_MeasureString(font, str, 0, 0, w, NULL);
    *h = NON_ASCII_SIZE;
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

void ascii_update_font_size(float size) {
    font_size = size;
    TTF_SetFontSize(font, size);
}

void ascii_init(SDL_Renderer *renderer, float size) {
    if (!TTF_Init()) {
        ERROR("Couldn't initialize SDL_ttf\n%s", SDL_GetError());
        EXIT(69);
    }

    ascii_table = ASCII_TABLE;
    ascii_table_len = strlen(ascii_table);

    engine = TTF_CreateRendererTextEngine(renderer);
    font = TTF_OpenFont(DEFAULT_FONT_PATH, size);
    text = TTF_CreateText(engine, font, "", 0);
}

#define BYTES_PER_PIXEL 3
SDL_Color get_pixel_color(SDL_Surface *img, int x, int y) {
    Uint8 *pixels = (Uint8 *)img->pixels;
    Uint8 *pixel = pixels + y *img->pitch + x*BYTES_PER_PIXEL;

    SDL_Color color;
    color.r = pixel[0];
    color.g = pixel[1];
    color.b = pixel[2];
    color.a = 255;

    return color;
}

/*  render the given surface with the ascii renderer.
    dst_rect specifies size of render area.
    format is in RGB24 since webcam formats are so sus.
*/
void ascii_render(SDL_Renderer *renderer, SDL_FRect *dst_rect, SDL_Surface *frame) {
    SDL_assert(frame->format == SDL_PIXELFORMAT_RGB24);

    SDL_SetRenderDrawColor(renderer, 0x18, 0x18, 0x18, 0xFF);
    SDL_RenderClear(renderer);

    float char_size = dst_rect->h / frame->h;
    for(int y = 0; y < frame->h; y++) { 
        int y_pos = y * char_size;
        for (int x = 0; x < frame->w; x++) {
            int x_pos = x * char_size;

            SDL_Color color = get_pixel_color(frame, x, y);

            Uint8 gray = GRAY(color.r, color.g, color.b);
            int index = gray * ((ascii_table_len - 1) / 255.0f);
            char c = ascii_table[index];

            render_char(c, x_pos, y_pos, color);
        }
    }
}

