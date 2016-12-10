#ifndef FIG_H
#define FIG_H
/* A single-header distribution of fig. */
/* To use, there must be ONE source file that contains the library implementation: */
/* #define FIG_IMPLEMENTATION */
/* #include <fig.h> */

/* To disable asserts: */
/* #define FIG_ASSERT(x) ((void) x) */

/* To provide custom typedefs for integer/bool types: */
/* #define FIG_TYPEDEFS
typedef TYPE_GOES_HERE fig_uint8_t;
typedef TYPE_GOES_HERE fig_uint16_t;
typedef TYPE_GOES_HERE fig_uint32_t; 
typedef TYPE_GOES_HERE fig_bool_t; */

/* To manually specify which formats to support: */
/* #define FIG_EXPLICIT_SUPPORT */

#ifndef FIG_EXPLICIT_SUPPORT
#define FIG_LOAD_GIF
#define FIG_SAVE_GIF
#endif 


#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>

#ifndef FIG_ASSERT
    #include <assert.h>
    #define FIG_ASSERT(x) assert(x)
#endif

#ifndef FIG_TYPEDEFS
    #define FIG_TYPEDEFS

    #include <stddef.h>

    #if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || (defined(_MSC_VER) && _MSC_VER >= 1600)
        #include <stdint.h>

        typedef uint8_t fig_uint8_t;
        typedef uint16_t fig_uint16_t;
        typedef uint32_t fig_uint32_t;
    #elif defined(_MSC_VER)
        typedef unsigned __int8 fig_uint8_t;
        typedef unsigned __int16 fig_uint16_t;
        typedef unsigned __int32 fig_uint32_t;
    #else
        typedef unsigned char fig_uint8_t;
        typedef unsigned short fig_uint16_t;
        typedef unsigned int fig_uint32_t;
    #endif

    #if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || (defined(_MSC_VER) && _MSC_VER >= 1700)
        #include <stdbool.h>
        typedef bool fig_bool_t;
    #else
        typedef char fig_bool_t;
    #endif
#endif

typedef int fig_assert_fig_uint8_t_is_1_byte_and_unsigned[sizeof(fig_uint8_t) == 1 && (fig_uint8_t) -1 >= 0 ? 1 : -1];
typedef int fig_assert_fig_uint16_t_is_2_byte_and_unsigned[sizeof(fig_uint16_t) == 2 && (fig_uint16_t) -1 >= 0 ? 1 : -1];
typedef int fig_assert_fig_uint32_t_is_4_byte_and_unsigned[sizeof(fig_uint32_t) == 4 && (fig_uint32_t) -1 >= 0 ? 1 : -1];

typedef struct fig_state fig_state;
typedef struct fig_palette fig_palette;
typedef struct fig_image fig_image;
typedef struct fig_animation fig_animation;
typedef struct fig_input fig_input;
typedef struct fig_input_callbacks fig_input_callbacks;
typedef struct fig_output fig_output;
typedef struct fig_output_callbacks fig_output_callbacks;

/* A function that allocates and manages blocks of memory.
 *
 * ud: a piece of userdata that can be used by the allocator.
 * ptr: pointer to the block of memory to be reallocated.
 * old_size: the size of the block before reallocation.
 * new_size: the desired size after reallocation.
 *
 * When new_size != 0, the function returns a valid pointer on success,
 * and NULL on failure. On failure, ptr is still valid, if it was a valid
 * pointer before the call.
 *
 * When ptr == NULL && old_size == 0 && new_size != 0, then the caller is
 * indicating that the memory block doesn't exist yet. The allocator should
 * allocate a block of the request sized new_size and return a pointer to
 * its start address.
 *
 * When ptr != NULL && old_size != 0 && new_size != 0, then the caller is
 * requesting that the given memory block located at ptr should be resized from
 * old_size to new_size.
 *
 * When (ptr != NULL && old_size == 0) || (ptr == NULL && old_size != 0),
 * the function should return NULL, because the block specified is invalid.
 * 
 * When new_size == 0, the memory at the specified ptr should be freed,
 * and the allocator should always return NULL. If the ptr is already NULL,
 * freeing it should have no effect (like free(NULL)) and just return NULL.
 */
typedef void *(*fig_allocator_t)(void *ud, void *ptr, size_t old_size, size_t new_size);

/* An enumeration of possible frame disposal modes, performed
   after this frame is finished, but before the next one is drawn. */
typedef enum fig_disposal_t {
    /* Replace this frame. */
    FIG_DISPOSAL_UNSPECIFIED,
    /* Draw next frame over this. */
    FIG_DISPOSAL_NONE,
    /* Clear the area used by the frame to background. */
    FIG_DISPOSAL_BACKGROUND,
    /* Clear to previous frame. */
    FIG_DISPOSAL_PREVIOUS,
    /* Number of disposal types. */
    FIG_DISPOSAL_COUNT
} fig_disposal_t;

/* An enumeration of possible seek origins for input/output streams. */
typedef enum fig_seek_origin_t {
    /* Seek forward from beginning of file. */
    FIG_SEEK_SET = SEEK_SET,
    /* Seek relative to current offset in file. */
    FIG_SEEK_CUR = SEEK_CUR,
    /* Seek backward from end of file. */
    FIG_SEEK_END = SEEK_END
} fig_seek_origin_t;



/* A collection of shared state used by the library.
 * Each state has an allocator, an error message on failure, and potentially
 * other internal state.
 * States should not be freed until every object created with them is freed. */
struct fig_state;

/* Create and return a state structure. Returns NULL on failure */
fig_state *fig_create_state(void);
/* Create and return a state structure, using a custom allocator.
 * Returns NULL on failure.
 * The userdata is passed to the allocator for each allocation by the allocator.
 * The allocator + userdata are used to create the state and all future
 * allocations that use the state. */
fig_state *fig_create_custom_state(fig_allocator_t alloc, void *ud);
/* Returns an error message for the most recent failure on this state.
 * Returns NULL if there is no error information.
 * Operations that succeed are not required to change the error.
 * (It is recommended to check this only when a failure has just occurred,
 * or to set error to NULL when it is significant) */
const char *fig_state_get_error(fig_state *self);
/* Set the error message associated with the state. */
void fig_state_set_error(fig_state *self, const char *message);
/* Set the error message of the state to indicate allocation failure. */
void fig_state_set_error_allocation_failed(fig_state *self);
/* Get the allocator function associated with this state. */
fig_allocator_t fig_state_get_allocator(fig_state *self);
/* Get the userdata associated with this state. */
void *fig_state_get_userdata(fig_state *self);
/* Free a state created with one of the fig_create_state functions. */
void fig_state_free(fig_state *self);



/* A palette of BRGA colors. */
struct fig_palette;

/* Create and return a new palette. Returns NULL on failure. */
fig_palette *fig_create_palette(fig_state *state);
/* Get the size of the palette. */
size_t fig_palette_count_colors(fig_palette *self);
/* Get a raw pointer to contiguous BRGA color data, possibly NULL. */
fig_uint32_t *fig_palette_get_colors(fig_palette *self);
/* Get a BGRA color from the palette at the given index. 0 <= index < size. */
fig_uint32_t fig_palette_get(fig_palette *self, size_t index);
/* Set a BGRA color in the palette at the given index. 0 <= index < size. */
void fig_palette_set(fig_palette *self, size_t index, fig_uint32_t color);
/* Resize the palette to the specified size.
 * Invalidates the color data pointer on success. 
 * If the palette size increases, the additional color entries must be initialized.
 * Returns whether the resize was successful. */
fig_bool_t fig_palette_resize(fig_palette *self, size_t size);
/* Free a palette created with fig_create_palette. */
void fig_palette_free(fig_palette *self);



/* An image containing a palette-indexed surface, and a BGRA render surface. */
struct fig_image;

