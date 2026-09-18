// binary-trees — see ../c/binary-trees.c. Box per node; trees drop at scope exit.
struct Node {
    left: Option<Box<Node>>,
    right: Option<Box<Node>>,
    item: i64,
}
fn bottom_up(item: i64, depth: i32) -> Box<Node> {
    if depth > 0 {
        Box::new(Node {
            left: Some(bottom_up(2 * item - 1, depth - 1)),
            right: Some(bottom_up(2 * item, depth - 1)),
            item,
        })
    } else {
        Box::new(Node { left: None, right: None, item })
    }
}
fn item_check(t: &Node) -> i64 {
    match &t.left {
        None => t.item,
        Some(l) => t.item + item_check(l) - item_check(t.right.as_ref().unwrap()),
    }
}
fn main() {
    let n: i32 = std::env::args().nth(1).and_then(|s| s.parse().ok()).unwrap_or(10);
    let min_depth = 4;
    let max_depth = if min_depth + 2 > n { min_depth + 2 } else { n };
    let stretch_depth = max_depth + 1;

    let stretch = bottom_up(0, stretch_depth);
    println!("stretch tree of depth {}\t check: {}", stretch_depth, item_check(&stretch));
    drop(stretch);

    let long_lived = bottom_up(0, max_depth);
    let mut depth = min_depth;
    while depth <= max_depth {
        let iterations: i64 = 1i64 << (max_depth - depth + min_depth);
        let mut check: i64 = 0;
        for i in 1..=iterations {
            check += item_check(&bottom_up(i, depth));
            check += item_check(&bottom_up(-i, depth));
        }
        println!("{}\t trees of depth {}\t check: {}", iterations * 2, depth, check);
        depth += 2;
    }
    println!("long lived tree of depth {}\t check: {}", max_depth, item_check(&long_lived));
}
