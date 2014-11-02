#include <stdlib.h>
#include <fig.h>

typedef struct fig_frame {
    size_t x;
    size_t y;
    size_t width;
    size_t height;
    fig_uint32_t delay;
    fig_palette *palette;
} fig_frame;

fig_frame *fig_create_frame() {
    fig_frame *self = (fig_frame *) malloc(sizeof(fig_frame));
    if(self != NULL) {
        self->x = 0;
        self->y = 0;
        self->width = 0;
        self->height = 0;
        self->delay = 0;
        self->palette = fig_create_palette();
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

void fig_frame_free(fig_frame *self) {
    if(self != NULL) {
        if(self->palette != NULL) {
            fig_palette_free(self->palette);
        }
    }
    free(self);
}
