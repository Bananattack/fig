#include <stdlib.h>
#include <fig.h>

typedef struct fig_frame {
    size_t x;
    size_t y;
    size_t width;
    size_t height;
    size_t delay;
    fig_disposal_t disposal;
    fig_palette *palette;
    fig_uint8_t *index_data;
    fig_uint32_t *color_data;
} fig_frame;

fig_frame *fig_create_frame(void) {
    fig_frame *self = (fig_frame *) malloc(sizeof(fig_frame));
    if(self != NULL) {
        self->x = 0;
        self->y = 0;
        self->width = 0;
        self->height = 0;
        self->delay = 0;
        self->disposal = FIG_DISPOSAL_UNSPECIFIED;
        self->palette = fig_create_palette();
        self->index_data = NULL;
        self->color_data = NULL;
        if(self->palette == NULL) {
            fig_frame_free(self);
            self = NULL;
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

size_t fig_frame_get_width(fig_frame *self) {
    return self->width;
}

size_t fig_frame_get_height(fig_frame *self) {
    return self->height;
}

size_t fig_frame_get_delay(fig_frame *self) {
    return self->delay;
}

fig_disposal_t fig_frame_get_disposal(fig_frame *self) {
    return self->disposal;
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

        if(index_data != NULL && color_data != NULL) {
            self->width = width;
            self->height = height;
            self->index_data = index_data;
            self->color_data = color_data;
            return 1;
        } else {
            free(index_data);
            free(color_data);
            return 0;
        }
    }
}

void fig_frame_set_delay(fig_frame *self, size_t value) {
    self->delay = value;
}

void fig_frame_set_disposal(fig_frame *self, fig_disposal_t value) {
    self->disposal = value;
}

 void fig_frame_calculate_colors(fig_frame *self, fig_image *image) {
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

     if(fig_palette_count_colors(self->palette) > 0) {
        palette = self->palette;
     } else {
        palette = fig_image_get_palette(image);
     }

     color_count = fig_palette_count_colors(palette);
     colors = fig_palette_get_colors(palette);
     image_size = self->width * self->height;
     index_data = self->index_data;
     color_data = self->color_data;

     for(i = 0; i < image_size; ++i) {
         fig_uint8_t index = index_data[i];
         if(index >= color_count) {
             index = 0;
         }
         color_data[i] = colors[index];
     }
 }

void fig_frame_free(fig_frame *self) {
    if(self != NULL) {
        if(self->palette != NULL) {
            fig_palette_free(self->palette);
        }
        free(self->index_data);
        free(self->color_data);
    }
    free(self);
}
