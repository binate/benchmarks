// mandelbrot — see ../c/mandelbrot.c for the algorithm.
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.io.OutputStream;

public final class Mandelbrot {
    public static void main(String[] args) throws IOException {
        int n = args.length > 0 ? Integer.parseInt(args[0]) : 200;
        int w = n, h = n;
        OutputStream out = new BufferedOutputStream(System.out);
        out.write(String.format("P4\n%d %d\n", w, h).getBytes());

        int byteAcc = 0, bitNum = 0;
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                double Zr = 0, Zi = 0, Tr = 0, Ti = 0;
                double Cr = 2.0 * x / w - 1.5;
                double Ci = 2.0 * y / h - 1.0;
                for (int i = 0; i < 50 && Tr + Ti <= 4.0; i++) {
                    Zi = 2.0 * Zr * Zi + Ci;
                    Zr = Tr - Ti + Cr;
                    Tr = Zr * Zr;
                    Ti = Zi * Zi;
                }
                byteAcc <<= 1;
                if (Tr + Ti <= 4.0) byteAcc |= 1;
                if (++bitNum == 8) {
                    out.write(byteAcc); byteAcc = 0; bitNum = 0;
                } else if (x == w - 1) {
                    byteAcc <<= (8 - w % 8);
                    out.write(byteAcc); byteAcc = 0; bitNum = 0;
                }
            }
        }
        out.flush();
    }
}
