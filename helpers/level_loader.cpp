// Level Loading Test

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 4096
#define MAX_ROWS 18
#define MAX_COLS 32

#define FLIPPED_HORIZONTALLY_FLAG 0x80000000
#define FLIPPED_VERTICALLY_FLAG   0x40000000

int tiles[MAX_ROWS][MAX_COLS][3] = {};

int main() {
    FILE *file = fopen("levels/level1/2.csv", "r");
    if (file == NULL) {
        printf("File open failed\n");
        return 1;
    }

    char line[MAX_LINE_LENGTH];
    int row = 0;

    while (fgets(line, sizeof(line), file)) {
        char *cell;
        int col = 0;

        printf("Row: %d\n", row);

        cell = strtok(line, ",\n");
        while (cell) {
            int gid = atoi(cell);

            if (gid == -1) {
                tiles[row][col][0] = -1;
                tiles[row][col][1] = 0;
                tiles[row][col][2] = 0;
            } else {
                int flip_h = (gid & FLIPPED_HORIZONTALLY_FLAG) != 0;
                int flip_v = (gid & FLIPPED_VERTICALLY_FLAG) != 0;

                // Mask out flip flags to get actual GID
                int id = gid & ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG);
                tiles[row][col][0] = id;
                tiles[row][col][1] = flip_h;
                tiles[row][col][2] = flip_v;
                printf("Col: %d: ID: %d, H: %d, V: %d\n", col, id, flip_h, flip_v);
            }

            cell = strtok(NULL, ",\n");
            col++;
        }

        printf("\n");
        row++;
    }

    fclose(file);
    return 0;
}
