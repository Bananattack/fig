#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fig.h>

struct fig_input {
    fig_state *state;
    void *userdata;
    fig_input_callbacks callbacks;
};

fig_input *fig_create_input(fig_state *state, fig_input_callbacks callbacks, void *ud) {
    if(state != NULL) {
        fig_input *self = (fig_input *) fig_state_get_allocator(state)(fig_state_get_userdata(state), NULL, 0, sizeof(fig_input));
        if(self != NULL) {
            self->state = state;
            self->userdata = ud;
            self->callbacks = callbacks;
        } else {
            fig_state_set_error_allocation_failed(state);
        }
        return self;
    }
    return NULL;
}



static size_t fig_file_read_(void *ud, void *dest, size_t size, size_t count) {
    return fread(dest, size, count, (FILE *) ud);
}

static fig_bool_t fig_file_seek_(void *ud, ptrdiff_t offset, fig_seek_origin_t whence) {
    return !fseek((FILE *) ud, (long) offset, (int) whence);
}

static ptrdiff_t fig_file_tell_(void *ud) {
    return (ptrdiff_t) ftell((FILE *) ud);
}

static const fig_input_callbacks fig_file_input_cb_ = {
    fig_file_read_,
    fig_file_seek_,
    fig_file_tell_,
    NULL
};

fig_input *fig_create_file_input(fig_state *state, FILE *f) {
    if(f != NULL) {
        return fig_create_input(state, fig_file_input_cb_, f);
    } else {
        fig_state_set_error(state, "file handle is invalid");
        return NULL;
    }        
}



typedef struct {
    fig_state *state;
    const char *data;
    size_t position;
    size_t length;
} fig_memory_view_;

static size_t fig_memory_view_read_(void *ud, void *dest, size_t size, size_t count) {
    fig_memory_view_ *mv = (fig_memory_view_ *) ud;
    if(size && count) {
        if(size * count > mv->length - mv->position) {
            count = (mv->length - mv->position) / size;
        }
        size *= count;
        memcpy(dest, mv->data + mv->position, size);
        mv->position += size;
    } else {
        size = count = 0;
    }
    return count;
}

static fig_bool_t fig_memory_view_seek_(void *ud, ptrdiff_t offset, fig_seek_origin_t whence) {
    fig_memory_view_ *mv = (fig_memory_view_ *) ud;
    switch (whence) {
        case FIG_SEEK_SET:
            if (offset < 0 || (size_t) offset >= mv->length) {
                return 0;
            }
            mv->position = (size_t) offset;
            break;
        case FIG_SEEK_CUR:
            if (offset < 0) {
                size_t magnitude = (size_t) -offset;
                if(mv->position >= magnitude) {
                    mv->position -= magnitude;
                } else {
                    return 0;
                }
            } else {
                size_t magnitude = (size_t) offset;
                if (mv->position + magnitude < mv->length) {
                    mv->position += magnitude;
                } else {
                    return 0;
                }
            }
            break;
        case FIG_SEEK_END:
            if (offset < 0 || (size_t) offset >= mv->length) {
                return 0;
            }
            mv->position = (mv->length - 1) - (size_t) offset;
            break;
        default:
            FIG_ASSERT(0);
            return 0;
    }
    return 1;
}

static ptrdiff_t fig_memory_view_tell_(void *ud) {
    return (ptrdiff_t) ((fig_memory_view_ *) ud)->position;
}

static void fig_memory_view_cleanup_(void *ud) {
    if(ud != NULL) {
        fig_memory_view_ *mv = (fig_memory_view_ *) ud;
        fig_state_get_allocator(mv->state)(fig_state_get_userdata(mv->state), mv, sizeof(fig_memory_view_), 0);
    }
}

static const fig_input_callbacks fig_memory_view_input_cb_ = {
    fig_memory_view_read_,
    fig_memory_view_seek_,
    fig_memory_view_tell_,
    fig_memory_view_cleanup_
};

