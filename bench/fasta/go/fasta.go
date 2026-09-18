// fasta — see ../c/fasta.c for the algorithm.
package main

import (
	"bufio"
	"os"
	"strconv"
)

const (
	im   = 139968
	ia   = 3877
	ic   = 29573
	line = 60
)

var seed = 42

func genRandom(max float64) float64 {
	seed = (seed*ia + ic) % im
	return max * float64(seed) / float64(im)
}

type aminoAcid struct {
	sym  byte
	prob float64
}

func makeCumulative(g []aminoAcid) {
	c := 0.0
	for i := range g {
		c += g[i].prob
		g[i].prob = c
	}
}
func selectRandom(g []aminoAcid) byte {
	r := genRandom(1.0)
	for i := range g {
		if r < g[i].prob {
			return g[i].sym
		}
	}
	return g[len(g)-1].sym
}
func repeatFasta(out *bufio.Writer, alu, title string, n int) {
	out.WriteString(title)
	pos := 0
	for n > 0 {
		l := line
		if n < l {
			l = n
		}
		for i := 0; i < l; i++ {
			out.WriteByte(alu[pos])
			pos = (pos + 1) % len(alu)
		}
		out.WriteByte('\n')
		n -= l
	}
}
func randomFasta(out *bufio.Writer, g []aminoAcid, title string, n int) {
	makeCumulative(g)
	out.WriteString(title)
	buf := make([]byte, line)
	for n > 0 {
		l := line
		if n < l {
			l = n
		}
		for i := 0; i < l; i++ {
			buf[i] = selectRandom(g)
		}
		out.Write(buf[:l])
		out.WriteByte('\n')
		n -= l
	}
}

func main() {
	n := 512
	if len(os.Args) > 1 {
		if v, err := strconv.Atoi(os.Args[1]); err == nil {
			n = v
		}
	}
	iub := []aminoAcid{
		{'a', 0.27}, {'c', 0.12}, {'g', 0.12}, {'t', 0.27},
		{'B', 0.02}, {'D', 0.02}, {'H', 0.02}, {'K', 0.02}, {'M', 0.02},
		{'N', 0.02}, {'R', 0.02}, {'S', 0.02}, {'V', 0.02}, {'W', 0.02}, {'Y', 0.02},
	}
	homo := []aminoAcid{
		{'a', 0.3029549426680}, {'c', 0.1979883004921},
		{'g', 0.1975473066391}, {'t', 0.3015094502008},
	}
	alu := "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTG" +
		"GGAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGA" +
		"GACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAA" +
		"AATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAAT" +
		"CCCAGCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAAC" +
		"CCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTG" +
		"CACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA"
	out := bufio.NewWriter(os.Stdout)
	defer out.Flush()
	repeatFasta(out, alu, ">ONE Homo sapiens alu\n", n*2)
	randomFasta(out, iub, ">TWO IUB ambiguity codes\n", n*3)
	randomFasta(out, homo, ">THREE Homo sapiens frequency\n", n*5)
}
