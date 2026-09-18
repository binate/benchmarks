# mandelbrot — see ../c/mandelbrot.c for the algorithm.
import sys


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 200
    w = h = n
    out = sys.stdout.buffer
    out.write(b"P4\n%d %d\n" % (w, h))

    ba = bytearray()
    byte_acc = 0
    bit_num = 0
    for y in range(h):
        for x in range(w):
            zr = zi = tr = ti = 0.0
            cr = 2.0 * x / w - 1.5
            ci = 2.0 * y / h - 1.0
            i = 0
            while i < 50 and tr + ti <= 4.0:
                zi = 2.0 * zr * zi + ci
                zr = tr - ti + cr
                tr = zr * zr
                ti = zi * zi
                i += 1
            byte_acc = (byte_acc << 1) & 0xFF
            if tr + ti <= 4.0:
                byte_acc |= 1
            bit_num += 1
            if bit_num == 8:
                ba.append(byte_acc)
                byte_acc = 0
                bit_num = 0
            elif x == w - 1:
                byte_acc = (byte_acc << (8 - w % 8)) & 0xFF
                ba.append(byte_acc)
                byte_acc = 0
                bit_num = 0
    out.write(bytes(ba))


main()
