#include <stdlib.h>
#include <string.h>
#include <fig.h>
#include <fig_gif.h>

enum {
    /* Header definitions */
    GIF_HEADER_LENGTH = 6,

    /* Logical screen descriptor packed fields */
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

    /* LZW definitions */
    GIF_LZW_MAX_BITS = 12,
    GIF_LZW_MAX_CODES = (1 << GIF_LZW_MAX_BITS),
    GIF_LZW_MAX_STACK_SIZE = (1 << GIF_LZW_MAX_BITS) + 1,
    GIF_LZW_NULL_CODE = 0xCACA
};

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
    fig_bool_t interlace;
} gif_image_desc;

static fig_bool_t gif_read_header(fig_source *src, gif_format *gif) {
    char buffer[GIF_HEADER_LENGTH];

    if(fig_source_read(src, buffer, GIF_HEADER_LENGTH, 1) != 1) {
        return 0;
    }
    if(memcmp(buffer, "GIF87a", GIF_HEADER_LENGTH) != 0
    && memcmp(buffer, "GIF89a", GIF_HEADER_LENGTH) != 0) {
        return 0;
    }
    memcpy(gif->version, buffer + 3, 3);
    return 1;
}

static fig_bool_t gif_read_screen_desc(fig_source *src, gif_format *gif) {
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

static fig_bool_t gif_read_image_desc(fig_source *src, gif_image_desc *image_desc) {
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

static fig_bool_t gif_skip_sub_blocks(fig_source *src) {
    fig_uint8_t length;

    do {
        if(!fig_source_read_u8(src, &length)) {
            return 0;
        }
        fig_source_seek(src, length, FIG_SEEK_CUR);
    } while(length > 0);
    return 1;
}

static fig_bool_t gif_read_palette(fig_source *src, size_t size, fig_palette *palette) {
    size_t i, j;
    fig_uint8_t buffer[256 * 3];

    if(!fig_source_read(src, buffer, size * 3, 1)
    || !fig_palette_resize(palette, size)) {
        return 0;
    }

    for(i = 0, j = 0; i < size; ++i, j += 3) {
        fig_palette_set(palette, i, 0xFF000000 | buffer[j] << 16 | buffer[j + 1] << 8 | buffer[j + 2]);
    }
    return 1;
}

static fig_bool_t gif_read_image_data(fig_source *src, gif_image_desc *image_desc, fig_uint8_t *pixel_data) {
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
    fig_uint8_t bits;
    fig_uint32_t value;
    fig_uint8_t first_char;
    size_t x;
    size_t y;
    fig_uint8_t pass;
    fig_uint8_t y_increment;

    if(!fig_source_read_u8(src, &min_code_size)) {
        return 0;
    }
    if(min_code_size > GIF_LZW_MAX_BITS) {
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
    bits = 0;
    value = 0;
    first_char = 0;
    x = 0;
    y = 0;
    pass = image_desc->interlace ? 3 : 0;
    y_increment = image_desc->interlace ? 8 : 1;

    for(;;) {
        if (bits < code_size) {
            fig_uint8_t byte;

            if(sub_block_length == 0) {
                if(!fig_source_read_u8(src, &sub_block_length)) {
                    return 0;
                }
                if(sub_block_length == 0) {
                    return 1;
                }
            }
            if(!fig_source_read_u8(src, &byte)) {
                return 0;
            }
            value |= byte << bits;
            bits += 8;
            --sub_block_length;
        } else {
            fig_uint16_t code = value & code_mask;

            value >>= code_size;
            bits -= code_size;

            if(code == clear) {
                code_size = min_code_size + 1;
                code_mask = (1 << code_size) - 1;
                avail = eoi + 1;
                old_code = GIF_LZW_NULL_CODE;
            } else if(code == eoi) {
                fig_source_seek(src, sub_block_length, FIG_SEEK_CUR);
                return gif_skip_sub_blocks(src);
            } else if(old_code == GIF_LZW_NULL_CODE) {
                if(code >= GIF_LZW_MAX_CODES
                || char_stack_size >= GIF_LZW_MAX_STACK_SIZE) {
                    return 0;
                }
                char_stack[char_stack_size++] = suffix_chars[code];
                first_char = code & 0xFF;
                old_code = code;
            } else if(code <= avail) {
                fig_uint16_t current_code = code;

                if(current_code == avail) {
                    if(char_stack_size >= GIF_LZW_MAX_STACK_SIZE) {
                        return 0;
                    }
                    char_stack[char_stack_size++] = first_char;
                    current_code = old_code;
                }
                while(current_code >= clear) {
                    if(current_code >= GIF_LZW_MAX_CODES
                    || char_stack_size >= GIF_LZW_MAX_STACK_SIZE) {
                        return 0;
                    }
                    char_stack[char_stack_size++] = suffix_chars[current_code];
                    current_code = prefix_codes[current_code];
                }

                first_char = suffix_chars[current_code];

                if(char_stack_size >= GIF_LZW_MAX_STACK_SIZE) {
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
                return 0;
            }

            while(char_stack_size > 0) {
                fig_uint8_t top;

                if(y >= image_desc->height) {
                    return 0;
                }

                top = char_stack[--char_stack_size];
                pixel_data[y * image_desc->width + x] = top;
                x++;

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

fig_image *fig_load_gif(fig_source *src) {
    gif_format gif;
    fig_image *img;
    fig_bool_t done;
    
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
                        fig_source_seek(src, length, FIG_SEEK_CUR);
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
                if(frame == NULL
                || !fig_frame_resize_canvas(frame, image_desc.width, image_desc.height)) {
                    goto failure;
                }
                if(image_desc.local_colors > 0
                && !gif_read_palette(src, image_desc.local_colors, fig_frame_get_palette(frame))) {
                    goto failure;
                }
                if(!gif_read_image_data(src, &image_desc, fig_frame_get_pixel_data(frame))) {
                    goto failure;
                }
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
