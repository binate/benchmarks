// fannkuch-redux — see ../c/fannkuch-redux.c for the algorithm.
#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char **argv) {
    int n = argc > 1 ? std::atoi(argv[1]) : 7;
    std::vector<int> perm(n), perm1(n), count(n);
    for (int i = 0; i < n; i++) perm1[i] = i;

    long maxFlips = 0, checksum = 0, permCount = 0;
    int r = n;
    for (;;) {
        while (r != 1) { count[r - 1] = r; r--; }
        perm = perm1;
        int flips = 0;
        int k = perm[0];
        while (k != 0) {
            for (int i = 0, j = k; i < j; i++, j--) { int t = perm[i]; perm[i] = perm[j]; perm[j] = t; }
            flips++;
            k = perm[0];
        }
        if (flips > maxFlips) maxFlips = flips;
        checksum += (permCount % 2 == 0) ? flips : -flips;
        for (;;) {
            if (r == n) {
                std::printf("%ld\nPfannkuchen(%d) = %ld\n", checksum, n, maxFlips);
                return 0;
            }
            int perm0 = perm1[0];
            for (int i = 0; i < r; i++) perm1[i] = perm1[i + 1];
            perm1[r] = perm0;
            if (--count[r] > 0) break;
            r++;
        }
        permCount++;
    }
}
