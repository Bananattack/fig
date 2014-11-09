#include <stdlib.h>
#include <fig.h>

typedef struct fig_image {
    size_t width;
    size_t height;
    fig_palette *palette;
    size_t frame_count;
    size_t frame_capacity;
    fig_frame **frame_data;
    size_t loop_count;
} fig_image;

fig_image *fig_create_image(void) {
    fig_image *self = (fig_image *) malloc(sizeof(fig_image));
    if(self != NULL) {
        self->width = 0;
        self->height = 0;
        self->palette = fig_create_palette();
        self->frame_count = 0;
        self->frame_capacity = 0;
        self->frame_data = NULL;
        self->loop_count = 0;

        if(self->palette == NULL) {
            return fig_image_free(self), NULL;
        }
    }
    return self;
}

fig_palette *fig_image_get_palette(fig_image *self) {
    return self->palette;
}

size_t fig_image_get_width(fig_image *self) {
    return self->width;
}

size_t fig_image_get_height(fig_image *self) {
    return self->height;
}

fig_bool_t fig_image_resize_canvas(fig_image *self, size_t width, size_t height) {
    self->width = width;
    self->height = height;
    return 1;
}

size_t fig_image_count_frames(fig_image *self) {
    return self->frame_count;
}

fig_frame **fig_image_get_frames(fig_image *self) {
    return self->frame_data;
}

size_t fig_image_get_loop_count(fig_image *self) {
    return self->loop_count;
}

void fig_image_set_loop_count(fig_image *self, size_t loop_count) {
    self->loop_count = loop_count;
}

void fig_image_swap_frames(fig_image *self, size_t index_a, size_t index_b) {
    fig_frame *temp;
    FIG_ASSERT(index_a < self->frame_count);
    FIG_ASSERT(index_b < self->frame_count);

    temp = self->frame_data[index_a];
    self->frame_data[index_a] = self->frame_data[index_b];
    self->frame_data[index_b] = temp;
}

fig_frame *fig_image_add_frame(fig_image *self) {
    fig_frame *frame;
    FIG_ASSERT(self->frame_capacity >= self->frame_count);

    if(self->frame_count == self->frame_capacity) {
        size_t capacity = self->frame_capacity << 1;
        if(capacity == 0) {
            capacity = 1;
        }

        fig_frame **data;
        data = (fig_frame **) realloc(self->frame_data, sizeof(fig_frame*) * capacity);
        if(data == NULL) {
            return NULL;
        }
        self->frame_data = data;
        self->frame_capacity = capacity;
    }

    frame = fig_create_frame();
    if(frame != NULL) {
        self->frame_data[self->frame_count++] = frame;
    }
    return frame;
}

fig_frame *fig_image_insert_frame(fig_image *self, size_t index) {
    fig_frame *frame;
    fig_frame **data;
    size_t j;
    FIG_ASSERT(index < self->frame_count + 1);

    frame = fig_image_add_frame(self);
    if(frame == NULL) {
        return NULL;
    }

    data = self->frame_data;
    for(j = self->frame_count - 1; j > index; --j) {
        data[j] = data[j - 1];
    }
    data[index] = frame;
    return frame;
}

void fig_animation_remove(fig_image *self, size_t index) {
    fig_frame **data;
    size_t j, end;
    FIG_ASSERT(index < self->frame_count);

    data = self->frame_data;
    fig_frame_free(data[index]);
    for(j = index, end = self->frame_count - 1; j < end; ++j) {
        data[j] = data[j + 1];
    }
    --self->frame_count;
}


void fig_image_free(fig_image *self) {
    if(self != NULL) {
        if(self->palette != NULL) {
            fig_palette_free(self->palette);
        }
        if(self->frame_data != NULL) {
            fig_frame **data;
            size_t i;
            data = self->frame_data;
            for(i = 0; i < self->frame_count; ++i) {
                fig_frame_free(data[i]);
            }
            free(self->frame_data);
        }
    }
    free(self);
}