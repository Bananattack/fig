#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fig.h>
#include <fig_gif.h>

const char *filename = "../examples/FullColourGIF.gif";

int main(int argc, char **argv) {
    char buffer[256];
    size_t i, j;
    FILE *f;
    fig_source *src;
    fig_image *image;

    (void) argc;
    (void) argv;

    f = fopen(filename, "rb");
    if(f == NULL) {
        return 1;
    }

    src = fig_create_file_source(f);
    image = fig_load_gif(src);
    if(image == NULL) {
        return 1;
    }
    fig_source_free(src);
    fclose(f);

    {
        fig_animation *anim;
        size_t anim_size;

        anim = fig_image_get_animation(image);
        anim_size = fig_animation_get_size(anim);

        for(i = 0; i < anim_size; ++i) {
            fig_frame *frame;
            size_t width, height, frame_size;
            fig_uint32_t *data;

            frame = fig_animation_get(anim, i);
            width = fig_frame_get_width(frame);
            height = fig_frame_get_height(frame);
            frame_size = width * height;
            data = fig_frame_get_color_data(frame);

            sprintf(buffer, "out.%03d.ppm", i);

            f = fopen(buffer, "wb");

            if(f != NULL) {
                fprintf(f, "P6 %d %d 255 ", width, height);
                for(j = 0; j < frame_size; ++j) {
                    fig_uint8_t out[3];

                    out[0] = (data[j] >> 16) & 0xFF;
                    out[1] = (data[j] >> 8) & 0xFF;
                    out[2] = data[j] & 0xFF;
                    fwrite(out, 3, 1, f);
                }
                fclose(f);
            }
        }
    }

    fig_image_free(image);

    return 0;
}