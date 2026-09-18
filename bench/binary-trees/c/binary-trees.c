/* binary-trees: allocate, walk, and free many binary trees — a memory-management
 * (allocation) benchmark. Single-threaded, plain per-node allocation (no arena),
 * so the languages differ in how they manage memory: malloc/free here, GC in
 * Go/Java, Box in Rust, reference counting in Binate/Python. The check values
 * are deterministic integers; all implementations must match byte-for-byte. */
#include <stdio.h>
#include <stdlib.h>

typedef struct tn { struct tn *left, *right; long item; } treeNode;

static treeNode *newNode(treeNode *l, treeNode *r, long item) {
    treeNode *n = malloc(sizeof(treeNode));
    n->left = l; n->right = r; n->item = item;
    return n;
}
static long itemCheck(treeNode *t) {
    if (t->left == NULL) return t->item;
    return t->item + itemCheck(t->left) - itemCheck(t->right);
}
static treeNode *bottomUp(long item, int depth) {
    if (depth > 0)
        return newNode(bottomUp(2 * item - 1, depth - 1), bottomUp(2 * item, depth - 1), item);
    return newNode(NULL, NULL, item);
}
static void deleteTree(treeNode *t) {
    if (t->left != NULL) { deleteTree(t->left); deleteTree(t->right); }
    free(t);
}
int main(int argc, char **argv) {
    int N = argc > 1 ? atoi(argv[1]) : 10;
    int minDepth = 4;
    int maxDepth = (minDepth + 2 > N) ? minDepth + 2 : N;
    int stretchDepth = maxDepth + 1;

    treeNode *stretch = bottomUp(0, stretchDepth);
    printf("stretch tree of depth %d\t check: %ld\n", stretchDepth, itemCheck(stretch));
    deleteTree(stretch);

    treeNode *longLived = bottomUp(0, maxDepth);

    for (int depth = minDepth; depth <= maxDepth; depth += 2) {
        long iterations = 1L << (maxDepth - depth + minDepth);
        long check = 0;
        for (long i = 1; i <= iterations; i++) {
            treeNode *t1 = bottomUp(i, depth);
            check += itemCheck(t1);
            deleteTree(t1);
            treeNode *t2 = bottomUp(-i, depth);
            check += itemCheck(t2);
            deleteTree(t2);
        }
        printf("%ld\t trees of depth %d\t check: %ld\n", iterations * 2, depth, check);
    }
    printf("long lived tree of depth %d\t check: %ld\n", maxDepth, itemCheck(longLived));
    deleteTree(longLived);
    return 0;
}
