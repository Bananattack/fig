#include <stdlib.h>
#include <fig.h>

typedef struct fig_frame {
    size_t x;
    size_t y;
    size_t width;
    size_t height;
    fig_uint32_t delay;
    fig_palette *palette;
    fig_uint8_t *data;
} fig_frame;

fig_frame *fig_create_frame(void) {
    fig_frame *self = (fig_frame *) malloc(sizeof(fig_frame));
    if(self != NULL) {
        self->x = 0;
        self->y = 0;
        self->width = 0;
        self->height = 0;
        self->delay = 0;
        self->palette = fig_create_palette();
        self->data = NULL;
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

fig_uint8_t *fig_frame_get_pixel_data(fig_frame *self) {
    return self->data;
}

size_t fig_frame_get_width(fig_frame *self) {
    return self->width;
}

size_t fig_frame_get_height(fig_frame *self) {
    return self->height;
}

fig_bool_t fig_frame_resize_canvas(fig_frame *self, size_t width, size_t height) {
    size_t size = width * height;
    if(size == 0) {
        free(self->data);
        self->data = NULL;
        return 1;
    } else {
        fig_uint8_t *data;
        data = (fig_uint8_t *) realloc(self->data, sizeof(fig_uint8_t) * size);
        if(data != NULL) {
            self->width = width;
            self->height = height;
            self->data = data;
            return 1;
        }
        return 0;
    }
}

void fig_frame_free(fig_frame *self) {
    if(self != NULL) {
        if(self->palette != NULL) {
            fig_palette_free(self->palette);
        }
        free(self->data);
    }
    free(self);
}
