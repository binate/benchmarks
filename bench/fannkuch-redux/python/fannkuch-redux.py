# fannkuch-redux — see ../c/fannkuch-redux.c for the algorithm.
import sys


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 7
    perm = [0] * n
    perm1 = list(range(n))
    count = [0] * n

    max_flips = 0
    checksum = 0
    perm_count = 0
    r = n
    while True:
        while r != 1:
            count[r - 1] = r
            r -= 1
        perm[:] = perm1
        flips = 0
        k = perm[0]
        while k != 0:
            i, j = 0, k
            while i < j:
                perm[i], perm[j] = perm[j], perm[i]
                i += 1
                j -= 1
            flips += 1
            k = perm[0]
        if flips > max_flips:
            max_flips = flips
        checksum += flips if perm_count % 2 == 0 else -flips
        while True:
            if r == n:
                sys.stdout.write("%d\nPfannkuchen(%d) = %d\n" % (checksum, n, max_flips))
                return
            perm0 = perm1[0]
            for i in range(r):
                perm1[i] = perm1[i + 1]
            perm1[r] = perm0
            count[r] -= 1
            if count[r] > 0:
                break
            r += 1
        perm_count += 1


main()
