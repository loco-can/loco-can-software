package main

import (
	"bytes"
	"context"
	"crypto/rand"
	"embed"
	"encoding/hex"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"html/template"
	"io"
	"net"
	"net/http"
	"os"
	"strings"
	"sync"
	"syscall"
	"time"
)

//go:embed gui.html
var guiFS embed.FS

type firmwareSource interface {
	Releases(ctx context.Context, tag string, stableOnly bool) ([]Release, error)
	Download(ctx context.Context, img Image, dest string) error
}

type app struct {
	source firmwareSource
	repo   string
	token  string
	flash    func(ctx context.Context, port string, data []byte, bauds []int, log flashLogger) error
	ports    func() ([]Port, error)
	identify func(ctx context.Context, port string) (ModuleInfo, error)
	job      *jobState
	quit   func()
	ctx    context.Context
	page   *template.Template
}

func newApp(ctx context.Context, source firmwareSource, repo, token string) (*app, error) {
	page, err := template.ParseFS(guiFS, "gui.html")
	if err != nil {
		return nil, err
	}
	if token == "" {
		token = newToken()
	}
	return &app{
		source: source,
		repo:   repo,
		token:  token,
		flash:    programHex,
		ports:    listPorts,
		identify: probeModule,
		job:      &jobState{},
		ctx:    ctx,
		page:   page,
	}, nil
}

func newToken() string {
	buf := make([]byte, 16)
	if _, err := rand.Read(buf); err != nil {
		return fmt.Sprintf("%d", time.Now().UnixNano())
	}
	return hex.EncodeToString(buf)
}

func (a *app) handler() http.Handler {
	mux := http.NewServeMux()
	mux.HandleFunc("GET /{$}", a.servePage)
	mux.HandleFunc("GET /api/ports", a.servePorts)
	mux.HandleFunc("GET /api/probe", a.serveProbe)
	mux.HandleFunc("GET /api/releases", a.serveReleases)
	mux.HandleFunc("POST /api/flash", a.serveFlash)
	mux.HandleFunc("GET /api/job", a.serveJob)
	mux.HandleFunc("POST /api/quit", a.serveQuit)
	return mux
}

func (a *app) allow(w http.ResponseWriter, r *http.Request) bool {
	if r.Header.Get("X-Loco-Token") != a.token {
		writeJSON(w, http.StatusUnauthorized, map[string]string{"error": "fehlendes Zugriffstoken"})
		return false
	}
	return true
}

func (a *app) servePage(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	w.Header().Set("Cache-Control", "no-store")
	if err := a.page.Execute(w, struct{ Token string }{Token: a.token}); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}

func (a *app) servePorts(w http.ResponseWriter, r *http.Request) {
	if !a.allow(w, r) {
		return
	}
	ports, err := a.ports()
	if err != nil {
		writeJSON(w, http.StatusInternalServerError, map[string]string{"error": err.Error()})
		return
	}
	type row struct {
		Name string `json:"name"`
		USB  string `json:"usb,omitempty"`
		Desc string `json:"desc,omitempty"`
	}
	out := make([]row, 0, len(ports))
	for _, p := range ports {
		out = append(out, row{Name: p.Name, USB: p.USB, Desc: p.Desc})
	}
	writeJSON(w, http.StatusOK, map[string]any{"ports": out})
}

func (a *app) serveProbe(w http.ResponseWriter, r *http.Request) {
	if !a.allow(w, r) {
		return
	}
	port := strings.TrimSpace(r.URL.Query().Get("port"))
	if port == "" {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": "Adapter wählen"})
		return
	}
	if a.identify == nil {
		writeJSON(w, http.StatusInternalServerError, map[string]string{"error": "keine Abfrage"})
		return
	}
	info, err := a.identify(r.Context(), port)
	if err != nil {
		writeJSON(w, http.StatusOK, map[string]any{"ok": false, "error": err.Error()})
		return
	}
	writeJSON(w, http.StatusOK, map[string]any{"ok": true, "module": info.Module, "version": info.Version, "type_id": info.TypeID, "can": info.CAN, "raw": info.Raw})
}

