#include <stdio.h>
#include <stdlib.h>

#include <SDL3_ttf/SDL_ttf.h>
#include "ascii.h"

#define ERROR(fmt, ...) SDL_Log("ERROR: " fmt, ##__VA_ARGS__)
#define EXIT(code) ({SDL_Quit(); exit(code);})

#define GRAY(R, G, B) (0.2126f*R + 0.7152f*G + 0.0722f*B)

// global state
TTF_TextEngine *engine;
TTF_Font *ascii_font;
TTF_Text *ascii_text;
TTF_Font *ui_font;
TTF_Text *ui_text;

char *ascii_table;
int ascii_table_len;

void measure_string(char *str, int *w, int *h) {
    TTF_MeasureString(ui_font, str, 0, 0, w, NULL);
    *h = TTF_GetFontSize(ui_font);
}

void render_string(char *str, int x, int y, SDL_Color col) {
    TTF_SetTextString(ui_text, str, 0);
    TTF_SetTextColor(ui_text, col.r, col.g, col.b, col.a);
    TTF_DrawRendererText(ui_text, x, y);
}

void render_ascii_char(char c, int x, int y, SDL_Color col) {
    TTF_SetTextString(ascii_text, &c, 1);
    TTF_SetTextColor(ascii_text, col.r, col.g, col.b, col.a);
    TTF_DrawRendererText(ascii_text, x, y);
}

void ascii_update_font_size(float size) {TTF_SetFontSize(ascii_font, size); }

void update_font_size(float size) { TTF_SetFontSize(ui_font, size); }

void ascii_init(SDL_Renderer *renderer, float size) {
    if (!TTF_Init()) {
        ERROR("Couldn't initialize SDL_ttf\n%s", SDL_GetError());
        EXIT(69);
    }

    ascii_table = ASCII_TABLE;
    ascii_table_len = strlen(ascii_table);

    engine = TTF_CreateRendererTextEngine(renderer);

    // load fonts and create text objects
    ascii_font = TTF_OpenFont("res/font.ttf", size);
    ascii_text = TTF_CreateText(engine, ascii_font, "", 0);
    ui_font = TTF_OpenFont("res/font2.ttf", 32.0f);
    ui_text = TTF_CreateText(engine, ui_font, "", 0);
}

void ascii_deinit() {
    TTF_CloseFont(ascii_font);
    TTF_DestroyText(ascii_text);
    TTF_CloseFont(ui_font);
    TTF_DestroyText(ui_text);
    TTF_Quit();
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

            render_ascii_char(c, x_pos, y_pos, color);
        }
    }
}

