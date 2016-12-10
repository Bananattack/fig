#include <string.h>
#include <fig.h>

#if defined(FIG_LOAD_GIF) || defined(FIG_SAVE_GIF)

enum {
    /* Header definitions */
    FIG_GIF_HEADER_LENGTH = 6,

    /* Block types */
    FIG_GIF_BLOCK_EXTENSION = 0x21,
    FIG_GIF_BLOCK_IMAGE = 0x2C,
    FIG_GIF_BLOCK_TERMINATOR = 0x3B,

    /* Extension types */
    FIG_GIF_EXT_GRAPHICS_CONTROL = 0xF9,
    FIG_GIF_EXT_PLAIN_TEXT = 0x01,
    FIG_GIF_EXT_COMMENT = 0xFE,
    FIG_GIF_EXT_APPLICATION = 0xFF,

    /* Logical screen descriptor packed fields */
    FIG_GIF_SCREEN_DESC_GLOBAL_COLOR = 0x80,
    FIG_GIF_IMAGE_DESC_COLOR_RESOLUTION_MASK = 0x70,
    FIG_GIF_SCREEN_DESC_PALETTE_DEPTH_MASK = 0x07,

    /* Image descriptor packed fields */
    FIG_GIF_IMAGE_DESC_LOCAL_COLOR = 0x80,
    FIG_GIF_IMAGE_DESC_INTERLACE = 0x40,
    FIG_GIF_IMAGE_DESC_PALETTE_DEPTH_MASK = 0x07,

    /* Graphics control packed fields */
    FIG_GIF_GRAPHICS_CTRL_TRANSPARENCY = 0x01,
    FIG_GIF_GRAPHICS_CTRL_DISPOSAL_MASK = 0x1C,
    FIG_GIF_GRAPHICS_CTRL_DISPOSAL_SHIFT = 2,

    /* Application extension definitions */
    FIG_GIF_APPLICATION_SIGNATURE_LENGTH = 11,
    FIG_GIF_APPLICATION_SIGNATURE_ID_LENGTH = 8,
    FIG_GIF_APPLICATION_SIGNATURE_AUTH_CODE_LENGTH = 3,

    /* LZW definitions */
    FIG_GIF_LZW_MAX_BITS = 12,
    FIG_GIF_LZW_MAX_CODES = (1 << FIG_GIF_LZW_MAX_BITS),
    FIG_GIF_LZW_MAX_STACK_SIZE = (1 << FIG_GIF_LZW_MAX_BITS) + 1,
    FIG_GIF_LZW_NULL_CODE = 0xCACA
};
const char * const FIG_GIF_HEADER_VERSION_87a = "GIF87a";
const char * const FIG_GIF_HEADER_VERSION_89a = "GIF89a";
const char * const FIG_GIF_APPLICATION_SIGNATURE_NETSCAPE = "NETSCAPE2.0";
#endif

#ifdef FIG_LOAD_GIF
typedef enum {
    FIG_GIF_DISPOSAL_UNSPECIFIED,
    FIG_GIF_DISPOSAL_NONE,
    FIG_GIF_DISPOSAL_BACKGROUND,
    FIG_GIF_DISPOSAL_PREVIOUS,
    FIG_GIF_DISPOSAL_COUNT
} fig_gif_disposal_t_;

typedef struct {
    fig_uint16_t width;
    fig_uint16_t height;
    size_t global_colors;
    fig_uint8_t background_index;
    fig_uint8_t aspect;
} fig_gif_screen_descriptor_;

typedef struct {
    fig_uint16_t x;
    fig_uint16_t y;
    fig_uint16_t width;
    fig_uint16_t height;
    size_t local_colors;
    fig_bool_t interlace;
} fig_gif_image_descriptor_;

typedef struct {
    fig_uint16_t delay;
    fig_bool_t transparent;
    fig_uint8_t transparency_index;
    fig_gif_disposal_t_ disposal;
} fig_gif_graphics_control_;

