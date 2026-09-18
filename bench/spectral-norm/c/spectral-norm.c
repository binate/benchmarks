/* spectral-norm: approximate the largest singular value of the infinite matrix
 * A(i,j) = 1/((i+j)(i+j+1)/2 + i + 1) by 10 rounds of the power method on A^T·A.
 * Single-threaded, scalar, fixed operation order — the reference all other
 * implementations must agree with. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static double a(int i, int j) {
    int s = i + j;
    return 1.0 / (s * (s + 1) / 2 + i + 1);
}
static void mul_Av(int n, const double *v, double *out) {
    for (int i = 0; i < n; i++) {
        double s = 0.0;
        for (int j = 0; j < n; j++) s += a(i, j) * v[j];
        out[i] = s;
    }
}
static void mul_Atv(int n, const double *v, double *out) {
    for (int i = 0; i < n; i++) {
        double s = 0.0;
        for (int j = 0; j < n; j++) s += a(j, i) * v[j];
        out[i] = s;
    }
}
static void mul_AtAv(int n, const double *v, double *out, double *tmp) {
    mul_Av(n, v, tmp);
    mul_Atv(n, tmp, out);
}
int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 100;
    double *u = malloc(n * sizeof *u), *v = malloc(n * sizeof *v), *tmp = malloc(n * sizeof *tmp);
    for (int i = 0; i < n; i++) u[i] = 1.0;
    for (int i = 0; i < 10; i++) { mul_AtAv(n, u, v, tmp); mul_AtAv(n, v, u, tmp); }
    double vBv = 0.0, vv = 0.0;
    for (int i = 0; i < n; i++) { vBv += u[i] * v[i]; vv += v[i] * v[i]; }
    printf("%.9f\n", sqrt(vBv / vv));
    free(u); free(v); free(tmp);
    return 0;
}