func (a *app) serveReleases(w http.ResponseWriter, r *http.Request) {
	if !a.allow(w, r) {
		return
	}
	stable := r.URL.Query().Get("stable") == "1"
	releases, err := a.source.Releases(r.Context(), "", stable)
	if err != nil {
		writeJSON(w, http.StatusBadGateway, map[string]string{"error": err.Error()})
		return
	}
	type imageRow struct {
		Module   string `json:"module"`
		Version  string `json:"version"`
		File     string `json:"file"`
		SHA256   string `json:"sha256,omitempty"`
		Protocol string `json:"protocol,omitempty"`
	}
	type releaseRow struct {
		Tag        string     `json:"tag"`
		Name       string     `json:"name"`
		Prerelease bool       `json:"prerelease"`
		Date       string     `json:"date"`
		Images     []imageRow `json:"images"`
	}
	out := make([]releaseRow, 0, len(releases))
	for _, rel := range releases {
		name := rel.Name
		if name == "" {
			name = rel.Tag
		}
		row := releaseRow{
			Tag:        rel.Tag,
			Name:       name,
			Prerelease: rel.Prerelease,
			Date:       rel.PublishedAt.Format("2006-01-02"),
			Images:     []imageRow{},
		}
		for _, img := range rel.Images {
			row.Images = append(row.Images, imageRow{
				Module:   img.Module,
				Version:  img.Version,
				File:     img.File,
				SHA256:   img.SHA256,
				Protocol: img.Protocol,
			})
		}
		out = append(out, row)
	}
	writeJSON(w, http.StatusOK, map[string]any{"releases": out})
}

type flashRequest struct {
	Tag  string `json:"tag"`
	File string `json:"file"`
	Port string `json:"port"`
	All  bool   `json:"all"`
}

func (a *app) serveFlash(w http.ResponseWriter, r *http.Request) {
	if !a.allow(w, r) {
		return
	}
	var req flashRequest
	var local []byte
	var localName string
	if strings.HasPrefix(r.Header.Get("Content-Type"), "multipart/form-data") {
		if err := r.ParseMultipartForm(2 << 20); err != nil {
			writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
			return
		}
		f, hdr, err := r.FormFile("hex")
		if err != nil {
			writeJSON(w, http.StatusBadRequest, map[string]string{"error": "HEX-Datei fehlt"})
			return
		}
		defer f.Close()
		local, err = io.ReadAll(io.LimitReader(f, 2<<20+1))
		if err != nil {
			writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
			return
		}
		if len(local) > 2<<20 {
			writeJSON(w, http.StatusBadRequest, map[string]string{"error": "HEX-Datei ist zu groß"})
			return
		}
		localName = hdr.Filename
		req.Port = r.FormValue("port")
		req.All = r.FormValue("all") == "1" || r.FormValue("all") == "true"
	} else {
		if err := json.NewDecoder(io.LimitReader(r.Body, 1<<20)).Decode(&req); err != nil {
			writeJSON(w, http.StatusBadRequest, map[string]string{"error": "ungültige Anfrage"})
			return
		}
	}
	ports, err := a.flashPorts(req)
	if err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	if local != nil {
		if err := a.start(func(ctx context.Context) error {
			return a.writeLocal(ctx, ports, local, localName)
		}); err != nil {
			writeJSON(w, http.StatusConflict, map[string]string{"error": err.Error()})
			return
		}
	} else {
		if req.Tag == "" || req.File == "" {
			writeJSON(w, http.StatusBadRequest, map[string]string{"error": "Release und Modul wählen"})
			return
		}
		releases, err := a.source.Releases(r.Context(), req.Tag, false)
		if err != nil {
			writeJSON(w, http.StatusBadGateway, map[string]string{"error": err.Error()})
			return
		}
		_, img, err := findReleaseImage(releases, req.Tag, req.File)
		if err != nil {
			writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
			return
		}
		if err := img.flashable(); err != nil {
			writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
			return
		}
		if err := a.start(func(ctx context.Context) error {
			return a.writeRelease(ctx, ports, img)
		}); err != nil {
			writeJSON(w, http.StatusConflict, map[string]string{"error": err.Error()})
			return
		}
	}
	writeJSON(w, http.StatusAccepted, map[string]bool{"started": true})
}

