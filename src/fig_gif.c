#include <stdlib.h>
#include <string.h>
#include <fig.h>
#include <fig_gif.h>

enum {
    GIF_HEADER_LENGTH = 6,

    /* Screen packed fields */
    GIF_SCREEN_PF_GLOBAL_COLOR = 0x80,
    GIF_SCREEN_PF_DEPTH_MASK = 0x07,

    /* Image descriptor packed fields */
    GIF_IMAGE_DESC_PF_LOCAL_COLOR = 0x80,
    GIF_IMAGE_DESC_PF_INTERLACE = 0x40,
    GIF_IMAGE_DESC_PF_DEPTH_MASK = 0x07,

    /* Block types */
    GIF_BLOCK_EXTENSION = 0x21,
    GIF_BLOCK_IMAGE = 0x2C,
    GIF_BLOCK_TERMINATOR = 0x3B,

    /* Extension types */
    GIF_EXT_GRAPHIC_CONTROL = 0x21,
    GIF_EXT_PLAIN_TEXT = 0x01,
    GIF_EXT_COMMENT = 0xFE,
    GIF_EXT_APPLICATION = 0xFF,

    /* LZW types */
    GIF_LZW_BITS = 12,
    GIF_LZW_DICTIONARY_SIZE = (1 << GIF_LZW_BITS)
};

const char GIF_HEADER_87[GIF_HEADER_LENGTH] = {'G', 'I', 'F', '8', '7', 'a'};
const char GIF_HEADER_89[GIF_HEADER_LENGTH] = {'G', 'I', 'F', '8', '9', 'a'};

typedef struct {
    char version[3];

    fig_uint16_t width;
    fig_uint16_t height;
    size_t global_colors;
    fig_uint8_t bg_index;
    fig_uint8_t aspect;
} gif_format;

typedef struct {
    fig_uint16_t x;
    fig_uint16_t y;
    fig_uint16_t width;
    fig_uint16_t height;
    size_t local_colors;
    int interlace;
} gif_image_desc;

static int gif_read_header(fig_source *src, gif_format *gif) {
    char buffer[GIF_HEADER_LENGTH];

    if(fig_source_read(src, buffer, GIF_HEADER_LENGTH, 1) != 1) {
        return 0;
    }
    if(memcmp(buffer, GIF_HEADER_87, GIF_HEADER_LENGTH) != 0
    && memcmp(buffer, GIF_HEADER_89, GIF_HEADER_LENGTH) != 0) {
        return 0;
    }
    memcpy(gif->version, buffer + 3, 3);
    return 1;
}

static int gif_read_screen_desc(fig_source *src, gif_format *gif) {
    fig_uint8_t packed_fields;

    if(fig_source_read_le_u16(src, &gif->width)
    && fig_source_read_le_u16(src, &gif->height)
    && fig_source_read_u8(src, &packed_fields)
    && fig_source_read_u8(src, &gif->bg_index)
    && fig_source_read_u8(src, &gif->aspect)) {
        if((packed_fields & GIF_SCREEN_PF_GLOBAL_COLOR) != 0) {
            gif->global_colors = (1 << ((packed_fields & GIF_SCREEN_PF_DEPTH_MASK) + 1));
        } else {
            gif->global_colors = 0;
        }
        return 1;
    } else {
        return 0;
    }
}

static int gif_read_image_desc(fig_source *src, gif_image_desc *image_desc) {
    fig_uint8_t packed_fields;

    if(fig_source_read_le_u16(src, &image_desc->x)
    && fig_source_read_le_u16(src, &image_desc->y)
    && fig_source_read_le_u16(src, &image_desc->width)
    && fig_source_read_le_u16(src, &image_desc->height)
    && fig_source_read_u8(src, &packed_fields)) {
        if((packed_fields & GIF_IMAGE_DESC_PF_LOCAL_COLOR) != 0) {
            image_desc->local_colors = 1 << ((packed_fields & GIF_IMAGE_DESC_PF_DEPTH_MASK) + 1);
        } else {
            image_desc->local_colors = 0;
        }
        image_desc->interlace = (packed_fields & GIF_IMAGE_DESC_PF_INTERLACE) != 0;
        return 1;
    } else {
        return 0;
    }
}

