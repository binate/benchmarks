// binary-trees — see ../c/binary-trees.c. Plain per-node new/delete.
#include <cstdio>
#include <cstdlib>

struct Node { Node *left, *right; long item; };

static Node *newNode(Node *l, Node *r, long item) {
    Node *n = new Node;
    n->left = l; n->right = r; n->item = item;
    return n;
}
static long itemCheck(Node *t) {
    if (t->left == nullptr) return t->item;
    return t->item + itemCheck(t->left) - itemCheck(t->right);
}
static Node *bottomUp(long item, int depth) {
    if (depth > 0)
        return newNode(bottomUp(2 * item - 1, depth - 1), bottomUp(2 * item, depth - 1), item);
    return newNode(nullptr, nullptr, item);
}
static void deleteTree(Node *t) {
    if (t->left != nullptr) { deleteTree(t->left); deleteTree(t->right); }
    delete t;
}
int main(int argc, char **argv) {
    int N = argc > 1 ? std::atoi(argv[1]) : 10;
    int minDepth = 4;
    int maxDepth = (minDepth + 2 > N) ? minDepth + 2 : N;
    int stretchDepth = maxDepth + 1;

    Node *stretch = bottomUp(0, stretchDepth);
    std::printf("stretch tree of depth %d\t check: %ld\n", stretchDepth, itemCheck(stretch));
    deleteTree(stretch);

    Node *longLived = bottomUp(0, maxDepth);
    for (int depth = minDepth; depth <= maxDepth; depth += 2) {
        long iterations = 1L << (maxDepth - depth + minDepth);
        long check = 0;
        for (long i = 1; i <= iterations; i++) {
            Node *t1 = bottomUp(i, depth);
            check += itemCheck(t1);
            deleteTree(t1);
            Node *t2 = bottomUp(-i, depth);
            check += itemCheck(t2);
            deleteTree(t2);
        }
        std::printf("%ld\t trees of depth %d\t check: %ld\n", iterations * 2, depth, check);
    }
    std::printf("long lived tree of depth %d\t check: %ld\n", maxDepth, itemCheck(longLived));
    deleteTree(longLived);
    return 0;
}
