#ifndef ASCII_H
#define ASCII_H
#include <SDL3/SDL.h>

#define DEFAULT_RES 100


/* Initialize ascii rendering.
   table file can be NULL for default
*/
void ascii_init(SDL_Renderer *renderer, float font_size, char *table_file);
void ascii_deinit();

// update font size for ascii renderer
void ascii_update_font_size(float size);
// must be called after ascii_init
int ascii_get_table_count();
// ascii rendering
void ascii_render(SDL_Renderer *renderer, SDL_FRect *dst_rect, SDL_Surface *frame, int table_index);

// update font size for regular text renderer
void update_font_size(float size);
void render_string(char *str, int x, int y, SDL_Color c);
void measure_string(char *str, int *w, int *h);

#endif
