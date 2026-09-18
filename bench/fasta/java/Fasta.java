// fasta — see ../c/fasta.c for the algorithm.
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.io.OutputStream;

public final class Fasta {
    static final int IM = 139968, IA = 3877, IC = 29573, LINE = 60;
    static long seed = 42;

    static double genRandom(double max) {
        seed = (seed * IA + IC) % IM;
        return max * seed / IM;
    }
    static void makeCumulative(double[] prob) {
        double c = 0;
        for (int i = 0; i < prob.length; i++) { c += prob[i]; prob[i] = c; }
    }
    static byte selectRandom(byte[] sym, double[] cprob) {
        double r = genRandom(1.0);
        for (int i = 0; i < sym.length; i++) if (r < cprob[i]) return sym[i];
        return sym[sym.length - 1];
    }
    static void repeatFasta(OutputStream out, byte[] alu, String title, int n) throws IOException {
        out.write(title.getBytes());
        int pos = 0;
        byte[] buf = new byte[LINE];
        while (n > 0) {
            int l = Math.min(n, LINE);
            for (int i = 0; i < l; i++) { buf[i] = alu[pos]; pos = (pos + 1) % alu.length; }
            out.write(buf, 0, l);
            out.write('\n');
            n -= l;
        }
    }
    static void randomFasta(OutputStream out, byte[] sym, double[] prob, String title, int n) throws IOException {
        makeCumulative(prob);
        out.write(title.getBytes());
        byte[] buf = new byte[LINE];
        while (n > 0) {
            int l = Math.min(n, LINE);
            for (int i = 0; i < l; i++) buf[i] = selectRandom(sym, prob);
            out.write(buf, 0, l);
            out.write('\n');
            n -= l;
        }
    }
    public static void main(String[] args) throws IOException {
        int n = args.length > 0 ? Integer.parseInt(args[0]) : 512;
        byte[] iubSym = {'a','c','g','t','B','D','H','K','M','N','R','S','V','W','Y'};
        double[] iubP = {0.27,0.12,0.12,0.27,0.02,0.02,0.02,0.02,0.02,0.02,0.02,0.02,0.02,0.02,0.02};
        byte[] homoSym = {'a','c','g','t'};
        double[] homoP = {0.3029549426680,0.1979883004921,0.1975473066391,0.3015094502008};
        byte[] alu = ("GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTG"
            + "GGAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGA"
            + "GACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAA"
            + "AATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAAT"
            + "CCCAGCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAAC"
            + "CCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTG"
            + "CACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA").getBytes();
        OutputStream out = new BufferedOutputStream(System.out);
        repeatFasta(out, alu, ">ONE Homo sapiens alu\n", n * 2);
        randomFasta(out, iubSym, iubP, ">TWO IUB ambiguity codes\n", n * 3);
        randomFasta(out, homoSym, homoP, ">THREE Homo sapiens frequency\n", n * 5);
        out.flush();
    }
}
