#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fig.h>

int main(int argc, char **argv) {
    char buffer[256];
    size_t i, j;
    FILE *f;
    fig_source *src;
    fig_animation *animation;

    if(argc < 2) {
        fputs("Usage: fig_gif2ppm filename\n", stderr);
        return 1;
    }

    f = fopen(argv[1], "rb");
    if(f == NULL) {
        return 1;
    }

    src = fig_create_file_source(f);
    animation = fig_load_gif(src);
    fig_source_free(src);
    fclose(f);
    if(animation == NULL) {
        return 1;
    }

    {
        size_t frame_count;
        fig_frame **frames;
        frame_count = fig_animation_count_frames(animation);
        frames = fig_animation_get_frames(animation);

        for(i = 0; i < frame_count; ++i) {
            fig_frame *frame;
            size_t width, height, frame_size;
            fig_uint32_t *data;

            frame = frames[i];
            width = fig_frame_get_render_width(frame);
            height = fig_frame_get_render_height(frame);
            frame_size = width * height;
            data = fig_frame_get_render_data(frame);

            sprintf(buffer, "out.%03d.ppm", i);

            f = fopen(buffer, "wb");

            if(f != NULL) {
                fprintf(f, "P6 %d %d 255 ", width, height);
                for(j = 0; j < frame_size; ++j) {
                    fig_uint8_t out[3];

                    if(((data[j] >> 24) & 0xFF) == 0) {
                        out[0] = 0xFF;
                        out[1] = 0x00;
                        out[2] = 0xFF;
                    } else {
                        out[0] = (data[j] >> 16) & 0xFF;
                        out[1] = (data[j] >> 8) & 0xFF;
                        out[2] = data[j] & 0xFF;
                    }
                    fwrite(out, 3, 1, f);
                }
                fclose(f);
            }
        }
    }

    fig_animation_free(animation);

    return 0;
}