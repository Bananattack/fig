#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fig.h>

struct fig_source {
    fig_state *state;
    void *userdata;
    fig_source_callbacks callbacks;
};

fig_source *fig_create_source(fig_state *state, fig_source_callbacks callbacks, void *ud) {
    if(state != NULL) {
        fig_source *self = (fig_source *) fig_state_get_allocator(state)(fig_state_get_userdata(state), NULL, 0, sizeof(fig_source));
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




static size_t file_read(void *ud, void *dest, size_t size, size_t count) {
    return fread(dest, size, count, (FILE *) ud);
}

static fig_bool_t file_seek(void *ud, ptrdiff_t offset, fig_seek_origin_t whence) {
    return !fseek((FILE *) ud, (long) offset, (int) whence);
}

static ptrdiff_t file_tell(void *ud) {
    return (ptrdiff_t) ftell((FILE *) ud);
}

static const fig_source_callbacks file_source_cb = {
    file_read,
    file_seek,
    file_tell,
    NULL
};

fig_source *fig_create_file_source(fig_state *state, FILE *f) {
    if(f != NULL) {
        return fig_create_source(state, file_source_cb, f);
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
} memfile;

static size_t memfile_read(void *ud, void *dest, size_t size, size_t count) {
    memfile *mf = (memfile *) ud;
    if(size && count) {
        if(size * count > mf->length - mf->position) {
            count = (mf->length - mf->position) / size;
        }
        size *= count;
        memcpy(dest, mf->data + mf->position, size);
        mf->position += size;
    } else {
        size = count = 0;
    }
    return count;
}

static fig_bool_t memfile_seek(void *ud, ptrdiff_t offset, fig_seek_origin_t whence) {
    memfile *mf = (memfile *) ud;
    switch (whence) {
        case FIG_SEEK_SET:
            if (offset < 0 || offset >= mf->length) {
                return 0;
            }
            mf->position = (size_t) offset;
            break;
        case FIG_SEEK_CUR:
            if (offset < 0) {
                size_t magnitude = (size_t) -offset;
                if(mf->position >= magnitude) {
                    mf->position -= magnitude;
                } else {
                    return 0;
                }
            } else {
                size_t magnitude = (size_t) offset;
                if (mf->position + magnitude < mf->length) {
                    mf->position += magnitude;
                } else {
                    return 0;
                }
            }
            break;
        case FIG_SEEK_END:
            if (offset < 0 || offset >= mf->length) {
                return 0;
            }
            mf->position = (mf->length - 1) - (size_t) offset;
            break;
        default:
            FIG_ASSERT(0);
            return 0;
    }
    return 1;
}

static ptrdiff_t memfile_tell(void *ud) {
    return (ptrdiff_t) ((memfile *) ud)->position;
}

static void memfile_cleanup(void *ud) {
    if(ud != NULL) {
        memfile *mf = (memfile *) ud;
        fig_state_get_allocator(mf->state)(fig_state_get_userdata(mf->state), mf, sizeof(memfile), 0);
    }
}

static const fig_source_callbacks memfile_source_cb = {
    memfile_read,
    memfile_seek,
    memfile_tell,
    memfile_cleanup
};

fig_source *fig_create_memory_source(fig_state *state, void *data, size_t length) {
    if(state != NULL) {
        fig_source *self;
        memfile *mf;

        if(data == NULL) {
            fig_state_set_error(state, "data is NULL");
            return NULL;
        }

        mf = (memfile *) fig_state_get_allocator(state)(fig_state_get_userdata(state), NULL, 0, sizeof(memfile));
        if(mf == NULL) {
            return NULL;
        } else {
            fig_state_set_error_allocation_failed(state);
            return NULL;
        }  
        mf->state = state;
        mf->data = data;
        mf->length = length;
        mf->position = 0;

        self = fig_create_source(state, file_source_cb, mf);
        if(self == NULL) {
            memfile_cleanup(mf);
            return NULL;
        }
        return self;
    }
    return NULL;
}




size_t fig_source_read(fig_source *self, void *dest, size_t size, size_t count) {
    if(self->callbacks.read) {
        return self->callbacks.read(self->userdata, dest, size, count);
    }
    return 0;
}

fig_bool_t fig_source_seek(fig_source *self, ptrdiff_t offset, fig_seek_origin_t whence) {
    if(self->callbacks.seek) {
        return self->callbacks.seek(self->userdata, offset, whence);
    }
    return 0;
}

ptrdiff_t fig_source_tell(fig_source *self) {
    if(self->callbacks.tell) {
        return self->callbacks.tell(self->userdata);
    }
    return -1;
}

fig_bool_t fig_source_read_u8(fig_source *self, fig_uint8_t *dest) {
    return fig_source_read(self, dest, 1, 1) == 1;
}

fig_bool_t fig_source_read_le_u16(fig_source *self, fig_uint16_t *dest) {
    fig_uint8_t result[2];
    if(fig_source_read(self, result, 2, 1) == 1) {
        *dest = result[1] << 8 | result[0];
        return 1;
    }
    return 0;
}

fig_bool_t fig_source_read_le_u32(fig_source *self, fig_uint32_t *dest) {
    fig_uint8_t result[4];
    if(fig_source_read(self, result, 4, 1) == 1) {
        *dest = result[3] << 24 | result[2] << 16 | result[1] << 8 | result[0];
        return 1;
    }
    return 0;
}

void fig_source_free(fig_source *self) {
    if(self != NULL) {
        if(self->callbacks.cleanup) {
            self->callbacks.cleanup(self->userdata);
        }
        fig_state_get_allocator(self->state)(fig_state_get_userdata(self->state), self, sizeof(fig_source), 0);
    }
}
