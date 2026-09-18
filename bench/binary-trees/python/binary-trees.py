# binary-trees — see ../c/binary-trees.c. Reference-counted nodes.
import sys
sys.setrecursionlimit(100000)


class Node:
    __slots__ = ("left", "right", "item")

    def __init__(self, left, right, item):
        self.left = left
        self.right = right
        self.item = item


def bottom_up(item, depth):
    if depth > 0:
        return Node(bottom_up(2 * item - 1, depth - 1), bottom_up(2 * item, depth - 1), item)
    return Node(None, None, item)


def item_check(t):
    if t.left is None:
        return t.item
    return t.item + item_check(t.left) - item_check(t.right)


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 10
    min_depth = 4
    max_depth = max(min_depth + 2, n)
    stretch_depth = max_depth + 1

    out = []
    out.append("stretch tree of depth %d\t check: %d" % (stretch_depth, item_check(bottom_up(0, stretch_depth))))

    long_lived = bottom_up(0, max_depth)
    depth = min_depth
    while depth <= max_depth:
        iterations = 1 << (max_depth - depth + min_depth)
        check = 0
        for i in range(1, iterations + 1):
            check += item_check(bottom_up(i, depth))
            check += item_check(bottom_up(-i, depth))
        out.append("%d\t trees of depth %d\t check: %d" % (iterations * 2, depth, check))
        depth += 2
    out.append("long lived tree of depth %d\t check: %d" % (max_depth, item_check(long_lived)))
    sys.stdout.write("\n".join(out) + "\n")


main()
