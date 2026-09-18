// mandelbrot — see ../c/mandelbrot.c for the algorithm.
use std::io::{self, Write};

fn main() {
    let n: i32 = std::env::args().nth(1).and_then(|s| s.parse().ok()).unwrap_or(200);
    let (w, h) = (n, n);
    let stdout = io::stdout();
    let mut out = io::BufWriter::new(stdout.lock());
    write!(out, "P4\n{} {}\n", w, h).unwrap();

    let mut byte_acc: u8 = 0;
    let mut bit_num = 0;
    for y in 0..h {
        for x in 0..w {
            let (mut zr, mut zi, mut tr, mut ti) = (0.0f64, 0.0f64, 0.0f64, 0.0f64);
            let cr = 2.0 * x as f64 / w as f64 - 1.5;
            let ci = 2.0 * y as f64 / h as f64 - 1.0;
            let mut i = 0;
            while i < 50 && tr + ti <= 4.0 {
                zi = 2.0 * zr * zi + ci;
                zr = tr - ti + cr;
                tr = zr * zr;
                ti = zi * zi;
                i += 1;
            }
            byte_acc <<= 1;
            if tr + ti <= 4.0 { byte_acc |= 1; }
            bit_num += 1;
            if bit_num == 8 {
                out.write_all(&[byte_acc]).unwrap();
                byte_acc = 0; bit_num = 0;
            } else if x == w - 1 {
                byte_acc <<= (8 - w % 8) as u32;
                out.write_all(&[byte_acc]).unwrap();
                byte_acc = 0; bit_num = 0;
            }
        }
    }
}
