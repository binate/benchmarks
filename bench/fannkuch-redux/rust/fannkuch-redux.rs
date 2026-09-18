// fannkuch-redux — see ../c/fannkuch-redux.c for the algorithm.
fn main() {
    let n: usize = std::env::args().nth(1).and_then(|s| s.parse().ok()).unwrap_or(7);
    let mut perm = vec![0i32; n];
    let mut perm1: Vec<i32> = (0..n as i32).collect();
    let mut count = vec![0usize; n];

    let mut max_flips: i64 = 0;
    let mut checksum: i64 = 0;
    let mut perm_count: i64 = 0;
    let mut r = n;
    loop {
        while r != 1 { count[r - 1] = r; r -= 1; }
        perm.copy_from_slice(&perm1);
        let mut flips: i64 = 0;
        let mut k = perm[0];
        while k != 0 {
            let (mut i, mut j) = (0usize, k as usize);
            while i < j { perm.swap(i, j); i += 1; j -= 1; }
            flips += 1;
            k = perm[0];
        }
        if flips > max_flips { max_flips = flips; }
        checksum += if perm_count % 2 == 0 { flips } else { -flips };
        loop {
            if r == n {
                println!("{}\nPfannkuchen({}) = {}", checksum, n, max_flips);
                return;
            }
            let perm0 = perm1[0];
            for i in 0..r { perm1[i] = perm1[i + 1]; }
            perm1[r] = perm0;
            count[r] -= 1;
            if count[r] > 0 { break; }
            r += 1;
        }
        perm_count += 1;
    }
}
