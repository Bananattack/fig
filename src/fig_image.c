#include <stdlib.h>
#include <fig.h>

typedef struct fig_image {
    size_t width;
    size_t height;
    fig_palette *palette;
    fig_animation *animation;
} fig_image;

fig_image *fig_create_image() {
    fig_image *self = malloc(sizeof(fig_image));
    if(self != NULL) {
        self->width = 0;
        self->height = 0;
        self->palette = fig_create_palette();
        self->animation = fig_create_animation();
        if(self->palette == NULL
        || self->animation == NULL) {
            fig_image_free(self);
            self = NULL;
        }
    }
    return self;
}

fig_palette *fig_image_get_palette(fig_image *self) {
    return self->palette;
}

fig_animation *fig_image_get_animation(fig_image *self) {
    return self->animation;
}

size_t fig_image_get_width(fig_image *self) {
    return self->width;
}

size_t fig_image_get_height(fig_image *self) {
    return self->height;
}

int fig_image_resize_canvas(fig_image *self, size_t width, size_t height) {
    self->width = width;
    self->height = height;
    return 1;
}

void fig_image_free(fig_image *self) {
    if(self != NULL) {
        if(self->palette != NULL) {
            fig_palette_free(self->palette);
        }
        if(self->animation != NULL) {
            fig_animation_free(self->animation);
        }
    }
    free(self);
}
