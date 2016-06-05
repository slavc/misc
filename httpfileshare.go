package main

import (
	"fmt"
	"os"
	"net/http"
	"log"
	"io"
	"path"
)

func main() {
	path := "."

	if len(os.Args) > 1 {
		path = os.Args[1]
	}

	log.Printf("Sharing directory '%v'.\n", path)

	http.HandleFunc("/", handleRequest)
	log.Fatal(http.ListenAndServe(":8000", nil))
}

func handleRequest(w http.ResponseWriter, r *http.Request) {
	// FIXME
	path := fmt.Sprintf("%v%v", ".", path.Clean(r.URL.Path))
	f, err := os.Open(path)
	if err != nil {
		fmt.Fprintf(w, "Error: %v.", err)
		return
	}
	defer f.Close()

	log.Printf("Serving file '%v'.\n", path)
	io.Copy(w, f)
}

