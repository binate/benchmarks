/* mandelbrot: render the Mandelbrot set as a P4 (raw 1-bit) PBM bitmap of
 * size N×N — 50 escape iterations per pixel, escape radius 2. Single-threaded,
 * scalar, fixed operation order — the reference all other implementations must
 * match byte-for-byte. */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int w, h, bit_num = 0;
    unsigned char byte_acc = 0;
    int iter = 50;
    double limit = 2.0;

    w = h = argc > 1 ? atoi(argv[1]) : 200;
    printf("P4\n%d %d\n", w, h);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            double Zr = 0.0, Zi = 0.0, Tr = 0.0, Ti = 0.0;
            double Cr = 2.0 * x / w - 1.5;
            double Ci = 2.0 * y / h - 1.0;
            for (int i = 0; i < iter && (Tr + Ti <= limit * limit); ++i) {
                Zi = 2.0 * Zr * Zi + Ci;
                Zr = Tr - Ti + Cr;
                Tr = Zr * Zr;
                Ti = Zi * Zi;
            }
            byte_acc <<= 1;
            if (Tr + Ti <= limit * limit) byte_acc |= 0x01;
            if (++bit_num == 8) {
                putc(byte_acc, stdout);
                byte_acc = 0; bit_num = 0;
            } else if (x == w - 1) {
                byte_acc <<= (8 - w % 8);
                putc(byte_acc, stdout);
                byte_acc = 0; bit_num = 0;
            }
        }
    }
    return 0;
}
