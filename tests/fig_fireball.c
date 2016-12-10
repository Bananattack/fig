#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fig.h>

const fig_uint32_t PICO8_PALETTE[16] = {
    0xFF000000,
    0xFF1D2B53,
    0xFF7E2553,
    0xFF008751,
    0xFFAB5236,
    0xFF5F574F,
    0xFFC2C3C7,
    0xFFFFF1E8,
    0xFFFF004D,
    0xFFFFA300,
    0xFFFFF024,
    0xFF00E756,
    0xFF29ADFF,
    0xFF83769C,
    0xFFFF77A8,
    0xFFFFCCAA,
};

const fig_uint8_t STARTING_COLOR_INDEXES[] = {
    10,
    10,
    10,
    10,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    4,
    4,
    4,
    2,
    2,
    2,
    2,
    1,
};


int main(int argc, char **argv) {
    char *dest_filename;
    FILE *dest_file;
    fig_state *state;
    fig_output *output;
    fig_animation *animation;
    fig_palette *palette;
    size_t i, j, k;
    size_t frame_index;
    fig_image *prev_image = NULL;

    dest_filename = argc >= 2 ? argv[1] : "out.gif";        

    state = fig_create_state();
    animation = fig_create_animation(state);
    palette = fig_animation_get_palette(animation);
    fig_palette_resize(palette, 16);
    for(i = 0; i != 16; ++i) {
        fig_palette_set(palette, i, PICO8_PALETTE[i]);
    }
    fig_animation_set_dimensions(animation, 128, 128);

    for(frame_index = 0; frame_index != 128; ++frame_index) {
        fig_uint8_t *indexed_data;

        fig_image *image = fig_animation_add_image(animation);
        fig_image_resize_indexed(image, 128, 128);
        fig_image_set_delay(image, 6);

        indexed_data = fig_image_get_indexed_data(image);

        if(prev_image != NULL) {
            memcpy(indexed_data, fig_image_get_indexed_data(prev_image), 128 * 128);
        } else {
            memset(indexed_data, 0, 128 * 128);
        }

        for(i = 0; i != 128; i++) {
            for(j = 0; j != 128; j++) {
                if(indexed_data[i * 128 + j] > 0) {
                    size_t y = i;
                    size_t x = j;
                    int r = rand() % 5;
                    if(r == 0 && x > 0) {
                        x--;
                    } else if(r == 1 && x < 127) {
                        x++;
                    } else if(r == 2 && y > 0) {
                        y--;
                    } else if(r == 3 && y < 127) {
                        y++;
                    }

                    indexed_data[y * 128 + x] = rand() % 8 < 4 ? indexed_data[i * 128 + j] / 2 : indexed_data[i * 128 + j];
                }
            }
        }

        for(k = rand() % 4 + 4; k != 0; --k) {
            size_t size = rand() % 8 + 8;
            size_t y = rand() % (128 - size);
            size_t x = rand() % (128 - size);            
            for(i = 0; i != size; ++i) {
                for(j = 0; j != size; ++j) {
                    fig_uint8_t color = STARTING_COLOR_INDEXES[rand() % (sizeof(STARTING_COLOR_INDEXES) / sizeof(*STARTING_COLOR_INDEXES))];
                    indexed_data[(y + i) * 128 + (x + j)] = color;
                }
            }
        }

        prev_image = image;
    }

    dest_file = fopen(dest_filename, "wb");
    if(dest_file == NULL) {
        fputs("failed to open output file '", stderr);
        fputs(dest_filename, stderr);
        fputs("' \n", stderr);
        return 1;
    }
    
    output = fig_create_file_output(state, dest_file);
    if(!fig_save_gif(state, output, animation)) {
        if(fig_state_get_error(state) != NULL) {
            fputs("error while saving: ", stderr);
            fputs(fig_state_get_error(state), stderr);
            fputs("\n", stderr);
        }
        return 1;
    }

    fig_output_free(output);
    fclose(dest_file);

    fig_animation_free(animation);
    fig_state_free(state);

    return 0;
}