fig_input *fig_create_memory_input(fig_state *state, void *data, size_t length) {
    if(state != NULL) {
        fig_input *self;
        fig_memory_view_ *mv;

        if(data == NULL) {
            fig_state_set_error(state, "data is NULL");
            return NULL;
        }

        mv = (fig_memory_view_ *) fig_state_get_allocator(state)(fig_state_get_userdata(state), NULL, 0, sizeof(fig_memory_view_));

        if(mv == NULL) {
            return NULL;
        } else {
            fig_state_set_error_allocation_failed(state);
            return NULL;
        }

        mv->state = state;
        mv->data = data;
        mv->length = length;
        mv->position = 0;

        self = fig_create_input(state, fig_memory_view_input_cb_, mv);

        if(self == NULL) {
            return fig_memory_view_cleanup_(mv), NULL;
        }
        return self;
    }
    return NULL;
}



size_t fig_input_read(fig_input *self, void *dest, size_t size, size_t count) {
    if(self->callbacks.read) {
        return self->callbacks.read(self->userdata, dest, size, count);
    }
    return 0;
}

fig_bool_t fig_input_seek(fig_input *self, ptrdiff_t offset, fig_seek_origin_t whence) {
    if(self->callbacks.seek) {
        return self->callbacks.seek(self->userdata, offset, whence);
    }
    return 0;
}

ptrdiff_t fig_input_tell(fig_input *self) {
    if(self->callbacks.tell) {
        return self->callbacks.tell(self->userdata);
    }
    return -1;
}

fig_bool_t fig_input_read_u8(fig_input *self, fig_uint8_t *dest) {
    return fig_input_read(self, dest, 1, 1) == 1;
}

fig_bool_t fig_input_read_le_u16(fig_input *self, fig_uint16_t *dest) {
    fig_uint8_t result[2];
    if(fig_input_read(self, result, 2, 1) == 1) {
        *dest = result[1] << 8 | result[0];
        return 1;
    }
    return 0;
}

fig_bool_t fig_input_read_le_u32(fig_input *self, fig_uint32_t *dest) {
    fig_uint8_t result[4];
    if(fig_input_read(self, result, 4, 1) == 1) {
        *dest = result[3] << 24 | result[2] << 16 | result[1] << 8 | result[0];
        return 1;
    }
    return 0;
}

void fig_input_free(fig_input *self) {
    if(self != NULL) {
        if(self->callbacks.cleanup) {
            self->callbacks.cleanup(self->userdata);
        }
        fig_state_get_allocator(self->state)(fig_state_get_userdata(self->state), self, sizeof(fig_input), 0);
    }
}



struct fig_output {
    fig_state *state;
    void *userdata;
    fig_output_callbacks callbacks;
};

fig_output *fig_create_output(fig_state *state, fig_output_callbacks callbacks, void *ud) {
    if(state != NULL) {
        fig_output *self = (fig_output *) fig_state_get_allocator(state)(fig_state_get_userdata(state), NULL, 0, sizeof(fig_output));

        if(self != NULL) {
            self->state = state;
            self->userdata = ud;
            self->callbacks = callbacks;
        } else {
            fig_state_set_error_allocation_failed(state);
        }
        return self;
    }
    return NULL;
}



static size_t fig_file_write_(void *ud, const void *src, size_t size, size_t count) {
    return fwrite(src, size, count, (FILE *) ud);
}

static const fig_output_callbacks fig_file_output_cb_ = {
    fig_file_write_,
    NULL
};

fig_output *fig_create_file_output(fig_state *state, FILE *f) {
    if(f != NULL) {
        return fig_create_output(state, fig_file_output_cb_, f);
    } else {
        fig_state_set_error(state, "file handle is invalid");
        return NULL;
    }        
}



typedef struct {
    fig_state *state;
    size_t size;
    size_t capacity;
    char *data;
} fig_memory_buffer_;

static size_t fig_memory_buffer_write_(void *ud, const void *src, size_t size, size_t count) {
    size_t new_capacity;
    fig_memory_buffer_ *buf = (fig_memory_buffer_ *) ud;

    new_capacity = buf->capacity;

    while(buf->size + size * count >= new_capacity && new_capacity != 0) {
        new_capacity <<= 1;
    }
    if(new_capacity == 0) {
        fig_state_set_error(buf->state, "capacity requested is too large");
        return 0;
    }
    if(new_capacity != buf->capacity) {
        char *new_data;
        new_data = (char *) fig_state_get_allocator(buf->state)(fig_state_get_userdata(buf->state),
            buf->data, buf->capacity, new_capacity);

        if(new_data == NULL) {
            fig_state_set_error_allocation_failed(buf->state);
            return 0;
        } else {
            buf->data = new_data;
            buf->capacity = new_capacity;
        }
    }
    
    memcpy(buf->data + buf->size, src, size * count);
    buf->size += size * count;
    return size;
}

