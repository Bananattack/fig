#include <stdlib.h>
#include <fig.h>

typedef struct fig_frame {
    size_t x;
    size_t y;
    size_t canvas_width;
    size_t canvas_height;
    size_t delay;
    fig_disposal_t disposal;
    fig_palette *palette;
    fig_bool_t transparent;
    size_t transparency_index;
    fig_uint8_t *index_data;
    fig_uint32_t *color_data;
    size_t render_width;
    size_t render_height;
    fig_uint32_t *render_data;
} fig_frame;

fig_frame *fig_create_frame(void) {
    fig_frame *self = (fig_frame *) malloc(sizeof(fig_frame));
    if(self != NULL) {
        self->x = 0;
        self->y = 0;
        self->canvas_width = 0;
        self->canvas_height = 0;
        self->delay = 0;
        self->disposal = FIG_DISPOSAL_UNSPECIFIED;
        self->palette = fig_create_palette();
        self->transparent = 0;
        self->transparency_index = 0;
        self->index_data = NULL;
        self->color_data = NULL;
        self->render_width = 0;
        self->render_height = 0;
        self->render_data = NULL;
        if(self->palette == NULL) {
            return fig_frame_free(self), NULL;
        }
    }
    return self;
}

fig_palette *fig_frame_get_palette(fig_frame *self) {
    return self->palette;
}

fig_uint8_t *fig_frame_get_index_data(fig_frame *self) {
    return self->index_data;
}

fig_uint32_t *fig_frame_get_color_data(fig_frame *self) {
    return self->color_data;
}

size_t fig_frame_get_x(fig_frame *self) {
    return self->x;
}

size_t fig_frame_get_y(fig_frame *self) {
    return self->y;
}

size_t fig_frame_get_canvas_width(fig_frame *self) {
    return self->canvas_width;
}

size_t fig_frame_get_canvas_height(fig_frame *self) {
    return self->canvas_height;
}

size_t fig_frame_get_delay(fig_frame *self) {
    return self->delay;
}

fig_disposal_t fig_frame_get_disposal(fig_frame *self) {
    return self->disposal;
}

fig_bool_t fig_frame_get_transparent(fig_frame *self) {
    return self->transparent;
}

size_t fig_frame_get_transparency_index(fig_frame *self) {
    return self->transparency_index;
}

void fig_frame_set_x(fig_frame *self, size_t value) {
    self->x = value;
}

void fig_frame_set_y(fig_frame *self, size_t value) {
    self->y = value;
}

fig_bool_t fig_frame_resize_canvas(fig_frame *self, size_t width, size_t height) {
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

void fig_frame_set_delay(fig_frame *self, size_t value) {
    self->delay = value;
}

void fig_frame_set_disposal(fig_frame *self, fig_disposal_t value) {
    self->disposal = value;
}

void fig_frame_set_transparent(fig_frame *self, fig_bool_t value) {
    self->transparent = value;
}

void fig_frame_set_transparency_index(fig_frame *self, size_t value) {
    self->transparency_index = value;
}

 void fig_frame_calculate_colors(fig_frame *self, fig_animation *animation) {
     fig_palette *palette;
     size_t color_count;
     fig_uint32_t *colors;
     size_t image_size;
     fig_uint8_t *index_data;
     fig_uint32_t *color_data;
     size_t i;

     if(self->index_data == NULL || self->color_data == NULL) {
         return;
     }

     palette = fig_frame_get_render_palette(self, animation);
     color_count = fig_palette_count_colors(palette);
     colors = fig_palette_get_colors(palette);
     image_size = self->canvas_width * self->canvas_height;
     index_data = self->index_data;
     color_data = self->color_data;

     for(i = 0; i < image_size; ++i) {
         fig_uint8_t index = index_data[i];
         FIG_ASSERT(index < color_count);
         color_data[i] = colors[index];
         if(self->transparent && index == self->transparency_index) {
             color_data[i] &= ~0xFF000000;
         }
     }
 }

size_t fig_frame_get_render_width(fig_frame *self) {
    return self->render_width;
}

size_t fig_frame_get_render_height(fig_frame *self) {
    return self->render_height;
}

fig_uint32_t *fig_frame_get_render_data(fig_frame *self) {
    return self->render_data;
}

fig_palette *fig_frame_get_render_palette(fig_frame *self, fig_animation *animation) {
     if(fig_palette_count_colors(self->palette) > 0) {
        return self->palette;
     } else {
        return fig_animation_get_palette(animation);
     }
}

fig_bool_t fig_frame_resize_render(fig_frame *self, size_t width, size_t height) {
    size_t size = width * height;
    if(size == 0) {
        free(self->render_data);
        self->render_data = NULL;
        return 1;
    } else {
        fig_uint32_t *render_data;
        render_data = (fig_uint32_t *) realloc(self->render_data, sizeof(fig_uint32_t) * size);
        if(render_data == NULL) {
            return 0;
        } else {
            self->render_width = width;
            self->render_height = height;
            self->render_data = render_data;
            return 1;
        }
    }
}

void fig_frame_free(fig_frame *self) {
    if(self != NULL) {
        if(self->palette != NULL) {
            fig_palette_free(self->palette);
        }
        free(self->index_data);
        free(self->color_data);
        free(self->render_data);
    }
    free(self);
}