func (a *app) flashPorts(req flashRequest) ([]Port, error) {
	if req.All {
		ports, err := a.ports()
		if err != nil {
			return nil, err
		}
		if len(ports) == 0 {
			return nil, fmt.Errorf("kein FTDI-Adapter gefunden")
		}
		return ports, nil
	}
	if strings.TrimSpace(req.Port) == "" {
		return nil, fmt.Errorf("Adapter wählen oder alle FTDI-Adapter einschalten")
	}
	return []Port{{Name: req.Port, Desc: "serial"}}, nil
}

func (a *app) start(work func(context.Context) error) error {
	if !a.job.begin() {
		return fmt.Errorf("ein Schreibvorgang läuft schon")
	}
	go func() {
		err := work(a.ctx)
		if err != nil {
			if errors.Is(err, context.Canceled) {
				err = fmt.Errorf("abgebrochen")
			}
			a.job.finish(err)
			return
		}
		a.job.finish(nil)
	}()
	return nil
}

func (a *app) writeRelease(ctx context.Context, ports []Port, img Image) error {
	a.job.add(fmt.Sprintf("Lade %s", img.File), 0, 0)
	dest, err := cachePath(a.repo, "gui", img.File)
	if err != nil {
		return err
	}
	// Keep the release tag in the cache path when the URL is known.
	if tag := releaseTagFromURL(img.URL); tag != "" {
		dest, err = cachePath(a.repo, tag, img.File)
		if err != nil {
			return err
		}
	}
	if err := a.source.Download(ctx, img, dest); err != nil {
		return err
	}
	data, err := os.ReadFile(dest)
	if err != nil {
		return err
	}
	a.job.add(fmt.Sprintf("Release  %s %s  %s", img.Module, img.Version, img.File), 0, 0)
	return a.writePorts(ctx, ports, data)
}

func releaseTagFromURL(raw string) string {
	const marker = "/releases/download/"
	i := strings.Index(raw, marker)
	if i < 0 {
		return ""
	}
	rest := raw[i+len(marker):]
	tag, _, ok := strings.Cut(rest, "/")
	if !ok || tag == "" {
		return ""
	}
	return tag
}

func (a *app) writeLocal(ctx context.Context, ports []Port, data []byte, name string) error {
	if img, ok := parseFirmwareName(name); ok {
		a.job.add(fmt.Sprintf("Lokale Datei  %s %s  %s", img.Module, img.Version, img.File), 0, 0)
	} else {
		a.job.add("Lokale Datei  "+name, 0, 0)
	}
	return a.writePorts(ctx, ports, data)
}

func (a *app) writePorts(ctx context.Context, ports []Port, data []byte) error {
	var failed []string
	for i, port := range ports {
		if err := ctx.Err(); err != nil {
			return err
		}
		a.job.add(fmt.Sprintf("Schreibe %s (%d von %d)", port.Name, i+1, len(ports)), 0, 0)
		err := a.flash(ctx, port.Name, data, []int{115200, 57600}, a.job.add)
		if err != nil {
			a.job.add(port.Name+": "+err.Error(), 0, 0)
			failed = append(failed, port.Name)
		}
	}
	if len(failed) > 0 {
		return fmt.Errorf("fehlgeschlagen: %s", strings.Join(failed, ", "))
	}
	return nil
}

func (a *app) serveJob(w http.ResponseWriter, r *http.Request) {
	if !a.allow(w, r) {
		return
	}
	writeJSON(w, http.StatusOK, a.job.snapshot())
}

func (a *app) serveQuit(w http.ResponseWriter, r *http.Request) {
	if !a.allow(w, r) {
		return
	}
	writeJSON(w, http.StatusOK, map[string]bool{"ok": true})
	if a.quit != nil {
		go a.quit()
	}
}

func findReleaseImage(releases []Release, tag, file string) (Release, Image, error) {
	for _, rel := range releases {
		if rel.Tag != tag {
			continue
		}
		for _, img := range rel.Images {
			if img.File == file {
				return rel, img, nil
			}
		}
	}
	return Release{}, Image{}, fmt.Errorf("Datei %s ist nicht im Release %s", file, tag)
}

