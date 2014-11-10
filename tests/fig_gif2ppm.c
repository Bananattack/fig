#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fig.h>

int main(int argc, char **argv) {
    char buffer[256];
    size_t i, j;
    FILE *f;
    fig_source *src;
    fig_image *image;

    if(argc < 2) {
        fputs("Usage: fig_gif2ppm filename\n", stderr);
        return 1;
    }

    f = fopen(argv[1], "rb");
    if(f == NULL) {
        return 1;
    }

    src = fig_create_file_source(f);
    image = fig_load_gif(src);
    fig_source_free(src);
    fclose(f);
    if(image == NULL) {
        return 1;
    }

    {
        size_t frame_count;
        fig_frame **frames;
        frame_count = fig_image_count_frames(image);
        frames = fig_image_get_frames(image);

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

    fig_image_free(image);

    return 0;
}