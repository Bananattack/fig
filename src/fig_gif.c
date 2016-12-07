#include <string.h>
#include <fig.h>

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
const char* const GIF_HEADER_VERSION_87a = "GIF87a";
const char* const GIF_HEADER_VERSION_89a = "GIF89a";
const char* const GIF_APPLICATION_SIGNATURE_NETSCAPE = "NETSCAPE2.0";
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
    if(animation == NULL
    || !fig_animation_resize_canvas(animation, screen_desc.width, screen_desc.height)) {
        fig_state_set_error(state, "failed to allocate animation canvas");
        return fig_animation_free(animation), NULL;
    }
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
                || !fig_image_resize_canvas(image, screen_desc.width, screen_desc.height)) {
                    fig_state_set_error(state, "failed to allocate frame image canvas");
                    return fig_animation_free(animation), NULL;
                }
                if(image_desc.local_colors > 0
                && !read_palette(input, image_desc.local_colors, fig_image_get_palette(image))) {
                    fig_state_set_error(state, "failed to read frame local palette");
                    return fig_animation_free(animation), NULL;
                }

                fig_image_set_indexed_x(image, image_desc.x);
                fig_image_set_indexed_y(image, image_desc.y);
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
                fig_animation_render_indexed(animation);
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

fig_bool_t write_image_data(fig_state *state, fig_output *output, fig_image *image, fig_palette *palette) {
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

fig_bool_t fig_save_gif(fig_state *state, fig_output *output, fig_animation *anim) {
    fig_palette *global_palette;
    fig_image **images;
    size_t image_count;
    size_t image_index;
    size_t global_color_depth;
    fig_uint8_t screen_desc_packed_fields;

    if(output == NULL) {
        fig_state_set_error(state, "output is NULL");
        return NULL;
    }
    if(anim == NULL) {
        fig_state_set_error(state, "anim is NULL");
        return NULL;
    }

    if(fig_output_write(output, GIF_HEADER_VERSION_89a, GIF_HEADER_LENGTH, 1) != 1) {
        error_write_failed(state);
        return 0;
    }
    
    global_palette = fig_animation_get_palette(anim);
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

    if(!fig_output_write_le_u16(output, fig_animation_get_width(anim))
    || !fig_output_write_le_u16(output, fig_animation_get_height(anim))
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

    image_count = fig_animation_count_images(anim);
    images = fig_animation_get_images(anim);    

    if(image_count > 1) {
        if(!fig_output_write_u8(output, GIF_BLOCK_EXTENSION)
        || !fig_output_write_u8(output, GIF_EXT_APPLICATION)
        || !fig_output_write_u8(output, GIF_APPLICATION_SIGNATURE_LENGTH)
        || fig_output_write(output, GIF_APPLICATION_SIGNATURE_NETSCAPE, GIF_APPLICATION_SIGNATURE_LENGTH, 1) != 1
        || !fig_output_write_u8(output, 3)
        || !fig_output_write_u8(output, 1)
        || !fig_output_write_le_u16(output, fig_animation_get_loop_count(anim))
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
            if(!fig_output_write_u8(output, GIF_BLOCK_EXTENSION)
            || !fig_output_write_u8(output, GIF_EXT_GRAPHICS_CONTROL)
            || !fig_output_write_u8(output, 4)
            || !fig_output_write_u8(output,
                (fig_image_get_transparent(image) ? GIF_GRAPHICS_CTRL_TRANSPARENCY : 0)
                | (convert_fig_disposal_to_gif_disposal(fig_image_get_disposal(image)) << GIF_GRAPHICS_CTRL_DISPOSAL_SHIFT))
            || !fig_output_write_le_u16(output, delay > 0xFFFF ? 0xFFFF : delay)
            || !fig_output_write_u8(output, fig_image_get_transparency_index(image) & 0xFF)
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

        if(!fig_output_write_u8(output, GIF_BLOCK_IMAGE)
        || !fig_output_write_le_u16(output, fig_image_get_indexed_x(image))
        || !fig_output_write_le_u16(output, fig_image_get_indexed_y(image))
        || !fig_output_write_le_u16(output, fig_image_get_indexed_width(image))
        || !fig_output_write_le_u16(output, fig_image_get_indexed_height(image))
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