static fig_bool_t fig_gif_read_header_(fig_input *input, fig_uint8_t *version) {
    char buffer[FIG_GIF_HEADER_LENGTH];

    if(fig_input_read(input, buffer, FIG_GIF_HEADER_LENGTH, 1) != 1) {
        return 0;
    }
    if(memcmp(buffer, FIG_GIF_HEADER_VERSION_87a, FIG_GIF_HEADER_LENGTH) != 0
    && memcmp(buffer, FIG_GIF_HEADER_VERSION_89a, FIG_GIF_HEADER_LENGTH) != 0) {
        return 0;
    }
    *version = (buffer[3] - '0') * 10 + (buffer[4] - '0');
    return 1;
}

static fig_bool_t fig_gif_read_screen_descriptor_(fig_input *input, fig_gif_screen_descriptor_ *screen_desc) {
    fig_uint8_t packed_fields;

    if(fig_input_read_le_u16(input, &screen_desc->width)
    && fig_input_read_le_u16(input, &screen_desc->height)
    && fig_input_read_u8(input, &packed_fields)
    && fig_input_read_u8(input, &screen_desc->background_index)
    && fig_input_read_u8(input, &screen_desc->aspect)) {
        if((packed_fields & FIG_GIF_SCREEN_DESC_GLOBAL_COLOR) != 0) {
            screen_desc->global_colors = (1 << ((packed_fields & FIG_GIF_SCREEN_DESC_PALETTE_DEPTH_MASK) + 1));
        } else {
            screen_desc->global_colors = 0;
        }
        return 1;
    } else {
        return 0;
    }
}

static fig_bool_t fig_gif_read_image_descriptor_(fig_input *input, fig_gif_image_descriptor_ *image_desc) {
    fig_uint8_t packed_fields;

    if(fig_input_read_le_u16(input, &image_desc->x)
    && fig_input_read_le_u16(input, &image_desc->y)
    && fig_input_read_le_u16(input, &image_desc->width)
    && fig_input_read_le_u16(input, &image_desc->height)
    && fig_input_read_u8(input, &packed_fields)) {
        if((packed_fields & FIG_GIF_IMAGE_DESC_LOCAL_COLOR) != 0) {
            image_desc->local_colors = 1 << ((packed_fields & FIG_GIF_IMAGE_DESC_PALETTE_DEPTH_MASK) + 1);
        } else {
            image_desc->local_colors = 0;
        }
        image_desc->interlace = (packed_fields & FIG_GIF_IMAGE_DESC_INTERLACE) != 0;
        return 1;
    } else {
        return 0;
    }
}

static fig_bool_t fig_gif_read_sub_block_(fig_state *state, fig_input *input, fig_uint8_t *block_size, fig_uint8_t *block, fig_uint8_t max_size, const char *failure_message) {
    if(!fig_input_read_u8(input, block_size)
    || *block_size > max_size
    || fig_input_read(input, block, *block_size, 1) != 1) {
        fig_state_set_error(state, failure_message);
        return 0;
    }
    return 1;
}

static fig_bool_t fig_gif_skip_sub_blocks_(fig_input *input) {
    fig_uint8_t length;

    do {
        if(!fig_input_read_u8(input, &length)) {
            return 0;
        }
        fig_input_seek(input, length, FIG_SEEK_CUR);
    } while(length > 0);
    return 1;
}

static fig_bool_t fig_gif_read_graphics_control_(fig_input *input, fig_gif_graphics_control_ *gfx_ctrl) {
    fig_uint8_t length;
    fig_uint8_t packed_fields;

    if(fig_input_read_u8(input, &length)
    && length == 4
    && fig_input_read_u8(input, &packed_fields)
    && fig_input_read_le_u16(input, &gfx_ctrl->delay)
    && fig_input_read_u8(input, &gfx_ctrl->transparency_index)) {
        gfx_ctrl->transparent = packed_fields & FIG_GIF_GRAPHICS_CTRL_TRANSPARENCY;
        gfx_ctrl->disposal = (fig_gif_disposal_t_) ((packed_fields & FIG_GIF_GRAPHICS_CTRL_DISPOSAL_MASK) >> FIG_GIF_GRAPHICS_CTRL_DISPOSAL_SHIFT);
        if(gfx_ctrl->disposal >= FIG_GIF_DISPOSAL_COUNT) {
            gfx_ctrl->disposal = FIG_GIF_DISPOSAL_UNSPECIFIED;
        }

        if(fig_gif_skip_sub_blocks_(input)) {
            return 1;
        }
    }
    return 0;
}

