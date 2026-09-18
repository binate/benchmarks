// fasta — see ../c/fasta.c for the algorithm.
#include <cstdio>
#include <cstdlib>
#include <cstring>

static const int IM = 139968, IA = 3877, IC = 29573, LINE = 60;
static long seed = 42;
static double genRandom(double max) { seed = (seed * IA + IC) % IM; return max * seed / IM; }

struct AminoAcid { char sym; double prob; };

static void makeCumulative(AminoAcid *g, int n) {
    double c = 0;
    for (int i = 0; i < n; i++) { c += g[i].prob; g[i].prob = c; }
}
static char selectRandom(AminoAcid *g, int n) {
    double r = genRandom(1.0);
    for (int i = 0; i < n; i++) if (r < g[i].prob) return g[i].sym;
    return g[n - 1].sym;
}
static void repeatFasta(const char *alu, const char *title, int n) {
    int len = (int)std::strlen(alu);
    std::fputs(title, stdout);
    int pos = 0;
    while (n > 0) {
        int line = n < LINE ? n : LINE;
        for (int i = 0; i < line; i++) { std::putchar(alu[pos]); pos = (pos + 1) % len; }
        std::putchar('\n');
        n -= line;
    }
}
static void randomFasta(AminoAcid *g, int gn, const char *title, int n) {
    makeCumulative(g, gn);
    std::fputs(title, stdout);
    char buf[LINE];
    while (n > 0) {
        int line = n < LINE ? n : LINE;
        for (int i = 0; i < line; i++) buf[i] = selectRandom(g, gn);
        std::fwrite(buf, 1, line, stdout);
        std::putchar('\n');
        n -= line;
    }
}
static AminoAcid iub[] = {
    {'a',0.27},{'c',0.12},{'g',0.12},{'t',0.27},
    {'B',0.02},{'D',0.02},{'H',0.02},{'K',0.02},{'M',0.02},
    {'N',0.02},{'R',0.02},{'S',0.02},{'V',0.02},{'W',0.02},{'Y',0.02},
};
static AminoAcid homo[] = {
    {'a',0.3029549426680},{'c',0.1979883004921},{'g',0.1975473066391},{'t',0.3015094502008},
};
static const char *alu =
    "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTG"
    "GGAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGA"
    "GACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAA"
    "AATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAAT"
    "CCCAGCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAAC"
    "CCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTG"
    "CACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";
int main(int argc, char **argv) {
    int n = argc > 1 ? std::atoi(argv[1]) : 512;
    repeatFasta(alu, ">ONE Homo sapiens alu\n", n * 2);
    randomFasta(iub, 15, ">TWO IUB ambiguity codes\n", n * 3);
    randomFasta(homo, 4, ">THREE Homo sapiens frequency\n", n * 5);
    return 0;
}
