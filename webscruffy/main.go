package main

import (
	"bytes"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"os/exec"
	"strings"
	"sync"
	"time"
)

const (
	DEFAULT_HTTP_ADDRESS = ":8000"
	ROOT_PREFIX = "/suml"
	DEFAULT_CACHE_SIZE = 250
	STATIC_DIR_PATH = "./static"
	CACHE_DIR_PATH = "./cache"
	CACHE_INDEX_FILENAME = "index"
)

const (
	CLASS uint = iota
	SEQUENCE
)

var cache *Cache

var httpAddress *string
var cacheSize *uint

func main() {
	log.SetOutput(os.Stdout)

	parseArguments()

	cache = NewCache(*cacheSize)
	cache.Load(CACHE_DIR_PATH + "/" + CACHE_INDEX_FILENAME)

	http.HandleFunc(ROOT_PREFIX, handleRootRequest)
	http.HandleFunc(ROOT_PREFIX + "/", handleRootRequest)
	http.HandleFunc(ROOT_PREFIX + "/cache", handleCacheStatsRequest)
	http.HandleFunc(ROOT_PREFIX + "/cache/", handleCacheStatsRequest)
	http.HandleFunc(ROOT_PREFIX + "/class/", handleClassRequest)
	http.HandleFunc(ROOT_PREFIX + "/seq/", handleSequenceRequest)
	http.Handle(ROOT_PREFIX + "/static/", http.StripPrefix(ROOT_PREFIX + "/static/", http.FileServer(http.Dir(STATIC_DIR_PATH))))
	http.ListenAndServe(*httpAddress, nil)
}

func parseArguments() {
	httpAddress = flag.String("http", DEFAULT_HTTP_ADDRESS, "Address to listen on for HTTP connections ('" + DEFAULT_HTTP_ADDRESS + "' by default).")
	cacheSize = flag.Uint("cachesize", DEFAULT_CACHE_SIZE, fmt.Sprintf("Maximum number of images to store in the on-disk cache (%v by default).", DEFAULT_CACHE_SIZE))
}

func handleRootRequest(w http.ResponseWriter, r *http.Request) {
	http.Redirect(w, r, ROOT_PREFIX + "/static/index.html", http.StatusTemporaryRedirect)
}

func handleCacheStatsRequest(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/plain")
	io.WriteString(w, "Cache stats:\n")
	cache.mutex.RLock()
	defer cache.mutex.RUnlock()
	io.WriteString(w, fmt.Sprintf("%v hits / %v misses\n", cache.hitCount, cache.missCount))
	io.WriteString(w, "Cache records:\n")
	for i, r := range cache.records {
		io.WriteString(w, fmt.Sprintf("%04d: DiagramType=%v Created=%v LastAccess=%v AccessCount=%v Rank=%v\n", i, r.DiagramType, r.Created, r.LastAccess, r.AccessCount, r.Rank()))
	}
}

func handleClassRequest(w http.ResponseWriter, r *http.Request) {
	path := r.URL.Path
	prefix := ROOT_PREFIX + "/class/"
	postfix := ".png"
	if len(path) > len(prefix) + len(postfix) {
		expression := path[len(prefix):len(path)-len(postfix)]
		handleDiagramRequest(w, r, CLASS, expression)
	} else {
		http.Error(w, "Bad request.", http.StatusBadRequest)
	}
}

func handleSequenceRequest(w http.ResponseWriter, r *http.Request) {
	path := r.URL.Path
	prefix := ROOT_PREFIX + "/seq/"
	postfix := ".png"
	if len(path) > len(prefix) + len(postfix) {
		expression := path[len(prefix):len(path)-len(postfix)]
		handleDiagramRequest(w, r, SEQUENCE, expression)
	} else {
		http.Error(w, "Bad request.", http.StatusBadRequest)
	}
}

func handleDiagramRequest(w http.ResponseWriter, r *http.Request, diagramType uint, expr string) {
	imageData, err := cache.Fetch(diagramType, expr)
	if err != nil {
		log.Printf("error: %v\n", err)
		w.WriteHeader(http.StatusInternalServerError)
		io.WriteString(w, "An error occured while handling the request.")
		return
	}
	w.Header().Set("Cache-Control", "max-age=2678400")
	io.Copy(w, bytes.NewReader(imageData))
}

type Cache struct {
	records []CacheRecord
	mutex sync.RWMutex
	hitCount uint
	missCount uint
}

func NewCache(size uint) *Cache {
	c := new(Cache)
	c.records = make([]CacheRecord, 0, size)
	return c
}

func (cache *Cache) Size() int {
	cache.mutex.RLock()
	defer cache.mutex.RUnlock()
	return len(cache.records)
}

func (cache *Cache) Capacity() int {
	cache.mutex.RLock()
	defer cache.mutex.RUnlock()
	return cap(cache.records)
}

