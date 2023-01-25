#include <bmpfile.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

#define WIDTH 1600
#define HEIGHT 600
#define SCALE 20

void draw_bmp(bmpfile_t *bmp, int x0, int y0) {
    // Configuration about the circle:
    const int radius = SCALE;
    rgb_pixel_t pixel = {255, 0, 0, 0}; // Color of the circle (BGRA)
    rgb_pixel_t center = {0, 0, 255, 0};

    // Check if x0,y0 are inside boudaries:
    if (x0 < radius) {
        x0 = radius;
    } else if (WIDTH - x0 < radius) {
        x0 = WIDTH - radius;
    }
    if (y0 < radius) {
        y0 = radius;
    } else if (HEIGHT - y0 < radius) {
        y0 = HEIGHT - radius;
    }

    // Code for drawing a circle of given radius...
    for(int x = -radius; x <= radius; x++) {
        for(int y = -radius; y <= radius; y++) {
        // If distance is smaller, point is within the circle
        if(x*x + y*y < radius*radius) {
            /*
            * Color the pixel at the specified (x,y) position
            * with the given pixel values
            */
            if (x == 0 && y == 0){
                bmp_set_pixel(bmp, x0, y0, center);
            } else bmp_set_pixel(bmp, x0 + x, y0 + y, pixel);
            }
        }
    }
}

void save_bmp(bmpfile_t *bmp, rgb_pixel_t *matrix) {
    for(int x = 0; x < WIDTH; x++) {
        for(int y = 0; y < HEIGHT; y++) {
            // Get pixel at x,y:
            rgb_pixel_t *pixel = bmp_get_pixel(bmp, x, y);
            // Store the pixel in the matrix:
            matrix[x + y*WIDTH].blue = pixel->blue;
            matrix[x + y*WIDTH].green = pixel->green;
            matrix[x + y*WIDTH].red = pixel->red;
            matrix[x + y*WIDTH].alpha = pixel->alpha;
        }
    }
}

void load_bmp(bmpfile_t *bmp, rgb_pixel_t *matrix) {
    for(int x = 0; x < WIDTH; x++) {
        for(int y = 0; y < HEIGHT; y++) {
            // Set pixel at x,y from matrix:
            bmp_set_pixel(bmp, x, y, matrix[x + y*WIDTH]);
        }
    }
}

void find_circle_center(rgb_pixel_t* matrix, int* pos) {
    int max_len = 0, count = 1; 
    for(int x = 0; x < WIDTH; x++) {
        // Length of the current chord:
        int len = 0;
        for(int y = 0; y < HEIGHT; y++) {
            // Get pixel at x,y:
            rgb_pixel_t pixel = matrix[x + y*WIDTH];
            // If pixel is not white, it is part of the circle:
            if (pixel.blue + pixel.green + pixel.red < 765) {
                // Check if previous pixel was white for the initial position:
                rgb_pixel_t prev_pixel  = matrix[x + (y-1)*WIDTH];
                if (prev_pixel.blue == 255 && prev_pixel.green == 255 && prev_pixel.red == 255) {
                    pos[0] = x;
                    pos[1] = y;
                }
                len++;
            }
        }
        // If this chord is bigger than the previous, keep on looking:
        if (len > max_len) {
            max_len = len;
            count = 1;
        }
        // If chord length is equal than the previous, add to count:
        else if (len == max_len) count++;
        // If chord length is less than previous, we break because we already know the center:
        else break;
    }
    // Compute the center of the circle:
    pos[0] = pos[0] - count / 2 - 1;
    pos[1] = pos[1] + max_len / 2 - 1;
    // The "-1" are needed because the last position stored in pos belongs 
    // to the first chord that is smaller than previous one.
}