/* Create and return a new image. Returns NULL on failure. */
fig_image *fig_create_image(fig_state *state);
/* Get the palette associated with the image. */
fig_palette *fig_image_get_palette(fig_image *self);
/* Get the x position of the image index data relative to the animation canvas. */
size_t fig_image_get_origin_x(fig_image *self);
/* Get the y position of the image index data relative to the animation canvas. */
size_t fig_image_get_origin_y(fig_image *self);
/* Get the width of the image indexed data. */
size_t fig_image_get_indexed_width(fig_image *self);
/* Get the height of the image indexed data. */
size_t fig_image_get_indexed_height(fig_image *self);
/* Get a raw pointer to image index data. */
fig_uint8_t *fig_image_get_indexed_data(fig_image *self);
/* Set the x position of the image index data relative to the animation canvas. */
void fig_image_set_origin_x(fig_image *self, size_t value);
/* Set the y position of the image index data relative to the animation canvas. */
void fig_image_set_origin_y(fig_image *self, size_t value);
/* Resize the indexed surface of the image.
 * Invalidates the indexed data pointer on success.
 * The data must be reinitialized after resizing.
 * Returns whether the resize was successful. */
fig_bool_t fig_image_resize_indexed(fig_image *self, size_t width, size_t height);
/* Get the width of the image render data. */
size_t fig_image_get_render_width(fig_image *self);
/* Get the height of the image render data. */
size_t fig_image_get_render_height(fig_image *self);
/* Get a raw pointer to image BGRA color data. */
fig_uint32_t *fig_image_get_render_data(fig_image *self);
/* Resize the render surface of the image.
 * Invalidates the render data pointer on success.
 * The data must be reinitialized after resizing.
 * Returns whether the resize was successful. */
fig_bool_t fig_image_resize_render(fig_image *self, size_t width, size_t height);
/* Get the delay to apply on this image. */
size_t fig_image_get_delay(fig_image *self);
/* Get the disposal to apply between this image and the next. */
fig_disposal_t fig_image_get_disposal(fig_image *self);
/* Get whether the image has transparent parts. */
fig_bool_t fig_image_get_transparent(fig_image *self);
/* Get the color that should be transparent during rendering. */
size_t fig_image_get_transparency_index(fig_image *self);
/* Set the delay to apply on this image. */
void fig_image_set_delay(fig_image *self, size_t value);
/* Set the disposal to apply between this image and the next. */
void fig_image_set_disposal(fig_image *self, fig_disposal_t value);
/* Set whether the image has transparent parts. */
void fig_image_set_transparent(fig_image *self, fig_bool_t value);
/* Set the color that should be transparent during rendering. */
void fig_image_set_transparency_index(fig_image *self, size_t value);
/* Free an image created with fig_create_image. */
void fig_image_free(fig_image *self);



/* An animation containing a multiple images and a palette */
struct fig_animation;

/* Create and return a new animation. Returns NULL on failure. */
fig_animation *fig_create_animation(fig_state *state);
/* Get the palette associated with the animation. */
fig_palette *fig_animation_get_palette(fig_animation *self);
/* Get the width of the animation canvas. */
size_t fig_animation_get_width(fig_animation *self);
/* Get the height of the animation canvas. */
size_t fig_animation_get_height(fig_animation *self);
/* Set the canvas area of the animation. */
void fig_animation_set_dimensions(fig_animation *self, size_t width, size_t height);
/* Get the image count of the animation. */
size_t fig_animation_count_images(fig_animation *self);
/* Get a raw pointer to a contiguous image array, possibly NULL. */
fig_image **fig_animation_get_images(fig_animation *self);
/* Get loop count of the animation. 0 = infinite looping */
size_t fig_animation_get_loop_count(fig_animation *self);
/* Set loop count of the animation. 0 = infinite looping */
void fig_animation_set_loop_count(fig_animation *self, size_t value);
/* Exchange order of two images at the given indices. 0 <= index < size */
void fig_animation_swap_images(fig_animation *self, size_t index_a, size_t index_b);
/* Create and add an image to the end of the animation, and return it.
 * The image is owned by the animation.
 * The image will be freed when the animations this image,
 * or as cleanup if the animation itself is freed.
 * Returns NULL on failure. */
fig_image *fig_animation_add_image(fig_animation *self);
/* Create and insert a new image at the given index, and return it.
 * The image is owned by the animation.
 * The image will be freed when the animations this image,
 * or as cleanup if the animation itself is freed.
 * Returns NULL on failure. 0 <= index <= size */
fig_image *fig_animation_insert_image(fig_animation *self, size_t index);
/* Remove an image from the animation and free it immediately. */
void fig_animation_remove_image(fig_animation *self, size_t index);
/* Render all the images by using their indexed data and palette to
 * generate their complete appearance. If this was succesful,
 * every image will contain a full color render surface of the result.
 * Returns whether the render was succesful. */
fig_bool_t fig_animation_render_images(fig_animation *self);
/* Get the palette to apply for rendering the specified image in the animation. */
fig_palette *fig_animation_get_render_palette(fig_animation *self, fig_image *image);
/* Free an animation created with fig_create_animation. */
void fig_animation_free(fig_animation *self);



/* A input stream used for reading binary data. */
struct fig_input;

/* An interface for defining a binary data input from which files are read. */
struct fig_input_callbacks {
    /* Read up to count elements of given size into dest, and return the
     * number of elements actually read. */
    size_t (*read)(void *ud, void *dest, size_t size, size_t count);

    /* Attempt to seek to a position within the stream, and return whether
     * this was successful. */
    fig_bool_t (*seek)(void *ud, ptrdiff_t offset, fig_seek_origin_t whence);

    /* Tell the current position in the input. */
    ptrdiff_t (*tell)(void *ud);

    /* Free any resources owned by this input. */
    void (*cleanup)(void *ud);
};

/* Make an input that has callbacks to read the provided opaque userdata.
 * Returns NULL on failure. */
fig_input *fig_create_input(fig_state *state, fig_input_callbacks callbacks, void *ud);
/* Create and return an input that uses a file handle to read data.
 * The file is user-owned, and is left open after the input is freed.
 * Returns NULL on failure. */
fig_input *fig_create_file_input(fig_state *state, FILE *f);
/* Create and return an input that uses a memory buffer to read data.
 * The data is user-owned, and is not freed when the input is freed.
 * Returns NULL on failure. */
fig_input *fig_create_memory_input(fig_state *state, void *data, size_t length);
/* Read up to count elements of given size into dest.
   Returns the number of elements actually read. */
size_t fig_input_read(fig_input *self, void *dest, size_t size, size_t count);
/* Read a uint8_t value into the dest, return whether it succeeeded. */
fig_bool_t fig_input_read_u8(fig_input *self, fig_uint8_t *dest);
/* Read a little endian uint16_t into dest, return whether it was successful. */
fig_bool_t fig_input_read_le_u16(fig_input *self, fig_uint16_t *dest);
/* Read a little endian uint32_t into dest, return whether it was successful. */
fig_bool_t fig_input_read_le_u32(fig_input *self, fig_uint32_t *dest);
/* Attempt to seek to a position within the stream, and return whether
this was successful. */
fig_bool_t fig_input_seek(fig_input *self, ptrdiff_t offset, fig_seek_origin_t whence);
/* Tell the current position in the input. */
ptrdiff_t fig_input_tell(fig_input *self);
/* Free an input created with one of the fig_create_input functions. */
void fig_input_free(fig_input *self);



/* An output stream used for writing binary data. */
struct fig_output;

/* An interface for defining a binary data output to which files are written. */
struct fig_output_callbacks {
    /* Write up to count elements of given size from src into the output.
     * Returns the number of elements actually written. */
    size_t (*write)(void *ud, const void *src, size_t size, size_t count);

    /* Free any resources owned by this output. */
    void (*cleanup)(void *ud);
};

