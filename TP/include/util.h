#ifndef UTIL_H
#define UTIL_H


static const int DIRS[8][2] = {
    {0,-1},   // UP
    {1,-1},   // UP_RIGHT
    {1,0},    // RIGHT
    {1,1},    // DOWN_RIGHT
    {0,1},    // DOWN
    {-1,1},   // DOWN_LEFT
    {-1,0},   // LEFT
    {-1,-1}   // UP_LEFT
};

/**
 * Converts a direction vector to an unsigned char (0-7)
 * Direction mapping (clockwise from top):
 * 0: North     (0, -1)
 * 1: NorthEast (1, -1)
 * 2: East      (1,  0)
 * 3: SouthEast (1,  1)
 * 4: South     (0,  1)
 * 5: SouthWest (-1, 1)
 * 6: West      (-1, 0)
 * 7: NorthWest (-1,-1)
 * 
 * @param direction: int vector [2] representing (x, y) direction
 * @return: unsigned char from 0-7, or 255 if invalid direction
 */
unsigned char direction_to_char(const int direction[2]);

#endif //UTIL_H
