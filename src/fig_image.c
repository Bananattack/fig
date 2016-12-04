#include <fig.h>

struct fig_image {
    fig_state *state;
    size_t indexed_x;
    size_t indexed_y;
    size_t indexed_width;
    size_t indexed_height;
    size_t canvas_width;
    size_t canvas_height;
    size_t delay;
    fig_disposal_t disposal;
    fig_palette *palette;
    fig_bool_t transparent;
    size_t transparency_index;
    fig_uint8_t *indexed_data;
    fig_uint32_t *canvas_data;
};

fig_image *fig_create_image(fig_state *state) {
    if(state != NULL) {
        fig_image *self = (fig_image *) fig_state_get_allocator(state)(fig_state_get_userdata(state), NULL, 0, sizeof(fig_image));
        if(self != NULL) {
            self->state = state;
            self->indexed_x = 0;
            self->indexed_y = 0;
            self->indexed_width = 0;
            self->indexed_height = 0;
            self->canvas_width = 0;
            self->canvas_height = 0;
            self->delay = 0;
            self->disposal = FIG_DISPOSAL_UNSPECIFIED;
            self->palette = fig_create_palette(state);
            if(self->palette == NULL) {
                return fig_image_free(self), NULL;
            }
            self->transparent = 0;
            self->transparency_index = 0;
            self->indexed_data = NULL;
            self->canvas_data = NULL;
        } else {
            fig_state_set_error_allocation_failed(state);
        }
        return self;
    }
    return NULL;
}

fig_palette *fig_image_get_palette(fig_image *self) {
    return self->palette;
}

size_t fig_image_get_indexed_x(fig_image *self) {
    return self->indexed_x;
}

size_t fig_image_get_indexed_y(fig_image *self) {
    return self->indexed_y;
}

size_t fig_image_get_indexed_width(fig_image *self) {
    return self->indexed_width;
}

size_t fig_image_get_indexed_height(fig_image *self) {
    return self->indexed_height;
}

fig_uint8_t *fig_image_get_indexed_data(fig_image *self) {
    return self->indexed_data;
}

void fig_image_set_indexed_x(fig_image *self, size_t value) {
    self->indexed_x = value;
}

void fig_image_set_indexed_y(fig_image *self, size_t value) {
    self->indexed_y = value;
}

fig_bool_t fig_image_resize_indexed(fig_image *self, size_t width, size_t height) {
    size_t old_size = self->indexed_width * self->indexed_height;
    size_t new_size = width * height;    
    if(new_size == 0) {
        fig_state_get_allocator(self->state)(fig_state_get_userdata(self->state), self->indexed_data, old_size, 0);
        self->indexed_data = NULL;
        self->indexed_width = 0;
        self->indexed_height = 0;
        return 1;
    } else {
        fig_uint8_t *index_data;
        index_data = (fig_uint8_t *) fig_state_get_allocator(self->state)(fig_state_get_userdata(self->state),
            self->indexed_data, old_size, new_size);
        if(index_data == NULL) {
            return 0;
        } else {
            self->indexed_width = width;
            self->indexed_height = height;
            self->indexed_data = index_data;
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
    size_t old_size = self->canvas_width * self->canvas_height;
    size_t new_size = width * height;
    if(new_size == 0) {
        fig_state_get_allocator(self->state)(fig_state_get_userdata(self->state), self->canvas_data, sizeof(fig_uint32_t) * old_size, 0);
        self->canvas_data = NULL;
        self->canvas_width = 0;
        self->canvas_height = 0;
        return 1;
    } else {
        fig_uint32_t *canvas_data;
        canvas_data = (fig_uint32_t *) fig_state_get_allocator(self->state)(fig_state_get_userdata(self->state),
            self->canvas_data, sizeof(fig_uint32_t) * old_size, sizeof(fig_uint32_t) * new_size);
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
        fig_allocator_t alloc = fig_state_get_allocator(self->state);
        void *ud = fig_state_get_userdata(self->state);     

        if(self->palette != NULL) {
            fig_palette_free(self->palette);
        }
   
        alloc(ud, self->indexed_data, self->indexed_width * self->indexed_height, 0);
        alloc(ud, self->canvas_data, sizeof(fig_uint32_t) * self->canvas_width * self->canvas_height, 0);
        alloc(ud, self, sizeof(fig_image), 0);
    }
}
