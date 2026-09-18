// fannkuch-redux — see ../c/fannkuch-redux.c for the algorithm.
public final class FannkuchRedux {
    public static void main(String[] args) {
        int n = args.length > 0 ? Integer.parseInt(args[0]) : 7;
        int[] perm = new int[n], perm1 = new int[n], count = new int[n];
        for (int i = 0; i < n; i++) perm1[i] = i;

        long maxFlips = 0, checksum = 0, permCount = 0;
        int r = n;
        while (true) {
            while (r != 1) { count[r - 1] = r; r--; }
            System.arraycopy(perm1, 0, perm, 0, n);
            long flips = 0;
            int k = perm[0];
            while (k != 0) {
                for (int i = 0, j = k; i < j; i++, j--) { int t = perm[i]; perm[i] = perm[j]; perm[j] = t; }
                flips++;
                k = perm[0];
            }
            if (flips > maxFlips) maxFlips = flips;
            checksum += (permCount % 2 == 0) ? flips : -flips;
            while (true) {
                if (r == n) {
                    System.out.printf("%d\nPfannkuchen(%d) = %d\n", checksum, n, maxFlips);
                    return;
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
}