static void fig_memory_buffer_cleanup_(void *ud) {
    if(ud != NULL) {
        fig_memory_buffer_ *buf = (fig_memory_buffer_ *) ud;
        fig_allocator_t alloc = fig_state_get_allocator(buf->state);
        void *alloc_ud = fig_state_get_userdata(buf->state);
        if(buf->data != NULL) {
            alloc(alloc_ud, buf->data, buf->capacity, 0);
        }
        alloc(alloc_ud, buf, sizeof(fig_memory_buffer_), 0);
    }
}

static const fig_output_callbacks fig_memory_buffer_output_cb_ = {
    fig_memory_buffer_write_,
    fig_memory_buffer_cleanup_
};

fig_output *fig_create_buffer_output(fig_state *state) {
    if(state != NULL) {
        fig_output *self;
        fig_memory_buffer_ *buf;

        buf = (fig_memory_buffer_ *) fig_state_get_allocator(state)(fig_state_get_userdata(state), NULL, 0, sizeof(fig_memory_buffer_));

        if(buf == NULL) {
            fig_state_set_error_allocation_failed(state);
            return NULL;
        }  

        buf->state = state;
        buf->size = 0;
        buf->capacity = 128;
        buf->data = (char *) fig_state_get_allocator(state)(fig_state_get_userdata(state), NULL, 0, buf->capacity);

        if(buf->data == NULL) {
            fig_state_set_error_allocation_failed(state);
            return fig_memory_buffer_cleanup_(buf), NULL;
        }

        self = fig_create_output(state, fig_memory_buffer_output_cb_, buf);
        if(self == NULL) {
            return fig_memory_buffer_cleanup_(buf), NULL;
        }
        return self;
    }
    return NULL;
}

fig_bool_t buffer_is_valid(fig_output *self) {
    fig_output_callbacks callbacks = fig_output_get_callbacks(self);
    return callbacks.write == fig_memory_buffer_output_cb_.write
        && callbacks.cleanup == fig_memory_buffer_output_cb_.cleanup;
}

size_t fig_buffer_output_get_size(fig_output *self) {
    FIG_ASSERT(buffer_is_valid(self));
    return ((fig_memory_buffer_ *) self->userdata)->size;
}

const char* fig_buffer_output_get_data(fig_output *self) {
    FIG_ASSERT(buffer_is_valid(self));
    return ((fig_memory_buffer_ *) self->userdata)->data;
}



fig_output_callbacks fig_output_get_callbacks(fig_output *self) {
    return self->callbacks;
}

size_t fig_output_write(fig_output *self, const void *src, size_t size, size_t count) {
    if(self->callbacks.write) {
        return self->callbacks.write(self->userdata, src, size, count);
    }
    return 0;
}

fig_bool_t fig_output_write_u8(fig_output *self, fig_uint8_t value) {
    return fig_output_write(self, &value, 1, 1) == 1;
}

fig_bool_t fig_output_write_le_u16(fig_output *self, fig_uint16_t value) {
    fig_uint8_t bytes[2];
    bytes[0] = value & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;
    return fig_output_write(self, &bytes, 2, 1) == 1;
}

fig_bool_t fig_output_write_le_u32(fig_output *self, fig_uint32_t value) {
    fig_uint8_t bytes[4];
    bytes[0] = value & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;
    bytes[2] = (value >> 16) & 0xFF;
    bytes[3] = (value >> 24) & 0xFF;
    return fig_output_write(self, &bytes, 4, 1) == 1;
}

void fig_output_free(fig_output *self) {
    if(self != NULL) {
        if(self->callbacks.cleanup) {
            self->callbacks.cleanup(self->userdata);
        }
        fig_state_get_allocator(self->state)(fig_state_get_userdata(self->state), self, sizeof(fig_output), 0);
    }
}
