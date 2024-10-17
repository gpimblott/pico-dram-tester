#include <stdio.h>
#include <stdint.h>
#include "prop_font.h"

int main(void)
{
    char c = 'A';
    uint8_t width = font_width_table[c];
    uint16_t offset = font_offset_table[c];
    int row;
    int bytes_per_row = (width + 7) >> 3;
    int b;

    printf("Character code %d, looks like %c\n", c, c);
    printf("Font is %d high by %d wide.\n", font_height, width);
    printf("Character offset is %d\n", offset);

    for (row = 0; row < font_height; row++) {
        printf("%.2d: ", row);
        for (b = 0; b < bytes_per_row; b++) {
            printf("%.2x ", font_data_table[offset + b + row * bytes_per_row]);
        }
        printf("\n");
    }

    return 0;
}
