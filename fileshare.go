package main

import (
	"fmt"
	"os"
	"net/http"
	"log"
	"io"
	"io/ioutil"
	"path"
	"html/template"
)

const HTML_TEMPLATE = `<!doctype html>
<html>
	<head>
		<meta charset="utf-8">
		<title>File Share</title>
	</head>
	<body>
		<section>
			<form method="post" action="/" enctype="multipart/form-data">
				<input type="file" name="fileName" accept="*"/>
				<input type="submit"/>
			</form>
		</section>
		<hr/>
		<section>
		<table>
			{{range .}}
			<tr>
				<td>
					<a href="/{{.Name}}">{{.Name}}{{if .IsDir}}/{{end}}</a>
				</td>
			</tr>
			{{end}}
		</table>
		</section>
	</body>
</html>`

var htmlTemplate *template.Template

func main() {
	var err error

	path := "."

	if len(os.Args) > 1 {
		path = os.Args[1]
	}

	htmlTemplate, err = template.New("htmlTemplate").Parse(HTML_TEMPLATE)
	if err != nil {
		log.Fatalf("Failed to parse the HTML template: %v", err)
	}

	log.Printf("Sharing directory '%v'.\n", path)

	http.HandleFunc("/", handleRequest)
	log.Fatal(http.ListenAndServe(":8800", nil))
}

func handleRequest(w http.ResponseWriter, r *http.Request) {
	switch r.Method {
	case http.MethodGet: handleGETRequest(w, r)
	case http.MethodPost: handlePOSTRequest(w, r)
	default: http.Error(w, "Method unsupported", http.StatusMethodNotAllowed)
	}
}

func handleGETRequest(w http.ResponseWriter, r *http.Request) {
	path := fmt.Sprintf("%v%v", ".", path.Clean(r.URL.Path))

	fi, err := os.Stat(path)
	if err != nil {
		http.Error(w, fmt.Sprintf("%s", err), http.StatusInternalServerError)
		return
	}

	if fi.IsDir() {
		listDirectory(w, r, path)
	} else {
		http.ServeFile(w, r, path)
	}
}

func listDirectory(w http.ResponseWriter, r *http.Request, path string) {
	fileInfos, err := ioutil.ReadDir(path)
	if err != nil {
		http.Error(w, fmt.Sprintf("Failed to list directory %v: %v", path, err), http.StatusInternalServerError)
		return
	}

	err = htmlTemplate.Execute(w, fileInfos)
	if err != nil {
		http.Error(w, fmt.Sprintf("Failed to execute HTML template: %v", err), http.StatusInternalServerError)
	}
}

func handlePOSTRequest(w http.ResponseWriter, r *http.Request) {
	r.ParseMultipartForm(128 * 1024 * 1024)

	for _, val := range r.MultipartForm.File {
		fileHeader := val[0]

		fileName := fileHeader.Filename
		if fileName == "" {
			http.Error(w, "Invalid file name", http.StatusBadRequest)
			return
		}
		path := fmt.Sprintf("%v%v", "./", path.Clean(fileName))

		uploadedFile, err := fileHeader.Open()
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to open uploaded file: %v", err), http.StatusInternalServerError)
			return
		}
		defer uploadedFile.Close()


		f, err := os.Create(path)
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to create file: %v", err), http.StatusInternalServerError)
			return
		}
		defer f.Close()

		io.Copy(f, uploadedFile)
	}

	handleGETRequest(w, r)
}
