#include <fig.h>

struct fig_palette {
    fig_state *state;
    size_t size;
    fig_uint32_t *data;
};

fig_palette *fig_create_palette(fig_state *state) {
    if(state != NULL) {
        fig_palette *self = (fig_palette *) fig_state_get_allocator(state)(fig_state_get_userdata(state), NULL, 0, sizeof(fig_palette));
        if(self != NULL) {
            self->state = state;
            self->size = 0;
            self->data = NULL;
        } else {
            fig_state_set_error_allocation_failed(state);
        }
        return self;
    }
    return NULL;
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
        if(size == 0) {
            fig_state_get_allocator(self->state)(fig_state_get_userdata(self->state),
                self->data, sizeof(fig_uint32_t) * self->size, 0);
            self->data = NULL;
        }
        self->size = size;

        return 1;
    } else {
        fig_uint32_t *data;
        data = (fig_uint32_t *) fig_state_get_allocator(self->state)(fig_state_get_userdata(self->state),
            self->data, sizeof(fig_uint32_t) * self->size, sizeof(fig_uint32_t) * size);

        if(data != NULL) {
            self->data = data;
            self->size = size;
            return 1;
        }
        fig_state_set_error_allocation_failed(self->state);
        return 0;
    }
}

void fig_palette_free(fig_palette *self) {
    if(self != NULL) {
        fig_allocator_t alloc = fig_state_get_allocator(self->state);
        void *ud = fig_state_get_userdata(self->state);    

        if(self->data != NULL) {
            alloc(ud, self->data, sizeof(fig_uint32_t) * self->size, 0);
        }
        alloc(ud, self, sizeof(fig_palette), 0);
    }
}
