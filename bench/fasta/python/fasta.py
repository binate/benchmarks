# fasta — see ../c/fasta.c for the algorithm.
import sys

IM = 139968
IA = 3877
IC = 29573
LINE = 60

seed = 42


def gen_random(max_):
    global seed
    seed = (seed * IA + IC) % IM
    return max_ * seed / IM


def make_cumulative(g):
    c = 0.0
    out = []
    for sym, prob in g:
        c += prob
        out.append((sym, c))
    return out


def select_random(g):
    r = gen_random(1.0)
    for sym, cprob in g:
        if r < cprob:
            return sym
    return g[-1][0]


def repeat_fasta(out, alu, title, n):
    out.append(title)
    pos = 0
    ln = len(alu)
    while n > 0:
        line = n if n < LINE else LINE
        chunk = []
        for _ in range(line):
            chunk.append(alu[pos])
            pos = (pos + 1) % ln
        out.append("".join(chunk) + "\n")
        n -= line


def random_fasta(out, g, title, n):
    g = make_cumulative(g)
    out.append(title)
    while n > 0:
        line = n if n < LINE else LINE
        out.append("".join(select_random(g) for _ in range(line)) + "\n")
        n -= line


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 512
    iub = [('a', 0.27), ('c', 0.12), ('g', 0.12), ('t', 0.27),
           ('B', 0.02), ('D', 0.02), ('H', 0.02), ('K', 0.02), ('M', 0.02),
           ('N', 0.02), ('R', 0.02), ('S', 0.02), ('V', 0.02), ('W', 0.02), ('Y', 0.02)]
    homo = [('a', 0.3029549426680), ('c', 0.1979883004921),
            ('g', 0.1975473066391), ('t', 0.3015094502008)]
    alu = ("GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTG"
           "GGAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGA"
           "GACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAA"
           "AATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAAT"
           "CCCAGCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAAC"
           "CCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTG"
           "CACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA")
    out = []
    repeat_fasta(out, alu, ">ONE Homo sapiens alu\n", n * 2)
    random_fasta(out, iub, ">TWO IUB ambiguity codes\n", n * 3)
    random_fasta(out, homo, ">THREE Homo sapiens frequency\n", n * 5)
    sys.stdout.write("".join(out))


main()
