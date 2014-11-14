#include <stdlib.h>
#include <fig.h>

typedef struct fig_image {
    size_t x;
    size_t y;
    size_t w;
    size_t h;
    size_t canvas_width;
    size_t canvas_height;
    size_t delay;
    fig_disposal_t disposal;
    fig_palette *palette;
    fig_bool_t transparent;
    size_t transparency_index;
    fig_uint8_t *index_data;
    fig_uint32_t *color_data;
} fig_image;

fig_image *fig_create_image(void) {
    fig_image *self = (fig_image *) malloc(sizeof(fig_image));
    if(self != NULL) {
        self->x = 0;
        self->y = 0;
        self->w = 0;
        self->h = 0;
        self->canvas_width = 0;
        self->canvas_height = 0;
        self->delay = 0;
        self->disposal = FIG_DISPOSAL_UNSPECIFIED;
        self->palette = fig_create_palette();
        self->transparent = 0;
        self->transparency_index = 0;
        self->index_data = NULL;
        self->color_data = NULL;
        if(self->palette == NULL) {
            return fig_image_free(self), NULL;
        }
    }
    return self;
}

fig_palette *fig_image_get_palette(fig_image *self) {
    return self->palette;
}

fig_uint8_t *fig_image_get_index_data(fig_image *self) {
    return self->index_data;
}

fig_uint32_t *fig_image_get_color_data(fig_image *self) {
    return self->color_data;
}

size_t fig_image_get_canvas_width(fig_image *self) {
    return self->canvas_width;
}

size_t fig_image_get_canvas_height(fig_image *self) {
    return self->canvas_height;
}

size_t fig_image_get_delay(fig_image *self) {
    return self->delay;
}

fig_disposal_t fig_image_get_disposal(fig_image *self) {
    return self->disposal;
}

fig_bool_t fig_image_get_transparent(fig_image *self) {
    return self->transparent;
}

size_t fig_image_get_transparency_index(fig_image *self) {
    return self->transparency_index;
}

void fig_image_get_region(fig_image *self, size_t *x, size_t *y, size_t *w, size_t *h) {
    if(x != NULL) { *x = self->x; }
    if(y != NULL) { *y = self->y; }
    if(w != NULL) { *w = self->w; }
    if(h != NULL) { *h = self->h; }
}

void fig_image_set_region(fig_image *self, size_t x, size_t y, size_t w, size_t h) {
    self->x = x;
    self->y = y;
    self->w = w;
    self->h = h;
}

fig_bool_t fig_image_resize_canvas(fig_image *self, size_t width, size_t height) {
    size_t size = width * height;
    if(size == 0) {
        free(self->index_data);
        free(self->color_data);
        self->index_data = NULL;
        self->color_data = NULL;
        return 1;
    } else {
        fig_uint8_t *index_data;
        fig_uint32_t *color_data;
        index_data = (fig_uint8_t *) realloc(self->index_data, sizeof(fig_uint8_t) * size);
        color_data = (fig_uint32_t *) realloc(self->color_data, sizeof(fig_uint32_t) * size);
        if(index_data == NULL) {
            return 0;
        } else if(color_data == NULL) {
            return self->index_data = index_data, 0;
        } else {
            self->canvas_width = width;
            self->canvas_height = height;
            self->index_data = index_data;
            self->color_data = color_data;
            return 1;
        }
    }
}

void fig_image_set_delay(fig_image *self, size_t value) {
    self->delay = value;
}

void fig_image_set_disposal(fig_image *self, fig_disposal_t value) {
    self->disposal = value;
}

void fig_image_set_transparent(fig_image *self, fig_bool_t value) {
    self->transparent = value;
}

void fig_image_set_transparency_index(fig_image *self, size_t value) {
    self->transparency_index = value;
}

fig_palette *fig_image_get_render_palette(fig_image *self, fig_animation *animation) {
    if(fig_palette_count_colors(self->palette) > 0) {
        return self->palette;
    } else {
        return fig_animation_get_palette(animation);
    }
}

void fig_image_free(fig_image *self) {
    if(self != NULL) {
        if(self->palette != NULL) {
            fig_palette_free(self->palette);
        }
        free(self->index_data);
        free(self->color_data);
    }
    free(self);
}
