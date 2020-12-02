package main

import (
	"encoding/binary"
	"log"
	"net/http"
	"os"
	"sort"
	"sync"
	"strconv"
	"fmt"
	"time"

	"github.com/korandiz/v4l"
)

func main() {
	log.SetFlags(log.Ldate | log.Ltime | log.Lmicroseconds | log.Lshortfile)

	if len(os.Args) < 2 {
		return
	}

	d, err := v4l.Open(os.Args[1])
	if err != nil {
		log.Fatalf("failed to open %s: %v", os.Args[1], err)
	}
	defer d.TurnOff()
	defer d.Close()

	cfgs, err := d.ListConfigs()
	if err != nil {
		log.Fatalf("failed to list configs: %v", err)
	}
	if len(cfgs) == 0 {
		log.Fatalf("config list is empty")
	}

	sort.Sort(DeviceConfigs(cfgs))

	//log.Printf("configs: %+v", cfgs)

	cfg := cfgs[0]

	var format [4]byte
	binary.LittleEndian.PutUint32(format[:], cfg.Format)

	if err = d.SetConfig(cfg); err != nil {
		log.Fatalf("failed to set config: %v", err)
	}

	if err = d.TurnOn(); err != nil {
		log.Fatalf("failed to turn the camera on: %v", err)
	}

	c, err := d.GetConfig()
	if err != nil {
		log.Fatalf("failed to get config: %v", err)
	}
	log.Printf("config: %+v", c)
	log.Printf("format: %s", string(format[:]))

	go readFrames(d)

	http.HandleFunc("/webcam/frames", serveFrames)
	http.HandleFunc("/webcam/stream", serveStream)
	err = http.ListenAndServe(":8000", nil)
	if err != nil {
		log.Printf("HTTP server failed: %v", err)
	}
}

type DeviceConfigs []v4l.DeviceConfig

func (cfgs DeviceConfigs) Len() int {
	return len(cfgs)
}

func (cfgs DeviceConfigs) Swap(i, j int) {
	cfgs[i], cfgs[j] = cfgs[j], cfgs[i]
}

func (cfgs DeviceConfigs) Less(i, j int) bool {
	resA := cfgs[i].Width * cfgs[i].Height
	resB := cfgs[j].Width * cfgs[j].Height
	if resA == resB {
		return cfgs[i].FPS.Cmp(cfgs[j].FPS) == 1
	} else {
		return resA > resB
	}
}

var frameBuffer []byte
var frameBufferMutex sync.RWMutex

func readFrames(d *v4l.Device) {
	for {
		frameBufferMutex.Lock()

		frame, err := d.Capture()
		if err != nil {
			log.Fatalf("failed to capture frame: %v", err)
		}

		frameBuffer = make([]byte, frame.Len())
		_, err = frame.Read(frameBuffer)
		if err != nil {
			log.Fatalf("failed to read frame buffer: %v", err)
		}

		frameBufferMutex.Unlock()
	}
}

func serveFrames(w http.ResponseWriter, r *http.Request) {
	w.Header().Add("Refresh", "1")
	w.Header().Add("Content-Type", "image/jpeg")
	frameBufferMutex.RLock()
	buf := make([]byte, len(frameBuffer))
	copy(buf, frameBuffer)
	frameBufferMutex.RUnlock()
	n, err := w.Write(buf)
	if err != nil {
		log.Printf("failed to write frame to HTTP client: %v", err)
	}
	if n != len(buf) {
		log.Printf("failed to write frame to HTTP client: short write")
	}
}

func serveStream(w http.ResponseWriter, r *http.Request) {
	log.Printf("client connected")
	defer log.Printf("client disconnected")

	r.ParseForm()
	l := r.Form["fps"]
	fps := 15
	if len(l) > 0 {
		fps, _ = strconv.Atoi(l[0])
	}

	const boundaryStr = "frameboundary"
	w.Header().Add("Content-Type", fmt.Sprintf("multipart/x-mixed-replace;boundary=%s", boundaryStr))
	boundary := []byte("--" + boundaryStr + "\r\n")

	crlf := []byte("\r\n")

	var err error
	var hdrs string
	frameDelay := 1000 / fps
	for {
		if _, err = w.Write(boundary); err != nil {
			return
		}

		frameBufferMutex.RLock()
		buf := make([]byte, len(frameBuffer))
		copy(buf, frameBuffer)
		frameBufferMutex.RUnlock()

		hdrs = fmt.Sprintf("Content-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n", len(buf))
		if _, err = w.Write([]byte(hdrs)); err != nil {
			return
		}

		if _, err = w.Write(buf); err != nil {
			return
		}

		if _, err = w.Write(crlf); err != nil {
			return
		}

		time.Sleep(time.Duration(frameDelay)*time.Millisecond)
	}
}
