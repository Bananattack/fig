#include <stdio.h>
#include <stdlib.h>
#include <fig.h>
#include <fig_gif.h>

int main(int argc, char **argv) {
    size_t i, j, size, size2;
    FILE *f;
    fig_source *src;
    fig_image *image;
    fig_palette *palette;
    fig_animation *anim;

    f = fopen("../examples/FullColourGIF.gif", "rb");
    if(f == NULL) {
        return 1;
    }

    src = fig_create_file_source(f);
    if(src == NULL) {
        fclose(f);
        return 1;
    }

    image = fig_load_gif(src);
    if(image == NULL) {
        fclose(f);
        fig_source_free(src);
        return 1;
    }

    printf("Size: %d x %d\n\n", fig_image_get_width(image), fig_image_get_height(image));
    printf("Palette:\n");
    
    palette = fig_image_get_palette(image);
    for(i = 0, size = fig_palette_get_size(palette); i < size; i++) {
        printf("    %02X: %08X\n", i, fig_palette_get(palette, i));
    }

    anim = fig_image_get_animation(image);
    printf("Animation:\n", i);
    for(i = 0, size = fig_animation_get_size(anim); i < size; i++) {
        printf("Frame #%d:\n", i);
        palette = fig_frame_get_palette(fig_animation_get(anim, i));
        for(j = 0, size2 = fig_palette_get_size(palette); j < size2; j++) {
            printf("    %02X: %08X\n", j, fig_palette_get(palette, j));
        }
    }

    fig_image_free(image);
    fig_source_free(src);
    fclose(f);

    return 0;
}
