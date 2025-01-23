#include <stdio.h>
#include <stdlib.h>

#include <SDL3_ttf/SDL_ttf.h>
#include "ascii.h"

#define ERROR(fmt, ...) SDL_Log("ERROR: " fmt, ##__VA_ARGS__)
#define EXIT(code) ({SDL_Quit(); exit(code);})

#define GRAY(R, G, B) (0.2126f*R + 0.7152f*G + 0.0722f*B)

typedef struct {
    char *string;
    int len;
} Table;

#define MAX_TABLE_LEN 128
#define DEFAULT_TABLE " .',:;xlxokXdO0KN"

// global state
TTF_TextEngine *engine;
TTF_Font *ascii_font;
TTF_Text *ascii_text;
TTF_Font *ui_font;
TTF_Text *ui_text;

#define MAX_TABLES 8
Table ascii_tables[MAX_TABLES];
int ascii_table_count = 0;

// defined in fonts.c
SDL_IOStream *get_font_stream(char *font_name);

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

int ascii_get_table_count() { return ascii_table_count; }

void ascii_init(SDL_Renderer *renderer, float size, char *table_file) {
    if (!TTF_Init()) {
        ERROR("Couldn't initialize SDL_ttf\n%s", SDL_GetError());
        EXIT(69);
    }

    // default table
    ascii_tables[ascii_table_count++] = (Table){DEFAULT_TABLE, strlen(DEFAULT_TABLE)};

    // read from ascii.tbl
    FILE *file;
    if (table_file == NULL) 
        file = fopen("ascii.tbl", "r");
    else 
        file = fopen(table_file, "r");

    if (file) {
        char line[MAX_TABLE_LEN];
        while(fgets(line, MAX_TABLE_LEN, file) != NULL) {
            if (ascii_table_count == MAX_TABLES) break;
            // allocate table strings
            size_t len = strlen(line) - 1; // remove newline
            char *string = malloc(len + 1);
            strncpy(string, line, len);
            string[len] = '\0';

            ascii_tables[ascii_table_count++] = (Table) {
                .string = string,
                .len = len,
            };
        }
        fclose(file);
    } else if (table_file) {
        ERROR("Could not open tbl file %s", table_file);
    }

    engine = TTF_CreateRendererTextEngine(renderer);

    // load fonts and create text objects
    SDL_IOStream *stream = get_font_stream("font.ttf");
    SDL_IOStream *stream2 = get_font_stream("font2.ttf");
    ascii_font = TTF_OpenFontIO(stream, true, size);
    ascii_text = TTF_CreateText(engine, ascii_font, "", 0);
    ui_font = TTF_OpenFontIO(stream2, true, 32.0f);
    ui_text = TTF_CreateText(engine, ui_font, "", 0);
}

void ascii_deinit() {
    // free strings that were read from tbl file
    for (int i = 1; i < ascii_table_count; i++){
        free(ascii_tables[i].string);
    }

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
    set table_index to 0 for default
    format is in RGB24 since webcam formats are so sus.
*/
void ascii_render(SDL_Renderer *renderer, SDL_FRect *dst_rect, SDL_Surface *frame, int table_index) {
    SDL_assert(frame->format == SDL_PIXELFORMAT_RGB24);
    SDL_assert(table_index >= 0 && table_index < ascii_table_count);

    SDL_SetRenderDrawColor(renderer, 0x18, 0x18, 0x18, 0xFF);
    SDL_RenderClear(renderer);

    float char_size = dst_rect->h / frame->h;
    for(int y = 0; y < frame->h; y++) { 
        int y_pos = y * char_size;
        for (int x = 0; x < frame->w; x++) {
            int x_pos = x * char_size;

            SDL_Color color = get_pixel_color(frame, x, y);

            Uint8 gray = GRAY(color.r, color.g, color.b);
            Table current_table = ascii_tables[table_index];
            int index = gray * ((current_table.len - 1) / 255.0f);
            char c = current_table.string[index];

            render_ascii_char(c, x_pos, y_pos, color);
        }
    }
}