func writeJSON(w http.ResponseWriter, status int, body any) {
	var buf bytes.Buffer
	enc := json.NewEncoder(&buf)
	enc.SetEscapeHTML(false)
	if err := enc.Encode(body); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", "application/json; charset=utf-8")
	w.Header().Set("Cache-Control", "no-store")
	w.WriteHeader(status)
	_, _ = w.Write(buf.Bytes())
}

type jobState struct {
	mu      sync.Mutex
	running bool
	lines   []string
	done    int
	total   int
	errText string
	ok      bool
}

func (j *jobState) begin() bool {
	j.mu.Lock()
	defer j.mu.Unlock()
	if j.running {
		return false
	}
	j.running = true
	j.lines = nil
	j.done = 0
	j.total = 0
	j.errText = ""
	j.ok = false
	return true
}

func (j *jobState) add(msg string, done, total int) {
	j.mu.Lock()
	defer j.mu.Unlock()
	if msg != "" {
		j.lines = append(j.lines, msg)
		if len(j.lines) > 400 {
			j.lines = j.lines[len(j.lines)-400:]
		}
	}
	if total > 0 {
		j.done = done
		j.total = total
	}
}

func (j *jobState) finish(err error) {
	j.mu.Lock()
	defer j.mu.Unlock()
	j.running = false
	if err != nil {
		j.errText = err.Error()
		j.ok = false
		return
	}
	j.ok = true
}

type jobView struct {
	Running bool     `json:"running"`
	Lines   []string `json:"lines"`
	Done    int      `json:"done"`
	Total   int      `json:"total"`
	Error   string   `json:"error,omitempty"`
	OK      bool     `json:"ok"`
}

func (j *jobState) snapshot() jobView {
	j.mu.Lock()
	defer j.mu.Unlock()
	lines := append([]string(nil), j.lines...)
	if lines == nil {
		lines = []string{}
	}
	return jobView{
		Running: j.running,
		Lines:   lines,
		Done:    j.done,
		Total:   j.total,
		Error:   j.errText,
		OK:      j.ok,
	}
}

func cmdGUI(ctx context.Context, args []string) error {
	fs := flag.NewFlagSet("gui", flag.ContinueOnError)
	fs.SetOutput(io.Discard)
	listen := fs.String("listen", "127.0.0.1:47221", "local address for the workshop window")
	noBrowser := fs.Bool("no-browser", false, "do not open a browser")
	var rel releaseFlags
	rel.add(fs)
	if args == nil {
		args = []string{}
	}
	if err := fs.Parse(args); err != nil {
		fmt.Fprint(os.Stderr, usageText)
		return err
	}
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()
	application, err := newApp(ctx, rel.client(), rel.repo, "")
	if err != nil {
		return err
	}
	application.quit = cancel

	ln, err := net.Listen("tcp", *listen)
	if err != nil {
		if errors.Is(err, syscall.EADDRINUSE) {
			if !*noBrowser {
				_ = openBrowser("http://" + *listen + "/")
			}
			fmt.Fprintf(os.Stderr, "Loco-CAN läuft bereits auf http://%s/ — bestehendes Fenster geöffnet. Zum Aktualisieren zuerst Beenden, dann neu starten.\n", *listen)
			return nil
		}
		return err
	}
	defer ln.Close()

	srv := &http.Server{Handler: application.handler()}
	go func() {
		<-ctx.Done()
		shut, stop := context.WithTimeout(context.Background(), 2*time.Second)
		defer stop()
		_ = srv.Shutdown(shut)
	}()

	url := "http://" + ln.Addr().String() + "/"
	fmt.Fprintf(os.Stderr, "Loco-CAN  %s\n", url)
	if !*noBrowser {
		if err := openBrowser(url); err != nil {
			fmt.Fprintf(os.Stderr, "%s\n%s\n", err, url)
			notify("Loco-CAN", "Browser konnte nicht geöffnet werden.\n"+url)
		}
	}
	err = srv.Serve(ln)
	if errors.Is(err, http.ErrServerClosed) {
		return nil
	}
	return err
}
