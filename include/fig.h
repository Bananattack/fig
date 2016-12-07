#ifndef FIG_H
#define FIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <fig_config.h>

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
/* Resize the palette to some capacity. Return 1 if successful, 0 otherwise. */
fig_bool_t fig_palette_resize(fig_palette *self, size_t size);
/* Free a palette created with fig_create_palette. */
void fig_palette_free(fig_palette *self);



/* An image containing BGRA color data and possibly paletted index data. */
struct fig_image;

/* Create and return a new image. Returns NULL on failure. */
fig_image *fig_create_image(fig_state *state);
/* Get the palette associated with the image. */
fig_palette *fig_image_get_palette(fig_image *self);
/* Get the x position of the image index data relative to the canvas. */
size_t fig_image_get_origin_x(fig_image *self);
/* Get the y position of the image index data relative to the canvas. */
size_t fig_image_get_origin_y(fig_image *self);
/* Get the width of the image canvas. */
size_t fig_image_get_indexed_width(fig_image *self);
/* Get the height of the image canvas. */
size_t fig_image_get_indexed_height(fig_image *self);
/* Get a raw pointer to image index data. */
fig_uint8_t *fig_image_get_indexed_data(fig_image *self);
/* Set the x position of the image index data relative to the canvas. */
void fig_image_set_origin_x(fig_image *self, size_t value);
/* Set the y position of the image index data relative to the canvas. */
void fig_image_set_origin_y(fig_image *self, size_t value);
/* Resize the canvas area of the image */
fig_bool_t fig_image_resize_indexed(fig_image *self, size_t width, size_t height);
/* Get the width of the image canvas. */
size_t fig_image_get_canvas_width(fig_image *self);
/* Get the height of the image canvas. */
size_t fig_image_get_canvas_height(fig_image *self);
/* Get a raw pointer to image BGRA color data. */
fig_uint32_t *fig_image_get_canvas_data(fig_image *self);
/* Resize the canvas area of the image */
fig_bool_t fig_image_resize_canvas(fig_image *self, size_t width, size_t height);
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
/* Get the palette to apply for rendering. */
fig_palette *fig_image_get_render_palette(fig_image *self, fig_animation *anim);
/* Free an image created with fig_create_image. */
void fig_image_free(fig_image *self);



/* An animation containing a multiple images and a palette */
struct fig_animation;

/* Create and return a new animation. Returns NULL on failure. */
fig_animation *fig_create_animation(fig_state *state);
/* Get the palette associated with the animation. */
fig_palette *fig_animation_get_palette(fig_animation *self);
/* Get the width of the animation. */
size_t fig_animation_get_width(fig_animation *self);
/* Get the height of the animation. */
size_t fig_animation_get_height(fig_animation *self);
/* Resize the canvas area of the animation. */
fig_bool_t fig_animation_resize_canvas(fig_animation *self, size_t width, size_t height);
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
/* Append a new image to the animation, and return it. NULL on failure. */
fig_image *fig_animation_add_image(fig_animation *self);
/* Insert a new image, and return it. NULL on failure. 0 <= index <= size */
fig_image *fig_animation_insert_image(fig_animation *self, size_t index);
/* Remove an image from the animation and free it. */
void fig_animation_remove_image(fig_animation *self, size_t index);
/* Render all the images offline to get their complete appearance. */
void fig_animation_render_indexed(fig_animation *self);
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
       this was successful. */
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
   Returns the number of elements actually written. */
size_t fig_output_write(fig_output *self, const void *src, size_t size, size_t count);
/* Write a uint8_t, and return whether it was successful. */
fig_bool_t fig_output_write_u8(fig_output *self, fig_uint8_t value);
/* Write a little endian uint16_t, and return whether it was successful. */
fig_bool_t fig_output_write_le_u16(fig_output *self, fig_uint16_t value);
/* Write a little endian uint32_t, and return whether it was successful. */
fig_bool_t fig_output_write_le_u32(fig_output *self, fig_uint32_t value);
/* Free an output created with one of the fig_create_output functions. */
void fig_output_free(fig_output *self);




#ifdef FIG_LOAD_GIF
fig_animation *fig_load_gif(fig_state *state, fig_input *input);
#endif

#ifdef FIG_SAVE_GIF
fig_bool_t fig_save_gif(fig_state *state, fig_output *output, fig_animation *anim);
#endif


#ifdef __cplusplus
}
#endif

#endif
