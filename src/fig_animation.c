#include <stdlib.h>
#include <fig.h>

typedef struct fig_animation {
    size_t loop_count;
    size_t size;
    size_t capacity;
    fig_frame **data;
} fig_animation;

fig_animation *fig_create_animation() {
    fig_animation *self = (fig_animation *) malloc(sizeof(fig_animation));
    if(self != NULL) {
        self->loop_count = 0;
        self->size = 0;
        self->capacity = 0;
        self->data = NULL;
    }
    return self;
}

size_t fig_animation_get_size(fig_animation *self) {
    return self->size;
}

fig_frame **fig_animation_get_data(fig_animation *self) {
    return self->data;
}

fig_frame *fig_animation_get(fig_animation *self, size_t index) {
    FIG_ASSERT(index < self->size);
    return self->data[index];
}

void fig_animation_exchange(fig_animation *self, size_t index_a, size_t index_b) {
    fig_frame *temp;
    FIG_ASSERT(index_a < self->size);
    FIG_ASSERT(index_b < self->size);

    temp = self->data[index_a];
    self->data[index_a] = self->data[index_b];
    self->data[index_b] = temp;
}

fig_frame *fig_animation_add(fig_animation *self) {
    fig_frame *frame;
    FIG_ASSERT(self->capacity >= self->size);

    if(self->size == self->capacity) {
        size_t capacity = self->capacity << 1;
        if(capacity == 0) {
            capacity = 1;
        }

        fig_frame **data;
        data = (fig_frame **) realloc(self->data, sizeof(fig_frame*) * capacity);
        if(data == NULL) {
            return NULL;
        }
        self->data = data;
        self->capacity = capacity;
    }

    frame = fig_create_frame();
    if(frame != NULL) {
        self->data[self->size++] = frame;
    }
    return frame;
}

fig_frame *fig_animation_insert(fig_animation *self, size_t index) {
    fig_frame *frame;
    fig_frame **data;
    size_t j;
    FIG_ASSERT(index < self->size + 1);

    frame = fig_animation_add(self);
    if(frame == NULL) {
        return NULL;
    }

    data = self->data;
    for(j = self->size - 1; j > index; --j) {
        data[j] = data[j - 1];
    }
    data[index] = frame;
    return frame;
}

void fig_animation_remove(fig_animation *self, size_t index) {
    fig_frame **data;
    size_t j, end;
    FIG_ASSERT(index < self->size);

    data = self->data;
    fig_frame_free(data[index]);
    for(j = index, end = self->size - 1; j < end; j++) {
        data[j] = data[j + 1];
    }
    self->size--;
}

void fig_animation_free(fig_animation *self) {
    if(self != NULL) {
        if(self->data != NULL) {
            fig_frame **data;
            size_t i;
            data = self->data;
            for(i = 0; i < self->size; i++) {
                fig_frame_free(data[i]);
            }
            free(self->data);
        }
    }
    free(self);
}
