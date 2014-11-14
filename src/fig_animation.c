#include <stdlib.h>
#include <string.h>
#include <fig.h>

typedef struct fig_animation {
    size_t width;
    size_t height;
    fig_palette *palette;
    size_t image_count;
    size_t image_capacity;
    fig_image **image_data;
    size_t loop_count;
} fig_animation;

fig_animation *fig_create_animation(void) {
    fig_animation *self = (fig_animation *) malloc(sizeof(fig_animation));
    if(self != NULL) {
        self->width = 0;
        self->height = 0;
        self->palette = fig_create_palette();
        self->image_count = 0;
        self->image_capacity = 0;
        self->image_data = NULL;
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

        data = (fig_image **) realloc(self->image_data, sizeof(fig_image*) * capacity);
        if(data == NULL) {
            return NULL;
        }
        self->image_data = data;
        self->image_capacity = capacity;
    }

    image = fig_create_image();
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

void fig_animation_remove(fig_animation *self, size_t index) {
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

static void clear_image(fig_animation *self, fig_image *image) {
    size_t i;
    size_t size;
    fig_uint32_t color;
    fig_uint32_t *color_data;

    size = fig_image_get_canvas_width(image) * fig_image_get_canvas_height(image);
    color = 0;
    color_data = fig_image_get_color_data(image);
    for(i = 0; i < size; ++i) {
        color_data[i] = color;
    }
}

static void dispose_image(fig_animation *self, fig_image *prev, fig_image *cur, fig_image *next) {
    fig_palette *palette;
    size_t color_count;
    fig_uint32_t *colors;
    size_t cur_x, cur_y, cur_w, cur_h;
    fig_bool_t cur_transparent;
    size_t cur_transparency_index;
    fig_uint8_t *cur_index_data;
    fig_bool_t next_transparent;
    size_t next_transparency_index;
    fig_uint32_t *next_color_data;
    fig_disposal_t disposal;

    palette = fig_image_get_render_palette(next, self);
    color_count = fig_palette_count_colors(palette);
    colors = fig_palette_get_colors(palette);
    fig_image_get_region(cur, &cur_x, &cur_y, &cur_w, &cur_h);
    cur_transparent = fig_image_get_transparent(cur);
    cur_transparency_index = fig_image_get_transparency_index(cur);
    cur_index_data = fig_image_get_index_data(cur);
    next_transparent = fig_image_get_transparent(next);
    next_transparency_index = fig_image_get_transparency_index(next);
    next_color_data = fig_image_get_color_data(next);
    disposal = fig_image_get_disposal(cur);

    switch(disposal) {
        case FIG_DISPOSAL_BACKGROUND: {
            size_t i, j;
            for(i = 0; i < cur_h; ++i) {
                for(j = 0; j < cur_w; ++j) {
                    size_t k = (cur_y + i) * self->width + (cur_x + j);
                    if(!cur_transparent || cur_index_data[k] != cur_transparency_index) {
                        next_color_data[k] = 0;
                    }
                }
            }
            break;
        }
        case FIG_DISPOSAL_PREVIOUS: {
            size_t i, j;
            fig_uint32_t *prev_color_data;

            prev_color_data = prev != NULL ? fig_image_get_color_data(prev) : NULL;
            
            for(i = 0; i < cur_h; ++i) {
                for(j = 0; j < cur_w; ++j) {
                    size_t k = (cur_y + i) * self->width + (cur_x + j);
                    if(!cur_transparent || cur_index_data[k] != cur_transparency_index) {
                        next_color_data[k] = prev != NULL ? prev_color_data[k] : 0;
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

static void blit_image(fig_animation *self, fig_image *image) {
    fig_palette *palette;
    size_t color_count;
    fig_uint32_t *colors;
    size_t x, y, w, h;
    fig_bool_t transparent;
    size_t transparency_index;
    fig_uint8_t *index_data;
    fig_uint32_t *color_data;
    size_t i, j;

    palette = fig_image_get_render_palette(image, self);
    color_count = fig_palette_count_colors(palette);
    colors = fig_palette_get_colors(palette);
    fig_image_get_region(image, &x, &y, &w, &h);
    transparent = fig_image_get_transparent(image);
    transparency_index = fig_image_get_transparency_index(image);
    index_data = fig_image_get_index_data(image);
    color_data = fig_image_get_color_data(image);

    for(i = 0; i < h; ++i) {
        for(j = 0; j < w; ++j) {
            size_t k = (y + i) * self->width + (x + j);
            fig_uint8_t index = index_data[k];

            if(!transparent || index != transparency_index) {
                color_data[k] = fig_palette_get(palette, index);
            }
        }
    }
}

void fig_animation_render(fig_animation *self) {
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

    for(i = 0; i < image_count; ++i) {
        next = images[i];
        if(cur == NULL) {
            clear_image(self, next);
        } else {
            memcpy(fig_image_get_color_data(next), fig_image_get_color_data(cur), sizeof(fig_uint32_t) * self->width * self->height);
            dispose_image(self, prev, cur, next);
        }

        blit_image(self, next);

        if(cur != NULL) {
            disposal = fig_image_get_disposal(cur);
            if(disposal == FIG_DISPOSAL_NONE || disposal == FIG_DISPOSAL_UNSPECIFIED) {
                prev = cur;
            }
        }
        cur = next;
    }
}

void fig_animation_free(fig_animation *self) {
    if(self != NULL) {
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
            free(self->image_data);
        }
    }
    free(self);
}