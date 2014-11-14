#include <stdlib.h>
#include <fig.h>

typedef struct fig_image {
    size_t index_x;
    size_t index_y;
    size_t index_width;
    size_t index_height;
    size_t canvas_width;
    size_t canvas_height;
    size_t delay;
    fig_disposal_t disposal;
    fig_palette *palette;
    fig_bool_t transparent;
    size_t transparency_index;
    fig_uint8_t *index_data;
    fig_uint32_t *canvas_data;
} fig_image;

fig_image *fig_create_image(void) {
    fig_image *self = (fig_image *) malloc(sizeof(fig_image));
    if(self != NULL) {
        self->index_x = 0;
        self->index_y = 0;
        self->index_width = 0;
        self->index_height = 0;
        self->canvas_width = 0;
        self->canvas_height = 0;
        self->delay = 0;
        self->disposal = FIG_DISPOSAL_UNSPECIFIED;
        self->palette = fig_create_palette();
        self->transparent = 0;
        self->transparency_index = 0;
        self->index_data = NULL;
        self->canvas_data = NULL;
        if(self->palette == NULL) {
            return fig_image_free(self), NULL;
        }
    }
    return self;
}

fig_palette *fig_image_get_palette(fig_image *self) {
    return self->palette;
}

size_t fig_image_get_index_x(fig_image *self) {
    return self->index_x;
}

size_t fig_image_get_index_y(fig_image *self) {
    return self->index_y;
}

size_t fig_image_get_index_width(fig_image *self) {
    return self->index_width;
}

size_t fig_image_get_index_height(fig_image *self) {
    return self->index_height;
}

fig_uint8_t *fig_image_get_index_data(fig_image *self) {
    return self->index_data;
}

void fig_image_set_index_x(fig_image *self, size_t value) {
    self->index_x = value;
}

void fig_image_set_index_y(fig_image *self, size_t value) {
    self->index_y = value;
}

fig_bool_t fig_image_resize_index(fig_image *self, size_t width, size_t height) {
    size_t size = width * height;
    if(size == 0) {
        free(self->index_data);
        self->index_data = NULL;
        self->index_width = width;
        self->index_height = height;
        return 1;
    } else {
        fig_uint8_t *index_data;
        index_data = (fig_uint8_t *) realloc(self->index_data, sizeof(fig_uint8_t) * size);
        if(index_data == NULL) {
            return 0;
        } else {
            self->index_width = width;
            self->index_height = height;
            self->index_data = index_data;
            return 1;
        }
    }
}

size_t fig_image_get_canvas_width(fig_image *self) {
    return self->canvas_width;
}

size_t fig_image_get_canvas_height(fig_image *self) {
    return self->canvas_height;
}

fig_uint32_t *fig_image_get_canvas_data(fig_image *self) {
    return self->canvas_data;
}

fig_bool_t fig_image_resize_canvas(fig_image *self, size_t width, size_t height) {
    size_t size = width * height;
    if(size == 0) {
        free(self->canvas_data);
        self->canvas_data = NULL;
        self->canvas_width = width;
        self->canvas_height = height;
        return 1;
    } else {
        fig_uint32_t *canvas_data;
        canvas_data = (fig_uint32_t *) realloc(self->canvas_data, sizeof(fig_uint32_t) * size);
        if(canvas_data == NULL) {
            return 0;
        } else {
            self->canvas_width = width;
            self->canvas_height = height;
            self->canvas_data = canvas_data;
            return 1;
        }
    }
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
        free(self->canvas_data);
    }
    free(self);
}