static int gif_skip_sub_blocks(fig_source *src) {
    fig_uint8_t length;
    do {
        if(!fig_source_read_u8(src, &length)) {
            return 0;
        }
        fig_source_seek(src, length, SEEK_CUR);
    } while(length > 0);
    return 1;
}

static int gif_read_palette(fig_source *src, size_t size, fig_palette *palette) {
    size_t i, j;
    fig_uint8_t buffer[256 * 3];

    if(!fig_source_read(src, buffer, size * 3, 1)
    || !fig_palette_resize(palette, size)) {
        return 0;
    }

    for(i = 0, j = 0; i < size; i++, j += 3) {
        fig_palette_set(palette, i, 0xFF000000 | buffer[j] << 16 | buffer[j + 1] << 8 | buffer[j + 2]);
    }
    return 1;
}

static int gif_read_lzw(fig_source *src) {
    fig_uint8_t codesize;
    fig_uint8_t length;

    if(!fig_source_read_u8(src, &codesize)) {
        return 0;
    }
    do {
        if(!fig_source_read_u8(src, &length)) {
            return 0;
        }
        fig_source_seek(src, length, SEEK_CUR);
    } while(length > 0);

    return 1;
}

fig_image *fig_load_gif(fig_source *src) {
    gif_format gif;
    fig_image *img;
    int done;
    
    if(src == NULL) {
        return NULL;
    }

    memset(&gif, 0, sizeof(gif));

    if(!gif_read_header(src, &gif)
    || !gif_read_screen_desc(src, &gif)) {
        return NULL;
    }

    img = fig_create_image();
    if(img == NULL
    || !fig_image_resize_canvas(img, gif.width, gif.height)) {
        goto failure;
    }

    if(gif.global_colors > 0
    && !gif_read_palette(src, gif.global_colors, fig_image_get_palette(img))) {
        goto failure;
    }

    done = 0;
    while(!done) {
        fig_uint8_t block_type;
        if(!fig_source_read_u8(src, &block_type)) {
            goto failure;
        }
        switch(block_type) {
            case GIF_BLOCK_EXTENSION: {
                fig_uint8_t extension_type;
                if(!fig_source_read_u8(src, &extension_type)) {
                    goto failure;
                }
                switch(extension_type) {
                    case GIF_EXT_GRAPHIC_CONTROL: {
                        fig_uint8_t length;
                        if(!fig_source_read_u8(src, &length)) {
                            goto failure;
                        }
                        fig_source_seek(src, length, SEEK_CUR);
                        if(!gif_skip_sub_blocks(src)) {
                            goto failure;
                        }
                        break;
                    }
                    /*case GIF_EXT_PLAIN_TEXT:
                    case GIF_EXT_COMMENT:
                    case GIF_EXT_APPLICATION:*/
                    default: {
                        if(!gif_skip_sub_blocks(src)) {
                            goto failure;
                        }
                        break;
                    }
                }
                break;
            }
            case GIF_BLOCK_IMAGE: {
                fig_animation *anim;
                fig_frame *frame;
                gif_image_desc image_desc;
                
                gif_read_image_desc(src, &image_desc);
                anim = fig_image_get_animation(img);
                frame = fig_animation_add(anim);
                if(frame == NULL) {
                    goto failure;
                }

                if(image_desc.local_colors > 0
                && !gif_read_palette(src, image_desc.local_colors, fig_frame_get_palette(frame))) {
                    goto failure;
                }

                fig_uint8_t codesize;
                fig_uint8_t length;

                if(!fig_source_read_u8(src, &codesize)) {
                    goto failure;
                }
                do {
                    if(!fig_source_read_u8(src, &length)) {
                        goto failure;
                    }
                    fig_source_seek(src, length, SEEK_CUR);
                } while(length > 0);
                break;
            }
            case GIF_BLOCK_TERMINATOR: {
                done = 1;
                break;
            }
            default:
                goto failure;
        }
    }

    return img;
failure:
    fig_image_free(img);
    return NULL;
}
