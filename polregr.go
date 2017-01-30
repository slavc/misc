/*
polregr performs a polynomial regression on the numbers fed in via stdin.

Math code lifted from https://rosettacode.org/wiki/Polynomial_regression#Library_gonum.2Fmatrix
*/
package main

import (
	"fmt"
	"flag"
	"os"
	"bufio"
	"strconv"
	"github.com/gonum/matrix/mat64"
)

var Degree = flag.Int("degree", 5, "desired degree of the resultant polynomial")
var PlainOutput = flag.Bool("plain", false, "output coefficients one-per-line instead of a polynomial")
var FloatPrecision = flag.Int("precision", 6, "number of decimal places to output")

func main() {
	flag.Parse()

	x, y := ReadNumbers()

	a := Vandermonde(x, *Degree)
	b := mat64.NewDense(len(y), 1, y)
	c := mat64.NewDense(*Degree+1, 1, nil)

	qr := new(mat64.QR)
	qr.Factorize(a)

	err := c.SolveQR(qr, false, b)
	if err != nil {
		fmt.Println(err)
	} else {
		Print(c)
	}
}

func ReadNumbers() (x []float64, y []float64) {
	scanner := bufio.NewScanner(os.Stdin)
	scanner.Split(bufio.ScanWords)
	i := 0
	for scanner.Scan() {
		f, err := strconv.ParseFloat(scanner.Text(), 64)
		if err != nil {
			return
		}
		if i % 2 == 0 {
			x = append(x, f)
		} else {
			y = append(y, f)
		}
		i++
	}
	return
}

func Vandermonde(a []float64, degree int) *mat64.Dense {
	x := mat64.NewDense(len(a), degree+1, nil)
	for i := range a {
		for j, p := 0, 1.; j <= degree; j, p = j+1, p*a[i] {
			x.Set(i, j, p)
		}
	}
	return x
}

func FloatToString(f float64) string {
	fmtstr := fmt.Sprintf("%%.%vf", *FloatPrecision)
	return fmt.Sprintf(fmtstr, f)
}


func Print(c *mat64.Dense) {
	indices := []string{"⁹", "⁸", "⁷", "⁶", "⁵", "⁴", "³", "²", "", ""}
	nrows, _ := c.Dims()
	if *PlainOutput {
		for row := 0; row < nrows; row++ {
			coeff := c.At(row, 0)
			fmt.Println(FloatToString(coeff))
		}
	} else if nrows > len(indices) {
		for row := 0; row < nrows; row++ {
			coeff := c.At(row, 0)
			if row > 0 {
				if coeff > 0.0 {
					fmt.Printf(" + ")
				} else {
					coeff = -coeff
					fmt.Printf(" - ")
				}
			}
			if row == nrows - 1 {
				fmt.Printf("%v", FloatToString(coeff))
			} else if row == nrows - 2 {
				fmt.Printf("%vx", FloatToString(coeff))
			} else {
				fmt.Printf("%vx^%v", FloatToString(coeff), nrows - row)
			}
		}
		fmt.Printf("\n")
	} else {
		indices = indices[len(indices)-nrows:]
		for row := 0; row < nrows; row++ {
			coeff := c.At(row, 0)
			if row > 0 {
				if coeff > 0.0 {
					fmt.Printf(" + ")
				} else {
					coeff = -coeff
					fmt.Printf(" - ")
				}
			}
			fmtstr := "%vx%v"
			if row == nrows - 1 {
				fmtstr = "%v%v"
			}
			fmt.Printf(fmtstr, FloatToString(coeff), indices[row])
		}
		fmt.Printf("\n")
	}
}