func (cache *Cache) Load(path string) error {
	cache.mutex.Lock()
	defer cache.mutex.Unlock()

	buf, err := ioutil.ReadFile(path)
	if err != nil {
		return err
	}

	err = json.Unmarshal(buf, &cache.records)
	if err != nil {
		return err
	}

	return nil
}

func (cache *Cache) Store(path string) error {
	cache.mutex.RLock()
	defer cache.mutex.RUnlock()

	buf, err := json.Marshal(cache.records)
	if err != nil {
		return err
	}

	return ioutil.WriteFile(path, buf, 0600)
}

func (cache *Cache) Fetch(diagramType uint, expression string) ([]byte, error) {
	var imageData []byte
	var err error

	cache.mutex.RLock()
	foundIndex := len(cache.records)
	for i, r := range cache.records {
		if r.DiagramType == diagramType && r.Expression == expression {
			path := fmt.Sprintf("%v/%v", CACHE_DIR_PATH, i)
			log.Printf("Found %v in cache, loading from %v...\n", r, path)
			imageData, err = ioutil.ReadFile(path)
			if err != nil {
				log.Printf("Error: failed to load image data from %v: %v.\n", path, err)
			} else {
				cache.hitCount++
				foundIndex = i
			}
			break
		}
	}
	cache.mutex.RUnlock()

	if imageData == nil {
		cache.missCount++
		log.Printf("Generating a new image...\n")
		imageData, err = generateDiagram(diagramType, expression)
	}
	if err != nil {
		return nil, err
	}

	go cache.Put(diagramType, expression, imageData, foundIndex)

	return imageData, nil
}

func (cache *Cache) Put(diagramType uint, expression string, imageData []byte, index int) (err error) {
	cache.mutex.Lock()
	defer cache.Store(fmt.Sprintf("%v/%v", CACHE_DIR_PATH, CACHE_INDEX_FILENAME))
	defer cache.mutex.Unlock()

	if index < len(cache.records) { // a record was found in cache, update it's stats
		log.Printf("Updating an existing record %v, at index %v...\n", cache.records[index], index)
		rp := &(cache.records[index])
		rp.LastAccess = time.Now()
		rp.AccessCount++

		return nil
	}

	r := CacheRecord{diagramType, expression, time.Now(), time.Now(), 1}
	

	var path string

	if len(cache.records) == 0 || len(cache.records) < cap(cache.records) {
		log.Printf("Appending a record %v to cache...\n", r)
		index = len(cache.records)
		cache.records = append(cache.records, r)
	} else {
		index = 0
		lowestRank := float32(-1.0)
		for i, r := range cache.records {
			rank := r.Rank()
			if rank < lowestRank || index == -1 {
				index = i
				lowestRank = rank
			}
			if lowestRank == 0.0 {
				break
			}
		}
		log.Printf("Replacing record %v at index %v with record %v...\n", cache.records[index], index, r)
		cache.records[index] = r
	}
	path = fmt.Sprintf("%v/%v", CACHE_DIR_PATH, index)
	err = ioutil.WriteFile(path, imageData, 0600)
	
	return err
}

func generateDiagram(diagramType uint, expression string) ([]byte, error) {
	args := make([]string, 0)
	args = append(args, "--png")
	args = append(args, "--scruffy")
	if diagramType == SEQUENCE {
		args = append(args, "--sequence")
	}	

	buffer := bytes.Buffer{}

	cmd := exec.Command("suml", args...)
	cmd.Stdin = strings.NewReader(expression)
	cmd.Stdout = &buffer
	err := cmd.Run()
	if err != nil {
		return nil, err
	}
	return buffer.Bytes(), nil
}

type CacheRecord struct {
	DiagramType uint
	Expression string
	Created time.Time
	LastAccess time.Time
	AccessCount uint
}

func (r CacheRecord) String() string {
	return fmt.Sprintf("{%v, %v, %v, %v, %v}", r.DiagramType, ellipsis(r.Expression, 10), r.Created, r.LastAccess, r.AccessCount)
}

func (r CacheRecord) Rank() float32 {
	const timeRange = 20.0 // in days
	const secondsInTimeRange float32 = timeRange * 24.0 * 60.0 * 60.0
	
	var factor float32

	secondsSinceLastAccess := float32(time.Since(r.LastAccess).Seconds())
	if secondsSinceLastAccess > secondsInTimeRange {
		factor = 0.0
	} else {
		factor = ((secondsInTimeRange - secondsSinceLastAccess) / secondsInTimeRange) * 10.0
	}

	return float32(r.AccessCount) * factor
}

func ellipsis(s string, l int) string {
	if len(s) > l {
		return s[:l] + "..."
	} else {
		return s
	}
}
