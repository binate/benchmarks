# spectral-norm — see ../c/spectral-norm.c for the algorithm.
import sys
import math


def a(i, j):
    s = i + j
    return 1.0 / (s * (s + 1) // 2 + i + 1)


def mul_Av(v, out):
    n = len(v)
    for i in range(n):
        s = 0.0
        for j in range(n):
            s += a(i, j) * v[j]
        out[i] = s


def mul_Atv(v, out):
    n = len(v)
    for i in range(n):
        s = 0.0
        for j in range(n):
            s += a(j, i) * v[j]
        out[i] = s


def mul_AtAv(v, out, tmp):
    mul_Av(v, tmp)
    mul_Atv(tmp, out)


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 100
    u = [1.0] * n
    v = [0.0] * n
    tmp = [0.0] * n
    for _ in range(10):
        mul_AtAv(u, v, tmp)
        mul_AtAv(v, u, tmp)
    vBv = vv = 0.0
    for i in range(n):
        vBv += u[i] * v[i]
        vv += v[i] * v[i]
    print("%.9f" % math.sqrt(vBv / vv))


main()
