// binary-trees — see ../c/binary-trees.c. Garbage-collected nodes.
public final class BinaryTrees {
    static final class Node {
        Node left, right;
        long item;
        Node(Node left, Node right, long item) { this.left = left; this.right = right; this.item = item; }
    }
    static Node bottomUp(long item, int depth) {
        if (depth > 0)
            return new Node(bottomUp(2 * item - 1, depth - 1), bottomUp(2 * item, depth - 1), item);
        return new Node(null, null, item);
    }
    static long itemCheck(Node t) {
        if (t.left == null) return t.item;
        return t.item + itemCheck(t.left) - itemCheck(t.right);
    }
    public static void main(String[] args) {
        int N = args.length > 0 ? Integer.parseInt(args[0]) : 10;
        int minDepth = 4;
        int maxDepth = Math.max(minDepth + 2, N);
        int stretchDepth = maxDepth + 1;

        StringBuilder sb = new StringBuilder();
        sb.append(String.format("stretch tree of depth %d\t check: %d%n", stretchDepth, itemCheck(bottomUp(0, stretchDepth))));

        Node longLived = bottomUp(0, maxDepth);
        for (int depth = minDepth; depth <= maxDepth; depth += 2) {
            long iterations = 1L << (maxDepth - depth + minDepth);
            long check = 0;
            for (long i = 1; i <= iterations; i++) {
                check += itemCheck(bottomUp(i, depth));
                check += itemCheck(bottomUp(-i, depth));
            }
            sb.append(String.format("%d\t trees of depth %d\t check: %d%n", iterations * 2, depth, check));
        }
        sb.append(String.format("long lived tree of depth %d\t check: %d%n", maxDepth, itemCheck(longLived)));
        System.out.print(sb);
    }
}
