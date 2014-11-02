#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fig.h>

typedef struct fig_source {
    void *userdata;
    fig_source_callbacks callbacks;
} fig_source;

fig_source *fig_create_source(fig_source_callbacks callbacks, void *userdata) {
    fig_source *self = (fig_source *) malloc(sizeof(fig_source));
    self->callbacks = callbacks;
    self->userdata = userdata;
    return self;
}




static size_t file_read(void *ud, void *dest, size_t size, size_t count) {
    return fread(dest, size, count, (FILE *) ud);
}

static fig_bool_t file_seek(void *ud, fig_offset_t offset, fig_seek_origin_t whence) {
    return !fseek((FILE *) ud, (long) offset, (int) whence);
}

static fig_offset_t file_tell(void *ud) {
    return (fig_offset_t) ftell((FILE *) ud);
}

static const fig_source_callbacks file_source_cb = {
    file_read,
    file_seek,
    file_tell,
    NULL
};

fig_source *fig_create_file_source(FILE *f) {
    return f != NULL ? fig_create_source(file_source_cb, f) : NULL;
}




typedef struct {
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

static fig_bool_t memfile_seek(void *ud, fig_offset_t offset, fig_seek_origin_t whence) {
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

static fig_offset_t memfile_tell(void *ud) {
    return (fig_offset_t) ((memfile *) ud)->position;
}

static void memfile_cleanup(void *ud) {
    free(ud);
}

static const fig_source_callbacks memfile_source_cb = {
    memfile_read,
    memfile_seek,
    memfile_tell,
    memfile_cleanup
};

fig_source *fig_create_memory_source(void *data, size_t length) {
    fig_source *self;
    memfile *mf;

    if(data == NULL) {
        return NULL;
    }

    mf = (memfile *) malloc(length);
    if(mf == NULL) {
        return NULL;
    }
    mf->data = data;
    mf->length = length;
    mf->position = 0;

    self = fig_create_source(file_source_cb, mf);
    if(self == NULL) {
        free(mf);
        return NULL;
    }
    return self;
}




size_t fig_source_read(fig_source *self, void *dest, size_t size, size_t count) {
    if(self->callbacks.read) {
        return self->callbacks.read(self->userdata, dest, size, count);
    }
    return 0;
}

fig_bool_t fig_source_seek(fig_source *self, fig_offset_t offset, fig_seek_origin_t whence) {
    if(self->callbacks.seek) {
        return self->callbacks.seek(self->userdata, offset, whence);
    }
    return 0;
}

fig_offset_t fig_source_tell(fig_source *self) {
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
    }
    free(self);
}