static fig_bool_t fig_gif_read_looping_control_(fig_input *input, fig_uint16_t *loop_count) {
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

static fig_bool_t fig_gif_read_palette_(fig_input *input, size_t size, fig_palette *palette) {
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

static void fig_gif_error_lzw_stack_overflow_(fig_state *state) {
    fig_state_set_error(state, "overflowed available LZW character stack");
}

static void fig_gif_error_lzw_invalid_code_(fig_state *state) {
    fig_state_set_error(state, "invalid LZW code encountered");
}

static fig_bool_t fig_gif_read_image_data_(fig_state *state, fig_input *input, fig_gif_image_descriptor_ *image_desc, fig_uint8_t *index_data) {
    fig_uint8_t min_code_size;
    fig_uint16_t clear;
    fig_uint16_t eoi;
    fig_uint8_t code_size;
    fig_uint16_t code_mask;
    fig_uint16_t avail;
    fig_uint16_t old_code;
    fig_uint16_t prefix_codes[FIG_GIF_LZW_MAX_CODES];
    fig_uint8_t suffix_chars[FIG_GIF_LZW_MAX_CODES];
    fig_uint8_t char_stack[FIG_GIF_LZW_MAX_STACK_SIZE];
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
    if(min_code_size > FIG_GIF_LZW_MAX_BITS) {
        fig_state_set_error(state, "minimum LZW code size is too large");
        return 0;
    }
    
    clear = 1 << min_code_size;
    eoi = clear + 1;
    code_size = min_code_size + 1;
    code_mask = (1 << code_size) - 1;
    avail = eoi + 1;
    old_code = FIG_GIF_LZW_NULL_CODE;
    
    {
        fig_uint16_t i;

        for(i = 0; i < clear; ++i) {
            prefix_codes[i] = FIG_GIF_LZW_NULL_CODE;
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
                old_code = FIG_GIF_LZW_NULL_CODE;
            } else if(code == eoi) {
                fig_input_seek(input, sub_block_length, FIG_SEEK_CUR);
                return fig_gif_skip_sub_blocks_(input);
            } else if(old_code == FIG_GIF_LZW_NULL_CODE) {
                if(code >= avail) {
                    fig_gif_error_lzw_invalid_code_(state);
                    return 0;
                }
                if(char_stack_size >= FIG_GIF_LZW_MAX_STACK_SIZE) {
                    fig_gif_error_lzw_stack_overflow_(state);
                    return 0;
                }

                char_stack[char_stack_size++] = suffix_chars[code];
                first_char = code & 0xFF;
                old_code = code;
            } else if(code <= avail) {
                fig_uint16_t current_code = code;

                if(current_code == avail) {
                    if(char_stack_size >= FIG_GIF_LZW_MAX_STACK_SIZE) {
                        fig_gif_error_lzw_stack_overflow_(state);
                        return 0;
                    }
                    char_stack[char_stack_size++] = first_char;
                    current_code = old_code;
                }
                while(current_code >= clear) {
                    if(current_code >= FIG_GIF_LZW_MAX_CODES) {
                        fig_gif_error_lzw_invalid_code_(state);
                        return 0;
                    }
                    if(char_stack_size >= FIG_GIF_LZW_MAX_STACK_SIZE) {
                        fig_gif_error_lzw_stack_overflow_(state);
                        return 0;
                    }
                    char_stack[char_stack_size++] = suffix_chars[current_code];
                    current_code = prefix_codes[current_code];
                }

                first_char = suffix_chars[current_code];

                if(char_stack_size >= FIG_GIF_LZW_MAX_STACK_SIZE) {
                    fig_gif_error_lzw_stack_overflow_(state);
                    return 0;
                }
                char_stack[char_stack_size++] = first_char;

                if(avail < FIG_GIF_LZW_MAX_CODES) {
                    prefix_codes[avail] = old_code;
                    suffix_chars[avail] = first_char;
                    ++avail;
                    if((avail & code_mask) == 0 && avail < FIG_GIF_LZW_MAX_CODES) {
                        ++code_size;
                        code_mask = (1 << code_size) - 1;
                    }
                }

                old_code = code;
            } else {
                fig_gif_error_lzw_invalid_code_(state);
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

static fig_disposal_t fig_convert_gif_disposal_to_fig_disposal_(fig_gif_disposal_t_ disposal) {
    switch(disposal) {
        case FIG_GIF_DISPOSAL_UNSPECIFIED: return FIG_DISPOSAL_UNSPECIFIED;
        case FIG_GIF_DISPOSAL_NONE: return FIG_DISPOSAL_NONE;
        case FIG_GIF_DISPOSAL_BACKGROUND: return FIG_DISPOSAL_BACKGROUND;
        case FIG_GIF_DISPOSAL_PREVIOUS: return FIG_DISPOSAL_PREVIOUS;
        default:
            FIG_ASSERT(0);
            return FIG_DISPOSAL_UNSPECIFIED;
    }
}

fig_animation *fig_load_gif(fig_state *state, fig_input *input) {
    fig_uint8_t version;
    fig_gif_screen_descriptor_ screen_desc;
    fig_gif_graphics_control_ gfx_ctrl;    
    fig_animation *animation;
    
    if(input == NULL) {
        fig_state_set_error(state, "input is NULL");
        return NULL;
    }

    memset(&screen_desc, 0, sizeof(screen_desc));
    memset(&gfx_ctrl, 0, sizeof(gfx_ctrl));

    if(!fig_gif_read_header_(input, &version)) {
        fig_state_set_error(state, "failed to read header");
        return NULL;
    }
    if(!fig_gif_read_screen_descriptor_(input, &screen_desc)) {
        fig_state_set_error(state, "failed to read screen descriptor");
        return NULL;
    }

    animation = fig_create_animation(state);
    if(animation == NULL) {
        return NULL;
    }
    fig_animation_set_dimensions(animation, screen_desc.width, screen_desc.height);

    if(screen_desc.global_colors > 0
    && !fig_gif_read_palette_(input, screen_desc.global_colors, fig_animation_get_palette(animation))) {
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
            case FIG_GIF_BLOCK_EXTENSION: {
                fig_uint8_t extension_type;

                if(!fig_input_read_u8(input, &extension_type)) {
                    fig_state_set_error(state, "failed to read extension block type");
                    return fig_animation_free(animation), NULL;
                }
                switch(extension_type) {
                    case FIG_GIF_EXT_GRAPHICS_CONTROL: {
                        if(!fig_gif_read_graphics_control_(input, &gfx_ctrl)) {
                            fig_state_set_error(state, "failed to read graphics control block");
                            return fig_animation_free(animation), NULL;
                        }
                        break;
                    }
                    /*case FIG_GIF_EXT_PLAIN_TEXT:
                    case FIG_GIF_EXT_COMMENT:*/
                    case FIG_GIF_EXT_APPLICATION: {
                        fig_uint8_t application_block_size;
                        fig_uint8_t application_block[FIG_GIF_APPLICATION_SIGNATURE_LENGTH];
                        if(!fig_gif_read_sub_block_(state, input, &application_block_size, application_block, sizeof(application_block), "failed to read application signature")) {
                            return fig_animation_free(animation), NULL;
                        }
                        if(memcmp(application_block, FIG_GIF_APPLICATION_SIGNATURE_NETSCAPE, FIG_GIF_APPLICATION_SIGNATURE_LENGTH) == 0) {
                            fig_uint16_t loop_count;
                            if(!fig_gif_read_looping_control_(input, &loop_count)) {
                                fig_state_set_error(state, "failed to read looping control block");
                                return fig_animation_free(animation), NULL;
                            }
                            fig_animation_set_loop_count(animation, loop_count);
                        }
                        if(!fig_gif_skip_sub_blocks_(input)) {
                            fig_state_set_error(state, "failed to skip extension block");
                            return fig_animation_free(animation), NULL;
                        }
                        break;
                    }
                    default: {
                        if(!fig_gif_skip_sub_blocks_(input)) {
                            fig_state_set_error(state, "failed to skip extension block");
                            return fig_animation_free(animation), NULL;
                        }
                        break;
                    }
                }
                break;
            }
            case FIG_GIF_BLOCK_IMAGE: {
                fig_image *image;
                fig_gif_image_descriptor_ image_desc;
                
                memset(&image_desc, 0, sizeof(image_desc));
                if(!fig_gif_read_image_descriptor_(input, &image_desc)) {
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
                && !fig_gif_read_palette_(input, image_desc.local_colors, fig_image_get_palette(image))) {
                    fig_state_set_error(state, "failed to read frame local palette");
                    return fig_animation_free(animation), NULL;
                }

                fig_image_set_origin_x(image, image_desc.x);
                fig_image_set_origin_y(image, image_desc.y);
                fig_image_set_transparent(image, gfx_ctrl.transparent);
                fig_image_set_transparency_index(image, gfx_ctrl.transparency_index);
                fig_image_set_delay(image, gfx_ctrl.delay);
                fig_image_set_disposal(image, fig_convert_gif_disposal_to_fig_disposal_(gfx_ctrl.disposal));

                if(!fig_gif_read_image_data_(state, input, &image_desc, fig_image_get_indexed_data(image))) {
                    return fig_animation_free(animation), NULL;
                }

                break;
            }
            case FIG_GIF_BLOCK_TERMINATOR: {
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
static void fig_gif_error_write_failed_(fig_state *state) {
    fig_state_set_error(state, "failed to write data");
}

static size_t fig_gif_log2_(size_t value) {
    size_t result = 0;
    while(value >>= 1) {
        result++;
    }
    return result;
}

static fig_bool_t fig_gif_write_palette_(fig_state *state, fig_output *output, fig_palette *palette, size_t color_depth) {
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
            fig_gif_error_write_failed_(state);
            return 0;
        }
    }

    return 1;
}

static size_t fig_gif_get_color_depth_(fig_palette *palette) {
    size_t color_depth = fig_gif_log2_(fig_palette_count_colors(palette));
    if(color_depth == 0) {
        color_depth++;
    }
    return color_depth;
}

static fig_bool_t fig_gif_block_flush_(fig_state *state, fig_output *output, fig_uint8_t *block) {
    size_t length = block[0];
    if(length == 0
    || fig_output_write(output, block, length + 1, 1) == 1) {
        block[0] = 0;
        return 1;
    } else {
        fig_gif_error_write_failed_(state);
        return 0;
    }
}

static fig_bool_t fig_gif_block_write_u8_(fig_state *state, fig_output *output, fig_uint8_t *block, fig_uint8_t value) {
    block[++block[0]] = value;
    if(block[0] == 0xFF) {
        return fig_gif_block_flush_(state, output, block);
    } else {
        return 1;
    }
}

static fig_bool_t fig_gif_block_write_bits_(fig_state *state, fig_output *output, fig_uint8_t *block, fig_uint32_t *accumulator, fig_uint8_t *accumulator_length, fig_uint16_t value, fig_uint8_t value_length) {
    fig_uint8_t current_accumulator_length = *accumulator_length;
    fig_uint32_t current_accumulator = *accumulator | ((fig_uint32_t) value << current_accumulator_length);

    current_accumulator_length += value_length;

    while(current_accumulator_length >= 8) {
        if(!fig_gif_block_write_u8_(state, output, block, current_accumulator & 0xFF)) {
            return 0;
        }

        current_accumulator >>= 8;
        current_accumulator_length -= 8;
    }

    *accumulator = current_accumulator;
    *accumulator_length = current_accumulator_length;
    return 1;
}

static fig_bool_t fig_gif_write_image_data_(fig_state *state, fig_output *output, fig_image *image, fig_palette *palette) {
    fig_uint8_t min_code_size;
    fig_uint16_t clear;
    fig_uint16_t eoi;
    fig_uint8_t code_size;
    fig_uint16_t code_mask;
    fig_uint16_t avail;
    fig_uint16_t old_code;
    fig_uint16_t prefix_codes[FIG_GIF_LZW_MAX_CODES];
    fig_uint8_t suffix_chars[FIG_GIF_LZW_MAX_CODES];
    fig_uint8_t block[256];
    size_t pixel_index;
    size_t pixel_count;
    size_t color_depth;
    size_t padded_palette_size;
    fig_uint8_t* pixels;
    fig_uint32_t accumulator;
    fig_uint8_t accumulator_length;

    color_depth = fig_gif_get_color_depth_(palette);
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
    old_code = FIG_GIF_LZW_NULL_CODE;
    accumulator = 0;
    accumulator_length = 0;
    block[0] = 0;
    
    {
        fig_uint16_t i;

        for(i = 0; i < clear; ++i) {
            prefix_codes[i] = FIG_GIF_LZW_NULL_CODE;
            suffix_chars[i] = i & 0xFF;
        }
    }

    if(!fig_output_write_u8(output, min_code_size)) {
        fig_gif_error_write_failed_(state);
        return 0;
    }
    if(!fig_gif_block_write_bits_(state, output, block, &accumulator, &accumulator_length, clear, code_size)) {
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

        if(old_code == FIG_GIF_LZW_NULL_CODE) {
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
                if(!fig_gif_block_write_bits_(state, output, block, &accumulator, &accumulator_length, old_code, code_size)) {
                    return 0;
                }

                if(avail >= FIG_GIF_LZW_MAX_CODES) {
                    if(!fig_gif_block_write_bits_(state, output, block, &accumulator, &accumulator_length, clear, code_size)) {
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

    if(!fig_gif_block_write_bits_(state, output, block, &accumulator, &accumulator_length, old_code, code_size)
    || !fig_gif_block_write_bits_(state, output, block, &accumulator, &accumulator_length, eoi, code_size)) {
        return 0;
    }

    while(accumulator_length > 0) {
        if(!fig_gif_block_write_u8_(state, output, block, accumulator & 0xFF)) {
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

    if(!fig_gif_block_flush_(state, output, block)
    || !fig_output_write_u8(output, 0)) {
        fig_gif_error_write_failed_(state);
        return 0;
    }

    return 1;
}

static fig_gif_disposal_t_ fig_convert_fig_disposal_to_gif_disposal_(fig_disposal_t disposal) {
    switch(disposal) {
        case FIG_DISPOSAL_UNSPECIFIED: return FIG_GIF_DISPOSAL_UNSPECIFIED;
        case FIG_DISPOSAL_NONE: return FIG_GIF_DISPOSAL_NONE;
        case FIG_DISPOSAL_BACKGROUND: return FIG_GIF_DISPOSAL_BACKGROUND;
        case FIG_DISPOSAL_PREVIOUS: return FIG_GIF_DISPOSAL_PREVIOUS;
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

    if(fig_output_write(output, FIG_GIF_HEADER_VERSION_89a, FIG_GIF_HEADER_LENGTH, 1) != 1) {
        fig_gif_error_write_failed_(state);
        return 0;
    }
    
    global_palette = fig_animation_get_palette(animation);
    if(global_palette != NULL
    && fig_palette_count_colors(global_palette) != 0) {
        global_color_depth = fig_gif_get_color_depth_(global_palette);

        if(global_color_depth > FIG_GIF_SCREEN_DESC_PALETTE_DEPTH_MASK + 1) {
            fig_state_set_error(state, "too many colors in global animation palette");
            return 0;
        }

        screen_desc_packed_fields = ((global_color_depth - 1) & FIG_GIF_SCREEN_DESC_PALETTE_DEPTH_MASK) | FIG_GIF_SCREEN_DESC_GLOBAL_COLOR;
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
        fig_gif_error_write_failed_(state);
        return 0;
    }
    
    if((screen_desc_packed_fields & FIG_GIF_SCREEN_DESC_GLOBAL_COLOR) != 0
    && !fig_gif_write_palette_(state, output, global_palette, global_color_depth)) {
        return 0;
    }

    image_count = fig_animation_count_images(animation);
    images = fig_animation_get_images(animation);    

    if(image_count > 1) {
        if(!fig_output_write_u8(output, FIG_GIF_BLOCK_EXTENSION)
        || !fig_output_write_u8(output, FIG_GIF_EXT_APPLICATION)
        || !fig_output_write_u8(output, FIG_GIF_APPLICATION_SIGNATURE_LENGTH)
        || fig_output_write(output, FIG_GIF_APPLICATION_SIGNATURE_NETSCAPE, FIG_GIF_APPLICATION_SIGNATURE_LENGTH, 1) != 1
        || !fig_output_write_u8(output, 3)
        || !fig_output_write_u8(output, 1)
        || !fig_output_write_le_u16(output, fig_animation_get_loop_count(animation) <= 0xFFFF ? (fig_uint16_t) fig_animation_get_loop_count(animation) : 0)
        || !fig_output_write_u8(output, 0)) {
            fig_gif_error_write_failed_(state);
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

            if(!fig_output_write_u8(output, FIG_GIF_BLOCK_EXTENSION)
            || !fig_output_write_u8(output, FIG_GIF_EXT_GRAPHICS_CONTROL)
            || !fig_output_write_u8(output, 4)
            || !fig_output_write_u8(output,
                (fig_image_get_transparent(image) ? FIG_GIF_GRAPHICS_CTRL_TRANSPARENCY : 0)
                | (fig_convert_fig_disposal_to_gif_disposal_(fig_image_get_disposal(image)) << FIG_GIF_GRAPHICS_CTRL_DISPOSAL_SHIFT))
            || !fig_output_write_le_u16(output, delay <= 0xFFFF ? (fig_uint16_t) delay : 0)
            || !fig_output_write_u8(output, fig_image_get_transparent(image) ? (fig_uint8_t) fig_image_get_transparency_index(image) : 0)
            || !fig_output_write_u8(output, 0)) {
                fig_gif_error_write_failed_(state);
                return 0;
            }
        }

        local_palette = fig_image_get_palette(image);
        if(local_palette != NULL
        && fig_palette_count_colors(local_palette) != 0) {
            local_color_depth = fig_gif_get_color_depth_(local_palette);

            if(local_color_depth > FIG_GIF_IMAGE_DESC_PALETTE_DEPTH_MASK + 1) {
                fig_state_set_error(state, "too many colors in local frame palette");
                return 0;
            }

            image_desc_packed_fields = ((local_color_depth - 1) & FIG_GIF_IMAGE_DESC_PALETTE_DEPTH_MASK) | FIG_GIF_IMAGE_DESC_LOCAL_COLOR;
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

        if(!fig_output_write_u8(output, FIG_GIF_BLOCK_IMAGE)
        || !fig_output_write_le_u16(output, (fig_uint16_t) fig_image_get_origin_x(image))
        || !fig_output_write_le_u16(output, (fig_uint16_t) fig_image_get_origin_y(image))
        || !fig_output_write_le_u16(output, (fig_uint16_t) fig_image_get_indexed_width(image))
        || !fig_output_write_le_u16(output, (fig_uint16_t) fig_image_get_indexed_height(image))
        || !fig_output_write_u8(output, image_desc_packed_fields)) {
            fig_gif_error_write_failed_(state);
            return 0;
        }

        if((image_desc_packed_fields & FIG_GIF_IMAGE_DESC_LOCAL_COLOR) != 0
        && !fig_gif_write_palette_(state, output, local_palette, local_color_depth)) {
            return 0;
        }
        if((image_desc_packed_fields & FIG_GIF_IMAGE_DESC_LOCAL_COLOR) == 0
        && (screen_desc_packed_fields & FIG_GIF_SCREEN_DESC_GLOBAL_COLOR) == 0) {
            fig_state_set_error(state, "image has no valid palette (local or global)");
            return 0;
        }

        fig_gif_write_image_data_(state, output, image, (image_desc_packed_fields & FIG_GIF_IMAGE_DESC_LOCAL_COLOR) != 0 ? local_palette : global_palette);
    }

    fig_output_write_u8(output, FIG_GIF_BLOCK_TERMINATOR);

    return 1;
}
#endif
