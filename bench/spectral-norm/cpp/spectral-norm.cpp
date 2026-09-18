// spectral-norm — see c/spectral-norm.c for the algorithm. Same scalar,
// fixed-order computation, expressed with std::vector.
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>

static double a(int i, int j) {
    int s = i + j;
    return 1.0 / (s * (s + 1) / 2 + i + 1);
}
static void mul_Av(const std::vector<double> &v, std::vector<double> &out) {
    int n = (int)v.size();
    for (int i = 0; i < n; i++) {
        double s = 0.0;
        for (int j = 0; j < n; j++) s += a(i, j) * v[j];
        out[i] = s;
    }
}
static void mul_Atv(const std::vector<double> &v, std::vector<double> &out) {
    int n = (int)v.size();
    for (int i = 0; i < n; i++) {
        double s = 0.0;
        for (int j = 0; j < n; j++) s += a(j, i) * v[j];
        out[i] = s;
    }
}
static void mul_AtAv(const std::vector<double> &v, std::vector<double> &out, std::vector<double> &tmp) {
    mul_Av(v, tmp);
    mul_Atv(tmp, out);
}
int main(int argc, char **argv) {
    int n = argc > 1 ? std::atoi(argv[1]) : 100;
    std::vector<double> u(n, 1.0), v(n, 0.0), tmp(n, 0.0);
    for (int i = 0; i < 10; i++) { mul_AtAv(u, v, tmp); mul_AtAv(v, u, tmp); }
    double vBv = 0.0, vv = 0.0;
    for (int i = 0; i < n; i++) { vBv += u[i] * v[i]; vv += v[i] * v[i]; }
    std::printf("%.9f\n", std::sqrt(vBv / vv));
    return 0;
}
