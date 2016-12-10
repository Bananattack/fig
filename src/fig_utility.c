#include <fig.h>

fig_uint32_t fig_pack_color(fig_uint8_t r, fig_uint8_t g, fig_uint8_t b, fig_uint8_t a) {
    return ((fig_uint32_t) r << 16)
        | ((fig_uint32_t) g << 8)
        | (fig_uint32_t) b
        | ((fig_uint32_t) a << 24);
}

void fig_unpack_color(fig_uint32_t color, fig_uint8_t *r, fig_uint8_t *g, fig_uint8_t *b, fig_uint8_t *a) {
    *r = (fig_uint8_t)((color >> 16) & 0xFF);
    *g = (fig_uint8_t)((color >> 8) & 0xFF);
    *b = (fig_uint8_t)(color & 0xFF);
    *a = (fig_uint8_t)((color >> 24) & 0xFF);
}