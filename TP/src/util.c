#include "util.h"
#include "common.h"

unsigned char direction_to_char(const int direction[2]) {
    for (int i = 0; i < 8; i++) {
        if (DIRS[i][0] == direction[0] && DIRS[i][1] == direction[1]) {
            return (unsigned char)i;
        }
    }
    return 255;
}