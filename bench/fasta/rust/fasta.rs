// fasta — see ../c/fasta.c for the algorithm.
use std::io::{self, Write};

const IM: i64 = 139968;
const IA: i64 = 3877;
const IC: i64 = 29573;
const LINE: usize = 60;

struct Rng { seed: i64 }
impl Rng {
    fn next(&mut self, max: f64) -> f64 {
        self.seed = (self.seed * IA + IC) % IM;
        max * self.seed as f64 / IM as f64
    }
}

fn make_cumulative(g: &mut [(u8, f64)]) {
    let mut c = 0.0;
    for e in g.iter_mut() { c += e.1; e.1 = c; }
}
fn select_random(g: &[(u8, f64)], rng: &mut Rng) -> u8 {
    let r = rng.next(1.0);
    for &(sym, cp) in g {
        if r < cp { return sym; }
    }
    g[g.len() - 1].0
}
fn repeat_fasta<W: Write>(out: &mut W, alu: &[u8], title: &str, mut n: i32) {
    out.write_all(title.as_bytes()).unwrap();
    let mut pos = 0usize;
    let mut buf = vec![0u8; LINE];
    while n > 0 {
        let l = if (n as usize) < LINE { n as usize } else { LINE };
        for b in buf.iter_mut().take(l) {
            *b = alu[pos];
            pos = (pos + 1) % alu.len();
        }
        out.write_all(&buf[..l]).unwrap();
        out.write_all(b"\n").unwrap();
        n -= l as i32;
    }
}
fn random_fasta<W: Write>(out: &mut W, g: &mut [(u8, f64)], title: &str, mut n: i32, rng: &mut Rng) {
    make_cumulative(g);
    out.write_all(title.as_bytes()).unwrap();
    let mut buf = vec![0u8; LINE];
    while n > 0 {
        let l = if (n as usize) < LINE { n as usize } else { LINE };
        for b in buf.iter_mut().take(l) { *b = select_random(g, rng); }
        out.write_all(&buf[..l]).unwrap();
        out.write_all(b"\n").unwrap();
        n -= l as i32;
    }
}
fn main() {
    let n: i32 = std::env::args().nth(1).and_then(|s| s.parse().ok()).unwrap_or(512);
    let mut iub: Vec<(u8, f64)> = vec![
        (b'a', 0.27), (b'c', 0.12), (b'g', 0.12), (b't', 0.27),
        (b'B', 0.02), (b'D', 0.02), (b'H', 0.02), (b'K', 0.02), (b'M', 0.02),
        (b'N', 0.02), (b'R', 0.02), (b'S', 0.02), (b'V', 0.02), (b'W', 0.02), (b'Y', 0.02),
    ];
    let mut homo: Vec<(u8, f64)> = vec![
        (b'a', 0.3029549426680), (b'c', 0.1979883004921),
        (b'g', 0.1975473066391), (b't', 0.3015094502008),
    ];
    let alu = b"GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTG\
GGAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGA\
GACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAA\
AATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAAT\
CCCAGCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAAC\
CCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTG\
CACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";
    let mut rng = Rng { seed: 42 };
    let stdout = io::stdout();
    let mut out = io::BufWriter::new(stdout.lock());
    repeat_fasta(&mut out, alu, ">ONE Homo sapiens alu\n", n * 2);
    random_fasta(&mut out, &mut iub, ">TWO IUB ambiguity codes\n", n * 3, &mut rng);
    random_fasta(&mut out, &mut homo, ">THREE Homo sapiens frequency\n", n * 5, &mut rng);
}
