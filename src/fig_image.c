#include <stdlib.h>
#include <string.h>
#include <fig.h>

typedef struct fig_animation {
    size_t width;
    size_t height;
    fig_palette *palette;
    size_t frame_count;
    size_t frame_capacity;
    fig_frame **frame_data;
    size_t loop_count;
} fig_animation;

fig_animation *fig_create_animation(void) {
    fig_animation *self = (fig_animation *) malloc(sizeof(fig_animation));
    if(self != NULL) {
        self->width = 0;
        self->height = 0;
        self->palette = fig_create_palette();
        self->frame_count = 0;
        self->frame_capacity = 0;
        self->frame_data = NULL;
        self->loop_count = 0;

        if(self->palette == NULL) {
            return fig_animation_free(self), NULL;
        }
    }
    return self;
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

fig_bool_t fig_animation_resize_canvas(fig_animation *self, size_t width, size_t height) {
    self->width = width;
    self->height = height;
    return 1;
}

size_t fig_animation_count_frames(fig_animation *self) {
    return self->frame_count;
}

fig_frame **fig_animation_get_frames(fig_animation *self) {
    return self->frame_data;
}

size_t fig_animation_get_loop_count(fig_animation *self) {
    return self->loop_count;
}

void fig_animation_set_loop_count(fig_animation *self, size_t value) {
    self->loop_count = value;
}

void fig_animation_swap_frames(fig_animation *self, size_t index_a, size_t index_b) {
    fig_frame *temp;
    FIG_ASSERT(index_a < self->frame_count);
    FIG_ASSERT(index_b < self->frame_count);

    temp = self->frame_data[index_a];
    self->frame_data[index_a] = self->frame_data[index_b];
    self->frame_data[index_b] = temp;
}

fig_frame *fig_animation_add_frame(fig_animation *self) {
    fig_frame *frame;
    FIG_ASSERT(self->frame_capacity >= self->frame_count);

    if(self->frame_count == self->frame_capacity) {
        fig_frame **data;
        size_t capacity = self->frame_capacity << 1;
        if(capacity == 0) {
            capacity = 1;
        }

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

fig_frame *fig_animation_insert_frame(fig_animation *self, size_t index) {
    fig_frame *frame;
    fig_frame **data;
    size_t i;
    FIG_ASSERT(index < self->frame_count + 1);

    frame = fig_animation_add_frame(self);
    if(frame == NULL) {
        return NULL;
    }

    data = self->frame_data;
    for(i = self->frame_count - 1; i > index; --i) {
        data[i] = data[i - 1];
    }
    data[index] = frame;
    return frame;
}

void fig_animation_remove(fig_animation *self, size_t index) {
    fig_frame **data;
    size_t i, end;
    FIG_ASSERT(index < self->frame_count);

    data = self->frame_data;
    fig_frame_free(data[index]);
    for(i = index, end = self->frame_count - 1; i < end; ++i) {
        data[i] = data[i + 1];
    }
    --self->frame_count;
}

static void clear_frame(fig_animation *self, fig_frame *frame) {
    size_t i;
    size_t size;
    fig_uint32_t color;
    fig_uint32_t *render_data;

    size = fig_frame_get_render_width(frame) * fig_frame_get_render_height(frame);
    color = 0;
    render_data = fig_frame_get_render_data(frame);
    for(i = 0; i < size; ++i) {
        render_data[i] = color;
    }
}

static void dispose_frame(fig_animation *self, fig_frame *prev, fig_frame *cur, fig_frame *next) {
    fig_palette *palette;
    size_t color_count;
    fig_uint32_t *colors;
    size_t cur_x, cur_y, cur_w, cur_h;
    fig_bool_t cur_transparent;
    size_t cur_transparency_index;
    fig_uint8_t *cur_index_data;
    fig_bool_t next_transparent;
    size_t next_transparency_index;
    fig_uint32_t *next_render_data;
    fig_disposal_t disposal;

    palette = fig_frame_get_render_palette(next, self);
    color_count = fig_palette_count_colors(palette);
    colors = fig_palette_get_colors(palette);
    cur_x = fig_frame_get_x(cur);
    cur_y = fig_frame_get_y(cur);
    cur_w = fig_frame_get_canvas_width(cur);
    cur_h = fig_frame_get_canvas_height(cur);
    cur_transparent = fig_frame_get_transparent(cur);
    cur_transparency_index = fig_frame_get_transparency_index(cur);
    cur_index_data = fig_frame_get_index_data(cur);
    next_transparent = fig_frame_get_transparent(next);
    next_transparency_index = fig_frame_get_transparency_index(next);
    next_render_data = fig_frame_get_render_data(next);
    disposal = fig_frame_get_disposal(cur);

    switch(disposal) {
        case FIG_DISPOSAL_BACKGROUND: {
            size_t i, j;
            for(i = 0; i < cur_h; ++i) {
                for(j = 0; j < cur_w; ++j) {
                    if(!cur_transparent || cur_index_data[i * cur_w + j] != cur_transparency_index) {
                        next_render_data[(cur_y + i) * self->width + (cur_x + j)] = 0;
                    }
                }
            }
            break;
        }
        case FIG_DISPOSAL_PREVIOUS: {
            size_t i, j;
            fig_uint32_t *prev_render_data;

            prev_render_data = prev != NULL ? fig_frame_get_render_data(prev) : NULL;
            
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

static void blit_frame(fig_animation *self, fig_frame *frame) {
    fig_palette *palette;
    size_t color_count;
    fig_uint32_t *colors;
    size_t x, y, w, h;
    fig_bool_t transparent;
    size_t transparency_index;
    fig_uint8_t *index_data;
    fig_uint32_t *render_data;
    size_t i, j;

    palette = fig_frame_get_render_palette(frame, self);
    color_count = fig_palette_count_colors(palette);
    colors = fig_palette_get_colors(palette);
    x = fig_frame_get_x(frame);
    y = fig_frame_get_y(frame);
    w = fig_frame_get_canvas_width(frame);
    h = fig_frame_get_canvas_height(frame);
    transparent = fig_frame_get_transparent(frame);
    transparency_index = fig_frame_get_transparency_index(frame);
    index_data = fig_frame_get_index_data(frame);
    render_data = fig_frame_get_render_data(frame);

    for(i = 0; i < h; ++i) {
        for(j = 0; j < w; ++j) {
            fig_uint8_t index = index_data[i * w + j];

            if(!transparent || index != transparency_index) {
                render_data[(y + i) * self->width + (x + j)] = fig_palette_get(palette, index);
            }
        }
    }
}

fig_bool_t fig_animation_render(fig_animation *self) {
    fig_frame **frames;
    size_t frame_count;
    fig_frame *prev;
    fig_frame *cur;
    fig_frame *next;
    fig_disposal_t disposal;
    size_t i;

    frames = self->frame_data;
    frame_count = self->frame_count;
    prev = NULL;
    cur = NULL;
    next = NULL;

    for(i = 0; i < frame_count; ++i) {
        next = frames[i];
        if(!fig_frame_resize_render(next, self->width, self->height)) {
            return 0;
        }

        if(cur == NULL) {
            clear_frame(self, next);
        } else {
            memcpy(fig_frame_get_render_data(next), fig_frame_get_render_data(cur), sizeof(fig_uint32_t) * self->width * self->height);
            dispose_frame(self, prev, cur, next);
        }

        blit_frame(self, next);

        if(cur != NULL) {
            disposal = fig_frame_get_disposal(cur);
            if(disposal == FIG_DISPOSAL_NONE || disposal == FIG_DISPOSAL_UNSPECIFIED) {
                prev = cur;
            }
        }
        cur = next;
    }
    return 1;
}

void fig_animation_free(fig_animation *self) {
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