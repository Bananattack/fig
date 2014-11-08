#include <stdlib.h>
#include <fig.h>

typedef struct fig_palette {
    size_t size;
    fig_uint32_t *data;
} fig_palette;

fig_palette *fig_create_palette(void) {
    fig_palette *self = (fig_palette *) malloc(sizeof(fig_palette));
    if(self != NULL) {
        self->size = 0;
        self->data = NULL;
    }
    return self;
}

size_t fig_palette_count_colors(fig_palette *self) {
    return self->size;
}

fig_uint32_t *fig_palette_get_colors(fig_palette *self) {
    return self->data;
}

fig_uint32_t fig_palette_get(fig_palette *self, size_t index) {
    FIG_ASSERT(index < self->size);
    return self->data[index];
}

void fig_palette_set(fig_palette *self, size_t index, fig_uint32_t color) {
    FIG_ASSERT(index < self->size);
    self->data[index] = color;
}

fig_bool_t fig_palette_resize(fig_palette *self, size_t size) {
    if(self->size >= size) {
        self->size = size;
        if(size == 0) {
            free(self->data);
            self->data = NULL;
        }
        return 1;
    } else {
        fig_uint32_t *data;
        data = (fig_uint32_t *) realloc(self->data, sizeof(fig_uint32_t) * size);

        if(data != NULL) {
            self->data = data;
            self->size = size;
            return 1;
        }
        return 0;
    }
}

void fig_palette_free(fig_palette *self) {
    if(self != NULL) {
        if(self->data != NULL) {
            free(self->data);
        }
    }
    free(self);
}
