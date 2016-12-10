#include <string.h>
#include <fig.h>

struct fig_animation {
    fig_state *state;
    size_t width;
    size_t height;
    fig_palette *palette;
    size_t image_count;
    size_t image_capacity;
    fig_image **image_data;
    size_t loop_count;
};

fig_animation *fig_create_animation(fig_state *state) {
    if(state != NULL) {
        fig_animation *self = (fig_animation *) fig_state_get_allocator(state)(fig_state_get_userdata(state), NULL, 0, sizeof(fig_animation));
        if(self != NULL) {
            self->state = state;
            self->width = 0;
            self->height = 0;
            self->palette = fig_create_palette(state);
            self->image_count = 0;
            self->image_capacity = 0;
            self->image_data = NULL;
            self->loop_count = 0;

            if(self->palette == NULL) {
                return fig_animation_free(self), NULL;
            }
        } else {
            fig_state_set_error_allocation_failed(state);
        }
        return self;
    }
    return NULL;
}

fig_palette *fig_animation_get_palette(fig_animation *self) {
    return self->palette;
}

size_t fig_animation_get_width(fig_animation *self) {
    return self->width;
}

size_t fig_animation_get_height(fig_animation *self) {
    return self->height;
}

void fig_animation_set_dimensions(fig_animation *self, size_t width, size_t height) {
    self->width = width;
    self->height = height;
}

size_t fig_animation_count_images(fig_animation *self) {
    return self->image_count;
}

fig_image **fig_animation_get_images(fig_animation *self) {
    return self->image_data;
}

size_t fig_animation_get_loop_count(fig_animation *self) {
    return self->loop_count;
}

void fig_animation_set_loop_count(fig_animation *self, size_t value) {
    self->loop_count = value;
}

void fig_animation_swap_images(fig_animation *self, size_t index_a, size_t index_b) {
    fig_image *temp;
    FIG_ASSERT(index_a < self->image_count);
    FIG_ASSERT(index_b < self->image_count);

    temp = self->image_data[index_a];
    self->image_data[index_a] = self->image_data[index_b];
    self->image_data[index_b] = temp;
}

fig_image *fig_animation_add_image(fig_animation *self) {
    fig_image *image;
    FIG_ASSERT(self->image_capacity >= self->image_count);

    if(self->image_count == self->image_capacity) {
        fig_image **data;
        size_t capacity = self->image_capacity << 1;
        if(capacity == 0) {
            capacity = 1;
        }

        data = (fig_image **) fig_state_get_allocator(self->state)(fig_state_get_userdata(self->state),
            self->image_data, sizeof(fig_image *) * self->image_capacity, sizeof(fig_image *) * capacity);
        if(data == NULL) {
            return NULL;
        }
        self->image_data = data;
        self->image_capacity = capacity;
    }

    image = fig_create_image(self->state);
    if(image != NULL) {
        self->image_data[self->image_count++] = image;
    }
    return image;
}

fig_image *fig_animation_insert_image(fig_animation *self, size_t index) {
    fig_image *image;
    fig_image **data;
    size_t i;
    FIG_ASSERT(index < self->image_count + 1);

    image = fig_animation_add_image(self);
    if(image == NULL) {
        return NULL;
    }

    data = self->image_data;
    for(i = self->image_count - 1; i > index; --i) {
        data[i] = data[i - 1];
    }
    data[index] = image;
    return image;
}

void fig_animation_remove_image(fig_animation *self, size_t index) {
    fig_image **data;
    size_t i, end;
    FIG_ASSERT(index < self->image_count);

    data = self->image_data;
    fig_image_free(data[index]);
    for(i = index, end = self->image_count - 1; i < end; ++i) {
        data[i] = data[i + 1];
    }
    --self->image_count;
}

static void fig_clear_image_(fig_animation *self, fig_image *image) {
    size_t i;
    size_t size;
    fig_uint32_t color;
    fig_uint32_t *render_data;

    size = fig_image_get_render_width(image) * fig_image_get_render_height(image);
    color = 0;
    render_data = fig_image_get_render_data(image);
    for(i = 0; i < size; ++i) {
        render_data[i] = color;
    }
}

