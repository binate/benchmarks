// binary-trees — see ../c/binary-trees.c. Garbage-collected nodes.
package main

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
)

type node struct {
	left, right *node
	item        int64
}

func bottomUp(item int64, depth int) *node {
	if depth > 0 {
		return &node{bottomUp(2*item-1, depth-1), bottomUp(2*item, depth-1), item}
	}
	return &node{nil, nil, item}
}
func itemCheck(t *node) int64 {
	if t.left == nil {
		return t.item
	}
	return t.item + itemCheck(t.left) - itemCheck(t.right)
}
func main() {
	n := 10
	if len(os.Args) > 1 {
		if v, err := strconv.Atoi(os.Args[1]); err == nil {
			n = v
		}
	}
	out := bufio.NewWriter(os.Stdout)
	defer out.Flush()

	minDepth := 4
	maxDepth := n
	if minDepth+2 > maxDepth {
		maxDepth = minDepth + 2
	}
	stretchDepth := maxDepth + 1

	stretch := bottomUp(0, int(stretchDepth))
	fmt.Fprintf(out, "stretch tree of depth %d\t check: %d\n", stretchDepth, itemCheck(stretch))

	longLived := bottomUp(0, maxDepth)
	for depth := minDepth; depth <= maxDepth; depth += 2 {
		iterations := int64(1) << uint(maxDepth-depth+minDepth)
		var check int64
		for i := int64(1); i <= iterations; i++ {
			check += itemCheck(bottomUp(i, depth))
			check += itemCheck(bottomUp(-i, depth))
		}
		fmt.Fprintf(out, "%d\t trees of depth %d\t check: %d\n", iterations*2, depth, check)
	}
	fmt.Fprintf(out, "long lived tree of depth %d\t check: %d\n", maxDepth, itemCheck(longLived))
}
