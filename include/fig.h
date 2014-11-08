#ifndef FIG_H
#define FIG_H

#include <fig_config.h>

#include <stdio.h>

#ifndef FIG_ASSERT
    #include <assert.h>
    #define FIG_ASSERT(x) assert(x)
#endif

#ifndef FIG_TYPEDEFS
    #define FIG_TYPEDEFS
    #if defined(_MSC_VER)
        typedef unsigned __int8 fig_uint8_t;
        typedef unsigned __int16 fig_uint16_t;
        typedef unsigned __int32 fig_uint32_t;
        typedef __int64 fig_offset_t;
        typedef char fig_bool_t;
    #elif __STDC_VERSION__ >= 199901L
        #include <stdint.h>
        #include <stdbool.h>
        typedef uint8_t fig_uint8_t;
        typedef uint16_t fig_uint16_t;
        typedef uint32_t fig_uint32_t;
        typedef int64_t fig_offset_t;
        typedef bool fig_bool_t;
    #endif
#endif

typedef int fig_validate_uint8_t[sizeof(fig_uint8_t) == 1 && (fig_uint8_t) -1 >= 0 ? 1 : -1];
typedef int fig_validate_uint16_t[sizeof(fig_uint16_t) == 2 && (fig_uint16_t) -1 >= 0 ? 1 : -1];
typedef int fig_validate_uint32_t[sizeof(fig_uint32_t) == 4 && (fig_uint32_t) -1 >= 0 ? 1 : -1];
typedef int fig_validate_offset_t[sizeof(fig_offset_t) >= 4 && (fig_offset_t) -1 < 0 ? 1 : -1];

typedef struct fig_palette fig_palette;
typedef struct fig_frame fig_frame;
typedef struct fig_image fig_image;
typedef struct fig_source_callbacks fig_source_callbacks;
typedef struct fig_source fig_source;

typedef enum fig_seek_origin_t {
    /* Seek forward from beginning of file. */
    FIG_SEEK_SET = SEEK_SET,
    /* Seek relative to current offset in file. */
    FIG_SEEK_CUR = SEEK_CUR,
    /* Seek backward from end of file. */
    FIG_SEEK_END = SEEK_END,
} fig_seek_origin_t;



/* A palette of BRGA colors. */
typedef struct fig_palette fig_palette;

/* Return a new palette, or NULL on failure. */
fig_palette *fig_create_palette(void);
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



/* An individual frame in an animation. */
typedef struct fig_frame fig_frame;

/* Return a new frame, or NULL on failure. */
fig_frame *fig_create_frame(void);
/* Get the palette associated with the frame. */
fig_palette *fig_frame_get_palette(fig_frame *self);
/* Get a raw pointer to frame index data. */
fig_uint8_t *fig_frame_get_index_data(fig_frame *self);
/* Get a raw pointer to frame BGRA color data. */
fig_uint32_t *fig_frame_get_color_data(fig_frame *self);
/* Get the x position of the frame. */
size_t fig_frame_get_x(fig_frame *self);
/* Get the y position of the frame. */
size_t fig_frame_get_y(fig_frame *self);
/* Get the width of the frame. */
size_t fig_frame_get_width(fig_frame *self);
/* Get the height of the frame. */
size_t fig_frame_get_height(fig_frame *self);
/* Get the delay to apply on this frame. */
size_t fig_frame_get_delay(fig_frame *self);
/* Get the disposal to apply between this frame and the next. */
fig_disposal_t fig_frame_get_disposal(fig_frame *self);
/* Set the x position of the frame. */
void fig_frame_set_x(fig_frame *self, size_t value);
/* Set the y position of the frame. */
void fig_frame_set_y(fig_frame *self, size_t value);
/* Resize the canvas area of the frame */
fig_bool_t fig_frame_resize_canvas(fig_frame *self, size_t width, size_t height);
/* Set the delay to apply on this frame. */
void fig_frame_set_delay(fig_frame *self, size_t value);
/* Set the disposal to apply between this frame and the next. */
void fig_frame_set_disposal(fig_frame *self, fig_disposal_t value);
/* Update the color data by converting the index data into BGRA colors. */
void fig_frame_calculate_colors(fig_frame *self, fig_image *image);
/* Free a frame created with fig_create_frame. */
void fig_frame_free(fig_frame *self);