/* Make an output that has callbacks to write to the provided opaque userdata.
 * Returns NULL on failure. */
fig_output *fig_create_output(fig_state *state, fig_output_callbacks callbacks, void *ud);
/* Create and return an output that uses a file handle to write data.
 * The file is user-owned, and is left open after the output is freed.
 * Returns NULL on failure. */
fig_output *fig_create_file_output(fig_state *state, FILE *f);
/* Create and return an output that uses an internal memory buffer to write data.
 * The buffer is owned by the output, and is freed when the output is freed.
 * Returns NULL on failure. */
fig_output *fig_create_buffer_output(fig_state *state);
/* Get the output callbacks associated with this output stream. */
fig_output_callbacks fig_output_get_callbacks(fig_output *self);
/* Get the current length of the buffer output. */
size_t fig_buffer_output_get_size(fig_output *self);
/* Get a pointer to data in the buffer output. */
const char *fig_buffer_output_get_data(fig_output *self);
/* Write up to count elements of given size from src into the output.
 * Returns the number of elements actually written. */
size_t fig_output_write(fig_output *self, const void *src, size_t size, size_t count);
/* Write a uint8_t, and return whether it was successful. */
fig_bool_t fig_output_write_u8(fig_output *self, fig_uint8_t value);
/* Write a little endian uint16_t, and return whether it was successful. */
fig_bool_t fig_output_write_le_u16(fig_output *self, fig_uint16_t value);
/* Write a little endian uint32_t, and return whether it was successful. */
fig_bool_t fig_output_write_le_u32(fig_output *self, fig_uint32_t value);
/* Free an output created with one of the fig_create_output functions. */
void fig_output_free(fig_output *self);



/* GIF format support */
#ifdef FIG_LOAD_GIF
fig_animation *fig_load_gif(fig_state *state, fig_input *input);
#endif
#ifdef FIG_SAVE_GIF
fig_bool_t fig_save_gif(fig_state *state, fig_output *output, fig_animation *animation);
#endif



/* Utility functions */
/* Returns a 32-bit BRGA color formed by combining the r, g, b, a components */
fig_uint32_t fig_pack_color(fig_uint8_t r, fig_uint8_t g, fig_uint8_t b, fig_uint8_t a);
/* Extracts the r, g, b, a components of the given 32-bit BGRA color. */
void fig_unpack_color(fig_uint32_t color, fig_uint8_t *r, fig_uint8_t *g, fig_uint8_t *b, fig_uint8_t *a);



#ifdef __cplusplus
}
#endif

#ifdef FIG_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void clear_image(fig_animation *self, fig_image *image) {
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