static void fig_dispose_indexed_(fig_animation *self, fig_image *prev, fig_image *cur, fig_image *next) {
    size_t cur_x, cur_y, cur_w, cur_h;
    fig_bool_t cur_transparent;
    size_t cur_transparency_index;
    fig_uint8_t *cur_index_data;
    fig_uint32_t *next_render_data;
    fig_disposal_t disposal;

    cur_x = fig_image_get_origin_x(cur);
    cur_y = fig_image_get_origin_y(cur);
    cur_w = fig_image_get_indexed_width(cur);
    cur_h = fig_image_get_indexed_height(cur);
    cur_transparent = fig_image_get_transparent(cur);
    cur_transparency_index = fig_image_get_transparency_index(cur);
    cur_index_data = fig_image_get_indexed_data(cur);
    next_render_data = fig_image_get_render_data(next);
    disposal = fig_image_get_disposal(cur);

    switch(disposal) {
        case FIG_DISPOSAL_BACKGROUND: {
            size_t i, j;
            for(i = 0; i < cur_h; ++i) {
                for(j = 0; j < cur_w; ++j) {
                    if(!cur_transparent || cur_index_data[i * cur_w + j] != cur_transparency_index) {
                        size_t k = (cur_y + i) * self->width + (cur_x + j);
                        next_render_data[k] = 0;
                    }
                }
            }
            break;
        }
        case FIG_DISPOSAL_PREVIOUS: {
            size_t i, j;
            fig_uint32_t *prev_render_data;

            prev_render_data = prev != NULL ? fig_image_get_render_data(prev) : NULL;
            
            for(i = 0; i < cur_h; ++i) {
                for(j = 0; j < cur_w; ++j) {
                    if(!cur_transparent || cur_index_data[i * cur_w + j] != cur_transparency_index) {
                        size_t k = (cur_y + i) * self->width + (cur_x + j);
                        next_render_data[k] = prev != NULL ? prev_render_data[k] : 0;
                    }
                }
            }
            break;
        }
        case FIG_DISPOSAL_UNSPECIFIED:
        case FIG_DISPOSAL_NONE:
        default:
            break;
    }
}

static void fig_blit_indexed_(fig_animation *self, fig_image *image) {
    fig_palette *palette;
    size_t x, y, w, h;
    fig_bool_t transparent;
    size_t transparency_index;
    fig_uint8_t *index_data;
    fig_uint32_t *render_data;
    size_t i, j;

    palette = fig_animation_get_render_palette(self, image);
    x = fig_image_get_origin_x(image);
    y = fig_image_get_origin_y(image);
    w = fig_image_get_indexed_width(image);
    h = fig_image_get_indexed_height(image);
    transparent = fig_image_get_transparent(image);
    transparency_index = fig_image_get_transparency_index(image);
    index_data = fig_image_get_indexed_data(image);
    render_data = fig_image_get_render_data(image);

    for(i = 0; i < h; ++i) {
        for(j = 0; j < w; ++j) {
            fig_uint8_t index = index_data[i * w + j];

            if(!transparent || index != transparency_index) {
                size_t k = (y + i) * self->width + (x + j);
                render_data[k] = fig_palette_get(palette, index);
            }
        }
    }
}

fig_bool_t fig_animation_render_images(fig_animation *self) {
    fig_image **images;
    size_t image_count;
    fig_image *prev;
    fig_image *cur;
    fig_image *next;
    fig_disposal_t disposal;
    size_t i;

    images = self->image_data;
    image_count = self->image_count;
    prev = NULL;
    cur = NULL;
    next = NULL;

    if(self->width == 0 && self->height == 0) {
        fig_state_set_error(self->state, "image is empty");
        return 0;
    }

    for(i = 0; i < image_count; ++i) {
        next = images[i];
        if(fig_image_get_render_width(next) != self->width
        || fig_image_get_render_height(next) != self->height) {
            if(!fig_image_resize_render(next, self->width, self->height)) {
                return 0;
            }
        }

        if(cur == NULL) {
            fig_clear_image_(self, next);
        } else {
            memcpy(fig_image_get_render_data(next), fig_image_get_render_data(cur), sizeof(fig_uint32_t) * self->width * self->height);
            fig_dispose_indexed_(self, prev, cur, next);
        }

        fig_blit_indexed_(self, next);

        if(cur != NULL) {
            disposal = fig_image_get_disposal(cur);
            if(disposal == FIG_DISPOSAL_NONE || disposal == FIG_DISPOSAL_UNSPECIFIED) {
                prev = cur;
            }
        }
        cur = next;
    }
    return 1;
}


fig_palette *fig_animation_get_render_palette(fig_animation *self, fig_image *image) {
    fig_palette *local_palette = fig_image_get_palette(image);
    if(fig_palette_count_colors(local_palette) > 0) {
        return local_palette;
    } else {
        return self->palette;
    }
}

void fig_animation_free(fig_animation *self) {
    if(self != NULL) {
        fig_allocator_t alloc = fig_state_get_allocator(self->state);
        void *ud = fig_state_get_userdata(self->state);

        if(self->palette != NULL) {
            fig_palette_free(self->palette);
        }
        if(self->image_data != NULL) {
            fig_image **data;
            size_t i;

            data = self->image_data;
            for(i = 0; i < self->image_count; ++i) {
                fig_image_free(data[i]);
            }
            alloc(ud, data, sizeof(fig_image *) * self->image_count, 0);
        }
        alloc(ud, self, sizeof(fig_animation), 0);
    }
}