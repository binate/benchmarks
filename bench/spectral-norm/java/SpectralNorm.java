// spectral-norm — see ../c/spectral-norm.c for the algorithm.
public final class SpectralNorm {
    static double a(int i, int j) {
        int s = i + j;
        return 1.0 / (s * (s + 1) / 2 + i + 1);
    }
    static void mulAv(double[] v, double[] out) {
        for (int i = 0; i < v.length; i++) {
            double s = 0.0;
            for (int j = 0; j < v.length; j++) s += a(i, j) * v[j];
            out[i] = s;
        }
    }
    static void mulAtv(double[] v, double[] out) {
        for (int i = 0; i < v.length; i++) {
            double s = 0.0;
            for (int j = 0; j < v.length; j++) s += a(j, i) * v[j];
            out[i] = s;
        }
    }
    static void mulAtAv(double[] v, double[] out, double[] tmp) {
        mulAv(v, tmp);
        mulAtv(tmp, out);
    }
    public static void main(String[] args) {
        int n = args.length > 0 ? Integer.parseInt(args[0]) : 100;
        double[] u = new double[n], v = new double[n], tmp = new double[n];
        java.util.Arrays.fill(u, 1.0);
        for (int i = 0; i < 10; i++) { mulAtAv(u, v, tmp); mulAtAv(v, u, tmp); }
        double vBv = 0.0, vv = 0.0;
        for (int i = 0; i < n; i++) { vBv += u[i] * v[i]; vv += v[i] * v[i]; }
        System.out.printf("%.9f\n", Math.sqrt(vBv / vv));
    }
}