static void dispose_indexed(fig_animation *self, fig_image *prev, fig_image *cur, fig_image *next) {
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

static void blit_indexed(fig_animation *self, fig_image *image) {
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
            clear_image(self, next);
        } else {
            memcpy(fig_image_get_render_data(next), fig_image_get_render_data(cur), sizeof(fig_uint32_t) * self->width * self->height);
            dispose_indexed(self, prev, cur, next);
        }

        blit_indexed(self, next);

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

#if defined(FIG_LOAD_GIF) || defined(FIG_SAVE_GIF)

enum {
    /* Header definitions */
    GIF_HEADER_LENGTH = 6,

    /* Block types */
    GIF_BLOCK_EXTENSION = 0x21,
    GIF_BLOCK_IMAGE = 0x2C,
    GIF_BLOCK_TERMINATOR = 0x3B,

    /* Extension types */
    GIF_EXT_GRAPHICS_CONTROL = 0xF9,
    GIF_EXT_PLAIN_TEXT = 0x01,
    GIF_EXT_COMMENT = 0xFE,
    GIF_EXT_APPLICATION = 0xFF,

    /* Logical screen descriptor packed fields */
    GIF_SCREEN_DESC_GLOBAL_COLOR = 0x80,
    GIF_IMAGE_DESC_COLOR_RESOLUTION_MASK = 0x70,
    GIF_SCREEN_DESC_PALETTE_DEPTH_MASK = 0x07,

    /* Image descriptor packed fields */
    GIF_IMAGE_DESC_LOCAL_COLOR = 0x80,
    GIF_IMAGE_DESC_INTERLACE = 0x40,
    GIF_IMAGE_DESC_PALETTE_DEPTH_MASK = 0x07,

    /* Graphics control packed fields */
    GIF_GRAPHICS_CTRL_TRANSPARENCY = 0x01,
    GIF_GRAPHICS_CTRL_DISPOSAL_MASK = 0x1C,
    GIF_GRAPHICS_CTRL_DISPOSAL_SHIFT = 2,

    /* Application extension definitions */
    GIF_APPLICATION_SIGNATURE_LENGTH = 11,
    GIF_APPLICATION_SIGNATURE_ID_LENGTH = 8,
    GIF_APPLICATION_SIGNATURE_AUTH_CODE_LENGTH = 3,

    /* LZW definitions */
    GIF_LZW_MAX_BITS = 12,
    GIF_LZW_MAX_CODES = (1 << GIF_LZW_MAX_BITS),
    GIF_LZW_MAX_STACK_SIZE = (1 << GIF_LZW_MAX_BITS) + 1,
    GIF_LZW_NULL_CODE = 0xCACA
};
const char * const GIF_HEADER_VERSION_87a = "GIF87a";
const char * const GIF_HEADER_VERSION_89a = "GIF89a";
const char * const GIF_APPLICATION_SIGNATURE_NETSCAPE = "NETSCAPE2.0";
#endif

#ifdef FIG_LOAD_GIF
typedef enum {
    GIF_DISPOSAL_UNSPECIFIED,
    GIF_DISPOSAL_NONE,
    GIF_DISPOSAL_BACKGROUND,
    GIF_DISPOSAL_PREVIOUS,
    GIF_DISPOSAL_COUNT
} gif_disposal_t;

typedef struct {
    fig_uint16_t width;
    fig_uint16_t height;
    size_t global_colors;
    fig_uint8_t background_index;
    fig_uint8_t aspect;
} gif_screen_descriptor;

typedef struct {
    fig_uint16_t x;
    fig_uint16_t y;
    fig_uint16_t width;
    fig_uint16_t height;
    size_t local_colors;
    fig_bool_t interlace;
} gif_image_descriptor;

typedef struct {
    fig_uint16_t delay;
    fig_bool_t transparent;
    fig_uint8_t transparency_index;
    gif_disposal_t disposal;
} gif_graphics_control;

static fig_bool_t read_header(fig_input *input, fig_uint8_t *version) {
    char buffer[GIF_HEADER_LENGTH];

    if(fig_input_read(input, buffer, GIF_HEADER_LENGTH, 1) != 1) {
        return 0;
    }
    if(memcmp(buffer, GIF_HEADER_VERSION_87a, GIF_HEADER_LENGTH) != 0
    && memcmp(buffer, GIF_HEADER_VERSION_89a, GIF_HEADER_LENGTH) != 0) {
        return 0;
    }
    *version = (buffer[3] - '0') * 10 + (buffer[4] - '0');
    return 1;
}

static fig_bool_t read_screen_descriptor(fig_input *input, gif_screen_descriptor *screen_desc) {
    fig_uint8_t packed_fields;

    if(fig_input_read_le_u16(input, &screen_desc->width)
    && fig_input_read_le_u16(input, &screen_desc->height)
    && fig_input_read_u8(input, &packed_fields)
    && fig_input_read_u8(input, &screen_desc->background_index)
    && fig_input_read_u8(input, &screen_desc->aspect)) {
        if((packed_fields & GIF_SCREEN_DESC_GLOBAL_COLOR) != 0) {
            screen_desc->global_colors = (1 << ((packed_fields & GIF_SCREEN_DESC_PALETTE_DEPTH_MASK) + 1));
        } else {
            screen_desc->global_colors = 0;
        }
        return 1;
    } else {
        return 0;
    }
}

static fig_bool_t read_image_descriptor(fig_input *input, gif_image_descriptor *image_desc) {
    fig_uint8_t packed_fields;

    if(fig_input_read_le_u16(input, &image_desc->x)
    && fig_input_read_le_u16(input, &image_desc->y)
    && fig_input_read_le_u16(input, &image_desc->width)
    && fig_input_read_le_u16(input, &image_desc->height)
    && fig_input_read_u8(input, &packed_fields)) {
        if((packed_fields & GIF_IMAGE_DESC_LOCAL_COLOR) != 0) {
            image_desc->local_colors = 1 << ((packed_fields & GIF_IMAGE_DESC_PALETTE_DEPTH_MASK) + 1);
        } else {
            image_desc->local_colors = 0;
        }
        image_desc->interlace = (packed_fields & GIF_IMAGE_DESC_INTERLACE) != 0;
        return 1;
    } else {
        return 0;
    }
}

static fig_bool_t read_sub_block(fig_state *state, fig_input *input, fig_uint8_t *block_size, fig_uint8_t *block, fig_uint8_t max_size, const char *failure_message) {
    if(!fig_input_read_u8(input, block_size)
    || *block_size > max_size
    || fig_input_read(input, block, *block_size, 1) != 1) {
        fig_state_set_error(state, failure_message);
        return 0;
    }
    return 1;
}

static fig_bool_t skip_sub_blocks(fig_input *input) {
    fig_uint8_t length;

    do {
        if(!fig_input_read_u8(input, &length)) {
            return 0;
        }
        fig_input_seek(input, length, FIG_SEEK_CUR);
    } while(length > 0);
    return 1;
}

static fig_bool_t read_graphics_control(fig_input *input, gif_graphics_control *gfx_ctrl) {
    fig_uint8_t length;
    fig_uint8_t packed_fields;

    if(fig_input_read_u8(input, &length)
    && length == 4
    && fig_input_read_u8(input, &packed_fields)
    && fig_input_read_le_u16(input, &gfx_ctrl->delay)
    && fig_input_read_u8(input, &gfx_ctrl->transparency_index)) {
        gfx_ctrl->transparent = packed_fields & GIF_GRAPHICS_CTRL_TRANSPARENCY;
        gfx_ctrl->disposal = (gif_disposal_t) ((packed_fields & GIF_GRAPHICS_CTRL_DISPOSAL_MASK) >> GIF_GRAPHICS_CTRL_DISPOSAL_SHIFT);
        if(gfx_ctrl->disposal >= GIF_DISPOSAL_COUNT) {
            gfx_ctrl->disposal = GIF_DISPOSAL_UNSPECIFIED;
        }

        if(skip_sub_blocks(input)) {
            return 1;
        }
    }
    return 0;
}

static fig_bool_t read_looping_control(fig_input *input, fig_uint16_t *loop_count) {
    fig_uint8_t length;
    fig_uint8_t ignore;
    if(fig_input_read_u8(input, &length)
    && length == 3
    && fig_input_read_u8(input, &ignore)
    && fig_input_read_le_u16(input, loop_count)) {
        return 1;
    }
    return 0;
}


static fig_bool_t read_palette(fig_input *input, size_t size, fig_palette *palette) {
    size_t i, j;
    fig_uint8_t buffer[256 * 3];

    if(!fig_input_read(input, buffer, size * 3, 1)
    || !fig_palette_resize(palette, size)) {
        return 0;
    }

    for(i = 0, j = 0; i < size; ++i, j += 3) {
        fig_palette_set(palette, i, 0xFF000000 | buffer[j] << 16 | buffer[j + 1] << 8 | buffer[j + 2]);
    }
    return 1;
}

static void error_lzw_stack_overflow(fig_state *state) {
    fig_state_set_error(state, "overflowed available LZW character stack");
}

static void error_lzw_invalid_code(fig_state *state) {
    fig_state_set_error(state, "invalid LZW code encountered");
}

static fig_bool_t read_image_data(fig_state *state, fig_input *input, gif_image_descriptor *image_desc, fig_uint8_t *index_data) {
    fig_uint8_t min_code_size;
    fig_uint16_t clear;
    fig_uint16_t eoi;
    fig_uint8_t code_size;
    fig_uint16_t code_mask;
    fig_uint16_t avail;
    fig_uint16_t old_code;
    fig_uint16_t prefix_codes[GIF_LZW_MAX_CODES];
    fig_uint8_t suffix_chars[GIF_LZW_MAX_CODES];
    fig_uint8_t char_stack[GIF_LZW_MAX_STACK_SIZE];
    fig_uint16_t char_stack_size;
    fig_uint8_t sub_block_length;
    fig_uint32_t accumulator;
    fig_uint8_t accumulator_length;
    fig_uint8_t first_char;
    size_t x;
    size_t y;
    fig_uint8_t pass;
    fig_uint8_t y_increment;

    if(!fig_input_read_u8(input, &min_code_size)) {
        fig_state_set_error(state, "failed to read minimum LZW code size");
        return 0;
    }
    if(min_code_size > GIF_LZW_MAX_BITS) {
        fig_state_set_error(state, "minimum LZW code size is too large");
        return 0;
    }
    
    clear = 1 << min_code_size;
    eoi = clear + 1;
    code_size = min_code_size + 1;
    code_mask = (1 << code_size) - 1;
    avail = eoi + 1;
    old_code = GIF_LZW_NULL_CODE;
    
    {
        fig_uint16_t i;

        for(i = 0; i < clear; ++i) {
            prefix_codes[i] = GIF_LZW_NULL_CODE;
            suffix_chars[i] = i & 0xFF;
        }
    }

    memset(char_stack, 0, sizeof(char_stack));
    char_stack_size = 0;
    sub_block_length = 0;
    accumulator = 0;
    accumulator_length = 0;
    first_char = 0;
    x = 0;
    y = 0;
    pass = image_desc->interlace ? 3 : 0;
    y_increment = image_desc->interlace ? 8 : 1;

    for(;;) {
        if (accumulator_length < code_size) {
            fig_uint8_t value;

            if(sub_block_length == 0) {
                if(!fig_input_read_u8(input, &sub_block_length)) {
                    fig_state_set_error(state, "failed to read LZW sub-block length");
                    return 0;
                }
                if(sub_block_length == 0) {
                    return 1;
                }
            }
            if(!fig_input_read_u8(input, &value)) {
                fig_state_set_error(state, "unexpected end-of-stream during LZW decompression");
                return 0;
            }
            accumulator |= value << accumulator_length;
            accumulator_length += 8;
            --sub_block_length;
        } else {
            fig_uint16_t code = accumulator & code_mask;

            accumulator >>= code_size;
            accumulator_length -= code_size;

            if(code == clear) {
                code_size = min_code_size + 1;
                code_mask = (1 << code_size) - 1;
                avail = eoi + 1;
                old_code = GIF_LZW_NULL_CODE;
            } else if(code == eoi) {
                fig_input_seek(input, sub_block_length, FIG_SEEK_CUR);
                return skip_sub_blocks(input);
            } else if(old_code == GIF_LZW_NULL_CODE) {
                if(code >= avail) {
                    error_lzw_invalid_code(state);
                    return 0;
                }
                if(char_stack_size >= GIF_LZW_MAX_STACK_SIZE) {
                    error_lzw_stack_overflow(state);
                    return 0;
                }

                char_stack[char_stack_size++] = suffix_chars[code];
                first_char = code & 0xFF;
                old_code = code;
            } else if(code <= avail) {
                fig_uint16_t current_code = code;

                if(current_code == avail) {
                    if(char_stack_size >= GIF_LZW_MAX_STACK_SIZE) {
                        error_lzw_stack_overflow(state);
                        return 0;
                    }
                    char_stack[char_stack_size++] = first_char;
                    current_code = old_code;
                }
                while(current_code >= clear) {
                    if(current_code >= GIF_LZW_MAX_CODES) {
                        error_lzw_invalid_code(state);
                        return 0;
                    }
                    if(char_stack_size >= GIF_LZW_MAX_STACK_SIZE) {
                        error_lzw_stack_overflow(state);
                        return 0;
                    }
                    char_stack[char_stack_size++] = suffix_chars[current_code];
                    current_code = prefix_codes[current_code];
                }

                first_char = suffix_chars[current_code];

                if(char_stack_size >= GIF_LZW_MAX_STACK_SIZE) {
                    error_lzw_stack_overflow(state);
                    return 0;
                }
                char_stack[char_stack_size++] = first_char;

                if(avail < GIF_LZW_MAX_CODES) {
                    prefix_codes[avail] = old_code;
                    suffix_chars[avail] = first_char;
                    ++avail;
                    if((avail & code_mask) == 0 && avail < GIF_LZW_MAX_CODES) {
                        ++code_size;
                        code_mask = (1 << code_size) - 1;
                    }
                }

                old_code = code;
            } else {
                error_lzw_invalid_code(state);
                return 0;
            }

            while(char_stack_size > 0) {
                fig_uint8_t top;

                if(y >= image_desc->height) {
                    break;
                }

                top = char_stack[--char_stack_size];
                index_data[y * image_desc->width + x] = top;
                ++x;

                if(x >= image_desc->width) {
                    x = 0;
                    y += y_increment;
                    if(y >= image_desc->height && pass > 0) {
                        y_increment = 1 << pass;
                        y = y_increment >> 1;
                        --pass;
                    }
                }
            }
        }
    }
}

static fig_disposal_t convert_gif_disposal_to_fig_disposal(gif_disposal_t disposal) {
    switch(disposal) {
        case GIF_DISPOSAL_UNSPECIFIED: return FIG_DISPOSAL_UNSPECIFIED;
        case GIF_DISPOSAL_NONE: return FIG_DISPOSAL_NONE;
        case GIF_DISPOSAL_BACKGROUND: return FIG_DISPOSAL_BACKGROUND;
        case GIF_DISPOSAL_PREVIOUS: return FIG_DISPOSAL_PREVIOUS;
        default:
            FIG_ASSERT(0);
            return FIG_DISPOSAL_UNSPECIFIED;
    }
}

fig_animation *fig_load_gif(fig_state *state, fig_input *input) {
    fig_uint8_t version;
    gif_screen_descriptor screen_desc;
    gif_graphics_control gfx_ctrl;    
    fig_animation *animation;
    
    if(input == NULL) {
        fig_state_set_error(state, "input is NULL");
        return NULL;
    }

    memset(&screen_desc, 0, sizeof(screen_desc));
    memset(&gfx_ctrl, 0, sizeof(gfx_ctrl));

    if(!read_header(input, &version)) {
        fig_state_set_error(state, "failed to read header");
        return NULL;
    }
    if(!read_screen_descriptor(input, &screen_desc)) {
        fig_state_set_error(state, "failed to read screen descriptor");
        return NULL;
    }

    animation = fig_create_animation(state);
    if(animation == NULL) {
        return NULL;
    }
    fig_animation_set_dimensions(animation, screen_desc.width, screen_desc.height);

    if(screen_desc.global_colors > 0
    && !read_palette(input, screen_desc.global_colors, fig_animation_get_palette(animation))) {
        fig_state_set_error(state, "failed to read global palette");
        return fig_animation_free(animation), NULL;
    }

    for(;;) {
        fig_uint8_t block_type;

        if(!fig_input_read_u8(input, &block_type)) {
            fig_state_set_error(state, "failed to read block type");
            return fig_animation_free(animation), NULL;
        }
        switch(block_type) {
            case GIF_BLOCK_EXTENSION: {
                fig_uint8_t extension_type;

                if(!fig_input_read_u8(input, &extension_type)) {
                    fig_state_set_error(state, "failed to read extension block type");
                    return fig_animation_free(animation), NULL;
                }
                switch(extension_type) {
                    case GIF_EXT_GRAPHICS_CONTROL: {
                        if(!read_graphics_control(input, &gfx_ctrl)) {
                            fig_state_set_error(state, "failed to read graphics control block");
                            return fig_animation_free(animation), NULL;
                        }
                        break;
                    }
                    /*case GIF_EXT_PLAIN_TEXT:
                    case GIF_EXT_COMMENT:*/
                    case GIF_EXT_APPLICATION: {
                        fig_uint8_t application_block_size;
                        fig_uint8_t application_block[GIF_APPLICATION_SIGNATURE_LENGTH];
                        if(!read_sub_block(state, input, &application_block_size, application_block, sizeof(application_block), "failed to read application signature")) {
                            return fig_animation_free(animation), NULL;
                        }
                        if(memcmp(application_block, GIF_APPLICATION_SIGNATURE_NETSCAPE, GIF_APPLICATION_SIGNATURE_LENGTH) == 0) {
                            fig_uint16_t loop_count;
                            if(!read_looping_control(input, &loop_count)) {
                                fig_state_set_error(state, "failed to read looping control block");
                                return fig_animation_free(animation), NULL;
                            }
                            fig_animation_set_loop_count(animation, loop_count);
                        }
                        if(!skip_sub_blocks(input)) {
                            fig_state_set_error(state, "failed to skip extension block");
                            return fig_animation_free(animation), NULL;
                        }
                        break;
                    }
                    default: {
                        if(!skip_sub_blocks(input)) {
                            fig_state_set_error(state, "failed to skip extension block");
                            return fig_animation_free(animation), NULL;
                        }
                        break;
                    }
                }
                break;
            }
            case GIF_BLOCK_IMAGE: {
                fig_image *image;
                gif_image_descriptor image_desc;
                
                memset(&image_desc, 0, sizeof(image_desc));
                if(!read_image_descriptor(input, &image_desc)) {
                    fig_state_set_error(state, "failed to read frame image descriptor");
                }
                image = fig_animation_add_image(animation);

                if(image == NULL
                || !fig_image_resize_indexed(image, image_desc.width, image_desc.height)
                || !fig_image_resize_render(image, screen_desc.width, screen_desc.height)) {
                    fig_state_set_error(state, "failed to allocate frame image surfaces");
                    return fig_animation_free(animation), NULL;
                }
                if(image_desc.local_colors > 0
                && !read_palette(input, image_desc.local_colors, fig_image_get_palette(image))) {
                    fig_state_set_error(state, "failed to read frame local palette");
                    return fig_animation_free(animation), NULL;
                }

                fig_image_set_origin_x(image, image_desc.x);
                fig_image_set_origin_y(image, image_desc.y);
                fig_image_set_transparent(image, gfx_ctrl.transparent);
                fig_image_set_transparency_index(image, gfx_ctrl.transparency_index);
                fig_image_set_delay(image, gfx_ctrl.delay);
                fig_image_set_disposal(image, convert_gif_disposal_to_fig_disposal(gfx_ctrl.disposal));

                if(!read_image_data(state, input, &image_desc, fig_image_get_indexed_data(image))) {
                    return fig_animation_free(animation), NULL;
                }

                break;
            }
            case GIF_BLOCK_TERMINATOR: {
                fig_animation_render_images(animation);
                return animation;
            }
            default:
                fig_state_set_error(state, "unrecognized block type");
                return fig_animation_free(animation), NULL;
        }
    }
}
#endif

#ifdef FIG_SAVE_GIF
static void error_write_failed(fig_state *state) {
    fig_state_set_error(state, "failed to write data");
}

static size_t log2(size_t value) {
    size_t result = 0;
    while(value >>= 1) {
        result++;
    }
    return result;
}

static fig_bool_t write_palette(fig_state *state, fig_output *output, fig_palette *palette, size_t color_depth) {
    fig_uint8_t rgb[3];
    size_t i;
    size_t color_count = fig_palette_count_colors(palette);
    size_t padded_palette_size = 1 << color_depth;

    for(i = 0; i < padded_palette_size; ++i) {
        if(i < color_count) {
            fig_uint32_t color = fig_palette_get(palette, i);
            rgb[0] = (color >> 16) & 0xFF;
            rgb[1] = (color >> 8) & 0xFF;
            rgb[2] = (color >> 0) & 0xFF;
        } else {
            rgb[0] = rgb[1] = rgb[2] = 0;
        }

        if(fig_output_write(output, rgb, 3, 1) != 1) {
            error_write_failed(state);
            return 0;
        }
    }

    return 1;
}

static size_t get_color_depth(fig_palette *palette) {
    size_t color_depth = log2(fig_palette_count_colors(palette));
    if(color_depth == 0) {
        color_depth++;
    }
    return color_depth;
}

static fig_bool_t block_flush(fig_state *state, fig_output *output, fig_uint8_t *block) {
    size_t length = block[0];
    if(length == 0
    || fig_output_write(output, block, length + 1, 1) == 1) {
        block[0] = 0;
        return 1;
    } else {
        error_write_failed(state);
        return 0;
    }
}

static fig_bool_t block_write_u8(fig_state *state, fig_output *output, fig_uint8_t *block, fig_uint8_t value) {
    block[++block[0]] = value;
    if(block[0] == 0xFF) {
        return block_flush(state, output, block);
    } else {
        return 1;
    }
}

static fig_bool_t block_write_bits(fig_state *state, fig_output *output, fig_uint8_t *block, fig_uint32_t *accumulator, fig_uint8_t *accumulator_length, fig_uint16_t value, fig_uint8_t value_length) {
    fig_uint8_t current_accumulator_length = *accumulator_length;
    fig_uint32_t current_accumulator = *accumulator | ((fig_uint32_t) value << current_accumulator_length);

    current_accumulator_length += value_length;

    while(current_accumulator_length >= 8) {
        if(!block_write_u8(state, output, block, current_accumulator & 0xFF)) {
            return 0;
        }

        current_accumulator >>= 8;
        current_accumulator_length -= 8;
    }

    *accumulator = current_accumulator;
    *accumulator_length = current_accumulator_length;
    return 1;
}

static fig_bool_t write_image_data(fig_state *state, fig_output *output, fig_image *image, fig_palette *palette) {
    fig_uint8_t min_code_size;
    fig_uint16_t clear;
    fig_uint16_t eoi;
    fig_uint8_t code_size;
    fig_uint16_t code_mask;
    fig_uint16_t avail;
    fig_uint16_t old_code;
    fig_uint16_t prefix_codes[GIF_LZW_MAX_CODES];
    fig_uint8_t suffix_chars[GIF_LZW_MAX_CODES];
    fig_uint8_t block[256];
    size_t pixel_index;
    size_t pixel_count;
    size_t color_depth;
    size_t padded_palette_size;
    fig_uint8_t* pixels;
    fig_uint32_t accumulator;
    fig_uint8_t accumulator_length;

    color_depth = get_color_depth(palette);
    padded_palette_size = 1 << color_depth;
    min_code_size = color_depth & 0xFF;

    if(min_code_size < 2) {
        min_code_size = 2;
    }

    clear = 1 << min_code_size;
    eoi = clear + 1;
    code_size = min_code_size + 1;
    code_mask = (1 << code_size) - 1;
    avail = eoi + 1;
    old_code = GIF_LZW_NULL_CODE;
    accumulator = 0;
    accumulator_length = 0;
    block[0] = 0;
    
    {
        fig_uint16_t i;

        for(i = 0; i < clear; ++i) {
            prefix_codes[i] = GIF_LZW_NULL_CODE;
            suffix_chars[i] = i & 0xFF;
        }
    }

    if(!fig_output_write_u8(output, min_code_size)) {
        error_write_failed(state);
        return 0;
    }
    if(!block_write_bits(state, output, block, &accumulator, &accumulator_length, clear, code_size)) {
        return 0;
    }

    pixel_count = fig_image_get_indexed_width(image) * fig_image_get_indexed_height(image);
    pixels = fig_image_get_indexed_data(image);

    for(pixel_index = 0; pixel_index != pixel_count; ++pixel_index) {
        fig_uint8_t pixel = pixels[pixel_index];
        if(pixel >= padded_palette_size) {
            fig_state_set_error(state, "encountered indexed pixel with index outside of valid palette range");
            return 0;
        }        

        if(old_code == GIF_LZW_NULL_CODE) {
            old_code = pixel;
        } else {
            fig_uint16_t i;
            fig_bool_t found = 0;
            for(i = eoi + 1; i != avail; ++i) {
                if(prefix_codes[i] == old_code && suffix_chars[i] == pixel) {
                    found = 1;
                    break;
                }
            }

            if(found) {
                old_code = i;
            } else {
                if(!block_write_bits(state, output, block, &accumulator, &accumulator_length, old_code, code_size)) {
                    return 0;
                }

                if(avail >= GIF_LZW_MAX_CODES) {
                    if(!block_write_bits(state, output, block, &accumulator, &accumulator_length, clear, code_size)) {
                        return 0;
                    }

                    code_size = min_code_size + 1;
                    code_mask = (1 << code_size) - 1;
                    avail = eoi + 1;
                } else {
                    if((avail & code_mask) == 0) {
                        ++code_size;
                        code_mask = (1 << code_size) - 1;
                    }

                    prefix_codes[avail] = old_code;
                    suffix_chars[avail] = pixel;
                    ++avail;
                }

                old_code = pixel;
            }
        }
    }

    if(!block_write_bits(state, output, block, &accumulator, &accumulator_length, old_code, code_size)
    || !block_write_bits(state, output, block, &accumulator, &accumulator_length, eoi, code_size)) {
        return 0;
    }

    while(accumulator_length > 0) {
        if(!block_write_u8(state, output, block, accumulator & 0xFF)) {
            return 0;
        }        
        if(accumulator_length > 8) {
            accumulator >>= 8;
            accumulator_length -= 8;
        } else {
            accumulator = 0;
            accumulator_length = 0;
        }
    }

    if(!block_flush(state, output, block)
    || !fig_output_write_u8(output, 0)) {
        error_write_failed(state);
        return 0;
    }

    return 1;
}

static gif_disposal_t convert_fig_disposal_to_gif_disposal(fig_disposal_t disposal) {
    switch(disposal) {
        case FIG_DISPOSAL_UNSPECIFIED: return GIF_DISPOSAL_UNSPECIFIED;
        case FIG_DISPOSAL_NONE: return GIF_DISPOSAL_NONE;
        case FIG_DISPOSAL_BACKGROUND: return GIF_DISPOSAL_BACKGROUND;
        case FIG_DISPOSAL_PREVIOUS: return GIF_DISPOSAL_PREVIOUS;
        default:
            FIG_ASSERT(0);
            return FIG_DISPOSAL_UNSPECIFIED;
    }
}

fig_bool_t fig_save_gif(fig_state *state, fig_output *output, fig_animation *animation) {
    fig_palette *global_palette;
    fig_image **images;
    size_t image_count;
    size_t image_index;
    size_t global_color_depth;
    fig_uint8_t screen_desc_packed_fields;

    if(output == NULL) {
        fig_state_set_error(state, "output is NULL");
        return 0;
    }
    if(animation == NULL) {
        fig_state_set_error(state, "anim is NULL");
        return 0;
    }

    if(fig_output_write(output, GIF_HEADER_VERSION_89a, GIF_HEADER_LENGTH, 1) != 1) {
        error_write_failed(state);
        return 0;
    }
    
    global_palette = fig_animation_get_palette(animation);
    if(global_palette != NULL
    && fig_palette_count_colors(global_palette) != 0) {
        global_color_depth = get_color_depth(global_palette);

        if(global_color_depth > GIF_SCREEN_DESC_PALETTE_DEPTH_MASK + 1) {
            fig_state_set_error(state, "too many colors in global animation palette");
            return 0;
        }

        screen_desc_packed_fields = ((global_color_depth - 1) & GIF_SCREEN_DESC_PALETTE_DEPTH_MASK) | GIF_SCREEN_DESC_GLOBAL_COLOR;
    } else {
        global_color_depth = 0;
        screen_desc_packed_fields = 0;
    }

    if(fig_animation_get_width(animation) >= 0xFFFF
    || fig_animation_get_height(animation) >= 0xFFFF) {
        fig_state_set_error(state, "animation canvas dimensions are too large");
        return 0;
    }

    if(!fig_output_write_le_u16(output, (fig_uint16_t) fig_animation_get_width(animation))
    || !fig_output_write_le_u16(output, (fig_uint16_t) fig_animation_get_height(animation))
    || !fig_output_write_u8(output, screen_desc_packed_fields)
    || !fig_output_write_u8(output, 0)
    || !fig_output_write_u8(output, 0)) {
        error_write_failed(state);
        return 0;
    }
    
    if((screen_desc_packed_fields & GIF_SCREEN_DESC_GLOBAL_COLOR) != 0
    && !write_palette(state, output, global_palette, global_color_depth)) {
        return 0;
    }

    image_count = fig_animation_count_images(animation);
    images = fig_animation_get_images(animation);    

    if(image_count > 1) {
        if(!fig_output_write_u8(output, GIF_BLOCK_EXTENSION)
        || !fig_output_write_u8(output, GIF_EXT_APPLICATION)
        || !fig_output_write_u8(output, GIF_APPLICATION_SIGNATURE_LENGTH)
        || fig_output_write(output, GIF_APPLICATION_SIGNATURE_NETSCAPE, GIF_APPLICATION_SIGNATURE_LENGTH, 1) != 1
        || !fig_output_write_u8(output, 3)
        || !fig_output_write_u8(output, 1)
        || !fig_output_write_le_u16(output, fig_animation_get_loop_count(animation) <= 0xFFFF ? (fig_uint16_t) fig_animation_get_loop_count(animation) : 0)
        || !fig_output_write_u8(output, 0)) {
            error_write_failed(state);
            return 0;
        }
    }

    for(image_index = 0; image_index != image_count; ++image_index) {
        fig_image *image = images[image_index];
        size_t delay = fig_image_get_delay(image);
        fig_palette *local_palette;
        size_t local_color_depth;
        fig_uint8_t image_desc_packed_fields;      

        if(image_count > 1) {
            if(fig_image_get_transparent(image)
            && (fig_image_get_transparency_index(image) >= fig_palette_count_colors(fig_animation_get_render_palette(animation, image))
                || fig_image_get_transparency_index(image) >= 0xFF)) {
                fig_state_set_error(state, "transparency index is outside of valid palette range");
                return 0;
            }

            if(!fig_output_write_u8(output, GIF_BLOCK_EXTENSION)
            || !fig_output_write_u8(output, GIF_EXT_GRAPHICS_CONTROL)
            || !fig_output_write_u8(output, 4)
            || !fig_output_write_u8(output,
                (fig_image_get_transparent(image) ? GIF_GRAPHICS_CTRL_TRANSPARENCY : 0)
                | (convert_fig_disposal_to_gif_disposal(fig_image_get_disposal(image)) << GIF_GRAPHICS_CTRL_DISPOSAL_SHIFT))
            || !fig_output_write_le_u16(output, delay <= 0xFFFF ? (fig_uint16_t) delay : 0)
            || !fig_output_write_u8(output, fig_image_get_transparent(image) ? (fig_uint8_t) fig_image_get_transparency_index(image) : 0)
            || !fig_output_write_u8(output, 0)) {
                error_write_failed(state);
                return 0;
            }
        }

        local_palette = fig_image_get_palette(image);
        if(local_palette != NULL
        && fig_palette_count_colors(local_palette) != 0) {
            local_color_depth = get_color_depth(local_palette);

            if(local_color_depth > GIF_IMAGE_DESC_PALETTE_DEPTH_MASK + 1) {
                fig_state_set_error(state, "too many colors in local frame palette");
                return 0;
            }

            image_desc_packed_fields = ((local_color_depth - 1) & GIF_IMAGE_DESC_PALETTE_DEPTH_MASK) | GIF_IMAGE_DESC_LOCAL_COLOR;
        } else {
            local_color_depth = 0;
            image_desc_packed_fields = 0;
        }

        if(fig_image_get_origin_x(image) >= 0xFFFF
        || fig_image_get_origin_y(image) >= 0xFFFF) {
            fig_state_set_error(state, "frame origin is outside of representatable 16-bit coordinate range");
        }

        if(fig_image_get_indexed_width(image) >= 0xFFFF
        || fig_image_get_indexed_height(image) >= 0xFFFF) {
            fig_state_set_error(state, "frame dimensions are too large.");
        }

        if(!fig_output_write_u8(output, GIF_BLOCK_IMAGE)
        || !fig_output_write_le_u16(output, (fig_uint16_t) fig_image_get_origin_x(image))
        || !fig_output_write_le_u16(output, (fig_uint16_t) fig_image_get_origin_y(image))
        || !fig_output_write_le_u16(output, (fig_uint16_t) fig_image_get_indexed_width(image))
        || !fig_output_write_le_u16(output, (fig_uint16_t) fig_image_get_indexed_height(image))
        || !fig_output_write_u8(output, image_desc_packed_fields)) {
            error_write_failed(state);
            return 0;
        }

        if((image_desc_packed_fields & GIF_IMAGE_DESC_LOCAL_COLOR) != 0
        && !write_palette(state, output, local_palette, local_color_depth)) {
            return 0;
        }
        if((image_desc_packed_fields & GIF_IMAGE_DESC_LOCAL_COLOR) == 0
        && (screen_desc_packed_fields & GIF_SCREEN_DESC_GLOBAL_COLOR) == 0) {
            fig_state_set_error(state, "image has no valid palette (local or global)");
            return 0;
        }

        write_image_data(state, output, image, (image_desc_packed_fields & GIF_IMAGE_DESC_LOCAL_COLOR) != 0 ? local_palette : global_palette);
    }

    fig_output_write_u8(output, GIF_BLOCK_TERMINATOR);

    return 1;
}
#endif

struct fig_image {
    fig_state *state;
    size_t indexed_x;
    size_t indexed_y;
    size_t indexed_width;
    size_t indexed_height;
    size_t render_width;
    size_t render_height;
    size_t delay;
    fig_disposal_t disposal;
    fig_palette *palette;
    fig_bool_t transparent;
    size_t transparency_index;
    fig_uint8_t *indexed_data;
    fig_uint32_t *render_data;
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
            self->render_width = 0;
            self->render_height = 0;
            self->delay = 0;
            self->disposal = FIG_DISPOSAL_UNSPECIFIED;
            self->palette = fig_create_palette(state);
            if(self->palette == NULL) {
                return fig_image_free(self), NULL;
            }
            self->transparent = 0;
            self->transparency_index = 0;
            self->indexed_data = NULL;
            self->render_data = NULL;
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

size_t fig_image_get_origin_x(fig_image *self) {
    return self->indexed_x;
}

size_t fig_image_get_origin_y(fig_image *self) {
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

void fig_image_set_origin_x(fig_image *self, size_t value) {
    self->indexed_x = value;
}

void fig_image_set_origin_y(fig_image *self, size_t value) {
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
            fig_state_set_error_allocation_failed(self->state);
            return 0;
        } else {
            self->indexed_width = width;
            self->indexed_height = height;
            self->indexed_data = index_data;
            return 1;
        }
    }
}

size_t fig_image_get_render_width(fig_image *self) {
    return self->render_width;
}

size_t fig_image_get_render_height(fig_image *self) {
    return self->render_height;
}

fig_uint32_t *fig_image_get_render_data(fig_image *self) {
    return self->render_data;
}

fig_bool_t fig_image_resize_render(fig_image *self, size_t width, size_t height) {
    size_t old_size = self->render_width * self->render_height;
    size_t new_size = width * height;
    if(new_size == 0) {
        fig_state_get_allocator(self->state)(fig_state_get_userdata(self->state), self->render_data, sizeof(fig_uint32_t) * old_size, 0);
        self->render_data = NULL;
        self->render_width = 0;
        self->render_height = 0;
        return 1;
    } else {
        fig_uint32_t *render_data;
        render_data = (fig_uint32_t *) fig_state_get_allocator(self->state)(fig_state_get_userdata(self->state),
            self->render_data, sizeof(fig_uint32_t) * old_size, sizeof(fig_uint32_t) * new_size);
        if(render_data == NULL) {
            fig_state_set_error_allocation_failed(self->state);
            return 0;
        } else {
            self->render_width = width;
            self->render_height = height;
            self->render_data = render_data;
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

void fig_image_free(fig_image *self) {
    if(self != NULL) {
        fig_allocator_t alloc = fig_state_get_allocator(self->state);
        void *ud = fig_state_get_userdata(self->state);     

        if(self->palette != NULL) {
            fig_palette_free(self->palette);
        }
   
        alloc(ud, self->indexed_data, self->indexed_width * self->indexed_height, 0);
        alloc(ud, self->render_data, sizeof(fig_uint32_t) * self->render_width * self->render_height, 0);
        alloc(ud, self, sizeof(fig_image), 0);
    }
}

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



static size_t file_read(void *ud, void *dest, size_t size, size_t count) {
    return fread(dest, size, count, (FILE *) ud);
}

static fig_bool_t file_seek(void *ud, ptrdiff_t offset, fig_seek_origin_t whence) {
    return !fseek((FILE *) ud, (long) offset, (int) whence);
}

static ptrdiff_t file_tell(void *ud) {
    return (ptrdiff_t) ftell((FILE *) ud);
}

static const fig_input_callbacks file_input_cb = {
    file_read,
    file_seek,
    file_tell,
    NULL
};

fig_input *fig_create_file_input(fig_state *state, FILE *f) {
    if(f != NULL) {
        return fig_create_input(state, file_input_cb, f);
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
            if (offset < 0 || (size_t) offset >= mf->length) {
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
            if (offset < 0 || (size_t) offset >= mf->length) {
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

static const fig_input_callbacks memfile_input_cb = {
    memfile_read,
    memfile_seek,
    memfile_tell,
    memfile_cleanup
};

fig_input *fig_create_memory_input(fig_state *state, void *data, size_t length) {
    if(state != NULL) {
        fig_input *self;
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

        self = fig_create_input(state, memfile_input_cb, mf);

        if(self == NULL) {
            return memfile_cleanup(mf), NULL;
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



static size_t file_write(void *ud, const void *src, size_t size, size_t count) {
    return fwrite(src, size, count, (FILE *) ud);
}

static const fig_output_callbacks file_output_cb = {
    file_write,
    NULL
};

fig_output *fig_create_file_output(fig_state *state, FILE *f) {
    if(f != NULL) {
        return fig_create_output(state, file_output_cb, f);
    } else {
        fig_state_set_error(state, "file handle is invalid");
        return NULL;
    }        
}



typedef struct buffer {
    fig_state *state;
    size_t size;
    size_t capacity;
    char *data;
} buffer;

static size_t buffer_write(void *ud, const void *src, size_t size, size_t count) {
    size_t new_capacity;
    buffer *buf = (buffer *) ud;

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

static void buffer_cleanup(void *ud) {
    if(ud != NULL) {
        buffer *buf = (buffer *) ud;
        fig_allocator_t alloc = fig_state_get_allocator(buf->state);
        void *alloc_ud = fig_state_get_userdata(buf->state);
        if(buf->data != NULL) {
            alloc(alloc_ud, buf->data, buf->capacity, 0);
        }
        alloc(alloc_ud, buf, sizeof(buffer), 0);
    }
}

static const fig_output_callbacks buffer_output_cb = {
    buffer_write,
    buffer_cleanup
};

fig_output *fig_create_buffer_output(fig_state *state) {
    if(state != NULL) {
        fig_output *self;
        buffer *buf;

        buf = (buffer *) fig_state_get_allocator(state)(fig_state_get_userdata(state), NULL, 0, sizeof(buffer));

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
            return buffer_cleanup(buf), NULL;
        }

        self = fig_create_output(state, buffer_output_cb, buf);
        if(self == NULL) {
            return buffer_cleanup(buf), NULL;
        }
        return self;
    }
    return NULL;
}

fig_bool_t buffer_is_valid(fig_output *self) {
    fig_output_callbacks callbacks = fig_output_get_callbacks(self);
    return callbacks.write == buffer_write
        && callbacks.cleanup == buffer_cleanup;
}

size_t fig_buffer_output_get_size(fig_output *self) {
    FIG_ASSERT(buffer_is_valid(self));
    return ((buffer *) self->userdata)->size;
}

const char* fig_buffer_output_get_data(fig_output *self) {
    FIG_ASSERT(buffer_is_valid(self));
    return ((buffer *) self->userdata)->data;
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

struct fig_state {
    const char *error;
    fig_allocator_t alloc;
    void *ud;
};

static void *default_alloc(void *ud, void *ptr, size_t old_size, size_t new_size) {
    (void) ud;

    if((ptr != NULL && old_size == 0)
    || (ptr == NULL && old_size != 0)) {
        return NULL;
    }

    if(new_size == 0) {
        free(ptr);
        return NULL;
    } else {
        return realloc(ptr, new_size);
    }
}

fig_state *fig_create_state(void) {
    return fig_create_custom_state(default_alloc, NULL);
}

fig_state *fig_create_custom_state(fig_allocator_t alloc, void *ud) {
    fig_state *self = (fig_state *) alloc(ud, NULL, 0, sizeof(fig_state));
    if(self != NULL) {
        self->error = NULL;
        self->alloc = alloc;
        self->ud = ud;
    }
    return self;
}

const char *fig_state_get_error(fig_state *self) {
    return self->error;
}

void fig_state_set_error(fig_state *self, const char* message) {
    self->error = message;
}

void fig_state_set_error_allocation_failed(fig_state *self) {
    fig_state_set_error(self, "allocation failed");
}

fig_allocator_t fig_state_get_allocator(fig_state *self) {
    return self->alloc;
}

void *fig_state_get_userdata(fig_state *self) {
    return self->ud;
}

void fig_state_free(fig_state *self) {
    if(self != NULL) {
        self->alloc(self->ud, self, sizeof(fig_state), 0);
    }
}

fig_uint32_t fig_pack_color(fig_uint8_t r, fig_uint8_t g, fig_uint8_t b, fig_uint8_t a) {
    return ((fig_uint32_t) r << 16)
        | ((fig_uint32_t) g << 8)
        | (fig_uint32_t) b
        | ((fig_uint32_t) a << 24);
}

void fig_unpack_color(fig_uint32_t color, fig_uint8_t *r, fig_uint8_t *g, fig_uint8_t *b, fig_uint8_t *a) {
    *r = (fig_uint8_t)((color >> 16) & 0xFF);
    *g = (fig_uint8_t)((color >> 8) & 0xFF);
    *b = (fig_uint8_t)(color & 0xFF);
    *a = (fig_uint8_t)((color >> 24) & 0xFF);
}
#endif

#endif