/* An image containing a background canvas, an animation, and a palette */
typedef struct fig_image fig_image;

/* Return a new image, or NULL on failure. */
fig_image *fig_create_image(void);
/* Get the palette associated with the image. */
fig_palette *fig_image_get_palette(fig_image *self);
/* Get the width of the image. */
size_t fig_image_get_width(fig_image *self);
/* Get the height of the image. */
size_t fig_image_get_height(fig_image *self);
/* Resize the canvas area of the image. */
fig_bool_t fig_image_resize_canvas(fig_image *self, size_t width, size_t height);
/* Get frame count of the image's animation. */
size_t fig_image_count_frames(fig_image *self);
/* Get a raw pointer to contiguous frame data, possibly NULL. */
fig_frame **fig_image_get_frames(fig_image *self);
/* Get loop count of the image's animation. 0 = infinite looping */
size_t fig_image_get_loop_count(fig_image *self);
/* Set loop count of the image's animation. 0 = infinite looping */
void fig_image_set_loop_count(fig_image *self, size_t loop_count);
/* Exchange order of two frames at the given indices. 0 <= index < size */
void fig_image_swap_frames(fig_image *self, size_t index_a, size_t index_b);
/* Append a new frame to the image, and return it. NULL on failure. */
fig_frame *fig_image_add_frame(fig_image *self);
/* Insert a new frame, and return it. NULL on failure. 0 <= index <= size */
fig_frame *fig_image_insert_frame(fig_image *self, size_t index);
/* Remove a frame from the image and free it. */
void fig_image_remove_frame(fig_image *self, size_t index);
/* Free an image created with fig_create_image. */
void fig_image_free(fig_image *self);



/* An interface for defining a binary data source from which files are read. */
typedef struct fig_source_callbacks {
    /* Read up to count elements of given size into dest, and return the
    number of elements actually read. */
    size_t (*read)(void *ud, void *dest, size_t size, size_t count);

    /* Attempt to seek to a position within the stream, and return whether
    this was successful. */
    fig_bool_t (*seek)(void *ud, fig_offset_t offset, fig_seek_origin_t whence);

    /* Tell the current position in the source. */
    fig_offset_t (*tell)(void *ud);

    /* Free any resources owned by this source. */
    void (*cleanup)(void *ud);
} fig_source_callbacks;



/* A source used for reading binary data. */
typedef struct fig_source fig_source;

/* Make a source that has callbacks to read opaque userdata. NULL on failure */
fig_source *fig_create_source(fig_source_callbacks callbacks, void *userdata);
/* Return a source that uses a file handle to read data. NULL on failure */
fig_source *fig_create_file_source(FILE *f);
/* Return a source that uses a memory buffer to read data. NULL on failure */
fig_source *fig_create_memory_source(void *data, size_t length);
/* Read up to count elements of given size into dest, and return the
number of elements actually read. */
size_t fig_source_read(fig_source *self, void *dest, size_t size, size_t count);
/* Read a uint8_t value into the dest, return whether it succeeeded. */
fig_bool_t fig_source_read_u8(fig_source *self, fig_uint8_t *dest);
/* Read a little endian uint16_t into dest, return whether it was succeeded. */
fig_bool_t fig_source_read_le_u16(fig_source *self, fig_uint16_t *dest);
/* Read a little endian uint32_t into dest, return whether it was succeeded. */
fig_bool_t fig_source_read_le_u32(fig_source *self, fig_uint32_t *dest);
/* Attempt to seek to a position within the stream, and return whether
this was successful. */
fig_bool_t fig_source_seek(fig_source *self, fig_offset_t offset, fig_seek_origin_t whence);
/* Tell the current position in the source. */
fig_offset_t fig_source_tell(fig_source *self);
/* Free a source created with one of the fig_create_source functions. */
void fig_source_free(fig_source *self);


#ifdef FIG_LOAD_GIF
fig_image *fig_load_gif(fig_source *src);
#endif

#endif
