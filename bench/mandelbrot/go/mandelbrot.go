// mandelbrot — see ../c/mandelbrot.c for the algorithm.
package main

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
)

func main() {
	n := 200
	if len(os.Args) > 1 {
		if v, err := strconv.Atoi(os.Args[1]); err == nil {
			n = v
		}
	}
	w, h := n, n
	out := bufio.NewWriter(os.Stdout)
	defer out.Flush()
	fmt.Fprintf(out, "P4\n%d %d\n", w, h)

	var byteAcc byte
	bitNum := 0
	for y := 0; y < h; y++ {
		for x := 0; x < w; x++ {
			var zr, zi, tr, ti float64
			cr := 2.0*float64(x)/float64(w) - 1.5
			ci := 2.0*float64(y)/float64(h) - 1.0
			for i := 0; i < 50 && tr+ti <= 4.0; i++ {
				zi = 2.0*zr*zi + ci
				zr = tr - ti + cr
				tr = zr * zr
				ti = zi * zi
			}
			byteAcc <<= 1
			if tr+ti <= 4.0 {
				byteAcc |= 1
			}
			bitNum++
			if bitNum == 8 {
				out.WriteByte(byteAcc)
				byteAcc = 0
				bitNum = 0
			} else if x == w-1 {
				byteAcc <<= uint(8 - w%8)
				out.WriteByte(byteAcc)
				byteAcc = 0
				bitNum = 0
			}
		}
	}
}
