/* fannkuch-redux: for every permutation of 1..n, count the "pancake flips"
 * (prefix reversals) needed to bring the first element to 1; report the maximum
 * flip count and an alternating-sign checksum over all permutations. Integer
 * work, permutation generation, tight array indexing. Output is deterministic;
 * all implementations must match byte-for-byte. */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 7;
    int *perm = malloc(n * sizeof(int));
    int *perm1 = malloc(n * sizeof(int));
    int *count = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) perm1[i] = i;

    long maxFlips = 0, checksum = 0;
    long permCount = 0;
    int r = n;
    for (;;) {
        while (r != 1) { count[r - 1] = r; r--; }

        for (int i = 0; i < n; i++) perm[i] = perm1[i];
        int flips = 0;
        int k = perm[0];
        while (k != 0) {
            for (int i = 0, j = k; i < j; i++, j--) {
                int t = perm[i]; perm[i] = perm[j]; perm[j] = t;
            }
            flips++;
            k = perm[0];
        }
        if (flips > maxFlips) maxFlips = flips;
        checksum += (permCount % 2 == 0) ? flips : -flips;

        for (;;) {
            if (r == n) {
                printf("%ld\nPfannkuchen(%d) = %ld\n", checksum, n, maxFlips);
                free(perm); free(perm1); free(count);
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
