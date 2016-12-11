#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fig.h>

int main(int argc, char **argv) {
    FILE *src_file;
    FILE *dest_file;
    fig_state *state;
    fig_input *input;
    fig_output *output;
    fig_animation *animation;

    if(argc < 3) {
        fputs("Usage: fig_gif2gif input_filename output_filename\n", stderr);
        return 1;
    }

    src_file = fopen(argv[1], "rb");
    if(src_file == NULL) {
        fputs("failed to open input file '", stderr);
        fputs(argv[1], stderr);
        fputs("' \n", stderr);
        return 1;
    }

    state = fig_create_state();
    input = fig_create_file_input(state, src_file);
    animation = fig_load_gif(state, input);
    fig_input_free(input);
    fclose(src_file);

    if(animation == NULL) {
        if(fig_state_get_error(state) != NULL) {
            fputs("error while reading: ", stderr);
            fputs(fig_state_get_error(state), stderr);
            fputs("\n", stderr);
        }
        return 1;
    }

    dest_file = fopen(argv[2], "wb");
    if(dest_file == NULL) {
        fputs("failed to open output file '", stderr);
        fputs(argv[2], stderr);
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